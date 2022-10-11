/*++
    设置经纪商交易/行情服务器的地址和端口号
	目前支持中信期货，上海中期以及SimNow的实盘及模拟盘交易
--*/


#ifndef _BROKER_SERVER_H_
#define _BROKER_SERVER_H_

#include "common.h"

class CTradeDb;

class CBrokerServer
{
public:
	//指向交易数据库的指针
	CTradeDb* m_pTradeDb;
	//交易架构名称
	string m_ArchName;
	//经纪商报盘柜台名称（如ZXQH_TRUE, ZXQH_SIM...)
	string m_CounterName;
	//行情服务器地址列表
	vector<string> m_MdFrontList;
	//交易服务器地址列表
	vector<string> m_TraderFrontList;

public:
	CBrokerServer(string strCounterName, string strArchName = "", CTradeDb* pTradeDb = NULL);
	~CBrokerServer();

	//从交易数据库获取经纪商交易服务器信息
	void GetBrokerServerFromDb();

	//设置经纪商服务器
	void SetBrokerServer();

	//中信期货模拟盘服务器
	void ZxqhSimServer();

	//上海中期模拟盘服务器
	void ShzqSimServer();

	//南华期货模拟盘服务器
	void NhqhSimServer();

	//南华期货股票期权模拟服务器
	void SoptNhqhSimServer();

	//南华期货证券交易模拟服务器
	void StockNhqhSimServer();

	//SimNow模拟盘服务器
	void SimNowServer();

	//中信期货实盘服务器
	void ZxqhTrueServer();

	//上海中期实盘服务器
	void ShzqTrueServer();

	//南华期货实盘服务器
	void NhqhTrueServer();


};

#endif