/*++
    CTP交易柜台类
--*/

#ifndef _CTP_CNT_IF_H_
#define _CTP_CNT_IF_H_

#include "common.h"
#include "RealMdSpi.h"
#include "RealTraderSpi.h"
#include "MyFlowControl.h"
#include "AppConfig.h"

class CRealApp;

class CCtpCntIf
{
public:
	//指向RealApp的指针
	CRealApp *m_pRealApp;
	//指向流量控制对象的指针
	CMyFlowControl *m_pFlowControl;

	//交易柜台代码, 'o': 股票期权; 's': 股票现货; 'f': 期货及期货期权
	//在应用程序当中, 对每个柜台而言此代码是唯一的
	char m_CntCode;
	//经纪商报盘柜台名称(如ZXQH_SIM, ZXQH_TRUE...)
	string m_CounterName;
	//经纪商代码
	TThostFtdcBrokerIDType m_brokerId;	
	//投资者代码
	TThostFtdcUserIDType m_userId;
	//交易密码
	TThostFtdcPasswordType	m_Password;  
	//是否需要用户产品认证
	bool    m_bNeedAuth;  
	//用户端产品信息
	string	m_UserProductInfo;
	//用户软件代码
	string	m_AppID;
	//认证码
	string	m_AuthCode;     

	//从交易API获取的合约信息列表
	vector<CThostFtdcInstrumentField> m_InstInfoList;
	//从行情API获取的深度行情数据列表
	vector<CThostFtdcDepthMarketDataField> m_DepthMarketDataList;
	//用于访问深度市场行情的互斥信号
	CRITICAL_SECTION m_DepthMarketDataCS;
	//从TraderAPI获取的合约交易状态列表
	vector<CThostFtdcInstrumentStatusField> m_InstrumentStatusList;
	//用于访问合约交易状态的互斥信号
	CRITICAL_SECTION m_InstrumentStatusCS;

	//给交易API和行情API发送各种请求的当前Id
    int m_requestId; 
	//交易SPI设置的通知事件对象
    CEventHandle m_hTraderSpiEvent;
	//行情SPI设置的通知事件对象
	CEventHandle m_hMdSpiEvent;
    //合约状态信息改变的通知事件对象
	CEventHandle m_hInstrumentStatusEvent;

	//行情API指针
	CThostFtdcMdApi*  m_pMdUserApi;
	//行情User SPI指针
	CRealMdSpi*  m_pMdUserSpi; 
	//交易API指针
	CThostFtdcTraderApi*  m_pTraderUserApi;
	//交易User SPI指针
	CRealTraderSpi*  m_pTraderUserSpi;

	//本柜台保证金最大占用比率，风险控制用
	double m_MaxMarginRatio;

	//当前查询资金账户是否成功
	bool m_bReqAccountSuccess;
	//查询资金账户的时间戳, 和API接口线程解耦
	SYSTEMTIME m_ReqAccountTimestamp;
	//当前查询投资者持仓是否成功
	bool m_bReqPositionSuccess;
	//查询投资者持仓明细的时间戳, 和API接口线程解耦
	SYSTEMTIME m_ReqPositionTimestamp;
	//投资者持仓明细列表
	vector<CInvestorPositionDetail> m_InvestorPositionDetailList;
	//用于访问TraderSpi投资者持仓明细的互斥信号
	CRITICAL_SECTION m_InvestorPositionCS;
	//投资者资金账户明细
	CThostFtdcTradingAccountField m_TradingAccount;
	//用于访问TraderSpi投资者资金账户明细的互斥信号
	CRITICAL_SECTION m_TradingAccountCS;
	//RealTraderSpi中m_orderList的复制列表
	vector<CThostFtdcOrderField> m_DupOrderList;
	//用于访问m_orderList的互斥信号
	CRITICAL_SECTION m_OrderListCS;
	//已经被确认的最新报单的位置
	int m_OrderAckedPos;
	//RealTraderSpi中m_tradeList的复制列表
	vector<CThostFtdcTradeField> m_DupTradeList;
	//用于访问m_tradeList的互斥信号
	CRITICAL_SECTION m_TradeListCS;
	//已经被确认的最新成交的位置
	int m_TradeAckedPos;
	//根据资金查询和持仓查询计算得到的投资者总权益, 包括现金和股票/期权市值之和
	double m_TotalEquity;

	//更新资金情况和投资者持仓所用到的临时成员变量
	SYSTEMTIME m_prevTime;
	//上次更新持仓信息的时间
	double m_PrePositionSeconds;  
	//上次更新用户账户的时间
	double m_PreAccountSeconds;   
	//上次更新账户信息的序列号
	int m_PreAccountSeqNo;
	//当前更新账户信息的序列号
	int m_CurAccountSeqNo;
	//上次保存报单情况时的位置
	int m_PreOrderSavePos;
	//上次保存成交情况时的位置
	int m_PreTradeSavePos;

	//检查报单和撤单是否通过
	bool m_bCheckPlaceAndCancelOrder;
    //检查报单和撤单重试的次数
	int m_CheckRetryTimes;

public:
	//构造函数
	CCtpCntIf(CRealApp *pRealApp, string CounterName);

	//析构函数
	~CCtpCntIf();

	//从数据库中获取配置信息
	void GetConfigFromDb();

	//初始化CTP API
	void InitCtpApi(THOST_TE_RESUME_TYPE StartStyle = THOST_TERT_RESUME);

	//清理CTP API对象
	void CleanupCtpApi();

	//计算投资者总权益
	void CalcTotalEquity();

	//存储交易柜台账户信息到数据库
	void SaveTradingAccountToDb();

};

#endif
