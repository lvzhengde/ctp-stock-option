/*++
    持仓和资金管理的顶层类
	将和交易柜台接口相关的成员变量都移至CCtpCntIf
--*/


#ifndef _POS_MONEY_MANAGE_H_
#define _POS_MONEY_MANAGE_H_

#include "common.h"

class CRealApp;

class CPosMoneyManage
{
public:
	//指向主应用对象的指针
	CRealApp* m_pRealApp;

public:
	CPosMoneyManage(CRealApp* pRealApp);
	virtual ~CPosMoneyManage();

	//初始化仓位和资金管理对象
	void Initialize();

	//退出前的清理工作
	void Cleanup();

	//将用户账户和持仓的详细情况输出到文本文件
	void DumpToTextFile();

	//定期刷新持仓信息和账户资金信息
	void RefreshTradeAccountInfo();

	//刷新报单和成交信息列表
	void RefreshOrderAndTradeList();

	//获取最新的投资者持仓列表，提供给ErrorHandle程序使用
	bool GetNewestInvestorPosition();

	//合并合约代码和买卖方向相同的投资者持仓查询结果
	void MergeInvestorPosition(vector<CThostFtdcInvestorPositionField>& InvestorPositionList);

	//保存报单情况到数据库
	void SaveOrderToDatabase();

	//保存成交情况到数据库
	void SaveTradeToDatabase();

	//存储各个交易柜台账户信息到数据库
	void SaveTradingAccountToDb();

};

#endif
