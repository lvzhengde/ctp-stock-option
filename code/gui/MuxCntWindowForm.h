#ifndef _MUXCNTWINDOWFORM_H
#define _MUXCNTWINDOWFORM_H

#include "ui_MuxCntWindow.h"
#include <QTime>
#include "RealApp.h"
#include "PlaceOrderForm.h"

class MuxCntWindowForm : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT
	
public:
    QLabel *m_plabelStatus;
	CRealApp *m_pRealApp;
    PlaceOrderForm *m_pPlaceOrderForm;
	CTshapeBase *m_pCurTshapeBase;
	CTshapeMonth *m_pCurTshapeMonth;
	CTshapeMonth *m_pPreTshapeMonth;
	vector<CThostFtdcInvestorPositionField*> m_InvestorPositionList;	
    vector<CThostFtdcOrderField> m_PendingOrderList;
	CRITICAL_SECTION m_PositionCS;
	CRITICAL_SECTION m_OrderCS;

protected:
	QMenu *m_pMenuTshape;
	QAction *m_pActionOrder;
	QPoint m_TshapePos;

	QMenu *m_pMenuPosition;
	QAction *m_pActionClose;
	QPoint m_PositionPos;

	QMenu *m_pMenuPendingOrder;
	QAction *m_pActionCancel;
	QPoint m_PendingOrderPos;

public:
    explicit MuxCntWindowForm(CRealApp *pRealApp, QWidget *parent = nullptr);
	virtual ~MuxCntWindowForm();

protected:
	void InitLabelTag();
	void InitTradeOperation();
	void UpdateMaketDataDisplay();
	void UpdateInvestorPositionDisplay();
	void UpdatePendingOrderDisplay();
	void UpdateOrderDisplay();
	void UpdateTradedDisplay();
	double GetTradePrice(CThostFtdcOrderField* pOrder);
	CThostFtdcOrderField GetOrder(CThostFtdcTradeField* pTrade);
	std::string TrimString(std::string strInput, int PrecisionVal);

protected:
    void timerEvent(QTimerEvent *e);
	void closeEvent(QCloseEvent* event);

private slots:
    void mvslotReceiveData(QString sData, int i); 

    void on_actionCancelAllOrder_triggered();
    void on_actionPlaceOrder_triggered();
    void on_actionExit_triggered();

    void on_tableTshape_cellDoubleClicked(int row, int col);
    void on_tablePosition_cellDoubleClicked(int row, int col);
    void on_tablePendingOrder_cellDoubleClicked(int row, int col);

	void on_comboContract_currentIndexChanged(int index);
	void on_comboDate_currentIndexChanged(int index);

	void on_tabWidget_currentChanged(int index);

	void ShowTshapeMenu(QPoint pos);
	void ActionPlaceOrder();

	void ShowPositionMenu(QPoint pos);
	void ActionClose();

	void ShowPendingOrderMenu(QPoint pos);
	void ActionCancel();
};

#endif
