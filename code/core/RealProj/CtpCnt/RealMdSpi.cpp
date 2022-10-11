#include "RealMdSpi.h"
#include "RealApp.h"
#include "CtpCntIf.h"

//++
//构造函数
//--
CRealMdSpi::CRealMdSpi(CThostFtdcMdApi* api, CRealApp* pApp, CCtpCntIf* pCtpCntIf)
{
	m_pUserApi = api;
	m_pRealApp = pApp;
	m_pCtpCntIf = pCtpCntIf;

	m_bSuccessLogin = false;
	m_Prompt = m_pRealApp->m_ArchName + "/" + m_pCtpCntIf->m_CounterName + "$";
}

void CRealMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast)
{
	cerr << m_Prompt << "--->>> "<< __FUNCTION__ << ENDLINE;
	IsErrorRspInfo(pRspInfo);
}

void CRealMdSpi::OnFrontDisconnected(int nReason)
{
	cerr << m_Prompt << "--->>> " << __FUNCTION__ << ENDLINE;
	cerr << m_Prompt <<" 响应 | 行情前置连接中断..." 
		<< " reason=" << nReason << ENDLINE;

	Beep(2000,500);  //以2000Hz的频率，蜂鸣500毫秒，当做报警信号
}
		
void CRealMdSpi::OnHeartBeatWarning(int nTimeLapse)
{
	cerr << m_Prompt << "--->>> " << __FUNCTION__ << ENDLINE;
	cerr << m_Prompt <<" 响应 | 行情前置心跳超时警告..." 
		<< " TimerLapse = " << nTimeLapse << ENDLINE;
}

void CRealMdSpi::OnFrontConnected()
{
	cerr << m_Prompt <<" 连接行情前置...成功"<<ENDLINE;
	ReqUserLogin(m_pCtpCntIf->m_brokerId, m_pCtpCntIf->m_userId, m_pCtpCntIf->m_Password);
	SleepMs(1000);
}

int CRealMdSpi::ReqUserLogin(TThostFtdcBrokerIDType	brokerId,
	        TThostFtdcUserIDType	userId,	TThostFtdcPasswordType	passwd)
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, brokerId);
	strcpy(req.UserID, userId);
	strcpy(req.Password, passwd);
	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqUserLogin(&req, m_pCtpCntIf->m_requestId);
    cerr << m_Prompt <<" 请求 | 发送行情前置登录请求..."<<((ret == 0) ? "成功" :"失败") << ENDLINE;	

	return ret;
}

///登出请求，根据响应，CTP不支持行情前置logout功能
int CRealMdSpi::ReqUserLogout(TThostFtdcBrokerIDType brokerId, TThostFtdcUserIDType	userId)
{
	CThostFtdcUserLogoutField UserLogout;
	memset(&UserLogout, 0, sizeof(CThostFtdcUserLogoutField));
	strcpy(UserLogout.BrokerID, brokerId); 
	strcpy(UserLogout.UserID, userId);  
	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqUserLogout(&UserLogout, m_pCtpCntIf->m_requestId); 
    cerr << m_Prompt <<" 请求 | 发送行情前置登出请求..."<<((ret == 0) ? "成功" :"失败") << ENDLINE;	

	return ret;
}

void CRealMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pRspUserLogin)
	{
        cerr << m_Prompt <<" 响应 | 登录行情前置成功...当前交易日:"
          <<pRspUserLogin->TradingDay<<ENDLINE;

		m_bSuccessLogin = true;
	}

	//如果确定是断线重连，则需要重新订阅行情
	if(m_pRealApp->m_bInitDone == true)
	{
		m_pRealApp->m_pMarketDataManage->SubscribeAllMarketData(m_pCtpCntIf->m_CntCode);
	}

	SetEvent(m_pCtpCntIf->m_hMdSpiEvent);
}

///登出请求响应
void CRealMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if ( !IsErrorRspInfo(pRspInfo) && pUserLogout ) {  
		// 保存会话参数	
		cerr << m_Prompt << " 响应 | 登出行情前置成功..."
			<< " 经纪商代码: " << pUserLogout->BrokerID 
			<< " 客户代码: " << pUserLogout->UserID << ENDLINE;
	}
}

int CRealMdSpi::SubscribeMarketData(vector<string> strInstList)
{
	size_t len = strInstList.size();
	TThostFtdcInstrumentIDType* ppInstrumentId = new TThostFtdcInstrumentIDType[len];
    char** pInstId = new char* [len]; 

	for(size_t i = 0; i < len; i++)  
	{
		strcpy(ppInstrumentId[i], strInstList[i].c_str());
		pInstId[i] = ppInstrumentId[i];  
	}

	int ret=m_pUserApi->SubscribeMarketData(pInstId, len);
	cerr << m_Prompt <<" 请求 | 发送行情订阅... "<<((ret == 0) ? "成功" : "失败")<< ENDLINE;

	delete[] ppInstrumentId;
	delete[] pInstId;

	return ret;
}

int CRealMdSpi::UnSubscribeMarketData(vector<string> strInstList)
{
	size_t len = strInstList.size();
	TThostFtdcInstrumentIDType* ppInstrumentId = new TThostFtdcInstrumentIDType[len];
    char** pInstId = new char* [len]; 

	for(size_t i = 0; i < len; i++)  
	{
		strcpy(ppInstrumentId[i], strInstList[i].c_str());
		pInstId[i] = ppInstrumentId[i];  
	}

	int ret=m_pUserApi->UnSubscribeMarketData(pInstId, len);
	cerr << m_Prompt <<" 请求 | 发送行情取消订阅... "<<((ret == 0) ? "成功" : "失败")<< ENDLINE;

	delete[] ppInstrumentId;
	delete[] pInstId;

	return ret;
}

void CRealMdSpi::OnRspSubMarketData(
         CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_pRealApp->m_bManualTrade == true && !IsErrorRspInfo(pRspInfo))
		cerr << m_Prompt << " 响应 |" << pSpecificInstrument->InstrumentID << "  行情订阅...成功"<<ENDLINE;
	else if(bIsLast && IsErrorRspInfo(pRspInfo))
		cerr << m_Prompt << " 响应 |" << "  行情订阅...结束, 出现错误" << ENDLINE;
}

void CRealMdSpi::OnRspUnSubMarketData(
             CThostFtdcSpecificInstrumentField *pSpecificInstrument,
             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_pRealApp->m_bManualTrade == true)
		cerr << m_Prompt <<" 响应 |" << pSpecificInstrument->InstrumentID <<"   行情取消订阅...成功"<<ENDLINE;
	else if(bIsLast== true && m_pRealApp->m_bMaintainDb == false)
		cerr << m_Prompt <<" 响应 |" << "   行情取消订阅...结束"<<ENDLINE;
}

void CRealMdSpi::OnRtnDepthMarketData(
             CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	CThostFtdcDepthMarketDataField DepthMarketData;

	memcpy(&DepthMarketData,  pDepthMarketData, sizeof(CThostFtdcDepthMarketDataField));

	//使用互斥信号保护市场行情列表
	EnterCriticalSection(&m_pCtpCntIf->m_DepthMarketDataCS);

	m_DepthMarketDataList.push_back(DepthMarketData);
	//只要有市场行情更新就触发事件，注意在其他函数设置此消息可能引发程序混乱
	//将信号设置和复位也用互斥区保护
	SetEvent(m_pCtpCntIf->m_hMdSpiEvent); 

	LeaveCriticalSection(&m_pCtpCntIf->m_DepthMarketDataCS);
}

bool CRealMdSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{	
  bool ret = ((pRspInfo) && (pRspInfo->ErrorID != 0));
  if (ret){
	string StatusMsg = StatusMessageCodeConvert(pRspInfo->ErrorMsg);
    cerr << m_Prompt <<" 响应 | " << StatusMsg << ENDLINE;
  }
  return ret;
}
