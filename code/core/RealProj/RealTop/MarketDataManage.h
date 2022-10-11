/*++
    管理各种T型报价及其他市场行情运行的顶层类
--*/


#ifndef _MARKET_DATA_MANAGE_H_
#define _MARKET_DATA_MANAGE_H_

#include "common.h"
#include "TshapeBase.h"
#include "TshapeCzce.h"
#include "TshapeDce.h"
#include "TshapeShfe.h"
#include "TshapeCffex.h"
#include "TshapeSse.h"
#include "TshapeSzse.h"

class CRealApp;

class CMarketDataManage
{
public:
	//各种商品T型报价指针列表
	vector<CTshapeBase*> m_pTshapeBaseList;
	//所有合约的总数目
	size_t m_TotalInstrumentNum;
	//上次计算隐含波动率及希腊值的时间
	SYSTEMTIME m_PreCalTime;

	//指向主应用对象的指针
	CRealApp* m_pRealApp;

public:
	CMarketDataManage(CRealApp* pRealApp);
	~CMarketDataManage();

	//初始化行情管理对象
	void Initialize();

	//退出前的清理工作
	void Cleanup();

	//抽取期权商品列表代码，生成各自的T型报价
	void InitTshape();

	//获取某一标的日K线数据
	//计算日间历史波动率的函数放入MyMath中
	void GetDayKlineData(TThostFtdcInstrumentIDType UnderlyingInstId, vector<CDayKlineItem>* pDayKlineList, size_t UnderlyingAssetType);

	//订阅所有T型报价的市场行情
	void SubscribeAllMarketData(char CntCode);

	//更新所有的T型报价
	void UpdateAllTshape(bool bArbOnly);

	//更新所有的T型报价的合约交易状态信息
	void UpdateAllTshapeInstrumentStatus();

	//将当日各个标的合约的K线数据导出到数据库
	void DumpDayKLine();

	//利用K线数据和T型报价计算当天的平均波动率和平均年化收益率
	void AvgTodayRateAndIV(CTshapeMonth* pTshapeMonth, double &MeanRate, double &MeanIV);

	//获取深度市场行情数据列表
	size_t GetDepthMarketDataList();

	//获取合约交易状态信息列表
	size_t GetInstrumentStatusList();

	//生成各种费率的数据表
	void CreateMiscRateTables();

	//获取标的的保证金率
	void GetUnderlyingMarginRate(char CntCode, string UnderlyingId, TThostFtdcInstrumentIDType InstrumentID, CThostFtdcInstrumentMarginRateField& MarginRate, int YearMonth = 0);

	//获取标的的手续费率
	void GetUnderlyingCommissionRate(char CntCode, string UnderlyingId, TThostFtdcInstrumentIDType InstrumentID, CThostFtdcInstrumentCommissionRateField& CommissionRate, int YearMonth = 0);

	//获取期权品种手续费率
	void GetOptionInstrCommRate(char CntCode, string OptionId, TThostFtdcInstrumentIDType InstrumentID, CThostFtdcOptionInstrCommRateField& OptionInstrCommRate, int YearMonth = 0);

	//获取ETF期权手续费
#ifndef FUTURE_OPTION_ONLY
	void GetETFOptionInstrCommRate(char CntCode, string OptionId, TThostFtdcInstrumentIDType InstrumentID, CThostFtdcETFOptionInstrCommRateField* pETFOptionInstrCommRate, int YearMonth = 0);
#endif
};

#endif
