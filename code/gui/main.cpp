#include <QApplication>
#include "MuxCntWindowForm.h"
#include "common.h"
#include "TestApp.h"
#include "RealApp.h"
#include "DbContractMng.h"
#include "OrderMonitor.h"
#include "TradeDb.h" 

//++
//检查配置目录和输出目录是否存在，不存在则创建
//--
void CheckAndCreateDir()
{
	//检查output目录是否存在，不存在则创建
	if(access("output", 0) != 0)
		system("mkdir output");

	//检查config目录是否存在，不存在则创建
	if(access("config", 0) != 0)
		system("mkdir config");
}

int main(int argc, char *argv[])
{
	//检查相关目录是否存在，不存在则创建
	CheckAndCreateDir();
    signal(SIGINT , HandleExitGui);           //按下CTRL + C键
    signal(SIGHUP , HandleExitGui);           //关闭控制台窗口
    signal(SIGQUIT, HandleExitGui);    
    signal(SIGTERM, HandleExitGui);    
    signal(SIGKILL, HandleExitGui);

    cerr << "输入交易架构名称> " ; 
	string strArchName;
    cin >> strArchName;
	CRealApp myRealApp(strArchName);
	myRealApp.InitializeApp();

	//订阅所有柜台欲交易合约的市场行情
	myRealApp.m_pMarketDataManage->SubscribeAllMarketData(0);
    SleepMs(1000);

	//启动行情处理线程
    pthread_t TidMarketData;
	int ret = pthread_create(&TidMarketData, NULL, CRealApp::MarketDataProc, &myRealApp);
	if(ret == 0) cerr << myRealApp.m_Prompt << "行情处理线程已启动..." << ENDLINE;
	
    QApplication app(argc, argv);

    MuxCntWindowForm mainwindow(&myRealApp);
    mainwindow.show();

	ret = app.exec();

	pthread_join(TidMarketData, NULL);

    return ret;
}

