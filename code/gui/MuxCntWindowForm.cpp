#include "MuxCntWindowForm.h"
#include "PlaceOrderForm.h"
#include <QMessageBox>

MuxCntWindowForm::MuxCntWindowForm(CRealApp *pRealApp, QWidget *parent)
    : m_pRealApp(pRealApp), QMainWindow(parent)
{
    setupUi(this);

	InitLabelTag();
	InitTradeOperation();

	vector<CTshapeBase*>* ppTshapeBaseList = &m_pRealApp->m_pMarketDataManage->m_pTshapeBaseList;
	for(size_t i = 0; i < ppTshapeBaseList->size(); i++)
	{
		CTshapeBase* pTshapeBase = (*ppTshapeBaseList)[i];
	    string strContract = pTshapeBase->m_ExchangeId + (" " + pTshapeBase->m_UnderlyingId);
		comboContract->addItem(strContract.c_str());
	}
	m_pCurTshapeBase = (*ppTshapeBaseList)[0];

	comboDate->clear(); //函数on_comboContract_currentIndexChanged可能在此之前运行一次
	for(size_t i = 0; i < m_pCurTshapeBase->m_pTshapeMonthList.size(); i++)
	{
	    CTshapeMonth *pTshapeMonth = m_pCurTshapeBase->m_pTshapeMonthList[i];
		CThostFtdcInstrumentField *pInstInfo = &pTshapeMonth->m_UnderlyingInstInfo;
		if(pTshapeMonth->m_pOptionPairList.size() > 0)
		{
		    pInstInfo = &pTshapeMonth->m_pOptionPairList[0]->m_pCallItem->m_InstInfo;
		}
        string str1 = pInstInfo->ExpireDate;
        string str2 = str1.substr(0, 4) + "/" + str1.substr(4, 2) + "/" + str1.substr(6, 2);
		string strExpireDay = str2 + (" (" + to_string(pTshapeMonth->m_ExpirationLeftDays) + ")");
		comboDate->addItem(strExpireDay.c_str());
	}
	m_pCurTshapeMonth = m_pCurTshapeBase->m_pTshapeMonthList[0];	
	m_pPreTshapeMonth = NULL;
	UpdateMaketDataDisplay();

	//持仓明细
    tablePosition->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); //第三列根据内容自适应
    tablePendingOrder->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); //第五列根据内容自适应
    tableOrder->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); //第五列根据内容自适应
    tableTraded->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); //第五列根据内容自适应
	if(tabWidget->currentIndex() == 0)
	{
	    UpdateInvestorPositionDisplay();
		UpdatePendingOrderDisplay();
	}
	else if(tabWidget->currentIndex() == 1)
	{
	    UpdateOrderDisplay();
	}
	else if(tabWidget->currentIndex() == 2)
	{
		UpdateTradedDisplay();
	}

	//状态栏显示
    QTime qtime = QTime::currentTime();
    QString stime = qtime.toString();
	m_plabelStatus = new QLabel;
	m_plabelStatus->setText("时间 "+ stime);
	statusBar()->addWidget(m_plabelStatus);
	
    m_pPlaceOrderForm	= new PlaceOrderForm(pRealApp, this);
	connect(m_pPlaceOrderForm, SIGNAL(mvsigSendData(QString, int)), this, SLOT(mvslotReceiveData(QString, int)));

    startTimer(500);	
}

MuxCntWindowForm::~MuxCntWindowForm()
{
    delete m_plabelStatus;
	delete m_pPlaceOrderForm;

    DeleteCriticalSection(&m_PositionCS);
    DeleteCriticalSection(&m_OrderCS);
	
    delete m_pMenuTshape;
    delete m_pActionOrder;

    delete m_pMenuPosition;
    delete m_pActionClose;

	delete m_pMenuPendingOrder;
	delete m_pActionCancel;
}

void MuxCntWindowForm::InitLabelTag()
{
	this->setWindowTitle("股票期权图形化交易");
	menuFile->setTitle("操作");
	actionPlaceOrder->setText("下单");
	actionCancelAllOrder->setText("全部撤单");
	actionExit->setText("退出");

    labelContract->setText("合约");
	labelUnderlying->setText("标的");

    tableTshape->setColumnCount(17);
    tableTshape->setRowCount(0);
	QStringList columnHeader = {"交易状态", "持仓量", "总量", "卖价", "买价", "涨跌", "幅度%", "最新", "C<行权价>P",
	   "最新", "幅度%", "涨跌", "买价", "卖价", "总量", "持仓量", "交易状态"};
    tableTshape->setHorizontalHeaderLabels(columnHeader);

	tabWidget->setTabText(0, "交易");
    labelPosition->setText("当前持仓");
	tablePosition->setColumnCount(14);
	tablePosition->setRowCount(0);
	columnHeader.clear();
	columnHeader << "市场名称" << "代码" << "名称" << "类别" << "买卖" << "备兑属性" << "持仓" << "可用" << "开仓均价" << "最新价"
	    << "市值" << "估算浮动盈亏" << "保证金" << "持仓类别";
	tablePosition->setHorizontalHeaderLabels(columnHeader);
	labelOrder->setText("可撤委托");
	tablePendingOrder->setColumnCount(15);
    tablePendingOrder->setRowCount(0);
	columnHeader.clear();
	columnHeader << "委托时间" << "市场名称" << "委托号" << "合约代码" << "合约名称" << "买卖" << "开平" << "委托价格" << "委手" 
		<< "成手" << "成交均价" << "状态信息" << "合约类别" << "期权备兑标志" << "备注";
    tablePendingOrder->setHorizontalHeaderLabels(columnHeader);

	tabWidget->setTabText(1, "委托");
	tableOrder->setColumnCount(15);
    tableOrder->setRowCount(0);
	columnHeader.clear();
	columnHeader << "委托时间" << "市场名称" << "委托号" << "合约代码" << "合约名称" << "买卖" << "开平" << "委托价格" << "委手" 
		<< "成手" << "成交均价" << "状态信息" << "合约类别" << "期权备兑标志" << "备注";
    tableOrder->setHorizontalHeaderLabels(columnHeader);


	tabWidget->setTabText(2, "成交");
	tableTraded->setColumnCount(17);
    tableTraded->setRowCount(0);
	columnHeader.clear();
	columnHeader << "成交时间" << "市场名称" << "合约代码" << "合约名称" << "买卖" << "开平" << "成交价格" << "成交数量" << "委托价格" 
		<< "委托数量" << "成交金额"  << "合约类别" << "备兑" << "委托号" << "委托时间" << "成交号" << "备注";
    tableTraded->setHorizontalHeaderLabels(columnHeader);
}

void MuxCntWindowForm::InitTradeOperation()
{
    InitializeCriticalSection(&m_PositionCS);
    InitializeCriticalSection(&m_OrderCS);

	//初始化contextMenu
	tableTshape->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pMenuTshape = new QMenu(tableTshape);
	m_pActionOrder = new QAction(m_pMenuTshape);
    m_pActionOrder->setText("下单");
	m_pMenuTshape->addAction(m_pActionOrder);
    connect(tableTshape, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(ShowTshapeMenu(QPoint)));
	connect(m_pActionOrder, SIGNAL(triggered(bool)), this, SLOT(ActionPlaceOrder()));

	tablePosition->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pMenuPosition = new QMenu(tablePosition);
	m_pActionClose = new QAction(m_pMenuPosition);
    m_pActionClose->setText("平仓");
	m_pMenuPosition->addAction(m_pActionClose);
    connect(tablePosition, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(ShowPositionMenu(QPoint)));
	connect(m_pActionClose, SIGNAL(triggered(bool)), this, SLOT(ActionClose()));

	tablePendingOrder->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pMenuPendingOrder = new QMenu(tablePendingOrder);
	m_pActionCancel = new QAction(m_pMenuPendingOrder);
    m_pActionCancel->setText("撤单");
	m_pMenuPendingOrder->addAction(m_pActionCancel);
    connect(tablePendingOrder, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(ShowPendingOrderMenu(QPoint)));
	connect(m_pActionCancel, SIGNAL(triggered(bool)), this, SLOT(ActionCancel()));
}

void MuxCntWindowForm::ShowTshapeMenu(QPoint pos)
{
	m_TshapePos = pos;
    m_pMenuTshape->exec(QCursor::pos());
}

void MuxCntWindowForm::ActionPlaceOrder()
{
    QTableWidgetItem *pItem = tableTshape->itemAt(m_TshapePos);
	if(pItem != NULL)
	{
	    int row = pItem->row();
		int col = pItem->column();

		on_tableTshape_cellDoubleClicked(row, col);
	}
}

void MuxCntWindowForm::ShowPositionMenu(QPoint pos)
{
	m_PositionPos = pos;
    m_pMenuPosition->exec(QCursor::pos());
}

void MuxCntWindowForm::ActionClose()
{
    QTableWidgetItem *pItem = tablePosition->itemAt(m_PositionPos);
	if(pItem != NULL)
	{
	    int row = pItem->row();
		int col = pItem->column();

		on_tablePosition_cellDoubleClicked(row, col);
	}
}

void MuxCntWindowForm::ShowPendingOrderMenu(QPoint pos)
{
	m_PendingOrderPos = pos;
    m_pMenuPendingOrder->exec(QCursor::pos());
}

void MuxCntWindowForm::ActionCancel()
{
    QTableWidgetItem *pItem = tablePendingOrder->itemAt(m_PendingOrderPos);
	if(pItem != NULL)
	{
	    int row = pItem->row();
		int col = pItem->column();

		on_tablePendingOrder_cellDoubleClicked(row, col);
	}
}

void MuxCntWindowForm::UpdateMaketDataDisplay()
{
    //显示标的市场行情
	string strInstID = m_pCurTshapeMonth->m_UnderlyingMarketData.InstrumentID;
	double PreSettlementPrice;
	double LastPrice = m_pCurTshapeMonth->m_UnderlyingMarketData.LastPrice;
	double PreClosePrice = m_pCurTshapeMonth->m_UnderlyingMarketData.PreClosePrice;
	double DeltaPrice = LastPrice - PreClosePrice;
	double DeltaPercent = 100.0 * DeltaPrice / PreClosePrice + 0.005;
	string strUnderMD = "标的" + strInstID + "  " + TrimString(to_string(LastPrice), 4) + "  " 
		+ TrimString(to_string(DeltaPrice), 4) + "  " + TrimString(to_string(DeltaPercent), 2) + "%";
	labelUnderlying->setText(strUnderMD.c_str());

    //显示期权T型报价行情	
	if(m_pCurTshapeMonth != m_pPreTshapeMonth)
	{
        tableTshape->setRowCount(m_pCurTshapeMonth->m_pOptionPairList.size());	
	}

    for(size_t i = 0; i < m_pCurTshapeMonth->m_pOptionPairList.size(); i++)
	{
		QTableWidgetItem *pCellItem = NULL;
		COptionPairItem* pOptionPair = m_pCurTshapeMonth->m_pOptionPairList[i];
    	COptionItem* pCallItem = pOptionPair->m_pCallItem;
    	COptionItem* pPutItem = pOptionPair->m_pPutItem;		

	    int deciNum = GetDecimals(pCallItem->m_InstInfo.PriceTick);		
		deciNum  = (deciNum > 5) ? 5 : deciNum;		

	    //行权价	
		pCellItem = tableTshape->item(i, 8);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 8, new QTableWidgetItem(QString::number(pOptionPair->m_StrikePrice, 'f', deciNum)));
		tableTshape->item(i, 8)->setTextAlignment(Qt::AlignHCenter);
		tableTshape->item(i, 8)->setBackground(QColor(200,200,200));

        //Call行情
		pCellItem = tableTshape->item(i, 0);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 0, new QTableWidgetItem(InsrumentStatusConvert(pCallItem->m_InstrumentStatus).c_str()));
		pCellItem = tableTshape->item(i, 1);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 1, new QTableWidgetItem(QString::number(pCallItem->m_DepthMarketData.OpenInterest)));
		pCellItem = tableTshape->item(i, 2);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 2, new QTableWidgetItem(QString::number(pCallItem->m_DepthMarketData.Volume)));
		pCellItem = tableTshape->item(i, 3);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 3, new QTableWidgetItem(QString::number(pCallItem->m_DepthMarketData.AskPrice1, 'f', deciNum)));
		pCellItem = tableTshape->item(i, 4);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 4, new QTableWidgetItem(QString::number(pCallItem->m_DepthMarketData.BidPrice1, 'f', deciNum)));
		LastPrice = pCallItem->m_DepthMarketData.LastPrice;
		PreSettlementPrice = pCallItem->m_DepthMarketData.PreSettlementPrice;
	    DeltaPrice = LastPrice - PreSettlementPrice;
	    DeltaPercent = 100.0 * DeltaPrice / PreSettlementPrice + 0.005;
		pCellItem = tableTshape->item(i, 5);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 5, new QTableWidgetItem(QString::number(DeltaPrice, 'f', deciNum)));
		pCellItem = tableTshape->item(i, 6);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 6, new QTableWidgetItem(QString::number(DeltaPercent, 'f', 2)));
		pCellItem = tableTshape->item(i, 7);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 7, new QTableWidgetItem(QString::number(LastPrice, 'f', deciNum)));

        
		//Put行情
		LastPrice = pPutItem->m_DepthMarketData.LastPrice;
		PreSettlementPrice = pPutItem->m_DepthMarketData.PreSettlementPrice;
	    DeltaPrice = LastPrice - PreSettlementPrice;
	    DeltaPercent = 100.0 * DeltaPrice / PreSettlementPrice + 0.005;
		pCellItem = tableTshape->item(i, 9);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 9, new QTableWidgetItem(QString::number(LastPrice, 'f', deciNum)));
		pCellItem = tableTshape->item(i, 10);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 10, new QTableWidgetItem(QString::number(DeltaPercent, 'f', 2)));
		pCellItem = tableTshape->item(i, 11);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 11, new QTableWidgetItem(QString::number(DeltaPrice, 'f', deciNum)));
		pCellItem = tableTshape->item(i, 12);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 12, new QTableWidgetItem(QString::number(pPutItem->m_DepthMarketData.BidPrice1, 'f', deciNum)));
		pCellItem = tableTshape->item(i, 13);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 13, new QTableWidgetItem(QString::number(pPutItem->m_DepthMarketData.AskPrice1, 'f', deciNum)));
		pCellItem = tableTshape->item(i, 14);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 14, new QTableWidgetItem(QString::number(pPutItem->m_DepthMarketData.Volume)));
		pCellItem = tableTshape->item(i, 15);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 15, new QTableWidgetItem(QString::number(pPutItem->m_DepthMarketData.OpenInterest)));
		pCellItem = tableTshape->item(i, 16);
		if(pCellItem != NULL) delete pCellItem;
        tableTshape->setItem(i, 16, new QTableWidgetItem(InsrumentStatusConvert(pPutItem->m_InstrumentStatus).c_str()));
	}	
}

void MuxCntWindowForm::UpdateInvestorPositionDisplay()
{
	vector<CThostFtdcInvestorPositionField*> activeInvestorPositionList;

	for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
        CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		vector<CInvestorPositionDetail>* pInvestPosiDetailList = &pCtpCntIf->m_InvestorPositionDetailList;
	    vector<CInvestorPositionDetail>::iterator ItInvPos;
	    for(ItInvPos = pInvestPosiDetailList->begin(); ItInvPos < pInvestPosiDetailList->end(); ItInvPos++)
	    {		
	        CThostFtdcInvestorPositionField* pInvestorPosition = &ItInvPos->CtpInvestorPosition;
			if(pInvestorPosition->Position != 0)
                activeInvestorPositionList.push_back(pInvestorPosition);
		}
	}

	tablePosition->clearContents();
    tablePosition->setRowCount(activeInvestorPositionList.size());	
    for(size_t i = 0; i < activeInvestorPositionList.size(); i++)
	{
	    char InstrumentType = 0;
	    size_t UnderlyingAssetType;
		CThostFtdcInstrumentField *pInstInfo = NULL;
		CThostFtdcDepthMarketDataField *pMd = NULL;

	    void* pNode = GetMarketNodePointer(m_pRealApp, activeInvestorPositionList[i]->InstrumentID, InstrumentType, UnderlyingAssetType);
		if(pNode == NULL) continue;

	    if(InstrumentType == 'u')
	    {
	    	CTshapeMonth* pTshapeMonth = (CTshapeMonth*)pNode;
			pInstInfo = &pTshapeMonth->m_UnderlyingInstInfo;
            pMd = &pTshapeMonth->m_UnderlyingMarketData;
	    }
	    else
	    {
	    	COptionItem* pOptionItem = (COptionItem*)pNode;
            pInstInfo = &pOptionItem->m_InstInfo;
            pMd = &pOptionItem->m_DepthMarketData;
	    }
	    string  InstrumentNameGBK, InstrumentName;
		InstrumentNameGBK = pInstInfo->InstrumentName;
		GBKToUTF8(InstrumentNameGBK, InstrumentName);

        tablePosition->setItem(i, 0, new QTableWidgetItem(activeInvestorPositionList[i]->ExchangeID));
        tablePosition->setItem(i, 1, new QTableWidgetItem(activeInvestorPositionList[i]->InstrumentID));

        tablePosition->setItem(i, 2, new QTableWidgetItem(InstrumentName.c_str()));

		string strType = (InstrumentType == 'c') ? "看涨" : (InstrumentType == 'p') ? "看跌" : "标的";
        tablePosition->setItem(i, 3, new QTableWidgetItem(strType.c_str()));

		string strDirection = (activeInvestorPositionList[i]->PosiDirection == THOST_FTDC_PD_Long) ? "买" : "卖";
        tablePosition->setItem(i, 4, new QTableWidgetItem(strDirection.c_str()));

		string strHedgeFlag = (activeInvestorPositionList[i]->HedgeFlag == THOST_FTDC_HF_Covered) ? "备兑" : "非备兑";
        tablePosition->setItem(i, 5, new QTableWidgetItem(strHedgeFlag.c_str()));

        tablePosition->setItem(i, 6, new QTableWidgetItem(QString::number(activeInvestorPositionList[i]->Position)));

		int frozenPosition = (activeInvestorPositionList[i]->PosiDirection == THOST_FTDC_PD_Long) ? activeInvestorPositionList[i]->ShortFrozen 
			: activeInvestorPositionList[i]->LongFrozen;
		int availPosition = activeInvestorPositionList[i]->Position - frozenPosition;
        tablePosition->setItem(i, 7, new QTableWidgetItem(QString::number(availPosition)));

		double avgOpenPrice = (activeInvestorPositionList[i]->PosiDirection == THOST_FTDC_PD_Long) ?
		   	activeInvestorPositionList[i]->OpenCost / (activeInvestorPositionList[i]->Position * pInstInfo->VolumeMultiple)
			: activeInvestorPositionList[i]->PositionCost / (activeInvestorPositionList[i]->Position * pInstInfo->VolumeMultiple);
        tablePosition->setItem(i, 8, new QTableWidgetItem(QString::number(avgOpenPrice, 'f', 4)));

        tablePosition->setItem(i, 9, new QTableWidgetItem(QString::number(pMd->LastPrice, 'f', 4)));

		double tmpValue = pMd->LastPrice * activeInvestorPositionList[i]->Position * pInstInfo->VolumeMultiple;
		double maketValue = (activeInvestorPositionList[i]->PosiDirection == THOST_FTDC_PD_Long) ? tmpValue : -tmpValue;
        tablePosition->setItem(i, 10, new QTableWidgetItem(QString::number(maketValue, 'f', 4)));

        tmpValue = (pMd->LastPrice - avgOpenPrice) * activeInvestorPositionList[i]->Position * pInstInfo->VolumeMultiple;
		double profit = (activeInvestorPositionList[i]->PosiDirection == THOST_FTDC_PD_Long) ? tmpValue : -tmpValue;
        tablePosition->setItem(i, 11, new QTableWidgetItem(QString::number(profit, 'f', 4)));

        tablePosition->setItem(i, 12, new QTableWidgetItem(QString::number(activeInvestorPositionList[i]->UseMargin, 'f', 2)));

		string strPositionType = (activeInvestorPositionList[i]->PosiDirection == THOST_FTDC_PD_Long) ? "权利仓" : "义务仓";
        tablePosition->setItem(i, 13, new QTableWidgetItem(strPositionType.c_str()));
	}

    EnterCriticalSection(&m_PositionCS);
	m_InvestorPositionList.swap(activeInvestorPositionList);
    LeaveCriticalSection(&m_PositionCS);
}

void MuxCntWindowForm::UpdatePendingOrderDisplay()
{
	double eps = 1e-9;
    vector<CThostFtdcOrderField> PendingOrderList;
    vector<CThostFtdcOrderField> tmpOrderList;
    //整理报单列表
    for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
    {
    	CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
    	CRealTraderSpi*  pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;
    	vector<CThostFtdcOrderField> ReducedOrderList;
    	vector<CThostFtdcOrderField> ActiveOrderList;
    
    	//复制TraderSpi中的报单列表
    	tmpOrderList.clear();
    	EnterCriticalSection(&pCtpCntIf->m_OrderListCS);
    	tmpOrderList.assign(pTraderUserSpi->m_orderList.begin(), pTraderUserSpi->m_orderList.end());
    	pCtpCntIf->m_DupOrderList.swap(tmpOrderList);
    	LeaveCriticalSection(&pCtpCntIf->m_OrderListCS);
    
    	//将pCtpCntIf->m_DupOrderList精简, 合并仅状态不同的重复报单
    	tmpOrderList.clear();
    	for(size_t j = 0; j < pCtpCntIf->m_DupOrderList.size(); j++)
    	{
    		CThostFtdcOrderField *pOrder = &pCtpCntIf->m_DupOrderList[j];
    		int BrokerOrderSeq = pOrder->BrokerOrderSeq;
    		int OrderRef = atoi(pOrder->OrderRef);
    		int FrontId = pOrder->FrontID;
    		int SessionId = pOrder->SessionID;
    		bool found = false;
    
    		for(size_t k = 0; k < tmpOrderList.size(); k++)
    		{
    			int tmpOrderRef = atoi(tmpOrderList[k].OrderRef);
    			//确认是同一报单的不同回报
    			if((FrontId == tmpOrderList[k].FrontID && SessionId == tmpOrderList[k].SessionID && OrderRef == tmpOrderRef)
    				|| (strcmp(tmpOrderList[k].ExchangeID, pOrder->ExchangeID) == 0 && strcmp(tmpOrderList[k].OrderSysID, pOrder->OrderSysID) == 0))
    			{
					if(BrokerOrderSeq == tmpOrderList[k].BrokerOrderSeq)
					{
    				    found = true;
    				    memcpy(&tmpOrderList[k], pOrder, sizeof(CThostFtdcOrderField));
    				    break;
					}					
    			}
    		}
    
    		//报单未找到
    		if(found == false)
    		{
    			tmpOrderList.push_back(*pOrder);
    		}
    	} //for j, pCtpCntIf->m_DupOrderList
    	ReducedOrderList.swap(tmpOrderList);
    
    	//找出精简报单列表中所有当前挂单
    	tmpOrderList.clear();
    	for(size_t j = 0; j < ReducedOrderList.size(); j++)
    	{
    		if(ReducedOrderList[j].OrderStatus != THOST_FTDC_OST_AllTraded && ReducedOrderList[j].OrderStatus != THOST_FTDC_OST_Canceled)
    		{
    			tmpOrderList.push_back(ReducedOrderList[j]);
    		}
    	} //for j
    	ActiveOrderList.swap(tmpOrderList);

		PendingOrderList.insert(PendingOrderList.end(), ActiveOrderList.begin(), ActiveOrderList.end());
    } //for i, 各个交易柜台	

    //显示当前未撤报单 
	tablePendingOrder->clearContents();
    tablePendingOrder->setRowCount(PendingOrderList.size());
	size_t i = 0;
	for(size_t j = PendingOrderList.size(); j > 0; j--)
    {
    	CThostFtdcOrderField* pOrder = &PendingOrderList[j-1];

	    char InstrumentType = 0;
	    size_t UnderlyingAssetType;
		CThostFtdcInstrumentField *pInstInfo = NULL;
		CThostFtdcDepthMarketDataField *pMd = NULL;

	    void* pNode = GetMarketNodePointer(m_pRealApp, pOrder->InstrumentID, InstrumentType, UnderlyingAssetType);
		if(pNode == NULL) continue;

	    if(InstrumentType == 'u')
	    {
	    	CTshapeMonth* pTshapeMonth = (CTshapeMonth*)pNode;
			pInstInfo = &pTshapeMonth->m_UnderlyingInstInfo;
            pMd = &pTshapeMonth->m_UnderlyingMarketData;
	    }
	    else
	    {
	    	COptionItem* pOptionItem = (COptionItem*)pNode;
            pInstInfo = &pOptionItem->m_InstInfo;
            pMd = &pOptionItem->m_DepthMarketData;
	    }
	    string  InstrumentNameGBK, InstrumentName;
		InstrumentNameGBK = pInstInfo->InstrumentName;
		GBKToUTF8(InstrumentNameGBK, InstrumentName);


        tablePendingOrder->setItem(i, 0, new QTableWidgetItem(pOrder->InsertTime));
        tablePendingOrder->setItem(i, 1, new QTableWidgetItem(pOrder->ExchangeID));
        tablePendingOrder->setItem(i, 2, new QTableWidgetItem(QString::number(pOrder->BrokerOrderSeq)));
        tablePendingOrder->setItem(i, 3, new QTableWidgetItem(pOrder->InstrumentID));
        tablePendingOrder->setItem(i, 4, new QTableWidgetItem(InstrumentName.c_str()));

		string strDirection = (pOrder->Direction == THOST_FTDC_D_Buy) ? "买" : "卖";
        tablePendingOrder->setItem(i, 5, new QTableWidgetItem(strDirection.c_str()));

		string strOffsetFlag = (pOrder->CombOffsetFlag[0] == '0') ? "开仓" : (pOrder->CombOffsetFlag[0] == '1') ? "平仓" : "平今";
        tablePendingOrder->setItem(i, 6, new QTableWidgetItem(strOffsetFlag.c_str()));

		QString suffix = " ";
		if(pOrder->LimitPrice < eps)
			suffix = suffix + "市价";
		if(pOrder->TimeCondition == THOST_FTDC_TC_IOC)
			suffix = suffix + "FOK";

        tablePendingOrder->setItem(i, 7, new QTableWidgetItem(QString::number(pOrder->LimitPrice, 'f', 4) + suffix));

        tablePendingOrder->setItem(i, 8, new QTableWidgetItem(QString::number(pOrder->VolumeTotalOriginal)));
        tablePendingOrder->setItem(i, 9, new QTableWidgetItem(QString::number(pOrder->VolumeTraded)));
        //tablePendingOrder->setItem(i, 10, new QTableWidgetItem(QString::number(Price))); //成交价格在OrderList中没有
        
		string strOrderStatus = GetOrderStatusInfo(pOrder);
        tablePendingOrder->setItem(i, 11, new QTableWidgetItem(strOrderStatus.c_str())); 

		string strType = (InstrumentType == 'c') ? "看涨" : (InstrumentType == 'p') ? "看跌" : "标的";
        tablePendingOrder->setItem(i, 12, new QTableWidgetItem(strType.c_str())); 

		string strHedgeFlag = (pOrder->CombHedgeFlag[0] == THOST_FTDC_HF_Covered) ? "备兑" : "非备兑";
        tablePendingOrder->setItem(i, 13, new QTableWidgetItem(strHedgeFlag.c_str())); 

		string StatusMsg = StatusMessageCodeConvert(pOrder->StatusMsg);
        tablePendingOrder->setItem(i, 14, new QTableWidgetItem(StatusMsg.c_str())); 

		i++;
    } //for i

    EnterCriticalSection(&m_OrderCS);
	m_PendingOrderList.swap(PendingOrderList);
    LeaveCriticalSection(&m_OrderCS);
}

void MuxCntWindowForm::UpdateOrderDisplay()
{
	double eps = 1e-9;
    vector<CThostFtdcOrderField> AllOrderList;
    vector<CThostFtdcOrderField> tmpOrderList;
    //整理报单列表
    for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
    {
    	CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
    	CRealTraderSpi*  pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;
    	vector<CThostFtdcOrderField> ReducedOrderList;
    
    	//复制TraderSpi中的报单列表
    	tmpOrderList.clear();
    	EnterCriticalSection(&pCtpCntIf->m_OrderListCS);
    	tmpOrderList.assign(pTraderUserSpi->m_orderList.begin(), pTraderUserSpi->m_orderList.end());
    	pCtpCntIf->m_DupOrderList.swap(tmpOrderList);
    	LeaveCriticalSection(&pCtpCntIf->m_OrderListCS);
    
    	//将pCtpCntIf->m_DupOrderList精简, 合并仅状态不同的重复报单
    	tmpOrderList.clear();
    	for(size_t j = 0; j < pCtpCntIf->m_DupOrderList.size(); j++)
    	{
    		CThostFtdcOrderField *pOrder = &pCtpCntIf->m_DupOrderList[j];
    		int BrokerOrderSeq = pOrder->BrokerOrderSeq;
    		int OrderRef = atoi(pOrder->OrderRef);
    		int FrontId = pOrder->FrontID;
    		int SessionId = pOrder->SessionID;
    		bool found = false;
    
    		for(size_t k = 0; k < tmpOrderList.size(); k++)
    		{
    			int tmpOrderRef = atoi(tmpOrderList[k].OrderRef);
    			//确认是同一报单的不同回报
    			if((FrontId == tmpOrderList[k].FrontID && SessionId == tmpOrderList[k].SessionID && OrderRef == tmpOrderRef)
    				|| (strcmp(tmpOrderList[k].ExchangeID, pOrder->ExchangeID) == 0 && strcmp(tmpOrderList[k].OrderSysID, pOrder->OrderSysID) == 0))
    			{
					if(BrokerOrderSeq == tmpOrderList[k].BrokerOrderSeq)
					{
    				    found = true;
    				    memcpy(&tmpOrderList[k], pOrder, sizeof(CThostFtdcOrderField));
    				    break;
					}
    			}
    		}
    
    		//报单未找到
    		if(found == false)
    		{
    			tmpOrderList.push_back(*pOrder);
    		}
    	} //for j, pCtpCntIf->m_DupOrderList
    	ReducedOrderList.swap(tmpOrderList);
    
		AllOrderList.insert(AllOrderList.end(), ReducedOrderList.begin(), ReducedOrderList.end());
    } //for i, 各个交易柜台	

    //显示所有报单 
	tableOrder->clearContents();
    tableOrder->setRowCount(AllOrderList.size());
	size_t i = 0;
	for(size_t j = AllOrderList.size(); j > 0; j--)
    {
    	CThostFtdcOrderField* pOrder = &AllOrderList[j-1];

	    char InstrumentType = 0;
	    size_t UnderlyingAssetType;
		CThostFtdcInstrumentField *pInstInfo = NULL;
		CThostFtdcDepthMarketDataField *pMd = NULL;

	    void* pNode = GetMarketNodePointer(m_pRealApp, pOrder->InstrumentID, InstrumentType, UnderlyingAssetType);
		if(pNode == NULL) continue;

	    if(InstrumentType == 'u')
	    {
	    	CTshapeMonth* pTshapeMonth = (CTshapeMonth*)pNode;
			pInstInfo = &pTshapeMonth->m_UnderlyingInstInfo;
            pMd = &pTshapeMonth->m_UnderlyingMarketData;
	    }
	    else
	    {
	    	COptionItem* pOptionItem = (COptionItem*)pNode;
            pInstInfo = &pOptionItem->m_InstInfo;
            pMd = &pOptionItem->m_DepthMarketData;
	    }
	    string  InstrumentNameGBK, InstrumentName;
		InstrumentNameGBK = pInstInfo->InstrumentName;
		GBKToUTF8(InstrumentNameGBK, InstrumentName);


        tableOrder->setItem(i, 0, new QTableWidgetItem(pOrder->InsertTime));
        tableOrder->setItem(i, 1, new QTableWidgetItem(pOrder->ExchangeID));
        tableOrder->setItem(i, 2, new QTableWidgetItem(QString::number(pOrder->BrokerOrderSeq)));
        tableOrder->setItem(i, 3, new QTableWidgetItem(pOrder->InstrumentID));
        tableOrder->setItem(i, 4, new QTableWidgetItem(InstrumentName.c_str()));

		string strDirection = (pOrder->Direction == THOST_FTDC_D_Buy) ? "买" : "卖";
        tableOrder->setItem(i, 5, new QTableWidgetItem(strDirection.c_str()));

		string strOffsetFlag = (pOrder->CombOffsetFlag[0] == '0') ? "开仓" : (pOrder->CombOffsetFlag[0] == '1') ? "平仓" : "平今";
        tableOrder->setItem(i, 6, new QTableWidgetItem(strOffsetFlag.c_str()));

		QString suffix = " ";
		if(pOrder->LimitPrice < eps)
			suffix = suffix + "市价";
		if(pOrder->TimeCondition == THOST_FTDC_TC_IOC)
			suffix = suffix + "FOK";
        tableOrder->setItem(i, 7, new QTableWidgetItem(QString::number(pOrder->LimitPrice, 'f', 4) + suffix));

        tableOrder->setItem(i, 8, new QTableWidgetItem(QString::number(pOrder->VolumeTotalOriginal)));
        tableOrder->setItem(i, 9, new QTableWidgetItem(QString::number(pOrder->VolumeTraded)));

		double tradePrice = GetTradePrice(pOrder);
		if(tradePrice > 0)
		{
            tableOrder->setItem(i, 10, new QTableWidgetItem(QString::number(tradePrice, 'f', 4))); 
		}
        
		string strOrderStatus = GetOrderStatusInfo(pOrder);
        tableOrder->setItem(i, 11, new QTableWidgetItem(strOrderStatus.c_str())); 

		string strType = (InstrumentType == 'c') ? "看涨" : (InstrumentType == 'p') ? "看跌" : "标的";
        tableOrder->setItem(i, 12, new QTableWidgetItem(strType.c_str())); 

		string strHedgeFlag = (pOrder->CombHedgeFlag[0] == THOST_FTDC_HF_Covered) ? "备兑" : "非备兑";
        tableOrder->setItem(i, 13, new QTableWidgetItem(strHedgeFlag.c_str())); 

		string StatusMsg = StatusMessageCodeConvert(pOrder->StatusMsg);
        tableOrder->setItem(i, 14, new QTableWidgetItem(StatusMsg.c_str())); 

		i++;
    } //for i
}

void MuxCntWindowForm::UpdateTradedDisplay()
{
	double eps = 1e-9;
	vector<CThostFtdcTradeField> AllTradeList;
    for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
    {
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
        AllTradeList.insert(AllTradeList.end(), pCtpCntIf->m_DupTradeList.begin(), pCtpCntIf->m_DupTradeList.end());
	}

	tableTraded->clearContents();
	tableTraded->setRowCount(AllTradeList.size());
	for(size_t i = 0; i < AllTradeList.size(); i++ )
	{
	    CThostFtdcTradeField* pTrade = &AllTradeList[i];

	    char InstrumentType = 0;
	    size_t UnderlyingAssetType;
		CThostFtdcInstrumentField *pInstInfo = NULL;
		CThostFtdcDepthMarketDataField *pMd = NULL;

	    void* pNode = GetMarketNodePointer(m_pRealApp, pTrade->InstrumentID, InstrumentType, UnderlyingAssetType);
		if(pNode == NULL) continue;

	    if(InstrumentType == 'u')
	    {
	    	CTshapeMonth* pTshapeMonth = (CTshapeMonth*)pNode;
			pInstInfo = &pTshapeMonth->m_UnderlyingInstInfo;
            pMd = &pTshapeMonth->m_UnderlyingMarketData;
	    }
	    else
	    {
	    	COptionItem* pOptionItem = (COptionItem*)pNode;
            pInstInfo = &pOptionItem->m_InstInfo;
            pMd = &pOptionItem->m_DepthMarketData;
	    }
	    string  InstrumentNameGBK, InstrumentName;
		InstrumentNameGBK = pInstInfo->InstrumentName;
		GBKToUTF8(InstrumentNameGBK, InstrumentName);

        tableTraded->setItem(i, 0, new QTableWidgetItem(pTrade->TradeTime));
        tableTraded->setItem(i, 1, new QTableWidgetItem(pTrade->ExchangeID));
        tableTraded->setItem(i, 2, new QTableWidgetItem(pTrade->InstrumentID));
        tableTraded->setItem(i, 3, new QTableWidgetItem(InstrumentName.c_str()));

		string strDirection = (pTrade->Direction == THOST_FTDC_D_Buy) ? "买" : "卖";
        tableTraded->setItem(i, 4, new QTableWidgetItem(strDirection.c_str()));

		string strOffsetFlag = (pTrade->OffsetFlag == '0') ? "开仓" : (pTrade->OffsetFlag == '1') ? "平仓" : "平今";
        tableTraded->setItem(i, 5, new QTableWidgetItem(strOffsetFlag.c_str()));

        tableTraded->setItem(i, 6, new QTableWidgetItem(QString::number(pTrade->Price, 'f', 4)));
        tableTraded->setItem(i, 7, new QTableWidgetItem(QString::number(pTrade->Volume)));

		CThostFtdcOrderField order = GetOrder(pTrade);
		QString suffix = " ";
		if(order.LimitPrice < eps)
			suffix = suffix + "市价";
		if(order.TimeCondition == THOST_FTDC_TC_IOC)
			suffix = suffix + "FOK";
        tableTraded->setItem(i, 8, new QTableWidgetItem(QString::number(order.LimitPrice, 'f', 4) + suffix));

        tableTraded->setItem(i, 9, new QTableWidgetItem(QString::number(order.VolumeTotalOriginal)));

		double amount = pTrade->Price * pInstInfo->VolumeMultiple;
        tableTraded->setItem(i, 10, new QTableWidgetItem(QString::number(amount, 'f', 4)));

		string strType = (InstrumentType == 'c') ? "看涨" : (InstrumentType == 'p') ? "看跌" : "标的";
        tableTraded->setItem(i, 11, new QTableWidgetItem(strType.c_str()));

		string strHedgeFlag = (pTrade->HedgeFlag == THOST_FTDC_HF_Covered) ? "备兑" : "非备兑";
        tableTraded->setItem(i, 12, new QTableWidgetItem(strHedgeFlag.c_str()));
        tableTraded->setItem(i, 13, new QTableWidgetItem(QString::number(order.BrokerOrderSeq)));
        tableTraded->setItem(i, 14, new QTableWidgetItem(order.InsertTime));
        tableTraded->setItem(i, 15, new QTableWidgetItem(pTrade->TradeID));

		string strNote = (pTrade->Direction == THOST_FTDC_D_Buy) ? "买入" : "卖出";
        tableTraded->setItem(i, 16, new QTableWidgetItem(strNote.c_str()));
	}

}

CThostFtdcOrderField MuxCntWindowForm::GetOrder(CThostFtdcTradeField* pTrade)
{
    vector<CThostFtdcOrderField> AllOrderList;
    vector<CThostFtdcOrderField> tmpOrderList;
    //整理报单列表
    for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
    {
    	CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
    	CRealTraderSpi*  pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;
    	vector<CThostFtdcOrderField> ReducedOrderList;
    
    	//复制TraderSpi中的报单列表
    	tmpOrderList.clear();
    	EnterCriticalSection(&pCtpCntIf->m_OrderListCS);
    	tmpOrderList.assign(pTraderUserSpi->m_orderList.begin(), pTraderUserSpi->m_orderList.end());
    	pCtpCntIf->m_DupOrderList.swap(tmpOrderList);
    	LeaveCriticalSection(&pCtpCntIf->m_OrderListCS);
    
    	//将pCtpCntIf->m_DupOrderList精简, 合并仅状态不同的重复报单
    	tmpOrderList.clear();
    	for(size_t j = 0; j < pCtpCntIf->m_DupOrderList.size(); j++)
    	{
    		CThostFtdcOrderField *pOrder = &pCtpCntIf->m_DupOrderList[j];
    		int BrokerOrderSeq = pOrder->BrokerOrderSeq;
    		int OrderRef = atoi(pOrder->OrderRef);
    		int FrontId = pOrder->FrontID;
    		int SessionId = pOrder->SessionID;
    		bool found = false;
    
    		for(size_t k = 0; k < tmpOrderList.size(); k++)
    		{
    			int tmpOrderRef = atoi(tmpOrderList[k].OrderRef);
    			//确认是同一报单的不同回报
    			if((FrontId == tmpOrderList[k].FrontID && SessionId == tmpOrderList[k].SessionID && OrderRef == tmpOrderRef)
    				|| (strcmp(tmpOrderList[k].ExchangeID, pOrder->ExchangeID) == 0 && strcmp(tmpOrderList[k].OrderSysID, pOrder->OrderSysID) == 0))
    			{
					if(BrokerOrderSeq == tmpOrderList[k].BrokerOrderSeq)
					{
    				    found = true;
    				    memcpy(&tmpOrderList[k], pOrder, sizeof(CThostFtdcOrderField));
    				    break;
					}					
    			}
    		}
    
    		//报单未找到
    		if(found == false)
    		{
    			tmpOrderList.push_back(*pOrder);
    		}
    	} //for j, pCtpCntIf->m_DupOrderList
    	ReducedOrderList.swap(tmpOrderList);
    
		AllOrderList.insert(AllOrderList.end(), ReducedOrderList.begin(), ReducedOrderList.end());
    } //for i, 各个交易柜台	

    CThostFtdcOrderField order;
	for(size_t i = 0; i < AllOrderList.size(); i++)
	{
        CThostFtdcOrderField *pOrder = &AllOrderList[i];
	    if((strcmp(pOrder->ExchangeID, pTrade->ExchangeID) == 0) && (strcmp(pOrder->OrderSysID, pTrade->OrderSysID) == 0))
		{
		    memcpy(&order, pOrder, sizeof(CThostFtdcOrderField));
		}
	}

	return order;
}

double MuxCntWindowForm::GetTradePrice(CThostFtdcOrderField* pOrder)
{
    double tradePrice = -1;

    for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
    {
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
		for(size_t j = 0; j < pCtpCntIf->m_DupTradeList.size(); j++)
		{
			CThostFtdcTradeField* pTrade = &pCtpCntIf->m_DupTradeList[j];
			//找到报单的成交回报
			if((strcmp(pOrder->ExchangeID, pTrade->ExchangeID) == 0) && (strcmp(pOrder->OrderSysID, pTrade->OrderSysID) == 0))
			{
                tradePrice = pTrade->Price;
			}
		}		
	}

    return tradePrice;
}

std::string MuxCntWindowForm::TrimString(std::string strInput, int PrecisionVal)
{
	std::string trimmedString = strInput.substr(0, strInput.find(".") + PrecisionVal + 1);
	return trimmedString;
}

void MuxCntWindowForm::on_actionPlaceOrder_triggered()
{
	if(!m_pPlaceOrderForm->isHidden())
        m_pPlaceOrderForm->close();

	m_pPlaceOrderForm->lineEdit->setText("");
	m_pPlaceOrderForm->doubleSpinBoxPrice->setValue(0);
	m_pPlaceOrderForm->doubleSpinBoxVolume->setValue(0);
	m_pPlaceOrderForm->labelVolumeMultiple->setText("合约乘数");
	m_pPlaceOrderForm->checkBoxFOK->setChecked(false);
    m_pPlaceOrderForm->show();
}

void MuxCntWindowForm::on_actionCancelAllOrder_triggered()
{
    QMessageBox:: StandardButton result = QMessageBox::information(this, "全部撤单", "是否撤销当前的全部挂单?", QMessageBox::Yes | QMessageBox::No);
    if(result == QMessageBox::Yes)
	{
        vector<CThostFtdcOrderField> tmpOrderList;
        //整理报单列表
        for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
        {
        	CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
        	CRealTraderSpi*  pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;
        	vector<CThostFtdcOrderField> ReducedOrderList;
        
        	//复制TraderSpi中的报单列表
        	tmpOrderList.clear();
        	EnterCriticalSection(&pCtpCntIf->m_OrderListCS);
        	tmpOrderList.assign(pTraderUserSpi->m_orderList.begin(), pTraderUserSpi->m_orderList.end());
        	pCtpCntIf->m_DupOrderList.swap(tmpOrderList);
        	LeaveCriticalSection(&pCtpCntIf->m_OrderListCS);
        
        	//将pCtpCntIf->m_DupOrderList精简, 合并仅状态不同的重复报单
        	tmpOrderList.clear();
        	for(size_t j = 0; j < pCtpCntIf->m_DupOrderList.size(); j++)
        	{
        		CThostFtdcOrderField *pOrder = &pCtpCntIf->m_DupOrderList[j];
        		int BrokerOrderSeq = pOrder->BrokerOrderSeq;
        		int OrderRef = atoi(pOrder->OrderRef);
        		int FrontId = pOrder->FrontID;
        		int SessionId = pOrder->SessionID;
        		bool found = false;
        
        		for(size_t k = 0; k < tmpOrderList.size(); k++)
        		{
        			int tmpOrderRef = atoi(tmpOrderList[k].OrderRef);
        			//确认是同一报单的不同回报
        			if((FrontId == tmpOrderList[k].FrontID && SessionId == tmpOrderList[k].SessionID && OrderRef == tmpOrderRef)
        				|| (strcmp(tmpOrderList[k].ExchangeID, pOrder->ExchangeID) == 0 && strcmp(tmpOrderList[k].OrderSysID, pOrder->OrderSysID) == 0))
        			{
					    if(BrokerOrderSeq == tmpOrderList[k].BrokerOrderSeq)
					    {
    				        found = true;
    				        memcpy(&tmpOrderList[k], pOrder, sizeof(CThostFtdcOrderField));
    				        break;
					    }						
        			}
        		}
        
        		//报单未找到
        		if(found == false)
        		{
        			tmpOrderList.push_back(*pOrder);
        		}
        	} //for j, pCtpCntIf->m_DupOrderList
        	ReducedOrderList.swap(tmpOrderList);
        
        	//找出精简报单列表中所有当前挂单
        	tmpOrderList.clear();
        	for(size_t j = 0; j < ReducedOrderList.size(); j++)
        	{
        		if(ReducedOrderList[j].OrderStatus != THOST_FTDC_OST_AllTraded && ReducedOrderList[j].OrderStatus != THOST_FTDC_OST_Canceled)
        		{
	    	        CThostFtdcOrderField* pOrder = &ReducedOrderList[j];
	    	        string strInstID = pOrder->InstrumentID;
	    	        int FrontId = pOrder->FrontID;
	    	        int SessionId = pOrder->SessionID;
	    	        int OrderRef = atoi(pOrder->OrderRef);

	    	        int ret = pTraderUserSpi->ReqOrderAction(pOrder, FrontId, SessionId, OrderRef, strInstID);
	    	        cerr << " 请求 | 发送撤单..." <<((ret == 0)?"成功! ":"失败! ") << " 合约: " << strInstID << ENDLINE;
        		}
        	} //for j
        } //for i, 各个交易柜台	
	}
}

void MuxCntWindowForm::on_actionExit_triggered()
{
    HandleExitGui(SIGINT);
	qApp->exit(0);
}

void MuxCntWindowForm::timerEvent(QTimerEvent *e) 
{
    Q_UNUSED(e);

    //m_pRealApp->SyncDepthMarketData(); //should used in a independent thread, otherwise it will block the GUI

    m_pRealApp->m_pPosMoneyManage->RefreshOrderAndTradeList();
    m_pRealApp->m_pPosMoneyManage->RefreshTradeAccountInfo();

    m_pPreTshapeMonth = m_pCurTshapeMonth;
    UpdateMaketDataDisplay();

	if(tabWidget->currentIndex() == 0)
	{
	    UpdateInvestorPositionDisplay();
		UpdatePendingOrderDisplay();
	}
	else if(tabWidget->currentIndex() == 1)
	{
	    UpdateOrderDisplay();
	}
	else if(tabWidget->currentIndex() == 2)
	{
		UpdateTradedDisplay();
	}

    //资金状况显示在状态栏
	static auto cnt = 0;
	string strTradingAccount;
    for(size_t i = 0; i < m_pRealApp->m_pCtpCntIfList.size(); i++)
	{
		CCtpCntIf* pCtpCntIf = m_pRealApp->m_pCtpCntIfList[i];
	    if(i == cnt)
		{
		    strTradingAccount = pCtpCntIf->m_CounterName + "  ";
            strTradingAccount = strTradingAccount + "权益: " + to_string(pCtpCntIf->m_TradingAccount.Balance) + "   ";
            strTradingAccount = strTradingAccount + "可用: " + to_string(pCtpCntIf->m_TradingAccount.Available) + "   ";
            strTradingAccount = strTradingAccount + "保证金: " + to_string(+ pCtpCntIf->m_TradingAccount.CurrMargin) + "   ";
            strTradingAccount = strTradingAccount + "平仓盈亏: " + to_string(+ pCtpCntIf->m_TradingAccount.CloseProfit) + "   ";
            strTradingAccount = strTradingAccount + "持仓盈亏: " + to_string(+ pCtpCntIf->m_TradingAccount.PositionProfit) + "   ";
            strTradingAccount = strTradingAccount + "手续费: " + to_string(+ pCtpCntIf->m_TradingAccount.Commission) + "   ";
		}
	}
	cnt++;
	if(cnt == m_pRealApp->m_pCtpCntIfList.size()) cnt = 0;

    QTime qtime = QTime::currentTime();
    QString stime = qtime.toString();
	stime = "   时间: "+ stime;

    m_plabelStatus->setText(strTradingAccount.c_str()+ stime);
}

void MuxCntWindowForm::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    HandleExitGui(SIGINT);
	qApp->exit(0);
}

void MuxCntWindowForm::mvslotReceiveData(QString sData, int i)
{
	cerr << sData.toStdString() << ", 返回值 = " << i << ENDLINE;
}

void MuxCntWindowForm::on_tableTshape_cellDoubleClicked(int row, int col)
{
	COptionPairItem* pOptionPair = m_pCurTshapeMonth->m_pOptionPairList[row];
    COptionItem* pCallItem = pOptionPair->m_pCallItem;
    COptionItem* pPutItem = pOptionPair->m_pPutItem;	
    CThostFtdcDepthMarketDataField *pMd = NULL;	

	if(col < 8)
        pMd = &pCallItem->m_DepthMarketData;
	else if(col > 8)
        pMd = &pPutItem->m_DepthMarketData;

	if(pMd != NULL)
	{
	    if(!m_pPlaceOrderForm->isHidden())
            m_pPlaceOrderForm->close();

	    m_pPlaceOrderForm->lineEdit->setText(pMd->InstrumentID);
	    m_pPlaceOrderForm->doubleSpinBoxPrice->setValue(pMd->LastPrice);
	    m_pPlaceOrderForm->doubleSpinBoxVolume->setValue(1);
        m_pPlaceOrderForm->radioButtonOpen->setChecked(true);
        m_pPlaceOrderForm->radioButtonBuy->setChecked(true);
	    m_pPlaceOrderForm->checkBoxFOK->setChecked(false);

        m_pPlaceOrderForm->show();
	}
}

void MuxCntWindowForm::on_tablePosition_cellDoubleClicked(int row, int col)
{
	CThostFtdcInvestorPositionField position;
	CThostFtdcInvestorPositionField* pInvestorPosition = NULL;
	char InstrumentType = 0;
	size_t UnderlyingAssetType;
	CThostFtdcInstrumentField *pInstInfo = NULL;
	CThostFtdcDepthMarketDataField *pMd = NULL;

    EnterCriticalSection(&m_PositionCS);
    if(m_InvestorPositionList.size() > 0 && row < m_InvestorPositionList.size())
	{
		memcpy(&position, m_InvestorPositionList[row], sizeof(CThostFtdcInvestorPositionField));
	    pInvestorPosition = &position;

	    void* pNode = GetMarketNodePointer(m_pRealApp, pInvestorPosition->InstrumentID, InstrumentType, UnderlyingAssetType);
	    if(pNode != NULL) 
	    {
	    	char cntCode = 0;

        	if(InstrumentType == 'u')
        	{
        		CTshapeMonth* pTshapeMonth = (CTshapeMonth*)pNode;
        		pInstInfo = &pTshapeMonth->m_UnderlyingInstInfo;
                pMd = &pTshapeMonth->m_UnderlyingMarketData;
        	}
        	else
        	{
        		COptionItem* pOptionItem = (COptionItem*)pNode;
                pInstInfo = &pOptionItem->m_InstInfo;
                pMd = &pOptionItem->m_DepthMarketData;
        	}
		}
	}
    LeaveCriticalSection(&m_PositionCS);

    if(pInvestorPosition != NULL)
	{
	    if(!m_pPlaceOrderForm->isHidden())
            m_pPlaceOrderForm->close();

	    m_pPlaceOrderForm->lineEdit->setText(pInvestorPosition->InstrumentID);
	    m_pPlaceOrderForm->doubleSpinBoxPrice->setValue(pMd->LastPrice);
	    m_pPlaceOrderForm->doubleSpinBoxVolume->setValue(pInvestorPosition->Position);
        m_pPlaceOrderForm->radioButtonClose->setChecked(true);

		char InvestorDirection = pInvestorPosition->PosiDirection;
		if(InvestorDirection == THOST_FTDC_PD_Long)
            m_pPlaceOrderForm->radioButtonSell->setChecked(true);
		else
            m_pPlaceOrderForm->radioButtonBuy->setChecked(true);

	    m_pPlaceOrderForm->checkBoxFOK->setChecked(false);

        m_pPlaceOrderForm->show();
	}
}

void MuxCntWindowForm::on_tablePendingOrder_cellDoubleClicked(int row, int col)
{
	CThostFtdcOrderField order;
	CThostFtdcOrderField* pOrder = NULL;
    string strInstID;
	int FrontId = 0;
	int SessionId = 0;
	int OrderRef = 0;

	char InstrumentType = 0;
	size_t UnderlyingAssetType;
	CCtpCntIf* pCtpCntIf = NULL;	
	CRealTraderSpi*  pTraderUserSpi = NULL;

    EnterCriticalSection(&m_OrderCS);
	if(m_PendingOrderList.size() > 0 && row < m_PendingOrderList.size())
	{
		int orderPos = m_PendingOrderList.size() - row - 1;
		memcpy(&order, &m_PendingOrderList[orderPos], sizeof(CThostFtdcOrderField));
	    pOrder = &order;

	    void* pNode = GetMarketNodePointer(m_pRealApp, pOrder->InstrumentID, InstrumentType, UnderlyingAssetType);
	    if(pNode != NULL) 
	    {
	    	char cntCode = 0;
        	if(InstrumentType == 'u')
        	{
        		CTshapeMonth* pTshapeMonth = (CTshapeMonth*)pNode;
	    		cntCode = pTshapeMonth->m_pParent->m_UnderCntCode;
        	}
        	else
        	{
        		COptionItem* pOptionItem = (COptionItem*)pNode;
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
            pTraderUserSpi = pCtpCntIf->m_pTraderUserSpi;
	    }

	    strInstID = pOrder->InstrumentID;
	    FrontId = pOrder->FrontID;
	    SessionId = pOrder->SessionID;
	    OrderRef = atoi(pOrder->OrderRef);
	}
    LeaveCriticalSection(&m_OrderCS);

	if(pOrder != NULL)
	{
        QMessageBox:: StandardButton result = QMessageBox::information(this, "撤单", "是否撤销当前挂单?", QMessageBox::Yes | QMessageBox::No);
        if(result == QMessageBox::Yes)
	    {
	        int ret = pTraderUserSpi->ReqOrderAction(pOrder, FrontId, SessionId, OrderRef, strInstID);
	        cerr << " 请求 | 发送撤单..." <<((ret == 0)?"成功! ":"失败! ") << " 合约: " << strInstID << ENDLINE;
	    }
	}
}

void MuxCntWindowForm::on_comboContract_currentIndexChanged(int index)
{
	vector<CTshapeBase*>* ppTshapeBaseList = &m_pRealApp->m_pMarketDataManage->m_pTshapeBaseList;
	
	if(index >= ppTshapeBaseList->size()) 
		return;

	m_pCurTshapeBase = (*ppTshapeBaseList)[index];

	comboDate->clear();
	for(size_t i = 0; i < m_pCurTshapeBase->m_pTshapeMonthList.size(); i++)
	{
	    CTshapeMonth *pTshapeMonth = m_pCurTshapeBase->m_pTshapeMonthList[i];
		CThostFtdcInstrumentField *pInstInfo = &pTshapeMonth->m_UnderlyingInstInfo;
		if(pTshapeMonth->m_pOptionPairList.size() > 0)
		{
		    pInstInfo = &pTshapeMonth->m_pOptionPairList[0]->m_pCallItem->m_InstInfo;
		}
        string str1 = pInstInfo->ExpireDate;
        string str2 = str1.substr(0, 4) + "/" + str1.substr(4, 2) + "/" + str1.substr(6, 2);
		string strExpireDay = str2 + (" (" + to_string(pTshapeMonth->m_ExpirationLeftDays) + ")");
		comboDate->addItem(strExpireDay.c_str());
	}
	m_pPreTshapeMonth = m_pCurTshapeMonth;
	m_pCurTshapeMonth = m_pCurTshapeBase->m_pTshapeMonthList[0];	

	UpdateMaketDataDisplay();
}

void MuxCntWindowForm::on_comboDate_currentIndexChanged(int index)
{
	if(m_pCurTshapeBase == NULL || index >= m_pCurTshapeBase->m_pTshapeMonthList.size()) 
		return;

	m_pPreTshapeMonth = m_pCurTshapeMonth;
	m_pCurTshapeMonth = m_pCurTshapeBase->m_pTshapeMonthList[index];	
	UpdateMaketDataDisplay();
}

void MuxCntWindowForm::on_tabWidget_currentChanged(int index)
{
	if(index == 0)
	{
	    UpdateInvestorPositionDisplay();
		UpdatePendingOrderDisplay();
	}
	else if(tabWidget->currentIndex() == 1)
	{
	    UpdateOrderDisplay();
	}
	else if(tabWidget->currentIndex() == 2)
	{
		UpdateTradedDisplay();
	}
}

