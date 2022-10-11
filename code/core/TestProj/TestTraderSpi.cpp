#include "TestTraderSpi.h"
#include "TestApp.h"


//UserID 和InversterID的区别:
//期货公司交易员为投资者下单时，UserID 为操作员代码，InversterID 为投资者代码
//投资者自己下单时，两者同为投资者代码

//++
//构造函数
//--
CTestTraderSpi::CTestTraderSpi(CThostFtdcTraderApi* api, CTestApp* pApp):m_pUserApi(api), m_pTestApp(pApp)
{
	m_pUserApi = api;
	m_pTestApp = pApp;

	m_OrderActionRef = 1;
	m_Prompt = m_pTestApp->m_ArchName + "/" + m_pTestApp->m_CounterName + "$";

	m_bInstrumentStatusTest = false;
};
    
void CTestTraderSpi::OnFrontConnected()
{
	cerr << m_Prompt << " 连接交易前置...成功" << ENDLINE;

	//连接成功后即进行客户端认证
	if(m_pTestApp->m_bNeedAuth == true)
	{
		TThostFtdcProductInfoType UserProductInfo;
#ifdef SEE_THROUGH_SUPERVISION
		TThostFtdcAppIDType	AppID;
#endif
		TThostFtdcAuthCodeType AuthCode;

		strcpy(UserProductInfo, m_pTestApp->m_UserProductInfo.c_str());
#ifdef SEE_THROUGH_SUPERVISION
		strcpy(AppID, m_pTestApp->m_AppID.c_str());
#endif
		strcpy(AuthCode, m_pTestApp->m_AuthCode.c_str());

#ifdef SEE_THROUGH_SUPERVISION
		ReqAuthenticate(m_pTestApp->m_brokerId, m_pTestApp->m_userId, UserProductInfo, AppID, AuthCode);
#else
		ReqAuthenticate(m_pTestApp->m_brokerId, m_pTestApp->m_userId, UserProductInfo, AuthCode);
#endif
	}
	else
        SetEvent(m_pTestApp->m_hEvent);
}

int CTestTraderSpi::ReqUserLogin(TThostFtdcBrokerIDType	vBrokerId,
	        TThostFtdcUserIDType	vUserId,	TThostFtdcPasswordType	vPasswd)
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, vBrokerId); //strcpy(brokerId, vBrokerId); 
	strcpy(req.UserID, vUserId);     //strcpy(userId, vUserId); 
	strcpy(req.Password, vPasswd);
	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqUserLogin(&req, m_pTestApp->m_requestId);
    cerr << m_Prompt << " 请求 | 发送交易前置登录请求..." << ((ret == 0) ? "成功" :"失败") << ENDLINE;	

	return ret;
}

///客户端认证请求
#ifdef SEE_THROUGH_SUPERVISION
int CTestTraderSpi::ReqAuthenticate(TThostFtdcBrokerIDType	BrokerId, TThostFtdcUserIDType	UserId, TThostFtdcProductInfoType	UserProductInfo,
		TThostFtdcAppIDType	AppID, TThostFtdcAuthCodeType	AuthCode)
#else
int CTestTraderSpi::ReqAuthenticate(TThostFtdcBrokerIDType	BrokerId, TThostFtdcUserIDType	UserId, TThostFtdcProductInfoType	UserProductInfo,
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

	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqAuthenticate(&ReqAuthenticateField, m_pTestApp->m_requestId);
    cerr << m_Prompt <<" 请求 | 发送客户端认证请求..." << ((ret == 0) ? "成功" :"失败") << ENDLINE;	

	return ret;
}

///客户端认证响应
void CTestTraderSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
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
	}

    SetEvent(m_pTestApp->m_hEvent);
}

///登出请求
int CTestTraderSpi::ReqUserLogout(TThostFtdcBrokerIDType brokerId, TThostFtdcUserIDType	userId)
{
	CThostFtdcUserLogoutField UserLogout;

	memset(&UserLogout, 0, sizeof(CThostFtdcUserLogoutField));
	strcpy(UserLogout.BrokerID, brokerId); 
	strcpy(UserLogout.UserID, userId);  
	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqUserLogout(&UserLogout, m_pTestApp->m_requestId); 
    cerr << m_Prompt << " 请求 | 发送交易前置登出请求..."<<((ret == 0) ? "成功" :"失败") << ENDLINE;	

	return ret;
}


void CTestTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if ( !IsErrorRspInfo(pRspInfo) && pRspUserLogin ) {  
		// 保存会话参数	
		m_frontId = pRspUserLogin->FrontID;
		m_sessionId = pRspUserLogin->SessionID;
		int nextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
		sprintf(m_orderRef, "%d", ++nextOrderRef);
		cerr << m_Prompt << " 响应 | 登录交易前置成功...当前交易日:"<<pRspUserLogin->TradingDay<<ENDLINE;     
	}

    if(bIsLast) SetEvent(m_pTestApp->m_hEvent);
}

///登出请求响应
void CTestTraderSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if ( !IsErrorRspInfo(pRspInfo) && pUserLogout ) {  
		// 保存会话参数	
		cerr << m_Prompt << " 响应 | 登出交易前置成功..."
			<< " 经纪商代码: " << pUserLogout->BrokerID 
			<< " 客户代码: " << pUserLogout->UserID << ENDLINE;
	}

    if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);
}

int CTestTraderSpi::ReqSettlementInfoConfirm()
{
	CThostFtdcSettlementInfoConfirmField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_pTestApp->m_brokerId);
	strcpy(req.InvestorID, m_pTestApp->m_userId);
	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqSettlementInfoConfirm(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt << " 请求 | 发送结算单确认..."<<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

void CTestTraderSpi::OnRspSettlementInfoConfirm(
        CThostFtdcSettlementInfoConfirmField  *pSettlementInfoConfirm, 
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{	
	if( !IsErrorRspInfo(pRspInfo) && pSettlementInfoConfirm){
    cerr << m_Prompt <<" 响应 | 结算单..." << pSettlementInfoConfirm->InvestorID
      << "...<"<<pSettlementInfoConfirm->ConfirmDate
      << " "<<pSettlementInfoConfirm->ConfirmTime << ">...确认" << ENDLINE;
  }
  if(bIsLast) 
  	SetEvent(m_pTestApp->m_hEvent);
}

int CTestTraderSpi::ReqQryInstrument(TThostFtdcInstrumentIDType instId)
//如果要查询豆粕相关的所有期货和期权合同将InstrumentID设置为”m”，如果要查询某个月份的豆粕期权和期货合约可输入”mYYMM”如”m1705”
{
	CThostFtdcQryInstrumentField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.InstrumentID, instId);//为空表示查询所有合约，为空时这条语句注释即可
	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqQryInstrument(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt <<" 请求 | 发送合约查询..."<<((ret == 0)?"成功":"失败")<<ENDLINE;

	return ret;
}

void CTestTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, 
         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
#ifndef	DUMP_TO_FILE
	if ( !IsErrorRspInfo(pRspInfo) &&  pInstrument){
		cerr << m_Prompt <<" 响应 | 合约:"<<pInstrument->InstrumentID
			<<" 交割月:"<<pInstrument->DeliveryMonth
			<<" 多头保证金率:"<<pInstrument->LongMarginRatio
			<<" 空头保证金率:"<<pInstrument->ShortMarginRatio<<ENDLINE;   
	}
  if(bIsLast)  
  	SetEvent(m_pTestApp->m_hEvent);
#else
	int InstListSize;
	CRspInstrument rspInstrument;

	InstListSize = m_pTestApp->m_RspInstrumentList.size();
	GetLocalTime(&rspInstrument.CurTime);

	if (!IsErrorRspInfo(pRspInfo) &&  pInstrument)
	{
		memcpy(&rspInstrument.Instrument,  pInstrument, sizeof(CThostFtdcInstrumentField));
		rspInstrument.IsLast = bIsLast;
		pMyTestApp->m_RspInstrumentList.push_back(rspInstrument);
	}
#endif
}

int CTestTraderSpi::ReqQryTradingAccount()
{
	CThostFtdcQryTradingAccountField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_pTestApp->m_brokerId);
	strcpy(req.InvestorID, m_pTestApp->m_userId);
	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqQryTradingAccount(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt <<" 请求 | 发送资金查询..."<<((ret == 0)?"成功":"失败")<<ENDLINE;

	return ret;
}

void CTestTraderSpi::OnRspQryTradingAccount(
    CThostFtdcTradingAccountField *pTradingAccount, 
   CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) &&  pTradingAccount){
    cerr << m_Prompt << " 响应 | 权益:" << pTradingAccount->Balance
      << " 可用:" << pTradingAccount->Available   
      << " 保证金:" << pTradingAccount->CurrMargin
      << " 平仓盈亏:" << pTradingAccount->CloseProfit
      << " 持仓盈亏:" << pTradingAccount->PositionProfit
      << " 手续费:" << pTradingAccount->Commission
      << " 冻结保证金:" << pTradingAccount->FrozenMargin
	  << " 冻结的资金:" << pTradingAccount->FrozenCash
	  << " 冻结的手续费:" << pTradingAccount->FrozenCommission
	  <<" 币种代码:" << pTradingAccount->CurrencyID
      //<<" 冻结手续费:"<<pTradingAccount->FrozenCommission 
      << ENDLINE;    
  }
  if(bIsLast) 
  	SetEvent(m_pTestApp->m_hEvent);
}

int CTestTraderSpi::ReqQryInvestorPosition(TThostFtdcInstrumentIDType instId)
{
	CThostFtdcQryInvestorPositionField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_pTestApp->m_brokerId);
	strcpy(req.InvestorID, m_pTestApp->m_userId);
	strcpy(req.InstrumentID, instId);
	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqQryInvestorPosition(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt <<" 请求 | 发送持仓查询..."<<((ret == 0)?"成功":"失败")<<ENDLINE;

	return ret;
}

void CTestTraderSpi::OnRspQryInvestorPosition(
    CThostFtdcInvestorPositionField *pInvestorPosition, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if( !IsErrorRspInfo(pRspInfo) &&  pInvestorPosition ){
		cerr << m_Prompt <<" 响应 | 合约:"<<pInvestorPosition->InstrumentID
			<<" 方向:"<<MapDirection(pInvestorPosition->PosiDirection-2,false)
			<<" 总持仓:"<<pInvestorPosition->Position
			<<" 昨仓:"<<pInvestorPosition->YdPosition 
			<<" 今仓:"<<pInvestorPosition->TodayPosition
			<<" 持仓盈亏:"<<pInvestorPosition->PositionProfit
			<<" 保证金:"<<pInvestorPosition->UseMargin<<ENDLINE;
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);
}

///请求查询合约保证金率
int CTestTraderSpi::ReqQryInstrumentMarginRate(TThostFtdcInstrumentIDType	InstrumentID)
{
	CThostFtdcQryInstrumentMarginRateField req;
	memset(&req, 0, sizeof(req));

	///经纪公司代码
	strcpy(req.BrokerID, m_pTestApp->m_brokerId);
	///投资者代码
	strcpy(req.InvestorID, m_pTestApp->m_userId);
	///合约代码
	strcpy(req.InstrumentID, InstrumentID);
	//////投机套保标志
	req.HedgeFlag = THOST_FTDC_HF_Speculation;	//投机

	m_pTestApp->m_requestId++;
	int iResult = m_pUserApi->ReqQryInstrumentMarginRate(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt << "--->>> 查询合约保证金率: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

///请求查询合约保证金率响应
void CTestTraderSpi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, 
												CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << m_Prompt << "--->>> " << __FUNCTION__ << ENDLINE;
	if (!IsErrorRspInfo(pRspInfo) &&  pInstrumentMarginRate)
	{
	    cerr << m_Prompt << "--->>> " << "合约保证金率:" << ENDLINE;
		cerr << "LongMarginRatioByMoney = " << pInstrumentMarginRate->LongMarginRatioByMoney << ENDLINE;
		cerr << "ShortMarginRatioByMoney = " << pInstrumentMarginRate->ShortMarginRatioByMoney << ENDLINE;
		cerr << "LongMarginRatioByVolume = " << pInstrumentMarginRate->LongMarginRatioByVolume << ENDLINE;
		cerr << "ShortMarginRatioByVolume = " << pInstrumentMarginRate->ShortMarginRatioByVolume << ENDLINE;
		cerr << "IsRelative = " << pInstrumentMarginRate->IsRelative << ENDLINE;
	}

    //连续查询可能导致未处理请求超过许可数
	//Sleep(1000);

	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);
		
}

///请求查询合约手续费率
int CTestTraderSpi::ReqQryInstrumentCommissionRate(TThostFtdcInstrumentIDType	InstrumentID)
{
	CThostFtdcQryInstrumentCommissionRateField req;
	memset(&req, 0, sizeof(req));

	///经纪公司代码
	strcpy(req.BrokerID, m_pTestApp->m_brokerId);
	///投资者代码
	strcpy(req.InvestorID, m_pTestApp->m_userId);
	///合约代码
	strcpy(req.InstrumentID, InstrumentID);

	m_pTestApp->m_requestId++;
	int iResult = m_pUserApi->ReqQryInstrumentCommissionRate(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt << "--->>> 查询合约手续费率: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

///请求查询合约手续费率响应
void CTestTraderSpi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, 
													CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << m_Prompt << "--->>> " << __FUNCTION__ << ENDLINE;
	if (!IsErrorRspInfo(pRspInfo) &&  pInstrumentCommissionRate)
	{
		cerr << m_Prompt << "--->>> " << "合约手续费率:" << ENDLINE;
		cerr << "OpenRatioByMoney = " << pInstrumentCommissionRate->OpenRatioByMoney << ENDLINE;
		cerr << "OpenRatioByVolume = " << pInstrumentCommissionRate->OpenRatioByVolume << ENDLINE;
		cerr << "CloseRatioByMoney = " << pInstrumentCommissionRate->CloseRatioByMoney << ENDLINE;
		cerr << "CloseRatioByVolume = " << pInstrumentCommissionRate->CloseRatioByVolume << ENDLINE;
		cerr << "CloseTodayRatioByMoney = " << pInstrumentCommissionRate->CloseTodayRatioByMoney << ENDLINE;
		cerr << "CloseTodayRatioByVolume = " << pInstrumentCommissionRate->CloseTodayRatioByVolume << ENDLINE;
	}

	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);	
}

///请求查询期权合约手续费
int CTestTraderSpi::ReqQryOptionInstrCommRate(TThostFtdcInstrumentIDType  InstrumentID)
{
	CThostFtdcQryOptionInstrCommRateField req;
	memset(&req, 0, sizeof(req));

	///经纪公司代码
	strcpy(req.BrokerID, m_pTestApp->m_brokerId);
	///投资者代码
	strcpy(req.InvestorID, m_pTestApp->m_userId);
	///合约代码
	strcpy(req.InstrumentID, InstrumentID);

	m_pTestApp->m_requestId++;
	int iResult = m_pUserApi->ReqQryOptionInstrCommRate(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt << "--->>> 查询期权合约手续费率: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

///请求查询期权合约手续费响应
void CTestTraderSpi::OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << m_Prompt << "--->>> " << __FUNCTION__ << ENDLINE;
	if (!IsErrorRspInfo(pRspInfo) &&  pOptionInstrCommRate)
	{
		cerr << m_Prompt << "--->>> " << "期权合约手续费率:" << ENDLINE;
		cerr << "OpenRatioByMoney = " << pOptionInstrCommRate->OpenRatioByMoney << ENDLINE;
		cerr << "OpenRatioByVolume = " << pOptionInstrCommRate->OpenRatioByVolume << ENDLINE;
		cerr << "CloseRatioByMoney = " << pOptionInstrCommRate->CloseRatioByMoney << ENDLINE;
		cerr << "CloseRatioByVolume = " << pOptionInstrCommRate->CloseRatioByVolume << ENDLINE;
		cerr << "CloseTodayRatioByMoney = " << pOptionInstrCommRate->CloseTodayRatioByMoney << ENDLINE;
		cerr << "CloseTodayRatioByVolume = " << pOptionInstrCommRate->CloseTodayRatioByVolume << ENDLINE;
		cerr << "StrikeRatioByMoney = " << pOptionInstrCommRate->StrikeRatioByMoney << ENDLINE; //行权手续费
		cerr << "StrikeRatioByVolume = " << pOptionInstrCommRate->StrikeRatioByVolume << ENDLINE; //行权手续费
	}

	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);	
}

///请求查询经纪公司交易参数，计算期权保证金时有用，CurrenyID可以在OnRspTradingAccount中获得, 人民币"CNY"
int CTestTraderSpi::ReqQryBrokerTradingParams()
{
	CThostFtdcQryBrokerTradingParamsField req;
	memset(&req, 0, sizeof(req));

	///经纪公司代码
	strcpy(req.BrokerID, m_pTestApp->m_brokerId);
	///投资者代码
	strcpy(req.InvestorID, m_pTestApp->m_userId);
	///币种代码
	strcpy(req.CurrencyID, "CNY");

	m_pTestApp->m_requestId++;
	int iResult = m_pUserApi->ReqQryBrokerTradingParams(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt << "--->>> 查询经纪公司交易参数: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

	///请求查询经纪公司交易参数响应
void CTestTraderSpi::OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << m_Prompt << "--->>> " << __FUNCTION__ << ENDLINE;
	if (!IsErrorRspInfo(pRspInfo) &&  pBrokerTradingParams)
	{
		cerr << m_Prompt << "--->>> " << "经纪公司交易参数:" << ENDLINE;
		cerr << "保证金价格类型: MarginPriceType = " << pBrokerTradingParams->MarginPriceType << ENDLINE;  //成交价或者结算价
		cerr << "盈亏算法: Algorithm = " << pBrokerTradingParams->Algorithm << ENDLINE;
		cerr << "可用是否包含平仓盈利: AvailIncludeCloseProfit = " << pBrokerTradingParams->AvailIncludeCloseProfit << ENDLINE;
		cerr << "币种代码: CurrencyID = " << pBrokerTradingParams->CurrencyID << ENDLINE;
		cerr << "期权权利金价格类型: OptionRoyaltyPriceType = " << pBrokerTradingParams->OptionRoyaltyPriceType << ENDLINE; //成交价或者结算价
	}

	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);
}

///请求查询期权交易成本
int CTestTraderSpi::ReqQryOptionInstrTradeCost(	TThostFtdcInstrumentIDType	InstrumentID, TThostFtdcPriceType	InputPrice, TThostFtdcPriceType	UnderlyingPrice)
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
	strcpy(req.BrokerID, m_pTestApp->m_brokerId);
	///投资者代码
	strcpy(req.InvestorID, m_pTestApp->m_userId);
	///合约代码
	strcpy(req.InstrumentID, InstrumentID);
	//////投机套保标志
	req.HedgeFlag = THOST_FTDC_HF_Speculation;	//投机
	req.InputPrice = InputPrice;
	req.UnderlyingPrice = UnderlyingPrice;

	m_pTestApp->m_requestId++;
	int iResult = m_pUserApi->ReqQryOptionInstrTradeCost(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt << "--->>> 查询期权交易成本: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

///请求查询期权交易成本响应
void CTestTraderSpi::OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
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

	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);	
}

//报单录入
int CTestTraderSpi::ReqOrderInsert(TThostFtdcInstrumentIDType instId,
    TThostFtdcDirectionType dir, TThostFtdcCombOffsetFlagType kpp,
    TThostFtdcPriceType price,   TThostFtdcVolumeType vol, 
	char TimeCondition, char VolumeCondition, char MarketOrderType, 
	TThostFtdcHedgeFlagType HedgeFlag, string strExchangeID)
{
	double eps = 0.0000001;

	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));	
	strcpy(req.BrokerID, m_pTestApp->m_brokerId);  //应用单元代码	
	strcpy(req.InvestorID, m_pTestApp->m_userId); //投资者代码	
	strcpy(req.InstrumentID, instId); //合约代码	
	strcpy(req.OrderRef, m_orderRef);  //报单引用
	int nextOrderRef = atoi(m_orderRef);
	sprintf(m_orderRef, "%d", ++nextOrderRef);
	strcpy(req.UserID, m_pTestApp->m_userId);  //用户代码
	if(strExchangeID != "") strcpy(req.ExchangeID, strExchangeID.c_str());
  
	req.LimitPrice = price;	//价格
	if(abs(req.LimitPrice) < eps){
		if(MarketOrderType == 'A' || MarketOrderType == 'a')      //价格类型=市价, 适用于期货期权
			req.OrderPriceType = THOST_FTDC_OPT_AnyPrice;         
		else if(MarketOrderType == 'B' || MarketOrderType == 'b') //最优价格, 适用于股票期权的市价报单
			req.OrderPriceType = THOST_FTDC_OPT_BestPrice;
		else if(MarketOrderType == 'F' || MarketOrderType == 'f') //证券市价单使用五档价
			req.OrderPriceType = THOST_FTDC_OPT_FiveLevelPrice;
		else
			req.OrderPriceType = MarketOrderType;
	}
	else{
		req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;  //价格类型=限价	
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

	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqOrderInsert(&req, m_pTestApp->m_requestId);

	double LocalTimeSeconds = GetCurrentSeconds();

	stringstream sstr;
	sstr.clear(); sstr.str(""); sstr.precision(18);
	sstr <<  "当前秒数: " << LocalTimeSeconds;

	cerr << m_Prompt << " 请求 | 发送报单..." << ((ret == 0)?"成功":"失败") << "  时间: " << sstr.str() << ENDLINE;

	return ret;
}

void CTestTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, 
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
	cerr << "--->>> " << __FUNCTION__ << ENDLINE;
	if( !IsErrorRspInfo(pRspInfo) && pInputOrder ){
		cerr << m_Prompt <<"响应 | 报单提交响应...报单引用:"<<pInputOrder->OrderRef<<ENDLINE;  
	}
	else if(pInputOrder) {
		; //错误的报单THOST拒绝接受，用pInputOrder和pRspInfo中的信息更新报单状态
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);	
}

//报单操作请求， 如果仅在Thost使用FrontId+SessionId+OrderRef进行撤单操作
//如果在交易所， 需要使用ExchangeID+OrderSysID撤单
//总结起来，以上五个参数都需要
int CTestTraderSpi::ReqOrderAction(TThostFtdcSequenceNoType orderSeq, int FrontId, int SessionId, int OrderRef)
{
	bool found=false; size_t i=0;
	for(i=0;i<m_orderList.size();i++){
		string strOrderSysID = m_orderList[i].OrderSysID;
		RemoveSpace(strOrderSysID);
		if(m_orderList[i].BrokerOrderSeq == orderSeq && strOrderSysID != ""){ 
			found = true; 
			break;
		}
	}

	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	req.OrderActionRef = m_OrderActionRef++;
	req.ActionFlag = THOST_FTDC_AF_Delete;       //操作标志 

	if(found == true)
	{
		CThostFtdcOrderField *pOrder = &m_orderList[i];
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
		string strExchangeId;
		string strOrderSysId;
		string strOrderRef;
		string strInstrumentId;

		cerr <<" 请求 | 无法在报单列表中找到该经纪商报单编号对应的报单！."<<ENDLINE; 
		cerr << "ExchangeID >"; cin >> strExchangeId;
		cerr << "OrderSysID >"; cin >> strOrderSysId;  //OrderSysID手工输入的都不会对
		cerr << "OrderRef >"; cin >> strOrderRef;
		cerr << "InstrumentID >"; cin >> strInstrumentId;

		strcpy(req.BrokerID, m_pTestApp->m_brokerId);   //经纪公司代码	
		strcpy(req.InvestorID, m_pTestApp->m_userId); //投资者代码
		strcpy(req.UserID, m_pTestApp->m_userId); //用户代码
		req.FrontID = m_frontId;           //前置编号	
		req.SessionID = m_sessionId;       //会话编号

		strcpy(req.ExchangeID, strExchangeId.c_str());
		strcpy(req.OrderSysID, strOrderSysId.c_str());
		strcpy(req.OrderRef, strOrderRef.c_str());
		strcpy(req.InstrumentID, strInstrumentId.c_str());
	} 

	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqOrderAction(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt << " 请求 | 发送撤单..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

void CTestTraderSpi::OnRspOrderAction(
      CThostFtdcInputOrderActionField *pInputOrderAction, 
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pInputOrderAction){
		cerr << m_Prompt << " 响应 | 撤单成功..."
			<< "交易所:"<<pInputOrderAction->ExchangeID
			<<" 交易所报单编号:"<<pInputOrderAction->OrderSysID<<ENDLINE;
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);	
}

int CTestTraderSpi::ReqQryOrder(CThostFtdcOrderField *pOrder)
{
	CThostFtdcQryOrderField qryOrder;
	memset(&qryOrder, 0, sizeof(CThostFtdcQryOrderField));

	strcpy(qryOrder.BrokerID, pOrder->BrokerID);
	strcpy(qryOrder.InvestorID, pOrder->InvestorID);
	strcpy(qryOrder.InstrumentID, pOrder->InstrumentID);
	strcpy(qryOrder.ExchangeID, pOrder->ExchangeID);
	strcpy(qryOrder.OrderSysID, pOrder->OrderSysID);
	//strcpy(qryOrder.InsertTimeStart, pOrder->InsertTime);
	//strcpy(qryOrder.InsertTimeEnd, pOrder->InsertTime);

	m_pTestApp->m_requestId++;
    int ret = m_pUserApi->ReqQryOrder(&qryOrder, m_pTestApp->m_requestId);
	cerr << m_Prompt << " 请求 | 报单查询..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

void CTestTraderSpi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pOrder){
		cerr << m_Prompt << " 响应 | 报单查询成功..."
			<< "交易所代号:"<< pOrder->ExchangeID
			<< " 交易所报单编号:" << pOrder->OrderSysID 
			<< " 报单状态:" << pOrder->OrderStatus
			<< ENDLINE;
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);	
}

///报单回报
void CTestTraderSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
//OnErrRtnOrde的错误信息通过OnRtnOrder返回，所以需要查询Oder的状态
{
	if(m_bInstrumentStatusTest == false)
	{	
	    CThostFtdcOrderField order;
	    memcpy(&order,  pOrder, sizeof(CThostFtdcOrderField));
	    m_orderList.push_back(order);

	    double LocalTimeSeconds = GetCurrentSeconds();

	    stringstream sstr;
	    sstr.clear(); sstr.str(""); sstr.precision(18);
	    sstr <<  "当前秒数: " << LocalTimeSeconds;	

	    string StatusMsg = StatusMessageCodeConvert(pOrder->StatusMsg);

	    cerr << m_Prompt << " 回报 | 报单已提交...经纪商报单序号:" << pOrder->BrokerOrderSeq << ENDLINE
	    	<< " 报单引用: " << pOrder->OrderRef << ENDLINE
	    	<< " 交易所报单编号:" << pOrder->OrderSysID << ENDLINE
	    	<< " 报单状态:" << pOrder->OrderStatus << ENDLINE
	    	<< " 状态消息:"<< StatusMsg << ENDLINE
	    	<< " 时间: " << sstr.str() << ENDLINE << ENDLINE;

	    SetEvent(m_pTestApp->m_hEvent);
	}
}

///成交通知
void CTestTraderSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	if(m_bInstrumentStatusTest == false)
	{
	    CThostFtdcTradeField trade;
	    memcpy(&trade,  pTrade, sizeof(CThostFtdcTradeField));
	    m_tradeList.push_back(trade);

	    cerr << m_Prompt << " 回报 | 成交, 编号:" << pTrade->TradeID << " 合约:" << pTrade->InstrumentID 
	    	<< "  数量:" << pTrade->Volume << "  价格:" << pTrade->Price << ENDLINE;

        SetEvent(m_pTestApp->m_hEvent);	
	}
}

///合约交易状态通知
void CTestTraderSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus) 
{
	if(m_bInstrumentStatusTest == true)
	{
	    cerr << "--->>> " << __FUNCTION__ << ENDLINE;

		SYSTEMTIME CurTime;
		GetLocalTime(&CurTime);
		stringstream sstr;
		sstr.clear(); sstr.str(""); sstr.precision(18);
		sstr << CurTime.wHour << ":" << CurTime.wMinute << ":" << CurTime.wSecond << ENDLINE;
		cerr << "CurrentTime = " << sstr.str() << ENDLINE;

		string strInstrumentStatus;
#ifndef FUTURE_OPTION_ONLY
		switch(pInstrumentStatus->InstrumentStatus)
		{
		///开盘前
		case THOST_FTDC_IS_BeforeTrading:
			strInstrumentStatus = "THOST_FTDC_IS_BeforeTrading";
			break;
		///非交易
		case THOST_FTDC_IS_NoTrading:
			strInstrumentStatus = "THOST_FTDC_IS_NoTrading";
			break;
		///连续交易
		case THOST_FTDC_IS_Continous:
			strInstrumentStatus = "THOST_FTDC_IS_Continous";
			break;
		///集合竞价报单
		case THOST_FTDC_IS_AuctionOrdering:
			strInstrumentStatus = "THOST_FTDC_IS_AuctionOrdering";
			break;
		///集合竞价价格平衡
		case THOST_FTDC_IS_AuctionBalance:
			strInstrumentStatus = "THOST_FTDC_IS_AuctionBalance";
			break;
		///集合竞价撮合
		case THOST_FTDC_IS_AuctionMatch:
			strInstrumentStatus = "THOST_FTDC_IS_AuctionMatch";
			break;
		///收盘
		case THOST_FTDC_IS_Closed:
			strInstrumentStatus = "THOST_FTDC_IS_Closed";
			break;
		///集合竞价
		case THOST_FTDC_IS_Auction:
			strInstrumentStatus = "THOST_FTDC_IS_Auction";
			break;
		///休市
		case THOST_FTDC_IS_BusinessSuspension:
			strInstrumentStatus = "THOST_FTDC_IS_BusinessSuspension";
			break;
		///波动性中断
		case THOST_FTDC_IS_VolatilityInterrupt:
			strInstrumentStatus = "THOST_FTDC_IS_VolatilityInterrupt";
			break;
		///临时停牌
		case THOST_FTDC_IS_TemporarySuspension:
			strInstrumentStatus = "THOST_FTDC_IS_TemporarySuspension";
			break;
		///收盘集合竞价
		case THOST_FTDC_IS_AuctionAfterClosed:
			strInstrumentStatus = "THOST_FTDC_IS_AuctionAfterClosed";
			break;
		///可恢复交易的熔断
		case THOST_FTDC_IS_ResumableFusing:
			strInstrumentStatus = "THOST_FTDC_IS_ResumableFusing";
			break;
		///不可恢复交易的熔断
		case THOST_FTDC_IS_UnResumableFusing:
			strInstrumentStatus = "THOST_FTDC_IS_UnResumableFusing";
			break;
		///盘后交易
		case THOST_FTDC_IS_TradingAfterClosed:
			strInstrumentStatus = "THOST_FTDC_IS_TradingAfterClosed";
			break;
		default:
			strInstrumentStatus = "Undefined_InstrumentStatus";
		}
#else
		strInstrumentStatus = pInstrumentStatus->InstrumentStatus;
#endif
	
		cerr << "ExchangeID = " << pInstrumentStatus->ExchangeID << ENDLINE;
		cerr << "InstrumentID = " << pInstrumentStatus->InstrumentID << ENDLINE;
		cerr << "InstrumentStatus = " << strInstrumentStatus << ENDLINE;
		cerr << "EnterTime = " << pInstrumentStatus->EnterTime << ENDLINE;
		cerr << "EnterReason = " << pInstrumentStatus->EnterReason << ENDLINE;
		cerr << ENDLINE;
	}

	CThostFtdcInstrumentStatusField InstrumentStatus;
	memcpy(&InstrumentStatus,  pInstrumentStatus, sizeof(CThostFtdcInstrumentStatusField));
	if(m_InstrumentStatusList.size() < 3000)
		m_InstrumentStatusList.push_back(InstrumentStatus);
	else
		m_InstrumentStatusList.clear();


	if(m_bInstrumentStatusTest == true) SetEvent(m_pTestApp->m_hEvent);	
};


///请求查询成交
int CTestTraderSpi::ReqQryTrade(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcTradeIDType TradeID)
{
	CThostFtdcQryTradeField qryTrade;

	memset(&qryTrade, 0, sizeof(CThostFtdcQryTradeField));
	strcpy(qryTrade.BrokerID, m_pTestApp->m_brokerId);
	strcpy(qryTrade.InvestorID, m_pTestApp->m_userId);
	strcpy(qryTrade.InstrumentID, InstrumentID);
	strcpy(qryTrade.ExchangeID, ExchangeID);
	strcpy(qryTrade.TradeID, TradeID);

	m_pTestApp->m_requestId++;
    int ret = m_pUserApi->ReqQryTrade(&qryTrade, m_pTestApp->m_requestId);
	cerr << m_Prompt << " 请求 | 成交查询..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

///请求查询成交响应
void CTestTraderSpi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
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
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);	
}

///请求期权行权
int CTestTraderSpi::ReqExecOrderInsert(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcVolumeType	Volume)
{
    CThostFtdcInputExecOrderField req; 
	memset(&req, 0, sizeof(CThostFtdcInputExecOrderField));
    strcpy(req.BrokerID, m_pTestApp->m_brokerId);  
    strcpy(req.InvestorID, m_pTestApp->m_userId); 
	strcpy(req.UserID, m_pTestApp->m_userId);
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

	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqExecOrderInsert(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt <<" 请求 | 发送行权请求..."<<((ret == 0)?"成功":"失败")<< ENDLINE;

	return ret;
}

///期权行权执行宣告录入请求响应
void CTestTraderSpi::OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
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
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);	
}

///执行宣告通知
void CTestTraderSpi::OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder)
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
	cerr << "pExecOrder->StatusMsg = " << StatusMessageCodeConvert(pExecOrder->StatusMsg) << ENDLINE;

	SetEvent(m_pTestApp->m_hEvent);	
}

///执行宣告操作请求/期权行权撤销
int CTestTraderSpi::ReqExecOrderAction(	TThostFtdcSequenceNoType	brokerExecOrderSeq)
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
		cerr <<" 请求 | 期权行权报单不存在."<<ENDLINE;
		SetEvent(m_pTestApp->m_hEvent);
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

	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqExecOrderAction(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt << " 请求 | 发送期权行权撤单..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

///执行宣告操作请求响应/期权行权撤单响应
void CTestTraderSpi::OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pInputExecOrderAction){
		cerr << m_Prompt << " 响应 | 期权行权撤单成功..."
			<< "交易所:"<<pInputExecOrderAction->ExchangeID
			<<" 交易所行权报单编号:"<<pInputExecOrderAction->ExecOrderSysID << ENDLINE;
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);
}

#ifndef FUTURE_OPTION_ONLY
///请求查询ETF期权合约手续费
int CTestTraderSpi::ReqQryETFOptionInstrCommRate(TThostFtdcInstrumentIDType  InstrumentID)
{
	CThostFtdcQryETFOptionInstrCommRateField req;
	memset(&req, 0, sizeof(req));

	///经纪公司代码
	strcpy(req.BrokerID, m_pTestApp->m_brokerId);
	///投资者代码
	strcpy(req.InvestorID, m_pTestApp->m_userId);
	///合约代码
	strcpy(req.InstrumentID, InstrumentID);

	m_pTestApp->m_requestId++;
	int iResult = m_pUserApi->ReqQryETFOptionInstrCommRate(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt << "--->>> 查询ETF期权合约手续费率: " << ((iResult == 0) ? " 成功" : " 失败") << ENDLINE;

	return iResult;
}

///请求查询ETF期权合约手续费响应
///卖出ETF期权是不收手续费的
void CTestTraderSpi::OnRspQryETFOptionInstrCommRate(CThostFtdcETFOptionInstrCommRateField *pETFOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << m_Prompt << "--->>> " << __FUNCTION__ << ENDLINE;
	if (!IsErrorRspInfo(pRspInfo) &&  pETFOptionInstrCommRate)
	{
		cerr << m_Prompt << "--->>> " << "期权合约手续费率:" << ENDLINE;
		cerr << "OpenRatioByMoney = " << pETFOptionInstrCommRate->OpenRatioByMoney << ENDLINE;
		cerr << "OpenRatioByVolume = " << pETFOptionInstrCommRate->OpenRatioByVolume << ENDLINE;
		cerr << "CloseRatioByMoney = " << pETFOptionInstrCommRate->CloseRatioByMoney << ENDLINE;
		cerr << "CloseRatioByVolume = " << pETFOptionInstrCommRate->CloseRatioByVolume << ENDLINE;
		cerr << "CloseTodayRatioByMoney = " << pETFOptionInstrCommRate->CloseTodayRatioByMoney << ENDLINE;
		cerr << "CloseTodayRatioByVolume = " << pETFOptionInstrCommRate->CloseTodayRatioByVolume << ENDLINE;
		cerr << "StrikeRatioByMoney = " << pETFOptionInstrCommRate->StrikeRatioByMoney << ENDLINE; //行权手续费
		cerr << "StrikeRatioByVolume = " << pETFOptionInstrCommRate->StrikeRatioByVolume << ENDLINE; //行权手续费
		cerr << "HedgeFlag = " << pETFOptionInstrCommRate->HedgeFlag << ENDLINE;
		cerr << "PosiDirection = " << pETFOptionInstrCommRate->PosiDirection << ENDLINE;
	}

	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);	
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
int CTestTraderSpi::ReqLockInsert(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcLockTypeType LockType, TThostFtdcVolumeType Volume)
{
	CThostFtdcInputLockField InputLock;

	memset(&InputLock, 0, sizeof(InputLock));
	strcpy(InputLock.BrokerID, m_pTestApp->m_brokerId);   //经纪公司代码	
	strcpy(InputLock.InvestorID, m_pTestApp->m_userId);   //投资者代码
	strcpy(InputLock.ExchangeID, ExchangeID);
	strcpy(InputLock.InstrumentID, InstrumentID);
	InputLock.LockType = LockType;
	InputLock.Volume = Volume;
	strcpy(InputLock.LockRef, m_orderRef);                //报单引用，将锁定和普通报单同等对待
	int nextOrderRef = atoi(m_orderRef);
	sprintf(m_orderRef, "%d", ++nextOrderRef);

	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqLockInsert(&InputLock, m_pTestApp->m_requestId);

	double LocalTimeSeconds = GetCurrentSeconds();

	stringstream sstr;
	sstr.clear(); sstr.str(""); sstr.precision(18);
	sstr <<  "当前秒数: " << LocalTimeSeconds;	

	if(LockType == THOST_FTDC_LCKT_Lock)
	{
		cerr << m_Prompt << " 请求 | 发送备兑证券锁定..." <<((ret == 0)?"成功":"失败") << "  时间: " << sstr.str() << ENDLINE;
	}
	else
	{
		cerr << m_Prompt << " 请求 | 发送备兑证券解锁..." <<((ret == 0)?"成功":"失败") << "  时间: " << sstr.str() << ENDLINE;
	}

	return ret;
}

//锁定应答
//如果报单无错误, 此函数不会被调用
void CTestTraderSpi::OnRspLockInsert(CThostFtdcInputLockField *pInputLock, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << __FUNCTION__ << ENDLINE;
	if( !IsErrorRspInfo(pRspInfo) && pInputLock ){
		if(pInputLock->LockType == THOST_FTDC_LCKT_Lock)
		{
			cerr << m_Prompt <<"响应 | 锁定证券提交...锁定引用:" << pInputLock->LockRef << ENDLINE;  
		}
		else
		{
			cerr << m_Prompt <<"响应 | 解锁证券提交...锁定引用:" << pInputLock->LockRef << ENDLINE;  
		}
	}
	else if(pInputLock) {
		; //错误的报单THOST拒绝接受，用pInputOrder和pRspInfo中的信息更新报单状态
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);
}

//锁定通知
//报单执行正确将只会调用此函数
void CTestTraderSpi::OnRtnLock(CThostFtdcLockField *pLock)
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

	double LocalTimeSeconds = GetCurrentSeconds();

	stringstream sstr;
	sstr.clear(); sstr.str(""); sstr.precision(18);
	sstr <<  "当前秒数: " << LocalTimeSeconds;

	if(pLock->LockType == THOST_FTDC_LCKT_Lock)
	{
		cerr << m_Prompt << " 回报 | 证券锁定报单已提交...锁定引用:" << pLock->LockRef << ENDLINE;
	}
	else
	{
		cerr << m_Prompt << " 回报 | 证券解锁报单已提交...锁定引用:" << pLock->LockRef << ENDLINE;
	}

	string StatusMsg = StatusMessageCodeConvert(pLock->StatusMsg);

	cerr << " 锁定编号:" << pLock->LockSysID << ENDLINE
		 << " 锁定状态:" << pLock->LockStatus << ENDLINE
		 << " 状态信息:"<< StatusMsg << ENDLINE
		 << " 时间: " << sstr.str() << ENDLINE << ENDLINE;

	SetEvent(m_pTestApp->m_hEvent);	
}

//锁定错误通知
//如果报单无错误, 此函数不会被调用
void CTestTraderSpi::OnErrRtnLockInsert(CThostFtdcInputLockField *pInputLock, CThostFtdcRspInfoField *pRspInfo)
{
	if(!IsErrorRspInfo(pRspInfo) && pInputLock )
	{
		;
	}
}

//请求查询锁定
//请求查询的响应常常不正确, 此函数可用性差
int CTestTraderSpi::ReqQryLock(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcOrderSysIDType LockSysID)
{
	CThostFtdcQryLockField QryLock;

	memset(&QryLock, 0, sizeof(CThostFtdcQryLockField));
	strcpy(QryLock.BrokerID, m_pTestApp->m_brokerId);
	strcpy(QryLock.InvestorID, m_pTestApp->m_userId);
	strcpy(QryLock.InstrumentID, InstrumentID);
	strcpy(QryLock.ExchangeID, ExchangeID);
	strcpy(QryLock.LockSysID, LockSysID);

	m_pTestApp->m_requestId++;
    int ret = m_pUserApi->ReqQryLock(&QryLock, m_pTestApp->m_requestId);
	cerr << m_Prompt << " 请求 | 锁定/解锁报单查询..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

//请求查询锁定应答
//几次锁定解锁之后, 此应答通常不正确
void CTestTraderSpi::OnRspQryLock(CThostFtdcLockField *pLock, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pLock){
		string StatusMsg = StatusMessageCodeConvert(pLock->StatusMsg);

		cerr << m_Prompt << " 响应 | 锁定/解锁报单查询成功..." << ENDLINE
	         << " 锁定编号:" << pLock->LockSysID << ENDLINE
		     << " 锁定状态:" << pLock->LockStatus << ENDLINE
		     << " 状态信息:"<< StatusMsg << ENDLINE;
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);
}

//请求查询锁定证券仓位
int CTestTraderSpi::ReqQryLockPosition(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID)
{
	CThostFtdcQryLockPositionField QryLockPosition;

	memset(&QryLockPosition, 0, sizeof(CThostFtdcQryLockPositionField));
	strcpy(QryLockPosition.BrokerID, m_pTestApp->m_brokerId);
	strcpy(QryLockPosition.InvestorID, m_pTestApp->m_userId);
	strcpy(QryLockPosition.InstrumentID, InstrumentID);
	strcpy(QryLockPosition.ExchangeID, ExchangeID);

	m_pTestApp->m_requestId++;
    int ret = m_pUserApi->ReqQryLockPosition(&QryLockPosition, m_pTestApp->m_requestId);
	cerr << m_Prompt << " 请求 | 查询锁定证券仓位..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

//请求查询锁定证券仓位应答
void CTestTraderSpi::OnRspQryLockPosition(CThostFtdcLockPositionField *pLockPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pLockPosition){
		cerr << m_Prompt << " 响应 | 查询锁定证券仓位成功..." << ENDLINE
	         << " 证券代码:" << pLockPosition->InstrumentID << ENDLINE
		     << " 数量:" << pLockPosition->Volume << ENDLINE           //总的已锁定证券数量
		     << " 冻结数量:"<< pLockPosition->FrozenVolume << ENDLINE; //备兑持仓使用冻结的数量
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);
}

#else

///期权自对冲录入请求
int CTestTraderSpi::ReqOptionSelfCloseInsert(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, 
							 TThostFtdcVolumeType Volume, TThostFtdcOptSelfCloseFlagType OptSelfCloseFlag)
{
    CThostFtdcInputOptionSelfCloseField req; 
	memset(&req, 0, sizeof(CThostFtdcInputOptionSelfCloseField));

    strcpy(req.BrokerID, m_pTestApp->m_brokerId);  
    strcpy(req.InvestorID, m_pTestApp->m_userId);  
	strcpy(req.InstrumentID, InstrumentID);
	strcpy(req.OptionSelfCloseRef, m_orderRef);  //报单引用，将自对冲和普通报单同等对待
	int nextOrderRef = atoi(m_orderRef);
	sprintf(m_orderRef, "%d", ++nextOrderRef);
	strcpy(req.ExchangeID, ExchangeID);
    req.HedgeFlag = THOST_FTDC_HF_Speculation;    //必填;  
    req.Volume = Volume; //必填 
	req.OptSelfCloseFlag = OptSelfCloseFlag;

	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqOptionSelfCloseInsert(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt <<" 请求 | 发送期权自对冲请求..."<<((ret == 0)?"成功":"失败")<< ENDLINE;

	return ret;
}

///期权自对冲操作请求
int CTestTraderSpi::ReqOptionSelfCloseAction(TThostFtdcSequenceNoType BrokerOptionSelfCloseSeq)
{
	bool found=false; size_t i=0;
	for(i=0;i<m_OptionSelfCloseList.size();i++){
		if(m_OptionSelfCloseList[i].BrokerOptionSelfCloseSeq == BrokerOptionSelfCloseSeq){ 
			found = true; 
			break;
		}
	}
	if(!found)
	{
		cerr << m_Prompt <<" 请求 | 期权自对冲报单不存在."<<ENDLINE; 
		return -5;  //自定义的错误编码
	} 

	CThostFtdcInputOptionSelfCloseActionField req;
	memset(&req, 0, sizeof(req));
	req.OptionSelfCloseActionRef = m_OrderActionRef++; //复用报单撤单引用
	req.ActionFlag = THOST_FTDC_AF_Delete;  //操作标志 

	CThostFtdcOptionSelfCloseField *pOptionSelfClose = &m_OptionSelfCloseList[i];
	strcpy(req.BrokerID, pOptionSelfClose->BrokerID);   //经纪公司代码	
	strcpy(req.InvestorID, pOptionSelfClose->InvestorID);   //投资者代码
	strcpy(req.OptionSelfCloseRef, pOptionSelfClose->OptionSelfCloseRef);
	req.FrontID = pOptionSelfClose->FrontID;
	req.SessionID = pOptionSelfClose->SessionID;
	strcpy(req.ExchangeID, pOptionSelfClose->ExchangeID);
	strcpy(req.OptionSelfCloseSysID, pOptionSelfClose->OptionSelfCloseSysID);
	strcpy(req.UserID, pOptionSelfClose->UserID);
	strcpy(req.InstrumentID, pOptionSelfClose->InstrumentID);
	strcpy(req.InvestUnitID, pOptionSelfClose->InvestUnitID);

	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqOptionSelfCloseAction(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt << " 请求 | 发送期权自对冲撤单..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

///请求查询期权自对冲
int CTestTraderSpi::ReqQryOptionSelfClose(TThostFtdcSequenceNoType BrokerOptionSelfCloseSeq)
{
	bool found=false; size_t i=0;
	for(i=0;i<m_OptionSelfCloseList.size();i++){
		if(m_OptionSelfCloseList[i].BrokerOptionSelfCloseSeq == BrokerOptionSelfCloseSeq){ 
			found = true; 
			break;
		}
	}
	if(!found)
	{
		cerr << m_Prompt <<" 请求 | 期权自对冲报单不存在."<<ENDLINE; 
		return -5;  //自定义的错误编码
	} 

	CThostFtdcQryOptionSelfCloseField req;
	memset(&req, 0, sizeof(req));

	CThostFtdcOptionSelfCloseField *pOptionSelfClose = &m_OptionSelfCloseList[i];
	strcpy(req.BrokerID, pOptionSelfClose->BrokerID);   //经纪公司代码	
	strcpy(req.InvestorID, pOptionSelfClose->InvestorID);   //投资者代码
	strcpy(req.ExchangeID, pOptionSelfClose->ExchangeID);
	strcpy(req.OptionSelfCloseSysID, pOptionSelfClose->OptionSelfCloseSysID);
	strcpy(req.InstrumentID, pOptionSelfClose->InstrumentID);

	m_pTestApp->m_requestId++;
	int ret = m_pUserApi->ReqQryOptionSelfClose(&req, m_pTestApp->m_requestId);
	cerr << m_Prompt << " 请求 | 发送期权自对冲报单查询..." <<((ret == 0)?"成功":"失败") << ENDLINE;

	return ret;
}

///期权自对冲录入请求响应
void CTestTraderSpi::OnRspOptionSelfCloseInsert(CThostFtdcInputOptionSelfCloseField *pInputOptionSelfClose, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << __FUNCTION__ << ENDLINE;
	if( !IsErrorRspInfo(pRspInfo) && pInputOptionSelfClose){
		cerr << m_Prompt <<"响应 | 期权自对冲报单提交...期权自对冲引用:"
			 << pInputOptionSelfClose->OptionSelfCloseRef << ENDLINE;  
	}
	else if(pInputOptionSelfClose) {
		; //错误的自对冲报单THOST拒绝接受，用pInputOptionSelfClose和pRspInfo中的信息更新报单状态
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);	
}

///期权自对冲操作请求响应
void CTestTraderSpi::OnRspOptionSelfCloseAction(CThostFtdcInputOptionSelfCloseActionField *pInputOptionSelfCloseAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pInputOptionSelfCloseAction){
		cerr << m_Prompt << " 响应 | 期权自对冲撤单..."
			<< "交易所:"<<pInputOptionSelfCloseAction->ExchangeID
			<<" 期权自对冲操作编号:"<<pInputOptionSelfCloseAction->OptionSelfCloseSysID << ENDLINE;
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);
}

///请求查询期权自对冲响应
void CTestTraderSpi::OnRspQryOptionSelfClose(CThostFtdcOptionSelfCloseField *pOptionSelfClose, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pOptionSelfClose){
		string StatusMsg = StatusMessageCodeConvert(pOptionSelfClose->StatusMsg);

		cerr << m_Prompt << " 响应 | 期权自对冲报单查询..."
			<< "交易所代号:"<< pOptionSelfClose->ExchangeID
			<< " 期权自对冲操作编号:" << pOptionSelfClose->OptionSelfCloseSysID 
			<< " 状态信息:" << StatusMsg
			<< ENDLINE;
	}
	if(bIsLast) 
		SetEvent(m_pTestApp->m_hEvent);	
}

///期权自对冲录入错误回报
void CTestTraderSpi::OnErrRtnOptionSelfCloseInsert(CThostFtdcInputOptionSelfCloseField *pInputOptionSelfClose, CThostFtdcRspInfoField *pRspInfo)
{
	cerr << "--->>> " << __FUNCTION__ << ENDLINE;
	if(!IsErrorRspInfo(pRspInfo) && pInputOptionSelfClose)
	{
		;
	}
	SetEvent(m_pTestApp->m_hEvent);
}

///期权自对冲操作错误回报
void CTestTraderSpi::OnErrRtnOptionSelfCloseAction(CThostFtdcOptionSelfCloseActionField *pOptionSelfCloseAction, CThostFtdcRspInfoField *pRspInfo)
{
	cerr << "--->>> " << __FUNCTION__ << ENDLINE;
	if(!IsErrorRspInfo(pRspInfo) && pOptionSelfCloseAction)
	{
		;
	}
	SetEvent(m_pTestApp->m_hEvent);
}

///期权自对冲通知
void CTestTraderSpi::OnRtnOptionSelfClose(CThostFtdcOptionSelfCloseField *pOptionSelfClose)
{
	CThostFtdcOptionSelfCloseField OptionSelfClose;
	memcpy(&OptionSelfClose,  pOptionSelfClose, sizeof(CThostFtdcOptionSelfCloseField));
	bool founded = false;    
	size_t i = 0;
	for(i = 0; i < m_OptionSelfCloseList.size(); i++)
	{
		if(m_OptionSelfCloseList[i].BrokerOptionSelfCloseSeq == pOptionSelfClose->BrokerOptionSelfCloseSeq) 
		{
			founded=true;    
			break;
		}
	}
	if(founded)  
		memcpy(&m_OptionSelfCloseList[i], pOptionSelfClose, sizeof(CThostFtdcOptionSelfCloseField)); 
	else  
		m_OptionSelfCloseList.push_back(OptionSelfClose);

	cerr << m_Prompt << " 回报 | 期权自对冲报单已提交...期权自对冲引用:" << pOptionSelfClose->OptionSelfCloseRef << ENDLINE;
	cerr << "pOptionSelfClose->OrderSubmitStatus = " << pOptionSelfClose->OrderSubmitStatus << ENDLINE;
	cerr << "pOptionSelfClose->StatusMsg = " << StatusMessageCodeConvert(pOptionSelfClose->StatusMsg) << ENDLINE;

	SetEvent(m_pTestApp->m_hEvent);
}

#endif

void CTestTraderSpi::OnFrontDisconnected(int nReason)
{
	cerr << m_Prompt <<" 响应 | 交易前置连接中断..." 
	  << " reason=" << nReason << ENDLINE;
}
		
void CTestTraderSpi::OnHeartBeatWarning(int nTimeLapse)
{
	cerr << m_Prompt <<" 响应 | 交易前置心跳超时警告..." 
	  << " TimerLapse = " << nTimeLapse << ENDLINE;
}

void CTestTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	IsErrorRspInfo(pRspInfo);
}

bool CTestTraderSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	// 如果ErrorID != 0, 说明收到了错误的响应
	bool ret = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (ret){
		string ErrorMsg = StatusMessageCodeConvert(pRspInfo->ErrorMsg);
        cerr << m_Prompt <<" 响应 | " << ErrorMsg << ENDLINE;
    }
	return ret;
}

void CTestTraderSpi::PrintOrders()
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
			<<" 状态:"<<StatusMsg<<ENDLINE;
	}

	SetEvent(m_pTestApp->m_hEvent);
}

void CTestTraderSpi::PrintTrades()
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

	SetEvent(m_pTestApp->m_hEvent);
}

bool CTestTraderSpi::IsMyOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->FrontID == m_frontId) &&
			(pOrder->SessionID == m_sessionId) &&
			(strcmp(pOrder->OrderRef, m_orderRef) == 0));
}

bool CTestTraderSpi::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
			(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
			(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

char CTestTraderSpi::MapDirection(char src, bool toOrig=true){
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

char CTestTraderSpi::MapOffset(char src, bool toOrig=true){
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

