#include "TestMdSpi.h"
#include "TestApp.h"
#include "StringCodeConvert.h"

//++
//构造函数
//--
CTestMdSpi::CTestMdSpi(CThostFtdcMdApi* api, CTestApp* pApp):m_pUserApi(api), m_pTestApp(pApp)
{
	m_pUserApi = api;
	m_pTestApp = pApp;

	m_Prompt = m_pTestApp->m_ArchName + "/" + m_pTestApp->m_CounterName + "$";
}


void CTestMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast)
{
    IsErrorRspInfo(pRspInfo);
}

void CTestMdSpi::OnFrontDisconnected(int nReason)
{
    cerr << m_Prompt <<" 响应 | 行情前置连接中断..." 
      << " reason=" << nReason << ENDLINE;
}
		
void CTestMdSpi::OnHeartBeatWarning(int nTimeLapse)
{
    cerr << m_Prompt <<" 响应 | 行情前置心跳超时警告..." 
      << " TimerLapse = " << nTimeLapse << ENDLINE;
}

void CTestMdSpi::OnFrontConnected()
{
	cerr << m_Prompt <<" 连接行情前置...成功"<<ENDLINE;

    SetEvent(m_pTestApp->m_hEvent);
}

void CTestMdSpi::ReqUserLogin(TThostFtdcBrokerIDType	brokerId,
	        TThostFtdcUserIDType	userId,	TThostFtdcPasswordType	passwd)
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, brokerId);
	strcpy(req.UserID, userId);
	strcpy(req.Password, passwd);
	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqUserLogin(&req, m_pTestApp->m_requestId);
    cerr << m_Prompt <<" 请求 | 发送行情前置登录请求..."<<((ret == 0) ? "成功" :"失败") << ENDLINE;	

    SetEvent(m_pTestApp->m_hEvent);
}

///登出请求
int CTestMdSpi::ReqUserLogout(TThostFtdcBrokerIDType brokerId, TThostFtdcUserIDType	userId)
{
	CThostFtdcUserLogoutField UserLogout;

	memset(&UserLogout, 0, sizeof(CThostFtdcUserLogoutField));
	strcpy(UserLogout.BrokerID, brokerId); 
	strcpy(UserLogout.UserID, userId);  
	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqUserLogout(&UserLogout, m_pTestApp->m_requestId); 
    cerr << m_Prompt <<" 请求 | 发送行情前置登出请求..."<<((ret == 0) ? "成功" :"失败") << ENDLINE;	

	return ret;
}

void CTestMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pRspUserLogin)
	{
        cerr << m_Prompt <<" 响应 | 登录行情前置成功...当前交易日:"
          <<pRspUserLogin->TradingDay<<ENDLINE;
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);
}

///登出请求响应
void CTestMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if ( !IsErrorRspInfo(pRspInfo) && pUserLogout ) {  
		// 保存会话参数	
		cerr << m_Prompt << " 响应 | 登出行情前置成功..."
			<< " 经纪商代码: " << pUserLogout->BrokerID 
			<< " 客户代码: " << pUserLogout->UserID << ENDLINE;
	}

    if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);
}

void CTestMdSpi::SubscribeMarketData(char* instIdList)
{
	vector<char*> list;
	char *token = strtok(instIdList, ",");
	while( token != NULL ){
		list.push_back(token); 
		token = strtok(NULL, ",");
	}
	size_t len = list.size();
	char** pInstId = new char* [len];  
	for(size_t i=0; i<len;i++)  pInstId[i]=list[i]; 
	int ret=m_pUserApi->SubscribeMarketData(pInstId, len);
	cerr << m_Prompt <<" 请求 | 发送行情订阅... "<<((ret == 0) ? "成功" : "失败")<< ENDLINE;

    SetEvent(m_pTestApp->m_hEvent);
}

void CTestMdSpi::SubscribeMarketDataVector()
{
	size_t len = m_pTestApp->m_RspInstrumentList.size();
	char** pInstId = new char* [len];  
	for(size_t i=0; i<len;i++)  pInstId[i]= m_pTestApp->m_RspInstrumentList[i].Instrument.InstrumentID;  
	int ret=m_pUserApi->SubscribeMarketData(pInstId, len);
	cerr << m_Prompt <<" 请求 | 发送行情订阅... "<<((ret == 0) ? "成功" : "失败")<< ENDLINE;

    SetEvent(m_pTestApp->m_hEvent);
}

void CTestMdSpi::OnRspSubMarketData(
         CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << m_Prompt <<" 响应 |  行情订阅...成功"<<ENDLINE;
	if(bIsLast)  
		SetEvent(m_pTestApp->m_hEvent);	
}

void CTestMdSpi::OnRspUnSubMarketData(
             CThostFtdcSpecificInstrumentField *pSpecificInstrument,
             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << m_Prompt <<" 响应 |  行情取消订阅...成功"<<ENDLINE;
	if(bIsLast)  
		SetEvent(m_pTestApp->m_hEvent);	
}

void CTestMdSpi::OnRtnDepthMarketData(
             CThostFtdcDepthMarketDataField *pDepthMarketData)
{
#ifndef DUMP_TO_FILE
	SYSTEMTIME CurTime;
	GetLocalTime(&CurTime);
	
  cerr << m_Prompt 
	<< " 本地时间: " << CurTime.wHour << ":" << CurTime.wMinute << ":" << CurTime.wSecond << "." << CurTime.wMilliseconds
	<< ENDLINE
	<< " 行情时间: " << pDepthMarketData->UpdateTime << "." << pDepthMarketData->UpdateMillisec
	<<ENDLINE
	<< " 行情 | 合约:" << pDepthMarketData->InstrumentID
    << " 现价:" << pDepthMarketData->LastPrice
	<< " 昨收盘价:" << pDepthMarketData->PreClosePrice
	<< " 涨停价:" << pDepthMarketData->UpperLimitPrice
	<< " 跌停价:" << pDepthMarketData->LowerLimitPrice
    << ENDLINE
    << " 卖一价:" << pDepthMarketData->AskPrice1 
    << " 卖一量:" << pDepthMarketData->AskVolume1 
    << " 卖二价:" << pDepthMarketData->AskPrice2 
    << " 卖二量:" << pDepthMarketData->AskVolume2 
    << " 卖三价:" << pDepthMarketData->AskPrice3 
    << " 卖三量:" << pDepthMarketData->AskVolume3 
    << " 卖四价:" << pDepthMarketData->AskPrice4 
    << " 卖四量:" << pDepthMarketData->AskVolume4 
    << " 卖五价:" << pDepthMarketData->AskPrice5 
    << " 卖五量:" << pDepthMarketData->AskVolume5 
	<<ENDLINE
    << " 买一价:" << pDepthMarketData->BidPrice1
    << " 买一量:" << pDepthMarketData->BidVolume1 
    << " 买二价:" << pDepthMarketData->BidPrice2
    << " 买二量:" << pDepthMarketData->BidVolume2 
    << " 买三价:" << pDepthMarketData->BidPrice3
    << " 买三量:" << pDepthMarketData->BidVolume3 
    << " 买四价:" << pDepthMarketData->BidPrice4
    << " 买四量:" << pDepthMarketData->BidVolume4 
    << " 买五价:" << pDepthMarketData->BidPrice5
    << " 买五量:" << pDepthMarketData->BidVolume5 
	<< ENDLINE << ENDLINE;
    //<<" 持仓量:"<< pDepthMarketData->OpenInterest <<ENDLINE;
#else
    int rtnMarketDataListSize;
    CRtnMarketData rtnMarketData;

	rtnMarketDataListSize = m_pTestApp->m_RtnMarketDataList.size();
	GetLocalTime(&rtnMarketData.CurTime);
	memcpy(&rtnMarketData.DepthMarketData,  pDepthMarketData, sizeof(CThostFtdcDepthMarketDataField));
	pMyTestApp->m_RtnMarketDataList.push_back(rtnMarketData);

#endif
}

bool CTestMdSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{	
  bool ret = ((pRspInfo) && (pRspInfo->ErrorID != 0));
  if (ret){
	string StatusMsg = StatusMessageCodeConvert(pRspInfo->ErrorMsg);
    cerr << m_Prompt <<" 响应 | "<< StatusMsg <<ENDLINE;
  }
  return ret;
}
