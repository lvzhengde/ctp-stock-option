/*++
    交易所交易限制规则过滤类
	交易所的限制规则包括撤单限制，成交量限制，持仓限制，等等
	SSE, SZSE, SHFE, DCE, CZCE, CFFEX等各个交易所都有自己独立的规则过滤函数
	规则过滤函数适用于开/平仓过程。对于平仓，要考虑保证金占用的风险度
--*/

#ifndef _EXCHANGE_RULE_FILTER_H_
#define _EXCHANGE_RULE_FILTER_H_

#include "common.h"

class CRealApp;

class CExchangeRuleFilter
{
//私有变量
private:
	//控制报单申报数量的读写锁
	SRWLOCK m_InsertOrderRwLock;
	//控制撤单数量的读写锁
	SRWLOCK m_CancelOrderRwLock;
	//控制成交总量的读写锁
	SRWLOCK m_TradeVolumeRwLock;
	//控制持仓总量的读写锁
	SRWLOCK m_OpenInterestsRwLock;

//公有变量
public:
	//指向主应用对象的指针
	CRealApp* m_pRealApp;

	//交易柜台代码, 'o': 股票期权; 's': 股票现货; 'f': 期货及期货期权
	//在应用程序当中, 对每个柜台而言此代码是唯一的
	char m_CntCode;
	//经纪商报盘柜台名称(如ZXQH_SIM, ZXQH_TRUE...)
	string m_CounterName;
	//交易所ID
	string m_ExchangeID;

	//当天总报单申报数量
	int m_InsertOrderNum;
	//当天总撤单数量
	int m_CancelOrderNum;
	//当天总成交数量（合约张数或者股票数量)
	int m_TradeVolume;
	//当前各合约持仓总数
	int m_OpenInterests;

	//允许的最大日内总撤单次数
	int m_MaxCancelOrderNum;
	//允许的最大日内总成交量
	int m_MaxTradeVolume;
	//允许的最大总持仓量
	int m_MaxOpenInterests;

public:
	//构造函数
	CExchangeRuleFilter(CRealApp *pRealApp);

	//析构函数
	~CExchangeRuleFilter();

	//增加申报单数目
	void IncreaseInsertOrderNum(char CntCode, string ExchangeID);

	//设置申报单数目
	void SetInsertOrderNum(int SetValue);

	//获取申报单数目
	int GetInsertOrderNum();

	//增加撤单数目
	void IncreaseCancelOrderNum(char CntCode, string ExchangeID);

	//设置撤单数目
	void SetCancelOrderNum(int SetValue);

	//获取撤单数目
	int GetCancelOrderNum();

	//增加成交量数目
	void IncreaseTradeVolume(char CntCode, CThostFtdcTradeField *pTrade);

	//设置成交量数目
	void SetTradeVolume(int SetValue);

	//获取成交量数目
	int GetTradeVolume();

	//计算持仓量数目
	void CalcOpenInterests(char CntCode, vector<CInvestorPositionDetail>* pInvestorPositionDetailList);

	//已知合约代码, 获取交易所代码
	string GetExchangeID(TThostFtdcInstrumentIDType InstrumentID);

	//设置持仓量数目
	void SetOpenInterests(int SetValue);

	//获取持仓量数目
	int GetOpenInterests();

	//检查开仓交易是否通过交易所规则限制
	bool IsExchangeRuleFilterPassForOpen();

	//检查平仓交易是否通过交易所规则限制
	bool IsExchangeRuleFilterPassForClose();

	//从数据库中获取当日的各种交易统计信息
	void GetTodayStatusFromDb();

	//将交易统计数据写入数据库
	void DumpTodayStatusToDb();
};

#endif
