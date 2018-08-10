/////////////////////////////////////////////////////////////////////////
/// @file  IPCPlaySDK.cpp
/// Copyright (c) 2015, xionggao.lee
/// All rights reserved.  
/// @brief IPC播放SDK实现
/// @version 1.0  
/// @author  xionggao.lee
/// @date  2015.12.17
///   
/// 修订说明：最初版本 
/////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <list>
#include <assert.h>
#include <memory>
#include <Shlwapi.h>
#include "IPCPlaySDK.h"
#include "IPCPlayer.hpp"

CAvRegister CIPCPlayer::avRegister;
//CTimerQueue CIPCPlayer::m_TimeQueue;
CCriticalSectionProxyPtr CIPCPlayer::m_csDsoundEnum = make_shared<CCriticalSectionProxy>();
shared_ptr<CDSoundEnum> CIPCPlayer::m_pDsoundEnum = nullptr;/*= make_shared<CDSoundEnum>()*/;	///< 音频设备枚举器
#ifdef _DEBUG
int	CIPCPlayer::m_nGloabalCount = 0;
CCriticalSectionProxyPtr CIPCPlayer::m_pCSGlobalCount = make_shared<CCriticalSectionProxy>();
#endif

#ifdef _DEBUG
extern CCriticalSectionProxy g_csPlayerHandles;
extern UINT	g_nPlayerHandles;
#endif
//shared_ptr<CDSound> CDvoPlayer::m_pDsPlayer = make_shared<CDSound>(nullptr);
shared_ptr<CSimpleWnd> CIPCPlayer::m_pWndDxInit = make_shared<CSimpleWnd>();	///< 视频显示时，用以初始化DirectX的隐藏窗口对象

using namespace std;
using namespace std::tr1;

//IPC IPC 返回流包头
struct IPC_StreamHeader
{
	UINT		    chn;	            //视频输入通道号，取值0~MAX-1.
	UINT		    stream;             //码流编号：0:主码流，1：子码流，2：第三码流。
	UINT		    frame_type;         //帧类型，范围参考APP_NET_TCP_STREAM_TYPE
	UINT		    frame_num;          //帧序号，范围0~0xFFFFFFFF.
	UINT		    sec;	            //时间戳,单位：秒，为1970年以来的秒数。
	UINT		    usec;               //时间戳,单位：微秒。
	UINT		    rev[2];
};

///	@brief			用于播放IPC私有格式的录像文件
///	@param [in]		szFileName		要播放的文件名
///	@param [in]		hWnd			显示图像的窗口
/// @param [in]		pPlayCallBack	播放时的回调函数指针
/// @param [in]		pUserPtr		供pPlayCallBack返回的用户自定义指针
/// @param [in]		szLogFile		日志文件名,若为null，则不开启日志
///	@return			若操作成功，返回一个IPC_PLAYHANDLE类型的播放句柄，所有后续播
///	放函数都要使用些接口，若操作失败则返回NULL,错误原因可参考
///	GetLastError的返回值
IPCPLAYSDK_API IPC_PLAYHANDLE	ipcplay_OpenFileA(IN HWND hWnd, IN char *szFileName, FilePlayProc pPlayCallBack, void *pUserPtr,char *szLogFile)
{
	if (!szFileName)
	{
		SetLastError(IPC_Error_InvalidParameters);
		return nullptr;
	}
	if (!PathFileExistsA(szFileName))
	{
		SetLastError(ERROR_FILE_NOT_FOUND);
		return nullptr;
	}
	CIPCPlayer *pPlayer = nullptr;
	try
	{
		CIPCPlayer *pPlayer = _New CIPCPlayer(hWnd, szFileName, szLogFile);
		if (pPlayer)
		{
			pPlayer->SetCallBack(FilePlayer, pPlayCallBack, pUserPtr);
#if _DEBUG
			g_csPlayerHandles.Lock();
			g_nPlayerHandles++;
			g_csPlayerHandles.Unlock();
#endif
			return pPlayer;
		}
		else
			return nullptr;
	}
	catch (std::exception &e)
	{
		if (pPlayer)
			delete pPlayer;
		return nullptr;
	}
}

///	@brief			用于播放IPC私有格式的录像文件
///	@param [in]		szFileName		要播放的文件名
///	@param [in]		hWnd			显示图像的窗口
/// @param [in]		pPlayCallBack	播放时的回调函数指针
/// @param [in]		pUserPtr		供pPlayCallBack返回的用户自定义指针
/// @param [in]		szLogFile		日志文件名,若为null，则不开启日志
///	@return			若操作成功，返回一个IPC_PLAYHANDLE类型的播放句柄，所有后续播
///	放函数都要使用些接口，若操作失败则返回NULL,错误原因可参考
///	GetLastError的返回值
IPCPLAYSDK_API IPC_PLAYHANDLE	ipcplay_OpenFileW(IN HWND hWnd, IN WCHAR *szFileName, FilePlayProc pPlayCallBack, void *pUserPtr, char *szLogFile)
{
	if (!szFileName || !PathFileExistsW(szFileName))
		return nullptr;
	char szFilenameA[MAX_PATH] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, szFileName, -1, szFilenameA, MAX_PATH, NULL, NULL);
	return ipcplay_OpenFileA(hWnd, szFilenameA, pPlayCallBack,pUserPtr,szLogFile);
}

///	@brief			初始化流播放句柄,仅用于流播放
///	@param [in]		hWnd			显示图像的窗口
/// @param [in]		nBufferSize	流播放时允许最大视频帧数缓存数量,该值必须大于0，否则将返回null
/// @param [in]		szLogFile		日志文件名,若为null，则不开启日志
///	@return			若操作成功，返回一个IPC_PLAYHANDLE类型的播放句柄，所有后续播
///	放函数都要使用些接口，若操作失败则返回NULL,错误原因可参考GetLastError的返回值
IPCPLAYSDK_API IPC_PLAYHANDLE	ipcplay_OpenRTStream(IN HWND hWnd, IN int nBufferSize , char *szLogFile)
{
	if ((hWnd && !IsWindow(hWnd))/* || !szStreamHeader || !nHeaderSize*/)
		return nullptr;
	if (0 == nBufferSize)
	{
		SetLastError(IPC_Error_InvalidCacheSize);
		return nullptr;
	}
	try
	{
		CIPCPlayer *pPlayer = _New CIPCPlayer(hWnd, nBufferSize, szLogFile);
		if (!pPlayer)
			return nullptr;
		if (szLogFile)
			TraceMsgA("%s %s.\n", __FUNCTION__, szLogFile);
#if _DEBUG
		g_csPlayerHandles.Lock();
		g_nPlayerHandles++;
		g_csPlayerHandles.Unlock();
#endif
		return pPlayer;
	}
	catch (...)
	{
		return nullptr;
	}
}

///	@brief			初始化流播放句柄,仅用于流播放
/// @param [in]		hUserHandle		网络连接句柄
///	@param [in]		hWnd			显示图像的窗口
/// @param [in]		nMaxFramesCache	流播放时允许最大视频帧数缓存数量
/// @param [in]		szLogFile		日志文件名,若为null，则不开启日志
///	@return			若操作成功，返回一个IPC_PLAYHANDLE类型的播放句柄，所有后续播
///	放函数都要使用些接口，若操作失败则返回NULL,错误原因可参考GetLastError的返回值
IPCPLAYSDK_API IPC_PLAYHANDLE	ipcplay_OpenStream(IN HWND hWnd, byte *szStreamHeader, int nHeaderSize, IN int nMaxFramesCache, char *szLogFile)
{
 	if ((hWnd && !IsWindow(hWnd))/* || !szStreamHeader || !nHeaderSize*/)
 		return nullptr;
	if (0 == nMaxFramesCache)
	{
		SetLastError(IPC_Error_InvalidCacheSize);
		return nullptr;
	}
	CIPCPlayer *pPlayer = _New CIPCPlayer(hWnd,nullptr,szLogFile);
	if (!pPlayer)
		return nullptr;
	if (szLogFile)
		TraceMsgA("%s %s.\n", __FUNCTION__, szLogFile);
	if (szStreamHeader && nHeaderSize)
	{
		pPlayer->SetMaxFrameCache(nMaxFramesCache);
		int nDvoError = pPlayer->SetStreamHeader((CHAR *)szStreamHeader, nHeaderSize);
		if (nDvoError == IPC_Succeed)
		{
#if _DEBUG
			g_csPlayerHandles.Lock();
			g_nPlayerHandles++;
			g_csPlayerHandles.Unlock();
#endif
			return pPlayer;
		}
		else
		{
			SetLastError(nDvoError);
			delete pPlayer;
			return nullptr;
		}
	}
	else
	{
#if _DEBUG
		g_csPlayerHandles.Lock();
		g_nPlayerHandles++;
		g_csPlayerHandles.Unlock();
#endif
		return pPlayer;
	}
}

IPCPLAYSDK_API int	ipcplay_SetStreamHeader(IN IPC_PLAYHANDLE hPlayHandle, byte *szStreamHeader, int nHeaderSize)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->SetStreamHeader((CHAR *)szStreamHeader, nHeaderSize);
}

/// @brief			关闭播放句柄
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		bAsyncClose		是否启异步关闭
///	-# true			刷新画面以背景色填充
///	-# false			不刷新画面,保留最后一帧的图像
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @remark			关闭播放句柄会导致播放进度完全终止，相关内存全部被释放,要再度播放必须重新打开文件或流数据
IPCPLAYSDK_API int ipcplay_Close(IN IPC_PLAYHANDLE hPlayHandle, bool bAsyncClose/* = true*/)
{
	TraceFunction();
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
#ifdef _DEBUG
	if (strlen(pPlayer->m_szLogFileName) > 0)
		TraceMsgA("%s %s.\n", __FUNCTION__, pPlayer->m_szLogFileName);
	DxTraceMsg("%s DvoPlayer Object:%d.\n", __FUNCTION__, pPlayer->m_nObjIndex);
	
#endif
	if (!pPlayer->StopPlay(25))
	{
		DxTraceMsg("%s Async close IPCPlay Object:%8X.\n",__FUNCTION__,pPlayer);
		g_csListPlayertoFree.Lock();
		g_listPlayerAsyncClose.push_back(hPlayHandle);
		g_csListPlayertoFree.Unlock();
	}
	else
	{
#ifdef _DEBUG
		g_csPlayerHandles.Lock();
		g_nPlayerHandles--;
		g_csPlayerHandles.Unlock();
#endif
		delete pPlayer;
	}

	return 0;
}

/// @brief			开启运行日志
/// @param			szLogFile		日志文件名,此参数为NULL时，则关闭日志
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @remark			默认情况下，不会开启日志,调用此函数后会开启日志，再次调用时则会关闭日志
IPCPLAYSDK_API int	EnableLog(IN IPC_PLAYHANDLE hPlayHandle, char *szLogFile)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->EnableRunlog(szLogFile);
	return 0;
}

IPCPLAYSDK_API int ipcplay_SetBorderRect(IN IPC_PLAYHANDLE hPlayHandle,HWND hWnd, LPRECT pRectBorder,bool bPercent)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	if (!pRectBorder || (pRectBorder->left < 0 || pRectBorder->right < 0 || pRectBorder->top < 0 || pRectBorder->bottom < 0))
		return IPC_Error_InvalidParameters;
	return pPlayer->SetBorderRect(hWnd, pRectBorder, bPercent);
}

IPCPLAYSDK_API int ipcplay_RemoveBorderRect(IN IPC_PLAYHANDLE hPlayHandle,HWND hWnd)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->RemoveBorderRect(hWnd);
	return IPC_Succeed;
}

IPCPLAYSDK_API int ipcplay_AddWindow(IN IPC_PLAYHANDLE hPlayHandle, HWND hRenderWnd, LPRECT pRectRender,bool bPercent)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->AddRenderWindow(hRenderWnd, pRectRender, bPercent);
}

IPCPLAYSDK_API int ipcplay_RemoveWindow(IN IPC_PLAYHANDLE hPlayHandle, HWND hRenderWnd)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->RemoveRenderWindow(hRenderWnd);
}

IPCPLAYSDK_API int  ipcplay_GetRenderWindows(IN IPC_PLAYHANDLE hPlayHandle, INOUT HWND* hWndArray, INOUT int& nCount)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->GetRenderWindows(hWndArray, nCount);
}

/// @brief			输入流数据
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @retval			0	操作成功
/// @retval			1	流缓冲区已满
/// @retval			-1	输入参数无效
/// @remark			播放流数据时，相应的帧数据其实并未立即播放，而是被放了播放队列中，应该根据ipcplay_     
///					的返回值来判断，是否继续播放，若说明队列已满，则应该暂停播放
IPCPLAYSDK_API int ipcplay_InputStream(IN IPC_PLAYHANDLE hPlayHandle, unsigned char *szFrameData, int nFrameSize)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->InputStream(szFrameData, nFrameSize,0);
}

/// @brief			输入流相机实时码流
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @retval			0	操作成功
/// @retval			1	流缓冲区已满
/// @retval			-1	输入参数无效
/// @remark			播放流数据时，相应的帧数据其实并未立即播放，而是被放了播放队列中，应该根据ipcplay_PlayStream
///					的返回值来判断，是否继续播放，若说明队列已满，则应该暂停播放
IPCPLAYSDK_API int ipcplay_InputIPCStream(IN IPC_PLAYHANDLE hPlayHandle, IN byte *pFrameData, IN int nFrameType, IN int nFrameLength, int nFrameNum, time_t nFrameTime)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
	{
		assert(false);
		return IPC_Error_InvalidParameters;
	}
	
	return pPlayer->InputStream(pFrameData,nFrameType,nFrameLength,nFrameNum,nFrameTime);
}

/// @brief			开始播放
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
///	@param [in]		bEnableAudio	是否播放音频
///	-# true			播放声音
///	-# false		关闭声音
/// @param [in]		bEnableHaccel	是否开启硬解码
/// #- true			开启硬解码功能
/// #- false		关闭硬解码功能
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @remark			当开启硬解码，而显卡不支持对应的视频编码的解码时，会自动切换到软件解码的状态,可通过
///					ipcplay_GetHaccelStatus判断是否已经开启硬解码
IPCPLAYSDK_API int ipcplay_Start(IN IPC_PLAYHANDLE hPlayHandle, 
									IN bool bEnableAudio/* = false*/, 
									IN bool bFitWindow /*= true*/, 
									IN bool bEnableHaccel/* = false*/)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->StartPlay(bEnableAudio, bEnableHaccel,bFitWindow);
}

/// @brief			开始同步播放
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		bFitWindow		视频是否适应窗口
/// @param [in]		pSyncSource		同步源，为另一IPCPlaySDK 句柄，若同步源为null,则创建同步时钟，自我同步
/// @param [in]		nVideoFPS		视频帧率
/// #- true			视频填满窗口,这样会把图像拉伸,可能会造成图像变形
/// #- false		只按图像原始比例在窗口中显示,超出比例部分,则以黑色背景显示
/// @retval			0	操作成功
/// @retval			1	流缓冲区已满
/// @retval			-1	输入参数无效
/// @remark			若pSyncSource为null,当前的播放器成为同步源，nVideoFPS不能为0，否则返回IPC_Error_InvalidParameters错误
///					若pSyncSource不为null，则当前播放器以pSyncSource为同步源，nVideoFPS值被忽略
int ipcplay_StartAsyncPlay(IN IPC_PLAYHANDLE hPlayHandle, bool bFitWindow , void *pSyncSource, int nVideoFPS)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;

	CIPCPlayer *pSyncPlayer = (CIPCPlayer *)pSyncSource;
	if (pSyncPlayer)
	{
		if (pSyncPlayer->nSize != sizeof(CIPCPlayer))
			return IPC_Error_InvalidParameters;
	}

	return pPlayer->StartAsyncPlay(bFitWindow, pSyncPlayer, nVideoFPS);
}

/// @brief			设置解码延时
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
///	@param [in]		nDecodeDelay	解码延时，单位为ms
///	-# -1			使用默认延时
///	-# 0			无延时
/// -# n			其它延时
IPCPLAYSDK_API void ipcplay_SetDecodeDelay(IPC_PLAYHANDLE hPlayHandle, int nDecodeDelay)
{
	if (!hPlayHandle)
		return ;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return ;
	pPlayer->SetDecodeDelay(nDecodeDelay);
}
/// @brief			判断播放器是否正在播放中
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @retval			true	播放器正在播放中
/// @retval			false	插放器已停止播放
IPCPLAYSDK_API bool ipcplay_IsPlaying(IN IPC_PLAYHANDLE hPlayHandle)
{
	if (!hPlayHandle)
		return false;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return false;
	return pPlayer->IsPlaying();
}
/// @brief 复位播放器,在窗口大小变化较大或要切换播放窗口时，建议复位播放器，否则画面质量可能会严重下降
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		hWnd			显示视频的窗口
/// @param [in]		nWidth			窗口宽度,该参数暂未使用,可设为0
/// @param [in]		nHeight			窗口高度,该参数暂未使用,可设为0
IPCPLAYSDK_API int  ipcplay_Reset(IN IPC_PLAYHANDLE hPlayHandle, HWND hWnd, int nWidth , int nHeight)
{
// 	if (!hPlayHandle)
// 		return IPC_Error_InvalidParameters;
// 	CDvoPlayer *pPlayer = (CDvoPlayer *)hPlayHandle;
// 	if (pPlayer->nSize != sizeof(CDvoPlayer))
// 		return IPC_Error_InvalidParameters;
// 	if (pPlayer->Reset(hWnd, nWidth, nHeight))
		return IPC_Succeed;
//	else
//		return IPC_Error_DxError;
}

/// @brief			使视频适应窗口
/// @param [in]		bFitWindow		视频是否适应窗口
/// #- true			视频填满窗口,这样会把图像拉伸,可能会造成图像变形
/// #- false		只按图像原始比例在窗口中显示,超出比例部分,则以原始背景显示
IPCPLAYSDK_API int ipcplay_FitWindow(IN IPC_PLAYHANDLE hPlayHandle, bool bFitWindow )
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->FitWindow(bFitWindow);
	return IPC_Succeed;
}

/// @brief			停止播放
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		bStopAsync		是否异步关闭
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
IPCPLAYSDK_API int ipcplay_Stop(IN IPC_PLAYHANDLE hPlayHandle,bool bStopAsync )
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->StopPlay(bStopAsync);
	return IPC_Succeed;
}

/// @brief			暂停文件播放
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @remark			这是一个开关函数，若当前句柄已经处于播放状态，首次调用ipcplay_Pause时，会播放进度则会暂停
///					再次调用时，则会再度播放
IPCPLAYSDK_API int ipcplay_Pause(IN IPC_PLAYHANDLE hPlayHandle)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->Pause();
	return IPC_Succeed;
}

/// @brief			清空播放缓存
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
IPCPLAYSDK_API int ipcplay_ClearCache(IN IPC_PLAYHANDLE hPlayHandle)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->ClearFrameCache();
	return IPC_Succeed;
}


/// @brief			开启硬解码功能
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		bEnableHaccel	是否开启硬解码功能
/// #- true			开启硬解码功能
/// #- false		关闭硬解码功能
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @remark			开启和关闭硬解码功能必须要ipcplay_Start之前调用才能生效
IPCPLAYSDK_API int  ipcplay_EnableHaccel(IN IPC_PLAYHANDLE hPlayHandle, IN bool bEnableHaccel/* = false*/)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->EnableHaccel(bEnableHaccel);
}

/// @brief			获取硬解码状态
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [out]	bEnableHaccel	返回当前hPlayHandle是否已开启硬解码功能
/// #- true			已开启硬解码功能
/// #- false		未开启硬解码功能
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
IPCPLAYSDK_API int  ipcplay_GetHaccelStatus(IN IPC_PLAYHANDLE hPlayHandle, OUT bool &bEnableHaccel)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	bEnableHaccel = pPlayer->GetHaccelStatus();
	return IPC_Succeed;
}

/// @brief			检查是否支持特定视频编码的硬解码
/// @param [in]		nCodec		视频编码格式,@see IPC_CODEC
/// @retval			true		支持指定视频编码的硬解码
/// @retval			false		不支持指定视频编码的硬解码
IPCPLAYSDK_API bool  ipcplay_IsSupportHaccel(IN IPC_CODEC nCodec)
{
	return CIPCPlayer::IsSupportHaccel(nCodec);
}

/// @brief			取得当前播放视频的帧信息
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [out]	pFilePlayInfo	文件播放的相关信息，参见@see FilePlayInfo
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
IPCPLAYSDK_API int  ipcplay_GetPlayerInfo(IN IPC_PLAYHANDLE hPlayHandle, OUT PlayerInfo *pPlayerInfo)
{
	if (!hPlayHandle || !pPlayerInfo)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->GetPlayerInfo(pPlayerInfo);
}

/// @brief			截取正放播放的视频图像
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		szFileName		要保存的文件名
/// @param [in]		nFileFormat		保存文件的编码格式,@see SNAPSHOT_FORMAT定义
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
IPCPLAYSDK_API int  ipcplay_SnapShotW(IN IPC_PLAYHANDLE hPlayHandle, IN WCHAR *szFileName, IN SNAPSHOT_FORMAT nFileFormat/* = XIFF_JPG*/)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	CHAR szFileNameA[MAX_PATH] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, szFileName, -1, szFileNameA, MAX_PATH, NULL, NULL);
	return ipcplay_SnapShotA(hPlayHandle, szFileNameA, nFileFormat);
}

/// @brief			截取正放播放的视频图像
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		szFileName		要保存的文件名
/// @param [in]		nFileFormat		保存文件的编码格式,@see SNAPSHOT_FORMAT定义
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
IPCPLAYSDK_API int  ipcplay_SnapShotA(IN IPC_PLAYHANDLE hPlayHandle, IN CHAR *szFileName, IN SNAPSHOT_FORMAT nFileFormat/* = XIFF_JPG*/)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->SnapShot(szFileName, nFileFormat);
}

/// @brief			设置播放的音量
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		nVolume			要设置的音量值，取值范围0~100，为0时，则为静音
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
IPCPLAYSDK_API int  ipcplay_SetVolume(IN IPC_PLAYHANDLE hPlayHandle, IN int nVolume)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->SetVolume(nVolume);
	return IPC_Succeed;
}

/// @brief			取得当前播放的音量
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [out]	nVolume			当前的音量值，取值范围0~100，为0时，则为静音
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
IPCPLAYSDK_API int  ipcplay_GetVolume(IN IPC_PLAYHANDLE hPlayHandle, OUT int &nVolume)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->SetVolume(nVolume);
	return IPC_Succeed;
}

/// @brief			设置当前播放的速度的倍率
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		nPlayRate		当前的播放的倍率,大于1时为加速播放,小于1时为减速播放，不能为0或小于0
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
IPCPLAYSDK_API int  ipcplay_SetRate(IN IPC_PLAYHANDLE hPlayHandle, IN float fPlayRate)
{
	if (!hPlayHandle ||fPlayRate <= 0.0f )
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->SetRate(fPlayRate);
}

/// @brief			播放下一帧
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @retval			-24	播放器未暂停
/// @remark			该函数仅适用于单帧播放
IPCPLAYSDK_API int  ipcplay_SeekNextFrame(IN IPC_PLAYHANDLE hPlayHandle)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	if (pPlayer->IsFilePlayer())
	{
		return pPlayer->SeekNextFrame();
	}
	else
		return IPC_Error_NotFilePlayer;
}

/// @brief			跳跃到指视频帧进行播放
/// @param [in]		hPlayHandle		由ipcplay_OpenFile返回的播放句柄
/// @param [in]		nFrameID		要播放帧的起始ID
/// @param [in]		bUpdate		是否更新画面,bUpdate为true则予以更新画面,画面则不更新
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @remark			1.若所指定时间点对应帧为非关键帧，帧自动移动到就近的关键帧进行播放
///					2.若所指定帧为非关键帧，帧自动移动到就近的关键帧进行播放
///					3.只有在播放暂时,bUpdate参数才有效
IPCPLAYSDK_API int  ipcplay_SeekFrame(IN IPC_PLAYHANDLE hPlayHandle, IN int nFrameID,bool bUpdate)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	if (pPlayer->IsFilePlayer())
	{
		return pPlayer->SeekFrame(nFrameID,bUpdate);
	}
	else
		return IPC_Error_NotFilePlayer;
}

/// @brief			跳跃到指定时间偏移进行播放
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		tTimeOffset		要播放的起始时间,单位为秒
/// @param [in]		bUpdate		是否更新画面,bUpdate为true则予以更新画面,画面则不更新
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @remark			1.若所指定时间点对应帧为非关键帧，帧自动移动到就近的关键帧进行播放
///					2.若所指定帧为非关键帧，帧自动移动到就近的关键帧进行播放
///					3.只有在播放暂时,bUpdate参数才有效
IPCPLAYSDK_API int  ipcplay_SeekTime(IN IPC_PLAYHANDLE hPlayHandle, IN time_t tTimeOffset,bool bUpdate)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	if (pPlayer->IsFilePlayer())	
		return pPlayer->SeekTime(tTimeOffset, bUpdate);
	else
		return IPC_Error_NotFilePlayer;
}

/// @brief 从文件中读取一帧，读取的起点默认值为0,SeekFrame或SeekTime可设定其起点位置
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [out]	pFrameBuffer	帧数据缓冲区
/// @param [out]	nBufferSize		帧缓冲区的大小
IPCPLAYSDK_API int  ipcplay_GetFrame(IN IPC_PLAYHANDLE hPlayHandle, OUT byte **pFrameBuffer, OUT UINT &nBufferSize)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	if (pPlayer->IsFilePlayer())
		return pPlayer->GetFrame(pFrameBuffer, nBufferSize);
	else
		return IPC_Error_NotFilePlayer;
}

/// @brief			设置最大视频帧的尺寸,默认最大的视频的尺寸为256K,当视频帧大于256K时,
/// 可能会造文件读取文件错误,因此需要设置视频帧的大小,在ipcplay_Start前调用才有效
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		nMaxFrameSize	最大视频帧的尺寸
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @remark			若所指定时间点对应帧为非关键帧，帧自动移动到就近的关键帧进行播放
IPCPLAYSDK_API int  ipcplay_SetMaxFrameSize(IN IPC_PLAYHANDLE hPlayHandle, IN UINT nMaxFrameSize)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->SetMaxFrameSize(nMaxFrameSize);
}


/// @brief			取得文件播放时,支持的最大视频帧的尺寸,默认最大的视频的尺寸为256K,当视频帧
/// 大于256K时,可能会造文件读取文件错误,因此需要设置视频帧的大小,在ipcplay_Start前调用才有效
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		nMaxFrameSize	最大视频帧的尺寸
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @remark			若所指定时间点对应帧为非关键帧，帧自动移动到就近的关键帧进行播放
IPCPLAYSDK_API int  ipcplay_GetMaxFrameSize(IN IPC_PLAYHANDLE hPlayHandle, INOUT UINT &nMaxFrameSize)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	if (pPlayer->IsFilePlayer())
	{
		nMaxFrameSize = pPlayer->GetMaxFrameSize();
		return IPC_Succeed;
	}
	else
		return IPC_Error_NotFilePlayer;
}

/// @brief			播放下一帧
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @remark			该函数仅适用于单帧播放,该函数功能暂未实现
// IPCIPCPLAYSDK_API int  ipcplay_SeekNextFrame(IN IPC_PLAYHANDLE hPlayHandle)
// {
// 	return IPC_Succeed;
// }


/// @brief 设置音频播放参数
/// @param nPlayFPS		音频码流的帧率
/// @param nSampleFreq	采样频率
/// @param nSampleBit	采样位置
/// @remark 在播放音频之前，应先设置音频播放参数,SDK内部默认参数nPlayFPS = 50，nSampleFreq = 8000，nSampleBit = 16
///         若音频播放参数与SDK内部默认参数一致，可以不用设置这些参数
IPCPLAYSDK_API int  ipcplay_SetAudioPlayParameters(IN IPC_PLAYHANDLE hPlayHandle, DWORD nPlayFPS /*= 50*/, DWORD nSampleFreq/* = 8000*/, WORD nSampleBit/* = 16*/)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->SetAudioPlayParameters(nPlayFPS, nSampleFreq, nSampleBit);
	return IPC_Succeed;
}

/// @brief			开/关音频播放
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		bEnable			是否播放音频
/// -#	true		开启音频播放
/// -#	false		禁用音频播放
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
IPCPLAYSDK_API int  ipcplay_EnableAudio(IN IPC_PLAYHANDLE hPlayHandle, bool bEnable/* = true*/)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->EnableAudio(bEnable);
}

/// @brief			刷新播放窗口
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// @remark			该功能一般用于播放结束后，刷新窗口，把画面置为黑色
IPCPLAYSDK_API int  ipcplay_Refresh(IN IPC_PLAYHANDLE hPlayHandle)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->Refresh();
	return IPC_Succeed;
}

/// @brief			设置获取用户回调接口,通过此回调，用户可通过回调得到一些数据,或对得到的数据作一些处理
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		回调函数的类型 @see IPC_CALLBACK
/// @param [in]		pUserCallBack	回调函数指针
/// @param [in]		pUserPtr		用户自定义指针，在调用回调时，将会传回此指针
IPCPLAYSDK_API int ipcplay_SetCallBack(IN IPC_PLAYHANDLE hPlayHandle, IPC_CALLBACK nCallBackType, IN void *pUserCallBack, IN void *pUserPtr)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->SetCallBack(nCallBackType, pUserCallBack, pUserPtr);
	return IPC_Succeed;
}

/// @brief			设置外部绘制回调接口
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		pUserPtr		用户自定义指针，在调用回调时，将会传回此指针
IPCPLAYSDK_API int ipcplay_SetExternDrawCallBack(IN IPC_PLAYHANDLE hPlayHandle, IN void *pExternCallBack,IN void *pUserPtr)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->SetCallBack(ExternDcDraw,pExternCallBack, pUserPtr);
}

/// @brief			设置获取YUV数据回调接口,通过此回调，用户可直接获取解码后的YUV数据
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		pUserPtr		用户自定义指针，在调用回调时，将会传回此指针
IPCPLAYSDK_API int ipcplay_SetYUVCapture(IN IPC_PLAYHANDLE hPlayHandle, IN void *pCaptureYUV, IN void *pUserPtr)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->SetCallBack(YUVCapture, pCaptureYUV, pUserPtr);
}

/// @brief			设置获取YUV数据回调接口,通过此回调，用户可直接获取解码后的YUV数据
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		pUserPtr		用户自定义指针，在调用回调时，将会传回此指针
IPCPLAYSDK_API int ipcplay_SetYUVCaptureEx(IN IPC_PLAYHANDLE hPlayHandle, IN void *pCaptureYUVEx, IN void *pUserPtr)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->SetCallBack(YUVCaptureEx,pCaptureYUVEx, pUserPtr);
}

/// @brief			设置IPC私用格式录像，帧解析回调,通过此回，用户可直接获取原始的帧数据
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		pUserPtr		用户自定义指针，在调用回调时，将会传回此指针
IPCPLAYSDK_API int ipcplay_SetFrameParserCallback(IN IPC_PLAYHANDLE hPlayHandle, IN void *pCaptureFrame, IN void *pUserPtr)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;

	return	pPlayer->SetCallBack(FramePaser,pCaptureFrame, pUserPtr);
}

/// @brief			生成一个IPC录像文件头
/// @param [in,out]	pMediaHeader	由用户提供的用以接收IPC录像文件头的缓冲区
/// @param [in,out]	pHeaderSize		指定用户提供的用缓冲区的长度，若操作成功，则返回已生成的IPC录像文件头长度
/// @param [in]		nAudioCodec		音频的编码类型
/// @param [in]		nVideoCodec		视频的编译类型
/// @param [in]		nFPS			视频的帧率
/// @remark		    若pMediaHeader为NULL,则pHeaderSize只返回所需缓冲区的长度
IPCPLAYSDK_API int ipcplay_BuildMediaHeader(INOUT byte *pMediaHeader, INOUT int  *pHeaderSize,IN IPC_CODEC nAudioCodec,IN IPC_CODEC nVideoCodec,USHORT nFPS)
{
	if (!pHeaderSize)
		return IPC_Error_InvalidParameters;
	if (!pMediaHeader)
	{
		*pHeaderSize = sizeof(IPC_MEDIAINFO);
		return IPC_Succeed;
	}
	else if (*pHeaderSize < sizeof(IPC_MEDIAINFO))
		return IPC_Error_BufferSizeNotEnough;
	
	// 音频和视频编码的类型必须限定下以下范围内
	if (nAudioCodec < CODEC_G711A ||
		nAudioCodec > CODEC_AAC||
		nVideoCodec < CODEC_H264||
		nVideoCodec > CODEC_H265)
		return IPC_Error_InvalidParameters;

	IPC_MEDIAINFO *pMedia = new IPC_MEDIAINFO();
	pMedia->nCameraType	 = 0;	// 0为迪维欧相机产生的文件
	pMedia->nFps			 = (UCHAR)nFPS;	
	pMedia->nVideoCodec  = nVideoCodec;
	pMedia->nAudioCodec  = nAudioCodec;	
	memcpy(pMediaHeader, pMedia, sizeof(IPC_MEDIAINFO));
	*pHeaderSize = sizeof(IPC_MEDIAINFO);
	delete pMedia;
	return IPC_Succeed;
}

/// @brief			生成一个IPC录像帧
/// @param [in,out]	pMediaFrame		由用户提供的用以接收IPC录像帧的缓冲区
/// @param [in,out]	pFrameSize		指定用户提供的用缓冲区的长度，若操作成功，则返回已生成的IPC录像帧长度
/// @param [in,out]	nFrameID		IPC录像帧的ID，第一帧必须为0，后续帧依次递增，音频帧和视频帧必须分开计数
/// @param [in]		pIPCIpcStream	从IPC IPC得到的码流数据
/// @param [in,out]	nStreamLength	输入时为从IPC IPC得到的码流数据长度，输出时为码流数据去头后的长度,即裸码流的长度
/// @remark		    若pMediaFrame为NULL,则pFrameSize只返回IPC录像帧长度
IPCPLAYSDK_API int ipcplay_BuildFrameHeader(OUT byte *pFrameHeader, INOUT int *HeaderSize, IN int nFrameID, IN byte *pIPCIpcStream, IN int &nStreamLength)
{
	if (!pIPCIpcStream || !nStreamLength)
		return sizeof(IPCFrameHeaderEx);
	if (!pFrameHeader)
	{
		*HeaderSize = sizeof(IPCFrameHeaderEx);
		return IPC_Succeed;
	}
	else if (*HeaderSize < sizeof(IPCFrameHeaderEx))
		return IPC_Error_BufferSizeNotEnough;

	IPC_StreamHeader *pStreamHeader = (IPC_StreamHeader *)pIPCIpcStream;
	int nSizeofHeader = sizeof(IPC_StreamHeader);
	byte *pFrameData = (byte *)(pStreamHeader)+nSizeofHeader;
	int nFrameLength = nStreamLength - sizeof(IPC_StreamHeader);
	*HeaderSize = sizeof(IPCFrameHeaderEx);
	
	IPCFrameHeaderEx FrameHeader;	
	switch (pStreamHeader->frame_type)
	{
	case 0:									// 海思I帧号为0，这是固件的一个BUG，有待修正
	case IPC_IDR_FRAME: 	// IDR帧
	case IPC_I_FRAME:		// I帧
		FrameHeader.nType = FRAME_I;
		break;
	case IPC_P_FRAME:       // P帧
	case IPC_B_FRAME:       // B帧
	case IPC_711_ALAW:      // 711 A律编码帧
	case IPC_711_ULAW:      // 711 U律编码帧
	case IPC_726:           // 726编码帧
	case IPC_AAC:           // AAC编码帧
		FrameHeader.nType = pStreamHeader->frame_type;
		break;
	default:
	{
		assert(false);
		break;
	}
	}
	FrameHeader.nTimestamp	 = pStreamHeader->sec * 1000 * 1000 + pStreamHeader->usec;
	FrameHeader.nLength		 = nFrameLength;
	FrameHeader.nFrameTag	 = IPC_TAG;		///< IPC_TAG
	FrameHeader.nFrameUTCTime= time(NULL);
	FrameHeader.nFrameID	 = nFrameID;
	memcpy(pFrameHeader, &FrameHeader, sizeof(IPCFrameHeaderEx));
	nStreamLength			 = nFrameLength;
	
	return IPC_Succeed;
}

/// @brief			设置探测码流类型时，等待码流的超时值
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		dwTimeout		等待码流的赶时值，单位毫秒
/// @remark			该函数必须要在ipcplay_Start之前调用，才能生效
IPCPLAYSDK_API int ipcplay_SetProbeStreamTimeout(IN IPC_PLAYHANDLE hPlayHandle, IN DWORD dwTimeout /*= 3000*/)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->SetProbeStreamTimeout(dwTimeout);
	return IPC_Succeed;
}

/// @brief			设置图像的像素格式
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		nPixelFMT		要设置的像素格式，详见见@see PIXELFMORMAT
/// @param [in]		pUserPtr		用户自定义指针，在调用回调时，将会传回此指针
/// @remark			若要设置外部显示回调，必须把显示格式设置为R8G8B8格式
IPCPLAYSDK_API int ipcplay_SetPixFormat(IN IPC_PLAYHANDLE hPlayHandle, IN PIXELFMORMAT nPixelFMT)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->SetPixelFormat(nPixelFMT);
}

IPCPLAYSDK_API int ipcplay_SetD3dShared(IN IPC_PLAYHANDLE hPlayHandle, IN bool bD3dShared )
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->SetD3dShared(bD3dShared);
	return IPC_Succeed;
}
IPCPLAYSDK_API void ipcplay_ClearD3DCache()
{
	
}

IPCPLAYSDK_API void *AllocAvFrame()
{
	return CIPCPlayer::_AllocAvFrame();
}

IPCPLAYSDK_API void AvFree(void*p)
{
	CIPCPlayer::_AvFree(p);
}

// IPCPLAYSDK_API int ipcplay_SuspendDecode(IN IPC_PLAYHANDLE hPlayHandle)
// {
// 	if (!hPlayHandle)
// 		return IPC_Error_InvalidParameters;
// 	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
// 	if (pPlayer->nSize != sizeof(CIPCPlayer))
// 		return IPC_Error_InvalidParameters;
// 	pPlayer->SuspendDecode();
// 	return IPC_Succeed;
// }
// 
// IPCPLAYSDK_API int ipcplay_ResumeDecode(IN IPC_PLAYHANDLE hPlayHandle)
// {
// 	if (!hPlayHandle)
// 		return IPC_Error_InvalidParameters;
// 	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
// 	if (pPlayer->nSize != sizeof(CIPCPlayer))
// 		return IPC_Error_InvalidParameters;
// 	pPlayer->ResumeDecode();
// 	return IPC_Succeed;
// }

// IPCPLAYSDK_API int AddRenderWnd(IN IPC_PLAYHANDLE hPlayHandle, IN HWND hRenderWnd)
// {
// 	if (!hPlayHandle)
// 		return IPC_Error_InvalidParameters;
// 	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
// 	if (pPlayer->nSize != sizeof(CIPCPlayer))
// 		return IPC_Error_InvalidParameters;
// 	return pPlayer->AddRenderWnd(hRenderWnd,nullptr);
// }

// IPCPLAYSDK_API int RemoveRenderWnd(IN IPC_PLAYHANDLE hPlayHandle, IN HWND hRenderWnd)
// {
// 	if (!hPlayHandle)
// 		return IPC_Error_InvalidParameters;
// 	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
// 	if (pPlayer->nSize != sizeof(CIPCPlayer))
// 		return IPC_Error_InvalidParameters;
// 	return pPlayer->RemoveRenderWnd(hRenderWnd);
// }


/// @brief			设置图像旋转角度
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		nAngle			要旋转的角度值，详情请见@see RocateAngle
/// @remark	注意    目前图像旋转功能仅支持软解
IPCPLAYSDK_API int ipcplay_SetRocateAngle(IN IPC_PLAYHANDLE hPlayHandle, RocateAngle nAngle )
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->SetRocate(nAngle);
}

/// @brief		把YUV图像转换为RGB24图像
/// @param [in]		hPlayHandle	由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		pY			YUV数据Y分量指针
/// @param [in]		pU			YUV数据U分量指针
/// @param [in]		pV			YUV数据V分量指针
/// @param [in]		nStrideY	YUV数据Y分量的副长，即一行数据的长度
/// @param [in]		nStrideUV	YUV数据UV分量的副长，即一行数据的长度
/// @param [in]		nWidth		YUV图像的宽度
/// @param [in]		nHeight		YUV图像的高度
/// @param [out]    pRGBBuffer	RGB24图像的缓存
/// @param [out]	nBufferSize	RGB24图像的缓存的长度
// IPCPLAYSDK_API int ipcplay_YUV2RGB24(IN IPC_PLAYHANDLE hPlayHandle,
// 	const unsigned char* pY,
//	const unsigned char* pU,
// 	const unsigned char* pV,
/*	int nStrideY,
	int nStrideUV,
	int nWidth,
	int nHeight,
	byte **ppRGBBuffer,
	long &nBufferSize)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->YUV2RGB24(pY, pU, pV, nStrideY, nStrideUV, nWidth, nHeight, ppRGBBuffer, nBufferSize);
}
*/

/// @brief			设置一组线条坐标
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		pPointArray		线条坐标数组
/// @param [in]		nCount			pPointArray中包含线条的坐标数量
/// @param [in]		fWidth			线条宽度
/// @param [in]		nColor			线条的颜色
/// @return 操作成功时，返回线条组的句柄，否则返回0
/// @remark	注意    设置好线条坐标后,SDK内部会根据坐标信息绘制线条，一组坐标的线条的颜色是相同的，并且是相连的，若要绘制多条不相连的线条，必须多次调用ipcplay_AddLineArray
IPCPLAYSDK_API long ipcplay_AddLineArray(IN IPC_PLAYHANDLE hPlayHandle, POINT *pPointArray, int nCount, float fWidth, DWORD nColor)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->AddLineArray(pPointArray, nCount, fWidth, nColor);
}

/// @brief			移除一组线条
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		nLineIndex		由ipcplay_AddLineArray返回的线条句柄
/// @return 操作成功时返回SDK内存仍在绘制线条组的数量，否则返回-1
IPCPLAYSDK_API int  ipcplay_RemoveLineArray(IN IPC_PLAYHANDLE hPlayHandle, long nLineIndex)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->RemoveLineArray(nLineIndex);
	return IPC_Succeed;
}


/// @brief			设置背景图片路径
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		szImageFile		背景图片路径，背景图片可以jpg,png或bmp文件,为null时，则删除背景图片
IPCPLAYSDK_API int ipcplay_SetBackgroundImageA(IN IPC_PLAYHANDLE hPlayHandle, LPCSTR szImageFile)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	WCHAR szImageFileW[1024] = { 0 };
	A2WHelper(szImageFile, szImageFileW, 1024);
	pPlayer->SetBackgroundImage(szImageFileW);
	return IPC_Succeed;
}

/// @brief			设置背景图片路径
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		szImageFile		背景图片路径，背景图片可以jpg,png或bmp文件，为null时，则删除背景图片
IPCPLAYSDK_API int ipcplay_SetBackgroundImageW(IN IPC_PLAYHANDLE hPlayHandle, LPCWSTR szImageFile)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->SetBackgroundImage(szImageFile);
	return IPC_Succeed;
}

/// @brief			启用DirectDraw作为渲染器,这将禁用D3D渲染,硬解码时无法启用D3D共享模式，这交大副降低硬解码的效率
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		szImageFile		背景图片路径，背景图片可以jpg,png或bmp文件,为null时，则删除背景图片
IPCPLAYSDK_API int ipcplay_EnableDDraw(IN IPC_PLAYHANDLE hPlayHandle, bool bEnable)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->EnableDDraw(bEnable);
}

/// @brief			启用流解析器
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		nCodec			编码格式
IPCPLAYSDK_API int ipcplay_EnableStreamParser(IN IPC_PLAYHANDLE hPlayHandle, IPC_CODEC nCodec )
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->EnableStreamParser(nCodec);
}

/// @brief			输入未解析，未分帧码流
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		pData			输入的数据流
/// @param [in]		nLength			数据流尺寸
IPCPLAYSDK_API int ipcplay_InputStream2(IN IPC_PLAYHANDLE hPlayHandle, byte *pData, int nLength)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->InputStream(pData, nLength);
}

/// @brief			输入大华视频帧
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		pData			大华视频帧
IPCPLAYSDK_API int ipcplay_InputDHStream(IN IPC_PLAYHANDLE hPlayHandle, byte *pBuffer,int nLength)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->InputDHStream(pBuffer,nLength);
}
/// @brief			绘制一个实心多边形
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		pPointArray		多边形顶点坐标数组
/// @param [in]		nCount			pPointArray中包含线条的坐标数量
/// @param [in]		pVertexIndex	pPointArray中坐标的排列顺序，即绘制多边形画笔移动的顺序
/// @param [in]		nColor			多边形的颜色
/// @return 操作成功时，返回线条组的句柄，否则返回0
/// @remark	注意    设置好线条坐标后,SDK内部会根据坐标信息绘制线条，一组坐标的线条的颜色是相同的，并且是相连的，若要绘制多条不相连的线条，必须多次调用ipcplay_AddLineArray
IPCPLAYSDK_API long ipcplay_AddPolygon(IN IPC_PLAYHANDLE hPlayHandle, POINT *pPointArray, int nCount, WORD *pVertexIndex, DWORD nColor)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	return pPlayer->AddPolygon(pPointArray, nCount, pVertexIndex, nColor);
}

/// @brief			移除一个多边形
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		nLineIndex		由ipcplay_AddPolygon返回的多边形句柄
IPCPLAYSDK_API int ipcplay_RemovePolygon(IN IPC_PLAYHANDLE hPlayHandle, long nLineIndex)
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->RemovePolygon(nLineIndex);
	return IPC_Succeed;
}

IPCPLAYSDK_API int ipcplay_SetCoordinateMode(IN IPC_PLAYHANDLE hPlayHandle, int nCoordinateMode )
{
	if (!hPlayHandle)
		return IPC_Error_InvalidParameters;
	CIPCPlayer *pPlayer = (CIPCPlayer *)hPlayHandle;
	if (pPlayer->nSize != sizeof(CIPCPlayer))
		return IPC_Error_InvalidParameters;
	pPlayer->SetCoordinateMode(nCoordinateMode);
	return IPC_Succeed;
}