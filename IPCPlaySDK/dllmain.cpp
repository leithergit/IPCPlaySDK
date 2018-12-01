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
CCriticalSectionProxy  g_csListPlayertoFree;

#ifdef _DEBUG
CCriticalSectionProxy g_csPlayerHandles;
UINT	g_nPlayerHandles = 0;
#endif

double	g_dfProcessLoadTime = 0.0f;
bool g_bEnableDDraw = false;

DWORD WINAPI Thread_Helper(void *);
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

		CHAR szPath[MAX_PATH] = { 0 };
		CHAR szTempPath[MAX_PATH] = { 0 };
		// 取得解码库加载路径
		GetModuleFileNameA(hModule, szTempPath, MAX_PATH);
		int nPos = StrReserverFind(szTempPath, '\\');
		strncpy_s(szPath, MAX_PATH, szTempPath, nPos);
		strcat_s(szPath, 1024, "\\RenderOption.ini");
		CHAR szValue[256] = { 0 };
		CHAR szKeyName[64] = { 0 };
		CHAR szGUID[64] = { 0 };
		int nIndex = 1;
		do 
		{
			HAccelRec HR;
			sprintf_s(szKeyName, 64, "Adapter%d", nIndex++);
			if (GetPrivateProfileStringA("HAccel", szKeyName, nullptr, szValue, 256, szPath) <= 0)
				break;
			sscanf_s(szValue, "%d,%[^;]", &HR.nMaxCount, szGUID, _countof(szGUID));
			TraceMsgA("AdapterID = %s,HAccelCount = %d.\n", szGUID, HR.nMaxCount);
			CIPCPlayer::m_MapHacceConfig.insert(pair<char*, HAccelRec>(szGUID, HR));
		} while (true);
		
		if (GetPrivateProfileInt(_T("RenderOption"), _T("EnableDDraw"), 0, szPath) > 0)
			g_bEnableDDraw = true;

		g_bThread_ClosePlayer = true;
		g_hEventThreadExit = CreateEvent(nullptr, true, false, nullptr);
		g_hThread_ClosePlayer = CreateThread(nullptr, 0, Thread_Helper, nullptr, 0, 0);
		
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

		//TraceMsgA("%s IPCPlaySDK is Unloaded.\r\n", __FUNCTION__);
		CloseHandle(g_hEventThreadExit);
		g_hEventThreadExit = nullptr;
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
DWORD WINAPI Thread_Helper(void *)
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