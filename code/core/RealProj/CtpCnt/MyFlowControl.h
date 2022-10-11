/*++
    用于交易柜台流量控制的类
	注意: 
	    1. 这只是一个粗略的流量控制, 只是控制各个策略的ScanForOpen/ScanForClose
	    2. 执行快照只在主进程中进行
--*/

#ifndef _MY_FLOWCONTROL_H_
#define _MY_FLOWCONTROL_H_

#include "common.h"

class CCtpCntIf;

class CMyFlowControl
{
//私有变量
private:
	//读写锁
	SRWLOCK m_RwLock;
	//指向交易柜台的指针
	CCtpCntIf* m_pCtpCntIf;

//公有变量
public:
	//当前报撤单数量(只有此变量需要加锁访问)
	int m_OrderNum;
	//快照时间间隔
	double m_SnapshotInterval;
	//报撤单快照时间(秒数)
	double m_SnapshotSeconds;
	//快照时间对应的报撤单数量
	int m_SnapshotOrderNum;

public:
	//构造函数
	CMyFlowControl(CCtpCntIf* pCtpCntIf = NULL, double SnapshotInterval = 1.0);

	//析构函数
	~CMyFlowControl();

	//获取读锁
	void readlock()
	{
		AcquireSRWLockShared(&m_RwLock);
	}

	//释放读锁
	void readunlock()
	{
		ReleaseSRWLockShared(&m_RwLock);
	}

	//获取写锁
	void writelock()
	{
		AcquireSRWLockExclusive(&m_RwLock);
	}

	//释放写锁
	void writeunlock()
	{
		ReleaseSRWLockExclusive(&m_RwLock);
	}

	//增加报撤单数目
	void IncreaseOrderNum();

	//获取报撤单数目
	int GetOrderNum();

	//对报撤单数目进行快照
	void SnapshotOrderNum();

	//判断当前报撤单数目是否超过m_SnapshotInterval时间内的最大允许值
	bool IsExceedMaxOrderNum(int MaxOrderNum);

};

#endif
