#include "StrategyVoidTest.h"
#include "RealApp.h"
#include "TradeDb.h"
#include "KeyValue.h"

//++
//构造函数
//--
CStrategyVoidTest::CStrategyVoidTest(CRealApp* pRealApp)
{
	m_pRealApp = pRealApp;
	m_StrategyName = "VoidTest";
}

//++
//析构函数
//--
CStrategyVoidTest::~CStrategyVoidTest()
{
}


//++
//策略退出时的清理工作
void CStrategyVoidTest::Cleanup()
{
	//清除组合对象申请的内存空间
	for(size_t i = 0; i < m_pPortfolioList.size(); i++)
	{
		if(m_pPortfolioList[i] != NULL)
		{
			delete m_pPortfolioList[i];
			m_pPortfolioList[i] = NULL;
		}
	}
}

//++
//初始化统计对象，一般在程序初始化的最后调用
//--
void CStrategyVoidTest::InitializeStat()
{
}

//++
//更新策略统计数据
//--
void CStrategyVoidTest::UpdateStat()
{
}

//++
//将统计数据写入数据库，在交易时段结束时调用
//--
bool CStrategyVoidTest::DumpStatData()
{
	return true;
}

//++
//策略初始化
//从数据库/配置文件，或者键盘输入得到配置信息设置策略交易期权的参数
//同时查询数据库中的表格获得组合历史持仓
//--
void CStrategyVoidTest::Initialize()
{
	CTradeDb* pTradeDb = m_pRealApp->m_pTradeDb;
	CKeyValue VoidTestKeyValue;
	string strVoidTestTableName;
	string strVoidTestFileName;

	//获取策略配置，填充m_OptionKindList列表
	cerr << "--->>>" << ENDLINE;
	cerr << "获取策略 " << m_StrategyName << " 配置" << ENDLINE;
	strVoidTestTableName = m_StrategyName + "_Config";

#ifdef WIN_CTP
	if(m_pRealApp->m_ArchName == "")
		strVoidTestFileName = ".\\config\\" + strVoidTestTableName + ".txt";
	else
		strVoidTestFileName = ".\\config\\" + m_pRealApp->m_ArchName + "\\" + strVoidTestTableName + ".txt";
#else
	if(m_pRealApp->m_ArchName == "")
		strVoidTestFileName = "./config/" + strVoidTestTableName + ".txt";
	else
		strVoidTestFileName = "./config/" + m_pRealApp->m_ArchName + "/" + strVoidTestTableName + ".txt";
#endif

	int RetRead = VoidTestKeyValue.ReadConfigFromDb(pTradeDb, strVoidTestTableName);
	if(RetRead <= 0) VoidTestKeyValue.ReadConfigFromTextFile(strVoidTestFileName);

	string key, value;
	string strCount;
	char tmpString[10];
	int OptionKindListSize;

	//获取期权品种的相关信息
	m_OptionKindList.clear();
	OptionKindListSize = VoidTestKeyValue.InputIntKeyValue("OptionKindListSize");
	cerr << "---<<<" << ENDLINE;
	for(int i = 0; i < OptionKindListSize; i++)
	{
		COptionKind OptionKind;

		sprintf(tmpString, "%d", i);
		strCount = tmpString;
		strCount = RemoveSpace(strCount);

		//获取OptionId
		key = "OptionId" + strCount;
		OptionKind.OptionId = VoidTestKeyValue.InputStringKeyValue(key);

		//获取UnderlyingId
		key = "UnderlyingId" + strCount;
		OptionKind.UnderlyingId = VoidTestKeyValue.InputStringKeyValue(key);

		//获取ExchangeId
		key = "ExchangeId" + strCount;
		value = VoidTestKeyValue.InputStringKeyValue(key);
		strcpy(OptionKind.ExchangeId, value.c_str());

		//针对每种期权品种，获取YearMonthList和SegTimeList及其它参数
		string strOptionKindTableName;
		string strOptionKindFileName;
		CKeyValue OptionKindKeyValue;
		int YearMonthListSize;
		int SegTimeListSize;

		cerr << "获取 " << OptionKind.ExchangeId << " 股票期权品种 " << OptionKind.OptionId << " 配置" << ENDLINE;
		cerr << "--->>>" << ENDLINE;
		strOptionKindTableName = m_StrategyName + "_" + OptionKind.OptionId + "_Config";

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

		//获取UnderlyingAssetType
		key = "UnderlyingAssetType";
		OptionKind.UnderlyingAssetType = OptionKindKeyValue.InputIntKeyValue(key);

		//获取YearMonthList
		OptionKind.YearMonthList.clear();
		YearMonthListSize = OptionKindKeyValue.InputIntKeyValue("YearMonthListSize");
		for(int j = 0; j < YearMonthListSize; j++)
		{
			size_t YearMonth;

			sprintf(tmpString, "%d", j);
			strCount = tmpString;
			strCount = RemoveSpace(strCount);

			key = "YearMonth" + strCount;
			YearMonth = OptionKindKeyValue.InputIntKeyValue(key);
			OptionKind.YearMonthList.push_back(YearMonth);
		}

		//获取SegTimeList
		OptionKind.SegTimeList.clear();
		SegTimeListSize = OptionKindKeyValue.InputIntKeyValue("SegTimeListSize");
		for(int j = 0; j < SegTimeListSize; j++)
		{
			CSegTime SegTime;

			sprintf(tmpString, "%d", j);
			strCount = tmpString;
			strCount = RemoveSpace(strCount);

			key = "SegBeginTime" + strCount;
			SegTime.SegBeginTime = OptionKindKeyValue.InputFloatKeyValue(key);
			key = "SegEndTime" + strCount;
			SegTime.SegEndTime = OptionKindKeyValue.InputFloatKeyValue(key);

			OptionKind.SegTimeList.push_back(SegTime);
		}

		m_OptionKindList.push_back(OptionKind);

		//针对每种期权产品，将其配置写入数据库
		OptionKindKeyValue.WriteConfigToDb(pTradeDb, strOptionKindTableName);

		cerr << "---<<<" << ENDLINE;
	} //for i

	//将策略顶层配置写入数据库
	VoidTestKeyValue.WriteConfigToDb(pTradeDb, strVoidTestTableName);

	cerr << "---<<<" << ENDLINE;

	//从数据库中获取历史交易组合信息，为减少函数代码，用子函数实现
	GetHistoryPortfolioFromDb();

	//根据从数据库中读出的数据初始化m_PortfolioCount
	InitPortfolioCount();
}

//++
//从数据库中获取历史交易组合信息
//--
void CStrategyVoidTest::GetHistoryPortfolioFromDb()
{
}

//++
//扫描T型报价, 产生可以开仓的组合列表
//参数
//    pPortfolioList: 引用返回值, 可以开仓的组合列表
//返回值
//    true: 有可以开仓的组合; false: 无可开仓组合
//--
bool CStrategyVoidTest::ScanForOpen(vector<CPortfolioBase*>& pPortfolioList)
{
	return false;
}

//++
//对ScanForOpen中产生的组合进行条件过滤
//在空函数的情况下就是不作任何处理
//参数
//    pPortfolioToOpen
//返回值
//    无
//--
void CStrategyVoidTest::OpenFilter(CPortfolioVoidTest* pPortfolioToOpen)
{
}

//++
//判断是否有最近开仓组合其包含有与待开仓组合相同的合约并且计算结果为亏损
//参数
//    pPortfolioToOpen: 待开仓组合
//    IntSecs: 时间间隔, 以秒为单位
//返回值
//    true: 有
//    false: 无
//--
bool CStrategyVoidTest::HasRecentlyLostTrade(CPortfolioVoidTest* pPortfolioToOpen, double IntSecs)
{
	return false;
}

//++
//扫描策略的所有已开未平组合, 产生可以平仓的组合列表
//参数
//    pPortfolioList: 引用返回值, 可以平仓的组合列表
//返回值
//    true: 有可以平仓的组合; false: 无可平仓组合
//--
bool CStrategyVoidTest::ScanForClose(vector<CPortfolioBase*>& pPortfolioList)
{
	return false;
}

