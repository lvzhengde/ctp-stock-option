#include "ExchangeRuleFilter.h"
#include "RealApp.h"
#include "TradeDb.h"

//++
//构造函数
//--
CExchangeRuleFilter::CExchangeRuleFilter(CRealApp *pRealApp)
{
	m_pRealApp = pRealApp;

	InitializeSRWLock(&m_InsertOrderRwLock);
	InitializeSRWLock(&m_CancelOrderRwLock);
	InitializeSRWLock(&m_TradeVolumeRwLock);
	InitializeSRWLock(&m_OpenInterestsRwLock);

	m_InsertOrderNum = 0;
	m_CancelOrderNum = 0;
	m_TradeVolume = 0;
	m_OpenInterests = 0;

	//按照上交所的规则设置, 留5%左右的余量
	m_MaxCancelOrderNum = 19950;
	m_MaxTradeVolume = 950;
	m_MaxOpenInterests = 950;
}

//++
//析构函数
//--
CExchangeRuleFilter::~CExchangeRuleFilter()
{
}

//++
//增加申报单数目
//参数
//    CntCode: 交易柜台代码
//    ExchangeID: 交易所代码
//返回值
//    无
//--
void CExchangeRuleFilter::IncreaseInsertOrderNum(char CntCode, string ExchangeID)
{
	if(CntCode == m_CntCode && strcmp(ExchangeID.c_str(), m_ExchangeID.c_str()) == 0)
	{
		AcquireSRWLockExclusive(&m_InsertOrderRwLock);
		m_InsertOrderNum++;
		ReleaseSRWLockExclusive(&m_InsertOrderRwLock);
	}
}

//++
//设置申报单数目
//参数
//    SetValue: 设置值
//返回值
//    无
//--
void CExchangeRuleFilter::SetInsertOrderNum(int SetValue)
{
	AcquireSRWLockExclusive(&m_InsertOrderRwLock);
	m_InsertOrderNum = SetValue;
	ReleaseSRWLockExclusive(&m_InsertOrderRwLock);
}

//++
//获取申报单数目
//参数
//    无
//返回值
//    当前申报单数目
//--
int CExchangeRuleFilter::GetInsertOrderNum()
{ 
	int ret = 0;

    AcquireSRWLockShared(&m_InsertOrderRwLock);
	ret = m_InsertOrderNum;
	ReleaseSRWLockShared(&m_InsertOrderRwLock);

	return ret;
}

//++
//增加撤单数目
//参数
//    CntCode: 交易柜台代码
//    ExchangeID: 交易所代码
//返回值
//    无
//--
void CExchangeRuleFilter::IncreaseCancelOrderNum(char CntCode, string ExchangeID)
{
	if(CntCode == m_CntCode && strcmp(ExchangeID.c_str(), m_ExchangeID.c_str()) == 0)
	{
		AcquireSRWLockExclusive(&m_CancelOrderRwLock);
		m_CancelOrderNum++;
		ReleaseSRWLockExclusive(&m_CancelOrderRwLock);
	}
}

//++
//设置撤单数目
//参数
//    SetValue: 设置值
//返回值
//    无
//--
void CExchangeRuleFilter::SetCancelOrderNum(int SetValue)
{
	AcquireSRWLockExclusive(&m_CancelOrderRwLock);
	m_CancelOrderNum = SetValue;
	ReleaseSRWLockExclusive(&m_CancelOrderRwLock);
}

//++
//获取撤单数目
//参数
//    无
//返回值
//    当前撤单数目
//--
int CExchangeRuleFilter::GetCancelOrderNum()
{
	int ret = 0;

    AcquireSRWLockShared(&m_CancelOrderRwLock);
	ret = m_CancelOrderNum;
	ReleaseSRWLockShared(&m_CancelOrderRwLock);

	return ret;
}

//++
//增加成交量数目
//参数
//    CntCode: 交易柜台代码
//    pTrade: 指向成交回报数据结构的指针
//返回值
//    无
//--
void CExchangeRuleFilter::IncreaseTradeVolume(char CntCode, CThostFtdcTradeField *pTrade)
{
	if(CntCode == m_CntCode && strcmp(pTrade->ExchangeID, m_ExchangeID.c_str()) == 0)
	{
		AcquireSRWLockExclusive(&m_TradeVolumeRwLock);
		m_TradeVolume += pTrade->Volume;
		ReleaseSRWLockExclusive(&m_TradeVolumeRwLock);
	}
}

//++
//设置成交量数目
//参数
//    SetValue: 设置值
//返回值
//    无
//--
void CExchangeRuleFilter::SetTradeVolume(int SetValue)
{
	AcquireSRWLockExclusive(&m_TradeVolumeRwLock);
	m_TradeVolume = SetValue;
	ReleaseSRWLockExclusive(&m_TradeVolumeRwLock);
}

//++
//获取成交量数目
//参数
//    无
//返回值
//    当前成交量数目
//--
int CExchangeRuleFilter::GetTradeVolume()
{
	int ret = 0;

    AcquireSRWLockShared(&m_TradeVolumeRwLock);
	ret = m_TradeVolume;
	ReleaseSRWLockShared(&m_TradeVolumeRwLock);

	return ret;
}

//++
//计算持仓量数目
//持仓量随时变化, 所以要根据最新查询结果设置
//参数
//    CntCode: 交易柜台代码
//    pInvestorPositionDetailList: 指向用户持仓明细列表的指针
//返回值
//    无
//--
void CExchangeRuleFilter::CalcOpenInterests(char CntCode, vector<CInvestorPositionDetail>* pInvestorPositionDetailList)
{
	if(CntCode == m_CntCode)
	{
		AcquireSRWLockExclusive(&m_OpenInterestsRwLock);

		m_OpenInterests = 0;  //由于持仓量可能时刻变化, 先清零
		for(size_t i = 0; i < pInvestorPositionDetailList->size(); i++)
		{
			CThostFtdcInvestorPositionField* pInvestorPosition = &(*pInvestorPositionDetailList)[i].CtpInvestorPosition;
			string strExchangeID = pInvestorPosition->ExchangeID;
			if(strExchangeID == "") 
			{
				strExchangeID = GetExchangeID(pInvestorPosition->InstrumentID);
			}

			if(strcmp(strExchangeID.c_str(), m_ExchangeID.c_str()) == 0)
			{
				m_OpenInterests += pInvestorPosition->Position;
			}
		}

		ReleaseSRWLockExclusive(&m_OpenInterestsRwLock);
	}
}

//++
//已知合约代码, 获取交易所代码
//参数
//    InstrumentID: 合约代码
//返回值
//    交易所代码
//--
string CExchangeRuleFilter::GetExchangeID(TThostFtdcInstrumentIDType InstrumentID)
{
	char InstrumentType = 0;
	size_t UnderlyingAssetType;
	string strExchangeID = "";

	void* pNode = GetMarketNodePointer(m_pRealApp, InstrumentID, InstrumentType, UnderlyingAssetType);
	if(InstrumentType == 'u')
	{
		CTshapeMonth* pTshapeMonth = (CTshapeMonth*)pNode;
		strExchangeID = pTshapeMonth->m_UnderlyingInstInfo.ExchangeID;
	}
	else if(InstrumentType == 'c' || InstrumentType == 'p')
	{
		COptionItem* pOptionItem = (COptionItem*)pNode;
		strExchangeID = pOptionItem->m_InstInfo.ExchangeID;
	}

	return strExchangeID;
}

//++
//设置持仓量数目
//参数
//    SetValue: 设置值
//返回值
//    无
//--
void CExchangeRuleFilter::SetOpenInterests(int SetValue)
{
	AcquireSRWLockExclusive(&m_OpenInterestsRwLock);
	m_OpenInterests = SetValue; 
	ReleaseSRWLockExclusive(&m_OpenInterestsRwLock);
}


//++
//获取持仓量数目
//参数
//    无
//返回值
//    当前持仓量数目
//--
int CExchangeRuleFilter::GetOpenInterests()
{
	int ret = 0;

    AcquireSRWLockShared(&m_OpenInterestsRwLock);
	ret = m_OpenInterests;
	ReleaseSRWLockShared(&m_OpenInterestsRwLock);

	return ret;

}

//++
//检查开仓交易是否通过交易所交易规则限制
//参数
//    无
//返回值
//    true: 通过交易所交易规则检查; false: 没有通过交易所交易规则检查
//--
bool CExchangeRuleFilter::IsExchangeRuleFilterPassForOpen()
{
	bool bCheckPass = true;
	int CancelOrderNum = GetCancelOrderNum();
	int TradeVolume = GetTradeVolume();
	int OpenInterests = GetOpenInterests();

	if(CancelOrderNum > m_MaxCancelOrderNum || TradeVolume > m_MaxTradeVolume || OpenInterests > m_MaxOpenInterests)
	{
		bCheckPass = false;
	}

	return bCheckPass;
}

//++
//检查平仓交易是否通过交易所规则限制
//参数
//    无
//返回值
//    true: 通过交易所交易规则检查; false: 没有通过交易所交易规则检查
//--
bool CExchangeRuleFilter::IsExchangeRuleFilterPassForClose()
{
	bool bCheckPass = true;
	int CancelOrderNum = GetCancelOrderNum();
	int TradeVolume = GetTradeVolume();

	if(CancelOrderNum > m_MaxCancelOrderNum || TradeVolume > m_MaxTradeVolume)
	{
		bCheckPass = false;
	}

	return bCheckPass;
}

//++
//从数据库中获取当日的各种交易统计信息
//参数
//    无
//返回值
//    无
//--
void CExchangeRuleFilter::GetTodayStatusFromDb()
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
    string strTableName;
	string strFieldDesc;
	string strSQLCmd;

	//检查数据表是否存在，不存在则创建
	int ret;
	strTableName = "ExchangeRuleFilterStat";
	strFieldDesc =  "(ID INTEGER PRIMARY KEY AUTOINCREMENT, DateOfStat DATE, CntCode VARCHAR, ExchangeID VARCHAR, InsertOrderNum INT, CancelOrderNum INT, \
					TradeVolume INT, OpenInterests INT, UNIQUE(DateOfStat, CntCode, ExchangeID))";
	ret = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
	if(ret < 0) 
	{
		cerr << m_pRealApp->m_Prompt << "数据表 " << strTableName << " 无法创建或者无法访问，程序返回！" << ENDLINE;
        PressAnyKeyToExit();
		pTradeDb->Destroy();
		exit(-1);
	}	

    int RetVal;
	char *errMsg = NULL;
	char **dbResult;
	int nRow, nColumn;
	int i, j;	

	//获取状态统计数据
	strSQLCmd = "SELECT * FROM " + strTableName + " WHERE (date(DateOfStat) = date('now')) AND \
		        (CntCode = '" + m_CntCode + "') AND (ExchangeID = '" + m_ExchangeID + "')";
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
	
		string strFieldValue;
		int InsertOrderNum = 0;
		int CancelOrderNum = 0;
		int TradeVolume = 0;
		int OpenInterests = 0;
		//按行读出数据表中的数据
        for(i = 0; i < nRow; i++)
        {
            index = pTradeDb->FieldIndex("InsertOrderNum", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			InsertOrderNum = atoi(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("CancelOrderNum", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			CancelOrderNum = atoi(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("TradeVolume", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			TradeVolume = atoi(strFieldValue.c_str());

            index = pTradeDb->FieldIndex("OpenInterests", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			OpenInterests = atoi(strFieldValue.c_str());
        }
		//使用数据库中读出的状态数据设置各个统计数据的初始值
		SetInsertOrderNum(InsertOrderNum);
		SetCancelOrderNum(CancelOrderNum);
		SetTradeVolume(TradeVolume);
		SetOpenInterests(OpenInterests);
	}

	sqlite3_free_table(dbResult);
}

//++
//将交易统计数据写入数据库
//参数
//    无
//返回值
//    无
//--
void CExchangeRuleFilter::DumpTodayStatusToDb()
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
    string strTableName = "ExchangeRuleFilterStat";
	string strSQLCmd;
	SYSTEMTIME CurTime;
	stringstream sstr;
	string strDate;

	GetLocalTime(&CurTime);
	SYSTEMTIME KlineTime;
	KlineTime = CalKLineDate(CurTime);
	//由于期货交易所存在夜盘, 所以需要使用K线时间
	sstr.clear(); sstr.str("");
	//setw()函数仅对<<后的输出数据有效, setfill()函数则是跟在setw后面填充相应的空位（默认是空格）
	//#include <iomanip>包含setw()和setfill()
	sstr << setw(4) << setfill('0') << KlineTime.wYear << "-" << setw(2) << setfill('0') << KlineTime.wMonth << "-" 
	     << setw(2) << setfill('0') << KlineTime.wDay;
	strDate = sstr.str();

	int InsertOrderNum = GetInsertOrderNum();
	int CancelOrderNum = GetCancelOrderNum();
	int TradeVolume = GetTradeVolume();
	int OpenInterests = GetOpenInterests();

	sstr.clear(); sstr.str(""); sstr.precision(18);
	sstr << "'" <<  strDate << "', '" << m_CntCode << "', '" << m_ExchangeID << "', " << InsertOrderNum <<  ", "
			<< CancelOrderNum << ", " << TradeVolume << ", " << OpenInterests;

	//先从数据表中删除当日统计数据
	strSQLCmd = "DELETE FROM " + strTableName + " WHERE (DateOfStat='" + strDate + "') AND \
		        (CntCode = '" + m_CntCode + "') AND (ExchangeID = '" + m_ExchangeID + "')";
	pTradeDb->ExecuteSqlCommand(strSQLCmd);

	//再插入当前日K线记录
	strSQLCmd = "INSERT INTO " + strTableName + "(DateOfStat, CntCode, ExchangeID, InsertOrderNum, CancelOrderNum, TradeVolume, OpenInterests) \
				 VALUES(" + sstr.str() + ")";
	pTradeDb->ExecuteSqlCommand(strSQLCmd);
}
