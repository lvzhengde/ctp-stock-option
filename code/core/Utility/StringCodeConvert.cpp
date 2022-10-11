#include "common.h"
#include "StringCodeConvert.h"

#ifdef WIN_CTP
//UTF-8转Unicode 
std::wstring Utf82Unicode(const std::string& utf8string) {
    int widesize = ::MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, NULL, 0);
    if (widesize == ERROR_NO_UNICODE_TRANSLATION)
    {
        throw std::exception("Invalid UTF-8 sequence.");
    }
    if (widesize == 0)
    {
        throw std::exception("Error in conversion.");
    }
    std::vector<wchar_t> resultstring(widesize);
    int convresult = ::MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, &resultstring[0], widesize);
    if (convresult != widesize)
    {
        throw std::exception("La falla!");
    }
    return std::wstring(&resultstring[0]);
}

//unicode 转为 ascii 
std::string WideByte2Acsi(std::wstring& wstrcode){
    int asciisize = ::WideCharToMultiByte(CP_OEMCP, 0, wstrcode.c_str(), -1, NULL, 0, NULL, NULL);
    if (asciisize == ERROR_NO_UNICODE_TRANSLATION)
    {
        throw std::exception("Invalid UTF-8 sequence.");
    }
    if (asciisize == 0)
    {
        throw std::exception("Error in conversion.");
    }
    std::vector<char> resultstring(asciisize);
    int convresult = ::WideCharToMultiByte(CP_OEMCP, 0, wstrcode.c_str(), -1, &resultstring[0], asciisize, NULL, NULL);
    if (convresult != asciisize)
    {
        throw std::exception("La falla!");
    }
    return std::string(&resultstring[0]);
}

//utf-8 转 ascii 
std::string UTF_82ASCII(std::string& strUtf8Code){
    using namespace std;
    string strRet = "";
    //先把 utf8 转为 unicode 
    wstring wstr = Utf82Unicode(strUtf8Code);
    //最后把 unicode 转为 ascii 
    strRet = WideByte2Acsi(wstr);
    return strRet;
}

//ascii 转 Unicode 
std::wstring Acsi2WideByte(std::string& strascii){
    using namespace std;
    int widesize = MultiByteToWideChar(CP_ACP, 0, (char*)strascii.c_str(), -1, NULL, 0);
    if (widesize == ERROR_NO_UNICODE_TRANSLATION)
    {
        throw std::exception("Invalid UTF-8 sequence.");
    }
    if (widesize == 0)
    {
        throw std::exception("Error in conversion.");
    }
    std::vector<wchar_t> resultstring(widesize);
    int convresult = MultiByteToWideChar(CP_ACP, 0, (char*)strascii.c_str(), -1, &resultstring[0], widesize);
    if (convresult != widesize)
    {
        throw std::exception("La falla!");
    }
    return std::wstring(&resultstring[0]);
}

//Unicode 转 Utf8 
std::string Unicode2Utf8(const std::wstring& widestring){
    using namespace std;
    int utf8size = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8size == 0)
    {
        throw std::exception("Error in conversion.");
    }
    std::vector<char> resultstring(utf8size);
    int convresult = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, &resultstring[0], utf8size, NULL, NULL);
    if (convresult != utf8size)
    {
        throw std::exception("La falla!");
    }
    return std::string(&resultstring[0]);
}

//ascii 转 Utf8 
std::string ASCII2UTF_8(std::string& strAsciiCode) {
    using namespace std;
    string strRet("");
    //先把 ascii 转为 unicode 
    wstring wstr = Acsi2WideByte(strAsciiCode);
    //最后把 unicode 转为 utf8 
    strRet = Unicode2Utf8(wstr);
    return strRet;
}

//与Linux兼容的函数，用ASCII形式的string存储GBK和UTF-8编码的汉字
int  GBKToUTF8(const string& input, string& output)
{
    int ret = 0;
    
	string strGBK = input;
	output = ASCII2UTF_8(strGBK);
	ret = output.size();

    return ret;
}

//与Linux兼容的函数，用ASCII形式的string存储GBK和UTF-8编码的汉字
int UTF8ToGBK(const string& input, string& output)
{
    int ret =0;
    
	string strUTF8 = input;
	output = UTF_82ASCII(strUTF8);
	ret = output.size();

    return ret;
}

#else //Linux

//iconv的原型如下：
//    size_t iconv(iconv_t cd,char **inbuf,size_t *inbytesleft,char **outbuf,size_t *outbytesleft);
//
//作为入参
//第一个参数cd是转换句柄。
//第二个参数inbuf是输入字符串的地址的地址。
//第三个参数inbytesleft是输入字符串的长度。
//第四个参数outbuf是输出缓冲区的首地址
//第五个参数outbytesLeft 是输出缓冲区的长度。
//
//
//作为出参
//第一个参数cd是转换句柄。
//第二个参数inbuf指向剩余字符串的地址
//第三个参数inbytesleft是剩余字符串的长度。
//第四个参数outbuf是输出缓冲区剩余空间的首地址
//第五个参数outbytesLeft 是输出缓冲区剩余空间的长度。

int UTF8ToGBK(char* input, size_t& charInPutLen, char* output, size_t& charOutPutLen)
{
    
    int ret =0;
    iconv_t cd;
    cd = iconv_open("GBK","utf-8");
    ret = iconv(cd, &input, &charInPutLen, &output, &charOutPutLen);
    iconv_close(cd);
    return ret;
}
 
int UTF8ToGBK(const string& input, string& output)
{
    int ret =0;
    size_t charInPutLen = input.length();
    if( charInPutLen == 0)
        return 0;
    //char *pSource =(char *)input.c_str();
    size_t charOutPutLen = 2*charInPutLen;
    char *pTemp = new char[charOutPutLen];
    memset(pTemp,0,2*charInPutLen);
    
    iconv_t cd;
    char *pSource =(char *)input.c_str();
    char *pOut = pTemp ;
    cd = iconv_open("utf-8", "GBK");
    ret = iconv(cd, &pSource, &charInPutLen, &pTemp, &charOutPutLen);
    iconv_close(cd);
    output = pOut;
    delete []pOut;//注意这里，不能使用delete []pTemp, iconv函数会改变指针pTemp的值
    return ret;
}
 
int  GBKToUTF8(char* input, size_t charInPutLen, char* output, size_t &charOutPutLen)
{
    int ret = 0;
    iconv_t cd;
    cd = iconv_open("utf-8", "GBK");
    ret = iconv(cd, &input, &charInPutLen, &output, &charOutPutLen);
    iconv_close(cd);
    return ret;
}
 
 
int  GBKToUTF8(const string& input, string& output)
{
    int ret = 0;
    size_t charInPutLen = input.length();
    if( charInPutLen == 0)
        return 0;
    
    size_t charOutPutLen = 2*charInPutLen+1;
    char *pTemp = new char[charOutPutLen];
    memset(pTemp,0,charOutPutLen);
    iconv_t cd;
    char *pSource =(char *)input.c_str();
    char *pOut = pTemp;
    cd = iconv_open("utf-8", "GBK");
    ret = iconv(cd, &pSource, &charInPutLen, &pTemp, &charOutPutLen);
    iconv_close(cd);
    output= pOut;
    delete []pOut; //注意这里，不能使用delete []pTemp, iconv函数会改变指针pTemp的值
    return ret;
}

int  MyCodeConvert(const string& input, string& output, const char *from_charset, const char *to_charset)
{
    int ret = 0;
    size_t charInPutLen = input.length();
    if( charInPutLen == 0)
        return 0;
    
    size_t charOutPutLen = 2*charInPutLen+1;
    char *pTemp = new char[charOutPutLen];
    memset(pTemp,0,charOutPutLen);
    iconv_t cd;
    char *pSource =(char *)input.c_str();
    char *pOut = pTemp;
    //cd = iconv_open("utf-8", "GBK");
	cd = iconv_open(to_charset,from_charset);
    ret = iconv(cd, &pSource, &charInPutLen, &pTemp, &charOutPutLen);
    iconv_close(cd);
    output= pOut;
    delete []pOut; //注意这里，不能使用delete []pTemp, iconv函数会改变指针pTemp的值
    return ret;
}
#endif //WIN/Linux CTP selection
