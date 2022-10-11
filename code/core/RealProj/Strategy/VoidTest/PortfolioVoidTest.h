/*++
    空策略测试类
	为CPortfolioBase的派生类
--*/

#ifndef _PORTFOLIO_VOIDTEST_H
#define _PORTFOLIO_VOIDTEST_H

#include "common.h"
#include "PortfolioBase.h"

class CPortfolioVoidTest: public CPortfolioBase
{
public:
	CPortfolioVoidTest();
	virtual ~CPortfolioVoidTest();

	//对从数据库中获取的组合做合法性检查
	virtual void ValidatePortfolio();

	//将组合的详细情况输出到文本文件
	virtual void DumpToTextFile(ofstream& OutFile);

	//将组合的最新情况写入数据库
	virtual bool SaveToDatabase(bool bForce);

	//确定是否可以开仓以及开仓的形式
	virtual void OpenDecision();

	//获取开仓参数
	bool GetOpenParams(double ThresholdValue);

	//确定是否可以平仓以及平仓的形式
	virtual void CloseDecision();

	//更新开仓判决条件
	virtual int UpdateOpenDecision(double& NetOpenIncome);

	//计算在新开仓条件下, 报单是否能满足开仓要求
	virtual int UpdateOpenFish(int FishOrderIndex, double NetOpenIncome, double& NewProfit);

	//针对报单, 更新平仓判决条件
	virtual int UpdateCloseDecision(double& NetOpenIncome, double& CloseIncome);

	//计算在新平仓条件下, 报单是否能满足平仓要求
	virtual int UpdateCloseFish(int FishOrderIndex, double NetOpenIncome, double CloseIncome);

	//产生平仓报单列表
	virtual bool CloseFishListGen(double CloseIncome);

	//手动出错恢复
	virtual void ManualErrorRecovery();
};

#endif

