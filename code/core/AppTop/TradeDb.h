/*++
//实现交易数据库相关功能
--*/
#ifndef _TRADE_DB_H
#define _TRADE_DB_H

#include "common.h"
#include <sqlite3.h>

using namespace std;

class CTradeDb
{
public:  //member variables
	//数据库目录路径，Windows中注意分隔符"\"用"\\"代替
	string m_strDirPath;     
	//数据库名，不能带路径分隔符
	string m_strDbName;    
	//数据库是否已经存在的标志
	bool m_bDbExists;

	//指向sqlite3数据库的链接对象的指针
	sqlite3 *m_pDB;

public:  //member functions
#ifdef WIN_CTP
	CTradeDb(string strDirPath = "..\\TradeDatabase", string strDbName = "TradeDb.db");
#else
	CTradeDb(string strDirPath = "../TradeDatabase", string strDbName = "TradeDb.db");
#endif

	~CTradeDb();

	//初始化数据库对象
	void Init();

	//销毁数据库对象
	void Destroy();

	//判断数据库是否存在，不存在则创建
	bool CreateSqliteDb(); 

	//使用Command对象执行SQL命令
	bool ExecuteSqlCommand(string strSqlCmd);

	//判断数据表是否存在，不存在则创建
	int CreateTableIfNotExist(string strTableName, string strFieldDesc);

	//判断数据表是否存在
	bool IsTableExist(string strTableName);

	//寻找FieldName对应的FieldIndex
	int  FieldIndex(const string& sFieldName, vector<string>& sFieldNameList);
};

#endif  
