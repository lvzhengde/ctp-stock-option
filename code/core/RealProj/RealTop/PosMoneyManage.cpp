#include "PosMoneyManage.h"
#include "RealApp.h"


//++
//构造函数
//--
CPosMoneyManage::CPosMoneyManage(CRealApp* pRealApp)
{
	m_pRealApp = pRealApp;
}

//++
//析构函数
//--
CPosMoneyManage::~CPosMoneyManage()
{
}

//++
//初始化行情管理对象
//--
void CPosMoneyManage::Initialize()
{
	DWORD waitReturn;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		CRealTraderSpi*  pTraderUserSpi = m_pRealApp->m_pCtpCntIfList[i]->m_pTraderUserSpi;

		pCtpCntIf->m_InvestorPositionDetailList.clear();
		memset(&pCtpCntIf->m_TradingAccount, 0, sizeof(CThostFtdcTradingAccountField));

		//++
		//初始化m_InvestorPositionDetailList列表
		//--
		vector<CThostFtdcInvestorPositionField> InvestorPositionList;
		InvestorPositionList.clear();
		CInvestorPositionDetail InvestorPositionDetail;
		vector<CInvestorPositionDetail> InvestorPositionDetailList;
		InvestorPositionDetailList.clear();
		TThostFtdcInstrumentIDType InstId;
	
		//设置为空是查询所有合约的持仓明细
		memset(InstId, 0, sizeof(TThostFtdcInstrumentIDType));
		pTraderUserSpi->ReqQryInvestorPosition(InstId);
		waitReturn = WaitForSingleObject(pCtpCntIf->m_hTraderSpiEvent, REQ_WAIT_TIME);   
		if(waitReturn != WAIT_OBJECT_0) 
		{
			cerr << m_pRealApp->m_Prompt << "超过 " << int(REQ_WAIT_TIME/1000) << " 秒没有收到查询投资者所有合约持仓明细的回应消息，程序返回！" << ENDLINE;
            PressAnyKeyToExit();
			exit(-1);
		}
		ResetEvent(pCtpCntIf->m_hTraderSpiEvent);
		SleepMs(REQ_INTERVAL);

		//使用Critical Section保护
		EnterCriticalSection(&pCtpCntIf->m_InvestorPositionCS);
		InvestorPositionList.assign(pTraderUserSpi->m_InvestorPositionList.begin(), pTraderUserSpi->m_InvestorPositionList.end());
		LeaveCriticalSection(&pCtpCntIf->m_InvestorPositionCS);

		//合并相同合约代码，相同持仓方向的结果
		MergeInvestorPosition(InvestorPositionList);

		for(size_t j = 0; j < InvestorPositionList.size(); j++)
		{
			memcpy(&InvestorPositionDetail.CtpInvestorPosition, &InvestorPositionList[j], sizeof(CThostFtdcInvestorPositionField));
			InvestorPositionDetail.ClaimedPosition = 0;
			InvestorPositionDetail.ClaimedYdPosition = 0;
			InvestorPositionDetail.ClaimedTodayPosition = 0;
			InvestorPositionDetailList.push_back(InvestorPositionDetail);
		}
		pCtpCntIf->m_InvestorPositionDetailList.swap(InvestorPositionDetailList);
	
		//++
		//初始化m_TradingAccount
		//--
		pTraderUserSpi->ReqQryTradingAccount();
		waitReturn = WaitForSingleObject(pCtpCntIf->m_hTraderSpiEvent, REQ_WAIT_TIME);   
		if(waitReturn != WAIT_OBJECT_0) 
		{
			cerr << m_pRealApp->m_Prompt << "超过 " << int(REQ_WAIT_TIME/1000) << " 秒没有收到查询投资者资金账户明细的回应消息，程序返回！" << ENDLINE;
            PressAnyKeyToExit();
			exit(-1);
		}
		ResetEvent(pCtpCntIf->m_hTraderSpiEvent);
		SleepMs(REQ_INTERVAL);

		//使用Critical Section保护
		EnterCriticalSection(&pCtpCntIf->m_TradingAccountCS);
		memcpy(&pCtpCntIf->m_TradingAccount, &pTraderUserSpi->m_TradingAccount, sizeof(CThostFtdcTradingAccountField));
		LeaveCriticalSection(&pCtpCntIf->m_TradingAccountCS);
	} //for i, 交易柜台
}

//++
//退出前的清理工作
//--
void CPosMoneyManage::Cleanup()
{
	//删除互斥区，必须先有初始化操作
	if(m_pRealApp->m_bInitDone == true)
	{
		;
	}
}

//++
//将用户账户和持仓的详细情况输出到文本文件
//--
void CPosMoneyManage::DumpToTextFile()
{
	ofstream OutFile;

	string strOutputDir = "./output/";
	if(m_pRealApp->m_ArchName != "") strOutputDir = "./output/" + m_pRealApp->m_ArchName + "/";
	string strInvestorPositionFile = strOutputDir + INVESTOR_POSITION_FILE;

	OutFile.open(strInvestorPositionFile.c_str(),  ios::trunc);

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		OutFile << "交易柜台: " << pCtpCntIf->m_CounterName << "***************************************" << ENDLINE;

		//先输出用户账户资金情况
		OutFile << fixed << setprecision(6) << "投资者账户资金情况" << ENDLINE
			<< "总权益: " << pCtpCntIf->m_TradingAccount.Balance << " 可用资金: " << pCtpCntIf->m_TradingAccount.Available 
			<< " 保证金: " << pCtpCntIf->m_TradingAccount.CurrMargin << " 平仓盈亏: " << pCtpCntIf->m_TradingAccount.CloseProfit 
			<< " 持仓盈亏: " << pCtpCntIf->m_TradingAccount.PositionProfit << " 手续费: " << pCtpCntIf->m_TradingAccount.Commission
			<< " 冻结保证金: " << pCtpCntIf->m_TradingAccount.FrozenMargin << ENDLINE
			<< " ---------------------------------------------------------------------------  " << ENDLINE << ENDLINE;

		//输出用户持仓情况
		OutFile << "投资者持仓情况" << ENDLINE;
		for(size_t j = 0; j < pCtpCntIf->m_InvestorPositionDetailList.size(); j++)
		{
			CThostFtdcInvestorPositionField* pInvestorPosition = &pCtpCntIf->m_InvestorPositionDetailList[j].CtpInvestorPosition;

			OutFile << "合约代码: " << pInvestorPosition->InstrumentID << " 持仓方向: " << CtpDirctionToMyDirection(pInvestorPosition->PosiDirection)
					<< " 总持仓: " << pInvestorPosition->Position << " Claimed持仓: " << pCtpCntIf->m_InvestorPositionDetailList[j].ClaimedPosition
					<< " 昨仓: " << pInvestorPosition->YdPosition << " 今仓: " << pInvestorPosition->TodayPosition << ENDLINE;
		}

		OutFile << ENDLINE << ENDLINE;
	}

	OutFile.close();
}

//++
//定期刷新投资者持仓信息，账户资金信息
//--
void CPosMoneyManage::RefreshTradeAccountInfo()
{
	double localTimeSeconds;
	SYSTEMTIME CurTime;

	GetLocalTime(&CurTime);
	localTimeSeconds = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		CRealTraderSpi*  pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;

		//刚进入交易程序
		if(pCtpCntIf->m_PrePositionSeconds == 0 || pCtpCntIf->m_PreAccountSeconds == 0 || pCtpCntIf->m_prevTime.wDay != CurTime.wDay)
		{
			pCtpCntIf->m_PrePositionSeconds = localTimeSeconds;
			pCtpCntIf->m_PreAccountSeconds = localTimeSeconds;
			memcpy(&pCtpCntIf->m_prevTime, &CurTime, sizeof(SYSTEMTIME));
			return;
		}

		//++
		//更新投资者持仓列表
		//--
		vector<CThostFtdcInvestorPositionField> InvestorPositionList;
		InvestorPositionList.clear();
		CInvestorPositionDetail InvestorPositionDetail;
		vector<CInvestorPositionDetail> InvestorPositionDetailList;
		InvestorPositionDetailList.clear();
		TThostFtdcInstrumentIDType InstId;
	
		//设置为空是查询所有合约的持仓明细
		memset(InstId, 0, sizeof(TThostFtdcInstrumentIDType));
		//每隔REFRESH_INTERNAL秒查询一次，查询时不更新，与资金查询时间拉开距离
		if(localTimeSeconds > (pCtpCntIf->m_PrePositionSeconds+REFRESH_INTERNAL)  && localTimeSeconds > (pCtpCntIf->m_PreAccountSeconds+REQ_INTERVAL/1000))
		{
			int ret = pTraderUserSpi->ReqQryInvestorPosition(InstId);
			pCtpCntIf->m_bReqPositionSuccess = (ret != 0) ? false : true;
			pCtpCntIf->m_PrePositionSeconds = localTimeSeconds;
		}
		else
		{
			//使用Critical Section保护
			EnterCriticalSection(&pCtpCntIf->m_InvestorPositionCS);
			InvestorPositionList.assign(pTraderUserSpi->m_InvestorPositionList.begin(), pTraderUserSpi->m_InvestorPositionList.end());
			memcpy(&pCtpCntIf->m_ReqPositionTimestamp, &pTraderUserSpi->m_ReqPositionTimestamp, sizeof(SYSTEMTIME));
			LeaveCriticalSection(&pCtpCntIf->m_InvestorPositionCS);

			//合并相同合约代码，相同持仓方向的结果
			MergeInvestorPosition(InvestorPositionList);

			for(size_t i = 0; i < InvestorPositionList.size(); i++)
			{
				memcpy(&InvestorPositionDetail.CtpInvestorPosition, &InvestorPositionList[i], sizeof(CThostFtdcInvestorPositionField));
				InvestorPositionDetail.ClaimedPosition = 0;
				InvestorPositionDetail.ClaimedYdPosition = 0;
				InvestorPositionDetail.ClaimedTodayPosition = 0;
				InvestorPositionDetailList.push_back(InvestorPositionDetail);
			}
			pCtpCntIf->m_InvestorPositionDetailList.swap(InvestorPositionDetailList);

			//为交易所规则过滤器更新投资者持仓量
			for(size_t i = 0; i < m_pRealApp->m_pExchangeRuleFilterList.size(); i++)
			{
				CExchangeRuleFilter *pExchangeRuleFilter = m_pRealApp->m_pExchangeRuleFilterList[i];
				pExchangeRuleFilter->CalcOpenInterests(pCtpCntIf->m_CntCode, &pCtpCntIf->m_InvestorPositionDetailList);
			}
		}

		//++
		//更新账户资金信息
		//--
		//每隔REFRESH_INTERNAL秒查询一次，查询时不更新
		//同时和投资者持仓查询错开1.5秒
		if(localTimeSeconds > (pCtpCntIf->m_PreAccountSeconds+REFRESH_INTERNAL) && localTimeSeconds > (pCtpCntIf->m_PrePositionSeconds+REQ_INTERVAL/1000))
		{
			int ret = pTraderUserSpi->ReqQryTradingAccount();
			pCtpCntIf->m_bReqAccountSuccess = (ret != 0) ? false : true;
			pCtpCntIf->m_PreAccountSeconds = localTimeSeconds;
			pCtpCntIf->m_CurAccountSeqNo++;
		}
		else
		{
			CThostFtdcTradingAccountField PreTradingAccount;
			memcpy(&PreTradingAccount, &pCtpCntIf->m_TradingAccount,  sizeof(CThostFtdcTradingAccountField));

			//使用Critical Section保护
			EnterCriticalSection(&pCtpCntIf->m_TradingAccountCS);
			memcpy(&pCtpCntIf->m_TradingAccount, &pTraderUserSpi->m_TradingAccount, sizeof(CThostFtdcTradingAccountField));
			memcpy(&pCtpCntIf->m_ReqAccountTimestamp, &pTraderUserSpi->m_ReqAccountTimestamp, sizeof(SYSTEMTIME));
			LeaveCriticalSection(&pCtpCntIf->m_TradingAccountCS);

			//当前时间超过查询时间1秒，复位增量保证金和总资金
			if(localTimeSeconds > (pCtpCntIf->m_PreAccountSeconds+0.5) && pCtpCntIf->m_PreAccountSeqNo != pCtpCntIf->m_CurAccountSeqNo)
			{
				pCtpCntIf->m_PreAccountSeqNo = pCtpCntIf->m_CurAccountSeqNo;
			}
		}
	} //for i, 交易柜台
}

//++
//刷新报单和成交信息列表
//参数
//    无
//返回值
//    无
//--
void CPosMoneyManage::RefreshOrderAndTradeList()
{
	CStrategysManage* pStrategyManage = m_pRealApp->m_pStrategysManage;
	vector<CThostFtdcOrderField> tmpOrderList;
	vector<CThostFtdcTradeField> tmpTradeList;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		CRealTraderSpi*  pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;
		tmpOrderList.clear();
		tmpTradeList.clear();

		//使用互斥信号保护orderList的访问
		EnterCriticalSection(&pCtpCntIf->m_OrderListCS);
		tmpOrderList.assign(pTraderUserSpi->m_orderList.begin(), pTraderUserSpi->m_orderList.end());
		pCtpCntIf->m_DupOrderList.swap(tmpOrderList);
		LeaveCriticalSection(&pCtpCntIf->m_OrderListCS);

		//使用互斥信号保护tradeList的访问
		EnterCriticalSection(&pCtpCntIf->m_TradeListCS);
		tmpTradeList.assign(pTraderUserSpi->m_tradeList.begin(), pTraderUserSpi->m_tradeList.end());
		pCtpCntIf->m_DupTradeList.swap(tmpTradeList);
		LeaveCriticalSection(&pCtpCntIf->m_TradeListCS);
	} //for i, 交易柜台
}

//++
//获取最新的投资者持仓列表，提供给ErrorHandle程序使用
//参数
//    无
//返回值
//    true: 成功获取最新的投资者持仓列表
//    false: 失败
//--
bool CPosMoneyManage::GetNewestInvestorPosition()
{
	DWORD waitReturn;
	vector<CThostFtdcInvestorPositionField> InvestorPositionList;
	CInvestorPositionDetail InvestorPositionDetail;
	vector<CInvestorPositionDetail> InvestorPositionDetailList;
	TThostFtdcInstrumentIDType InstId;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		CRealTraderSpi*  pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;
		InvestorPositionList.clear();
		InvestorPositionDetailList.clear();

		//睡眠1秒以上是为了防止每秒查询数超过要求
		SleepMs(REQ_INTERVAL);	
		//防止其它消息干扰，先将信号复位
		ResetEvent(pCtpCntIf->m_hTraderSpiEvent);
		//设置为空是查询所有合约的持仓明细
		memset(InstId, 0, sizeof(TThostFtdcInstrumentIDType));
		pTraderUserSpi->ReqQryInvestorPosition(InstId);
		waitReturn = WaitForSingleObject(pCtpCntIf->m_hTraderSpiEvent, REQ_WAIT_TIME);   
		if(waitReturn != WAIT_OBJECT_0) 
		{
			cerr << m_pRealApp->m_Prompt << "超过 " << int(REQ_WAIT_TIME/1000) << " 秒没有收到查询投资者所有合约持仓明细的回应消息！" << ENDLINE;
			return false;
		}
		ResetEvent(pCtpCntIf->m_hTraderSpiEvent);

		//使用Critical Section保护
		EnterCriticalSection(&pCtpCntIf->m_InvestorPositionCS);
		InvestorPositionList.assign(pTraderUserSpi->m_InvestorPositionList.begin(), pTraderUserSpi->m_InvestorPositionList.end());
		LeaveCriticalSection(&pCtpCntIf->m_InvestorPositionCS);

		//合并相同合约代码，相同持仓方向的结果
		MergeInvestorPosition(InvestorPositionList);

		for(size_t j = 0; j < InvestorPositionList.size(); j++)
		{
			memcpy(&InvestorPositionDetail.CtpInvestorPosition, &InvestorPositionList[j], sizeof(CThostFtdcInvestorPositionField));
			InvestorPositionDetail.ClaimedPosition = 0;
			InvestorPositionDetail.ClaimedYdPosition = 0;
			InvestorPositionDetail.ClaimedTodayPosition = 0;
			InvestorPositionDetailList.push_back(InvestorPositionDetail);
		}
		pCtpCntIf->m_InvestorPositionDetailList.swap(InvestorPositionDetailList);
	} //for i, 交易柜台

	return true;
}

//++
//合并合约代码和买卖方向相同的投资者持仓查询结果
//主要因为是上期所投资者持仓查询对于同一合约同一买卖方向可能得到多个结果
//参数
//    InvestorPositionList: 引用，传入时是查询得到的投资者持仓查询结果，传出时是合并后的结果
//返回值
//    无
//--
void CPosMoneyManage::MergeInvestorPosition(vector<CThostFtdcInvestorPositionField>& InvestorPositionList)
{
	vector<CThostFtdcInvestorPositionField> tmpInvestorPositionList;

	for(size_t i = 0; i < InvestorPositionList.size(); i++)
	{
		CThostFtdcInvestorPositionField InvestorPosition;

		//'@'是查找到重复合约代码和持仓方向后所作的特别标志
		if(InvestorPositionList[i].InstrumentID[0] != '@')
		{
			memcpy(&InvestorPosition, &InvestorPositionList[i], sizeof(CThostFtdcInvestorPositionField));

			for(size_t j = i+1; j < InvestorPositionList.size(); j++)
			{
				if(strcmp(InvestorPosition.InstrumentID, InvestorPositionList[j].InstrumentID) == 0 && InvestorPosition.PosiDirection == InvestorPositionList[j].PosiDirection)
				{
					InvestorPosition.YdPosition = InvestorPosition.YdPosition + InvestorPositionList[j].YdPosition;
					InvestorPosition.Position = InvestorPosition.Position + InvestorPositionList[j].Position;
					InvestorPosition.TodayPosition = InvestorPosition.TodayPosition + InvestorPositionList[j].TodayPosition;
					InvestorPosition.OpenAmount = InvestorPosition.OpenAmount + InvestorPositionList[j].OpenAmount;
					InvestorPosition.CloseAmount = InvestorPosition.CloseAmount + InvestorPositionList[j].CloseAmount;
					InvestorPosition.OpenVolume = InvestorPosition.OpenVolume + InvestorPositionList[j].OpenVolume;
					InvestorPosition.CloseVolume = InvestorPosition.CloseVolume + InvestorPositionList[j].CloseVolume;
					InvestorPosition.PositionCost = InvestorPosition.PositionCost + InvestorPositionList[j].PositionCost;
					InvestorPosition.UseMargin = InvestorPosition.UseMargin + InvestorPositionList[j].UseMargin;
					InvestorPosition.PositionProfit = InvestorPosition.PositionProfit + InvestorPositionList[j].PositionProfit;
					InvestorPosition.OpenCost = InvestorPosition.OpenCost + InvestorPositionList[j].OpenCost;
					InvestorPosition.ExchangeMargin = InvestorPosition.ExchangeMargin + InvestorPositionList[j].ExchangeMargin;
					InvestorPosition.Commission = InvestorPosition.Commission + InvestorPositionList[j].Commission;

					//设置重复标志
					InvestorPositionList[j].InstrumentID[0] = '@';	
				}
				 
			}
			tmpInvestorPositionList.push_back(InvestorPosition);
		}
	}

	//交换列表
	InvestorPositionList.swap(tmpInvestorPositionList);
}

//++
//保存报单情况到数据库
//--
void CPosMoneyManage::SaveOrderToDatabase()
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
    string strTableName;
	string strFieldDesc;
	string strSQLCmd;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		CRealTraderSpi*  pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;

		strTableName = pCtpCntIf->m_CounterName + "_OrderLog";
		strFieldDesc = "(ID INTEGER PRIMARY KEY AUTOINCREMENT, OrderSysID VARCHAR, InstrumentID VARCHAR, Direction VARCHAR, \
						CombOffsetFlag VARCHAR, CombHedgeFlag VARCHAR, OrderSubmitStatus VARCHAR, \
					   OrderStatus VARCHAR, LimitPrice DOUBLE, VolumeTotalOriginal INT, VolumeTotal INT, VolumeTraded INT, \
					   StatusMsg VARCHAR, InsertDate VARCHAR, InsertTime VARCHAR, \
					   ExchangeID VARCHAR, BrokerID VARCHAR, InvestorID VARCHAR)";
		int Ret = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
		if(Ret < 0) 
		{
			cerr << m_pRealApp->m_Prompt << "数据表 " << strTableName << " 无法创建或者无法访问，程序返回！" << ENDLINE;
            PressAnyKeyToExit();
			return;
		}

		//将非当日的报单信息删除, 防止保存到数据库的数据太多
	    strSQLCmd = "DELETE FROM " + strTableName + " WHERE InsertDate <> date('now')"; 
        pTradeDb->ExecuteSqlCommand(strSQLCmd);

		//复制报单列表
		vector<CThostFtdcOrderField> tmpOrderList;
		tmpOrderList.clear();
		EnterCriticalSection(&pCtpCntIf->m_OrderListCS);
		tmpOrderList.assign(pTraderUserSpi->m_orderList.begin(), pTraderUserSpi->m_orderList.end());
		LeaveCriticalSection(&pCtpCntIf->m_OrderListCS);

		//将报单信息插入数据表
		pTradeDb->ExecuteSqlCommand("BEGIN TRANSACTION;");
		for(size_t j = pCtpCntIf->m_PreOrderSavePos; j < tmpOrderList.size(); j++)
		{
			stringstream sstr;
			sstr.clear(); sstr.str(""); sstr.precision(18);

			sstr << "'" << tmpOrderList[j].OrderSysID << "', '" << tmpOrderList[j].InstrumentID << "', '" << ToNoZeroChar(tmpOrderList[j].Direction) 
				<< "', '" << tmpOrderList[j].CombOffsetFlag 
				<< "', '" << tmpOrderList[j].CombHedgeFlag << "', '"<< ToNoZeroChar(tmpOrderList[j].OrderSubmitStatus) << "', '" 
				<< ToNoZeroChar(tmpOrderList[j].OrderStatus) <<  "', " << tmpOrderList[j].LimitPrice
				<< ", " << tmpOrderList[j].VolumeTotalOriginal << ", " << tmpOrderList[j].VolumeTotal << ", " << tmpOrderList[j].VolumeTraded 
				<< ", '" << StatusMessageCodeConvert(tmpOrderList[j].StatusMsg, true)
				<< "', '" << tmpOrderList[j].InsertDate << "', '" << tmpOrderList[j].InsertTime << "', '" << tmpOrderList[j].ExchangeID 
				<< "', '" << tmpOrderList[j].BrokerID << "', '"
				<< tmpOrderList[j].InvestorID << "'" ;

			strSQLCmd = "INSERT INTO " + strTableName + "(OrderSysID, InstrumentID, Direction, CombOffsetFlag, CombHedgeFlag, OrderSubmitStatus, OrderStatus, \
														 LimitPrice, VolumeTotalOriginal, VolumeTotal, VolumeTraded, StatusMsg, InsertDate, InsertTime, ExchangeID, \
														 BrokerID, InvestorID) VALUES(" + sstr.str() + ")";

			bool bExecuteSql = pTradeDb->ExecuteSqlCommand(strSQLCmd);
			if(bExecuteSql == false)
				cerr << "插入报单信息到 OrderLog 数据表出错！" << ENDLINE;
		}
		pTradeDb->ExecuteSqlCommand("END TRANSACTION;");

		pCtpCntIf->m_PreOrderSavePos = tmpOrderList.size();
	} //for i, 交易柜台
}

//++
//保存成交情况到数据库
//--
void CPosMoneyManage::SaveTradeToDatabase()
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
    string strTableName;
	string strFieldDesc;
	string strSQLCmd;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		CRealTraderSpi*  pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;

		strTableName = pCtpCntIf->m_CounterName + "_TradeLog";
		strFieldDesc = "(ID INTEGER PRIMARY KEY AUTOINCREMENT, TradeID VARCHAR, InstrumentID VARCHAR, Direction VARCHAR,\
					   OffsetFlag VARCHAR, HedgeFlag VARCHAR, Price DOUBLE, \
					   Volume INT, TradeDate VARCHAR, TradeTime VARCHAR, OrderSysID VARCHAR, TradeType VARCHAR,  \
					   ExchangeID VARCHAR, BrokerID VARCHAR, InvestorID VARCHAR)";
		int Ret = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
		if(Ret < 0) 
		{
			cerr << m_pRealApp->m_Prompt << "数据表 " << strTableName << " 无法创建或者无法访问，程序返回！" << ENDLINE;
            PressAnyKeyToExit();
			return;
		}

		//复制成交列表
		vector<CThostFtdcTradeField> tmpTradeList;
		tmpTradeList.clear();

		//使用互斥信号保护tradeList的访问
		EnterCriticalSection(&pCtpCntIf->m_TradeListCS);
		tmpTradeList.assign(pTraderUserSpi->m_tradeList.begin(), pTraderUserSpi->m_tradeList.end());
		LeaveCriticalSection(&pCtpCntIf->m_TradeListCS);

		//将成交信息插入数据表
		for(size_t j = pCtpCntIf->m_PreTradeSavePos; j < tmpTradeList.size(); j++)
		{
			stringstream sstr;
			sstr.clear(); sstr.str(""); sstr.precision(18);

			sstr << "'" << tmpTradeList[j].TradeID << "', '" << tmpTradeList[j].InstrumentID << "', '" << ToNoZeroChar(tmpTradeList[j].Direction) 
				<< "', '" << ToNoZeroChar(tmpTradeList[j].OffsetFlag) 
				<< "', '" << ToNoZeroChar(tmpTradeList[j].HedgeFlag) << "', "<< tmpTradeList[j].Price << ", " << tmpTradeList[j].Volume << ", '" 
				<< tmpTradeList[j].TradeDate
				<< "', '" << tmpTradeList[j].TradeTime << "', '" << tmpTradeList[j].OrderSysID << "', '" << ToNoZeroChar(tmpTradeList[j].TradeType) 
				<< "', '" << tmpTradeList[j].ExchangeID 
				<< "', '" << tmpTradeList[j].BrokerID << "', '" << tmpTradeList[j].InvestorID << "'" ;

			strSQLCmd = "INSERT INTO " + strTableName + "(TradeID, InstrumentID, Direction, OffsetFlag, HedgeFlag, Price, Volume, \
														 TradeDate, TradeTime, OrderSysID, TradeType, ExchangeID, BrokerID, InvestorID) VALUES(" + sstr.str() + ")";

			bool bExecuteSql = pTradeDb->ExecuteSqlCommand(strSQLCmd);
			if(bExecuteSql == false)
				cerr << "插入成交信息到 TradeLog 数据表出错！" << ENDLINE;
		}

		pCtpCntIf->m_PreTradeSavePos = tmpTradeList.size();
	} //for i, 交易柜台
}

//++
//存储各个交易柜台账户信息到数据库
//--
void CPosMoneyManage::SaveTradingAccountToDb()
{
	double SumTotalEquity = 0;

	//首选刷新各个柜台的账户信息和持仓信息
	SleepMs(1000);
	RefreshTradeAccountInfo();

	//计算并显示各个柜台的账户信息
	cerr << ENDLINE << m_pRealApp->m_Prompt << "各交易柜台资金账户信息: " << ENDLINE;
	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		string strPrompt = m_pRealApp->m_ArchName + "/" + pCtpCntIf->m_CounterName + "$";
		pCtpCntIf->CalcTotalEquity();
		SumTotalEquity += pCtpCntIf->m_TotalEquity;

		cerr << strPrompt << ENDLINE
			 << " TotalEquity: " << pCtpCntIf->m_TotalEquity
			 << " Balance: " << pCtpCntIf->m_TradingAccount.Balance
			 << " Available:" << pCtpCntIf->m_TradingAccount.Available   
			 << " CurrMargin:" << pCtpCntIf->m_TradingAccount.CurrMargin
			 << ENDLINE
			 << " CloseProfit:" << pCtpCntIf->m_TradingAccount.CloseProfit
			 << " PositionProfit:" <<pCtpCntIf->m_TradingAccount.PositionProfit
			 << " Commission:" << pCtpCntIf->m_TradingAccount.Commission
			 << " FrozenMargin:" << pCtpCntIf->m_TradingAccount.FrozenMargin
			 << ENDLINE;   
	}
	cerr << m_pRealApp->m_Prompt << " 各个柜台客户权益总和: SumTotalEquity = " << SumTotalEquity << ENDLINE << ENDLINE;

	//保存各柜台资金账户信息到数据库
	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		SYSTEMTIME CurTime;
		GetLocalTime(&CurTime);
		pCtpCntIf->SaveTradingAccountToDb();
		string strPrompt = m_pRealApp->m_ArchName + "/" + pCtpCntIf->m_CounterName + "$";
		cerr << strPrompt << " 资金账户信息已经写入数据库, 当前时间 " << CurTime.wYear << "-" << CurTime.wMonth << "-" << CurTime.wDay << " " << CurTime.wHour
					<< ":" << CurTime.wMinute << ":" << CurTime.wSecond << ENDLINE;
	}
}
