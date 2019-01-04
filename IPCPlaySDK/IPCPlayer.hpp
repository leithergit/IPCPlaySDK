#pragma once
/////////////////////////////////////////////////////////////////////////
/// @file  IPCPlayer.hpp
/// Copyright (c) 2016, xionggao.lee
/// All rights reserved.  
/// @brief IPCIPC相机播放SDK实现
/// @version 1.0  
/// @author  xionggao.lee
/// @date  2015.12.17
///   
/// 修订说明：最初版本 
/////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
//#include <vld.h>
#include "Common.h"
#include "Media.h"
#include "IPCPlaySDK.h"
#include "./DxSurface/DxSurface.h"
#include "AutoLock.h"
#include "./VideoDecoder/VideoDecoder.h"
#include "StreamParser.hpp"
#include "TimeUtility.h"
#include "Utility.h"
#include "DSoundPlayer.hpp"
#include "AudioDecoder.h"
#include "DirectDraw.h"
#include "TimeTrace.h"
#include "StreamFrame.h"
#include "Snapshot.h"
#include "YUVFrame.h"
#include "RenderUnit.h"
#include "SimpleWnd.h"
#include "MMEvent.h"
#include "Stat.h"
#include "./DxSurface/gpu_memcpy_sse4.h"
//#include "DHStream.hpp"
#include "DhStreamParser.h"
#define  Win7MajorVersion	6
class CIPCPlayer;
/// @brief 文件帧信息，用于标识每一帧在文件的位置
struct FileFrameInfo
{
	UINT	nOffset;	///< 帧数据所在的文件偏移
	UINT	nFrameSize;	///< 包含 IPCFrameHeaderEx在内的帧数据的尺寸
	bool	bIFrame;	///< 是否I帧
	time_t	tTimeStamp;
};

 
// 用于异步渲染的帧缓存
struct CAvFrame
{
private:
	CAvFrame()
	{
	}
public:
	time_t	tFrame;
	AVFrame *pFrame;
	int		nImageSize;
	byte	*pImageBuffer;
	CAvFrame(AVFrame *pSrcFrame,time_t tFrame)
	{
		ZeroMemory(this, sizeof(CAvFrame));
		pFrame = av_frame_alloc();
		char szAvError[256] = { 0 };
		int nImageSize = av_image_get_buffer_size((AVPixelFormat)pSrcFrame->format, pSrcFrame->width, pSrcFrame->height, 16);
		if (nImageSize < 0)
		{
			av_strerror(nImageSize, szAvError, 1024);
			DxTraceMsg("%s av_image_get_buffer_size failed:%s.\n", __FUNCTION__, szAvError);
			assert(false);
		}
		pImageBuffer = (byte *)_aligned_malloc(nImageSize, 16);
		if (!pImageBuffer)
		{
			DxTraceMsg("%s Alloc memory failed @%s %d.\n", __FUNCTION__, __FUNCTION__, __LINE__);
			assert(false);
			return;
		}
		// 把显示图像与YUV帧关联
		av_image_fill_arrays(pFrame->data, pFrame->linesize, pImageBuffer, (AVPixelFormat)pSrcFrame->format, pSrcFrame->width, pSrcFrame->height, 16);
		pFrame->width = pSrcFrame->width;
		pFrame->height = pSrcFrame->height;
		pFrame->format = pSrcFrame->format;
		int nAvError = av_frame_copy(pFrame, pSrcFrame);
		if (nAvError < 0)
		{
			av_strerror(nAvError, szAvError, 1024);
			DxTraceMsg("%s av_image_get_buffer_size failed:%s.\n", __FUNCTION__, szAvError);
			assert(false);
		}
		this->tFrame = tFrame;
	}
	~CAvFrame()
	{
		if (pFrame)
		{
			av_frame_unref(pFrame);
			av_free(pFrame);
			pFrame = nullptr;
		}
		if (pImageBuffer)
		{
			_aligned_free(pImageBuffer);
			pImageBuffer = nullptr;
		}
	}
};
typedef shared_ptr<CAvFrame> CAVFramePtr;


// //调试开关
// struct _DebugPin
// {
// 	bool bSwitch;
// 	double dfLastTime;
// 	_DebugPin()
// 	{
// 		bSwitch = true;
// 		dfLastTime = GetExactTime();
// 	}
// 
// };
// 
// struct _DebugSwitch
// {
// 	_DebugPin InputIpcStream;	
// 	_DebugPin DecodeSucceed;
// 	_DebugPin DecodeFailure;
// 	_DebugPin RenderSucceed;
// 	_DebugPin RenderFailure;
// };
// #ifdef _DEBUG
// #define SwitchTrace(pDebugSwitch,member) { if (pDebugSwitch && pDebugSwitch->##member##.bSwitch) if (TimeSpanEx(pDebugSwitch->##member##.dfLastTime) >= 5.0f) {	DxTraceMsg("%s %d %s.\n", __FUNCTION__, __LINE__,#member);	pDebugSwitch->##member##.dfLastTime = GetExactTime();}}
// #endif 
struct _OutputTime
{
	DWORD nDecode;
	DWORD nInputStream;
	DWORD nRender;
	DWORD nQueueSize;
	_OutputTime()
	{
		// 自动初始化所有DWORD型变量，即使后续增加变量也不用再作改动
		DWORD *pThis = (DWORD *)this;
		for (int i = 0; i < sizeof(_OutputTime) / 4; i++)
			pThis[i] = timeGetTime();
		nInputStream = 0;
	}
};

/// @brief 用于码流探测的类
struct StreamProbe
{
	StreamProbe(int nBuffSize = 256 * 1024)
	{
		ZeroMemory(this, sizeof(StreamProbe));
		pProbeBuff = (byte *)_aligned_malloc(nBuffSize, 16);
		nProbeBuffSize = nBuffSize;
	}
	~StreamProbe()
	{
		_aligned_free(pProbeBuff);
	}
	bool GetProbeStream(byte *pStream, int nStreamLength)
	{
		if (!pStream || !nStreamLength)
			return false;
		if ((nProbeDataLength + nStreamLength) > nProbeBuffSize)
			return false;
		memcpy_s(&pProbeBuff[nProbeDataLength], nProbeBuffSize - nProbeDataLength, pStream, nStreamLength);
		nProbeDataLength += nStreamLength;
		nProbeDataRemained = nProbeDataLength;
		nProbeOffset = 0;
		return true;
	}

	byte		*pProbeBuff;			///< 码流控件探测缓冲区
	int			nProbeBuffSize;			///< 探测缓冲的尺寸
	int			nProbeDataLength;		///< 探测缓冲区数居的长度
	int			nProbeDataRemained;
	AVCodecID	nProbeAvCodecID;		///< 探测到的码流ID
	int			nProbeWidth;
	int			nProbeHeight;
	int			nProbeCount;			///< 探测次数
	int			nProbeOffset;			///< 码流探测时视频帧复制时的帧内偏移
};

extern volatile bool g_bThread_ClosePlayer/* = false*/;
extern list<IPC_PLAYHANDLE > g_listPlayerAsyncClose;
extern CCriticalSectionProxy  g_csListPlayertoFree;
extern double	g_dfProcessLoadTime ;
extern HMODULE  g_hDllModule;
//extern HWND g_hSnapShotWnd;

/// IPCIPCPlay SDK主要功能实现类
struct HAccelRec
{
	HAccelRec()
	{
		ZeroMemory(this, sizeof(HAccelRec));
	}
	int		nMaxCount;		// 最大允许路数
	int		nOpenedCount;	// 已经开启路数
};

typedef shared_ptr<HAccelRec> HAccelRecPtr;
class CIPCPlayer
{
public:
	int		nSize;
private:
	list<FrameNV12Ptr>		m_listNV12;		// YUV缓存，硬解码
	list<FrameYV12Ptr>		m_listYV12;		// YUV缓存，软解码
	list<StreamFramePtr>	m_listAudioCache;///< 音频流播放帧缓冲
	list<StreamFramePtr>	m_listVideoCache;///< 视频流播放帧缓冲
	//map<HWND, CDirectDrawPtr> m_MapDDraw;
	list <RenderUnitPtr>	m_listRenderUnit;
	list <RenderWndPtr>		m_listRenderWnd;	///< 多窗口显示同一视频图像
	list<CAVFramePtr>		m_listAVFrame;		///<视频帧缓存，用于异步显示图像
public:
	static map<string, HAccelRecPtr>m_MapHacceConfig;	///不同显卡上开启硬解的路数
	static CCriticalSectionProxy m_csMapHacceConfig;
private:
		
	CCriticalSectionProxy	m_cslistRenderWnd;
	CCriticalSectionProxy	m_csAudioCache;		
	CCriticalSectionProxy	m_csBorderRect;
	CCriticalSectionProxy	m_csListRenderUnit;
	CCriticalSectionProxy	m_csParser;
	CCriticalSectionProxy	m_csSeekOffset;
	CCriticalSectionProxy	m_csCaptureYUV;
	CCriticalSectionProxy	m_csFilePlayCallBack;
	CCriticalSectionProxy	m_csCaptureYUVEx;
	CCriticalSectionProxy	m_csYUVFilter;
	CCriticalSectionProxy	m_csVideoCache;
	CCriticalSectionProxy	m_csCaptureRGB;
	CCriticalSectionProxy	m_cslistAVFrame;	// 异步渲染帧缓存锁
	CCriticalSectionProxy	m_csTimebase;
	//////////////////////////////////////////////////////////////////////////
	/// 注意：所有CCriticalSectionProxy类的对象和模板类的对象都必须定义在m_nZeroOffset
	/// 成员变量之前，否则可能会出访问错误
	//////////////////////////////////////////////////////////////////////////
	int			m_nZeroOffset;
	bool		m_bEnableDDraw = false;			// 是否启用DDRAW，启用DDRAW将禁用D3D，并且无法启用硬件黄享模式，导致解码效果下降
	UINT					m_nListAvFrameMaxSize;	///<视频帧缓存最大容量
	/************************************************************************/
	//   ********注意 ********                                                               
	//请匆移动m_nZeroOffset变量定义的位置，因为在构造函数中,为实现快速初始化，需要使用      
	//ZeroMemory对其它成员初始化，但要以此为边界避开所有STL容器类对象,因此所有容器类对象必
	//须定义在此变量之前 
	//***********************************************************************/								
	
	//CCriticalSectionProxy	m_csListYUV;
	int					m_nMaxYUVCache;	// 允许最大的YUV缓存数量
	int					m_nMaxFrameCache;///< 最大视频缓冲数量,默认值100
		///< 当m_FrameCache中的视频帧数量超过m_nMaxFrameCache时，便无法再继续输入流数
public:
	static CCriticalSectionProxyPtr m_pCSGlobalCount;
	shared_ptr<CMMEvent> m_pRenderTimer;
	CHAR		m_szLogFileName[512];
//#ifdef _DEBUG
	
	int			m_nObjIndex;			///< 当前对象的计数
	static int	m_nGloabalCount;		///< 当前进程中CIPCPlayer对象总数
private:
	DWORD		m_nLifeTime;			///< 当前对象存在时间
	_OutputTime	m_OuputTime;
//#endif
private:
	// 视频播放相关变量
	int			m_nTotalFrames;			///< 当前文件中有效视频帧的数量,仅当播放文件时有效
	int			m_nTotalTime;			///< 当前文件中播放总时长,仅当播放文件时有效
	shared_ptr<StreamProbe>m_pStreamProbe;
	/// 不再需要主窗口，所有的渲染窗口都在渲染窗口表中
	HWND		m_hRenderWnd;			///< 播放视频的窗口句柄
	
	volatile bool m_bIpcStream;			///< 输入流为IPC流
	volatile DWORD m_nProbeStreamTimeout;///< 探测码流超时间，单位毫秒
	D3DFORMAT	m_nPixelFormat;
	IPC_CODEC	m_nVideoCodec;			///< 视频编码格式 @see IPC_CODEC	
	static		CAvRegister avRegister;	
	static		CTimerQueue m_TimeQueue;///< 队列定时器
	//static shared_ptr<CDSound> m_pDsPlayer;///< DirectSound播放对象指针
	shared_ptr<CDSound> m_pDsPlayer;///< DirectSound播放对象指针
	//shared_ptr<CDSound> m_pDsPlayer;	///< DirectSound播放对象指针
	CDSoundBuffer* m_pDsBuffer;
	Coordinte		m_nCoordinate = Coordinte_Wnd;
	DxSurfaceInitInfo	m_DxInitInfo;
	CDxSurfaceEx* m_pDxSurface;			///< Direct3d Surface封装类,用于显示视频
	
	void *m_pDCCallBack = nullptr;
	void *m_pDCCallBackParam = nullptr;
  	CDirectDraw *m_pDDraw;				///< DirectDraw封装类对象，用于在xp下显示视频
	WCHAR		*m_pszBackImagePath;
	bool		m_bEnableBackImage = false;
  	shared_ptr<ImageSpace> m_pYUVImage = NULL;
// 	bool		m_bDxReset;				///< 是否重置DxSurface
// 	HWND		m_hDxReset;
	shared_ptr<CVideoDecoder>	m_pDecoder;
	
	bool		m_bRefreshWnd;			///< 停止播放时是否刷新画面
	int			m_nVideoWidth;			///< 视频宽度
	int			m_nVideoHeight;			///< 视频高度	
	int			m_nDecodeDelay;
	AVPixelFormat	m_nDecodePixelFmt;	///< 解码后的象素格式
	int			m_nFrameEplased;		///< 已经播放帧数
	int			m_nCurVideoFrame;		///< 当前正播放的视频帧ID
	time_t		m_nFirstFrameTime;		///< 文件播放或流回放的第1帧的时间
	time_t		m_tCurFrameTimeStamp;
	time_t		m_tLastFrameTime;
	USHORT		m_nPlayFPS;				///< 实际播放时帧率
	USHORT		m_nPlayFrameInterval;	///< 播放时帧间隔	
	HANDLE		m_hEventDecodeStart;	///< 视频解码已经开始事件
	int			m_nSkipFrames;			///< 跳帧表中的元素数量
	bool		m_bAsnycClose;		///< 是否启用D3D缓存
	double		m_dfLastTimeVideoPlay;	///< 前一次视频播放的时间
	double		m_dfLastTimer;
	double		m_dfTimesStart;			///< 开始播放的时间
	bool		m_bEnableHaccel;		///< 是否启用硬解码
	bool		m_bFitWindow;			///< 视频显示是否填满窗口
										///< 为true时，则把视频填满窗口,这样会把图像拉伸,可能会造成图像变形
										///< 为false时，则只按图像原始比例在窗口中显示,超出比例部分,则以黑色背景显示

	shared_ptr<RECT>m_pBorderRect;		///< 视频显示的边界，边界外的图象不予显示
	bool		m_bBorderInPencent;	///< m_pBorderRect中的各分量是否为百分比,即若left=20,则使用图像20%的宽度，若top=10,则使用图像10%的高度
	bool		m_bPlayerInialized;		///< 播放器是否已经完成初始化
	shared_ptr<PixelConvert> m_pConvert;
public:		// 实时流播放参数
	static shared_ptr<CSimpleWnd>m_pWndDxInit;///< 视频显示时，用以初始化DirectX的隐藏窗口对象
	bool		m_bProbeStream ;
	int			m_nProbeOffset ;
	volatile bool m_bAsyncRender ;
	HANDLE		m_hRenderAsyncEvent;	///< 异步渲染事件
	
	time_t		m_tSyncTimeBase;		///< 同步时间轴
	CIPCPlayer *m_pSyncPlayer;			///< 同步播放对象
	int			m_nVideoFPS;			///< 视频帧的原始帧率
	int			m_nDisplayAdapter = 0;
	
private:	// 音频播放相关变量

	IPC_CODEC	m_nAudioCodec;			///< 音频编码格式 @see IPC_CODEC
	bool		m_bEnableAudio;			///< 是否启用音频播放
	DWORD		m_dwAudioOffset;		///< 音频缓中区中的偏移地址
	HANDLE		m_hAudioFrameEvent[2];	///< 音频帧已播放事件
	DWORD		m_dwAudioBuffLength;	///< 
	int			m_nNotifyNum;			///< 音频分区段数量
	double		m_dfLastTimeAudioPlay;	///< 前一次播放音频的时间
	double		m_dfLastTimeAudioSample;///< 前一次音频采样的时间
	int			m_nAudioFrames;			///< 当前缓存中音频帧数量
	int			m_nCurAudioFrame;		///< 当前正播放的音频帧ID
	DWORD		m_nSampleFreq;			///< 音频采样频率
	WORD		m_nSampleBit;			///< 采样位宽
	WORD		m_nAudioPlayFPS;		///< 音频播放帧率

	static shared_ptr<CDSoundEnum> m_pDsoundEnum;	///< 音频设备枚举器
	static CCriticalSectionProxyPtr m_csDsoundEnum;
private:
	HANDLE		m_hThreadFileParser;	///< 解析IPC私有格式录像的线程
	HANDLE		m_hThreadDecode;		///< 视频解码和播放线程
	HANDLE		m_hThreadPlayAudio;		///< 音频解码和播放线程
	HANDLE		m_hThreadAsyncReander;	///< 异步显示线程
	HANDLE		m_hThreadStreamParser;	///< 流解析线程;
	HANDLE		m_hThreadReversePlay;	///< 逆序播放线程
	
	//HANDLE		m_hThreadGetFileSummary;///< 文件信息摘要线程
	UINT		m_nVideoCache;
	UINT		m_nAudioCache;
	//HANDLE		m_hCacheFulled;			///< 缓存在索引线程被填满的事件
	UINT		m_nHeaderFrameID;		///< 缓存中第1帧的ID
	UINT		m_nTailFrameID;			///< 级存中最后一帧的ID
	//bool		m_bThreadSummaryRun;
	bool		m_bSummaryIsReady;		///< 文件摘要信息准备完毕
	bool		m_bStopFlag;			///< 播放已停止标志，不再接收码流
	volatile bool m_bThreadParserRun;
	volatile bool m_bThreadDecodeRun;
	volatile bool m_bThreadReversePlayRun; ///<  启用逆序播放
	volatile bool m_bThreadPlayAudioRun;
	volatile bool m_bStreamParserRun = false;
	
	shared_ptr<CStreamParser> m_pStreamParser = nullptr;
	//shared_ptr<CDHStreamParser> m_pDHStreamParser = nullptr;
	shared_ptr<DhStreamParser> m_pDHStreamParser = nullptr;
#ifdef _DEBUG
	double		m_dfFirstFrameTime;
#endif
	byte*		m_pParserBuffer;		///< 数据解析缓冲区
	UINT		m_nParserBufferSize;	///< 数据解析缓冲区尺寸
	DWORD		m_nParserDataLength;	///< 数据解析缓冲区中的有效数据长度
	UINT		m_nParserOffset;		///< 数据解析缓冲区尺寸当前已经解析的偏移
			
	//volatile bool m_bThreadFileAbstractRun;
	bool		m_bPause;				///< 是否处于暂停状态
	byte		*m_pYUV;				///< YVU捕捉专用内存
	int			m_nYUVSize ;			///<
	shared_ptr<byte>m_pYUVPtr;
	// 截图操作相关句变量
	HANDLE		m_hEvnetYUVReady;		///< YUV数据就绪事件
	HANDLE		m_hEventYUVRequire;		///< YUV数据请求事件,立即把当前解码帧进行复制m_pAvFrameSnapshot
	HANDLE		m_hEventFrameCopied;	///< 截图动作已完成复制事件
	shared_ptr<CSnapshot>m_pSnapshot;
	
private:	// 文件播放相关变量
	
	HANDLE		m_hVideoFile;				///< 正在播放的文件句柄
	INT64		m_nSummaryOffset;		///< 在读取索引时获得的文件解析偏移
#ifdef _DEBUG
	bool		m_bSeekSetDetected = false;///< 是否存在跳动帧动作
#endif
	shared_ptr<IPC_MEDIAINFO>m_pMediaHeader;/// 媒体文件头
	long		m_nSDKVersion;
	IPCFrameHeader m_FirstFrame, m_LastFrame;
	UINT		m_nFrametoRead;			///< 当前将要读取的视频帧ID
	char		*m_pszFileName;			///< 正在播放的文件名,该字段仅在文件播放时间有效
	FileFrameInfo	*m_pFrameOffsetTable;///< 视频帧ID对应文件偏移表
	volatile LONGLONG m_nSeekOffset;	///< 读文件的偏移
	
	float		m_fPlayRate;			///< 当前的播放的倍率,大于1时为加速播放,小于1时为减速播放，不能为0或小于0
	int			m_nMaxFrameSize;		///< 最大I帧的大小，以字节为单位,默认值256K
	
	USHORT		m_nFileFrameInterval;	///< 文件中,视频帧的原始帧间隔
	float		m_fPlayInterval;		///< 帧播放间隔,单位毫秒
	bool		m_bFilePlayFinished;	///< 文件播放完成标志, 为true时，播放结束，为false时，则未结束
	bool		m_bPlayOneFrame;		///< 启用单帧播放
	DWORD		m_dwStartTime;
private:
// 	CaptureFrame	m_pfnCaptureFrame;
// 	void*			m_pUserCaptureFrame;

	CaptureYUV		m_pfnCaptureYUV;
	void*			m_pUserCaptureYUV;
	CaptureYUVEx	m_pfnCaptureYUVEx;
	void*			m_pUserCaptureYUVEx;
	FilePlayProc	m_pFilePlayCallBack;
	void*			m_pUserFilePlayer;
	CaptureYUVEx	m_pfnYUVFilter;
	void*			m_pUserYUVFilter;
	CaptureRGB		m_pCaptureRGB;
	void*			m_pUserCaptureRGB;
private:
	shared_ptr<CRunlog> m_pRunlog;	///< 运行日志
#define __countof(array) (sizeof(array)/sizeof(array[0]))
#pragma warning (disable:4996)
	void OutputMsg(char *pFormat, ...)
	{
		int nBuff;
		CHAR szBuffer[4096];
		va_list args;
		va_start(args, pFormat);		
		nBuff = _vsnprintf(szBuffer, __countof(szBuffer), pFormat, args);
		//::wvsprintf(szBuffer, pFormat, args);
		//assert(nBuff >=0);
//#ifdef _DEBUG
		OutputDebugStringA(szBuffer);
//#endif
		if (m_pRunlog)
			m_pRunlog->Runlog(szBuffer);
		va_end(args);
	}
public:
	
private:
	CIPCPlayer();

	LONGLONG GetSeekOffset()
	{
		CAutoLock lock(&m_csSeekOffset, false,__FILE__, __FUNCTION__, __LINE__);
		return m_nSeekOffset;
	}
	void SetSeekOffset(LONGLONG nSeekOffset)
	{
		CAutoLock lock(&m_csSeekOffset, false, __FILE__, __FUNCTION__, __LINE__);
		m_nSeekOffset = nSeekOffset;
	}
	// 超大文件寻址
	__int64 LargerFileSeek(HANDLE hFile, __int64 nOffset64, DWORD MoveMethod);

	/// @brief 判断输入帧是否为IPC私有的视频帧
	/// @param[in]	pFrameHeader	IPC私有帧结构指针详见@see IPCFrameHeaderEx
	/// @param[out] bIFrame			判断输入帧是否为I帧
	/// -# true			输入帧为I帧
	///	-# false		输入帧为其它帧
	static bool IsIPCVideoFrame(IPCFrameHeader *pFrameHeader, bool &bIFrame, int nSDKVersion);

	/// @brief 取得视频文件第一个I帧或最后一个视频帧时间
	/// @param[out]	帧时间
	/// @param[in]	是否取第一个I帧时间，若为true则取第一个I帧的时间否则取最一个视频帧时间
	/// @remark 该函数只用于从旧版的IPC录像文件中取得第一帧和最后一帧
	int GetFrame(IPCFrameHeader *pFrame, bool bFirstFrame = true);
	
	/// @brief 取得视频文件帧的ID和时间,相当于视频文件中包含的视频总帧数
	int GetLastFrameID(int &nLastFrameID);

	//shared_ptr<CSimpleWnd> m_pSimpleWnd /*= make_shared<CSimpleWnd>()*/;

	// 初始化DirectX显示组件
	bool InitizlizeDx(AVFrame *pAvFrame = nullptr);
	
	void UnInitlizeDx()
	{
		Safe_Delete(m_pDxSurface);
		Safe_Delete(m_pDDraw);
	}
#ifdef _DEBUG
	int m_nRenderFPS = 0;
	int m_nRenderFrames = 0;
	double dfTRender = 0.0f;
#endif
	/// @brief 渲染一帧
	void RenderFrame(AVFrame *pAvFrame)
	{
		if (!m_hRenderWnd)
			return;
		Autolock(m_cslistRenderWnd.Get());
		if (m_bEnableDDraw)
		{
			if (m_listRenderUnit.size() <= 0 || m_bStopFlag)
				return;
		}
		else  if (m_listRenderWnd.size() <= 0 || m_bStopFlag)
			return;
		
		lock.Unlock();
		if (pAvFrame->width != m_nVideoWidth ||
			pAvFrame->height != m_nVideoHeight)
		{
			Safe_Delete(m_pDxSurface);
			Safe_Delete(m_pDDraw);
			m_nVideoWidth = pAvFrame->width;
			m_nVideoHeight = pAvFrame->height;
			InitizlizeDx();
		}
		
		if (m_bFitWindow)
		{
			if (m_pDxSurface)
			{
				m_pDxSurface->Render(pAvFrame, m_nRocateAngle);
				Autolock(&m_cslistRenderWnd);
				if (m_listRenderWnd.size() <= 0)
					return;
				
				for (auto it = m_listRenderWnd.begin(); it != m_listRenderWnd.end(); it++)
				{
					if ((*it)->pRtBorder)
					{
						RECT rtBorder = { 0 };
						if (!(*it)->bPercent)
						{
							rtBorder.left = (*it)->pRtBorder->left;
							rtBorder.right = m_nVideoWidth - (*it)->pRtBorder->right;
							rtBorder.top = (*it)->pRtBorder->top;
							rtBorder.bottom = m_nVideoHeight - (*it)->pRtBorder->bottom;
						}
						else
						{
							rtBorder.left = (m_nVideoWidth*(*it)->pRtBorder->left) / 100;
							rtBorder.right = m_nVideoWidth - (m_nVideoWidth*(*it)->pRtBorder->right / 100);
							rtBorder.top = m_nVideoHeight*(*it)->pRtBorder->top / 100;
							rtBorder.bottom = m_nVideoHeight - (m_nVideoHeight*(*it)->pRtBorder->bottom / 100);
						}
						rtBorder.left = FFALIGN(rtBorder.left, 4);
						rtBorder.right = FFALIGN(rtBorder.right, 4);
						rtBorder.top = FFALIGN(rtBorder.top, 4);
						rtBorder.bottom = FFALIGN(rtBorder.bottom, 4);
						m_pDxSurface->Present((*it)->hRenderWnd, &rtBorder);
					}
					else
						m_pDxSurface->Present((*it)->hRenderWnd);
				}
			}
			else if (m_pDDraw)
			{
				if (pAvFrame->format == AV_PIX_FMT_DXVA2_VLD)
				{
					byte *pY = NULL , *pUV = NULL;
					int nPitch;
					LockDxvaFrame(pAvFrame, &pY, &pUV, nPitch);
					m_pYUVImage->dwLineSize[0] = nPitch;
					m_pYUVImage->pBuffer[0] = (PBYTE)pY;
					m_pYUVImage->pBuffer[1] = (PBYTE)pUV;
					UnlockDxvaFrame(pAvFrame);
				}
				else
				{
					m_pYUVImage->pBuffer[0] = (PBYTE)pAvFrame->data[0];
					m_pYUVImage->pBuffer[1] = (PBYTE)pAvFrame->data[1];
					m_pYUVImage->pBuffer[2] = (PBYTE)pAvFrame->data[2];
					m_pYUVImage->dwLineSize[0] = pAvFrame->linesize[0];
					m_pYUVImage->dwLineSize[1] = pAvFrame->linesize[1];
					m_pYUVImage->dwLineSize[2] = pAvFrame->linesize[2];
// 					RECT rtBorder;
// 					Autolock(&m_csBorderRect);
// 					if (m_pBorderRect)
// 					{
// 						CopyRect(&rtBorder, m_pBorderRect.get());
// 						lock.Unlock();
// 						m_pDDraw->Draw(*m_pYUVImage, &rtBorder, nullptr, true);
// 						Autolock1(&m_csListRenderUnit);
// 						for (auto it = m_listRenderUnit.begin(); it != m_listRenderUnit.end() && !m_bStopFlag; it++)
// 							(*it)->RenderImage(*m_pYUVImage, &rtBorder, nullptr, true);
// 					}
// 					else
// 					{
// 						lock.Unlock();
// 						m_pDDraw->Draw(*m_pYUVImage, nullptr, nullptr, true);
// 						Autolock1(&m_csListRenderUnit);
// 						for (auto it = m_listRenderUnit.begin(); it != m_listRenderUnit.end() && !m_bStopFlag; it++)
// 							(*it)->RenderImage(*m_pYUVImage, nullptr, nullptr, true);
// 					}
				}
				// RECT rtBorder;
				//m_pDDraw->Draw(*m_pYUVImage, &rtBorder, nullptr, true);
				Autolock(&m_csListRenderUnit);
				for (auto it = m_listRenderUnit.begin(); it != m_listRenderUnit.end() && !m_bStopFlag; it++)
				{
					if ((*it)->pRtBorder)
					{
						RECT rtBorder = { 0 };
						if (!(*it)->bPercent)
						{
							rtBorder.left = (*it)->pRtBorder->left;
							rtBorder.right = m_nVideoWidth - (*it)->pRtBorder->right;
							rtBorder.top = (*it)->pRtBorder->top;
							rtBorder.bottom = m_nVideoHeight - (*it)->pRtBorder->bottom;
						}
						else
						{
							rtBorder.left = (m_nVideoWidth*(*it)->pRtBorder->left) / 100;
							rtBorder.right = m_nVideoWidth - (m_nVideoWidth*(*it)->pRtBorder->right / 100);
							rtBorder.top = m_nVideoHeight*(*it)->pRtBorder->top / 100;
							rtBorder.bottom = m_nVideoHeight - (m_nVideoHeight*(*it)->pRtBorder->bottom / 100);
						}
						rtBorder.left = FFALIGN(rtBorder.left, 4);
						rtBorder.right = FFALIGN(rtBorder.right, 4);
						rtBorder.top = FFALIGN(rtBorder.top, 4);
						rtBorder.bottom = FFALIGN(rtBorder.bottom, 4);
						(*it)->RenderImage(*m_pYUVImage, &rtBorder, nullptr, true);
					}
					else
						(*it)->RenderImage(*m_pYUVImage, nullptr, nullptr, true);
				}
			}
		}
		else
		{
			RECT rtRender;
			GetWindowRect(m_hRenderWnd, &rtRender);
			int nWndWidth = rtRender.right - rtRender.left;
			int nWndHeight = rtRender.bottom - rtRender.top;
			float fScaleWnd = (float)nWndHeight / nWndWidth;
			float fScaleVideo = (float)pAvFrame->height / pAvFrame->width;
			if (fScaleVideo < fScaleWnd)			// 窗口高度超出比例,需要去掉一部分高度,视频需要上下居中
			{
				int nNewHeight = (int)nWndWidth * fScaleVideo;
				int nOverHeight = nWndHeight - nNewHeight;
				if ((float)nOverHeight / nWndHeight > 0.01f)	// 窗口高度超出1%,则调整高度，否则忽略该差异
				{
					rtRender.top += nOverHeight / 2;
					rtRender.bottom -= nOverHeight / 2;
				}
			}
			else if (fScaleVideo > fScaleWnd)		// 窗口宽度超出比例,需要去掉一部分宽度，视频需要左右居中
			{
				int nNewWidth = nWndHeight/fScaleVideo;
				int nOverWidth = nWndWidth - nNewWidth;
				if ((float)nOverWidth / nWndWidth > 0.01f)
				{
					rtRender.left += nOverWidth / 2;
					rtRender.right -= nOverWidth / 2;
				}
			}
			if (m_pDxSurface)
			{
				m_cslistRenderWnd.Lock();
				ScreenToClient(m_hRenderWnd, (LPPOINT)&rtRender);
				ScreenToClient(m_hRenderWnd, ((LPPOINT)&rtRender) + 1);
				m_cslistRenderWnd.Unlock();
				RECT rtBorder;

				if (m_pBorderRect)
				{
					CopyRect(&rtBorder, m_pBorderRect.get());
					m_pDxSurface->Render(pAvFrame,m_nRocateAngle);
					Autolock(&m_cslistRenderWnd);
					m_pDxSurface->Present(m_hRenderWnd, &rtBorder, &rtRender);
					if (m_listRenderWnd.size() > 0)
					{
						for (auto it = m_listRenderWnd.begin(); it != m_listRenderWnd.end(); it++)
							m_pDxSurface->Present((*it)->hRenderWnd, &rtBorder, &rtRender);
					}
				}
				else
				{
					m_pDxSurface->Render(pAvFrame, m_nRocateAngle);
					Autolock(&m_cslistRenderWnd);
					m_pDxSurface->Present(m_hRenderWnd, nullptr, &rtRender);
					if (m_listRenderWnd.size() > 0)
					{
						for (auto it = m_listRenderWnd.begin(); it != m_listRenderWnd.end(); it++)
							m_pDxSurface->Present((*it)->hRenderWnd, nullptr, &rtRender);
					}
				}
			}
			else if (m_pDDraw)
			{
				m_pYUVImage->pBuffer[0] = (PBYTE)pAvFrame->data[0];
				m_pYUVImage->pBuffer[1] = (PBYTE)pAvFrame->data[1];
				m_pYUVImage->pBuffer[2] = (PBYTE)pAvFrame->data[2];
				m_pYUVImage->dwLineSize[0] = pAvFrame->linesize[0];
				m_pYUVImage->dwLineSize[1] = pAvFrame->linesize[1];
				m_pYUVImage->dwLineSize[2] = pAvFrame->linesize[2];
				RECT rtBorder;
				Autolock(&m_csBorderRect);
				if (m_pBorderRect)
				{
					CopyRect(&rtBorder, m_pBorderRect.get());
					lock.Unlock();
					m_pDDraw->Draw(*m_pYUVImage, &rtBorder, &rtRender, true);
				}
				else
				{
					lock.Unlock();
					m_pDDraw->Draw(*m_pYUVImage,nullptr, &rtRender, true);
				}	
			}
		}
	}
	// 二分查找
	int BinarySearch(time_t tTime)
	{
		int low = 0;
		int high = m_nTotalFrames - 1;
		int middle = 0;
		while (low <= high)
		{
			middle = (low + high) / 2;
			if (tTime >= m_pFrameOffsetTable[middle].tTimeStamp &&
				tTime <= m_pFrameOffsetTable[middle + 1].tTimeStamp)
				return middle;
			//在左半边
			else if (tTime < m_pFrameOffsetTable[middle].tTimeStamp)
				high = middle - 1;
			//在右半边
			else if (tTime > m_pFrameOffsetTable[middle + 1].tTimeStamp)
				low = middle + 1;
		}
		//没找到
		return -1;
	}
public:
	/// @brief  启用日志
	/// @param	szLogFile		日志文件名,若该参数为null，则禁用日志
	void EnableRunlog(const char *szLogFile)
	{
		char szFileLog[MAX_PATH] = { 0 };
		if (szLogFile && strlen(szLogFile) != 0)
		{
			strcpy(szFileLog, szLogFile);
			m_pRunlog = make_shared<CRunlog>(szFileLog);
		}
		else
			m_pRunlog = nullptr;
	}

	int EnableDDraw(bool bEnableDDraw = true)
	{
		if (m_pDDraw || m_pDxSurface)
			return IPC_Error_DXRenderInitialized;
		m_bEnableDDraw = bEnableDDraw;
		return IPC_Succeed;
	}

	/// @brief 设置音频播放参数
	/// @param nPlayFPS		音频码流的帧率
	/// @param nSampleFreq	采样频率
	/// @param nSampleBit	采样位置
	/// @remark 在播放音频之前，应先设置音频播放参数,SDK内部默认参数nPlayFPS = 50，nSampleFreq = 8000，nSampleBit = 16
	///         若音频播放参数与SDK内部默认参数一致，可以不用设置这些参数
	void SetAudioPlayParameters(DWORD nPlayFPS = 50, DWORD nSampleFreq = 8000, WORD nSampleBit = 16)
	{
		m_nAudioPlayFPS	 = nPlayFPS;
		m_nSampleFreq	 = nSampleFreq;
		m_nSampleBit	 = nSampleBit;
	}
	void ClearOnException()
	{
		if (m_pszFileName)
		{
			delete[]m_pszFileName;
			m_pszFileName = nullptr;
		}
		if (m_pRunlog)
			m_pRunlog = nullptr;
		/*
		使用CCriticalSectionProxy类代理，不再直接调用DeleteCriticalSection函数
		DeleteCriticalSection(&m_csVideoCache); 
		*/
		if (m_hVideoFile)
			CloseHandle(m_hVideoFile);
	}
	CIPCPlayer(HWND hWnd, CHAR *szFileName = nullptr, char *szLogFile = nullptr);
	
	CIPCPlayer(HWND hWnd, int nBufferSize = 2048 * 1024, char *szLogFile = nullptr);

	~CIPCPlayer();
	
	/// @brief  是否为文件播放
	/// @retval			true	文件播放
	/// @retval			false	流播放
	bool IsFilePlayer()
	{
		if (m_hVideoFile || m_pszFileName)
			return true;
		else
			return false;
	}
	// 
	class WndFinder {
	public:
		WndFinder(const HWND hWnd)
		{
			this->hWnd = hWnd; 
		}
	
		bool operator()(RenderWndPtr& Value)
		{
			if (Value->hRenderWnd == hWnd)
				return true;
			else
				return false;
		}
	public:
		HWND hWnd;
	};

	class UnitFinder {
	public:
		UnitFinder(const HWND hWnd)
		{
			this->hWnd = hWnd;
		}

		bool operator()(RenderUnitPtr& Value)
		{
			if (Value->hRenderWnd == hWnd)
				return true;
			else
				return false;
		}
	public:
		HWND hWnd;
	};

	class FrameFinder {
	public:
		FrameFinder(const time_t tFrame)
		{
			this->tFrame = tFrame;
		}

		bool operator()(CAVFramePtr& Value)
		{
			if (abs(Value->tFrame - tFrame)< 40)
				return true;
			else
				return false;
		}
	public:
		time_t tFrame;
	};
	int AddRenderWindow(HWND hRenderWnd, LPRECT pRtRender, bool bPercent = false);

	int RemoveRenderWindow(HWND hRenderWnd);

	// 获取显示图像窗口的数量
	int GetRenderWindows(HWND* hWndArray, int &nSize);

	void SetRefresh(bool bRefresh = true)
	{
		m_bRefreshWnd = bRefresh;
	}

	// 设置流播放头
	// 在流播放时，播放之前，必须先设置流头
	int SetStreamHeader(CHAR *szStreamHeader, int nHeaderSize);

	int SetMaxFrameSize(int nMaxFrameSize = 256*1024)
	{
		if (m_hThreadFileParser || m_hThreadDecode)
			return IPC_Error_PlayerHasStart;
		m_nMaxFrameSize = nMaxFrameSize;
		return IPC_Succeed;
	}

	inline int GetMaxFrameSize()
	{
		return m_nMaxFrameSize;
	}

	inline void SetMaxFrameCache(int nMaxFrameCache = 100)
	{
		m_nMaxFrameCache = nMaxFrameCache;
	}

	void ClearFrameCache()
	{
		m_csVideoCache.Lock();
		m_listVideoCache.clear();
		m_csVideoCache.Lock();

		m_csAudioCache.Lock();
		m_listAudioCache.clear();
		m_csAudioCache.Lock();
	}
	
	/// @brief 设置显示边界,边界外的图像将不予以显示
	/// @param rtBorder	边界参数 详见以下图表
	/// @param bPercent 是否使用百分比控制边界
	/// left	左边界距离
	/// top		上边界距离
	/// right	右边界距离
	/// bottom  下边界距离 
	///┌─────────────────────────────────────┐
	///│                  │                  │
	///│                 top                 │
	///│                  │                  │─────── the source rect
	///│       ┌─────────────────────┐       │
	///│       │                     │       │
	///│       │                     │       │
	///│─left─ │  the clipped rect   │─right─│
	///│       │                     │       │
	///│       │                     │       │
	///│       └─────────────────────┘       │
	///│                  │                  │
	///│                bottom               │
	///│                  │                  │
	///└─────────────────────────────────────┘
	/// @remark 边界的上下左右位置不可颠倒
	int SetBorderRect(HWND hWnd, LPRECT pRectBorder, bool bPercent = false);

	void RemoveBorderRect(HWND hWnd)
	{
		Autolock(&m_cslistRenderWnd);
		auto itFind = find_if(m_listRenderWnd.begin(), m_listRenderWnd.end(), WndFinder(hWnd));
		if (itFind != m_listRenderWnd.end())
			(*itFind)->SetBorder(nullptr);
	}
	
	void SetDecodeDelay(int nDecodeDelay = -1)
	{
		if (nDecodeDelay == 0)
			EnableAudio(false);
		else
			EnableAudio(true);
		m_nDecodeDelay = nDecodeDelay;
		if (m_nDecodeDelay <= 2)
		{
			m_fPlayInterval = nDecodeDelay;
		}
	}
	/// @brief			开始播放
	/// @param [in]		bEnaleAudio		是否开启音频播放
	/// #- true			开启音频播放
	/// #- false		关闭音频播放
	/// @param [in]		bEnableHaccel	是否开启硬解码功能
	/// #- true			启用硬解码
	/// #- false		禁用硬解码
	/// @param [in]		bFitWindow		视频是否适应窗口
	/// #- true			视频填满窗口,这样会把图像拉伸,可能会造成图像变形
	/// #- false		只按图像原始比例在窗口中显示,超出比例部分,则以黑色背景显示
	/// @retval			0	操作成功
	/// @retval			1	流缓冲区已满
	/// @retval			-1	输入参数无效
	/// @remark			播放流数据时，相应的帧数据其实并未立即播放，而是被放了播放队列中，应该根据ipcplay_PlayStream
	///					的返回值来判断，是否继续播放，若说明队列已满，则应该暂停播放
	int StartPlay(bool bEnaleAudio = false, bool bEnableHaccel = false, bool bFitWindow = true);

	/// @brief			开始同步播放
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
	int StartSyncPlay(bool bFitWindow = true, CIPCPlayer *pSyncSource = nullptr, int nVideoFPS = 25);

	/// 移动播放时间点到指位位置
	/// tTime			需要到达的时间点，单位毫秒
	int SeekTime(time_t tTime)
	{
		if (!m_bAsyncRender)
			return IPC_Error_NotAsyncPlayer;
		if (m_hThreadAsyncReander)
			return IPC_Error_PlayerNotStart;
		m_tSyncTimeBase = tTime;
	}

	/// @brief			判断播放器是否正在播放中	
	/// @retval			true	播放器正在播放中
	/// @retval			false	插放器已停止播放
	bool IsPlaying()
	{
		if (m_hThreadDecode)
			return true;
		else
			return false;
	}

	/// @brief 复位播放窗口
	bool Reset(HWND hWnd = nullptr, int nWidth = 0, int nHeight = 0)
	{
// 		CAutoLock lock(&m_csDxSurface, false, __FILE__, __FUNCTION__, __LINE__);
// 		if (m_pDxSurface)
// 		{
// 			m_bDxReset = true;
// 			m_hDxReset = hWnd;
// 			return true;
// 		}
// 		else
// 			return false;
		return true;
	}

	int SetPixelFormat(PIXELFMORMAT nPixelFmt = YV12)
	{
		switch (nPixelFmt)
		{
		case YV12:
			m_nPixelFormat = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');
			break;
		case NV12:
			m_nPixelFormat = (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2');
			break;
		case R8G8B8:
			m_nPixelFormat = D3DFMT_A8R8G8B8;
			break;
		default:
			assert(false);
			return IPC_Error_InvalidParameters;
			break;
		}
		return IPC_Succeed;
	}

	int GetFileHeader();
	
	void TryEnableHAccel(CHAR* szAdapterID,int nBuffer)
	{
		HMONITOR hMonitor = MonitorFromWindow(m_hRenderWnd, MONITOR_DEFAULTTONEAREST);
		if (hMonitor)
		{
			MONITORINFOEX mi;
			mi.cbSize = sizeof(MONITORINFOEX);
			if (GetMonitorInfo(hMonitor, &mi))
			{
				for (int i = 0; i < g_pD3D9Helper.m_nAdapterCount; i++)
				{
					if (strcmp(g_pD3D9Helper.m_AdapterArray[i].DeviceName, mi.szDevice) == 0)
					{
						m_nDisplayAdapter = i;
						OutputMsg("%s Wnd[%08X] is on Monitor:[%s],it's connected on Adapter[%i]:%s.\n", __FUNCTION__, m_hRenderWnd, mi.szDevice, i, g_pD3D9Helper.m_AdapterArray[i].Description);
						WCHAR szGuidW[64] = { 0 };
						CHAR szGuidA[64] = { 0 };
						StringFromGUID2(g_pD3D9Helper.m_AdapterArray[i].DeviceIdentifier, szGuidW,64);
						
						W2AHelper(szGuidW, szGuidA, 64);
						if (szAdapterID)
							strcpy_s(szAdapterID, nBuffer, szGuidA);
						CAutoLock lock(m_csMapHacceConfig.Get());
						auto itFind = m_MapHacceConfig.find(szGuidA);
						if (itFind != m_MapHacceConfig.end())
						{
							if (itFind->second->nOpenedCount < itFind->second->nMaxCount)
							{
								m_bD3dShared = true;
								m_bEnableHaccel = true;
								itFind->second->nOpenedCount++;
								OutputMsg("%s HAccels On:Monitor:%s,Adapter:%s is %d.\n", __FUNCTION__, mi.szDevice, g_pD3D9Helper.m_AdapterArray[i].Description, itFind->second->nOpenedCount);
							}
							else
							{
								OutputMsg("%s HAccels On:Monitor:%s,Adapter:%s has reached up limit：%d.\n", __FUNCTION__, mi.szDevice, g_pD3D9Helper.m_AdapterArray[i].Description, itFind->second->nMaxCount);
							}
						}
						break;
					}
				}
			}
		}

	}
	void FitWindow(bool bFitWindow)
	{
		m_bFitWindow = bFitWindow;
	}

	/// @brief			输入IPC私格式的码流数据
	/// @retval			0	操作成功
	/// @retval			1	流缓冲区已满
	/// @retval			-1	输入参数无效
	/// @remark			播放流数据时，相应的帧数据其实并未立即播放，而是被放了播放队列中，应该根据ipcplay_PlayStream
	///					的返回值来判断，是否继续播放，若说明队列已满，则应该暂停输入
	int InputStream(unsigned char *szFrameData, int nFrameSize, UINT nCacheSize, bool bThreadInside = false/*是否内部线程调用标志*/);
	

	/// @brief			输入IPC码流数据
	/// @retval			0	操作成功
	/// @retval			1	流缓冲区已满
	/// @retval			-1	输入参数无效
	/// @remark			播放流数据时，相应的帧数据其实并未立即播放，而是被放了播放队列中，应该根据ipcplay_PlayStream
	///					的返回值来判断，是否继续播放，若说明队列已满，则应该暂停播放
// 	UINT m_nAudioFrames1 = 0;
// 	UINT m_nVideoFraems = 0;
// 	DWORD m_dwInputStream = timeGetTime();
	int InputStream(IN byte *pFrameData, IN int nFrameType, IN int nFrameLength, int nFrameNum, time_t nFrameTime);

	// 输入未解析码流
	int InputStream(IN byte *pData, IN int nLength);

	bool IsH264KeyFrame(byte *pFrame)
	{
		int RTPHeaderBytes = 0;
		if (pFrame[0] == 0 &&
			pFrame[1] == 0 &&
			pFrame[2] == 0 &&
			pFrame[3] == 1)
		{
			int nal_type = pFrame[4] & 0x1F;
			if (nal_type == 5 || nal_type == 7 || nal_type == 8 || nal_type == 2)
			{
				return true;
			}
		}

		return false;
	}

	int InputDHStream(byte *pBuffer, int nLength);
	
	bool StopPlay(DWORD nTimeout = 50);

	void Pause()
	{
		m_bPause = !m_bPause;
	}

	/// @brief			设置探测码流类型时，等待码流的超时值
	/// @param [in]		dwTimeout 等待码流的赶时值，单位毫秒
	/// @remark			该函数必须要在Start之前调用，才能生效
	void SetProbeStreamTimeout(DWORD dwTimeout = 3000)
	{
		m_nProbeStreamTimeout = dwTimeout;
	}

	/// @brief			开启硬解码功能
	/// @param [in]		bEnableHaccel	是否开启硬解码功能
	/// #- true			开启硬解码功能
	/// #- false		关闭硬解码功能
	/// @remark			开启和关闭硬解码功能必须要Start之前调用才能生效
	int  EnableHaccel(IN bool bEnableHaccel = false);

	/// @brief			获取硬解码状态
	/// @retval			true	已开启硬解码功能
	/// @retval			flase	未开启硬解码功能
	inline bool  GetHaccelStatus()
	{
		return m_bEnableHaccel;
	}


/// @brief			启用/禁用异步渲染
/// @param [in]		hPlayHandle		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
/// @param [in]		bEnable			是否启用异步渲染
/// -#	true		启用异步渲染
/// -#	false		禁用异步渲染
/// @retval			0	操作成功
/// @retval			-1	输入参数无效
/// 启用异步渲染后，解码得到的YUV数据会被存入YUV队列，渲染时从YUV队列取出，因此必须先设置YUV队列的最大值，不然可能会导致内存不足
	int EnableAsyncRender(bool bEnable = true,int nFrameCache = 50)
	{
		if (m_hThreadDecode)
			return IPC_Error_PlayerHasStart;
		m_bAsyncRender = bEnable;
		m_nListAvFrameMaxSize = nFrameCache;
		return IPC_Succeed;
	}
	/// @brief			检查是否支持特定视频编码的硬解码
	/// @param [in]		nCodec		视频编码格式,@see IPC_CODEC
	/// @retval			true		支持指定视频编码的硬解码
	/// @retval			false		不支持指定视频编码的硬解码
	static bool  IsSupportHaccel(IN IPC_CODEC nCodec);

	/// @brief			取得当前播放视频的即时信息
	int GetPlayerInfo(PlayerInfo *pPlayInfo);
	
// 	static void CloseShareMemory(HANDLE &hMemHandle, void *pMemory)
// 	{
// 		if (!pMemory || !hMemHandle)
// 			return;
// 		if (pMemory)
// 		{
// 			UnmapViewOfFile(pMemory);
// 			pMemory = NULL;
// 		}
// 		if (hMemHandle)
// 		{
// 			CloseHandle(hMemHandle);
// 			hMemHandle = NULL;
// 		}
// 	}
// 
// 	static void *OpenShareMemory(HANDLE &hMemHandle, TCHAR *szUniqueName)
// 	{
// 		void *pShareSection = NULL;
// 		bool bResult = false;
// 		__try
// 		{
// 			__try
// 			{
// 				if (!szUniqueName || _tcslen(szUniqueName) < 1)
// 					__leave;
// 				hMemHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, szUniqueName);
// 				if (hMemHandle == INVALID_HANDLE_VALUE ||
// 					hMemHandle == NULL)
// 				{
// 					_TraceMsg(_T("%s %d MapViewOfFile Failed,ErrorCode = %d.\r\n"), __FILEW__, __LINE__, GetLastError());
// 					__leave;
// 				}
// 				pShareSection = MapViewOfFile(hMemHandle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
// 				if (pShareSection == NULL)
// 				{
// 					_TraceMsg(_T("%s %d MapViewOfFile Failed,ErrorCode = %d.\r\n"), __FILEW__, __LINE__, GetLastError());
// 					__leave;
// 				}
// 				bResult = true;
// 				_TraceMsg(_T("Open Share memory map Succeed."));
// 			}
// 			__finally
// 			{
// 				if (!bResult)
// 				{
// 					CloseShareMemory(hMemHandle, pShareSection);
// 					pShareSection = NULL;
// 				}
// 			}
// 		}
// 		__except (1)
// 		{
// 			TraceMsg(_T("Exception in OpenShareMemory.\r\n"));
// 		}
// 		return pShareSection;
// 	}
	/// @brief			截取正放播放的视频图像
	/// @param [in]		szFileName		要保存的文件名
	/// @param [in]		nFileFormat		保存文件的编码格式,@see SNAPSHOT_FORMAT定义
	/// @retval			0	操作成功
	/// @retval			-1	输入参数无效
	int  SnapShot(IN CHAR *szFileName, IN SNAPSHOT_FORMAT nFileFormat = XIFF_JPG);
	
	/// @brief			处理截图请求
	/// remark			处理完成后，将置信m_hEventSnapShot事件
	void ProcessSnapshotRequire(AVFrame *pAvFrame);

	/// @brief			设置播放的音量
	/// @param [in]		nVolume			要设置的音量值，取值范围0~100，为0时，则为静音
	/// @retval			0	操作成功
	/// @retval			-1	输入参数无效
	void  SetVolume(IN int nVolume)
	{
		if (m_bEnableAudio && m_pDsBuffer)
		{
			//int nDsVolume = nVolume * 100 - 10000;
			m_pDsBuffer->SetVolume(nVolume);
		}
	}

	/// @brief			取得当前播放的音量
	int  GetVolume()
	{
		if (m_bEnableAudio && m_pDsBuffer)
			return (m_pDsBuffer->GetVolume() + 10000) / 100;
		else
			return 0;
	}

	/// @brief			设置当前播放的速度的倍率
	/// @param [in]		fPlayRate		当前的播放的倍率,大于1时为加速播放,小于1时为减速播放，不能为0或小于0
	/// @retval			0	操作成功
	/// @retval			-1	输入参数无效
	int  SetRate(IN float fPlayRate);

	/// @brief			跳跃到指视频帧进行播放
	/// @param [in]		nFrameID	要播放帧的起始ID
	/// @param [in]		bUpdate		是否更新画面,bUpdate为true则予以更新画面,画面则不更新
	/// @retval			0	操作成功
	/// @retval			-1	输入参数无效
	/// @remark			1.若所指定时间点对应帧为非关键帧，帧自动移动到就近的关键帧进行播放
	///					2.若所指定帧为非关键帧，帧自动移动到就近的关键帧进行播放
	///					3.只有在播放暂时,bUpdate参数才有效
	///					4.用于单帧播放时只能向前移动
	int  SeekFrame(IN int nFrameID, bool bUpdate = false);
	
	/// @brief			跳跃到指定时间偏移进行播放
	/// @param [in]		tTimeOffset		要播放的起始时间,单位秒,如FPS=25,则0.04秒为第2帧，1.00秒，为第25帧
	/// @param [in]		bUpdate		是否更新画面,bUpdate为true则予以更新画面,画面则不更新
	/// @retval			0	操作成功
	/// @retval			-1	输入参数无效
	/// @remark			1.若所指定时间点对应帧为非关键帧，帧自动移动到就近的关键帧进行播放
	///					2.若所指定帧为非关键帧，帧自动移动到就近的关键帧进行播放
	///					3.只有在播放暂时,bUpdate参数才有效
	int  SeekTime(IN time_t tTimeOffset, bool bUpdate);

	/// 跳到指定的帧，并只渲染一帧
	/// @param [in]		tTimeOffset		要播放的起始时间,单位秒,如FPS=25,则0.04秒为第2帧，1.00秒，为第25帧
	/// @param [in]		bUpdate		是否更新画面,bUpdate为true则予以更新画面,画面则不更新
	/// @retval			0	操作成功
	/// @retval			-1	输入参数无效
	/// @remark			1.只有异步渲染时，该函数才能生效，否则返回IPC_Error_NotAsyncPlayer错误
	///					2.必须启动播放器后，该函数才能生交，否则返回IPC_Error_PlayerNotStart错误
	int  AsyncSeekTime(IN time_t tTimeOffset, bool bUpdate);

	/// @brief 从文件中读取一帧，读取的起点默认值为0,SeekFrame或SeekTime可设定其起点位置
	/// @param [in,out]		pBuffer		帧数据缓冲区,可设置为null
	/// @param [in,out]		nBufferSize 帧缓冲区的大小
	int GetFrame(INOUT byte **pBuffer, OUT UINT &nBufferSize);
	
	/// @brief			播放下一帧
	/// @retval			0	操作成功
	/// @retval			-24	播放器未暂停
	/// @remark			该函数仅适用于单帧播放
	int  SeekNextFrame();

	/// @brief			开/关音频播放
	/// @param [in]		bEnable			是否播放音频
	/// -#	true		开启音频播放
	/// -#	false		禁用音频播放
	/// @retval			0	操作成功
	/// @retval			-1	输入参数无效
	/// @retval			IPC_Error_AudioFailed	音频播放设备未就绪
	int  EnableAudio(bool bEnable = true);

	/// @brief			刷新播放窗口
	/// @retval			0	操作成功
	/// @retval			-1	输入参数无效
	/// @remark			该功能一般用于播放结束后，刷新窗口，把画面置为黑色
	void  Refresh();

	void SetBackgroundImage(LPCWSTR szImageFilePath = nullptr);

	// 添加线条失败时，返回0，否则返回线条组的句柄
	long AddLineArray(POINT *pPointArray, int nCount, float fWidth, D3DCOLOR nColor);

	int	RemoveLineArray(long nIndex);

	long AddPolygon(POINT *pPointArray, int nCount, WORD *pVertexIndex, DWORD nColor);

	int RemovePolygon(long nIndex);

	int SetCoordinateMode(int nMode = 1)
	{
		m_nCoordinate = (Coordinte)nMode;
		return IPC_Succeed;
	}
	int SetCallBack(IPC_CALLBACK nCallBackType, IN void *pUserCallBack, IN void *pUserPtr);
	

	/// @brief			取得文件的索引的信息,如文件总帧数,文件偏移表
	/// 以读取大块文件内容的形式，获取帧信息，执行效率上比GetFileSummary0要高
	/// 为提高效率，防止文件解析线程和索引线程竞争磁盘读取权，在得取索引信息的同时，
	/// 向缓冲中投入16秒(400帧)的视频数据，相当于解析线程可以延迟8-10秒才开始工作，
	/// 减少了竞争，提升了速度;与此同时，用户可以立即看到画面，提升了用户体验；
	int GetFileSummary(volatile bool &bWorking);

	/// @brief			解析帧数据
	/// @param [in,out]	pBuffer			来自IPC私有录像文件中的数据流
	/// @param [in,out]	nDataSize		pBuffer中有效数据的长度
	/// @param [out]	pFrameParser	IPC私有录像的帧数据	
	/// @retval			true	操作成功
	/// @retval			false	失败，pBuffer已经没有有效的帧数据
	bool ParserFrame(INOUT byte **ppBuffer,	INOUT DWORD &nDataSize,	FrameParser* pFrameParser);

	///< @brief 视频文件解析线程
	static UINT __stdcall ThreadFileParser(void *p);

	int EnableStreamParser(IPC_CODEC nCodec = CODEC_H264);

	///< @brief 视频流解析线程
	static UINT __stdcall ThreadStreamParser(void *p);

	/// @brief			获取码流类型	
	/// @param [in]		pVideoCodec		由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
	/// @param [out]	pAudioCodec	返回当前hPlayHandle是否已开启硬解码功能
	/// @remark 码流类型定义请参考:@see IPC_CODEC
	/// @retval			0	操作成功
	/// @retval			-1	输入参数无效
	/// @retval			IPC_Error_PlayerNotStart	播放器尚未启动,无法取得播放过程的信息或属性
	int GetCodec(IPC_CODEC *pVideoCodec, IPC_CODEC *pAudioCodec)
	{
		if (!m_hThreadDecode)
			return IPC_Error_PlayerNotStart;
		if (pVideoCodec)
			*pVideoCodec = m_nVideoCodec;
		if (pAudioCodec)
			*pAudioCodec = m_nAudioCodec;
		return IPC_Succeed;
	}

	// 探测视频码流类型,要求必须输入I帧
	bool ProbeStream(byte *szFrameBuffer, int nBufferLength);
			
	/// @brief 把NV12图像转换为YUV420P图像
	void CopyNV12ToYUV420P(byte *pYV12, byte *pNV12[2], int src_pitch[2], unsigned width, unsigned height);

	/// @brief 把Dxva硬解码NV12帧转换成YV12图像
	void CopyDxvaFrame(byte *pYuv420, AVFrame *pAvFrameDXVA);
	
	void CopyDxvaFrameYV12(byte **ppYV12, int &nStrideY, int &nWidth, int &nHeight, AVFrame *pAvFrameDXVA);
	
	void CopyDxvaFrameNV12(byte **ppNV12, int &nStrideY, int &nWidth, int &nHeight, AVFrame *pAvFrameDXVA);
	
	bool LockDxvaFrame(AVFrame *pAvFrameDXVA, byte **ppSrcY, byte **ppSrcUV, int &nPitch);

	void UnlockDxvaFrame(AVFrame *pAvFrameDXVA);

	// 把YUVC420P帧复制到YV12缓存中
	void CopyFrameYUV420(byte *pYUV420, int nYUV420Size, AVFrame *pFrame420P);
	
	void ProcessYUVFilter(AVFrame *pAvFrame, LONGLONG nTimestamp);
	
	void ProcessYUVCapture(AVFrame *pAvFrame, LONGLONG nTimestamp);

// 	int YUV2RGB24(const unsigned char* pY,
// 		const unsigned char* pU,
// 		const unsigned char* pV,
// 		int nStrideY,
// 		int nStrideUV,
// 		int nWidth,
// 		int nHeight,
// 		byte **pRGBBuffer,
// 		long &nLineSize)
// 	{
// 		AVFrame *pFrameYUV = av_frame_alloc();
// 		pFrameYUV->data[0] = (uint8_t *)pY;
// 		pFrameYUV->data[1] = (uint8_t *)pU;
// 		pFrameYUV->data[2] = (uint8_t *)pV;
// 		pFrameYUV->linesize[0] = nStrideY;
// 		pFrameYUV->linesize[1] = nStrideUV;
// 		pFrameYUV->linesize[2] = nStrideUV;
// 		pFrameYUV->width = nWidth;
// 		pFrameYUV->height = nHeight;
// 		pFrameYUV->format = AV_PIX_FMT_YUV420P;
// 		//pFrameYUV->pict_type = AV_PICTURE_TYPE_P;
// 		if (!m_pConvert)
// 			m_pConvert = make_shared<PixelConvert>(pFrameYUV, D3DFMT_R8G8B8);
// 		int nStatus = m_pConvert->ConvertPixel();
// 		if (nStatus> 0 )
// 		{
// 			*pRGBBuffer = m_pConvert->pImage;
// 			nLineSize = m_pConvert->pFrameNew->linesize[0];
// 		}
// 		return nStatus;
// 	}
	
	/// @brief			码流探测读取数据包回调函数
	/// @param [in]		opaque		用户输入的回调函数参数指针
	/// @param [in]		buf			读取数据的缓存
	/// @param [in]		buf_size	缓存的长度
	/// @return			实际读取数据的长度
	static int ReadAvData(void *opaque, uint8_t *buf, int buf_size);

	/// @brief			实现指时时长的延时
	/// @param [in]		dwDelay		延时时长，单位为ms
	/// @param [in]		bStopFlag	处理停止的标志,为false时，则停止延时
	/// @param [in]		nSleepGranularity	延时时Sleep的粒度,粒度越小，中止延时越迅速，同时也更耗CPU，反之亦然
	static void Delay(DWORD dwDelay, volatile bool &bStopFlag, WORD nSleepGranularity = 20)
	{
		DWORD dwTotal = 0;
		while (bStopFlag && dwTotal < dwDelay)
		{
			Sleep(nSleepGranularity);
			dwTotal += nSleepGranularity;
		}
	}
	bool m_bD3dShared = false;
	void SetD3dShared(bool bD3dShared = true)
	{
		m_bD3dShared = bD3dShared;
	}

	// 指针逆分配器
	// 用于配合智能指针回收内存
	
	static UINT __stdcall ThreadDecode(void *p);
	
	static UINT __stdcall ThreadPlayAudioGSJ(void *p);

	static UINT __stdcall ThreadPlayAudioIPC(void *p);

	RocateAngle m_nRocateAngle;
	AVFrame		*m_pRacoateFrame = nullptr;
	byte		*m_pRocateImage = nullptr;
	int SetRocate(RocateAngle nRocate = Rocate90)
	{
		if (m_bEnableHaccel)
			return IPC_Error_AudioDeviceNotReady;
		m_nRocateAngle = nRocate;
		return IPC_Succeed;
	}

	bool InitialziePlayer();
	
	
	// 用于异步渲染的弹出帧操作
	bool PopFrame(CAVFramePtr &pAvFrame,int &nSize)
	{
		CAutoLock lock(m_cslistAVFrame.Get(), false, __FILE__, __FUNCTION__, __LINE__);
		if (m_listAVFrame.size())
		{
			pAvFrame = m_listAVFrame.front();
			m_listAVFrame.pop_front();
			nSize = m_listAVFrame.size();
			return true;
		}
		else
			return false;
	}
	
	/// 启用单帧播放
	/// 须在Start之前调用
	int EnabelPlayOneFrame(bool bEanble = true)
	{
		if (m_hThreadDecode || m_hThreadAsyncReander)
			return IPC_Error_PlayerHasStart;
		m_bPlayOneFrame = true;
		return IPC_Succeed;
	}
	
	// 用于异步渲染的压入帧操作
	int PushFrame(AVFrame *pSrcFrame,time_t tFrame)
	{
		CAVFramePtr pFrame = make_shared<CAvFrame>(pSrcFrame,tFrame);
		CAutoLock lock(m_cslistAVFrame.Get(),false,__FILE__,__FUNCTION__,__LINE__);
		if (m_listAVFrame.size() < m_nListAvFrameMaxSize)
			m_listAVFrame.push_back(pFrame);
		else
		{
			while (m_listAVFrame.size() >= m_nListAvFrameMaxSize)
				m_listAVFrame.pop_front();
		}
		return m_listAVFrame.size();
	}
	
	static void *_AllocAvFrame()
	{
		return av_frame_alloc();
	}
	static void _AvFree(void *p)
	{
		av_free(p);
	}
	
// 	int EnableAsyncRender(bool bAsync)
// 	{
// 		if (m_hThreadDecode)
// 			return IPC_Error_PlayerHasStart;
// 		m_bAsyncRender = bAsync;
// 		return IPC_Succeed;
// 	}
	static UINT __stdcall ThreadAsyncRender(void *p);
	static UINT __stdcall ThreadSyncDecode(void *p);
// 	static UINT __stdcall ThreadReversePlay(void *p)
// 	{
// 		CIPCPlayer *pThis = (CIPCPlayer *)p;
// 		return pThis->ReversePlayRun();
// 	}
//
// 	UINT ReversePlayRun();
	/// @brief			启用逆向播放
	/// @remark			逆向播放的原理是先高速解码，把图像放入先入先出队列的缓存进行播放，当需要逆向播放放，则从缓存尾部向头部播放，形成逆向效果
	/// @param [in]		bFlag			是否启用逆向播放，为true时则启用，为false时，则关闭，关闭和开户动作都会视频帧缓存
	/// @param [in]		nCacheFrames	逆向播放视频帧缓存容量
	/// void EnableReservePlay(bool bFlag = true);
	int SetDisplayAdapter(int nAdapterNo)
	{
		if (nAdapterNo >= 0)
		{ 
			m_nDisplayAdapter = nAdapterNo;
			return IPC_Succeed;
		}
		else
			return IPC_Error_InvalidParameters;
	}
};