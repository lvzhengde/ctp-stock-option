/*++
    包含常用的头文件，不作它用
--*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <omp.h>
#ifdef WIN_CTP
#include "windows.h"
#include <process.h>
#include <io.h>
#include <conio.h>
#include <time.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>
#include <iconv.h> 
#include <signal.h>
#include <stdarg.h>
#include <wchar.h>
#include "WinFuncSim.h"
#endif

#include "StringCodeConvert.h"
#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"
#include "UserDefinedDataStruct.h"
#include "MyUtilities.h"
#include "MyMath.h"
#include "SafeVector.h"
#include "TshapeDataClass.h"

//#define SEE_THROUGH_SUPERVISION
//#define FUTURE_OPTION_ONLY

#endif
