// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "IPCPlayer.hpp"
#include "CriticalSectionProxy.h"
#ifdef Release_D
#undef assert
#define assert	((void)0)
#endif

HANDLE g_hThread_ClosePlayer = nullptr;
HANDLE g_hEventThreadExit = nullptr;
HMODULE g_hDllModule = nullptr;
volatile bool g_bThread_ClosePlayer/* = false*/;
list<IPC_PLAYHANDLE > g_listPlayerAsyncClose;
list<IPC_PLAYHANDLE> g_listPlayer;
CCriticalSectionAgent  g_csListPlayertoFree;

#ifdef _DEBUG
CCriticalSectionAgent g_csPlayerHandles;
UINT	g_nPlayerHandles = 0;
#endif

HANDLE		g_hSharedMemory = nullptr;


SharedMemory *g_pSharedMemory = nullptr;
double	g_dfProcessLoadTime = 0.0f;
bool g_bEnableDDraw = false;
//#pragma data_seg("Shared")        // 声明共享数据段，并命名该数据段
// AdapterHAccel* g_pGlobalHaccelConfig = nullptr;
// int		g_nGlobalAdapterCount = 0;
//#pragma data_seg()
/*#pragma comment(linker,"/SECTION:Shared,RWS")*/
HANDLE  g_hHAccelMutexArray[10] = { 0 };
extern map<string, DxSurfaceList>g_DxSurfacePool;	// 用于缓存DxSurface对象
extern CCriticalSectionAgent g_csDxSurfacePool;
HANDLE g_hGlobalMutex = nullptr;
UINT __stdcall Thread_Helper(void *);
bool CreateShareMemory();
void ReleaseShareMemory();
bool OpenShareMemory();
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		g_hDllModule = hModule;
		g_dfProcessLoadTime = GetExactTime();
		//TraceMsgA("%s IPCPlaySDK is loaded.\r\n", __FUNCTION__);
#ifdef _DEBUG
		g_nPlayerHandles = 0;
#endif
		bool bResult = false;
		SECURITY_DESCRIPTOR sd;
		SECURITY_ATTRIBUTES sa;
		DWORD dwErrorCode = 0;
		char szMessage[1024] = { 0 };
		CHAR szPath[MAX_PATH] = { 0 };
		TCHAR szAdapterMutexName[64] = { 0 };
		bool bOpenMutex = false;
		__try
		{
			if (!OpenShareMemory())
			{
				if (!CreateShareMemory())
					__leave;
				if (!::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
				{
					TraceMsgA(_T("%s InitializeSecurityDescriptor failed,ErrorCode = %d."), __FUNCTION__, GetLastError());
					__leave;
				}
				if (!::SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
				{
					TraceMsgA(_T("%s SetSecurityDescriptorDacl failed,ErrorCode = %d."), __FUNCTION__, GetLastError());
					__leave;
				}
				sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				sa.bInheritHandle = FALSE;
				sa.lpSecurityDescriptor = &sd;
				CHAR szTempPath[MAX_PATH] = { 0 };
				// 取得解码库加载路径
				GetModuleFileNameA(hModule, szTempPath, MAX_PATH);
				int nPos = StrReserverFind(szTempPath, '\\');
				strncpy_s(szPath, MAX_PATH, szTempPath, nPos);
				strcat_s(szPath, MAX_PATH, "\\RenderOption.ini");
				CHAR szValue[256] = { 0 };
				CHAR szKeyName[64] = { 0 };
				CHAR szGUID[64] = { 0 };
				int nIndex = 1;
				int nConfigIndex = 0;
				do
				{
					/* Ini Sample:
					[HAccel]
					Adapter1 = 3, {D7B78E66-5A5B-11CF-7767-4070BDC2DA35}
					*/
					int nMaxHaccelCount = 0;
					sprintf_s(szKeyName, 64, "Adapter%d", nIndex++);
					if (GetPrivateProfileStringA("HAccel", szKeyName, nullptr, szValue, 256, szPath) <= 0)
						break;

					sscanf_s(szValue, "%d,%[^;]", &nMaxHaccelCount, szGUID, _countof(szGUID));
					TraceMsgA("AdapterID = %s,HAccelCount = %d.\n", szGUID, nMaxHaccelCount);
					_stprintf_s(szAdapterMutexName, 64, _T("Global\\%s"), szGUID);
					g_hHAccelMutexArray[nConfigIndex] = CreateMutex(&sa, FALSE, szAdapterMutexName);
					_tcscpy_s(g_pSharedMemory->HAccelArray[nConfigIndex].szAdapterGuid, 64, szGUID);
					g_pSharedMemory->HAccelArray[nConfigIndex].nMaxHaccel = nMaxHaccelCount;
					g_pSharedMemory->HAccelArray[nConfigIndex].nOpenCount = 0;
					nConfigIndex++;
				} while (true);
				g_pSharedMemory->nAdapterCount = nConfigIndex;
			}
			else
			{
				for (int i = 0; i < g_pSharedMemory->nAdapterCount; i++)
				{
					_stprintf_s(szAdapterMutexName, 64, _T("Global\\%s"), g_pSharedMemory->HAccelArray[i].szAdapterGuid);
					g_hHAccelMutexArray[i] = OpenMutex(MUTEX_ALL_ACCESS, FALSE, szAdapterMutexName);
				}
			}
			if (GetPrivateProfileInt(_T("RenderOption"), _T("EnableDDraw"), 0, szPath) > 0)
				g_bEnableDDraw = true;

			g_bThread_ClosePlayer = true;
			g_hEventThreadExit = CreateEvent(nullptr, true, false, nullptr);
			UINT nThreadID;
			g_hThread_ClosePlayer = (HANDLE)_beginthreadex(nullptr, 0, Thread_Helper, nullptr, 0, &nThreadID);
			bResult = true;
		}
		__finally
		{
			if (!bResult)
			{
				MessageBox(nullptr, szMessage, _T("IPCPlaySDK"), MB_OK | MB_ICONERROR);
				exit(-1);
			}
		}
	}
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
	{
		g_bThread_ClosePlayer = false;
		DWORD dwExitCode = 0;
		GetExitCodeThread(g_hThread_ClosePlayer, &dwExitCode);
		if (dwExitCode == STILL_ACTIVE)
			if (WaitForSingleObject(g_hEventThreadExit, 2000) == WAIT_TIMEOUT)
			{
				TraceMsgA("%s %d(%s) WaitForSingleObject Timeout.\n)", __FILE__, __LINE__, __FUNCTION__);
			}
		Sleep(20);
		GetExitCodeThread(g_hThread_ClosePlayer, &dwExitCode);
		if (dwExitCode != STILL_ACTIVE)
			TerminateThread(g_hThread_ClosePlayer, 0);
		CloseHandle(g_hThread_ClosePlayer);

		CloseHandle(g_hEventThreadExit);
		g_hEventThreadExit = nullptr;
		for (int i = 0; i < g_pSharedMemory->nAdapterCount; i++)
		{
			ReleaseMutex(g_hHAccelMutexArray[i]);
			CloseHandle(g_hHAccelMutexArray[i]);
			g_hHAccelMutexArray[i] = nullptr;
		}
		ReleaseShareMemory();
	}
		break;
	}
	return TRUE;
}

void PreventScreenSave()
{
	INPUT input;
	memset(&input, 0, sizeof(INPUT));
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	SendInput(1, &input, sizeof(INPUT));
}
UINT __stdcall Thread_Helper(void *)
{
//#ifdef _DEBUG
	DWORD dwTime = timeGetTime();
	DWORD dwLastPreventTime = timeGetTime();
//#endif
	while (g_bThread_ClosePlayer)
	{
		if ((timeGetTime() - dwLastPreventTime) > 30*1000)
		{
			PreventScreenSave();				// 30秒一次，禁止发生屏保
			dwLastPreventTime = timeGetTime();
		}
		g_csListPlayertoFree.Lock();
		if (g_listPlayerAsyncClose.size() > 0)
			g_listPlayer.swap(g_listPlayerAsyncClose);
		g_csListPlayertoFree.Unlock();
		if (g_listPlayer.size() > 0)
		{
			bool bAsync = false;
			for (auto it = g_listPlayer.begin(); it != g_listPlayer.end();)
			{
				CIPCPlayer *pPlayer = (CIPCPlayer *)(*it);
				double dfT = GetExactTime();
				if (pPlayer->StopPlay(INFINITE))
				{
					delete pPlayer;
					double dfTimespan = TimeSpanEx(dfT);
					TraceMsgA("%s Timespan for StopPlay = %.3f.\n", __FUNCTION__, dfTimespan);
					it = g_listPlayer.erase(it);
				}
				else
					it++;
			}
		}
#if _DEBUG
		if ((timeGetTime() - dwTime) >= 30000)
		{
			CIPCPlayer::m_pCSGlobalCount->Lock();			
			TraceMsgA("%s Count of CIPCPlayer Object = %d.\n", __FUNCTION__, CIPCPlayer::m_nGloabalCount);
			CIPCPlayer::m_pCSGlobalCount->Unlock();
			g_csPlayerHandles.Lock();
			TraceMsgA("%s Count of IPCPlayerHandle  = %d.\n", __FUNCTION__, g_nPlayerHandles);
			g_csPlayerHandles.Unlock();
			dwTime = timeGetTime();
		}
#endif
 		Sleep(10);
	}
	TraceMsgA("%s Exit.\n", __FUNCTION__);
	SetEvent(g_hEventThreadExit);
	return 0;
}


/************************************************************************/
/* CreateShareMemory Must called in a system service process			*/
/* and Transfered a correct entry string
/************************************************************************/
bool CreateShareMemory()
{
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	bool bResult = false;
	DWORD dwErrorCode = 0;
	__try
	{
		__try
		{
			if (!::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
			{
				TraceMsgA(_T("%s InitializeSecurityDescriptor failed,ErrorCode = %d."), __FUNCTION__, GetLastError());
				__leave;
			}
			if (!::SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
			{
				TraceMsgA(_T("%s SetSecurityDescriptorDacl failed,ErrorCode = %d."), __FUNCTION__, GetLastError());
				__leave;
			}
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.bInheritHandle = FALSE;
			sa.lpSecurityDescriptor = &sd;
			g_hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, &sa,
				PAGE_READWRITE | SEC_COMMIT,
				0,
				sizeof(SharedMemory),
				_SharedMemoryName);
			if (!g_hSharedMemory)
			{
				TraceMsgA(_T("%s CreateFileMapping failed,ErrorCode = %d."), __FUNCTION__, GetLastError());
				__leave;
			}
			g_pSharedMemory = (SharedMemory *)MapViewOfFile(g_hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			if (!g_pSharedMemory)
			{
				TraceMsgA(_T("%s MapViewOfFile failed,ErrorCode = %d."), __FUNCTION__, GetLastError());
				__leave;
			}
			
			ZeroMemory(g_pSharedMemory, sizeof(SharedMemory));
			bResult = true;
		}
		__finally
		{
			if (!bResult)
			{
				ReleaseShareMemory();
			}
		}
	}
	__except (1)
	{
		_TraceMsg(_T("%s Excpetion occured."), __FUNCTION__);
	}
	return bResult;
};


void ReleaseShareMemory()
{
	__try
	{
		if (!g_pSharedMemory)
		{
			UnmapViewOfFile(g_pSharedMemory);
			g_pSharedMemory = nullptr;
		}
		if (g_hSharedMemory)
		{
			CloseHandle(g_hSharedMemory);
			g_hSharedMemory = nullptr;
		}
	}
	__except (1)
	{
		_TraceMsg(_T("%s Exception in ReleaseShareMemory failed,ErrorCode = %d."), __FUNCTION__, GetLastError());
	}
}

bool OpenShareMemory()
{
	bool bResult = false;
	__try
	{
		__try
		{
			g_hSharedMemory = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, _SharedMemoryName);
			if (g_hSharedMemory == INVALID_HANDLE_VALUE ||
				g_hSharedMemory == NULL)
			{
				TraceMsgA(_T("%s MapViewOfFile Failed,ErrorCode = %d.\r\n"), __FUNCTION__, GetLastError());
				__leave;
			}
			g_pSharedMemory = (SharedMemory *)MapViewOfFile(g_hSharedMemory, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
			if (g_pSharedMemory == NULL)
			{
				TraceMsgA(_T("%s MapViewOfFile Failed,ErrorCode = %d.\r\n"), __FUNCTION__, GetLastError());
				__leave;
			}
			bResult = true;
			_TraceMsg(_T("Open Share memory map Succeed."));
		}
		__finally
		{
			if (!bResult)
			{
				ReleaseShareMemory();
			}
		}
	}
	__except (1)
	{
		TraceMsgA(_T("%s Exception occured.\r\n"),__FUNCTION__);
	}
	return bResult;
}

