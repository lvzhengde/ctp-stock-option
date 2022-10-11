#include "MyFlowControl.h"
#include "CtpCntIf.h"

//++
//构造函数
//--
CMyFlowControl::CMyFlowControl(CCtpCntIf* pCtpCntIf, double SnapshotInterval)
{
	InitializeSRWLock(&m_RwLock);

	m_pCtpCntIf = pCtpCntIf;
	m_SnapshotInterval = SnapshotInterval;
	m_OrderNum = 0;
	m_SnapshotSeconds = 0;
	m_SnapshotOrderNum = 0;
}

//++
//析构函数
//--
CMyFlowControl::~CMyFlowControl()
{
}

//++
//增加报撤单数目
//--
void CMyFlowControl::IncreaseOrderNum()
{
	writelock();
	m_OrderNum++;
	writeunlock();
}

//++
//获取报撤单数目
//参数
//    无
//返回值
//    当前报撤单数目
//--
int CMyFlowControl::GetOrderNum()
{
	int OrderNum;

	readlock();
	OrderNum = m_OrderNum;
	readunlock();

	return OrderNum;
}

//++
//对报撤单数目进行快照
//--
void CMyFlowControl::SnapshotOrderNum()
{
	double CurTimeSecs;
	SYSTEMTIME CurTime;
	GetLocalTime(&CurTime);
	CurTimeSecs = CurTime.wHour*3600 + CurTime.wMinute*60 + CurTime.wSecond + CurTime.wMilliseconds*0.001;

	if(CurTimeSecs >= (m_SnapshotSeconds + m_SnapshotInterval))
	{
		m_SnapshotSeconds = CurTimeSecs;
		m_SnapshotOrderNum = GetOrderNum();
	}
}

//++
//判断当前报撤单数目是否超过m_SnapshotInterval时间内的最大允许值
//参数
//   MaxOrderNum: m_SnapshotInterval时间内的最大报撤单允许值
//返回值
//   true: 超过
//   false: 没有超过
//--
bool CMyFlowControl::IsExceedMaxOrderNum(int MaxOrderNum)
{
	bool bExceed = false;

	int OrderNum = GetOrderNum();
	if(OrderNum >= (m_SnapshotOrderNum + MaxOrderNum))
	{
		bExceed = true;
	}

	return bExceed;
}


