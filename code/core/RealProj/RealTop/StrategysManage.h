/*++
    管理各种策略运行的顶层类
--*/


#ifndef _STRATEGYS_MANAGE_H_
#define _STRATEGYS_MANAGE_H_

#include "common.h"
#include "StrategyBase.h"
#include "StrategyVoidTest.h"

class CRealApp;

class CStrategysManage
{
public:
	//策略指针列表
	vector<CStrategyBase*> m_pStrategyBaseList;
    //综合各种策略使用的交易期权类型列表
	vector<COptionKind> m_SynOptionKindList;
	//上次刷新交易信息存入数据库的时间秒数
	double m_prevDbSaveTimeSecs;
	//上次清理已平仓组合的时间秒数
	double m_prePurgeTimeSecs;
	//指向主应用对象的指针
	CRealApp* m_pRealApp;

	//正在进行开仓交易的套利组合指针列表
	CSafeVector<CPortfolioBase*> m_pPortfInOpenList;
	//正在进行平仓交易的套利组合指针列表
	CSafeVector<CPortfolioBase*> m_pPortfInCloseList;

public:
	CStrategysManage(CRealApp* pRealApp);
	~CStrategysManage();

	//初始化策略管理对象
	void Initialize();

	//退出前的清理工作
	void Cleanup();

	//初始化统计对象，一般在程序初始化的最后调用
	void InitializeStat();

	//更新策略统计数据
	void UpdateStat();

	//将统计数据写入数据库，在交易时段结束时调用
	bool DumpStatData();

	//宣称所有策略的部位占有数目
	int ClaimPosition();

	//宣称所有策略中特定持仓方向的某个合约的部位占有数目
	int ClaimPosition(string InstId, char Direction);

	//同步所有策略中Portfolio和PositionTrade的状态
	void SyncPortfolioAndPosition();

	//寻找所有策略中各个组合部位对应的市场行情节点
	void FindMarketDataNode();

	//将各个策略组合持仓的详细情况输出到文本文件
	void DumpPortfolioToTextFile();

	//将所有策略的最新情况写入数据库
	void SaveToDatabase(bool bForce = false);

	//清除各个策略的组合队列中已经平仓并且不需要保存的组合
	void PurgeClosedPortfolio();

	//确定欲开/平仓组合各合约对手报单数量满足要求
	bool VolumeSatisified(CPortfolioBase* pPortfolio, char OffsetFlag);

	//搜寻开/平仓交易列表, 确定当前包含FISH单的组合是否可以交易
	bool FiltFishInTradeQueue(CPortfolioBase* pPortfolio);

	//查找开仓队列, 寻找是否有冲突的开仓组合
	bool IsConflictInOpenQueue(CPortfolioBase* pPortfolio);

	//获取开仓队列的保证金占用
	double GetMarginOccupyInOpenQueue(char CntCode);

	//获取未平仓组合的保证金之和
	double SumUnClosedMargin(char CntCode);

	//判读是否有柜台保证金占用已经超过阈值
	bool MarginExceedThreshold();

	//添加新开仓组合到列表
	int AddToOpenQueue();

	//添加新平仓组合到列表
	int AddToCloseQueue();

	//处理开平仓组合列表中的FISH单
	int FishOrderProc(bool bTradeError = false);

	//交易确认操作
	int TradeAcknowledge();

	//计算所有策略未平仓的组合数目
	int CountUnClosedPortfolio();

	//计算当日所有策略已经开仓的组合数目之和
	int CountOpensToday();

	//检查各策略的组合是否有快到期的情况
	void ExpireCheck();

	//显示开/平仓队列的状况, 供出错时调试使用
	void ShowQueueStatus(bool bNeedLock = true);

	//计算股票期权非当月到期策略组合持仓中对应标的的数量总和
	int CountAvailSoptUnderAmount(CTshapeBase* pTshapeBase);

	//计算股票期权当月到期无标的对冲策略组合单边期权持仓数量
	int CountSoptCurMonOptionNum(CTshapeBase* pTshapeBase);

	//扫描各个策略组合, 查看是否需要自动行权
	void CheckAutoExecOption();
};

#endif
