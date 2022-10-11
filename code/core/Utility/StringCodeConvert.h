#ifndef  _STRING_CODE_CONVERT_
#define  _STRING_CODE_CONVERT_

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>

#ifdef WIN_CTP
#include <windows.h>
#endif

using namespace std;

#ifdef WIN_CTP
//UTF-8转Unicode 
std::wstring Utf82Unicode(const std::string& utf8string);

//unicode 转为 ascii 
std::string WideByte2Acsi(std::wstring& wstrcode);

//utf-8 转 ascii 
std::string UTF_82ASCII(std::string& strUtf8Code);

//ascii 转 Unicode 
std::wstring Acsi2WideByte(std::string& strascii);

//Unicode 转 Utf8 
std::string Unicode2Utf8(const std::wstring& widestring);

//ascii 转 Utf8 
std::string ASCII2UTF_8(std::string& strAsciiCode);

//与Linux兼容的函数，用ASCII形式的string存储GBK和UTF-8编码的汉字
int GBKToUTF8(const string& input, string& output);

//与Linux兼容的函数，用ASCII形式的string存储GBK和UTF-8编码的汉字
int UTF8ToGBK(const string& input, string& output);

#else

int UTF8ToGBK(char* input, size_t& charInPutLen, char* output, size_t& charOutPutLen); 

int UTF8ToGBK(const string& input, string& output);

int GBKToUTF8(char* input, size_t charInPutLen, char* output, size_t &charOutPutLen);

int GBKToUTF8(const string& input, string& output);

int MyCodeConvert(const string& input, string& output, const char *from_charset, const char *to_charset);

#endif //WIN/Linux CTP

#endif //_STRING_CODE_CONVERT_
