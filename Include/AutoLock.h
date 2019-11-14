#pragma once

#include <assert.h>
#include <Windows.h>
#include <stdio.h>
#include "CriticalSectionProxy.h"
#include <MMSystem.h>
#ifdef Release_D
#undef assert
#define assert	((void)0)
#endif
/// @brief 可以自动加锁和解锁临界区的类
///
/// 适用于解锁条件复杂,不方便手动解锁临界区代码的场合
///
/// @code
///	CRITICAL_SECTION cs;
/// CAutoLock  lock(&cs,true);
/// @endcode 
///
#define _OuputLockTime
#define _LockOverTime	100

#define Autolock(cs)	CAutoLock lock(cs,false,__FILE__,__FUNCTION__,__LINE__);
#define Autolock1(cs)	CAutoLock lock1(cs,false,__FILE__,__FUNCTION__,__LINE__);
#define Autolock2(cs)	CAutoLock lock2(cs,false,__FILE__,__FUNCTION__,__LINE__);
#define Autolock3(cs)	CAutoLock lock3(cs,false,__FILE__,__FUNCTION__,__LINE__);
#define Autolock4(cs)	CAutoLock lock4(cs,false,__FILE__,__FUNCTION__,__LINE__);
#define Autolock5(cs)	CAutoLock lock5(cs,false,__FILE__,__FUNCTION__,__LINE__);

class CAutoLock
{
private:
	CRITICAL_SECTION *m_pCS;
	bool	m_bAutoDelete;
	bool	m_bLocked;
#ifdef _LockOverTime
	DWORD	m_dwLockTime;
	CHAR   *m_pszFile;
	char   *m_pszFunction;
	int		m_nLockLine;
#endif

	explicit CAutoLock():m_pCS(NULL),m_bAutoDelete(false)
	{
	}
public:
	CAutoLock(CRITICAL_SECTION *pCS, bool bAutoDelete = false, const CHAR *szFile = nullptr, char *szFunction = nullptr, int nLine = 0)
	{
		ZeroMemory(this, sizeof(CAutoLock));
		assert(pCS != NULL);
		if (pCS)
		{
#ifdef _LockOverTime
			m_dwLockTime = timeGetTime();
			if (szFile)
			{
				m_pszFile = new CHAR[strlen(szFile) + 1];
				strcpy(m_pszFile, szFile);
				m_pszFunction = new char[strlen(szFunction) + 1];
				strcpy(m_pszFunction, szFunction);
				m_nLockLine = nLine;
			}
#endif
			m_pCS = pCS;
			::EnterCriticalSection(m_pCS);
			m_bLocked = true;
			if (timeGetTime() - m_dwLockTime >= _LockOverTime)
			{
				CHAR szOuput[1024] = { 0 };
				if (szFile)
				{
					sprintf(szOuput, "Wait Lock @File:%s:%d(%s),Waittime = %d.\n", m_pszFile, m_nLockLine, m_pszFunction , timeGetTime() - m_dwLockTime);
				}
				else
					sprintf(szOuput, "Wait Lock Waittime = %d.\n", timeGetTime() - m_dwLockTime);
				OutputDebugStringA(szOuput);
			}
		}
	}
	CAutoLock(CCriticalSectionProxy *pCS, bool bAutoDelete = false, const CHAR *szFile = nullptr, char *szFunction = nullptr, int nLine = 0)
	{
		ZeroMemory(this, sizeof(CAutoLock));
		assert(pCS != NULL);
		if (pCS)
		{
#ifdef _LockOverTime
			m_dwLockTime = timeGetTime();
			if (szFile)
			{
				m_pszFile = new CHAR[strlen(szFile) + 1];
				strcpy(m_pszFile, szFile);
				m_pszFunction = new char[strlen(szFunction) + 1];
				strcpy(m_pszFunction, szFunction);
				m_nLockLine = nLine;
			}
#endif
			m_pCS = pCS->Get();
			::EnterCriticalSection(m_pCS);
			m_bLocked = true;
			if (timeGetTime() - m_dwLockTime >= _LockOverTime)
			{
				CHAR szOuput[1024] = { 0 };
				if (szFile)
				{
					sprintf(szOuput, "Wait Lock @File:%s:%d(%s),Waittime = %d.\n", m_pszFile, m_nLockLine, m_pszFunction, timeGetTime() - m_dwLockTime);
				}
				else
					sprintf(szOuput, "Wait Lock Waittime = %d.\n", timeGetTime() - m_dwLockTime);
				OutputDebugStringA(szOuput);
			}
		}
	}
	void Lock()
	{
		if (m_bLocked)
			return;
#if _LockOverTime
		m_dwLockTime = timeGetTime();
#endif
		::EnterCriticalSection(m_pCS);
		m_bLocked = true;
		if (timeGetTime() - m_dwLockTime >= _LockOverTime)
		{
			CHAR szOuput[1024] = { 0 };
			if (m_pszFile)
			{
				sprintf(szOuput, "Wait Lock @File:%s:%d(%s),Waittime = %d.\n", m_pszFile, m_nLockLine, m_pszFunction, timeGetTime() - m_dwLockTime);
			}
			else
				sprintf(szOuput, "Wait Lock Waittime = %d.\n", timeGetTime() - m_dwLockTime);
			OutputDebugStringA(szOuput);
		}
		
		
	}
	void Unlock()
	{
		if (!m_bLocked)
			return;
		if (m_pCS)
		{
			::LeaveCriticalSection(m_pCS);
			m_bLocked = false;
#ifdef _OuputLockTime
			CHAR szOuput[1024] = { 0 };
			if ((timeGetTime() - m_dwLockTime) > _LockOverTime)
			{
				if (m_pszFile)
				{
					sprintf(szOuput, "Lock @File:%s:%d(%s),locktime = %d.\n", m_pszFile, m_nLockLine, m_pszFunction, timeGetTime() - m_dwLockTime);
				}
				else
					sprintf(szOuput, "Lock locktime = %d.\n", timeGetTime() - m_dwLockTime);
				OutputDebugStringA(szOuput);
			}
		
#endif
			
		}
	}
	~CAutoLock()
	{
		if (m_bLocked)
			Unlock();
		
		if (m_pszFile)
		{
			delete[]m_pszFile;
			m_pszFile = nullptr;
		}
		if (m_pszFunction)
		{
			delete[]m_pszFunction;
			m_pszFunction = nullptr;
		}
		
		if (m_bAutoDelete)
			::DeleteCriticalSection((CRITICAL_SECTION *)m_pCS);
	}
};

class CTryLock
{
private:
	CRITICAL_SECTION *m_pCS;
	bool	m_bAutoDelete;
	BOOL	m_bLocked;
public:
	CTryLock():m_pCS(NULL),m_bAutoDelete(false),m_bLocked(false)
	{
	}

	BOOL TryLock(CRITICAL_SECTION *pCS,bool bAutoDelete = false)
	{
		assert(pCS != NULL);
		if (pCS)
		{
			m_pCS = pCS;
			m_bAutoDelete = bAutoDelete;
			return (m_bLocked = ::TryEnterCriticalSection(m_pCS));
		}
		return false;
	}
	~CTryLock()
	{
		if (m_pCS)
		{
			if (m_bLocked)
				::LeaveCriticalSection(m_pCS);
			if (m_bAutoDelete)
				::DeleteCriticalSection(m_pCS);
		}
	}
};