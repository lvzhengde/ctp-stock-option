//++
//定义T型报价用到的类和数据结构
//--

#ifndef _TSHAPE_DATA_CLASS_H_
#define _TSHAPE_DATA_CLASS_H_
 
#include <string>  
#include <vector>
#ifdef WIN_CTP
#include "windows.h"
#include <process.h>
#else
#include <pthread.h>
#include "WinFuncSim.h"
#endif
#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"
#include "UserDefinedDataStruct.h"
#include "SafeVector.h"

using namespace std;  

//前向声明各指针用到了类
//注意：只能用于定义指针
class COptionPairItem;
class CTshapeMonth;
class CTshapeBase;

//单一期权结构
class COptionItem 
{
private:
	SRWLOCK m_RwLock;

public:
	//期权合约相关信息
	CThostFtdcInstrumentField m_InstInfo;
	//从行情API得到的当前行情数据
	CThostFtdcDepthMarketDataField  m_DepthMarketData;
	//当前合约交易状态(取自结构CThostFtdcInstrumentStatusField的状态域)
	TThostFtdcInstrumentStatusType m_InstrumentStatus;
	//期权内在价值
	double m_IntrinsicValue;
	//虚值程度
	double m_OutOfTheMoney;
	//期权时间价值
	double m_TimeValue;
	//单手期权所需的保证金
	double m_Margin;   
	//各种希腊值
	double m_Delta;
	double m_Gamma;
	double m_Vega;
	double m_Theta;
	double m_Rho;
	//隐含波动率, 以百分比表示
	double m_ImpliedVolatility;
	//平均隐含波动率, 以百分比表示
	double m_MeanIV;
	//当前波动率与平均波动率偏差的平均值, 以百分比表示
	double m_MeanIVDev;
	//期权理论价格，由实际波动率算得
	double m_TheoreticalPrice;
    //更新期权合约的最新Tick Data Count
    size_t m_TickCountStamp;
	//本期权累计撤单次数
	int m_CancelTimes;
	//指向父类结构的指针, 便于追溯
	COptionPairItem* m_pParent;

public:
	//构造函数
	COptionItem()
	{
		InitializeSRWLock(&m_RwLock);
	    memset(&m_InstInfo, 0, sizeof(CThostFtdcInstrumentField));
		memset(&m_DepthMarketData, 0, sizeof(CThostFtdcDepthMarketDataField));
		m_InstrumentStatus = THOST_FTDC_IS_Continous;  //使程序重启后缺省为可交易状态
		m_IntrinsicValue = 0;
		m_OutOfTheMoney = 0;
		m_TimeValue = 0;
		m_Margin = 0;   
		m_Delta = 0;
		m_Gamma = 0;
		m_Vega = 0;
		m_Theta = 0;
		m_Rho = 0;
		m_ImpliedVolatility = 0;
		m_MeanIV = 0;
		m_MeanIVDev = 0;
		m_TheoreticalPrice = 0;
		m_TickCountStamp = 0;
		m_CancelTimes = 0;
	    m_pParent = NULL;
	}

	//拷贝构造函数, 已经在函数内部加锁
	//如果出现在vector定义中不是指针, 引用参数必须加const否则编译会报错
	COptionItem(COptionItem& rhs)
	{
		InitializeSRWLock(&m_RwLock);

		AcquireSRWLockShared(&rhs.m_RwLock);		
		*this = rhs;
		ReleaseSRWLockShared(&rhs.m_RwLock);	
	}

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

	//重载 "="操作符
	//定义对象初始化需要使用拷贝函数
	COptionItem& operator=(const COptionItem &rhs)
	{
	    memcpy(&m_InstInfo, &rhs.m_InstInfo, sizeof(CThostFtdcInstrumentField));
		memcpy(&m_DepthMarketData, &rhs.m_DepthMarketData, sizeof(CThostFtdcDepthMarketDataField));
		m_InstrumentStatus = rhs.m_InstrumentStatus;
		m_IntrinsicValue = rhs.m_IntrinsicValue;
		m_OutOfTheMoney = rhs.m_OutOfTheMoney;
		m_TimeValue = rhs.m_TimeValue;
		m_Margin = rhs.m_Margin;   
		m_Delta = rhs.m_Delta;
		m_Gamma = rhs.m_Gamma;
		m_Vega = rhs.m_Vega;
		m_Theta = rhs.m_Theta;
		m_Rho = rhs.m_Rho;
		m_ImpliedVolatility = rhs.m_ImpliedVolatility;
		m_MeanIV = rhs.m_MeanIV;
		m_MeanIVDev = rhs.m_MeanIVDev;
		m_TheoreticalPrice = rhs.m_TheoreticalPrice;
		m_TickCountStamp = rhs.m_TickCountStamp;
		m_CancelTimes = rhs.m_CancelTimes;
	    m_pParent = rhs.m_pParent;

		return *this;
	}

	//加锁获取市场行情数据, 注意外面不能再加锁
	CThostFtdcDepthMarketDataField GetLockedMarketData()
	{
		CThostFtdcDepthMarketDataField ret;

		AcquireSRWLockShared(&m_RwLock);
		memcpy(&ret, &m_DepthMarketData, sizeof(CThostFtdcDepthMarketDataField));
		ReleaseSRWLockShared(&m_RwLock);

		return ret;
	}

	//获取加锁的合约交易状态信息
	TThostFtdcInstrumentStatusType GetLockedInstrumentStatus()
	{
		TThostFtdcInstrumentStatusType ret;

		AcquireSRWLockShared(&m_RwLock);
		ret = m_InstrumentStatus;
		ReleaseSRWLockShared(&m_RwLock);

		return ret;
	}
};

//同一行权价的看涨和看跌期权结构
class COptionPairItem 
{
private:
	SRWLOCK m_RwLock;

public:
	//行权价
	double m_StrikePrice;
	//合约调整标志，适用于上期所股票期权
	char m_AdjustFlag;
	//根据平价套利公式计算得到的平均利率, 以百分比表示
	double m_MeanRate;
	//看涨期权
	COptionItem* m_pCallItem;
	//看跌期权
	COptionItem* m_pPutItem;
	//指向父类结构的指针, 便于追溯
	CTshapeMonth* m_pParent;

public:
	COptionPairItem() 
	{
		InitializeSRWLock(&m_RwLock);

		m_StrikePrice = 0;
		m_AdjustFlag = ' ';
		m_MeanRate = 0;
		m_pCallItem = NULL;
		m_pPutItem = NULL;
		m_pParent = NULL;
	}

	//如果出现在vector定义中不是指针, 引用参数必须加const否则编译会报错
	COptionPairItem(COptionPairItem& rhs)
	{
		InitializeSRWLock(&m_RwLock);

		AcquireSRWLockShared(&rhs.m_RwLock);		
		*this = rhs;
		ReleaseSRWLockShared(&rhs.m_RwLock);	
	}

	//析构函数
	virtual ~COptionPairItem()
	{
		if(m_pCallItem != NULL)
		{
			delete m_pCallItem;
			m_pCallItem = NULL;
		}

		if(m_pPutItem != NULL)
		{
			delete m_pPutItem;
			m_pPutItem = NULL;
		}
	}

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

	//获取同步加锁后的平均收益率
	double GetLockedMeanRate()
	{
		double ret;

		AcquireSRWLockShared(&m_RwLock);
		ret = m_MeanRate;
		ReleaseSRWLockShared(&m_RwLock);

		return ret;
	}
};

//用以计算标的平均价的结构
struct CUnderAvg 
{
	//计算得到的平均价格
	double AvgPrice;
	//计算得到的价格差的平方和
	double SumSquarePriceDiff;
	//计算得到的Tick价格方差
	double StdTickPrice;
	//本时段累计成交量
	double SigmaVolume;
	//计算用到的总的Tick数目
	int TickNumForCalc;
	//当前平均价格开始计算时的列表位置
	int StartPos;
	//当前平均价格计算结束时的列表位置
	int EndPos;
};

//单一月份T型报价结构
class CTshapeMonth 
{
private:
	SRWLOCK m_RwLock;

public:
	//期权标的对应的年月分, YYYYMM(201705)格式
	size_t m_YearMonth;
	//标的或替代标的合约代码
	TThostFtdcInstrumentIDType  m_UnderlyingInstId;
	//标的合约相关信息
	CThostFtdcInstrumentField  m_UnderlyingInstInfo;
	//标的合约当前市场行情数据
	CThostFtdcDepthMarketDataField m_UnderlyingMarketData;
	//标的合约当前交易状态信息
	TThostFtdcInstrumentStatusType m_UnderlyingInstrumentStatus;
	//标的合约的Tick数据列表
	vector<CThostFtdcDepthMarketDataField> m_UnderlyingTickDataList;
	//计算平均价的结构, 用以计算本交易时段以成交量加权的平均价格
	CUnderAvg m_UnderAvg;
	//标的累计撤单次数
	int m_UnderCancelTimes;
	//标的合约日K线数据列表
	vector<CDayKlineItem> m_DayKlineList;
	//标的的保证金率，对于期货各个月份合约的保证金率可能不同
	CThostFtdcInstrumentMarginRateField m_UnderlyingInstMarginRate;
	//标的的手续费率，对于期货各个月份的手续费率也可能不同
	CThostFtdcInstrumentCommissionRateField m_UnderlyingInstCommissionRate;
	//期权合约手续费率，对于期货期权各个月份的手续费率可能不同
	CThostFtdcOptionInstrCommRateField m_OptionInstCommRate;
	//单手标的开多单所需要的保证金
	double m_LongUnderlyingMargin;
	//单手标的开空单所需要的保证金
	double m_ShortUnderlyingMargin;
	//标的合约的日间历史波动率, 以百分比表示
	double m_InterDayHv;
	//标的合约的日内历史波动率，以百分比表示
	double m_IntraDayHv;
	//根据平价套利公式计算得到的平均利率, 以百分比表示
	double m_MeanRate;
	//使用本月全部Call/Put期权计算得到的平均隐含波动率, 以百分比表示
	double m_MeanIV;
	//使用本月Call期权计算得到的平均隐含波动率, 以百分比表示
	double m_MeanCallIV;
	//使用本月Put期权计算得到的平均隐含波动率, 以百分比表示
	double m_MeanPutIV;
	//期权离到期剩下的天数
	int m_ExpirationLeftDays;
	//T型报价各个行权价期权对列表
	vector<COptionPairItem*> m_pOptionPairList;
    //更新标的合约的最新Tick Data Count
    size_t m_TickCountStamp;
	//指向父类结构的指针, 便于追溯
	CTshapeBase* m_pParent;

public:
	//缺省构造函数
	CTshapeMonth()
	{
		InitializeSRWLock(&m_RwLock);
	    m_YearMonth = 0;
		memset(m_UnderlyingInstId, 0, sizeof(TThostFtdcInstrumentIDType));
	    memset(&m_UnderlyingInstInfo, 0, sizeof(CThostFtdcInstrumentField));
	    memset(&m_UnderlyingMarketData, 0, sizeof(CThostFtdcDepthMarketDataField));
		m_UnderlyingInstrumentStatus = THOST_FTDC_IS_Continous;  //使程序重启后缺省为可交易状态
		memset(&m_UnderAvg, 0, sizeof(CUnderAvg));
	    m_UnderCancelTimes = 0;
	    m_LongUnderlyingMargin = 0;
	    m_ShortUnderlyingMargin = 0;
	    m_InterDayHv = 0;
	    m_IntraDayHv = 0;
		m_MeanRate = 0;
		m_MeanIV = 0;
		m_MeanCallIV = 0;
		m_MeanPutIV = 0;
	    m_ExpirationLeftDays = 0;
        m_TickCountStamp = 0;
	    m_pParent = NULL;
	}

	//拷贝构造函数, 已经在函数内部加锁
	//如果出现在vector定义中不是指针, 引用参数必须加const否则编译会报错
	CTshapeMonth(CTshapeMonth& rhs)
	{
		InitializeSRWLock(&m_RwLock);

		AcquireSRWLockShared(&rhs.m_RwLock);		
		*this = rhs;
		ReleaseSRWLockShared(&rhs.m_RwLock);
	}

	//析构函数
	virtual ~CTshapeMonth()
	{
		for(size_t i = 0; i < m_pOptionPairList.size(); i++)
		{
			delete m_pOptionPairList[i];
			m_pOptionPairList[i] = NULL;
		}
	}

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

	//重载 "="操作符
	//定义时初始化需要使用拷贝构造函数
	CTshapeMonth& operator=(const CTshapeMonth &rhs)
	{
	    m_YearMonth = rhs.m_YearMonth;
	    strcpy_s(m_UnderlyingInstId, rhs.m_UnderlyingInstId);
	    memcpy(&m_UnderlyingInstInfo, &rhs.m_UnderlyingInstInfo, sizeof(CThostFtdcInstrumentField));
	    memcpy(&m_UnderlyingMarketData, &rhs.m_UnderlyingMarketData, sizeof(CThostFtdcDepthMarketDataField));
		m_UnderlyingInstrumentStatus = rhs.m_UnderlyingInstrumentStatus;
		memcpy(&m_UnderAvg, &rhs.m_UnderAvg, sizeof(CUnderAvg));
	    m_UnderCancelTimes = rhs.m_UnderCancelTimes;
	    m_LongUnderlyingMargin = rhs.m_LongUnderlyingMargin;
	    m_ShortUnderlyingMargin = rhs.m_ShortUnderlyingMargin;
	    m_InterDayHv = rhs.m_InterDayHv;
	    m_IntraDayHv = rhs.m_IntraDayHv;
		m_MeanRate = rhs.m_MeanRate;
		m_MeanIV = rhs.m_MeanIV;
		m_MeanCallIV = rhs.m_MeanCallIV;
		m_MeanPutIV = rhs.m_MeanPutIV;
	    m_ExpirationLeftDays = rhs.m_ExpirationLeftDays;
        m_TickCountStamp = rhs.m_TickCountStamp;
	    m_pParent = rhs.m_pParent;

		return *this;
	}

	//加锁获取标的市场行情数据, 注意外面不能再加锁
	CThostFtdcDepthMarketDataField GetLockedMarketData()
	{
		CThostFtdcDepthMarketDataField ret;

		AcquireSRWLockShared(&m_RwLock);
		memcpy(&ret, &m_UnderlyingMarketData, sizeof(CThostFtdcDepthMarketDataField));
		ReleaseSRWLockShared(&m_RwLock);

		return ret;
	}

	//获取加锁的合约交易状态信息
	TThostFtdcInstrumentStatusType GetLockedInstrumentStatus()
	{
		TThostFtdcInstrumentStatusType ret;

		AcquireSRWLockShared(&m_RwLock);
		ret = m_UnderlyingInstrumentStatus;
		ReleaseSRWLockShared(&m_RwLock);

		return ret;
	}

	//获取同步加锁后的平均收益率
	double GetLockedMeanRate()
	{
		double ret;

		AcquireSRWLockShared(&m_RwLock);
		ret = m_MeanRate;
		ReleaseSRWLockShared(&m_RwLock);

		return ret;
	}
};

#endif
