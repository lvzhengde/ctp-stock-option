#include "common.h"
#include "AppConfig.h"
#include "PositionTrade.h"
#include "TradeDb.h"
#include "KeyValue.h"
#include "RealApp.h"

//++
//程序暂停，等待输入任意字符
//--
void PauseForInput(void)
{
#ifdef WIN_CTP
    getch();
#else
	FlushKbInBuf(); //清空键盘输入缓存
	getch();
#endif
}

//++
//按任意键退出程序
//--
void PressAnyKeyToExit(void)
{
#ifdef WIN_CTP
	 system("pause");
#else
	 cerr << "按任意键继续..." << ENDLINE;
	 PauseForInput();
#endif
}

//++
//获取用户密码，输入密码时加密显示，支持出错时删除重输
//参数
//    Passwd: 用于存储密码的字符数组
//--
void GetPasswd(TThostFtdcPasswordType Passwd) {
	unsigned char c;
	int i = 0;
	int MaxPasswdLen;

	MaxPasswdLen = sizeof(TThostFtdcPasswordType); //不能用sizeof(Passwd), 作为参数传递时为size=4的指针
	while ((c=getch()) != '\r') {
		if(i < MaxPasswdLen && isprint(c)) {
			Passwd[i++] = c;
			putchar('*');
		}
		else if(i > 0 && (c == '\b' || c == 127)) {
			--i;
			putchar('\b');
			putchar(' ');
			putchar('\b');
		}
	}
	putchar('\n');
	Passwd[i] = '\0';
}

//++
//移除string前后的空格以及TAB字符
//参数
//    str: 需要移除空格的字符串
//返回值
//    string引用，即字符串本身。需要参数和返回值都是引用
//--
string& RemoveSpace(string& str)
{
	string buff(str); 

	//字符串为空的情况下可能出错
	if(str.size() == 0) return str;

	//去除空格
    char space = ' '; 
	size_t BeginStr =  buff.find_first_not_of(space);
	size_t EndStr = buff.find_last_not_of(space);
	if(BeginStr == string::npos || EndStr == string::npos) //全是空格
	{
		str = "";
		return str;
	}
	str.assign(buff.begin() + buff.find_first_not_of(space), 
	buff.begin() + buff.find_last_not_of(space) + 1); 

	//去除TAB
	char tab = '\t';
	buff = str;
	BeginStr =  buff.find_first_not_of(tab);
	EndStr = buff.find_last_not_of(tab);
	if(BeginStr == string::npos || EndStr == string::npos) //全是tab
	{
		str = "";
		return str;
	}
    str.assign(buff.begin() + buff.find_first_not_of(tab), 
    buff.begin() + buff.find_last_not_of(tab) + 1); 

	return str;
}

//++
//去除字符串前后的空格, 返回string
//参数
//    pChar: 需要移除空格的字符串指针
//返回值
//    string: 去除前后空格后的字符串
//--
string RemoveSpace(char* pChar)
{
	string str = pChar;
	string buff(str); 

	//字符串为空的情况下可能出错
	if(str.size() == 0) return str;

	//去除空格
    char space = ' '; 
	size_t BeginStr =  buff.find_first_not_of(space);
	size_t EndStr = buff.find_last_not_of(space);
	if(BeginStr == string::npos || EndStr == string::npos) //全是空格
	{
		str = "";
		return str;
	}
	str.assign(buff.begin() + buff.find_first_not_of(space), 
	buff.begin() + buff.find_last_not_of(space) + 1); 

	//去除TAB
	char tab = '\t';
	buff = str;
	BeginStr =  buff.find_first_not_of(tab);
	EndStr = buff.find_last_not_of(tab);
	if(BeginStr == string::npos || EndStr == string::npos) //全是tab
	{
		str = "";
		return str;
	}
    str.assign(buff.begin() + buff.find_first_not_of(tab), 
    buff.begin() + buff.find_last_not_of(tab) + 1); 

	return str;
}

//++
//获取当前时间的秒数
//--
double GetCurrentSeconds()
{
	double LocalTimeSeconds;

#if 0
#ifdef WIN_CTP
	SYSTEMTIME CurTime;
	GetLocalTime(&CurTime);
	LocalTimeSeconds = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;
#else
	struct timeval tv;
    gettimeofday(&tv,NULL);
	LocalTimeSeconds = tv.tv_sec + tv.tv_usec*1e-6;
#endif
#endif

	SYSTEMTIME CurTime;
	GetLocalTime(&CurTime);
	LocalTimeSeconds = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;

    return LocalTimeSeconds;
}

//++
//转换交易所返回消息的编码格式
//--
string StatusMessageCodeConvert(char *pStatusMsg, bool bForce2UTF8)
{
	string StatusMsg = pStatusMsg;
	string StatusMsgConverted;
#ifdef WIN_CTP
	if(bForce2UTF8 == true)
	    GBKToUTF8(StatusMsg, StatusMsgConverted);
	else
        StatusMsgConverted = StatusMsg;
#else
	GBKToUTF8(StatusMsg, StatusMsgConverted);
#endif

	return StatusMsgConverted;
}

//++
//将EvaluateStrategy函数计算得到的盈利曲线输出文件，可供Matlab或者Excel画图使用
//参数
//    strOutFileName: 输出的目标文件名
//    profitDataList: 计算得到的盈利曲线
//返回值
//    无
//--
void OutputProfitCurve(string strOutFileName, vector<CProfitData> profitDataList)
{
	fstream outFile;  //如果纯粹是输出，当用ofstream, 使用fstream缺省是输入方式
	int   profitListSize;

	profitListSize = profitDataList.size();
	outFile.open(strOutFileName, ios::out);   //用于输出必须指定ios::out属性

	if( ! outFile.is_open()) {
		cerr << "打开策略盈利曲线输出文件出错!" << ENDLINE;
		return;
	}

	for(int i = 0; i < profitListSize; i++) {
		outFile << fixed << setprecision(6) << profitDataList[i].price << "    \t" << profitDataList[i].profit << "    \t"
			<< profitDataList[i].probability << ENDLINE;
	}

	outFile.close();
}

//++
//将查询SQL数据表得到的日期时间格式转化为Windows SYSTEMTIME格式，忽略毫秒部分
//注意: SQL数据库中日期年月日的标志分隔符为'/'，如果只是找'-'可能出错
//参数
//    strDatetime: 查询SQL数据表得到的时间日期字符串
//返回值
//    SYSTEMTIME: 转换得到的系统时间格式的时间
//--
SYSTEMTIME SqlDatetimeToSystemtime(string strDatetime)
{
	SYSTEMTIME CurTime;
	size_t pos;
	string strRemain;
	string strYear, strMonth, strDay;
	string strHour, strMinute, strSecond;

	memset(&CurTime, 0, sizeof(SYSTEMTIME));

	//查找年份
	pos = strDatetime.find('-');
	if(pos == string::npos) pos = strDatetime.find('/'); //分隔符可能是'/'

	if(pos != string::npos)
	{
		strYear = strDatetime.substr(0, pos);
		strRemain = strDatetime.substr(pos+1);
		CurTime.wYear = atoi(strYear.c_str());
	}

	//查找月份
	pos = strRemain.find('-');
	if(pos == string::npos) pos = strRemain.find('/'); //分隔符可能是'/'

	if(pos != string::npos)
	{
		strMonth = strRemain.substr(0, pos);
		strRemain = strRemain.substr(pos+1);
		CurTime.wMonth = atoi(strMonth.c_str());
	}

	//查找日期, 日期和后面的时间用空格隔开
	//将字符串前后的冗余空格去除
	strRemain = RemoveSpace(strRemain);
	pos = strRemain.find(' ');
	if(pos != string::npos)
	{
		strDay = strRemain.substr(0, pos);
		strRemain = strRemain.substr(pos+1);
		CurTime.wDay = atoi(strDay.c_str());
	}

	//查找小时
	pos = strRemain.find(':');
	if(pos != string::npos)
	{
		strHour = strRemain.substr(0, pos);
		strRemain = strRemain.substr(pos+1);
		CurTime.wHour = atoi(strHour.c_str());
	}

	//查找分钟
	pos = strRemain.find(':');
	if(pos != string::npos)
	{
		strMinute = strRemain.substr(0, pos);
		strRemain = strRemain.substr(pos+1);
		CurTime.wMinute = atoi(strMinute.c_str());
	}

	//最后得到秒数
	strSecond = strRemain;
	CurTime.wSecond = atoi(strSecond.c_str());

	return CurTime;
}

//++
//将查询得到的SQL日期转换为CDate日期
//参数
//    strAccessDate: 查询SQL数据表得到的日期字符串
//返回值
//    CDate: 自定义的日期结构
//--
CDate SqlDateToCDate(string strAccessDate)
{
	CDate myDate;
	size_t pos;
	string strRemain;
	string strYear, strMonth;

	memset(&myDate, 0, sizeof(CDate));

	//查找年份
	pos = strAccessDate.find('-');
	if(pos == string::npos) pos = strAccessDate.find('/'); //分隔符可能是'/'

	if(pos != string::npos)
	{
		strYear = strAccessDate.substr(0, pos);
		strRemain = strAccessDate.substr(pos+1);
		myDate.year = atoi(strYear.c_str());
	}

	//查找月份
	pos = strRemain.find('-');
	if(pos == string::npos) pos = strRemain.find('/'); //分隔符可能是'/'

	if(pos != string::npos)
	{
		strMonth = strRemain.substr(0, pos);
		strRemain = strRemain.substr(pos+1);
		myDate.month = atoi(strMonth.c_str());
	}

	//查找日期
	//将字符串前后的冗余空格去除
	strRemain = RemoveSpace(strRemain);
	myDate.day = atoi(strRemain.c_str());

	return myDate;
}

//++
//将SYSTEMTIME转换为SQL数据库DATETIME格式，不包括"#"
//参数
//    系统时间格式的时间
//返回值
//    SQL数据库DATETIME格式的日期时间
//--
string SystemtimeToSqlDatetime(SYSTEMTIME CurTime)
{
	stringstream sstr;
	string strDate;
	string strTime;
	string strDatetime;

	//需要保证日期是合法的格式，否则插入或修改数据库会报错
	//如果日期格式不合法，则将其修改为合法的格式
	if(CurTime.wYear < 1900 || CurTime.wYear > 2300) CurTime.wYear = 1900;
	if(CurTime.wMonth < 1 || CurTime.wMonth > 12) CurTime.wMonth = 1;
	if(CurTime.wDay > 31 || CurTime.wDay < 1) CurTime.wDay = 1;
	if(CurTime.wHour >24 || CurTime.wHour < 0) CurTime.wHour = 0;
	if(CurTime.wMinute > 60 || CurTime.wMinute < 0) CurTime.wMinute = 0;
	if(CurTime.wSecond > 61 || CurTime.wSecond < 0) CurTime.wSecond = 0;  //60和61是闰秒

	sstr.clear(); sstr.str(""); //sstr.precision(18);
	sstr << setw(4) << setfill('0') << CurTime.wYear << "-" << setw(2) << setfill('0') << CurTime.wMonth << "-" 
	     << setw(2) << setfill('0') << CurTime.wDay;
	strDate = sstr.str();

	sstr.clear(); sstr.str(""); //sstr.precision(18);
	sstr << setw(2) << setfill('0') << CurTime.wHour << ":" << setw(2) << setfill('0') << CurTime.wMinute << ":" 
	     << setw(2) << setfill('0') << CurTime.wSecond;
	strTime = sstr.str();

	strDatetime = strDate + " " + strTime;

	return strDatetime;
}

//++
//将SYSTEMTIME转换为SQL数据库DATE格式(例如"2017-03-10")，不包括"#"
//参数
//    系统时间格式的时间
//返回值
//    SQL数据库DATE格式的日
//--
string SystemtimeToSqlDate(SYSTEMTIME CurTime)
{
	stringstream sstr;
	string strDate;

	//如果是非法的日期，将其转换为合法日期格式
	if(CurTime.wYear < 1900 || CurTime.wYear > 2300) CurTime.wYear = 1900;
	if(CurTime.wMonth < 1 || CurTime.wMonth > 12) CurTime.wMonth = 1;
	if(CurTime.wDay > 31 || CurTime.wDay < 1) CurTime.wDay = 1;

	sstr.clear(); sstr.str(""); //sstr.precision(18);
	sstr << setw(4) << setfill('0') << CurTime.wYear << "-" << setw(2) << setfill('0') << CurTime.wMonth << "-" 
	     << setw(2) << setfill('0') << CurTime.wDay;
	strDate = sstr.str();

	return strDate;
}

//++
//将CTP的买卖方向表示转化为程序自己的买卖方向表示
//参数
//    CtpDirection: CTP的买卖方向表示
//返回值
//    程序自己的买卖方向表示方式, 'b'/'B'为买，'s'/'S'为卖
//--
char CtpDirctionToMyDirection(TThostFtdcPosiDirectionType CtpDirection)
{
	char MyDirection = ' ';

	if(CtpDirection == THOST_FTDC_PD_Long)
		MyDirection = 'b';
	else if(CtpDirection == THOST_FTDC_PD_Short)
		MyDirection = 's';
	else if(CtpDirection == THOST_FTDC_PD_Net) //股票标的持仓方向是 THOST_FTDC_PD_Net
		MyDirection = '/';
	else 
		MyDirection = CtpDirection;

	return MyDirection;
}

//++
//将交易状态代码转变成状态名称
//参数
//    TradeStatusCode: 交易状态代码
//返回值
//    string: 交易状态名称
string TradeStatusCodeToName(char TradeStatusCode)
{
	string TradeStatusName;

	switch(TradeStatusCode)
	{
		case(Status_AllClosed):
			TradeStatusName = "Status_AllClosed";
			break;
		case(Status_AllOpened):
			TradeStatusName = "Status_AllOpened";
			break;
		case(Status_OpenWaitForAck):
			TradeStatusName = "Status_OpenWaitForAck";
			break;
		case(Status_CloseWaitForAck):
			TradeStatusName = "Status_CloseWaitForAck";
			break;
		case(Status_UnInitialized):
			TradeStatusName = "Status_UnInitialized";
			break;
		case(Status_OpenAckTimeout):
			TradeStatusName = "Status_OpenAckTimeout";
			break;
		case(Status_CloseAckTimeout):
			TradeStatusName = "Status_CloseAckTimeout";
			break;
		case(Status_BrokenLegs):
			TradeStatusName = "Status_BrokenLegs";
			break;
		case(Status_FishOpen):
			TradeStatusName = "Status_FishOpen";
			break;
		case(Status_FishClose):
			TradeStatusName = "Status_FishClose";
			break;
		case(Status_FishOpenCancel):
			TradeStatusName = "Status_FishOpenCancel";
			break;
		case(Status_FishCloseCancel):
			TradeStatusName = "Status_FishCloseCancel";
			break;
		case(Status_FishOpenTraded):
			TradeStatusName = "Status_FishOpenTraded";
			break;
		case(Status_FishCloseTraded):
			TradeStatusName = "Status_FishCloseTraded";
			break;
		default:
			TradeStatusName = "Status Undefined";
	}

	return TradeStatusName;
}

//++
//将数据库同步状态代码转变成状态名称
//参数
//    DbSyncStatusCode: 数据库同步状态代码
//返回值
//    string: 数据库同步状态名称
string DbSyncStatusCodeToName(char DbSyncStatusCode)
{
	string DbSyncStatusName;

	switch(DbSyncStatusCode)
	{
		case(SyncDb_Same_Data):
			DbSyncStatusName = "SyncDb_Same_Data";
			break;
		case(SyncDb_New_Data):
			DbSyncStatusName = "SyncDb_New_Data";
			break;
		case(SyncDb_Update_Data):
			DbSyncStatusName = "SyncDb_Update_Data";
			break;
		default:
			DbSyncStatusName = "SyncDb_Undefined";
	}

	return DbSyncStatusName;
}

//++
//显示经纪商账户的相关信息
//凡是实盘账户，都需要给出警示蜂鸣声，并提示是否继续
//参数
//    strCounterName: 经纪商报盘柜台名称
//返回值
//    无
//--
void DisplayBrokerInfo(string strCounterName, string strArchName, CTradeDb* pTradeDb)
{
	char StillGo = 'N';
	string strPrompt = strArchName + "$";
	bool bLocalGenDb = false;

	//数据库对象不存在则重新生成
	if(pTradeDb == NULL)
	{
#ifdef WIN_CTP
		string strDirPath = "..\\TradeDatabase";
#else
		string strDirPath = "../TradeDatabase";
#endif
		string strDbName = "TradeDb.db";
		if(strArchName != "") strDbName = "TradeDb-" + strArchName + ".db";
		pTradeDb = new CTradeDb(strDirPath, strDbName);
		pTradeDb->Init();
		bLocalGenDb = true;
	}

	CKeyValue FrontKeyValue(true);
	string key, value;

	string strFrontTableName = "FrontServer_" + strCounterName;
	int RetRead = FrontKeyValue.ReadConfigFromDb(pTradeDb, strFrontTableName);
#ifdef WIN_CTP
	string strConfigDir = ".\\config";
	if(strArchName != "") strConfigDir = ".\\config\\" + strArchName;
	string strFilePath = strConfigDir + "\\" + strFrontTableName + ".txt";
#else
	string strConfigDir = "./config";
	if(strArchName != "") strConfigDir = "./config/" + strArchName;
	string strFilePath = strConfigDir + "/" + strFrontTableName + ".txt";
#endif
	if(RetRead <= 0) FrontKeyValue.ReadConfigFromTextFile(strFilePath);

	//读取相关信息
	string BrokerDesc = FrontKeyValue.InputStringKeyValue("BrokerDesc");
#ifdef WIN_CTP
			BrokerDesc = UTF_82ASCII(BrokerDesc);
#endif
	int IsTrueTrade = FrontKeyValue.InputIntKeyValue("IsTrueTrade");

	if(IsTrueTrade == 0)
	{
		cerr << "输入 " << BrokerDesc << " 账户信息" << ENDLINE;
	}
	else
	{
		//以1000Hz的频率，蜂鸣0.5秒，给出警示
		Beep(1000,500);
		cerr << "注意：这是 " << BrokerDesc << " 账户，是否继续运行？（Y/N)：" ; cin >> StillGo;
        if(StillGo != 'Y' && StillGo != 'y')
			exit(-1);

		cerr << "输入 " << BrokerDesc << " 账户信息" << ENDLINE;
	}

	//将配置信息更新回数据库
	FrontKeyValue.WriteConfigToDb(pTradeDb, strFrontTableName);

	//如果是在程序内部申请的数据库对象，需要删除
	if(bLocalGenDb == true && pTradeDb != NULL)
	{
		pTradeDb->Destroy();
		delete pTradeDb;
		pTradeDb = NULL;
	}
}

//++
//确定现在是否有合约处于交易时段内
//参数
//    pOptionKindList: 用于交易的期权商品类型列表，包含交易时段信息
//返回值
//    true: 有合约处于交易时段
//    false: 无合约处于交易时段
//--
bool IsTradingTime(vector<COptionKind>* pOptionKindList)
{
	double LocalTimeSeconds;
	SYSTEMTIME CurTime;
	int hours;
	int minutes;
	int seconds;
	double SegEndTimeSeconds;
	double SegBeginTimeSeconds;

	//获取本地当前时间
	GetLocalTime(&CurTime);
	LocalTimeSeconds = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;

	bool bIsTradingTime = false;
	//逐期权商品类型迭代
	vector<COptionKind>::iterator ItOptionKind;
	for(ItOptionKind = pOptionKindList->begin(); ItOptionKind < pOptionKindList->end(); ItOptionKind++)
	{
		//逐时间段迭代
		for(size_t j = 0; j < ItOptionKind->SegTimeList.size(); j++)
		{
			CSegTime* pSegTime = &ItOptionKind->SegTimeList[j];

			//计算交易开始时间以秒计算的数值
			hours = int(pSegTime->SegBeginTime/1e4);
			minutes = int((pSegTime->SegBeginTime/1e2) - hours*1e2);
			seconds = int((pSegTime->SegBeginTime) - hours*1e4 - minutes*1e2);
			SegBeginTimeSeconds = hours*3600 + minutes*60 + seconds;

			//计算交易时段结束时间以秒计算的数值
			hours = int(pSegTime->SegEndTime/1e4);
			minutes = int((pSegTime->SegEndTime/1e2) - hours*1e2);
			seconds = int((pSegTime->SegEndTime) - hours*1e4 - minutes*1e2);
			SegEndTimeSeconds = hours*3600 + minutes*60 + seconds;

			//判断当前时间是否在该交易时段内
			if((LocalTimeSeconds > SegBeginTimeSeconds) && (LocalTimeSeconds < SegEndTimeSeconds))
			{
				bIsTradingTime = true;
				break;
			}
		} //for j
	} //for ItOptionKind

	return bIsTradingTime;
}

//++
//确定期权品种是否处于交易时段内
//参数
//    pOptionKind: 期权品种指针
//返回值
//    true: 处于交易时段
//    false: 不处于交易时段
//--
bool IsTradingTime(COptionKind* pOptionKind)
{
	if(pOptionKind == NULL) return false;

	double LocalTimeSeconds;
	SYSTEMTIME CurTime;
	int hours;
	int minutes;
	int seconds;
	double SegEndTimeSeconds;
	double SegBeginTimeSeconds;

	//获取本地当前时间
	GetLocalTime(&CurTime);
	LocalTimeSeconds = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;

	bool bIsTradingTime = false;
	//逐时间段迭代
	for(size_t j = 0; j < pOptionKind->SegTimeList.size(); j++)
	{
		CSegTime* pSegTime = &pOptionKind->SegTimeList[j];

		//计算交易开始时间以秒计算的数值
		hours = int(pSegTime->SegBeginTime/1e4);
		minutes = int((pSegTime->SegBeginTime/1e2) - hours*1e2);
		seconds = int((pSegTime->SegBeginTime) - hours*1e4 - minutes*1e2);
		SegBeginTimeSeconds = hours*3600 + minutes*60 + seconds;

		//计算交易时段结束时间以秒计算的数值
		hours = int(pSegTime->SegEndTime/1e4);
		minutes = int((pSegTime->SegEndTime/1e2) - hours*1e2);
		seconds = int((pSegTime->SegEndTime) - hours*1e4 - minutes*1e2);
		SegEndTimeSeconds = hours*3600 + minutes*60 + seconds;

		//判断当前时间是否在该交易时段内
		if((LocalTimeSeconds > SegBeginTimeSeconds) && (LocalTimeSeconds < SegEndTimeSeconds))
		{
			bIsTradingTime = true;
			break;
		}
	} //for j

	return bIsTradingTime;
}

//++
//判断合约是否处于允许交易的时段内
//参数
//    pOptionKind: 合约的期货期权商品类型指针，包含交易时段信息
//    pVoidRealApp: 策略交易应用程序对象指针
//    pTradeTimeSecs: 指向当前交易时间秒数的指针, 缺省为空
//返回值
//    true: 合约处于可交易时段
//    false: 合约处于不可交易时段
//--
bool IsAllowedTradeTime(COptionKind* pOptionKind, CRealApp* pRealApp, double* pTradeTimeSecs)
{
	double CurTimeSecs;
	SYSTEMTIME CurTime;
	int hours;
	int minutes;
	int seconds;
	double SegBeginSecs;
	double SegEndSecs;

	if(pTradeTimeSecs == NULL)
	{
		GetLocalTime(&CurTime);
		CurTimeSecs = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;
	}
	else
	{
		CurTimeSecs = *pTradeTimeSecs;
	}

	bool bAllowedTradeTime = false;
	for(size_t i = 0; i < pOptionKind->SegTimeList.size(); i++)
	{
		//计算交易开始时间以秒计算的数值
		hours = int(pOptionKind->SegTimeList[i].SegBeginTime/1e4);
		minutes = int((pOptionKind->SegTimeList[i].SegBeginTime/1e2) - hours*1e2);
		seconds = int((pOptionKind->SegTimeList[i].SegBeginTime) - hours*1e4 - minutes*1e2);
		SegBeginSecs = hours*3600 + minutes*60 + seconds;

		//计算交易时段结束时间以秒计算的数值
		hours = int(pOptionKind->SegTimeList[i].SegEndTime/1e4);
		minutes = int((pOptionKind->SegTimeList[i].SegEndTime/1e2) - hours*1e2);
		seconds = int((pOptionKind->SegTimeList[i].SegEndTime) - hours*1e4 - minutes*1e2);
		SegEndSecs = hours*3600 + minutes*60 + seconds;

		if((CurTimeSecs >  (SegBeginSecs+pRealApp->m_TradeStartWindSec)) && (CurTimeSecs < (SegEndSecs-pRealApp->m_TradeStopWindSec)))
		{
			bAllowedTradeTime = true;
			break;
		}
	} //for i

	return bAllowedTradeTime;
}

//++
//判断string字符串是否是数字
//参数
//    s: 待判断的字符串
//返回值
//    true: s是字符串
//    false: s不是字符串
//--
bool IsNum(string s)  
{  
	stringstream sin(s);  
	double t;  
	char p; 

	//sin>>t表示把sin转换成double的变量（其实对于int和float型的都会接收），
	//如果转换成功，则值为非0，如果转换不成功就返回为0  
	if(!(sin >> t))  
		return false;  

	//解释：此部分用于检测错误输入中，数字加字符串的输入形式（例如：34.f），在上面的的部分（sin>>t）已经接收并转换了输入的数字部分，
	//在stringstream中相应也会把那一部分给清除，如果此时传入字符串是数字加字符串的输入形式，则此部分可以识别并接收字符部分，例如上面所说的，
	//接收的是.f这部分，所以条件成立，返回false;如果剩下的部分不是字符，那么则sin>>p就为0,则进行到下一步else里面 
	if(sin >> p)  
		return false;  
	else  
		return true;  
}  

//++
//将用double表示的年月日信息转化为SYSTEMTIME格式
//参数
//    YearMonthDay: 用double表示的年月日信息如20170509
//返回值
//    SYSTEMTIME格式的日期信息， 时间部分为零
//--
SYSTEMTIME DoubleToSystemtime(double YearMonthDay)
{
	SYSTEMTIME CurTime;
	memset(&CurTime, 0, sizeof(SYSTEMTIME));

	CurTime.wYear = int(YearMonthDay/1e4);
	CurTime.wMonth = int(YearMonthDay/1e2) - int(CurTime.wYear * 1e2);
	CurTime.wDay = int(YearMonthDay) - int(CurTime.wYear * 1e4) - int(CurTime.wMonth * 1e2);

	return CurTime;
}

//++
//将SYSTEMTIME转换为用double表示的年月日信息
//参数
//    CurTime: SYSTEMTIME格式的日期信息,只转换日期部分
//返回值
//    用double表示的年月日信息如20170509
//--

double SystemtimeToDouble(SYSTEMTIME CurTime)
{
	double YearMonthDay = CurTime.wYear * 1e4 + CurTime.wMonth * 1e2 + CurTime.wDay;
	return YearMonthDay;
}

//++
//模拟交易所自动撮合系统撮合成交价
//当买入价大于、等于卖出价时产生自动撮合成交
//注意: 上交所/深交所股票期权撮合成交规则和期货交易所不同, 详情参考其交易规则
//参数
//    BuyPrice：买入价
//    SellPrice: 卖出价
//    LastPrice: 前一成交价
//    strExchangeId: 交易所代码
//    Direction: 报单买卖方向
//    UpperLimitPrice: 涨停价格
//    LowerLimitPrice: 跌停价格
//返回值
//    自动撮合得到的成交价
//--
double PriceMatchMaking(double BuyPrice, double SellPrice, double LastPrice, string strExchangeId, char Direction, double UpperLimitPrice, double LowerLimitPrice)
{
	double TradePrice = 0;
	double eps = 0.000001;

	//合法性判断，买入价必须大于等于卖出价才能自动撮合成交
	if(BuyPrice < SellPrice)
	{
		cerr << "错误：输入的买入价小于卖出价，不能产生自动撮合成交!" << ENDLINE;
		Beep(2000,500);  //以2000Hz的频率，蜂鸣500毫秒
		PressAnyKeyToExit();
	}

	if(strExchangeId == "SSE" || strExchangeId == "SZSE")
	{
		if(Direction == 'b')
		{
			TradePrice = SellPrice;
		}
		else
		{
			TradePrice = BuyPrice;
		}
	}
	else
	{
		//撮合成交价等于买入价，卖出价和前一成交价三者居中的一个价格
		if(BuyPrice >= SellPrice && SellPrice >= LastPrice)
		{
			TradePrice = SellPrice;
		}
		else if(BuyPrice >= LastPrice && LastPrice >= SellPrice)
		{
			TradePrice = LastPrice;
		}
		else if(LastPrice >= BuyPrice && BuyPrice >= SellPrice)
		{
			TradePrice = BuyPrice;
		}
	}

	//如果TradePrice为0或者超过涨停价则应该是没有对手报价的情况，则考虑最坏情况(卖出跌停价，买入涨停价)
	if((TradePrice < eps) || (TradePrice > (UpperLimitPrice+eps)))
	{
		if(Direction == 'b')
			TradePrice = UpperLimitPrice;
		else
			TradePrice = LowerLimitPrice;
	}

	return TradePrice;
}

//++
//控制交易时段开始期间的一小段时间是是否允许交易的（主要是控制集合竞价结束后的价格剧烈变动）
//使用行情数据内置的时间来确定交易是否可以
//参数
//    pMdList：交易组合的行情数据结构
//    pOptionKind：期权品种描述结构
//    TickInterval：行情更新的间隔，以秒为单位，一般取0.5秒
//返回值
//    true: 可以开始交易
//    false: 不能交易
//--
bool BeginTimeTradeAllow(vector<CThostFtdcDepthMarketDataField*> pMdList, COptionKind* pOptionKind, double TickInterval)
{
	//交易时段开始至少两个Tick时间之后才允许交易
	double MinStartTime = 2 * TickInterval;

	int hours;
	int minutes;
	int seconds;
	double segEndTimeSeconds;
	double segBeginTimeSeconds;

	//选择行情列表中最新的行情数据来使用
	double TempMdTime = 0;
	double LatestMdTime = 0;
	for(size_t i = 0; i < pMdList.size(); i++)
	{
		string str1 = pMdList[i]->UpdateTime; //updateTime的格式为10:50:30

		//在仿真模拟测试的时候，可能出现updateTime为空的情况
		int strSize = str1.size();
		if(strSize < 8)
			str1 = str1 + "00000000";

		string str2 = str1;
		string str3 = str1;

		str1 = str1.substr(0, 2);
		str2 = str2.substr(3, 2);
		str3 = str3.substr(6, 2);

		hours = atoi(str1.c_str());
		minutes = atoi(str2.c_str());
		seconds = atoi(str3.c_str());

		TempMdTime = hours*3600 + minutes*60 + seconds + pMdList[i]->UpdateMillisec/1e3;
		if(TempMdTime > LatestMdTime)
			LatestMdTime = TempMdTime;
	}

	//确定最新行情数据是在交易开始时间指定的若干个Tick时间之后得到的
	bool bAllowedTradeTime = false;
	for(size_t i = 0; i < pOptionKind->SegTimeList.size(); i++)
	{
		//计算交易开始时间以秒计算的数值
		hours = int(pOptionKind->SegTimeList[i].SegBeginTime/1e4);
		minutes = int((pOptionKind->SegTimeList[i].SegBeginTime/1e2) - hours*1e2);
		seconds = int((pOptionKind->SegTimeList[i].SegBeginTime) - hours*1e4 - minutes*1e2);
		segBeginTimeSeconds = hours*3600 + minutes*60 + seconds;

		//计算交易时段结束时间以秒计算的数值
		hours = int(pOptionKind->SegTimeList[i].SegEndTime/1e4);
		minutes = int((pOptionKind->SegTimeList[i].SegEndTime/1e2) - hours*1e2);
		seconds = int((pOptionKind->SegTimeList[i].SegEndTime) - hours*1e4 - minutes*1e2);
		segEndTimeSeconds = hours*3600 + minutes*60 + seconds;

		if((LatestMdTime >  (segBeginTimeSeconds+MinStartTime)) && (LatestMdTime < segEndTimeSeconds))
		{
			bAllowedTradeTime = true;
			break;
		}
	} //for i

	return bAllowedTradeTime;
}

//++
//获取两个Tick数据之间的时间差, 一般Tick数据来自于同一合约同一交易日
//参数
//    TickData1: 先到达的Tick数据
//    TickData2: 后到达的Tick数据
//返回值
//    两个Tick数据之间的时间差(TickData2 - TickData1)
//--
double GetTickDataTimeSpan(CThostFtdcDepthMarketDataField TickData1, CThostFtdcDepthMarketDataField TickData2)
{
	//计算早先到达的Tick数据的时间对应的秒数
	int UpdateMillisec =TickData1.UpdateMillisec;
	string str1 = TickData1.UpdateTime; //UpdateTime的格式为10:50:30
	string str2 = str1;
	string str3 = str1;

	str1 = str1.substr(0, 2);
	str2 = str2.substr(3, 2);
	str3 = str3.substr(6, 2);

	int hours = atoi(str1.c_str());
	int minutes = atoi(str2.c_str());
	int seconds = atoi(str3.c_str());

	double BeginTimeSeconds = hours*3600 + minutes*60 + seconds + UpdateMillisec*(1/1e3);

	//计算后到达的Tick数据的时间对应的秒数
	UpdateMillisec =TickData2.UpdateMillisec;
	str1 = TickData2.UpdateTime; 
	str2 = str1;
	str3 = str1;

	str1 = str1.substr(0, 2);
	str2 = str2.substr(3, 2);
	str3 = str3.substr(6, 2);

    hours = atoi(str1.c_str());
	minutes = atoi(str2.c_str());
	seconds = atoi(str3.c_str());

	double EndTimeSeconds = hours*3600 + minutes*60 + seconds + UpdateMillisec*(1/1e3);

	return (EndTimeSeconds - BeginTimeSeconds);
}

//++
//判断是否临近日盘或者夜盘收盘
//参数
//    pOptionKind: 指向期权品种结构的指针
//    JudgeSeconds: 判断是否临近收盘的时间间隔（秒）
//    RemainSeconds: 引用参数，离最近的收盘时段还剩多少秒
//返回值
//    true: 临近收盘还剩不到JudgeSeconds秒
//    false: 还不到收盘时间
//--
bool NearDayOrNightClose(COptionKind* pOptionKind, double JudgeSeconds, double& RemainSeconds)
{
	//根据交易时段信息和当前时间得到TodayAlpha
	double localTimeSeconds;
	SYSTEMTIME CurTime;
	GetLocalTime(&CurTime);
	localTimeSeconds = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;

	double DayLastSegEndTimeSeconds = -1;
	double NightLastSegEndTimeSeconds = -1;
	for(size_t i = 0; i < pOptionKind->SegTimeList.size(); i++)
	{
		int hours;
		int minutes;
		int seconds;
		double segEndTimeSeconds;

		//计算交易时段结束时间以秒计算的数值
		hours = int(pOptionKind->SegTimeList[i].SegEndTime/1e4);
		minutes = int((pOptionKind->SegTimeList[i].SegEndTime/1e2) - hours*1e2);
		seconds = int((pOptionKind->SegTimeList[i].SegEndTime) - hours*1e4 - minutes*1e2);
		segEndTimeSeconds = hours*3600 + minutes*60 + seconds;

		//日盘下午
		if(hours > 12 && hours < 18)
		{
			if(DayLastSegEndTimeSeconds > 0 && DayLastSegEndTimeSeconds < segEndTimeSeconds)
				DayLastSegEndTimeSeconds = segEndTimeSeconds;
			else if(DayLastSegEndTimeSeconds < 0)
				DayLastSegEndTimeSeconds = segEndTimeSeconds;
		}

		//夜盘
		if(hours > 18 && hours < 24)
		{
			if(NightLastSegEndTimeSeconds > 0 && NightLastSegEndTimeSeconds < segEndTimeSeconds)
				NightLastSegEndTimeSeconds = segEndTimeSeconds;
			else if(NightLastSegEndTimeSeconds < 0)
				NightLastSegEndTimeSeconds = segEndTimeSeconds;
		}
	} //for i

	//确定是否临近收盘
    bool bNearClose = false;	
	RemainSeconds = 0;
	if(CurTime.wHour > 8 && CurTime.wHour < 18) //日盘
	{
		RemainSeconds = DayLastSegEndTimeSeconds - localTimeSeconds;

		if((DayLastSegEndTimeSeconds - localTimeSeconds) < JudgeSeconds)
			bNearClose = true;
	}
	else if(CurTime.wHour > 18 && CurTime.wHour < 24) //夜盘
	{
		RemainSeconds = NightLastSegEndTimeSeconds - localTimeSeconds;

		if((NightLastSegEndTimeSeconds - localTimeSeconds) < JudgeSeconds)
		    bNearClose = true;
	}

	return bNearClose;
}

//++
//将值为0的字符转换为'-'字符，否则保持原样
//参数
//    InChar: 输入字符，值可能为0
//返回值
//    如果输入字符值为0，返回'-'字符，否则返回输入字符本身
//--

char ToNoZeroChar(char InChar)
{
	return (InChar == 0) ? '-' : InChar;
}

//++
//判断股票期权合约是否处于熔断状态
//注意：
//    本函数只能判断是否可能处于第一个熔断状态，其后的判断不了（不过这种可能性较低）
//    根据交易所规则，第一次熔断的的参考价格可能是集合竞价产生的开盘价格，也可能是昨结算价
//    很难判断集合竞价是否产生开盘价，因此本函数两种可能性都考虑
//    通常，集合竞价时给出的行情数据第二至第五档都为0，可以据此判断是否处于集合竞价状态，此函数可不用。
//参数
//    pMd: 指向深度市场行情结构的指针
//    pInstInfo: 指向合约信息结构的指针
//返回值
//    true: 当前可能处于熔断状态
//    false: 当前不大可能处于熔断状态
//--
bool IsFusing(CThostFtdcDepthMarketDataField* pMd, CThostFtdcInstrumentField* pInstInfo)
{
	double OpenPrice = pMd->OpenPrice;
	double PreSettlementPrice = pMd->PreSettlementPrice;
	double LastPrice = pMd->LastPrice;
	double PriceTick = pInstInfo->PriceTick;
	double AbsPriceChange1 = abs(OpenPrice - LastPrice);
	double AbsPriceChange2 = abs(PreSettlementPrice - LastPrice);
	double delta = 0.05;
	double LowerRatio = 0.5 - delta;
	double UpperRatio = 0.5 + delta;

	bool bFusing1 = false;
	bool bFusing2 = false;

	//判断是否处于开盘价涨跌50%左右的范围且价格涨跌绝对值达到或者超过最小报价单位5倍
	if(AbsPriceChange1 > (OpenPrice*LowerRatio) && AbsPriceChange1 < (OpenPrice*UpperRatio) && AbsPriceChange1 >= (5*PriceTick))
		bFusing1 = true;

	//判断是否处于昨结算价涨跌50%左右的范围且价格涨跌绝对值达到或者超过最小报价单位5倍
	if(AbsPriceChange2 > (PreSettlementPrice*LowerRatio) && AbsPriceChange2 < (PreSettlementPrice*UpperRatio) && AbsPriceChange2 >= (5*PriceTick))
		bFusing2 = true;

	bool bFusing = (bFusing1 == true || bFusing2 == true);

	return bFusing;
}

//++
//判断组合是否处于异常交易状态
//避免在异常状态时交易
//参数
//    pPositionTradeList: 指向组合头寸列表的指针
//返回值
//    true: 组合有对应合约处于异常状态
//    false: 组合所有合约都处于正常交易状态
//--
bool IsAbnormalState(vector<CPositionTrade*>* pPositionTradeList)
{
	double eps = 0.0000001;
	bool bAbnormal = false;

	//计算组合中FISH单的数目（如果组合中只有一个FISH单，则不用考虑对手价的情况）
	int FishOrderNum = 0;
	for(size_t i = 0; i < pPositionTradeList->size(); i++)
	{
		CPositionTrade *pPosition = (*pPositionTradeList)[i];
		if(pPosition->m_bFishOrder == true) FishOrderNum++;
	}

	//逐个判断组合中的头寸是否处于异常状态，不便交易
	for(size_t i = 0; i < pPositionTradeList->size(); i++)
	{
		CPositionTrade* pPositionTrade = (*pPositionTradeList)[i];
		CRealApp *pRealApp = pPositionTrade->m_pRealApp;
		CPortfolioBase* pPortfolioBase = pPositionTrade->m_pParent;
		COptionKind* pOptionKind = pPortfolioBase->m_pOptionKind;

		CThostFtdcDepthMarketDataField Md;
		TThostFtdcInstrumentStatusType InstrumentStatus;
		CThostFtdcInstrumentField* pInstInfo = NULL;
		if(pPositionTrade->m_InstrumentType != 'u') //股票期权
		{
			COptionItem* pOptionItem = (COptionItem*)pPositionTrade->m_pMarketDataNode;
			Md = pOptionItem->GetLockedMarketData();
			InstrumentStatus = pOptionItem->GetLockedInstrumentStatus();
			pInstInfo = &pOptionItem->m_InstInfo;
		}
		else  //标的
		{
			CTshapeMonth* pTshapMonth = (CTshapeMonth*)pPositionTrade->m_pMarketDataNode;
			Md = pTshapMonth->GetLockedMarketData();
			InstrumentStatus = pTshapMonth->GetLockedInstrumentStatus();
			pInstInfo = &pTshapMonth->m_UnderlyingInstInfo;
		}
		//避免推送行情得到的涨跌停板价格不对
		double UpperLimitPrice = GetUpperLimitPrice(&Md, pPositionTrade->m_InstrumentType);
		double LowerLimitPrice = GetLowerLimitPrice(&Md, pPositionTrade->m_InstrumentType);

		//定义逻辑变量并附初值
		bool bAskAbnormal = false;
		bool bBidAbnormal = false;
		bool bVolatilityInterrupt = false;
		bool bBigPrice12Gap = false;

		bool bIsInOpenState = (pPositionTrade->m_OpenInterests == 0) ? true : false; //根据头寸有无持仓来判断是否处于开仓状态（避免修改函数传入参数)
		bool bTheOnlyFishOrder = (pPositionTrade->m_bFishOrder == true && FishOrderNum == 1) ? true : false; //是否唯一的FISH单

#ifndef FUTURE_OPTION_ONLY  //股票期权的情况
		//在开/平仓决断的时候,已经排除了熔断情况下报单的可能
		//FISH单已经发出，通常不主动撤单, 除非盈利条件不满足或者非熔断条件下和对应1档报价相差过大
		if(bTheOnlyFishOrder == false)
		{
			//只考虑交易的对手方, 对于FISH单则对手报价也不用考虑
			//卖报价方, 考虑一二档报价必须要有
			if(Md.AskVolume1 <= 0 || Md.AskPrice1 < LowerLimitPrice || Md.AskPrice1 > UpperLimitPrice
				|| Md.AskVolume2 <= 0 || Md.AskPrice2 < LowerLimitPrice || Md.AskPrice2 > UpperLimitPrice)
			{
				if((pPositionTrade->m_Direction == 'b' && bIsInOpenState == true) || (pPositionTrade->m_Direction == 's' && bIsInOpenState == false))
				{
					bAskAbnormal = true;
				}
			}
			//买报价方, 也需要考虑一二档报价，当极度虚值时可能没有二档报价
			if((Md.BidVolume1 <= 0 || Md.BidPrice1 < LowerLimitPrice || Md.BidPrice1 > UpperLimitPrice)
				|| ((Md.BidVolume2 <= 0 || Md.BidPrice2 < LowerLimitPrice || Md.BidPrice2 > UpperLimitPrice) && Md.BidPrice1 >= (5*pInstInfo->PriceTick)))
			{
				if((pPositionTrade->m_Direction == 's' && bIsInOpenState == true) || (pPositionTrade->m_Direction == 'b' && bIsInOpenState == false))
				{
					bBidAbnormal = true;
				}
			}

			//判断是否处于熔断状态
			if(InstrumentStatus == THOST_FTDC_IS_VolatilityInterrupt)      //合约处于熔断状态
			{
				bVolatilityInterrupt = true;		
			}
			else if(Md.AskPrice1 < (Md.BidPrice1 + eps)) //买1报价等于卖1报价, 熔断标志之一(防止程序重启时没有得到熔断状态推送)
			{
				bVolatilityInterrupt = true;
			}

			//同样, 下单时需要防止对手方第一档和第二档之间价差过大以致滑点过大
			if((pPositionTrade->m_Direction == 'b' && bIsInOpenState == true) || (pPositionTrade->m_Direction == 's' && bIsInOpenState == false))
			{
				double Ask12PriceGap = abs(Md.AskPrice2 - Md.AskPrice1);
				bBigPrice12Gap = (Ask12PriceGap > (pRealApp->m_BigGapScaleFactor * pInstInfo->PriceTick)) ? true : false;
			}
			else
			{
				double Bid12PriceGap = abs(Md.BidPrice1 - Md.BidPrice2);
				bBigPrice12Gap = (Bid12PriceGap > (pRealApp->m_BigGapScaleFactor * pInstInfo->PriceTick)) ? true : false;
			}
		} //if(bTheOnlyFishOrder == false)

		//总结起来
		if(bAskAbnormal == true || bBidAbnormal == true || bVolatilityInterrupt == true || bBigPrice12Gap == true)
		{
			bAbnormal = true;
			break;
		}
#else //期货期权的情况
		//期货期权只有买1和卖1报价
		//只考虑交易的对手方, 对于FISH单则对手报价也不用考虑
		//卖报价方
		if((Md.AskVolume1 <= 0 || Md.AskPrice1 < LowerLimitPrice || Md.AskPrice1 > UpperLimitPrice) && (bTheOnlyFishOrder == false))
		{
			if((pPositionTrade->m_Direction == 'b' && bIsInOpenState == true) || (pPositionTrade->m_Direction == 's' && bIsInOpenState == false))
			{
				bAskAbnormal = true;
			}
		}
		//买报价方
		if((Md.BidVolume1 <= 0 || Md.BidPrice1 < LowerLimitPrice || Md.BidPrice1 > UpperLimitPrice) && (bTheOnlyFishOrder == false))
		{
		    if((pPositionTrade->m_Direction == 's' && bIsInOpenState == true) || (pPositionTrade->m_Direction == 'b' && bIsInOpenState == false))
			{
				bBidAbnormal = true;
			}
		}
		//判断是否处于熔断状态
		if(Md.AskPrice1 < (Md.BidPrice1 + eps) && (Md.AskPrice1 > eps)) //买1报价等于卖1报价且不为0, 熔断标志之一(防止程序重启时没有得到熔断状态推送)
		{
			bVolatilityInterrupt = true;
		}

		if(bAskAbnormal == true || bBidAbnormal == true || bVolatilityInterrupt == true)
		{
			bAbnormal = true;
			break;
		}

#endif
	} //i PositionTrade

	return bAbnormal;
}

//++
//调试输出到Debugger如DebugView, Unicode字符串格式
//参数
//    strOutputString: 固定参数, 格式字符串（参考printf)
//    ...: 可变参数
//返回值
//    无
//--
void OutputDebugStringEx(const wchar_t *strOutputString, ...)
{  
    va_list vlArgs;  
    va_start(vlArgs, strOutputString);  //获取第一个可变参数首地址
    size_t nLen = _vscwprintf(strOutputString, vlArgs) + 1;  //获取格式化字符串最后输出的字符个数
    wchar_t *strBuffer = new wchar_t[nLen];  
    _vsnwprintf_s(strBuffer, nLen, nLen, strOutputString, vlArgs);  //格式化输出到缓冲区
    va_end(vlArgs);  //清空
    OutputDebugStringW(strBuffer);  
    delete [] strBuffer;  
}

//++
//调试输出到Debugger如DebugView, ANSI字符串格式
//参数
//    strOutputString: 固定参数, 格式字符串（参考printf)
//    ...: 可变参数
//返回值
//    无
//--
void OutputDebugStringEx(const char *strOutputString, ...)
{  
    va_list vlArgs;  
    va_start(vlArgs, strOutputString);  
    size_t nLen = _vscprintf(strOutputString, vlArgs) + 1;  
    char *strBuffer = new char[nLen];  
    _vsnprintf_s(strBuffer, nLen, nLen, strOutputString, vlArgs);  
    va_end(vlArgs);  
    OutputDebugStringA(strBuffer);  
    delete [] strBuffer;  
} 

//++
//线程活动状态指示
//参数
//    PreHbSecs: 引用参数, 线程上次输出HeartBeat的时间秒数
//    Interval: 设定的输出HeartBeat的时间间隔
//    strPrefix: 输出时的提示前缀
//返回值
//    无
//--
void HeartBeatIndicate(double& PreHbSecs, double Interval, string strPrefix)
{
	double CurrentSecs;
	SYSTEMTIME CurTime;

	GetLocalTime(&CurTime);
	CurrentSecs = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond;
	if(CurrentSecs > (PreHbSecs + Interval))
		PreHbSecs = CurrentSecs;
	else
		return;

	string strFormat = strPrefix + " 时间 %d:%d:%d\r\n";
	OutputDebugStringEx(strFormat.c_str(), CurTime.wHour, CurTime.wMinute, CurTime.wSecond);
}

//++
///获取合约的市场节点指针, 直接遍历整个T型报价获取
//需要在订阅行情后一段时间才能获取到新的市场行情
//参数
//    pRealApp: 指向应用的指针
//    InstrumentID: 合约代码
//    InstrumentType: 引用, 合约类型, 'u'标的， 'c'看涨期权, 'p'看跌期权
//    UnderlyingAssetType: 引用, 标的资产类型, 0: 商品期货, 1: 股指期货，2: ETF, 3: 股票
//返回值
//    市场节点指针
//--
void* GetMarketNodePointer(CRealApp* pRealApp, TThostFtdcInstrumentIDType InstrumentID, char &InstrumentType, size_t &UnderlyingAssetType)
{
	void* pNode = NULL;
	CMarketDataManage* pMarketDataManage = pRealApp->m_pMarketDataManage;
	vector<CTshapeBase*>* pTshapeBaseList = &pMarketDataManage->m_pTshapeBaseList;
	bool found = false;

	vector<CTshapeBase*>::iterator ItTshape;
	CTshapeBase* pTshapeBase = NULL;  //初始化为空指针
	for(ItTshape = pTshapeBaseList->begin(); ItTshape < pTshapeBaseList->end(); ItTshape++)
	{
		pTshapeBase = *ItTshape;
		UnderlyingAssetType = pTshapeBase->m_UnderlyingAssetType;

		for(size_t i = 0; i < pTshapeBase->m_pTshapeMonthList.size(); i++)
		{
			if(strcmp(InstrumentID, pTshapeBase->m_pTshapeMonthList[i]->m_UnderlyingInstInfo.InstrumentID) == 0)
			{
				InstrumentType = 'u';
				pNode = pTshapeBase->m_pTshapeMonthList[i];
				found = true;
				break;
			}

			for(size_t j = 0; j < pTshapeBase->m_pTshapeMonthList[i]->m_pOptionPairList.size(); j++)
			{
				COptionItem* pCallItem = pTshapeBase->m_pTshapeMonthList[i]->m_pOptionPairList[j]->m_pCallItem;
				COptionItem* pPutItem = pTshapeBase->m_pTshapeMonthList[i]->m_pOptionPairList[j]->m_pPutItem;

				//看涨期权判断
				if(strcmp(InstrumentID, pCallItem->m_InstInfo.InstrumentID) == 0)
				{
					InstrumentType = 'c';
					pNode = pCallItem;
					found = true;
					break;
				}

				//看跌期权判断
				if(strcmp(InstrumentID, pPutItem->m_InstInfo.InstrumentID) == 0)
				{
					InstrumentType = 'p';
					pNode = pPutItem;
					found = true;
					break;
				}
			}// for j, OptionPair
			if(found == true) break;
		} //for i, TshapeMonth
		if(found == true) break;
	} //for ItTshape

	return pNode;
}

//++
//对报单价格进行过滤，主要针对交易所设置价格保护带等情况
//参数
//    InPrice: 输入价格
//    pOptionKind: 指向交易合约COptionKind的指针
//    InstrumentType: 交易合约类型
//    dir: 买卖方向
//    pMd: 交易合约市场行情指针
//
//返回值
//    过滤后的价格
//--
double PriceFilt(double InPrice, COptionKind* pOptionKind, char InstrumentType, char dir, CThostFtdcDepthMarketDataField* pMd)
{
	double OutPrice = InPrice;
	string ExchangeId = pOptionKind->ExchangeId;

	if(ExchangeId == "CFFEX")
	{
        OutPrice = CffexPriceBand(InPrice, pOptionKind, InstrumentType, dir, pMd);
	}

    return OutPrice;
}

//++
//中金所价格保护带处理, 目前适用于沪深300股指期权仿真交易
//参数
//    InPrice: 输入价格
//    pOptionKind: 指向交易合约COptionKind的指针
//    InstrumentType: 交易合约类型
//    dir: 买卖方向
//    pMd: 交易合约市场行情指针
//
//返回值
//    中金所价格保护带处理后的价格
//--
double CffexPriceBand(double InPrice, COptionKind* pOptionKind, char InstrumentType, char dir, CThostFtdcDepthMarketDataField* pMd)
{
	double OutPrice = InPrice;

	if(pOptionKind->OptionId == "IO" && InstrumentType  != 'u' && pMd->Volume > 0)
	{
	    double BasePrice; 
	    double ProtectBand;
		double UpperBandPrice;
		double LowerBandPrice;
		int temp;
		double q, r;
		double eps = 1e-6;

		//确定基准价
		if(pMd->BidPrice1 > pMd->LastPrice && pMd->BidVolume1 > 0)
			BasePrice = pMd->BidPrice1;
		else if(pMd->AskPrice1 < pMd->LastPrice && pMd->AskVolume1 > 0)
			BasePrice = pMd->AskPrice1;
		else
			BasePrice = pMd->LastPrice;

        //确定保护带
		if(BasePrice < 100)
			ProtectBand = 30;
        else if(BasePrice >= 100 && BasePrice < 250)
			ProtectBand = 75;
        else if(BasePrice >= 250 && BasePrice < 500)
			ProtectBand = 150;
        else if(BasePrice >= 500 && BasePrice < 1000)
			ProtectBand = 300;
		else
			ProtectBand = 600;
		
        temp = int((BasePrice + ProtectBand)/10.0);
		q = (BasePrice + ProtectBand)/10.0;
	    r = q - double(temp);

        //确定上带价
	    if(r > eps)
		    UpperBandPrice = double(temp + 1) * 10.0;
	    else
		    UpperBandPrice = BasePrice + ProtectBand;	

		if(UpperBandPrice > pMd->UpperLimitPrice)
            UpperBandPrice = pMd->UpperLimitPrice;

		//确定下带价
	    LowerBandPrice = double(temp) * 10.0;
		if(LowerBandPrice < pMd->LowerLimitPrice)
            LowerBandPrice = pMd->LowerLimitPrice;

		//限制交易价格
		if(dir = 'b' && InPrice > UpperBandPrice)
			OutPrice = UpperBandPrice;
		else if(dir = 's' && InPrice < LowerBandPrice)
			OutPrice = LowerBandPrice;
	
	}

    return OutPrice;
}

//将InstrumentStatus由代码转化为字符串
string InsrumentStatusConvert(TThostFtdcInstrumentStatusType code)
{
	string status;

	switch(code)
	{
		case(THOST_FTDC_IS_BeforeTrading):
			status = "开盘前";
			break;
		case(THOST_FTDC_IS_NoTrading):
			status = "非交易";
			break;
		case(THOST_FTDC_IS_Continous):
			status = "连续交易";
			break;
		case(THOST_FTDC_IS_AuctionOrdering):
			status = "集合竞价报单";
			break;
		case(THOST_FTDC_IS_AuctionBalance):
			status = "集合竞价价格平衡";
			break;
		case(THOST_FTDC_IS_AuctionMatch):
			status = "集合竞价撮合";
			break;
		case(THOST_FTDC_IS_Closed):
			status = "收盘";
			break;
		case(THOST_FTDC_IS_Auction):
			status = "集合竞价";
			break;
		case(THOST_FTDC_IS_BusinessSuspension):
			status = "休市";
			break;
		case(THOST_FTDC_IS_VolatilityInterrupt):
			status = "波动性中断";
			break;
		case(THOST_FTDC_IS_TemporarySuspension):
			status = "临时停牌";
			break;
		case(THOST_FTDC_IS_AuctionAfterClosed):
			status = "收盘集合竞价";
			break;
		case(THOST_FTDC_IS_ResumableFusing):
			status = "可恢复交易的熔断";
			break;
		case(THOST_FTDC_IS_UnResumableFusing):
			status = "不可恢复交易的熔断";
			break;
		case(THOST_FTDC_IS_TradingAfterClosed):
			status = "盘后交易";
			break;
		default:
			status = "未定义的交易状态";
	}

	return status;
}

//获取报单的状态信息
string GetOrderStatusInfo(CThostFtdcOrderField* pOrder)
{
	string strOrderStatus;
	if(pOrder->OrderStatus == THOST_FTDC_OST_AllTraded)
        strOrderStatus = "全部成交";
	else if(pOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing)
        strOrderStatus = "部分成交还在队列中";
	else if(pOrder->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing)
        strOrderStatus = "部分成交不在队列中";
	else if(pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
        strOrderStatus = "已撤单";
	else if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_InsertSubmitted)
	    strOrderStatus = "已经提交";
	else if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_CancelSubmitted)
        strOrderStatus = "撤单已经提交";
	else if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_ModifySubmitted)
        strOrderStatus = "修改已经提交";
	else if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_Accepted)
        strOrderStatus = "已经接受";
	else if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected)
        strOrderStatus = "报单已经被拒绝";
	else if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_CancelRejected)
        strOrderStatus = "撤单已经被拒绝";
	else if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_ModifyRejected)
        strOrderStatus = "改单已经被拒绝";
	else
        strOrderStatus = "其它状态";

	return strOrderStatus;
}

