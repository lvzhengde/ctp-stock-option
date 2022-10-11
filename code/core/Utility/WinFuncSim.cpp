#ifndef WIN_CTP
#include "common.h"
#include "WinFuncSim.h"
#include <stdio_ext.h>

int getch(void)
{
     struct termios tm, tm_old;
     int fd = 0, ch;
 
     if (tcgetattr(fd, &tm) < 0) {//保存现在的终端设置
          return -1;
     }
 
     tm_old = tm;
     cfmakeraw(&tm);//更改终端设置为原始模式，该模式下所有的输入数据以字节为单位被处理
     if (tcsetattr(fd, TCSANOW, &tm) < 0) {//设置上更改之后的设置
          return -1;
     }
 
     ch = getchar();
     if (tcsetattr(fd, TCSANOW, &tm_old) < 0) {//更改设置为最初的样子
          return -1;
     }
    
     return ch;
}

void Beep(int frequency, int milliseconds)
{
     int fd = open("/dev/console", O_RDONLY);
     if (fd == -1)
	 {
	     //The most basic beep is still '\a' , if your terminal supports it
	     fprintf(stdout, "\aBeep!\n" );
	 } 
	 else
	 {
		 ioctl(fd, KDMKTONE, (milliseconds << 16) + (1193180/frequency));
	 }
}

//清空键盘输入缓存
void FlushKbInBuf(void)
{
	//方案1， 可能陷入死循环
    //char n; 
    //while((n = getchar()) != '\n' && n != EOF);
	//方案2, 工作有异常
	//setbuf(stdin, NULL);   //清空缓冲区
	//方案3
	//char sbuf[1024];
	//fgets(sbuf, 1024, stdin);	 setbuf(stdin, NULL);   //清空缓冲区
	//方案4, 必须包含文件 <stdio_ext.h>
	__fpurge(stdin);

}

void GetLocalTime(SYSTEMTIME *CurTime)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
	struct tm * pTM;
	//pTM = localtime(&tv.tv_sec);
	struct tm today;
	time_t now = tv.tv_sec + 8*60*60;//UTC to BeiJing timezone
	gmtime_r(&now, &today);
    pTM = &today;

	CurTime->wYear = pTM->tm_year + 1900;
	CurTime->wMonth = pTM->tm_mon + 1;
	CurTime->wDayOfWeek = pTM->tm_wday;
	CurTime->wDay = pTM->tm_mday;
	CurTime->wHour = pTM->tm_hour;
	CurTime->wMinute = pTM->tm_min;
	CurTime->wSecond = pTM->tm_sec;
	CurTime->wMilliseconds = DWORD(tv.tv_usec*1e-3);
}

void SetEvent(CEventHandle& hEvent)
{
    hEvent->Set();
}

void ResetEvent(CEventHandle& hEvent)
{
    hEvent->Reset();
}

unsigned int WaitForSingleObject(CEventHandle& hEvent, unsigned int dwMilliseconds)
{
    bool bRet;
	unsigned int ret;

	if(dwMilliseconds == INFINITE)
	{
	    bRet = hEvent->Wait();
	}
	else
	{
	    bRet = hEvent->TryWait(dwMilliseconds); 
	}

	if(bRet == true)
		ret = WAIT_OBJECT_0;
	else
		ret = 0xF;

	return ret;
}

unsigned int WaitForMultipleObjects(unsigned int nCount, CEventHandle *pEvent, bool bWaitAll, unsigned int dwMilliseconds)
{
    unsigned int ret = WAIT_FAILED;
	unsigned int minMilliseconds = 5;
	unsigned int i, j;

	bool *bEventTriggered = new bool[nCount];
    for(i = 0; i <nCount; i++)
	{
		bEventTriggered[i] = false;
	}

	bool bExitLoop = false;
	struct timeval tv0;
    gettimeofday(&tv0,NULL);
	double t0 = tv0.tv_sec + tv0.tv_usec*1e-6;
	double t1 = t0;
	while(bExitLoop == false)
	{
        for(i = 0; i < nCount; i++)
	    {
			bool bRet = false;
	    	if(bEventTriggered[i] == false)
	    	{
	            bRet = pEvent[i]->TryWait(minMilliseconds); 
	    	}

	    	if(bRet == true)
	    	{
	            bEventTriggered[i] = true;
	    	    if(bWaitAll == false)
	    		{
	    		    ret = WAIT_OBJECT_0 + i;
					bExitLoop = true;
	    			break;
	    		}	
	    	}

			//检测是否所有事件都已触发
			if(bWaitAll == true)
			{
		      bool bAll = true;
		      for(j = 0; j < nCount; j++)
			  {
			      if(bEventTriggered[j] == false)
				  {
					  bAll = false;
					  break;
				  }
			  }	  

			  if(bAll == true)
			  {
			      ret = WAIT_OBJECT_0;
				  bExitLoop = true;
				  break;
			  }
			} //bWaitAll

			//检查是否超时
	        struct timeval tv1;
            gettimeofday(&tv1,NULL);
	        t1 = tv1.tv_sec + tv1.tv_usec*1e-6;
			double interval = t1 - t0;
	    	if(interval*1000 >= dwMilliseconds && bExitLoop == false)
	    	{
	    	    ret = WAIT_TIMEOUT;
				bExitLoop = true;
	    		break;
	    	}
	    } //for i
	} //while

	return ret;
}

int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
 
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
    ch = getchar();
 
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
 
    if(ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }
 
    return 0;
}

#endif  //WIN_CTP
