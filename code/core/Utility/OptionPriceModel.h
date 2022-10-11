//++
//使用各种期权定价模型对期权定价
//同时计算出期权希腊值delta, gamma, rho, theta和vega
//--
#ifndef _OPTIONPRICEMODEL_H_
#define _OPTIONPRICEMODEL_H_

#include "common.h"

class COptionPriceModel 
{
public:    //member variables
	//标的资产类型, //标的资产类型, 0: 商品期货, 1: 股指期货，2: ETF, 3: 股票
	size_t m_UnderlyingAssetType;
	//期权行权价
	double mK;    
	//标的现价
	double mS;  
	//以天计算的期权有效期，需要转化为以年为单位
	double mTday; 
	//以百分比表示的年化无风险利率，需要转换为年化复利率
	double mR;    
	//以百分比表示的年化波动率
	double mV;    

	//期权价格
	double mCallPrice;
	double mPutPrice;

	//希腊值
	double mDeltaCall;
	double mGammaCall;
	double mVegaCall;
	double mThetaCall;
	double mRhoCall;
	double mDeltaPut;
	double mGammaPut;
	double mVegaPut;
	double mThetaPut;
	double mRhoPut;

public:    //member functions
	COptionPriceModel(size_t UnderlyingAssetType);
	~COptionPriceModel();

	//计算欧式股票期权价格的Black-Scholes公式
	void BlackScholes();
	//计算欧式期货期权的Black公式
	void Black();
	//期权定价的主函数
	void PriceModelMain();
	//计算隐含波动率
	bool ImpliedVolatility(bool bCallType, double targetOptionPrice);

};

#endif
