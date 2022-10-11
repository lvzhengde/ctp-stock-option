#include "TradeDb.h"


/*++
//带参数的构造函数, 可以指定路径名和数据库名称
--*/
CTradeDb::CTradeDb(string strDirPath, string strDbName)
{
	m_strDirPath = strDirPath;
	m_strDbName = strDbName;
	m_bDbExists = false;
	m_pDB = NULL;
}

/*++
//析构函数
--*/
CTradeDb::~CTradeDb()
{
	Destroy();
}

/*++
//初始化
//    1. 检测是否多线程安全，否则重新配置
//    2. 检查数据库是否存在，如果不存在则创建数据库
--*/
void CTradeDb::Init()
{
	int ret;

    //判断是否启用串行模式
	if (sqlite3_threadsafe() != 1) {
		ret = sqlite3_config(SQLITE_CONFIG_SERIALIZED);//设置为串行模式

		if (ret != SQLITE_OK) {
			cerr << "SQLite3 is not compiled with serialized threading mode!\n" << ENDLINE;
            m_bDbExists = false;
			return;
		}
	}

	//sqlite初始化
	ret = sqlite3_initialize();
	if (ret != SQLITE_OK) {
		cerr << "database_init sqlite_initalize error" << ENDLINE;
        m_bDbExists = false;
		return;;
	}

	//如果数据库不存在则创建数据库
	m_bDbExists = CreateSqliteDb();
}

/*++
//退出前的清理工作，释放申请对象的内存，等等
--*/
void CTradeDb::Destroy()
{

	if(m_pDB != NULL)
	{
      sqlite3_close(m_pDB);
      m_pDB = NULL;
	}
}

/*++
//创建Sqlite数据库
//返回值
//    true: 创建数据库成功或者数据库已经存在
//    false: 创建数据库失败
--*/
bool CTradeDb::CreateSqliteDb()
{
	bool bRetValue = true;
	string strDirPath = m_strDirPath;
#ifdef WIN_CTP
	string strDBFilePath = strDirPath + "\\" +m_strDbName;
#else
	string strDBFilePath = strDirPath + "/" +m_strDbName;
#endif

	//先判断数据库目录是否存在
    string strCommand;
	if(access(strDirPath.c_str(), 0) != 0)
	{
		cerr << "数据库目录: " << strDirPath << " 不存在，创建目录..." << ENDLINE;
		strCommand = "mkdir " + strDirPath;
		system(strCommand.c_str());
	}

	//打开或创建数据库使用的是同一个函数
	int nRet = sqlite3_open(strDBFilePath.c_str(), &m_pDB);
	if(nRet != SQLITE_OK)
	{
		cerr << "打开或在创建数据库失败: " << sqlite3_errmsg(m_pDB);
        bRetValue = false;	
	}

	return bRetValue;
}

//++
//执行SQL命令
//参数
//    strSqlCmd: 需要执行的SQL命令
//返回值
//    true: 命令执行成功
//    false: 命令执行失败
//--
bool CTradeDb::ExecuteSqlCommand(string strSqlCmd)
{
	bool bRetVal;

	bRetVal = true;
	char* errMsg = NULL;
	int nRet = sqlite3_exec(m_pDB, strSqlCmd.c_str(), 0, 0, &errMsg);
	if(nRet != SQLITE_OK)
	{
		cerr << "SQL命令 '" << strSqlCmd << "' 执行失败，错误原因：" << errMsg << ENDLINE;
		sqlite3_free(errMsg);		
		bRetVal = false;
	}

	return bRetVal;
}

//++
//判断数据表是否存在，不存在则创建
//参数
//    strTableName: 数据表名称
//    strFieldDesc: 数据库字段描述
//返回值
//    0: 数据表已经存在
//    1: 数据表不存在，创建成功
//   -1(<0): 数据表不存在，创建失败
//--
int CTradeDb::CreateTableIfNotExist(string strTableName, string strFieldDesc)
{
	int RetVal;
    string strSQLCmd;

	RetVal = 0;
	if(IsTableExist(strTableName) == false)
	{
		cerr << "数据表" << strTableName << "不存在，重新创建" << ENDLINE;
		strSQLCmd = "CREATE TABLE " + strTableName + " " + strFieldDesc;
        char* errMsg = NULL;
		int nRet = sqlite3_exec(m_pDB, strSQLCmd.c_str(), 0, 0, &errMsg);

		if(nRet != SQLITE_OK)
		{
			cerr << "创建 '" << strTableName << "' 数据表失败, 原因: " << errMsg << ENDLINE;
		    sqlite3_free(errMsg);		
			RetVal = -1;
		}
		else
		{
			RetVal = 1;
		}
	}

	return RetVal;
}

//++
//判断数据表是否存在
//参数
//    strTableName: 数据表名称
//返回值
//    true: 数据表存在
//    false: 数据表不存在
//--
bool CTradeDb::IsTableExist(string strTableName)
{
	bool RetVal = true;
    string strSQLCmd = "select * from sqlite_master where type = 'table' and name = '" + strTableName + "'";
	int nRet;
	char *errMsg = NULL;
	char **dbResult;
	int nRow, nColumn;

	nRet = sqlite3_get_table(m_pDB, strSQLCmd.c_str(), &dbResult, &nRow, &nColumn, &errMsg);

	if(nRet != SQLITE_OK || nRow <= 0)
	{
		if(errMsg != NULL)
		{
		    sqlite3_free(errMsg);				
		}
		RetVal = false;
	}
	sqlite3_free_table(dbResult);

	return RetVal;
}

int CTradeDb::FieldIndex(const string& strFieldName, vector<string>& strFieldNameList)
{
    int index;
    int i;

    for(i = 0; i < strFieldNameList.size(); i++)
    {
        if(strFieldName == strFieldNameList[i])
            index = i;
    }

    return index;
}

