#include "CtpCntIf.h"
#include "RealApp.h"
#include "ManualTrade.h"
#include "KeyValue.h"
#include "BrokerServer.h"

//++
//构造函数
//--
CCtpCntIf::CCtpCntIf(CRealApp *pRealApp, string CounterName)
{
	m_pRealApp = pRealApp;
	m_pFlowControl = new CMyFlowControl(this, 1.0);
	m_CounterName = CounterName;

	m_requestId = 0;
	m_bNeedAuth = false;     
	m_UserProductInfo = ""; 
	m_AppID = "";
	m_AuthCode = "";     

#ifdef WIN_CTP
	//信号初始化，手动复位，初始无信号状态
    m_hTraderSpiEvent = CreateEvent(NULL, true, false, NULL);
	m_hMdSpiEvent = CreateEvent(NULL, true, false, NULL);
	m_hInstrumentStatusEvent = CreateEvent(NULL, true, false, NULL);
#else
    m_hTraderSpiEvent = new CMyEvent;
	m_hMdSpiEvent = new CMyEvent; 
	m_hInstrumentStatusEvent = new CMyEvent; 
#endif

	//初始化互斥区
	InitializeCriticalSection(&m_DepthMarketDataCS);
	InitializeCriticalSection(&m_InstrumentStatusCS);
	InitializeCriticalSection(&m_InvestorPositionCS);
	InitializeCriticalSection(&m_TradingAccountCS);
	InitializeCriticalSection(&m_OrderListCS);
	InitializeCriticalSection(&m_TradeListCS);

	//没有初始化之前各个指针都强行置空
	m_pMdUserApi = NULL;
	m_pMdUserSpi = NULL; 
	m_pTraderUserApi = NULL;
	m_pTraderUserSpi = NULL;

	m_MaxMarginRatio = 0.35;

	//初始化时间戳
	m_bReqAccountSuccess = false;
	m_bReqPositionSuccess = false;
	memset(&m_ReqAccountTimestamp, 0, sizeof(SYSTEMTIME));
	memset(&m_ReqPositionTimestamp, 0, sizeof(SYSTEMTIME));

	m_OrderAckedPos = 0;
	m_TradeAckedPos = 0;
	m_TotalEquity = 0;

	//初始化更新资金情况和持仓情况所用到的临时成员变量
	memset(&m_prevTime, 0, sizeof(SYSTEMTIME));
	m_PrePositionSeconds = 0;  
	m_PreAccountSeconds = 0;  
	m_PreAccountSeqNo = 0;
	m_CurAccountSeqNo = 0;

	m_PreOrderSavePos = 0;
	m_PreTradeSavePos = 0;

	//交易柜台报单和撤单检查相关参数
	m_bCheckPlaceAndCancelOrder = false;
	m_CheckRetryTimes = 0;
}

//++
//析构函数
//--
CCtpCntIf::~CCtpCntIf()
{
	//删除互斥区
	DeleteCriticalSection(&m_DepthMarketDataCS);
	DeleteCriticalSection(&m_InstrumentStatusCS);
	DeleteCriticalSection(&m_InvestorPositionCS);
	DeleteCriticalSection(&m_TradingAccountCS);
	DeleteCriticalSection(&m_OrderListCS);
	DeleteCriticalSection(&m_TradeListCS);

#ifndef WIN_CTP
    if(m_hTraderSpiEvent != NULL) delete m_hTraderSpiEvent;
	if(m_hMdSpiEvent != NULL) delete m_hMdSpiEvent;
	if(m_hInstrumentStatusEvent != NULL) delete m_hInstrumentStatusEvent;
#endif

	CleanupCtpApi();
}

//++
//从数据库中获取配置信息
//--
void CCtpCntIf::GetConfigFromDb()
{
	CKeyValue KeyValue;
	string key, value;
	string strTableName = "Config_" + m_CounterName;
	cerr << m_pRealApp->m_Prompt << "获取" << m_CounterName << "配置数据表信息" << ENDLINE;
	cerr << "--->>>" << ENDLINE;

	int RetRead = KeyValue.ReadConfigFromDb(m_pRealApp->m_pTradeDb, strTableName);

#ifdef WIN_CTP
	string strConfigDir = ".\\config";
	if(m_pRealApp->m_ArchName != "") strConfigDir = ".\\config\\" + m_pRealApp->m_ArchName;
	string strFilePath = strConfigDir + "\\" + strTableName + ".txt";
	if(RetRead <= 0) KeyValue.ReadConfigFromTextFile(strFilePath);
#else
	string strConfigDir = "./config";
	if(m_pRealApp->m_ArchName != "") strConfigDir = "./config/" + m_pRealApp->m_ArchName;
	string strFilePath = strConfigDir + "/" + strTableName + ".txt";
	if(RetRead <= 0) KeyValue.ReadConfigFromTextFile(strFilePath);
#endif
	 
	//显示经纪商信息
    DisplayBrokerInfo(m_CounterName, m_pRealApp->m_ArchName, m_pRealApp->m_pTradeDb);

	value = KeyValue.InputStringKeyValue("CntCode");
	m_CntCode = value[0];
	value = KeyValue.InputStringKeyValue("BrokerId");
	strcpy(m_brokerId, value.c_str());
	value = KeyValue.InputStringKeyValue("UserId");
	strcpy(m_userId, value.c_str());
	cerr << m_pRealApp->m_Prompt << "输入 Password> "; GetPasswd(m_Password);

	//获取用户认证信息
	int NeedAuth =  KeyValue.InputIntKeyValue("NeedAuth");
	m_bNeedAuth = (NeedAuth != 0) ? true : false;
	if(m_bNeedAuth == true)
	{
		m_UserProductInfo = KeyValue.InputStringKeyValue("UserProductInfo");
#ifdef SEE_THROUGH_SUPERVISION
		m_AppID = KeyValue.InputStringKeyValue("AppID");
#endif
		m_AuthCode =  KeyValue.InputStringKeyValue("AuthCode");
	}

	m_MaxMarginRatio = KeyValue.InputFloatKeyValue("MaxMarginRatio");

	//将配置信息写入数据库
	KeyValue.WriteConfigToDb(m_pRealApp->m_pTradeDb, strTableName);
	cerr << "---<<<" << ENDLINE << ENDLINE;
}

//++
//初始化CTP API
//参数
//   StartStyle 
//       THOST_TERT_RESUME: 从上次断线处开始重传
//       THOST_TERT_QUICK: 不做任何重传
//       THOST_TERT_RESTART: 从今天开始
//返回值
//    无
//--
void CCtpCntIf::InitCtpApi(THOST_TE_RESUME_TYPE StartStyle)
{
	string strCmd;

	cerr << m_pRealApp->m_Prompt  << "启动" << m_CounterName << "交易API线程" << ENDLINE;
	//判断TraderApi流文件目录是否存在，不存在则创建
	if(access("traderapiflow", 0) != 0)
	{
		system("mkdir traderapiflow");
	}

#ifdef WIN_CTP
	string strTradeFlowDir = ".\\traderapiflow";
	if(m_pRealApp->m_ArchName != "") strTradeFlowDir = ".\\traderapiflow\\" + m_pRealApp->m_ArchName;
	if(m_pRealApp->m_ArchName != "" && access(strTradeFlowDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strTradeFlowDir;
		system(strCmd.c_str());
	}

	strTradeFlowDir = strTradeFlowDir + "\\" +m_CounterName;
	if(access(strTradeFlowDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strTradeFlowDir;
		system(strCmd.c_str());
	}
	strTradeFlowDir = strTradeFlowDir + "\\";
#else
	string strTradeFlowDir = "./traderapiflow";
	if(m_pRealApp->m_ArchName != "") strTradeFlowDir = "./traderapiflow/" + m_pRealApp->m_ArchName;
	if(m_pRealApp->m_ArchName != "" && access(strTradeFlowDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strTradeFlowDir;
		system(strCmd.c_str());
	}

	strTradeFlowDir = strTradeFlowDir + "/" +m_CounterName;
	if(access(strTradeFlowDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strTradeFlowDir;
		system(strCmd.c_str());
	}
	strTradeFlowDir = strTradeFlowDir + "/";
#endif

	//初始化UserApi
	m_pTraderUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi(strTradeFlowDir.c_str());
	m_pTraderUserSpi = new CRealTraderSpi(m_pTraderUserApi, m_pRealApp, this);
	m_pTraderUserApi->RegisterSpi((CThostFtdcTraderSpi*)m_pTraderUserSpi);	// 注册事件类
	//THOST_TERT_RESTART: 从今天开始
	//THOST_TERT_RESUME: 从上次断线处开始重传
	//THOST_TERT_QUICK: 不做任何重传
	m_pTraderUserApi->SubscribePublicTopic(StartStyle);				// 注册公有流
	m_pTraderUserApi->SubscribePrivateTopic(StartStyle);			    // 注册私有流

	//注册多个交易托管服务器前置地址
	CBrokerServer BrokerServer(m_CounterName, m_pRealApp->m_ArchName, m_pRealApp->m_pTradeDb);
	for(size_t i = 0; i < BrokerServer.m_TraderFrontList.size(); i++)
	{
		char TraderFront[200];
		strcpy(TraderFront, BrokerServer.m_TraderFrontList[i].c_str());
		m_pTraderUserApi->RegisterFront(TraderFront);
	}

	m_pTraderUserApi->Init();
	cerr << m_pRealApp->m_Prompt  << "--->>> " << "Initialing " << m_CounterName << " TraderApi" << ENDLINE;

	cerr << m_pRealApp->m_Prompt  << "启动" << m_CounterName << "行情API线程" << ENDLINE;
	//判断MdApi流文件目录是否存在，不存在则创建
	if(access("mdapiflow", 0) != 0)
	{
		system("mkdir mdapiflow");
	}

#ifdef WIN_CTP
	string strMdFlowDir = ".\\mdapiflow";
	if(m_pRealApp->m_ArchName != "") strMdFlowDir = ".\\mdapiflow\\" + m_pRealApp->m_ArchName;
	if(m_pRealApp->m_ArchName != "" && access(strMdFlowDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strMdFlowDir;
		system(strCmd.c_str());
	}

	strMdFlowDir = strMdFlowDir + "\\" + m_CounterName;
	if(access(strMdFlowDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strMdFlowDir;
		system(strCmd.c_str());
	}
	strMdFlowDir = strMdFlowDir + "\\";
#else
	string strMdFlowDir = "./mdapiflow";
	if(m_pRealApp->m_ArchName != "") strMdFlowDir = "./mdapiflow/" + m_pRealApp->m_ArchName;
	if(m_pRealApp->m_ArchName != "" && access(strMdFlowDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strMdFlowDir;
		system(strCmd.c_str());
	}

	strMdFlowDir = strMdFlowDir + "/" + m_CounterName;
	if(access(strMdFlowDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strMdFlowDir;
		system(strCmd.c_str());
	}
	strMdFlowDir = strMdFlowDir + "/";
#endif

	//初始化UserApi
	m_pMdUserApi=CThostFtdcMdApi::CreateFtdcMdApi(strMdFlowDir.c_str());
	m_pMdUserSpi=new CRealMdSpi(m_pMdUserApi, m_pRealApp, this);      //创建回调处理类对象MdSpi
	m_pMdUserApi->RegisterSpi(m_pMdUserSpi);		   // 回调对象注入接口类

	//注册多个行情托管服务器前置地址
	for(size_t i = 0; i < BrokerServer.m_MdFrontList.size(); i++)
	{
		char MdFront[200];
		strcpy(MdFront, BrokerServer.m_MdFrontList[i].c_str());
		m_pMdUserApi->RegisterFront(MdFront);
	}

	m_pMdUserApi->Init(); 					   //接口线程启动, 开始工作
	cerr << m_pRealApp->m_Prompt << "--->>> " << "Initialing " << m_CounterName << " MdApi" << ENDLINE;

	DWORD waitReturn;
	//等待行情前置登录回应
	waitReturn = WaitForSingleObject(m_hMdSpiEvent, LOGIN_WAIT_TIME);  
	if(waitReturn != WAIT_OBJECT_0) 
	{
		cerr << m_pRealApp->m_Prompt << "超过 " << int(LOGIN_WAIT_TIME/1000) << " 秒没有收到" 
			<< m_CounterName << "行情前置登录回应消息！" << ENDLINE;
	}
	ResetEvent(m_hMdSpiEvent);

	//等待结算单确认
	waitReturn = WaitForSingleObject(m_hTraderSpiEvent, LOGIN_WAIT_TIME);   
	if(waitReturn != WAIT_OBJECT_0) 
	{
		cerr << m_pRealApp->m_Prompt << "超过 " << int(LOGIN_WAIT_TIME/1000) << " 秒没有收到"
			<< m_CounterName << "结算单确认消息！" << ENDLINE;
	}
	ResetEvent(m_hTraderSpiEvent);
}

//++
//清理CTP API对象
//--
void CCtpCntIf::CleanupCtpApi()
{
	//登出交易前置
	if(m_pTraderUserSpi != NULL)
	{
		if(m_pTraderUserSpi->m_bSuccessLogin == true)
		{
			m_pTraderUserSpi->ReqUserLogout(m_brokerId, m_userId);
			m_pTraderUserSpi->m_bSuccessLogin = false;
		}
	}

	SleepMs(1);    //1毫秒足够，否则时间过长前置会重新连接并登陆

	//清理行情对象  
	if(m_pMdUserApi) {
		m_pMdUserApi->RegisterSpi(NULL);
		m_pMdUserApi->Release();
		m_pMdUserApi = NULL;
	}
	if(m_pMdUserSpi) {
		delete m_pMdUserSpi;
		m_pMdUserSpi = NULL;
	}

	//清理交易对象
	if(m_pTraderUserApi) {
		m_pTraderUserApi->RegisterSpi(NULL);
		m_pTraderUserApi->Release();
		m_pTraderUserApi = NULL;
	}
	if(m_pTraderUserSpi) {
		delete m_pTraderUserSpi;
		m_pTraderUserSpi = NULL;
	}

	//清理流量控制对象
	if(m_pFlowControl) {
		delete m_pFlowControl;
		m_pFlowControl = NULL;
	}
}

//++
//计算投资者总权益
//需要在投资者资金查询和持仓查询之后进行, 并且已经有最新市场行情更新
//--
void CCtpCntIf::CalcTotalEquity()
{
	m_TotalEquity = m_TradingAccount.Balance;

	for(size_t i = 0; i < m_InvestorPositionDetailList.size(); i++)
	{
		CThostFtdcInvestorPositionField* pInvestorPosition = &m_InvestorPositionDetailList[i].CtpInvestorPosition;
		char InstrumentType;
		size_t UnderlyingAssetType;
		void* pNode = GetMarketNodePointer(m_pRealApp, pInvestorPosition->InstrumentID, InstrumentType, UnderlyingAssetType);
	    double ValidPrice = 0;

		//如果找不到对应的T型报价市场节点, 跳过此合约
		if(pNode == NULL) continue;

		//只有ETF/股票和期权持仓才计算币值权益, 期货不计算
		if(InstrumentType == 'u' && (UnderlyingAssetType == 2 || UnderlyingAssetType == 3))
		{
			CTshapeMonth* pTshapeMonth = (CTshapeMonth*)pNode;
			CThostFtdcDepthMarketDataField Md = pTshapeMonth->GetLockedMarketData();
			//避免推送行情得到的涨跌停板价格不对
			double UpperLimitPrice = GetUpperLimitPrice(&Md, InstrumentType);
			double LowerLimitPrice = GetLowerLimitPrice(&Md, InstrumentType);
			int VolumeMultiple = pTshapeMonth->m_UnderlyingInstInfo.VolumeMultiple;

			if(Md.LastPrice >= LowerLimitPrice && Md.LastPrice <= UpperLimitPrice)
				ValidPrice = Md.LastPrice;
			else if(Md.ClosePrice >= LowerLimitPrice && Md.ClosePrice <= UpperLimitPrice)
				ValidPrice = Md.ClosePrice;
			else
				ValidPrice = Md.PreClosePrice;

			m_TotalEquity += ValidPrice * pInvestorPosition->Position * VolumeMultiple;
		}
		else if(InstrumentType == 'c' || InstrumentType == 'p')
		{
			COptionItem* pOptionItem = (COptionItem*)pNode;
			CThostFtdcDepthMarketDataField Md = pOptionItem->GetLockedMarketData();
			//避免推送行情得到的涨跌停板价格不对
			double UpperLimitPrice = GetUpperLimitPrice(&Md, InstrumentType);
			double LowerLimitPrice = GetLowerLimitPrice(&Md, InstrumentType);
			int VolumeMultiple = pOptionItem->m_InstInfo.VolumeMultiple;

			if(Md.LastPrice >= LowerLimitPrice && Md.LastPrice <= UpperLimitPrice)
				ValidPrice = Md.LastPrice;
			else if(Md.SettlementPrice >= LowerLimitPrice && Md.SettlementPrice <= UpperLimitPrice)
				ValidPrice = Md.SettlementPrice;
			else
				ValidPrice = Md.PreSettlementPrice;

			if(pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
				m_TotalEquity += ValidPrice * pInvestorPosition->Position * VolumeMultiple;
			else if(pInvestorPosition->PosiDirection == THOST_FTDC_PD_Short)
				m_TotalEquity -= ValidPrice * pInvestorPosition->Position * VolumeMultiple;
		}
	} //for i, 遍历投资者持仓
}

//++
//存储交易柜台账户信息到数据库
//--
void CCtpCntIf::SaveTradingAccountToDb()
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
    string strTableName;
	string strFieldDesc;
	string strSQLCmd;
	SYSTEMTIME CurTime;
	stringstream sstr;
	string strDate;
	double eps = 0.00001;

	GetLocalTime(&CurTime);

	//检查数据表是否存在，不存在则创建
	int CreateSuccess;
	strTableName = m_CounterName + "_TradingAccount";
	//Open, Close等关键字不能用作数据表的字段名，否则会出错
	strFieldDesc =  "(ID INTEGER PRIMARY KEY AUTOINCREMENT, DateOfTrade DATE UNIQUE, TotalEquity DOUBLE, Balance DOUBLE, \
		            Available DOUBLE, CurrMargin DOUBLE, CloseProfit DOUBLE, \
					PositionProfit DOUBLE, Commission DOUBLE, FrozenMargin DOUBLE)";
	CreateSuccess = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
	if(CreateSuccess < 0) 
	{
		cerr << m_pRealApp->m_Prompt << "数据表 " << strTableName << " 无法创建或者无法访问，程序返回!" << ENDLINE;
        PressAnyKeyToExit();
	}

	//生成数据库格式的当前日期
	sstr.clear(); sstr.str("");
	sstr << setw(4) << setfill('0') << CurTime.wYear << "-" << setw(2) << setfill('0') << CurTime.wMonth << "-" 
	     << setw(2) << setfill('0') << CurTime.wDay;
	strDate = sstr.str();

	//根据需要计算m_TotalEquity
	if(m_TotalEquity < eps)
		CalcTotalEquity();

    //生成数据库保存所需的SQL字段信息
	sstr.clear(); sstr.str(""); sstr.precision(18);
	sstr << "'" << strDate << "', " << m_TotalEquity << ", " << m_TradingAccount.Balance << ", " << m_TradingAccount.Available <<  ", "
			<< m_TradingAccount.CurrMargin << ", " << m_TradingAccount.CloseProfit << ", " << m_TradingAccount.PositionProfit << ", "
			<< m_TradingAccount.Commission << ", " << m_TradingAccount.FrozenMargin;

	//先从数据表中删除当日资金账户信息
	strSQLCmd = "DELETE FROM " + strTableName + " WHERE DateOfTrade='" + strDate + "'";
	pTradeDb->ExecuteSqlCommand(strSQLCmd);

	//再插入当前日资金账户记录
	strSQLCmd = "INSERT INTO " + strTableName + "(DateOfTrade, TotalEquity, Balance, Available, CurrMargin, CloseProfit, PositionProfit, \
												Commission, FrozenMargin) VALUES(" + sstr.str() + ")";
	pTradeDb->ExecuteSqlCommand(strSQLCmd);
}
