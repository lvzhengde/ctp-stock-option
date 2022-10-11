#include "PortfolioVoidTest.h"
#include "RealApp.h"

//++
//构造函数
//--
CPortfolioVoidTest::CPortfolioVoidTest()
{
}

//++
//析构函数
//--
CPortfolioVoidTest::~CPortfolioVoidTest()
{
}

//++
//对从数据库中获取的组合做合法性检查
//--
void CPortfolioVoidTest::ValidatePortfolio()
{
}

//++
//将组合的详细情况输出到文本文件
//参数
//    OutFile: 输出的流文件
//返回值
//    无
//--
void  CPortfolioVoidTest::DumpToTextFile(ofstream& OutFile)
{
}

//++
//将组合的最新情况写入数据库
//--
bool CPortfolioVoidTest::SaveToDatabase(bool bForce)
{
	return true;
}

//++
//确定是否可以开仓
//       
//参数
//    无
//返回值
//    无
void CPortfolioVoidTest::OpenDecision()
{
}

//++
//获取开仓参数
//参数
//    ThresholdValue
//返回值
//    true: 可以开仓
//    false: 不可开仓
//--
bool CPortfolioVoidTest::GetOpenParams(double ThresholdValue)
{
	return false;
}

//++
//确定是否可以平仓
//--
void CPortfolioVoidTest::CloseDecision()
{
}

//++
//更新开仓判决条件各项信息
//参数
//    NetOpenIncome
//返回值
//    = 0: 开仓条件满足
//    <>0: 开仓条件不能满足
//--
int CPortfolioVoidTest::UpdateOpenDecision(double& NetOpenIncome)
{

	return 1;
}

//计算在新开仓条件下, 报单是否能满足开仓要求
//参数
//    FishOrderIndex 
//    NetOpenIncome
//    NewProfit
//返回值
//    = 0: 开仓条件满足
//    <>0: 开仓条件不能满足
//--
int CPortfolioVoidTest::UpdateOpenFish(int FishOrderIndex, double NetOpenIncome, double& NewProfit)
{
	return 1;
}

//++
//更新平仓判决条件各项信息
//参数
//    NetOpenIncome
//    CloseIncome
//返回值
//    = 0: 平仓条件满足
//    <>0: 平仓条件不能满足
//--
int CPortfolioVoidTest::UpdateCloseDecision(double& NetOpenIncome, double& CloseIncome)
{
	return 1;
}

//++
//计算在新平仓条件是否能满足平仓要求
//参数
//    NetOpenIncome
//    CloseIncome
//返回值
//    = 0: 平仓条件满足
//    <>0: 平仓条件不能满足
//--
int CPortfolioVoidTest::UpdateCloseFish(int FishOrderIndex, double NetOpenIncome, double CloseIncome)
{
	return 1;
}

//++
//产生平仓报单列表
//参数
//    CloseIncome
//返回值
//    true: 有平仓报单可以产生
//    false: 无平仓报单可用
//--
bool CPortfolioVoidTest::CloseFishListGen(double CloseIncome)
{
	return false;
}

//++
//手动出错恢复
//--
void CPortfolioVoidTest::ManualErrorRecovery()
{
}


