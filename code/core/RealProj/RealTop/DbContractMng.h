/*++
    管理交易数据库中交易合约换月的类
	目前只对期货交易起作用, 并且只支持一个柜台下标为[0]
--*/


#ifndef _DB_CONTRACT_MNG_H_
#define _DB_CONTRACT_MNG_H_

#include "common.h"
#include "RealApp.h"
#include "TradeDb.h"
#include "MarketDataManage.h"

//比较月份T型报价标的的成交量
bool CompUnderVolume(CTshapeMonth* pTshapeMonth0, CTshapeMonth* pTshapeMonth1);

class CDbContractMng
{
public:

	//指向主应用对象的指针
	CRealApp* m_pRealApp;

	//交易策略名称列表
	vector<string> m_StrategyNameList;

public:
	CDbContractMng(CRealApp* pRealApp);
	~CDbContractMng();

	//数据库交易合约管理主程序
	void DbContractMain();

	//交易合约换月管理
	void ChangeTradeMonth();

	//删除不交易的品种
	void DeleteOptionKind();

	//单个合约换月
	void IndividualChangeMonth(string StrategyName, COptionKind* pOptionKind);

	//从统计数据表中获取交易年月份
	void GetDbStatYearMonth(string strTableName, vector<size_t>& YearMonthList);

	//构建T型报价并获取市场行情, 在获取市场行情后，将T型报价按照标的成交量排序
	CTshapeBase* ConstructAndUpdateTshape(COptionKind* pOptionKind);

	//删除不再交易的期货合约的K线数据表
	void DropKlineTable(COptionKind* pOptionKind, size_t YearMonth);

	//删除当前年月份之前的所有K线数据表
	void DeleteAllKlinesBeforeThisYearMonth(COptionKind* pOptionKind);

	//删除单个交易品种的相关设置和数据
	void DeleteIndividualOptionKind(string StrategyName, COptionKind* pOptionKind);

	//删除交易品种后修改策略配置文件
	void ModifyStrategyConfig(string StrategyName, vector<COptionKind>* pOptionKindList);

};

#endif
