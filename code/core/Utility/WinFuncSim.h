/*++
   在Linux中模拟的Windows函数 
--*/

#ifndef WIN_CTP
#ifndef _WIN_FUNC_SIM_H_
#define _WIN_FUNC_SIM_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
//#include <termio.h>
#include <termios.h>
#include <unistd.h>
#include "UserDefinedDataStruct.h"

#define INFINITE            (0xFFFFFFFF)  // Infinite timeout
#define WAIT_OBJECT_0       (0)
#define WAIT_TIMEOUT        (258L)
#define WAIT_FAILED         (0xFFFFFFFF)

#define _getch()  getch()
#define _kbhit()  kbhit()

#define strcpy_s(x, y) strcpy(x, y)

#define _vscwprintf(x, y)  vswprintf(0, 0, x, y)
#define _vsnwprintf_s(a, b, c, d, e)  vswprintf(a, b, d, e)
#define _vscprintf(x, y)  vsnprintf(0, 0, x, y)
#define _vsnprintf_s(a, b, c, d, e)  vsnprintf(a, b, d, e)

#define OutputDebugStringW(x) //(cout << x << ENDLINE)
#define OutputDebugStringA(x) //(cout << x << ENDLINE)

#define InitializeSRWLock(x)   pthread_rwlock_init(x, NULL)
#define AcquireSRWLockShared(x)  pthread_rwlock_rdlock(x)
#define ReleaseSRWLockShared(x)  pthread_rwlock_unlock(x)
#define AcquireSRWLockExclusive(x)  pthread_rwlock_wrlock(x)
#define ReleaseSRWLockExclusive(x)  pthread_rwlock_unlock(x)

#define InitializeCriticalSection(x)  pthread_mutex_init(x, NULL)
#define DeleteCriticalSection(x)  pthread_mutex_destroy(x)
#define EnterCriticalSection(x)  pthread_mutex_lock(x)
#define LeaveCriticalSection(x)  pthread_mutex_unlock(x)

int getch(void);

void Beep(int frequency, int milliseconds);

void FlushKbInBuf(void);

void GetLocalTime(SYSTEMTIME *CurTime);

void SetEvent(CEventHandle& hEvent);

void ResetEvent(CEventHandle& hEvent);

unsigned int WaitForSingleObject(CEventHandle& hEvent, unsigned int dwMilliseconds);

unsigned int WaitForMultipleObjects(unsigned int nCount, CEventHandle *pEvent, bool bWaitAll, unsigned int dwMilliseconds);

int kbhit(void);

#endif  //_WIN_FUNC_SIM_H
#endif  //WIN_CTP

