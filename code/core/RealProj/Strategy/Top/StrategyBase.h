/*++
    运行策略的基类
	针对各种策略，分别派生出子类
    每一个策略，都会针对一个或者数个期权品种进行，各品种有不同年月份的合约和交易时段，这个可定义在父类
	具体策略，有不同的组合持仓情况，在派生类中定义
	策略有一些共通的函数，如初始化，开仓机会查找，平仓机会查找，开仓，平仓，资金和仓位管理，数据库操作，退出时的清理工作等
	这些定义为纯虚函数
--*/


#ifndef _STRATEGY_BASE_H_
#define _STRATEGY_BASE_H_

#include "common.h"
#include "PortfolioBase.h"

class CRealApp;

class CStrategyBase
{
public:
	//组合列表
	CSafeVector<CPortfolioBase*> m_pPortfolioList;
	//允许策略未平仓组合数目大小限制
	int m_UnClosedSizeLimit;
    //策略当日交易所允许的最多亏损次数（只限于本次程序运行不中断的情形）
	int m_MaxLostTradesToday;
	//策略当日交易所允许的最大亏损（只限于本次程序运行不中断的情形）
	double m_MaxLoss;
	//是否不受开/平仓交易队列大小的限制
	bool m_bUnLimitedByTradeQueueSize;
	//是否是无风险套利组合或者价差组合, 如果为否退出时可能需要强制平仓
	bool m_bArbOrSpread;
	//用于UpdateStat
	double m_prevStatTimeSec;
	//上次本策略开仓扫描完成时DbgView显示的时间秒数
	double m_PreScanForOpenSec;
	//上次本策略平仓扫描完成时DbgView显示的时间秒数
	double m_PreScanForCloseSec;

	//策略名称
	string m_StrategyName;
	//策略交易基本单位
	size_t m_TradeUnit;
	//组合交易计数
	int m_PortfolioCount;
    //提供给策略使用的交易期权类型的详细说明列表
	vector<COptionKind> m_OptionKindList;

	//指向主应用对象的指针
	CRealApp* m_pRealApp;

public:
	CStrategyBase();
	virtual ~CStrategyBase();

	//策略初始化
	virtual void Initialize() = 0;

	//策略退出时的清理工作
	virtual void Cleanup() = 0;

	//初始化统计对象，一般在程序初始化的最后调用
	virtual void InitializeStat() = 0;

	//更新策略统计数据
	virtual void UpdateStat() = 0;

	//将统计数据写入数据库，在交易时段结束时调用
	virtual bool DumpStatData() = 0;

    //扫描T型报价, 产生可以开仓的组合列表
	virtual bool ScanForOpen(vector<CPortfolioBase*>& pPortfolioList) = 0;

    //扫描策略的所有已开未平组合, 产生可以平仓的组合列表
	virtual bool ScanForClose(vector<CPortfolioBase*>& pPortfolioList) = 0;

	//宣称整个策略的部位占有数目
	virtual int ClaimPosition(char ClaimedStatus);

	//宣称整个策略中特定持仓方向的某个合约的部位占有数目
	virtual int ClaimPosition(string InstId, char Direction, char ClaimedStatus);

	//同步Portfolio和PositionTrade的状态
	virtual void SyncPortfolioAndPosition();

	//寻找整个策略中组合各个部位对应的市场行情节点
	virtual void FindMarketDataNode();

	//将整个策略的详细情况输出到文本文件
	virtual void DumpToTextFile(ofstream& OutFile);

	//初始化PortfolioCount并抽取出从数据库中得到的交易组合的PortfolioNumber
	virtual void InitPortfolioCount();

	//将策略所有组合的最新情况写入数据库
	virtual bool SaveToDatabase(bool bForce = false);

	//清除策略组合队列中已经平仓并且保存完毕的组合
	void PurgeClosedPortfolio();
	  
	//计算策略未平仓的组合数目
	virtual int CountUnClosedPortfolio(COptionKind *pOptionKind = NULL);

	//计算本策略当日亏损的交易次数
	virtual int CountLostTradesToday();

	//计算股票期权非当月到期组合持仓中对应标的的数量总和
	int CountAvailSoptUnderAmount(CTshapeBase* pTshapeBase);

	//计算股票期权当月到期无标的对冲策略组合单边期权持仓数量
	int CountSoptCurMonOptionNum(CTshapeBase* pTshapeBase);

	//判断股票期权当月合约临近到期组合是否可以开仓的函数
	void OpenFilterNearExpire(CPortfolioBase* pPortfolio);

	//判断股票期权当月合约临近到期含有标的组合是否可以平仓的函数
	void CloseFilterNearExpire(CPortfolioBase* pPortfolio);

	//排序和删减策略ScanForOpen函数得到的可开仓组合列表
	virtual void SortAndTrunc(vector<CPortfolioBase*>& pPortfolioList);

	//移除欲加入开仓队列的超过对应OptionKind规定的MaxUnClosedPortfolio数目的组合
	void RemoveRedundantPortfolio(vector<CPortfolioBase*>& pPortfolioList);

	//按ESC键时强制方向性组合进行选择性平仓
	void EscForceClose();

	//在DbgView中显示开平仓结束信息, 策略诊断用
	void ScanDbgViewDisplay(char OffsetFlag = 0);

	//根据平均隐含波动率计算标的到期时的价格分布
	void GetUnderPricePDF(CTshapeMonth* pTshapeMonth, vector<CPricePDF>& PricePDFList);

	//计算当日有多少个开仓未平组合
	int CountOpensToday();

	//累计当日已完成交易的组合的交易盈利
	double AccumulateTodayTradeProfit();

	//确定策略涉及的各个柜台是否报撤单数目超标
	bool IsExceedOrderNum(COptionKind *pOptionKind, int CntType);
};

#endif
