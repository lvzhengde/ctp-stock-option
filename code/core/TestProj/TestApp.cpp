#include "TestApp.h"
#include "AppConfig.h"
#include "BrokerServer.h"
#include "TradeDb.h"
#include "KeyValue.h"


//定义指针供类中的静态函数使用
//只有在单柜台系统中才用
CTestApp* pTestApp;

CTestApp::CTestApp(string strArchName)
{
	string strCmd;

	//设置柜台系统名称
	m_ArchName = RemoveSpace(strArchName);

#ifdef WIN_CTP
	string strConfigDir = ".\\config";
	if(m_ArchName != "") strConfigDir = ".\\config\\" + m_ArchName;
#else
	string strConfigDir = "./config";
	if(m_ArchName != "") strConfigDir = "./config/" + m_ArchName;
#endif
	if(m_ArchName != "" && access(strConfigDir.c_str(), 0) != 0)
	{
		strCmd = "mkdir " + strConfigDir;
		system(strCmd.c_str());
	}

	m_requestId = 0;
	pTestApp = this;

	m_bNeedAuth = false;     
	m_UserProductInfo = ""; 
	m_AppID = "";
	m_AuthCode = "";     
	m_pTradeDb = NULL;

	//没有初始化之前各个指针都强行置空
	m_pMdUserApi = NULL;
	m_pMdUserSpi = NULL; 
	m_pTraderUserApi = NULL;
	m_pTraderUserSpi = NULL;
}

CTestApp::~CTestApp()
{
#ifndef WIN_CTP
    if(m_hEvent != NULL) delete m_hEvent;
#endif
	if(m_pTradeDb != NULL)
	{
		m_pTradeDb->Destroy();
		delete m_pTradeDb;
		m_pTradeDb = NULL;
	}
}

//++
//将测试中产生的报单信息和成交信息都写入数据库
//--
void CTestApp::SaveInfoToDatabase()
{
	if(m_pTradeDb == NULL) return;
	CTradeDb* pTradeDb = m_pTradeDb; 

    string strTableName;
	string strFieldDesc;
	string strSQLCmd;

	//创建报单数据表
	strTableName = m_CounterName + "_OrderLog";
	strFieldDesc = "(ID INTEGER PRIMARY KEY AUTOINCREMENT, OrderSysID VARCHAR, InstrumentID VARCHAR, Direction VARCHAR, CombOffsetFlag VARCHAR, CombHedgeFlag VARCHAR, OrderSubmitStatus VARCHAR, \
				   OrderStatus VARCHAR, LimitPrice DOUBLE, VolumeTotalOriginal INT, VolumeTotal INT, VolumeTraded INT, StatusMsg VARCHAR, InsertDate VARCHAR, InsertTime VARCHAR, \
				   ExchangeID VARCHAR, BrokerID VARCHAR, InvestorID VARCHAR)";
	int Ret = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
	if(Ret < 0) 
	{
		cerr << "数据表 " << strTableName << " 无法创建或者无法访问，程序返回！" << ENDLINE;
        PressAnyKeyToExit();
		return;
	}

	//将报单信息插入数据表
	vector<CThostFtdcOrderField> tmpOrderList;
	tmpOrderList.assign(m_pTraderUserSpi->m_orderList.begin(), m_pTraderUserSpi->m_orderList.end());

	cerr << "测试报单信息写入数据库..." << ENDLINE;
	for(size_t i = 0; i < tmpOrderList.size(); i++)
	{
		stringstream sstr;
		sstr.clear(); sstr.str(""); sstr.precision(18);

		sstr << "'" << tmpOrderList[i].OrderSysID << "', '" << tmpOrderList[i].InstrumentID << "', '" << ToNoZeroChar(tmpOrderList[i].Direction) << "', '" << tmpOrderList[i].CombOffsetFlag 
			<< "', '" << tmpOrderList[i].CombHedgeFlag << "', '"<< ToNoZeroChar(tmpOrderList[i].OrderSubmitStatus) << "', '" << ToNoZeroChar(tmpOrderList[i].OrderStatus) <<  "', " << tmpOrderList[i].LimitPrice
			<< ", " << tmpOrderList[i].VolumeTotalOriginal << ", " << tmpOrderList[i].VolumeTotal << ", " << tmpOrderList[i].VolumeTraded << ", '" << StatusMessageCodeConvert(tmpOrderList[i].StatusMsg, true) 
			<< "', '" << tmpOrderList[i].InsertDate << "', '" << tmpOrderList[i].InsertTime << "', '" << tmpOrderList[i].ExchangeID << "', '" << tmpOrderList[i].BrokerID << "', '"
			<< tmpOrderList[i].InvestorID << "'" ;

		strSQLCmd = "INSERT INTO " + strTableName + "(OrderSysID, InstrumentID, Direction, CombOffsetFlag, CombHedgeFlag, OrderSubmitStatus, OrderStatus, \
													 LimitPrice, VolumeTotalOriginal, VolumeTotal, VolumeTraded, StatusMsg, InsertDate, InsertTime, ExchangeID, \
													 BrokerID, InvestorID) VALUES(" + sstr.str() + ")";

		bool bExecuteSql = pTradeDb->ExecuteSqlCommand(strSQLCmd);
		if(bExecuteSql == false)
			cerr << "插入报单信息到 OrderLog 数据表出错！" << ENDLINE;
	}

	//创建成交数据表
	strTableName = m_CounterName + "_TradeLog";
	strFieldDesc = "(ID INTEGER PRIMARY KEY AUTOINCREMENT, TradeID VARCHAR, InstrumentID VARCHAR, Direction VARCHAR, OffsetFlag VARCHAR, HedgeFlag VARCHAR, Price DOUBLE, \
				   Volume INT, TradeDate VARCHAR, TradeTime VARCHAR, OrderSysID VARCHAR, TradeType VARCHAR,  \
				   ExchangeID VARCHAR, BrokerID VARCHAR, InvestorID VARCHAR)";
	Ret = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
	if(Ret < 0) 
	{
		cerr << "数据表 " << strTableName << " 无法创建或者无法访问，程序返回！" << ENDLINE;
		PressAnyKeyToExit();
		return;
	}

	//将成交信息插入数据表
	vector<CThostFtdcTradeField> tmpTradeList;
	tmpTradeList.assign(m_pTraderUserSpi->m_tradeList.begin(), m_pTraderUserSpi->m_tradeList.end());

	cerr << "测试成交信息写入数据库..." << ENDLINE;
	for(size_t i = 0; i < tmpTradeList.size(); i++)
	{
		stringstream sstr;
		sstr.clear(); sstr.str(""); sstr.precision(18);

		sstr << "'" << tmpTradeList[i].TradeID << "', '" << tmpTradeList[i].InstrumentID << "', '" << ToNoZeroChar(tmpTradeList[i].Direction) << "', '" << ToNoZeroChar(tmpTradeList[i].OffsetFlag) 
			<< "', '" << ToNoZeroChar(tmpTradeList[i].HedgeFlag) << "', "<< tmpTradeList[i].Price << ", " << tmpTradeList[i].Volume << ", '" << tmpTradeList[i].TradeDate
			<< "', '" << tmpTradeList[i].TradeTime << "', '" << tmpTradeList[i].OrderSysID << "', '" << ToNoZeroChar(tmpTradeList[i].TradeType) << "', '" << tmpTradeList[i].ExchangeID 
			<< "', '" << tmpTradeList[i].BrokerID << "', '" << tmpTradeList[i].InvestorID << "'" ;

		strSQLCmd = "INSERT INTO " + strTableName + "(TradeID, InstrumentID, Direction, OffsetFlag, HedgeFlag, Price, Volume, \
													 TradeDate, TradeTime, OrderSysID, TradeType, ExchangeID, BrokerID, InvestorID) VALUES(" + sstr.str() + ")";

		bool bExecuteSql = pTradeDb->ExecuteSqlCommand(strSQLCmd);
		if(bExecuteSql == false)
			cerr << "插入报单信息到 TradeLog 数据表出错！" << ENDLINE;
	}
}

void CTestApp::ShowTraderCommand(CTestTraderSpi* p, bool print)
{ 
	if(print)
	{
		cerr << "---------------------------------------------------------------" << ENDLINE;
		cerr << " [1]  ReqUserLogin                      -- 登录" << ENDLINE;
		cerr << " [2]  ReqSettlementInfoConfirm          -- 结算单确认" << ENDLINE;
		cerr << " [3]  ReqQryInstrument                  -- 查询合约" << ENDLINE;
		cerr << " [4]  ReqQryTradingAccount              -- 查询资金" << ENDLINE;
		cerr << " [5]  ReqQryInvestorPosition            -- 查询持仓" << ENDLINE;
		cerr << " [6]  ReqOrderInsert                    -- 报单" << ENDLINE;
		cerr << " [7]  ReqOrderAction                    -- 撤单" << ENDLINE;
		cerr << " [8]  ReqQryInstrumentMarginRate        -- 查询期货保证金率" << ENDLINE;
		cerr << " [9]  ReqQryInstrumentCommissionRate    -- 查询期货合约手续费率" << ENDLINE;
		cerr << " [a]  ReqQryOptionInstrCommRate         -- 查询期货期权合约手续费率" << ENDLINE;  //主要是查询期权行权费用
#ifndef FUTURE_OPTION_ONLY
		cerr << " [b]  ReqQryETFOptionInstrCommRate      -- 查询ETF期权合约手续费率" << ENDLINE;  
#endif
		cerr << " [c]  ReqQryBrokerTradingParams         -- 查询经纪公司交易参数" << ENDLINE;
		cerr << " [d]  ReqQryOptionInstrTradeCost        -- 查询期货期权交易成本" << ENDLINE;
		cerr << " [e]  ReqExecOrderInsert                -- 期权行权录入请求" << ENDLINE;
		cerr << " [f]  ReqExecOrderAction                -- 期权行权撤单请求" << ENDLINE;
		cerr << " [g]  ReqQryOrder                       -- 报单查询请求" << ENDLINE;
		cerr << " [h]  ReqQryTrade                       -- 成交查询请求" << ENDLINE;
		cerr << " [i]  PrintOrders                       -- 显示报单" << ENDLINE;
		cerr << " [j]  PrintTrades                       -- 显示成交" << ENDLINE;
#ifndef FUTURE_OPTION_ONLY
		cerr << " [k]  ReqLockInsert                     -- 备兑证券锁定/解锁" << ENDLINE;
		cerr << " [l]  ReqQryLock                        -- 请求查询锁定" << ENDLINE;
		cerr << " [m]  ReqQryLockPosition                -- 请求查询锁定证券仓位" << ENDLINE;
#else
		cerr << " [o]  ReqOptionSelfCloseInsert          -- 期权自对冲录入请求" << ENDLINE;
		cerr << " [p]  ReqOptionSelfCloseAction          -- 期权自对冲操作请求" << ENDLINE;
		cerr << " [q]  ReqQryOptionSelfClose             -- 请求查询期权自对冲" << ENDLINE;
#endif
		cerr << " [s]  OnRtnInstrumentStatus             -- 显示合约状态信息改变" << ENDLINE;
		cerr << " [0]  Exit                              -- 退出" << ENDLINE;
		cerr << "--------------------------------------------------------------" << ENDLINE;
	}   

	//由于创建时可能处于有信号状态，需要一开始就就将其复位为无信号状态
	ResetEvent(m_hEvent);

	TThostFtdcInstrumentIDType    instId;
	TThostFtdcDirectionType       dir;
	TThostFtdcCombOffsetFlagType  kpp;
	TThostFtdcPriceType           price;
	TThostFtdcVolumeType          vol;
	char                          TimeCondition;
	char                          VolumeCondition;
	char                          MarketOrderType;
    TThostFtdcHedgeFlagType       HedgeFlag;
	TThostFtdcSequenceNoType      orderSeq;
	TThostFtdcExchangeIDType      exchangeId;
	TThostFtdcTradeIDType         tradeId;
	TThostFtdcSequenceNoType	  brokerExecOrderSeq;

	char cmd;  cin>>cmd;
	switch(cmd){
		case '1': 
			{
				p->ReqUserLogin(m_brokerId, m_userId, m_Password); 
				break;
			}
		case '2': 
			{
				p->ReqSettlementInfoConfirm(); 
				break;
			}
		case '3': 
			{
				cerr<<" 合约 > "; cin>>instId; 
				if(strcmp(instId, "*") == 0) memset(instId, 0, sizeof(TThostFtdcInstrumentIDType));
				p->ReqQryInstrument(instId); 
				break;
			}
		case '4': 
			{
				p->ReqQryTradingAccount(); 
				break;
			}
		case '5': 
			{
				cerr<<" 合约 > "; cin>>instId; 
				if(strcmp(instId, "*") == 0) memset(instId, 0, sizeof(TThostFtdcInstrumentIDType));
				p->ReqQryInvestorPosition(instId); 
				break;
			}
		case '6': 
			{
				string strExchangeID;
				cerr<<" 合约 > "; cin>>instId; 
				cerr<<" 方向 > "; cin>>dir; 
				cerr<<" 开平 > "; cin>>kpp;
				cerr<<" 价格 > "; cin>>price;
				cerr<<" 数量 > "; cin>>vol;  
				cerr<<"有效期类型I(OC)/G(FD) > "; cin>>TimeCondition;
				cerr<<"成交量类型C(omplete)/A(ny) > "; cin>>VolumeCondition;
				cerr<<"市价单类型A(nyPrice)/B(estPrice)/F(iveLevel) > "; cin>>MarketOrderType;
				cerr<<"投机套保标志(投机: '1'/备兑: '4') > "; cin>>HedgeFlag;
				cerr<<"交易所代码 >"; cin >> strExchangeID;
				if(strExchangeID != "SSE" && strExchangeID != "SZSE" && strExchangeID != "DCE" && strExchangeID != "CZCE" 
					&& strExchangeID != "SHFE" && strExchangeID != "CFFEX")
				{
					strExchangeID = "";
				}
				p->ReqOrderInsert(instId, dir, kpp, price, vol, TimeCondition, VolumeCondition, MarketOrderType, HedgeFlag, strExchangeID); 
				break;
			}
		case '7': 
			{ 
				cerr<<" 经纪商报单序号 > "; cin>>orderSeq;
				p->ReqOrderAction(orderSeq);
				break;
			}
		case '8':
			{
				cerr << "合约 > "; cin >> instId;
				p->ReqQryInstrumentMarginRate(instId);
				break;
			}
		case '9':
			{
				cerr << "合约 > "; cin >> instId;
				p->ReqQryInstrumentCommissionRate(instId);
				break;
			}
		case 'a':
			{
				cerr << "期权合约 > "; cin >> instId;
				p->ReqQryOptionInstrCommRate(instId);
				break;
			}
#ifndef FUTURE_OPTION_ONLY
		case 'b':
			{
				cerr << "ETF期权合约 > "; cin >> instId;
				p->ReqQryETFOptionInstrCommRate(instId);
				break;
			}
#endif
		case 'c':
			{
				p->ReqQryBrokerTradingParams();
				break;
			}
		case 'd':
			{
				double inputPrice, underlyingPrice;
				cerr << "期权合约 > "; cin >> instId;
				cerr << "期权权利金价格 > "; cin >> inputPrice;
				cerr << "标的合约价格 > "; cin >> underlyingPrice;
				p->ReqQryOptionInstrTradeCost(instId, inputPrice, underlyingPrice);
				break;
			}
		case 'e':
			{
				TThostFtdcVolumeType	 volume; 
		        //TThostFtdcPosiDirectionType posiDirection;  //#define THOST_FTDC_PD_Long '2', #define THOST_FTDC_PD_Short '3'
				cerr << "期权合约 > "; cin >> instId;
				cerr << "交易所代码 > "; cin >> exchangeId;
				cerr << "行权数量 > "; cin >> volume;
				//cerr << "持仓多空方向(多: '2', 空: '3')> "; cin >> posiDirection; //所有要行权的期权都是多头方向'2'
				p->ReqExecOrderInsert(instId, exchangeId, volume); //, posiDirection);
				break;
			}
		case 'f':
			{
				cerr<<" 经纪商期权行权报单序号 > "; cin>>brokerExecOrderSeq;
				p->ReqExecOrderAction(brokerExecOrderSeq);
				break;
			}
		case 'g':
			{
				CThostFtdcOrderField *pOrder = NULL;

				//期货交易所的OrderSysID用手工输入的可能不对
				cerr<<"经纪商报单序号 > "; cin>>orderSeq;
				bool found=false; size_t i=0;
				for(i=0;i<p->m_orderList.size();i++){
					string strOrderSysID = p->m_orderList[i].OrderSysID;
					RemoveSpace(strOrderSysID);
					if(p->m_orderList[i].BrokerOrderSeq == orderSeq){
						pOrder = &p->m_orderList[i];
						found = true; 
					}
				}

				if(found == true)
				{
					p->ReqQryOrder(pOrder);
				}
				else
				{
					cerr << "经纪商报单序号不存在！"; 
					SetEvent(m_hEvent);
				}
			
				break;
			}
		case 'h':
			{
				cerr << "合约代码 > "; cin >> instId;
				cerr << "交易所代码 > "; cin >> exchangeId;
				cerr << "成交编号TradeID > "; cin >> tradeId;
				p->ReqQryTrade(instId, exchangeId, tradeId);
				break;
			}
		case 'i': 
			{
				p->PrintOrders();
				break;
			}
		case 'j': 
			{
				p->PrintTrades();
				break;
			}
#ifndef FUTURE_OPTION_ONLY
		case 'k':
			{
		        //TThostFtdcLockTypeType 锁定: THOST_FTDC_LCKT_Lock '1'; 解锁: THOST_FTDC_LCKT_Unlock '2'
				TThostFtdcLockTypeType LockType;
				TThostFtdcVolumeType	 volume; 
				cerr << "证券代码 > "; cin >> instId;
				cerr << "交易所代码 > "; cin >> exchangeId;
				cerr << "操作方向(锁定 '1'/解锁 '2') > "; cin >> LockType;
				cerr << "操作数量 > "; cin >> volume;
				p->ReqLockInsert(instId, exchangeId, LockType, volume);
				break;
			}
		case 'l':
			{
				cerr << "证券代码 > "; cin >> instId;
				cerr << "交易所代码 > "; cin >> exchangeId;
                TThostFtdcOrderSysIDType LockSysID;
				int LockRef;
				cerr<<"锁定/解锁报单序号 > "; cin >> LockRef;
				bool found=false; size_t i=0;
				for(i=0;i<p->m_lockOrderList.size();i++){
					string strLockSysID = p->m_lockOrderList[i].LockSysID;
					RemoveSpace(strLockSysID);
					if(atoi(p->m_lockOrderList[i].LockRef) == LockRef && strLockSysID != ""){ 
						found = true; 
						break;
					}
				}
				if(found == true)
				{
					strcpy(LockSysID, p->m_lockOrderList[i].LockSysID);
				}
				else
				{
					cerr << "锁定/解锁报单编号LockSysID > "; 
					cin >> LockSysID;
				}

				p->ReqQryLock(instId, exchangeId, LockSysID);
				break;
			}
		case 'm':
			{
				cerr << "证券代码 > "; cin >> instId;
				cerr << "交易所代码 > "; cin >> exchangeId;
				p->ReqQryLockPosition(instId, exchangeId);
				break;
			}
#else
		case 'o': 
			{
				TThostFtdcOptSelfCloseFlagType OptSelfCloseFlag;
				cerr <<" 合约 > "; cin >> instId; 
				cerr << "交易所代码 > "; cin >> exchangeId;
				cerr << "数量 > "; cin >> vol; 
				cerr << "自对冲类型 '1': 自对冲期权仓位; '2': 保留期权仓位; '3': 自对冲卖方履约后的期货仓位" <<ENDLINE;
				cerr << "OptSelfCloseFlag >"; cin >> OptSelfCloseFlag;
				p->ReqOptionSelfCloseInsert(instId, exchangeId, vol, OptSelfCloseFlag); 
				break;
			}
		case 'p': 
			{ 
				TThostFtdcSequenceNoType BrokerOptionSelfCloseSeq;
				cerr<<" 经纪公司报单编号 > "; cin>>BrokerOptionSelfCloseSeq;
				p->ReqOptionSelfCloseAction(BrokerOptionSelfCloseSeq);
				break;
			}
		case 'q': 
			{ 
				TThostFtdcSequenceNoType BrokerOptionSelfCloseSeq;
				cerr<<" 经纪公司报单编号 > "; cin>>BrokerOptionSelfCloseSeq;
				p->ReqQryOptionSelfClose(BrokerOptionSelfCloseSeq);
				break;
			}
#endif
		case 's':
			{
				p->m_bInstrumentStatusTest = true;
				break;
			}
		case '0': 
			{
				SaveInfoToDatabase();
				p->ReqUserLogout(m_brokerId, m_userId);
				return; //exit(0); //exit是不太友好的退出
			}
	}  
	WaitForSingleObject(m_hEvent,INFINITE);
	ResetEvent(m_hEvent);
		
	PauseForInput(); 	
	ShowTraderCommand(p, true);
}

void CTestApp::ShowMdCommand(CTestMdSpi* p, bool print)
{
	if(print){
		cerr<<"-----------------------------------------------"<<ENDLINE;
		cerr<<" [1] ReqUserLogin              -- 登录"<<ENDLINE;
		cerr<<" [2] SubscribeMarketData       -- 行情订阅"<<ENDLINE;
		cerr<<" [0] Exit                      -- 退出"<<ENDLINE;
		cerr<<"----------------------------------------------"<<ENDLINE;
	}

	char instIdList[100];

	//由于创建时可能处于有信号状态，需要一开始就就将其复位为无信号状态
	ResetEvent(m_hEvent);

	char cmd;  cin>>cmd;
	switch(cmd){
	case '1': 
		{
			p->ReqUserLogin(m_brokerId, m_userId, m_Password); 
			break;
		}
	case '2': 
		{
			cerr<<" 合约 > "; cin>>instIdList; 
			p->SubscribeMarketData(instIdList); 
			break;
		}
	case '0':
		{
			//p->ReqUserLogout(m_brokerId, m_userId); //CTP不支持行情前置登出功能
			return; //exit(0); //exit是不太友好的退出
		}
	}
	WaitForSingleObject(m_hEvent,INFINITE);
	ResetEvent(m_hEvent);

	PauseForInput();
	ShowMdCommand(p, true);
}

void CTestApp::DumpInstAndMarketData()
{
	string strCmd;

	cerr << "启动交易API线程" << ENDLINE;
	//判断TraderApi流文件目录是否存在，不存在则创建
	if(access("traderapiflow", 0) != 0)
	{
		system("mkdir traderapiflow");
	}

#ifdef WIN_CTP
	string strTradeFlowDir = ".\\traderapiflow";
	if(m_ArchName != "") strTradeFlowDir = ".\\traderapiflow\\" + m_ArchName;
	if(m_ArchName != "" && access(strTradeFlowDir.c_str(), 0) != 0)
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
	if(m_ArchName != "") strTradeFlowDir = "./traderapiflow/" + m_ArchName;
	if(m_ArchName != "" && access(strTradeFlowDir.c_str(), 0) != 0)
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
	m_pTraderUserSpi = new CTestTraderSpi(m_pTraderUserApi, this);
	m_pTraderUserApi->RegisterSpi((CThostFtdcTraderSpi*)m_pTraderUserSpi);	// 注册事件类
	m_pTraderUserApi->SubscribePublicTopic(THOST_TERT_QUICK);				// 注册公有流
	m_pTraderUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);			    // 注册私有流

	//注册多个交易托管服务器前置地址
	CBrokerServer BrokerServer(m_CounterName, m_ArchName);
	for(size_t i = 0; i < BrokerServer.m_TraderFrontList.size(); i++)
	{
		char TraderFront[200];
		strcpy(TraderFront, BrokerServer.m_TraderFrontList[i].c_str());
		m_pTraderUserApi->RegisterFront(TraderFront);
	}
  
	m_pTraderUserApi->Init();
	cerr << "--->>> " << "Initialing UserApi" << ENDLINE;

	cerr << "启动行情API线程" << ENDLINE;
	//判断MdApi流文件目录是否存在，不存在则创建
	if(access("mdapiflow", 0) != 0)
	{
		system("mkdir mdapiflow");
	}

#ifdef WIN_CTP
	string strMdFlowDir = ".\\mdapiflow";
	if(m_ArchName != "") strMdFlowDir = ".\\mdapiflow\\" + m_ArchName;
	if(m_ArchName != "" && access(strMdFlowDir.c_str(), 0) != 0)
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
	if(m_ArchName != "") strMdFlowDir = "./mdapiflow/" + m_ArchName;
	if(m_ArchName != "" && access(strMdFlowDir.c_str(), 0) != 0)
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
	m_pMdUserSpi=new CTestMdSpi(m_pMdUserApi, this);      //创建回调处理类对象MdSpi
	m_pMdUserApi->RegisterSpi(m_pMdUserSpi);		   // 回调对象注入接口类

	//注册多个行情托管服务器前置地址
	for(size_t i = 0; i < BrokerServer.m_MdFrontList.size(); i++)
	{
		char MdFront[200];
		strcpy(MdFront, BrokerServer.m_MdFrontList[i].c_str());
		m_pMdUserApi->RegisterFront(MdFront);
	}

	m_pMdUserApi->Init(); 					   //接口线程启动, 开始工作
	cerr << "--->>> " << "Initialing MdApi" << ENDLINE;

	SleepMs(20000);

	//登录交易服务器，上海中期模拟
	TThostFtdcBrokerIDType	    BrokerId = "8000";
	TThostFtdcUserIDType	    userId = "333602580";
	TThostFtdcPasswordType	    passwd = "023994";
	TThostFtdcInstrumentIDType  instId = "m1705"; //"IO1702"; //"cu1702"; //"SR705"; //"m1705";
	//char instIdList[100] = "m1705";

	//中信期货模拟
	//TThostFtdcBrokerIDType	    BrokerId = "66666";
	//TThostFtdcUserIDType	    userId = "903936";
	//TThostFtdcPasswordType	    passwd = "023994";
	//TThostFtdcInstrumentIDType  instId = "m1705";

	//SimNow模拟
	//TThostFtdcBrokerIDType	    BrokerId = "9999";
	//TThostFtdcUserIDType	    userId = "043657";
	//TThostFtdcPasswordType	    passwd = "878618";
	//TThostFtdcInstrumentIDType  instId = "m1705";

	cerr << "登录交易服务器" << ENDLINE;
	m_pTraderUserSpi->ReqUserLogin(BrokerId, userId, passwd); 
	SleepMs(5000);

	//登录行情服务器
	cerr << "登录行情服务器" << ENDLINE;
	m_pMdUserSpi->ReqUserLogin(BrokerId, userId, passwd);
	SleepMs(5000);

	//获取合约信息
	//cerr << "查询合约信息" << ENDLINE;
 //   m_pTraderUserSpi->ReqQryInstrument(instId);
	//SleepMs(5000);

	////订阅行情
	//cerr << "订阅行情信息" << ENDLINE;
 //   m_pMdUserSpi->SubscribeMarketDataVector();  //订阅标的期货和对应期权行情
	////m_pMdUserSpi->SubscribeMarketData(instId);  //只订阅标的期货行情

	//SleepMs(5000);

    QueryAndSubscribe((void*)m_pTraderUserSpi, (void*)m_pMdUserSpi, instId);

	char cInput = 'C';
	cerr << "输入'B'或者'b'结束等待行情" << ENDLINE;
	while(cInput != 'B' && cInput != 'b')
	{
		cin >> cInput;
		SleepMs(100);
	}

	//输出合约信息到文件
	ofstream InstLogFile;
	ofstream MarketDataLogFile;
	int InstListSize;
	string strLast;
	stringstream sstr;
	bool bIsLast;

	cerr << "输出合约信息到文件" << ENDLINE;

	InstLogFile.open("InstLogFile.txt", ios::trunc);
	InstListSize = m_RspInstrumentList.size();
    for(int i = 0; i < InstListSize; i++) 
	{
		sstr.clear(); sstr.str("");
		sstr << m_RspInstrumentList[i].CurTime.wYear << "-" << m_RspInstrumentList[i].CurTime.wMonth << "-" << m_RspInstrumentList[i].CurTime.wDay << " "
			 << m_RspInstrumentList[i].CurTime.wHour << ":" << m_RspInstrumentList[i].CurTime.wMinute << ":" << m_RspInstrumentList[i].CurTime.wSecond << "."
			 << m_RspInstrumentList[i].CurTime.wMilliseconds << " ";
		bIsLast = m_RspInstrumentList[i].IsLast;
		strLast = (bIsLast == true) ? "Is Last Message " : "--------------- ";
		InstLogFile << strLast 
		  << sstr.str() << "合约:" 
		  << m_RspInstrumentList[i].Instrument.InstrumentID
		  << " 交割月:" << m_RspInstrumentList[i].Instrument.DeliveryMonth
		  << " 多头保证金率:" << m_RspInstrumentList[i].Instrument.LongMarginRatio
		  << " 空头保证金率:" << m_RspInstrumentList[i].Instrument.ShortMarginRatio
		  <<ENDLINE;

		cerr << "InstrumentID = ";
		for(int j=0; j<sizeof(TThostFtdcInstrumentIDType); j++)
			cerr << m_RspInstrumentList[i].Instrument.InstrumentID[j]; //检验字符串结尾填充的都是空字符0x0-->对应于字母a
		cerr << ENDLINE;

		string strInstId;
		//检验string赋值算字符串长度时，是不会把NULL字符算进去的，比如InstrumentID存放"m1705"的长度就是5，存放"m1705-C-2400"时长度为12
		strInstId = m_RspInstrumentList[i].Instrument.InstrumentID;
		cerr << "strInstId.length = " << strInstId.length() << ENDLINE;   
	}
	InstLogFile.close();

	//输出行情信息到文件
	cerr << "输出行情信息到文件" << ENDLINE;

	int MarketDataListSize;
	MarketDataLogFile.open("MarketDataLogFile.txt", ios::trunc);
	MarketDataListSize = m_RtnMarketDataList.size();
	for(int i = 0; i < MarketDataListSize; i++)
	{
		sstr.clear(); sstr.str("");
		sstr << m_RtnMarketDataList[i].CurTime.wYear << "-" << m_RtnMarketDataList[i].CurTime.wMonth << "-" << m_RtnMarketDataList[i].CurTime.wDay << " "
			 << m_RtnMarketDataList[i].CurTime.wHour << ":" << m_RtnMarketDataList[i].CurTime.wMinute << ":" << m_RtnMarketDataList[i].CurTime.wSecond << "."
			 << m_RtnMarketDataList[i].CurTime.wMilliseconds << " ";

		MarketDataLogFile << "本地时间：" << sstr.str()
			<< " 交易所时间" << m_RtnMarketDataList[i].DepthMarketData.UpdateTime << "." << m_RtnMarketDataList[i].DepthMarketData.UpdateMillisec
			<< " 合约:" << m_RtnMarketDataList[i].DepthMarketData.InstrumentID
			<< " 现价:" << m_RtnMarketDataList[i].DepthMarketData.LastPrice
			//<<" 最高价:" << m_RtnMarketDataList[i].DepthMarketData.HighestPrice
			//<<" 最低价:" << m_RtnMarketDataList[i].DepthMarketData.LowestPrice
			<< " 卖一价:" << m_RtnMarketDataList[i].DepthMarketData.AskPrice1 
			<< " 卖一量:" << m_RtnMarketDataList[i].DepthMarketData.AskVolume1 
			<< " 买一价:" << m_RtnMarketDataList[i].DepthMarketData.BidPrice1
			<< " 买一量:" << m_RtnMarketDataList[i].DepthMarketData.BidVolume1
			//<<" 持仓量:"<< m_RtnMarketDataList[i].DepthMarketData.OpenInterest 
			<<ENDLINE;
	}
	MarketDataLogFile.close();

	cerr << "输入任意键继续" << ENDLINE;
    PauseForInput(); 
}

//测试void指针指向trader SPI和Md SPI
void CTestApp::QueryAndSubscribe(void* pTraderSpi, void* pMdSpi, char* InstID)
{
	CTestMdSpi* pMdUserSpi = (CTestMdSpi*)pMdSpi ;
	CTestTraderSpi* pTraderUserSpi = (CTestTraderSpi*)pTraderSpi;

	//获取合约信息
	cerr << "查询合约信息" << ENDLINE;
    pTraderUserSpi->ReqQryInstrument(InstID);
	SleepMs(5000);

	//订阅行情
	cerr << "订阅行情信息" << ENDLINE;
    pMdUserSpi->SubscribeMarketDataVector();  //订阅标的期货和对应期权行情
	//pMdUserSpi->SubscribeMarketData(instId);  //只订阅标的期货行情

	SleepMs(5000);
}

void CTestApp::TestMd()
{
	string strCmd;

	//判断MdApi流文件目录是否存在，不存在则创建
	if(access("mdapiflow", 0) != 0)
	{
		system("mkdir mdapiflow");
	}

#ifdef WIN_CTP
	string strMdFlowDir = ".\\mdapiflow";
	if(m_ArchName != "") strMdFlowDir = ".\\mdapiflow\\" + m_ArchName;
	if(m_ArchName != "" && access(strMdFlowDir.c_str(), 0) != 0)
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
	if(m_ArchName != "") strMdFlowDir = "./mdapiflow/" + m_ArchName;
	if(m_ArchName != "" && access(strMdFlowDir.c_str(), 0) != 0)
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
	m_pMdUserSpi=new CTestMdSpi(m_pMdUserApi, this); //创建回调处理类对象MdSpi
	m_pMdUserApi->RegisterSpi(m_pMdUserSpi);			 // 回调对象注入接口类

	CBrokerServer BrokerServer(m_CounterName, m_ArchName);
	for(size_t i = 0; i < BrokerServer.m_MdFrontList.size(); i++)
	{
		char MdFront[200];
		strcpy(MdFront, BrokerServer.m_MdFrontList[i].c_str());
		m_pMdUserApi->RegisterFront(MdFront);
	}

	m_pMdUserApi->Init();      //接口线程启动, 开始工作

	DWORD waitReturn;
	waitReturn = WaitForSingleObject(m_hEvent, LOGIN_WAIT_TIME);  
	if(waitReturn != WAIT_OBJECT_0) 
	{
		cerr << "超过" << int(LOGIN_WAIT_TIME/1000) << " 秒没有收到行情前置连接回应消息！" << ENDLINE;
	}
	ResetEvent(m_hEvent);

	ShowMdCommand(m_pMdUserSpi,true); 

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
}

void CTestApp::TestTrader()
{
	string strCmd;

	cerr << "启动交易API线程" << ENDLINE;
	//判断TraderApi流文件目录是否存在，不存在则创建
	if(access("traderapiflow", 0) != 0)
	{
		system("mkdir traderapiflow");
	}

#ifdef WIN_CTP
	string strTradeFlowDir = ".\\traderapiflow";
	if(m_ArchName != "") strTradeFlowDir = ".\\traderapiflow\\" + m_ArchName;
	if(m_ArchName != "" && access(strTradeFlowDir.c_str(), 0) != 0)
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
	if(m_ArchName != "") strTradeFlowDir = "./traderapiflow/" + m_ArchName;
	if(m_ArchName != "" && access(strTradeFlowDir.c_str(), 0) != 0)
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
	m_pTraderUserSpi = new CTestTraderSpi(m_pTraderUserApi, this);
	m_pTraderUserApi->RegisterSpi((CThostFtdcTraderSpi*)m_pTraderUserSpi);			// 注册事件类
	m_pTraderUserApi->SubscribePublicTopic(THOST_TERT_QUICK);					// 注册公有流
	m_pTraderUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);			  // 注册私有流

	//注册多个交易托管服务器前置地址
	CBrokerServer BrokerServer(m_CounterName, m_ArchName);
	for(size_t i = 0; i < BrokerServer.m_TraderFrontList.size(); i++)
	{
		char TraderFront[200];
		strcpy(TraderFront, BrokerServer.m_TraderFrontList[i].c_str());
		m_pTraderUserApi->RegisterFront(TraderFront);
	}
  
	m_pTraderUserApi->Init();

	//等待交易前置连接回应
	DWORD waitReturn;
	waitReturn = WaitForSingleObject(m_hEvent, LOGIN_WAIT_TIME);  
	if(waitReturn != WAIT_OBJECT_0) 
	{
		cerr << "超过" << int(LOGIN_WAIT_TIME/1000) << " 秒没有收到交易前置连接回应消息！" << ENDLINE;
	}
	ResetEvent(m_hEvent);
	
	ShowTraderCommand(m_pTraderUserSpi,true); 

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
}

//测试主程序
void CTestApp::TestMain()
{
#ifdef WIN_CTP
	//类中的静态函数才可能当作一般函数用，否则会报错
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE) 
	{ 
		cerr << "不能安装控制台消息处理回调函数" << ENDLINE;
		return; 
	}
#else
    signal(SIGINT , SignalHandler);           //按下CTRL + C键
    signal(SIGHUP , SignalHandler);           //关闭控制台窗口
    signal(SIGQUIT, SignalHandler);       
    signal(SIGTERM, SignalHandler);     
    signal(SIGKILL, SignalHandler);
#endif

#ifdef DUMP_TO_FILE
	cerr << "测试合约查询和多个合约行情返回的情况" << ENDLINE;
	DumpInstAndMarketData();
#else

#ifdef WIN_CTP
	m_hEvent = CreateEvent(NULL, true, false, NULL); 
#else
	m_hEvent = new CMyEvent;
#endif


	CKeyValue GlobalKeyValue;
	string key, value;

#ifdef WIN_CTP
	string strDirPath = "..\\TradeDatabase";
	string strDbName = "TradeDb.db";
	if(m_ArchName != "") strDbName = "TradeDb-" + m_ArchName + ".db";
	m_pTradeDb = new CTradeDb(strDirPath, strDbName);
	m_pTradeDb->Init();
	CTradeDb* pTradeDb = m_pTradeDb;

	cerr << "从全局配置数据表获取交易柜台信息" << ENDLINE;
	int RetRead = GlobalKeyValue.ReadConfigFromDb(pTradeDb, "GlobalConfig");

	string strConfigDir = ".\\config";
	if(m_ArchName != "") strConfigDir = ".\\config\\" + m_ArchName;
	string strFilePath = strConfigDir + "\\GlobalConfig.txt";
	if(RetRead <= 0) GlobalKeyValue.ReadConfigFromTextFile(strFilePath);
#else
	string strDirPath = "../TradeDatabase";
	string strDbName = "TradeDb.db";
	if(m_ArchName != "") strDbName = "TradeDb-" + m_ArchName + ".db";
	m_pTradeDb = new CTradeDb(strDirPath, strDbName);
	m_pTradeDb->Init();
	CTradeDb* pTradeDb = m_pTradeDb;

	cerr << "从全局配置数据表获取交易柜台信息" << ENDLINE;
	int RetRead = GlobalKeyValue.ReadConfigFromDb(pTradeDb, "GlobalConfig");

	string strConfigDir = "./config";
	if(m_ArchName != "") strConfigDir = "./config/" + m_ArchName;
	string strFilePath = strConfigDir + "/GlobalConfig.txt";
	if(RetRead <= 0) GlobalKeyValue.ReadConfigFromTextFile(strFilePath);
#endif

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

	//只能有一个柜台用于测试
	if(CounterSize == 1)
	{
		CounterName = CounterNameList[0];
	}
	else if(CounterSize > 1)
	{
		size_t CounterIndex = 0;
		cerr <<  "输入柜台索引 CounterIndex >"; cin >> CounterIndex;

		if(CounterIndex >= CounterSize)
			CounterName = CounterNameList[CounterSize-1];
		else if(CounterIndex <= 0)
			CounterName = CounterNameList[0];
		else
			CounterName = CounterNameList[CounterIndex];
	}
	else if(CounterSize <= 0)
	{
		cerr << "输入柜台名称 CounterName >"; cin >> CounterName;
		RemoveSpace(CounterName);
	}	 
	m_CounterName = CounterName;

	CKeyValue KeyValue;
	string strTableName = "Config_" + m_CounterName;
	cerr << "获取" << m_CounterName << "配置数据表信息" << ENDLINE;
	RetRead = KeyValue.ReadConfigFromDb(pTradeDb, strTableName);

#ifdef WIN_CTP
	strConfigDir = ".\\config";
	if(m_ArchName != "") strConfigDir = ".\\config\\" + m_ArchName;
	strFilePath = strConfigDir + "\\" + strTableName + ".txt";
	if(RetRead <= 0) KeyValue.ReadConfigFromTextFile(strFilePath);
#else
	strConfigDir = "./config";
	if(m_ArchName != "") strConfigDir = "./config/" + m_ArchName;
	strFilePath = strConfigDir + "/" + strTableName + ".txt";
	if(RetRead <= 0) KeyValue.ReadConfigFromTextFile(strFilePath);
#endif
	 
	//显示经纪商信息
    DisplayBrokerInfo(m_CounterName, m_ArchName, pTradeDb);

	value = KeyValue.InputStringKeyValue("BrokerId");
	strcpy(m_brokerId, value.c_str());
	value = KeyValue.InputStringKeyValue("UserId");
	strcpy(m_userId, value.c_str());
	cerr << "输入 Password> "; GetPasswd(m_Password);

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

	//将柜台配置信息写入数据库
	KeyValue.WriteConfigToDb(pTradeDb, strTableName);
	//将全局配置信息备份回数据库
	GlobalKeyValue.WriteConfigToDb(pTradeDb, "GlobalConfig");

	char SelTest;
	cerr << ENDLINE;
	cerr << "测试功能选择" << ENDLINE;
	cerr << "1. 行情测试" << ENDLINE;
	cerr << "2. 交易测试" << ENDLINE;
	cerr << "请输入> "; cin >> SelTest;
	cerr << ENDLINE;

#ifdef WIN_CTP
	system("cls");
#else
	system("clear");
#endif

	if(SelTest == '1')    
		TestMd();
	else if(SelTest == '2') 
		TestTrader();
	else
		cerr << "错误的输入" << ENDLINE;

#endif
}

#ifdef WIN_CTP
BOOL WINAPI CTestApp::ConsoleHandler(DWORD CEvent)
{
	switch(CEvent)
	{
		case CTRL_C_EVENT:            //按下CTRL + C键
		case CTRL_BREAK_EVENT:        //按下CTRL+BREAK键
		case CTRL_CLOSE_EVENT:        //关闭控制台窗口
		case CTRL_LOGOFF_EVENT:       //用户退出!
		case CTRL_SHUTDOWN_EVENT:     //用户关机
			//清理行情对象  
			if(pTestApp->m_pMdUserApi) {
				pTestApp->m_pMdUserApi->RegisterSpi(NULL);
				pTestApp->m_pMdUserApi->Release();
				pTestApp->m_pMdUserApi = NULL;
			}
			if(pTestApp->m_pMdUserSpi) {
				delete pTestApp->m_pMdUserSpi;
				pTestApp->m_pMdUserSpi = NULL;
			}

			//清理交易对象
			if(pTestApp->m_pTraderUserApi) {
				pTestApp->m_pTraderUserApi->RegisterSpi(NULL);
				pTestApp->m_pTraderUserApi->Release();
				pTestApp->m_pTraderUserApi = NULL;
			}
			if(pTestApp->m_pTraderUserSpi) {
				delete pTestApp->m_pTraderUserSpi;
				pTestApp->m_pTraderUserSpi = NULL;
			}
			MessageBoxA(NULL,"测试程序将被关闭!", "signal", MB_OK);
			exit(1);
			break;
		default:
			cerr << "不明控制台消息！" << ENDLINE;
	}
	return TRUE;
}

#else
void CTestApp::SignalHandler(int signo)
{
	switch(signo)
	{
		case SIGINT:            //按下CTRL + C键
		case SIGHUP:            //关闭控制台窗口
		case SIGQUIT:       
		case SIGTERM:     
		case SIGKILL:
			//清理行情对象  
			if(pTestApp->m_pMdUserApi) {
				pTestApp->m_pMdUserApi->RegisterSpi(NULL);
				pTestApp->m_pMdUserApi->Release();
				pTestApp->m_pMdUserApi = NULL;
			}
			if(pTestApp->m_pMdUserSpi) {
				delete pTestApp->m_pMdUserSpi;
				pTestApp->m_pMdUserSpi = NULL;
			}

			//清理交易对象
			if(pTestApp->m_pTraderUserApi) {
				pTestApp->m_pTraderUserApi->RegisterSpi(NULL);
				pTestApp->m_pTraderUserApi->Release();
				pTestApp->m_pTraderUserApi = NULL;
			}
			if(pTestApp->m_pTraderUserSpi) {
				delete pTestApp->m_pTraderUserSpi;
				pTestApp->m_pTraderUserSpi = NULL;
			}
			cerr << "收到控制信号，测试程序将被关闭!" << ENDLINE;
			exit(1);
			break;
		default:
			cerr << "不明控制台信号！" << ENDLINE;
	}
	return;
}


#endif

