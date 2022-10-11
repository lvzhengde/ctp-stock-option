#ifndef _REAL_TRADERSPI_H_
#define _REAL_TRADERSPI_H_

#include "common.h"

class CRealApp;
class CCtpCntIf;

class CRealTraderSpi : public CThostFtdcTraderSpi
{
public:
    CRealTraderSpi(CThostFtdcTraderApi* api, CRealApp* pApp, CCtpCntIf* pCtpCntIf);

	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected();

		///客户端认证响应
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///投资者结算结果确认响应
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, 
		int nRequestID, bool bIsLast);
	
	///请求查询合约响应
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询资金账户响应
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者持仓响应
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询合约保证金率响应
	virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询合约手续费率响应
	virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询期权合约手续费响应
	virtual void OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询经纪公司交易参数响应
	virtual void OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询期权交易成本响应
	virtual void OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///报单录入请求响应
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///报单操作请求响应
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///报单查询请求相应
	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询成交响应
	virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///期权执行宣告录入请求响应
	virtual void OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///执行宣告操作请求响应
	virtual void OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///期权执行宣告通知
	virtual void OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder);

	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	virtual void OnFrontDisconnected(int nReason);
		
	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	virtual void OnHeartBeatWarning(int nTimeLapse);
	
	///报单通知
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

	///成交通知
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

	///合约交易状态通知
	virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus);

#ifndef FUTURE_OPTION_ONLY
	///请求查询ETF期权合约手续费响应
	virtual void OnRspQryETFOptionInstrCommRate(CThostFtdcETFOptionInstrCommRateField *pETFOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	///锁定通知
	virtual void OnRtnLock(CThostFtdcLockField *pLock);

	///锁定错误通知
	virtual void OnErrRtnLockInsert(CThostFtdcInputLockField *pInputLock, CThostFtdcRspInfoField *pRspInfo);

	///锁定应答
	virtual void OnRspLockInsert(CThostFtdcInputLockField *pInputLock, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询锁定应答
	virtual void OnRspQryLock(CThostFtdcLockField *pLock, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询锁定证券仓位应答
	virtual void OnRspQryLockPosition(CThostFtdcLockPositionField *pLockPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
#endif

public:
	///用户登录请求
	int ReqUserLogin(TThostFtdcBrokerIDType	brokerId,
	        TThostFtdcUserIDType	userId,	TThostFtdcPasswordType	passwd);

#ifdef SEE_THROUGH_SUPERVISION
	int ReqAuthenticate(TThostFtdcBrokerIDType	BrokerId, TThostFtdcUserIDType	UserId, TThostFtdcProductInfoType	UserProductInfo,
		    TThostFtdcAppIDType	AppID, TThostFtdcAuthCodeType	AuthCode);
#else
	///客户端认证请求
	int ReqAuthenticate(TThostFtdcBrokerIDType	BrokerId, TThostFtdcUserIDType	UserId, TThostFtdcProductInfoType	UserProductInfo,
		    TThostFtdcAuthCodeType	AuthCode);
#endif

	///登出请求
	int ReqUserLogout(TThostFtdcBrokerIDType	brokerId, TThostFtdcUserIDType	userId);

	///投资者结算结果确认
	int ReqSettlementInfoConfirm();

	///请求查询合约
	int ReqQryInstrument(TThostFtdcInstrumentIDType instId);

	///请求查询资金账户
	int ReqQryTradingAccount();

	///请求查询投资者持仓
	int ReqQryInvestorPosition(TThostFtdcInstrumentIDType instId);

	///请求查询合约保证金率
	int ReqQryInstrumentMarginRate(TThostFtdcInstrumentIDType  InstrumentID);

	///请求查询合约手续费率
	int ReqQryInstrumentCommissionRate(TThostFtdcInstrumentIDType  InstrumentID);

	///请求查询期权合约手续费
	int ReqQryOptionInstrCommRate(TThostFtdcInstrumentIDType  InstrumentID);

	///请求查询经纪公司交易参数，计算期权保证金时有用
	int ReqQryBrokerTradingParams();

	///请求查询期权交易成本
	int ReqQryOptionInstrTradeCost(	TThostFtdcInstrumentIDType	InstrumentID, TThostFtdcPriceType	InputPrice, TThostFtdcPriceType	UnderlyingPrice);

	///报单录入请求
    int ReqOrderInsert(TThostFtdcInstrumentIDType instId,
        TThostFtdcDirectionType dir, TThostFtdcCombOffsetFlagType kpp,
        TThostFtdcPriceType price,   TThostFtdcVolumeType vol, 
	    char TimeCondition, char VolumeCondition, char MarketOrderType, 
		TThostFtdcHedgeFlagType HedgeFlag = THOST_FTDC_HF_Speculation,
		string strExchangeID = "");

	//撤单请求, 使用对应报单字段来填充撤单字段
	int ReqOrderAction(CThostFtdcOrderField *pOrder, int FrontId = 0, int SessionId = 0, int OrderRef = 0, string strInstId = "", string strExchangeID = "");

	//期权行权录入请求
	int ReqExecOrderInsert(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcVolumeType	Volume);

	///执行宣告操作请求/期权行权撤销
	int ReqExecOrderAction(	TThostFtdcSequenceNoType	brokerExecOrderSeq);

#ifndef FUTURE_OPTION_ONLY
	///请求查询ETF期权合约手续费
	int ReqQryETFOptionInstrCommRate(TThostFtdcInstrumentIDType  InstrumentID);

	///锁定/解锁备兑证券请求
	int ReqLockInsert(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcLockTypeType LockType, TThostFtdcVolumeType Volume);

	//请求查询锁定
	int ReqQryLock(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcOrderSysIDType LockSysID);

	//请求查询锁定证券仓位
	int ReqQryLockPosition(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID);
#endif

	//报单查询请求
	int ReqQryOrder(CThostFtdcOrderField *pOrder);

	///请求查询成交
	int ReqQryTrade(TThostFtdcInstrumentIDType InstrumentID, TThostFtdcExchangeIDType ExchangeID, TThostFtdcTradeIDType TradeID);

	// 是否收到成功的响应
	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);

	// 是否我的报单回报
	bool IsMyOrder(CThostFtdcOrderField *pOrder);

	// 是否正在交易的报单
	bool IsTradingOrder(CThostFtdcOrderField *pOrder);

	void PrintOrders();
	void PrintTrades();

	char MapDirection(char src, bool toOrig);
	char MapOffset(char src, bool toOrig);

private:
	CThostFtdcTraderApi* m_pUserApi;
	CRealApp* m_pRealApp;
	CCtpCntIf* m_pCtpCntIf;

	//API输出信息的提示符
	string m_Prompt;    

public:
	//投资者是否成功登录
	bool m_bSuccessLogin;
	//前置编号
	int	 m_frontId;
	//会话编号
	int	 m_sessionId;	
	//报单引用
	TThostFtdcOrderRefType m_orderRef;
	//报单操作引用(撤单用)
	int m_OrderActionRef;

	//合约信息列表
	vector<CThostFtdcInstrumentField> m_InstInfoList;
	//报单列表
	vector<CThostFtdcOrderField> m_orderList;
	//成交列表
	vector<CThostFtdcTradeField> m_tradeList;
	//行权报单列表
	vector<CThostFtdcExecOrderField> m_execOrderList;
    //合约交易状态信息列表
	vector<CThostFtdcInstrumentStatusField> m_InstrumentStatusList;

	//用来存储临时查询结果的成员变量
	//保证金率
	CThostFtdcInstrumentMarginRateField m_InstrumentMarginRate;
	//经纪商参数
	CThostFtdcBrokerTradingParamsField m_BrokerTradingParams;
	//合约手续费率
	CThostFtdcInstrumentCommissionRateField m_InstrumentCommissionRate;
	//期权手续费率
	CThostFtdcOptionInstrCommRateField m_OptionInstrCommRate;
	//ETF期权手续费率，分多钟情况，为列表形式
#ifndef FUTURE_OPTION_ONLY
	vector<CThostFtdcETFOptionInstrCommRateField> m_ETFOptionInstrCommRateList;
	//通知备兑证券锁定/解锁的事件对象
	CEventHandle m_hLockEvent;
	//备兑证券锁定/解锁报单列表
	vector<CThostFtdcLockField> m_lockOrderList;
	//请求查询锁定证券仓位应答结果
	CThostFtdcLockPositionField m_LockPosition;
#endif

	//查询资金账户的时间戳
	SYSTEMTIME m_ReqAccountTimestamp;
	//投资者资金账户明细
	CThostFtdcTradingAccountField m_TradingAccount;
	//查询投资者持仓明细的时间戳
	SYSTEMTIME m_ReqPositionTimestamp;
	//投资者持仓明细列表
	vector<CThostFtdcInvestorPositionField> m_InvestorPositionList;
	//收集投资者持仓明细的列表
	vector<CThostFtdcInvestorPositionField> m_CollectInvestorPositionList;

	//是否发生报单错误
	bool m_bOrderInsertError;
	//是否发生撤单错误
	bool m_bOrderActionError;
	//是否发生锁定/解锁错误
	bool m_bLockInsertError;
};

#endif
