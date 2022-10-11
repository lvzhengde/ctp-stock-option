/*++
    自定义的实用函数
--*/

#ifndef _MYUTILITIES_H_
#define _MYUTILITIES_H_

#include "UserDefinedDataStruct.h"

class CRealApp;
class CTradeDb;
class CPositionTrade;
class CTradeDb;

//程序暂停，等待输入任意字符
void PauseForInput(void);

//按任意键退出程序
void PressAnyKeyToExit(void);

//获取用户密码，加密输出
void GetPasswd(TThostFtdcPasswordType Passwd);

//去除string前后的空格
string& RemoveSpace(string& str);

//去除字符串前后的空格, 返回string
string RemoveSpace(char* pChar);

//获取当前时间的秒数
double GetCurrentSeconds();

//转换交易所返回消息的编码格式
string StatusMessageCodeConvert(char *pStatusMsg, bool bForce2UTF8 = false);

//输出盈利曲线到文件
void OutputProfitCurve(string strOutFileName, vector<CProfitData> profitDataList);

//将查询SQL数据表得到的DATETIME格式转化为Windows SYSTEMTIME格式，忽略毫秒部分
SYSTEMTIME SqlDatetimeToSystemtime(string strDatetime);

//将查询得到的SQL日期转换为CDate日期
CDate SqlDateToCDate(string strAccessDate);

//将SYSTEMTIME转换为SQL数据库DATETIME格式，不包括"#"
string SystemtimeToSqlDatetime(SYSTEMTIME CurTime);

//将SYSTEMTIME转换为SQL数据库DATE格式(例如"2017-03-10")，不包括"#"
string SystemtimeToSqlDate(SYSTEMTIME CurTime);

//将CTP的买卖方向表示转化为程序自己的买卖方向表示
char CtpDirctionToMyDirection(TThostFtdcPosiDirectionType CtpDirection);

//将交易状态代码转变成状态名称
string TradeStatusCodeToName(char TradeStatusCode);

//将数据库同步状态代码转变成状态名称
string DbSyncStatusCodeToName(char DbSyncStatusCode);

//显示经纪商账户的相关信息
void DisplayBrokerInfo(string strCounterName, string strArchName = "", CTradeDb* pTradeDb = NULL);

//确定现在是否有合约处于交易时段内
bool IsTradingTime(vector<COptionKind>* pOptionKindList);

//确定期权品种是否处于交易时段内
bool IsTradingTime(COptionKind* pOptionKind);

//判断合约是否处于允许交易的时段内
bool IsAllowedTradeTime(COptionKind* pOptionKind, CRealApp* pRealApp, double* pTradeTimeSecs = NULL);

//判断一个string字符串是否是数字
bool IsNum(string s);

//将用double表示的年月日信息转化为SYSTEMTIME格式
SYSTEMTIME DoubleToSystemtime(double YearMonthDay);

//将SYSTEMTIME转换为用double表示的年月日信息
double SystemtimeToDouble(SYSTEMTIME CurTime);

//模拟交易所自动撮合系统撮合成交价
double PriceMatchMaking(double BuyPrice, double SellPrice, double LastPrice, string strExchangeId, char Direction, double UpperLimitPrice, double LowerLimitPrice);

//控制交易时段开始期间的一小段时间是是否允许交易的（主要是控制集合竞价结束后的价格剧烈变动）, 使用行情数据中的时间
bool BeginTimeTradeAllow(vector<CThostFtdcDepthMarketDataField*> pMdList, COptionKind* pOptionKind, double TickInterval);

//获取两个Tick数据之间的时间差
double GetTickDataTimeSpan(CThostFtdcDepthMarketDataField TickData1, CThostFtdcDepthMarketDataField TickData2);

//判断是否临近日盘或者夜盘收盘, 以及计算离收盘还有多少秒
bool NearDayOrNightClose(COptionKind* pOptionKind, double JudgeSeconds, double& RemainSeconds);

//将值为0的字符转换为'-'字符，否则保持原样
char ToNoZeroChar(char InChar);

//判断股票期权合约是否处于熔断状态
bool IsFusing(CThostFtdcDepthMarketDataField* pMd, CThostFtdcInstrumentField* pInstInfo);

//判断股票期权组合是否处于异常交易状态
bool IsAbnormalState(vector<CPositionTrade*>* pPositionTradeList);

//调试输出到Debugger如DebugView, Unicode字符串格式
void OutputDebugStringEx(const wchar_t *strOutputString, ...);

//调试输出到Debugger如DebugView, ANSI字符串格式
void OutputDebugStringEx(const char *strOutputString, ...);

//线程活动状态指示
void HeartBeatIndicate(double& PreHbSecs, double Interval, string strPrefix);

//获取合约的市场节点指针, 直接遍历整个T型报价获取
void* GetMarketNodePointer(CRealApp* pRealApp, TThostFtdcInstrumentIDType InstrumentID, char &InstrumentType, size_t &UnderlyingAssetType);

//对报单价格进行过滤，主要针对交易所设置价格保护带等情况
double PriceFilt(double InPrice, COptionKind* pOptionKind, char InstrumentType, char dir, CThostFtdcDepthMarketDataField* pMd);

//中金所价格保护带处理
double CffexPriceBand(double InPrice, COptionKind* pOptionKind, char InstrumentType, char dir, CThostFtdcDepthMarketDataField* pMd);

//将InstrumentStatus由代码转化为字符串
string InsrumentStatusConvert(TThostFtdcInstrumentStatusType code);

//获取报单的状态信息
string GetOrderStatusInfo(CThostFtdcOrderField* pOrder);

#endif
