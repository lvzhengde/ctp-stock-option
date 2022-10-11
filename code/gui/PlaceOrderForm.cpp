#include "PlaceOrderForm.h"
#include "MuxCntWindowForm.h"
#include <QMessageBox>

PlaceOrderForm::PlaceOrderForm(CRealApp *pRealApp, MuxCntWindowForm* pMainWindow, QWidget *parent)
	: m_pRealApp(pRealApp), m_pMainWindow(pMainWindow), QWidget(parent)
{
    setupUi(this);
	InitForm();
    startTimer(500);	

	bIsOpened = false;
}

PlaceOrderForm::~PlaceOrderForm()
{

}

void PlaceOrderForm::on_lineEdit_textChanged(const QString &text)
{
    string strInstId = text.toStdString();
	TThostFtdcInstrumentIDType    InstrumentID;
    strcpy(InstrumentID, strInstId.c_str());

	char InstrumentType = 0;
	size_t UnderlyingAssetType;
	CThostFtdcInstrumentField *pInstInfo = NULL;
	CThostFtdcDepthMarketDataField *pMd = NULL;
	CCtpCntIf* pCtpCntIf = NULL;	
    string  InstrumentNameGBK, InstrumentName;

	void* pNode = GetMarketNodePointer(m_pRealApp, InstrumentID, InstrumentType, UnderlyingAssetType);
	if(pNode != NULL) 
	{
		char cntCode = 0;

    	if(InstrumentType == 'u')
    	{
    		CTshapeMonth* pTshapeMonth = (CTshapeMonth*)pNode;
    		pInstInfo = &pTshapeMonth->m_UnderlyingInstInfo;
            pMd = &pTshapeMonth->m_UnderlyingMarketData;
			cntCode = pTshapeMonth->m_pParent->m_UnderCntCode;
    	}
    	else
    	{
    		COptionItem* pOptionItem = (COptionItem*)pNode;
            pInstInfo = &pOptionItem->m_InstInfo;
            pMd = &pOptionItem->m_DepthMarketData;
			cntCode = pOptionItem->m_pParent->m_pParent->m_pParent->m_OptCntCode;
    	}

    	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
    	{
    		if(cntCode == m_pRealApp->m_pCtpCntIfList[i]->m_CntCode)
    		{
    			pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
    			break;
    		}
    	}        

    	InstrumentNameGBK = pInstInfo->InstrumentName;
    	GBKToUTF8(InstrumentNameGBK, InstrumentName);
	}

	m_OrderInfo.InstrumentType = InstrumentType;
	m_OrderInfo.UnderlyingAssetType = UnderlyingAssetType;
	m_OrderInfo.pInstInfo = pInstInfo;
	m_OrderInfo.pMd = pMd;
	m_OrderInfo.pCtpCntIf = pCtpCntIf;

	labelInstrumentName->setText(InstrumentName.c_str());
	DisplayMaketData();

	if(pInstInfo != NULL)
	{
        labelVolumeMultiple->setText("合约乘数: " + QString::number(pInstInfo->VolumeMultiple));

		doubleSpinBoxPrice->setRange(0, pMd->UpperLimitPrice * 50);
		doubleSpinBoxPrice->setSingleStep(pInstInfo->PriceTick);
		doubleSpinBoxPrice->setDecimals(GetDecimals(pInstInfo->PriceTick));

		int MaxOrderVolume = max(pInstInfo->MaxMarketOrderVolume, pInstInfo->MaxLimitOrderVolume);
		int MinOrderVolume = min(pInstInfo->MinMarketOrderVolume, pInstInfo->MinLimitOrderVolume);
		doubleSpinBoxVolume->setRange(MinOrderVolume, MaxOrderVolume);
	}
}

void PlaceOrderForm::on_btnSendData_clicked()
{
	double eps = 1e-9;
	int ret = -100;

    if(m_OrderInfo.pInstInfo != NULL)
	{
        if(radioButtonBuy->isChecked())	
			m_OrderInfo.Direction = 'b';
		else if(radioButtonSell->isChecked())
			m_OrderInfo.Direction = 's';
		else
            m_OrderInfo.Direction = 0;

		if(radioButtonOpen->isChecked())
	        m_OrderInfo.OffsetFlag = 'o';
		else if(radioButtonClose->isChecked())
			m_OrderInfo.OffsetFlag = 'c';
		else
            m_OrderInfo.OffsetFlag = 0;

        m_OrderInfo.Price = doubleSpinBoxPrice->value();
        m_OrderInfo.Volume = int(doubleSpinBoxVolume->value() + eps);

		if(checkBoxFOK->isChecked())
			m_OrderInfo.bFOK = true;
		else
            m_OrderInfo.bFOK = false;

        QMessageBox:: StandardButton result = QMessageBox::information(this, "下单确认", "是否确认下单?", QMessageBox::Yes | QMessageBox::No);
        if(result == QMessageBox::Yes)
		{
			if(m_OrderInfo.Price < eps)  //市价单
			{
			    result = QMessageBox::information(this, "市价单", "当前报单为市价单!\r\n是否确认下单?", QMessageBox::Yes | QMessageBox::No);
				if(result == QMessageBox::Yes)
                    ret = PlaceOrder();
                else
                    ret = -200;
			}
			else //限价单
			{
	    	    ret = PlaceOrder();
			}
		}
		else
		{
			ret = -200;
		}
	}
	else
	{
        QMessageBox::information(this, "下单确认", "不合法的订单，无法发送!", QMessageBox::Ok);	
	}

    QString sdata = (ret == -100) ? "没有报单!" : (ret == -200) ? "报单取消" : (ret == 0) ? "报单成功!" : "报单失败!";
	emit mvsigSendData(sdata, ret);

	close();
}

int PlaceOrderForm::PlaceOrder()
{
	double eps = 1e-9;	
	TThostFtdcInstrumentIDType    InstrumentID;
	TThostFtdcDirectionType       dir;
	TThostFtdcCombOffsetFlagType  kpp;
	TThostFtdcPriceType           price;
	TThostFtdcVolumeType          vol;

	strcpy(InstrumentID, m_OrderInfo.pInstInfo->InstrumentID);
    string strExchangeID = m_OrderInfo.pInstInfo->ExchangeID;

	dir = m_OrderInfo.Direction;
	price = m_OrderInfo.Price;
	vol = m_OrderInfo.Volume;

	if(dir == 'b' && price > m_OrderInfo.pMd->UpperLimitPrice)
        price = m_OrderInfo.pMd->UpperLimitPrice;
	if(dir == 's' && price >= eps && price < m_OrderInfo.pMd->LowerLimitPrice)
		price = m_OrderInfo.pMd->LowerLimitPrice;

	if(m_OrderInfo.OffsetFlag == 'c')
	{
    	if(strExchangeID != "SHFE")
    	{
    		kpp[0] = 'c'; //平仓
    		if(m_OrderInfo.InstrumentType == 'u' && (strExchangeID == "SSE" || strExchangeID == "SZSE"))
    		{
    			kpp[0] = 'o'; //标的证券使用此方式
    		}
    	} 
    	else //上期所平仓
    	{
    		bool IsTodayPosition = IsShfeTodayPosition();
    	
    		if(IsTodayPosition == true) 
    			kpp[0] = 'j'; //平今
    		else
    			kpp[0] = 'c'; //平昨
    	}		
	}
	else
	{
		kpp[0] = m_OrderInfo.OffsetFlag;
	}

	char TimeCondition = 0; 
	char VolumeCondition = 0;
	char MarketOrderType = 0;
	if(strExchangeID == "SSE")    //上海证券交易所
	{
		if(m_OrderInfo.InstrumentType != 'u')     //股票期权
		{
            MarketOrderType = 'B';    //上交所股票期权支持的市价单类型为BestPrice

			if(m_OrderInfo.bFOK == true)
			{
				TimeCondition = 'I';
				VolumeCondition = 'C';
			}
			else
			{
				TimeCondition = 'G';
				VolumeCondition = 'A';
			}
		}
		else   //证券标的
		{
            MarketOrderType = 'F';    //上交所股票标的支持的市价单类型为FiveLevel

			if(m_OrderInfo.bFOK == true) 
			{
				price = 0;     //上交所证券即时成交只支持市价单, 市价单采用五档价格即时成交
				TimeCondition = 'I';
				VolumeCondition = 'A';
			}
			else   //其它情况都只能采用限价单
			{
				TimeCondition = 'G';
				VolumeCondition = 'A';
			}
		}
	}
	else if(strExchangeID == "SZSE")    //深圳证券交易所
	{
		if(m_OrderInfo.InstrumentType != 'u')     //股票期权
		{
            MarketOrderType = 'A';    //深交所股票期权支持的市价单类型为AnyPrice(CV/AV均可), 或者FiveLevelPrice(只支持AV)

			if(m_OrderInfo.bFOK == true)
			{
				TimeCondition = 'I';
				VolumeCondition = 'C';  //深交所限价FOK单只支持CV
				if(price < eps)         //市价单, AV/CV均支持, 优先使用AV方式
				  VolumeCondition = 'A'; 
			}
			else
			{
				TimeCondition = 'G';
				VolumeCondition = 'A';  //深交所GFD限价单只支持AV
			}
		}
		else   //证券标的
		{
			cerr << "深交所不支持股票期权标的证券交易, 其下单方式尚未经过测试" << ENDLINE;					
		}
	}
	else  //其它四个期货交易所
	{
		MarketOrderType = 'A';     //商品交易所支持的市价单类型为AnyPrice

		if(m_OrderInfo.bFOK == true)
		{
			TimeCondition = 'I';
			VolumeCondition = 'A';
		}
		else
		{
			TimeCondition = 'G';
			VolumeCondition = 'A';
		}
	}

	//设置投机套保标志
	TThostFtdcHedgeFlagType HedgeFlag = THOST_FTDC_HF_Speculation;
#ifndef FUTURE_OPTION_ONLY
	bool bCoveredOpen = false;
	if(bCoveredOpen == true && m_OrderInfo.InstrumentType == 'c' 
		&& (m_OrderInfo.UnderlyingAssetType == 2 || m_OrderInfo.UnderlyingAssetType == 3))
	{
		HedgeFlag = THOST_FTDC_HF_Covered;
	}
	else
	{
		HedgeFlag = THOST_FTDC_HF_Speculation;
	}
#endif

	CRealTraderSpi*  pTraderUserSpi = m_OrderInfo.pCtpCntIf->m_pTraderUserSpi;
	int ret = pTraderUserSpi->ReqOrderInsert(InstrumentID, dir, kpp, price, vol, TimeCondition, VolumeCondition, MarketOrderType, HedgeFlag, strExchangeID);
	return ret;
}

bool PlaceOrderForm::IsShfeTodayPosition()
{
	bool bIsTodayPosition = false;

    EnterCriticalSection(&m_pMainWindow->m_PositionCS);
	for(int i = 0; i < m_pMainWindow->m_InvestorPositionList.size(); i++)
	{
	    CThostFtdcInvestorPositionField* pInvestorPosition = m_pMainWindow->m_InvestorPositionList[i];

		char InvestorDirection = pInvestorPosition->PosiDirection;
		if(InvestorDirection == THOST_FTDC_PD_Long)
			InvestorDirection = 'b';
		else if(InvestorDirection == THOST_FTDC_PD_Short)
			InvestorDirection = 's';	

		//凡是有今仓则先平今仓
		if(strcmp(m_OrderInfo.pInstInfo->ExchangeID, pInvestorPosition->ExchangeID) == 0 
				&& strcmp(m_OrderInfo.pInstInfo->InstrumentID, pInvestorPosition->InstrumentID) == 0
				&& m_OrderInfo.Direction != InvestorDirection
				&& pInvestorPosition->TodayPosition > 0
				&& pInvestorPosition->Position > 0)
		{
		    bIsTodayPosition = true;
			break;
		}
	}
    LeaveCriticalSection(&m_pMainWindow->m_PositionCS);

    return bIsTodayPosition;
}

void PlaceOrderForm::InitForm()
{
	this->setWindowTitle("报单界面");
	
    labelInstrumentID->setText("合约:");
	labelInstrumentName->setText("合约名称");

	groupBoxOffsetFlag->setTitle("开平");
	radioButtonOpen->setText("开仓");
	radioButtonOpen->setChecked(true);
	radioButtonClose->setText("平仓");

	labelVolumeMultiple->setText("合约乘数");
    checkBoxFOK->setChecked(false);

	labelPrice->setText("价格");
	labelVolume->setText("数量");	
    doubleSpinBoxVolume->setSingleStep(1);
    doubleSpinBoxVolume->setDecimals(0);

	groupBoxDirection->setTitle("买卖方向");
	radioButtonBuy->setText("买入");
	radioButtonBuy->setChecked(true);
	radioButtonSell->setText("卖出");

	btnSendData->setText("下单");
}

void PlaceOrderForm::DisplayMaketData()
{
	listWidget->clear();
	CThostFtdcDepthMarketDataField *pMd = m_OrderInfo.pMd;
	CThostFtdcInstrumentField *pInstInfo = m_OrderInfo.pInstInfo;
    	
    if(pMd != NULL && pInstInfo != NULL)
	{
	    int deciNum = GetDecimals(pInstInfo->PriceTick);
		deciNum  = (deciNum > 5) ? 5 : deciNum;

	    QString qstrPrice;

	    string strItem;
		qstrPrice = (pMd->AskVolume5 > 0) ? QString::number(pMd->AskPrice5, 'f', deciNum) : "-"; 
		strItem = "卖五    " + qstrPrice.toStdString() + "    " + to_string(pMd->AskVolume5);
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		qstrPrice = (pMd->AskVolume4 > 0) ? QString::number(pMd->AskPrice4, 'f', deciNum) : "-"; 
		strItem = "卖四    " + qstrPrice.toStdString() + "    " + to_string(pMd->AskVolume4);
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		qstrPrice = (pMd->AskVolume3 > 0) ? QString::number(pMd->AskPrice3, 'f', deciNum) : "-"; 
		strItem = "卖三    " + qstrPrice.toStdString() + "    " + to_string(pMd->AskVolume3);
        listWidget->addItem(strItem.c_str());
	
		strItem.clear();
		qstrPrice = (pMd->AskVolume2 > 0) ? QString::number(pMd->AskPrice2, 'f', deciNum) : "-"; 
		strItem = "卖二    " + qstrPrice.toStdString() + "    " + to_string(pMd->AskVolume2);
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		qstrPrice = (pMd->AskVolume1 > 0) ? QString::number(pMd->AskPrice1, 'f', deciNum) : "-"; 
		strItem = "卖一    " + qstrPrice.toStdString() + "    " + to_string(pMd->AskVolume1);
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		strItem = "------------------------------------";
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		qstrPrice = (pMd->BidVolume1 > 0) ? QString::number(pMd->BidPrice1, 'f', deciNum) : "-"; 
		strItem = "买一    " + qstrPrice.toStdString() + "    " + to_string(pMd->BidVolume1);
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		qstrPrice = (pMd->BidVolume2 > 0) ? QString::number(pMd->BidPrice2, 'f', deciNum) : "-"; 
		strItem = "买二    " + qstrPrice.toStdString() + "    " + to_string(pMd->BidVolume2);
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		qstrPrice = (pMd->BidVolume3 > 0) ? QString::number(pMd->BidPrice3, 'f', deciNum) : "-"; 
		strItem = "买三    " + qstrPrice.toStdString() + "    " + to_string(pMd->BidVolume3);
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		qstrPrice = (pMd->BidVolume4 > 0) ? QString::number(pMd->BidPrice4, 'f', deciNum) : "-"; 
		strItem = "买四    " + qstrPrice.toStdString() + "    " + to_string(pMd->BidVolume4);
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		qstrPrice = (pMd->BidVolume5 > 0) ? QString::number(pMd->BidPrice5, 'f', deciNum) : "-"; 
		strItem = "买五    " + qstrPrice.toStdString() + "    " + to_string(pMd->BidVolume5);
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		strItem = "------------------------------------";
        listWidget->addItem(strItem.c_str());
 
		QString qstrLastPrice = (pMd->LastPrice >= 0 && pMd->LastPrice <= pMd->UpperLimitPrice) ? QString::number(pMd->LastPrice, 'f', deciNum) : "-";
		QString qstrOpenPrice = (pMd->OpenPrice >= 0 && pMd->OpenPrice <= pMd->UpperLimitPrice) ? QString::number(pMd->OpenPrice, 'f', deciNum) : "-";
		strItem.clear();
		strItem = "最新    " + qstrLastPrice.toStdString() + "  开盘    " + qstrOpenPrice.toStdString();
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		double deltaPrice = (pMd->LastPrice >= 0 && pMd->LastPrice <= pMd->UpperLimitPrice) ? pMd->LastPrice - pMd->PreSettlementPrice : 0;
		QString qstrHighestPrice = (pMd->HighestPrice >= 0 && pMd->HighestPrice <= pMd->UpperLimitPrice) ? QString::number(pMd->HighestPrice, 'f', deciNum) : "-";
		strItem = "涨跌    " + QString::number(deltaPrice, 'f', deciNum).toStdString() + "  最高    " + qstrHighestPrice.toStdString();
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		double deltaPercent = 100.0 * deltaPrice / pMd->PreSettlementPrice + 0.005;
		QString qstrLowestPrice = (pMd->LowestPrice >= 0 && pMd->LowestPrice <= pMd->UpperLimitPrice) ? QString::number(pMd->LowestPrice, 'f', deciNum) : "-";
		strItem = "幅度    " + QString::number(deltaPercent, 'f', 2).toStdString() + "  最低    " + qstrLowestPrice.toStdString();
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		strItem = "总手    " + to_string(pMd->Volume) + "  均价    " + QString::number(pMd->AveragePrice, 'f', deciNum).toStdString();
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		strItem = "持仓    " + QString::number(pMd->OpenInterest, 'f', 0).toStdString() + "  昨结    " + QString::number(pMd->PreSettlementPrice, 'f', deciNum).toStdString();
        listWidget->addItem(strItem.c_str());

		strItem.clear();
		strItem = "涨停    " + QString::number(pMd->UpperLimitPrice, 'f', deciNum).toStdString() + "  跌停    " + QString::number(pMd->LowerLimitPrice, 'f', deciNum).toStdString();
        listWidget->addItem(strItem.c_str());
	}
}

void PlaceOrderForm::timerEvent(QTimerEvent *e) 
{
    Q_UNUSED(e);
	DisplayMaketData();
}

void PlaceOrderForm::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
	//emit mvsigSendData("取消报单", 0);	
}

