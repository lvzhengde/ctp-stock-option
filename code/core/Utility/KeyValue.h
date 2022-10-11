#ifndef _KEYVALUE_H
#define _KEYVALUE_H

#include "common.h"

class CKeyValue
{
public:  //member variables
	vector<CConfigItem> mConfigItemList; 
	bool m_bSuppressDisplay;

public:  //member functions
	CKeyValue(bool bSuppressDisplay = false);

	~CKeyValue();
	int ReadConfigFromDb(void* pDbObject, string strTableName);

	int WriteConfigToDb(void* pDbObject, string strTableName);

	bool ReadConfigFromTextFile(string strFilePath);

	bool GetKeyValue(const string key, string& value);

	string InputStringKeyValue(string key);

	double InputFloatKeyValue(string key);

	int InputIntKeyValue(string key);
};

#endif