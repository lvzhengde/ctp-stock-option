#include "TshapeSzse.h"
#include "OptionPriceModel.h"


//++
//构造函数
//参数
//    OptionId: 对于股票期权其OptionId和m_UnderlyingId都是标的交易所代码
//    UnderlyingAssetType: 标的类型，ETF或者股票，缺省是ETF
//--
CTshapeSzse::CTshapeSzse(string OptionId, size_t UnderlyingAssetType)
{
	//交易所代码必须是深圳证券交易所
	TThostFtdcExchangeIDType ExchangeId = "SZSE"; 
	m_OptionId = OptionId;
	m_UnderlyingId = OptionId;
    m_UnderlyingAssetType = UnderlyingAssetType;
	strcpy(m_ExchangeId, ExchangeId);

	m_MarginRatio1 = 0.12;
	m_MarginRatio2 = 0.07;

	m_bUnderMdOnly = false;
	m_TickCountStamp = 0;
}

//++
//析构函数
//--
CTshapeSzse::~CTshapeSzse()
{
}

//++
//根据ReqQryInstrument返回的CThostFtdcInstrumentField列表，构造T型报价结构，并且计算出对应月份的期权离到期日还剩多少天
//CThostFtdcInstrumentField中的日期都是YYYYMMDD形式
//注意：
//    在构造T型报价的过程中，各个结构的地址因为插入排序的原因会经常变动，所以给父指针赋值的操作应该在T型报价构造完毕之后进行
//    对于股票期权而言，标的没有到期日
//参数
//    InstInfoList: 合约信息列表
//返回值
//    无
//--
void CTshapeSzse::ConstructTshape(vector<CThostFtdcInstrumentField> InstInfoList)
{
#ifndef FUTURE_OPTION_ONLY
	double eps = (1e-6);
	CThostFtdcInstrumentField UnderlyingInstrumentField;
	memset(&UnderlyingInstrumentField, 0, sizeof(CThostFtdcInstrumentField));

	for(size_t i = 0; i < InstInfoList.size(); i++)
	{
		size_t YearMonth;
		double StrikePrice;
		char OptionTypeCode;
		int RetVal;
		bool found;
		bool bCreateMonthItem;

		found = false;
		bCreateMonthItem = false;

		//确保是上海证券交易所的股票期权合约
		YearMonth = 0;
		StrikePrice = 0;
		OptionTypeCode = 0;
		if(strcmp(m_ExchangeId, InstInfoList[i].ExchangeID) != 0)
			RetVal = -1;
		else
			RetVal = ParseInstrumentField(InstInfoList[i], YearMonth, StrikePrice, OptionTypeCode);

		//查找是否在可以交易的年月份列表中
		for(size_t j = 0; j < m_YearMonthList.size(); j++)
		{
			if(YearMonth == m_YearMonthList[j])
			{
				found = true;
				break;
			}
		}

		//年月份列表为空的情况下，创建所有的T型报价
		if( m_YearMonthList.size() == 0 || found == true) 
			bCreateMonthItem = true;

		//设置对应的期权合约
		if(RetVal == 0 && bCreateMonthItem == true && m_bUnderMdOnly == false)
		{
			int MonthItemPos = 0;

			for(size_t j = 0; j < m_pTshapeMonthList.size(); j++)
			{
				//该月份TshapeMonthItem已经存在
				if(YearMonth == m_pTshapeMonthList[j]->m_YearMonth)
				{
					MonthItemPos = j;
					bCreateMonthItem = false;
					break;
				}
			}

			//先生成TshapeMonthItem
			if(bCreateMonthItem == true)
			{
				CTshapeMonth* pTshapeMonthItem = new CTshapeMonth;

				pTshapeMonthItem->m_YearMonth = YearMonth;
				pTshapeMonthItem->m_TickCountStamp = 0;
				pTshapeMonthItem->m_UnderCancelTimes = 0;
				strcpy(pTshapeMonthItem->m_UnderlyingInstId, m_UnderlyingId.c_str());

				MonthItemPos = InsertOrderByYearMonth(m_pTshapeMonthList, pTshapeMonthItem);
			}

			//年月份相符
			if(YearMonth == m_pTshapeMonthList[MonthItemPos]->m_YearMonth)
			{
				bool bCreateOptionPairItem;
				int OptionPairListSize;
				int OptionPairItemPos = 0;

		        bCreateOptionPairItem = true;
				OptionPairListSize = m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList.size();
				for (int j = 0; j < OptionPairListSize; j++)
				{
					//查找该OptionPairItem是否存在, 比较行权价格和合约编码的调整位
					if(FloatEqual(StrikePrice, m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList[j]->m_StrikePrice, eps)
						&& InstInfoList[i].InstrumentCode[18] == m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList[j]->m_AdjustFlag)
					{
						bCreateOptionPairItem = false;
						OptionPairItemPos = j;
						break;
					}
				}

				if(bCreateOptionPairItem == true)
				{
					COptionPairItem* pOptionPairItem = new COptionPairItem;

					pOptionPairItem->m_StrikePrice = StrikePrice;
					pOptionPairItem->m_AdjustFlag = InstInfoList[i].InstrumentCode[18];
					if(pOptionPairItem->m_AdjustFlag == ' ') //未分红时为空格, 标志需要与上交所保持一致
					{
						pOptionPairItem->m_AdjustFlag = 'M';
					}
					OptionPairItemPos = InsertOrderByStrikePrice(m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList, pOptionPairItem);
					OptionPairListSize = OptionPairListSize + 1;
				}

				//更新相应的OptionItem
				if(OptionTypeCode == 'C' || OptionTypeCode == 'c')
				{
					COptionItem* pOldCallItem = m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList[OptionPairItemPos]->m_pCallItem;
					int CreateOrReplace = CreateOrReplaceOptionItem(&InstInfoList[i], pOldCallItem);

					if(CreateOrReplace >= 0)
					{
						COptionItem* pNewCallItem = new COptionItem;
						memcpy(&pNewCallItem->m_InstInfo, &InstInfoList[i], sizeof(CThostFtdcInstrumentField));
						pNewCallItem->m_TickCountStamp = 0;
						pNewCallItem->m_CancelTimes = 0;

					    if(CreateOrReplace == 1) 
						{
							delete pOldCallItem;
							pOldCallItem = NULL;
						}
                        m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList[OptionPairItemPos]->m_pCallItem = pNewCallItem;

						//调试用
						if(!FloatEqual(InstInfoList[i].StrikePrice, m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList[OptionPairItemPos]->m_StrikePrice, eps))
						{
							cerr << "生成T型报价出错, InstrumentID = " << InstInfoList[i].InstrumentID << ", InstrumentCode = " 
								<< InstInfoList[i].InstrumentCode << ", StrikePrice = " << InstInfoList[i].StrikePrice << ENDLINE;
						}
					} //创建或者替换期权节点
				}
				else if(OptionTypeCode == 'P' || OptionTypeCode == 'p')
				{
					COptionItem* pOldPutItem = m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList[OptionPairItemPos]->m_pPutItem;
					int CreateOrReplace = CreateOrReplaceOptionItem(&InstInfoList[i], pOldPutItem);

					if(CreateOrReplace >= 0)
					{
						COptionItem* pNewPutItem = new COptionItem;
						memcpy(&pNewPutItem->m_InstInfo, &InstInfoList[i], sizeof(CThostFtdcInstrumentField));
						pNewPutItem->m_TickCountStamp = 0;
						pNewPutItem->m_CancelTimes = 0;

					    if(CreateOrReplace == 1) 
						{
							delete pOldPutItem;
							pOldPutItem = NULL;
						}
                        m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList[OptionPairItemPos]->m_pPutItem = pNewPutItem;

						//调试用
						if(!FloatEqual(InstInfoList[i].StrikePrice, m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList[OptionPairItemPos]->m_StrikePrice, eps))
						{
							cerr << "生成T型报价出错, InstrumentID = " << InstInfoList[i].InstrumentID << ", InstrumentCode = " 
								<< InstInfoList[i].InstrumentCode << ", StrikePrice = " << InstInfoList[i].StrikePrice << ENDLINE;
						}
					} //创建或者替换期权节点
				}

				//计算期权离到期日还剩多少天
				if(m_pTshapeMonthList[MonthItemPos]->m_ExpirationLeftDays == 0) //判断是否已经赋值
				{
					char expirationDate[20];

					string str1 = InstInfoList[i].ExpireDate;
					string str2 = str1.substr(0, 4) + "/" + str1.substr(4, 2) + "/" + str1.substr(6, 2);
					strcpy(expirationDate, str2.c_str());
					m_pTshapeMonthList[MonthItemPos]->m_ExpirationLeftDays = CalcExpirationDays(expirationDate);
				}
			}  //if(YearMonth == m_pTshapeMonthList[MonthItemPos].YearMonth)
		}  //if(RetVal == 0)
		else if(RetVal == 1)   //标的证券合约
		{
			//保留合约信息给后面的程序使用
			memcpy(&UnderlyingInstrumentField, &InstInfoList[i], sizeof(CThostFtdcInstrumentField));

			//如果只生成标的节点，则要创建T型报价
			if(m_bUnderMdOnly == true)
			{
				CTshapeMonth* pTshapeMonthItem = new CTshapeMonth;

				pTshapeMonthItem->m_YearMonth = YearMonth;
				pTshapeMonthItem->m_TickCountStamp = 0;
				pTshapeMonthItem->m_UnderCancelTimes = 0;
				m_pTshapeMonthList.push_back(pTshapeMonthItem);
			}
		}
	}  //for(i)

	//统一设置标的证券合约信息
	for(size_t i = 0; i < m_pTshapeMonthList.size(); i++)
	{
		memcpy(&m_pTshapeMonthList[i]->m_UnderlyingInstInfo, &UnderlyingInstrumentField, sizeof(CThostFtdcInstrumentField));
	}

	//给各结构的父指针赋值（此时各结构的地址保持固定）
	for(size_t i = 0; i < m_pTshapeMonthList.size(); i++)
	{
		m_pTshapeMonthList[i]->m_pParent = this;

		//先删除不合法的COptionPairItem
		DeleteInvalidOptionPairItem(m_pTshapeMonthList[i]->m_pOptionPairList);

		for(size_t j = 0; j < m_pTshapeMonthList[i]->m_pOptionPairList.size(); j++)
		{
		    m_pTshapeMonthList[i]->m_pOptionPairList[j]->m_pParent = m_pTshapeMonthList[i];
			m_pTshapeMonthList[i]->m_pOptionPairList[j]->m_pCallItem->m_pParent = m_pTshapeMonthList[i]->m_pOptionPairList[j];
			m_pTshapeMonthList[i]->m_pOptionPairList[j]->m_pPutItem->m_pParent = m_pTshapeMonthList[i]->m_pOptionPairList[j];
		}
	}
#endif
}

//++
//根据CThostFtdcInstrumentField进行解析，判断是否是期权合约，或者是标的合约，并返回对应的年月份，行权价，期权类型代码
//参数
//    InstrumentField: 期权合约信息
//    YearMonth: 引用，解出来的期权对应的到期年月份
//    StrikePrice: 引用，解出来的期权行权价
//    OptionTypeCode: 引用，解出来的期权类型代码
//    pNode: 用于指向股票期权节点的指针
//返回值
//    -1: 非意图交易的期权合约，也不是期权对应的标的证券
//     0： 意图交易的期权合约
//     1: 意图交易的期权合约的标的证券
//--
int CTshapeSzse::ParseInstrumentField(CThostFtdcInstrumentField InstrumentField, size_t& YearMonth, double& StrikePrice, char& OptionTypeCode)
{
	string InstrumentID = InstrumentField.InstrumentID;
	string UnderlyingInstrID = InstrumentField.UnderlyingInstrID;
	RemoveSpace(InstrumentID);
	RemoveSpace(UnderlyingInstrID);

	int RetVal = -1;
	OptionTypeCode = ' ';

#ifndef FUTURE_OPTION_ONLY
	if(InstrumentID == m_UnderlyingId && InstrumentField.ProductClass == THOST_FTDC_PC_Stock)         //标的
	{
		YearMonth = 0;
		StrikePrice = 0;
		OptionTypeCode = ' ';

		RetVal = 1;
	}
	else if(UnderlyingInstrID == m_OptionId && InstrumentField.ProductClass == THOST_FTDC_PC_ETFOption)  //期权
	{
		YearMonth = InstrumentField.DeliveryYear*100 + InstrumentField.DeliveryMonth;
		StrikePrice = InstrumentField.StrikePrice;
		if(InstrumentField.OptionsType == THOST_FTDC_CP_CallOptions)
			OptionTypeCode = 'C';
		else if(InstrumentField.OptionsType == THOST_FTDC_CP_PutOptions)
			OptionTypeCode = 'P';

		RetVal = 0;
	}
#endif

	return RetVal;
}


//++
//根据TThostFtdcInstrumentIDType进行解析，判断是否是期权合约，或者是标的合约，并返回对应的年月份，行权价，期权类型代码
//注意：股票期权的标的没有到期日，如果是标的设置pNode指向第一个月份的TshapeMonth
//参数
//    InstrumentID: 合约代码
//    YearMonth: 引用，解出来的期权标的对应的年月份
//    StrikePrice: 引用，解出来的期权行权价
//    OptionTypeCode: 引用，解出来的期权类型代码
//    pNode: 用于指向股票期权节点的指针
//返回值
//    -1: 非意图交易的期权合约，也不是期权对应的标的合约
//     0： 意图交易的期权合约
//     1: 意图交易的期权合约的标的证券
//--
int CTshapeSzse::ParseInstrumentId(TThostFtdcInstrumentIDType InstrumentID, size_t& YearMonth, double& StrikePrice, char& OptionTypeCode, void* &pNode)
{
	bool found = false;
	int RetVal = -1;

	if(m_UnderlyingId == InstrumentID)      //是标的合约
	{
		YearMonth = 0;
		StrikePrice = 0;
		OptionTypeCode = ' ';
		pNode = m_pTshapeMonthList[0];

		RetVal = 1;
	}
	else    //查找整个T型报价，直至找到对应节点
	{
		for(size_t i = 0; i < m_pTshapeMonthList.size(); i++)
		{
			for(size_t j = 0; j < m_pTshapeMonthList[i]->m_pOptionPairList.size(); j++)
			{
				COptionItem* pCallItem = m_pTshapeMonthList[i]->m_pOptionPairList[j]->m_pCallItem;
				COptionItem* pPutItem = m_pTshapeMonthList[i]->m_pOptionPairList[j]->m_pPutItem;

				//看涨期权判断
				if(strcmp(InstrumentID, pCallItem->m_InstInfo.InstrumentID) == 0)
				{
					YearMonth = m_pTshapeMonthList[i]->m_YearMonth;
					StrikePrice = m_pTshapeMonthList[i]->m_pOptionPairList[j]->m_StrikePrice;
					OptionTypeCode = 'C';
					pNode = pCallItem;

					found = true;
					RetVal = 0;
					break;
				}

				//看跌期权判断
				if(strcmp(InstrumentID, pPutItem->m_InstInfo.InstrumentID) == 0)
				{
					YearMonth = m_pTshapeMonthList[i]->m_YearMonth;
					StrikePrice = m_pTshapeMonthList[i]->m_pOptionPairList[j]->m_StrikePrice;
					OptionTypeCode = 'P';
					pNode = pPutItem;

					found = true;
					RetVal = 0;
					break;
				}
			}

			if(found == true) break;
		}
	}

	return RetVal;
}

//++
//计算期权一手合约对应的各种参数，包括内在价值，时间价值，虚实程度和保证金等
//注意：计算的是开仓保证金，非维持保证金
//参数
//    pOptionItem: 指向期权项的指针
//    nMonthPos: 该月期权在m_pTshapeMonthList中的位置
//返回值
//    无
//--
void CTshapeSzse::CalcOptionMiscValues(COptionItem* pOptionItem, size_t nMonthPos)
{
	char OptionType;
	double Royalty;
	double UnderlyingPrice;
	double StrikePrice;

	if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_CallOptions)
	    OptionType = 'C';
	else if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_PutOptions)
		OptionType = 'P';

	//计算时间价值, 内在价值和虚值程度
	Royalty = pOptionItem->m_DepthMarketData.LastPrice;
	UnderlyingPrice = m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.LastPrice;
	StrikePrice = pOptionItem->m_InstInfo.StrikePrice;
	pOptionItem->m_TimeValue = CalcTimeValue(OptionType, UnderlyingPrice, StrikePrice, Royalty, pOptionItem->m_IntrinsicValue, pOptionItem->m_OutOfTheMoney);

	//以下估算期权卖出开仓所需的保证金
	double UsedRoyalty;
	double UsedOutOfTheMoney;
	double VolumeMultiple = (double)pOptionItem->m_InstInfo.VolumeMultiple;

	if(m_BrokerTradingParams[0].OptionRoyaltyPriceType == THOST_FTDC_ORPT_PreSettlementPrice) //使用昨结算价
		UsedRoyalty = pOptionItem->m_DepthMarketData.PreSettlementPrice;
	else
		UsedRoyalty = pOptionItem->m_DepthMarketData.LastPrice;

	//虚值程度, 对于证券标的只有昨收盘价，没有结算价
	if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_CallOptions)
		UsedOutOfTheMoney = max((StrikePrice - m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.LastPrice), 0.0);
	else if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_PutOptions)
		UsedOutOfTheMoney = max((m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.LastPrice - StrikePrice), 0.0);

	double tmp1, tmp2;
	if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_CallOptions)      //卖出看涨期权
	{
		tmp1 = m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.LastPrice * m_MarginRatio1 - UsedOutOfTheMoney;
		tmp2 = m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.LastPrice * m_MarginRatio2;
		pOptionItem->m_Margin = (UsedRoyalty + max(tmp1, tmp2)) * VolumeMultiple;
	}
	else if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_PutOptions)  //卖出看跌期权
	{
		tmp1 = m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.LastPrice * m_MarginRatio1 - UsedOutOfTheMoney;
		tmp2 = StrikePrice * m_MarginRatio2;
		double tmp3 = UsedRoyalty + max(tmp1, tmp2);
		pOptionItem->m_Margin = (min(tmp3, StrikePrice)) * VolumeMultiple;
	}
}

//++
//确定是否创建/或者代替原来的期权节点, 在模拟盘时有用（此时由于种种原因可能有多个合约代码对应同一期权)
//参数
//    pInstrumentField: 指向期权合约信息的指针
//    pOptionItem: 指向期权项的指针
//返回值
//    0: 创建期权节点
//    > 0: 替换原来的期权节点
//    < 0: 不作任何改变
//--
int CTshapeSzse::CreateOrReplaceOptionItem(CThostFtdcInstrumentField *pInstrumentField, COptionItem* pOptionItem)
{
	int ret = 0;

	//如果该期权节点不存在则创建
	if(pOptionItem == NULL) return 0;

	//否则比较创建日期, 通常认为创建日期较晚的是正常的合约
	int CreateDateNew = atoi(pInstrumentField->CreateDate);
	int CreateDateOld = atoi(pOptionItem->m_InstInfo.CreateDate);

	if(CreateDateNew > 0 && CreateDateNew > CreateDateOld)
		ret = 1;
	else
		ret = -1;

	return ret;
}
