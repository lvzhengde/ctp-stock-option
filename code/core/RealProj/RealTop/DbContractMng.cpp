//++
//目前只支持期货交易, 只有一个柜台其下标为[0]
//--
#include "DbContractMng.h"
#include "KeyValue.h"

//++
//构造函数
//--
CDbContractMng::CDbContractMng(CRealApp* pRealApp)
{
	m_pRealApp = pRealApp;
}

//++
//析构函数
//--
CDbContractMng::~CDbContractMng()
{
}

//++
//交易数据库管理维护主程序
//--
void CDbContractMng::DbContractMain()
{
	//获取账号密码，初始化CTP API
	CKeyValue GlobalKeyValue;
	string key, value;

	m_pRealApp->CreateObjects();
	m_pRealApp->m_pTradeDb->Init();
	m_pRealApp->m_bMaintainDb = true;
	//需要初始化后才能使用此指针
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;

	cerr << "注意: 本程序只适用于期货交易, 并且只支持一个交易柜台！" << ENDLINE << ENDLINE;
	cerr << "从全局配置数据表获取账户信息" << ENDLINE;
	int RetRead = GlobalKeyValue.ReadConfigFromDb(pTradeDb, "GlobalConfig");

#ifdef WIN_CTP
	string strConfigDir = ".\\config";
	if(m_pRealApp->m_ArchName != "") strConfigDir = ".\\config\\" + m_pRealApp->m_ArchName;
	string strFilePath = strConfigDir + "\\GlobalConfig.txt";
	if(RetRead <= 0) GlobalKeyValue.ReadConfigFromTextFile(strFilePath);
#else
	string strConfigDir = "./config";
	if(m_pRealApp->m_ArchName != "") strConfigDir = "./config/" + m_pRealApp->m_ArchName;
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

	//只可能有一个柜台
	if(CounterSize == 1)
	{
		CounterName = CounterNameList[0];
	}
	else if(CounterSize > 1)
	{
		size_t CounterIndex = 0;
		cerr << "输入柜台索引 CounterIndex >"; cin >> CounterIndex;

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

	m_pRealApp->m_pCtpCntIfList.clear();
	CCtpCntIf* pCtpCntIf = new CCtpCntIf(m_pRealApp, CounterName);
	pCtpCntIf->GetConfigFromDb();
	m_pRealApp->m_pCtpCntIfList.push_back(pCtpCntIf);

	//获取交易策略名称列表
	size_t StrategySize = GlobalKeyValue.InputIntKeyValue("StrategySize");
	for(size_t i = 0; i < StrategySize; i++)
	{
		string StrategyName;

		sprintf(tmpString, "%d", i);
		strCount = tmpString;
		strCount = RemoveSpace(strCount);
		key = "StrategyName" + strCount;
		StrategyName = GlobalKeyValue.InputStringKeyValue(key);
		m_StrategyNameList.push_back(StrategyName);
	}

	//将配置信息备份回数据库
	GlobalKeyValue.WriteConfigToDb(pTradeDb, "GlobalConfig");

	//功能选择
	char SelFunc;
	cerr << ENDLINE;
	cerr << "数据库维护功能选择" << ENDLINE;
	cerr << "1. 交易合约换月" << ENDLINE;
	cerr << "2. 删除不交易品种" << ENDLINE;
	cerr << "请输入> "; cin >> SelFunc;
	cerr << ENDLINE;

	if(SelFunc == '1') {   
		//初始化CTP API
		m_pRealApp->m_pCtpCntIfList[0]->InitCtpApi(THOST_TERT_RESUME);
		ChangeTradeMonth();
	}
	else if(SelFunc == '2') 
		DeleteOptionKind();
	else
		cerr << "错误的输入, 程序结束" << ENDLINE;

	//为避免报错，结束程序前要清除主动操作的各种对象
	m_pRealApp->m_bMaintainDb = false;
	m_pRealApp->DestroyObjects();
	m_pRealApp->m_pCtpCntIfList[0]->CleanupCtpApi();
}

//++
//交易合约换月管理
//--
void CDbContractMng::ChangeTradeMonth()
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;

	//逐策略设置交易合约
	for(size_t i = 0; i < m_StrategyNameList.size(); i++)
	{
		CKeyValue StrategyKeyValue;
		string StrategyTableName;
		string StrategyFileName;

		//获取策略配置，填充OptionKindList列表
		cerr << "--->>>" << ENDLINE;
		cerr << "获取策略 " << m_StrategyNameList[i] << " 配置" << ENDLINE;
		StrategyTableName = m_StrategyNameList[i] + "_Config";
#ifdef WIN_CTP
		if(m_pRealApp->m_ArchName == "")
			StrategyFileName = ".\\config\\" + StrategyTableName + ".txt";
		else
			StrategyFileName = ".\\config\\" + m_pRealApp->m_ArchName + "\\" + StrategyTableName + ".txt";
#else
		if(m_pRealApp->m_ArchName == "")
			StrategyFileName = "./config/" + StrategyTableName + ".txt";
		else
			StrategyFileName = "./config/" + m_pRealApp->m_ArchName + "/" + StrategyTableName + ".txt";
#endif

		int RetRead = StrategyKeyValue.ReadConfigFromDb(pTradeDb, StrategyTableName);
		if(RetRead <= 0) StrategyKeyValue.ReadConfigFromTextFile(StrategyFileName);

		string key, value;
		string strCount;
		char tmpString[10];
		int OptionKindListSize;

		//获取期权品种的相关信息
		OptionKindListSize = StrategyKeyValue.InputIntKeyValue("OptionKindListSize");
		cerr << "---<<<" << ENDLINE;
		for(int j = 0; j < OptionKindListSize; j++)
		{
			COptionKind OptionKind;

			sprintf(tmpString, "%d", j);
			strCount = tmpString;
			strCount = RemoveSpace(strCount);

			//获取OptionId
			key = "OptionId" + strCount;
			OptionKind.OptionId = StrategyKeyValue.InputStringKeyValue(key);

			//获取UnderlyingId
			key = "UnderlyingId" + strCount;
			OptionKind.UnderlyingId = StrategyKeyValue.InputStringKeyValue(key);

			//获取ExchangeId
			key = "ExchangeId" + strCount;
			value = StrategyKeyValue.InputStringKeyValue(key);
			strcpy(OptionKind.ExchangeId, value.c_str());

			//针对每种期权品种，获取YearMonthList和SegTimeList及其它参数
			string strOptionKindTableName;
			string strOptionKindFileName;
			CKeyValue OptionKindKeyValue(true);
			int YearMonthListSize;
			int SegTimeListSize;

			cerr << "获取 " << OptionKind.ExchangeId << " 交易品种 " << OptionKind.OptionId << " 配置" << ENDLINE;
			cerr << "--->>>" << ENDLINE;
			strOptionKindTableName = m_StrategyNameList[i] + "_" + OptionKind.OptionId + "_Config";
#ifdef WIN_CTP
			if(m_pRealApp->m_ArchName == "")
				strOptionKindFileName = ".\\config\\" + strOptionKindTableName + ".txt";
			else
				strOptionKindFileName = ".\\config\\" + m_pRealApp->m_ArchName + "\\" + strOptionKindTableName + ".txt";
#else
			if(m_pRealApp->m_ArchName == "")
				strOptionKindFileName = "./config/" + strOptionKindTableName + ".txt";
			else
				strOptionKindFileName = "./config/" + m_pRealApp->m_ArchName + "/" + strOptionKindTableName + ".txt";
#endif

			RetRead = OptionKindKeyValue.ReadConfigFromDb(pTradeDb, strOptionKindTableName);
			if(RetRead <= 0) OptionKindKeyValue.ReadConfigFromTextFile(strOptionKindFileName);

			//获取期权交易柜台代码
			key = "OptCntCode";
			value = OptionKindKeyValue.InputStringKeyValue(key);
			OptionKind.OptCntCode = value[0];

			//获取标的交易柜台代码
			key = "UnderCntCode";
			value = OptionKindKeyValue.InputStringKeyValue(key);
			OptionKind.UnderCntCode = value[0];

			//获取bUnderMdOnly
			int UnderMdOnly;
			key = "UnderMdOnly";
			UnderMdOnly = OptionKindKeyValue.InputIntKeyValue(key);
			OptionKind.bUnderMdOnly = (UnderMdOnly == 0) ? false : true;

			//获取StopBeforeExpire
			key = "StopBeforeExpire";
			OptionKind.StopBeforeExpire = OptionKindKeyValue.InputIntKeyValue(key);

			//获取UnderlyingAssetType
			key = "UnderlyingAssetType";
			OptionKind.UnderlyingAssetType = OptionKindKeyValue.InputIntKeyValue(key);

			//获取YearMonthList
			OptionKind.YearMonthList.clear();
			YearMonthListSize = OptionKindKeyValue.InputIntKeyValue("YearMonthListSize");
			for(int k = 0; k < YearMonthListSize; k++)
			{
				unsigned int YearMonth;

				sprintf(tmpString, "%d", k);
				strCount = tmpString;
				strCount = RemoveSpace(strCount);

				key = "YearMonth" + strCount;
				YearMonth = OptionKindKeyValue.InputIntKeyValue(key);
				OptionKind.YearMonthList.push_back(YearMonth);
			}

			//获取SegTimeList
			OptionKind.SegTimeList.clear();
			SegTimeListSize = OptionKindKeyValue.InputIntKeyValue("SegTimeListSize");
			for(int k = 0; k < SegTimeListSize; k++)
			{
				CSegTime SegTime;

				sprintf(tmpString, "%d", k);
				strCount = tmpString;
				strCount = RemoveSpace(strCount);

				key = "SegBeginTime" + strCount;
				SegTime.SegBeginTime = OptionKindKeyValue.InputFloatKeyValue(key);
				key = "SegEndTime" + strCount;
				SegTime.SegEndTime = OptionKindKeyValue.InputFloatKeyValue(key);

				OptionKind.SegTimeList.push_back(SegTime);
			}

			//针对该交易品种进行换月设置
			IndividualChangeMonth(m_StrategyNameList[i], &OptionKind);

			cerr << "---<<<" << ENDLINE;
		} //for j, 逐交易品种
		cerr << "---<<<" << ENDLINE;
	} //for i, 逐策略
}

//++
//删除不交易的品种
//--
void CDbContractMng::DeleteOptionKind()
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;

	//逐策略设置交易合约
	for(size_t i = 0; i < m_StrategyNameList.size(); i++)
	{
		CKeyValue StrategyKeyValue;
		string StrategyTableName;
		string StrategyFileName;
		vector<COptionKind> OptionKindList;

		//获取策略配置，填充OptionKindList列表
		cerr << "--->>>" << ENDLINE;
		cerr << "获取策略 " << m_StrategyNameList[i] << " 配置" << ENDLINE;
		StrategyTableName = m_StrategyNameList[i] + "_Config";
#ifdef WIN_CTP
		if(m_pRealApp->m_ArchName == "")
			StrategyFileName = ".\\config\\" + StrategyTableName + ".txt";
		else
			StrategyFileName = ".\\config\\" + m_pRealApp->m_ArchName + "\\" + StrategyTableName + ".txt";
#else
		if(m_pRealApp->m_ArchName == "")
			StrategyFileName = "./config/" + StrategyTableName + ".txt";
		else
			StrategyFileName = "./config/" + m_pRealApp->m_ArchName + "/" + StrategyTableName + ".txt";
#endif

		int RetRead = StrategyKeyValue.ReadConfigFromDb(pTradeDb, StrategyTableName);
		if(RetRead <= 0) StrategyKeyValue.ReadConfigFromTextFile(StrategyFileName);

		string key, value;
		string strCount;
		char tmpString[10];
		int OptionKindListSize;

		//获取期权品种的相关信息
		OptionKindListSize = StrategyKeyValue.InputIntKeyValue("OptionKindListSize");
		cerr << "---<<<" << ENDLINE;
		for(int j = 0; j < OptionKindListSize; j++)
		{
			COptionKind OptionKind;

			sprintf(tmpString, "%d", j);
			strCount = tmpString;
			strCount = RemoveSpace(strCount);

			//获取OptionId
			key = "OptionId" + strCount;
			OptionKind.OptionId = StrategyKeyValue.InputStringKeyValue(key);

			//获取UnderlyingId
			key = "UnderlyingId" + strCount;
			OptionKind.UnderlyingId = StrategyKeyValue.InputStringKeyValue(key);

			//获取ExchangeId
			key = "ExchangeId" + strCount;
			value = StrategyKeyValue.InputStringKeyValue(key);
			strcpy(OptionKind.ExchangeId, value.c_str());

			//针对每种期权品种，获取YearMonthList和SegTimeList及其它参数
			string strOptionKindTableName;
			string strOptionKindFileName;
			CKeyValue OptionKindKeyValue(true);
			int YearMonthListSize;
			int SegTimeListSize;

			cerr << "获取 " << OptionKind.ExchangeId << " 交易品种 " << OptionKind.OptionId << " 配置" << ENDLINE;
			cerr << "--->>>" << ENDLINE;
			strOptionKindTableName = m_StrategyNameList[i] + "_" + OptionKind.OptionId + "_Config";
#ifdef WIN_CTP
			if(m_pRealApp->m_ArchName == "")
				strOptionKindFileName = ".\\config\\" + strOptionKindTableName + ".txt";
			else
				strOptionKindFileName = ".\\config\\" + m_pRealApp->m_ArchName + "\\" + strOptionKindTableName + ".txt";
#else
			if(m_pRealApp->m_ArchName == "")
				strOptionKindFileName = "./config/" + strOptionKindTableName + ".txt";
			else
				strOptionKindFileName = "./config/" + m_pRealApp->m_ArchName + "/" + strOptionKindTableName + ".txt";
#endif

			RetRead = OptionKindKeyValue.ReadConfigFromDb(pTradeDb, strOptionKindTableName);
			if(RetRead <= 0) OptionKindKeyValue.ReadConfigFromTextFile(strOptionKindFileName);

			//获取YearMonthList
			OptionKind.YearMonthList.clear();
			YearMonthListSize = OptionKindKeyValue.InputIntKeyValue("YearMonthListSize");
			for(int k = 0; k < YearMonthListSize; k++)
			{
				unsigned int YearMonth;

				sprintf(tmpString, "%d", k);
				strCount = tmpString;
				strCount = RemoveSpace(strCount);

				key = "YearMonth" + strCount;
				YearMonth = OptionKindKeyValue.InputIntKeyValue(key);
				OptionKind.YearMonthList.push_back(YearMonth);
			}

			//获取SegTimeList
			OptionKind.SegTimeList.clear();
			SegTimeListSize = OptionKindKeyValue.InputIntKeyValue("SegTimeListSize");
			for(int k = 0; k < SegTimeListSize; k++)
			{
				CSegTime SegTime;

				sprintf(tmpString, "%d", k);
				strCount = tmpString;
				strCount = RemoveSpace(strCount);

				key = "SegBeginTime" + strCount;
				SegTime.SegBeginTime = OptionKindKeyValue.InputFloatKeyValue(key);
				key = "SegEndTime" + strCount;
				SegTime.SegEndTime = OptionKindKeyValue.InputFloatKeyValue(key);

				OptionKind.SegTimeList.push_back(SegTime);
			}

			char chDel;
			cerr << "是否删除该交易品种？（Y/N)：" ; cin >> chDel;
			if(chDel == 'Y' || chDel == 'y') 
			{
				DeleteIndividualOptionKind(m_StrategyNameList[i], &OptionKind);
			}

			OptionKindList.push_back(OptionKind);
			cerr << "---<<<" << ENDLINE;
		} //for j, 逐交易品种

		//修改策略配置文件
		ModifyStrategyConfig(m_StrategyNameList[i], &OptionKindList);
		cerr << "---<<<" << ENDLINE;
	} //for i, 逐策略
}

//++
//单个合约换月
//参数
//    StrategyName: 策略名称
//    pOptionKind: 指向期权品种信息的结构
//返回值
//    无
//--
void  CDbContractMng::IndividualChangeMonth(string StrategyName, COptionKind* pOptionKind)
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
	vector<size_t>* pOldYearMonthList = &pOptionKind->YearMonthList;
	vector<size_t> NewYearMonthList;
	vector<size_t> DbYearMonthList;
	CTshapeBase* pTshapeBase = NULL;
    string strTableName;
	string strSQLCmd;
	stringstream sstr;

	//输出当前所交易的合约年月份
	cerr << "当前交易合约年月份: " << ENDLINE;
	if(pOldYearMonthList->size() == 0)
	{
		cerr << "    所有可能的年月份" << ENDLINE;
	}
	else 
	{
		for(size_t i = 0; i < pOldYearMonthList->size(); i++)
			cerr << "OldYearMonth" << i << " = " << (*pOldYearMonthList)[i] << ENDLINE;
	}

	//获取统计数据表中存在的交易年月份, 统计数据表已经不使用
	//strTableName =  StrategyName + "_" + pOptionKind->OptionId + "_Stat";
	//GetDbStatYearMonth(strTableName, DbYearMonthList);
	//cerr << "统计数据表中具有数据的年月份: " << ENDLINE;
	//for(size_t i = 0; i < DbYearMonthList.size(); i++)
	//	cerr << "DbYearMonth" << i << " = " << DbYearMonthList[i] << ENDLINE;

	char chMonth;
	cerr << "是否进行换月设置？（Y/N)：" ; cin >> chMonth;
	if(chMonth != 'Y' && chMonth != 'y') return;

	//合并上述两个列表
	for(size_t i = 0; i < pOldYearMonthList->size(); i++)
	{
		bool found = false;
		for(size_t j = 0; j < DbYearMonthList.size(); j++)
		{
			if((*pOldYearMonthList)[i] == DbYearMonthList[j])
			{
				found = true;
				break;
			}
		}
		if(found == false) DbYearMonthList.push_back((*pOldYearMonthList)[i]);
	}

	//构建T型报价并获取市场行情, 在获取市场行情后，将T型报价按照标的成交量由大到小排序
    pTshapeBase = ConstructAndUpdateTshape(pOptionKind);
	cerr << "交易所获取的合约年月份和交易量，持仓量: " << ENDLINE;
	for(size_t i = 0; i < pTshapeBase->m_pTshapeMonthList.size(); i++)
	{
		CTshapeMonth* pTshapeMonth = pTshapeBase->m_pTshapeMonthList[i];
		CThostFtdcDepthMarketDataField* pMd = &pTshapeMonth->m_UnderlyingMarketData;
		cerr  << "ExYearMonth" << i << " = " << pTshapeMonth->m_YearMonth << ", \t"
			  << "Volume = " << pMd->Volume << ", \t"
			  << "OpenInterest = " << pMd->OpenInterest 
			  << ENDLINE;
	}

	//构造新的欲交易的合约的年月份列表
	size_t NewYearMonthSize = 0;
	string strNewYearMonthSize;
	cerr << "输入 NewYearMonthSize（输入0将交易所有年月份）> "; 
	bool bGetValidNum = true;
	do {
	    cin >> strNewYearMonthSize;
		bGetValidNum = IsNum(strNewYearMonthSize);
		if(bGetValidNum == false)
			cerr << "请输入合法数字 >";
	} while(bGetValidNum == false);
	NewYearMonthSize = atoi(strNewYearMonthSize.c_str());

	for(size_t i = 0; i < pTshapeBase->m_pTshapeMonthList.size(); i++)
	{
		int TradeLeftDays;
		if(pOptionKind->bUnderMdOnly == true)
		{
			char ExpireDate[20];
			string str1 = pTshapeBase->m_pTshapeMonthList[i]->m_UnderlyingInstInfo.ExpireDate;
			string str2 = str1.substr(0, 4) + "/" + str1.substr(4, 2) + "/" + str1.substr(6, 2);
			strcpy(ExpireDate, str2.c_str());
			TradeLeftDays = CalcExpirationDays(ExpireDate);
		}
		else
		{
			TradeLeftDays = pTshapeBase->m_pTshapeMonthList[i]->m_ExpirationLeftDays;
		}

		if(TradeLeftDays > pOptionKind->StopBeforeExpire) 
		{
			NewYearMonthList.push_back(pTshapeBase->m_pTshapeMonthList[i]->m_YearMonth);
		}

		if(NewYearMonthList.size() == NewYearMonthSize && NewYearMonthSize != 0) break;
	}

	//删除停止交易合约的K线数据表, 注: 统计数据表已不再使用
	bool bFoundInTrade;
    for(size_t i = 0; i < DbYearMonthList.size(); i++)
	{
		bFoundInTrade = false;
		for(size_t j = 0; j < pTshapeBase->m_pTshapeMonthList.size(); j++)
		{
			if(DbYearMonthList[i] == pTshapeBase->m_pTshapeMonthList[j]->m_YearMonth)
			{
				bFoundInTrade = true;
				break;
			}
		}

		if(bFoundInTrade == false)
		{
			//strTableName =  StrategyName + "_" + pOptionKind->OptionId + "_Stat";
			//sstr.clear(); sstr.str("");
			//sstr << DbYearMonthList[i];
			//string strYearMonth = sstr.str();

			//strSQLCmd = "DELETE FROM " + strTableName + " WHERE (YearMonth = " + strYearMonth + ")"; 
			//pTradeDb->ExecuteSqlCommand(strSQLCmd);

			//删除对应K线数据表，因为该合约已停止交易，其它策略也用不到
			DropKlineTable(pOptionKind, DbYearMonthList[i]);
		}
	}

	//检查旧的交易年月列表是否有合约仍旧在交易
	//如果是则将其加入新的交易年月列表
	bool bFoundInNew;
    for(size_t i = 0; i < DbYearMonthList.size(); i++)
	{
		bFoundInTrade = false;
		for(size_t j = 0; j < pTshapeBase->m_pTshapeMonthList.size(); j++)
		{
			if(DbYearMonthList[i] == pTshapeBase->m_pTshapeMonthList[j]->m_YearMonth)
			{
				bFoundInTrade = true;
				break;
			}
		}

		bFoundInNew = false;
		for(size_t j = 0; j < NewYearMonthList.size(); j++)
		{
			if(DbYearMonthList[i] == NewYearMonthList[j])
			{
				bFoundInNew = true;
				break;
			}
		}

		if(bFoundInTrade == true && bFoundInNew == false) 
			NewYearMonthList.push_back(DbYearMonthList[i]);
	}

	//更新相应的配置数据表
	strTableName =  StrategyName + "_" + pOptionKind->OptionId + "_Config";
	strSQLCmd = "DELETE FROM " + strTableName + " WHERE KeyId LIKE 'YearMonth%'";
	pTradeDb->ExecuteSqlCommand(strSQLCmd);

	char tmpString[10];
	string strCount;
	string key, value;

	sstr.clear(); sstr.str("");
	size_t YearMonthSize = (NewYearMonthSize == 0) ? 0 : NewYearMonthList.size();
	sstr << YearMonthSize;
	key = "YearMonthListSize";
	value = sstr.str();
	strSQLCmd = "INSERT INTO " + strTableName + " (KeyId, ValueExp) VALUES('" + key + "', '" + value + "')";
	pTradeDb->ExecuteSqlCommand(strSQLCmd);

	if(NewYearMonthSize > 0)
	{
		for(size_t i = 0; i < NewYearMonthList.size(); i++)
		{
			sprintf(tmpString, "%d", i);
			strCount = tmpString;
			strCount = RemoveSpace(strCount);
			sstr.clear(); sstr.str("");
			sstr << NewYearMonthList[i];

			key = "YearMonth" + strCount;
			value = sstr.str();

			strSQLCmd = "INSERT INTO " + strTableName + " (KeyId, ValueExp) VALUES('" + key + "', '" + value + "')";
			pTradeDb->ExecuteSqlCommand(strSQLCmd);
		}
	}

	//删除当前年月份之前的所有K线数据
	DeleteAllKlinesBeforeThisYearMonth(pOptionKind);

	//释放申请的内存空间
	if(pTshapeBase != NULL) delete pTshapeBase;
}

//++
//从统计数据表中获取交易年月份
//参数
//    strTableName: 统计数据表名称
//    YearMonthList: 引用，得到的交易年月份列表
//返回值
//    无
//--
void CDbContractMng::GetDbStatYearMonth(string strTableName, vector<size_t>& YearMonthList)
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;

    int RetVal;
	char *errMsg = NULL;
	char **dbResult;
	int nRow, nColumn;
	int i, j;
	string strSQLCmd;

	strSQLCmd = "SELECT DISTINCT YearMonth FROM " + strTableName; 
	RetVal = sqlite3_get_table(pTradeDb->m_pDB, strSQLCmd.c_str(), &dbResult, &nRow, &nColumn, &errMsg);

	if(RetVal != SQLITE_OK)
	{
		cerr << "打开统计数据表: " << strTableName << " 出错: " << errMsg << ENDLINE;
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

            index = pTradeDb->FieldIndex("YearMonth", FieldNameList) + (i+1)*nColumn;
			strFieldValue = dbResult[index];
			size_t YearMonth = atoi(strFieldValue.c_str());
			YearMonthList.push_back(YearMonth);
        }
	}

	sqlite3_free_table(dbResult);
}

//++
//构建T型报价并获取市场行情, 在获取市场行情后，将T型报价按照标的成交量排序
//参数
//    pOptionKind: 指向交易品种信息结构的指针 
//返回值
//     pTshapeBase: 指向T型报价的指针
//--
CTshapeBase* CDbContractMng::ConstructAndUpdateTshape(COptionKind* pOptionKind)
{
	CRealMdSpi*  pMdUserSpi = m_pRealApp->m_pCtpCntIfList[0]->m_pMdUserSpi; 
	CRealTraderSpi*  pTraderUserSpi = m_pRealApp->m_pCtpCntIfList[0]->m_pTraderUserSpi;
	CMarketDataManage* pMarketDataManage = m_pRealApp->m_pMarketDataManage;
	DWORD waitReturn;
	TThostFtdcInstrumentIDType  instId;
    CTshapeBase* pTshapeBase = NULL;

	//先将所有合约的信息都查询出来
	vector<CThostFtdcInstrumentField> SynInstInfoList;
	strcpy(instId, "*");
	pTraderUserSpi->ReqQryInstrument(instId);
	waitReturn = WaitForSingleObject(m_pRealApp->m_pCtpCntIfList[0]->m_hTraderSpiEvent, REQ_WAIT_TIME);   
	if(waitReturn != WAIT_OBJECT_0) 
	{
		cerr << m_pRealApp->m_Prompt << "超过 " << int(REQ_WAIT_TIME/1000) << " 秒没有收到 " << instId << " 合约查询回应消息，程序返回！" << ENDLINE;
        PressAnyKeyToExit();
		exit(-1);
	}
	ResetEvent(m_pRealApp->m_pCtpCntIfList[0]->m_hTraderSpiEvent);
	SynInstInfoList.swap(m_pRealApp->m_pCtpCntIfList[0]->m_InstInfoList);

	//先根据交易所信息实例化T型报价对象
	if(strcmp(pOptionKind->ExchangeId, "CZCE") == 0)
		pTshapeBase = new CTshapeCzce(pOptionKind->OptionId);
	else if(strcmp(pOptionKind->ExchangeId, "DCE") == 0)
		pTshapeBase = new CTshapeDce(pOptionKind->OptionId);
	else if(strcmp(pOptionKind->ExchangeId, "SHFE") == 0)
		pTshapeBase = new CTshapeShfe(pOptionKind->OptionId);
	else if(strcmp(pOptionKind->ExchangeId, "CFFEX") == 0)
		pTshapeBase = new CTshapeCffex(pOptionKind->OptionId, pOptionKind->UnderlyingId);
	else if(strcmp(pOptionKind->ExchangeId, "SSE") == 0)
		pTshapeBase = new CTshapeSse(pOptionKind->OptionId, pOptionKind->UnderlyingAssetType);
	else if(strcmp(pOptionKind->ExchangeId, "SZSE") == 0)
		pTshapeBase = new CTshapeSzse(pOptionKind->OptionId, pOptionKind->UnderlyingAssetType);
	else
	{
		cerr << m_pRealApp->m_Prompt << "目前还不支持交易所：" << pOptionKind->ExchangeId << " 期权， 程序返回！" << ENDLINE;
        PressAnyKeyToExit();
		exit(-1);
	}
	pTshapeBase->m_pRealApp = m_pRealApp;

	//确定需要构造的T型报价的年月份, 设置为空即交易所有可能年月份
	pTshapeBase->m_YearMonthList.clear();

	//确定是否只构造标的报价链而不是整个T型报价
	pTshapeBase->m_bUnderMdOnly = pOptionKind->bUnderMdOnly;
	pTshapeBase->m_OptCntCode = pOptionKind->OptCntCode;
	pTshapeBase->m_UnderCntCode = pOptionKind->UnderCntCode;
	pTshapeBase->m_pOptionKind = pOptionKind;
	pTshapeBase->ConstructTshape(SynInstInfoList);

	//订阅行情
	cerr << "订阅行情信息" << ENDLINE;
	vector<string> strInstList;
	pTshapeBase->GenInstrumentList(m_pRealApp->m_pCtpCntIfList[0]->m_CntCode, strInstList);
	pMarketDataManage->m_TotalInstrumentNum = strInstList.size();
	pMdUserSpi->SubscribeMarketData(strInstList);

    //等待行情数据并更新T型报价
	size_t MaxTickNum = 2;
	SleepMs(2000);    //需要等待足够时间
	for(size_t i = 0; i < MaxTickNum; i++)
	{
		waitReturn = WaitForSingleObject(m_pRealApp->m_pCtpCntIfList[0]->m_hMdSpiEvent, MD_WAIT_TIME); 
		if(waitReturn != WAIT_OBJECT_0)
		{
			SYSTEMTIME CurTime;
			GetLocalTime(&CurTime);
			cerr << "没有行情更新，等待Tick采样数据超时或者出错！" << ENDLINE;
			cerr << "当前时间: " << CurTime.wHour << ":" << CurTime.wMinute << ":" << CurTime.wSecond << ENDLINE;
		}
		else
		{
			pMarketDataManage->GetDepthMarketDataList();
			pTshapeBase->UpdateTshape(m_pRealApp->m_pCtpCntIfList[0]->m_DepthMarketDataList);
			
		} 
	} //for i

	//取消订阅
	pMdUserSpi->UnSubscribeMarketData(strInstList);
	SleepMs(250);
	pMarketDataManage->GetDepthMarketDataList();

	//对返回的T型报价年月份按成交量进行排序
	vector<CTshapeMonth*>* pTshapeMonthList = &pTshapeBase->m_pTshapeMonthList;
	sort(pTshapeMonthList->begin(), pTshapeMonthList->end(), CompUnderVolume);

	return pTshapeBase;
}

//++
//删除不再交易的期货合约的K线数据表
//参数
//    pOptionKind: 指向交易品种信息结构的指针 
//    YearMonth: 欲删除K线的合约年月份
//返回值
//    无
//--
void CDbContractMng::DropKlineTable(COptionKind* pOptionKind, size_t YearMonth)
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
    string strTableName;
	string strSQLCmd;
	stringstream sstr;

	sstr.clear(); sstr.str("");
	sstr << YearMonth;
	string strYearMonth = sstr.str();
	RemoveSpace(strYearMonth);

	if(pOptionKind->UnderlyingAssetType != 2 && pOptionKind->UnderlyingAssetType != 3)
	{
		string InstId;

		if(strcmp(pOptionKind->ExchangeId, "CZCE") == 0)
			InstId = pOptionKind->UnderlyingId + strYearMonth.substr(3, 3);
		else
			InstId = pOptionKind->UnderlyingId + strYearMonth.substr(2, 4);

		strTableName = "zzKline_" + InstId;
		strSQLCmd = "DROP TABLE " + strTableName;
		pTradeDb->ExecuteSqlCommand(strSQLCmd);
	}
}

//++
//删除当前年月份之前的所有K线数据表
//参数
//    pOptionKind: 指向交易品种信息结构的指针 
//返回值
//    无
//--
void CDbContractMng::DeleteAllKlinesBeforeThisYearMonth(COptionKind* pOptionKind)
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
    string strTableName;
	string strSQLCmd;
	stringstream sstr;

	SYSTEMTIME CurTime;
	GetLocalTime(&CurTime);

	string strKlinePrefix = "zzKline_" + pOptionKind->UnderlyingId;
	//由于读写权限问题, 无法使用系统表MSysObjects
    //sstr << "SELECT NAME FROM MSYSOBJECTS WHERE TYPE=1 AND NAME NOT LIKE 'Msys*' AND (LEFT(NAME, "
	//	 << strKlinePrefix.size() << ") = " << strKlinePrefix << ")";

	//先删除前5年所有年月份的所有可能的K线数据表
	for(int Year = CurTime.wYear - 5; Year < CurTime.wYear; Year++)
	{
		for(int Month = 1; Month <= 12; Month++)
		{
			int YearMonth = Year * 100 + Month;
			sstr.clear(); sstr.str("");
			sstr << YearMonth;
            string strYearMonth = sstr.str();
			RemoveSpace(strYearMonth);
			if(strcmp(pOptionKind->ExchangeId, "CZCE") == 0)
			{
				strYearMonth = strYearMonth.substr(3, 3);
			}
			else
			{
				strYearMonth = strYearMonth.substr(2, 4);
			}

			strTableName = strKlinePrefix + strYearMonth;
			if(pTradeDb->IsTableExist(strTableName))
			{
				strSQLCmd = "DROP TABLE " + strTableName;
				pTradeDb->ExecuteSqlCommand(strSQLCmd);
			}		
		}
	}

	//删除当年当前月份之前的所有可能的K线数据表
	for(int Month = 1; Month < CurTime.wMonth; Month++)
	{
		int YearMonth = CurTime.wYear * 100 + Month;
		sstr.clear(); sstr.str("");
		sstr << YearMonth;
        string strYearMonth = sstr.str();
		RemoveSpace(strYearMonth);

		if(strcmp(pOptionKind->ExchangeId, "CZCE") == 0)
		{
			strYearMonth = strYearMonth.substr(3, 3);
		}
		else
		{
			strYearMonth = strYearMonth.substr(2, 4);
		}

		strTableName = strKlinePrefix + strYearMonth;
		if(pTradeDb->IsTableExist(strTableName))
		{
			strSQLCmd = "DROP TABLE " + strTableName;
			pTradeDb->ExecuteSqlCommand(strSQLCmd);
		}		
	}
}

//++
//删除单个交易品种的相关设置和数据
//参数
//    StrategyName: 策略名称
//    pOptionKind: 指向交易品种信息结构的指针
//返回值
//    无
//--
void CDbContractMng::DeleteIndividualOptionKind(string StrategyName, COptionKind* pOptionKind)
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
    string strTableName;
	string strSQLCmd;

	//删除统计数据表
	strTableName =  StrategyName + "_" + pOptionKind->OptionId + "_Stat";
	if(pTradeDb->IsTableExist(strTableName))
	{
		strSQLCmd = "DROP TABLE " + strTableName; 
		pTradeDb->ExecuteSqlCommand(strSQLCmd);
	}

	////删除K线数据表（有可能其它策略还在交易该品种， 需要K线数据）
	//for(size_t i = 0; i < pOptionKind->YearMonthList.size(); i++)
	//	DropKlineTable(pOptionKind, pOptionKind->YearMonthList[i]);

	//删除策略交易品种配置数据表
	strTableName =  StrategyName + "_" + pOptionKind->OptionId + "_Config";
	if(pTradeDb->IsTableExist(strTableName))
	{
		strSQLCmd = "DROP TABLE " + strTableName; 
		pTradeDb->ExecuteSqlCommand(strSQLCmd);
	}

	//将相应key值设为空字符串，作为删除后的标志
	pOptionKind->OptionId = "";
	pOptionKind->UnderlyingId = "";
}

//++
//删除交易品种后修改策略配置文件
//参数
//    StrategyName: 策略名称
//    pOptionKindList: 指向交易品种信息结构列表的指针
//返回值
//    无
//--
void CDbContractMng::ModifyStrategyConfig(string StrategyName, vector<COptionKind>* pOptionKindList)
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
    string strTableName;
	string strSQLCmd;
	stringstream sstr;
	vector<COptionKind> NewOptionKindList;

	//重新构造一个没有删除数据的交易品种信息列表
	for(size_t i = 0; i < pOptionKindList->size(); i++)
	{
		if((*pOptionKindList)[i].OptionId != "")
			NewOptionKindList.push_back((*pOptionKindList)[i]);
	}

	//删除策略配置数据表的旧信息
	strTableName =  StrategyName + "_Config";

	strSQLCmd = "DELETE FROM " + strTableName + " WHERE (KeyId LIKE 'OptionId%') OR (KeyId LIKE 'UnderlyingId%') OR (KeyId LIKE 'ExchangeId%')";
	pTradeDb->ExecuteSqlCommand(strSQLCmd);

	//更新交易品种数目尺寸
	sstr.clear(); sstr.str("");
	sstr << NewOptionKindList.size();
	strSQLCmd = "UPDATE " + strTableName + " SET ValueExp = '" + sstr.str() + "' WHERE KeyId = 'OptionKindListSize'";
	pTradeDb->ExecuteSqlCommand(strSQLCmd);

	//将更新后的交易品种信息写入策略配置数据表
	for(size_t i = 0; i < NewOptionKindList.size(); i++)
	{
		string key, value;
		string strCount;
		char tmpString[10];
		sprintf(tmpString, "%d", i);
		strCount = tmpString;
		strCount = RemoveSpace(strCount);

		//插入OptionId
		key = "OptionId" + strCount;
		value = NewOptionKindList[i].OptionId;
		strSQLCmd = "INSERT INTO " + strTableName + "(KeyId, ValueExp) VALUES('" + key + "', '" + value + "')";
		pTradeDb->ExecuteSqlCommand(strSQLCmd);

		//插入UnderlyingId
		key = "UnderlyingId" + strCount;
		value = NewOptionKindList[i].UnderlyingId;
		strSQLCmd = "INSERT INTO " + strTableName + "(KeyId, ValueExp) VALUES('" + key + "', '" + value + "')";
		pTradeDb->ExecuteSqlCommand(strSQLCmd);

		//插入ExchangeId
		key = "ExchangeId" + strCount;
		value = NewOptionKindList[i].ExchangeId;
		strSQLCmd = "INSERT INTO " + strTableName + "(KeyId, ValueExp) VALUES('" + key + "', '" + value + "')";
		pTradeDb->ExecuteSqlCommand(strSQLCmd);
	}
}

//++
//比较月份T型报价标的的成交量, 用于Vector排序，按从大到小排列
//--
bool CompUnderVolume(CTshapeMonth* pTshapeMonth0, CTshapeMonth* pTshapeMonth1)
{
    CThostFtdcDepthMarketDataField* pMd0 = &pTshapeMonth0->m_UnderlyingMarketData;
	CThostFtdcDepthMarketDataField* pMd1 = &pTshapeMonth1->m_UnderlyingMarketData;

	return (pMd0->Volume > pMd1->Volume);
}
