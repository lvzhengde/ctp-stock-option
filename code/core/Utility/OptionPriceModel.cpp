#include "OptionPriceModel.h"

#define MIN_EXPIRE_LEFT_DAYS        (0.5)     //为防止到期日剩余时间为0时计算出错, 设置的最小剩余时间

//++
//构造函数
//--
COptionPriceModel::COptionPriceModel(size_t UnderlyingAssetType)
{
	m_UnderlyingAssetType = UnderlyingAssetType;
}

//++
//析构函数
//--
COptionPriceModel::~COptionPriceModel()
{

}

//++
//使用标准的Black-Scholes方程解决股票期权定价问题，不计股息和红利的欧式期权
//--
void COptionPriceModel::BlackScholes()
{
	double r;
	double T;
	double sigma;

	r = log(1+mR/100.0);
	T = mTday/365.0;
	sigma = mV/100.0;

	double sqrtT = sqrt(T);
    double d1 = (log(mS / mK) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrtT);
    double d2 = d1 - sigma * sqrtT;

	double cndd1 = FastNormCDF(d1);
	double cndd2 = FastNormCDF(d2);

	mCallPrice = cndd1 * mS - cndd2 * mK * exp(-r*T);
	mPutPrice = (1.0 - cndd2) * mK * exp(-r*T) - (1.0 - cndd1) * mS;

	//计算希腊值
	double pdfd1 = NormPDF(d1);

	mDeltaCall = cndd1;
	mGammaCall = pdfd1/(mS*sigma*sqrtT);
	mVegaCall = mS * pdfd1 * sqrtT;
	mThetaCall = -mS * pdfd1 * sigma / (2*sqrtT) - r * mK * exp(-r*T) * cndd2;
	mRhoCall = mK * T * exp(-r*T) * cndd2;

	mDeltaPut = cndd1 - 1;
	mGammaPut = pdfd1/(mS*sigma*sqrtT);
	mVegaPut = mS * pdfd1 * sqrtT;
	mThetaPut = -mS * pdfd1 * sigma / (2*sqrtT) + r * mK * exp(-r*T) * (1.0 - cndd2);
	mRhoPut = -mK * T * exp(-r*T) * (1.0 - cndd2);
}

//++
//计算欧式期货期权定价的Black公式
//--
void COptionPriceModel::Black()
{
	double r;
	double T;
	double sigma;

	r = log(1+mR/100.0);
	T = mTday/365.0;
	sigma = mV/100.0;

	double sqrtT = sqrt(T);
    double d1 = (log(mS / mK) + (0.5 * sigma * sigma) * T) / (sigma * sqrtT);
    double d2 = d1 - sigma * sqrtT;

	double cndd1 = FastNormCDF(d1);
	double cndd2 = FastNormCDF(d2);
	double expmrt = exp(-r*T);

	mCallPrice = expmrt * (cndd1 * mS - cndd2 * mK);
	mPutPrice = expmrt * ((1.0 - cndd2) * mK - (1.0 - cndd1) * mS);

	//计算希腊值
	double pdfd1 = NormPDF(d1);

	mDeltaCall = expmrt * cndd1;
	mGammaCall = expmrt * pdfd1/(mS*sigma*sqrtT);
	mVegaCall =  expmrt * mS * pdfd1 * sqrtT;
	mThetaCall = expmrt * (-mS * pdfd1 * sigma / (2*sqrtT) - r * mK * cndd2 + r * mS * cndd1);
	mRhoCall = -T * mCallPrice; 

	mDeltaPut = expmrt * (cndd1 - 1);
	mGammaPut = expmrt * pdfd1/(mS*sigma*sqrtT);
	mVegaPut =  expmrt * mS * pdfd1 * sqrtT;
	mThetaPut =  expmrt * (-mS * pdfd1 * sigma / (2*sqrtT) + r * mK * (1.0 - cndd2) - r * mS * cndd1);
	mRhoPut = -T * mPutPrice; 
}

//++
//期权定价的主函数
//--
void COptionPriceModel::PriceModelMain()
{
	//限制到期剩余天数的最小值
	if(mTday < MIN_EXPIRE_LEFT_DAYS)
	{
		mTday = MIN_EXPIRE_LEFT_DAYS;
	}

	if(m_UnderlyingAssetType == 0)
		BlackScholes(); //Black(); //全部都改为使用BlackScholes公式
	else if(m_UnderlyingAssetType == 1 || m_UnderlyingAssetType == 2 || m_UnderlyingAssetType == 3)
		BlackScholes();
	else
		cerr << "目前尚不支持的期权类型" << ENDLINE;
}

//++
//根据期权价格计算隐含波动率，计算结果放回类成员变量mV。
//采用二分法求解，搜索范围为0-1000%，当与目标价格之差小于eps时停止搜索
//参数
//    bCallType: true 由看涨期权价格计算； false 由看跌期权价格计算
//    targetOptionPrice: 目标期权的当前实际价格
//返回值
//    true: 计算成功
//    false: 计算失败
//--
bool COptionPriceModel::ImpliedVolatility(bool bCallType, double targetOptionPrice)
{
	double eps = 0.00001;
	double v1 = 0;
	double v2 = 1000;
	double calculatedPrice;
	int loopCount;
	bool bRetVal;

	bRetVal = true;
	loopCount = 0;
	int LoopCountMax = 100;

	double epsPrice = eps * mS;

	do 
	{
		mV = (v1+v2)/2;
		//BlackScholes();
		PriceModelMain();
		calculatedPrice = (bCallType == true) ? mCallPrice : mPutPrice;

		if(calculatedPrice > targetOptionPrice) //设置波动率过大
			v2 = mV;
		else
			v1 = mV;

		loopCount = loopCount + 1;
		if(loopCount >= LoopCountMax)
		{
			//cerr << "隐含波动率求解过程循环次数过多，退出求解循环！" << endl;
			bRetVal = false;
			break;
		}

	} while ((abs(calculatedPrice - targetOptionPrice) > epsPrice) && (mV > eps));  //隐含波动率接近于0可能导致死循环

	return bRetVal;
}
