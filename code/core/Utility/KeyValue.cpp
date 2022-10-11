#include "KeyValue.h"
#include "TradeDb.h"

//++
//构造函数
//--
CKeyValue::CKeyValue(bool bSuppressDisplay)
{
	m_bSuppressDisplay = bSuppressDisplay;
}

//++
//析构函数
//--
CKeyValue::~CKeyValue()
{
	
}

//++
//从文本文件中读出交易的配置信息
//文本文件的变量内容应该是如下格式的
// id=20
// name=bob
// address=beijing
//参数
//    strFilePath: 文本文件路径
//返回值
//    true: 读取配置信息成功
//    false: 失败
 //--
bool CKeyValue::ReadConfigFromTextFile(string strFilePath)
{
	string key, value;
	CConfigItem configItem;
    fstream cfgFile;

    cfgFile.open(strFilePath);//打开文件	
    if(!cfgFile.is_open())
    {
        cout << "找不到配置文件: " << strFilePath << ENDLINE;
        return false;
    }

    char tmp[1000];
	mConfigItemList.clear();

    while(!cfgFile.eof())//循环读取每一行
    {
        cfgFile.getline(tmp,1000);//每行读取前1000个字符，1000个应该足够了
        string line(tmp);
		line = RemoveSpace(line);
        size_t pos = line.find('=');//找到每行的“=”号位置，之前是key之后是value
		if(line.c_str()[0] == '#') continue;   //注释，跳过此行
        if(pos==string::npos) 
		{
			if(strcmp(line.c_str(), "") == 0)   //空行可以继续
				continue;
			else
			{
				cout << "配置文件格式不对！" << ENDLINE;
				return false;
			}
		}
        key = line.substr(0,pos);//取=号之前
		value = line.substr(pos+1);//取=号之后

		configItem.key = RemoveSpace(key);
		configItem.value = RemoveSpace(value);
		configItem.bDbRecordExist = false;
		mConfigItemList.push_back(configItem);
    }

    return (mConfigItemList.size()>0);
}

//++
//从数据库获取配置信息
//参数
//    pDbObject: 自定义的数据库对象指针
//    strTableName: 数据表名称，数据表必须有KeyId和ValueExp两个字段
//返回值
//    >=0: 获取的配置信息长度
//    -1(<0): 读取数据库失败
//--
int CKeyValue::ReadConfigFromDb(void* pDbObject, string strTableName)
{
	string strFieldDesc;
	CTradeDb* pTradeDb = (CTradeDb*)pDbObject;

	//确定数据表是否存在，不存在则创建
	strFieldDesc = "(ID INTEGER PRIMARY KEY AUTOINCREMENT, KeyId VARCHAR UNIQUE, ValueExp VARCHAR, Description VARCHAR)";
	int nCreateDb = pTradeDb->CreateTableIfNotExist(strTableName, strFieldDesc);
	if(nCreateDb == -1)
		return -1;
	else if(nCreateDb == 1)
		return 0;

    int RetVal;
	char *errMsg = NULL;
	char **dbResult;
	int nRow, nColumn;
	int i, j;
    string strSQLCmd;
	string key, value;
	CConfigItem configItem;

	RetVal = 0;
	mConfigItemList.clear();

	strSQLCmd = "SELECT KeyId, ValueExp FROM " + strTableName;
	RetVal = sqlite3_get_table(pTradeDb->m_pDB, strSQLCmd.c_str(), &dbResult, &nRow, &nColumn, &errMsg);

	if(RetVal != SQLITE_OK)
	{
		cerr << "打开数据表: " << strTableName << " 出错: " << errMsg << ENDLINE;
		sqlite3_free(errMsg);		
		RetVal = -1;
	}
	else 
	{   //数据表存在
		//设置FieldName List
        vector<string> FieldNameList;
		int index = 0;
        for(j = 0; j < nColumn; j++)
        {
           FieldNameList.push_back(dbResult[j]);
        }

        for(i = 0; i < nRow; i++)
        {
			string KeyId, ValueExp;
            index = pTradeDb->FieldIndex("KeyId", FieldNameList) + (i+1)*nColumn;
			KeyId =  dbResult[index];
            index = pTradeDb->FieldIndex("ValueExp", FieldNameList) + (i+1)*nColumn;
            ValueExp = dbResult[index];

			key = RemoveSpace(KeyId);
			value = RemoveSpace(ValueExp);
			configItem.key = key;
			configItem.value = value;
			configItem.bDbRecordExist = true;
			mConfigItemList.push_back(configItem);
        }
		RetVal = mConfigItemList.size();
	}
	sqlite3_free_table(dbResult);

	return RetVal;
}

//++
//将配置信息写入数据库, 在之前将先删除相关数据表的内容
//参数
//    pDbObject: 自定义的数据库对象指针
//    strTableName: 数据表名称，数据表必须有KeyId和ValueExp两个字段
//返回值
//    >=0: 写入的配置信息长度
//    -1(<0): 写入数据库出现错误
//--
int  CKeyValue::WriteConfigToDb(void* pDbObject, string strTableName)
{
    int nWriteNum;
    string strSQLCmd;
	string key, value;
	CTradeDb* pTradeDb = (CTradeDb*)pDbObject;

	nWriteNum = 0;
	for(size_t i = 0; i < mConfigItemList.size(); i++)
	{
		//只写入数据库中不存在的记录
		if(mConfigItemList[i].bDbRecordExist == false)
		{
			key = mConfigItemList[i].key;
			value = mConfigItemList[i].value;
			strSQLCmd = "INSERT INTO " + strTableName + "(KeyId, ValueExp) \
						VALUES('" + key + "', '" + value + "')";
			if(pTradeDb->ExecuteSqlCommand(strSQLCmd) == false)
				return -1;

			mConfigItemList[i].bDbRecordExist = true;
			nWriteNum = nWriteNum + 1;
		}
	}

	return nWriteNum;
}

//++
//获取key对应的value
//参数
//    key: 键
//    value: 键对应的值
//返回值
//    true: 成功获取key对应的value
//    false: 没有找到
//--
bool CKeyValue::GetKeyValue(const string key, string& value)
{
	int listSize;
	listSize = mConfigItemList.size();

	for(int i = 0; i < listSize; i++)
	{
		if(key == mConfigItemList[i].key) 
		{
			value = mConfigItemList[i].value;
			return true;
		}
	}

	return false;
}

//++
//通过标准键盘或者配置数据表/文件获取key值对应的string输入
//参数
//    key: 键
//返回值
//    string输入
//--
string CKeyValue::InputStringKeyValue(string key)
{
	string value;

	key = RemoveSpace(key);

	if(GetKeyValue(key, value))
	{
		if(m_bSuppressDisplay == false)
		{
			cerr << "输入 " << key << "> ";
#ifdef WIN_CTP
			cerr << UTF_82ASCII(value) << ENDLINE;
#else
			cerr << value << ENDLINE;
#endif
		}
	}
	else
	{
		cerr << "输入 " << key << "> ";
		CConfigItem configItem;
		cin >> value;
		value = RemoveSpace(value);
		configItem.key = key;
		configItem.value = value;
		configItem.bDbRecordExist = false;
		mConfigItemList.push_back(configItem);
	}

	return value;
}

//++
//通过标准键盘或者配置数据表/文件获取key值对应的浮点数输入
//参数
//    key: 键
//返回值
//    浮点数输入
//--
double CKeyValue::InputFloatKeyValue(string key)
{
	double Retval;
	string value;

	key = RemoveSpace(key);

	if(GetKeyValue(key, value))
	{
		if(m_bSuppressDisplay == false)
		{
			cerr << "输入 " << key << "> ";
			cerr << value << ENDLINE;
		}
	}
	else
	{
		cerr << "输入 " << key << "> ";
		CConfigItem configItem;
		cin >> value;
		value = RemoveSpace(value);
		configItem.key = key;
		configItem.value = value;
		configItem.bDbRecordExist = false;
		mConfigItemList.push_back(configItem);
	}
	Retval = atof(value.c_str());

	return Retval;
}

//++
//通过标准键盘或者配置数据表/文件获取key值对应的整数输入
//参数
//    key: 键
//返回值
//    整数输入
//--
int CKeyValue::InputIntKeyValue(string key)
{
	int Retval;
	string value;

	key = RemoveSpace(key);

	if(GetKeyValue(key, value))
	{
		if(m_bSuppressDisplay == false)
		{
			cerr << "输入 " << key << "> ";
			cerr << value << ENDLINE;
		}
	}
	else
	{
		cerr << "输入 " << key << "> ";
		CConfigItem configItem;
		cin >> value;
		value = RemoveSpace(value);
		configItem.key = key;
		configItem.value = value;
		configItem.bDbRecordExist = false;
		mConfigItemList.push_back(configItem);
	}
	Retval = atoi(value.c_str());

	return Retval;
}

