#include "RealTraderSpi.h"
#include "RealApp.h"
#include "CtpCntIf.h"

//UserID 和InversterID的区别:
//期货公司交易员为投资者下单时，UserID 为操作员代码，InversterID 为投资者代码
//投资者自己下单时，两者同为投资者代码

//++
//构造函数
//--
CRealTraderSpi::CRealTraderSpi(CThostFtdcTraderApi* api, CRealApp* pApp, CCtpCntIf* pCtpCntIf)
{
	m_pUserApi = api;
	m_pRealApp = pApp;
	m_pCtpCntIf = pCtpCntIf;

	m_bSuccessLogin = false;
	m_Prompt = m_pRealApp->m_ArchName + "/" + m_pCtpCntIf->m_CounterName + "$";

	//初始化费率参数
	memset(&m_InstrumentMarginRate, 0, sizeof(CThostFtdcInstrumentMarginRateField));
	memset(&m_InstrumentCommissionRate, 0, sizeof(CThostFtdcInstrumentCommissionRateField));
	memset(&m_OptionInstrCommRate, 0, sizeof(CThostFtdcOptionInstrCommRateField));

	//初始化时间戳
	memset(&m_ReqAccountTimestamp, 0, sizeof(SYSTEMTIME));
	memset(&m_ReqPositionTimestamp, 0, sizeof(SYSTEMTIME));

	//初始化备兑证券锁定/解锁事件对象
#ifndef FUTURE_OPTION_ONLY
#ifdef WIN_CTP
	m_hLockEvent = CreateEvent(NULL, true, false, NULL); 
#else
    m_hLockEvent = new CMyEvent;
#endif
	memset(&m_LockPosition, 0, sizeof(CThostFtdcLockPositionField));
#endif

	//初始化报单操作引用(主要用于撤单)
	m_OrderActionRef = 1;

	m_bOrderInsertError = false;
	m_bOrderActionError = false;
	m_bLockInsertError = false;
}

//交易前置连接成功
void CRealTraderSpi::OnFrontConnected()
{
	cerr << m_Prompt <<" 连接交易前置...成功"<<ENDLINE;
	SleepMs(1000);

	//连接成功后即进行客户端认证
	if(m_pCtpCntIf->m_bNeedAuth == true)
	{
		TThostFtdcProductInfoType UserProductInfo;
#ifdef SEE_THROUGH_SUPERVISION
		TThostFtdcAppIDType	AppID;
#endif
		TThostFtdcAuthCodeType AuthCode;

		strcpy(UserProductInfo, m_pCtpCntIf->m_UserProductInfo.c_str());
#ifdef SEE_THROUGH_SUPERVISION
		strcpy(AppID, m_pCtpCntIf->m_AppID.c_str());
#endif
		strcpy(AuthCode, m_pCtpCntIf->m_AuthCode.c_str());

#ifdef SEE_THROUGH_SUPERVISION
		ReqAuthenticate(m_pCtpCntIf->m_brokerId, m_pCtpCntIf->m_userId, UserProductInfo, AppID, AuthCode);
#else
		ReqAuthenticate(m_pCtpCntIf->m_brokerId, m_pCtpCntIf->m_userId, UserProductInfo, AuthCode);
#endif
	}
	else    //不需要认证
	{
	    ReqUserLogin(m_pCtpCntIf->m_brokerId, m_pCtpCntIf->m_userId, m_pCtpCntIf->m_Password);
	}
}

//用户登录
int CRealTraderSpi::ReqUserLogin(TThostFtdcBrokerIDType	vBrokerId,
	        TThostFtdcUserIDType	vUserId,	TThostFtdcPasswordType	vPasswd)
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, vBrokerId); 
	strcpy(req.UserID, vUserId);     
	strcpy(req.Password, vPasswd);
	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqUserLogin(&req, m_pCtpCntIf->m_requestId);
    cerr << m_Prompt <<" 请求 | 发送交易前置登录请求..."<<((ret == 0) ? "成功" :"失败") << ENDLINE;	

	return ret;
}

///客户端认证请求
#ifdef SEE_THROUGH_SUPERVISION
int CRealTraderSpi::ReqAuthenticate(TThostFtdcBrokerIDType	BrokerId, TThostFtdcUserIDType	UserId, TThostFtdcProductInfoType	UserProductInfo,
		TThostFtdcAppIDType	AppID, TThostFtdcAuthCodeType	AuthCode)
#else
int CRealTraderSpi::ReqAuthenticate(TThostFtdcBrokerIDType	BrokerId, TThostFtdcUserIDType	UserId, TThostFtdcProductInfoType	UserProductInfo,
		TThostFtdcAuthCodeType	AuthCode)
#endif
{
	CThostFtdcReqAuthenticateField ReqAuthenticateField;
	memset(&ReqAuthenticateField, 0, sizeof(ReqAuthenticateField));

	strcpy(ReqAuthenticateField.BrokerID, BrokerId);  
	strcpy(ReqAuthenticateField.UserID, UserId);
#ifdef SEE_THROUGH_SUPERVISION
	strcpy(ReqAuthenticateField.AppID, AppID);
#else
	strcpy(ReqAuthenticateField.UserProductInfo, UserProductInfo);
#endif
	strcpy(ReqAuthenticateField.AuthCode, AuthCode);

	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqAuthenticate(&ReqAuthenticateField, m_pCtpCntIf->m_requestId);
    cerr << m_Prompt <<" 请求 | 发送客户端认证请求..." << ((ret == 0) ? "成功" :"失败") << ENDLINE;	

	return ret;
}

///客户端认证响应
void CRealTraderSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if ( !IsErrorRspInfo(pRspInfo) && pRspAuthenticateField ) {  
		cerr << m_Prompt << " 响应 | 客户端认证请求...成功:" << ENDLINE
			 << " 经纪公司代码: " << pRspAuthenticateField->BrokerID << ENDLINE
			 << " 用户代码: " << pRspAuthenticateField->UserID << ENDLINE
#ifdef SEE_THROUGH_SUPERVISION
		     << " AppID: " << pRspAuthenticateField->AppID << ENDLINE;
#else
			 << " 用户端产品信息: " << pRspAuthenticateField->UserProductInfo << ENDLINE;
#endif

		//登录交易前置
		if(bIsLast == true && m_pCtpCntIf->m_bNeedAuth == true)
		{
			ReqUserLogin(m_pCtpCntIf->m_brokerId, m_pCtpCntIf->m_userId, m_pCtpCntIf->m_Password);
		}
	}
}

///登出请求
int CRealTraderSpi::ReqUserLogout(TThostFtdcBrokerIDType brokerId, TThostFtdcUserIDType	userId)
{
	CThostFtdcUserLogoutField UserLogout;

	memset(&UserLogout, 0, sizeof(CThostFtdcUserLogoutField));
	strcpy(UserLogout.BrokerID, brokerId); 
	strcpy(UserLogout.UserID, userId);  
	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqUserLogout(&UserLogout, m_pCtpCntIf->m_requestId); 
    cerr << m_Prompt <<" 请求 | 发送交易前置登出请求..."<<((ret == 0) ? "成功" :"失败") << ENDLINE;	

	return ret;
}

void CRealTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << m_Prompt << "--->>> " << __FUNCTION__ << ENDLINE;

	if ( !IsErrorRspInfo(pRspInfo) && pRspUserLogin ) {  
		// 保存会话参数	
		m_frontId = pRspUserLogin->FrontID;
		m_sessionId = pRspUserLogin->SessionID;
		int nextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
		sprintf(m_orderRef, "%d", ++nextOrderRef);
		cerr << m_Prompt <<" 响应 | 登录交易前置成功...当前交易日:"
		<<pRspUserLogin->TradingDay<<ENDLINE; 

		if(bIsLast)
		{
			m_bSuccessLogin = true;
			SleepMs(1000);
			ReqSettlementInfoConfirm();
		}
	}
	else if(IsErrorRspInfo(pRspInfo))
    {
		string ErrorMsg = StatusMessageCodeConvert(pRspInfo->ErrorMsg);
		cerr << m_Prompt << "--->>> 交易登录错误: " << pRspInfo->ErrorID << ErrorMsg << ENDLINE;
    }
}

///登出请求响应
void CRealTraderSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if ( !IsErrorRspInfo(pRspInfo) && pUserLogout ) {  
		// 保存会话参数	
		cerr << m_Prompt << " 响应 | 登出交易前置成功..."
			<< " 经纪商代码: " << pUserLogout->BrokerID 
			<< " 客户代码: " << pUserLogout->UserID << ENDLINE;
	}
}

int CRealTraderSpi::ReqSettlementInfoConfirm()
{
	CThostFtdcSettlementInfoConfirmField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_pCtpCntIf->m_brokerId);
	strcpy(req.InvestorID, m_pCtpCntIf->m_userId);
	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqSettlementInfoConfirm(&req, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt <<" 请求 | 发送结算单确认..."<<((ret == 0)?"成功":"失败")<<ENDLINE;

	return ret;
}

void CRealTraderSpi::OnRspSettlementInfoConfirm(
        CThostFtdcSettlementInfoConfirmField  *pSettlementInfoConfirm, 
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{	
	if( !IsErrorRspInfo(pRspInfo) && pSettlementInfoConfirm){
    cerr << m_Prompt <<" 响应 | 结算单..."<<pSettlementInfoConfirm->InvestorID
      <<"...<"<<pSettlementInfoConfirm->ConfirmDate
      <<" "<<pSettlementInfoConfirm->ConfirmTime<<">...确认"<<ENDLINE;
  }
  if(bIsLast) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);
}

int CRealTraderSpi::ReqQryInstrument(TThostFtdcInstrumentIDType instId)
//如果要查询豆粕相关的所有期货和期权合同将InstrumentID设置为”m”，如果要查询某个月份的豆粕期权和期货合约可输入”mYYMM”如”m1705”
{
    TThostFtdcInstrumentIDType tmpInstId;
	CThostFtdcQryInstrumentField req;

	memset(&req, 0, sizeof(req));
	//instId为*表示查询所有合约
	if(strcmp(instId, "*") == 0) 
		memset(tmpInstId, 0, sizeof(TThostFtdcInstrumentIDType));
	else
		strcpy(tmpInstId, instId);
	strcpy(req.InstrumentID, tmpInstId);

	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqQryInstrument(&req, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << " 请求 | 发送合约 " << instId << " 查询..." << ((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

void CRealTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, 
         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	CThostFtdcInstrumentField InstInfo;

	if (!IsErrorRspInfo(pRspInfo) &&  pInstrument)
	{
		memcpy(&InstInfo,  pInstrument, sizeof(CThostFtdcInstrumentField));
		m_InstInfoList.push_back(InstInfo);
	}
	if(bIsLast)
	{
		vector<CThostFtdcInstrumentField> tmpInstInfoList;

		m_pCtpCntIf->m_InstInfoList.swap(m_InstInfoList);
		m_InstInfoList.swap(tmpInstInfoList);

		SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);
	}

}

int CRealTraderSpi::ReqQryTradingAccount()
{
	CThostFtdcQryTradingAccountField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_pCtpCntIf->m_brokerId);
	strcpy(req.InvestorID, m_pCtpCntIf->m_userId);
	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqQryTradingAccount(&req, m_pCtpCntIf->m_requestId);

	//避免交易时大量显示此信息
	if(m_pRealApp->m_bInitDone == false || m_pRealApp->m_bManualTrade == true) 
	{
	    cerr << m_Prompt << " 请求 | 发送资金查询..." << ((ret == 0)?"成功":"失败") << ENDLINE;
	}
	else if(ret != 0)
	{
		//判断当前是否交易时间, 非交易时间不必显示资金查询失败, 否则会使显示信息过于杂乱
		bool bIsTradingTime = IsTradingTime(&m_pRealApp->m_pStrategysManage->m_SynOptionKindList);
		if(bIsTradingTime == true)
		{
			cerr << m_Prompt << " 请求 | 发送资金查询...失败" << " 返回值 ret = " << ret << ENDLINE;
		}
	}

	return ret;
}

void CRealTraderSpi::OnRspQryTradingAccount(
    CThostFtdcTradingAccountField *pTradingAccount, 
   CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) &&  pTradingAccount)
	{
		if(m_pRealApp->m_bInitDone == false || m_pRealApp->m_bManualTrade == true) 
		{
			cerr << m_Prompt << " 响应 | 权益:" << pTradingAccount->Balance
				<< " 可用:" << pTradingAccount->Available   
				<< " 保证金:" << pTradingAccount->CurrMargin
				<< " 平仓盈亏:" << pTradingAccount->CloseProfit
				<< " 持仓盈亏:" << pTradingAccount->PositionProfit
				<< " 手续费:" << pTradingAccount->Commission
				<< " 冻结保证金:" << pTradingAccount->FrozenMargin
				<<" 币种代码:" << pTradingAccount->CurrencyID
				<< ENDLINE;    
		}

		//设置投资者账户资金情况
		if(bIsLast)
		{
			//使用Critical Section保护
			if(m_pRealApp->m_bManualTrade == false) 
			{
				SYSTEMTIME CurTime;
				GetLocalTime(&CurTime);

				EnterCriticalSection(&m_pCtpCntIf->m_TradingAccountCS);
				memcpy(&m_TradingAccount, pTradingAccount, sizeof(CThostFtdcTradingAccountField));
				memcpy(&m_ReqAccountTimestamp, &CurTime, sizeof(SYSTEMTIME));
				LeaveCriticalSection(&m_pCtpCntIf->m_TradingAccountCS);
			}
		}
	}
	if(bIsLast && !m_pRealApp->m_bInitDone) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);
}

int CRealTraderSpi::ReqQryInvestorPosition(TThostFtdcInstrumentIDType instId)
{
	CThostFtdcQryInvestorPositionField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_pCtpCntIf->m_brokerId);
	strcpy(req.InvestorID, m_pCtpCntIf->m_userId);
	strcpy(req.InstrumentID, instId);
	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqQryInvestorPosition(&req, m_pCtpCntIf->m_requestId);

	//避免交易时大量显示此信息
	if(m_pRealApp->m_bInitDone == false || m_pRealApp->m_bManualTrade == true) 
	    cerr << m_Prompt << " 请求 | 发送持仓查询..." << ((ret == 0)?"成功":"失败") << ENDLINE;
	else if(ret != 0)
	{
		//判断当前是否交易时间, 非交易时间不必显示持仓查询失败, 否则会使显示信息过于杂乱
		bool bIsTradingTime = IsTradingTime(&m_pRealApp->m_pStrategysManage->m_SynOptionKindList);
		if(bIsTradingTime == true)
		{
			cerr << m_Prompt << " 请求 | 发送持仓查询...失败" << " 返回值 ret = " << ret << ENDLINE;
		}
	}

	return ret;
}

void CRealTraderSpi::OnRspQryInvestorPosition(
    CThostFtdcInvestorPositionField *pInvestorPosition, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	vector<CThostFtdcInvestorPositionField> tmpInvestorPositionList;
	CThostFtdcInvestorPositionField InvestorPosition;

	tmpInvestorPositionList.clear();
	memset(&InvestorPosition, 0, sizeof(CThostFtdcInvestorPositionField));

	if( !IsErrorRspInfo(pRspInfo) &&  pInvestorPosition )
	{
		//只有手动交易或者初始化时才显示持仓情况
		if(m_pRealApp->m_bInitDone == false || m_pRealApp->m_bManualTrade == true) 
		{
			cerr << m_Prompt <<" 响应 | 合约:"<<pInvestorPosition->InstrumentID
				<<" 方向:"<<MapDirection(pInvestorPosition->PosiDirection-2,false)
				<<" 总持仓:"<<pInvestorPosition->Position
				<<" 昨仓:"<<pInvestorPosition->YdPosition 
				<<" 今仓:"<<pInvestorPosition->TodayPosition
				<<" 持仓盈亏:"<<pInvestorPosition->PositionProfit
				<<" 保证金:"<<pInvestorPosition->UseMargin<<ENDLINE;
		}

		memcpy(&InvestorPosition, pInvestorPosition, sizeof(CThostFtdcInvestorPositionField));
		m_CollectInvestorPositionList.push_back(InvestorPosition);

		if(bIsLast) 
		{
			//采用swap方法赋值并释放List所占空间
			//使用Critical Section保护
			if(m_pRealApp->m_bManualTrade == false) 
			{
				SYSTEMTIME CurTime;
				GetLocalTime(&CurTime);

				EnterCriticalSection(&m_pCtpCntIf->m_InvestorPositionCS);
				m_InvestorPositionList.swap(m_CollectInvestorPositionList);
				memcpy(&m_ReqPositionTimestamp, &CurTime, sizeof(SYSTEMTIME));
				LeaveCriticalSection(&m_pCtpCntIf->m_InvestorPositionCS);
				m_CollectInvestorPositionList.swap(tmpInvestorPositionList);		
			}
		}
	}
	if(bIsLast && !m_pRealApp->m_bInitDone) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

///请求查询合约保证金率
int CRealTraderSpi::ReqQryInstrumentMarginRate(TThostFtdcInstrumentIDType	InstrumentID)
{
	CThostFtdcQryInstrumentMarginRateField req;
	memset(&req, 0, sizeof(req));

	///经纪公司代码
	strcpy_s(req.BrokerID, m_pCtpCntIf->m_brokerId);
	///投资者代码
	strcpy_s(req.InvestorID, m_pCtpCntIf->m_userId);
	///合约代码
	strcpy_s(req.InstrumentID, InstrumentID);
	//////投机套保标志
	req.HedgeFlag = THOST_FTDC_HF_Speculation;	//投机

	m_pCtpCntIf->m_requestId++;
	int iResult = m_pUserApi->ReqQryInstrumentMarginRate(&req, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << "--->>> 查询合约 " << InstrumentID << " 保证金率: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

///请求查询合约保证金率响应
void CRealTraderSpi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, 
												CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//cerr << "--->>> " << __FUNCTION__ << ENDLINE;
	if (!IsErrorRspInfo(pRspInfo) &&  pInstrumentMarginRate)
	{
		memcpy(&m_InstrumentMarginRate,  pInstrumentMarginRate, sizeof(CThostFtdcInstrumentMarginRateField));
	}

    //连续查询可能导致未处理请求超过许可数
	//SleepMs(1000);
	if(bIsLast) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

///请求查询合约手续费率
int CRealTraderSpi::ReqQryInstrumentCommissionRate(TThostFtdcInstrumentIDType	InstrumentID)
{
	CThostFtdcQryInstrumentCommissionRateField req;
	memset(&req, 0, sizeof(req));

	///经纪公司代码
	strcpy_s(req.BrokerID, m_pCtpCntIf->m_brokerId);
	///投资者代码
	strcpy_s(req.InvestorID, m_pCtpCntIf->m_userId);
	///合约代码
	strcpy_s(req.InstrumentID, InstrumentID);

	m_pCtpCntIf->m_requestId++;
	int iResult = m_pUserApi->ReqQryInstrumentCommissionRate(&req, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << "--->>> 查询合约 " << InstrumentID << " 手续费率: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

///请求查询合约手续费率响应
void CRealTraderSpi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, 
													CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//cerr << "--->>> " << __FUNCTION__ << ENDLINE;
	if (!IsErrorRspInfo(pRspInfo) &&  pInstrumentCommissionRate)
	{
		memcpy(&m_InstrumentCommissionRate, pInstrumentCommissionRate, sizeof(CThostFtdcInstrumentCommissionRateField));
	}

	if(bIsLast) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

///请求查询期权合约手续费
int CRealTraderSpi::ReqQryOptionInstrCommRate(TThostFtdcInstrumentIDType  InstrumentID)
{
	CThostFtdcQryOptionInstrCommRateField req;
	memset(&req, 0, sizeof(req));

	///经纪公司代码
	strcpy_s(req.BrokerID, m_pCtpCntIf->m_brokerId);
	///投资者代码
	strcpy_s(req.InvestorID, m_pCtpCntIf->m_userId);
	///合约代码
	strcpy_s(req.InstrumentID, InstrumentID);

	m_pCtpCntIf->m_requestId++;
	int iResult = m_pUserApi->ReqQryOptionInstrCommRate(&req, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << "--->>> 查询期权合约 " << InstrumentID << " 手续费率: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

///请求查询期权合约手续费响应
void CRealTraderSpi::OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//cerr << "--->>> " << __FUNCTION__ << ENDLINE;
	if (!IsErrorRspInfo(pRspInfo) &&  pOptionInstrCommRate)
	{
		memcpy(&m_OptionInstrCommRate, pOptionInstrCommRate, sizeof(CThostFtdcOptionInstrCommRateField));
	}

	if(bIsLast) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

///请求查询经纪公司交易参数，计算期权保证金时有用，CurrenyID可以在OnRspTradingAccount中获得, 人民币"CNY"
int CRealTraderSpi::ReqQryBrokerTradingParams()
{
	CThostFtdcQryBrokerTradingParamsField req;
	memset(&req, 0, sizeof(req));

	///经纪公司代码
	strcpy_s(req.BrokerID, m_pCtpCntIf->m_brokerId);
	///投资者代码
	strcpy_s(req.InvestorID, m_pCtpCntIf->m_userId);
	///币种代码
	strcpy_s(req.CurrencyID, "CNY");

	m_pCtpCntIf->m_requestId++;
	int iResult = m_pUserApi->ReqQryBrokerTradingParams(&req, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << "--->>> 查询经纪公司交易参数: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

///请求查询经纪公司交易参数响应
void CRealTraderSpi::OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//cerr << "--->>> " << __FUNCTION__ << ENDLINE;
	if (!IsErrorRspInfo(pRspInfo) &&  pBrokerTradingParams)
	{
		memcpy(&m_BrokerTradingParams,  pBrokerTradingParams, sizeof(CThostFtdcBrokerTradingParamsField));
	}

	if(bIsLast) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

///请求查询期权交易成本
int CRealTraderSpi::ReqQryOptionInstrTradeCost(	TThostFtdcInstrumentIDType	InstrumentID, TThostFtdcPriceType	InputPrice, TThostFtdcPriceType	UnderlyingPrice)
/*++
参数
    InstrumentID：合约代码
	InputPrice：期权和玉报价
	UnderlyingPrice：标的价格,填0则用昨结算价
--*/
{
	CThostFtdcQryOptionInstrTradeCostField req;
	memset(&req, 0, sizeof(req));

	///经纪公司代码
	strcpy_s(req.BrokerID, m_pCtpCntIf->m_brokerId);
	///投资者代码
	strcpy_s(req.InvestorID, m_pCtpCntIf->m_userId);
	///合约代码
	strcpy_s(req.InstrumentID, InstrumentID);
	//////投机套保标志
	req.HedgeFlag = THOST_FTDC_HF_Speculation;	//投机
	req.InputPrice = InputPrice;
	req.UnderlyingPrice = UnderlyingPrice;

	m_pCtpCntIf->m_requestId++;
	int iResult = m_pUserApi->ReqQryOptionInstrTradeCost(&req, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << "--->>> 查询期权交易成本: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

///请求查询期权交易成本响应
void CRealTraderSpi::OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << m_Prompt << "--->>> " << __FUNCTION__ << ENDLINE;
	if (!IsErrorRspInfo(pRspInfo) &&  pOptionInstrTradeCost)
	{
		cerr << m_Prompt << "--->>> " << "期权交易成本:" << ENDLINE;
		cerr << "期权合约保证金不变部分: FixedMargin = " << pOptionInstrTradeCost->FixedMargin << ENDLINE;  
		cerr << "期权合约最小保证金: MiniMargin = " << pOptionInstrTradeCost->MiniMargin << ENDLINE;
		cerr << "期权合约权利金: Royalty = " << pOptionInstrTradeCost->Royalty << ENDLINE;
		cerr << "交易所期权合约保证金不变部分: ExchFixedMargin = " << pOptionInstrTradeCost->ExchFixedMargin << ENDLINE;
		cerr << "交易所期权合约最小保证金: ExchMiniMargin = " << pOptionInstrTradeCost->ExchMiniMargin << ENDLINE; //成交价或者结算价
	}

	if(bIsLast) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

///报单录入请求
int CRealTraderSpi::ReqOrderInsert(TThostFtdcInstrumentIDType instId,
    TThostFtdcDirectionType dir, TThostFtdcCombOffsetFlagType kpp,
    TThostFtdcPriceType price,   TThostFtdcVolumeType vol, 
	char TimeCondition, char VolumeCondition, char MarketOrderType, 
	TThostFtdcHedgeFlagType HedgeFlag, string strExchangeID)
{
	double eps = 0.0000001;

	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));	
	strcpy(req.BrokerID, m_pCtpCntIf->m_brokerId);  //应用单元代码	
	strcpy(req.InvestorID, m_pCtpCntIf->m_userId); //投资者代码	
	strcpy(req.InstrumentID, instId); //合约代码	
	strcpy(req.OrderRef, m_orderRef);  //报单引用
	int nextOrderRef = atoi(m_orderRef);
	sprintf(m_orderRef, "%d", ++nextOrderRef);
	strcpy(req.UserID, m_pCtpCntIf->m_userId);  //用户代码
	if(strExchangeID != "") strcpy(req.ExchangeID, strExchangeID.c_str());
  
	req.LimitPrice = price;	//价格
	if(abs(req.LimitPrice) < eps){
		if(MarketOrderType == 'A' || MarketOrderType == 'a')
			req.OrderPriceType = THOST_FTDC_OPT_AnyPrice;         //价格类型=市价, 商品交易所使用
		else if(MarketOrderType == 'B' || MarketOrderType == 'b')
			req.OrderPriceType = THOST_FTDC_OPT_BestPrice;        //市价: 证券交易所股票期权使用
		else if(MarketOrderType == 'F' || MarketOrderType == 'f') //证券市价单使用五档价
			req.OrderPriceType = THOST_FTDC_OPT_FiveLevelPrice;
		else
			req.OrderPriceType = MarketOrderType;
	}
	else
	{
		req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;//价格类型=限价	
	}

	if(TimeCondition == 'I' || TimeCondition == 'i')
		req.TimeCondition = THOST_FTDC_TC_IOC;//有效期类型:立即完成，否则撤销
	else if(TimeCondition == 'G' || TimeCondition == 'g')
		req.TimeCondition = THOST_FTDC_TC_GFD;  //有效期类型:当日有效
	else
		req.TimeCondition = TimeCondition;

	req.Direction = MapDirection(dir,true);  //买卖方向	
	req.CombOffsetFlag[0] = MapOffset(kpp[0],true); //组合开平标志:开仓
	req.CombHedgeFlag[0] = HedgeFlag;	  //组合投机套保标志	
	req.VolumeTotalOriginal = vol;	///数量		

	if(VolumeCondition == 'C' || VolumeCondition == 'c')
		req.VolumeCondition = THOST_FTDC_VC_CV; //成交量类型:全部数量, 适用于股票期权
	else if(VolumeCondition == 'A' || VolumeCondition == 'a')
	    req.VolumeCondition = THOST_FTDC_VC_AV; //成交量类型:任何数量, 适用于期货期权
	else
		req.VolumeCondition = VolumeCondition;

	req.MinVolume = 1;	//最小成交量:1	
	req.ContingentCondition = THOST_FTDC_CC_Immediately;  //触发条件:立即
	
	//TThostFtdcPriceType	StopPrice;  //止损价
	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;	//强平原因:非强平	
	req.IsAutoSuspend = 0;  //自动挂起标志:否	
	req.UserForceClose = 0;   //用户强评标志:否

	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqOrderInsert(&req, m_pCtpCntIf->m_requestId);

	//只有手动报单才需要此处回报
	if(m_pRealApp->m_bManualTrade == true) 
	{
		string strOffset;
		if(kpp[0] == 'o' || kpp[0] == 'O' || kpp[0] == '0')
			strOffset = "开仓";
		else if(kpp[0] == 'c' || kpp[0] == 'C' || kpp[0] == '1')
			strOffset = "平仓";
		else if(kpp[0] == 'j' || kpp[0] == 'J' || kpp[0] == '2')
			strOffset = "平今";
		else 
			strOffset = "错误操作";

		cerr << m_Prompt << " 请求 | 报单, " << ((ret == 0)?"成功！":"失败！") << " 合约:" << instId << " " << strOffset << ENDLINE;
	}

	//为交易所规则过滤器增加报单数目
	for(size_t i = 0; i < m_pRealApp->m_pExchangeRuleFilterList.size(); i++)
	{
		CExchangeRuleFilter *pExchangeRuleFilter = m_pRealApp->m_pExchangeRuleFilterList[i];
		pExchangeRuleFilter->IncreaseInsertOrderNum(m_pCtpCntIf->m_CntCode, strExchangeID);
	}

	return ret;
}

void CRealTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, 
          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
/*++
  Thost 收到报单指令，如果没有通过参数校验，拒绝接受报单指令。用户就会收到
  OnRspOrderInsert消息，其中包含了错误编码和错误消息。
  如果Thost接受了报单指令，用户不会收到OnRspOrderInsert，而会收到OnRtnOrder，
  用来更新委托状态。
  交易所收到报单后，通过校验。用户会收到OnRtnOrder、OnRtnTrade。
  如果交易所认为报单错误，用户就会收到OnErrRtnOrder，但这个OnErrRtnOrder发到CTP后，触发OnRtnOrder，所以作为投资者可以不关心OnErrRtnOrde
--*/
{
	cerr << m_Prompt << "--->>> " << __FUNCTION__ << ENDLINE;
	bool bErrorRsp = IsErrorRspInfo(pRspInfo);
	if( !bErrorRsp && pInputOrder ){
		cerr << m_Prompt <<"响应 | 报单提交响应...报单引用:"<<pInputOrder->OrderRef<<ENDLINE;  
	}
	else if(bErrorRsp) {
		m_bOrderInsertError = true; 
	}
	if(bIsLast && m_pRealApp->m_bInitDone) 
		SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

//撤单请求, 使用对应报单字段来填充撤单字段
//完全版的撤单请求, 使用FrontID+SessionID+OrderRef/ExchangeID+OrderSysID 撤单
//报单在Thost使用FrontID+SessionID+OrderRef撤单
//报单在交易所使用ExchangeID+OrderSysID撤单, 参见综合交易平台API特别说明
int CRealTraderSpi::ReqOrderAction(CThostFtdcOrderField *pOrder, int FrontId, int SessionId, int OrderRef, string strInstId, string strExchangeID)
{
	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	req.OrderActionRef = m_OrderActionRef++;
	req.ActionFlag = THOST_FTDC_AF_Delete;       //操作标志 

	if(pOrder != NULL)
	{
		strcpy(req.BrokerID, pOrder->BrokerID);
		strcpy(req.InvestorID, pOrder->InvestorID);
		strcpy(req.OrderRef, pOrder->OrderRef);
		req.FrontID = pOrder->FrontID;
		req.SessionID = pOrder->SessionID;
		strcpy(req.ExchangeID, pOrder->ExchangeID);
		strcpy(req.OrderSysID, pOrder->OrderSysID);
		req.LimitPrice = pOrder->LimitPrice;
		strcpy(req.UserID, pOrder->UserID);
		strcpy(req.InstrumentID, pOrder->InstrumentID);
		strcpy(req.InvestUnitID, pOrder->InvestUnitID);
	}
	else
	{
		strcpy(req.BrokerID, m_pCtpCntIf->m_brokerId);  //经纪公司代码	
		strcpy(req.InvestorID, m_pCtpCntIf->m_userId);  //投资者代码
		strcpy(req.UserID, m_pCtpCntIf->m_userId);  //用户代码
		strcpy(req.InstrumentID, strInstId.c_str());

		req.FrontID = FrontId;           //前置编号	
		req.SessionID = SessionId;       //会话编号

		TThostFtdcOrderRefType CtpOrderRef;
		sprintf(CtpOrderRef, "%d", OrderRef);
		strcpy(req.OrderRef, CtpOrderRef); //报单引用	
	}

	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqOrderAction(&req, m_pCtpCntIf->m_requestId);

	//为交易所规则过滤器增加撤单数目
	for(size_t i = 0; i < m_pRealApp->m_pExchangeRuleFilterList.size(); i++)
	{
		CExchangeRuleFilter *pExchangeRuleFilter = m_pRealApp->m_pExchangeRuleFilterList[i];
		if(pOrder != NULL) strExchangeID = pOrder->ExchangeID;
		pExchangeRuleFilter->IncreaseCancelOrderNum(m_pCtpCntIf->m_CntCode, strExchangeID);
	}

	return ret;
}

void CRealTraderSpi::OnRspOrderAction(
      CThostFtdcInputOrderActionField *pInputOrderAction, 
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	bool bErrorRsp = IsErrorRspInfo(pRspInfo);
	if (!bErrorRsp && pInputOrderAction){
		cerr << m_Prompt << " 响应 | 撤单成功..."
			<< "交易所:"<<pInputOrderAction->ExchangeID
			<<" 交易所报单编号:"<<pInputOrderAction->OrderSysID<<ENDLINE;
	}
	else if(bErrorRsp) {
		m_bOrderActionError = true;
	}

	if(bIsLast  && m_pRealApp->m_bInitDone) 
		SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

int CRealTraderSpi::ReqQryOrder(CThostFtdcOrderField *pOrder)
{
	CThostFtdcQryOrderField qryOrder;
	memset(&qryOrder, 0, sizeof(CThostFtdcQryOrderField));

	strcpy(qryOrder.BrokerID, pOrder->BrokerID);
	strcpy(qryOrder.InvestorID, pOrder->InvestorID);
	strcpy(qryOrder.InstrumentID, pOrder->InstrumentID);
	strcpy(qryOrder.ExchangeID, pOrder->ExchangeID);
	strcpy(qryOrder.OrderSysID, pOrder->OrderSysID);
	//如果报单编号为空, 在无时间限制的情况下, 将查出所有该合约代码的报单状态
	//strcpy(qryOrder.InsertTimeStart, pOrder->InsertTime);
	//strcpy(qryOrder.InsertTimeEnd, pOrder->InsertTime);

	m_pCtpCntIf->m_requestId++;
    int ret = m_pUserApi->ReqQryOrder(&qryOrder, m_pCtpCntIf->m_requestId);

	return ret;
}

void CRealTraderSpi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pOrder){
		if(m_pRealApp->m_bManualTrade == true) 
		{
			cerr << m_Prompt << " 响应 | 报单查询成功..."
				<< "交易所代号:"<< pOrder->ExchangeID
				<< " 交易所报单编号:" << pOrder->OrderSysID 
				<< " 报单状态:" << pOrder->OrderStatus
				<< ENDLINE;
		}
		else  //将报单信息保存入列表
		{
			CThostFtdcOrderField order;
			memcpy(&order,  pOrder, sizeof(CThostFtdcOrderField));
			EnterCriticalSection(&m_pCtpCntIf->m_OrderListCS);
			m_orderList.push_back(order);
			LeaveCriticalSection(&m_pCtpCntIf->m_OrderListCS);
		}
	}

	if(bIsLast && m_pRealApp->m_bInitDone) 
		SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

///报单回报
void CRealTraderSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
//OnErrRtnOrde的错误信息通过OnRtnOrder返回，所以需要查询Oder的状态
{
	CThostFtdcOrderField order;
	memcpy(&order,  pOrder, sizeof(CThostFtdcOrderField));

	//加入互斥信号保护m_orderList的修改
	if(m_pRealApp->m_bManualTrade == false) EnterCriticalSection(&m_pCtpCntIf->m_OrderListCS);
	m_orderList.push_back(order);
	if(m_pRealApp->m_bManualTrade == false) LeaveCriticalSection(&m_pCtpCntIf->m_OrderListCS);

	//自动交易程序初始化完成, 需要设置事件信号
	if(m_pRealApp->m_bInitDone == true)
		SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	

	//只在手动交易的时候显示报单回报，否则显示内容过多，同时耗时亦多
	if(m_pRealApp->m_bManualTrade == true) 
	{
		cerr << m_Prompt << " 回报 | 报单提交, 经纪商序号:" << pOrder->BrokerOrderSeq << " 交易所编号:" << pOrder->OrderSysID 
			 << " 合约:" << pOrder->InstrumentID << ENDLINE;
	}
}

///成交通知
void CRealTraderSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	CThostFtdcTradeField trade;
	memcpy(&trade,  pTrade, sizeof(CThostFtdcTradeField));

	//加入互斥信号保护m_tradeList的修改
	if(m_pRealApp->m_bManualTrade == false) EnterCriticalSection(&m_pCtpCntIf->m_TradeListCS);
	m_tradeList.push_back(trade);
	if(m_pRealApp->m_bManualTrade == false) LeaveCriticalSection(&m_pCtpCntIf->m_TradeListCS);

	cerr << m_Prompt << " 回报 | 成交, 编号:" << RemoveSpace(pTrade->TradeID) << " 合约:" << pTrade->InstrumentID 
		<< "  数量:" << pTrade->Volume << "  价格:" << pTrade->Price << ENDLINE;

	//自动交易程序初始化完成, 需要设置事件信号
	if(m_pRealApp->m_bInitDone == true)
	{
		SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	

#ifndef FUTURE_OPTION_ONLY
		//供备兑平仓使用
		SetEvent(m_hLockEvent);	
#endif
	}

	//为交易所规则过滤器增加成交量
	for(size_t i = 0; i < m_pRealApp->m_pExchangeRuleFilterList.size(); i++)
	{
		CExchangeRuleFilter *pExchangeRuleFilter = m_pRealApp->m_pExchangeRuleFilterList[i];
		pExchangeRuleFilter->IncreaseTradeVolume(m_pCtpCntIf->m_CntCode, pTrade);
	}
}

///合约交易状态通知
void CRealTraderSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus) 
{
	//处理机制和处理市场行情推送结果一致
	CThostFtdcInstrumentStatusField InstrumentStatus;
	memcpy(&InstrumentStatus,  pInstrumentStatus, sizeof(CThostFtdcInstrumentStatusField));

	//使用互斥信号保护合约交易状态列表
	EnterCriticalSection(&m_pCtpCntIf->m_InstrumentStatusCS);

	m_InstrumentStatusList.push_back(InstrumentStatus);
	//只要有合约交易状态更新就触发事件，注意在其他函数设置此消息可能引发程序混乱
	//将信号设置和复位也用互斥区保护
	SetEvent(m_pCtpCntIf->m_hInstrumentStatusEvent); 

	LeaveCriticalSection(&m_pCtpCntIf->m_InstrumentStatusCS);
};

///请求查询成交
int CRealTraderSpi::ReqQryTrade(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcTradeIDType TradeID)
{
	CThostFtdcQryTradeField qryTrade;

	memset(&qryTrade, 0, sizeof(CThostFtdcQryTradeField));
	strcpy(qryTrade.BrokerID, m_pCtpCntIf->m_brokerId);
	strcpy(qryTrade.InvestorID, m_pCtpCntIf->m_userId);
	strcpy(qryTrade.InstrumentID, InstrumentID);
	strcpy(qryTrade.ExchangeID, ExchangeID);
	strcpy(qryTrade.TradeID, TradeID);

	m_pCtpCntIf->m_requestId++;
    int ret = m_pUserApi->ReqQryTrade(&qryTrade, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << " 请求 | 成交查询..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

///请求查询成交响应
void CRealTraderSpi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pTrade){
		cerr << m_Prompt << " 响应 | 成交查询成功..."
			<< "交易所代号:"<< pTrade->ExchangeID
			<< " 报单编号:" << pTrade->OrderSysID 
			<< " 成交编号:" << pTrade->TradeID
			<< " 成交价格:" << pTrade->Price
			<< " 成交数量:" << pTrade->Volume
			<< ENDLINE;
	}
	if(bIsLast) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

///请求期权行权
int CRealTraderSpi::ReqExecOrderInsert(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcVolumeType Volume)
{
    CThostFtdcInputExecOrderField req; 
	memset(&req, 0, sizeof(CThostFtdcInputExecOrderField));
    strcpy(req.BrokerID, m_pCtpCntIf->m_brokerId);  
    strcpy(req.InvestorID, m_pCtpCntIf->m_userId);  
	strcpy(req.UserID, m_pCtpCntIf->m_userId);
	strcpy(req.InstrumentID, InstrumentID);

	strcpy(req.ExecOrderRef, m_orderRef);  //报单引用，将行权和普通报单同等对待
	int nextOrderRef = atoi(m_orderRef);
	sprintf(m_orderRef, "%d", ++nextOrderRef);
	//use_CloseToday/_CloseYesterday for SHFE
    req.OffsetFlag = THOST_FTDC_OF_Close;         //必填;    
    req.HedgeFlag = THOST_FTDC_HF_Speculation;    //必填;  
    req.Volume = Volume; //必填   
    req.ActionType = THOST_FTDC_ACTP_Exec; //必填，执行类型 
    req.PosiDirection = THOST_FTDC_PD_Long; //必填, 保留头寸申请的持仓方向, THOST_FTDC_PD_Long('2'), THOST_FTDC_PD_Short('3'), 所有要行权的期权都是多头方向
	TThostFtdcExchangeIDType strExchangeID = "CFFEX";
    if(strcmp(strExchangeID, ExchangeID) == 0)  { 
        req.ReservePositionFlag = THOST_FTDC_EOPF_UnReserve;  //这是中金所的填法，大商所郑商所填THOST_FTDC_EOPF_Reserve  
		req.CloseFlag = THOST_FTDC_EOCF_AutoClose;            //这是中金所的填法，大商所郑商所填THOST_FTDC_EOCF_NotToClose 
	}
	else {
		req.ReservePositionFlag = THOST_FTDC_EOPF_Reserve;
		req.CloseFlag = THOST_FTDC_EOCF_NotToClose;  
	}

	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqExecOrderInsert(&req, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << " 请求 | 发送行权请求..." << ((ret == 0)?"成功":"失败") << " 合约:" << InstrumentID << ENDLINE;

	return ret;
}

///期权行权执行宣告录入请求响应
void CRealTraderSpi::OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
/*++
  如果交易核心认为该行权指令不合法，函数OnRspExecOrderInsert会被调用。如果交易核心认为该行权指令合法，函数OnRtnExecOrder会被调用。
  行权之后，如果收到返回信息“未执行”，则说明该行权指令已经被交易所接收。由于交易所都是在盘后结算时进行行权配对，所以盘中行权返回“未执行”的信息。
--*/
{
	cerr << "--->>> " << __FUNCTION__ << ENDLINE;
	if( !IsErrorRspInfo(pRspInfo) && pInputExecOrder ){
		cerr << m_Prompt << "响应 | 期权行权提交响应...行权报单引用:" << pInputExecOrder->ExecOrderRef << ENDLINE;  
	}
	else if(pInputExecOrder) {
		; //错误的行权报单THOST拒绝接受，用pInputExecOrder和pRspInfo中的信息更新报单状态
	}
	if(bIsLast && m_pRealApp->m_bInitDone) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

///执行宣告通知
void CRealTraderSpi::OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder)
/*++
  如果交易核心认为该行权指令合法，函数OnRtnExecOrder会被调用。
  行权之后，如果收到返回信息“未执行”，则说明该行权指令已经被交易所接收。
  由于交易所都是在盘后结算时进行行权配对，所以盘中行权返回“未执行”的信息。
--*/
{
	CThostFtdcExecOrderField execOrder;
	memcpy(&execOrder,  pExecOrder, sizeof(CThostFtdcExecOrderField));
	bool founded=false;    size_t i=0;
	for(i=0; i<m_execOrderList.size(); i++){
		if(m_execOrderList[i].BrokerExecOrderSeq == pExecOrder->BrokerExecOrderSeq) {
			founded=true;    
			break;
		}
	}
	if(founded)  memcpy(&m_execOrderList[i], pExecOrder, sizeof(CThostFtdcExecOrderField)); 
	else  m_execOrderList.push_back(execOrder);
	cerr << m_Prompt << " 回报 | 期权行权报单已提交...经纪公司报单编号:" << pExecOrder->BrokerExecOrderSeq << ENDLINE;
	cerr << "pExecOrder->OrderSubmitStatus = " << pExecOrder->OrderSubmitStatus << ENDLINE;
	string StatusMsg = StatusMessageCodeConvert(pExecOrder->StatusMsg);
	cerr << "pExecOrder->StatusMsg = " << StatusMsg << ENDLINE;

	if(m_pRealApp->m_bInitDone) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

///执行宣告操作请求/期权行权撤销
int CRealTraderSpi::ReqExecOrderAction(	TThostFtdcSequenceNoType	brokerExecOrderSeq)
{
	bool found=false; size_t i=0;
	for(i=0;i<m_execOrderList.size();i++){
		if(m_execOrderList[i].BrokerExecOrderSeq == brokerExecOrderSeq){ 
			found = true; 
			break;
		}
	}
	if(!found)
	{
		cerr << m_Prompt <<" 请求 | 期权行权报单不存在."<<ENDLINE; 
		return -5;  //自定义的错误编码
	} 

	CThostFtdcInputExecOrderActionField req;
	memset(&req, 0, sizeof(req));
	req.ExecOrderActionRef = m_OrderActionRef++;  //复用报单操作引用
	req.ActionFlag = THOST_FTDC_AF_Delete;  //操作标志 

	CThostFtdcExecOrderField *pExecOrder = &m_execOrderList[i];
	strcpy(req.BrokerID, pExecOrder->BrokerID);   //经纪公司代码	
	strcpy(req.InvestorID, pExecOrder->InvestorID);   //投资者代码
	strcpy(req.ExecOrderRef, pExecOrder->ExecOrderRef);
	req.SessionID = pExecOrder->SessionID;
	req.FrontID = pExecOrder->FrontID;
	strcpy(req.ExchangeID, pExecOrder->ExchangeID);
	strcpy(req.ExecOrderSysID, pExecOrder->ExecOrderSysID);
	strcpy(req.UserID, pExecOrder->UserID);
	strcpy(req.InstrumentID, pExecOrder->InstrumentID);
	strcpy(req.InvestUnitID, pExecOrder->InvestUnitID);

	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqExecOrderAction(&req, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << " 请求 | 发送期权行权撤单..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

///执行宣告操作请求响应/期权行权撤单响应
void CRealTraderSpi::OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pInputExecOrderAction){
		cerr << m_Prompt << " 响应 | 期权行权撤单成功..."
			<< "交易所:"<<pInputExecOrderAction->ExchangeID
			<<" 交易所行权报单编号:"<<pInputExecOrderAction->ExecOrderSysID << ENDLINE;
	}
	if(bIsLast && m_pRealApp->m_bInitDone) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

///请求查询ETF期权合约手续费
#ifndef FUTURE_OPTION_ONLY
int CRealTraderSpi::ReqQryETFOptionInstrCommRate(TThostFtdcInstrumentIDType  InstrumentID)
{
	CThostFtdcQryETFOptionInstrCommRateField req;
	memset(&req, 0, sizeof(req));

	///经纪公司代码
	strcpy_s(req.BrokerID, m_pCtpCntIf->m_brokerId);
	///投资者代码
	strcpy_s(req.InvestorID, m_pCtpCntIf->m_userId);
	///合约代码
	strcpy_s(req.InstrumentID, InstrumentID);

	m_pCtpCntIf->m_requestId++;
	int iResult = m_pUserApi->ReqQryETFOptionInstrCommRate(&req, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << "--->>> 查询ETF期权合约手续费率: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

///请求查询ETF期权合约手续费响应
///卖出ETF期权是不收手续费的
void CRealTraderSpi::OnRspQryETFOptionInstrCommRate(CThostFtdcETFOptionInstrCommRateField *pETFOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//需要读取程序清空此列表
	if (!IsErrorRspInfo(pRspInfo) &&  pETFOptionInstrCommRate)
	{
		m_ETFOptionInstrCommRateList.push_back(*pETFOptionInstrCommRate);
	}

	if(bIsLast) SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);	
}

//++
//锁定/解锁备兑证券请求
//参数
//    InstrumentID: 证券代码
//    ExchangeID: 交易所代码
//    LockType: 操作类型, 锁定/解锁
//    Volume: 操作的证券数量
//返回值
//    0: 无错误正常返回
//    <>0: 出现错误
//--
int CRealTraderSpi::ReqLockInsert(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcLockTypeType LockType, TThostFtdcVolumeType Volume)
{
	CThostFtdcInputLockField InputLock;

	memset(&InputLock, 0, sizeof(InputLock));
	strcpy(InputLock.BrokerID, m_pCtpCntIf->m_brokerId);   //经纪公司代码	
	strcpy(InputLock.InvestorID, m_pCtpCntIf->m_userId);   //投资者代码
	strcpy(InputLock.ExchangeID, ExchangeID);
	strcpy(InputLock.InstrumentID, InstrumentID);
	InputLock.LockType = LockType;
	InputLock.Volume = Volume;
	strcpy(InputLock.LockRef, m_orderRef);                //报单引用，将锁定和普通报单同等对待
	int nextOrderRef = atoi(m_orderRef);
	sprintf(m_orderRef, "%d", ++nextOrderRef);

	m_pCtpCntIf->m_requestId++;
	int ret = m_pUserApi->ReqLockInsert(&InputLock, m_pCtpCntIf->m_requestId);

	if(LockType == THOST_FTDC_LCKT_Lock)
	{
		cerr << m_Prompt << " 请求 | 发送备兑证券锁定..." <<((ret == 0)?"成功":"失败") << ENDLINE;
	}
	else
	{
		cerr << m_Prompt << " 请求 | 发送备兑证券解锁..." <<((ret == 0)?"成功":"失败") << ENDLINE;
	}

	return ret;
}

//锁定应答
//如果报单无错误, 此函数不会被调用
void CRealTraderSpi::OnRspLockInsert(CThostFtdcInputLockField *pInputLock, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << __FUNCTION__ << ENDLINE;
	bool bErrorRsp = IsErrorRspInfo(pRspInfo);
	if( !bErrorRsp && pInputLock ){
		if(pInputLock->LockType == THOST_FTDC_LCKT_Lock)
		{
			cerr << m_Prompt <<"响应 | 锁定证券提交...锁定引用:" << pInputLock->LockRef << ENDLINE;  
		}
		else
		{
			cerr << m_Prompt <<"响应 | 解锁证券提交...锁定引用:" << pInputLock->LockRef << ENDLINE;  
		}
	}
	else if(bErrorRsp) {
		m_bLockInsertError = true;
	}

	if(bIsLast && m_pRealApp->m_bInitDone) 
		SetEvent(m_hLockEvent);	
}

//锁定通知
//报单执行正确将只会调用此函数
void CRealTraderSpi::OnRtnLock(CThostFtdcLockField *pLock)
{
	CThostFtdcLockField Lock;
	memcpy(&Lock,  pLock, sizeof(CThostFtdcLockField));

	//总是存入最新的推送信息
	bool founded = false;    
	size_t i = 0;
	for(i = 0; i < m_lockOrderList.size(); i++)
	{
		if(atoi(m_lockOrderList[i].LockRef) == atoi(pLock->LockRef) && m_lockOrderList[i].FrontID == pLock->FrontID
			&& m_lockOrderList[i].SessionID == pLock->SessionID) 
		{
			founded=true;    
			break;
		}
	}
	if(founded)  memcpy(&m_lockOrderList[i], pLock, sizeof(CThostFtdcLockField)); 
	else  m_lockOrderList.push_back(Lock);

	//只有锁定/解锁指令被接收才设置信号
	if(pLock->LockStatus == THOST_FTDC_OAS_Accepted)
	{
		SetEvent(m_hLockEvent);	
	}
}

//锁定错误通知
//如果报单无错误, 此函数不会被调用
void CRealTraderSpi::OnErrRtnLockInsert(CThostFtdcInputLockField *pInputLock, CThostFtdcRspInfoField *pRspInfo)
{
	if(!IsErrorRspInfo(pRspInfo) && pInputLock )
	{
		;
	}

	m_bLockInsertError = true;
	SetEvent(m_hLockEvent);	
}


//请求查询锁定
//请求查询的响应常常不正确, 此函数可用性差
int CRealTraderSpi::ReqQryLock(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcOrderSysIDType LockSysID)
{
	CThostFtdcQryLockField QryLock;

	memset(&QryLock, 0, sizeof(CThostFtdcQryLockField));
	strcpy(QryLock.BrokerID, m_pCtpCntIf->m_brokerId);
	strcpy(QryLock.InvestorID, m_pCtpCntIf->m_userId);
	strcpy(QryLock.InstrumentID, InstrumentID);
	strcpy(QryLock.ExchangeID, ExchangeID);
	strcpy(QryLock.LockSysID, LockSysID);

	m_pCtpCntIf->m_requestId++;
    int ret = m_pUserApi->ReqQryLock(&QryLock, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << " 请求 | 锁定/解锁报单查询..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

//请求查询锁定应答
//几次锁定解锁之后, 此应答通常不正确
void CRealTraderSpi::OnRspQryLock(CThostFtdcLockField *pLock, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pLock){
		string StatusMsg = StatusMessageCodeConvert(pLock->StatusMsg);
		
		cerr << m_Prompt << " 响应 | 锁定/解锁报单查询成功..." << ENDLINE
	         << " 锁定编号:" << pLock->LockSysID << ENDLINE
		     << " 锁定状态:" << pLock->LockStatus << ENDLINE
		     << " 状态信息:"<< StatusMsg << ENDLINE;
	}
	if(bIsLast) SetEvent(m_hLockEvent);	
}

//请求查询锁定证券仓位
int CRealTraderSpi::ReqQryLockPosition(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID)
{
	CThostFtdcQryLockPositionField QryLockPosition;

	memset(&QryLockPosition, 0, sizeof(CThostFtdcQryLockPositionField));
	strcpy(QryLockPosition.BrokerID, m_pCtpCntIf->m_brokerId);
	strcpy(QryLockPosition.InvestorID, m_pCtpCntIf->m_userId);
	strcpy(QryLockPosition.InstrumentID, InstrumentID);
	strcpy(QryLockPosition.ExchangeID, ExchangeID);

	m_pCtpCntIf->m_requestId++;
    int ret = m_pUserApi->ReqQryLockPosition(&QryLockPosition, m_pCtpCntIf->m_requestId);
	cerr << m_Prompt << " 请求 | 查询锁定证券仓位..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

//请求查询锁定证券仓位应答
void CRealTraderSpi::OnRspQryLockPosition(CThostFtdcLockPositionField *pLockPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pLockPosition){
		if(m_pRealApp->m_bManualTrade == true)
		{
			cerr << m_Prompt << " 响应 | 查询锁定证券仓位成功..." << ENDLINE
				 << " 证券代码:" << pLockPosition->InstrumentID << ENDLINE
				 << " 数量:" << pLockPosition->Volume << ENDLINE           //总的已锁定证券数量
				 << " 冻结数量:"<< pLockPosition->FrozenVolume << ENDLINE; //备兑持仓使用冻结的数量
		}
		memcpy(&m_LockPosition, pLockPosition, sizeof(CThostFtdcLockPositionField));
	}
	if(bIsLast && m_pRealApp->m_bInitDone) SetEvent(m_hLockEvent);	
}
#endif

void CRealTraderSpi::OnFrontDisconnected(int nReason)
{
	cerr << m_Prompt <<" 响应 | 交易前置连接中断..." 
	  << " reason=" << nReason << ENDLINE;
	
	Beep(2000,500);  //以2000Hz的频率，蜂鸣500毫秒，当做报警信号
}
		
void CRealTraderSpi::OnHeartBeatWarning(int nTimeLapse)
{
	cerr << m_Prompt <<" 响应 | 交易前置心跳超时警告..." 
	  << " TimerLapse = " << nTimeLapse << ENDLINE;
}

void CRealTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	IsErrorRspInfo(pRspInfo);
}

bool CRealTraderSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	// 如果ErrorID != 0, 说明收到了错误的响应
	bool ret = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (ret){
		string ErrorMsg = StatusMessageCodeConvert(pRspInfo->ErrorMsg);
        cerr << m_Prompt <<" 响应 | "<< ErrorMsg << ENDLINE;
    }
	return ret;
}

void CRealTraderSpi::PrintOrders()
{
	CThostFtdcOrderField* pOrder; 
	for(size_t i=0; i<m_orderList.size(); i++){
		pOrder = &m_orderList[i];
	    string StatusMsg = StatusMessageCodeConvert(pOrder->StatusMsg);
		
		cerr << m_Prompt <<" 报单 | 合约:"<<pOrder->InstrumentID
			<<" 方向:"<<MapDirection(pOrder->Direction,false)
			<<" 开平:"<<MapOffset(pOrder->CombOffsetFlag[0],false)
			<<" 价格:"<<pOrder->LimitPrice
			<<" 数量:"<<pOrder->VolumeTotalOriginal
			<<" 序号:"<<pOrder->BrokerOrderSeq 
			<<" 报单编号:"<<pOrder->OrderSysID
			<<" 状态:"<< StatusMsg <<ENDLINE;
	}

	SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);
}

void CRealTraderSpi::PrintTrades()
{
	CThostFtdcTradeField* pTrade;
	for(size_t i=0; i<m_tradeList.size(); i++){
		pTrade = &m_tradeList[i];
		cerr << m_Prompt <<" 成交 | 合约:"<< pTrade->InstrumentID 
			<<" 方向:"<<MapDirection(pTrade->Direction,false)
			<<" 开平:"<<MapOffset(pTrade->OffsetFlag,false) 
			<<" 价格:"<<pTrade->Price
			<<" 数量:"<<pTrade->Volume
			<<" 报单编号:"<<pTrade->OrderSysID
			<<" 成交编号:"<<pTrade->TradeID<<ENDLINE;
	}

	SetEvent(m_pCtpCntIf->m_hTraderSpiEvent);
}

bool CRealTraderSpi::IsMyOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->FrontID == m_frontId) &&
			(pOrder->SessionID == m_sessionId) &&
			(strcmp(pOrder->OrderRef, m_orderRef) == 0));
}

bool CRealTraderSpi::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
			(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
			(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

char CRealTraderSpi::MapDirection(char src, bool toOrig=true){
	if(toOrig){
		if('b'==src||'B'==src) {
			src='0';
		}
		else if('s'==src||'S'==src){
			src='1';
		}
	}
	else{
		if('0'==src){
			src='B';
		}
		else if('1'==src){
			src='S';
		}
	}
	return src;
}

char CRealTraderSpi::MapOffset(char src, bool toOrig=true){
	if(toOrig){
		if('o'==src||'O'==src){
			src='0';
		}
		else if('c'==src||'C'==src){
			src='1';
		}
		else if('j'==src||'J'==src){
			src='3';
		}
	}
	else{
		if('0'==src){
			src='O';
		}
		else if('1'==src){
			src='C';
		}
		else if('3'==src){
			src='J';
		}
	}
	return src;
}

