#include "BrokerServer.h"
#include "TradeDb.h"
#include "KeyValue.h"

//++
//构造函数
//--
CBrokerServer::CBrokerServer(string strCounterName, string strArchName, CTradeDb* pTradeDb)
{
	m_CounterName = strCounterName;
	m_ArchName = strArchName;
	m_pTradeDb = pTradeDb;

	GetBrokerServerFromDb();
}

//++
//析构函数
//--
CBrokerServer::~CBrokerServer()
{
}

//++
//设置经纪商服务器
//--
void CBrokerServer::SetBrokerServer()
{
	if(m_CounterName == "ZXQH_SIM")
		ZxqhSimServer();
	else if(m_CounterName == "SHZQ_SIM")
		ShzqSimServer();
	else if(m_CounterName == "NHQH_SIM")
		NhqhSimServer();
	else if(m_CounterName == "SOPT_NHQH_SIM")
		SoptNhqhSimServer();
	else if(m_CounterName == "STOCK_NHQH_SIM")
		StockNhqhSimServer();
	else if(m_CounterName == "SIMNOW")
		SimNowServer();
	else if(m_CounterName == "ZXQH_TRUE")
		ZxqhTrueServer();
	else if(m_CounterName == "SHZQ_TRUE")
		ShzqTrueServer();
	else if(m_CounterName == "NHQH_TRUE")
		NhqhTrueServer();
	else
	{
		cerr << "从交易数据库获取代号为 " << m_CounterName << " 的经纪商交易服务器相关信息..." <<  ENDLINE;
		GetBrokerServerFromDb();
	}
}

//++
//从交易数据库获取经纪商交易服务器信息
//--
void CBrokerServer::GetBrokerServerFromDb()
{
	CTradeDb* pTradeDb = (CTradeDb*)m_pTradeDb;
	string strPrompt = m_ArchName + "$";

	//数据库对象不存在则重新生成
	if(pTradeDb == NULL)
	{
#ifdef WIN_CTP
		string strDirPath = "..\\TradeDatabase";
#else
		string strDirPath = "../TradeDatabase";
#endif
		string strDbName = "TradeDb.db";
		if(m_ArchName != "") strDbName = "TradeDb-" + m_ArchName + ".db";
		pTradeDb = new CTradeDb(strDirPath, strDbName);
		pTradeDb->Init();
	}

	CKeyValue FrontKeyValue(true);
	string key, value;
	string strCount;
	char tmpString[10];

#ifdef WIN_CTP
	cerr << "获取交易和行情前置服务器信息..." << ENDLINE;
	string strFrontTableName = "FrontServer_" + m_CounterName;
	int RetRead = FrontKeyValue.ReadConfigFromDb(pTradeDb, strFrontTableName);
	string strConfigDir = ".\\config";
	if(m_ArchName != "") strConfigDir = ".\\config\\" + m_ArchName;
	string strFilePath = strConfigDir + "\\" + strFrontTableName + ".txt";
	if(RetRead <= 0) FrontKeyValue.ReadConfigFromTextFile(strFilePath);
#else
	cerr << "获取交易和行情前置服务器信息..." << ENDLINE;
	string strFrontTableName = "FrontServer_" + m_CounterName;
	int RetRead = FrontKeyValue.ReadConfigFromDb(pTradeDb, strFrontTableName);
	string strConfigDir = "./config";
	if(m_ArchName != "") strConfigDir = "./config/" + m_ArchName;
	string strFilePath = strConfigDir + "/" + strFrontTableName + ".txt";
	if(RetRead <= 0) FrontKeyValue.ReadConfigFromTextFile(strFilePath);
#endif

	//读取交易前置信息
	int TradeFrontSize = FrontKeyValue.InputIntKeyValue("TradeFrontSize");
	m_TraderFrontList.clear();
	for(int i = 0; i < TradeFrontSize; i++)
	{
		sprintf(tmpString, "%d", i);
		strCount = tmpString;
		strCount = RemoveSpace(strCount);

		key = "TradeFront" + strCount;
		value = FrontKeyValue.InputStringKeyValue(key);
		m_TraderFrontList.push_back(value);
	}

	//读取行情前置信息
	int MdFrontSize = FrontKeyValue.InputIntKeyValue("MdFrontSize");
	m_MdFrontList.clear();
	for(int i = 0; i < MdFrontSize; i++)
	{
		sprintf(tmpString, "%d", i);
		strCount = tmpString;
		strCount = RemoveSpace(strCount);

		key = "MdFront" + strCount;
		value = FrontKeyValue.InputStringKeyValue(key);
		m_MdFrontList.push_back(value);
	}

	//将配置信息更新回数据库
	FrontKeyValue.WriteConfigToDb(pTradeDb, strFrontTableName);

	//如果是在程序内部申请的数据库对象，需要删除
	if(m_pTradeDb == NULL && pTradeDb != NULL)
	{
		pTradeDb->Destroy();
		delete pTradeDb;
		pTradeDb = NULL;
	}
}

//++
//中信期货模拟盘服务器
// 前置地址，中信期货仿真-电信（1,3对换，是因为1经过测试不能使用）, 测试发现3/4是可用的
//BrokerID=66666
//UserID = 903936
//Passwd: ID后六位
//--
void CBrokerServer::ZxqhSimServer()
{
	string strMdFront[] = {
		"tcp://ctpfz1-front2.citicsf.com:51213", 
		"tcp://ctpfz1-front4.citicsf.com:51213",
		"tcp://ctpfz1-front3.citicsf.com:51213", 
		"tcp://ctpfz1-front1.citicsf.com:51213" 
	};

	string strTraderFront[] = {
		"tcp://ctpfz1-front2.citicsf.com:51205",
		"tcp://ctpfz1-front4.citicsf.com:51205",
		"tcp://ctpfz1-front3.citicsf.com:51205",
		"tcp://ctpfz1-front1.citicsf.com:51205"
	};

	m_MdFrontList.clear();
	m_TraderFrontList.clear();
	for(size_t i = 0; i < 4; i++)
	{
	    m_MdFrontList.push_back(strMdFront[i]);
	    m_TraderFrontList.push_back(strTraderFront[i]);
	}	    
}

//++
//上海中期模拟盘服务器
// 前置地址，上海中期实盘交易
//mdFront     "tcp://zjzx-md11.ctp.shcifco.com:41213"
//tradeFront  "tcp://zjzx-front11.ctp.shcifco.com:41205"

// 前置地址，上海中期期权模拟, 在...\上海中期-博易云期权仿真版\pobo5\cfg\backup\Consign.txt文件中找到
//根据上海中期的端口规范，猜测行情服务器端口后两位数字为13
//BrokerID=8000 
//UserID = 333602580
//Passwd: ID后六位
//--
void CBrokerServer::ShzqSimServer()
{
	string strMdFront[] = {
        "tcp://101.231.179.196:32313"
	};

	string strTraderFront[] = {
        "tcp://101.231.179.196:32305"
	};

	m_MdFrontList.clear();
	m_TraderFrontList.clear();
	for(size_t i = 0; i < 1; i++)
	{
	    m_MdFrontList.push_back(strMdFront[i]);
	    m_TraderFrontList.push_back(strTraderFront[i]);
	}	    
}

//++
//南华期货模拟盘服务器
//在快期下寻找broker文件
//broker BrokerID="1008" CounterName="南华期货期权仿真" >
//UserId = 90095641
//--
void CBrokerServer::NhqhSimServer()
{
	string strMdFront[] = {
        "tcp://115.238.106.253: 41213"
	};

	string strTraderFront[] = {
        "tcp://115.238.106.253: 41205"
	};

	m_MdFrontList.clear();
	m_TraderFrontList.clear();
	for(size_t i = 0; i < 1; i++)
	{
	    m_MdFrontList.push_back(strMdFront[i]);
	    m_TraderFrontList.push_back(strTraderFront[i]);
	}	    
}

//++
//南华期货股票期权模拟服务器
//broker BrokerID="8050" CounterName="南华股票期权仿真" 
//UserId = 90095641
//--
void CBrokerServer::SoptNhqhSimServer()
{
	string strMdFront[] = {
        "tcp://115.238.106.252:41313"
	};

	string strTraderFront[] = {
        "tcp://115.238.106.252:41305"
	};

	m_MdFrontList.clear();
	m_TraderFrontList.clear();
	for(size_t i = 0; i < 1; i++)
	{
	    m_MdFrontList.push_back(strMdFront[i]);
	    m_TraderFrontList.push_back(strTraderFront[i]);
	}	    
}

//++
//南华期货证券交易模拟服务器
//broker BrokerID="8050" CounterName="南华证券交易仿真" 
//UserId = 0000001338
//--

void CBrokerServer::StockNhqhSimServer()
{
	string strMdFront[] = {
        "tcp://115.238.106.253:41413"
	};

	string strTraderFront[] = {
        "tcp://115.238.106.253:41405"
	};

	m_MdFrontList.clear();
	m_TraderFrontList.clear();
	for(size_t i = 0; i < 1; i++)
	{
	    m_MdFrontList.push_back(strMdFront[i]);
	    m_TraderFrontList.push_back(strTraderFront[i]);
	}	    
}

//++
//SimNow模拟盘服务器
//--
void CBrokerServer::SimNowServer()
{
	string strMdFront[] = {
        //"tcp://180.168.146.187: 10010",
		"tcp://180.168.146.187: 10011",
		"tcp://218.202.237.33 : 10012",
		"tcp://180.168.146.187: 10013"     //CTP mini
	};

	string strTraderFront[] = {
        //"tcp://180.168.146.187: 10000",
		"tcp://180.168.146.187: 10001",
		"tcp://218.202.237.33 : 10002",
		"tcp://180.168.146.187: 10003"      //CTP mini
	};

	m_MdFrontList.clear();
	m_TraderFrontList.clear();
	for(size_t i = 0; i < 3; i++)
	{
	    m_MdFrontList.push_back(strMdFront[i]);
	    m_TraderFrontList.push_back(strTraderFront[i]);
	}	    

}

//++
//中信期货实盘服务器
//BrokerId: 66666
//--
void CBrokerServer::ZxqhTrueServer()
{
	string strMdFront[] = {
		"tcp://ctp1-md1.citicsf.com:41213", 
		"tcp://ctp1-md3.citicsf.com:41213",
		"tcp://180.169.101.177:41213", 
		"tcp://ctp1-md2.citicsf.com:41213",
		"tcp://ctp1-md4.citicsf.com:41213",
		"tcp://27.115.78.193:41213"
	};

	string strTraderFront[] = {
		"tcp://ctp1-front1.citicsf.com:41205",
		"tcp://ctp1-front3.citicsf.com:41205",
		"tcp://180.169.101.177:41205",
		"tcp://ctp1-front2.citicsf.com:41205",
		"tcp://ctp1-front4.citicsf.com:41205",
		"tcp://58.246.202.226:41205"
	};

	m_MdFrontList.clear();
	m_TraderFrontList.clear();
	for(size_t i = 0; i < 6; i++)
	{
	    m_MdFrontList.push_back(strMdFront[i]);
	    m_TraderFrontList.push_back(strTraderFront[i]);
	}	    
}

//++
//上海中期实盘服务器
//BrokerId: 8070
//--
void CBrokerServer::ShzqTrueServer()
{
	string strMdFront[] = {
		"tcp://zjzx-md11.ctp.shcifco.com:41213", 
		"tcp://27.115.57.145:41213",
		"tcp://zjzx-md12.ctp.shcifco.com:41213", 
		"tcp://27.115.57.146:41213",
		"tcp://zjzx-md13.ctp.shcifco.com:41213",
		"tcp://27.115.57.147:41213",
		"tcp://zjzx-md14.ctp.shcifco.com:41213", 
		"tcp://27.115.57.148:41213",
		"tcp://zjzx-md15.ctp.shcifco.com:41213",
		"tcp://27.115.57.150:41213",

		"tcp://180.168.214.244:41213", 
		"tcp://180.168.214.245:41213",
		"tcp://180.168.214.246:41213", 
		"tcp://180.168.214.247:41213",
		"tcp://180.168.214.248:41213"
	};

	string strTraderFront[] = {
		"tcp://zjzx-front11.ctp.shcifco.com:41205",
		"tcp://27.115.57.145:41205",
		"tcp://zjzx-front12.ctp.shcifco.com:41205",
		"tcp://27.115.57.146:41205",
		"tcp://zjzx-front13.ctp.shcifco.com:41205",
		"tcp://27.115.57.147:41205",
		"tcp://zjzx-front14.ctp.shcifco.com:41205",
		"tcp://27.115.57.148:41205",
		"tcp://zjzx-front15.ctp.shcifco.com:41205",
		"tcp://27.115.57.150:41205",

		"tcp://180.168.214.244:41205",
		"tcp://180.168.214.245:41205",
		"tcp://180.168.214.246:41205",
		"tcp://180.168.214.247:41205",
		"tcp://180.168.214.248:41205"
	};

	m_MdFrontList.clear();
	m_TraderFrontList.clear();
	for(size_t i = 0; i < 15; i++)
	{
	    m_MdFrontList.push_back(strMdFront[i]);
	    m_TraderFrontList.push_back(strTraderFront[i]);
	}	    
}

//++
//南华期货实盘服务器
//南华期货联通站点经测速更快20ms以内
//BrokerId: 8050
//--
void CBrokerServer::NhqhTrueServer()
{
	string strMdFront[] = {
		"tcp://27.115.56.233:41213", 
		"tcp://27.115.56.234:41213",
		"tcp://124.160.44.174:41213", 
		"tcp://124.160.44.169:41213",

		"tcp://180.169.30.170:41213", 
		"tcp://122.224.116.158:41213",
		"tcp://220.189.211.198:41213"
	};

	string strTraderFront[] = {
		"tcp://27.115.56.233:41205",
		"tcp://27.115.56.234:41205",
		"tcp://124.160.44.174:41205",
		"tcp://124.160.44.169:41205",

		"tcp://180.169.30.170:41205",
		"tcp://122.224.116.158:41205",
		"tcp://220.189.211.198:41205"
	};

	m_MdFrontList.clear();
	m_TraderFrontList.clear();
	for(size_t i = 0; i < 7; i++)
	{
	    m_MdFrontList.push_back(strMdFront[i]);
	    m_TraderFrontList.push_back(strTraderFront[i]);
	}	    
}

