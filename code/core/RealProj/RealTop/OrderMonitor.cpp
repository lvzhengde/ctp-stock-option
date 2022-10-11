#include "OrderMonitor.h"
#include "KeyValue.h"

#define  DISP_SECS    (5)

//++
//构造函数
//--
COrderMonitor::COrderMonitor(CRealApp* pRealApp)
{
	m_pRealApp = pRealApp;
}

//++
//析构函数
//--
COrderMonitor::~COrderMonitor()
{
}

//++
//报单监控主程序
//--
void COrderMonitor::OrderMonitorMain()
{
#ifdef WIN_CTP
	//安装控制台消息处理回调函数，以便优雅地退出程序
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)CRealApp::ConsoleHandler, TRUE) == FALSE) 
	{ 
		cerr << "不能安装控制台消息处理回调函数" << ENDLINE;
		return; 
	}
#else  //Linux
    signal(SIGINT , CRealApp::ConsoleHandler);           //按下CTRL + C键
    signal(SIGHUP , CRealApp::ConsoleHandler);           //关闭控制台窗口
    signal(SIGQUIT, CRealApp::ConsoleHandler);       
    signal(SIGTERM, CRealApp::ConsoleHandler);     
    signal(SIGKILL, CRealApp::ConsoleHandler);
#endif //WIN_CTP

	CKeyValue GlobalKeyValue;
	string key, value;

    m_pRealApp->m_bManualTrade = false;
	m_pRealApp->CreateObjects();
	m_pRealApp->m_pTradeDb->Init();

	cerr << m_pRealApp->m_Prompt << "从全局配置数据表获取账户信息" << ENDLINE;
	int RetRead = GlobalKeyValue.ReadConfigFromDb(m_pRealApp->m_pTradeDb, "GlobalConfig");

#ifdef WIN_CTP
	string strConfigDir = ".\\config";
	if(m_pRealApp->m_ArchName != "") strConfigDir = ".\\config\\" + m_pRealApp->m_ArchName;
	string strFilePath = strConfigDir + "\\GlobalConfig.txt";
#else
	string strConfigDir = "./config";
	if(m_pRealApp->m_ArchName != "") strConfigDir = "./config/" + m_pRealApp->m_ArchName;
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

	//将配置信息备份回数据库
	GlobalKeyValue.WriteConfigToDb(m_pRealApp->m_pTradeDb, "GlobalConfig");

	//初始化各个交易柜台
    m_pRealApp->m_pCtpCntIfList.clear();
	for(size_t i = 0; i < CounterNameList.size(); i++)
	{
		cerr << ENDLINE;
		CounterName = CounterNameList[i];
		CCtpCntIf* pCtpCntIf = new CCtpCntIf(m_pRealApp, CounterName);
		pCtpCntIf->GetConfigFromDb();
		pCtpCntIf->InitCtpApi(THOST_TERT_RESTART); //从今天开始重传
		//初始化互斥区, 参考PosMoneyManage
		InitializeCriticalSection(&pCtpCntIf->m_OrderListCS);
		m_pRealApp->m_pCtpCntIfList.push_back(pCtpCntIf);
	}

	//监控循环
	MonitorLoop();

	//为避免报错，先要清除主动操作的各种对象
	//互斥区将在DestroyObjects()中删除, 所以不需要特意删除, 参考PosMoneyManage
	m_pRealApp->DestroyObjects();
	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		 m_pRealApp->m_pCtpCntIfList[i]->CleanupCtpApi();
	}
}

//++
//监控循环程序
//--
void COrderMonitor::MonitorLoop()
{
	char KbIn;
	bool bCancelAllOrder = false;
	bool bExitLoop = false;
	double PreDispSec = 0;
	double DispSec = 0;
	SYSTEMTIME CurTime;

	//挂单监控循环
	cerr << ENDLINE << m_pRealApp->m_Prompt << "报单监控程序开始运行..." << ENDLINE;
	while(true)
	{
		bCancelAllOrder = false;
		bExitLoop = false;
		//按ESC键时可以选择一键撤单或者结束程序
		if(_kbhit()) 
		{
			KbIn = _getch();
			if(KbIn == 27)  //判断是否为ESC键
			{
				//确认是否一键撤单
				cerr << m_pRealApp->m_Prompt << "是否一键撤单? Y/N >"; cin >> KbIn;
				if(KbIn == 'Y' || KbIn == 'y') 
				{
					bCancelAllOrder = true;
				}

				//一键撤单的时候不能选择退出程序, 需要迅速处理
				if(bCancelAllOrder == false)
				{
					cerr << m_pRealApp->m_Prompt << "是否退出程序? Y/N >"; cin >> KbIn;
					if(KbIn == 'Y' || KbIn == 'y') 
					{
						bExitLoop = true;
					}
				}
			} //判断是否为ESC键
		} //判断是否有按下键盘

		//显示时间控制
		GetLocalTime(&CurTime);
		DispSec = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;
		if(DispSec > (PreDispSec+DISP_SECS))
		{
			cerr << "时间: " << CurTime.wHour << ":" << CurTime.wMinute << ":" << CurTime.wSecond << "." << CurTime.wMilliseconds << ", "
				 << "----------------------------------------------------------" << ENDLINE;
		}

		//整理报单列表
		vector<CThostFtdcOrderField> tmpOrderList;
		for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
		{
			CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
			CRealTraderSpi*  pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;
			vector<CThostFtdcOrderField> ReducedOrderList;
			vector<CThostFtdcOrderField> ActiveOrderList;

			//复制TraderSpi中的报单列表
			tmpOrderList.clear();
			EnterCriticalSection(&pCtpCntIf->m_OrderListCS);
			tmpOrderList.assign(pTraderUserSpi->m_orderList.begin(), pTraderUserSpi->m_orderList.end());
			pCtpCntIf->m_DupOrderList.swap(tmpOrderList);
			LeaveCriticalSection(&pCtpCntIf->m_OrderListCS);

			//将pCtpCntIf->m_DupOrderList精简, 合并仅状态不同的重复报单
			tmpOrderList.clear();
			for(size_t j = 0; j < pCtpCntIf->m_DupOrderList.size(); j++)
			{
				CThostFtdcOrderField *pOrder = &pCtpCntIf->m_DupOrderList[j];
				int BrokerOrderSeq = pOrder->BrokerOrderSeq;
				int OrderRef = atoi(pOrder->OrderRef);
				int FrontId = pOrder->FrontID;
				int SessionId = pOrder->SessionID;
				bool found = false;

				for(size_t k = 0; k < tmpOrderList.size(); k++)
				{
					int tmpOrderRef = atoi(tmpOrderList[k].OrderRef);
					//确认是同一报单的不同回报
					if((FrontId == tmpOrderList[k].FrontID && SessionId == tmpOrderList[k].SessionID && OrderRef == tmpOrderRef)
						|| (strcmp(tmpOrderList[k].ExchangeID, pOrder->ExchangeID) == 0 && strcmp(tmpOrderList[k].OrderSysID, pOrder->OrderSysID) == 0))
					{
						found = true;
						memcpy(&tmpOrderList[k], pOrder, sizeof(CThostFtdcOrderField));
						break;
					}
				}

				//报单未找到
				if(found == false)
				{
					tmpOrderList.push_back(*pOrder);
				}
			} //for j, pCtpCntIf->m_DupOrderList
			ReducedOrderList.swap(tmpOrderList);

			//找出精简报单列表中所有当前挂单
			tmpOrderList.clear();
			for(size_t j = 0; j < ReducedOrderList.size(); j++)
			{
				if(ReducedOrderList[j].OrderStatus != THOST_FTDC_OST_AllTraded && ReducedOrderList[j].OrderStatus != THOST_FTDC_OST_Canceled)
				{
					tmpOrderList.push_back(ReducedOrderList[j]);
				}
			} //for j
			ActiveOrderList.swap(tmpOrderList);

			//显示该柜台的当前挂单, 每两秒左右显示一次
			if(DispSec > (PreDispSec+DISP_SECS))
			{
				for(size_t j = 0; j < ActiveOrderList.size(); j++)
				{
					CThostFtdcOrderField* pOrder = &ActiveOrderList[j];
					if(j == 0) cerr << pCtpCntIf->m_CounterName << " 交易柜台当前挂单: " << ENDLINE;
					cerr << "合约代码:" << pOrder->InstrumentID << ", 交易所: " << pOrder->ExchangeID 
						 << ", 交易所报单编号:" << pOrder->OrderSysID << ", 状态消息:"<< StatusMessageCodeConvert(pOrder->StatusMsg) 
						 << ", 买卖方向: " << pOrder->Direction << ", 开平标志: " << pOrder->CombOffsetFlag
						 << ENDLINE;
					if(j == (ActiveOrderList.size()-1)) cerr << "+++++++++" << ENDLINE;
				} //for j
			} //if

			//一键撤单
			if(bCancelAllOrder == true)
			{
				CancelAllActiveOrder(pTraderUserSpi, ActiveOrderList);
			}
		} //for i, 各个交易柜台	

		//需要结束循环后, 在循环外设置显示时间
		if(DispSec > (PreDispSec+DISP_SECS))
		{
			PreDispSec = DispSec;
			cerr << ENDLINE;
		}

		//给其它程序让出CPU时间
		SleepMs(200);

		if(bExitLoop == true) break;
	} //while
	cerr << m_pRealApp->m_Prompt << "退出报单监控程序..." << ENDLINE << ENDLINE;
}

//++
//一键撤销所有当前活跃挂单
//参数
//    ActiveOrderList: 当前活跃挂单列表
//    pTraderUserSpi: 交易柜台SPI
//返回值
//    无
//--
void COrderMonitor::CancelAllActiveOrder(CRealTraderSpi*  pTraderUserSpi, vector<CThostFtdcOrderField> &ActiveOrderList)
{
	string strInstID = "";
	int FrontId;
	int SessionId;
	int OrderRef;

	for(size_t i = 0; i < ActiveOrderList.size(); i++)
	{
		CThostFtdcOrderField* pOrder = &ActiveOrderList[i];
		strInstID = pOrder->InstrumentID;
		FrontId = pOrder->FrontID;
		SessionId = pOrder->SessionID;
		OrderRef = atoi(pOrder->OrderRef);

		int ret = pTraderUserSpi->ReqOrderAction(pOrder, FrontId, SessionId, OrderRef, strInstID);
		cerr << " 请求 | 发送撤单..." <<((ret == 0)?"成功! ":"失败! ") << " 合约: " << strInstID << ENDLINE;
	}
}
