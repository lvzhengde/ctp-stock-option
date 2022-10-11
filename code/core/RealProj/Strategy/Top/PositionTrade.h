/*++
   部位交易类
   包括一些部位交易的基本成员变量，如合约代码，开仓价格，方向等
   和Portfolio类，Strategy类，StrategyManage类， App类构成从底向上的分层关系
--*/


#ifndef _POSITION_TRADE_BASE_H_
#define _POSITION_TRADE_BASE_H_

#include "common.h"

class CRealApp;
class CCtpCntIf;
class CPortfolioBase;

class CPositionTrade
{
public:
	//合约类型, 'u': 基础合约， 'c': 看涨期权， 'p'看跌期权，只准使用小写字母
	char m_InstrumentType;
	//合约代码
	string m_InstId;
	//期权的行权价, 凸性套利时需要使用
	double m_StrikePrice;
	//标的的年月份，跨期套利时需要使用
	size_t m_YearMonth;
	//买卖方向， 'b'买持仓， 's'卖持仓，只准使用小写字母
	char m_Direction;
	//开仓数量(应该等于组合开仓Volume乘以对应的乘数Option/UnderlyingMultiple)
	int m_Volume;
	//当前持仓数量
	int m_OpenInterests;
	//开仓平均价格
	double m_OpenPrice;
	//平仓平均价格，如果小于等于0则是未平仓
	double m_ClosePrice;
	//开仓手续费
	double m_OpenCommission;
	//平仓手续费, 如果小于等于0则未平仓
	double m_CloseCommission;
	//平仓盈亏
	double m_CloseProfit;
	//头寸的最小买卖价差，开仓时用
	double m_MinAskBidGap;
	//交易一手头寸所需要的手续费, 开仓时用
	double m_UnitCommission;

	//开仓拆单的情况下，各个报单的数量
	CSafeVector<int> m_OpenVolList;
	//开仓各个报单调用的返回状态
	CSafeVector<int> m_OpenRetList;
	//开仓时报单引用列表（可能拆单成几个发送）
	//数组不能作为vector的数据类型，否则编译不会通过（fatal error)
	//vector<TThostFtdcOrderRefType> OpenOrderRefList;
	CSafeVector<int>  m_OpenOrderRefList;
	//开仓的FrontId列表
	CSafeVector<int> m_OpenFrontIdList;
	//开仓的SessionId列表
	CSafeVector<int> m_OpenSessionIdList;
	//开仓的报单列表
	CSafeVector<CThostFtdcOrderField> m_OpenOrderList;
	//开仓的成交列表
	CSafeVector<CThostFtdcTradeField> m_OpenTradeList;
	//平仓可能拆单，各个报单的数量
	CSafeVector<int> m_CloseVolList;
	//平仓各个报单调用的返回状态
	CSafeVector<int> m_CloseRetList;
	//平仓时报单引用列表
	//数组不能作为vector的数据类型，否则编译不会通过（fatal error)
	//vector<TThostFtdcOrderRefType> CloseOrderRefList;
	CSafeVector<int> m_CloseOrderRefList;
	//平仓的FrontId列表
	CSafeVector<int> m_CloseFrontIdList;
	//平仓的SessionId列表
	CSafeVector<int> m_CloseSessionIdList;
	//平仓的报单列表
	CSafeVector<CThostFtdcOrderField> m_CloseOrderList;
	//平仓的成交列表
	CSafeVector<CThostFtdcTradeField> m_CloseTradeList;

	//用于开仓FISH单处理的当前报单回报
	CThostFtdcOrderField m_OpenOrder;
	//用于开仓FISH单处理的当前成交回报
	CThostFtdcTradeField m_OpenTrade;	
	//用于平仓FISH单处理的当前报单回报
	CThostFtdcOrderField m_CloseOrder;
	//用于平仓FISH单处理的当前成交回报
	CThostFtdcTradeField m_CloseTrade;	

	//部位目前的状态, '0': 完全平仓， '1': 完全持仓， '2': 开仓指令发出未完全确认， 
	//'3': 平仓指令发出未完全确认，'4': 尚未初始化， '5': 开仓超时确认出错， '6'：平仓超时确认出错
	//其它为未定义状态
	char m_PositionStatus;
	//和数据库保持同步的状态(和数据库一致，新数据，相对数据库有更新）
	char m_SyncDbStatus;

	//开仓预期撮合成交的价格
	double m_OpenMatchPrice;
	//开仓下FISH单的价格
	double m_OpenFishPrice;
	//开仓下FISH单相对于撮合成交价格的收入增益（一手交易）
	double m_OpenFishIncrement;
	//开仓时对手报价是否处于正常状态
	bool m_bOpenAbnormalState;
	//平仓预期撮合成交的价格
	double m_CloseMatchPrice;
	//平仓下FISH单的价格
	double m_CloseFishPrice;
	//平仓下FISH单相对于撮合成交价格的收入增益(一手交易）
	double m_CloseFishIncrement;
	//平仓时对手报价是否处于正常状态
	bool m_bCloseAbnormalState;
	//计算收入时的合约乘数, 这个和查询合约信息得到的结果一致
	double m_VolumeMultiple;
	//一手交易所占用的保证金
	double m_MarginOccupy;
	//是否下FISH单
	bool m_bFishOrder;
	//撤销FISH单的时间戳
	SYSTEMTIME m_FishCancelTime;
	//从第一次开仓/平仓撤单到现在所经历的时间(是多次撤单经历时间的累加值)
	double m_TimeSinceFirstCancel;
	//最近下单时间, 用于超时确认
	SYSTEMTIME m_OrderPlaceTime;
	//上次查询报单时的OderRef值, 避免频繁查询用（每秒不能超过两次）
	int m_PreQryOrderRef;
	//是否已经执行自动行权
	bool m_bAutoExecOption;
	//等待行权代替平仓
	bool m_bExecuteReplaceClose;

	//指向市场节点的指针
	void* m_pMarketDataNode;
	//指向合约信息结构的指针
	CThostFtdcInstrumentField* m_pInstInfo;
	//指向撤单次数的指针
	int* m_pCancelTimes;
	//指向上一级结构的指针
	CPortfolioBase* m_pParent;
	//指向交易柜台的指针
	CCtpCntIf* m_pCtpCntIf;
	//指向应用对象的指针
	CRealApp* m_pRealApp;

public:
	CPositionTrade();
	~CPositionTrade() {};

	//宣称部位的占有数目
	int ClaimPosition(char ClaimedStatus);

	//计算手续费和盈利
	void CalcCommissionAndProfit();

	//寻找部位对应的市场行情节点
	void FindMarketDataNode();

	//获取交易柜台指针
	void GetCtpCntIf();

	//将头寸的详细情况输出到文本文件
	void DumpToTextFile(ofstream& OutFile);

	//将头寸的最新情况写入数据库
	bool SaveToDatabase(bool bForce);

	//获取头寸交易中需要使用的各种相关信息
	void GetPositionInfo(CThostFtdcDepthMarketDataField &Md, size_t &TickCountStamp, TThostFtdcInstrumentStatusType &InstrumentStatus);

	//获取加锁同步后的对应T型报价节点市场行情数据
	CThostFtdcDepthMarketDataField GetLockedMarketData();

	//获取加锁同步后的对应T型报价节点合约交易状态信息
	TThostFtdcInstrumentStatusType GetLockedInstrumentStatus();

	//决定开仓相关参数
	void OpenDecision();

	//更新开仓参数
    void UpdateOpenDecision(int PortfolioFishOrderNum = 1);

	//决定平仓相关参数
	void CloseDecision();

	//更新平仓参数
	void UpdateCloseDecision(int PortfolioFishOrderNum = 1);

	//发送开仓报单
	int PlaceOpenOrder(bool bForceIoc = false, int ForceVol = 1);

	//发送平仓报单
	int PlaceCloseOrder(bool bForceIoc = false, int ForceVol = 1);

	//锁定备兑证券
	int LockSecurities();

	//解锁备兑证券
	int UnlockSecurities();

	//发送IOC或者GFD报单
	int ReqOrderInsert(TThostFtdcInstrumentIDType instId, TThostFtdcDirectionType dir, TThostFtdcCombOffsetFlagType kpp, 
		TThostFtdcPriceType price, TThostFtdcVolumeType vol, char style);

	//期权到期日自动行权
	void AutoExecOptionOnExpireDay();

	//获取开仓时报单成交相关信息
	void RtnOpen(CThostFtdcOrderField &RtnOrder, CThostFtdcTradeField &RtnTrade);

	//获取平仓时报单成交相关信息
	void RtnClose(CThostFtdcOrderField &RtnOrder, CThostFtdcTradeField &RtnTrade);

	//撤销FISH单报单
    int CancelFishOrder();

	//报单查询请求
	int ReqQryOrder();

	//判定是否今仓，供上期所平仓用
	bool IsShfeTodayPosition(SYSTEMTIME OpenTimestamp);
	
	//交易确认操作
	int TradeAcknowledge();

	//手动出错恢复
	void ManualErrorRecovery();

	//对头寸的状态进行合法性检查
	bool CheckStatus();

	//供SaveToDatabase及TradeAcknowledge函数使用, 判断是否处于FISH单状态
	bool StillInFishStatus(string strCallFuncName);
};

//比较两个头寸的开仓FISH单收益
bool CmpOpenFishInc(CPositionTrade* pTrade1, CPositionTrade* pTrade2);
//比较两个头寸的平仓FISH单收益
bool CmpCloseFishInc(CPositionTrade* pTrade1, CPositionTrade* pTrade2);

#endif
