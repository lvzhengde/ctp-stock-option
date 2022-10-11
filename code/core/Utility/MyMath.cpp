#include "common.h"
#include "OptionPriceModel.h"

//++
//计算日期x离公元元年元月元日的天数
//--
int GetAbsDays(CDate x)
{
	int i;
	int month_day[] = {31,28,31,30,31,30,31,31,30,31,30,31};
	int year = x.year-1;  // 因为欲求距离公元元年元月元日的天数
	int days = year * 365 + year/4 - year/100 + year/400;  //求得之前闰年的数量并在天数上进行相加
	if(x.year%4==0 && x.year%100!=0 || x.year%400==0) 
		month_day[1]++; //当前年为闰年，二月加 1
	for(i=0; i<x.month-1; i++)
		days += month_day[i];
	days += x.day-1;  //今天不计入天数计数

	return days;
}

//++
//计算两个日期b和a之间相差的天数b-a
//--
int GetDiffDays(CDate a, CDate b)
{
	 return GetAbsDays(b) - GetAbsDays(a);
}

//++
//根据当前的日期计算需要更新入数据库的K线日期
//参数
//    CurTime: 当前系统时间
//返回值
//    SYSTEMTIME: K线日期
//--
SYSTEMTIME CalKLineDate(SYSTEMTIME CurTime)
{
	SYSTEMTIME KlineTime;
	int DaysInc = 0;

	int month_day[] = {31,28,31,30,31,30,31,31,30,31,30,31};
	int year = CurTime.wYear-1;  // 因为欲求距离公元元年元月元日的天数
	int days = year * 365 + year/4 - year/100 + year/400;  //求得之前闰年的数量并在天数上进行相加
	if(CurTime.wYear%4==0 && CurTime.wYear%100!=0 || CurTime.wYear%400==0) 
		month_day[1]++; //当前年为闰年，二月加 1

	memset(&KlineTime, 0, sizeof(SYSTEMTIME));

	if(CurTime.wHour < 18)  //日盘
	{
		memcpy(&KlineTime, &CurTime, sizeof(SYSTEMTIME));
	}
	else    //夜盘
	{	
		if(CurTime.wDayOfWeek < 5)          //非周五
			DaysInc = 1;
		else if(CurTime.wDayOfWeek == 5)    //周五夜盘对应的是下周一
			DaysInc = 3;

		KlineTime.wYear = CurTime.wYear;
		int DstDay = CurTime.wDay + DaysInc;
		if(CurTime.wMonth != 12)     //非年底
		{
			if(DstDay <= month_day[CurTime.wMonth - 1]) //还在当月
			{
				KlineTime.wMonth = CurTime.wMonth;
				KlineTime.wDay = DstDay;
			}
			else   //下月
			{
				KlineTime.wMonth = CurTime.wMonth + 1;
				KlineTime.wDay = DstDay - month_day[CurTime.wMonth - 1];
			}
		}
		else  //年底
		{
			if(DstDay <= month_day[CurTime.wMonth - 1]) //还在当年当月
			{
				KlineTime.wMonth = CurTime.wMonth;
				KlineTime.wDay = DstDay;
			}
			else   //下年
			{
				KlineTime.wYear = CurTime.wYear + 1;
				KlineTime.wMonth = 1;
				KlineTime.wDay = DstDay - month_day[CurTime.wMonth - 1];
			}
		}
	}

	return KlineTime;
}

//++
//计算两个SYSTEMTIME之间的日历日期差
//参数
//    StartTime: 起始时间
//    EndTime: 结束时间
//返回值
//    两个时间之间相差的日历天数
//--
int GetSystemtimeDiffDays(SYSTEMTIME StartTime, SYSTEMTIME EndTime)
{
	CDate StartDate;
	CDate EndDate;

	StartDate.year = StartTime.wYear;
	StartDate.month = StartTime.wMonth;
	StartDate.day = StartTime.wDay;

	EndDate.year = EndTime.wYear;
	EndDate.month = EndTime.wMonth;
	EndDate.day = EndTime.wDay;

	return GetDiffDays(StartDate, EndDate);
}


//++
//计算当前日期离到期日还有多少天
//参数 expirationDate为到期日期，格式为YYYY/MM/DD，添0对齐
//--
int CalcExpirationDays(char expirationDate[20])
{
	SYSTEMTIME CurTime;
	CDate startDate;
	CDate endDate;

	GetLocalTime(&CurTime);
	startDate.year = (int)CurTime.wYear;
	startDate.month = (int)CurTime.wMonth;
	startDate.day = (int)CurTime.wDay;

	string str1 = expirationDate;
	string str2 = expirationDate;
	string str3 = expirationDate;

	str1 = str1.substr(0, 4);
	str2 = str2.substr(5, 2);
	str3 = str3.substr(8, 2);

	endDate.year = atoi(str1.c_str());
	endDate.month = atoi(str2.c_str());
	endDate.day = atoi(str3.c_str());

	int diffDay;
	diffDay	= GetDiffDays(startDate, endDate);

    return diffDay;
}

//++
//使用快速方法计算正态分布的累积概率分布函数
//--
double FastNormCDF(double d)
{
    const double a1 = 0.31938153, a2 = -0.356563782, a3 = 1.781477937, a4 = -1.821255978, a5 = 1.330274429;
    const double isqrt2pi = 0.39894228040143267793994605993438;
    double x = 1.0 / (1.0 + 0.2316419 * abs(d));
    
    double cdf = isqrt2pi * exp(-0.5 * d * d) * (x * (a1 + x * (a2 + x * (a3 + x * (a4 + x * a5)))));
    return (d > 0) ? 1.0 - cdf : cdf;
}

//++
//使用积分的方法计算正态分布函数的累积概率分布函数
//--
double IntegralNormCDF(double d)
{
	const double pi = 3.14159265358979323846;
	const double isqrt2pi = 1.0/sqrt(2*pi);
	const double dx = 0.000001;
	const double startX = -10.0;
	double x;
	double cdf;

	cdf = 0;
	for(x = startX; x <= d; x = x+dx)
	{
		cdf = cdf + exp(-x*x/2.0)*dx;
	}
	cdf = cdf * isqrt2pi;

	return cdf;
}

//++
//标准正态分布的概率密度函数
//均值为0， 方差为1
//--
double NormPDF(double x)
{
	const double pi = 3.14159265358979323846;
	const double isqrt2pi = 1.0/sqrt(2*pi);
	double pdf;

	pdf = exp(-0.5*x*x) * isqrt2pi;

	return pdf;
}

//++
//计算时间t之后标的资产价格小于等于s的概率
//参数
//    s: 标的资产的目标价格
//    s0: 标的资产的当前价格
//    u: 标的资产的漂移速度，比如对于股票指数相关资产可以设为年化10% = 0.1
//    v: 标的资产的年化波动率，采用20天平均较好，股票指数资产常见20% = 0.2
//    t: 距离目前的时间，以一年时间365天归一化
//--
double PriceProbability(double s, double s0, double u, double v, double t)
{
	double x;
	double mu;
	double sigma;
	double cdf;

	//首先保证输入参数没有问题
	if(s <= 0 || s0 <= 0 || v <= 0 || t<= 0)
	{
		cerr << "相关参数s, s0, v, t不能小于等于0" << ENDLINE;
		cerr << "s = " << s << "  s0 = " << s0 << "  v = " << v << "   t = " << t << ENDLINE;
		return 0;
	}

	mu = u * t;
	sigma = v * sqrt(t);
	x = (log(s/s0) - mu)/sigma;
	cdf = FastNormCDF(x);

	return cdf;
}

//++
//计算资产在各个价格上的概率分布。
//参数
//  pricePDFList: 运算结果放在此参数中
//  s0: 资产价格的现值
//  u:  资产价格的漂移率， 10%=0.1
//  v:  资产价格的波动率， 以小数表示
//  t:  时间间隔，以年为单位
//  ds: 计算概率所用到的资产价格之间的最小间距
//  sigmaMultiple: 最大资产价格离当前价格的标准差倍数
//返回值
//  无
//--
void GetPricePDF(vector<CPricePDF>& pricePDFList, double s0, double u, double v, double t, double ds, double sigmaMultiple)
{
	double cdf1, cdf2;
	double s;
	double sigma;

	//计算各个频率点的概率
	CPricePDF pricePDF;

	pricePDFList.clear();

	//为避免计算错误，对t做限制，避免被零除
	if(t < 0.1/365.0) t = 0.1/365.0;

	sigma = s0 * v * sqrt(t);
	s = 0.0000001;
	do
	{
		cdf1 = PriceProbability(s, s0, u, v, t);
		cdf2 = PriceProbability(s+ds, s0, u, v, t);
		pricePDF.price = s;
		pricePDF.probability = cdf2 - cdf1;
		pricePDFList.push_back(pricePDF);
		s = s + ds;
	} while( s <= (s0+sigmaMultiple*sigma));

	return;
}

//++
//计算策略的可盈利性
//参数
//    positionItemList: 持仓列表，每项包含合约类型，交易方向，数量，期权执行价格，交易价格，期权到期日等信息
//    underlyingInfo: 标的资产的当前相关信息，包括标的资产类型，当前价，历史波动率，资产漂移率，以及无风险利率等
//    targetDay: 计算针对的目标平仓日期，必须小于最近期权的到期日，格式为YYYY/MM/DD
//    bIvUsed:   true， 使用最初计算得到的隐含波动率来计算后面期权的理论价格； false， 使用标的的历史波动率
//    strategyResult: 相关计算结果，包括盈利性和风险参数
//    profitDataList: 盈利曲线
//返回值
//    true: 计算成功 
//    false: 计算失败
//--
bool EvaluateStrategy(vector<CPositionItem> positionItemList, CUnderlyingInfo underlyingInfo, char targetDay[20], bool bIvUsed, 
					  CStrategyResult& strategyResult, vector<CProfitData>& profitDataList)
{
	bool bRetVal;
	size_t tradeListSize;

	bRetVal = true;
	tradeListSize = positionItemList.size();

	//首先保证日期输入的正确性
	int targetDiffDay;
	int minExpDiffDay;

	targetDiffDay = CalcExpirationDays(targetDay);
	minExpDiffDay = CalcExpirationDays(positionItemList[0].expirationDate);
	for (size_t i = 0; i < tradeListSize; i++) 
	{
		positionItemList[i].diffDay = CalcExpirationDays(positionItemList[i].expirationDate);
		if(positionItemList[i].diffDay < minExpDiffDay)
			minExpDiffDay = positionItemList[i].diffDay;
	}

	if(targetDiffDay <= 0) {
		cerr << "错误：平仓目标日期早于当前日期！" << ENDLINE;
		bRetVal = false;
	}
	else if(targetDiffDay > minExpDiffDay) {
		cerr << "错误：平仓目标日期晚于最先到期期权的到期日！" << ENDLINE;
		bRetVal = false;
	}

	//计算每个期权的当前希腊值，隐含波动率并得到整个策略的希腊值
	strategyResult.delta = 0;
	strategyResult.gamma = 0;
	strategyResult.vega = 0;
	strategyResult.theta = 0;
	strategyResult.rho = 0;

	for(size_t i = 0; i < tradeListSize; i++) 
	{
		bool bCallType;
		double optionPrice;
		COptionPriceModel optionPriceModel(underlyingInfo.UnderlyingAssetType);

		bCallType = (positionItemList[i].instrumentType == 'c' || positionItemList[i].instrumentType == 'C') ? true : false;
		optionPrice = positionItemList[i].price;
		optionPriceModel.mS = underlyingInfo.currentPrice;
		optionPriceModel.mK = positionItemList[i].strikePrice;
		optionPriceModel.mR = underlyingInfo.riskFreeRate;
		optionPriceModel.mTday = positionItemList[i].diffDay;

		//下面的计算可以同时得到隐含波动率和希腊值
		if(positionItemList[i].instrumentType == 'u' || positionItemList[i].instrumentType == 'U')
		{
			if(positionItemList[i].direction == 'b' || positionItemList[i].direction == 'B')
			{
				positionItemList[i].delta = 1.0;
				positionItemList[i].gamma = 0;
				positionItemList[i].vega  = 0;
				positionItemList[i].theta = 0;
				positionItemList[i].rho   = 0;
				positionItemList[i].iv    = 0;  
			}
			else if(positionItemList[i].direction == 's' || positionItemList[i].direction == 'S')
			{
				positionItemList[i].delta = -1.0;
				positionItemList[i].gamma = 0;
				positionItemList[i].vega  = 0;
				positionItemList[i].theta = 0;
				positionItemList[i].rho   = 0;
				positionItemList[i].iv    = 0;  
			}

			strategyResult.delta = strategyResult.delta + positionItemList[i].amount * positionItemList[i].delta;
		}
		else if(optionPriceModel.ImpliedVolatility(bCallType, optionPrice) == true)
		{
			positionItemList[i].delta = (bCallType == true) ? optionPriceModel.mDeltaCall : optionPriceModel.mDeltaPut;
			positionItemList[i].gamma = (bCallType == true) ? optionPriceModel.mGammaCall : optionPriceModel.mGammaPut;
			positionItemList[i].vega  = (bCallType == true) ? optionPriceModel.mVegaCall : optionPriceModel.mVegaPut;
			positionItemList[i].theta = (bCallType == true) ? optionPriceModel.mThetaCall : optionPriceModel.mThetaPut;
			positionItemList[i].rho   = (bCallType == true) ? optionPriceModel.mRhoCall : optionPriceModel.mRhoPut;
			positionItemList[i].iv    = optionPriceModel.mV;  

			//买入期权
			if(positionItemList[i].direction == 'b' || positionItemList[i].direction == 'B') 
			{  
				strategyResult.delta = strategyResult.delta + positionItemList[i].amount * positionItemList[i].delta;
				strategyResult.gamma = strategyResult.gamma + positionItemList[i].amount * positionItemList[i].gamma;
				strategyResult.vega  = strategyResult.vega + positionItemList[i].amount * positionItemList[i].vega;
				strategyResult.theta = strategyResult.theta + positionItemList[i].amount * positionItemList[i].theta;
				strategyResult.rho   = strategyResult.rho + positionItemList[i].amount * positionItemList[i].rho;
			}
			//卖出期权
			else if(positionItemList[i].direction == 's' || positionItemList[i].direction == 'S')
			{                                   
				strategyResult.delta = strategyResult.delta - positionItemList[i].amount * positionItemList[i].delta;
				strategyResult.gamma = strategyResult.gamma - positionItemList[i].amount * positionItemList[i].gamma;
				strategyResult.vega  = strategyResult.vega - positionItemList[i].amount * positionItemList[i].vega;
				strategyResult.theta = strategyResult.theta - positionItemList[i].amount * positionItemList[i].theta;
				strategyResult.rho   = strategyResult.rho - positionItemList[i].amount * positionItemList[i].rho;
			}
		}
		else
		{
			cerr << "计算隐含波动率和希腊值时出错" << "i = " << i << ENDLINE;
			bRetVal = false;
		}
	} //for i

	//计算在平仓目标日期标的价格概率分布和各个期权的在价格分布上的盈利之和
	vector<CPricePDF> pricePDFList;
	size_t PDFListSize;
	double ds = 0.001;
	double sigmaMultiple = 20;

	//需要根据标的价格重新计算ds，否则会导致计算量大增
	if(underlyingInfo.currentPrice < 10.0)
		ds = 0.001;
	else
		ds = underlyingInfo.currentPrice/10000;

    GetPricePDF(pricePDFList, underlyingInfo.currentPrice, underlyingInfo.driftRate/100.0, underlyingInfo.hv/100.0, targetDiffDay/365.0, ds, sigmaMultiple);
	PDFListSize = pricePDFList.size();
	profitDataList.clear();

	for(size_t i = 0; i < PDFListSize; i++) {         //遍历所有的价格
		CProfitData profitData;
		profitData.profit = 0;

		for(size_t j = 0; j < tradeListSize; j++) {   //遍历所有的期权交易
			double OpenPrice;
			double ClosePrice;
			COptionPriceModel optionPriceModel(underlyingInfo.UnderlyingAssetType);

			optionPriceModel.mS = pricePDFList[i].price;                         //注意价格不是标的当前价格
			optionPriceModel.mK = positionItemList[j].strikePrice;
			optionPriceModel.mR = underlyingInfo.riskFreeRate;
			optionPriceModel.mTday = positionItemList[j].diffDay - targetDiffDay;  //在平仓日离期权到期的天数
			optionPriceModel.mV = (bIvUsed == true) ? positionItemList[j].iv : underlyingInfo.hv;

			if(positionItemList[j].instrumentType != 'u' && positionItemList[j].instrumentType != 'U')
			{
				optionPriceModel.PriceModelMain();
			}

			OpenPrice = positionItemList[j].price;
			if(positionItemList[j].instrumentType == 'u' || positionItemList[j].instrumentType == 'U')  //基础资产
			{
				ClosePrice = pricePDFList[i].price;
				if(positionItemList[j].direction == 'b' || positionItemList[j].direction == 'B') //买入
				{
					profitData.profit = profitData.profit + positionItemList[j].amount * (ClosePrice - OpenPrice);
				}
				else if(positionItemList[j].direction == 's' || positionItemList[j].direction == 'S') //卖出
				{
					profitData.profit =profitData.profit + positionItemList[j].amount * (OpenPrice - ClosePrice);
				}
			}
			else if(positionItemList[j].instrumentType == 'c' || positionItemList[j].instrumentType == 'C') //看涨期权
			{     
				ClosePrice = optionPriceModel.mCallPrice;
				if(positionItemList[j].direction == 'b' || positionItemList[j].direction == 'B') //买入期权
					profitData.profit =profitData.profit + positionItemList[j].amount * (ClosePrice - OpenPrice);
				else if(positionItemList[j].direction == 's' || positionItemList[j].direction == 'S') //卖出期权
					profitData.profit =profitData.profit + positionItemList[j].amount * (OpenPrice - ClosePrice);
			}
			else if(positionItemList[j].instrumentType == 'p' || positionItemList[j].instrumentType == 'P') //看跌期权
			{                                    
				ClosePrice = optionPriceModel.mPutPrice;
				if(positionItemList[j].direction == 'b' || positionItemList[j].direction == 'B') //买入期权
					profitData.profit =profitData.profit + positionItemList[j].amount * (ClosePrice - OpenPrice);
				else if(positionItemList[j].direction == 's' || positionItemList[j].direction == 'S') //卖出期权
					profitData.profit =profitData.profit + positionItemList[j].amount * (OpenPrice - ClosePrice);
			}
		} //loop j

		profitData.price = pricePDFList[i].price;
		profitData.probability = pricePDFList[i].probability;

		profitDataList.push_back(profitData);
	} // loop i

	//计算策略的其它参数，如盈亏平衡点，平均盈利，盈利概率等
	int profitListSize;

	profitListSize = profitDataList.size();
	strategyResult.breakEvenPointNum = 0;
	strategyResult.unchangedProfit = 0;
	for(int i = 0; i < profitListSize-1; i++) 
	{ 
		//获取盈亏平衡点
		if((profitDataList[i].profit < 0 && profitDataList[i+1].profit >= 0) || (profitDataList[i].profit > 0 && profitDataList[i+1].profit <= 0))
		{
			if(strategyResult.breakEvenPointNum >= 10)
				cerr << "盈亏平衡点数目已经大于10，strategyResult.breakEvenPointNum = " << strategyResult.breakEvenPointNum << ENDLINE;
			else
				strategyResult.breakEvenPoint[strategyResult.breakEvenPointNum] = (profitDataList[i].price + profitDataList[i+1].price)/2.0;
			strategyResult.breakEvenPointNum = strategyResult.breakEvenPointNum + 1;
		}

		//计算价格保持不变时的盈亏
		if(profitDataList[i].price < underlyingInfo.currentPrice && profitDataList[i+1].price >= underlyingInfo.currentPrice)
		{
			strategyResult.unchangedProfit = (profitDataList[i].profit + profitDataList[i+1].profit)/2.0;
		}
	} //for i

	strategyResult.maximProfit = 0;
	strategyResult.maximLoss = 0;
	strategyResult.winProbability = 0;
	strategyResult.loseProbability = 0;
	strategyResult.averageProfit = 0;
	for(int i = 0; i < profitListSize; i++) { 
		if(profitDataList[i].profit > strategyResult.maximProfit) 
			strategyResult.maximProfit = profitDataList[i].profit;

		if(profitDataList[i].profit < strategyResult.maximLoss)
			strategyResult.maximLoss = profitDataList[i].profit;

		if(profitDataList[i].profit > 0)
			strategyResult.winProbability = strategyResult.winProbability + profitDataList[i].probability;
		else
			strategyResult.loseProbability = strategyResult.loseProbability + profitDataList[i].probability;

		strategyResult.averageProfit = strategyResult.averageProfit + profitDataList[i].profit * profitDataList[i].probability;
	}

	return bRetVal;
}

//++
//计算日间历史波动率
//计算资产在各个价格上的概率分布。
//参数
//    pDayKlineList: 指向日K线数据列表的指针
//    NumOfDays: 所使用的天数
//返回值
//    double: 波动率，用百分比表示
//--
double CalcInterDayHv(vector<CDayKlineItem>* pDayKlineList, size_t NumOfDays)
{
	return 20.0;
}

//++
//计算合约某个时段长度的加权平均价格
//参数
//    MaketDataList: 市场行情数据列表
//    TimeSpanSeconds: 计算的时间跨度，以秒为单位
//返回值
//    按成交量加权的价格平均值
//--
double CalcWeightedAvgPrice(vector<CThostFtdcDepthMarketDataField> MarketDataList, double TimeSpanSeconds)
{
	double eps = 0.1;
	double BeginTimeSeconds = 0;
	double EndTimeSeconds = 0;
	double TimeSummed = 0;
	double DeltaVolume = 0;
	double SigmaVolume = 0;
	double WeightedPrice = 0;
	double AvgPrice = 0;
	int i = MarketDataList.size();

	//当长时间无成交时可能导致SigmaVolume为0，从而AvgPrice为0，将会导致不正常开仓（AvgPrice为0通常出现在成交不活跃的远月合约）
	while(TimeSummed < TimeSpanSeconds && i > 0)
	{
		i = i - 1;
		if(i > 0)
			DeltaVolume = MarketDataList[i].Volume - MarketDataList[i-1].Volume;
		//else if(i == 0)   //第一个数据可能不是开盘第一个数据，Volume会非常大
		//	DeltaVolume = MarketDataList[i].Volume;

		//为避免出现全部DeltaVolume为0导致SigmaVolume为0的情况出现，将DeltaVolume加上一个小值
		DeltaVolume = abs(DeltaVolume) + eps;

		SigmaVolume = SigmaVolume + DeltaVolume;
		WeightedPrice = WeightedPrice + DeltaVolume * MarketDataList[i].LastPrice;

		int UpdateMillisec = MarketDataList[i].UpdateMillisec;
		string str1 = MarketDataList[i].UpdateTime; //UpdateTime的格式为10:50:30
		string str2 = str1;
		string str3 = str1;

		str1 = str1.substr(0, 2);
		str2 = str2.substr(3, 2);
		str3 = str3.substr(6, 2);

	    int hours = atoi(str1.c_str());
		int minutes = atoi(str2.c_str());
		int seconds = atoi(str3.c_str());

		BeginTimeSeconds = hours*3600 + minutes*60 + seconds + UpdateMillisec*(1/1e3);
		if(i == (MarketDataList.size() - 1))
			EndTimeSeconds = BeginTimeSeconds;

		TimeSummed = EndTimeSeconds - BeginTimeSeconds;
	};

	AvgPrice = WeightedPrice / SigmaVolume;

	return AvgPrice;
}

//++
//判断是否是极端价格, 用于K线数据的处理
//参数
//    Price: 当前输入价格
//返回值
//    true: 是极端价格; false: 正常价格
//--
bool IsExtremePrice(double Price)
{
	double MinPrice = 0.000001;
	double MaxPrice = 1000000;
	bool ret = false;

	if(Price < MinPrice || Price > MaxPrice)
	{
		ret = true;
	}

	return ret;
}

//++
//是否有行情报价或者行情报价是否正常
//参数
//    pMd: 指向深度市场行情的指针
//返回值
//    true: 有正常的市场行情报价; false: 无正常的市场行情报价
//--
bool IsDepthMarketDataOk(CThostFtdcDepthMarketDataField* pMd)
{
	bool bRet = true;

	if(pMd->AskPrice1 > pMd->UpperLimitPrice || pMd->AskPrice1 < pMd->LowerLimitPrice 
		|| pMd->BidPrice1 > pMd->UpperLimitPrice || pMd->BidPrice1 < pMd->LowerLimitPrice)
	{
		bRet = false;
	}

	return bRet;
}

//++
//获取涨停板价格
//主要用于校正行情数据中出现涨跌停板价格为0的情况
//参数
//    pMd: 市场行情数据结构指针
//    InstrumentType: 交易对象类型
//    pUnderMd: 指向标的合约的市场行情数据结构指针, 计算期权涨跌停价格用
//返回值
//    涨停板价格
//--
double GetUpperLimitPrice(CThostFtdcDepthMarketDataField* pMd, char InstrumentType, CThostFtdcDepthMarketDataField* pUnderMd)
{
	double UpperLimitPrice = pMd->UpperLimitPrice;

	if(InstrumentType == 'u')
	{
		//目前只针对上交所处理
		if(strcmp(pMd->ExchangeID, "SSE") == 0 && IsExtremePrice(UpperLimitPrice) == true)
		{
			if(IsExtremePrice(pMd->PreClosePrice) == false)
			{
				UpperLimitPrice = pMd->PreClosePrice*1.1;
				//四舍五入保留三位小数
				UpperLimitPrice = int(UpperLimitPrice*1000 + 0.5)/1000.0;
			}
		}
	}

	return UpperLimitPrice;
}

//++
//获取涨停板价格
//主要用于校正行情数据中出现涨跌停板价格为0的情况
//参数
//    pMd: 市场行情数据结构指针
//    InstrumentType: 交易对象类型
//    pUnderMd: 指向标的合约的市场行情数据结构指针, 计算期权涨跌停价格用
//返回值
//    跌停板价格
//--
double GetLowerLimitPrice(CThostFtdcDepthMarketDataField* pMd, char InstrumentType, CThostFtdcDepthMarketDataField* pUnderMd)
{
	double LowerLimitPrice = pMd->LowerLimitPrice;

	if(InstrumentType == 'u')
	{
		//目前只针对上交所处理
		if(strcmp(pMd->ExchangeID, "SSE") == 0 && IsExtremePrice(LowerLimitPrice) == true)
		{
			if(IsExtremePrice(pMd->PreClosePrice) == false)
			{
				LowerLimitPrice = pMd->PreClosePrice*0.9;
				//四舍五入保留三位小数
				LowerLimitPrice = int(LowerLimitPrice*1000 + 0.5)/1000.0;
			}
		}
	}

	return LowerLimitPrice;
}

//++
//计算标的行情的买卖委托量比例(委买手数 - 委卖手数)/(委买手数 + 委卖手数) * 100
//参数
//    pMd: 市场行情数据结构
//返回值
//    无
//--
double BidAskDiffRatio(CThostFtdcDepthMarketDataField* pMd)
{
	double DiffRatio = 0;
	double eps = 0.000001;
	double BidVolume = pMd->BidVolume1 + pMd->BidVolume2 + pMd->BidVolume3 + pMd->BidVolume4 + pMd->BidVolume5;
	double AskVolume = pMd->AskVolume1 + pMd->AskVolume2 + pMd->AskVolume3 + pMd->AskVolume4 + pMd->AskVolume5;

	DiffRatio = 100.0 * (BidVolume - AskVolume) / (BidVolume + AskVolume + eps);
	return DiffRatio;
}

//++
//计算标的多空情况
//参数
//    pTshapeMonth: 指向对应月份T型报价的指针
//    TickDataLen: 使用的TickData数据列表的长度
//    DirIndicator: 引用参数
//                  > 0 : 强烈看多
//                  = 0 : 偏中性
//                  < 0 : 强烈看空
//返回值
//    true : TickData列表长度大于TickDataLen, 可以获得多空信号
//    false: TickData列表长度小于TickDataLen, 不能获得多空信号
//--
bool GetUnderTickSignal(CTshapeMonth *pTshapeMonth, size_t TickDataLen, int &DirIndicator)
{
	bool ret = true;
	DirIndicator = 0;
	double eps = 0.000001;
	vector<CThostFtdcDepthMarketDataField> TickDataList;

	//获取用于计算的TickData数据
	pTshapeMonth->readlock();
	size_t ListSize = pTshapeMonth->m_UnderlyingTickDataList.size();
	if(ListSize > TickDataLen)
	{
        vector<CThostFtdcDepthMarketDataField>::iterator ItBegin = pTshapeMonth->m_UnderlyingTickDataList.end() - (TickDataLen + 1);
		TickDataList.insert(TickDataList.end(), ItBegin, pTshapeMonth->m_UnderlyingTickDataList.end());
	}
	else
	{
		ret = false; //不能在同步加锁区域内返回
	}
	pTshapeMonth->readunlock();

	double PlusTickRatio = 0;
	double MinusTickRatio = 0;
	double MeanBidRatio = 0;
	if(ret == true)
	{
		int PlusTickCnt = 0;
		int MinusTickCnt = 0;
		int NeutralTickCnt = 0;
		double lambda = 0.7;
		for(size_t i = 1; i < TickDataList.size(); i++)
		{
			CThostFtdcDepthMarketDataField *pCurMd = &TickDataList[i];
			CThostFtdcDepthMarketDataField *pPreMd = &TickDataList[i-1];

			//计算涨码和跌码
			if(pCurMd->LastPrice > pPreMd->LastPrice)
				PlusTickCnt++;
			else if(pCurMd->LastPrice < pPreMd->LastPrice)
				MinusTickCnt++;
			else if(pCurMd->LastPrice >= (pCurMd->AskPrice1 - eps) && pCurMd->AskPrice1 >= (pPreMd->AskPrice1 - eps)) //以卖报价成交, 且当前卖报价大于等于前卖报价
				PlusTickCnt++;
			else if(pCurMd->LastPrice <= (pCurMd->BidPrice1 + eps) && pCurMd->BidPrice1 <= (pPreMd->BidPrice1 + eps)) //以买报价成交, 且当前买报价小于等于前买报价
				MinusTickCnt++;
			else //以买卖报价中间的价格成交
				NeutralTickCnt++;

			//计算买卖委托比例的平均值
			double BidRatio = BidAskDiffRatio(pCurMd);
			if( i == 1)
				MeanBidRatio = BidRatio;
			else
				MeanBidRatio = lambda * MeanBidRatio + (1 - lambda) * BidRatio;
		} //for i;
		double TotalTickCnt =  double(PlusTickCnt + MinusTickCnt + NeutralTickCnt);
		PlusTickRatio = double(PlusTickCnt) / TotalTickCnt;
		MinusTickRatio = double(MinusTickCnt) / TotalTickCnt;

		//计算多空信号
		if(PlusTickRatio > 0.75 || MeanBidRatio > 45)
			DirIndicator = 1;
		else if(MinusTickRatio > 0.75 || MeanBidRatio < (-45))
			DirIndicator = -1;
		else
			DirIndicator = 0;
	}

	return ret;
}
