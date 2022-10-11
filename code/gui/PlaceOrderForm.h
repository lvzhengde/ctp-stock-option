#ifndef _PLACEORDERFORM_H
#define _PLACEORDERFORM_H

#include "ui_PlaceOrder.h"
#include "RealApp.h"

class MuxCntWindowForm;

struct COrderInfo
{
	char InstrumentType = 0;
	size_t UnderlyingAssetType;
	CThostFtdcInstrumentField *pInstInfo = NULL;
	CThostFtdcDepthMarketDataField *pMd = NULL;
	CRealTraderSpi*  pTraderUserSpi = NULL;
	CCtpCntIf* pCtpCntIf = NULL;
	char Direction = 0;
	char OffsetFlag = 0;
	double Price = 0;
	int Volume = 0;
	bool bFOK = false;
};

class PlaceOrderForm : public QWidget, public Ui::Form
{
    Q_OBJECT

public:
    explicit PlaceOrderForm(CRealApp *pRealApp, MuxCntWindowForm* pMainWindow, QWidget *parent = nullptr);
	virtual ~PlaceOrderForm();
	void InitForm();
	void DisplayMaketData();
	int PlaceOrder();
	bool IsShfeTodayPosition();

protected:
    void timerEvent(QTimerEvent *e);
	void closeEvent(QCloseEvent* event);

public:
	CRealApp *m_pRealApp;
	MuxCntWindowForm* m_pMainWindow;
    COrderInfo m_OrderInfo;	
	bool bIsOpened;

signals:
	void mvsigSendData(QString sData, int i);

private slots:
	void on_lineEdit_textChanged(const QString &text);
	void on_btnSendData_clicked();

};

inline int GetDecimals(double x)
{
    int decimals = 0;

	string strInput = to_string(x);
	int decimalPos =  strInput.find(".");

	if(decimalPos != string::npos)
	{
        for(int i = decimalPos+1; i < strInput.size(); i++)
	    {
	    	decimals++;
	        if(strInput[i] != '0') 
	    		break;
	    }
	}

	return decimals;
}

#endif
