/*++
    空测试策略
--*/
#ifndef _STRATEGY_VOIDTEST_H_
#define _STRATEGY_VOIDTEST_H_

#include "StrategyBase.h"
#include "PortfolioVoidTest.h"

class CStrategyVoidTest: public CStrategyBase
{
//成员变量
public:

//成员函数
public:
	CStrategyVoidTest(CRealApp* pRealApp);
	virtual ~CStrategyVoidTest();

	//策略初始化
	virtual void Initialize();

	//策略退出时的清理工作
	virtual void Cleanup();

	//初始化统计对象，一般在程序初始化的最后调用
	virtual void InitializeStat();

	//更新策略统计数据
	virtual void UpdateStat();

	//将统计数据写入数据库，在交易时段结束时调用
	virtual bool DumpStatData();

	//从数据库中获取历史交易组合信息，为减少函数代码，用子函数实现
	void GetHistoryPortfolioFromDb();

    //扫描T型报价, 产生可以开仓的组合列表
	virtual bool ScanForOpen(vector<CPortfolioBase*>& pPortfolioList);

	//对ScanForOpen中产生的组合进行条件过滤
	void OpenFilter(CPortfolioVoidTest* pPortfolioToOpen);

	//判断是否有最近开仓组合其包含有与待开仓组合相同的合约并且计算结果为亏损
	bool HasRecentlyLostTrade(CPortfolioVoidTest* pPortfolioToOpen, double IntSecs);

    //扫描策略的所有已开未平组合, 产生可以平仓的组合列表
	virtual bool ScanForClose(vector<CPortfolioBase*>& pPortfolioList);
};

#endif

