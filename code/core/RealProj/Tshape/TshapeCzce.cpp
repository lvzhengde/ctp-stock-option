#include "TshapeCzce.h"
#include "OptionPriceModel.h"


//++
//构造函数
//--
CTshapeCzce::CTshapeCzce(string OptionId)
{
	//交易所代码必须是郑州商品交易所
	TThostFtdcExchangeIDType ExchangeId = "CZCE"; 
	m_OptionId = OptionId;
	//郑商所期权代码和标的代码一样（指合约代码的前一位或者两位）
	m_UnderlyingId = OptionId;
    m_UnderlyingAssetType = 0;
	strcpy(m_ExchangeId, ExchangeId);

	m_bUnderMdOnly = false;
	m_TickCountStamp = 0;
}

//++
//析构函数
//--
CTshapeCzce::~CTshapeCzce()
{
}

//++
//根据ReqQryInstrument返回的CThostFtdcInstrumentField列表，构造T型报价结构，并且计算出对应月份的期权离到期日还剩多少天
//CThostFtdcInstrumentField中的日期都是YYYYMMDD形式
//注意：
//    在构造T型报价的过程中，各个结构的地址因为插入排序的原因会经常变动，所以给父指针赋值的操作应该在T型报价构造完毕之后进行
//--
void CTshapeCzce::ConstructTshape(vector<CThostFtdcInstrumentField> InstInfoList)
{
	//可以接受的浮点数表示精度误差
	double eps = (1e-6);
	void *pNode = NULL;

	int InstInfoListSize;
	TThostFtdcInstrumentIDType UnderlyingInstId;
	string str1;
	string str2;
	size_t InstIdPrefixSize;

	InstIdPrefixSize = m_OptionId.size();
	InstInfoListSize = InstInfoList.size();

	for(int i = 0; i < InstInfoListSize; i++)
	{
		size_t YearMonth;
		double StrikePrice;
		char OptionTypeCode;
		int RetVal;
		int YearMonthListSize;
		int TshapeMonthListSize;
		bool found;
		bool bCreateMonthItem;

		found = false;
		bCreateMonthItem = false;

		YearMonthListSize = m_YearMonthList.size();
		TshapeMonthListSize = m_pTshapeMonthList.size();

		//确保是郑商所的商品期权合约
		YearMonth = 0;
		StrikePrice = 0;
		OptionTypeCode = 0;
		if(strcmp(m_ExchangeId, InstInfoList[i].ExchangeID) != 0)
			RetVal = -1;
		else
			RetVal = ParseInstrumentId(InstInfoList[i].InstrumentID, YearMonth, StrikePrice, OptionTypeCode, pNode);

		//查找是否在可以交易的年月份列表中
		for(int j = 0; j < YearMonthListSize; j++)
		{
			if(YearMonth == m_YearMonthList[j])
			{
				found = true;
				break;
			}
		}

		//年月份列表为空的情况下，创建所有的T型报价
		if(YearMonthListSize == 0 || found == true) 
			bCreateMonthItem = true;
		else //否则跳过此合约，否则访问后面没有元素的m_pTshapeMonthList会出错
			continue;

		//获取标的期货合约代码
		if(RetVal == 1)
		{
			strcpy(UnderlyingInstId, InstInfoList[i].InstrumentID);
		}
		else if(RetVal == 0)
		{
			str1 = InstInfoList[i].InstrumentID;
			str2 = str1.substr(0, InstIdPrefixSize+3);
			strcpy(UnderlyingInstId, str2.c_str());
		}

		//是设置对应的期权合约或者标的期货合约
		if(RetVal == 1 || RetVal == 0)
		{
			int MonthItemPos = 0;

			for(int j = 0; j < TshapeMonthListSize; j++)
			{
				//该月份TshapeMonthItem已经存在
				if(YearMonth == m_pTshapeMonthList[j]->m_YearMonth)
				{
					MonthItemPos = j;
					bCreateMonthItem = false;
					break;
				}
			}

			//先按标的期货合约生成TshapeMonthItem
			if(bCreateMonthItem == true)
			{
				CTshapeMonth* pTshapeMonthItem = new CTshapeMonth;

				pTshapeMonthItem->m_YearMonth = YearMonth;
				pTshapeMonthItem->m_TickCountStamp = 0;
				pTshapeMonthItem->m_UnderCancelTimes = 0;
				strcpy(pTshapeMonthItem->m_UnderlyingInstId, UnderlyingInstId);

				MonthItemPos = InsertOrderByYearMonth(m_pTshapeMonthList, pTshapeMonthItem);
				TshapeMonthListSize = TshapeMonthListSize + 1;
			}

			//如果是标的合约则更新其合约信息，必须年月份相符
			if(RetVal == 1 && YearMonth == m_pTshapeMonthList[MonthItemPos]->m_YearMonth)
			{
				memcpy(&m_pTshapeMonthList[MonthItemPos]->m_UnderlyingInstInfo, &InstInfoList[i], sizeof(CThostFtdcInstrumentField));
			}

			//如果只需要构造标的合约报价链，则跳过以下构造期权T型报价的部分
			if(m_bUnderMdOnly == true)
				continue;

			//是期权合约，且年月份相符
			if(RetVal == 0 && YearMonth == m_pTshapeMonthList[MonthItemPos]->m_YearMonth)
			{
				bool bCreateOptionPairItem;
				int OptionPairListSize;
				int OptionPairItemPos = 0;

		        bCreateOptionPairItem = true;
				OptionPairListSize = m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList.size();
				for (int j = 0; j < OptionPairListSize; j++)
				{
					//查找该OptionPairItem是否存在
					if(FloatEqual(StrikePrice, m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList[j]->m_StrikePrice, eps))
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
					OptionPairItemPos = InsertOrderByStrikePrice(m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList, pOptionPairItem);
					OptionPairListSize = OptionPairListSize + 1;
				}

				//更新相应的OptionItem
				if(OptionTypeCode == 'C' || OptionTypeCode == 'c')
				{
					COptionItem* pCallItem = new COptionItem;
					m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList[OptionPairItemPos]->m_pCallItem = pCallItem;
					memcpy(&pCallItem->m_InstInfo, &InstInfoList[i], sizeof(CThostFtdcInstrumentField));
					pCallItem->m_TickCountStamp = 0;
					pCallItem->m_CancelTimes = 0;
				}
				else if(OptionTypeCode == 'P' || OptionTypeCode == 'p')
				{
					COptionItem* pPutItem = new COptionItem;
					m_pTshapeMonthList[MonthItemPos]->m_pOptionPairList[OptionPairItemPos]->m_pPutItem = pPutItem;
					memcpy(&pPutItem->m_InstInfo, &InstInfoList[i], sizeof(CThostFtdcInstrumentField));
					pPutItem->m_TickCountStamp = 0;
					pPutItem->m_CancelTimes = 0;
				}

				//计算期权离到期日还剩多少天
				if(m_pTshapeMonthList[MonthItemPos]->m_ExpirationLeftDays == 0) //判断是否已经赋值
				{
					char expirationDate[20];

					str1 = InstInfoList[i].ExpireDate;
					str2 = str1.substr(0, 4) + "/" + str1.substr(4, 2) + "/" + str1.substr(6, 2);
					strcpy(expirationDate, str2.c_str());
					m_pTshapeMonthList[MonthItemPos]->m_ExpirationLeftDays = CalcExpirationDays(expirationDate);
				}
			}  //if(RetVal == 0)
		}  //if(RetVal == 1 || RetVal == 0)
	}  //for(i)

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
}

//++
//根据TThostFtdcInstrumentIDType进行解析，判断是否是期权合约，或者是标的合约，并返回对应的年月份，行权价，期权类型代码
//参数
//    InstrumentID: 合约代码
//    YearMonth: 引用，解出来的期权标的对应的年月份
//    StrikePrice: 引用，解出来的期权行权价
//    OptionTypeCode: 引用，解出来的期权类型代码
//    pNode: 用于指向股票期权节点的指针
//返回值
//    -1: 非意图交易的期权合约，也不是期权对应的标的合约
//     0： 意图交易的期权合约
//     1: 意图交易的期权合约的标的期货合约
//--
int CTshapeCzce::ParseInstrumentId(TThostFtdcInstrumentIDType InstrumentID, size_t& YearMonth, double& StrikePrice, char& OptionTypeCode, void* &pNode)
{
	SYSTEMTIME CurTime;
	string strInstId;
	size_t InstIdSize;
	size_t InstIdPrefixSize;
	int InstIdYear;
	int InstIdMonth;
	int SingleDigitYear;
	int Year;
	string str1;

	YearMonth = 0;
	StrikePrice = 0;
	OptionTypeCode = 0;

	GetLocalTime(&CurTime);
	SingleDigitYear = CurTime.wYear - int(CurTime.wYear/10)*10;

	strInstId = InstrumentID;
	InstIdSize = strInstId.size();
	InstIdPrefixSize = m_OptionId.size();

	str1 = strInstId.substr(0, InstIdPrefixSize);
	if(strcmp(str1.c_str(), m_OptionId.c_str()) != 0 || InstIdSize < (InstIdPrefixSize+3))
		return -1;

	str1 = strInstId.substr(InstIdPrefixSize, 1);
	InstIdYear = atoi(str1.c_str());
	if(InstIdYear >= SingleDigitYear)
		Year = int(CurTime.wYear/10)*10 + InstIdYear;
	else
		Year = int(1+CurTime.wYear/10)*10 + InstIdYear;

	str1 = strInstId.substr(InstIdPrefixSize+1, 2);
	InstIdMonth = atoi(str1.c_str());

	YearMonth = Year*10*10 + InstIdMonth;

	//为期权对应的标的期货合约
	if(InstIdSize == (InstIdPrefixSize+3))
		return 1;

	OptionTypeCode = InstrumentID[InstIdPrefixSize+3];

	int StrikePriceLen = InstIdSize -(InstIdPrefixSize+4);
	if(StrikePriceLen <= 0)
		return -1;

	str1 = strInstId.substr(InstIdPrefixSize+4, StrikePriceLen);
	//有可能是套利合约，判断标准是看行权价部分是否为数字
	if(IsNum(str1) == false)
		return -1;

	StrikePrice = atof(str1.c_str());

	return 0;
}

//++
//计算期权一手合约对应的各种参数，包括内在价值，时间价值，虚实程度和保证金等
//参数
//    pOptionItem: 指向期权项的指针
//    nMonthPos: 该月期权在m_pTshapeMonthList中的位置
//返回值
//    无
//--
void CTshapeCzce::CalcOptionMiscValues(COptionItem* pOptionItem, size_t nMonthPos)
{
	char OptionType;
	double Royalty;
	double UnderlyingPrice;
	double StrikePrice;

	if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_CallOptions)
	    OptionType = 'C';
	else if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_PutOptions)
		OptionType = 'P';

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

	if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_CallOptions)
		UsedOutOfTheMoney = max((StrikePrice - m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.PreSettlementPrice)*VolumeMultiple, 0.0);
	else if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_PutOptions)
		UsedOutOfTheMoney = max((m_pTshapeMonthList[nMonthPos]->m_UnderlyingMarketData.PreSettlementPrice - StrikePrice)*VolumeMultiple, 0.0);

	double tmp1, tmp2;
	if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_CallOptions)      //卖出看涨期权
	{
		tmp1 = m_pTshapeMonthList[nMonthPos]->m_ShortUnderlyingMargin - UsedOutOfTheMoney * 0.5;
		tmp2 = m_pTshapeMonthList[nMonthPos]->m_ShortUnderlyingMargin * 0.5;
		pOptionItem->m_Margin = UsedRoyalty * VolumeMultiple + max(tmp1, tmp2);
	}
	else if(pOptionItem->m_InstInfo.OptionsType == THOST_FTDC_CP_PutOptions)  //卖出看跌期权
	{
		tmp1 = m_pTshapeMonthList[nMonthPos]->m_LongUnderlyingMargin - UsedOutOfTheMoney * 0.5;
		tmp2 = m_pTshapeMonthList[nMonthPos]->m_LongUnderlyingMargin * 0.5;
		pOptionItem->m_Margin = UsedRoyalty * VolumeMultiple + max(tmp1, tmp2);
	}
}
