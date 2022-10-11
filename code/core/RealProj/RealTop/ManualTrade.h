#ifndef _MANUALTRADE_H
#define _MANUALTRADE_H

#include "common.h"
#include "RealApp.h"
#include "RealMdSpi.h"
#include "RealTraderSpi.h"
#include "AppConfig.h"
#include "TshapeCzce.h"

//手动交易和策略计算菜单功能选择
void ShowManualTradeCommand(CRealApp* pRealApp, bool print);

//获取投资组合的交易列表输入
void GetPortfolioInput(string strKeyValueFile, vector<CTradeItem>& TradeItemList, string& strTargetDay, 
					   char& TimeCondition, char& VolumeCondition, char& MarketOrderType, int Intention);

//为投资组合中的合约获取当前市场行情参数
bool GetPortfolioMarketData(CRealApp* pRealApp, vector<CTradeItem> TradeItemList, CThostFtdcDepthMarketDataField* pMarketDataArray, int Intention);

//组合下单
void PlacePortfolioOrder(CRealApp* pRealApp, vector<CTradeItem> TradeItemList, CThostFtdcDepthMarketDataField* pMarketDataArray, 
						 char TimeCondition, char VolumeCondition, char MarketOrderType);

//组合策略手动下单主程序
void PortfolioManualTrade(CRealApp* pRealApp);

//组合策略计算
bool CalculatePortfolio(vector<CTradeItem> TradeItemList, CThostFtdcDepthMarketDataField* pMarketDataArray, string strTargetDay, vector<CProfitData>& profitDataList);

//组合策略计算主程序
void StrategyCalcCommand(CRealApp* pRealApp);

//组合策略计算以及手动下单主程序
void CalculateAndPlaceOrder(CRealApp* pRealApp);

//按照对冲平仓清算方式交易
void LiquidationStyleTrade(CRealApp* pRealApp);

//对冲平仓清算方式下单
void PlaceLiquidationStyleOrder(CRealApp* pRealApp, vector<CTradeItem> TradeItemList, CThostFtdcDepthMarketDataField* pMarketDataArray, 
								char TimeCondition, char VolumeCondition, char MarketOrderType);

#endif
