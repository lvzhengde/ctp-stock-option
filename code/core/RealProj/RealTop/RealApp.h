#ifndef _REAL_APP_H_
#define _REAL_APP_H_

#include "common.h"
#include "RealMdSpi.h"
#include "RealTraderSpi.h"
#include "AppConfig.h"
#include "TshapeCzce.h"
#include "TshapeDce.h"
#include "TradeDb.h"
#include "CtpCntIf.h"
#include "StrategysManage.h"
#include "MarketDataManage.h"
#include "PosMoneyManage.h"
#include "ExchangeRuleFilter.h"

class CRealApp
{
public:
	//交易架构的名称
	string m_ArchName;
	//交易柜台列表指针
	vector<CCtpCntIf*> m_pCtpCntIfList;
	//交易所规则限制过滤器指针列表
	vector<CExchangeRuleFilter*> m_pExchangeRuleFilterList;
	//各个柜台每秒最大报单数目
	int m_MaxOrderNumPerSecond;
	//判定卖1/卖2或者买1/买2之间价差是否过大的比列因子, 用于股票期权交易
	double m_BigGapScaleFactor;

	//当前是否为手动交易模式
	bool m_bManualTrade;
	//初始化是否结束的状态标志
	bool m_bInitDone;
	//当前是否在维护数据库
	bool m_bMaintainDb;
	//自动交易过程中是否出现错误
	bool m_bAutoTradeErr;

	//整个程序使用的数据库对象指针
	CTradeDb* m_pTradeDb;
	//策略管理对象指针
	CStrategysManage* m_pStrategysManage;
	//行情管理对象指针
	CMarketDataManage* m_pMarketDataManage;
	//持仓和资金管理对象指针
	CPosMoneyManage* m_pPosMoneyManage;

	//全局性的控制参数
	//市场无风险利率
	double m_RiskFreeRate;
	//交易时段开盘后不交易的一段时间，以秒计算。通常开盘时有较多的套利机会，此值设为0
	double m_TradeStartWindSec;
	//接近交易时段结束停止交易的时间窗口，以秒计算
	double m_TradeStopWindSec;
	//后处理的时间点列表, HHMMSS格式, 用浮点数表示
	vector<double> m_PostProcTimeList;

	//盘口报单量须大于m_TradeUnitMultiple*TradeUnit时才考虑交易，是流动性指标
	int m_TradeUnitMultiple;
	//开仓时估算本金资金是保证金占用的倍数，计算收益率时使用
	double m_MarginMultiple;
	//手动交易报单时价格比盘口价格优惠的PriceTick的倍数
	int m_ManTradePriceTickMultiple;
    //从报单发送到收到交易所响应所花费的最大来回时间（秒数）
	double m_OrderRoundTripTime;
	//成交回报须在m_TradeAckTimeout秒之内，不然超时出错
	double m_TradeAckTimeout;
	//开仓时需要考虑的超过无风险年化收益率的倍数，超过此收益率才开仓
    double m_YieldMultiple;
	//限制今天能够开仓的最大组合数目
	int m_MaxOpensToday;
	//是否重新通过API查询得到各种费率参数（保证金，手续费）
	bool m_bReQryRateParams;

	//开仓队列允许的最大组合数目
	size_t m_MaxOpenQueueSize;
	//平仓队列允许的最大组合数目
	size_t m_MaxCloseQueueSize;
	//同一合约, 同一买卖方向允许下FISH单的最大数目
	size_t m_MaxSameFishOrderNum;
	//当月合约到期前的限定天数, 在此天数内股票期权策略开仓受持有标的限制
	int m_LimitDaysBeforeExpire;
	//交易下单时，要求对手挂单量是总和报单量的倍数
	int m_VolumeScale;

	//后处理过程中用到的变量，设置最多有20个时间分段
	bool m_bProcessedSeg[20]; 
	SYSTEMTIME m_PreTime;
	//柜台输出信息的提示符
	string m_Prompt;    

	//退出程序的标志
	bool m_bExitApp;
	//检查报单和撤单是否通过
	bool m_bCheckPlaceAndCancelOrder;
	//交易引擎是否出现错误的标志
	bool m_bTradeEngineError;
	//是否强制交易引擎撤单
	bool m_bForceCancelOrder;
	//行情处理线程句柄(Windows)或事件对象(Linux)
	CEventHandle m_hMdThread;
	//数据处理线程的句柄(Windows)或事件对象(Linux)
	CEventHandle m_hDataThread;
	//交易引擎线程的句柄(Windows)或事件对象(Linux)
	CEventHandle m_hTradeThread;

	//用于行情线程通知数据处理线程的事件对象
	CEventHandle m_hMdToDataProc;
	//用于行情线程通知交易引擎线程的事件对象（暂时不用，可能扰乱报单通知）
	CEventHandle m_hMdToTradeEngine;
	//用于行情线程通知策略扫描线程（目前为主线程）的事件对象
	CEventHandle m_hMdToStrategyScan;

	//调试输出文件
#ifdef DEBUG_LOG_FILE
	ofstream m_DebugLogFile;
#endif

public:
	CRealApp(string strArchName = "");
	~CRealApp();

	//初始化对象指针
	void CreateObjects();

	//销毁对象
	void DestroyObjects();

	//RealApp顶层的初始化
	void InitializeApp();

	//从数据库中获取全局配置信息
	void GetGlobalConfigFromDb();

	//从数据库中获取交易所规则限制过滤器配置信息
	void GetExchangeRuleFilterConfigFromDb();

	//交易前的准备检查工作
	void PreTradeCheck();

	//手动交易和策略计算主程序
	void ManualTradeMain();

	//在自动交易程序运行前, 检查各个柜台的报单和撤单是否正常
	bool CheckPlaceAndCancelOrder();

	//获取用于报单撤单的合约行情和其它信息
	bool GetMdAndInstInfo(CCtpCntIf* pOptCtpCntIf, CThostFtdcDepthMarketDataField &Md, CThostFtdcInstrumentField* &pInstInfo);

	//自动交易主程序
	void AutoTradeMain();

	//退出自动交易的处理函数
	bool ExitAutoTradeProc(bool &bClearErrorFlag);

	//同步获取市场行情数据
	bool SyncDepthMarketData();

	//后处理，用于存储交易信息到数据库
	void PostProcessing(bool bForce = false);

#ifdef WIN_CTP	
	//响应控制台事件的处理程序
	static BOOL WINAPI ConsoleHandler(DWORD CEvent);

	//行情处理线程，用于更新T型报价并设置相关事件
	static unsigned __stdcall MarketDataProc(LPVOID param);

	//进行数据处理的线程函数, 主要用于收集统计数据和保存交易组合信息
	static unsigned __stdcall TradeDataProc(LPVOID param);

	//交易引擎线程函数
	static unsigned __stdcall TradeEngine(LPVOID param);
#else //Linux
	//响应控制台事件的处理程序
	static void ConsoleHandler(int signo);
    
	//行情处理线程，用于更新T型报价并设置相关事件
	static void* MarketDataProc(void* param);

	//进行数据处理的线程函数, 主要用于收集统计数据和保存交易组合信息
	static void* TradeDataProc(void* param);

	//交易引擎线程函数
	static void* TradeEngine(void* param);

	//设置当前线程为高优先级
	void SetCurrentThreadHighPriority(bool bSet);
#endif //WIN_CTP
};

//处理退出GUI程序的情况
void HandleExitGui(int signo);

#endif
