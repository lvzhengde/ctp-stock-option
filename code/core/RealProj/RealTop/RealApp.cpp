#include "RealApp.h"
#include "ManualTrade.h"
#include "KeyValue.h"
#include "BrokerServer.h"

#define MAX_POSTPROC_SECONDS      (300)     //5分钟

//定义指针供类中的静态函数使用
//只有在单柜台系统中才用
CRealApp* pRealApp;

//++
//构造函数
//--
CRealApp::CRealApp(string strArchName)
{
	string strCmd;

	//初始化交易架构名称
	m_ArchName = RemoveSpace(strArchName);
	//交易架构提示符
	m_Prompt = m_ArchName + "$";

#ifdef WIN_CTP
	string strConfigDir = ".\\config";
	if(m_ArchName != "") strConfigDir = ".\\config\\" + m_ArchName;
	if(m_ArchName != "" && access(strConfigDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strConfigDir;
		system(strCmd.c_str());
	}

	string strOutputDir = ".\\output";
	if(m_ArchName != "") strOutputDir = ".\\output\\" + m_ArchName;
	if(m_ArchName != "" && access(strOutputDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strOutputDir;
		system(strCmd.c_str());
	}
#else
	string strConfigDir = "./config";
	if(m_ArchName != "") strConfigDir = "./config/" + m_ArchName;
	if(m_ArchName != "" && access(strConfigDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strConfigDir;
		system(strCmd.c_str());
	}

	string strOutputDir = "./output";
	if(m_ArchName != "") strOutputDir = "./output/" + m_ArchName;
	if(m_ArchName != "" && access(strOutputDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strOutputDir;
		system(strCmd.c_str());
	}
#endif

	m_MaxOrderNumPerSecond = 20;
	m_BigGapScaleFactor = 100;
	m_bManualTrade = false;
	m_bInitDone = false;
	m_bMaintainDb = false;
	m_bAutoTradeErr = false;

	m_bExitApp = false;
	m_bCheckPlaceAndCancelOrder = false;
	m_bTradeEngineError = false;
	m_bForceCancelOrder = false;

#ifdef WIN_CTP
    m_hMdThread = NULL;
	m_hDataThread = NULL;
	m_hTradeThread = NULL;
	m_hMdToDataProc = CreateEvent(NULL, true, false, NULL);
	m_hMdToTradeEngine = CreateEvent(NULL, true, false, NULL);
	m_hMdToStrategyScan = CreateEvent(NULL, true, false, NULL);
#else
    m_hMdThread = new CMyEvent;
	m_hDataThread = new CMyEvent;
	m_hTradeThread = new CMyEvent;
	m_hMdToDataProc = new CMyEvent;
	m_hMdToTradeEngine = new CMyEvent;
	m_hMdToStrategyScan = new CMyEvent;
#endif

	//供类中的静态函数使用
	pRealApp = this;

	//没有初始化之前各个指针都强行置空
	m_pTradeDb = NULL;
	m_pStrategysManage = NULL;
	m_pMarketDataManage = NULL;
	m_pPosMoneyManage = NULL;

	//交易控制参数的初始化
	m_RiskFreeRate = 3.0;
	m_TradeStartWindSec = 0.0; //开盘时有较多交易机会，此值设为0
	m_TradeStopWindSec = 120.0;

	m_TradeUnitMultiple = 1;
	m_MarginMultiple = 2.0;
	m_ManTradePriceTickMultiple = 10;
	m_OrderRoundTripTime = 2.0;
	m_TradeAckTimeout = 60.0;
    m_YieldMultiple = 3.0;
	m_MaxOpensToday = 2;
	m_bReQryRateParams = false;
	m_MaxOpenQueueSize = 2;
	m_MaxCloseQueueSize = 2;
	m_MaxSameFishOrderNum = 1;
	m_LimitDaysBeforeExpire = 1;
	m_VolumeScale = 1;

	//初始化后处理程序相关变量
	memset(&m_PreTime, 0, sizeof(SYSTEMTIME));
	for(int i = 0; i < 20; i++)
	{
		m_bProcessedSeg[i] = false;
	}
}

//++
//析构函数
//--
CRealApp::~CRealApp()
{
	DestroyObjects();

	//销毁柜台对象需要单独分开
	for(size_t i = 0; i < m_pCtpCntIfList.size(); i++)
	{
		if(m_pCtpCntIfList[i] != NULL)
		{
			delete m_pCtpCntIfList[i];
			m_pCtpCntIfList[i] = NULL;
		}
	}

	//删除Linux事件对象
#ifndef WIN_CTP
    if(m_hMdThread != NULL) delete m_hMdThread;
	if(m_hDataThread != NULL) delete m_hDataThread;
	if(m_hTradeThread != NULL) delete m_hTradeThread;
	if(m_hMdToDataProc != NULL) delete m_hMdToDataProc;
	if(m_hMdToTradeEngine != NULL) delete m_hMdToTradeEngine;
	if(m_hMdToStrategyScan != NULL) delete m_hMdToStrategyScan;
#endif
}

//++
//初始化对象指针
//--
void CRealApp::CreateObjects()
{
	if(m_pTradeDb == NULL)
	{
#ifdef WIN_CTP
		string strDirPath = "..\\TradeDatabase";
#else
		string strDirPath = "../TradeDatabase";
#endif
		string strDbName = "TradeDb.db";
		if(m_ArchName != "") strDbName = "TradeDb-" + m_ArchName + ".db";

		m_pTradeDb = new CTradeDb(strDirPath, strDbName);
	}

	if(m_pStrategysManage == NULL)
	{
		m_pStrategysManage = new CStrategysManage(this);
	}

	if(m_pMarketDataManage == NULL)
	{
		m_pMarketDataManage = new CMarketDataManage(this);
	}

	if(m_pPosMoneyManage == NULL)
	{
		m_pPosMoneyManage = new CPosMoneyManage(this);
	}

	//调试输出文件
#ifdef DEBUG_LOG_FILE
	m_DebugLogFile.open(DEBUG_LOG_FILE,  ios::trunc); 
	if(!m_DebugLogFile.is_open())
		cerr << m_Prompt << "不能打开调试日志文件DebugLog.txt，可能output目录不存在！" << ENDLINE;
	else
		m_DebugLogFile << fixed << setprecision(6) << ENDLINE;
#endif

}

//++
//销毁对象
//--
void CRealApp::DestroyObjects()
{
	if(m_pTradeDb != NULL)
	{
		m_pTradeDb->Destroy();
		delete m_pTradeDb;
		m_pTradeDb = NULL;
	}

	if(m_pStrategysManage != NULL)
	{
		m_pStrategysManage->Cleanup();
		delete m_pStrategysManage;
		m_pStrategysManage = NULL;
	}

	if(m_pMarketDataManage != NULL)
	{
		m_pMarketDataManage->Cleanup();
		delete m_pMarketDataManage;
		m_pMarketDataManage = NULL;
	}

	if(m_pPosMoneyManage != NULL)
	{
		m_pPosMoneyManage->Cleanup();
		delete m_pPosMoneyManage ;
		m_pPosMoneyManage = NULL;
	}

	//调试输出文件
#ifdef DEBUG_LOG_FILE
	if(m_DebugLogFile.is_open() == true) m_DebugLogFile.close();
#endif

}

//++
//RealApp顶层的初始化
//包括：生成各种对象，从GlobalConfig中读取配置信息，策略配置，CTP API初始化，构造T型报价，查询持仓和资金，
//策略交易前的检查，订阅行情等
//以上必须按顺序执行，不然会出错
//--
void CRealApp::InitializeApp()
{
	m_bManualTrade = false;
	m_bInitDone = false;

	//生成各种对象
	CreateObjects();

	//初始化数据库对象
	m_pTradeDb->Init();

	//获取全局配置信息，生成策略对象
	GetGlobalConfigFromDb();

	//初始化策略管理对象
	m_pStrategysManage->Initialize();

	//初始化CTP API对象
	for(size_t i = 0; i < m_pCtpCntIfList.size(); i++)
	{
		m_pCtpCntIfList[i]->InitCtpApi(THOST_TERT_RESTART);
	}

	//初始化行情管理对象
	m_pMarketDataManage->Initialize();

	//初始化持仓和资金管理对象
	m_pPosMoneyManage->Initialize();

	//交易前的检查准备工作
	//图形界面程序不进行自动交易, 取消交易前检查
#if 0
	PreTradeCheck();
#endif

	//初始化各个策略的统计对象
	m_pStrategysManage->InitializeStat();

	//初始化结束标志
	m_bInitDone = true;
}

//++
//从数据库中获取全局配置信息
//--
void CRealApp::GetGlobalConfigFromDb()
{
	CKeyValue GlobalKeyValue;
	string key, value;
	string strCount;
	char tmpString[10];
	size_t CounterSize;
	size_t StrategySize;

	cerr << m_Prompt << "获取全局配置数据表信息" << ENDLINE;
	int RetRead = GlobalKeyValue.ReadConfigFromDb(m_pTradeDb, "GlobalConfig");

#ifdef WIN_CTP
	string strConfigDir = ".\\config";
	if(m_ArchName != "") strConfigDir = ".\\config\\" + m_ArchName;
	string strFilePath = strConfigDir + "\\GlobalConfig.txt";
#else
	string strConfigDir = "./config";
	if(m_ArchName != "") strConfigDir = "./config/" + m_ArchName;
	string strFilePath = strConfigDir + "/GlobalConfig.txt";
#endif
	if(RetRead <= 0) GlobalKeyValue.ReadConfigFromTextFile(strFilePath);

	//获取交易柜台信息
	CounterSize = GlobalKeyValue.InputIntKeyValue("CounterSize");
	for(size_t i = 0; i < CounterSize; i++)
	{
		string CounterName;

		sprintf(tmpString, "%d", i);
		strCount = tmpString;
		strCount = RemoveSpace(strCount);
		key = "CounterName" + strCount;
		CounterName = GlobalKeyValue.InputStringKeyValue(key);

		CCtpCntIf* pCtpCntIf = new CCtpCntIf(this, CounterName);
		pCtpCntIf->GetConfigFromDb();
		m_pCtpCntIfList.push_back(pCtpCntIf);
	}

	//获取交易所规则过滤器配置信息
	GetExchangeRuleFilterConfigFromDb();

	//获取其它全局配置信息
	m_MaxOrderNumPerSecond = GlobalKeyValue.InputIntKeyValue("MaxOrderNumPerSecond");
#ifndef FUTURE_OPTION_ONLY
	m_BigGapScaleFactor = GlobalKeyValue.InputFloatKeyValue("BigGapScaleFactor");
#endif
	m_RiskFreeRate = GlobalKeyValue.InputFloatKeyValue("RiskFreeRate");
	m_TradeStartWindSec = GlobalKeyValue.InputFloatKeyValue("TradeStartWindSec");
	m_TradeStopWindSec = GlobalKeyValue.InputFloatKeyValue("TradeStopWindSec");
	m_TradeUnitMultiple = GlobalKeyValue.InputIntKeyValue("TradeUnitMultiple"); 
	m_MarginMultiple = GlobalKeyValue.InputFloatKeyValue("MarginMultiple"); 
	m_OrderRoundTripTime = GlobalKeyValue.InputIntKeyValue("OrderRoundTripTime");
	m_TradeAckTimeout = GlobalKeyValue.InputFloatKeyValue("TradeAckTimeout");
    m_YieldMultiple = GlobalKeyValue.InputFloatKeyValue("YieldMultiple"); 
	m_MaxOpensToday = GlobalKeyValue.InputIntKeyValue("MaxOpensToday");

	int ReQryRateParams;
	ReQryRateParams = GlobalKeyValue.InputIntKeyValue("ReQryRateParams");
	m_bReQryRateParams = (ReQryRateParams == 0) ? false : true;

	m_MaxOpenQueueSize = GlobalKeyValue.InputIntKeyValue("MaxOpenQueueSize");
	m_MaxCloseQueueSize = GlobalKeyValue.InputIntKeyValue("MaxCloseQueueSize");
	m_MaxSameFishOrderNum = GlobalKeyValue.InputIntKeyValue("MaxSameFishOrderNum"); 
	m_LimitDaysBeforeExpire = GlobalKeyValue.InputIntKeyValue("LimitDaysBeforeExpire"); 
	m_VolumeScale = GlobalKeyValue.InputIntKeyValue("VolumeScale"); 

	//获取策略设置，生成策略对象
	StrategySize = GlobalKeyValue.InputIntKeyValue("StrategySize");
	for(size_t i = 0; i < StrategySize; i++)
	{
		string StrategyName;

		sprintf(tmpString, "%d", i);
		strCount = tmpString;
		strCount = RemoveSpace(strCount);
		key = "StrategyName" + strCount;
		StrategyName = GlobalKeyValue.InputStringKeyValue(key);

		//不支持同一策略多次实例化
		bool found = false;
		for(size_t j = 0; j < m_pStrategysManage->m_pStrategyBaseList.size(); j++)
		{
			if(StrategyName == m_pStrategysManage->m_pStrategyBaseList[j]->m_StrategyName)
			{
				found = true;
				break;
			}
		}

		if(StrategyName == "VoidTest" && found == false)      //空策略
		{
			CStrategyBase* pStrategyBase;
			pStrategyBase = new CStrategyVoidTest(this);
			m_pStrategysManage->m_pStrategyBaseList.push_back(pStrategyBase);
		}
        else
		{
	        cerr << "不存在的交易策略!" << ENDLINE;
            PressAnyKeyToExit();
		    exit(-1);
		}
	}

	//获取存储标的K线数据入数据库的时间节点，即m_PostProcTimeList
	m_PostProcTimeList.clear();
	size_t PostProcTimeSize = GlobalKeyValue.InputIntKeyValue("PostProcTimeSize");
	for(size_t j = 0; j < PostProcTimeSize; j++)
	{
		double PostProcTime;

		sprintf(tmpString, "%d", j);
		strCount = tmpString;
		strCount = RemoveSpace(strCount);

		key = "PostProcTime" + strCount;
		PostProcTime = GlobalKeyValue.InputFloatKeyValue(key);

		m_PostProcTimeList.push_back(PostProcTime);
	}

	//将配置信息备份回数据库
	GlobalKeyValue.WriteConfigToDb(m_pTradeDb, "GlobalConfig");
}

//++
//从数据库中获取交易所规则限制过滤器配置信息
//参数
//    无
//返回值
//    无
//--
void CRealApp::GetExchangeRuleFilterConfigFromDb()
{
	CKeyValue FilterKeyValue;
	string key, value;
	string strCount;
	char tmpString[10];
	size_t FilterSize;

	cerr << m_Prompt << "获取交易所规则限制过滤器配置数据表信息" << ENDLINE;
	int RetRead = FilterKeyValue.ReadConfigFromDb(m_pTradeDb, "ExchangeRuleFilterConfig");

#ifdef WIN_CTP
	string strConfigDir = ".\\config";
	if(m_ArchName != "") strConfigDir = ".\\config\\" + m_ArchName;
	string strFilePath = strConfigDir + "\\ExchangeRuleFilterConfig.txt";
#else
	string strConfigDir = "./config";
	if(m_ArchName != "") strConfigDir = "./config/" + m_ArchName;
	string strFilePath = strConfigDir + "/ExchangeRuleFilterConfig.txt";
#endif
	if(RetRead <= 0) FilterKeyValue.ReadConfigFromTextFile(strFilePath);

	//获取过滤器配置信息
	FilterSize = FilterKeyValue.InputIntKeyValue("FilterSize");
	for(size_t i = 0; i < FilterSize; i++)
	{
		//柜台名称
		string CounterName;
		sprintf(tmpString, "%d", i);
		strCount = tmpString;
		strCount = RemoveSpace(strCount);
		key = "CounterName" + strCount;
		CounterName = FilterKeyValue.InputStringKeyValue(key);

		//柜台代码
		char CntCode;
		key = "CntCode" + strCount;
		value = FilterKeyValue.InputStringKeyValue(key);
		CntCode = value[0];

		//交易所代码
		string ExchangeID;
		key = "ExchangeID" + strCount;
		ExchangeID = FilterKeyValue.InputStringKeyValue(key);
		RemoveSpace(ExchangeID);

		//允许的最大撤单数目
		int MaxCancelOrderNum;
		key = "MaxCancelOrderNum" + strCount;
		MaxCancelOrderNum = FilterKeyValue.InputIntKeyValue(key);

		//允许的最大成交量
	    int MaxTradeVolume;
		key = "MaxTradeVolume" + strCount;
		MaxTradeVolume = FilterKeyValue.InputIntKeyValue(key);

		//允许的最大持仓量
		int MaxOpenInterests;
		key = "MaxOpenInterests" + strCount;
		MaxOpenInterests = FilterKeyValue.InputIntKeyValue(key);

		CExchangeRuleFilter* pExchangeRuleFilter = new CExchangeRuleFilter(this);
		pExchangeRuleFilter->m_CounterName = CounterName;
		pExchangeRuleFilter->m_CntCode = CntCode;
		pExchangeRuleFilter->m_ExchangeID = ExchangeID;
		pExchangeRuleFilter->m_MaxCancelOrderNum = MaxCancelOrderNum;
		pExchangeRuleFilter->m_MaxTradeVolume = MaxTradeVolume;
		pExchangeRuleFilter->m_MaxOpenInterests = MaxOpenInterests;
		pExchangeRuleFilter->GetTodayStatusFromDb();
		m_pExchangeRuleFilterList.push_back(pExchangeRuleFilter);
	}

	//将配置信息写回数据库
	FilterKeyValue.WriteConfigToDb(m_pTradeDb, "ExchangeRuleFilterConfig");
	cerr << "---<<<" << ENDLINE << ENDLINE;
}

//++
//交易前的准备检查工作
//主要是检查从数据库中查到的策略组合持仓和投资者持仓查询得到的持仓明细是否一致
//另外也寻找各个头寸所对应的市场行情节点
//--
void CRealApp::PreTradeCheck()
{
	int ClaimResult;

	//首先检查各个策略的组合持仓是否能被满足
	ClaimResult = m_pStrategysManage->ClaimPosition();
	if(ClaimResult < 0)
	{
		cerr << m_Prompt << "数据库中查到的策略组合持仓要求得不到满足，请人工调整数据库及投资者账户持仓，程序将退出！" << ENDLINE;
		m_pPosMoneyManage->DumpToTextFile();
		m_pStrategysManage->DumpPortfolioToTextFile();
        PressAnyKeyToExit();
		//为避免报错，先要清除主动操作的各种对象
		DestroyObjects();
		for(size_t i = 0; i < m_pCtpCntIfList.size(); i++)
		{
		   m_pCtpCntIfList[i]->CleanupCtpApi();
		}
		exit(-1);
	}

	//即使持仓要求能得到满足，也要检查各个合约Claim的Position和总的Position是否一致
	bool bMisMatch = false;
	for(size_t i = 0; i < m_pCtpCntIfList.size(); i++)
	{
		for(size_t j = 0; j < m_pCtpCntIfList[i]->m_InvestorPositionDetailList.size(); j++)
		{
			CInvestorPositionDetail* pInvestorPositionDetail = &m_pCtpCntIfList[i]->m_InvestorPositionDetailList[j];
			CThostFtdcInvestorPositionField* pInvestorPosition = &pInvestorPositionDetail->CtpInvestorPosition;

			if(pInvestorPosition->Position != pInvestorPositionDetail->ClaimedPosition)
			{
				bMisMatch = true;
				cerr << m_Prompt << "合约代码: " << pInvestorPosition->InstrumentID << " 持仓方向: " << CtpDirctionToMyDirection(pInvestorPosition->PosiDirection)
					 << " 总持仓: " << pInvestorPosition->Position << " Claimed持仓: " << pInvestorPositionDetail->ClaimedPosition << " 不一致!" << ENDLINE;
			}
		}
	}

	//提供一次机会选择是否应该退出程序
	if(bMisMatch == true)
	{
		char EndProgram;
		cerr << m_Prompt << "策略持仓和投资者账户持仓出现不一致的情况，是否结束程序运行？（Y/N)：" ; cin >> EndProgram;
        if(EndProgram == 'Y' || EndProgram == 'y') 
		{
			m_pPosMoneyManage->DumpToTextFile();
			m_pStrategysManage->DumpPortfolioToTextFile();
            PressAnyKeyToExit();
			//为避免报错，先要清除主动操作的各种对象
			DestroyObjects();
			for(size_t i = 0; i < m_pCtpCntIfList.size(); i++)
			{
			   m_pCtpCntIfList[i]->CleanupCtpApi();
			}
			exit(-1);
		}
	}

	//同步策略中组合和头寸的状态
	m_pStrategysManage->SyncPortfolioAndPosition();

	//寻找各个头寸对应的市场行情节点
	m_pStrategysManage->FindMarketDataNode();

	//检查各个策略的各组合到期情况
	m_pStrategysManage->ExpireCheck();
}

//++
//在自动交易程序运行前, 检查各个柜台的报单和撤单是否正常
//如果报撤单不正常则停止策略运行
//参数
//    无
//返回值
//     true: 报撤单正常
//     false: 报撤单不正常
//--
bool CRealApp::CheckPlaceAndCancelOrder()
{
	bool ret = true;
	bool bWaitExchange = false;

	//逐柜台检查报撤单是否正常
	for(size_t i = 0; i < m_pCtpCntIfList.size(); i++)
	{
		CCtpCntIf* pCtpCntIf = m_pCtpCntIfList[i];

		//已经测试成功, 则不需要继续处理
		if(pCtpCntIf->m_bCheckPlaceAndCancelOrder == true) continue;

		CRealTraderSpi*  pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;
		CThostFtdcDepthMarketDataField Md;
		CThostFtdcInstrumentField* pInstInfo = NULL;
		bool bGetContract = GetMdAndInstInfo(pCtpCntIf, Md, pInstInfo);

		//如果无法获得可以下单的合约
		if(bGetContract == false)
		{
			ret = false;
			break;
		} //if

		string strExchangeId = pInstInfo->ExchangeID;
		RemoveSpace(strExchangeId);

		//如果重试次数超过2次, 也不要再试了
		if(pCtpCntIf->m_CheckRetryTimes >= 2)
		{
			cerr << "交易柜台: " << pCtpCntIf->m_CounterName << ", 报单撤单测试次数超过2次或者测试报单已成交, 请按ESC键退出!" << ENDLINE;
			ret = false;
			break;
		}

		//相关状态变量
		double PlaceOrderSecs = 0;
		double DiffSecs = 0;
		double CurSecs = 0;
		bool bTraded = false;
		CThostFtdcOrderField Order;
		int retP;
		int retC;
		bool bPlaceOrderFail = false;

		//以跌停价发送买入报单
		TThostFtdcInstrumentIDType    InstrumentID; strcpy(InstrumentID, Md.InstrumentID);
		TThostFtdcDirectionType       dir = 'b'; //买入
		TThostFtdcCombOffsetFlagType  kpp; kpp[0] = 'o'; //开仓
		double OrderPrice = Md.LowerLimitPrice + 2 * pInstInfo->PriceTick; //跌停价+2*PriceTick
		TThostFtdcPriceType           price = (OrderPrice < Md.BidPrice1) ? OrderPrice : Md.LowerLimitPrice;
		TThostFtdcVolumeType          vol = (pCtpCntIf->m_CntCode != 's') ? 1 : 100;  //使用最小交易单位, 证券是100
		char TimeCondition = 'G';   //GFD当日有效限价单
		char VolumeCondition  = 'A';
		//上交所股票期权市价单使用BestPrice, 深交所股票期权市价单使用AnyPrice
		char MarketOrderType = (pCtpCntIf->m_CntCode == 'o' && strExchangeId == "SSE") ? 'B' : ((pCtpCntIf->m_CntCode == 's') ? 'F' : 'A');
		int OrderRef = atoi(pTraderUserSpi->m_orderRef);
		int FrontId = pTraderUserSpi->m_frontId;
		int SessionId = pTraderUserSpi->m_sessionId;

		//在提交报单之前等待1秒, 防止本地时间因为校准的原因可能比交易所提前
		if(bWaitExchange == false)
		{
		    SleepMs(1000);
			bWaitExchange = true;
		}

		retP = pTraderUserSpi->ReqOrderInsert(InstrumentID, dir, kpp, price, vol, TimeCondition, VolumeCondition, MarketOrderType);
		if(retP != 0)
		{
			cerr << "提交测试报单不成功！" << ENDLINE;
		}

		SYSTEMTIME CurTime;
		GetLocalTime(&CurTime);
		PlaceOrderSecs = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond;
		memset(&Order, 0, sizeof(CThostFtdcOrderField));

		//等待报单被提交到交易所
		bool bPlaceQry = false;
		while(Order.OrderStatus != THOST_FTDC_OST_NoTradeQueueing && retP == 0)
		{
			SleepMs(100); //报单回应需要一定时间
			//检查报单状态, 报单列表是按由旧到新排列的
			EnterCriticalSection(&pCtpCntIf->m_OrderListCS);
			for(size_t j = 0; j < pTraderUserSpi->m_orderList.size(); j++)
			{
				CThostFtdcOrderField *pOrder = &pTraderUserSpi->m_orderList[j];
				int BrokerOrderSeq = pOrder->BrokerOrderSeq;
				int rtnOrderRef = atoi(pOrder->OrderRef);
				if(FrontId == pOrder->FrontID && SessionId == pOrder->SessionID && OrderRef == rtnOrderRef)
				{
				  
					if(pOrder->OrderStatus == THOST_FTDC_OST_AllTraded || pOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing
						|| pOrder->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing) 
					{
						memcpy(&Order, pOrder, sizeof(CThostFtdcOrderField));
						break; 
					}
					else if(pOrder->OrderStatus == THOST_FTDC_OST_NoTradeQueueing)
					{
						memcpy(&Order, pOrder, sizeof(CThostFtdcOrderField));
					}
					else if(Order.OrderStatus != THOST_FTDC_OST_NoTradeQueueing)
					{
						memcpy(&Order, pOrder, sizeof(CThostFtdcOrderField));
					}
				}
			} //for j
			LeaveCriticalSection(&pCtpCntIf->m_OrderListCS);

			GetLocalTime(&CurTime);
			CurSecs = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond;
			DiffSecs = CurSecs - PlaceOrderSecs;

			if(Order.OrderStatus == THOST_FTDC_OST_AllTraded || Order.OrderStatus == THOST_FTDC_OST_PartTradedQueueing
				|| Order.OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing)
			{
				cerr << "测试报单已经成交, 请按ESC键退出及时处理!" << ENDLINE;
				bTraded = true;
				break;
			}
			else if(DiffSecs > 5.0 && bPlaceQry == false)   //查询报单
			{
				int retQ = pTraderUserSpi->ReqQryOrder(&Order);
				bPlaceQry = true;
			}
			else if(DiffSecs > 10.0)
			{
				cerr << "测试报单已经提交超过10秒, 交易所无回应!" << ENDLINE;
				bPlaceOrderFail = true;
				break;
			}
		} //while

		//测试撤单, 只要没有成功交易都需要撤单
		if(bTraded == false)
		{
			SleepMs(1000);  //等待1秒
			//发送撤单
			string strInstID = Order.InstrumentID;
			retC = pTraderUserSpi->ReqOrderAction(&Order, FrontId, SessionId, OrderRef, strInstID);
			if(retC != 0)
			{
				cerr << "对测试报单进行撤单不成功!" << ENDLINE;
			}

			DiffSecs = 0;
			memset(&Order, 0, sizeof(CThostFtdcOrderField));
		    double CancelOrderSecs = 0;
			GetLocalTime(&CurTime);
			CancelOrderSecs = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond;
			bool bCancelQry = false;
			while(Order.OrderStatus != THOST_FTDC_OST_Canceled && retC == 0)
			{
				SleepMs(100); //报单回应需要一定时间
				//检查报单状态, 报单列表是按由旧到新排列的
				EnterCriticalSection(&pCtpCntIf->m_OrderListCS);
				for(size_t j = 0; j < pTraderUserSpi->m_orderList.size(); j++)
				{
					CThostFtdcOrderField *pOrder = &pTraderUserSpi->m_orderList[j];
					int BrokerOrderSeq = pOrder->BrokerOrderSeq;
					int rtnOrderRef = atoi(pOrder->OrderRef);
					if(FrontId == pOrder->FrontID && SessionId == pOrder->SessionID && OrderRef == rtnOrderRef)
					{
				  
						if(pOrder->OrderStatus == THOST_FTDC_OST_AllTraded || pOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing
							|| pOrder->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing) 
						{
							memcpy(&Order, pOrder, sizeof(CThostFtdcOrderField));
							break; 
						}
						else if(pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
						{
							memcpy(&Order, pOrder, sizeof(CThostFtdcOrderField));
						}
						else if(Order.OrderStatus != THOST_FTDC_OST_Canceled)
						{
							memcpy(&Order, pOrder, sizeof(CThostFtdcOrderField));
						}
					}
				} //for j
				LeaveCriticalSection(&pCtpCntIf->m_OrderListCS);

				GetLocalTime(&CurTime);
				CurSecs = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond;
				DiffSecs = CurSecs - CancelOrderSecs;

				if(Order.OrderStatus == THOST_FTDC_OST_AllTraded || Order.OrderStatus == THOST_FTDC_OST_PartTradedQueueing
					|| Order.OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing)
				{
					cerr << "测试报单在撤单之前已经成交, 请按ESC键退出及时处理!" << ENDLINE;
					bTraded = true;
					break;
				}
				else if(DiffSecs > 5.0 && bCancelQry == false)   //查询报单
				{
					int retC = pTraderUserSpi->ReqQryOrder(&Order);
					bCancelQry = true;
				}
				else if(DiffSecs > 10.0)
				{
					cerr << "测试报单撤单请求已经提交超过10秒, 交易所无回应!" << ENDLINE;
					break;
				}
			} //while
		} //if 报单未成交

		//只需检查最后撤单结果即可
		//报单已经成交, 算是一种异常情况, 需要手动处理
		if(bTraded == true)
		{
			pCtpCntIf->m_CheckRetryTimes = 10;
			pCtpCntIf->m_bCheckPlaceAndCancelOrder = false;
			ret = false;
			break;
		}
		else if(bPlaceOrderFail == true)
		{
			pCtpCntIf->m_CheckRetryTimes++;
			pCtpCntIf->m_bCheckPlaceAndCancelOrder = false;
			ret = false;
			break;
		}
		else if((DiffSecs > 10.0 && Order.OrderStatus != THOST_FTDC_OST_Canceled) || retC != 0) 
		{
			pCtpCntIf->m_CheckRetryTimes++;
			pCtpCntIf->m_bCheckPlaceAndCancelOrder = false;
			ret = false;
			break;
		}
		else
		{
			pCtpCntIf->m_bCheckPlaceAndCancelOrder = true;
		}
	}//for i, 各个交易柜台

	return ret;
}

//++
//获取用于报单撤单的合约行情和其它信息
//参数
//    pOptCtpCntIf: 交易柜台指针
//    Md: 引用, 合约深度行情
//    pInstInfo: 合约信息指针引用
//返回值
//    true: 得到合约深度行情和相关信息
//    false: 无法得到合约信息
//--
bool CRealApp::GetMdAndInstInfo(CCtpCntIf* pOptCtpCntIf, CThostFtdcDepthMarketDataField &Md, CThostFtdcInstrumentField* &pInstInfo)
{
	bool bGetContract = false;
	vector<CTshapeBase*>* ppTshapeList = &m_pMarketDataManage->m_pTshapeBaseList;

	//检查是否可以得到该柜台交易的期权合约
	for(size_t j = 0; j < ppTshapeList->size(); j++)
	{
		CTshapeBase * pTshapeBase = (*ppTshapeList)[j];
		COptionKind* pOptionKind = NULL;

		//确定该T型报价的OptionKind
		for(size_t k = 0; k < m_pStrategysManage->m_SynOptionKindList.size(); k++)
		{
			if(pTshapeBase->m_OptionId == m_pStrategysManage->m_SynOptionKindList[k].OptionId)
			{
				pOptionKind = &m_pStrategysManage->m_SynOptionKindList[k];
				break;
			}
		} //for k
		//需要检查是否在交易时段内
		if(IsAllowedTradeTime(pOptionKind, this) == false) continue;

		if(pOptCtpCntIf->m_CntCode == pTshapeBase->m_OptCntCode)
		{
			for(size_t k = 0; k < pTshapeBase->m_pTshapeMonthList.size(); k++)
			{
				CTshapeMonth* pTshapeMonth = pTshapeBase->m_pTshapeMonthList[k];
				CThostFtdcDepthMarketDataField UnderMd = pTshapeMonth->GetLockedMarketData();
				double UnderPrice = max(UnderMd.LastPrice, (UnderMd.AskPrice1 + UnderMd.BidPrice1) * 0.5);

				//寻找实值一档的看跌期权
				for(size_t m = 0; m < pTshapeMonth->m_pOptionPairList.size(); m++)
				{
					COptionPairItem* pOptionPair = pTshapeMonth->m_pOptionPairList[m];
					double PriceTickX = 10 * pOptionPair->m_pPutItem->m_InstInfo.PriceTick;
					if(pOptionPair->m_StrikePrice > (UnderPrice + PriceTickX))
					{
						Md = pOptionPair->m_pPutItem->GetLockedMarketData();
						pInstInfo = &pOptionPair->m_pPutItem->m_InstInfo;
						bGetContract = true;
						break;
					}
				} //for m
				if(bGetContract == true) break;
			} //for k
		}
		if(bGetContract == true) break;
	} //for j

	//找不到期权合约, 看看能否找到标的合约
	if(bGetContract == false)
	{
		for(size_t j = 0; j < ppTshapeList->size(); j++)
		{
			CTshapeBase * pTshapeBase = (*ppTshapeList)[j];
			COptionKind* pOptionKind = NULL;

			//确定该T型报价的OptionKind
			for(size_t k = 0; k < m_pStrategysManage->m_SynOptionKindList.size(); k++)
			{
				if(pTshapeBase->m_OptionId == m_pStrategysManage->m_SynOptionKindList[k].OptionId)
				{
					pOptionKind = &m_pStrategysManage->m_SynOptionKindList[k];
					break;
				}
			} //for k
			//需要检查是否在交易时段内
			if(IsAllowedTradeTime(pOptionKind, this) == false) continue;

		    if(pOptCtpCntIf->m_CntCode == pTshapeBase->m_UnderCntCode)
			{
				for(size_t k = 0; k < pTshapeBase->m_pTshapeMonthList.size(); k++)
				{
					CTshapeMonth* pTshapeMonth = pTshapeBase->m_pTshapeMonthList[k];
					CThostFtdcDepthMarketDataField UnderMd = pTshapeMonth->GetLockedMarketData();
					double UnderPrice = min(UnderMd.LastPrice, UnderMd.BidPrice1);
					double PriceTickX = 10 * pTshapeMonth->m_UnderlyingInstInfo.PriceTick;

					//标的价格需要大于跌停价
					if(UnderPrice > (UnderMd.LowerLimitPrice + PriceTickX))
					{
						Md = UnderMd;
						pInstInfo = &pTshapeMonth->m_UnderlyingInstInfo;
						bGetContract = true;
						break;
					}
				} //for k
			}
			if(bGetContract == true) break;
		} //for j
	}

	return bGetContract;
}

//++
//策略自动交易主程序
//用于初始化, 启动各子线程, 接收各种状态和控制信息以确定处理方法
//--
void CRealApp::AutoTradeMain()
{
#ifdef WIN_CTP
	//安装控制台消息处理回调函数，以便优雅地退出程序
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE) 
	{ 
		cerr << m_Prompt << "不能安装控制台消息处理回调函数" << ENDLINE;
		return; 
	}
#else  //Linux
    signal(SIGINT , ConsoleHandler);           //按下CTRL + C键
    signal(SIGHUP , ConsoleHandler);           //关闭控制台窗口
    signal(SIGQUIT, ConsoleHandler);       
    signal(SIGTERM, ConsoleHandler);     
    signal(SIGKILL, ConsoleHandler);
#endif //WIN_CTP

	//初始化交易程序
	InitializeApp();
	cerr << m_Prompt << "程序初始化完成, 请检查参数配置是否正确, 按任意键继续..." << ENDLINE;
	PauseForInput();

	//订阅所有柜台欲交易合约的市场行情
	m_pMarketDataManage->SubscribeAllMarketData(0);
	SleepMs(1000);
    cerr << m_Prompt << ENDLINE;

	double CurDispErrSecs = 0;
	double PreDispErrSecs = 0;
	double PreHbSecs = 0;
	SYSTEMTIME CurTime;
	//自动交易出错标志
	m_bAutoTradeErr = false;
	BOOL bRet = 0;

#ifdef WIN_CTP
	//设置程序的优先级为高优先级
	bRet = SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);

	//启动行情处理线程
	m_hMdThread = (CEventHandle)_beginthreadex(NULL, 0, CRealApp::MarketDataProc, this, 0, NULL);
	bRet = SetThreadPriority(m_hMdThread, THREAD_PRIORITY_ABOVE_NORMAL);
	if(m_hMdThread != NULL) cerr << m_Prompt << "行情处理线程已启动..." << ENDLINE;

	//启动数据处理线程
	m_hDataThread = (CEventHandle)_beginthreadex(NULL, 0, CRealApp::TradeDataProc, this, 0, NULL);
	if(m_hDataThread != NULL) cerr << m_Prompt << "数据处理线程已启动..." << ENDLINE;

	//启动交易引擎线程
	m_hTradeThread = (CEventHandle)_beginthreadex(NULL, 0, CRealApp::TradeEngine, this, 0, NULL);
	bRet = SetThreadPriority(m_hTradeThread, THREAD_PRIORITY_TIME_CRITICAL);
	if(m_hTradeThread != NULL) cerr << m_Prompt << "交易引擎线程已启动..." << ENDLINE;
#else //Linux

	int ret;
	
	//设置当前进程为高优先级
	pid_t pid = getpid();
	struct sched_param param;
	param.sched_priority = sched_get_priority_max(SCHED_RR);
	sched_setscheduler(pid, SCHED_RR, &param);
	//设置主线程为高优先级
	SetCurrentThreadHighPriority(true);

	//启动行情处理线程
    pthread_t TidMarketData;
	ret = pthread_create(&TidMarketData, NULL, CRealApp::MarketDataProc, this);
	if(ret == 0) cerr << m_Prompt << "行情处理线程已启动..." << ENDLINE;
    
	//启动数据处理线程
	pthread_t TidTradeData;
	ret = pthread_create(&TidTradeData, NULL, CRealApp::TradeDataProc, this);
	if(ret == 0) cerr << m_Prompt << "数据处理线程已启动..." << ENDLINE;

	//启动交易引擎线程
	pthread_t TidTradeEngine;
	ret = pthread_create(&TidTradeEngine, NULL, CRealApp::TradeEngine, this);
	if(ret == 0) cerr << m_Prompt << "交易引擎线程已启动..." << ENDLINE;
#endif //WIN_CTP

	//交易主循环程序
	SleepMs(2000);
	cerr << m_Prompt << "自动化交易主程序开始运行..." << ENDLINE;
	m_bCheckPlaceAndCancelOrder = false;

	//支持OMP并行嵌套, OMP支持后扫描速度反而变慢
	//缺省选项project->properties->c/c++->languages->Open MP support选No
	//omp_set_nested(1);//设置支持嵌套并行

	while(true) {
		int ret = 0;

		//确定是否有更新的市场行情数据
		bool bGetMd = false;
		DWORD waitReturn = WaitForSingleObject(m_hMdToStrategyScan, MD_WAIT_TIME); 
		ResetEvent(m_hMdToStrategyScan); 
		if(waitReturn == WAIT_OBJECT_0)
		{
			bGetMd = true;
		}

		//检查报单撤单是否正常
		if(bGetMd == true && m_bCheckPlaceAndCancelOrder == false)
		{
			m_bCheckPlaceAndCancelOrder = CheckPlaceAndCancelOrder();
		}

	//#define TIME_MEASURE_AUTO_MAIN
	#ifdef TIME_MEASURE_AUTO_MAIN
		static double maxAutoTradetime = 0;
		double BeginSecs = omp_get_wtime( );
	#endif

		//成功获取市场行情则进行策略开平仓交易处理
		if(bGetMd == true && m_bAutoTradeErr == false && m_bCheckPlaceAndCancelOrder == true)
		{
			//开仓
			ret |= m_pStrategysManage->AddToOpenQueue();
			//平仓
			ret |= m_pStrategysManage->AddToCloseQueue();
		}

		//对各个柜台的报撤单数目进行快照
		for(size_t i = 0; i < m_pCtpCntIfList.size(); i++)
		{
			CMyFlowControl *pFlowControl = m_pCtpCntIfList[i]->m_pFlowControl;
			pFlowControl->SnapshotOrderNum();
		}

		//刷新持仓和账户资金信息
		m_pPosMoneyManage->RefreshTradeAccountInfo();

	//测试用
	#ifdef TIME_MEASURE_AUTO_MAIN
		double EndSecs = omp_get_wtime( );
		double AutoTradetime = (EndSecs - BeginSecs);
		maxAutoTradetime = (AutoTradetime > maxAutoTradetime) ? AutoTradetime : maxAutoTradetime;
		cerr << "自动交易主要路径所花费时间 当前值 = " << AutoTradetime << ", 最大值 = " << maxAutoTradetime << ENDLINE;
	#endif

		//检查API接口是否有错误回报
		bool bApiError = false;
		for(size_t i = 0; i < m_pCtpCntIfList.size(); i++)
		{
			if(m_pCtpCntIfList[i]->m_pTraderUserSpi->m_bOrderInsertError || m_pCtpCntIfList[i]->m_pTraderUserSpi->m_bOrderActionError
				|| m_pCtpCntIfList[i]->m_pTraderUserSpi->m_bLockInsertError)
			{
				bApiError = true;
			}
		}

		//设置自动交易是否出现错误的标志
		if(ret != 0 || bApiError || m_bTradeEngineError || m_bAutoTradeErr)
		{
			GetLocalTime(&CurTime);
			CurDispErrSecs = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond;
			if(CurDispErrSecs > (PreDispErrSecs+5) || m_bAutoTradeErr == false)
			{
				Beep(2000,500);  //以2000Hz的频率，蜂鸣500毫秒
				m_pStrategysManage->ShowQueueStatus(true);
                cerr << m_Prompt << "自动交易过程出现错误, 按ESC键退出交易循环进行手动处理！" << ENDLINE;
				PreDispErrSecs = CurDispErrSecs;
			}

			m_bAutoTradeErr = true;  		
		}

		//检查是否有头寸需要自动行权
		m_pStrategysManage->CheckAutoExecOption();

		//按ESC键时可以选择结束自动交易
		if(_kbhit()) {
			bool bClearErrorFlag = false;
			bool bExitApp = ExitAutoTradeProc(bClearErrorFlag);
			if(bExitApp == true) 
			{
				break;
			}
			else if(bClearErrorFlag == true)
			{
				ret = 0;
				bApiError = false;
				m_bTradeEngineError = false;
				m_bAutoTradeErr = false;
				for(size_t i = 0; i < m_pCtpCntIfList.size(); i++)
				{
					m_pCtpCntIfList[i]->m_pTraderUserSpi->m_bOrderInsertError = false;
					m_pCtpCntIfList[i]->m_pTraderUserSpi->m_bOrderActionError = false;
					m_pCtpCntIfList[i]->m_pTraderUserSpi->m_bLockInsertError = false;
				}
			} //if bExitApp
		}

		//给出HeartBeat信息, 表示线程还在活动
		string strPrefix = "MuxCnt " + m_Prompt + " 策略扫描主线程";
		HeartBeatIndicate(PreHbSecs, THREAD_ACTIVE_INDIC, strPrefix);
	} //while

#ifndef WIN_CTP //Linux
	pthread_join(TidMarketData, NULL);
	pthread_join(TidTradeData, NULL);
	pthread_join(TidTradeEngine, NULL);
#endif //not WIN_CTP

	//为避免报错，先要清除主动操作的各种对象
	DestroyObjects();
	for(size_t i = 0; i < m_pCtpCntIfList.size(); i++)
	{
		m_pCtpCntIfList[i]->CleanupCtpApi();
	}

	return;
}

//++
//退出自动交易的处理函数
//参数
//    bClearErrorFlag: 引用, true: 清除程序运行的各种错误标志; false: 不作任何改变
//返回值
//    true: 退出自动交易
//    false: 继续交易
//--
bool CRealApp::ExitAutoTradeProc(bool &bClearErrorFlag)
{
	DWORD wRet;
	m_bExitApp = false;
	bClearErrorFlag = false;
	char kill = _getch();

#ifndef WIN_CTP
	FlushKbInBuf(); //清空键盘输入缓存
#endif	

	//判断是否要设置报撤单检查通过标志
	if(kill == 's' || kill == 'S')
	{
		if(m_bCheckPlaceAndCancelOrder == false)
		{
			cerr << m_Prompt << "设置报单撤单检查标志为通过? Y/N >"; cin >> kill;
			if(kill == 'Y' || kill == 'y') 
		    {
				m_bCheckPlaceAndCancelOrder = true;
			}
		}
	}
	//判断是否为ESC键
	else if(kill == 27)  
	{
		//确认是否退出
		cerr << m_Prompt << "结束自动交易? Y/N >"; cin >> kill;
		if(kill == 'Y' || kill == 'y') 
		{
			//强制交易引擎撤单
			m_bForceCancelOrder = true;
			//等待撤单处理完毕
			SleepMs(2000);  
			//设置程序退出标志
			m_bExitApp = true;
			//等待FISH单线程退出, 之后相关变量的访问不需加锁
			wRet = WaitForSingleObject(m_hTradeThread, WAIT_THREAD_FINISH);
			if(wRet != WAIT_OBJECT_0)
			{
				cerr << m_Prompt << "等待交易引擎线程结束超时!" << ENDLINE;
			}

			//退出时再次自动撤单, 开仓队列
			for(size_t i = 0; i < m_pStrategysManage->m_pPortfInOpenList.size(); i++)
			{
				CPortfolioBase *pPortfolio = m_pStrategysManage->m_pPortfInOpenList[i];
				for(size_t j = 0; j < pPortfolio->m_pPositionTradeList.size(); j++)
				{
					CPositionTrade *pTrade = pPortfolio->m_pPositionTradeList[j];
					if(pTrade->m_PositionStatus == Status_FishOpen || pTrade->m_PositionStatus == Status_FishClose)
					{
						pTrade->CancelFishOrder();
					}
				}				
			}

			//退出时再次自动撤单, 平仓队列
			for(size_t i = 0; i < m_pStrategysManage->m_pPortfInCloseList.size(); i++)
			{
				CPortfolioBase *pPortfolio = m_pStrategysManage->m_pPortfInCloseList[i];
				for(size_t j = 0; j < pPortfolio->m_pPositionTradeList.size(); j++)
				{
					CPositionTrade *pTrade = pPortfolio->m_pPositionTradeList[j];
					if(pTrade->m_PositionStatus == Status_FishOpen || pTrade->m_PositionStatus == Status_FishClose)
					{
						pTrade->CancelFishOrder();
					}
				}				
			}

			//退出前对没有对冲的交易策略进行强制选择性平仓
			for(size_t i = 0; i < m_pStrategysManage->m_pStrategyBaseList.size(); i++)
			{
				CStrategyBase *pStrategy = m_pStrategysManage->m_pStrategyBaseList[i];
				pStrategy->EscForceClose();
			}

			//检查交易中是否出现错误
			if(m_bAutoTradeErr == true)
			{
				cerr << m_Prompt << "手动进行错误恢复? Y/N >"; cin >> kill;
				if(kill == 'Y' || kill == 'y')
				{
					//长时间处于开仓队列中而没移出的组合属于出错
					for(size_t i = 0; i < m_pStrategysManage->m_pPortfInOpenList.size(); i++)
						m_pStrategysManage->m_pPortfInOpenList[i]->ManualErrorRecovery();

					//长时间处于平仓队列而没移出的组合属于出错
					for(size_t i = 0; i < m_pStrategysManage->m_pPortfInCloseList.size(); i++)
						m_pStrategysManage->m_pPortfInCloseList[i]->ManualErrorRecovery();
				}
			}

			//等待数据处理线程退出, 之后相关数据不需加锁
			wRet = WaitForSingleObject(m_hDataThread, WAIT_THREAD_FINISH);
			if(wRet != WAIT_OBJECT_0)
			{
				cerr << m_Prompt << "等待数据处理线程结束超时!" << ENDLINE;
			}

			//保存交易数据
			cerr << m_Prompt << "退出时保存交易数据? Y/N >"; cin >> kill;
			if(kill == 'Y' || kill == 'y') 
			{
				PostProcessing(true); //强制保存数据
			}

			//等待行情处理线程退出
			wRet = WaitForSingleObject(m_hMdThread, WAIT_THREAD_FINISH);
			if(wRet != WAIT_OBJECT_0)
			{
				cerr << m_Prompt << "等待行情处理线程结束超时!" << ENDLINE;
			}
		} //if 选择退出程序
		else if(kill == 'N' || kill == 'n')
		{
			if(m_pStrategysManage->m_pPortfInOpenList.size() != 0 || m_pStrategysManage->m_pPortfInCloseList.size() != 0)
			{
				cerr << m_Prompt << "开仓队列/平仓队列不全为空, 无法自动清除各种错误标志!" << ENDLINE;
				bClearErrorFlag = false;
			}
			else
			{
				cerr << m_Prompt << "将自动清除各种错误标志, 程序继续运行!" << ENDLINE;
				bClearErrorFlag = true;
			}
		}
		else //无效输入
		{
		    cerr << "按ESC后无效输入!" << ENDLINE;
		}
	} //if 按了ESC键
	else
	{
		cerr << "未定义功能的按键输入!" << ENDLINE;
	}

#ifndef WIN_CTP
	FlushKbInBuf(); //清空键盘输入缓存
#endif	

	return m_bExitApp;
}

//++
//同步获取市场行情数据, 市场行情包括合约交易状态信息
//参数
//    无
//返回值
//    true: 同步市场行情成功
//    false: 同步市场行情失败
//--
bool CRealApp::SyncDepthMarketData()
{
	DWORD nIndex;
	bool bRet = true;

	size_t CounterSize = m_pCtpCntIfList.size();
	CEventHandle *hMdSpiEvent = new CEventHandle[CounterSize*2];
	for(size_t i = 0; i < CounterSize; i++)
	{
		hMdSpiEvent[2*i] = m_pCtpCntIfList[i]->m_hMdSpiEvent;
		hMdSpiEvent[2*i+1] = m_pCtpCntIfList[i]->m_hInstrumentStatusEvent;
	}

	//等待行情更新，最多等10秒
	nIndex = WaitForMultipleObjects(CounterSize*2, hMdSpiEvent, FALSE, MD_WAIT_TIME);
	if(nIndex == WAIT_TIMEOUT || nIndex == WAIT_FAILED)  //某些商品流动性不太好，可能经常出现10秒之内没有行情数据更新
	{
		//判断是否在交易时段内
		bool bIsTradingTime = IsTradingTime(&m_pStrategysManage->m_SynOptionKindList);

		//在交易时段内输出警示信息
		//非交易时段不作处理，否则会一直显示信息，干扰判断
		if(bIsTradingTime == true)
		{
			SYSTEMTIME CurTime;
			GetLocalTime(&CurTime);
			cerr << m_Prompt << "没有行情更新，等待Tick采样数据超时或者出错！" << ENDLINE;
			cerr << m_Prompt << "当前时间: " << CurTime.wHour << ":" << CurTime.wMinute << ":" << CurTime.wSecond << ENDLINE;
		}
		bRet = false;
	}
	else
	{
		//测试用
		//#define TIME_MEASURE_MD
		#ifdef TIME_MEASURE_MD
		    static double maxMdRefreshtime = 0;
			double BeginSecs = omp_get_wtime( );
		#endif

		//更新合约Tick市场行情数据
		bool bArbOnly = false;
		m_pMarketDataManage->GetDepthMarketDataList();
		m_pMarketDataManage->UpdateAllTshape(bArbOnly);	

		//更新合约交易状态信息
		m_pMarketDataManage->GetInstrumentStatusList();
		m_pMarketDataManage->UpdateAllTshapeInstrumentStatus();	

		bRet = true;

		//测试用
		#ifdef TIME_MEASURE_MD
			double EndSecs = omp_get_wtime( );
			double MdRefreshtime = (EndSecs - BeginSecs);
			maxMdRefreshtime = (MdRefreshtime > maxMdRefreshtime) ? MdRefreshtime : maxMdRefreshtime;
			cerr << "行情更新和参数计算花费时间 当前值 = " << MdRefreshtime << ", 最大值 = " << maxMdRefreshtime << ENDLINE;
		#endif
	} //else if nIndex

	delete[] hMdSpiEvent;
	return bRet;
}

//++
//后处理，用于存储交易信息到数据库
//参数
//    bForce: 强制存储统计信息和报单成交信息, 不管时间条件
//返回值
//    无
//--
void CRealApp::PostProcessing(bool bForce)
{
	//将当前还未写入数据库的交易写入数据库
	m_pStrategysManage->SaveToDatabase(bForce);

	//在设定的时间点将K线数据写入数据库
	double LocalTimeSeconds;
	double PostProcTimeSeconds;
	SYSTEMTIME CurTime;
	int hours;
	int minutes;
	int seconds;

	GetLocalTime(&CurTime);
	LocalTimeSeconds = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;

	//新的一天需要重新初始化
	if(m_PreTime.wDay != CurTime.wDay)
	{
		memcpy(&m_PreTime, &CurTime, sizeof(SYSTEMTIME));
		for(int i = 0; i < 20; i++)
		{
			m_bProcessedSeg[i] = false;
		}
	}

	for(size_t i = 0; i < m_PostProcTimeList.size(); i++)
	{
		//该时段尚未被处理
		if(m_bProcessedSeg[i] == false)    
		{
			hours = int(m_PostProcTimeList[i]/1e4);
			minutes = int((m_PostProcTimeList[i]/1e2) - hours*1e2);
			seconds = int((m_PostProcTimeList[i]) - hours*1e4 - minutes*1e2);
			PostProcTimeSeconds = hours*3600 + minutes*60 + seconds;

			//交易时段结束后MAX_POSTPROC_SECONDS秒内
			if((LocalTimeSeconds > PostProcTimeSeconds && (LocalTimeSeconds-PostProcTimeSeconds) <= MAX_POSTPROC_SECONDS) || bForce)
			{
				cerr << m_Prompt << "标的K线数据写入数据库..., 当前时间 " << CurTime.wYear << "-" << CurTime.wMonth << "-" << CurTime.wDay << " " << CurTime.wHour
					<< ":" << CurTime.wMinute << ":" << CurTime.wSecond << ENDLINE;
				//将K线数据更新入数据库
				m_pMarketDataManage->DumpDayKLine();

				GetLocalTime(&CurTime);
				cerr << m_Prompt << "策略统计数据写入数据库..., 当前时间 " << CurTime.wYear << "-" << CurTime.wMonth << "-" << CurTime.wDay << " " << CurTime.wHour
					<< ":" << CurTime.wMinute << ":" << CurTime.wSecond << ENDLINE;
				//将策略统计数据更新入数据库
			    m_pStrategysManage->DumpStatData();

				GetLocalTime(&CurTime);
				cerr << m_Prompt << "策略统计数据已经写入数据库, 当前时间 " << CurTime.wYear << "-" << CurTime.wMonth << "-" << CurTime.wDay << " " << CurTime.wHour
					<< ":" << CurTime.wMinute << ":" << CurTime.wSecond << ENDLINE;

				//将报单信息写入数据库
				m_pPosMoneyManage->SaveOrderToDatabase();
				GetLocalTime(&CurTime);
				cerr << m_Prompt << "报单信息已经写入数据库, 当前时间 " << CurTime.wYear << "-" << CurTime.wMonth << "-" << CurTime.wDay << " " << CurTime.wHour
					<< ":" << CurTime.wMinute << ":" << CurTime.wSecond << ENDLINE;

				//将成交信息写入数据库
				m_pPosMoneyManage->SaveTradeToDatabase();
				GetLocalTime(&CurTime);
				cerr << m_Prompt << "成交信息已经写入数据库, 当前时间 " << CurTime.wYear << "-" << CurTime.wMonth << "-" << CurTime.wDay << " " << CurTime.wHour
					<< ":" << CurTime.wMinute << ":" << CurTime.wSecond << ENDLINE;

				//将各个交易柜台的资金账户信息写入数据库
				m_pPosMoneyManage->SaveTradingAccountToDb();

				//将各柜台/交易所交易规则统计信息写入数据库
				for(size_t j = 0; j < m_pExchangeRuleFilterList.size(); j++)
				{
					CExchangeRuleFilter* pExchangeRuleFilter = m_pExchangeRuleFilterList[j];
					pExchangeRuleFilter->DumpTodayStatusToDb();
				}
				GetLocalTime(&CurTime);
				cerr << m_Prompt << "各柜台/交易所交易规则统计信息已经写入数据库, 当前时间 " << CurTime.wYear << "-" << CurTime.wMonth << "-" << CurTime.wDay << " " << CurTime.wHour
					<< ":" << CurTime.wMinute << ":" << CurTime.wSecond << ENDLINE;

				//到了保存数据时间, 将组合交易信息强制写入数据库
				if(bForce == false)
				{
					m_pStrategysManage->SaveToDatabase(true);
				}

				m_bProcessedSeg[i] = true;
				break;  //同一时间不应该有多个时段满足保存数据的要求
			}
		} //该时段尚未被处理
	} //for i
}

//++
//手动交易和策略计算主程序
//--
void CRealApp::ManualTradeMain()
{
#ifdef WIN_CTP
	//安装控制台消息处理回调函数，以便优雅地退出程序
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE) 
	{ 
		cerr << "不能安装控制台消息处理回调函数" << ENDLINE;
		return; 
	}
#else //Linux
    signal(SIGINT , ConsoleHandler);           //按下CTRL + C键
    signal(SIGHUP , ConsoleHandler);           //关闭控制台窗口
    signal(SIGQUIT, ConsoleHandler);       
    signal(SIGTERM, ConsoleHandler);     
    signal(SIGKILL, ConsoleHandler);
#endif //WIN_CTP

	m_bManualTrade = true;
	CKeyValue GlobalKeyValue;
	string key, value;

	CreateObjects();
	m_pTradeDb->Init();

	cerr << m_Prompt << "从全局配置数据表获取账户信息" << ENDLINE;
	int RetRead = GlobalKeyValue.ReadConfigFromDb(m_pTradeDb, "GlobalConfig");

#ifdef WIN_CTP
	string strConfigDir = ".\\config";
	if(m_ArchName != "") strConfigDir = ".\\config\\" + m_ArchName;
	string strFilePath = strConfigDir + "\\GlobalConfig.txt";
#else
	string strConfigDir = "./config";
	if(m_ArchName != "") strConfigDir = "./config/" + m_ArchName;
	string strFilePath = strConfigDir + "/GlobalConfig.txt";
#endif
	if(RetRead <= 0) GlobalKeyValue.ReadConfigFromTextFile(strFilePath);

	//获取交易柜台信息
	string strCount;
	char tmpString[10];
	string CounterName;
	vector<string> CounterNameList;
	size_t CounterSize = GlobalKeyValue.InputIntKeyValue("CounterSize");
	for(size_t i = 0; i < CounterSize; i++)
	{
		sprintf(tmpString, "%d", i);
		strCount = tmpString;
		strCount = RemoveSpace(strCount);
		key = "CounterName" + strCount;
		CounterName = GlobalKeyValue.InputStringKeyValue(key);
		CounterNameList.push_back(CounterName);
	}

	//文件下单手工交易只可能有一个柜台
	if(CounterSize == 1)
	{
		CounterName = CounterNameList[0];
	}
	else if(CounterSize > 1)
	{
		size_t CounterIndex = 0;
		cerr << m_Prompt << "输入柜台索引 CounterIndex >"; cin >> CounterIndex;

		if(CounterIndex >= CounterSize)
			CounterName = CounterNameList[CounterSize-1];
		else if(CounterIndex <= 0)
			CounterName = CounterNameList[0];
		else
			CounterName = CounterNameList[CounterIndex];
	}
	else if(CounterSize <= 0)
	{
		cerr << m_Prompt << "输入柜台名称 CounterName >"; cin >> CounterName;
		RemoveSpace(CounterName);
	}

	m_pCtpCntIfList.clear();
	CCtpCntIf* pCtpCntIf = new CCtpCntIf(this, CounterName);
	pCtpCntIf->GetConfigFromDb();
	m_pCtpCntIfList.push_back(pCtpCntIf);

	//手动交易，需要设置m_PriceTickMultiple
	m_ManTradePriceTickMultiple = GlobalKeyValue.InputIntKeyValue("ManTradePriceTickMultiple");

	//将配置信息备份回数据库
	GlobalKeyValue.WriteConfigToDb(m_pTradeDb, "GlobalConfig");

	m_pCtpCntIfList[0]->InitCtpApi(THOST_TERT_RESUME);
	ShowManualTradeCommand(this, true);

	//为避免报错，先要清除主动操作的各种对象
	DestroyObjects();
	m_pCtpCntIfList[0]->CleanupCtpApi();
}

//++
//控制台事件处理函数
//--
#ifdef WIN_CTP
BOOL WINAPI CRealApp::ConsoleHandler(DWORD CEvent)
{
	switch(CEvent)
	{
		case CTRL_C_EVENT:            //按下CTRL + C键
		case CTRL_BREAK_EVENT:        //按下CTRL+BREAK键
		case CTRL_CLOSE_EVENT:        //关闭控制台窗口
		case CTRL_LOGOFF_EVENT:       //用户退出!
		case CTRL_SHUTDOWN_EVENT:     //用户关机
			//设置程序退出标志并等待各线程结束	
			pRealApp->m_bForceCancelOrder = true;
			SleepMs(2000);
			pRealApp->m_bExitApp = true;

			if(pRealApp->m_hDataThread != NULL) 
				WaitForSingleObject(pRealApp->m_hDataThread, WAIT_THREAD_FINISH);
			if(pRealApp->m_hTradeThread != NULL) 
				WaitForSingleObject(pRealApp->m_hTradeThread, WAIT_THREAD_FINISH);
			if(pRealApp->m_hMdThread != NULL)
				WaitForSingleObject(pRealApp->m_hMdThread, WAIT_THREAD_FINISH);

			//清理CTP API和销毁各种动态生成的对象
			SleepMs(10);  //等待各种清理工作完成
			pRealApp->DestroyObjects();
			for(size_t i = 0; i < pRealApp->m_pCtpCntIfList.size(); i++)
			{
				pRealApp->m_pCtpCntIfList[i]->CleanupCtpApi();
			}

			if(pRealApp->m_bManualTrade == true)
			{
				MessageBoxA(NULL,"交易程序将被强制关闭!", "signal", MB_OK);
			}
			else
			{
				cerr << "按CTRL+C/Break, 或者其它系统原因导致程序退出！" << ENDLINE;
			}
			exit(1);

			break;
		default:
			cerr << "不明控制台消息！" << ENDLINE;
	}
	return TRUE;
}
#else  //Linux
void CRealApp::ConsoleHandler(int signo)
{
	switch(signo)
	{
		case SIGINT:            //按下CTRL + C键
		case SIGHUP:            //关闭控制台窗口
		case SIGQUIT:       
		case SIGTERM:     
		case SIGKILL:
			//设置程序退出标志并等待各线程结束	
			pRealApp->m_bForceCancelOrder = true;
			SleepMs(2000);
			pRealApp->m_bExitApp = true;

		    WaitForSingleObject(pRealApp->m_hDataThread, WAIT_THREAD_FINISH);
		    WaitForSingleObject(pRealApp->m_hTradeThread, WAIT_THREAD_FINISH);
		    WaitForSingleObject(pRealApp->m_hMdThread, WAIT_THREAD_FINISH);

			//清理CTP API和销毁各种动态生成的对象
			SleepMs(10);  //等待各种清理工作完成
			pRealApp->DestroyObjects();
			for(size_t i = 0; i < pRealApp->m_pCtpCntIfList.size(); i++)
			{
				pRealApp->m_pCtpCntIfList[i]->CleanupCtpApi();
			}

			if(pRealApp->m_bManualTrade == true)
			{
				cerr << "交易程序将被强制关闭!" << ENDLINE;
			}
			else
			{
				cerr << "按CTRL+C/Break, 或者其它系统原因导致程序退出！" << ENDLINE;
			}
			exit(1);

			break;
		default:
			cerr << "不明控制台消息！" << ENDLINE;
	}

	return;
}
#endif // WIN_CTP

//++
//行情处理线程，用于更新T型报价并设置相关事件
//--
#ifdef WIN_CTP
unsigned __stdcall CRealApp::MarketDataProc(LPVOID param)
{
	CRealApp *pReal = (CRealApp*)param;
	double PreHbSecs = 0;

	SleepMs(10);
	while(true) 
	{
		if(pReal->m_bExitApp) break;

		bool bGetMd = pReal->SyncDepthMarketData();
		if(bGetMd == true)
		{
			SetEvent(pReal->m_hMdToDataProc);
			//SetEvent(pReal->m_hMdToTradeEngine);
			SetEvent(pReal->m_hMdToStrategyScan);
		}

		//给出HeartBeat信息, 表示线程还在活动
		string strPrefix = "MuxCnt " + pReal->m_Prompt + " 行情处理线程";
		HeartBeatIndicate(PreHbSecs, THREAD_ACTIVE_INDIC, strPrefix);
	}

	return 1;
}
#else //Linux
void* CRealApp::MarketDataProc(void *param)
{
	CRealApp *pReal = (CRealApp*)param;
	double PreHbSecs = 0;

	//设置线程为高优先级
	pReal->SetCurrentThreadHighPriority(true);

	SleepMs(10);
	while(true) 
	{
		if(pReal->m_bExitApp)
		{
            SetEvent(pReal->m_hMdThread);
		    break;
		}

		bool bGetMd = pReal->SyncDepthMarketData();
		if(bGetMd == true)
		{
			SetEvent(pReal->m_hMdToDataProc);
			//SetEvent(pReal->m_hMdToTradeEngine);
			SetEvent(pReal->m_hMdToStrategyScan);
		}

		//给出HeartBeat信息, 表示线程还在活动
		string strPrefix = "MuxCnt " + pReal->m_Prompt + " 行情处理线程";
		HeartBeatIndicate(PreHbSecs, THREAD_ACTIVE_INDIC, strPrefix);
	}

	return NULL;
}
#endif  //WIN_CTP


//++
//进行数据处理的线程函数, 主要用于收集统计数据和保存交易组合信息
//将此过程和交易过程解耦将使交易反应更为迅速
//对于行情统计数据而言, 读取旧数据并不在乎, 所以不用加锁
//--
#ifdef WIN_CTP
unsigned __stdcall CRealApp::TradeDataProc(LPVOID param)
{
	CRealApp *pReal = (CRealApp*)param;
	double PreHbSecs = 0;

	SleepMs(10);
	while(true) 
	{
		if(pReal->m_bExitApp) break;

		DWORD waitReturn = WaitForSingleObject(pReal->m_hMdToDataProc, MD_WAIT_TIME); 
		ResetEvent(pReal->m_hMdToDataProc); 

		//更新各个策略的统计数据
		if(waitReturn == WAIT_OBJECT_0)
		{
			pReal->m_pStrategysManage->UpdateStat();
		}

		//后处理，主要是存储交易信息到数据库
		pReal->PostProcessing(false);

		//清除已经平仓且不需要保存的组合, 需要迅速处理
		pReal->m_pStrategysManage->PurgeClosedPortfolio();

		//给出HeartBeat信息, 表示线程还在活动
		string strPrefix = "MuxCnt " + pReal->m_Prompt + " 数据处理线程";
		HeartBeatIndicate(PreHbSecs, THREAD_ACTIVE_INDIC, strPrefix);
	}

	return 1;
}
#else  //Linux
void* CRealApp::TradeDataProc(void* param)
{
	CRealApp *pReal = (CRealApp*)param;
	double PreHbSecs = 0;

	//设置线程为高优先级
	pReal->SetCurrentThreadHighPriority(true);

	SleepMs(10);
	while(true) 
	{
		if(pReal->m_bExitApp)
		{
			SetEvent(pReal->m_hDataThread);
	        break;
		}

		DWORD waitReturn = WaitForSingleObject(pReal->m_hMdToDataProc, MD_WAIT_TIME); 
		ResetEvent(pReal->m_hMdToDataProc); 

		//更新各个策略的统计数据
		if(waitReturn == WAIT_OBJECT_0)
		{
			pReal->m_pStrategysManage->UpdateStat();
		}

		//后处理，主要是存储交易信息到数据库
		pReal->PostProcessing(false);

		//清除已经平仓且不需要保存的组合, 需要迅速处理
		pReal->m_pStrategysManage->PurgeClosedPortfolio();

		//给出HeartBeat信息, 表示线程还在活动
		string strPrefix = "MuxCnt " + pReal->m_Prompt + " 数据处理线程";
		HeartBeatIndicate(PreHbSecs, THREAD_ACTIVE_INDIC, strPrefix);
	}

	return NULL;
}
#endif //WIN_CTP

//++
//交易引擎线程函数
//采用轮询方式获取行情数据(20ms)
//报单和成交信息的获取是轮询与事件驱动相结合
//--
#ifdef WIN_CTP
unsigned __stdcall CRealApp::TradeEngine(LPVOID param)
{
	CRealApp *pReal = (CRealApp*)param;
    pReal->m_bTradeEngineError = false;
	pReal->m_bForceCancelOrder = false;
	double PreHbSecs = 0;

	size_t CounterSize = pReal->m_pCtpCntIfList.size();
	CEventHandle *hTraderSpiEvent = new CEventHandle[CounterSize];
	for(size_t i = 0; i < CounterSize; i++)
	{
		hTraderSpiEvent[i] = pReal->m_pCtpCntIfList[i]->m_hTraderSpiEvent;
	}

	SleepMs(10);
	while(true) 
	{
		if(pReal->m_bExitApp) break;

		//等待交易API的报单和成交信息, 超时时间20ms
		DWORD nIndex = WaitForMultipleObjects(CounterSize, hTraderSpiEvent, FALSE, 20);
		for(size_t i = 0; i < CounterSize; i++)
		{
			ResetEvent(hTraderSpiEvent[i]); 
		}

		//测试用
//#define TIME_MEASURE
#ifdef TIME_MEASURE
		SYSTEMTIME CurTime;
		GetLocalTime(&CurTime);
		double BeginSecs = CurTime.wHour*3600.0 + CurTime.wMinute*60.0 + CurTime.wSecond + CurTime.wMilliseconds*0.001;
#endif

		//刷新报单和成交信息列表
		pReal->m_pPosMoneyManage->RefreshOrderAndTradeList();

		//处理FISH单(行情和报单/成交回应驱动其工作)
		bool bForceCancel = (pReal->m_bForceCancelOrder || pReal->m_bAutoTradeErr) ? true : false;
		int ret = pReal->m_pStrategysManage->FishOrderProc(bForceCancel);

		//交易确认
		ret |= pReal->m_pStrategysManage->TradeAcknowledge();

		if(ret != 0) pReal->m_bTradeEngineError = true;

		//给出HeartBeat信息, 表示线程还在活动
		string strPrefix = "MuxCnt " + pReal->m_Prompt + " 交易引擎线程";
		HeartBeatIndicate(PreHbSecs, THREAD_ACTIVE_INDIC, strPrefix);

		//测试用
#ifdef TIME_MEASURE
		GetLocalTime(&CurTime);
		double EndSecs = CurTime.wHour*3600.0 + CurTime.wMinute*60.0 + CurTime.wSecond + CurTime.wMilliseconds*0.001;
		cerr << "交易引擎计算时间 = " << (EndSecs - BeginSecs) << ENDLINE;
#endif
	}

	delete[] hTraderSpiEvent;
	return 1;
}
#else  //Linux
void* CRealApp::TradeEngine(void* param)
{
	CRealApp *pReal = (CRealApp*)param;
    pReal->m_bTradeEngineError = false;
	pReal->m_bForceCancelOrder = false;
	double PreHbSecs = 0;

	//设置线程为高优先级
	pReal->SetCurrentThreadHighPriority(true);

	size_t CounterSize = pReal->m_pCtpCntIfList.size();
	CEventHandle *hTraderSpiEvent = new CEventHandle[CounterSize];
	for(size_t i = 0; i < CounterSize; i++)
	{
		hTraderSpiEvent[i] = pReal->m_pCtpCntIfList[i]->m_hTraderSpiEvent;
	}

	SleepMs(10);
	while(true) 
	{
		if(pReal->m_bExitApp)
		{
			SetEvent(pReal->m_hTradeThread);
			break;
		}

		//等待交易API的报单和成交信息, 超时时间20ms
		DWORD nIndex = WaitForMultipleObjects(CounterSize, hTraderSpiEvent, FALSE, 20);
		for(size_t i = 0; i < CounterSize; i++)
		{
			ResetEvent(hTraderSpiEvent[i]); 
		}

		//测试用
//#define TIME_MEASURE
#ifdef TIME_MEASURE
		SYSTEMTIME CurTime;
		GetLocalTime(&CurTime);
		double BeginSecs = CurTime.wHour*3600.0 + CurTime.wMinute*60.0 + CurTime.wSecond + CurTime.wMilliseconds*0.001;
#endif

		//刷新报单和成交信息列表
		pReal->m_pPosMoneyManage->RefreshOrderAndTradeList();

		//处理FISH单(行情和报单/成交回应驱动其工作)
		bool bForceCancel = (pReal->m_bForceCancelOrder || pReal->m_bAutoTradeErr) ? true : false;
		int ret = pReal->m_pStrategysManage->FishOrderProc(bForceCancel);

		//交易确认
		ret |= pReal->m_pStrategysManage->TradeAcknowledge();

		if(ret != 0) pReal->m_bTradeEngineError = true;

		//给出HeartBeat信息, 表示线程还在活动
		string strPrefix = "MuxCnt " + pReal->m_Prompt + " 交易引擎线程";
		HeartBeatIndicate(PreHbSecs, THREAD_ACTIVE_INDIC, strPrefix);

		//测试用
#ifdef TIME_MEASURE
		GetLocalTime(&CurTime);
		double EndSecs = CurTime.wHour*3600.0 + CurTime.wMinute*60.0 + CurTime.wSecond + CurTime.wMilliseconds*0.001;
		cerr << "交易引擎计算时间 = " << (EndSecs - BeginSecs) << ENDLINE;
#endif
	}

	delete[] hTraderSpiEvent;
	return NULL;
}

//设置当前线程为高优先级
void CRealApp::SetCurrentThreadHighPriority(bool bSet) 
{
  // Start out with a standard, low-priority setup for the sched params.
  struct sched_param sp;
  bzero((void*)&sp, sizeof(sp));
  int policy = SCHED_OTHER;
 
  // If desired, set up high-priority sched params structure.
  if (bSet) {
    // RR scheduler, ranked above default SCHED_OTHER queue
    policy = SCHED_RR;
    // The priority only compares us to other SCHED_RR threads, so we
    // just pick a random priority halfway between min & max.
    const int priority = (sched_get_priority_max(policy) + sched_get_priority_min(policy)) / 2;
 
    sp.sched_priority = priority;
  }
 
  // Actually set the sched params for the current thread.
  if (0 != pthread_setschedparam(pthread_self(), policy, &sp)) {
    printf("将线程号为 #%u 的线程设置为高优先级线程出错，请使用超级用户运行程序!\r\n", pthread_self());
  }
}
#endif //WIN_CTP

//处理退出GUI程序的情况
void HandleExitGui(int signo)
{
	switch(signo)
	{
		case SIGINT:            //按下CTRL + C键
		case SIGHUP:            //关闭控制台窗口
		case SIGQUIT:       
		case SIGTERM:     
		case SIGKILL:
			//设置程序退出标志并等待各线程结束	
			pRealApp->m_bExitApp = true;
			SleepMs(1000);

		    WaitForSingleObject(pRealApp->m_hMdThread, WAIT_THREAD_FINISH);

			//清理CTP API和销毁各种动态生成的对象
			SleepMs(10);  //等待各种清理工作完成
			pRealApp->DestroyObjects();
			for(size_t i = 0; i < pRealApp->m_pCtpCntIfList.size(); i++)
			{
				pRealApp->m_pCtpCntIfList[i]->CleanupCtpApi();
			}

		    cerr << "按CTRL+C/Break, 或者其它系统原因导致程序退出！" << ENDLINE;
			exit(1);

			break;
		default:
			cerr << "不明控制台消息！" << ENDLINE;
	}

	return;
}

