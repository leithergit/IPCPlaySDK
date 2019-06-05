#pragma once
#include <TCHAR.H>
#include <windows.h>
#include <time.h>
#include <assert.h>
#include <math.h>
//#include <thread>
//#include <chrono>
#include <MMSystem.h>
#pragma comment(lib,"winmm.lib")
#ifdef Release_D
#undef assert
#define assert	((void)0)
#endif

// using namespace std::this_thread;
// using namespace std::chrono;
#ifdef _UNICODE
#define GetDateTime			GetDateTimeW
#define	_DateTime			DateTimeW
#define UTC2DateTimeString	UTC2DateTimeStringW
#else
#define GetDateTime			GetDateTimeA
#define	_DateTime			DateTimeA
#define UTC2DateTimeString	UTC2DateTimeStringA
#endif


int		GetDateTimeA(CHAR *szDateTime,int nSize);
int		GetDateTimeW(WCHAR *szDateTime,int nSize);

bool	IsLeapYear(UINT nYear);
UINT64	DateTimeString2UTC(TCHAR *szTime,UINT64 &nTime);
void	UTC2DateTimeStringA(UINT64 nTime,CHAR *szTime,int nSize);
void	UTC2DateTimeStringW(UINT64 nTime,WCHAR *szTime,int nSize);
BOOL	SystemTime2UTC(SYSTEMTIME *pSystemTime,UINT64 *pTime);
BOOL	UTC2SystemTime(UINT64 *pTime,SYSTEMTIME *pSystemTime);

// NTP校时包
struct   NTP_Packet
{
	int			Control_Word;   
	int			root_delay;   
	int			root_dispersion;   
	int			reference_identifier;   
	__int64		reference_timestamp;   
	__int64		originate_timestamp;   
	__int64		receive_timestamp;   
	int			transmit_timestamp_seconds;   
	int			transmit_timestamp_fractions;   
};

/************************************************************************/
/* 函数说明:自动与时间服务器同步更新
/* 参数说明:无
/* 返 回 值:成功返回TRUE，失败返回FALSE
/************************************************************************/
BOOL NTPTiming(const char* szTimeServer);


#define TimeSpan(t)		(time(NULL) - (time_t)t)
#define TimeSpanEx(t)	(GetExactTime() - t)
#define MMTimeSpan(t)	(timeGetTime() - t)
typedef struct __ExactTimeBase
{
	LONGLONG	dfFreq;
	LONGLONG	dfCounter;
	time_t		nBaseClock;
	double		dfMilliseconds;
	__ExactTimeBase()
	{
		ZeroMemory(this,sizeof(ETB));
		SYSTEMTIME systime1;
		SYSTEMTIME systime2;
		ZeroMemory(&systime1, sizeof(SYSTEMTIME));
		ZeroMemory(&systime2, sizeof(SYSTEMTIME));

		HANDLE hProcess			= GetCurrentProcess();
		HANDLE hThread			= GetCurrentThread();

		DWORD dwPriorityClass	= GetPriorityClass(hProcess);		
		DWORD dwThreadPriority	= GetThreadPriority(hThread);

		DWORD dwError			= 0;
		// 把进程优先级调整到实时级
		if (!SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS))
		{
			dwError = GetLastError();
			if (!SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
			{
				dwError = GetLastError();
				if (!SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS))
				{
					dwError = GetLastError();
				}
			}
		}
		// 把线程优先级调整到实时级
		if (!SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL))
		{
			dwError = GetLastError();
			if (!SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST))
			{
				dwError = GetLastError();
				if (!SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL))
				{
					dwError = GetLastError();
				}
			}
		}
		GetSystemTime(&systime1);
		// 校准基准时钟
		while (true)
		{
			GetSystemTime(&systime2);
			if (memcmp(&systime1, &systime2, sizeof(SYSTEMTIME)) != 0)
				break;
		}
		// 恢复线程和进程的优先级
		SetThreadPriority(hThread, dwThreadPriority);
		SetPriorityClass(hProcess, dwPriorityClass);

		SystemTime2UTC(&systime2,(UINT64 *)&nBaseClock);
		dfMilliseconds = (double)(systime2.wMilliseconds /1000);

#ifdef _DEBUG
		TCHAR szText[64] = {0};
		_stprintf_s(szText,_T("BaseClock of ETB = %I64d.\n"),nBaseClock);
		OutputDebugString(szText);
#endif
		LARGE_INTEGER LarInt;		
		QueryPerformanceFrequency(&LarInt);	
		dfFreq = LarInt.QuadPart;
		QueryPerformanceCounter(&LarInt);
		dfCounter = LarInt.QuadPart;
	}

	~__ExactTimeBase()
	{
	}

}ETB;
extern ETB g_etb;
#define	InitPerformanceClock	

/// @brief 取系统精确时间,单位秒,精度为25微秒左右
double  GetExactTime();

/// @brief 线程休眠方式
struct CThreadSleep
{
	enum SleepWay
	{
		Sys_Sleep = 0,		///< 直接调用系统函数Sleep
		Wmm_Sleep = 1,		///< 使用多媒体时间提高精度
		Std_Sleep = 2		///< 使C++11提供的线程休眠函数
	};
	CThreadSleep()
	{
		double dfSumSpan1 = 0.0f, dfSumSpan2 = 0.0f, dfSumSpan3 = 0.0f;
		double dfT1 = 0.0f;
		for (int i = 0; i < 32; i++)
		{
			dfT1 = GetExactTime();
			Sleep(1);
			dfSumSpan1 += TimeSpanEx(dfT1);

			dfT1 = GetExactTime();
			timeBeginPeriod(1); //设置精度为1毫秒
			::Sleep(1); //当前线程挂起一毫秒
			timeEndPeriod(1); //结束精度设置
			dfSumSpan2 += TimeSpanEx(dfT1);

			dfT1 = GetExactTime();
			//std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			Sleep(1);
			dfSumSpan3 += TimeSpanEx(dfT1);
		}
		double dfSpanSum = dfSumSpan1;
		
		if (dfSumSpan1 <= dfSumSpan2)
		{
			nSleepWay = Sys_Sleep;
		}
		else
		{
			dfSpanSum = dfSumSpan2;
			nSleepWay = Wmm_Sleep;
		}
		if (dfSumSpan3 < dfSpanSum)
		{
			nSleepWay = Std_Sleep;
			dfSpanSum = dfSumSpan3;
		}
		nSleepPrecision = (DWORD)(round(1000 * dfSpanSum)/32);
		if (nSleepPrecision == 0)
			nSleepPrecision = 1;
	}
	void operator ()(DWORD nTimems)
	{
		switch(nSleepWay)
		{
		case Sys_Sleep:
			::Sleep(nTimems);
			break;		
		case Wmm_Sleep:
		{
			timeBeginPeriod(1); //设置精度为1毫秒
			::Sleep(nTimems);	//当前线程挂起一毫秒
			timeEndPeriod(1);	//结束精度设置
		}
			break;
		case Std_Sleep:
			//std::this_thread::sleep_for(std::chrono::nanoseconds(nTimems*1000));
			Sleep(1);
			break;
		default:
			assert(false);
			break;
		}
	}
	inline DWORD GetPrecision()
	{
		return nSleepPrecision;
	}

private:
	SleepWay	nSleepWay;
	DWORD		nSleepPrecision;
};

extern CThreadSleep ThreadSleep;
#define GetSleepPricision()	ThreadSleep.GetPrecision();
#define  SaveWaitTime()	CWaitTime WaitTime(__FILE__,__LINE__,__FUNCTION__);

class CWaitTime
{
	DWORD dwTimeEnter;
	char szFile[512];
	int nLine;
	char szFunction[256];
public:
	CWaitTime(char *szInFile, int nInLine, char *szInFunction)
	{
		dwTimeEnter = timeGetTime();
		strcpy(szFile, szInFile);
		nLine = nInLine;
		strcpy(szFunction, szInFunction);
	}
	~CWaitTime()
	{
		if ((timeGetTime() - dwTimeEnter) > 200)
		{
			char szText[1024] = { 0 };
			sprintf(szText, "Wait Timeout @File:%s %d(%s) WaitTime = %d(ms).\n", szFile, nLine, szFunction, (timeGetTime() - dwTimeEnter));
			OutputDebugStringA(szText);
		}
	}
};

/// 定时器队列，可替代多媒体定时器，但没有多媒体定时器的性能性瓶颈
class CTimerQueue
{
private:
	HANDLE m_hTimerQueue;
public:
	CTimerQueue()
	{
		m_hTimerQueue = CreateTimerQueue();
	}
	~CTimerQueue()
	{
		if (m_hTimerQueue)
		{
			DeleteTimerQueue(m_hTimerQueue);
			m_hTimerQueue = nullptr;
		}
	}

	inline HANDLE TimerQueue()
	{
		return m_hTimerQueue;
	}

	/// 创建定时器
	/// pTimerRoutine	定时器回调函数，定义为:
	///					VOID CALLBACK WaitOrTimerCallback(IN PVOID   pContext,IN bool TimerOrWaitFired);
	/// pContext		传递给定时器回调函数的参数
	/// dwDueTime		定时器成功创建后，定时器第一次响应所需的时间(单位毫秒)，若该时间为0，则定时器立即响应
	/// dwPeriod		定时器响应周期，若该值为0，则定时器只响应一次，否则每隔相应时间便响应一次，直到取消定时器
	HANDLE CreateTimer(WAITORTIMERCALLBACK pTimerRoutine, void *pContext, DWORD dwDueTime, DWORD dwPeriod)
	{
		HANDLE hTimer = nullptr;
		if (!CreateTimerQueueTimer(&hTimer, m_hTimerQueue, pTimerRoutine, pContext, dwDueTime, dwPeriod, 0))
		{
			return nullptr;
		}
		else
			return hTimer;
	}

	/// 删除定时器
	/// hTimer			定时器句柄
	/// hCompleteEvent	定时器删除成功通知事件，系统已经取消定时器时并且定时器回调函数完成时激活该事件；该参数可为NULL,
	///					取值为INVALID_HANDLE_VALUE时，DeleteTimer等待所有已运行的定时器回调函数完成后才返回；
	///					取值为NULL时，DeleteTimer将为定时器打上删除标记并立即返回，若定时器已失效，定时器回调函数将运
	///					行结束，然后不会有任何通知发出，多数调用者不会使用该选项，并且应等待定时器回调函数运行结束，然后
	///					执行其它必须的清理工作
	void DeleteTimer(HANDLE &hTimer, HANDLE hCompleteEvent = nullptr)
	{
		if (hTimer)
		{
			DeleteTimerQueueTimer(m_hTimerQueue, hTimer, hCompleteEvent);
			hTimer = nullptr;
		}
	}
};