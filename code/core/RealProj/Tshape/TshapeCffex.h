/*++
    中国金融期货交易所期权T型报价
--*/


#ifndef _TSHAPE_CFFEX_H_
#define _TSHAPE_CFFEX_H_

#include "TshapeBase.h"

class CTshapeCffex: public CTshapeBase
{
	//计算卖出期权保证金时用到的股指期权合约保证金调整系数
	double m_MarginAdjustRatio;

	//计算保证金时用到的虚值保证金优惠比率
	double m_OfmoPrefRatio;

	//计算保证金时用到的最低保障系数
	double m_LowestSafeRatio;

public:
	CTshapeCffex(string OptionId, string UnderlyingId);
	virtual ~CTshapeCffex();

	//根据ReqQryInstrument返回的CThostFtdcInstrumentField列表，构造T型报价结构
	virtual void ConstructTshape(vector<CThostFtdcInstrumentField> InstInfoList);

	//根据CThostFtdcInstrumentField进行解析，判断是否是期权合约，或者是标的合约，并返回对应的年月份，行权价，期权类型代码
	virtual int ParseInstrumentId(TThostFtdcInstrumentIDType InstrumentID, size_t& YearMonth, double& StrikePrice, char& OptionTypeCode, void* &pNode);

	//计算期权一手合约对应的各种参数，包括内在价值，时间价值，虚实程度和保证金等
	virtual void CalcOptionMiscValues(COptionItem* pOptionItem, size_t nMonthPos);
};

#endif
