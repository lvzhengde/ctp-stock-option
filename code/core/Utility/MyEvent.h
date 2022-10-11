#ifndef WIN_CTP
#ifndef My_Event_Header
#define My_Event_Header
 
#include <iostream>
#include <pthread.h>
#include <errno.h>
 
using namespace std;
 
//---------------------------------------------------------------
 
class CEventImpl
{
protected:
	
	/*
	 动态方式初始化互斥锁,初始化状态变量m_cond
	`manualReset  true   人工重置
				  false  自动重置
	*/
	CEventImpl(bool manualReset);		
	
	/*
	 注销互斥锁,注销状态变量m_cond
	*/
	~CEventImpl();
 
	/*
	 将当前事件对象设置为有信号状态
	 若自动重置，则等待该事件对象的所有线程只有一个可被调度
	 若人工重置，则等待该事件对象的所有线程变为可被调度
	*/
	void SetImpl();
 
	/*
	 以当前事件对象，阻塞线程，将其永远挂起
	 直到事件对象被设置为有信号状态
	*/
	bool WaitImpl();
 
	/*
	 以当前事件对象，阻塞线程，将其挂起指定时间间隔
	 之后线程自动恢复可调度
	*/
	bool WaitImpl(long milliseconds);
 
	/*
	 将当前事件对象设置为无信号状态
	*/
	void ResetImpl();
 
private:
	bool            m_manual;
	volatile bool   m_state;
	pthread_mutex_t m_mutex;
	pthread_cond_t  m_cond;
};
 
inline void CEventImpl::SetImpl()
{
	if (pthread_mutex_lock(&m_mutex))	
		cout<<"cannot signal event (lock)"<<endl;
 
	//设置状态变量为true，对应有信号
	m_state = true;
 
	//cout<<"CEventImpl::SetImpl m_state = "<<m_state<<endl;
 
	//重新激活所有在等待m_cond变量的线程
	if (pthread_cond_broadcast(&m_cond))
	{
		pthread_mutex_unlock(&m_mutex);
		cout<<"cannot signal event"<<endl;
	}
	pthread_mutex_unlock(&m_mutex);
}
 
inline void CEventImpl::ResetImpl()
{
	if (pthread_mutex_lock(&m_mutex))	
		cout<<"cannot reset event"<<endl;
 
	//设置状态变量为false，对应无信号
	m_state = false;
 
	//cout<<"CEventImpl::ResetImpl m_state = "<<m_state<<endl;
 
	pthread_mutex_unlock(&m_mutex);
}
 
//---------------------------------------------------------------
 
class CMyEvent: private CEventImpl
{
public:
	CMyEvent(bool bManualReset = true);
	~CMyEvent();
 
	void Set();
	bool Wait();
	bool Wait(long milliseconds);
	bool TryWait(long milliseconds);
	void Reset();
 
private:
	CMyEvent(const CMyEvent&);
	CMyEvent& operator = (const CMyEvent&);
};
 
 
inline void CMyEvent::Set()
{
	SetImpl();
}
 
inline bool CMyEvent::Wait()
{
	return WaitImpl();
}
 
inline bool CMyEvent::Wait(long milliseconds)
{
	if (!WaitImpl(milliseconds))
	{
		cout<<"time out"<<endl;
		return false;
	}
	else
	{
		return true;
	}
}
 
inline bool CMyEvent::TryWait(long milliseconds)
{
	return WaitImpl(milliseconds);
}
 
inline void CMyEvent::Reset()
{
	ResetImpl();
}
 
#endif
#endif //WIN_CTP
