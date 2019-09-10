// Runlog.h: interface for the CRunlog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RUNLOG_H__C589DB41_33FE_4903_A7D7_BCE9F5074877__INCLUDED_)
#define AFX_RUNLOG_H__C589DB41_33FE_4903_A7D7_BCE9F5074877__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <windows.h>
#include <TCHAR.H>

#ifdef _STD_SMARTPTR				// 使用STL的智能指针
#include <memory>
using namespace std;
//using namespace std::tr1;
#else
#include <boost/shared_ptr.hpp>
using namespace boost;
#endif

#include <string>
#include <list>
#include <algorithm>
using namespace std;
#include <process.h>
#include <MMSystem.h>
#pragma comment(lib,"winmm.lib")

// #ifndef byte
// #define byte unsigned char
// #endif

//class  CRunlog;

// #ifndef __ENABLE_RUNLOG
 #define __ENABLE_RUNLOG
// #else
// #undef  __ENABLE_RUNLOG
// #endif	
class CRunlogbase
{
public:
	CRunlogbase()
	{
		InitializeCriticalSection(&m_csFile);
		InitializeCriticalSection(&m_csRunlogSection);
		//InitializeCriticalSection(&m_csTimer);
		m_nCount = 0;
		m_hLogFile = NULL;
		timeBeginPeriod(10);
		m_nTimeID = timeSetEvent(200,200,(LPTIMECALLBACK)HandleFlush,(DWORD_PTR)this,TIME_PERIODIC);
	}
	virtual ~CRunlogbase()
	{
		//EnterCriticalSection(&m_csTimer);
		timeKillEvent(m_nTimeID);
		timeEndPeriod(10);
		Sleep(20);
		//LeaveCriticalSection(&m_csTimer);
		DeleteCriticalSection(&m_csFile);
		DeleteCriticalSection(&m_csRunlogSection);

		//TraceMsgW(L"~%s", __FUNCTION__);
		//CString strPrint;
		//strPrint.Format(L"~%s\n", __FUNCTION__);
		printf("~%s\n",__FUNCTION__);
		//OutputDebugStringW(strPrint);
	}
	
	static void CALLBACK HandleFlush(UINT wTimerID,UINT msg,DWORD dwUser, DWORD dw1, DWORD dw2)
	{
		CRunlogbase *pRunlog = (CRunlogbase *)dwUser;
		if (!pRunlog)
			return;
		pRunlog->m_nCount++;
		if (pRunlog->m_nTimeID == wTimerID &&
			(pRunlog->m_nCount + 1) %20 == 0)
		{
			pRunlog->m_nCount = 0;
			pRunlog->Flush();
		}
	}
	// 检查文件的日期,若文件的创建日期与当前日期，则创建新文件

	virtual void Flush()
	{
		if (!m_bCanlog)
			return;
		list<string>listTemp;
		::EnterCriticalSection(&m_csRunlogSection);
		if (m_listBuffer.size() > 0)
		{
			listTemp.swap(m_listBuffer);
		}
		::LeaveCriticalSection(&m_csRunlogSection);

		if (listTemp.size() > 0)
		{
			string strBuffer;
			for (list<string>::iterator it = listTemp.begin();it != listTemp.end();)
			{
				strBuffer.append((*it).data(),(*it).size());
				it = listTemp.erase(it);
			}
			EnterCriticalSection(&m_csFile);
			DWORD dwWritten = 0;
			if (m_hLogFile)
			{
				SetFilePointer(m_hLogFile,0,NULL,FILE_END);
				WriteFile(m_hLogFile,strBuffer.data(),strBuffer.size(),&dwWritten,NULL);
			}
			LeaveCriticalSection(&m_csFile);
		}
	}
protected:
	UINT m_nTimeID;
	UINT64 m_nCount;// 计数器
	list <string>m_listBuffer;
	SYSTEMTIME m_systimeCreate;		// 日志创建或更换时间
	bool m_bCanlog;
	HANDLE m_hLogFile;
	SYSTEMTIME m_tTime;
	CRITICAL_SECTION m_csFile;
	CRITICAL_SECTION m_csRunlogSection;
	//CRITICAL_SECTION m_csTimer;
	//friend class CRunlogHelper;
};

class CRunlogA:public CRunlogbase
{
public:
	CRunlogA();
	CRunlogA(LPCSTR lpszFileName);
	
	// ANSI版本
	void Runlog(LPCSTR pFormat, ...);
	// 删除RunlogEx函数，代之以Runlog宏
	//void RunlogEx(LPCSTR pFormat, ...);
	void RunlogBin(LPCSTR szTitle, unsigned char *pBinBuff, int nLen, CHAR chSeperator = ' ');
	void RunlogHuge(unsigned char *szHugeData, int nDateLen, LPCSTR pFormat, ...);
	void Runlogv(LPCSTR format, va_list ap);
	void RunlogHugev(LPCSTR szHugeText, const CHAR *format, va_list ap);	
	virtual ~CRunlogA();
private:
	CHAR	m_szFileName[MAX_PATH];	
	shared_ptr<char>m_pLogBuffer;
	// 检查文件的日期,若文件的创建日期与当前日期，则创建新文件
	void CheckDateTime();
};
class  CRunlogW:public CRunlogbase
{
public:
	CRunlogW();
	CRunlogW(LPCWSTR lpszFileName);
	SYSTEMTIME m_tTime;
	// UNICODE版本
	void Runlog(LPCWSTR pFormat, ...);
	void RunlogBin(LPCWSTR szTitle,unsigned char *pBinBuff,int nLen,WCHAR chSeperator = L' ');
	void RunlogHuge(unsigned char *szHugeData,int nDateLen,LPCWSTR pFormat,...);		
	void Runlogv(LPCWSTR format, va_list ap);
	void RunlogHugev(LPCWSTR szHugeText,const WCHAR *format, va_list ap);
	virtual ~CRunlogW();
	
private:

	WCHAR	m_szFileName[MAX_PATH];
	shared_ptr<WCHAR>m_pLogBuffer;
	// 检查文件的日期,若文件的创建日期与当前日期，则创建新文件
	void CheckDateTime();
};

#ifndef _UNICODE
typedef		CRunlogA CRunlog;
#else
typedef		CRunlogW CRunlog;
#endif

// class CRunlogHelper
// {
// public:
// 	HANDLE m_hThreadFlushLog;
// 	volatile  bool m_bThreadRun;
// 	CRITICAL_SECTION	m_csListRunlog;
// 	list<CRunlogbase *> m_ListRunlog;
// 	static UINT __stdcall ThreadFlushlog(void *p)
// 	{
// 		return  ((CRunlogHelper *)p)->Run();
// 	}
// 	UINT Run()
// 	{
// 		//timeBeginPeriod(10);
// 		DWORD dwTnow = timeGetTime();
// 		while(m_bThreadRun)
// 		{// 每200毫秒写一次日志
// 			if ((timeGetTime() - dwTnow) > 200)
// 			{
// 				FlushAlllog();
// 				dwTnow = timeGetTime();
// 			}
// 			Sleep(10);
// 		}
// 		//timeEndPeriod(10);
// 		return 0;
// 	}
// 	CRunlogHelper()
// 	{
// 		InitializeCriticalSection(&m_csListRunlog);
// 		m_bThreadRun = true;
// 		m_hThreadFlushLog = (HANDLE)_beginthreadex(NULL,0,ThreadFlushlog,this,0,0);
// 	}
// 	~CRunlogHelper()
// 	{
// 		m_bThreadRun = false;
// 		//SetThreadPriority(m_hThreadFlushLog,THREAD_PRIORITY_HIGHEST);
// 		WaitForSingleObject(m_hThreadFlushLog,200);
// 		TerminateThread(m_hThreadFlushLog,0xFFFF);
// 		FlushAlllog();
// 		m_ListRunlog.clear();
// 		DeleteCriticalSection(&m_csListRunlog);
// 	}
// 	void FlushAlllog()
// 	{
// 		EnterCriticalSection(&m_csListRunlog);
// 		for (list<CRunlogbase *>::iterator it  = m_ListRunlog.begin();
// 			it != m_ListRunlog.end(); it ++)
// 		{
// 			(*it)->Flush();
// 		}
// 		LeaveCriticalSection(&m_csListRunlog);
// 	}
// 	void Add(CRunlogbase *pRunlog)
// 	{
// 		EnterCriticalSection(&m_csListRunlog);
// 		m_ListRunlog.push_back(pRunlog);
// 		LeaveCriticalSection(&m_csListRunlog);
// 	}
// 	void Remove(CRunlogbase *pRunlog)
// 	{
// 		EnterCriticalSection(&m_csListRunlog);
// 		list<CRunlogbase*>::iterator itFind = find(m_ListRunlog.begin(),m_ListRunlog.end(),pRunlog);
// 		if (itFind != m_ListRunlog.end())
// 		{
// 			m_ListRunlog.erase(itFind);
// 		}
// 		LeaveCriticalSection(&m_csListRunlog);
// 	}
// };
// 
// typedef shared_ptr<CRunlogHelper>	CRunlogHelperPtr;
// extern CRunlogHelperPtr g_pRunlogHelperPtr;

// template <class T>
// struct CLogBufferT
// {
// private:
// 	explicit CLogBufferT()
// 	{
// 
// 	}
// 	T *pBuffer;
// 	int nBufferLen;
// 
// public:
// 	CLogBufferT(T *szBuffer,int nLength)
// 	{
// 		ZeroMemory(this,sizeof(CLogBufferT));
// 		pBuffer = new T[nLength];
// 		if (pBuffer)
// 		{
// 			memcpy(pBuffer,szBuffer,nLength);
// 			this->nBufferLen = nLength;
// 		}
// 	}
// 
// 	~CLogBufferT()
// 	{
// 		delete []pBuffer;
// 		nBufferLen = 0;
// 	}
// };
// typedef CLogBufferT<char>CLogBufferA;
// typedef CLogBufferT<WCHAR>CLogBufferW;
// 
// #ifndef _UNICODE
// typedef shared_ptr<CLogBufferA> CLogBufferPtr;
// #else
// typedef shared_ptr<CLogBufferW> CLogBufferPtr;
// #endif

void EnableRunlog(bool bEnable=true);

#endif // !defined(AFX_RUNLOG_H__C589DB41_33FE_4903_A7D7_BCE9F5074877__INCLUDED_)
