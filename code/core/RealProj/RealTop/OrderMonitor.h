/*++
    用于当前挂单监控的类
	监控交易架构下的所有交易柜台, 必要时可以一键撤单
--*/


#ifndef _ORDER_MONITOR_H_
#define _ORDER_MONITOR_H_

#include "common.h"
#include "RealApp.h"
#include "TradeDb.h"

class COrderMonitor
{
public:

	//指向主应用对象的指针
	CRealApp* m_pRealApp;

public:
	COrderMonitor(CRealApp* pRealApp);
	~COrderMonitor();

	//报单监控主程序
	void OrderMonitorMain();

	//监控循环程序
	void MonitorLoop();

	//一键撤销所有当前活跃挂单
	void CancelAllActiveOrder(CRealTraderSpi*  pTraderUserSpi, vector<CThostFtdcOrderField> &ActiveOrderList);

};

#endif
