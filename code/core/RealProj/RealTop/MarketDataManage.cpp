#include "MarketDataManage.h"
#include "RealApp.h"
#include "TradeDb.h"
#include "StrategysManage.h"

#define MAX_TICK_NUM_TO_KEEP    (5)

//++
//构造函数
//--
CMarketDataManage::CMarketDataManage(CRealApp* pRealApp)
{
	m_pRealApp = pRealApp;
	m_TotalInstrumentNum = 0;
	memset(&m_PreCalTime, 0, sizeof(SYSTEMTIME));
}

//++
//析构函数
//--
CMarketDataManage::~CMarketDataManage()
{
	Cleanup();
}

//++
//初始化行情管理对象
//--
void CMarketDataManage::Initialize()
{
	//初始化各个期权的T型报价
	InitTshape();

	//获取各个期权的日K线数据并计算日间历史波动率
	for(size_t i = 0; i <  m_pTshapeBaseList.size(); i++)
	{
		for(size_t j = 0; j < m_pTshapeBaseList[i]->m_pTshapeMonthList.size(); j++)
		{
			vector<CDayKlineItem>* pDayKlineList;
			TThostFtdcInstrumentIDType UnderlyingInstId;
			//double Hv;

			pDayKlineList = &m_pTshapeBaseList[i]->m_pTshapeMonthList[j]->m_DayKlineList;
			strcpy(UnderlyingInstId, m_pTshapeBaseList[i]->m_pTshapeMonthList[j]->m_UnderlyingInstId);

			GetDayKlineData(UnderlyingInstId, pDayKlineList, m_pTshapeBaseList[i]->m_UnderlyingAssetType);

			//在套利中暂时不用计算历史波动率
			//Hv = CalcInterDayHv(pDayKlineList, HV_USED_DAYS);
			//if(pDayKlineList->size() > HV_USED_DAYS)
			//	m_pTshapeBaseList[i]->m_pTshapeMonthList[j].InterDayHv
		} //for j

		//初始化期权对平价套利收益率
		m_pTshapeBaseList[i]->InitOptionPairMeanRate(0);
	} //for i
}

//++
//退出前的清理工作
//--
void CMarketDataManage::Cleanup()
{
	for(size_t i = 0; i < m_pTshapeBaseList.size(); i++)
	{
		if(m_pTshapeBaseList[i] != NULL)
		{
			delete m_pTshapeBaseList[i];
			m_pTshapeBaseList[i] = NULL;
		}
	}
}

//++
//抽取期权商品列表中的期权代码，生成各自的T型报价
//--
void CMarketDataManage::InitTshape()
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
	vector<COptionKind>* pSynOptionKindList = &m_pRealApp->m_pStrategysManage->m_SynOptionKindList;

	DWORD waitReturn;
	TThostFtdcInstrumentIDType  instId;
	vector<COptionKind>::iterator ItOpt;

	//生成各种费率的数据表
	CreateMiscRateTables();

	//先将所有柜台的所有合约的信息都查询出来
	DWORD InstReqWaitTime = REQ_WAIT_TIME * 10;  //查询所有合约可能费时较长
	vector<CThostFtdcInstrumentField> SynInstInfoList;
	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CRealMdSpi*  pMdUserSpi = m_pRealApp->m_pCtpCntIfList[i]->m_pMdUserSpi; 
		CRealTraderSpi*  pTraderUserSpi = m_pRealApp->m_pCtpCntIfList[i]->m_pTraderUserSpi;
		vector<CThostFtdcInstrumentField> tmpInstInfoList;
		tmpInstInfoList.clear();

		strcpy(instId, "*");
		pTraderUserSpi->ReqQryInstrument(instId);
		waitReturn = WaitForSingleObject(m_pRealApp->m_pCtpCntIfList[i]->m_hTraderSpiEvent, InstReqWaitTime);   
		if(waitReturn != WAIT_OBJECT_0) 
		{
			cerr << m_pRealApp->m_Prompt << "超过 " << int(InstReqWaitTime/1000) << " 秒没有收到 " << instId << " 合约查询回应消息，程序返回！" << ENDLINE;
            PressAnyKeyToExit();
			exit(-1);
		}
		ResetEvent(m_pRealApp->m_pCtpCntIfList[i]->m_hTraderSpiEvent);
		SleepMs(REQ_INTERVAL);
		tmpInstInfoList.swap(m_pRealApp->m_pCtpCntIfList[i]->m_InstInfoList);
		SynInstInfoList.insert(SynInstInfoList.end(), tmpInstInfoList.begin(), tmpInstInfoList.end());
	}

	//统一获取经纪商参数, [0]是期权或期货柜台的经纪商交易参数, [1]是股票或ETF标的经纪商交易参数
	CThostFtdcBrokerTradingParamsField BrokerTradingParams[2];
	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CRealTraderSpi*  pTraderUserSpi = m_pRealApp->m_pCtpCntIfList[i]->m_pTraderUserSpi;
		pTraderUserSpi->ReqQryBrokerTradingParams();
		waitReturn = WaitForSingleObject(m_pRealApp->m_pCtpCntIfList[i]->m_hTraderSpiEvent, REQ_WAIT_TIME); 
		if(waitReturn != WAIT_OBJECT_0) 
		{
			cerr << m_pRealApp->m_Prompt << "超过 " << int(REQ_WAIT_TIME/1000) << " 秒没有收到 经纪商交易参数信息响应，程序返回！" << ENDLINE;
            PressAnyKeyToExit();
			exit(-1);
		}
		if(m_pRealApp->m_pCtpCntIfList[i]->m_CntCode == 'o' || m_pRealApp->m_pCtpCntIfList[i]->m_CntCode == 'f')  //股票期权或者期货期权
			memcpy(&BrokerTradingParams[0], &pTraderUserSpi->m_BrokerTradingParams, sizeof(CThostFtdcBrokerTradingParamsField));
		else if(m_pRealApp->m_pCtpCntIfList[i]->m_CntCode == 's')   //股票或者ETF标的
			memcpy(&BrokerTradingParams[1], &pTraderUserSpi->m_BrokerTradingParams, sizeof(CThostFtdcBrokerTradingParamsField));

		ResetEvent(m_pRealApp->m_pCtpCntIfList[i]->m_hTraderSpiEvent);
		SleepMs(REQ_INTERVAL);
	}

	//根据获取的期权类型代码，交易所信息等构造相应的T型报价
	for(ItOpt = pSynOptionKindList->begin(); ItOpt < pSynOptionKindList->end(); ItOpt++)
	{
		CTshapeBase* pTshapeBase;

		//先根据交易所信息实例化T型报价对象
		if(strcmp((*ItOpt).ExchangeId, "CZCE") == 0)
		{
			pTshapeBase = new CTshapeCzce((*ItOpt).OptionId);
		}
		else if(strcmp((*ItOpt).ExchangeId, "DCE") == 0)
		{
			pTshapeBase = new CTshapeDce((*ItOpt).OptionId);
		}
		else if(strcmp((*ItOpt).ExchangeId, "SHFE") == 0)
		{
			pTshapeBase = new CTshapeShfe((*ItOpt).OptionId);
		}
		else if(strcmp((*ItOpt).ExchangeId, "CFFEX") == 0)
		{
			pTshapeBase = new CTshapeCffex((*ItOpt).OptionId, (*ItOpt).UnderlyingId);
		}
		else if(strcmp((*ItOpt).ExchangeId, "SSE") == 0)
		{
			pTshapeBase = new CTshapeSse((*ItOpt).OptionId, (*ItOpt).UnderlyingAssetType);
		}
		else if(strcmp((*ItOpt).ExchangeId, "SZSE") == 0)
		{
			pTshapeBase = new CTshapeSzse((*ItOpt).OptionId, (*ItOpt).UnderlyingAssetType);
		}
		else
		{
			cerr << m_pRealApp->m_Prompt << "目前还不支持交易所：" << (*ItOpt).ExchangeId << " 期权， 程序返回！" << ENDLINE;
            PressAnyKeyToExit();
			exit(-1);
		}

		//确定T型报价的归属数据结构
		pTshapeBase->m_pRealApp = m_pRealApp;
		pTshapeBase->m_pOptionKind = &(*ItOpt);

		//确定T型报价期权和标的的交易柜台代码
		pTshapeBase->m_OptCntCode = ItOpt->OptCntCode;
		pTshapeBase->m_UnderCntCode = ItOpt->UnderCntCode;

		//确定需要构造的T型报价的年月份
		pTshapeBase->m_YearMonthList.clear();
		for(size_t j = 0; j < (*ItOpt).YearMonthList.size(); j++)
		{
			pTshapeBase->m_YearMonthList.push_back((*ItOpt).YearMonthList[j]);
		}

		//确定是否只构造标的报价链而不是整个T型报价
		pTshapeBase->m_bUnderMdOnly = (*ItOpt).bUnderMdOnly;
		pTshapeBase->ConstructTshape(SynInstInfoList);

		//初始化设置T型报价中与COptionKind相关的参数
		//对于CZCE和DCE，UnderlyingId和UnderlyingAssetType等不用再设置
		pTshapeBase->m_DriftRate = (*ItOpt).UnderlyingDriftRate;
		pTshapeBase->m_MeanRate = m_pRealApp->m_RiskFreeRate;
		pTshapeBase->m_MeanIV = (*ItOpt).UnderlyingDefaultHv;
		for(size_t j = 0; j < pTshapeBase->m_pTshapeMonthList.size(); j++)
		{
			pTshapeBase->m_pTshapeMonthList[j]->m_InterDayHv = (*ItOpt).UnderlyingDefaultHv;
			pTshapeBase->m_pTshapeMonthList[j]->m_IntraDayHv = (*ItOpt).UnderlyingDefaultHv;
		}

		//该期权T型报价存入列表
		m_pTshapeBaseList.push_back(pTshapeBase);
	} //for ItOpt

	//++
	//获取保证金率，手续费率，经纪商参数等
	//--
	for(size_t i = 0; i < m_pTshapeBaseList.size(); i++)
	{
		//找到一个有对应期权的标的合约（因此不会在到期月）
		size_t nSel;
		for(size_t j = 0; j < m_pTshapeBaseList[i]->m_pTshapeMonthList.size(); j++)
		{
			if(m_pTshapeBaseList[i]->m_pTshapeMonthList[j]->m_pOptionPairList.size() > 0)
			{
				nSel = j;
				break;
			}
		} //for j

		//只有标的合约，则选择离到期日最近的合约
		if(m_pTshapeBaseList[i]->m_bUnderMdOnly == true) 
			nSel =  0;

		//获取标的合约保证金率
		CThostFtdcInstrumentMarginRateField MarginRate;
		GetUnderlyingMarginRate(m_pTshapeBaseList[i]->m_UnderCntCode, m_pTshapeBaseList[i]->m_UnderlyingId, 
			m_pTshapeBaseList[i]->m_pTshapeMonthList[nSel]->m_UnderlyingInstId, MarginRate);
		memcpy(&m_pTshapeBaseList[i]->m_UnderlyingInstMarginRate, &MarginRate, sizeof(CThostFtdcInstrumentMarginRateField));

		//设置经纪商参数, [0]: 期权或期货经纪商交易参数, [1]: 股票或ETF经纪商交易参数
	    memcpy(&m_pTshapeBaseList[i]->m_BrokerTradingParams[0], &BrokerTradingParams[0], sizeof(CThostFtdcBrokerTradingParamsField));
	    memcpy(&m_pTshapeBaseList[i]->m_BrokerTradingParams[1], &BrokerTradingParams[1], sizeof(CThostFtdcBrokerTradingParamsField));
	
		//获取标的合约手续费率
		CThostFtdcInstrumentCommissionRateField CommissionRate;
		GetUnderlyingCommissionRate(m_pTshapeBaseList[i]->m_UnderCntCode, m_pTshapeBaseList[i]->m_UnderlyingId, 
			m_pTshapeBaseList[i]->m_pTshapeMonthList[nSel]->m_UnderlyingInstId, CommissionRate);
		memcpy(&m_pTshapeBaseList[i]->m_UnderlyingInstCommissionRate, &CommissionRate, sizeof(CThostFtdcInstrumentCommissionRateField));

		//获取期权合约手续费率
		//只有在生成完整T型报价的情况下才查询期权手续费，否则会出错
		if(m_pTshapeBaseList[i]->m_bUnderMdOnly == false)
		{
			TThostFtdcInstrumentIDType OptionInstId;
			//选取T型报价中间部分期权的合约代码
			int MidSize = m_pTshapeBaseList[i]->m_pTshapeMonthList[nSel]->m_pOptionPairList.size() >> 1;
			strcpy(OptionInstId, m_pTshapeBaseList[i]->m_pTshapeMonthList[nSel]->m_pOptionPairList[MidSize]->m_pCallItem->m_InstInfo.InstrumentID);

			if(m_pTshapeBaseList[i]->m_UnderlyingAssetType == 0 || m_pTshapeBaseList[i]->m_UnderlyingAssetType == 1)  //标的为商品期货或者股指期货
			{
				CThostFtdcOptionInstrCommRateField OptionInstrCommRate;
				GetOptionInstrCommRate(m_pTshapeBaseList[i]->m_OptCntCode, m_pTshapeBaseList[i]->m_OptionId, OptionInstId, OptionInstrCommRate);
				memcpy(&m_pTshapeBaseList[i]->m_OptionInstCommRate, &OptionInstrCommRate, sizeof(CThostFtdcOptionInstrCommRateField));
			}
#ifndef FUTURE_OPTION_ONLY
			else if(m_pTshapeBaseList[i]->m_UnderlyingAssetType == 2 || m_pTshapeBaseList[i]->m_UnderlyingAssetType == 3)  //标的为ETF或者股票
			{
				GetETFOptionInstrCommRate(m_pTshapeBaseList[i]->m_OptCntCode, m_pTshapeBaseList[i]->m_OptionId, OptionInstId, m_pTshapeBaseList[i]->m_ETFOptionInstrCommRate);
			}
#endif
		}

		//标的为ETF或者股票跳过以下处理
		if(m_pTshapeBaseList[i]->m_UnderlyingAssetType == 2 || m_pTshapeBaseList[i]->m_UnderlyingAssetType == 3)  
		{ 
			continue;
		}

		//获取期货期权各个月份的手续费率和保证金率(各月可能不相同)
		for(size_t j = 0; j < m_pTshapeBaseList[i]->m_pTshapeMonthList.size(); j++)
		{
			//获取当月期货合约的保证金率和手续费率
			CTshapeMonth *pTshapeMonth = m_pTshapeBaseList[i]->m_pTshapeMonthList[j];

			//获取保证金率
			GetUnderlyingMarginRate(m_pTshapeBaseList[i]->m_UnderCntCode, m_pTshapeBaseList[i]->m_UnderlyingId, 
				pTshapeMonth->m_UnderlyingInstId, MarginRate, pTshapeMonth->m_YearMonth);
			memcpy(&pTshapeMonth->m_UnderlyingInstMarginRate, &MarginRate, sizeof(CThostFtdcInstrumentMarginRateField));
	
			//获取标的合约手续费率
			GetUnderlyingCommissionRate(m_pTshapeBaseList[i]->m_UnderCntCode, m_pTshapeBaseList[i]->m_UnderlyingId, 
				pTshapeMonth->m_UnderlyingInstId, CommissionRate, pTshapeMonth->m_YearMonth);
			memcpy(&pTshapeMonth->m_UnderlyingInstCommissionRate, &CommissionRate, sizeof(CThostFtdcInstrumentCommissionRateField));

			//获取期货期权合约手续费率
			if(m_pTshapeBaseList[i]->m_bUnderMdOnly == false)
			{
				//可能会出现期权合约已经到期, 只有期货合约没有期权合约情形
				if(pTshapeMonth->m_pOptionPairList.size() > 0)
				{
					int MidSize = pTshapeMonth->m_pOptionPairList.size() >> 1;
					CThostFtdcInstrumentField *pOptionInstInfo = &pTshapeMonth->m_pOptionPairList[MidSize]->m_pCallItem->m_InstInfo;
					CThostFtdcOptionInstrCommRateField OptionInstrCommRate;
					GetOptionInstrCommRate(m_pTshapeBaseList[i]->m_OptCntCode, m_pTshapeBaseList[i]->m_OptionId, pOptionInstInfo->InstrumentID, 
						OptionInstrCommRate, pTshapeMonth->m_YearMonth);
					memcpy(&pTshapeMonth->m_OptionInstCommRate, &OptionInstrCommRate, sizeof(CThostFtdcOptionInstrCommRateField));
				}
				else
				{
					memcpy(&pTshapeMonth->m_OptionInstCommRate, &m_pTshapeBaseList[i]->m_OptionInstCommRate, sizeof(CThostFtdcOptionInstrCommRateField));
				}
			}
		} //for j
	} //for i
}

//++
//获取某一标的日K线数据
//参数
//    UnderlyingInstId: 标的合约代码
//    pDayKlineList: 指向日K数据的指针
//    UnderlyingAssetType: 标的类型
//返回值
//    无
//--
void CMarketDataManage::GetDayKlineData(TThostFtdcInstrumentIDType UnderlyingInstId, vector<CDayKlineItem>* pDayKlineList, size_t UnderlyingAssetType)
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
	string strTableNamePrefix;
    string strTableName;
	string strFieldDesc;
	string strSQLCmd;

	//检查数据表是否存在，不存在则创建
	int RetCreateDayK;
	strTableNamePrefix = "zzKline_";  //前缀加"zz"是为了在Access数据库中排序放在后面
	strTableName = strTableNamePrefix + UnderlyingInstId;
	//Open, Close等关键字不能用作数据表的字段名，否则会出错
	strFieldDesc =  "(ID INTEGER PRIMARY KEY AUTOINCREMENT, DateOfKline DATE UNIQUE, OpenPrice DOUBLE, ClosePrice DOUBLE, HighPrice DOUBLE, \
					 LowPrice DOUBLE, YdSettlement DOUBLE, YdClose DOUBLE, MeanIV DOUBLE, MeanRate DOUBLE)";
	RetCreateDayK = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
	if(RetCreateDayK < 0) 
	{
		cerr << m_pRealApp->m_Prompt << "数据表 " << strTableName << " 无法创建或者无法访问，程序返回！" << ENDLINE;
        PressAnyKeyToExit();
		pTradeDb->Destroy();
		exit(-1);
	}

	//获取K线数据，为节省空间，最多获取GET_KLINE_NUM日数据(Access用TOP NUM, MySQL用LIMIT NUM)
	stringstream sstr;
	sstr.clear(); sstr.str("");
	sstr << "SELECT * FROM " << strTableName << " ORDER BY DateOfKline DESC LIMIT " << GET_KLINE_NUM;
	strSQLCmd = sstr.str();

    int RetVal;
	char *errMsg = NULL;
	char **dbResult;
	int nRow, nColumn;
	int i, j;

	pDayKlineList->clear();
	RetVal = sqlite3_get_table(pTradeDb->m_pDB, strSQLCmd.c_str(), &dbResult, &nRow, &nColumn, &errMsg);
	if(RetVal != SQLITE_OK)
	{
		cerr << m_pRealApp->m_Prompt << "打开数据表: " << strTableName << " 出错: " << errMsg << ENDLINE;
		sqlite3_free(errMsg);		
	}
	else
	{   //数据表存在
		//设置FieldName List
        vector<string> FieldNameList;
		int index = 0;
        for(j = 0; j < nColumn; j++)
        {
           FieldNameList.push_back(dbResult[j]);
        }
		
		//按行读出数据表中的数据
        for(i = 0; i < nRow; i++)
        {
		    string strFieldValue;
		    CDayKlineItem DayKlineItem;

            index = pTradeDb->FieldIndex("OpenPrice", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
		    DayKlineItem.Open = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("ClosePrice", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
		    DayKlineItem.Close = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("HighPrice", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
		    DayKlineItem.High = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("LowPrice", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
		    DayKlineItem.Low = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("YdSettlement", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
		    DayKlineItem.YdSettlement = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("YdClose", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
		    DayKlineItem.YdClose = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("MeanIV", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
		    DayKlineItem.MeanIV = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("MeanRate", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
		    DayKlineItem.MeanRate = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("DateOfKline", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
		    DayKlineItem.DateOfKline = SqlDateToCDate(strFieldValue);

			//从数据库中获取的存在极大极小值的K线数据不能用（可能是在夜盘存入）
			//对于股票或者ETF结算值不存在
			if(IsExtremePrice(DayKlineItem.Open) || IsExtremePrice(DayKlineItem.Close) || IsExtremePrice(DayKlineItem.High)
				|| IsExtremePrice(DayKlineItem.Low) || (IsExtremePrice(DayKlineItem.YdSettlement) 
				&& (UnderlyingAssetType == 0 || UnderlyingAssetType == 1))			
				|| IsExtremePrice(DayKlineItem.YdClose))
				;   //空操作, 不能使用continue, 否则执行不到MoveNext()
			else
				pDayKlineList->push_back(DayKlineItem);
        } //nRow loop
	}
	sqlite3_free_table(dbResult);
}

//++
//订阅所有T型报价的市场行情
//参数
//    CntCode: 柜台代码, = 0表示所有柜台只用于初始化过程
//返回值
//    无
//--
void CMarketDataManage::SubscribeAllMarketData(char CntCode)
{
	vector<string> strInstIdList;
	vector<string> strCntInstIdList;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		if(CntCode == 0 || m_pRealApp->m_pCtpCntIfList[i]->m_CntCode == CntCode)
		{
			CRealMdSpi*  pMdUserSpi = m_pRealApp->m_pCtpCntIfList[i]->m_pMdUserSpi; 
			strInstIdList.clear();

			//获取各个T型报价需要获取市场行情的合约代码列表
			for(size_t j = 0; j < m_pTshapeBaseList.size(); j++)
			{
				m_pTshapeBaseList[j]->GenInstrumentList(m_pRealApp->m_pCtpCntIfList[i]->m_CntCode, strCntInstIdList);
				strInstIdList.insert(strInstIdList.end(), strCntInstIdList.begin(), strCntInstIdList.end());
			}

			//得到所有合约的总数目, 在初始化时才求此值
			if(CntCode == 0)
			{
				m_TotalInstrumentNum += strInstIdList.size();
			}

			//订阅行情
			pMdUserSpi->SubscribeMarketData(strInstIdList);
		} //if, 是欲订阅行情的柜台
	} //for i, 交易柜台
}

//++
//更新所有的T型报价
//参数
//    bArbOnly: true: 仅用于套利; false: 用于更广泛的目的
//返回值
//    无
//--
void CMarketDataManage::UpdateAllTshape(bool bArbOnly)
{
	SYSTEMTIME CurTime;
	GetLocalTime(&CurTime);
	double CurSecs = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;
	double PreSecs = m_PreCalTime.wHour*3600 + m_PreCalTime.wMinute*60 + m_PreCalTime.wSecond + m_PreCalTime.wMilliseconds*0.001;
	double CalInterval = TSHAPE_CAL_INTERVAL; //3秒计算一次, 和标的行情间隔相当

	//合并各个柜台的市场行情数据
	vector<CThostFtdcDepthMarketDataField> DepthMarketDataList;
	for(size_t j = 0; j < m_pRealApp->m_pCtpCntIfList.size(); j++)
	{
		vector<CThostFtdcDepthMarketDataField>* pCntMDList = &m_pRealApp->m_pCtpCntIfList[j]->m_DepthMarketDataList;
		//确保每个柜台有行情更新时都重新计算相关值
		if(pCntMDList->size() > 0)
		{
			DepthMarketDataList.insert(DepthMarketDataList.end(), pCntMDList->begin(), pCntMDList->end());
		}
	} //for j, 各个柜台

	//更新各个期权品种的T型报价
	for(size_t i = 0; i < m_pTshapeBaseList.size(); i++)
	{
		if(DepthMarketDataList.size() > 0)
		{
			m_pTshapeBaseList[i]->UpdateTshape(DepthMarketDataList);

			//固定时间间隔计算隐含波动率及希腊值等参数
			if(CurSecs >= (PreSecs+CalInterval))
			{
				//测试用
				//#define TIME_MEASURE
				#ifdef TIME_MEASURE
					double BeginSecs = omp_get_wtime( );
				#endif

				m_pTshapeBaseList[i]->MathematicalRefreshment(bArbOnly);
				memcpy(&m_PreCalTime, &CurTime, sizeof(SYSTEMTIME));

				//测试用
				#ifdef TIME_MEASURE
					double EndSecs = omp_get_wtime( );
					cerr << "计算希腊值和隐含波动率计算花费时间 = " << (EndSecs - BeginSecs) << ENDLINE;
				#endif
			}
		}
	} //for i, 各个品种T型报价
}

//++
//更新所有的T型报价的合约交易状态信息
//参数
//    无
//返回值
//    无
//--
void CMarketDataManage::UpdateAllTshapeInstrumentStatus()
{
	//合并各个柜台的合约交易状态数据
	vector<CThostFtdcInstrumentStatusField> InstrumentStatusList;
	for(size_t j = 0; j < m_pRealApp->m_pCtpCntIfList.size(); j++)
	{
		vector<CThostFtdcInstrumentStatusField>* pCntInstrumentStatusList = &m_pRealApp->m_pCtpCntIfList[j]->m_InstrumentStatusList;
		//确保每个柜台有合约交易状态更新时都更新T型报价各个合约的交易状态数据
		if(pCntInstrumentStatusList->size() > 0)
		{
			InstrumentStatusList.insert(InstrumentStatusList.end(), pCntInstrumentStatusList->begin(), pCntInstrumentStatusList->end());
		}
	} //for j, 各个柜台

	//更新各个期权品种的T型报价
	for(size_t i = 0; i < m_pTshapeBaseList.size(); i++)
	{
		if(InstrumentStatusList.size() > 0)
		{
			m_pTshapeBaseList[i]->UpdateTshapeInstrumentStatus(InstrumentStatusList);
		}
	} //for i, 各个品种T型报价
}

//++
//将当日各个标的合约的K线数据导出到数据库
//--
void CMarketDataManage::DumpDayKLine()
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
	string strTableNamePrefix;
    string strTableName;
	string strSQLCmd;
	SYSTEMTIME CurTime;
	stringstream sstr;
	string strDate;

	GetLocalTime(&CurTime);

	strTableNamePrefix = "zzKline_";  //前缀加"zz"是为了在Access数据库中排序放在后面

	for(size_t i = 0; i < m_pTshapeBaseList.size(); i++)
	{
		for(size_t j = 0; j < m_pTshapeBaseList[i]->m_pTshapeMonthList.size(); j++)
		{
			CThostFtdcDepthMarketDataField* pMarketData = &m_pTshapeBaseList[i]->m_pTshapeMonthList[j]->m_UnderlyingMarketData;  //缺省值
			strTableName = strTableNamePrefix + m_pTshapeBaseList[i]->m_pTshapeMonthList[j]->m_UnderlyingInstId;

			//收盘后TradingDay有可能为空（中信期货模拟盘就是这种情况）, 先寻找TradingDay不为空的市场行情数据
			string strTradingDay = pMarketData->TradingDay;  //缺省值
			RemoveSpace(strTradingDay);

			m_pTshapeBaseList[i]->m_pTshapeMonthList[j]->readlock();
			size_t k = m_pTshapeBaseList[i]->m_pTshapeMonthList[j]->m_UnderlyingTickDataList.size();
			do
			{
				if(m_pTshapeBaseList[i]->m_pTshapeMonthList[j]->m_UnderlyingTickDataList.size() == 0) break;
				if(strTradingDay != "") break;

				k = k - 1;
				strTradingDay = m_pTshapeBaseList[i]->m_pTshapeMonthList[j]->m_UnderlyingTickDataList[k].TradingDay;
				RemoveSpace(strTradingDay);
				pMarketData = &m_pTshapeBaseList[i]->m_pTshapeMonthList[j]->m_UnderlyingTickDataList[k];
			} while(k > 0);
			m_pTshapeBaseList[i]->m_pTshapeMonthList[j]->readunlock();

			if(strTradingDay.size() >= 8)
			{
				strDate = strTradingDay.substr(0, 4) + "-" + strTradingDay.substr(4, 2) + "-" + strTradingDay.substr(6, 2);
			}
			else
			{
				SYSTEMTIME KlineTime;
				KlineTime = CalKLineDate(CurTime);
				sstr.clear(); sstr.str("");
				sstr << setw(4) << setfill('0') << KlineTime.wYear << "-" << setw(2) << setfill('0') << KlineTime.wMonth << "-" 
				     << setw(2) << setfill('0') << KlineTime.wDay;
				strDate = sstr.str();
			}

			//如果行情数据不对，则需要重新计算Open, Close, High和Low
			//目前看来，只有ClosePrice不正常，需要过滤掉异常条件
			double ClosePrice;
			if(abs(pMarketData->ClosePrice - pMarketData->LastPrice) > 0.2*pMarketData->LastPrice)
				ClosePrice = pMarketData->LastPrice;
			else
				ClosePrice = pMarketData->ClosePrice;

			//隐含波动率和隐含年化收益率
			double MeanIV = 0;
			double MeanRate = 0;
			AvgTodayRateAndIV(m_pTshapeBaseList[i]->m_pTshapeMonthList[j], MeanRate, MeanIV);

			sstr.clear(); sstr.str(""); sstr.precision(18);
			sstr << "'" << strDate << "', " << pMarketData->OpenPrice << ", " << ClosePrice << ", " << pMarketData->HighestPrice <<  ", "
					<< pMarketData->LowestPrice << ", " << pMarketData->PreSettlementPrice << ", " << pMarketData->PreClosePrice <<  ", "
					<< MeanIV << ", " << MeanRate;

			//对于股票或者ETF没有结算价一说
			if(IsExtremePrice(pMarketData->OpenPrice) || IsExtremePrice(ClosePrice) || IsExtremePrice(pMarketData->HighestPrice)
				|| IsExtremePrice(pMarketData->LowestPrice) || (IsExtremePrice(pMarketData->PreSettlementPrice) 
				&& (m_pTshapeBaseList[i]->m_UnderlyingAssetType == 0 || m_pTshapeBaseList[i]->m_UnderlyingAssetType == 1))
				|| IsExtremePrice(pMarketData->PreClosePrice))
			{
				;  //不作任何操作
			}
			else
			{
				//先从数据表中删除当日K线数据
				strSQLCmd = "DELETE FROM " + strTableName + " WHERE DateOfKline='" + strDate + "'";
				pTradeDb->ExecuteSqlCommand(strSQLCmd);

				//再插入当前日K线记录
				strSQLCmd = "INSERT INTO " + strTableName + "(DateOfKline, OpenPrice, ClosePrice, HighPrice, LowPrice, YdSettlement, YdClose, \
							MeanIV, MeanRate) VALUES(" + sstr.str() + ")";
				pTradeDb->ExecuteSqlCommand(strSQLCmd);
			}

			if(m_pTshapeBaseList[i]->m_UnderlyingAssetType == 2 || m_pTshapeBaseList[i]->m_UnderlyingAssetType == 3)  //标的为ETF或者股票，只存一次K线即可
				break;
		} //for j
	} //for i
}

//++
//利用K线数据和T型报价计算当天的平均波动率和平均年化收益率
//参数
//    pTshapeMonth: 指向月份T型报价的指针
//    MeanRate: 引用, 平均年化收益率
//    MeanIV: 引用, 平均年化隐含波动率
//返回值
//    无
//--
void CMarketDataManage::AvgTodayRateAndIV(CTshapeMonth* pTshapeMonth, double &MeanRate, double &MeanIV)
{
	CTshapeBase* pTshapeBase = pTshapeMonth->m_pParent;
	if(pTshapeBase->m_UnderlyingAssetType == 2 || pTshapeBase->m_UnderlyingAssetType == 3)  //ETF或者股票
	{
		pTshapeBase->readlock();
		MeanRate = pTshapeBase->m_MeanRate;
		MeanIV = pTshapeBase->m_MeanIV;
		pTshapeBase->readunlock();
	}
	else
	{
		pTshapeMonth->readlock();
		MeanRate = pTshapeMonth->m_MeanRate;
		MeanIV = pTshapeMonth->m_MeanIV;
		pTshapeMonth->readunlock();
	}

	//判断是否有当日K线数据, 如果有则做平均
	vector<CDayKlineItem>* pDayKlineList = &pTshapeMonth->m_DayKlineList;
	if(pDayKlineList->size() > 0)
	{
		CDayKlineItem *pLatestKLine = &(*pDayKlineList)[0];
		CDate *pLatestDay = &pLatestKLine->DateOfKline;
		SYSTEMTIME CurTime;
		GetLocalTime(&CurTime);

		if(pLatestDay->year == CurTime.wYear && pLatestDay->month == CurTime.wMonth && pLatestDay->day == CurTime.wDay)
		{
			MeanRate = (MeanRate + pLatestKLine->MeanRate) * 0.5;
			MeanIV = (MeanIV + pLatestKLine->MeanIV) * 0.5;
		}
	}
}

//++
//获取深度市场行情数据
//参数
//    无
//返回值
//    市场行情数据列表的大小
//--
size_t CMarketDataManage::GetDepthMarketDataList()
{
	size_t MarketDataSize = 0;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CRealMdSpi*  pMdUserSpi = m_pRealApp->m_pCtpCntIfList[i]->m_pMdUserSpi; 

		//为避免数据损失，市场行情数据由应用程序主动获取
		//如果应用程序没有动作，则数据一直累积，不会丢失
		vector<CThostFtdcDepthMarketDataField> tmpDepthMarketDataList;
		tmpDepthMarketDataList.clear();

		//使用互斥信号保护市场行情列表
		EnterCriticalSection(&m_pRealApp->m_pCtpCntIfList[i]->m_DepthMarketDataCS);

		m_pRealApp->m_pCtpCntIfList[i]->m_DepthMarketDataList.swap(pMdUserSpi->m_DepthMarketDataList);
		pMdUserSpi->m_DepthMarketDataList.swap(tmpDepthMarketDataList);
		//复位信号也使用互斥区保护
		ResetEvent(m_pRealApp->m_pCtpCntIfList[i]->m_hMdSpiEvent);

		LeaveCriticalSection(&m_pRealApp->m_pCtpCntIfList[i]->m_DepthMarketDataCS);

		//为防止大量的市场行情数据阻塞处理过程，需要对m_DepthMarketDataList作限制
		size_t MdSize = m_pRealApp->m_pCtpCntIfList[i]->m_DepthMarketDataList.size();
		size_t MaxMdSize = m_TotalInstrumentNum*MAX_TICK_NUM_TO_KEEP;
		if(MdSize > MaxMdSize)
		{
			vector<CThostFtdcDepthMarketDataField> tmpMdList;
			tmpMdList.clear();
			vector<CThostFtdcDepthMarketDataField>::iterator ItBegin = m_pRealApp->m_pCtpCntIfList[i]->m_DepthMarketDataList.end() - MaxMdSize;
			tmpMdList.insert(tmpMdList.end(), ItBegin, m_pRealApp->m_pCtpCntIfList[i]->m_DepthMarketDataList.end());
			m_pRealApp->m_pCtpCntIfList[i]->m_DepthMarketDataList.swap(tmpMdList);
		}

		MarketDataSize += m_pRealApp->m_pCtpCntIfList[i]->m_DepthMarketDataList.size();
	}

	return MarketDataSize;
}

//++
//获取合约交易状态信息列表
//参数
//    无
//返回值
//    合约交易状态信息列表的大小
//--
size_t CMarketDataManage::GetInstrumentStatusList()
{
	size_t InstrumentStatusSize = 0;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CRealTraderSpi*  pTraderUserSpi = m_pRealApp->m_pCtpCntIfList[i]->m_pTraderUserSpi; 

		//为避免数据损失，合约交易状态信息由应用程序主动获取
		//如果应用程序没有动作，则数据一直累积，不会丢失
		vector<CThostFtdcInstrumentStatusField> tmpInstrumentStatusList;
		tmpInstrumentStatusList.clear();

		//使用互斥信号保护市场行情列表
		EnterCriticalSection(&m_pRealApp->m_pCtpCntIfList[i]->m_InstrumentStatusCS);

		m_pRealApp->m_pCtpCntIfList[i]->m_InstrumentStatusList.swap(pTraderUserSpi->m_InstrumentStatusList);
		pTraderUserSpi->m_InstrumentStatusList.swap(tmpInstrumentStatusList);
		//复位信号也使用互斥区保护
		ResetEvent(m_pRealApp->m_pCtpCntIfList[i]->m_hInstrumentStatusEvent);

		LeaveCriticalSection(&m_pRealApp->m_pCtpCntIfList[i]->m_InstrumentStatusCS);

		//为防止大量的合约交易状态数据阻塞处理过程，需要对m_InstrumentStatusList作限制
		size_t StatusSize = m_pRealApp->m_pCtpCntIfList[i]->m_InstrumentStatusList.size();
		size_t MaxStatusSize = m_TotalInstrumentNum*MAX_TICK_NUM_TO_KEEP;
		if(StatusSize > MaxStatusSize)
		{
			vector<CThostFtdcInstrumentStatusField> tmpStatusList;
			tmpStatusList.clear();
			vector<CThostFtdcInstrumentStatusField>::iterator ItBegin = m_pRealApp->m_pCtpCntIfList[i]->m_InstrumentStatusList.end() - MaxStatusSize;
			tmpStatusList.insert(tmpStatusList.end(), ItBegin, m_pRealApp->m_pCtpCntIfList[i]->m_InstrumentStatusList.end());
			m_pRealApp->m_pCtpCntIfList[i]->m_InstrumentStatusList.swap(tmpStatusList);
		}

		InstrumentStatusSize += m_pRealApp->m_pCtpCntIfList[i]->m_InstrumentStatusList.size();
	}

	return InstrumentStatusSize;
}

//++
//生成各种费率的数据表
//注意：为了防止特殊字符导致字符串出错，字符型数据要转换为INT型存入数据表
//--
void CMarketDataManage::CreateMiscRateTables()
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;

    string strTableName;
	string strFieldDesc;
	int RetCreateTb;

	//生成标的保证金率的数据表
	strTableName = "UnderlyingMarginRate";
	strFieldDesc =  "(ID INTEGER PRIMARY KEY AUTOINCREMENT, UnderlyingId VARCHAR, InstrumentID VARCHAR, YearMonth INT, InvestorRange INT, \
					 BrokerID VARCHAR, InvestorID VARCHAR, \
					HedgeFlag INT, LongMarginRatioByMoney DOUBLE, LongMarginRatioByVolume DOUBLE, ShortMarginRatioByMoney DOUBLE, \
					ShortMarginRatioByVolume DOUBLE, IsRelative INT, UNIQUE(UnderlyingId, YearMonth, BrokerID, InvestorID))";

	RetCreateTb = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
	if(RetCreateTb < 0) 
	{
		cerr << m_pRealApp->m_Prompt << "数据表 " << strTableName << " 无法创建或者无法访问" << ENDLINE;
        PressAnyKeyToExit();
		pTradeDb->Destroy();
		exit(-1);
	}

	//生成标的手续费率的数据表
	strTableName = "UnderlyingCommissionRate";
	strFieldDesc =  "(ID INTEGER PRIMARY KEY AUTOINCREMENT, UnderlyingId VARCHAR, InstrumentID VARCHAR, YearMonth INT, \
					 InvestorRange INT, BrokerID VARCHAR, InvestorID VARCHAR, \
					OpenRatioByMoney DOUBLE, OpenRatioByVolume DOUBLE, CloseRatioByMoney DOUBLE, CloseRatioByVolume DOUBLE, \
					CloseTodayRatioByMoney DOUBLE, CloseTodayRatioByVolume DOUBLE, ExchangeID VARCHAR, BizType  INT,  \
					UNIQUE(UnderlyingId, YearMonth, BrokerID, InvestorID))";

	RetCreateTb = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
	if(RetCreateTb < 0) 
	{
		cerr << m_pRealApp->m_Prompt << "数据表 " << strTableName << " 无法创建或者无法访问" << ENDLINE;
        PressAnyKeyToExit();
		pTradeDb->Destroy();
		exit(-1);
	}

	//生成期权手续费率数据表
	strTableName = "OptionInstrCommRate";
	strFieldDesc =  "(ID INTEGER PRIMARY KEY AUTOINCREMENT, OptionId VARCHAR, InstrumentID VARCHAR, YearMonth INT, \
					 InvestorRange INT, BrokerID VARCHAR, InvestorID VARCHAR, \
					OpenRatioByMoney DOUBLE, OpenRatioByVolume DOUBLE, CloseRatioByMoney DOUBLE, CloseRatioByVolume DOUBLE, \
					CloseTodayRatioByMoney DOUBLE, CloseTodayRatioByVolume DOUBLE, StrikeRatioByMoney DOUBLE, StrikeRatioByVolume DOUBLE, \
					ExchangeID VARCHAR, UNIQUE(OptionId, YearMonth, BrokerID, InvestorID))";

	RetCreateTb = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
	if(RetCreateTb < 0) 
	{
		cerr << m_pRealApp->m_Prompt << "数据表 " << strTableName << " 无法创建或者无法访问" << ENDLINE;
        PressAnyKeyToExit();
		pTradeDb->Destroy();
		exit(-1);
	}

#ifndef FUTURE_OPTION_ONLY
	//生成ETF期权手续费率数据表, ETF期权费率和HedgeFlag以及PosiDirection有关
	strTableName = "ETFOptionInstrCommRate";
	strFieldDesc =  "(ID INTEGER PRIMARY KEY AUTOINCREMENT, OptionId VARCHAR, InstrumentID VARCHAR, YearMonth INT, \
					 InvestorRange INT, BrokerID VARCHAR, InvestorID VARCHAR, \
					OpenRatioByMoney DOUBLE, OpenRatioByVolume DOUBLE, CloseRatioByMoney DOUBLE, CloseRatioByVolume DOUBLE, \
					CloseTodayRatioByMoney DOUBLE, CloseTodayRatioByVolume DOUBLE, StrikeRatioByMoney DOUBLE, StrikeRatioByVolume DOUBLE, \
					ExchangeID VARCHAR, HedgeFlag INT, PosiDirection INT, UNIQUE(OptionId, YearMonth, BrokerID, InvestorID, HedgeFlag, PosiDirection))";

	RetCreateTb = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
	if(RetCreateTb < 0) 
	{
		cerr << m_pRealApp->m_Prompt << "数据表 " << strTableName << " 无法创建或者无法访问" << ENDLINE;
        PressAnyKeyToExit();
		pTradeDb->Destroy();
		exit(-1);
	}
#endif
}

//++
//获取标的的保证金率
//参数
//    CntCode: 标的交易柜台代码
//    UnderlyingId: 标的品种代码
//    InstrumentID: 用以查询的某个具体标的合约代码
//    MarginRate: 引用参数，保证金费率结构变量
//    YearMonth: 标的期货合约的月份
//返回值
//    无
//--
void CMarketDataManage::GetUnderlyingMarginRate(char CntCode, string UnderlyingId, TThostFtdcInstrumentIDType InstrumentID, CThostFtdcInstrumentMarginRateField& MarginRate, int YearMonth)
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
	CRealTraderSpi*  pTraderUserSpi = NULL;
	CCtpCntIf* pCtpCntIf = NULL;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		if(m_pRealApp->m_pCtpCntIfList[i]->m_CntCode == CntCode)
		{
			pTraderUserSpi = m_pRealApp->m_pCtpCntIfList[i]->m_pTraderUserSpi;
			pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		}
	}

	if(pCtpCntIf == NULL)
	{
		cerr << m_pRealApp->m_Prompt << " " << InstrumentID << "： 找不到交易柜台!" << ENDLINE;
		return;
	}

	string strTableName = "UnderlyingMarginRate";
	string strSQLCmd;
	stringstream sstr;
	string SqlCondition;

    int RetVal;
	char *errMsg = NULL;
	char **dbResult;
	int nRow, nColumn;
	int i, j;	

	//先执行一次查询操作
	sstr.clear(); sstr.str("");
	sstr << " WHERE (UnderlyingId = '" << UnderlyingId << "') AND (BrokerID = '" << pCtpCntIf->m_brokerId << "') AND (InvestorID = '"
		 << pCtpCntIf->m_userId << "') AND (YearMonth = " << YearMonth << ")";
	SqlCondition = sstr.str();
	strSQLCmd = "SELECT * FROM " + strTableName + SqlCondition; 
	RetVal = sqlite3_get_table(pTradeDb->m_pDB, strSQLCmd.c_str(), &dbResult, &nRow, &nColumn, &errMsg);

	//如果没有在数据库中查到保证金率信息，或者程序要求重新从API查询
	if(RetVal != SQLITE_OK || nRow <= 0 || m_pRealApp->m_bReQryRateParams == true)
	{
		if(errMsg != NULL)
		{
		    sqlite3_free(errMsg);				
		}

		//查询合约保证金率
		pTraderUserSpi->ReqQryInstrumentMarginRate(InstrumentID);
		DWORD waitReturn = WaitForSingleObject(pCtpCntIf->m_hTraderSpiEvent, REQ_WAIT_TIME); 
		if(waitReturn != WAIT_OBJECT_0) 
		{
			cerr << m_pRealApp->m_Prompt << "超过 " << int(REQ_WAIT_TIME/1000) << " 秒没有收到 " << InstrumentID << " 保证金率查询回应消息，程序返回！" << ENDLINE;
            PressAnyKeyToExit();
			exit(-1);
		}
		memcpy(&MarginRate, &pTraderUserSpi->m_InstrumentMarginRate, sizeof(CThostFtdcInstrumentMarginRateField));
		ResetEvent(pCtpCntIf->m_hTraderSpiEvent);
		SleepMs(REQ_INTERVAL);

		//强行删除数据库中该合约保证金率的旧有记录
		strSQLCmd = "DELETE FROM " + strTableName + SqlCondition;
		pTradeDb->ExecuteSqlCommand(strSQLCmd);

		//插入保证金率的新记录
		sstr.clear(); sstr.str(""); sstr.precision(18);
		sstr << "'" << UnderlyingId << "', '" << InstrumentID << "', " << YearMonth << ", " << int(MarginRate.InvestorRange) << ", '" << pCtpCntIf->m_brokerId << "', '"
			<< pCtpCntIf->m_userId << "', " << int(MarginRate.HedgeFlag) << ", " << MarginRate.LongMarginRatioByMoney << ", "
			<< MarginRate.LongMarginRatioByVolume << ", " << MarginRate.ShortMarginRatioByMoney << ", " << MarginRate.ShortMarginRatioByVolume << ", "
			<< MarginRate.IsRelative;

	    strSQLCmd = "INSERT INTO " + strTableName + "(UnderlyingId, InstrumentID, YearMonth, InvestorRange, BrokerID, InvestorID, HedgeFlag, LongMarginRatioByMoney, \
					 LongMarginRatioByVolume, ShortMarginRatioByMoney, ShortMarginRatioByVolume, IsRelative) VALUES(" + sstr.str() + ")";
		pTradeDb->ExecuteSqlCommand(strSQLCmd);
	}
	else
	{   //数据表存在
		//设置FieldName List
        vector<string> FieldNameList;
		int index = 0;
        for(j = 0; j < nColumn; j++)
        {
           FieldNameList.push_back(dbResult[j]);
        }

		//按行读出数据表中的数据
        for(i = 0; i < nRow; i++)
        {
		    string strFieldValue;

            index = pTradeDb->FieldIndex("InstrumentID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(MarginRate.InstrumentID, strFieldValue.c_str());
			
            index = pTradeDb->FieldIndex("InvestorRange", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			MarginRate.InvestorRange = atoi(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("BrokerID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(MarginRate.BrokerID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("InvestorID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(MarginRate.InvestorID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("HedgeFlag", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			MarginRate.HedgeFlag = atoi(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("LongMarginRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			MarginRate.LongMarginRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("LongMarginRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			MarginRate.LongMarginRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("ShortMarginRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			MarginRate.ShortMarginRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("ShortMarginRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			MarginRate.ShortMarginRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("IsRelative", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			MarginRate.IsRelative = atoi(strFieldValue.c_str());
		} //nRow		
	}

	sqlite3_free_table(dbResult);
}

//++
//获取标的的手续费率
//参数
//    CntCode: 交易柜台代码
//    UnderlyingId: 标的品种代码
//    InstrumentID: 用以查询的某个具体标的合约代码
//    CommissionRate: 引用参数，手续费费率结构变量
//    YearMonth: 标的期货合约的月份
//返回值
//    无
//--

void CMarketDataManage::GetUnderlyingCommissionRate(char CntCode, string UnderlyingId, TThostFtdcInstrumentIDType InstrumentID, CThostFtdcInstrumentCommissionRateField& CommissionRate, int YearMonth)
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
	CRealTraderSpi*  pTraderUserSpi = NULL;
	CCtpCntIf* pCtpCntIf = NULL;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		if(m_pRealApp->m_pCtpCntIfList[i]->m_CntCode == CntCode)
		{
			pTraderUserSpi = m_pRealApp->m_pCtpCntIfList[i]->m_pTraderUserSpi;
			pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		}
	}

	if(pCtpCntIf == NULL)
	{
		cerr << m_pRealApp->m_Prompt << " " << InstrumentID << "： 找不到交易柜台!" << ENDLINE;
		return;
	}

	string strTableName = "UnderlyingCommissionRate";;
	string strSQLCmd;
	stringstream sstr;
	string SqlCondition;

    int RetVal;
	char *errMsg = NULL;
	char **dbResult;
	int nRow, nColumn;
	int i, j;	

	//先执行一次查询操作
	sstr.clear(); sstr.str("");
	sstr << " WHERE (UnderlyingId = '" << UnderlyingId << "') AND (BrokerID = '" << pCtpCntIf->m_brokerId << "') AND (InvestorID = '"
		 << pCtpCntIf->m_userId << "') AND (YearMonth = " << YearMonth << ")";
	SqlCondition = sstr.str();
	strSQLCmd = "SELECT * FROM " + strTableName + SqlCondition; 
	RetVal = sqlite3_get_table(pTradeDb->m_pDB, strSQLCmd.c_str(), &dbResult, &nRow, &nColumn, &errMsg);

	//如果没有在数据库中查到手续费率信息，或者程序要求重新从API查询
	if(RetVal != SQLITE_OK || nRow <= 0 || m_pRealApp->m_bReQryRateParams == true)
	{
		if(errMsg != NULL)
		{
		    sqlite3_free(errMsg);				
		}

		//查询合约手续费率
		pTraderUserSpi->ReqQryInstrumentCommissionRate(InstrumentID);
		DWORD waitReturn = WaitForSingleObject(pCtpCntIf->m_hTraderSpiEvent, REQ_WAIT_TIME); 
		if(waitReturn != WAIT_OBJECT_0) 
		{
			cerr << m_pRealApp->m_Prompt << "超过 " << int(REQ_WAIT_TIME/1000) << " 秒没有收到 " << InstrumentID << " 标的合约手续费率查询回应消息，程序返回！" << ENDLINE;
            PressAnyKeyToExit();
			exit(-1);
		}
		memcpy(&CommissionRate, &pTraderUserSpi->m_InstrumentCommissionRate, sizeof(CThostFtdcInstrumentCommissionRateField));
		ResetEvent(pCtpCntIf->m_hTraderSpiEvent);
		SleepMs(REQ_INTERVAL);

		//强行删除数据库中该合约保证金率的旧有记录
		strSQLCmd = "DELETE FROM " + strTableName + SqlCondition;
		pTradeDb->ExecuteSqlCommand(strSQLCmd);

		//插入手续费率的新记录
		//注意：字符型数据要强制转换为int型
		//查询获取的BrokerID, InvestorID, ExchangeID等有可能为空
		sstr.clear(); sstr.str(""); sstr.precision(18);
		sstr << "'" << UnderlyingId << "', '" << InstrumentID << "', " << YearMonth << ", " << int(CommissionRate.InvestorRange) << ", '" << pCtpCntIf->m_brokerId << "', '"
			<< pCtpCntIf->m_userId << "', " << CommissionRate.OpenRatioByMoney << ", " << CommissionRate.OpenRatioByVolume << ", "
			<< CommissionRate.CloseRatioByMoney << ", " << CommissionRate.CloseRatioByVolume << ", " << CommissionRate.CloseTodayRatioByMoney << ", "
			<< CommissionRate.CloseTodayRatioByVolume << ", '" << CommissionRate.ExchangeID << "', " << int(CommissionRate.BizType);

	    strSQLCmd = "INSERT INTO " + strTableName + " (UnderlyingId, InstrumentID, YearMonth, InvestorRange, BrokerID, InvestorID, OpenRatioByMoney, OpenRatioByVolume, \
					 CloseRatioByMoney, CloseRatioByVolume, CloseTodayRatioByMoney, CloseTodayRatioByVolume, ExchangeID, BizType) VALUES(" + sstr.str() + ")";
		pTradeDb->ExecuteSqlCommand(strSQLCmd);
	}
	else
	{   //数据表存在
		//设置FieldName List
        vector<string> FieldNameList;
		int index = 0;
        for(j = 0; j < nColumn; j++)
        {
           FieldNameList.push_back(dbResult[j]);
        }

		//按行读出数据表中的数据
        for(i = 0; i < nRow; i++)
        {
		    string strFieldValue;

            index = pTradeDb->FieldIndex("InstrumentID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(CommissionRate.InstrumentID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("InvestorRange", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			CommissionRate.InvestorRange = atoi(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("BrokerID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(CommissionRate.BrokerID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("InvestorID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(CommissionRate.InvestorID, strFieldValue.c_str());
			
            index = pTradeDb->FieldIndex("OpenRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			CommissionRate.OpenRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("OpenRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			CommissionRate.OpenRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			CommissionRate.CloseRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			CommissionRate.CloseRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseTodayRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			CommissionRate.CloseTodayRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseTodayRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			CommissionRate.CloseTodayRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("ExchangeID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(CommissionRate.ExchangeID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("BizType", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			CommissionRate.BizType = atoi(strFieldValue.c_str());
		} //nRow
	}

	sqlite3_free_table(dbResult);
}

//++
//获取期权品种手续费率
//参数
//    CntCode: 交易柜台代码
//    OptionId: 期权品种代码
//    InstrumentID: 用以查询的某个具体期权合约代码
//    OptionInstrCommRate: 引用参数，手续费费率结构变量
//    YearMonth: 标的期货合约的月份
//返回值
//    无
//--
void CMarketDataManage::GetOptionInstrCommRate(char CntCode, string OptionId, TThostFtdcInstrumentIDType InstrumentID, CThostFtdcOptionInstrCommRateField& OptionInstrCommRate, int YearMonth)
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
	CRealTraderSpi*  pTraderUserSpi = NULL;
	CCtpCntIf* pCtpCntIf = NULL;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		if(m_pRealApp->m_pCtpCntIfList[i]->m_CntCode == CntCode)
		{
			pTraderUserSpi = m_pRealApp->m_pCtpCntIfList[i]->m_pTraderUserSpi;
			pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		}
	}

	if(pCtpCntIf == NULL)
	{
		cerr << m_pRealApp->m_Prompt << " " << InstrumentID << "： 找不到交易柜台!" << ENDLINE;
		return;
	}

	string strTableName = "OptionInstrCommRate";;
	string strSQLCmd;
	stringstream sstr;
	string SqlCondition;

    int RetVal;
	char *errMsg = NULL;
	char **dbResult;
	int nRow, nColumn;
	int i, j;	

	//先执行一次查询操作
	sstr.clear(); sstr.str("");
	sstr << " WHERE (OptionId = '" << OptionId << "') AND (BrokerID = '" << pCtpCntIf->m_brokerId << "') AND (InvestorID = '"
		 << pCtpCntIf->m_userId << "') AND (YearMonth = " << YearMonth << ")";
	SqlCondition = sstr.str();
	strSQLCmd = "SELECT * FROM " + strTableName + SqlCondition; 
	RetVal = sqlite3_get_table(pTradeDb->m_pDB, strSQLCmd.c_str(), &dbResult, &nRow, &nColumn, &errMsg);

	//如果没有在数据库中查到手续费率信息，或者程序要求重新从API查询
	if(RetVal != SQLITE_OK || nRow <= 0 || m_pRealApp->m_bReQryRateParams == true)
	{
		if(errMsg != NULL)
		{
		    sqlite3_free(errMsg);				
		}

		//查询期权合约手续费率
		pTraderUserSpi->ReqQryOptionInstrCommRate(InstrumentID);
		DWORD waitReturn = WaitForSingleObject(pCtpCntIf->m_hTraderSpiEvent, REQ_WAIT_TIME); 
		if(waitReturn != WAIT_OBJECT_0) 
		{
			cerr << m_pRealApp->m_Prompt << "超过 " << int(REQ_WAIT_TIME/1000) << " 秒没有收到 " << InstrumentID << " 期权合约手续费率查询回应消息，程序返回！" << ENDLINE;
            PressAnyKeyToExit();
			exit(-1);
		}
		memcpy(&OptionInstrCommRate, &pTraderUserSpi->m_OptionInstrCommRate, sizeof(CThostFtdcOptionInstrCommRateField));
		ResetEvent(pCtpCntIf->m_hTraderSpiEvent);
		SleepMs(REQ_INTERVAL);

		//强行删除数据库中该合约保证金率的旧有记录
		strSQLCmd = "DELETE FROM " + strTableName + SqlCondition;
		pTradeDb->ExecuteSqlCommand(strSQLCmd);

		//插入手续费率的新记录
		sstr.clear(); sstr.str(""); sstr.precision(18);
		sstr << "'" << OptionId << "', '" << InstrumentID << "', " << YearMonth << ", " << int(OptionInstrCommRate.InvestorRange) << ", '" << pCtpCntIf->m_brokerId << "', '"
			<< pCtpCntIf->m_userId << "', " << OptionInstrCommRate.OpenRatioByMoney << ", " << OptionInstrCommRate.OpenRatioByVolume << ", "
			<< OptionInstrCommRate.CloseRatioByMoney << ", " << OptionInstrCommRate.CloseRatioByVolume << ", " << OptionInstrCommRate.CloseTodayRatioByMoney << ", "
			<< OptionInstrCommRate.CloseTodayRatioByVolume << ", " << OptionInstrCommRate.StrikeRatioByMoney << ", "
			<< OptionInstrCommRate.StrikeRatioByVolume << ", '" << OptionInstrCommRate.ExchangeID << "'";

	    strSQLCmd = "INSERT INTO " + strTableName + "(OptionId, InstrumentID, YearMonth, InvestorRange, BrokerID, InvestorID, OpenRatioByMoney, OpenRatioByVolume, \
					 CloseRatioByMoney, CloseRatioByVolume, CloseTodayRatioByMoney, CloseTodayRatioByVolume, StrikeRatioByMoney, StrikeRatioByVolume, \
					 ExchangeID) VALUES(" + sstr.str() + ")";
		pTradeDb->ExecuteSqlCommand(strSQLCmd);
	}
	else
	{   //数据表存在
		//设置FieldName List
        vector<string> FieldNameList;
		int index = 0;
        for(j = 0; j < nColumn; j++)
        {
           FieldNameList.push_back(dbResult[j]);
        }

		//按行读出数据表中的数据
        for(i = 0; i < nRow; i++)
        {
		    string strFieldValue;

            index = pTradeDb->FieldIndex("InstrumentID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(OptionInstrCommRate.InstrumentID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("InvestorRange", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			OptionInstrCommRate.InvestorRange = atoi(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("BrokerID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(OptionInstrCommRate.BrokerID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("InvestorID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(OptionInstrCommRate.InvestorID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("OpenRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			OptionInstrCommRate.OpenRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("OpenRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			OptionInstrCommRate.OpenRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			OptionInstrCommRate.CloseRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			OptionInstrCommRate.CloseRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseTodayRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			OptionInstrCommRate.CloseTodayRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseTodayRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			OptionInstrCommRate.CloseTodayRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("StrikeRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			OptionInstrCommRate.StrikeRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("StrikeRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			OptionInstrCommRate.StrikeRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("ExchangeID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(OptionInstrCommRate.ExchangeID, strFieldValue.c_str());
		} //nRow
	}

	sqlite3_free_table(dbResult);
}

//++
//获取ETF期权手续费
//参数
//    CntCode: 交易柜台代码
//    OptionId: 期权品种代码
//    InstrumentID: 用以查询的某个具体期权合约代码
//    pETFOptionInstrCommRate: 指针，指向ETF期权手续费数组
//    YearMonth: 期权合约的月份
//返回值
//    无
//--
#ifndef FUTURE_OPTION_ONLY
void CMarketDataManage::GetETFOptionInstrCommRate(char CntCode, string OptionId, TThostFtdcInstrumentIDType InstrumentID, CThostFtdcETFOptionInstrCommRateField* pETFOptionInstrCommRate, int YearMonth)
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
	CRealTraderSpi*  pTraderUserSpi = NULL;
	CCtpCntIf* pCtpCntIf = NULL;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		if(m_pRealApp->m_pCtpCntIfList[i]->m_CntCode == CntCode)
		{
			pTraderUserSpi = m_pRealApp->m_pCtpCntIfList[i]->m_pTraderUserSpi;
			pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		}
	}

	if(pCtpCntIf == NULL)
	{
		cerr << m_pRealApp->m_Prompt << " " << InstrumentID << "： 找不到交易柜台!" << ENDLINE;
		return;
	}

	vector<CThostFtdcETFOptionInstrCommRateField> ETFOptionInstrCommRateList;
	string strTableName = "ETFOptionInstrCommRate";;
	string strSQLCmd;
	stringstream sstr;
	string SqlCondition;

    int RetVal;
	char *errMsg = NULL;
	char **dbResult;
	int nRow, nColumn;

	//先执行一次查询操作
	sstr.clear(); sstr.str("");
	sstr << " WHERE (OptionId = '" << OptionId << "') AND (BrokerID = '" << pCtpCntIf->m_brokerId << "') AND (InvestorID = '"
		 << pCtpCntIf->m_userId << "') AND (YearMonth = " << YearMonth << ")";
	SqlCondition = sstr.str();
	strSQLCmd = "SELECT * FROM " + strTableName + SqlCondition; 
	RetVal = sqlite3_get_table(pTradeDb->m_pDB, strSQLCmd.c_str(), &dbResult, &nRow, &nColumn, &errMsg);

	//如果没有在数据库中查到手续费率信息，或者程序要求重新从API查询
	if(RetVal != SQLITE_OK || nRow < 4 || m_pRealApp->m_bReQryRateParams == true)
	{
		if(errMsg != NULL)
		{
		    sqlite3_free(errMsg);				
		}

		//查询期权合约手续费率
		pTraderUserSpi->ReqQryETFOptionInstrCommRate(InstrumentID);
		DWORD waitReturn = WaitForSingleObject(pCtpCntIf->m_hTraderSpiEvent, REQ_WAIT_TIME); 
		if(waitReturn != WAIT_OBJECT_0) 
		{
			cerr << m_pRealApp->m_Prompt << "超过 " << int(REQ_WAIT_TIME/1000) << " 秒没有收到 " << InstrumentID << " ETF期权合约手续费率查询回应消息，程序返回！" << ENDLINE;
            PressAnyKeyToExit();
			exit(-1);
		}
		ETFOptionInstrCommRateList.swap(pTraderUserSpi->m_ETFOptionInstrCommRateList);
		ResetEvent(pCtpCntIf->m_hTraderSpiEvent);
		SleepMs(REQ_INTERVAL);

		//强行删除数据库中该合约保证金率的旧有记录
		strSQLCmd = "DELETE FROM " + strTableName + SqlCondition;
		pTradeDb->ExecuteSqlCommand(strSQLCmd);

		//插入手续费率的新记录
		for(size_t i = 0; i < ETFOptionInstrCommRateList.size(); i++)
		{
			sstr.clear(); sstr.str(""); sstr.precision(18);
			sstr << "'" << OptionId << "', '" << InstrumentID << "', "  << YearMonth << ", " << int(ETFOptionInstrCommRateList[i].InvestorRange) << ", '" << pCtpCntIf->m_brokerId << "', '"
				<< pCtpCntIf->m_userId << "', " << ETFOptionInstrCommRateList[i].OpenRatioByMoney << ", " << ETFOptionInstrCommRateList[i].OpenRatioByVolume << ", "
				<< ETFOptionInstrCommRateList[i].CloseRatioByMoney << ", " << ETFOptionInstrCommRateList[i].CloseRatioByVolume << ", " << ETFOptionInstrCommRateList[i].CloseTodayRatioByMoney << ", "
				<< ETFOptionInstrCommRateList[i].CloseTodayRatioByVolume << ", " << ETFOptionInstrCommRateList[i].StrikeRatioByMoney << ", "
				<< ETFOptionInstrCommRateList[i].StrikeRatioByVolume << ", '" << ETFOptionInstrCommRateList[i].ExchangeID << "', " << int(ETFOptionInstrCommRateList[i].HedgeFlag) << ", "
				<< int(ETFOptionInstrCommRateList[i].PosiDirection);

			strSQLCmd = "INSERT INTO " + strTableName + "(OptionId, InstrumentID, YearMonth, InvestorRange, BrokerID, InvestorID, OpenRatioByMoney, OpenRatioByVolume, \
						 CloseRatioByMoney, CloseRatioByVolume, CloseTodayRatioByMoney, CloseTodayRatioByVolume, StrikeRatioByMoney, StrikeRatioByVolume, \
						 ExchangeID, HedgeFlag, PosiDirection) VALUES(" + sstr.str() + ")";
			pTradeDb->ExecuteSqlCommand(strSQLCmd);
		}
	}
	else
	{   //数据表存在
		//设置FieldName List
        vector<string> FieldNameList;
		int index = 0;
        for(int j = 0; j < nColumn; j++)
        {
           FieldNameList.push_back(dbResult[j]);
        }

		//按行读出数据表中的数据
        for(int i = 0; i < nRow; i++)
        {
		    string strFieldValue;
		    CThostFtdcETFOptionInstrCommRateField ETFOptionInstrCommRate;

            index = pTradeDb->FieldIndex("InstrumentID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(ETFOptionInstrCommRate.InstrumentID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("InvestorRange", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			ETFOptionInstrCommRate.InvestorRange = atoi(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("BrokerID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(ETFOptionInstrCommRate.BrokerID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("InvestorID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(ETFOptionInstrCommRate.InvestorID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("OpenRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			ETFOptionInstrCommRate.OpenRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("OpenRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			ETFOptionInstrCommRate.OpenRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			ETFOptionInstrCommRate.CloseRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			ETFOptionInstrCommRate.CloseRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseTodayRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			ETFOptionInstrCommRate.CloseTodayRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CloseTodayRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			ETFOptionInstrCommRate.CloseTodayRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("StrikeRatioByMoney", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			ETFOptionInstrCommRate.StrikeRatioByMoney = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("StrikeRatioByVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			ETFOptionInstrCommRate.StrikeRatioByVolume = atof(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("ExchangeID", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			strFieldValue = RemoveSpace(strFieldValue);
			strcpy(ETFOptionInstrCommRate.ExchangeID, strFieldValue.c_str());

            index = pTradeDb->FieldIndex("HedgeFlag", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			ETFOptionInstrCommRate.HedgeFlag = atoi(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("PosiDirection", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			ETFOptionInstrCommRate.PosiDirection = atoi(strFieldValue.c_str());

			ETFOptionInstrCommRateList.push_back(ETFOptionInstrCommRate);
		} //nRow
	} //else

	sqlite3_free_table(dbResult);

	//使用ETFOptionInstrCommRateList填充pETFOptionInstrCommRate数组
	for(size_t i = 0; i < ETFOptionInstrCommRateList.size(); i++)
	{
		if(ETFOptionInstrCommRateList[i].HedgeFlag == THOST_FTDC_HF_Speculation && ETFOptionInstrCommRateList[i].PosiDirection == THOST_FTDC_PD_Long)
			memcpy(pETFOptionInstrCommRate, &ETFOptionInstrCommRateList[i], sizeof(CThostFtdcETFOptionInstrCommRateField));
		else if(ETFOptionInstrCommRateList[i].HedgeFlag == THOST_FTDC_HF_Speculation && ETFOptionInstrCommRateList[i].PosiDirection == THOST_FTDC_PD_Short)
			memcpy((pETFOptionInstrCommRate+1), &ETFOptionInstrCommRateList[i], sizeof(CThostFtdcETFOptionInstrCommRateField));
		else if(ETFOptionInstrCommRateList[i].HedgeFlag == THOST_FTDC_HF_Covered && ETFOptionInstrCommRateList[i].PosiDirection == THOST_FTDC_PD_Long)
			memcpy((pETFOptionInstrCommRate+2), &ETFOptionInstrCommRateList[i], sizeof(CThostFtdcETFOptionInstrCommRateField));
		else if(ETFOptionInstrCommRateList[i].HedgeFlag == THOST_FTDC_HF_Covered && ETFOptionInstrCommRateList[i].PosiDirection == THOST_FTDC_PD_Short)
			memcpy((pETFOptionInstrCommRate+3), &ETFOptionInstrCommRateList[i], sizeof(CThostFtdcETFOptionInstrCommRateField));
	}
}
#endif




