#ifndef WIN_CTP
#include "MyEvent.h"
#include "UserDefinedDataStruct.h"
#include <sys/time.h>
 
CEventImpl::CEventImpl(bool manualReset): m_manual(manualReset), m_state(false)
{
	if (pthread_mutex_init(&m_mutex, NULL))
		cout<<"cannot create event (mutex)"<<ENDLINE;
	if (pthread_cond_init(&m_cond, NULL))
		cout<<"cannot create event (condition)"<<ENDLINE;
}
 
CEventImpl::~CEventImpl()
{
	pthread_cond_destroy(&m_cond);
	pthread_mutex_destroy(&m_mutex);
}
 
bool CEventImpl::WaitImpl()
{
	if (pthread_mutex_lock(&m_mutex))
	{
		cout<<"wait for event failed (lock)"<<ENDLINE; 
		return false;
	}
	while (!m_state) 
	{
		//cout<<"CEventImpl::WaitImpl while m_state = "<<m_state<<ENDLINE;
 
		//对互斥体进行原子的解锁工作,然后等待状态信号
		if (pthread_cond_wait(&m_cond, &m_mutex))
		{
			pthread_mutex_unlock(&m_mutex);
			cout<<"wait for event failed"<<ENDLINE;
			return false;
		}
	}
	if (m_manual == false)
		m_state = false;
	pthread_mutex_unlock(&m_mutex);
 
	//cout<<"CEventImpl::WaitImpl end m_state = "<<m_state<<ENDLINE;
 
	return true;
}
 
bool CEventImpl::WaitImpl(long milliseconds)
{
	int rc = 0;
	struct timespec abstime;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	abstime.tv_sec  = tv.tv_sec + milliseconds / 1000;
	abstime.tv_nsec = tv.tv_usec*1000 + (milliseconds % 1000)*1000000;
	if (abstime.tv_nsec >= 1000000000)
	{
		abstime.tv_nsec -= 1000000000;
		abstime.tv_sec++;
	}
 
	if (pthread_mutex_lock(&m_mutex) != 0)
	{
		cout<<"wait for event failed (lock)"<<ENDLINE; 
		return false;
	}
	while (!m_state) 
	{
		//自动释放互斥体并且等待m_cond状态,并且限制了最大的等待时间
		if ((rc = pthread_cond_timedwait(&m_cond, &m_mutex, &abstime)))
		{
			if (rc == ETIMEDOUT) break;
			pthread_mutex_unlock(&m_mutex);
			cout<<"cannot wait for event"<<ENDLINE;
			return false;
		}
	}
	if (rc == 0 && m_manual == false) 
		m_state = false;
	pthread_mutex_unlock(&m_mutex);
	return rc == 0;
}
 
CMyEvent::CMyEvent(bool bManualReset): CEventImpl(bManualReset)
{
}
 
CMyEvent::~CMyEvent()
{
}

#endif //WIN_CTP
