/*++
    上海证券交易所股票期权T型报价
--*/


#ifndef _TSHAPE_SSE_H_
#define _TSHAPE_SSE_H_

#include "TshapeBase.h"

class CTshapeSse: public CTshapeBase
{
	//计算保证金的第一个比例12%
	double m_MarginRatio1;

	//计算保证金的第二个比例7%
	double m_MarginRatio2;

public:
	CTshapeSse(string OptionId, size_t UnderlyingAssetType = 2);
	virtual ~CTshapeSse();

	//根据ReqQryInstrument返回的CThostFtdcInstrumentField列表，构造T型报价结构
	virtual void ConstructTshape(vector<CThostFtdcInstrumentField> InstInfoList);

	//根据InstrumentID进行解析，判断是否是期权合约，或者是标的合约，并返回对应的年月份，行权价，期权类型代码, 以及期权节点指针
	virtual int ParseInstrumentId(TThostFtdcInstrumentIDType InstrumentID, size_t& YearMonth, double& StrikePrice, char& OptionTypeCode, void* &pNode);

	//计算期权一手合约对应的各种参数，包括内在价值，时间价值，虚实程度和保证金等
	virtual void CalcOptionMiscValues(COptionItem* pOptionItem, size_t nMonthPos);

	//根据CThostFtdcInstrumentField进行解析，判断是否是期权合约，或者是标的合约，并返回对应的年月份，行权价，期权类型代码
	int ParseInstrumentField(CThostFtdcInstrumentField InstrumentField, size_t& YearMonth, double& StrikePrice, char& OptionTypeCode);

	//确定是否创建/或者代替原来的期权节点, 在模拟盘时有用（此时由于种种原因可能有多个合约代码对应同一期权)
	int CreateOrReplaceOptionItem(CThostFtdcInstrumentField *pInstrumentField, COptionItem* pOptionItem); 
};

#endif
