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
// HANDLE  g_hHAccelMutexArray[10] = { 0 };
extern map<string, DxSurfaceList>g_DxSurfacePool;	// 用于缓存DxSurface对象
extern CCriticalSectionAgent g_csDxSurfacePool;
HANDLE g_hGlobalMutex = nullptr;
UINT __stdcall Thread_Helper(void *);
DWORD CreateShareMemory();
void ReleaseShareMemory();
DWORD OpenShareMemory();
char* TrimRight(char *szString)
{
	char* psz = szString;
	char* pszLast = NULL;
	while (*psz != 0)
	{
		if (*psz == 0x20 || *psz == '\t')
		{
			if (pszLast == NULL)
				pszLast = psz;
		}
		else
		{
			pszLast = NULL;
		}
		psz++;
	}

	if (pszLast != NULL)
	{
		int iLast = int(pszLast - szString);
		szString[iLast] = '\0';
	}
	return szString;
}

// Remove all leading whitespace
char *TrimLeft(char *szString)
{
	char* psz = szString;
	while (*psz == 0x20 || *psz == '\t')
	{
		psz++;
	}
	return psz;
}

// Remove all leading and trailing whitespace
char* Trim(char *szString)
{
	return TrimRight(TrimLeft(szString));
}
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
		CHAR szPath[MAX_PATH] = { 0 };
		WCHAR szAdapterMutexName[64] = { 0 };
		bool bOpenMutex = false;
		__try
		{
			g_pD3D9Helper.UpdateAdapterInfo();
			if ((dwErrorCode = OpenShareMemory())!=0 )
			{
				if ((dwErrorCode = CreateShareMemory())!= 0)
					__leave;
				if (!::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
				{
					dwErrorCode = GetLastError();
					TraceMsgA(_T("%s InitializeSecurityDescriptor failed,ErrorCode = %d."), __FUNCTION__, dwErrorCode);
					__leave;
				}
				if (!::SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
				{
					TraceMsgA(_T("%s SetSecurityDescriptorDacl failed,ErrorCode = %d."), __FUNCTION__, dwErrorCode);
					__leave;
				}
				sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				sa.bInheritHandle = FALSE;
				sa.lpSecurityDescriptor = &sd;
				CHAR szTempPath[MAX_PATH] = { 0 };
				// 取得解码库加载路径
				/*GetModuleFileNameA(hModule, szTempPath, MAX_PATH);
				int nPos = StrReserverFind(szTempPath, '\\');
				strncpy_s(szPath, MAX_PATH, szTempPath, nPos);
				strcat_s(szPath, MAX_PATH, "\\RenderOption.ini");*/
				CHAR szValue[256] = { 0 };
				CHAR szKeyName[64] = { 0 };
				WCHAR szGUID[64] = { 0 };
				int nIndex = 1;
				int nConfigIndex = 0;
				char szAdapterArray[10][64] = { 0 };
				if (g_pD3D9Helper.m_nAdapterCount > 0)
				{
					for (int nIndex = 0; nIndex < g_pD3D9Helper.m_nAdapterCount; nIndex++)
					{
						StringFromGUID2(g_pD3D9Helper.m_AdapterArray[nIndex].DeviceIdentifier, szGUID, 64);
						//CHAR szGuidA[64] = { 0 };
						//W2AHelper(szGuidW, szGuidA, 64);
						swprintf_s(szAdapterMutexName, 64, L"Global\\%s", szGUID);
						g_pSharedMemory->HAccelArray[nConfigIndex].hMutex = CreateMutexW(&sa, FALSE, szAdapterMutexName);;
						wcscpy_s(g_pSharedMemory->HAccelArray[nConfigIndex].szAdapterGuid, 64, szGUID);
						g_pSharedMemory->HAccelArray[nConfigIndex].nMaxHaccel = 0;
						g_pSharedMemory->HAccelArray[nConfigIndex].nOpenCount = 0;
					}
				}
				//do
				//{
				//	/* Ini Sample:
				//	[HAccel]
				//	Adapter1 = 3, {D7B78E66-5A5B-11CF-7767-4070BDC2DA35}
				//	*/
				//	int nMaxHaccelCount = 0;
				//	sprintf_s(szKeyName, 64, "Adapter%d", nIndex++);
				//	if (GetPrivateProfileStringA("HAccel", szKeyName, nullptr, szValue, 256, szPath) <= 0)
				//		break;
				//	sscanf_s(szValue, "%d,%[^;]", &nMaxHaccelCount, szGUID, _countof(szGUID));
				//	char *pGuid = Trim(szGUID);
				//	TraceMsgA("AdapterID = \"%s\",HAccelCount = %d.\n", pGuid, nMaxHaccelCount);
				//	_stprintf_s(szAdapterMutexName, 64, _T("Global\\%s"), pGuid);
				//	g_pSharedMemory->HAccelArray[nConfigIndex].hMutex == CreateMutex(&sa, FALSE, szAdapterMutexName);;
				//	_tcscpy_s(g_pSharedMemory->HAccelArray[nConfigIndex].szAdapterGuid, 64, pGuid);
				//	g_pSharedMemory->HAccelArray[nConfigIndex].nMaxHaccel = nMaxHaccelCount;
				//	g_pSharedMemory->HAccelArray[nConfigIndex].nOpenCount = 0;
				//	
				//	nConfigIndex++;
				//} while (true);
				
				g_pSharedMemory->nAdapterCount = g_pD3D9Helper.m_nAdapterCount;
				g_pSharedMemory->bHAccelPreferred = false;
				g_bEnableDDraw = false;
			}
			else
			{
				for (int i = 0; i < g_pSharedMemory->nAdapterCount; i++)
				{
					swprintf_s(szAdapterMutexName, 64, L"Global\\%s", g_pSharedMemory->HAccelArray[i].szAdapterGuid);
					g_pSharedMemory->HAccelArray[i].hMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, szAdapterMutexName);
				}
			}
			
			bResult = true;
		}
		__finally
		{
			g_bThread_ClosePlayer = true;
			g_hEventThreadExit = CreateEvent(nullptr, true, false, nullptr);
			UINT nThreadID;
			g_hThread_ClosePlayer = (HANDLE)_beginthreadex(nullptr, 0, Thread_Helper, nullptr, 0, &nThreadID);
			if (!bResult)
			{
				TCHAR szErrorMessage[1024] = { 0 };
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					dwErrorCode,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPSTR)szErrorMessage,
					1024,
					NULL);
				TraceMsgA("%s ERROR:%s", __FUNCTION__, szErrorMessage);
				//MessageBox(nullptr, szErrorMessage, _T("IPCPlaySDK"), MB_OK | MB_ICONERROR);
				//exit(-1);
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
		if (g_pSharedMemory)
		{
			for (int i = 0; i < g_pSharedMemory->nAdapterCount; i++)
			{
				ReleaseMutex(g_pSharedMemory->HAccelArray[i].hMutex);
				CloseHandle(g_pSharedMemory->HAccelArray[i].hMutex);
				g_pSharedMemory->HAccelArray[i].hMutex = nullptr;
			}
			ReleaseShareMemory();
		}

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
DWORD CreateShareMemory()
{
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	DWORD dwResult = 0;
	DWORD dwErrorCode = 0;
	__try
	{
		__try
		{
			if (!::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
			{
				dwResult = GetLastError();
				TraceMsgA(_T("%s InitializeSecurityDescriptor failed,ErrorCode = %d."), __FUNCTION__, GetLastError());
				__leave;
			}
			if (!::SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
			{
				dwResult = GetLastError();
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
				_IPCPlayerSharedMemory);
			if (!g_hSharedMemory)
			{
				dwResult = GetLastError();
				TraceMsgA(_T("%s CreateFileMapping failed,ErrorCode = %d."), __FUNCTION__, GetLastError());
				__leave;
			}
			g_pSharedMemory = (SharedMemory *)MapViewOfFile(g_hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			if (!g_pSharedMemory)
			{
				dwResult = GetLastError();
				TraceMsgA(_T("%s MapViewOfFile failed,ErrorCode = %d."), __FUNCTION__, GetLastError());
				__leave;
			}
			
			ZeroMemory(g_pSharedMemory, sizeof(SharedMemory));
		}
		__finally
		{
			if (dwResult)
			{
				ReleaseShareMemory();
			}
		}
	}
	__except (1)
	{
		_TraceMsg(_T("%s Excpetion occured."), __FUNCTION__);
	}
	return dwResult;
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

DWORD OpenShareMemory()
{
	DWORD dwResult = 0;
	__try
	{
		__try
		{
			g_hSharedMemory = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, _IPCPlayerSharedMemory);
			if (g_hSharedMemory == INVALID_HANDLE_VALUE ||
				g_hSharedMemory == NULL)
			{
				dwResult = GetLastError();
				TraceMsgA(_T("%s MapViewOfFile Failed,ErrorCode = %d.\r\n"), __FUNCTION__, GetLastError());
				__leave;
			}
			g_pSharedMemory = (SharedMemory *)MapViewOfFile(g_hSharedMemory, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
			if (g_pSharedMemory == NULL)
			{
				dwResult = GetLastError();
				TraceMsgA(_T("%s MapViewOfFile Failed,ErrorCode = %d.\r\n"), __FUNCTION__, GetLastError());
				__leave;
			}
			dwResult = true;
			_TraceMsg(_T("Open Share memory map Succeed."));
		}
		__finally
		{
			if (!dwResult)
			{
				ReleaseShareMemory();
			}
		}
	}
	__except (1)
	{
		TraceMsgA(_T("%s Exception occured.\r\n"),__FUNCTION__);
	}
	return dwResult;
}
