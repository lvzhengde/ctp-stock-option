#ifndef _TEST_APP_H_
#define _TEST_APP_H_

#include "common.h"
#include "TestMdSpi.h"
#include "TestTraderSpi.h"
#include "MyEvent.h"

class CTestApp
{
public:
	string m_ArchName;    //交易架构的名称
	string   m_CounterName;                  //经纪商报盘柜台名称
	TThostFtdcBrokerIDType m_brokerId;		// 经纪商代码
	TThostFtdcUserIDType m_userId;		    // 投资者代码
	TThostFtdcPasswordType	m_Password;     //交易密码
	bool    m_bNeedAuth;     //是否需要用户产品认证
	string	m_UserProductInfo;  //用户端产品信息
	string m_AppID;
	string	m_AuthCode;     //认证码
	CTradeDb* m_pTradeDb;   //交易数据库

    vector<CRspInstrument>   m_RspInstrumentList;
    vector<CRtnMarketData>   m_RtnMarketDataList;
    int m_requestId;  
    CEventHandle m_hEvent;

	CThostFtdcMdApi*  m_pMdUserApi;
	CTestMdSpi*  m_pMdUserSpi; 
	CThostFtdcTraderApi*  m_pTraderUserApi;
	CTestTraderSpi*  m_pTraderUserSpi;

public:
	CTestApp(string strArchName = "");
	~CTestApp();

	void SaveInfoToDatabase();
	void ShowTraderCommand(CTestTraderSpi* p, bool print=false);
	void ShowMdCommand(CTestMdSpi* p, bool print=false);
	void DumpInstAndMarketData();
    void QueryAndSubscribe(void* pTraderSpi, void* pMdSpi, char* InstID);
	void TestMd();
	void TestTrader();
	void TestMain();
#ifdef WIN_CTP
	static BOOL WINAPI ConsoleHandler(DWORD CEvent);
#else
    static void SignalHandler(int signo);
#endif
};

#endif
