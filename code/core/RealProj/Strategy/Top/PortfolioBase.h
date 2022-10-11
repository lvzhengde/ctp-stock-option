/*++
   投资组合的基类
   包括一些组合的基本成员变量如持仓交易明细列表，期权商品类型指针，交易数量等
   其基本的成员函数包括ClaimPosition， 计算预期利润，最终利润等
--*/


#ifndef _PORTFOLIO_BASE_H_
#define _PORTFOLIO_BASE_H_

#include "common.h"
#include "PositionTrade.h"
#include "TshapeBase.h"

class CRealApp;
class CStrategyBase;

class CPortfolioBase
{
public:
	//如果组合含有标的, 是否备兑开仓
	bool m_bCoveredOpen;
	//对应合约代码的年月份，对于跨月套利则是近月合约的年月份
	size_t m_YearMonth;
	//交易序列号，由交易日和组合序号构成，格式为"2017-03-10#1"，唯一标识一个组合交易
	//#后面跟的是PortfolioNumber
	string m_TradeSequence;
	//组合序号（每天从0开始计数)
	int m_PortfolioNumber;
	//开仓或者试图开仓时间戳
	SYSTEMTIME m_OpenTimestamp;
	//平仓或者试图平仓时间戳
	SYSTEMTIME m_CloseTimestamp;
	//交易所代码
	string m_ExchangeId;
	//期权代号，"m"指豆粕期权
	string m_OptionId;  
	//标的或标的替代的代号，如IO对应IF, m对应m
	string m_UnderlyingId;
	//开仓数量（基于组合基本单位）
	int m_Volume;
	//组合平仓盈亏
	double m_CloseProfit;
	//组合总的手续费
	double m_TotalCommission;
	//对应的COptionKind指针
	COptionKind* m_pOptionKind;
	//组合各组成合约部位交易明细列表
	vector<CPositionTrade*> m_pPositionTradeList;
	//组合目前的状态
	char m_PortfolioStatus;
	//和数据库保持同步的状态(和数据库一致，新数据，相对数据库有更新）
	char m_SyncDbStatus;

	//指向上一层结构的指针
	CStrategyBase* m_pParent;
	//指向App对象的指针
	CRealApp* m_pRealApp;

	//以下是用于开仓平仓的临时成员变量
	//可以开仓信号
	bool m_bOpenSignal;
	//可以平仓信号
	bool m_bCloseSignal;

	//一手交易对应的最小买卖价差，计入合约乘数等信息
	double m_MinAskBidGap;
	//一手交易对应的手续费（包括开仓和平仓），这个只是近似值（考虑到手续费可能按金额算）
	double m_UnitCommission;
	//一手交易对应的本金
	double m_PrincipalMoney;
	//一手交易估算的盈利，扣除掉买卖价差和手续费
	double m_UnitEstProfit;
	//期望平仓时可以获取的年化收益率
	double m_ExpectAnnualRate;
	//期望平仓时年化收益率对应的平仓天数
	double m_ExpectCloseDays;
	//平仓时的目标收入（指卖出价格减买入价格）
	double m_TargetCloseIncome;

	//部分头寸不是平仓而是等待被行权
	bool m_bExecuteReplaceClose;
	//只有部分头寸被平仓, 而其它头寸等待被行权时的目标平仓收益
	double m_ExecuteCloseIncome;

	//是否下FISH单
	bool m_bFishOrder;
	//组合中FISH单头寸的数量
	int m_FishOrderNum;

public:
	CPortfolioBase();
	virtual ~CPortfolioBase();

	//对从数据库中获取的组合做合法性检查
	virtual void ValidatePortfolio() = 0;

	//将组合的详细情况输出到文本文件
	virtual void DumpToTextFile(ofstream& OutFile) = 0;

	//将组合的最新情况写入数据库
	virtual bool SaveToDatabase(bool bForce) = 0;

	//手动出错恢复
	virtual void ManualErrorRecovery() = 0;

	//发送开仓报单
	virtual int PlaceOpenOrder();

	//发送平仓报单
	virtual int PlaceCloseOrder();

	//检查是否Fish状态冲突
	virtual int FishConflictStatus();

	//更新开仓判决条件
	virtual int UpdateOpenDecision(double& NetOpenIncome) = 0;

	//计算在新开仓条件下, FISH单是否能满足开仓要求
	virtual int UpdateOpenFish(int FishOrderIndex, double NetOpenIncome, double& NewProfit) = 0;

	//针对FISH单, 更新平仓判决条件
	virtual int UpdateCloseDecision(double& NetOpenIncome, double& CloseIncome) = 0;

	//计算在新平仓条件下, FISH单是否能满足平仓要求
	virtual int UpdateCloseFish(int FishOrderIndex, double NetOpenIncome, double CloseIncome) = 0;

	//处理开仓FISH单得到成交回报的情况
	void OpenFishTradeProc(int FishOrderIndex, double &AllFishCloseProfit, double &NewProfitCorrectVal, bool &bOtherPosiInCloseState);

	//处理开仓FISH单
	virtual int OpenFishHandler(bool bTradeError = false);

	//处理平仓FISH单
	virtual int CloseFishHandler(bool bTradeError = false);

	//宣称整个组合的部位占有数目
	virtual int ClaimPosition(char ClaimedStatus);

	//宣称整个组合中特定持仓方向的某个合约的部位占有数目
	virtual int ClaimPosition(string InstId, char Direction, char ClaimedStatus);

	//同步Portfolio和PositionTrade的状态
	virtual void SyncPortfolioAndPosition();

	//计算整个组合的手续费和盈利
	virtual void CalcCommissionAndProfit();

	//寻找组合各个部位对应的市场行情节点
	virtual void FindMarketDataNode();

	//交易确认操作
	virtual int TradeAcknowledge();

	//刷新时间戳
	void RefreshTimestamp(bool bIsOpenTimestamp);

	//组合到期检查
	virtual void ExpireCheck(ofstream& OutFile);

	//获取组合保证金占用
	virtual double GetMarginOccupy(char CntCode);

	//确认是否有最近平仓的标的合约, 用于涉及股票期权标的的策略组合平仓
	bool HasNewlyClosedUnderInst(double IntervalSecs, string InstId, CCtpCntIf* pCtpCntIf);

	//确认是否有新开仓的组合并且组合中有合约属于指定的交易柜台
	bool HasNewlyOpenedPortfolio(double IntervalSecs, CCtpCntIf* pCtpCntIf);

	//按ESC键时强制方向性组合进行选择性平仓
	void EscForceClose();

	//计算组合中头寸行权价离标的当前价格的最大距离比例
	double MaxDistanceRatio();
};

//比较两个组合的年化收益率和到期时间，用于开仓排序
bool CmpPortfolio(CPortfolioBase* pPortfolio1, CPortfolioBase* pPortfolio2);

#endif
