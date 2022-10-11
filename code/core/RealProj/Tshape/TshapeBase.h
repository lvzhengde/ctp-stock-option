/*++
    期权T型报价的基类
	针对郑商所，大商所，上期所，中金所以及上交所，深交所的期权，分别派生出子类
	每一个品种的期权（如豆粕，白糖）对应一个T型报价, T型报价可以有多个月份, 每个月份有多个行权价格, 用Vector或者数组组织起来
	所有商品期权的T型报价，在顶层也可以用Vector或者数组组织起来
--*/


#ifndef _TSHAPEBASE_H_
#define _TSHAPEBASE_H_

#include "common.h"

class CRealApp;

class CTshapeBase
{
private:
	SRWLOCK m_RwLock;

public:
	//指向RealApp的指针
	CRealApp* m_pRealApp;

	//指向该T型报价对应的OptionKind的指针
	COptionKind* m_pOptionKind;

	//true: 只构造标的的报价链而去掉期权的T型报价, false: 构造完整的T型报价
	bool  m_bUnderMdOnly;

	//期权代号，"m"指豆粕期权
	string m_OptionId;  

	//标的或标的替代的代号，如IO对应IF, m对应m
	string m_UnderlyingId;

	//标的资产类型, //标的资产类型, 0: 商品期货, 1: 股指期货，2: ETF, 3: 股票
	size_t m_UnderlyingAssetType;

	//期权交易的柜台代码, 'o': 股票期权, 'f': 期货期权
	char m_OptCntCode;

	//标的交易的柜台代码, 's': 股票或者ETF证券, 'f': 期货
	char m_UnderCntCode;

	//交易所代码, 郑商所: CZCE, 大商所: DCE, 上期所: SHFE, 中金所: CFFEX, 上海证券交易所: SSE, 深圳证券交易所: SZSE
	TThostFtdcExchangeIDType m_ExchangeId;   

	//需要获取T型报价的期权年月列表，整数201705形式，前4位表示年份，后两位为月份，如果为空则是所有可能的期权合约年月
	vector<size_t> m_YearMonthList;    

	//标的的保证金率，用一个月份的标的合约ID查出来即可
	CThostFtdcInstrumentMarginRateField m_UnderlyingInstMarginRate;

	//标的的手续费率，用一个月份的合约ID查出来即可
	CThostFtdcInstrumentCommissionRateField m_UnderlyingInstCommissionRate;

	//ETF期权手续费，有四种情况
	//[0]: 投机多持仓; [1]: 投机空持仓; [2]: 备兑多持仓; [3]: 备兑空持仓
#ifndef FUTURE_OPTION_ONLY
	CThostFtdcETFOptionInstrCommRateField m_ETFOptionInstrCommRate[4];
#endif

	//期权合约手续费率，用任一期权合约ID查出来即可(注意：期权合约的手续费和标的手续费要分开查询)
	CThostFtdcOptionInstrCommRateField m_OptionInstCommRate;

	//经纪商交易参数，这些参数可以用来计算卖出期权时的保证金, [0]: 期权的经纪商交易参数, [1]: 标的的经纪商交易参数
	CThostFtdcBrokerTradingParamsField m_BrokerTradingParams[2];

	//当前的平均无风险利率，以百分比表示，初始化之后由平价套利公式随时间平均计算得到
	double m_MeanRate;

	//当前标的商品的年化漂移率（可以认为和通货膨胀率相当), 以百分比表示
	double m_DriftRate;

	//根据各月的平均隐含波动率加权平均计算的到的隐含波动率, 以百分比表示
	double m_MeanIV;

	//包含各个月份T型报价的总的T型报价列表
	vector<CTshapeMonth*> m_pTshapeMonthList;

	//最新的Tick Data计数值，用作时间戳替代
	size_t m_TickCountStamp;

public:
	CTshapeBase();
	virtual ~CTshapeBase();

	//根据ReqQryInstrument返回的CThostFtdcInstrumentField列表，构造T型报价结构
	virtual void ConstructTshape(vector<CThostFtdcInstrumentField> InstInfoList) = 0;

	//根据CThostFtdcInstrumentField进行解析，判断是否是期权合约，或者是标的合约，并返回对应的年月份，行权价，期权类型代码
	virtual int ParseInstrumentId(TThostFtdcInstrumentIDType InstrumentID, size_t& YearMonth, double& StrikePrice, char& OptionTypeCode, void* &pNode) = 0;

	//计算期权一手合约对应的各种参数，包括内在价值，时间价值，虚实程度和保证金等
	virtual void CalcOptionMiscValues(COptionItem* pOptionItem, size_t nMonthPos) = 0;

	//基类强制实现的普通函数，用于参数订阅行情的合约代码列表
	void GenInstrumentList(char CntCode, vector<string> &strInstIdList);

	//基类强制实现的普通函数，用于计算期权的时间价值，内在价值和虚值程度
	double CalcTimeValue(char OptionType, double UnderlyingPrice, double StrikePrice, double Royalty, double& IntrinsicValue, double& OutOfTheMoney);

	//计算标的本交易时段的以成交量加权的平均价格
	void CalcUnderAvgPrice(bool bFirstMd, vector<CThostFtdcDepthMarketDataField> MarketDataList, CUnderAvg& UnderAvg);

	//缩减保留的市场行情数据
	void TruncTickData(CTshapeMonth *pTshapeMonth, size_t KeepLen);

	//根据OnRtnDepthMarketData返回的行情数据列表，更新T型报价结构
	virtual void UpdateTshape(vector<CThostFtdcDepthMarketDataField>& MarketDataList);

	//根据OnRtnInstrumentStatus返回的合约交易状态列表，更新T型报价结构
	virtual void UpdateTshapeInstrumentStatus(vector<CThostFtdcInstrumentStatusField>& InstrumentStatusList);

	//插入TshapeMonthItem时，按照年月排序，并返回插入的位置
	virtual int InsertOrderByYearMonth(vector<CTshapeMonth*>& pTshapeMonthList, CTshapeMonth* pTshapeMonthItem);

	//插入COptionPairItem时，按照行权价格大小排序，并返回插入的位置
	virtual int InsertOrderByStrikePrice(vector<COptionPairItem*>& pOptionPairList, COptionPairItem* pOptionPairItem);

	//删除OptionPairList中不合法的COptionPairItem;
	virtual void DeleteInvalidOptionPairItem(vector<COptionPairItem*>& pOptionPairList);

	//用于计算标的期货一手合约对应的保证金
	virtual void CalcUnderlyingMargin(size_t nMonthPos);

	//计算期权合约对应的希腊值和隐含波动率
	virtual void CalcGreeksAndIv(COptionItem* pOptionItem, size_t nMonthPos);

	//根据标的实际波动率计算期权合约的理论价格
	virtual void CalcTheoreticalRoyalty(COptionPairItem* pOptionPairItem, size_t nMonthPos);

	//根据平价公式计算OptionPair的平均无风险利率
	void CalcOptionPairMeanRate(COptionPairItem* pOptionPairItem, size_t nMonthPos);

	//初始化期权对根据平价公式计算的平均无风险利率
	void InitOptionPairMeanRate(double DefaultRate = 0);

	//获取整个T型报价平均无风险利率
	void GetTshapeMeanRate();

	//更新整个T型报价的数学运算结果包括希腊值，隐含波动率以及各种价值
	virtual void MathematicalRefreshment(bool bArbOnly);

	//是否在允许对隐含波动率及各种数学值进行更新的时间段内
    bool IsAllowedRefreshTime(double StartAddSecs = 0);

	//计算各种策略交易所使用的年化收益率, 以百分比表示
	double MeanRateForTrade(CTshapeMonth* pTshapeMonth, double DefaultRate, double MinRate);

	//计算各种日间策略交易所使用的年化波动率, 以百分比表示
	double MeanIVForTrade(CTshapeMonth* pTshapeMonth, double DefaultIV, double MinIV, double MaxIV);

	//获取读锁
	void readlock()
	{
		AcquireSRWLockShared(&m_RwLock);
	}

	//释放读锁
	void readunlock()
	{
		ReleaseSRWLockShared(&m_RwLock);
	}

	//获取写锁
	void writelock()
	{
		AcquireSRWLockExclusive(&m_RwLock);
	}

	//释放写锁
	void writeunlock()
	{
		ReleaseSRWLockExclusive(&m_RwLock);
	}
};

#endif
