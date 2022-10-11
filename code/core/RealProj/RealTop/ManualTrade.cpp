//++
//注：手动交易只能用到一个柜台, 其下标为[0]
//--
#include "ManualTrade.h"
#include "KeyValue.h"

//++
//手动交易和策略计算菜单功能选择
//--
void ShowManualTradeCommand(CRealApp* pRealApp, bool print)
{
	if(print){
		cerr << ENDLINE;
		cerr << "-----------------------------------------------" << ENDLINE;
		cerr << "功能选择" << ENDLINE;
		cerr << "1. 组合策略计算" << ENDLINE;
		cerr << "2. 组合策略手动下单" << ENDLINE;
		cerr << "3. 组合策略计算及手动下单" << ENDLINE;
		cerr << "4. 对冲平仓方式交易" << ENDLINE;
		cerr << "0. 退出" << ENDLINE;
		cerr << "-----------------------------------------------" << ENDLINE;
	}

	char cmd;  cin>>cmd;
	switch(cmd){
	case '1': 
		{
			StrategyCalcCommand(pRealApp);
			break;
		}
	case '2': 
		{
			PortfolioManualTrade(pRealApp);
			break;
		}
	case '3':
		{
			CalculateAndPlaceOrder(pRealApp);
			break;
		}
	case '4':
		{
			LiquidationStyleTrade(pRealApp);
			break;
		}
	case '0': 
		cerr << "输出手动交易报单信息到数据库..." << ENDLINE;
		pRealApp->m_pPosMoneyManage->SaveOrderToDatabase();
		cerr << "输出手动交易成交信息到数据库..." << ENDLINE;
		pRealApp->m_pPosMoneyManage->SaveTradeToDatabase();

		return; //exit(0);
	default:
		cerr << "无此功能" << ENDLINE;
	}

    PressAnyKeyToExit();
	ShowManualTradeCommand(pRealApp, true);
}

//++
//获取投资组合的交易列表输入
//参数
//    strKeyValueFile: 配置文件路径
//    TradeItemList: 交易的合约列表包括交易方向，期权类型，数量，期权执行价格，期权价格，期权到期日以及标的相关资料等
//    strTargetDay: 目标平仓日期
//    TimeCondition: 报单有效期类型
//    VolumeCondition: 报单成交量类型
//    MarketOrderType: 报单市价单类型
//    Intention: 获取输入的意图， 0: 仅仅用作报单用, 1: 用于策略计算, 2: 用于策略计算和报单
//返回值
//    无
//--
void GetPortfolioInput(string strKeyValueFile, vector<CTradeItem>& TradeItemList, string& strTargetDay, 
					   char& TimeCondition, char& VolumeCondition, char& MarketOrderType, int Intention)
{
	CKeyValue MyKeyValue;
	string key, value;
	CTradeItem TradeItem;
	int TradeSize = 0;

	MyKeyValue.ReadConfigFromTextFile(strKeyValueFile);

	//获取交易列表参数
	key = "TradeSize";
	cerr << "输入交易数目: TradeSize = ";
	if(MyKeyValue.GetKeyValue(key, value))
	{
		TradeSize = atoi(value.c_str());
		cerr << TradeSize << ENDLINE;
	}
	else
	{
		cin >> TradeSize;
	}

	string str1, str2;
	char tmpString[10];
	TradeItemList.clear();
	for(int i = 0; i < TradeSize; i++)
	{
		sprintf(tmpString, "%d", i);
		str1 = "InstrumentType";
		str2 = tmpString;
		str2 = RemoveSpace(str2);
		key = str1 + str2;
		cerr << ENDLINE << "输入第 " << i << " 个合约类型（InstrumentType，'u': 基础合约， 'c': 看涨期权， 'p'看跌期权）： ";
		if(MyKeyValue.GetKeyValue(key, value))
		{
			TradeItem.InstrumentType = value[0];
			cerr << TradeItem.InstrumentType << ENDLINE;
		}
		else
		{
			cin >> TradeItem.InstrumentType;
		}

		str1 = "InstrumentId";
		key = str1 + str2;
		cerr << "输入第 " << i << " 个合约代码（InstrumentId）： ";
		if(MyKeyValue.GetKeyValue(key, value))
		{
			TradeItem.InstrumentId = value;
			cerr << TradeItem.InstrumentId << ENDLINE;
		}
		else
		{
			cin >> TradeItem.InstrumentId;
		}

		str1 = "PriceTick";
		key = str1 + str2;
		cerr << "输入第 " << i << " 个最小报价单位（PriceTick）： ";
		if(MyKeyValue.GetKeyValue(key, value))
		{
			TradeItem.PriceTick = atof(value.c_str());
			cerr << TradeItem.PriceTick << ENDLINE;
		}
		else
		{
			cin >> TradeItem.PriceTick;
		}

		str1 = "Price";
		key = str1 + str2;
		cerr << "输入第 " << i << " 个交易价格（Price，<0 按照市价交易）： ";
		if(MyKeyValue.GetKeyValue(key, value))
		{
			TradeItem.Price = atof(value.c_str());
			cerr << TradeItem.Price << ENDLINE;
		}
		else
		{
			cin >> TradeItem.Price;
		}

		str1 = "Amount";
		key = str1 + str2;
		cerr << "输入第 " << i << " 个交易数量（Amount）： ";
		if(MyKeyValue.GetKeyValue(key, value))
		{
			TradeItem.Amount = atoi(value.c_str());
			cerr << TradeItem.Amount << ENDLINE;
		}
		else
		{
			cin >> TradeItem.Amount;
		}

		str1 = "Direction";
		key = str1 + str2;
		cerr << "输入第 " << i << " 个买卖方向（Direction，'b': 买入, 's': 卖出）： ";
		if(MyKeyValue.GetKeyValue(key, value))
		{
			TradeItem.Direction = value[0];
			cerr << TradeItem.Direction << ENDLINE;
		}
		else
		{
			cin >> TradeItem.Direction;
		}

		str1 = "OffsetFlag";
		key = str1 + str2;
		cerr << "输入第 " << i << " 个开平仓标志（OffsetFlag，'o': 开仓, 'c': 平仓）： ";
		if(MyKeyValue.GetKeyValue(key, value))
		{
			TradeItem.OffsetFlag = value[0];
			cerr << TradeItem.OffsetFlag << ENDLINE;
		}
		else
		{
			cin >> TradeItem.OffsetFlag;
		}

		////以下为策略计算才需要的输入
		if(Intention == 1 || Intention == 2)
		{
			//判断是否为基础标的合约代码
			if(TradeItem.InstrumentType == 'u')
			{
				TradeItem.UnderlyingInstId = TradeItem.InstrumentId;
			}
			else
			{
				str1 = "UnderlyingInstId";
				key = str1 + str2;
				cerr << "输入第 " << i << " 个标的合约代码（UnderlyingInstId）： ";
				if(MyKeyValue.GetKeyValue(key, value))
				{
					TradeItem.UnderlyingInstId = value;
					cerr << TradeItem.UnderlyingInstId << ENDLINE;
				}
				else
				{
					cin >> TradeItem.UnderlyingInstId;
				}

				//期权到期日
				str1 = "ExpireDate";
				key = str1 + str2;
				cerr << "输入第 " << i << " 个合约到期日（ExpireDate）： ";
				if(MyKeyValue.GetKeyValue(key, value))
				{
					TradeItem.ExpireDate = value;
					cerr << TradeItem.ExpireDate << ENDLINE;
				}
				else
				{
					cin >> TradeItem.ExpireDate;
				}

				//期权执行价格
				str1 = "StrikePrice";
				key = str1 + str2;
				cerr << "输入第 " << i << " 个期权执行价格（StrikePrice）： ";
				if(MyKeyValue.GetKeyValue(key, value))
				{
					TradeItem.StrikePrice = atof(value.c_str());
					cerr << TradeItem.StrikePrice << ENDLINE;
				}
				else
				{
					cin >> TradeItem.StrikePrice;
				}
			} //if TradeItem.InstrumentType

			////以下确保同一个标的的各种参数相同
			bool found;
			found = false;
			for (size_t j = 0; j < TradeItemList.size(); j++)
			{
				if(TradeItem.UnderlyingInstId == TradeItemList[j].UnderlyingInstId)
				{
					TradeItem.UnderlyingAssetType = TradeItemList[j].UnderlyingAssetType;
					TradeItem.UnderlyingPrice = TradeItemList[j].UnderlyingPrice;
					TradeItem.Hv = TradeItemList[j].Hv;
					TradeItem.DriftRate = TradeItemList[j].DriftRate;
					TradeItem.RiskFreeRate = TradeItemList[j].RiskFreeRate;

					found = true;
					break;
				}
			} //for j

			///为第一次输入该标的信息
			if(found == false)
			{
				str1 = "UnderlyingAssetType";
				key = str1 + str2;
				cerr << "输入第 " << i << " 个标的资产类型（UnderlyingAssetType，//标的资产类型, 0: 商品期货, 1: 股指期货，2: ETF, 3: 股票）： ";
				if(MyKeyValue.GetKeyValue(key, value))
				{
					TradeItem.UnderlyingAssetType = atoi(value.c_str());
					cerr << TradeItem.UnderlyingAssetType << ENDLINE;
				}
				else
				{
					cin >> TradeItem.UnderlyingAssetType;
				}

				str1 = "UnderlyingPrice";
				key = str1 + str2;
				cerr << "输入第 " << i << " 个标的合约当前价格（UnderlyingPrice，<0 通过行情查得）： ";
				if(MyKeyValue.GetKeyValue(key, value))
				{
					TradeItem.UnderlyingPrice = atof(value.c_str());
					cerr << TradeItem.UnderlyingPrice << ENDLINE;
				}
				else
				{
					cin >> TradeItem.UnderlyingPrice;
				}

				str1 = "Hv";
				key = str1 + str2;
				cerr << "输入第 " << i << " 个标的资产历史波动率（Hv，以百分比表示）： ";
				if(MyKeyValue.GetKeyValue(key, value))
				{
					TradeItem.Hv = atof(value.c_str());
					cerr << TradeItem.Hv << ENDLINE;
				}
				else
				{
					cin >> TradeItem.Hv;
				}

				str1 = "DriftRate";
				key = str1 + str2;
				cerr << "输入第 " << i << " 个标的资产年化漂移率（DriftRate，以百分比表示）： ";
				if(MyKeyValue.GetKeyValue(key, value))
				{
					TradeItem.DriftRate = atof(value.c_str());
					cerr << TradeItem.DriftRate << ENDLINE;
				}
				else
				{
					cin >> TradeItem.DriftRate;
				}

				str1 = "RiskFreeRate";
				key = str1 + str2;
				cerr << "输入第 " << i << " 个标的年化无风险利率（RiskFreeRate，以百分比表示）： ";
				if(MyKeyValue.GetKeyValue(key, value))
				{
					TradeItem.RiskFreeRate = atof(value.c_str());
					cerr << TradeItem.RiskFreeRate << ENDLINE;
				}
				else
				{
					cin >> TradeItem.RiskFreeRate;
				}
			} //if found == false
		} //if Intention

		TradeItemList.push_back(TradeItem);
	} //for i

	//输入目标平仓日期
	if(Intention == 1 || Intention == 2)
	{
		key = "TargetDay";
		cerr <<  "输入目标平仓日期（格式YYYY/MM/DD，添0对齐）： ";
		if(MyKeyValue.GetKeyValue(key, value))
		{
			strTargetDay = value;
			cerr << strTargetDay << ENDLINE;
		}
		else
		{
			cin >> strTargetDay;
		}
	}

	//输入报单有效期类型
	if(Intention == 0 || Intention == 2)
	{
		key = "TimeCondition";
		cerr <<  "输入报单有效期类型（I(OC)/G(FD)）： ";
		if(MyKeyValue.GetKeyValue(key, value))
		{
			TimeCondition = value[0];
			cerr << TimeCondition << ENDLINE;
		}
		else
		{
			cin >> TimeCondition;
		}
	}

	//输入报单成交量类型
	if(Intention == 0 || Intention == 2)
	{
		key = "VolumeCondition";
		cerr <<  "输入报单成交量类型（A(ny)/C(omplete)）： ";
		if(MyKeyValue.GetKeyValue(key, value))
		{
			VolumeCondition = value[0];
			cerr << VolumeCondition << ENDLINE;
		}
		else
		{
			cin >> VolumeCondition;
		}
	}

	//输入报单市价单类型
	if(Intention == 0 || Intention == 2)
	{
		key = "MarketOrderType";
		cerr <<  "输入报单市价单类型（A(nyPrice)/B(estPrice)/F(iveLevel)）： ";
		if(MyKeyValue.GetKeyValue(key, value))
		{
			MarketOrderType = value[0];
			cerr << MarketOrderType << ENDLINE;
		}
		else
		{
			cin >> MarketOrderType;
		}
	}
}

//++
//为投资组合中的合约获取当前市场行情参数
//参数
//    pRealApp: 指向应用类的指针
//    TradeItemList: 交易列表
//    pMarketDataArray: 对应于交易列表的市场行情数据数组，大小为交易列表大小+1，最后一个下标对应于标的行情
//    Intention: 意图， 0: 仅仅用作报单用, 1: 用于策略计算, 2: 用于策略计算和报单
//返回值
//    true: 成功获取各个交易对应合约的当前市场行情
//    false: 失败
//--
bool GetPortfolioMarketData(CRealApp* pRealApp, vector<CTradeItem> TradeItemList, CThostFtdcDepthMarketDataField* pMarketDataArray, int Intention)
{
	CRealMdSpi*  pMdUserSpi = pRealApp->m_pCtpCntIfList[0]->m_pMdUserSpi; 
	CRealTraderSpi*  pTraderUserSpi = pRealApp->m_pCtpCntIfList[0]->m_pTraderUserSpi;
	CMarketDataManage* pMarketDataManage = pRealApp->m_pMarketDataManage;

	bool bRetVal = false;
    vector<string> strInstIdList;
	size_t TradeItemListSize = TradeItemList.size();

	//获取合约代码，行情订阅
	for(size_t i = 0; i < TradeItemListSize; i++)
	{
		strInstIdList.push_back(TradeItemList[i].InstrumentId);
	}
	if((Intention == 1 || Intention == 2) && TradeItemListSize > 0)
	{
		strInstIdList.push_back(TradeItemList[TradeItemListSize-1].UnderlyingInstId);
	}

	cerr << pRealApp->m_Prompt << "订阅行情信息" << ENDLINE;
	pMarketDataManage->m_TotalInstrumentNum = strInstIdList.size();
	pMdUserSpi->SubscribeMarketData(strInstIdList);

	size_t TickCount;
	TickCount = 0;
	while(TickCount < 10)
	{
		DWORD waitReturn;
		waitReturn = WaitForSingleObject(pRealApp->m_pCtpCntIfList[0]->m_hMdSpiEvent, MD_WAIT_TIME);   //最多等10秒

		if(waitReturn != WAIT_OBJECT_0) //等待10秒，某些商品流动性不太好，可能经常出现10秒之内没有行情数据更新
		{
			SYSTEMTIME CurTime;
			GetLocalTime(&CurTime);
			cerr << pRealApp->m_Prompt << "没有行情更新，等待Tick采样数据超时或者出错！" << ENDLINE;
			cerr << pRealApp->m_Prompt << "当前时间: " << CurTime.wHour << ":" << CurTime.wMinute << ":" << CurTime.wSecond << ENDLINE;
			TickCount = TickCount + 1;
		}
		else
		{
			size_t MarketDataSize;
			pMarketDataManage->GetDepthMarketDataList();  //要更新市场行情，必须调用此函数
			MarketDataSize = pRealApp->m_pCtpCntIfList[0]->m_DepthMarketDataList.size();

			for(size_t i = 0; i < MarketDataSize; i++)
			{
				for(size_t j = 0; j < TradeItemListSize; j++)
				{
					if(strcmp(pRealApp->m_pCtpCntIfList[0]->m_DepthMarketDataList[i].InstrumentID, TradeItemList[j].InstrumentId.c_str()) == 0)
					{
						memcpy(&pMarketDataArray[j], &pRealApp->m_pCtpCntIfList[0]->m_DepthMarketDataList[i], sizeof(CThostFtdcDepthMarketDataField));
					}
				} //for j

				//标的行情数据
				if((Intention == 1 || Intention == 2) && TradeItemListSize > 0)
				{
					if(strcmp(pRealApp->m_pCtpCntIfList[0]->m_DepthMarketDataList[i].InstrumentID, TradeItemList[TradeItemListSize-1].UnderlyingInstId.c_str()) == 0)
					{
						memcpy(&pMarketDataArray[TradeItemListSize], &pRealApp->m_pCtpCntIfList[0]->m_DepthMarketDataList[i], sizeof(CThostFtdcDepthMarketDataField));
					}
				}
			} //for i

			TickCount = TickCount + 1;
		} //if waitReturn

		//检查是否行情数据都已获得
		bRetVal = true;
		for (size_t i = 0; i < TradeItemListSize; i++)
		{
			if(strcmp(pMarketDataArray[i].InstrumentID, TradeItemList[i].InstrumentId.c_str()) != 0)
			{
				bRetVal = false;
				break;
			}
		}

		//标的行情
		if((Intention == 1 || Intention == 2) && TradeItemListSize > 0 && bRetVal == true)
		{
			if(strcmp(pMarketDataArray[TradeItemListSize].InstrumentID, TradeItemList[TradeItemListSize-1].UnderlyingInstId.c_str()) != 0)
			{
				bRetVal = false;
			}
		}

		////这个必须有，注：GetDepthMarketDataList()已有实现
		//ResetEvent(pRealApp->m_hMdSpiEvent);
		if(bRetVal == true && TickCount > 3) break;

	} // while TickCount

	cerr << pRealApp->m_Prompt << "取消行情订阅..." << ENDLINE;
	pMdUserSpi->UnSubscribeMarketData(strInstIdList);

	return bRetVal;
}

//++
//组合下单
//参数
//    pRealApp: 指向应用类的指针
//    TradeItemList: 交易列表
//    pMarketDataArray: 对应于交易列表的市场行情数据数组，大小为交易列表大小+1，最后一个下标对应于标的行情
//    TimeCondition: 有效期类型, 'I'/'i': IOC; 'G'/'g': GFD
//    VolumeCondition: 成交量类型, 'C'/'c': 全部数量; 'A'/'a': 任意数量
//    MarketOrderType: 市价单类型, A(nyPrice)/B(estPrice)/F(iveLevel) 
//返回值
//    无
//--
void PlacePortfolioOrder(CRealApp* pRealApp, vector<CTradeItem> TradeItemList, CThostFtdcDepthMarketDataField* pMarketDataArray, 
						 char TimeCondition, char VolumeCondition, char MarketOrderType)
{
	CRealMdSpi*  pMdUserSpi = pRealApp->m_pCtpCntIfList[0]->m_pMdUserSpi; 
	CRealTraderSpi*  pTraderUserSpi = pRealApp->m_pCtpCntIfList[0]->m_pTraderUserSpi;

	TThostFtdcInstrumentIDType    instId;
	TThostFtdcDirectionType       dir;
	TThostFtdcCombOffsetFlagType  kpp;
	TThostFtdcPriceType           price;
	TThostFtdcVolumeType          vol;

	for(size_t i = 0; i < TradeItemList.size(); i++)
	{
		strcpy(instId, TradeItemList[i].InstrumentId.c_str());
		dir = TradeItemList[i].Direction;
		kpp[0] = TradeItemList[i].OffsetFlag;
		vol = TradeItemList[i].Amount;

		//处理价格
		if(TradeItemList[i].Price >= 0)
			price = TradeItemList[i].Price;
		else
		{
			if(dir == 'b' || dir == 'B')
			{
				price = pMarketDataArray[i].AskPrice1 + pRealApp->m_ManTradePriceTickMultiple*TradeItemList[i].PriceTick;
				if(price > pMarketDataArray[i].UpperLimitPrice) price = pMarketDataArray[i].UpperLimitPrice;
			}
			else if(dir == 's' || dir == 'S')
			{
				price = pMarketDataArray[i].BidPrice1 - pRealApp->m_ManTradePriceTickMultiple*TradeItemList[i].PriceTick;
				if(price < pMarketDataArray[i].LowerLimitPrice) price = pMarketDataArray[i].LowerLimitPrice;
			}
		} //if Price

		int RetVal = pTraderUserSpi->ReqOrderInsert(instId, dir, kpp, price, vol, TimeCondition, VolumeCondition, MarketOrderType);
		if(RetVal != 0)
		{
			cerr << pRealApp->m_Prompt << TradeItemList[i].InstrumentId << "， 买卖方向: " << dir << " 开平标志: " << kpp << "， 报单失败，停止组合报单" << ENDLINE;
			break;
		}
		//Sleep(1);
	} //for i
}

//++
//组合策略手动下单主程序
//参数
//    pRealApp: 指向应用类的指针
//返回值
//    无
//--
void PortfolioManualTrade(CRealApp* pRealApp)
{
    vector<CTradeItem> TradeItemList;
	CThostFtdcDepthMarketDataField* pMarketDataArray;
	int Intention;
	string strKeyValueFile;
	string strTargetDay;
	char TimeCondition;
	char VolumeCondition;
	char MarketOrderType;

	Intention = 0;
	
	string strConfigDir = "./config/";
	if(pRealApp->m_ArchName != "") strConfigDir = "./config/" + pRealApp->m_ArchName + "/";
    strKeyValueFile = strConfigDir + MANUAL_TRADE_CONFIG;

	GetPortfolioInput(strKeyValueFile, TradeItemList, strTargetDay, TimeCondition, VolumeCondition, MarketOrderType, Intention);

	//必须要给一次反悔的机会
	char myConfirm;
	cerr << pRealApp->m_Prompt << ENDLINE << "请确定要采取下单行动！(输入'Y'或者'N')>";
	cin >> myConfirm;
	if(myConfirm == 'n' || myConfirm == 'N')
		return;

	//确定是否需要获取市场行情
	bool bNeedMdData = false;
	bool bGetMdData = true;

	for(size_t i = 0; i < TradeItemList.size(); i++)
	{
		if(TradeItemList[i].Price < 0)
		{
			bNeedMdData = true;
			break;
		}
	}

	size_t TradeItemListSize = TradeItemList.size();
	pMarketDataArray = new CThostFtdcDepthMarketDataField[TradeItemListSize+1];
	if(bNeedMdData == true)
		bGetMdData = GetPortfolioMarketData(pRealApp, TradeItemList, pMarketDataArray, Intention);

	if(bGetMdData == false)
		cerr << pRealApp->m_Prompt << "不能获取有效市场行情，终止组合报单!" << ENDLINE;
	else
		PlacePortfolioOrder(pRealApp, TradeItemList, pMarketDataArray, TimeCondition, VolumeCondition, MarketOrderType); 

	delete[] pMarketDataArray;
}

//++
//组合策略计算
//参数
//    TradeItemList: 交易列表
//    pMarketDataArray: 存放行情数据的动态数组
//    TargetDay: 目标平仓日期
//    profitDataList: 计算得到的盈亏曲线
//返回值
//    true: 计算成功
//    false: 失败
//--
bool CalculatePortfolio(vector<CTradeItem> TradeItemList, CThostFtdcDepthMarketDataField* pMarketDataArray, string strTargetDay, vector<CProfitData>& profitDataList)
{
	bool bRetVal = true;
	vector<CPositionItem> positionItemList;
	CUnderlyingInfo underlyingInfo;
	char targetDay[20];
	bool bIvUsed;
	CStrategyResult strategyResult;
	size_t TradeItemListSize;

	TradeItemListSize = TradeItemList.size();
	if(TradeItemListSize <= 0)
	{
		cerr << "没有可以计算的项目" << ENDLINE;
		return false;
	}

	bIvUsed = false;
	strcpy(targetDay, strTargetDay.c_str());

	//从TradeItemList和pMarketDataArray中获取underlyingInfo信息
	underlyingInfo.UnderlyingAssetType = TradeItemList[0].UnderlyingAssetType;
	underlyingInfo.driftRate =  TradeItemList[0].DriftRate;
	underlyingInfo.hv = TradeItemList[0].Hv;
	underlyingInfo.riskFreeRate = TradeItemList[0].RiskFreeRate;
	if(TradeItemList[0].UnderlyingPrice > 0)
		underlyingInfo.currentPrice = TradeItemList[0].UnderlyingPrice;
	else if(strcmp(TradeItemList[0].UnderlyingInstId.c_str(), pMarketDataArray[TradeItemListSize].InstrumentID) == 0)
		underlyingInfo.currentPrice = pMarketDataArray[TradeItemListSize].LastPrice;
	else
	{
		cerr << "不能获取正确的标的价格信息" << ENDLINE;
		return false;
	}

	//从TradeItemList和pMarketDataArray中获取positionItemList信息
	CPositionItem PostionItem;
	string strUnderlyingInstId;
	strUnderlyingInstId = TradeItemList[0].UnderlyingInstId;
	for(size_t i = 0; i < TradeItemListSize; i++)
	{
		//只对开仓的情况计算
		if(TradeItemList[i].UnderlyingInstId == strUnderlyingInstId && TradeItemList[i].OffsetFlag != 'c' && TradeItemList[i].OffsetFlag != 'C')
		{
			PostionItem.amount = TradeItemList[i].Amount;
			PostionItem.direction = TradeItemList[i].Direction;
			strcpy(PostionItem.expirationDate, TradeItemList[i].ExpireDate.c_str());
			PostionItem.instrumentType = TradeItemList[i].InstrumentType;
			PostionItem.strikePrice = TradeItemList[i].StrikePrice;
			if(TradeItemList[i].Price > 0)
				PostionItem.price = TradeItemList[i].Price;
			else if((TradeItemList[i].Direction == 'b' || TradeItemList[i].Direction == 'B') && pMarketDataArray[i].AskVolume1 > 0)
				PostionItem.price = pMarketDataArray[i].AskPrice1;
			else if((TradeItemList[i].Direction == 's' || TradeItemList[i].Direction == 'S') && pMarketDataArray[i].BidVolume1 > 0)
				PostionItem.price = pMarketDataArray[i].BidPrice1;
			else
			{
				cerr << "无法获取" << pMarketDataArray[i].InstrumentID << "的有效价格" << ENDLINE;
				return false;
			}

			positionItemList.push_back(PostionItem);
		}
		else if(TradeItemList[i].UnderlyingInstId != strUnderlyingInstId)
		{
			cerr << "组合中各项目的标的信息不一致" << ENDLINE;
			return false;
		}
	}

	bRetVal = EvaluateStrategy(positionItemList, underlyingInfo, targetDay, bIvUsed, strategyResult, profitDataList);

	if(bRetVal == true) {
		//输出策略定量计算结果
		cerr << ENDLINE;
		for(int i = 0; i < strategyResult.breakEvenPointNum; i++) {
			cerr << "盈亏平衡点" << i << "为：" << strategyResult.breakEvenPoint[i] << ENDLINE;
		}
		cerr << "价格保持不变时的盈利为：" << strategyResult.unchangedProfit << ENDLINE;
		cerr << "最大盈利为：" << strategyResult.maximProfit << ENDLINE;
		cerr << "最大损失为：" << strategyResult.maximLoss << ENDLINE;
		cerr << "盈利概率为：" << strategyResult.winProbability << ENDLINE;
		cerr << "亏损概率为：" << strategyResult.loseProbability << ENDLINE;
		cerr << "平均盈利为: " << strategyResult.averageProfit << ENDLINE;
		cerr << "组合策略delta = " << strategyResult.delta << ENDLINE;
		cerr << "组合策略gamma = " << strategyResult.gamma << ENDLINE;
		cerr << "组合策略vega = " << strategyResult.vega << ENDLINE;
		cerr << "组合策略theta = " << strategyResult.theta << ENDLINE;
		cerr << "组合策略rho = " << strategyResult.rho << ENDLINE;
	}

	return bRetVal;
}

//++
//策略计算主程序
//参数
//    pRealApp: 指向应用类的指针
//返回值
//    无
//--
void StrategyCalcCommand(CRealApp* pRealApp)
{
    vector<CTradeItem> TradeItemList;
	CThostFtdcDepthMarketDataField* pMarketDataArray;
	int Intention;
	string strKeyValueFile;
	string strTargetDay;
	char TimeCondition;
	char VolumeCondition;
	char MarketOrderType;
    vector<CProfitData> profitDataList;
	bool bCalcSuccess = false;

	Intention = 1;
	
	string strConfigDir = "./config/";
	if(pRealApp->m_ArchName != "") strConfigDir = "./config/" + pRealApp->m_ArchName + "/";
    strKeyValueFile = strConfigDir + MANUAL_TRADE_CONFIG;

	GetPortfolioInput(strKeyValueFile, TradeItemList, strTargetDay, TimeCondition, VolumeCondition, MarketOrderType, Intention);

	size_t TradeItemListSize = TradeItemList.size();
	pMarketDataArray = new CThostFtdcDepthMarketDataField[TradeItemListSize+1];

	//确定是否需要获取市场行情
	bool bNeedMdData = false;
	bool bGetMdData = true;

	for(size_t i = 0; i < TradeItemListSize; i++)
	{
		if(TradeItemList[i].Price < 0 || TradeItemList[i].UnderlyingPrice < 0)
		{
			bNeedMdData = true;
			break;
		}
	}

	if(bNeedMdData == true)
		bGetMdData = GetPortfolioMarketData(pRealApp, TradeItemList, pMarketDataArray, Intention);

	double localTimeSeconds;
	SYSTEMTIME CurTime;
	GetLocalTime(&CurTime);
	localTimeSeconds = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;

	if(bGetMdData == false)
		cerr << pRealApp->m_Prompt << "不能获取有效市场行情，停止策略计算!" << ENDLINE;
	else
		bCalcSuccess = CalculatePortfolio(TradeItemList, pMarketDataArray, strTargetDay, profitDataList);

	GetLocalTime(&CurTime);
	double localTimeSeconds1 = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;
	cerr << pRealApp->m_Prompt << "策略计算加输出显示所花费的时间 = " << localTimeSeconds1 - localTimeSeconds << ENDLINE;

	if(bCalcSuccess == true)
	{
		string strOutputDir = "./output/";
		if(pRealApp->m_ArchName != "") strOutputDir = "./output/" + pRealApp->m_ArchName + "/";
		string strOutFileName = strOutputDir + STRATEGY_OUTFILE;
		OutputProfitCurve(strOutFileName, profitDataList);
	}
	else
	{
		cerr << pRealApp->m_Prompt << "策略计算失败!" << ENDLINE;
	}

	delete[] pMarketDataArray;
}

//++
//组合策略计算以及手动下单主程序
//参数
//    pRealApp: 指向应用类的指针
//返回值
//    无
//--
void CalculateAndPlaceOrder(CRealApp* pRealApp)
{
    vector<CTradeItem> TradeItemList;
	CThostFtdcDepthMarketDataField* pMarketDataArray;
	int Intention;
	string strKeyValueFile;
	string strTargetDay;
	char TimeCondition;
	char VolumeCondition;
	char MarketOrderType;
    vector<CProfitData> profitDataList;
	bool bCalcSuccess = false;

	//必须要给一次反悔的机会
	char myConfirm;
	cerr << pRealApp->m_Prompt << ENDLINE << "请确定要采取下单行动！(输入'Y'或者'N')>";
	cin >> myConfirm;
	if(myConfirm == 'n' || myConfirm == 'N')
		return;

	Intention = 2;
	
	string strConfigDir = "./config/";
	if(pRealApp->m_ArchName != "") strConfigDir = "./config/" + pRealApp->m_ArchName + "/";
    strKeyValueFile = strConfigDir + MANUAL_TRADE_CONFIG;

	GetPortfolioInput(strKeyValueFile, TradeItemList, strTargetDay, TimeCondition, VolumeCondition, MarketOrderType, Intention);

	//确定是否需要获取市场行情
	bool bNeedMdData = false;
	bool bGetMdData = true;

	for(size_t i = 0; i < TradeItemList.size(); i++)
	{
		if(TradeItemList[i].Price < 0 || TradeItemList[i].UnderlyingPrice < 0)
		{
			bNeedMdData = true;
			break;
		}
	}

	size_t TradeItemListSize = TradeItemList.size();
	pMarketDataArray = new CThostFtdcDepthMarketDataField[TradeItemListSize+1];

	if(bNeedMdData == true)
		bGetMdData = GetPortfolioMarketData(pRealApp, TradeItemList, pMarketDataArray, Intention);

	if(bGetMdData == false)
		cerr << pRealApp->m_Prompt << "不能获取有效市场行情，停止策略计算和下单操作!" << ENDLINE;
	else
		bCalcSuccess = CalculatePortfolio(TradeItemList, pMarketDataArray, strTargetDay, profitDataList);

	if(bCalcSuccess == true)
		PlacePortfolioOrder(pRealApp, TradeItemList, pMarketDataArray, TimeCondition, VolumeCondition, MarketOrderType);  //文件下单都采用GFD方式
	else
		cerr << pRealApp->m_Prompt << "策略计算失败，停止下单" << ENDLINE;

	delete[] pMarketDataArray;
}

//++
//按照对冲平仓清算方式交易，特点是不按对价报单，是按买卖之间的中间价报单
//通常使用这种方式来处理行权之后得到的期货持仓，如果市场买卖报价价差过大则可以自成交
//参数
//    pRealApp: 指向应用类的指针
//返回值
//    无
//--
void LiquidationStyleTrade(CRealApp* pRealApp)
{
    vector<CTradeItem> TradeItemList;
	CThostFtdcDepthMarketDataField* pMarketDataArray;
	int Intention;
	string strKeyValueFile;
	string strTargetDay;
	char TimeCondition;
	char VolumeCondition;
	char MarketOrderType;

	//必须要给一次反悔的机会
	char myConfirm;
	cerr << pRealApp->m_Prompt << ENDLINE << "请确定要采取下单行动！(输入'Y'或者'N')>";
	cin >> myConfirm;
	if(myConfirm == 'n' || myConfirm == 'N')
		return;

	Intention = 0;
	
	string strConfigDir = "./config/";
	if(pRealApp->m_ArchName != "") strConfigDir = "./config/" + pRealApp->m_ArchName + "/";
    strKeyValueFile = strConfigDir + MANUAL_TRADE_CONFIG;

	GetPortfolioInput(strKeyValueFile, TradeItemList, strTargetDay, TimeCondition, VolumeCondition, MarketOrderType, Intention);

	//确定是否需要获取市场行情
	bool bNeedMdData = false;
	bool bGetMdData = true;

	for(size_t i = 0; i < TradeItemList.size(); i++)
	{
		if(TradeItemList[i].Price < 0)
		{
			bNeedMdData = true;
			break;
		}
	}

	size_t TradeItemListSize = TradeItemList.size();
	pMarketDataArray = new CThostFtdcDepthMarketDataField[TradeItemListSize+1];
	if(bNeedMdData == true)
		bGetMdData = GetPortfolioMarketData(pRealApp, TradeItemList, pMarketDataArray, Intention);

	//除了下单操作外，和PortfolioManualTrade函数几乎一致
	if(bGetMdData == false)
		cerr << pRealApp->m_Prompt << "不能获取有效市场行情，终止组合报单!" << ENDLINE;
	else
		PlaceLiquidationStyleOrder(pRealApp, TradeItemList, pMarketDataArray, TimeCondition, VolumeCondition, MarketOrderType);

	delete[] pMarketDataArray;
}

//++
//对冲平仓清算方式下单，报单不是按优于对价的方式进行，而是按照市场买卖报价的中间价进行
//适用于市场买卖价差较大，本人持有同一合约相反方向头寸的情形
//参数
//    pRealApp: 指向应用类的指针
//    TradeItemList: 交易列表
//    pMarketDataArray: 对应于交易列表的市场行情数据数组，大小为交易列表大小+1，最后一个下标对应于标的行情
//    TimeCondition: 报单有效期类型
//    VolumeCondition: 报单成交数量类型
//    MarketOrderType: 市价单类型, AnyPrice/BestPrice
//返回值
//    无
//--
void PlaceLiquidationStyleOrder(CRealApp* pRealApp, vector<CTradeItem> TradeItemList, CThostFtdcDepthMarketDataField* pMarketDataArray, 
								char TimeCondition, char VolumeCondition, char MarketOrderType)
{
	CRealMdSpi*  pMdUserSpi = pRealApp->m_pCtpCntIfList[0]->m_pMdUserSpi; 
	CRealTraderSpi*  pTraderUserSpi = pRealApp->m_pCtpCntIfList[0]->m_pTraderUserSpi;

	TThostFtdcInstrumentIDType    instId;
	TThostFtdcDirectionType       dir;
	TThostFtdcCombOffsetFlagType  kpp;
	TThostFtdcPriceType           price;
	TThostFtdcVolumeType          vol;

	for(size_t i = 0; i < TradeItemList.size(); i++)
	{
		strcpy(instId, TradeItemList[i].InstrumentId.c_str());
		dir = TradeItemList[i].Direction;
		kpp[0] = TradeItemList[i].OffsetFlag;
		vol = TradeItemList[i].Amount;

		//处理价格，如果采用限价方式和PlacePortfolioOrder方法一致
		if(TradeItemList[i].Price >= 0)
			price = TradeItemList[i].Price;
		else
		{
			//计算买卖价差的1/2是PriceTick的倍数
			int BuyHalfGapMultiple = int(0.501+(pMarketDataArray[i].AskPrice1 - pMarketDataArray[i].BidPrice1)/(2*TradeItemList[i].PriceTick));
			int SellHalfGapMultiple = int(0.001+(pMarketDataArray[i].AskPrice1 - pMarketDataArray[i].BidPrice1)/(2*TradeItemList[i].PriceTick));

			if(dir == 'b' || dir == 'B')
			{
				price = pMarketDataArray[i].BidPrice1 + BuyHalfGapMultiple * TradeItemList[i].PriceTick;
				if(price > pMarketDataArray[i].UpperLimitPrice) price = pMarketDataArray[i].UpperLimitPrice;
			}
			else if(dir == 's' || dir == 'S')
			{
				price = pMarketDataArray[i].BidPrice1 + SellHalfGapMultiple * TradeItemList[i].PriceTick;
				if(price < pMarketDataArray[i].LowerLimitPrice) price = pMarketDataArray[i].LowerLimitPrice;
			}
		} //if Price

		//对于自交易的对冲平仓方式，不能使用IOC，不然可能全部被撤单；需要使用GFD方式。
		int RetVal = pTraderUserSpi->ReqOrderInsert(instId, dir, kpp, price, vol, TimeCondition, VolumeCondition, MarketOrderType);
		if(RetVal != 0)
		{
			cerr << pRealApp->m_Prompt << TradeItemList[i].InstrumentId << "， 买卖方向: " << dir << " 开平标志: " << kpp << "， 报单失败，停止组合报单" << ENDLINE;
			break;
		}
		//Sleep(1);
	} //for i
}

