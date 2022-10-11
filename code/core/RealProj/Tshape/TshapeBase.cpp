#include "TshapeBase.h"
#include "OptionPriceModel.h"
#include "AppConfig.h"
#include "RealApp.h"

#define MAX_UNDER_TICKDATA_SIZE     (1200)    //标的保留的最大Tick数据数目
#define MIN_CAL_LEFT_DAYS           (3)       //计算T型报价隐含波动率/年化利率平均值所用月份的最低剩余天数
#define MAX_IMPLIED_VLTY            (500)     //隐含波动率的最大值
#define IIR_LAMBDA                  (0.90)    //使用IIR滤波器计算各种平均值的
#define DST_UPPER_RATIO             (0.25)    //离开平值此距离比例的期权不再参与平均波动率等计算
#define MAX_PAR_RATE                (10.0)    //根据平价套利算出来的年化收益率的最大百分比数值限制
//平价套利年化收益率最小百分比限制
#ifndef FUTURE_OPTION_ONLY
	#define MIN_PAR_RATE            (0)        //股票期权须不小于零
#else
    #define MIN_PAR_RATE            (-10.0)    //期货期权可能出现负值
#endif
//++
//构造函数
//注意：不能在基类的构造函数和析构函数中调用纯虚函数（此时虚函数表尚未生成)
//可以在普通函数中调用纯虚函数
//--
CTshapeBase::CTshapeBase()
{
    InitializeSRWLock(&m_RwLock);

	m_pRealApp = NULL;
	m_pOptionKind = NULL;

	//cerr << "调用纯虚函数基类的构造函数" << ENDLINE;
	m_bUnderMdOnly = false;
	m_TickCountStamp = 0;
	m_MeanIV = 0;
}

//++
//析构函数
//--
CTshapeBase::~CTshapeBase() 
{
	for(size_t i = 0; i < m_pTshapeMonthList.size(); i++)
	{
		delete m_pTshapeMonthList[i];
		m_pTshapeMonthList[i] = NULL;
	}
};

//++
//基类强制实现的普通函数，用于参数订阅行情的合约代码列表
//参数
//    CntCode: 交易柜台代码
//    strInstIdList: 引用参数, 用以存放T型报价和CntCode对应的柜台的合约代码列表
//返回值
//    无
//--
void CTshapeBase::GenInstrumentList(char CntCode, vector<string> &strInstIdList)
{
	size_t i;
	size_t j;
    string strInstId;

	strInstIdList.clear();
	for(i = 0; i < m_pTshapeMonthList.size(); i++)
	{
		strInstId = m_pTshapeMonthList[i]->m_UnderlyingInstId;
		if(CntCode == m_UnderCntCode)
		{
			if(m_UnderlyingAssetType == 0 || m_UnderlyingAssetType == 1)       //标的为商品期货或者股指期货
			{
				strInstIdList.push_back(strInstId);
			}
			else if(m_UnderlyingAssetType == 2 || m_UnderlyingAssetType == 3)  //标的为ETF或者股票
			{
				//标的都一样，取第一个就行了
				if(i == 0) strInstIdList.push_back(strInstId);
			}
		}

		if(CntCode == m_OptCntCode)
		{
			for(j = 0; j < m_pTshapeMonthList[i]->m_pOptionPairList.size(); j++)
			{
				strInstId = m_pTshapeMonthList[i]->m_pOptionPairList[j]->m_pCallItem->m_InstInfo.InstrumentID;
				strInstIdList.push_back(strInstId);

				strInstId = m_pTshapeMonthList[i]->m_pOptionPairList[j]->m_pPutItem->m_InstInfo.InstrumentID;
				strInstIdList.push_back(strInstId);
			}
		}
	} //for i
}

//++
//基类强制实现的普通函数，用于计算期权的时间价值，内在价值和虚值程度
//参数
//    OptionType: 期权类型'C'/'c'看涨期权，'P'/'p'看跌期权
//    UnderlyingPrice: 标的价格
//    StrikePrice: 行权价
//    Royalty: 权利金
//    IntrinsicValue：内在价值
//    OutOfTheMoney: 虚值程度
//返回值
//    时间价值，如果为负则表示有时间贴水
//--
double CTshapeBase::CalcTimeValue(char OptionType, double UnderlyingPrice, double StrikePrice, double Royalty, double& IntrinsicValue, double& OutOfTheMoney)
{
	double TimeValue = 0;

	if(OptionType == 'C' || OptionType == 'c')
	{
		if(UnderlyingPrice >= StrikePrice)
		{
			IntrinsicValue = UnderlyingPrice - StrikePrice;
			OutOfTheMoney = 0;
			TimeValue = Royalty - IntrinsicValue;
		}
		else
		{
			IntrinsicValue = 0;
			OutOfTheMoney = StrikePrice - UnderlyingPrice;
			TimeValue = Royalty;
		}
	}
	else if(OptionType == 'P' || OptionType == 'p')
	{
		if(UnderlyingPrice >= StrikePrice)
		{
			IntrinsicValue = 0;
			OutOfTheMoney = UnderlyingPrice - StrikePrice;
			TimeValue = Royalty;
		}
		else
		{
			IntrinsicValue = StrikePrice - UnderlyingPrice;
			OutOfTheMoney = 0;
			TimeValue = Royalty - IntrinsicValue;
		}
	}

	return TimeValue;
}

//++
//计算标的本交易时段的以成交量加权的平均价格，使用增量计算的方法以加快时间
//参数
//    bFirstMd: 确定是否是第一次获得市场行情数据
//    MarketDataList: 市场行情数据列表
//    UnderAvg: 引用参数，标的平均价格结构，包含计算所需的一些数据
//返回值
//    无
//--
void CTshapeBase::CalcUnderAvgPrice(bool bFirstMd, vector<CThostFtdcDepthMarketDataField> MarketDataList, CUnderAvg& UnderAvg)
{
	double eps = 0.000001;
	double DeltaVolume = 0;
	double SigmaVolume = 0;
	double WeightedPrice = 0;
	double Price = 0;

	//注意：可能一下来很多行情数据
	if(bFirstMd == true)
	{
		for(size_t i = 0; i < MarketDataList.size(); i++)
		{
			//加eps是防止出现连续成交量为0的情况
			if(i == 0)
				DeltaVolume = eps;
			else
				DeltaVolume = MarketDataList[i].Volume - MarketDataList[i-1].Volume + eps;

			SigmaVolume += DeltaVolume;
			Price = (MarketDataList[i].AskPrice1 + MarketDataList[i].BidPrice1) * 0.5;
			WeightedPrice += DeltaVolume * Price;
		}

		UnderAvg.AvgPrice = WeightedPrice / SigmaVolume;
		UnderAvg.SigmaVolume = SigmaVolume;
		UnderAvg.StartPos = 0;

		//计算标准差等统计值
		for(size_t i = 0; i < MarketDataList.size(); i++)
		{
			UnderAvg.TickNumForCalc++;
			Price = (MarketDataList[i].AskPrice1 + MarketDataList[i].BidPrice1) * 0.5;
			UnderAvg.SumSquarePriceDiff += (Price - UnderAvg.AvgPrice) * (Price - UnderAvg.AvgPrice);
		}
		if(UnderAvg.TickNumForCalc > 0) UnderAvg.StdTickPrice = sqrt(UnderAvg.SumSquarePriceDiff/UnderAvg.TickNumForCalc);
		UnderAvg.EndPos = MarketDataList.size() - 1;
	}
	else if(MarketDataList.size() > 1)
	{
		for(size_t i = UnderAvg.EndPos + 1; i < MarketDataList.size(); i++)
		{
			DeltaVolume = MarketDataList[i].Volume - MarketDataList[i-1].Volume + eps;
			SigmaVolume += DeltaVolume;
			Price = (MarketDataList[i].AskPrice1 + MarketDataList[i].BidPrice1) * 0.5;
			WeightedPrice += DeltaVolume * Price;
		}

		WeightedPrice += UnderAvg.SigmaVolume * UnderAvg.AvgPrice;
		SigmaVolume += UnderAvg.SigmaVolume;
		UnderAvg.AvgPrice = WeightedPrice / SigmaVolume;
		UnderAvg.SigmaVolume = SigmaVolume;

		//计算标准差等统计数据
		for(size_t i = UnderAvg.EndPos + 1; i < MarketDataList.size(); i++)
		{
			UnderAvg.TickNumForCalc++;
			Price = (MarketDataList[i].AskPrice1 + MarketDataList[i].BidPrice1) * 0.5;
			UnderAvg.SumSquarePriceDiff += (Price - UnderAvg.AvgPrice) * (Price - UnderAvg.AvgPrice);
		}
		if(UnderAvg.TickNumForCalc > 0) UnderAvg.StdTickPrice = sqrt(UnderAvg.SumSquarePriceDiff/UnderAvg.TickNumForCalc);
		UnderAvg.EndPos = MarketDataList.size() - 1;
	}
}

//++
//缩减保留的市场行情数据
//参数
//    pTshapeMonth: 指向月份T型报价的指针
//    KeepLen: 要保留的长度，注意头部的60个TickData必须保留
//返回值
//    无
//--
void CTshapeBase::TruncTickData(CTshapeMonth *pTshapeMonth, size_t KeepLen)
{
	vector<CThostFtdcDepthMarketDataField> *pMarketDataList = &pTshapeMonth->m_UnderlyingTickDataList;
	size_t MdSize = pMarketDataList->size();

	if(MdSize > (KeepLen+60))
	{
		vector<CThostFtdcDepthMarketDataField> tmpMdList;
		vector<CThostFtdcDepthMarketDataField>::iterator it = pMarketDataList->begin() + 60;
		//保留头60个Tick数据
		tmpMdList.insert(tmpMdList.end(), pMarketDataList->begin(), it);
		//保留后KeepLen个Tick数据
		it = pMarketDataList->end() - KeepLen;
		tmpMdList.insert(tmpMdList.end(), it, pMarketDataList->end());
		//交换列表，得到最后缩减后的市场行情数据
		pMarketDataList->swap(tmpMdList);

		pTshapeMonth->m_UnderAvg.EndPos = pMarketDataList->size() - 1;
	}
}

//++
//根据OnRtnDepthMarketData返回的行情数据列表，更新T型报价结构
//参数
//    MarketDataList: 引用参数，市场行情数据列表
//                    当一个合约代码的行情数据已经使用完毕，将被标记为不可用
//返回值
//    无
//--
void CTshapeBase::UpdateTshape(vector<CThostFtdcDepthMarketDataField>& MarketDataList)
{
	//可以接受的浮点数表示精度误差
	double eps = (1e-6);
	void *pNode = NULL;

	//集合竞价时段不能计算标的平均价格等值, 否则可能给出不正常的值
	//但开盘后应该立即计算, 否则各个合约的保证金计算将不正常, 所以给出一个提前时间进行计算
	double StartAdjSecs = -5.0; 
	bool bAllowedRefreshTime = IsAllowedRefreshTime(StartAdjSecs);

	//每一次更新都将顶层的m_TickCountStamp加1
	m_TickCountStamp = m_TickCountStamp + 1;

	for(size_t i = 0; i < MarketDataList.size(); i++)
	{
		size_t YearMonth;
		double StrikePrice;
		char OptionTypeCode;
		int RetVal;

		bool found = false;
		string strExchangeID = MarketDataList[i].ExchangeID;
		RemoveSpace(strExchangeID);

		if(MarketDataList[i].InstrumentID[0] == '@')   //该合约已经被使用，不必再处理，以加快速度
		{
			RetVal = -1;
		}
		//合约交易所代码和T型报价交易所代码不一致，不必处理，以加快速度
		//注意: 只有上交所和深交所行情中有交易所代码，中金所/上期所/郑商所/大商所的行情中ExchangeID为空
		else if((strExchangeID != "") && (strcmp(m_ExchangeId, MarketDataList[i].ExchangeID) != 0)) 
		{
			RetVal = -1;
		}
		else if(m_UnderlyingAssetType == 0 || m_UnderlyingAssetType == 1)  //标的为商品期货或者股指期货
		{
			RetVal = ParseInstrumentId(MarketDataList[i].InstrumentID, YearMonth, StrikePrice, OptionTypeCode, pNode);
		}
		else if(m_UnderlyingAssetType == 2 || m_UnderlyingAssetType == 3)  //标的为ETF或者股票
		{
			RetVal = ParseInstrumentId(MarketDataList[i].InstrumentID, YearMonth, StrikePrice, OptionTypeCode, pNode);
		}

		//更新相应合约的市场行情信息
		if((RetVal == 1 || RetVal == 0) && (m_UnderlyingAssetType == 0 || m_UnderlyingAssetType == 1))  //标的为期货
		{
			for(size_t j = 0; j < m_pTshapeMonthList.size(); j++)
			{
				if(YearMonth == m_pTshapeMonthList[j]->m_YearMonth)
				{
					if(RetVal == 1)  //标的合约
					{
						m_pTshapeMonthList[j]->writelock();

						//只有在正常交易时段才更新标的行情列表和平均值
						if(bAllowedRefreshTime == true)
						{
							bool bFirstMd = (m_pTshapeMonthList[j]->m_UnderlyingTickDataList.size() == 0) ? true : false;
							m_pTshapeMonthList[j]->m_UnderlyingTickDataList.push_back(MarketDataList[i]);
							//更新标的的价格成交量加权平均值
							CalcUnderAvgPrice(bFirstMd, m_pTshapeMonthList[j]->m_UnderlyingTickDataList, m_pTshapeMonthList[j]->m_UnderAvg);

							//对标的TickDataList的大小作限制
							size_t KeepLen = (MAX_UNDER_TICKDATA_SIZE >> 1);
							if(m_pTshapeMonthList[j]->m_UnderlyingTickDataList.size() > MAX_UNDER_TICKDATA_SIZE)
							{
								TruncTickData(m_pTshapeMonthList[j], KeepLen);
							}
						}
						memcpy(&m_pTshapeMonthList[j]->m_UnderlyingMarketData, &MarketDataList[i], sizeof(CThostFtdcDepthMarketDataField));
						m_pTshapeMonthList[j]->m_TickCountStamp = m_TickCountStamp;

						m_pTshapeMonthList[j]->writeunlock();
						found = true;
					}
					else if(RetVal == 0) //期权合约
					{
						for(size_t k = 0; k < m_pTshapeMonthList[j]->m_pOptionPairList.size(); k++)
						{
							if(FloatEqual(StrikePrice, m_pTshapeMonthList[j]->m_pOptionPairList[k]->m_StrikePrice, eps))
							{
								if(OptionTypeCode == 'C' || OptionTypeCode == 'c')
								{
									COptionItem* pCallItem;
									pCallItem = m_pTshapeMonthList[j]->m_pOptionPairList[k]->m_pCallItem;
									pCallItem->writelock();
									memcpy(&pCallItem->m_DepthMarketData, &MarketDataList[i], sizeof(CThostFtdcDepthMarketDataField));
									pCallItem->m_TickCountStamp = m_TickCountStamp;
									pCallItem->writeunlock();
									found = true;
									break; //k
								}
								else if(OptionTypeCode == 'P' || OptionTypeCode == 'p')
								{
									COptionItem* pPutItem;
									pPutItem = m_pTshapeMonthList[j]->m_pOptionPairList[k]->m_pPutItem;
									pPutItem->writelock();
									memcpy(&pPutItem->m_DepthMarketData, &MarketDataList[i], sizeof(CThostFtdcDepthMarketDataField));
									pPutItem->m_TickCountStamp = m_TickCountStamp;
									pPutItem->writeunlock();
									found = true;
									break; //k
								}
							} //if StrikePrice
						} //for k
					} //else if(RetVal == 0)
				} //if(YearMonth == m_pTshapeMonthList[j].YearMonth)

				if(found == true) 
				{
					break; //j
				}
			} //for(int j)
		} //if((RetVal == 1 || RetVal == 0) && (m_UnderlyingAssetType == 0 || m_UnderlyingAssetType == 1))
		else if((RetVal == 1 || RetVal == 0) && (m_UnderlyingAssetType == 2 || m_UnderlyingAssetType == 3)) //标的为ETF或者股票
		{
			if(RetVal == 1)     //标的
			{
				//更新所有月份的标的合约
				for(size_t j = 0; j < m_pTshapeMonthList.size(); j++)
				{
					m_pTshapeMonthList[j]->writelock();

					//只有在正常交易时段才更新标的行情列表和平均值
					if(bAllowedRefreshTime == true)
					{
						bool bFirstMd = (m_pTshapeMonthList[j]->m_UnderlyingTickDataList.size() == 0) ? true : false;
						m_pTshapeMonthList[j]->m_UnderlyingTickDataList.push_back(MarketDataList[i]);
						//更新标的的价格成交量加权平均值
						CalcUnderAvgPrice(bFirstMd, m_pTshapeMonthList[j]->m_UnderlyingTickDataList, m_pTshapeMonthList[j]->m_UnderAvg);

						//对标的TickDataList的大小作限制
						size_t KeepLen = (MAX_UNDER_TICKDATA_SIZE >> 1);
						if(m_pTshapeMonthList[j]->m_UnderlyingTickDataList.size() > MAX_UNDER_TICKDATA_SIZE)
						{
							TruncTickData(m_pTshapeMonthList[j], KeepLen);
						}
					}
					memcpy(&m_pTshapeMonthList[j]->m_UnderlyingMarketData, &MarketDataList[i], sizeof(CThostFtdcDepthMarketDataField));
					m_pTshapeMonthList[j]->m_TickCountStamp = m_TickCountStamp;

					m_pTshapeMonthList[j]->writeunlock();
				}
			}
			else if(RetVal == 0)
			{
			    COptionItem* pOptionItem = (COptionItem*)pNode;
				pOptionItem->writelock();
                memcpy(&pOptionItem->m_DepthMarketData, &MarketDataList[i], sizeof(CThostFtdcDepthMarketDataField));
				pOptionItem->m_TickCountStamp = m_TickCountStamp;
				pOptionItem->writeunlock();
			}

			found = true;
		}

		if(found == true) 
		{
			//设置该合约代码已被使用标志
			MarketDataList[i].InstrumentID[0] = '@';
		}
	} //for(int i)
}

//++
//根据OnRtnInstrumentStatus返回的合约交易状态列表，更新T型报价结构
//参数
//    InstrumentStatusList: 引用参数，合约交易状态列表
//                          当一个合约代码的交易状态数据已经使用完毕，将被标记为不可用
//返回值
//    无
//--
void CTshapeBase::UpdateTshapeInstrumentStatus(vector<CThostFtdcInstrumentStatusField>& InstrumentStatusList)
{
	//可以接受的浮点数表示精度误差
	double eps = (1e-6);
	void *pNode = NULL;

	for(size_t i = 0; i < InstrumentStatusList.size(); i++)
	{
		size_t YearMonth;
		double StrikePrice;
		char OptionTypeCode;
		int RetVal;

		bool found = false;
		string strExchangeID = InstrumentStatusList[i].ExchangeID;
		RemoveSpace(strExchangeID);

		if(InstrumentStatusList[i].InstrumentID[0] == '@')   //该合约已经被使用，不必再处理，以加快速度
		{
			RetVal = -1;
		}
		//合约交易所代码和T型报价交易所代码不一致，不必处理，以加快速度
		//注意: 只有上交所和深交所行情中有交易所代码，中金所/上期所/郑商所/大商所的行情中ExchangeID为空
		else if((strExchangeID != "") && (strcmp(m_ExchangeId, InstrumentStatusList[i].ExchangeID) != 0)) 
		{
			RetVal = -1;
		}
		else if(m_UnderlyingAssetType == 0 || m_UnderlyingAssetType == 1)  //标的为商品期货或者股指期货
		{
			RetVal = ParseInstrumentId(InstrumentStatusList[i].InstrumentID, YearMonth, StrikePrice, OptionTypeCode, pNode);
		}
		else if(m_UnderlyingAssetType == 2 || m_UnderlyingAssetType == 3)  //标的为ETF或者股票
		{
			RetVal = ParseInstrumentId(InstrumentStatusList[i].InstrumentID, YearMonth, StrikePrice, OptionTypeCode, pNode);
		}

		//更新相应合约的市场行情信息
		if((RetVal == 1 || RetVal == 0) && (m_UnderlyingAssetType == 0 || m_UnderlyingAssetType == 1))  //标的为期货
		{
			for(size_t j = 0; j < m_pTshapeMonthList.size(); j++)
			{
				if(YearMonth == m_pTshapeMonthList[j]->m_YearMonth)
				{
					if(RetVal == 1)  //标的合约
					{
						m_pTshapeMonthList[j]->writelock();
						m_pTshapeMonthList[j]->m_UnderlyingInstrumentStatus = InstrumentStatusList[i].InstrumentStatus;
						m_pTshapeMonthList[j]->writeunlock();
						found = true;
					}
					else if(RetVal == 0) //期权合约
					{
						for(size_t k = 0; k < m_pTshapeMonthList[j]->m_pOptionPairList.size(); k++)
						{
							if(FloatEqual(StrikePrice, m_pTshapeMonthList[j]->m_pOptionPairList[k]->m_StrikePrice, eps))
							{
								if(OptionTypeCode == 'C' || OptionTypeCode == 'c')
								{
									COptionItem* pCallItem;
									pCallItem = m_pTshapeMonthList[j]->m_pOptionPairList[k]->m_pCallItem;
									pCallItem->writelock();
									pCallItem->m_InstrumentStatus = InstrumentStatusList[i].InstrumentStatus;
									pCallItem->writeunlock();
									found = true;
									break; //k
								}
								else if(OptionTypeCode == 'P' || OptionTypeCode == 'p')
								{
									COptionItem* pPutItem;
									pPutItem = m_pTshapeMonthList[j]->m_pOptionPairList[k]->m_pPutItem;
									pPutItem->writelock();
									pPutItem->m_InstrumentStatus = InstrumentStatusList[i].InstrumentStatus;
									pPutItem->writeunlock();
									found = true;
									break; //k
								}
							} //if StrikePrice
						} //for k
					} //else if(RetVal == 0)
				} //if(YearMonth == m_pTshapeMonthList[j].YearMonth)

				if(found == true) 
				{
					break; //j
				}
			} //for(int j)
		} //if((RetVal == 1 || RetVal == 0) && (m_UnderlyingAssetType == 0 || m_UnderlyingAssetType == 1))
		else if((RetVal == 1 || RetVal == 0) && (m_UnderlyingAssetType == 2 || m_UnderlyingAssetType == 3)) //标的为ETF或者股票
		{
			if(RetVal == 1)     //标的
			{
				//更新所有月份的标的合约
				for(size_t j = 0; j < m_pTshapeMonthList.size(); j++)
				{
					m_pTshapeMonthList[j]->writelock();
					m_pTshapeMonthList[j]->m_UnderlyingInstrumentStatus = InstrumentStatusList[i].InstrumentStatus;
					m_pTshapeMonthList[j]->writeunlock();
				}
			}
			else if(RetVal == 0)
			{
			    COptionItem* pOptionItem = (COptionItem*)pNode;
				pOptionItem->writelock();
				pOptionItem->m_InstrumentStatus = InstrumentStatusList[i].InstrumentStatus;
				pOptionItem->writeunlock();
			}

			found = true;
		}

		if(found == true) 
		{
			//设置该合约代码已被使用标志
			InstrumentStatusList[i].InstrumentID[0] = '@';
		}
	} //for(int i)
}

//++
//插入TshapeMonthItem时，按照年月排序，并返回插入的位置
//参数
//    pTshapeMonthList: 按照年月排列的T型报价列表
//    pTshapeMonthItem: 单月的T型报价
//返回值
//    插入的单月T型报价在T型报价列表中的位置
//--
int CTshapeBase::InsertOrderByYearMonth(vector<CTshapeMonth*>& pTshapeMonthList, CTshapeMonth* pTshapeMonthItem)
{
	int TshapeMonthListSize;
	int InsertPos;
	vector<CTshapeMonth*>::iterator it;

    InsertPos = 0;
	TshapeMonthListSize = pTshapeMonthList.size();

	if(TshapeMonthListSize == 0) 
	{
		pTshapeMonthList.push_back(pTshapeMonthItem);
	    InsertPos = 0;
	}
	else if(pTshapeMonthItem->m_YearMonth < pTshapeMonthList[0]->m_YearMonth)
	{
		pTshapeMonthList.insert(pTshapeMonthList.begin(), pTshapeMonthItem);
		InsertPos = 0;
	}
	else if(pTshapeMonthItem->m_YearMonth >= pTshapeMonthList[TshapeMonthListSize-1]->m_YearMonth)
	{
		pTshapeMonthList.push_back(pTshapeMonthItem);
		InsertPos = TshapeMonthListSize;
	}
	else
	{
		for(it = pTshapeMonthList.begin(); it < pTshapeMonthList.end()-1; it++)
		{
			InsertPos = InsertPos + 1;
			if(pTshapeMonthItem->m_YearMonth >= (*it)->m_YearMonth && pTshapeMonthItem->m_YearMonth < (*(it+1))->m_YearMonth)
			{
				pTshapeMonthList.insert(it+1, pTshapeMonthItem);
				break;
			}
		}
	}

	return InsertPos;
}


//++
//插入COptionPairItem时，按照行权价格大小排序，并返回插入的位置
//参数
//    pOptionPairList: 按行权价大小排列的OptionPairItem列表
//    pOptionPairItem: 待插入的COptionPairItem
//返回值
//   插入的pOptionPairItem在列表中的位置
//--
int CTshapeBase::InsertOrderByStrikePrice(vector<COptionPairItem*>& pOptionPairList, COptionPairItem* pOptionPairItem)
{
	int OptionPairListSize;
	int InsertPos;
	vector<COptionPairItem*>::iterator it;

    InsertPos = 0;
	OptionPairListSize = pOptionPairList.size();

	if(OptionPairListSize == 0)
	{
		pOptionPairList.push_back(pOptionPairItem);
		InsertPos = 0;
	}
	else if(pOptionPairItem->m_StrikePrice < pOptionPairList[0]->m_StrikePrice)
	{
		pOptionPairList.insert(pOptionPairList.begin(), pOptionPairItem);
		InsertPos = 0;
	}
	else if(pOptionPairItem->m_StrikePrice >= pOptionPairList[OptionPairListSize-1]->m_StrikePrice)
	{
		pOptionPairList.push_back(pOptionPairItem);
		InsertPos = OptionPairListSize;
	}
	else
	{
		for(it = pOptionPairList.begin(); it < pOptionPairList.end()-1; it++)
		{
			InsertPos = InsertPos + 1;
			if(pOptionPairItem->m_StrikePrice >= (*it)->m_StrikePrice && pOptionPairItem->m_StrikePrice < (*(it+1))->m_StrikePrice)
			{
				pOptionPairList.insert(it+1, pOptionPairItem);
				break;
			}
		}
	}

	return InsertPos;
}

//++
//删除OptionPairList中不合法的COptionPairItem
//如果m_pCallItem或者m_pPutItem为NULL则该COptionPairItem非法，应该删除
//参数
//    pOptionPairList: 按行权价大小排列的OptionPairItem列表
//返回值
//    无
//--
void CTshapeBase::DeleteInvalidOptionPairItem(vector<COptionPairItem*>& pOptionPairList)
{
	vector<COptionPairItem*> tmpOptionPairList;

	for(size_t i = 0; i < pOptionPairList.size(); i++)
	{
		COptionPairItem* pOptionPairItem = pOptionPairList[i];
		if(pOptionPairItem->m_pCallItem != NULL && pOptionPairItem->m_pPutItem != NULL)
		{
		    tmpOptionPairList.push_back(pOptionPairItem);
		}
		else
		{
		    if(pOptionPairItem->m_pCallItem != NULL)
			{
				delete pOptionPairItem->m_pCallItem;
				pOptionPairItem->m_pCallItem = NULL;
			}

			if(pOptionPairItem->m_pPutItem != NULL)
			{
				delete pOptionPairItem->m_pPutItem;
				pOptionPairItem->m_pPutItem = NULL;
			}

			delete pOptionPairItem;
		}
	}

	pOptionPairList.swap(tmpOptionPairList);
}

//++
//用于计算标的期货一手合约对应的保证金（多空分开计算，没有考虑是否相对于交易所收取）.
//注意：没有考虑基础商品乘数，通常情况下基础商品乘数为1.0
//参数
//    nMonthPos: 在m_pTshapeMonthList中的位置
//返回值
//    无
//--
void CTshapeBase::CalcUnderlyingMargin(size_t nMonthPos)
{
	CThostFtdcDepthMarketDataField* pDepthMarketData;
	double UsedUnderlyingPrice;

	if(m_UnderlyingAssetType == 0 || m_UnderlyingAssetType == 1)  //标的为商品期货或者股指期货
	{
		pDepthMarketData = &m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData;
		if(m_BrokerTradingParams[0].MarginPriceType == THOST_FTDC_MPT_PreSettlementPrice) //使用昨结算价
			UsedUnderlyingPrice = pDepthMarketData->PreSettlementPrice;
		else //其它情况都使用最新价
			UsedUnderlyingPrice = pDepthMarketData->LastPrice;

		double VolumeMultiple = (double)m_pTshapeMonthList[nMonthPos]->m_UnderlyingInstInfo.VolumeMultiple;

		m_pTshapeMonthList[nMonthPos]->m_LongUnderlyingMargin = UsedUnderlyingPrice * VolumeMultiple * m_pTshapeMonthList[nMonthPos]->m_UnderlyingInstMarginRate.LongMarginRatioByMoney 
															 + m_pTshapeMonthList[nMonthPos]->m_UnderlyingInstMarginRate.LongMarginRatioByVolume;
		m_pTshapeMonthList[nMonthPos]->m_ShortUnderlyingMargin = UsedUnderlyingPrice * VolumeMultiple * m_pTshapeMonthList[nMonthPos]->m_UnderlyingInstMarginRate.ShortMarginRatioByMoney 
															 + m_pTshapeMonthList[nMonthPos]->m_UnderlyingInstMarginRate.ShortMarginRatioByVolume;
	}
	else if (m_UnderlyingAssetType == 2 || m_UnderlyingAssetType == 3) //标的为ETF或者股票, 保证金为全额
	{
		pDepthMarketData = &m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData;
		UsedUnderlyingPrice = pDepthMarketData->LastPrice;

		double VolumeMultiple = (double)m_pTshapeMonthList[nMonthPos]->m_UnderlyingInstInfo.VolumeMultiple;

		m_pTshapeMonthList[nMonthPos]->m_LongUnderlyingMargin = UsedUnderlyingPrice * VolumeMultiple;
		m_pTshapeMonthList[nMonthPos]->m_ShortUnderlyingMargin = UsedUnderlyingPrice * VolumeMultiple;
	}
}

//++
//计算期权合约对应的希腊值和隐含波动率
//参数
//    pOptionItem: 指向期权项的指针
//    nMonthPos: 该月期权在m_pTshapeMonthList中的位置
//返回值
//    无
//--
void CTshapeBase::CalcGreeksAndIv(COptionItem* pOptionItem, size_t nMonthPos)
{
	bool bCallType;
	double optionPrice;
	COptionPriceModel optionPriceModel(m_UnderlyingAssetType);  
	double eps = 0.000001;

	if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_CallOptions)
		bCallType = true;
	else
		bCallType = false;

	//计算希腊值和隐含波动率使用AskPrice1和BidPrice1的中间值, 不使用最新价
	double MidPrice = (pOptionItem->m_DepthMarketData.AskPrice1 + pOptionItem->m_DepthMarketData.BidPrice1)/2.0;
	optionPrice = MidPrice;
	double MidUnderPrice = (m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.AskPrice1 
		                    + m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.BidPrice1)/2.0;
	optionPriceModel.mS = MidUnderPrice;
	optionPriceModel.mK = pOptionItem->m_InstInfo.StrikePrice;
	optionPriceModel.mR = m_MeanRate;
	optionPriceModel.mTday = (double)m_pTshapeMonthList[nMonthPos]->m_ExpirationLeftDays;

	//计算隐含波动率并得到各种希腊值
	if(optionPriceModel.ImpliedVolatility(bCallType, optionPrice) == true)
	{
		pOptionItem->m_Delta = (bCallType == true) ? optionPriceModel.mDeltaCall : optionPriceModel.mDeltaPut;
		pOptionItem->m_Gamma = (bCallType == true) ? optionPriceModel.mGammaCall : optionPriceModel.mGammaPut;
		pOptionItem->m_Vega  = (bCallType == true) ? optionPriceModel.mVegaCall : optionPriceModel.mVegaPut;
		pOptionItem->m_Theta = (bCallType == true) ? optionPriceModel.mThetaCall : optionPriceModel.mThetaPut;
		pOptionItem->m_Rho   = (bCallType == true) ? optionPriceModel.mRhoCall : optionPriceModel.mRhoPut;
		pOptionItem->m_ImpliedVolatility    = optionPriceModel.mV;  

		//求隐含波动率的平均值
		double lambda = IIR_LAMBDA;

		//必须对计算得到的隐含波动率有所限制, 防止异常值造成影响
		if(pOptionItem->m_ImpliedVolatility < 0)
		{
			pOptionItem->m_ImpliedVolatility = 0;
		}
		else if(pOptionItem->m_ImpliedVolatility > MAX_IMPLIED_VLTY)
		{
			pOptionItem->m_ImpliedVolatility = MAX_IMPLIED_VLTY;
		}

		if(pOptionItem->m_MeanIV < eps)
		{
			pOptionItem->m_MeanIV = pOptionItem->m_ImpliedVolatility;
		}
		else
		{
			pOptionItem->m_MeanIV = lambda * pOptionItem->m_MeanIV + (1 - lambda) * pOptionItem->m_ImpliedVolatility;
		}

		//计算波动率偏差
		double tmpIVDev = abs(pOptionItem->m_ImpliedVolatility - pOptionItem->m_MeanIV);
		pOptionItem->m_MeanIVDev = lambda * pOptionItem->m_MeanIVDev + (1 - lambda) * tmpIVDev;
	}
	else
	{
		; //cerr << "计算 " << pOptionItem->m_InstInfo.InstrumentID << " 隐含波动率和希腊值时出错" << ENDLINE;
	}
}

//++
//根据标的实际波动率计算期权合约的理论价格
//参数
//    pOptionPairItem: 指向期权合约对的指针
//    nMonthPos: 该月期权在m_pTshapeMonthList中的位置
//返回值
//    无
//--
void CTshapeBase::CalcTheoreticalRoyalty(COptionPairItem* pOptionPairItem, size_t nMonthPos)
{
	COptionPriceModel optionPriceModel(m_UnderlyingAssetType);  //初始化为商品期货期权模型

	optionPriceModel.mS = m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.LastPrice;
	optionPriceModel.mK = pOptionPairItem->m_StrikePrice;
	optionPriceModel.mR = m_MeanRate;
	optionPriceModel.mTday = (double)m_pTshapeMonthList[nMonthPos]->m_ExpirationLeftDays;
	optionPriceModel.mV = m_pTshapeMonthList[nMonthPos]->m_InterDayHv;

	optionPriceModel.PriceModelMain();
	pOptionPairItem->m_pCallItem->m_TheoreticalPrice = optionPriceModel.mCallPrice;
	pOptionPairItem->m_pPutItem->m_TheoreticalPrice = optionPriceModel.mPutPrice;
}

//++
//根据平价公式计算OptionPair的平均无风险利率
//参数
//    pOptionItem: 指向期权项的指针
//    nMonthPos: 该月期权在m_pTshapeMonthList中的位置
//返回值
//    无
//--
void CTshapeBase::CalcOptionPairMeanRate(COptionPairItem* pOptionPairItem, size_t nMonthPos)
{
	double eps = 0.0000001;
	double MidUnderPrice = (m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.AskPrice1 
		                    + m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.BidPrice1) * 0.5;
	double MidCallPrice = (pOptionPairItem->m_pCallItem->m_DepthMarketData.AskPrice1
		                    + pOptionPairItem->m_pCallItem->m_DepthMarketData.BidPrice1) * 0.5;
	double MidPutPrice = (pOptionPairItem->m_pPutItem->m_DepthMarketData.AskPrice1
		                    + pOptionPairItem->m_pPutItem->m_DepthMarketData.BidPrice1) * 0.5;
	
	//确定市场行情是否正确
	bool bIsUnderMdOk = IsDepthMarketDataOk(&m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData);
	bool bIsCallMdOk = IsDepthMarketDataOk(&pOptionPairItem->m_pCallItem->m_DepthMarketData);
	bool bIsPutMdOk = IsDepthMarketDataOk(&pOptionPairItem->m_pPutItem->m_DepthMarketData);
	bool bIsMdOk = (bIsUnderMdOk == true && bIsCallMdOk == true && bIsPutMdOk == true);

	//将到期日折算成以年为单位的时间
	double t = double(m_pTshapeMonthList[nMonthPos]->m_ExpirationLeftDays) / 365.0;

	//根据平价套利公式计算年化利率
	//股票期权的平价理论等式： C + K*exp(-rt) = P + S
	//期货期权平价理论公式: (C - P) = exp(-rt)*(F - K）
	//根据测算结果, 期货期权的平价公式不适用, 使用自己定义的(C - P + K) * exp(-rt) = F
	double tmpRate = 0;
	double x = (MidPutPrice + MidUnderPrice - MidCallPrice);
	double y = (MidCallPrice - MidPutPrice + pOptionPairItem->m_StrikePrice);
	if(x > eps && pOptionPairItem->m_StrikePrice > eps && t > eps && bIsMdOk == true
		&& (m_UnderlyingAssetType == 1 || m_UnderlyingAssetType == 2 || m_UnderlyingAssetType == 3))    //股指期货, ETF或者股票
	{
        tmpRate = - log((MidPutPrice + MidUnderPrice - MidCallPrice) / pOptionPairItem->m_StrikePrice) / t;
	}
	else if(y > eps && MidUnderPrice > eps && t > eps && bIsMdOk == true && m_UnderlyingAssetType == 0)  //商品期货
	{
		tmpRate = log(y / MidUnderPrice) / t;
	}

	//转化为百分比数目, 并对其值进行限制
	tmpRate = tmpRate * 100;
	if(tmpRate < MIN_PAR_RATE)
	{
		tmpRate = MIN_PAR_RATE;
	}
	else if(tmpRate > MAX_PAR_RATE)
	{
		tmpRate = MAX_PAR_RATE;
	}

	//按照时间平均
	double lambda = IIR_LAMBDA;
	pOptionPairItem->writelock();
	if(abs(pOptionPairItem->m_MeanRate) < eps)
	{
		pOptionPairItem->m_MeanRate = tmpRate;
	}
	else
	{
		pOptionPairItem->m_MeanRate = lambda * pOptionPairItem->m_MeanRate + (1 - lambda) * tmpRate;
	}
	pOptionPairItem->writeunlock();
}

//++
//初始化期权对根据平价公式计算的平均无风险利率
//参数
//    DefaultRate: 数据库中没有数据时的缺省收益率
//返回值
//    无
//--
void CTshapeBase::InitOptionPairMeanRate(double DefaultRate)
{
	//按月份循环
	for(size_t i = 0; i < m_pTshapeMonthList.size(); i++)
	{
		CTshapeMonth *pTshapeMonth = m_pTshapeMonthList[i];
		vector<CDayKlineItem>* pDayKlineList = &pTshapeMonth->m_DayKlineList;
		size_t KlineSize = pDayKlineList->size();
		if(KlineSize > 0)
		{
			DefaultRate = (*pDayKlineList)[0].MeanRate;
			if(DefaultRate < MIN_PAR_RATE)
			{
				DefaultRate = MIN_PAR_RATE;
			}
			else if(DefaultRate > MAX_PAR_RATE)
			{
				DefaultRate = MAX_PAR_RATE;
			}
		}

		//按行权价进行初始化
		for(size_t j = 0; j < pTshapeMonth->m_pOptionPairList.size(); j++)
		{
			COptionPairItem *pOptionPair = pTshapeMonth->m_pOptionPairList[j];
			pOptionPair->m_MeanRate = DefaultRate;
		} // for j
	} //for i
}

//++
//获取整个T型报价平均无风险利率
//参数
//    无
//返回值
//    无
//--
void CTshapeBase::GetTshapeMeanRate()
{
	double eps = 0.00000001;
	double SumTshapeVolume = 0;
	double SumTshapeVolWgtRate = 0;

	//j按月份循环
	for(size_t j = 0; j < m_pTshapeMonthList.size(); j++)
	{
		CTshapeMonth* pTshapeMonth = m_pTshapeMonthList[j];
		double SumMonthVolume = 0;
		double SumMonthVolDistWgt = 0;
		double SumMonthWeightedRate = 0;
		double a = DST_UPPER_RATIO;

		//k按行权价循环
		for(size_t k = 0; k < pTshapeMonth->m_pOptionPairList.size(); k++)
		{
			COptionPairItem* pOptionPairItem = pTshapeMonth->m_pOptionPairList[k];
			COptionItem* pCallItem = pTshapeMonth->m_pOptionPairList[k]->m_pCallItem;
			COptionItem* pPutItem = pTshapeMonth->m_pOptionPairList[k]->m_pPutItem;
			CThostFtdcDepthMarketDataField* pUnderMd = &pTshapeMonth->m_UnderlyingMarketData;

			//确定行权价离标的价格的距离及距离加权系数
			double UnderMidPrice = (pUnderMd->AskPrice1 + pUnderMd->BidPrice1) * 0.5;
			double UnderLimitPriceGap = pUnderMd->UpperLimitPrice - pUnderMd->LowerLimitPrice;
			double UnderLimitRatio = UnderLimitPriceGap / UnderMidPrice;
			a = (UnderLimitRatio > eps && UnderLimitRatio < DST_UPPER_RATIO) ? UnderLimitRatio : DST_UPPER_RATIO;
			double x = (UnderMidPrice > eps) ? abs(pOptionPairItem->m_StrikePrice - UnderMidPrice) / UnderMidPrice : 0;
			double DistanceWeight = (x < a) ? (x - a) * (x - a) / (a * a) : 0;

			//计算每个期权对根据平价套利公式所隐含的年化利率
			CalcOptionPairMeanRate(pOptionPairItem, j);

			double VolDistWgt = (pCallItem->m_DepthMarketData.Volume + pPutItem->m_DepthMarketData.Volume) * DistanceWeight;
			double VolDistWgtRate = VolDistWgt * pOptionPairItem->m_MeanRate;
			SumMonthVolDistWgt += VolDistWgt;
			SumMonthWeightedRate += VolDistWgtRate;
			SumMonthVolume += pCallItem->m_DepthMarketData.Volume + pPutItem->m_DepthMarketData.Volume;
		}//for k, T型报价行权价

		//计算当月的平均隐含年化利率
		pTshapeMonth->writelock();
		pTshapeMonth->m_MeanRate = (SumMonthVolDistWgt > eps) ? (SumMonthWeightedRate / SumMonthVolDistWgt) : 0;
        pTshapeMonth->writeunlock();

		//只有当月合约剩余天数满足要求时才纳入整个T型报价平均隐含年化利率计算
		int ExpireLeftDays = pTshapeMonth->m_ExpirationLeftDays;
		if(ExpireLeftDays >= MIN_CAL_LEFT_DAYS)
		{
			SumTshapeVolume += SumMonthVolume;
			SumTshapeVolWgtRate += SumMonthVolume * pTshapeMonth->m_MeanRate;
		}
	} //for j, T型报价月份

	//计算整个T型报价按照成交量加权平均得到的隐含年化利率
	double lambda = IIR_LAMBDA;
	double tmpMeanRate = (SumTshapeVolume > eps) ? (SumTshapeVolWgtRate / SumTshapeVolume) : 0;

	//对整个T型报价隐含年化利率的限制, 目前只适用于股指期货, ETF或者股票
	if(tmpMeanRate >= 0 && tmpMeanRate <= MAX_PAR_RATE)
	{
		tmpMeanRate = tmpMeanRate;
	}
	else if(tmpMeanRate < 0)
	{
		tmpMeanRate = 0;
	}
	else if(tmpMeanRate > MAX_PAR_RATE)
	{
		tmpMeanRate = MAX_PAR_RATE;
	}
	else //非法值, 比如0/0的结果
	{
		cerr << "根据平价套利公式计算平均无风险年化收益率 得到非法结果!" << ENDLINE;
		tmpMeanRate = 0;
	}

	writelock();
	if(m_MeanRate < eps)
	{
		m_MeanRate = tmpMeanRate;
	}
	else
	{
		m_MeanRate = lambda * m_MeanRate + (1 - lambda) * tmpMeanRate;
	}
	writeunlock();
}

//++
//更新整个T型报价的数学运算结果包括希腊值，隐含波动率以及各种价值
//将CalcUnderlyingMargin(), CalcOptionMiscValues(), CalcGreeksAndIv(), CalcTheoreticalRoyalty()各个函数的功能
//计算平均隐含波动率的方法参考麦克米伦 <<期权投资策略>>, P328
//按顺序组合在一起
//参数
//    bArbOnly: true, 仅仅是套利，则不需要计算希腊值和理论价格; false: 其它策略
//--
void CTshapeBase::MathematicalRefreshment(bool bArbOnly)
{
	double eps = 0.00000001;
	double SumTshapeVolume = 0;
	double SumTshapeVolWgtIV = 0;

	//集合竞价时段不能计算隐含波动率和希腊值等, 否则可能给出不正常的值
	//但开盘后应该立即计算, 否则各个合约的保证金计算将不正常, 所以给出一个提前时间进行计算
	double StartAdjSecs = -5.0; 
	bool bAllowedRefreshTime = IsAllowedRefreshTime(StartAdjSecs);
	if(bAllowedRefreshTime == false) 
	{
		return;
	}

	//根据平价套利公式计算年化收益率
	GetTshapeMeanRate();

	//j按月份循环
	for(size_t j = 0; j < m_pTshapeMonthList.size(); j++)
	{
		CTshapeMonth* pTshapeMonth = m_pTshapeMonthList[j];
		double SumMonthVolume = 0;
		double SumMonthVolDistWgt = 0;
		double SumMonthWeightedIV = 0;
		double CallSumMonthVolDistWgt = 0;
		double CallSumMonthWeightedIV = 0;
		double PutSumMonthVolDistWgt = 0;
		double PutSumMonthWeightedIV = 0;
		double a = DST_UPPER_RATIO;

		//计算标的的保证金
		pTshapeMonth->writelock();
		CalcUnderlyingMargin(j);
		pTshapeMonth->writeunlock();

		//k按行权价循环
		for(size_t k = 0; k < pTshapeMonth->m_pOptionPairList.size(); k++)
		{
			COptionPairItem* pOptionPairItem = pTshapeMonth->m_pOptionPairList[k];
			COptionItem* pCallItem = pTshapeMonth->m_pOptionPairList[k]->m_pCallItem;
			COptionItem* pPutItem = pTshapeMonth->m_pOptionPairList[k]->m_pPutItem;
			CThostFtdcDepthMarketDataField* pUnderMd = &pTshapeMonth->m_UnderlyingMarketData;

			//确定行权价离标的价格的距离及距离加权系数
			double UnderMidPrice = (pUnderMd->AskPrice1 + pUnderMd->BidPrice1) * 0.5;
			double UnderLimitPriceGap = pUnderMd->UpperLimitPrice - pUnderMd->LowerLimitPrice;
			double UnderLimitRatio = UnderLimitPriceGap / UnderMidPrice;
			a = (UnderLimitRatio > eps && UnderLimitRatio < DST_UPPER_RATIO) ? UnderLimitRatio : DST_UPPER_RATIO;
			double x = (UnderMidPrice > eps) ? abs(pOptionPairItem->m_StrikePrice - UnderMidPrice) / UnderMidPrice : 0;
			double DistanceWeight = (x < a) ? (x - a) * (x - a) / (a * a) : 0;

			//看涨期权相关计算
			pCallItem->writelock();
			CalcOptionMiscValues(pCallItem, j);
			if(bArbOnly == false)
			{
				CalcGreeksAndIv(pCallItem, j);

				double VolDistWgt = pCallItem->m_DepthMarketData.Volume * DistanceWeight;
				double VolDistWgtIV = VolDistWgt * pCallItem->m_MeanIV;
				CallSumMonthVolDistWgt += VolDistWgt;
				CallSumMonthWeightedIV += VolDistWgtIV;
				SumMonthVolDistWgt += VolDistWgt;
				SumMonthWeightedIV += VolDistWgtIV;
				SumMonthVolume += pCallItem->m_DepthMarketData.Volume;
			}
			pCallItem->writeunlock();

			//看跌期权相关计算
			pPutItem->writelock();
			CalcOptionMiscValues(pPutItem, j);
			if(bArbOnly == false)
			{
				CalcGreeksAndIv(pPutItem, j);

				double VolDistWgt = pPutItem->m_DepthMarketData.Volume * DistanceWeight;
				double VolDistWgtIV = VolDistWgt * pPutItem->m_MeanIV;
				PutSumMonthVolDistWgt += VolDistWgt;
				PutSumMonthWeightedIV += VolDistWgtIV;
				SumMonthVolDistWgt += VolDistWgt;
				SumMonthWeightedIV += VolDistWgtIV;
				SumMonthVolume += pPutItem->m_DepthMarketData.Volume;
			}
			pPutItem->writeunlock();

			//期权对相关计算
            CalcTheoreticalRoyalty(pOptionPairItem, j);
		}//for k, T型报价行权价

		//计算当月的平均隐含波动率
		if(bArbOnly == false)
		{
			pTshapeMonth->writelock();
			pTshapeMonth->m_MeanCallIV = (CallSumMonthVolDistWgt > eps) ? (CallSumMonthWeightedIV / CallSumMonthVolDistWgt) : 0;
			pTshapeMonth->m_MeanPutIV = (PutSumMonthVolDistWgt > eps) ? (PutSumMonthWeightedIV / PutSumMonthVolDistWgt) : 0;
			pTshapeMonth->m_MeanIV = (SumMonthVolDistWgt > eps) ? (SumMonthWeightedIV / SumMonthVolDistWgt) : 0;
            pTshapeMonth->writeunlock();

			//只有当月合约剩余天数满足要求时才纳入整个T型报价平均隐含波动率计算
			int ExpireLeftDays = pTshapeMonth->m_ExpirationLeftDays;
			if(ExpireLeftDays >= MIN_CAL_LEFT_DAYS)
			{
				SumTshapeVolume += SumMonthVolume;
				SumTshapeVolWgtIV += SumMonthVolume * pTshapeMonth->m_MeanIV;
			}
		}
	} //for j, T型报价月份

	//计算整个T型报价按照成交量加权平均得到的隐含波动率
	if(bArbOnly == false)
	{
		double lambda = IIR_LAMBDA;
		double tmpMeanIV = (SumTshapeVolume > eps) ? (SumTshapeVolWgtIV / SumTshapeVolume) : 0;

		//必须对计算得到的隐含波动率有所限制, 防止异常值造成影响
		if(tmpMeanIV >= 0 && tmpMeanIV <= MAX_IMPLIED_VLTY)
		{
			tmpMeanIV = tmpMeanIV;
		}
		else if(tmpMeanIV < 0)
		{
			tmpMeanIV = 0;
		}
		else if(tmpMeanIV > MAX_IMPLIED_VLTY)
		{
			tmpMeanIV = MAX_IMPLIED_VLTY;
		}
		else //非法值, 比如0/0的结果
		{
			cerr << "标的平均隐含波动率计算, 得到非法结果!" << ENDLINE;
			tmpMeanIV = 0;
		}

		writelock();
		if(m_MeanIV < eps)
		{
			m_MeanIV = tmpMeanIV;
		}
		else
		{
			m_MeanIV = lambda * m_MeanIV + (1 - lambda) * tmpMeanIV;
		}
		writeunlock();
	}
}

//++
//是否在允许对隐含波动率及各种数学值进行更新的时间段内
//参数
//    StartAdjSecs: 调整的秒数, 一般为负值, 防止开始交易后还没有计算保证金等值
//返回值
//    true: 处于可更新时段
//    false: 处于不可更新时段
//--
bool CTshapeBase::IsAllowedRefreshTime(double StartAdjSecs)
{
	double CurTimeSecs;
	SYSTEMTIME CurTime;

	GetLocalTime(&CurTime);
	CurTimeSecs = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond;

	bool bAllowedTime = false;
	for(size_t i = 0; i < m_pOptionKind->SegTimeList.size(); i++)
	{
		//计算交易开始时间以秒计算的数值
		int hours = int(m_pOptionKind->SegTimeList[i].SegBeginTime/1e4);
		int minutes = int((m_pOptionKind->SegTimeList[i].SegBeginTime/1e2) - hours*1e2);
		int seconds = int((m_pOptionKind->SegTimeList[i].SegBeginTime) - hours*1e4 - minutes*1e2);
		double SegBeginSecs = hours*3600 + minutes*60 + seconds;

		//计算交易时段结束时间以秒计算的数值
		hours = int(m_pOptionKind->SegTimeList[i].SegEndTime/1e4);
		minutes = int((m_pOptionKind->SegTimeList[i].SegEndTime/1e2) - hours*1e2);
		seconds = int((m_pOptionKind->SegTimeList[i].SegEndTime) - hours*1e4 - minutes*1e2);
		double SegEndSecs = hours*3600 + minutes*60 + seconds;

		if(CurTimeSecs > (SegBeginSecs + (m_pRealApp->m_TradeStartWindSec + StartAdjSecs)) 
			&& CurTimeSecs < (SegEndSecs - m_pRealApp->m_TradeStopWindSec))
		{
			bAllowedTime = true;
			break;
		}
	} //for i

	return bAllowedTime;
}

//++
//计算各种策略交易所使用的年化收益率, 以百分比表示
//参数
//    pTshapeMonth: 指向月份T型报价的指针
//    DefaultRate: 缺省年化收益率, 一般为5%
//    MinRate: 最小年化收益率, 一般为2%
//返回值
//    平均计算得到的年化收益率
//--
double CTshapeBase::MeanRateForTrade(CTshapeMonth* pTshapeMonth, double DefaultRate, double MinRate)
{
	int ExpireLefDays = pTshapeMonth->m_ExpirationLeftDays;

	//获取根据T型报价按时平均得到的年化收益率
	double TshapeMeanRate = 0;
	if(m_UnderlyingAssetType == 2 || m_UnderlyingAssetType == 3)  //ETF或者股票
	{
		readlock();
		TshapeMeanRate = m_MeanRate;
		readunlock();
	}
	else
	{
		pTshapeMonth->readlock();
		TshapeMeanRate = pTshapeMonth->m_MeanRate;
		pTshapeMonth->readunlock();
	}

	//根据K线数据获取前3日的平均年化收益率
	double DayMeanRate = 0;
	vector<CDayKlineItem>* pDayKlineList = &pTshapeMonth->m_DayKlineList;
	//注意: K线数据包括当日已经存入数据库的统计结果
	size_t KlineSize = pDayKlineList->size();
	if(KlineSize >= 3)
	{
		DayMeanRate = ((*pDayKlineList)[0].MeanRate + (*pDayKlineList)[1].MeanRate + (*pDayKlineList)[2].MeanRate) / 3.0;
	}
	else if(KlineSize == 1)
	{
		DayMeanRate = ((*pDayKlineList)[0].MeanRate + 2 * DefaultRate) / 3.0;
	}
	else if(KlineSize == 2)
	{
		DayMeanRate = ((*pDayKlineList)[0].MeanRate + (*pDayKlineList)[1].MeanRate + DefaultRate) / 3.0;
	}
	else
	{
		DayMeanRate = DefaultRate;
	}

	//对使用的MinRate按到期时间的长短乘以权重
	double RealMinRate = MinRate;
	if(ExpireLefDays > 60)  //当月和次月流动性较好, 不用调整
	{
		double Weight = 1.0 + double(ExpireLefDays) * 0.002; // 0.002系数可调, 大致为0.1/60
		RealMinRate = Weight * MinRate;
		if(RealMinRate > DefaultRate)
		{
			RealMinRate = DefaultRate;
		}
	}

	//计算交易所用的年化收益率
	double TradeRate = DayMeanRate * 0.75 + TshapeMeanRate * 0.25;
	if(TradeRate < RealMinRate)
	{
		TradeRate = RealMinRate;
	}
	else if(TradeRate > DefaultRate)
	{
		TradeRate = DefaultRate;
	}

	return TradeRate;
}

//++
//计算各种日间策略交易所使用的年化波动率, 以百分比表示
//参数
//    pTshapeMonth: 指向月份T型报价的指针
//    DefaultIV: 缺省年化波动率, 一般为25%
//    MinIV: 最小年化波动率, 一般为20%
//    MaxIV: 最大年化波动率, 一般为50%
//返回值
//    平均计算得到的年化波动率
//--
double CTshapeBase::MeanIVForTrade(CTshapeMonth* pTshapeMonth, double DefaultIV, double MinIV, double MaxIV)
{
	//获取根据T型报价按时平均得到的年化波动率
	double TshapeMeanIV = 0;
	if(m_UnderlyingAssetType == 2 || m_UnderlyingAssetType == 3)  //ETF或者股票
	{
		readlock();
		TshapeMeanIV = m_MeanIV;
		readunlock();
	}
	else
	{
		pTshapeMonth->readlock();
		TshapeMeanIV = pTshapeMonth->m_MeanIV;
		pTshapeMonth->readunlock();
	}

	//根据K线数据获取前3日的平均年化波动率
	double DayMeanIV = 0;
	vector<CDayKlineItem>* pDayKlineList = &pTshapeMonth->m_DayKlineList;
	//注意: K线数据包括当日已经存入数据库的统计结果
	size_t KlineSize = pDayKlineList->size();
	if(KlineSize >= 3)
	{
		DayMeanIV = ((*pDayKlineList)[0].MeanIV + (*pDayKlineList)[1].MeanIV + (*pDayKlineList)[2].MeanIV) / 3.0;
	}
	else if(KlineSize == 1)
	{
		DayMeanIV = ((*pDayKlineList)[0].MeanIV + 2 * DefaultIV) / 3.0;
	}
	else if(KlineSize == 2)
	{
		DayMeanIV = ((*pDayKlineList)[0].MeanIV + (*pDayKlineList)[1].MeanIV + DefaultIV) / 3.0;
	}
	else
	{
		DayMeanIV = DefaultIV;
	}

	//计算交易所用的年化收益率
	double TradeIV = DayMeanIV * 0.75 + TshapeMeanIV * 0.25;
	if(TradeIV < MinIV)
	{
		TradeIV = MinIV;
	}
	else if(TradeIV > MaxIV)
	{
		TradeIV = MaxIV;
	}

	return TradeIV;
}

