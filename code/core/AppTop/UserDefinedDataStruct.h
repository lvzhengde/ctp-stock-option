#ifndef _USER_DEFINED_DATA_STRUCT_H_
#define _USER_DEFINED_DATA_STRUCT_H_

#include <string>
#include <vector>
#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"
#include "MyEvent.h"

#ifndef WIN_CTP
#include <pthread.h>
#endif

using namespace std;
#pragma warning(disable : 4996)

//定义Linux和Windows通用的事件类型
#ifdef WIN_CTP
    typedef HANDLE CEventHandle;
	#define SleepMs(x) Sleep(x)   //sleep x milliseconds
    #define ENDLINE    endl
#else
    typedef CMyEvent* CEventHandle;
    #define SleepMs(x) usleep(x*1000)   //sleep x milliseconds	
	typedef pthread_rwlock_t SRWLOCK;
	typedef pthread_mutex_t CRITICAL_SECTION;
	typedef bool BOOL;
    #define FALSE false
    #define TRUE  true

    typedef unsigned int  DWORD;	
    //定义系统时间结构
    typedef struct _SYSTEMTIME {
        DWORD wYear;
        DWORD wMonth;
        DWORD wDayOfWeek;
        DWORD wDay;
        DWORD wHour;
        DWORD wMinute;
        DWORD wSecond;
        DWORD wMilliseconds;
    } SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

    #define ENDLINE    "\r\n"
#endif

struct CConfigItem
{
	string key;
	string value;
	bool   bDbRecordExist;
};

struct CRspInstrument {
	CThostFtdcInstrumentField Instrument;
	SYSTEMTIME CurTime;
	bool IsLast;
};

struct CRtnMarketData {
	CThostFtdcDepthMarketDataField DepthMarketData;
	SYSTEMTIME CurTime;
};

//自定义的日期结构
struct CDate
{
    int year;
    int month;
    int day;
};

struct CPricePDF
{
	double price;
	double probability;
};

struct CTradeItem
{
	//合约类型, 'u': 基础合约， 'c': 看涨期权， 'p'看跌期权
	char InstrumentType;
	//合约代码
	string InstrumentId;
	//最小报价单位
	double PriceTick;
	//交易价格, 如果<0，则按当前Ask/Bid价格+/- 10 PriceTick交易
	double Price;
	//交易数量
	size_t Amount;
	//买卖方向, 'b': 买入, 's': 卖出
	char Direction;
	//开平仓标志, 'o': 开仓, 'c': 平仓
	char OffsetFlag;

	////以下为策略计算时需要
	//期权到期日，格式为YYYY/MM/DD
	string ExpireDate;
	//期权执行价格
	double StrikePrice;
	//标的资产类型,//标的资产类型, 0: 商品期货, 1: 股指期货，2: ETF, 3: 股票
	size_t UnderlyingAssetType;
	//标的合约代码
	string UnderlyingInstId;
	//标的合约当前价格，<0 通过行情查得
	double UnderlyingPrice;
	//标的资产历史波动率，以百分比表示
	double Hv;
	//标的资产年化漂移率，以百分比表示
    double DriftRate;
	//年化无风险利率，以百分比表示
	double RiskFreeRate; 

	//指向行情节点的指针
	void* pMarketDataNode;
};

struct CPositionItem 
{
	//合约类型, 'u': 基础合约， 'c': 看涨期权， 'p'看跌期权
	char instrumentType;
	//交易方向, 'b': 买入， 's': 卖出
    char direction;
	//交易数量
	int  amount;    
	//期权执行价格
	double strikePrice; 
	//交易价格
	double price;  
	//期权到期日，格式为YYYY/MM/DD
	char expirationDate[20];    
	//期权到期日和当前日期相隔的天数
	int  diffDay;   

	//针对单个买入期权的希腊值和隐含波动率，计算得来
	double delta;
	double gamma;
	double vega;
	double theta;
	double rho;
	//以百分比表示的年化隐含波动率
	double iv;

	//指向行情节点的指针
	void* pMarketDataNode;
};

struct CUnderlyingInfo
{
	//标的资产类型, //标的资产类型, 0: 商品期货, 1: 股指期货，2: ETF, 3: 股票
	size_t UnderlyingAssetType;
	//标的资产当前价
	double currentPrice;   
	//标的资产历史波动率，以百分比表示
	double hv; 
	//标的资产漂移率，以百分比表示
	double driftRate;  
	//无风险利率，以百分比表示
	double riskFreeRate;    
};

struct CStrategyResult
{
	int    breakEvenPointNum;   //盈亏平衡点的个数
	double breakEvenPoint[10];  //最多可以有十个盈亏平衡点 
	double unchangedProfit;
	double maximProfit;
	double maximLoss; 
	double winProbability;
	double loseProbability; 
	double averageProfit;
	//整个策略的希腊值（是各个部分之和）
	double delta;
	double gamma;
	double vega;
	double theta;
	double rho;
};

struct CProfitData
{
	double price;          //标的价格
	double probability;    //标的处于该价格的概率
	double profit;         //盈利
};

//日K线数据
struct CDayKlineItem {
	//开盘价
	double Open;
	//收盘价
	double Close;
	//最高价
	double High;
	//最低价
	double Low;
	//昨结算价
	double YdSettlement;
	//昨收盘价
	double YdClose;
	//当前K线日期
	CDate  DateOfKline;
	//当日平均隐含波动率
	double MeanIV;
	//当日平均隐含年化收益率
	double MeanRate;
};

//日内交易时段定义，时间表示为HHMMSS形式的浮点数
struct CSegTime
{
	//交易开始时间
	double SegBeginTime;
	//交易结束时间
	double SegEndTime;
};

//提供给策略使用的交易期权品种类型的详细说明
//注意每个策略都有这么一个OptionKind List
struct COptionKind
{
	//true: 只构造标的的报价链而去掉期权的T型报价, false: 构造完整的T型报价
	bool  bUnderMdOnly;
	//期权代号，"m"指豆粕期权
	string OptionId;  
	//标的或标的替代的代号，如IO对应IF, m对应m
	string UnderlyingId;
	//标的资产类型, 0: 商品期货, 1: 股指期货，2: ETF, 3: 股票
	size_t UnderlyingAssetType;
	//交易所代码, 郑商所: CZCE, 大商所: DCE, 上期所: SHFE, 中金所: CFFEX, 上交所: SSE, 深交所: SZSE
	TThostFtdcExchangeIDType ExchangeId;   
	//期权柜台代码
	char OptCntCode;
	//标的柜台代码
	char UnderCntCode;
	//标的商品的缺省波动率设置，百分比表示
	double UnderlyingDefaultHv;
	//当前标的商品的年化漂移率（可以认为和通货膨胀率相当), 以百分比表示
	double UnderlyingDriftRate;
	//期权和标的期货的比例对应关系OptionMultiple手期权对应UnderlyingMultiple手期货
	//主要用于股指期权的情形，对应商品期权一般是1:1关系
	int OptionMultiple;
	int UnderlyingMultiple;
	//需要交易的期权年月列表，YYYYMM形式，前4位表示年份，后两位为月份，如果为空则是所有可能的期权合约年月
	vector<size_t> YearMonthList;    
	//该期权的交易时段信息
	vector<CSegTime> SegTimeList;
	//为避免大头针效应，在期权到期日前停止开仓操作的天数（只许平仓，不准开仓）
	int StopBeforeExpire;
	//策略统计单方向的分段数
	int SingleSideNum;
	//策略统计每个区段对应的盈利收入数量（通常为PriceTick*VolumeMultiple)
	double DeltaProfit;
	//策略交易时滑点控制因子，滑点是PriceTick的整数倍，计算盈利时应当考虑扣除滑点因素
	//套利策略很大程度上靠Ask-Bid价差赚钱, SlipFactor也反映了此盈利基础的基本值
	double SlipFactor;
	//策略交易时认为平均HIT_COUNT_COEF个HIT_COUNT(Tick数目)对应一次价格出现机会
	double HitCountCoef;
	//策略交易使用标准差的倍数
	double StdDevMultiple;
	//是否使用市价报单进行交易
	bool bUseMarketOrder;
	//市价单支持情况, 0: 期权标的都支持市价单, 1: 仅标的支持市价单, 2: 仅期权支持市价单, 其它情况： 标的期权均不支持市价单
	//只是在报单的时候区别对待, 其它情况仍旧认为支持市价单
	int MarketOrderType;
	//本期权合约产品报单时价格比盘口价格优惠的PriceTick的倍数，使成交更容易
	int PriceTickMultiple;
    //下FISH单时相对于本方最优价位移的Tick数量
	int FishPriceTickMultiple; 
	//取消FISH单时的位移Tick数量, 避免频繁撤单可能要大一些
	int CancelFishPriceTickMultiple; 
	//单个合约最大撤单次数限制
	int MaxCancelTimes; 
	//开仓时是否只允许FISH单, 不准跨过Ask-Bid线主动交易
	bool bFishOrderOnly;
	//运用统计分析数据时所需要的最小统计天数
	int MinStatDays;
	//每个组合允许下FISH单的最大数量
	size_t MaxFishPerPortfolio; 
	//是否允许同一合约相反方向的交易
	bool bPermitReverseTrade;
	//本期权代码允许的最大未平仓组合数目
	int MaxUnClosedPortfolio;
	//计算盈利时平仓所需的最小天数，需要避免此值过小导致算得的预期收益率过大而实际收入却很小
	double MinCloseDays;
	//报单至撤单的最小时间间隔(以秒为单位), 只限于开仓条件不满足主动撤单的情形
	double MinOrderToCancelSecs;

	//使用构造函数初始化结构体
	COptionKind()
	{
		//针对各个成员初始化
		UnderlyingDefaultHv = 15;
		UnderlyingDriftRate = 3;
		OptionMultiple = 1;
		UnderlyingMultiple = 1;
		HitCountCoef = 100;
		StdDevMultiple = 1.0;
		MarketOrderType = 0;
		PriceTickMultiple = 10;
		FishPriceTickMultiple = 0;
		CancelFishPriceTickMultiple = 10;
		MaxCancelTimes = 200;
		bFishOrderOnly = true;
	    MinStatDays = 1;
	    MaxFishPerPortfolio = 1; 
	    bPermitReverseTrade = false;
		MaxUnClosedPortfolio = 0;
		MinCloseDays = 1.0;
		MinOrderToCancelSecs = 0;
	}
};

//投资者持仓详细情况
//主要用于策略交易后的比对
//注意：持仓查询记录中的昨持仓是今天开盘前的一个初始值，不会因为平昨或者平仓而减少
//有意义的是今持仓和总持仓
struct CInvestorPositionDetail
{
	//通过CTP查询得到的投资者持仓明细
	CThostFtdcInvestorPositionField CtpInvestorPosition;
	//被明确宣布占有的总的持仓量
	int ClaimedPosition;
	//被策略明确宣布占有的昨日持仓数量，暂时不用
	int ClaimedYdPosition;
	//被策略明确宣布占有的今日持仓数量，暂时不用
	int ClaimedTodayPosition;
};

///组合或头寸完全平仓
#define Status_AllClosed '0'
///组合或头寸完全持仓
#define Status_AllOpened '1'
///组合或头寸开仓指令发出未完全确认
#define Status_OpenWaitForAck '2'
///组合或头寸平仓指令发出未完全确认
#define Status_CloseWaitForAck '3'
///组合尚未初始化
#define Status_UnInitialized '4'
///组合或头寸开仓超时确认出错
#define Status_OpenAckTimeout '5'
///组合或头寸平仓超时确认出错
#define Status_CloseAckTimeout '6'
///组合被拆散，适用于卷筒套利近月合约到期被行权的情况
#define Status_BrokenLegs '7'
///组合或头寸处于开仓FISH单状态
#define Status_FishOpen   '8'
///组合或头寸处于平仓FISH单状态
#define Status_FishClose  '9'
///组合或头寸处于开仓FISH单撤单状态
#define Status_FishOpenCancel  'a'
///组合或头寸处于平仓FISH单撤单状态
#define Status_FishCloseCancel 'b'
//头寸开仓FISH单已经成交
#define Status_FishOpenTraded 'c'
//头寸平仓FISH单已经成交
#define Status_FishCloseTraded 'd'

//当前数据和数据库一致
#define SyncDb_Same_Data   '0'
//数据库中还没有此数据
#define SyncDb_New_Data    '1'
//数据相对数据库已经更新
#define SyncDb_Update_Data '2'

#endif //_USER_DEFINED_DATA_STRUCT_H_
