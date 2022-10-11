//++
//用于线程安全的Vector类，可以根据需要主动加锁
//对于模板类而言, 类定义和函数实现都必须放在.h文件中
//不能分开放入.h和.cpp文件, 否则编译不能通过
//--

#ifndef _SAFE_VECTOR_H_
#define _SAFE_VECTOR_H_

#include <iostream>  
#include <string>  
#include <sstream>  
#include <vector>
#ifdef WIN_CTP
#include<conio.h>
#include "windows.h"
#include <process.h>
#else
#include "WinFuncSim.h"
#include <pthread.h>
#endif

//使用vector必须要有以下声明
using namespace std;  

template <typename Type>
class CSafeVector
{
private:
	vector<Type> m_List;
	SRWLOCK m_RwLock;
	size_t m_MaxSize;

public:
	CSafeVector();
	void SetSize(size_t MaxListSize);
	void clear();
	size_t size();
	Type& operator[](size_t index);
	void push_back(Type value);
	void swap(vector<Type> &SwapList);
	void duplicate(vector<Type> &dupList);
	void append(const vector<Type> AppendList);
	void readlock();
	void readunlock();
	void writelock();
	void writeunlock();
};

template <typename Type>
CSafeVector<Type>::CSafeVector()
{
	InitializeSRWLock(&m_RwLock);
	m_MaxSize = 0;
}

template <typename Type>
void CSafeVector<Type>::SetSize(size_t MaxListSize)
{
	m_MaxSize = MaxListSize;
}

template <typename Type>
void CSafeVector<Type>::duplicate(vector<Type> &dupList)
{
	dupList.clear();
	dupList.insert(dupList.end(), m_List.begin(), m_List.end());
}

template <typename Type>
void CSafeVector<Type>::push_back(Type value)
{
	size_t KeepLen = m_List.size() >> 1;
	if(m_MaxSize > 0 && m_List.size() >= m_MaxSize)
	{
		vector<Type> tmpList;
#ifdef WIN_CTP
		vector<Type>::iterator it = m_List.end() - KeepLen;
#else
		typename vector<Type>::iterator it = m_List.end() - KeepLen;
#endif
		tmpList.insert(tmpList.end(), it, m_List.end());
		m_List.swap(tmpList);
		m_List.push_back(value);
	}
	else
	{
		m_List.push_back(value);
	}
}

template <typename Type>
void CSafeVector<Type>::swap(vector<Type> &SwapList)
{
	m_List.swap(SwapList);
}

template <typename Type>
void CSafeVector<Type>::clear()
{
	m_List.clear();
}

template <typename Type>
void CSafeVector<Type>::append(const vector<Type> AppendList)
{
	m_List.insert(m_List.end(), AppendList.begin(), AppendList.end());
}

template <typename Type>
size_t CSafeVector<Type>::size()
{
	return m_List.size();
}

template <typename Type>
Type& CSafeVector<Type>::operator[](size_t index)
{
	Type& ret = m_List[index];
	return ret;
}

template <typename Type>
void CSafeVector<Type>::readlock()
{
	AcquireSRWLockShared(&m_RwLock);
}

template <typename Type>
void CSafeVector<Type>::readunlock()
{
	ReleaseSRWLockShared(&m_RwLock);
}

template <typename Type>
void CSafeVector<Type>::writelock()
{
	AcquireSRWLockExclusive(&m_RwLock);
}

template <typename Type>
void CSafeVector<Type>::writeunlock()
{
	ReleaseSRWLockExclusive(&m_RwLock);
}

#endif
