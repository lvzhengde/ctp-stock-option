/*++
    自定义的数学运算函数
--*/

#ifndef _MYMATH_H_
#define _MYMATH_H_

#include "UserDefinedDataStruct.h"

class CTshapeMonth;

//浮点数相等的判别，考虑到表示精度误差的影响
#define FloatEqual(a, b, eps) (a >= (b-eps) && a <= (b+eps))

//获取指定日期离公元元年元月元日的绝对天数
int GetAbsDays(CDate x);

//计算两个日期之间的天数差
int GetDiffDays(CDate a, CDate b);

//根据当前的日期计算需要更新入数据库的K线日期
SYSTEMTIME CalKLineDate(SYSTEMTIME CurTime);

//计算两个SYSTEMTIME之间的日历日期差
int GetSystemtimeDiffDays(SYSTEMTIME StartTime, SYSTEMTIME EndTime);

//计算今天离到期日还剩多少天, 日期格式为YYYY/MM/DD, 分隔符可以是其它，添0对齐
int CalcExpirationDays(char expirationDate[20]); 

//使用快速方法计算正态分布的累积概率分布函数
double FastNormCDF(double d);

//使用积分的方法计算正态分布函数的累积概率分布函数
double IntegralNormCDF(double d);

//标准正态分布的概率密度函数
double NormPDF(double x);

//计算时间t之后标的资产价格小于等于s的概率
double PriceProbability(double s, double s0, double u, double v, double t);

//计算资产在各个价格上的概率分布
void GetPricePDF(vector<CPricePDF>& pricePDFList, double s0, double u, double v, double t, double ds, double sigmaMultiple);

//计算策略的可盈利性
bool EvaluateStrategy(vector<CPositionItem> positionItemList, CUnderlyingInfo underlyingInfo, char targetDay[20], bool bIvUsed, 
					  CStrategyResult& strategyResult, vector<CProfitData>& profitDataList);

//计算日间历史波动率
double CalcInterDayHv(vector<CDayKlineItem>* pDayKlineList, size_t NumOfDays);

//计算合约某个时段长度的加权平均价格
double CalcWeightedAvgPrice(vector<CThostFtdcDepthMarketDataField> MarketDataList, double TimeSpanSeconds);

//判断是否是极端价格, 用于K线数据的处理
bool IsExtremePrice(double Price);

//是否有行情报价或者行情报价是否正常
bool IsDepthMarketDataOk(CThostFtdcDepthMarketDataField* pMd);

//获取涨停板价格
double GetUpperLimitPrice(CThostFtdcDepthMarketDataField* pMd, char InstrumentType, CThostFtdcDepthMarketDataField* pUnderMd = NULL);

//获取跌停板价格
double GetLowerLimitPrice(CThostFtdcDepthMarketDataField* pMd, char InstrumentType, CThostFtdcDepthMarketDataField* pUnderMd = NULL);

//计算标的行情的买卖委托量比例(委买手数 - 委卖手数)/(委买手数 + 委卖手数) * 100
double BidAskDiffRatio(CThostFtdcDepthMarketDataField* pMd);

//计算标的多空情况
bool GetUnderTickSignal(CTshapeMonth *pTshapeMonth, size_t TickDataLen, int &DirIndicator);

#endif
