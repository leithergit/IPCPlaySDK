#include "IPCPlayer.hpp"

extern bool g_bEnableDDraw;
extern SharedMemory *g_pSharedMemory;
//extern HANDLE g_hHAccelMutexArray[10];

map<string, DxSurfaceList>g_DxSurfacePool;	// ÓÃÓÚ»º´æDxSurface¶ÔÏó
CCriticalSectionAgent g_csDxSurfacePool;
SwitcherPtrListAgent g_SwitcherList[16][64];

#ifdef _DEBUG
extern CCriticalSectionAgent g_csPlayerHandles;
extern UINT	g_nPlayerHandles;
#endif

CIPCPlayer::CIPCPlayer()
{
#if _DEBUG
	g_csPlayerHandles.Lock();
	g_nPlayerHandles++;
	g_csPlayerHandles.Unlock();
#endif
	ZeroMemory(&m_nZeroOffset, sizeof(CIPCPlayer) - offsetof(CIPCPlayer, m_nZeroOffset));
	/*
	Ê¹ÓÃCCriticalSectionAgentÀà´úÀí£¬²»ÔÙÖ±½Óµ÷ÓÃInitializeCriticalSectionº¯Êý
	InitializeCriticalSection(&m_csVideoCache);*/

	// ÆúÓÃ´úÂë£¬ÓëÒì²½äÖÈ¾µÄÖ¡»º´æÏà¹Ø
	// InitializeCriticalSection(&m_cslistAVFrame);
	/*
	InitializeCriticalSection(&m_csAudioCache);
	InitializeCriticalSection(&m_csParser);
	//InitializeCriticalSection(&m_csBorderRect);
	//InitializeCriticalSection(&m_csListYUV);
	InitializeCriticalSection(&m_csListRenderUnit);
	InitializeCriticalSection(&m_cslistRenderWnd);

	InitializeCriticalSection(&m_csCaptureYUV);
	InitializeCriticalSection(&m_csCaptureYUVEx);
	InitializeCriticalSection(&m_csFilePlayCallBack);
	InitializeCriticalSection(&m_csYUVFilter);
	*/
	m_nMaxFrameSize = 1024 * 256;
	nSize = sizeof(CIPCPlayer);
	m_nAudioPlayFPS = 50;
	m_nSampleFreq = 8000;
	m_nSampleBit = 16;
	m_nListAvFrameMaxSize = 50;
	m_nProbeStreamTimeout = 10000;	// ºÁÃë
	m_nMaxYUVCache = 10;
	m_nPixelFormat = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');
	m_nDecodeDelay = -1;
	m_nCoordinate = Coordinte_Video;
	m_hInputFrameEvent = CreateEvent(nullptr, false, false, nullptr);
}

CIPCPlayer::CIPCPlayer(HWND hWnd, CHAR *szFileName, char *szLogFile)
{
#if _DEBUG
	g_csPlayerHandles.Lock();
	g_nPlayerHandles++;
	g_csPlayerHandles.Unlock();
#endif
	ZeroMemory(&m_nZeroOffset, sizeof(CIPCPlayer) - offsetof(CIPCPlayer, m_nZeroOffset));
#ifdef _DEBUG
	m_pCSGlobalCount->Lock();
	m_nObjIndex = m_nGloabalCount++;
	m_pCSGlobalCount->Unlock();
	m_nLifeTime = timeGetTime();

	// 		m_OuputTime.nDecode = m_nLifeTime;
	// 		m_OuputTime.nInputStream = m_nLifeTime;
	// 		m_OuputTime.nRender = m_nLifeTime;

	OutputMsg("%s \tObject:%d m_nLifeTime = %d.\n", __FUNCTION__, m_nObjIndex, m_nLifeTime);
#endif 
	m_nSDKVersion = IPC_IPC_SDK_VERSION_2015_12_16;
	if (szLogFile)
	{
		strcpy(m_szLogFileName, szLogFile);
		m_pRunlog = make_shared<CRunlogA>(szLogFile);
	}
	m_hEvnetYUVReady = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	m_hEventDecodeStart = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	m_hEventYUVRequire = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	m_hEventFrameCopied = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	/*
	Ê¹ÓÃCCriticalSectionAgentÀà´úÀí£¬²»ÔÙÖ±½Óµ÷ÓÃInitializeCriticalSectionº¯Êý
	InitializeCriticalSection(&m_csVideoCache); */

	m_csDsoundEnum->Lock();
	if (!m_pDsoundEnum)
		m_pDsoundEnum = make_shared<CDSoundEnum>();	///< ÒôÆµÉè±¸Ã¶¾ÙÆ÷
	m_csDsoundEnum->Unlock();
	m_nAudioPlayFPS = 50;
	m_nSampleFreq = 8000;
	m_nSampleBit = 16;
	m_nListAvFrameMaxSize = 50;
	m_nProbeStreamTimeout = 10000;	// ºÁÃë
	m_nMaxYUVCache = 10;
	m_nCoordinate = Coordinte_Video;
#ifdef _DEBUG
	OutputMsg("%s Alloc a \tObject:%d.\n", __FUNCTION__, m_nObjIndex);
#endif
	nSize = sizeof(CIPCPlayer);
	m_nMaxFrameSize = 1024 * 256;
	m_nVideoFPS = 25;				// FPSµÄÄ¬ÈÏÖµÎª25
	m_fPlayRate = 1;
	m_fPlayInterval = 40.0f;
	//m_nVideoCodec	 = CODEC_H264;		// ÊÓÆµÄ¬ÈÏÊ¹ÓÃH.264±àÂë
	m_nVideoCodec = CODEC_UNKNOWN;
	m_nAudioCodec = CODEC_G711U;		// ÒôÆµÄ¬ÈÏÊ¹ÓÃG711U±àÂë
	//#ifdef _DEBUG
	//		m_nMaxFrameCache = 200;				// Ä¬ÈÏ×î´óÊÓÆµ»º³åÊýÁ¿Îª200
	// #else
	m_nMaxFrameCache = 100;				// Ä¬ÈÏ×î´óÊÓÆµ»º³åÊýÁ¿Îª100
	m_nPixelFormat = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');
	m_hInputFrameEvent = CreateEvent(nullptr, false, false, nullptr);
	m_hRenderWnd = hWnd;
	m_nDecodeDelay = -1;
	// #endif
	if (szFileName)
	{
		m_pszFileName = _New char[MAX_PATH];
		strcpy(m_pszFileName, szFileName);
		// ´ò¿ªÎÄ¼þ
		m_hVideoFile = CreateFileA(m_pszFileName,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_ARCHIVE,
			NULL);
		if (m_hVideoFile != INVALID_HANDLE_VALUE)
		{
			int nError = GetFileHeader();
			if (nError != IPC_Succeed)
			{
				OutputMsg("%s %d(%s):Not a IPC Media File.\n", __FILE__, __LINE__, __FUNCTION__);
				ClearOnException();
				throw std::exception("Not a IPC Media File.");
			}
			// GetLastFrameIDÈ¡µÃµÄÊÇ×îºóÒ»Ö¡µÄID£¬×ÜÖ¡Êý»¹ÒªÔÚ´Ë»ù´¡ÉÏ+1
			if (m_pMediaHeader)
			{
				m_nSDKVersion = m_pMediaHeader->nSDKversion;
				switch (m_nSDKVersion)
				{
				case IPC_IPC_SDK_VERSION_2015_09_07:
				case IPC_IPC_SDK_VERSION_2015_10_20:
				case IPC_IPC_SDK_GSJ_HEADER:
				{
					m_nVideoFPS = 25;
					if (!m_pMediaHeader->nVideoWidth || !m_pMediaHeader->nVideoHeight)
					{
						//m_nVideoCodec = CODEC_UNKNOWN;
						m_nVideoWidth = 0;
						m_nVideoHeight = 0;
					}
					else
					{
						
						m_nVideoWidth = m_pMediaHeader->nVideoWidth;
						m_nVideoHeight = m_pMediaHeader->nVideoHeight;
					}
					m_nVideoCodec = m_pMediaHeader->nVideoCodec;
					m_nAudioCodec = m_pMediaHeader->nAudioCodec;
					// È¡µÃµÚÒ»Ö¡ºÍ×îºóÒ»Ö¡µÄÐÅÏ¢
					if (GetFrame(&m_FirstFrame, true) != IPC_Succeed ||
						GetFrame(&m_LastFrame, false) != IPC_Succeed)
					{
						OutputMsg("%s %d(%s):Can't get the First or Last.\n", __FILE__, __LINE__, __FUNCTION__);
						ClearOnException();
						throw std::exception("Can't get the First or Last.");
					}
					// È¡µÃÎÄ¼þ×ÜÊ±³¤(ms)
					__int64 nTotalTime = 0;
					__int64 nTotalTime2 = 0;
					if (m_pMediaHeader->nCameraType == 1)	// °²Ñ¶Ê¿Ïà»ú
					{
						nTotalTime = (m_LastFrame.nFrameUTCTime - m_FirstFrame.nFrameUTCTime) * 100;
						nTotalTime2 = (m_LastFrame.nTimestamp - m_FirstFrame.nTimestamp) / 10000;
					}
					else
					{
						nTotalTime = (m_LastFrame.nFrameUTCTime - m_FirstFrame.nFrameUTCTime) * 1000;
						nTotalTime2 = (m_LastFrame.nTimestamp - m_FirstFrame.nTimestamp) / 1000;
					}
					if (nTotalTime < 0)
					{
						OutputMsg("%s %d(%s):The Frame timestamp is invalid.\n", __FILE__, __LINE__, __FUNCTION__);
						ClearOnException();
						throw std::exception("The Frame timestamp is invalid.");
					}
					if (nTotalTime2 > 0)
					{
						m_nTotalTime = nTotalTime2;
						// ¸ù¾Ý×ÜÊ±¼äÔ¤²â×ÜÖ¡Êý
						m_nTotalFrames = m_nTotalTime / 40;		// ÀÏ°æÎÄ¼þÊ¹ÓÃ¹Ì¶¨Ö¡ÂÊ,Ã¿Ö¡¼ä¸ôÎª40ms
						m_nTotalFrames += 25;
					}
					else if (nTotalTime > 0)
					{
						m_nTotalTime = nTotalTime;
						// ¸ù¾Ý×ÜÊ±¼äÔ¤²â×ÜÖ¡Êý
						m_nTotalFrames = m_nTotalTime / 40;		// ÀÏ°æÎÄ¼þÊ¹ÓÃ¹Ì¶¨Ö¡ÂÊ,Ã¿Ö¡¼ä¸ôÎª40ms
						m_nTotalFrames += 50;
					}
					else
					{
						OutputMsg("%s %d(%s):Frame timestamp error.\n", __FILE__, __LINE__, __FUNCTION__);
						ClearOnException();
						throw std::exception("Frame timestamp error.");
					}
					break;
				}

				case IPC_IPC_SDK_VERSION_2015_12_16:
				{
					int nError = GetLastFrameID(m_nTotalFrames);
					if (nError != IPC_Succeed)
					{
						OutputMsg("%s %d(%s):Can't get last FrameID .\n", __FILE__, __LINE__, __FUNCTION__);
						ClearOnException();
						throw std::exception("Can't get last FrameID.");
					}
					m_nTotalFrames++;
					m_nVideoCodec = m_pMediaHeader->nVideoCodec;
					m_nAudioCodec = m_pMediaHeader->nAudioCodec;
					if (m_nVideoCodec == CODEC_UNKNOWN)
					{
						m_nVideoWidth = 0;
						m_nVideoHeight = 0;
					}
					else
					{
						m_nVideoWidth = m_pMediaHeader->nVideoWidth;
						m_nVideoHeight = m_pMediaHeader->nVideoHeight;
// 						if (!m_nVideoWidth || !m_nVideoHeight)
// 						{
// 							// 								OutputMsg("%s %d(%s):Invalid Mdeia File Header.\n", __FILE__, __LINE__, __FUNCTION__);
// 							// 								ClearOnException();
// 							// 								throw std::exception("Invalid Mdeia File Header.");
// 							m_nVideoCodec = CODEC_UNKNOWN;
// 						}
					}
					if (m_pMediaHeader->nFps == 0)
						m_nVideoFPS = 25;
					else
						m_nVideoFPS = m_pMediaHeader->nFps;
				}
				break;
				default:
				{
					OutputMsg("%s %d(%s):Invalid SDK Version.\n", __FILE__, __LINE__, __FUNCTION__);
					ClearOnException();
					throw std::exception("Invalid SDK Version.");
				}
				}
				m_nFileFrameInterval = 1000 / m_nVideoFPS;
			}
			// 				m_hCacheFulled = CreateEvent(nullptr, true, false, nullptr);
			// 				m_bThreadSummaryRun = true;
			// 				m_hThreadGetFileSummary = (HANDLE)_beginthreadex(nullptr, 0, ThreadGetFileSummary, this, 0, 0);
			// 				if (!m_hThreadGetFileSummary)
			// 				{
			// 					OutputMsg("%s %d(%s):_beginthreadex ThreadGetFileSummary Failed.\n", __FILE__, __LINE__, __FUNCTION__);
			// 					ClearOnException();
			// 					throw std::exception("_beginthreadex ThreadGetFileSummary Failed.");
			// 				}
			m_nParserBufferSize = m_nMaxFrameSize * 4;
			m_pParserBuffer = (byte *)_aligned_malloc(m_nParserBufferSize, 16);
		}
		else
		{
			OutputMsg("%s %d(%s):Open file failed.\n", __FILE__, __LINE__, __FUNCTION__);
			ClearOnException();
			throw std::exception("Open file failed.");
		}
	}

}

CIPCPlayer::CIPCPlayer(HWND hWnd, int nBufferSize, char *szLogFile)
{
#if _DEBUG
	g_csPlayerHandles.Lock();
	g_nPlayerHandles++;
	g_csPlayerHandles.Unlock();
#endif
	ZeroMemory(&m_nZeroOffset, sizeof(CIPCPlayer) - offsetof(CIPCPlayer, m_nZeroOffset));
#ifdef _DEBUG
	m_pCSGlobalCount->Lock();
	m_nObjIndex = m_nGloabalCount++;
	m_pCSGlobalCount->Unlock();
	m_nLifeTime = timeGetTime();

	OutputMsg("%s \tObject:%d m_nLifeTime = %d.\n", __FUNCTION__, m_nObjIndex, m_nLifeTime);
#endif 
	m_nSDKVersion = IPC_IPC_SDK_VERSION_2015_12_16;
	if (szLogFile)
	{
		strcpy(m_szLogFileName, szLogFile);
		m_pRunlog = make_shared<CRunlogA>(szLogFile);
	}
	m_hEvnetYUVReady = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	m_hEventDecodeStart = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	m_hEventYUVRequire = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	m_hEventFrameCopied = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// Ê¹ÓÃCCriticalSectionAgentÀà´úÀí£¬²»ÔÙÖ±½Óµ÷ÓÃInitializeCriticalSectionº¯Êý

	m_csDsoundEnum->Lock();
	if (!m_pDsoundEnum)
		m_pDsoundEnum = make_shared<CDSoundEnum>();	///< ÒôÆµÉè±¸Ã¶¾ÙÆ÷
	m_csDsoundEnum->Unlock();
	m_nAudioPlayFPS = 50;
	m_nSampleFreq = 8000;
	m_nSampleBit = 16;
	m_nListAvFrameMaxSize = 50;
	m_nProbeStreamTimeout = 10000;	// ºÁÃë
	m_nMaxYUVCache = 10;
#ifdef _DEBUG
	OutputMsg("%s Alloc a \tObject:%d.\n", __FUNCTION__, m_nObjIndex);
#endif
	nSize = sizeof(CIPCPlayer);
	m_nMaxFrameSize = 1024 * 256;
	m_nVideoFPS = 25;				// FPSµÄÄ¬ÈÏÖµÎª25
	m_fPlayRate = 1;
	m_fPlayInterval = 40.0f;
	//m_nVideoCodec	 = CODEC_H264;		// ÊÓÆµÄ¬ÈÏÊ¹ÓÃH.264±àÂë
	m_nVideoCodec = CODEC_UNKNOWN;
	m_nAudioCodec = CODEC_G711U;		// ÒôÆµÄ¬ÈÏÊ¹ÓÃG711U±àÂë
	//#ifdef _DEBUG
	//		m_nMaxFrameCache = 200;				// Ä¬ÈÏ×î´óÊÓÆµ»º³åÊýÁ¿Îª200
	// #else
	m_nMaxFrameCache = 100;				// Ä¬ÈÏ×î´óÊÓÆµ»º³åÊýÁ¿Îª100
	m_nPixelFormat = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');

	AddRenderWindow(hWnd, nullptr);
	m_hInputFrameEvent = CreateEvent(nullptr, false, false, nullptr);
	
#ifndef _WIN64
	m_bEnableDDraw = g_bEnableDDraw;
#endif
	
	m_hRenderWnd = hWnd;
	m_nDecodeDelay = -1;
	// #endif

}

CIPCPlayer::~CIPCPlayer()
{
//#ifdef _DEBUG
//	OutputMsg("%s \tReady to Free a \tObject:%d.\n", __FUNCTION__, m_nObjIndex);
//#endif
	//StopPlay(0);

	if (m_pParserBuffer)
	{
		//delete[]m_pParserBuffer;
		_aligned_free(m_pParserBuffer);
#ifdef _DEBUG
		m_pParserBuffer = nullptr;
#endif
	}
	if (m_pDsBuffer)
	{
		m_pDsPlayer->DestroyDsoundBuffer(m_pDsBuffer);
#ifdef _DEBUG
		m_pDsBuffer = nullptr;
#endif
	}
	if (m_pDecoder)
		m_pDecoder.reset();

	if (m_pRocateImage)
	{
		_aligned_free(m_pRocateImage);
		m_pRocateImage = nullptr;
	}
	if (m_pRacoateFrame)
	{
		av_free(m_pRacoateFrame);
		m_pRacoateFrame = nullptr;
	}
	m_listVideoCache.clear();
	if (m_pszFileName)
		delete[]m_pszFileName;

	if (m_hVideoFile)
		CloseHandle(m_hVideoFile);

	if (m_hEvnetYUVReady)
		CloseHandle(m_hEvnetYUVReady);
	if (m_hEventDecodeStart)
		CloseHandle(m_hEventDecodeStart);

	if (m_hEventYUVRequire)
		CloseHandle(m_hEventYUVRequire);
	if (m_hEventFrameCopied)
		CloseHandle(m_hEventFrameCopied);

	if (m_hRenderAsyncEvent && !m_pSyncPlayer)
	{
		CloseHandle(m_hRenderAsyncEvent);
		m_hRenderAsyncEvent = nullptr;
	}
	if (m_hInputFrameEvent)
	{
		CloseHandle(m_hInputFrameEvent);
		m_hInputFrameEvent = nullptr;
	}
#ifndef _WIN64
	if (m_pDDraw)
	{
		delete m_pDDraw;
		m_pDDraw = nullptr;
	}
#endif

	if (m_pDxSurface)
	{
		//PutDxSurfacePool(m_pDxSurface);
		Safe_Delete(m_pDxSurface);
// 		delete m_pDxSurface;
// 		m_pDxSurface = nullptr;
	}
	/*
	Ê¹ÓÃCCriticalSectionAgentÀà´úÀí£¬²»ÔÙÖ±½Óµ÷ÓÃDeleteCriticalSectionº¯Êý
	DeleteCriticalSection(&m_csVideoCache);*/

	if (m_pszBackImagePath)
	{
		delete[]m_pszBackImagePath;
	}
#ifdef _DEBUG
	OutputMsg("%s \tFinish Free a \tObject:%d.\n", __FUNCTION__, m_nObjIndex);
	//OutputMsg("%s \tObject:%d Exist Time = %u(ms).\n", __FUNCTION__, m_nObjIndex, timeGetTime() - m_nLifeTime);
	g_csPlayerHandles.Lock();
	g_nPlayerHandles--;
	g_csPlayerHandles.Unlock();
#endif
}

__int64 CIPCPlayer::LargerFileSeek(HANDLE hFile, __int64 nOffset64, DWORD MoveMethod)
{
	LARGE_INTEGER Offset;
	Offset.QuadPart = nOffset64;
	Offset.LowPart = SetFilePointer(hFile, Offset.LowPart, &Offset.HighPart, MoveMethod);

	if (Offset.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
	{
		Offset.QuadPart = -1;
	}
	return Offset.QuadPart;
}

bool CIPCPlayer::IsIPCVideoFrame(IPCFrameHeader *pFrameHeader, bool &bIFrame, int nSDKVersion)
{
	bIFrame = false;
	if (nSDKVersion >= IPC_IPC_SDK_VERSION_2015_12_16 && nSDKVersion != IPC_IPC_SDK_GSJ_HEADER)
	{
		switch (pFrameHeader->nType)
		{
		case FRAME_P:		// BPÖ¡ÊýÁ¿×î¶à£¬ËùÒÔÇ°ÖÃ£¬ÒÔ¼õÉÙ±È½Ï´ÎÊý
		case FRAME_B:
			return true;
		case 0:
		case FRAME_IDR:
		case FRAME_I:
			bIFrame = true;
			return true;
		default:
			return false;
		}
	}
	else
	{
		switch (pFrameHeader->nType)
		{// ¾É°æSDKÖÐ£¬0ÎªbbpÖ¡ ,1ÎªIÖ¡ ,2ÎªÒôÆµÖ¡
		case 0:
			return true;
			break;
		case 1:
			bIFrame = true;
			return true;
			break;
		default:
			return false;
			break;
		}
	}
}

int CIPCPlayer::GetFrame(IPCFrameHeader *pFrame, bool bFirstFrame)
{
	if (!m_hVideoFile)
		return IPC_Error_FileNotOpened;
	LARGE_INTEGER liFileSize;
	if (!GetFileSizeEx(m_hVideoFile, &liFileSize))
		return GetLastError();
	byte *pBuffer = _New byte[m_nMaxFrameSize];
	shared_ptr<byte>TempBuffPtr(pBuffer);
	DWORD nMoveMothod = FILE_BEGIN;
	__int64 nMoveOffset = sizeof(IPC_MEDIAINFO);

	if (!bFirstFrame)
	{
		nMoveMothod = FILE_END;
		nMoveOffset = -m_nMaxFrameSize;
	}

	if (liFileSize.QuadPart >= m_nMaxFrameSize &&
		LargerFileSeek(m_hVideoFile, nMoveOffset, nMoveMothod) == INVALID_SET_FILE_POINTER)
		return GetLastError();
	DWORD nBytesRead = 0;
	DWORD nDataLength = m_nMaxFrameSize;

	if (!ReadFile(m_hVideoFile, pBuffer, nDataLength, &nBytesRead, nullptr))
	{
		OutputMsg("%s ReadFile Failed,Error = %d.\n", __FUNCTION__, GetLastError());
		return GetLastError();
	}
	nDataLength = nBytesRead;
	char *szKey1 = "MOVD";
	char *szKey2 = "IMWH";
	int nOffset = KMP_StrFind(pBuffer, nDataLength, (byte *)szKey1, 4);
	if (nOffset < 0)
	{
		nOffset = KMP_StrFind(pBuffer, nDataLength, (byte *)szKey2, 4);
		if (nOffset < 0)
			return IPC_Error_MaxFrameSizeNotEnough;
	}
	nOffset -= offsetof(IPCFrameHeader, nFrameTag);	// »ØÍËµ½Ö¡Í·
	if (nOffset < 0)
		return IPC_Error_NotVideoFile;
	// ±éÀúËùÓÐ×îºóm_nMaxFrameSize³¤¶ÈÄÚµÄËùÓÐÖ¡
	pBuffer += nOffset;
	nDataLength -= nOffset;
	bool bFoundVideo = false;

	FrameParser Parser;
#ifdef _DEBUG
	int nAudioFrames = 0;
	int nVideoFrames = 0;
	while (ParserFrame(&pBuffer, nDataLength, &Parser))
	{
		switch (Parser.pHeader->nType)
		{
		case 0:
		case 1:
		{
			nVideoFrames++;
			bFoundVideo = true;
			break;
		}
		case 2:      // G711 AÂÉ±àÂëÖ¡
		case FRAME_G711U:      // G711 UÂÉ±àÂëÖ¡
		{
			nAudioFrames++;
			break;
		}
		default:
		{
			assert(false);
			break;
		}
		}
		if (bFoundVideo && bFirstFrame)
			break;

		nOffset += Parser.nFrameSize;
	}
	OutputMsg("%s In Last %d bytes:VideoFrames:%d\tAudioFrames:%d.\n", __FUNCTION__, m_nMaxFrameSize, nVideoFrames, nAudioFrames);
#else
	while (ParserFrame(&pBuffer, nDataLength, &Parser))
	{
		if (Parser.pHeaderEx->nType == 0 ||
			Parser.pHeaderEx->nType == 1)
		{
			bFoundVideo = true;
		}
		if (bFoundVideo && bFirstFrame)
		{
			break;
		}
		nOffset += Parser.nFrameSize;
	}
#endif
	if (SetFilePointer(m_hVideoFile, sizeof(IPC_MEDIAINFO), nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return GetLastError();
	if (bFoundVideo)
	{
		memcpy_s(pFrame, sizeof(IPCFrameHeader), Parser.pHeader, sizeof(IPCFrameHeader));
		return IPC_Succeed;
	}
	else
		return IPC_Error_MaxFrameSizeNotEnough;
}

int CIPCPlayer::GetLastFrameID(int &nLastFrameID)
{
	if (!m_hVideoFile)
		return IPC_Error_FileNotOpened;
	LARGE_INTEGER liFileSize;
	if (!GetFileSizeEx(m_hVideoFile, &liFileSize))
		return GetLastError();
	byte *pBuffer = _New byte[m_nMaxFrameSize];
	shared_ptr<byte>TempBuffPtr(pBuffer);

	if (liFileSize.QuadPart >= m_nMaxFrameSize &&
		LargerFileSeek(m_hVideoFile, -m_nMaxFrameSize, FILE_END) == INVALID_SET_FILE_POINTER)
		return GetLastError();
	DWORD nBytesRead = 0;
	DWORD nDataLength = m_nMaxFrameSize;

	if (!ReadFile(m_hVideoFile, pBuffer, nDataLength, &nBytesRead, nullptr))
	{
		OutputMsg("%s ReadFile Failed,Error = %d.\n", __FUNCTION__, GetLastError());
		return GetLastError();
	}
	nDataLength = nBytesRead;
	char *szKey1 = "MOVD";		// ÐÂ°æµÄIPCÂ¼ÏñÎÄ¼þÍ·
	char *szKey2 = "IMWH";		// ÀÏ°æ±¾µÄIPCÂ¼ÏñÎÄ¼þ£¬Ê¹ÓÃÁË¸ßÊÓ½ÝµÄÎÄ¼þÍ·
	int nOffset = KMP_StrFind(pBuffer, nDataLength, (byte *)szKey1, 4);
	if (nOffset < 0)
	{
		nOffset = KMP_StrFind(pBuffer, nDataLength, (byte *)szKey2, 4);
		if (nOffset < 0)
			return IPC_Error_MaxFrameSizeNotEnough;
		else if (nOffset < offsetof(IPCFrameHeader, nFrameTag))
		{
			pBuffer += (nOffset + 4);
			nDataLength -= (nOffset + 4);
			nOffset = KMP_StrFind(pBuffer, nDataLength, (byte *)szKey2, 4);
		}
	}
	else if (nOffset < offsetof(IPCFrameHeader, nFrameTag))
	{
		pBuffer += (nOffset + 4);
		nDataLength -= (nOffset + 4);
		nOffset = KMP_StrFind(pBuffer, nDataLength, (byte *)szKey1, 4);
	}
	nOffset -= offsetof(IPCFrameHeader, nFrameTag);	// »ØÍËµ½Ö¡Í·
	if (nOffset < 0)
		return IPC_Error_NotVideoFile;
	// ±éÀúËùÓÐ×îºóm_nMaxFrameSize³¤¶ÈÄÚµÄËùÓÐÖ¡
	pBuffer += nOffset;
	nDataLength -= nOffset;
	bool bFoundVideo = false;

	FrameParser Parser;
#ifdef _DEBUG
	int nAudioFrames = 0;
	int nVideoFrames = 0;
	while (ParserFrame(&pBuffer, nDataLength, &Parser))
	{
		switch (Parser.pHeaderEx->nType)
		{
		case 0:
		case FRAME_B:
		case FRAME_I:
		case FRAME_IDR:
		case FRAME_P:
		{
			nVideoFrames++;
			nLastFrameID = Parser.pHeaderEx->nFrameID;
			bFoundVideo = true;
			break;
		}
		case FRAME_G711A:      // G711 AÂÉ±àÂëÖ¡
		case FRAME_G711U:      // G711 UÂÉ±àÂëÖ¡
		case FRAME_G726:       // G726±àÂëÖ¡
		case FRAME_AAC:        // AAC±àÂëÖ¡¡£
		{
			nAudioFrames++;
			break;
		}
		default:
		{
			assert(false);
			break;
		}
		}

		nOffset += Parser.nFrameSize;
	}
	OutputMsg("%s In Last %d bytes:VideoFrames:%d\tAudioFrames:%d.\n", __FUNCTION__, m_nMaxFrameSize, nVideoFrames, nAudioFrames);
#else
	while (ParserFrame(&pBuffer, nDataLength, &Parser))
	{
		if (Parser.pHeaderEx->nType == FRAME_B ||
			Parser.pHeaderEx->nType == FRAME_I ||
			Parser.pHeaderEx->nType == FRAME_P)
		{
			nLastFrameID = Parser.pHeaderEx->nFrameID;
			bFoundVideo = true;
		}
		nOffset += Parser.nFrameSize;
	}
#endif
	if (SetFilePointer(m_hVideoFile, sizeof(IPC_MEDIAINFO), nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return GetLastError();
	if (bFoundVideo)
		return IPC_Succeed;
	else
		return IPC_Error_MaxFrameSizeNotEnough;
}

CDxSurfaceEx *CIPCPlayer::GetDxSurfaceFromPool(int nWidth, int  nHeight)
{
	char szResolution[32] = { 0 };
	sprintf_s(szResolution, 16, "%d*%d", nWidth, nHeight);
	string strResolutoin = szResolution;
	CAutoLock Poollock(g_csDxSurfacePool.Get());
	auto PoolFinder = g_DxSurfacePool.find(strResolutoin);
	if (PoolFinder != g_DxSurfacePool.end())
	{
		DxSurfaceList &ItemList = PoolFinder->second;
		if (ItemList.size() > 0)
		{
			CDxSurfaceEx *pSurface = (CDxSurfaceEx*)ItemList.front();
			ItemList.pop_front();
			return pSurface;
		}
	}
	return nullptr;
}

void CIPCPlayer::PutDxSurfacePool(CDxSurfaceEx *pSurface)
{
	if (!pSurface)
		return;
	char szResolution[32] = { 0 };
	sprintf_s(szResolution, 16, "%d*%d", pSurface->m_nVideoWidth, pSurface->m_nVideoHeight);
	string strResolutoin = szResolution;
	CAutoLock Poollock(g_csDxSurfacePool.Get());
	auto PoolFinder = g_DxSurfacePool.find(strResolutoin);
	if (PoolFinder != g_DxSurfacePool.end())
	{
		PoolFinder->second.push_back(pSurface);
	}
	else
	{
		DxSurfaceList SurfaceList;
		SurfaceList.push_back(pSurface);
		g_DxSurfacePool.insert(pair<string, DxSurfaceList>(strResolutoin, SurfaceList));
	}
}

bool CIPCPlayer::InitizlizeDx(AVFrame *pAvFrame )
{
	DeclareRunTime(10);
// Ê¹ÓÃm_hRenderWndÎª¿ÕÊ±£¬ÔòÊ¹ÓÃm_pWndDxInitµÄ´°¿Ú
// 	if (!m_hRenderWnd)
// 		return true;
	//		¿ÉÄÜ´æÔÚÖ»½âÂë²»ÏÔÊ¾Í¼ÏñµÄÇé¿ö
	// 		if (!m_hRenderWnd)
	// 			return false;
	// ³õÊ¼ÏÔÊ¾×é¼þ
	//if (GetOsMajorVersion() < Win7MajorVersion)
#ifndef _WIN64
	if (m_bEnableDDraw)
	{
		OutputMsg("%s DirectDraw is enabled.\n", __FUNCTION__);
		m_pDDraw = _New CDirectDraw();
		if (m_pDDraw)
		{
			if (m_bEnableHaccel)
			{
				DDSURFACEDESC2 ddsd = { 0 };
				FormatNV12::Build(ddsd, m_nVideoWidth, m_nVideoHeight);
				m_cslistRenderWnd.Lock();
				if (m_hRenderWnd)
					m_pDDraw->Create<FormatNV12>(m_hRenderWnd, ddsd);
				else
					m_pDDraw->Create<FormatNV12>(m_pWndDxInit->GetSafeHwnd(), ddsd);
				m_cslistRenderWnd.Unlock();
				m_pYUVImage = make_shared<ImageSpace>();
				m_pYUVImage->dwLineSize[0] = m_nVideoWidth;
				m_pYUVImage->dwLineSize[1] = m_nVideoWidth >> 1;
			}
			else
			{
				//¹¹ÔìDirectDraw±íÃæ  
				DDSURFACEDESC2 ddsd = { 0 };
				FormatYV12::Build(ddsd, m_nVideoWidth, m_nVideoHeight);
				m_cslistRenderWnd.Lock();
				if (m_hRenderWnd)
					m_pDDraw->Create<FormatYV12>(m_hRenderWnd, ddsd);
				else
					m_pDDraw->Create<FormatYV12>(m_pWndDxInit->GetSafeHwnd(), ddsd);
				m_cslistRenderWnd.Unlock();
				m_pYUVImage = make_shared<ImageSpace>();
				m_pYUVImage->dwLineSize[0] = m_nVideoWidth;
				m_pYUVImage->dwLineSize[1] = m_nVideoWidth >> 1;
				m_pYUVImage->dwLineSize[2] = m_nVideoWidth >> 1;
				m_pDDraw->SetExternDraw(m_pDCCallBack, m_pDCCallBackParam);
			}
			LineLockAgent(m_csListRenderUnit);
			for (auto it = m_listRenderUnit.begin(); it != m_listRenderUnit.end(); it++)
				(*it)->ReInitialize(m_nVideoWidth, m_nVideoHeight);
		}
		return true;
	}
	else
#endif
	{
		SaveRunTime();
// 		m_pDxSurface = GetDxSurfaceFromPool(m_nVideoWidth, m_nVideoHeight);
// 		if (m_pDxSurface)
// 			return true;
// 		else
		if (!m_pDxSurface)
			m_pDxSurface = _New CDxSurfaceEx;

		// Ê¹ÓÃÏß³ÌÄÚCDxSurface¶ÔÏóÏÔÊ¾Í¼Ïó
		if (!m_pDxSurface)		// D3DÉè±¸ÉÐÎ´³õÊ¼»¯
		{
			OutputMsg("%s Failed in new a CDxSurfaceEx to m_pDxSurface.\n", __FUNCTION__);
			return false;
		}
		
		SaveRunTime();
		//m_pSimpleWnd = make_shared<CSimpleWnd>(m_nVideoWidth, m_nVideoHeight);
		DxSurfaceInitInfo &InitInfo = m_DxInitInfo;
		InitInfo.nSize = sizeof(DxSurfaceInitInfo);
		if (m_bEnableHaccel)
		{
			m_pDxSurface->SetD3DShared(m_bD3dShared);
			AVCodecID nCodecID = AV_CODEC_ID_H264;
			if (m_nVideoCodec == CODEC_H265)
				nCodecID = AV_CODEC_ID_HEVC;
			InitInfo.nFrameWidth = CVideoDecoder::GetAlignedDimension(nCodecID, m_nVideoWidth);
			InitInfo.nFrameHeight = CVideoDecoder::GetAlignedDimension(nCodecID, m_nVideoHeight);
			InitInfo.nD3DFormat = (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2');
		}
		else
		{
			///if (GetOsMajorVersion() < 6)
			///	InitInfo.nD3DFormat = D3DFMT_A8R8G8B8;		// ÔÚXP»·¾³ÏÂ£¬Ä³Ð©¼¯³ÉÏÔÊ¾ÔÚÏÔÊ¾YV12ÏóËØÊ±£¬»áµ¼ÖÂCPUÕ¼ÓÃÂÊ»ºÂýÉÏÉý£¬Õâ¿ÉÄÜÊÇD3D9»ò¸Ã¼¯³ÉÏÔÊ¾µÄÒ»¸öBUG
			InitInfo.nFrameWidth = m_nVideoWidth;
			InitInfo.nFrameHeight = m_nVideoHeight;
			InitInfo.nD3DFormat = m_nPixelFormat;//(D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');
		}
		SaveRunTime();
		InitInfo.bWindowed = TRUE;
// 			if (!m_pWndDxInit->GetSafeHwnd())
// 				InitInfo.hPresentWnd = m_hRenderWnd;
// 			else
		//InitInfo.hPresentWnd = m_pWndDxInit->GetSafeHwnd();
		/// ¼¯ÖÐÊ¹ÓÃÍ¬Ò»¸ö´°¿Ú×÷d3d³õÊ¼»¯£¬ÓÐ¿ÉÄÜÊÇÔì³É¶àÏÔÆ÷ÊÓÆµÏÔÊ¾Ê±¿¨¶ÙµÄÔ­Òò
		/// 2018.11.30 ÔÚÕâÀï×÷Ò»¸ö²âÊÔ£¬Ê¹ÓÃÔ­Ê¼ÊäÈë´°¿Ú½øÐÐÏÔÊ¾£¬²¢¸ù¾Ý´°¿ÚËùÔÚÏÔÊ¾Æ÷Á¬½ÓµÄÏÔ¿¨ÐòºÅ£¬¾ö¶¨ÓÃÄÄÒ»¿éÏÔ¿¨À´×÷d3d³õÊ¼»¯
		if (m_hRenderWnd)
			InitInfo.hPresentWnd = m_hRenderWnd;
		else
			InitInfo.hPresentWnd = m_pWndDxInit->GetSafeHwnd();
			
		SaveRunTime();
		if (m_nRocateAngle == Rocate90 ||
			m_nRocateAngle == Rocate270 ||
			m_nRocateAngle == RocateN90)
			swap(InitInfo.nFrameWidth, InitInfo.nFrameHeight);
		// ×¼±¸¼ÓÔØ±³¾°Í¼Æ¬
		SaveRunTime();
		if (m_pszBackImagePath)
			m_pDxSurface->SetBackgroundPictureFile(m_pszBackImagePath, m_hRenderWnd);
		SaveRunTime();
		m_pDxSurface->SetDisplayAdapter(m_nDisplayAdapter);
		m_pDxSurface->DisableVsync();		// ½ûÓÃ´¹Ö±Í¬²½£¬²¥·ÅÖ¡²ÅÓÐ¿ÉÄÜ³¬¹ýÏÔÊ¾Æ÷µÄË¢ÐÂÂÊ£¬´Ó¶ø´ïµ½¸ßËÙ²¥·ÅµÄÄ¿µÄ
		if (!m_pDxSurface->InitD3D(InitInfo.hPresentWnd,
			InitInfo.nFrameWidth,
			InitInfo.nFrameHeight,
			InitInfo.bWindowed,
			InitInfo.nD3DFormat,
			m_pRunlog.get()))
		{
			OutputMsg("%s Initialize DxSurface failed.\n", __FUNCTION__);
#ifdef _DEBUG
			OutputMsg("%s \tObject:%d DxSurface failed,Line %d Time = %d.\n", __FUNCTION__, m_nObjIndex, __LINE__, timeGetTime() - m_nLifeTime);
#endif
			delete m_pDxSurface;
			m_pDxSurface = nullptr;
			return false;
		}
		//m_pDxSurface->TestDxCheck(InitInfo.hPresentWnd, InitInfo.nFrameWidth, InitInfo.nFrameHeight);
		SaveRunTime();
		m_pDxSurface->SetCoordinateMode(m_nCoordinate);
		m_pDxSurface->SetExternDraw(m_pDCCallBack, m_pDCCallBackParam);
			
		SaveRunTime();
		return true;
		
	}
}


int CIPCPlayer::AddRenderWindow(HWND hRenderWnd, LPRECT pRtRender, bool bPercent )
{
	if (!hRenderWnd)
		return IPC_Error_InvalidParameters;
// 	if (hRenderWnd == m_hRenderWnd)
// 		return IPC_Succeed;
	LineLockAgent(m_cslistRenderWnd);
	if (!m_hRenderWnd)
	{
		m_hRenderWnd = hRenderWnd;
	}
#ifndef _WIN64
	if (m_bEnableDDraw)
	{
		if (m_listRenderUnit.size() >= 3)
			return IPC_Error_RenderWndOverflow;
		auto itFind = find_if(m_listRenderUnit.begin(), m_listRenderUnit.end(), UnitFinder(hRenderWnd));
		if (itFind != m_listRenderUnit.end())
			return IPC_Succeed;

		m_listRenderUnit.push_back(make_shared<RenderUnit>(hRenderWnd, pRtRender, m_bEnableHaccel));
	}
	else
#endif
	{
		if (m_listRenderWnd.size() >= 4)
			return IPC_Error_RenderWndOverflow;
		auto itFind = find_if(m_listRenderWnd.begin(), m_listRenderWnd.end(), WndFinder(hRenderWnd));
		if (itFind != m_listRenderWnd.end())
			return IPC_Succeed;

		m_listRenderWnd.push_back(make_shared<RenderWnd>(hRenderWnd, pRtRender, bPercent));
		OutputMsg("%s size of m_listRenderWnd = %d.\n", __FUNCTION__,m_listRenderWnd.size());
	}

	return IPC_Succeed;
}

bool CIPCPlayer::TryEnableHAccelOnAdapter(CHAR* szAdapterID, int nBuffer)
{
	if (!szAdapterID || nBuffer < 40)
		return false;
	if (!g_pSharedMemory)
		return false;
	// È¡µÃ´°¿ÚËùÔÚµÄÏÔÊ¾Æ÷¾ä±ú
	HMONITOR hMonitor = MonitorFromWindow(m_hRenderWnd, MONITOR_DEFAULTTONEAREST);
	if (!hMonitor)
		return false;
	
	MONITORINFOEX mi;
	mi.cbSize = sizeof(MONITORINFOEX);
	if (!GetMonitorInfo(hMonitor, &mi))
		return false;
	
	for (int i = 0; i < g_pD3D9Helper.m_nAdapterCount; i++)
	{
		bool bFound = false;
		for (int n = 0; n < g_pD3D9Helper.m_AdapterArray[i].nMonitorCount; n++)
		{// 查找匹配的显示器
			if (strcmp(g_pD3D9Helper.m_AdapterArray[i].szMonitorArray[n], mi.szDevice) == 0)
			{
				bFound = true;
				break;
			}
		}
		if (bFound)
		{// ÕÒµ½ÏÔÊ¾Æ÷ËùÔÚµÄÏÔÎ´
			m_nDisplayAdapter = i;
			OutputMsg("%s Wnd[%08X] is on Monitor:[%s],it's connected on Adapter[%i]:%s.\n", 
					__FUNCTION__, 
					m_hRenderWnd, 
					mi.szDevice, i, 
					g_pD3D9Helper.m_AdapterArray[i].Description);
			WCHAR szGuidW[64] = { 0 };
			
			StringFromGUID2(g_pD3D9Helper.m_AdapterArray[i].DeviceIdentifier, szGuidW, 64);		
			WCHAR szAdapterMutexName[64] = { 0 };
			HANDLE hMutexAdapter = nullptr;
			for (int k = 0; k < g_pSharedMemory->nAdapterCount; k++)
			{
				if (!g_pSharedMemory->HAccelArray[k].hMutex)
					break;

				if (wcscmp(g_pSharedMemory->HAccelArray[k].szAdapterGuid, szGuidW) != 0)
					continue;
				
				if (WaitForSingleObject(g_pSharedMemory->HAccelArray[k].hMutex, 100) == WAIT_TIMEOUT)
					break;
				if (g_pSharedMemory->HAccelArray[k].nOpenCount < g_pSharedMemory->HAccelArray[k].nMaxHaccel)
				{
					g_pSharedMemory->HAccelArray[k].nOpenCount++;
					WideCharToMultiByte(CP_ACP, NULL, szGuidW, wcslen(szGuidW), szAdapterID, nBuffer, NULL, NULL);
					ReleaseMutex(g_pSharedMemory->HAccelArray[k].hMutex);
					OutputMsg("%s HAccels On:Monitor:%s,Adapter:%s is %d.\n", 
						__FUNCTION__, 
						mi.szDevice, 
						g_pD3D9Helper.m_AdapterArray[k].Description, 
						g_pSharedMemory->HAccelArray[k].nOpenCount);
					return true;
				}
				else
				{
					ZeroMemory(szAdapterID, nBuffer);
					ReleaseMutex(g_pSharedMemory->HAccelArray[k].hMutex);
					OutputMsg("%s HAccels On:Monitor:%s,Adapter:%s has reached up limit %d.\n", 
						__FUNCTION__, 
						mi.szDevice, 
						g_pD3D9Helper.m_AdapterArray[k].Description, 
						g_pSharedMemory->HAccelArray[k].nOpenCount);
					return false;
				}
			}
		}
	}
	return false;
}

int CIPCPlayer::RemoveRenderWindow(HWND hRenderWnd)
{
	if (!hRenderWnd)
		return IPC_Error_InvalidParameters;

	LineLockAgent(m_cslistRenderWnd);
	if (m_listRenderWnd.size() < 1)
		return IPC_Succeed;
#ifndef _WIN64
	if (m_bEnableDDraw)
	{
		auto itFind = find_if(m_listRenderUnit.begin(), m_listRenderUnit.end(), UnitFinder(hRenderWnd));
		if (itFind != m_listRenderUnit.end())
		{
			m_listRenderUnit.erase(itFind);
			//InvalidateRect(hRenderWnd, nullptr, true);
		}
		if (hRenderWnd == m_hRenderWnd)
		{
			if (m_listRenderUnit.size() > 0)
				m_hRenderWnd = m_listRenderUnit.front()->hRenderWnd;
			else
				m_hRenderWnd = nullptr;
			return IPC_Succeed;
		}
	}
	else
#endif
	{
		auto itFind = find_if(m_listRenderWnd.begin(), m_listRenderWnd.end(), WndFinder(hRenderWnd));
		if (itFind != m_listRenderWnd.end())
		{
			m_listRenderWnd.erase(itFind);
			//InvalidateRect(hRenderWnd, nullptr, true);
		}
		if (hRenderWnd == m_hRenderWnd)
		{
			if (m_listRenderWnd.size() > 0)
				m_hRenderWnd = m_listRenderWnd.front()->hRenderWnd;
			else
				m_hRenderWnd = nullptr;
		}
		OutputMsg("%s size of m_listRenderWnd = %d.\n", __FUNCTION__,m_listRenderWnd.size());
		return IPC_Succeed;
	}
	
	return IPC_Succeed;
}

// »ñÈ¡ÏÔÊ¾Í¼Ïñ´°¿ÚµÄÊýÁ¿
int CIPCPlayer::GetRenderWindows(HWND* hWndArray, int &nSize)
{
	if (!hWndArray && !nSize)
		return IPC_Error_InvalidParameters;
	LineLockAgent(m_cslistRenderWnd);
	if (!hWndArray)
	{
		nSize = m_listRenderWnd.size();
		return IPC_Succeed;
	}
	else
	{
		if (!nSize)
			return IPC_Error_InvalidParameters;
		if (nSize < m_listRenderWnd.size())
			return IPC_Error_BufferOverflow;
		else
		{
			int nRetSize = 0;
			for (auto it = m_listRenderWnd.begin(); it != m_listRenderWnd.end(); it++)
				hWndArray[nRetSize++] = (*it)->hRenderWnd;
			nSize = nRetSize;
			return IPC_Succeed;
		}
	}
}

// ÉèÖÃÁ÷²¥·ÅÍ·
// ÔÚÁ÷²¥·ÅÊ±£¬²¥·ÅÖ®Ç°£¬±ØÐëÏÈÉèÖÃÁ÷Í·
int CIPCPlayer::SetStreamHeader(CHAR *szStreamHeader, int nHeaderSize)
{
	if (nHeaderSize != sizeof(IPC_MEDIAINFO))
		return IPC_Error_InvalidParameters;
	IPC_MEDIAINFO *pMediaHeader = (IPC_MEDIAINFO *)szStreamHeader;
	if (pMediaHeader->nMediaTag != IPC_TAG)
		return IPC_Error_InvalidParameters;
	m_pMediaHeader = make_shared<IPC_MEDIAINFO>();
	if (m_pMediaHeader)
	{
		memcpy_s(m_pMediaHeader.get(), sizeof(IPC_MEDIAINFO), szStreamHeader, sizeof(IPC_MEDIAINFO));
		m_nSDKVersion = m_pMediaHeader->nSDKversion;
		switch (m_nSDKVersion)
		{
		case IPC_IPC_SDK_VERSION_2015_09_07:
		case IPC_IPC_SDK_VERSION_2015_10_20:
		case IPC_IPC_SDK_GSJ_HEADER:
		{
			m_nVideoFPS = 25;
			m_nVideoCodec = CODEC_UNKNOWN;
			m_nVideoWidth = 0;
			m_nVideoHeight = 0;
			break;
		}
		case IPC_IPC_SDK_VERSION_2015_12_16:
		{
			m_nVideoCodec = m_pMediaHeader->nVideoCodec;
			m_nAudioCodec = m_pMediaHeader->nAudioCodec;
			if (m_nVideoCodec == CODEC_UNKNOWN)
			{
				m_nVideoWidth = 0;
				m_nVideoHeight = 0;
			}
			else
			{
				m_nVideoWidth = m_pMediaHeader->nVideoWidth;
				m_nVideoHeight = m_pMediaHeader->nVideoHeight;
			}
			if (m_pMediaHeader->nFps == 0)
				m_nVideoFPS = 25;
			else
				m_nVideoFPS = m_pMediaHeader->nFps;
			m_nPlayFrameInterval = 1000 / m_nVideoFPS;
		}
		break;
		default:
			return IPC_Error_InvalidSDKVersion;
		}
		m_nFileFrameInterval = 1000 / m_nVideoFPS;
		return IPC_Succeed;
	}
	else
		return IPC_Error_InsufficentMemory;
}

int CIPCPlayer::SetBorderRect(HWND hWnd, LPRECT pRectBorder, bool bPercent )
{
	RECT rtVideo = { 0 };
	// 		rtVideo.left = rtBorder.left;
	// 		rtVideo.right = m_nVideoWidth - rtBorder.right;
	// 		rtVideo.top += rtBorder.top;
	// 		rtVideo.bottom = m_nVideoHeight - rtBorder.bottom;
	if (bPercent)
	{
		if ((pRectBorder->left + pRectBorder->right) >= 100 ||
			(pRectBorder->top + pRectBorder->bottom) >= 100)
			return IPC_Error_InvalidParameters;
	}
	else
	{
		if ((pRectBorder->left + pRectBorder->right) >= m_nVideoWidth ||
			(pRectBorder->top + pRectBorder->bottom) >= m_nVideoHeight)
			return IPC_Error_InvalidParameters;
	}

	LineLockAgent(m_cslistRenderWnd);
	auto itFind = find_if(m_listRenderWnd.begin(), m_listRenderWnd.end(), WndFinder(hWnd));
	if (itFind != m_listRenderWnd.end())
		(*itFind)->SetBorder(pRectBorder, bPercent);
	return IPC_Succeed;
}

int CIPCPlayer::SetSwitcherCallBack(WORD nScreenWnd, HWND hWnd,void *pVideoSwitchCB , void *pUserPtr)
{
	byte nScreen = HIBYTE(nScreenWnd);
	byte nWnd = LOBYTE(nScreenWnd);
	if (nScreen > 15 || nWnd>64)
		return IPC_Error_InvalidParameters;
	
	if (pVideoSwitchCB &&
		pUserPtr)
	{
		TraceMsgA("%s Screen:%d\tWnd:%d\tm_hRenderWnd = %08X\n", __FUNCTION__, nScreen, nWnd, m_hRenderWnd);
		SwitcherPtrListAgent* pSwitcherListAgent = &g_SwitcherList[nScreen][nWnd];
		pSwitcherListAgent->cs.Lock();
		pSwitcherListAgent->list.push_back(make_shared<CSwitcherInfo>(hWnd, this, pVideoSwitchCB, pUserPtr));
		pSwitcherListAgent->cs.Unlock();
		//m_csMapSwitcher.Lock();
		auto itFind = m_MapSwitcher.find(nScreenWnd);
		if (itFind == m_MapSwitcher.end())
		{
			m_MapSwitcher.insert(pair<WORD,SwitcherPtrListAgent*>(nScreenWnd, pSwitcherListAgent));
		}
		//m_csMapSwitcher.Unlock();

		return IPC_Succeed;
	}
	else
		return IPC_Error_InvalidParameters;
}
int CIPCPlayer::StartPlay(bool bEnaleAudio , bool bEnableHaccel , bool bFitWindow)
{
#ifdef _DEBUG
	OutputMsg("%s \tObject:%d Time = %d.\n", __FUNCTION__, m_nObjIndex, timeGetTime() - m_nLifeTime);
#endif
	m_bPause = false;
	m_bFitWindow = bFitWindow;
	if (GetOsMajorVersion() >= 6)
		m_bEnableHaccel = bEnableHaccel;
	m_bPlayerInialized = false;
	// 		if (!m_hWnd || !IsWindow(m_hWnd))
	// 		{
	// 			return IPC_Error_InvalidWindow;
	// 		}
	if (m_pszFileName)
	{
		if (m_hVideoFile == INVALID_HANDLE_VALUE)
		{
			return GetLastError();
		}

		if (!m_pMediaHeader ||	// ÎÄ¼þÍ·ÎÞÐ§
			!m_nTotalFrames)	// ÎÞ·¨È¡µÃÊÓÆµ×ÜÖ¡Êý
			return IPC_Error_NotVideoFile;
		// Æô¶¯ÎÄ¼þ½âÎöÏß³Ì
		m_bThreadParserRun = true;
		m_hThreadFileParser = (HANDLE)_beginthreadex(nullptr, 0, ThreadFileParser, this, 0, 0);

	}
	else
	{
		m_listVideoCache.clear();
		m_listAudioCache.clear();
	}

	if (m_hRenderWnd)
		AddRenderWindow(m_hRenderWnd, nullptr);
	else
	{
		OutputMsg("%s Warning!Render Windows is null!\n", __FUNCTION__);
	}

	m_bStopFlag = false;
	// Æô¶¯Á÷²¥·ÅÏß³Ì
	m_bThreadDecodeRun = true;
// 	if (m_pStreamParser)
// 	{
// 		m_bStreamParserRun = true;
// 		m_hThreadStreamParser = (HANDLE)_beginthreadex(nullptr, 256 * 1024, ThreadStreamParser, this, 0, 0);
// 	}
// 	m_pDecodeHelperPtr = make_shared<DecodeHelper>();
// 	m_hQueueTimer = m_TimeQueue.CreateTimer(TimerCallBack, this, 0, 20);
	m_hRenderAsyncEvent = CreateEvent(nullptr, false, false, nullptr);
	if (m_bAsyncRender)
	{
		m_listAVFrame.clear();
		m_hThreadDecode = (HANDLE)_beginthreadex(nullptr, 256 * 1024, ThreadSyncDecode, this, 0, 0);
		if (!m_bPlayOneFrame)		// Î´ÆôÓÃµ¥Ö¡²¥·Å
			m_hThreadAsyncReander = (HANDLE)_beginthreadex(nullptr, 256 * 1024, ThreadAsyncRender, this, 0, 0);
	}
	else
	{
		//m_hThreadDecode = (HANDLE)_beginthreadex(nullptr, 256 * 1024, ThreadDecodeCache, this, CREATE_SUSPENDED, 0);
		m_hThreadDecode = (HANDLE)_beginthreadex(nullptr, 256 * 1024, ThreadDecode, this, CREATE_SUSPENDED, 0);
		SetThreadPriority(m_hThreadDecode, THREAD_PRIORITY_HIGHEST);
		ResumeThread(m_hThreadDecode);

	}
		
	//m_hThreadReander = (HANDLE)_beginthreadex(nullptr, 256*1024, ThreadRender, this, 0, 0);

	if (!m_hThreadDecode)
	{
#ifdef _DEBUG
		OutputMsg("%s \tObject:%d ThreadPlayVideo Start failed:%d.\n", __FUNCTION__, m_nObjIndex, GetLastError());
#endif
		return IPC_Error_VideoThreadStartFailed;
	}
#ifndef _WIN64
	if (m_hRenderWnd)
		EnableAudio(bEnaleAudio);
	else
		EnableAudio(false);
#endif
	m_dwStartTime = timeGetTime();
	return IPC_Succeed;
}

int CIPCPlayer::StartSyncPlay(bool bFitWindow,CIPCPlayer *pSyncSource,int nVideoFPS )
{
#ifdef _DEBUG
	OutputMsg("%s \tObject:%d Time = %d.\n", __FUNCTION__, m_nObjIndex, timeGetTime() - m_nLifeTime);
#endif
	if (!pSyncSource)
	{// ×ÔÉí³ÉÎªÍ¬²½Ô´
		if (nVideoFPS <= 0)
			return IPC_Error_InvalidParameters;

		int nIPCPlayInterval = 1000 / nVideoFPS;
		m_nVideoFPS = nVideoFPS;
		m_hRenderAsyncEvent = CreateEvent(nullptr, false, false, nullptr);
		m_pRenderTimer = make_shared<CMMEvent>(m_hRenderAsyncEvent, nIPCPlayInterval);
	}
	else
	{// pSyncSource³ÉÎªÍ¬²½Ô´
		m_pSyncPlayer = pSyncSource;
		m_nVideoFPS = pSyncSource->m_nVideoFPS;
		m_hRenderAsyncEvent = pSyncSource->m_hRenderAsyncEvent;
	}
	m_bPause = false;
	m_bFitWindow = bFitWindow;
	
	m_bEnableHaccel = false;
	m_bPlayerInialized = false;

	m_listVideoCache.clear();
	m_listAudioCache.clear();

	AddRenderWindow(m_hRenderWnd, nullptr);

	m_bStopFlag = false;
	// Æô¶¯Á÷²¥·ÅÏß³Ì
	m_bThreadDecodeRun = true;
	
	m_hThreadDecode = (HANDLE)_beginthreadex(nullptr, 256 * 1024, ThreadSyncDecode, this, 0, 0);
	//m_hThreadAsyncReander = (HANDLE)_beginthreadex(nullptr, 256 * 1024, ThreadAsyncRender, this, 0, 0);

	if (!m_hThreadDecode)
	{
#ifdef _DEBUG
		OutputMsg("%s \tObject:%d ThreadPlayVideo Start failed:%d.\n", __FUNCTION__, m_nObjIndex, GetLastError());
#endif
		return IPC_Error_VideoThreadStartFailed;
	}
#ifndef _WIN64
	EnableAudio(false);
#endif
	m_dwStartTime = timeGetTime();
	return IPC_Succeed;
}
int CIPCPlayer::GetFileHeader()
{
	if (!m_hVideoFile)
		return IPC_Error_FileNotOpened;
	DWORD dwBytesRead = 0;
	m_pMediaHeader = make_shared<IPC_MEDIAINFO>();
	if (!m_pMediaHeader)
	{
		CloseHandle(m_hVideoFile);
		return -1;
	}

	if (SetFilePointer(m_hVideoFile, 0, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		assert(false);
		return 0;
	}

	if (!ReadFile(m_hVideoFile, (void *)m_pMediaHeader.get(), sizeof(IPC_MEDIAINFO), &dwBytesRead, nullptr))
	{
		CloseHandle(m_hVideoFile);
		return GetLastError();
	}
	// ·ÖÎöÊÓÆµÎÄ¼þÍ·
	if ((m_pMediaHeader->nMediaTag != IPC_TAG &&
		m_pMediaHeader->nMediaTag != GSJ_TAG) ||
		dwBytesRead != sizeof(IPC_MEDIAINFO))
	{
		CloseHandle(m_hVideoFile);
		return IPC_Error_NotVideoFile;
	}
	m_nSDKVersion = m_pMediaHeader->nSDKversion;
	switch (m_nSDKVersion)
	{

	case IPC_IPC_SDK_VERSION_2015_09_07:
	case IPC_IPC_SDK_VERSION_2015_10_20:
	case IPC_IPC_SDK_GSJ_HEADER:
	{
		m_nVideoFPS = 25;
		if (!m_pMediaHeader->nVideoWidth || !m_pMediaHeader->nVideoHeight)
		{
			m_nVideoCodec = CODEC_UNKNOWN;
			m_nVideoWidth = 0;
			m_nVideoHeight = 0;
		}
	}
	break;
	case IPC_IPC_SDK_VERSION_2015_12_16:
	{
		m_nVideoCodec = m_pMediaHeader->nVideoCodec;
		m_nAudioCodec = m_pMediaHeader->nAudioCodec;
		if (m_nVideoCodec == CODEC_UNKNOWN)
		{
			m_nVideoWidth = 0;
			m_nVideoHeight = 0;
		}
		else
		{
			m_nVideoWidth = m_pMediaHeader->nVideoWidth;
			m_nVideoHeight = m_pMediaHeader->nVideoHeight;
			//if (!m_nVideoWidth || !m_nVideoHeight)
				//return IPC_Error_MediaFileHeaderError;
				m_nVideoCodec = CODEC_UNKNOWN;
		}
		if (m_pMediaHeader->nFps == 0)
			m_nVideoFPS = 25;
		else
			m_nVideoFPS = m_pMediaHeader->nFps;
	}
	break;
	default:

		return IPC_Error_InvalidSDKVersion;
	}
	m_nFileFrameInterval = 1000 / m_nVideoFPS;

	return IPC_Succeed;
}

int CIPCPlayer::InputStream(unsigned char *szFrameData, int nFrameSize, UINT nCacheSize, bool bThreadInside /*ÊÇ·ñÄÚ²¿Ïß³Ìµ÷ÓÃ±êÖ¾*/)
{
	if (!szFrameData || nFrameSize < sizeof(IPCFrameHeader))
		return IPC_Error_InvalidFrame;

	m_bIpcStream = false;
	int nMaxCacheSize = m_nMaxFrameCache;
	if (nCacheSize != 0)
		nMaxCacheSize = nCacheSize;
	if (m_bStopFlag)
		return IPC_Error_PlayerHasStop;
	if (!bThreadInside)
	{// Èô²»ÊÇÄÚ²¿Ïß³Ì£¬ÔòÐèÒª¼ì²éÊÓÆµºÍÒôÆµ½âÂëÊÇ·ñÒÑ¾­ÔËÐÐ
		if (!m_bThreadDecodeRun || !m_hThreadDecode)
		{
#ifdef _DEBUG
			//				OutputMsg("%s \tObject:%d Video decode thread not run.\n", __FUNCTION__, m_nObjIndex);
#endif
			return IPC_Error_VideoThreadNotRun;
		}
	}

	IPCFrameHeader *pHeaderEx = (IPCFrameHeader *)szFrameData;
	if (m_nSDKVersion >= IPC_IPC_SDK_VERSION_2015_12_16 && m_nSDKVersion != IPC_IPC_SDK_GSJ_HEADER)
	{
		switch (pHeaderEx->nType)
		{
		case FRAME_P:
		case FRAME_B:
		case 0:
		case FRAME_I:
		case FRAME_IDR:
		{
			// 				if (!m_hThreadPlayVideo)
			// 					return IPC_Error_VideoThreadNotRun;
			CAutoLock lock(&m_csVideoCache, false, __FILE__, __FUNCTION__, __LINE__);
			if (m_listVideoCache.size() >= nMaxCacheSize)
				return IPC_Error_FrameCacheIsFulled;
			StreamFramePtr pStream = make_shared<StreamFrame>(szFrameData, nFrameSize, m_nFileFrameInterval, m_nSDKVersion);
			if (!pStream)
				return IPC_Error_InsufficentMemory;
			m_listVideoCache.push_back(pStream);
			break;
		}
		case FRAME_G711A:      // G711 AÂÉ±àÂëÖ¡
		case FRAME_G711U:      // G711 UÂÉ±àÂëÖ¡
		case FRAME_G726:       // G726±àÂëÖ¡
		case FRAME_AAC:        // AAC±àÂëÖ¡¡£
		{
			// 				if (!m_hThreadPlayVideo)
			// 					return IPC_Error_AudioThreadNotRun;

			if (m_fPlayRate != 1.0f)
				break;
			CAutoLock lock(&m_csAudioCache, false, __FILE__, __FUNCTION__, __LINE__);
			if (m_listAudioCache.size() >= nMaxCacheSize * 2)
			{
				if (m_bEnableAudio)
					return IPC_Error_FrameCacheIsFulled;
				else
					m_listAudioCache.pop_front();
			}
			StreamFramePtr pStream = make_shared<StreamFrame>(szFrameData, nFrameSize, m_nFileFrameInterval / 2);
			if (!pStream)
				return IPC_Error_InsufficentMemory;
			m_listAudioCache.push_back(pStream);
			m_nAudioFrames++;
			break;
		}
		default:
		{
			assert(false);
			return IPC_Error_InvalidFrameType;
			break;
		}
		}
	}
	else
	{
		switch (pHeaderEx->nType)
		{
		case 0:				// ÊÓÆµBPÖ¡
		case 1:				// ÊÓÆµIÖ¡
		{
			CAutoLock lock(&m_csVideoCache, false, __FILE__, __FUNCTION__, __LINE__);
			if (m_listVideoCache.size() >= nMaxCacheSize)
				return IPC_Error_FrameCacheIsFulled;
			StreamFramePtr pStream = make_shared<StreamFrame>(szFrameData, nFrameSize, m_nFileFrameInterval, m_nSDKVersion);
			if (!pStream)
				return IPC_Error_InsufficentMemory;
			m_listVideoCache.push_back(pStream);
			break;
		}

		case 2:				// ÒôÆµÖ¡
		case FRAME_G711U:
			//case FRAME_G726:    // G726±àÂëÖ¡
		{
			if (!m_bEnableAudio)
				break;
			if (m_fPlayRate != 1.0f)
				break;
			CAutoLock lock(&m_csAudioCache, false, __FILE__, __FUNCTION__, __LINE__);
			if (m_listAudioCache.size() >= nMaxCacheSize * 2)
				return IPC_Error_FrameCacheIsFulled;
			Frame(szFrameData)->nType = CODEC_G711U;			// ¾É°æSDKÖ»Ö§³ÖG711U½âÂë£¬ËùÒÔÕâÀïÇ¿ÖÆ×ª»»ÎªG711U£¬ÒÔÕýÈ·½âÂë
			StreamFramePtr pStream = make_shared<StreamFrame>(szFrameData, nFrameSize, m_nFileFrameInterval / 2);
			if (!pStream)
				return IPC_Error_InsufficentMemory;
			m_listAudioCache.push_back(pStream);
			m_nAudioFrames++;
			break;
		}
		default:
		{
			//assert(false);
			return IPC_Error_InvalidFrameType;
			break;
		}
		}
	}

	return IPC_Succeed;
}

int CIPCPlayer::InputStream(IN byte *pFrameData, IN int nFrameType, IN int nFrameLength, int nFrameNum, time_t nFrameTime)
{
	if (m_bStopFlag)
		return IPC_Error_PlayerHasStop;
#ifdef _DEBUG
	nFrameTime = (time_t)(timeGetTime());
#endif
	if (!m_bThreadDecodeRun || !m_hThreadDecode)
	{
		CAutoLock lock(&m_csVideoCache, false, __FILE__, __FUNCTION__, __LINE__);
		if (m_listVideoCache.size() >= 25)
		{
			OutputMsg("%s \tObject:%d Video decode thread not run.\n", __FUNCTION__, m_nObjIndex);
			return IPC_Error_VideoThreadNotRun;
		}
	}
	DWORD dwThreadCode = 0;
	GetExitCodeThread(m_hThreadDecode, &dwThreadCode);
	if (dwThreadCode != STILL_ACTIVE)		// Ïß³ÌÒÑÍË³ö
	{
		OutputMsg("%s ThreadDecode has exit Abnormally.\n", __FUNCTION__);
		return IPC_Error_VideoThreadAbnormalExit;
	}

	m_bIpcStream = true;
	switch (nFrameType)
	{
	case 0:									// º£Ë¼IÖ¡ºÅÎª0£¬ÕâÊÇ¹Ì¼þµÄÒ»¸öBUG£¬ÓÐ´ýÐÞÕý
	case IPC_IDR_FRAME: 	// IDRÖ¡¡£
	case IPC_I_FRAME:		// IÖ¡¡£		
	case IPC_P_FRAME:       // PÖ¡¡£
	case IPC_B_FRAME:       // BÖ¡¡£
	case IPC_GOV_FRAME: 	// GOVÖ¡¡£
	{
		StreamFramePtr pStream = make_shared<StreamFrame>(pFrameData, nFrameType, nFrameLength, nFrameNum, nFrameTime);
		CAutoLock lock(&m_csVideoCache, false, __FILE__, __FUNCTION__, __LINE__);
		if (m_listVideoCache.size() >= m_nMaxFrameCache)
		{
			// #ifdef _DEBUG
			// 					OutputMsg("%s \tObject:%d Video Frame cache is Fulled.\n", __FUNCTION__, m_nObjIndex);
			// #endif
			return IPC_Error_FrameCacheIsFulled;
		}
		if (!pStream)
		{
			// #ifdef _DEBUG
			// 					OutputMsg("%s \tObject:%d InsufficentMemory when alloc memory for video frame.\n", __FUNCTION__, m_nObjIndex);
			// #endif
			return IPC_Error_InsufficentMemory;
		}
		m_listVideoCache.push_back(pStream);
		SetEvent(m_hInputFrameEvent);
	}
	break;
	case IPC_711_ALAW:      // 711 AÂÉ±àÂëÖ¡
	case IPC_711_ULAW:      // 711 UÂÉ±àÂëÖ¡
	case IPC_726:           // 726±àÂëÖ¡
	case IPC_AAC:           // AAC±àÂëÖ¡¡£
	{
		m_nAudioCodec = (IPC_CODEC)nFrameType;
		// 				if ((timeGetTime() - m_dwInputStream) >= 20000)
		// 				{
		// 					OutputMsg("%s VideoFrames = %d\tAudioFrames = %d.\n", __FUNCTION__, m_nVideoFraems, m_nAudioFrames1);
		// 					m_dwInputStream = timeGetTime();
		// 				}
		if (!m_bEnableAudio)
			break;
		StreamFramePtr pStream = make_shared<StreamFrame>(pFrameData, nFrameType, nFrameLength, nFrameNum, nFrameTime);
		if (!pStream)
			return IPC_Error_InsufficentMemory;
		CAutoLock lock(&m_csAudioCache, false, __FILE__, __FUNCTION__, __LINE__);
		m_nAudioFrames++;
		m_listAudioCache.push_back(pStream);
	}
	break;
	default:
	{
		assert(false);
		break;
	}
	}
	return 0;
}

// ÊäÈëÎ´½âÎöÂëÁ÷
int CIPCPlayer::InputStream(IN byte *pData, IN int nLength)
{
	//TraceFunction();
	if (!pData || !nLength)
		return IPC_Error_InvalidParameters;
	if (m_pStreamParser/* || m_hThreadStreamParser*/)
	{
		m_bIpcStream = true;
#ifdef _DEBUG
		if (m_dfFirstFrameTime == 0.0f)
			m_dfFirstFrameTime = GetExactTime();
#endif
		CAutoLock lock(&m_csVideoCache,false,__FILE__,__FUNCTION__,__LINE__);
		if (m_listVideoCache.size() >= m_nMaxFrameCache)
			return IPC_Error_FrameCacheIsFulled;
		lock.Unlock();
		list<FrameBufferPtr> listFrame;
		if (m_pStreamParser->ParserFrame(pData, nLength, listFrame) > 0)
		{
			for (auto it = listFrame.begin(); it != listFrame.end();)
			{
				IPC_STREAM_TYPE nFrameType;
				if (IsH264KeyFrame((*it)->pBuffer))
					nFrameType = IPC_I_FRAME;
				else
					nFrameType = IPC_P_FRAME;
				if (InputStream((*it)->pBuffer, nFrameType, (*it)->nLength, 0, 0) == IPC_Succeed)
				{
					it = listFrame.erase(it);
				}
				else
				{
					Sleep(10);
					continue;
				}
			}
		}
		return IPC_Succeed;
	}
	else
		return IPC_Error_StreamParserNotStarted;
}
int CIPCPlayer::InputDHStream(byte *pBuffer, int nLength)
{
	if (!pBuffer || !nLength)
		return IPC_Error_InvalidParameters;
	if (!m_pDHStreamParser)
	{
		m_pDHStreamParser = make_shared<DhStreamParser>();
	}
	
	m_pDHStreamParser->InputData(pBuffer, nLength);
	DH_FRAME_INFO *pDHFrame = nullptr;
	do
	{
		pDHFrame = m_pDHStreamParser->GetNextFrame();
		if (!pDHFrame)	// ÒÑ¾­Ã»ÓÐÊý¾Ý¿ÉÒÔ½âÎö£¬ÖÕÖ¹Ñ­»·
			break;
		if (pDHFrame->nType != DH_FRAME_TYPE_VIDEO)
			break;
		int nStatus = InputStream(pDHFrame->pContent,
			pDHFrame->nSubType == DH_FRAME_TYPE_VIDEO_I_FRAME ? IPC_I_FRAME : IPC_P_FRAME,
			pDHFrame->nLength,
			pDHFrame->nRequence,
			(time_t)pDHFrame->nTimeStamp);
		if (IPC_Succeed == nStatus)
		{
			pDHFrame = nullptr;
			continue;
		}
		else
			return nStatus;
		
	} while (true);
	return IPC_Succeed;
}
bool CIPCPlayer::StopPlay(DWORD nTimeout)
{
#ifdef _DEBUG
	//TraceFunction();
	OutputMsg("%s \tObject:%d Time = %d.\n", __FUNCTION__, m_nObjIndex, timeGetTime() - m_nLifeTime);
#endif
	TraceMsgA("%s\n", __FUNCTION__);
	m_bStopFlag = true;
	m_bThreadParserRun = false;
	m_bThreadDecodeRun = false;
	m_bThreadPlayAudioRun = false;
	m_bStreamParserRun = false;
	HANDLE hArray[16] = { 0 };
	int nHandles = 0;
	if (!m_pSyncPlayer)
		SetEvent(m_hRenderAsyncEvent);

	//m_csMapSwitcher.Lock();
	for (auto it = m_MapSwitcher.begin(); it != m_MapSwitcher.end();it ++)
	{
		it->second->cs.Lock();
		if (it->second->list.size())
		{
			CSwitcherInfoPtr pSwitchPtr = it->second->list.front();
			if (pSwitchPtr->pPlayerHandle == this)
			{
				pSwitchPtr->Reset();
				it->second->list.pop_front();
			}
		}
		it->second->cs.Unlock();
	}
	//m_csMapSwitcher.Unlock();
#ifndef _WIN64
	if (m_bEnableAudio)
	{
		EnableAudio(false);
	}
#endif

	m_cslistRenderWnd.Lock();
	m_hRenderWnd = nullptr;
	for (auto it = m_listRenderWnd.begin(); it != m_listRenderWnd.end();)
		it = m_listRenderWnd.erase(it);
	m_cslistRenderWnd.Unlock();
	
	m_bPause = false;
	DWORD dwThreadExitCode = 0;
	if (m_hThreadFileParser)
	{
		GetExitCodeThread(m_hThreadFileParser, &dwThreadExitCode);
		if (dwThreadExitCode == STILL_ACTIVE)		// Ïß³ÌÈÔÔÚÔËÐÐ
			hArray[nHandles++] = m_hThreadFileParser;
	}

	// 		if (m_hThreadReander)
	// 		{
	// 			GetExitCodeThread(m_hThreadReander, &dwThreadExitCode);
	// 			if (dwThreadExitCode == STILL_ACTIVE)		// Ïß³ÌÈÔÔÚÔËÐÐ
	// 				hArray[nHandles++] = m_hThreadReander;
	// 		}

	if (m_hThreadStreamParser)
	{
		GetExitCodeThread(m_hThreadStreamParser, &dwThreadExitCode);
		if (dwThreadExitCode == STILL_ACTIVE)		// Ïß³ÌÈÔÔÚÔËÐÐ
			hArray[nHandles++] = m_hThreadStreamParser;
	}

	if (m_hThreadDecode)
	{
		GetExitCodeThread(m_hThreadDecode, &dwThreadExitCode);
		if (dwThreadExitCode == STILL_ACTIVE)		// Ïß³ÌÈÔÔÚÔËÐÐ
			hArray[nHandles++] = m_hThreadDecode;
	}

	if (m_hThreadAsyncReander)
	{
		GetExitCodeThread(m_hThreadAsyncReander, &dwThreadExitCode);
		if (dwThreadExitCode == STILL_ACTIVE)		// Ïß³ÌÈÔÔÚÔËÐÐ
			hArray[nHandles++] = m_hThreadAsyncReander;
	}
	if (m_hThreadPlayAudio)
	{
		ResumeThread(m_hThreadPlayAudio);
		GetExitCodeThread(m_hThreadPlayAudio, &dwThreadExitCode);
		if (dwThreadExitCode == STILL_ACTIVE)		// Ïß³ÌÈÔÔÚÔËÐÐ
			hArray[nHandles++] = m_hThreadPlayAudio;
	}
	// 		if (m_hThreadGetFileSummary)
	// 			hArray[nHandles++] = m_hThreadGetFileSummary;
	m_csAudioCache.Lock();
	m_listAudioCache.clear();
	m_csAudioCache.Unlock();

	m_csVideoCache.Lock();
	m_listVideoCache.clear();
	m_csVideoCache.Unlock();

	m_cslistAVFrame.Lock();
	m_listAVFrame.clear();
	m_cslistAVFrame.Unlock();

	if (nHandles > 0)
	{
		double dfWaitTime = GetExactTime();
		if (WaitForMultipleObjects(nHandles, hArray, true, nTimeout) == WAIT_TIMEOUT)
		{
			OutputMsg("%s Object %d Wait for thread exit timeout.\n", __FUNCTION__, m_nObjIndex);
			m_bAsnycClose = true;
			return false;
		}
		double dfWaitTimeSpan = TimeSpanEx(dfWaitTime);
		OutputMsg("%s Object %d Wait for thread TimeSpan = %.3f.\n", __FUNCTION__, m_nObjIndex, dfWaitTimeSpan);
	}
	else
		OutputMsg("%s Object %d All Thread has exit normal!.\n", __FUNCTION__, m_nObjIndex);
	if (m_hThreadFileParser)
	{
		CloseHandle(m_hThreadFileParser);
		m_hThreadFileParser = nullptr;
		OutputMsg("%s ThreadParser has exit.\n", __FUNCTION__);
	}
	if (m_hThreadDecode)
	{
		CloseHandle(m_hThreadDecode);
		m_hThreadDecode = nullptr;
#ifdef _DEBUG
		OutputMsg("%s ThreadPlayVideo Object:%d has exit.\n", __FUNCTION__, m_nObjIndex);
#endif
	}
	if (m_hThreadAsyncReander)
	{
		CloseHandle(m_hThreadAsyncReander);
		m_hThreadAsyncReander = nullptr;
	}
	if (m_hThreadStreamParser)
	{
		CloseHandle(m_hThreadStreamParser);
		m_hThreadStreamParser = nullptr;
		OutputMsg("%s ThreadStreamParser Object:%d has exit.\n", __FUNCTION__, m_nObjIndex);
	}
	if (m_hThreadPlayAudio)
	{
		CloseHandle(m_hThreadPlayAudio);
		m_hThreadPlayAudio = nullptr;
		OutputMsg("%s ThreadPlayAudio has exit.\n", __FUNCTION__);
	}
#ifndef _WIN64
	EnableAudio(false);
#endif
	if (m_pFrameOffsetTable)
	{
		delete[]m_pFrameOffsetTable;
		m_pFrameOffsetTable = nullptr;
	}

	if (m_hRenderAsyncEvent &&!m_pSyncPlayer)
	{
		CloseHandle(m_hRenderAsyncEvent);
		m_hRenderAsyncEvent = nullptr;
	}
	m_pDHStreamParser = nullptr;	
	m_pStreamParser = nullptr;
	m_hThreadDecode = nullptr;
	m_hThreadFileParser = nullptr;
	m_hThreadPlayAudio = nullptr;
	m_pFrameOffsetTable = nullptr;
	return true;
}

int  CIPCPlayer::EnableHaccel(IN bool bEnableHaccel )
{
	if (m_hThreadDecode)
		return IPC_Error_PlayerHasStart;
	else
	{
		if (bEnableHaccel)
		{
			if (GetOsMajorVersion() >= 6)
			{
				m_nPixelFormat = (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2');
				m_bEnableHaccel = bEnableHaccel;
				m_bD3dShared = true;
			}
			else
				return IPC_Error_UnsupportHaccel;
		}
		else
			m_bEnableHaccel = bEnableHaccel;
		return IPC_Succeed;
	}
}

bool  CIPCPlayer::IsSupportHaccel(IN IPC_CODEC nCodec)
{
	enum AVCodecID nAvCodec = AV_CODEC_ID_NONE;
	switch (nCodec)
	{
	case CODEC_H264:
		nAvCodec = AV_CODEC_ID_H264;
		break;
	case CODEC_H265:
		nAvCodec = AV_CODEC_ID_H265;
		break;
	default:
		return false;
	}
	shared_ptr<CVideoDecoder>pDecoder = make_shared<CVideoDecoder>();
	UINT nAdapter = D3DADAPTER_DEFAULT;
	if (!pDecoder->InitD3D(nAdapter))
		return false;
	if (pDecoder->CodecIsSupported(nAvCodec) == S_OK)
		return true;
	else
		return false;
}

int CIPCPlayer::GetPlayerInfo(PlayerInfo *pPlayInfo)
{
	if (!pPlayInfo)
		return IPC_Error_InvalidParameters;
	if (m_hThreadDecode || m_hVideoFile)
	{
		ZeroMemory(pPlayInfo, sizeof(PlayerInfo));
		pPlayInfo->nVideoCodec = m_nVideoCodec;
		pPlayInfo->nVideoWidth = m_nVideoWidth;
		pPlayInfo->nVideoHeight = m_nVideoHeight;
		pPlayInfo->nAudioCodec = m_nAudioCodec;
		pPlayInfo->bAudioEnabled = m_bEnableAudio;
		pPlayInfo->tTimeEplased = (time_t)(1000 * (GetExactTime() - m_dfTimesStart));
		pPlayInfo->nFPS = m_nVideoFPS;
		pPlayInfo->nPlayFPS = m_nPlayFPS;
		pPlayInfo->nCacheSize = m_nVideoCache;
		pPlayInfo->nCacheSize2 = m_nAudioCache;
		pPlayInfo->bD3DReady = (m_pDxSurface != nullptr)?(m_pDxSurface->m_pDirect3DDeviceEx != nullptr):false;
		if (!m_bIpcStream)
		{
			pPlayInfo->bFilePlayFinished = m_bFilePlayFinished;
			pPlayInfo->nCurFrameID = m_nCurVideoFrame;
			pPlayInfo->nTotalFrames = m_nTotalFrames;
			if (m_nSDKVersion >= IPC_IPC_SDK_VERSION_2015_12_16 && m_nSDKVersion != IPC_IPC_SDK_GSJ_HEADER)
			{
				pPlayInfo->tTotalTime = m_nTotalFrames * 1000 / m_nVideoFPS;
				pPlayInfo->tCurFrameTime = (m_tCurFrameTimeStamp - m_tFirstFrameTime) / 1000;
				pPlayInfo->tFirstFrameTime = m_tFirstFrameTime;
			}
			else
			{
				pPlayInfo->tTotalTime = m_nTotalTime;
				if (m_pMediaHeader->nCameraType == 1)	// °²Ñ¶Ê¿Ïà»ú
				{
					pPlayInfo->tCurFrameTime = (m_tCurFrameTimeStamp - m_FirstFrame.nTimestamp) / (1000 * 1000);
					pPlayInfo->tFirstFrameTime = m_tFirstFrameTime/(1000*1000);
				}
				else
				{
					pPlayInfo->tCurFrameTime = (m_tCurFrameTimeStamp - m_FirstFrame.nTimestamp) / 1000;
					pPlayInfo->nCurFrameID = pPlayInfo->tCurFrameTime*m_nVideoFPS / 1000;
					pPlayInfo->tFirstFrameTime = m_tFirstFrameTime;
				}
			}
		}
		else
			pPlayInfo->tTotalTime = 0;

		return IPC_Succeed;
	}
	else
		return IPC_Error_PlayerNotStart;
}

int CIPCPlayer::SnapShot(IN CHAR *szFileName, IN SNAPSHOT_FORMAT nFileFormat)
{
	if (!szFileName || !strlen(szFileName))
		return -1;
	if (m_hThreadDecode)
	{
		if (WaitForSingleObject(m_hEvnetYUVReady, 5000) == WAIT_TIMEOUT)
			return IPC_Error_PlayerNotStart;
		//char szAvError[1024] = { 0 };
		// 			if (m_pSnapshot && m_pSnapshot->GetPixelFormat() != m_nDecodePirFmt)
		// 				m_pSnapshot.reset();
		/*if (!m_pSnapshot)
		{
			if (m_nDecodePixelFmt == AV_PIX_FMT_DXVA2_VLD)
				m_pSnapshot = make_shared<CSnapshot>(AV_PIX_FMT_YUV420P, m_nVideoWidth, m_nVideoHeight);
			else
				m_pSnapshot = make_shared<CSnapshot>(m_nDecodePixelFmt, m_nVideoWidth, m_nVideoHeight);

			if (!m_pSnapshot)
				return IPC_Error_InsufficentMemory;
			if (m_pSnapshot->SetCodecID(m_nVideoCodec) != IPC_Succeed)
				return IPC_Error_UnsupportedCodec;
		}

		SetEvent(m_hEventYUVRequire);
		if (WaitForSingleObject(m_hEventFrameCopied, 2000) == WAIT_TIMEOUT)
			return IPC_Error_SnapShotFailed;
		int nResult = IPC_Succeed;
		switch (nFileFormat)
		{
		case XIFF_BMP:
			if (!m_pSnapshot->SaveBmp(szFileName))
				nResult = IPC_Error_SnapShotFailed;
			break;
		case XIFF_JPG:
			if (!m_pSnapshot->SaveJpeg(szFileName))
				nResult = IPC_Error_SnapShotFailed;
			break;
		default:
			nResult = IPC_Error_UnsupportedFormat;
			break;
		}*/
		if (!m_pDxSurface)
			return IPC_Error_PlayerNotStart;
		if (m_pDxSurface->CapturetoFile(szFileName, (D3DXIMAGE_FILEFORMAT)nFileFormat))
			return IPC_Succeed;
		else
			return IPC_Error_SnapShotFailed;
	}
	else
		return IPC_Error_PlayerNotStart;
}

void CIPCPlayer::ProcessSnapshotRequire(AVFrame *pAvFrame)
{
	if (!pAvFrame)
		return;
	if (WaitForSingleObject(m_hEventYUVRequire, 0) == WAIT_TIMEOUT)
		return;

	if (pAvFrame->format == AV_PIX_FMT_YUV420P ||
		pAvFrame->format == AV_PIX_FMT_YUVJ420P)
	{// ÔÝ²»Ö§³Ödxva Ó²½âÂëÖ¡
		m_pSnapshot->CopyFrame(pAvFrame);
		SetEvent(m_hEventFrameCopied);
	}
	else if (pAvFrame->format == AV_PIX_FMT_DXVA2_VLD)
	{
		m_pSnapshot->CopyDxvaFrame(pAvFrame);
		SetEvent(m_hEventFrameCopied);
	}
	else
	{
		return;
	}
}

int CIPCPlayer::SetRate(IN float fPlayRate)
{
#ifdef _DEBUG
	m_nRenderFPS = 0;
	dfTRender = 0.0f;
	m_nRenderFrames = 0;
#endif
	if (m_bIpcStream)
	{
		return IPC_Error_NotFilePlayer;
	}
	if (fPlayRate > (float)m_nVideoFPS)
		fPlayRate = m_nVideoFPS;
	// È¡µÃµ±Ç°ÏÔÊ¾Æ÷µÄË¢ÐÂÂÊ£¬ÏÔÊ¾Æ÷µÄË¢ÐÂÂÊ¾ö¶¨ÁËÏÔÊ¾Í¼ÏñµÄ×î¸ßÖ¡Êý
	// Í¨¹ýÍ³¼ÆÃ¿ÏÔÊ¾Ò»Ö¡Í¼Ïñ(º¬½âÂëºÍÏÔÊ¾)ºÄ·ÑµÄÊ±¼ä

	DEVMODE   dm;
	dm.dmSize = sizeof(DEVMODE);
	::EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
	m_fPlayInterval = (int)(1000 / (m_nVideoFPS*fPlayRate));
	/// marked by xionggao.lee @2016.03.23
	// 		float nMinPlayInterval = ((float)1000) / dm.dmDisplayFrequency;
	// 		if (m_fPlayInterval < nMinPlayInterval)
	// 			m_fPlayInterval = nMinPlayInterval;
	m_nPlayFPS = 1000 / m_fPlayInterval;
	m_fPlayRate = fPlayRate;

	return IPC_Succeed;
}

int CIPCPlayer::SeekFrame(IN int nFrameID, bool bUpdate )
{
	if (!m_bSummaryIsReady)
		return IPC_Error_SummaryNotReady;

	if (!m_hVideoFile || !m_pFrameOffsetTable)
		return IPC_Error_NotFilePlayer;

	if (nFrameID < 0 || nFrameID > m_nTotalFrames)
		return IPC_Error_InvalidFrame;
	m_csVideoCache.Lock();
	m_listVideoCache.clear();
	m_csVideoCache.Unlock();

	m_csAudioCache.Lock();
	m_listAudioCache.clear();
	m_csAudioCache.Unlock();

	// ´ÓÎÄ¼þÕªÒªÖÐ£¬È¡µÃÎÄ¼þÆ«ÒÆÐÅÏ¢
	// ²éÕÒ×î½üµÄIÖ¡
	int nForward = nFrameID, nBackWord = nFrameID;
	while (nForward < m_nTotalFrames)
	{
		if (m_pFrameOffsetTable[nForward].bIFrame)
			break;
		nForward++;
	}
	if (nForward >= m_nTotalFrames)
		nForward--;

	while (nBackWord > 0 && nBackWord < m_nTotalFrames)
	{
		if (m_pFrameOffsetTable[nBackWord].bIFrame)
			break;
		nBackWord--;
	}

	if ((nForward - nFrameID) <= (nFrameID - nBackWord))
		m_nFrametoRead = nForward;
	else
		m_nFrametoRead = nBackWord;
	m_nCurVideoFrame = m_nFrametoRead;
	//OutputMsg("%s Seek to Frame %d\tFrameTime = %I64d\n", __FUNCTION__, m_nFrametoRead, m_pFrameOffsetTable[m_nFrametoRead].tTimeStamp/1000);
	if (m_hThreadFileParser)
		SetSeekOffset(m_pFrameOffsetTable[m_nFrametoRead].nOffset);
	else
	{// Ö»ÓÃÓÚµ¥´¿µÄ½âÎöÎÄ¼þÊ±ÒÆ¶¯ÎÄ¼þÖ¸Õë
		CAutoLock lock(&m_csParser, false, __FILE__, __FUNCTION__, __LINE__);
		m_nParserOffset = 0;
		m_nParserDataLength = 0;
		LONGLONG nOffset = m_pFrameOffsetTable[m_nFrametoRead].nOffset;
		if (LargerFileSeek(m_hVideoFile, nOffset, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		{
			OutputMsg("%s LargerFileSeek  Failed,Error = %d.\n", __FUNCTION__, GetLastError());
			return GetLastError();
		}
	}
	if (bUpdate &&
		m_hThreadDecode &&	// ±ØÐëÆô¶¯²¥·ÅÏß³Ì
		m_bPause &&				// ±ØÐëÊÇÔÝÍ£Ä£Ê½			
		m_pDecoder)				// ½âÂëÆ÷±ØÐëÒÑÆô¶¯
	{
		// ¶ÁÈ¡Ò»Ö¡,²¢ÓèÒÔ½âÂë,ÏÔÊ¾
		DWORD nBufferSize = m_pFrameOffsetTable[m_nFrametoRead].nFrameSize;
		LONGLONG nOffset = m_pFrameOffsetTable[m_nFrametoRead].nOffset;

		byte *pBuffer = _New byte[nBufferSize + 1];
		if (!pBuffer)
			return IPC_Error_InsufficentMemory;

		unique_ptr<byte>BufferPtr(pBuffer);
		DWORD nBytesRead = 0;
		if (LargerFileSeek(m_hVideoFile, nOffset, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		{
			OutputMsg("%s LargerFileSeek  Failed,Error = %d.\n", __FUNCTION__, GetLastError());
			return GetLastError();
		}

		if (!ReadFile(m_hVideoFile, pBuffer, nBufferSize, &nBytesRead, nullptr))
		{
			OutputMsg("%s ReadFile Failed,Error = %d.\n", __FUNCTION__, GetLastError());
			return GetLastError();
		}
		AVPacket *pAvPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
		shared_ptr<AVPacket>AvPacketPtr(pAvPacket, av_free);
		AVFrame *pAvFrame = av_frame_alloc();
		shared_ptr<AVFrame>AvFramePtr(pAvFrame, av_free);
		av_init_packet(pAvPacket);
		m_nCurVideoFrame = Frame(pBuffer)->nFrameID;
		pAvPacket->size = Frame(pBuffer)->nLength;
		if (m_nSDKVersion >= IPC_IPC_SDK_VERSION_2015_12_16 && m_nSDKVersion != IPC_IPC_SDK_GSJ_HEADER)
			pAvPacket->data = pBuffer + sizeof(IPCFrameHeaderEx);
		else
			pAvPacket->data = pBuffer + sizeof(IPCFrameHeader);
		int nGotPicture = 0;
		char szAvError[1024] = { 0 };
		int nAvError = m_pDecoder->Decode(pAvFrame, nGotPicture, pAvPacket);
		if (nAvError < 0)
		{
			av_strerror(nAvError, szAvError, 1024);
			OutputMsg("%s Decode error:%s.\n", __FUNCTION__, szAvError);
			return IPC_Error_DecodeFailed;
		}
		av_packet_unref(pAvPacket);
		if (nGotPicture)
		{
			RenderFrame(pAvFrame);
		}
		av_frame_unref(pAvFrame);
		return IPC_Succeed;
	}

	return IPC_Succeed;
}

int CIPCPlayer::AsyncSeekTime(IN time_t tTimeOffset, bool bUpdate)
{
	if (!m_bAsyncRender)
		return IPC_Error_NotAsyncPlayer;
	if (!m_hThreadDecode )
		return IPC_Error_PlayerNotStart;
	if (!m_nVideoWidth || !m_nVideoHeight)
	{
		return IPC_Error_PlayerNotStart;
	}
	if (!InitizlizeDx())
		return IPC_Error_DxError;
	
	LineLockAgent(m_cslistAVFrame);
	if (m_listAVFrame.size() < 1)
		return IPC_Error_InvalidTimeOffset;
	auto itFinder = find_if(m_listAVFrame.begin(), m_listAVFrame.end(), FrameFinder(tTimeOffset));

	if (itFinder != m_listAVFrame.end())
	{
		time_t tLastFrameTime = (*itFinder)->tFrame;
		RenderFrame((*itFinder)->pFrame);
		int nSize1 = m_listAVFrame.size();
		m_listAVFrame.erase(m_listAVFrame.begin(), itFinder);
		int nSize2 = m_listAVFrame.size();
		//OutputMsg("%s Remove Frames = %d\tTimeOffset = %I64d\tLastFrameTime = %I64d.\n", __FUNCTION__, nSize1 - nSize2,tTimeOffset, tLastFrameTime);
		return IPC_Succeed;
	}
	else
	{// Çø¼ä¿ç¶È½Ï´ó£¬Ã»ÓÐÆ¥ÅäµÄÖ¡
		auto itFront = m_listAVFrame.front();
		auto itBack = m_listAVFrame.back();
		if (itBack->tFrame < tTimeOffset)			// Ê±¼äÆ«ÒÆÒÑ¾­³¬¹ýµ±Öµ·¶Î§
		{
			m_listAVFrame.clear();
			return IPC_Error_InvalidTimeOffset;
		}
		else if (itFront->tFrame > tTimeOffset)			// ÍêÈ«²»ÔÚÇø¼äÄÚ
			return IPC_Error_InvalidTimeOffset;
		else
		{	// ²éÕÒÊ±¼äÉÏ×î½Ó½üµÄÖ¡
			time_t tLastTime = 0;
			auto itFinder = m_listAVFrame.end();
			auto itLast = m_listAVFrame.begin();
			for (auto it = m_listAVFrame.begin(); it != m_listAVFrame.end(); it++)
			{
				if (!tLastTime)
				{
					tLastTime = (*it)->tFrame;
					itLast = it;
					continue;
				}
				else
				{
					if ((tTimeOffset >= tLastTime) && (tTimeOffset <= (*it)->tFrame))
					{
						// Æ¥ÅäÊ±¼äÉÏ×î½Ó½üµÄÄÇÒ»Ö¡£¬¼´Ê±¼ä²î×îÐ¡µÄÖ¡
						time_t tTimespan1 = tTimeOffset - tLastTime;
						time_t tTimespan2 = (*it)->tFrame - tTimeOffset;
						if (tTimespan1 < tTimespan2)
							itFinder = itLast;
						else
							itFinder = it;
						break;
					}
					else
					{
						tLastTime = (*it)->tFrame;
						itLast = it;
					}
				}
			}
			if (itFinder != m_listAVFrame.end())
			{
				time_t tLastFrameTime = (*itFinder)->tFrame;
				RenderFrame((*itFinder)->pFrame);
				int nSize1 = m_listAVFrame.size();
				m_listAVFrame.erase(m_listAVFrame.begin(), itFinder);
				int nSize2 = m_listAVFrame.size();
				//OutputMsg("%s Remove Frames = %d\tTimeOffset = %I64d\tLastFrameTime = %I64d.\n", __FUNCTION__, nSize1 - nSize2, tTimeOffset, tLastFrameTime);
				return IPC_Succeed;
			}
			else
				return IPC_Error_InvalidTimeOffset;
		}
	}
	
}

int CIPCPlayer::SeekTime(IN time_t tTimeOffset, bool bUpdate)
{
	if (!m_hVideoFile)
		return IPC_Error_NotFilePlayer;
	if (!m_bSummaryIsReady)
		return IPC_Error_SummaryNotReady;

	int nFrameID = 0;
	if (m_nVideoFPS == 0 || m_nFileFrameInterval == 0)
	{// Ê¹ÓÃ¶þ·Ö·¨²éÕÒ
		nFrameID = BinarySearch(tTimeOffset);
		if (nFrameID == -1)
			return IPC_Error_InvalidTimeOffset;
	}
	else
	{
		int nTimeDiff = tTimeOffset;// -m_pFrameOffsetTable[0].tTimeStamp;
		if (nTimeDiff < 0)
			return IPC_Error_InvalidTimeOffset;
		nFrameID = nTimeDiff / m_nFileFrameInterval;
	}
	return SeekFrame(nFrameID, bUpdate);
}

int CIPCPlayer::GetFrame(INOUT byte **pBuffer, OUT UINT &nBufferSize)
{
	if (!m_hVideoFile)
		return IPC_Error_NotFilePlayer;
	if (m_hThreadDecode || m_hThreadFileParser)
		return IPC_Error_PlayerHasStart;
	if (!m_pFrameOffsetTable || !m_nTotalFrames)
		return IPC_Error_SummaryNotReady;

	DWORD nBytesRead = 0;
	FrameParser Parser;
	CAutoLock lock(&m_csParser, false, __FILE__, __FUNCTION__, __LINE__);
	byte *pFrameBuffer = &m_pParserBuffer[m_nParserOffset];
	if (!ParserFrame(&pFrameBuffer, m_nParserDataLength, &Parser))
	{
		// ²ÐÁôÊý¾Ý³¤ÎªnDataLength
		memmove(m_pParserBuffer, pFrameBuffer, m_nParserDataLength);
		if (!ReadFile(m_hVideoFile, &m_pParserBuffer[m_nParserDataLength], (m_nParserBufferSize - m_nParserDataLength), &nBytesRead, nullptr))
		{
			OutputMsg("%s ReadFile Failed,Error = %d.\n", __FUNCTION__, GetLastError());
			return GetLastError();
		}
		m_nParserOffset = 0;
		m_nParserDataLength += nBytesRead;
		pFrameBuffer = m_pParserBuffer;
		if (!ParserFrame(&pFrameBuffer, m_nParserDataLength, &Parser))
		{
			return IPC_Error_NotVideoFile;
		}
	}
	m_nParserOffset += Parser.nFrameSize;
	*pBuffer = (byte *)Parser.pHeaderEx;
	nBufferSize = Parser.nFrameSize;
	return IPC_Succeed;
}

int CIPCPlayer::SeekNextFrame()
{
	if (m_hThreadDecode &&	// ±ØÐëÆô¶¯²¥·ÅÏß³Ì
		m_bPause &&				// ±ØÐëÊÇÔÝÍ£Ä£Ê½			
		m_pDecoder)				// ½âÂëÆ÷±ØÐëÒÑÆô¶¯
	{
		if (!m_hVideoFile || !m_pFrameOffsetTable)
			return IPC_Error_NotFilePlayer;

		m_csVideoCache.Lock();
		m_listVideoCache.clear();
		//m_nFrameOffset = 0;
		m_csVideoCache.Unlock();

		m_csAudioCache.Lock();
		m_listAudioCache.clear();
		m_csAudioCache.Unlock();

		// ¶ÁÈ¡Ò»Ö¡,²¢ÓèÒÔ½âÂë,ÏÔÊ¾
		DWORD nBufferSize = m_pFrameOffsetTable[m_nCurVideoFrame].nFrameSize;
		LONGLONG nOffset = m_pFrameOffsetTable[m_nCurVideoFrame].nOffset;

		byte *pBuffer = _New byte[nBufferSize + 1];
		if (!pBuffer)
			return IPC_Error_InsufficentMemory;

		unique_ptr<byte>BufferPtr(pBuffer);
		DWORD nBytesRead = 0;
		if (LargerFileSeek(m_hVideoFile, nOffset, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		{
			OutputMsg("%s LargerFileSeek  Failed,Error = %d.\n", __FUNCTION__, GetLastError());
			return GetLastError();
		}

		if (!ReadFile(m_hVideoFile, pBuffer, nBufferSize, &nBytesRead, nullptr))
		{
			OutputMsg("%s ReadFile Failed,Error = %d.\n", __FUNCTION__, GetLastError());
			return GetLastError();
		}
		AVPacket *pAvPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
		shared_ptr<AVPacket>AvPacketPtr(pAvPacket, av_free);
		AVFrame *pAvFrame = av_frame_alloc();
		shared_ptr<AVFrame>AvFramePtr(pAvFrame, av_free);
		av_init_packet(pAvPacket);
		pAvPacket->size = Frame(pBuffer)->nLength;
		if (m_nSDKVersion >= IPC_IPC_SDK_VERSION_2015_12_16 && m_nSDKVersion != IPC_IPC_SDK_GSJ_HEADER)
			pAvPacket->data = pBuffer + sizeof(IPCFrameHeaderEx);
		else
			pAvPacket->data = pBuffer + sizeof(IPCFrameHeader);
		int nGotPicture = 0;
		char szAvError[1024] = { 0 };
		int nAvError = m_pDecoder->Decode(pAvFrame, nGotPicture, pAvPacket);
		if (nAvError < 0)
		{
			av_strerror(nAvError, szAvError, 1024);
			OutputMsg("%s Decode error:%s.\n", __FUNCTION__, szAvError);
			return IPC_Error_DecodeFailed;
		}
		av_packet_unref(pAvPacket);
		if (nGotPicture)
		{
			RenderFrame(pAvFrame);
			ProcessYUVCapture(pAvFrame, (LONGLONG)GetExactTime() * 1000);

			m_tCurFrameTimeStamp = m_pFrameOffsetTable[m_nCurVideoFrame].tTimeStamp;
			m_nCurVideoFrame++;
			LineLockAgent(m_csFilePlayCallBack);
			if (m_pFilePlayCallBack)
				m_pFilePlayCallBack(this, m_pUserFilePlayer);
		}
		av_frame_unref(pAvFrame);
		return IPC_Succeed;
	}
	else
		return IPC_Error_PlayerIsNotPaused;
}

#ifndef _WIN64
int CIPCPlayer::EnableAudio(bool bEnable )
{
	// TraceFunction();
	// 		if (m_fPlayRate != 1.0f)
	// 			return IPC_Error_AudioFailed;
	if (m_nAudioCodec == CODEC_UNKNOWN)
	{
		return IPC_Error_UnsupportedCodec;
	}
	if (m_pDsoundEnum->GetAudioPlayDevices() <= 0)
		return IPC_Error_AudioDeviceNotReady;
	if (bEnable)
	{
		if (!m_pDsPlayer)
			m_pDsPlayer = make_shared<CDSound>(m_hRenderWnd);
		if (!m_pDsPlayer->IsInitialized())
		{
			if (!m_pDsPlayer->Initialize(m_hRenderWnd, m_nAudioPlayFPS, 1, m_nSampleFreq, m_nSampleBit))
			{
				m_pDsPlayer = nullptr;
				m_bEnableAudio = false;
				return IPC_Error_AudioDeviceNotReady;
			}
		}
		m_pDsBuffer = m_pDsPlayer->CreateDsoundBuffer();
		if (!m_pDsBuffer)
		{
			m_bEnableAudio = false;
			assert(false);
			return IPC_Error_AudioDeviceNotReady;
		}
		m_bThreadPlayAudioRun = true;
		m_hAudioFrameEvent[0] = CreateEvent(nullptr, false, true, nullptr);
		m_hAudioFrameEvent[1] = CreateEvent(nullptr, false, true, nullptr);

		m_hThreadPlayAudio = (HANDLE)_beginthreadex(nullptr, 0, m_nAudioPlayFPS == 8 ? ThreadPlayAudioGSJ : ThreadPlayAudioIPC, this, 0, 0);
		m_pDsBuffer->StartPlay();
		m_pDsBuffer->SetVolume(50);
		m_dfLastTimeAudioPlay = 0.0f;
		m_dfLastTimeAudioSample = 0.0f;
		m_bEnableAudio = true;
	}
	else
	{
		if (m_hThreadPlayAudio)
		{
			if (m_bThreadPlayAudioRun)		// ÉÐÎ´Ö´ÐÐÍ£Ö¹ÒôÆµ½âÂëÏß³ÌµÄ²Ù×÷
			{
				m_bThreadPlayAudioRun = false;
				ResumeThread(m_hThreadPlayAudio);
				WaitForSingleObject(m_hThreadPlayAudio, INFINITE);
				CloseHandle(m_hThreadPlayAudio);
				m_hThreadPlayAudio = nullptr;
				OutputMsg("%s ThreadPlayAudio has exit.\n", __FUNCTION__);
				m_csAudioCache.Lock();
				m_listAudioCache.clear();
				m_csAudioCache.Unlock();
			}
			CloseHandle(m_hAudioFrameEvent[0]);
			CloseHandle(m_hAudioFrameEvent[1]);
		}

		if (m_pDsBuffer)
		{
			m_pDsPlayer->DestroyDsoundBuffer(m_pDsBuffer);
			m_pDsBuffer = nullptr;
		}
		if (m_pDsPlayer)
			m_pDsPlayer = nullptr;
		m_bEnableAudio = false;
	}
	return IPC_Succeed;
}
#endif
void CIPCPlayer::Refresh()
{
	LineLockAgent(m_cslistRenderWnd);				
	if (m_listRenderWnd.size() > 0)
	{
		for (auto it = m_listRenderWnd.begin(); it != m_listRenderWnd.end(); it++)
			::InvalidateRect((*it)->hRenderWnd, nullptr, true);
	}
}

void CIPCPlayer::SetBackgroundImage(LPCWSTR szImageFilePath)
{
	if (szImageFilePath)
	{
		if (PathFileExistsW(szImageFilePath))
		{
			m_pszBackImagePath = new WCHAR[wcslen(szImageFilePath) + 1];
			wcscpy_s(m_pszBackImagePath, wcslen(szImageFilePath) + 1, szImageFilePath);
		}
	}
	else
	{
		if (!m_pszBackImagePath)
		{// µÚÒ»´Îµ÷ÓÃ£¬ÇÒÎ´´«ÈëÍ¼ÏñÎÄ¼þÃû£¬ÔòÆôÓÃÄ¬ÈÏÎÄ¼þÃû
			m_pszBackImagePath = new WCHAR[1024];
			GetAppPathW(m_pszBackImagePath, 1024);
			wcscat_s(m_pszBackImagePath, 1024, L"\\BackImage.jpg");
			if (!PathFileExistsW(m_pszBackImagePath))
			{
				delete[]m_pszBackImagePath;
				m_pszBackImagePath = nullptr;
			}
		}
		else
		{
			delete[]m_pszBackImagePath;
			m_pszBackImagePath = nullptr;
		}
	}
}

// Ìí¼ÓÏßÌõÊ§°ÜÊ±£¬·µ»Ø0£¬·ñÔò·µ»ØÏßÌõ×éµÄ¾ä±ú
long CIPCPlayer::AddLineArray(POINT *pPointArray, int nCount, float fWidth, D3DCOLOR nColor)
{
	if (m_pDxSurface)
	{
		return m_pDxSurface->AddD3DLineArray(pPointArray, nCount, fWidth, nColor);
	}
	else
	{
		//assert(false);
		return IPC_Error_DXRenderNotInitialized;
	}
}

int	CIPCPlayer::RemoveLineArray(long nIndex)
{
	if (m_pDxSurface)
	{
		return m_pDxSurface->RemoveD3DLineArray(nIndex);
	}
	else
	{
		//assert(false);
		return IPC_Error_DXRenderNotInitialized;
	}
}

long CIPCPlayer::AddPolygon(POINT *pPointArray, int nCount, WORD *pVertexIndex, DWORD nColor)
{
	if (m_pDxSurface)
		return m_pDxSurface->AddPolygon(pPointArray, nCount, pVertexIndex, nColor);
	else
		return IPC_Error_DXRenderNotInitialized;
}

int CIPCPlayer::RemovePolygon(long nIndex)
{
	if (m_pDxSurface)
	{
		m_pDxSurface->RemovePolygon(nIndex);
		return IPC_Succeed;
	}
	else
	{
		//assert(false);
		return IPC_Error_DXRenderNotInitialized;
	}
}

int CIPCPlayer::SetCallBack(IPC_CALLBACK nCallBackType, IN void *pUserCallBack, IN void *pUserPtr)
{
	switch (nCallBackType)
	{
	case ExternDcDraw:
	{
		m_pDCCallBack = pUserCallBack;
		m_pDCCallBackParam = pUserPtr;
		if (m_pDxSurface)
		{
			m_pDxSurface->SetExternDraw(pUserCallBack, pUserPtr);
		}
		return IPC_Succeed;
	}
	break;
	case ExternDcDrawEx:
		if (m_pDxSurface)
		{
			assert(false);
			return IPC_Succeed;
		}
		else
			return IPC_Error_DxError;
		break;
	case YUVCapture:
	{
		LineLockAgent(m_csCaptureYUV);
		m_pfnCaptureYUV = (CaptureYUV)pUserCallBack;
		m_pUserCaptureYUV = pUserPtr;
	}
	break;
	case YUVCaptureEx:
	{
		LineLockAgent(m_csCaptureYUVEx);
		m_pfnCaptureYUVEx = (CaptureYUVEx)pUserCallBack;
		m_pUserCaptureYUVEx = pUserPtr;
	}
	break;
	case YUVFilter:
	{
		LineLockAgent(m_csYUVFilter);
		m_pfnYUVFilter = (CaptureYUVEx)pUserCallBack;
		m_pUserYUVFilter = pUserPtr;
	}
	break;
	case FramePaser:
	{
		// 			m_pfnCaptureFrame = (CaptureFrame)pUserCallBack;
		// 			m_pUserCaptureFrame = pUserPtr;
	}
	break;
	case FilePlayer:
	{
		AutoLock(&m_csFilePlayCallBack);
		m_pFilePlayCallBack = (FilePlayProc)pUserCallBack;
		m_pUserFilePlayer = pUserPtr;
	}
	break;
	case RGBCapture:
	{
		AutoLock(&m_csCaptureRGB);
		m_pCaptureRGB = (CaptureRGB)pUserCallBack;
		m_pUserCaptureRGB = pUserPtr;
	}
		break;
	default:
		return IPC_Error_InvalidParameters;
		break;
	}
	return IPC_Succeed;
}

int CIPCPlayer::GetFileSummary(volatile bool &bWorking)
{
	//#ifdef _DEBUG
	double dfTimeStart = GetExactTime();
	//#endif
	DWORD nBufferSize = 1024 * 1024 * 2;

	// ²»ÔÙ·ÖÎöÎÄ¼þ£¬ÒòÎªStartPlayÒÑ¾­×÷¹ý·ÖÎöµÄÈ·ÈÏ
	DWORD nOffset = sizeof(IPC_MEDIAINFO);
	if (SetFilePointer(m_hVideoFile, nOffset, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		assert(false);
		return -1;
	}
	if (!m_pFrameOffsetTable)
	{
		m_pFrameOffsetTable = _New FileFrameInfo[m_nTotalFrames];
		ZeroMemory(m_pFrameOffsetTable, sizeof(FileFrameInfo)*m_nTotalFrames);
	}

	byte *pBuffer = _New byte[nBufferSize];
	while (!pBuffer)
	{
		if (nBufferSize <= 1024 * 512)
		{// Á¬512KµÄÄÚ´æ¶¼ÎÞ·¨ÉêÇëµÄ»°£¬ÔòÍË³ö
			OutputMsg("%s Can't alloc enough memory.\n", __FUNCTION__);
			assert(false);
			return IPC_Error_InsufficentMemory;
		}
		nBufferSize /= 2;
		pBuffer = _New byte[nBufferSize];
	}
	shared_ptr<byte>BufferPtr(pBuffer);
	byte *pFrame = nullptr;
	int nFrameSize = 0;
	int nVideoFrames = 0;
	int nAudioFrames = 0;
	LONG nSeekOffset = 0;
	DWORD nBytesRead = 0;
	DWORD nDataLength = 0;
	byte *pFrameBuffer = nullptr;
	FrameParser Parser;
	int nFrameOffset = sizeof(IPC_MEDIAINFO);
	bool bIFrame = false;
	bool bStreamProbed = false;		// ÊÇ·ñÒÑ¾­Ì½²â¹ýÂëÁ÷
	const UINT nMaxCache = 100;
	bool bFirstBlockIsFilled = true;
	int nAllFrames = 0;
	//m_bEnableAudio = true;			// ÏÈ¿ªÆôÒôÆµ±ê¼Ç£¬ÒÔÊäÈëÒôÆµÊý¾Ý,ÈôºóÆÚ¹Ø±ÕÒôÆµ£¬Ôò»º´æÊý¾Ý»á×Ô¶¯É¾³ý

	while (true && bWorking)
	{
		double dfT1 = GetExactTime();
		if (!ReadFile(m_hVideoFile, &pBuffer[nDataLength], (nBufferSize - nDataLength), &nBytesRead, nullptr))
		{
			OutputMsg("%s ReadFile Failed,Error = %d.\n", __FUNCTION__, GetLastError());
			return IPC_Error_ReadFileFailed;
		}
		dfT1 = GetExactTime();
		if (nBytesRead == 0)		// Î´¶ÁÈ¡ÈÎºÎÄÚÈÝ£¬ÒÑ¾­´ïµ½ÎÄ¼þ½áÎ²
			break;
		pFrameBuffer = pBuffer;
		nDataLength += nBytesRead;
		int nLength1 = 0;
		while (true && bWorking)
		{
			if (!ParserFrame(&pFrameBuffer, nDataLength, &Parser))
				break;
			//TraceMsgA("%s FrameID = %d ,Size = %d.\n", __FUNCTION__, Parser.pHeaderEx->nFrameID,Parser.nFrameSize);
			nAllFrames++;
			nLength1 = nBufferSize * 1024 * 1024 - (pFrameBuffer - pBuffer);
			if (nLength1 != nDataLength)
			{
				int nBreak = 3;
			}
			if (bFirstBlockIsFilled)
			{
				if (InputStream((byte *)Parser.pHeader, Parser.nFrameSize, (UINT)nMaxCache, true) == IPC_Error_FrameCacheIsFulled &&
					bWorking)
				{
					m_nSummaryOffset = nFrameOffset;
					m_csVideoCache.Lock();
					int nVideoSize = m_listVideoCache.size();
					m_csVideoCache.Unlock();

					m_csAudioCache.Lock();
					int nAudioSize = m_listAudioCache.size();
					m_csAudioCache.Unlock();

					m_nHeaderFrameID = m_listVideoCache.front()->FrameHeader()->nFrameID;
					OutputMsg("HeadFrame ID = %d.\n", m_nHeaderFrameID);
					bFirstBlockIsFilled = false;
				}
			}

			if (IsIPCVideoFrame(Parser.pHeader, bIFrame, m_nSDKVersion))	// Ö»¼ÇÂ¼ÊÓÆµÖ¡µÄÎÄ¼þÆ«ÒÆ
			{
				// 					if (m_nVideoCodec == CODEC_UNKNOWN &&
				// 						bIFrame &&
				// 						!bStreamProbed)
				// 					{// ³¢ÊÔÌ½²âÂëÁ÷
				// 						bStreamProbed = ProbeStream((byte *)Parser.pRawFrame, Parser.nRawFrameSize);
				// 					}
				if (nVideoFrames < m_nTotalFrames)
				{
					if (m_pFrameOffsetTable)
					{
						m_pFrameOffsetTable[nVideoFrames].nOffset = nFrameOffset;
						m_pFrameOffsetTable[nVideoFrames].nFrameSize = Parser.nFrameSize;
						m_pFrameOffsetTable[nVideoFrames].bIFrame = bIFrame;
						// ¸ù¾ÝÖ¡IDºÍÎÄ¼þ²¥·Å¼ä¸ôÀ´¾«È·µ÷ÕûÃ¿Ò»Ö¡µÄ²¥·ÅÊ±¼ä
						if (m_nSDKVersion >= IPC_IPC_SDK_VERSION_2015_12_16 && m_nSDKVersion != IPC_IPC_SDK_GSJ_HEADER)
							m_pFrameOffsetTable[nVideoFrames].tTimeStamp = nVideoFrames*m_nFileFrameInterval * 1000;
						else
							m_pFrameOffsetTable[nVideoFrames].tTimeStamp = Parser.pHeader->nTimestamp;
					}
				}
				else
					OutputMsg("%s %d(%s) Frame (%d) overflow.\n", __FILE__, __LINE__, __FUNCTION__, nVideoFrames);
				nVideoFrames++;
			}
			else
			{
				m_nAudioCodec = (IPC_CODEC)Parser.pHeaderEx->nType;
				nAudioFrames++;
			}

			nFrameOffset += Parser.nFrameSize;
		}
		nOffset += nBytesRead;
		// 			if (bFirstBlockIsFilled && m_bThreadSummaryRun)
		// 			{
		// 				SetEvent(m_hCacheFulled);
		// 				m_nSummaryOffset = nFrameOffset;
		// 				CAutoLock lock(&m_csVideoCache);
		// 				m_nHeaderFrameID = m_listVideoCache.front()->FrameHeader()->nFrameID;
		// 				OutputMsg("VideoCache = %d\tAudioCache = %d.\n", m_listVideoCache.size(), m_listAudioCache.size());
		// 				bFirstBlockIsFilled = false;
		// 			}
		// ²ÐÁôÊý¾Ý³¤ÎªnDataLength
		memcpy(pBuffer, pFrameBuffer, nDataLength);
		ZeroMemory(&pBuffer[nDataLength], nBufferSize - nDataLength);
	}
	m_nTotalFrames = nVideoFrames;
	//#ifdef _DEBUG
	OutputMsg("%s TimeSpan = %.3f\tnVideoFrames = %d\tnAudioFrames = %d.\n", __FUNCTION__, TimeSpanEx(dfTimeStart), nVideoFrames, nAudioFrames);
	//#endif		
	m_bSummaryIsReady = true;
	return IPC_Succeed;
}

bool CIPCPlayer::ParserFrame(INOUT byte **ppBuffer,	INOUT DWORD &nDataSize,	FrameParser* pFrameParser)
{
	int nOffset = 0;
	if (nDataSize < sizeof(IPCFrameHeaderEx))
		return false;
	if (Frame(*ppBuffer)->nFrameTag != IPC_TAG &&
		Frame(*ppBuffer)->nFrameTag != GSJ_TAG)
	{
		static char *szKey1 = "MOVD";
		static char *szKey2 = "IMWH";
		nOffset = KMP_StrFind(*ppBuffer, nDataSize, (byte *)szKey1, 4);
		if (nOffset < 0)
			nOffset = KMP_StrFind(*ppBuffer, nDataSize, (byte *)szKey2, 4);
		nOffset -= offsetof(IPCFrameHeader, nFrameTag);
	}

	if (nOffset < 0)
		return false;

	byte *pFrameBuff = *ppBuffer;
	if (m_nSDKVersion < IPC_IPC_SDK_VERSION_2015_12_16 || m_nSDKVersion == IPC_IPC_SDK_GSJ_HEADER)
	{// ¾É°æÎÄ¼þ
		// Ö¡Í·ÐÅÏ¢²»ÍêÕû
		if ((nOffset + sizeof(IPCFrameHeader)) >= nDataSize)
			return false;
		pFrameBuff += nOffset;
		// Ö¡Êý¾Ý²»ÍêÕû
		if (nOffset + FrameSize2(pFrameBuff) >= nDataSize)
			return false;
		if (pFrameParser)
		{
			pFrameParser->pHeader = (IPCFrameHeader *)(pFrameBuff);
			bool bIFrame = false;
			// 				if (IsIPCVideoFrame(pFrameParser->pHeaderEx, bIFrame,m_nSDKVersion))
			// 					OutputMsg("Frame ID:%d\tType = Video:%d.\n", pFrameParser->pHeaderEx->nFrameID, pFrameParser->pHeaderEx->nType);
			// 				else
			// 					OutputMsg("Frame ID:%d\tType = Audio:%d.\n", pFrameParser->pHeaderEx->nFrameID, pFrameParser->pHeaderEx->nType);
			pFrameParser->nFrameSize = FrameSize2(pFrameBuff);
			pFrameParser->pRawFrame = *ppBuffer + sizeof(IPCFrameHeader);
			pFrameParser->nRawFrameSize = Frame2(pFrameBuff)->nLength;
		}
		nDataSize -= (FrameSize2(pFrameBuff) + nOffset);
		pFrameBuff += FrameSize2(pFrameBuff);
	}
	else
	{// ÐÂ°æÎÄ¼þ
		// Ö¡Í·ÐÅÏ¢²»ÍêÕû
		if ((nOffset + sizeof(IPCFrameHeaderEx)) >= nDataSize)
			return false;

		pFrameBuff += nOffset;

		// Ö¡Êý¾Ý²»ÍêÕû
		if (nOffset + FrameSize(pFrameBuff) >= nDataSize)
			return false;

		if (pFrameParser)
		{
			pFrameParser->pHeaderEx = (IPCFrameHeaderEx *)pFrameBuff;
			bool bIFrame = false;
			// 				if (IsIPCVideoFrame(pFrameParser->pHeaderEx, bIFrame,m_nSDKVersion))
			// 					OutputMsg("Frame ID:%d\tType = Video:%d.\n", pFrameParser->pHeaderEx->nFrameID, pFrameParser->pHeaderEx->nType);
			// 				else
			// 					OutputMsg("Frame ID:%d\tType = Audio:%d.\n", pFrameParser->pHeaderEx->nFrameID, pFrameParser->pHeaderEx->nType);
			pFrameParser->nFrameSize = FrameSize(pFrameBuff);
			pFrameParser->pRawFrame = pFrameBuff + sizeof(IPCFrameHeaderEx);
			pFrameParser->nRawFrameSize = Frame(pFrameBuff)->nLength;
		}
		nDataSize -= (FrameSize(pFrameBuff) + nOffset);
		pFrameBuff += FrameSize(pFrameBuff);
	}
	*ppBuffer = pFrameBuff;
	return true;
}

///< @brief ÊÓÆµÎÄ¼þ½âÎöÏß³Ì
UINT __stdcall CIPCPlayer::ThreadFileParser(void *p)
{// ÈôÖ¸¶¨ÁËÓÐÐ§µÄ´°¿Ú¾ä±ú£¬Ôò°Ñ½âÎöºóµÄÎÄ¼þÊý¾Ý·ÅÈë²¥·Å¶ÓÁÐ£¬·ñÔò²»·ÅÈë²¥·Å¶ÓÁÐ
	CIPCPlayer* pThis = (CIPCPlayer *)p;
	LONGLONG nSeekOffset = 0;
	DWORD nBufferSize = pThis->m_nMaxFrameSize * 4;
	DWORD nBytesRead = 0;
	DWORD nDataLength = 0;

	LARGE_INTEGER liFileSize;
	if (!GetFileSizeEx(pThis->m_hVideoFile, &liFileSize))
		return 0;

	if (pThis->GetFileSummary(pThis->m_bThreadParserRun) != IPC_Succeed)
	{
		assert(false);
		return 0;
	}

	byte *pBuffer = _New byte[nBufferSize];
	shared_ptr<byte>BufferPtr(pBuffer);
	FrameParser Parser;
	pThis->m_tLastFrameTime = 0;
	if (SetFilePointer(pThis->m_hVideoFile, (LONG)sizeof(IPC_MEDIAINFO), nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		pThis->OutputMsg("%s SetFilePointer Failed,Error = %d.\n", __FUNCTION__, GetLastError());
		assert(false);
		return 0;
	}

#ifdef _DEBUG
	double dfT1 = GetExactTime();
	bool bOuputTime = false;
#endif
	IPCFrameHeaderEx HeaderEx;
	int nInputResult = 0;
	bool bFileEnd = false;

	while (pThis->m_bThreadParserRun)
	{
		if (pThis->m_bPause)
		{
			Sleep(20);
			continue;
		}
		if (pThis->m_nSummaryOffset)
		{
			CAutoLock lock(&pThis->m_csVideoCache);
			if (pThis->m_listVideoCache.size() < pThis->m_nMaxFrameCache)
			{
				if (SetFilePointer(pThis->m_hVideoFile, (LONG)pThis->m_nSummaryOffset, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
					pThis->OutputMsg("%s SetFilePointer Failed,Error = %d.\n", __FUNCTION__, GetLastError());
				pThis->m_nSummaryOffset = 0;
				lock.Unlock();
				Sleep(20);
			}
			else
			{
				lock.Unlock();
				Sleep(20);
				continue;
			}
		}
		else if (nSeekOffset = pThis->GetSeekOffset())	// ÊÇ·ñÐèÒªÒÆ¶¯ÎÄ¼þÖ¸Õë,ÈônSeekOffset²»Îª0£¬ÔòÐèÒªÒÆ¶¯ÎÄ¼þÖ¸Õë
		{
			pThis->OutputMsg("Detect SeekFrame Operation.\n");

			pThis->m_csVideoCache.Lock();
			pThis->m_listVideoCache.clear();
			pThis->m_csVideoCache.Unlock();

			pThis->m_csAudioCache.Lock();
			pThis->m_listAudioCache.clear();
			pThis->m_csAudioCache.Unlock();

			pThis->SetSeekOffset(0);
			bFileEnd = false;
			pThis->m_bFilePlayFinished = false;
			nDataLength = 0;
#ifdef _DEBUG
			pThis->m_bSeekSetDetected = true;
#endif
			if (SetFilePointer(pThis->m_hVideoFile, (LONG)nSeekOffset, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
				pThis->OutputMsg("%s SetFilePointer Failed,Error = %d.\n", __FUNCTION__, GetLastError());
		}
		if (bFileEnd)
		{// ÎÄ¼þ¶ÁÈ¡½áÊÂ£¬ÇÒ²¥·Å¶ÓÁÐÎª¿Õ£¬ÔòÈÏÎª²¥·Å½áÊø
			pThis->m_csVideoCache.Lock();
			int nVideoCacheSize = pThis->m_listVideoCache.size();
			pThis->m_csVideoCache.Unlock();
			if (nVideoCacheSize == 0)
			{
				pThis->m_bFilePlayFinished = true;
			}
			Sleep(20);
			continue;
		}
		if (!ReadFile(pThis->m_hVideoFile, &pBuffer[nDataLength], (nBufferSize - nDataLength), &nBytesRead, nullptr))
		{
			pThis->OutputMsg("%s ReadFile Failed,Error = %d.\n", __FUNCTION__, GetLastError());
			return 0;
		}

		if (nBytesRead == 0)
		{// µ½´ïÎÄ¼þ½áÎ²
			pThis->OutputMsg("%s Reaching File end nBytesRead = %d.\n", __FUNCTION__, nBytesRead);
			LONGLONG nOffset = 0;
			if (!GetFilePosition(pThis->m_hVideoFile, nOffset))
			{
				pThis->OutputMsg("%s GetFilePosition Failed,Error =%d.\n", __FUNCTION__, GetLastError());
				return 0;
			}
			if (nOffset == liFileSize.QuadPart)
			{
				bFileEnd = true;
				pThis->OutputMsg("%s Reaching File end.\n", __FUNCTION__);
			}
		}
		else
			pThis->m_bFilePlayFinished = false;
		nDataLength += nBytesRead;
		byte *pFrameBuffer = pBuffer;

		bool bIFrame = false;
		bool bFrameInput = true;
		while (pThis->m_bThreadParserRun)
		{
			if (pThis->m_bPause)		// Í¨¹ýpause º¯Êý£¬ÔÝÍ£Êý¾Ý¶ÁÈ¡
			{
				Sleep(20);
				continue;
			}
			if (bFrameInput)
			{
				bFrameInput = false;
				if (!pThis->ParserFrame(&pFrameBuffer, nDataLength, &Parser))
					break;
				nInputResult = pThis->InputStream((byte *)Parser.pHeader, Parser.nFrameSize, 0);
				switch (nInputResult)
				{
				case IPC_Succeed:
				case IPC_Error_InvalidFrameType:
				default:
					bFrameInput = true;
					break;
				case IPC_Error_FrameCacheIsFulled:	// »º³åÇøÒÑÂú
					bFrameInput = false;
					Sleep(10);
					break;
				}
			}
			else
			{
				nInputResult = pThis->InputStream((byte *)Parser.pHeader, Parser.nFrameSize,0);
				switch (nInputResult)
				{
				case IPC_Succeed:
				case IPC_Error_InvalidFrameType:
				default:
					bFrameInput = true;
					break;
				case IPC_Error_FrameCacheIsFulled:	// »º³åÇøÒÑÂú
					bFrameInput = false;
					Sleep(10);
					break;
				}
			}
		}
		// ²ÐÁôÊý¾Ý³¤ÎªnDataLength
		memmove(pBuffer, pFrameBuffer, nDataLength);
#ifdef _DEBUG
		ZeroMemory(&pBuffer[nDataLength], nBufferSize - nDataLength);
#endif
		// ÈôÊÇµ¥´¿½âÎöÊý¾ÝÏß³Ì£¬ÔòÐèÒªÔÝ»º¶ÁÈ¡Êý¾Ý
		// 			if (!pThis->m_hWnd )
		// 			{
		// 				Sleep(10);
		// 			}
	}
	return 0;
}

int CIPCPlayer::EnableStreamParser(IPC_CODEC nCodec)
{
	if (m_pStreamParser || m_hThreadStreamParser)
		return IPC_Error_StreamParserExisted;
	m_pStreamParser = make_shared<CStreamParser>();
	AVCodecID nAvCodec = AV_CODEC_ID_NONE;
	switch (nCodec)
	{
	case CODEC_H264:
		nAvCodec = AV_CODEC_ID_H264;
		break;
	case CODEC_H265:
		nAvCodec = AV_CODEC_ID_H265;
		break;
	default:
		return IPC_Error_UnsupportedCodec;
	}
	int nError = m_pStreamParser->InitStreamParser(nAvCodec);
	if (nError != IPC_Succeed)
		return nError;

	return IPC_Succeed;
}

///< @brief ÊÓÆµÁ÷½âÎöÏß³Ì
UINT __stdcall CIPCPlayer::ThreadStreamParser(void *p)
{
	CIPCPlayer* pThis = (CIPCPlayer *)p;
	return 0;
// 	int nBufferLength = 0;
// 	bool bGetFrame = false;
// 	bool bSleeped = false;
// 	list<AVPacketPtr> listFrame;
// 	while (pThis->m_bStreamParserRun)
// 	{
// 		bSleeped = false;
// 		if (pThis->m_pStreamParser->ParserFrame(listFrame) > 0)
// 		{
// 			for (auto it = listFrame.begin(); pThis->m_bStreamParserRun && it != listFrame.end();)
// 			{
// 				bSleeped = false;
// 				
// 				if (pThis->InputStream((*it)->data, IPC_I_FRAME, (*it)->size, 0, 0) == IPC_Succeed)
// 				{
// 					it = listFrame.erase(it);
// 					pThis->m_pStreamParser->SetFrameCahceFulled(false);
// 				}
// 				else
// 				{
// 					pThis->m_pStreamParser->SetFrameCahceFulled(true);
// 					bSleeped = true;
// 					Sleep(10);
// 					continue;
// 				}
// 			}
// 		}
// 		if (!bSleeped)
// 			Sleep(10);
// 	}
// 	
// 	return 0;
}

// Ì½²âÊÓÆµÂëÁ÷ÀàÐÍ,ÒªÇó±ØÐëÊäÈëIÖ¡
bool CIPCPlayer::ProbeStream(byte *szFrameBuffer, int nBufferLength)
{
	shared_ptr<CVideoDecoder>pDecodec = make_shared<CVideoDecoder>();
	if (!m_pStreamProbe)
		m_pStreamProbe = make_shared<StreamProbe>();

	if (!m_pStreamProbe)
		return false;
	m_pStreamProbe->nProbeCount++;
	m_pStreamProbe->GetProbeStream(szFrameBuffer, nBufferLength);
	if (m_pStreamProbe->nProbeDataLength <= 64)
		return false;
	if (pDecodec->ProbeStream(this, ReadAvData, m_nMaxFrameSize) != 0)
	{
		OutputMsg("%s Failed in ProbeStream,you may need to input more stream.\n", __FUNCTION__);
		pDecodec->CancelProbe();
		//assert(false);
		return false;
	}
	pDecodec->CancelProbe();
	if (pDecodec->m_nCodecId == AV_CODEC_ID_NONE)
	{
		OutputMsg("%s Unknown Video Codec or not found any codec in the stream.\n", __FUNCTION__);
		assert(false);
		return false;
	}

	if (!pDecodec->m_pAVCtx->width || !pDecodec->m_pAVCtx->height)
	{
		assert(false);
		return false;
	}
	if (pDecodec->m_nCodecId == AV_CODEC_ID_H264)
	{
		m_nVideoCodec = CODEC_H264;
		OutputMsg("%s Video Codec:%H.264 Width = %d\tHeight = %d.\n", __FUNCTION__, pDecodec->m_pAVCtx->width, pDecodec->m_pAVCtx->height);
	}
	else if (pDecodec->m_nCodecId == AV_CODEC_ID_HEVC)
	{
		m_nVideoCodec = CODEC_H265;
		OutputMsg("%s Video Codec:%H.265 Width = %d\tHeight = %d.\n", __FUNCTION__, pDecodec->m_pAVCtx->width, pDecodec->m_pAVCtx->height);
	}
	else
	{
		m_nVideoCodec = CODEC_UNKNOWN;
		OutputMsg("%s Unsupported Video Codec.\n", __FUNCTION__);
		assert(false);
		return false;
	}
	m_pStreamProbe->nProbeAvCodecID = pDecodec->m_nCodecId;
	m_pStreamProbe->nProbeWidth = pDecodec->m_pAVCtx->width;
	m_pStreamProbe->nProbeHeight = pDecodec->m_pAVCtx->height;
	m_nVideoHeight = pDecodec->m_pAVCtx->height;
	m_nVideoWidth = pDecodec->m_pAVCtx->width;
	return true;
}

/// @brief °ÑNV12Í¼Ïñ×ª»»ÎªYUV420PÍ¼Ïñ
void CIPCPlayer::CopyNV12ToYUV420P(byte *pYV12, byte *pNV12[2], int src_pitch[2], unsigned width, unsigned height)
{
	byte* dstV = pYV12 + width*height;
	byte* dstU = pYV12 + width*height / 4;
	UINT heithtUV = height / 2;
	UINT widthUV = width / 2;
	byte *pSrcUV = pNV12[1];
	byte *pSrcY = pNV12[0];
	int &nYpitch = src_pitch[0];
	int &nUVpitch = src_pitch[1];

	// ¸´ÖÆY·ÖÁ¿
	for (int i = 0; i < height; i++)
		memcpy(pYV12 + i*width, pSrcY + i*nYpitch, width);

	// ¸´ÖÆVU·ÖÁ¿
	for (int i = 0; i < heithtUV; i++)
	{
		for (int j = 0; j < width; j++)
		{
			dstU[i*widthUV + j] = pSrcUV[i*nUVpitch + 2 * j];
			dstV[i*widthUV + j] = pSrcUV[i*nUVpitch + 2 * j + 1];
		}
	}
}

/// @brief °ÑDxvaÓ²½âÂëNV12Ö¡×ª»»³ÉYV12Í¼Ïñ
void CIPCPlayer::CopyDxvaFrame(byte *pYuv420, AVFrame *pAvFrameDXVA)
{
	if (pAvFrameDXVA->format != AV_PIX_FMT_DXVA2_VLD)
		return;

	IDirect3DSurface9* pSurface = (IDirect3DSurface9 *)pAvFrameDXVA->data[3];
	D3DLOCKED_RECT lRect;
	D3DSURFACE_DESC SurfaceDesc;
	pSurface->GetDesc(&SurfaceDesc);
	HRESULT hr = pSurface->LockRect(&lRect, nullptr, D3DLOCK_READONLY);
	if (FAILED(hr))
	{
		OutputMsg("%s IDirect3DSurface9::LockRect failed:hr = %08.\n", __FUNCTION__, hr);
		return;
	}

	// Y·ÖÁ¿Í¼Ïñ
	byte *pSrcY = (byte *)lRect.pBits;

	// UV·ÖÁ¿Í¼Ïñ
	//byte *pSrcUV = (byte *)lRect.pBits + lRect.Pitch * SurfaceDesc.Height;
	byte *pSrcUV = (byte *)lRect.pBits + lRect.Pitch * pAvFrameDXVA->height;

	byte* dstY = pYuv420;
	byte* dstV = pYuv420 + pAvFrameDXVA->width*pAvFrameDXVA->height;
	byte* dstU = pYuv420 + pAvFrameDXVA->width*pAvFrameDXVA->height / 4;

	UINT heithtUV = pAvFrameDXVA->height / 2;
	UINT widthUV = pAvFrameDXVA->width / 2;

	// ¸´ÖÆY·ÖÁ¿
	for (int i = 0; i < pAvFrameDXVA->height; i++)
		memcpy(&dstY[i*pAvFrameDXVA->width], &pSrcY[i*lRect.Pitch], pAvFrameDXVA->width);

	// ¸´ÖÆVU·ÖÁ¿
	for (int i = 0; i < heithtUV; i++)
	{
		for (int j = 0; j < widthUV; j++)
		{
			dstU[i*widthUV + j] = pSrcUV[i*lRect.Pitch + 2 * j];
			dstV[i*widthUV + j] = pSrcUV[i*lRect.Pitch + 2 * j + 1];
		}
	}

	pSurface->UnlockRect();
}

void CIPCPlayer::CopyDxvaFrameYV12(byte **ppYV12, int &nStrideY, int &nWidth, int &nHeight, AVFrame *pAvFrameDXVA)
{
	if (pAvFrameDXVA->format != AV_PIX_FMT_DXVA2_VLD)
		return;

	IDirect3DSurface9* pSurface = (IDirect3DSurface9 *)pAvFrameDXVA->data[3];
	D3DLOCKED_RECT lRect;
	D3DSURFACE_DESC SurfaceDesc;
	pSurface->GetDesc(&SurfaceDesc);
	HRESULT hr = pSurface->LockRect(&lRect, nullptr, D3DLOCK_READONLY);
	if (FAILED(hr))
	{
		OutputMsg("%s IDirect3DSurface9::LockRect failed:hr = %08.\n", __FUNCTION__, hr);
		return;
	}

	// Y·ÖÁ¿Í¼Ïñ
	byte *pSrcY = (byte *)lRect.pBits;
	nStrideY = lRect.Pitch;
	nWidth = SurfaceDesc.Width;
	nHeight = SurfaceDesc.Height;

	int nPictureSize = lRect.Pitch*SurfaceDesc.Height;
	int nYUVSize = nPictureSize * 3 / 2;
	*ppYV12 = (byte *)_aligned_malloc(nYUVSize, 32);
#ifdef _DEBUG
	ZeroMemory(*ppYV12, nYUVSize);
#endif
	gpu_memcpy(*ppYV12, lRect.pBits, nPictureSize);

	UINT heithtUV = SurfaceDesc.Height >> 1;
	UINT widthUV = lRect.Pitch >> 1;
	byte *pSrcUV = (byte *)lRect.pBits + nPictureSize;
	byte* dstV = *ppYV12 + nPictureSize;
	byte* dstU = *ppYV12 + nPictureSize + nPictureSize / 4;
	// ¸´ÖÆVU·ÖÁ¿
	int nOffset = 0;
	for (int i = 0; i < heithtUV; i++)
	{
		for (int j = 0; j < widthUV; j++)
		{
			dstV[nOffset / 2 + j] = pSrcUV[nOffset + 2 * j];
			dstU[nOffset / 2 + j] = pSrcUV[nOffset + 2 * j + 1];
		}
		nOffset += lRect.Pitch;
	}
	pSurface->UnlockRect();
}

void CIPCPlayer::CopyDxvaFrameNV12(byte **ppNV12, int &nStrideY, int &nWidth, int &nHeight, AVFrame *pAvFrameDXVA)
{
	if (pAvFrameDXVA->format != AV_PIX_FMT_DXVA2_VLD)
		return;

	IDirect3DSurface9* pSurface = (IDirect3DSurface9 *)pAvFrameDXVA->data[3];
	D3DLOCKED_RECT lRect;
	D3DSURFACE_DESC SurfaceDesc;
	pSurface->GetDesc(&SurfaceDesc);
	HRESULT hr = pSurface->LockRect(&lRect, nullptr, D3DLOCK_READONLY);
	if (FAILED(hr))
	{
		OutputMsg("%s IDirect3DSurface9::LockRect failed:hr = %08.\n", __FUNCTION__, hr);
		return;
	}
	// Y·ÖÁ¿Í¼Ïñ
	byte *pSrcY = (byte *)lRect.pBits;
	nStrideY = lRect.Pitch;
	nWidth = SurfaceDesc.Width;
	nHeight = SurfaceDesc.Height;

	int nPictureSize = lRect.Pitch*SurfaceDesc.Height;
	int nYUVSize = nPictureSize * 3 / 2;
	*ppNV12 = (byte *)_aligned_malloc(nYUVSize, 32);
#ifdef _DEBUG
	ZeroMemory(*ppNV12, nYUVSize);
#endif
	gpu_memcpy(*ppNV12, lRect.pBits, nYUVSize);
	pSurface->UnlockRect();
}

bool CIPCPlayer::LockDxvaFrame(AVFrame *pAvFrameDXVA, byte **ppSrcY, byte **ppSrcUV, int &nPitch)
{
	if (pAvFrameDXVA->format != AV_PIX_FMT_DXVA2_VLD)
		return false;

	IDirect3DSurface9* pSurface = (IDirect3DSurface9 *)pAvFrameDXVA->data[3];
	D3DLOCKED_RECT lRect;
	D3DSURFACE_DESC SurfaceDesc;
	pSurface->GetDesc(&SurfaceDesc);
	HRESULT hr = pSurface->LockRect(&lRect, nullptr, D3DLOCK_READONLY);
	if (FAILED(hr))
	{
		OutputMsg("%s IDirect3DSurface9::LockRect failed:hr = %08.\n", __FUNCTION__, hr);
		return false;
	}
	// Y·ÖÁ¿Í¼Ïñ
	*ppSrcY = (byte *)lRect.pBits;
	// UV·ÖÁ¿Í¼Ïñ
	//(PBYTE)SrcRect.pBits + SrcRect.Pitch * m_pDDraw->m_dwHeight;
	*ppSrcUV = (byte *)lRect.pBits + lRect.Pitch * pAvFrameDXVA->height;
	nPitch = lRect.Pitch;
	return true;
}

void CIPCPlayer::UnlockDxvaFrame(AVFrame *pAvFrameDXVA)
{
	if (pAvFrameDXVA->format != AV_PIX_FMT_DXVA2_VLD)
		return;

	IDirect3DSurface9* pSurface = (IDirect3DSurface9 *)pAvFrameDXVA->data[3];
	pSurface->UnlockRect();
}
// °ÑYUVC420PÖ¡¸´ÖÆµ½YV12»º´æÖÐ
void CIPCPlayer::CopyFrameYUV420(byte *pYUV420, int nYUV420Size, AVFrame *pFrame420P)
{
	byte *pDest = pYUV420;
	int nStride = pFrame420P->width;
	int nSize = nStride * nStride;
	int nHalfSize = (nSize) >> 1;
	byte *pDestY = pDest;										// Y·ÖÁ¿ÆðÊ¼µØÖ·

	byte *pDestU = pDest + nSize;								// U·ÖÁ¿ÆðÊ¼µØÖ·
	int nSizeofU = nHalfSize >> 1;

	byte *pDestV = pDestU + (size_t)(nHalfSize >> 1);			// V·ÖÁ¿ÆðÊ¼µØÖ·
	int nSizeofV = nHalfSize >> 1;

	// YUV420PµÄUºÍV·ÖÁ¿¶Ôµ÷£¬±ã³ÉÎªYV12¸ñÊ½
	// ¸´ÖÆY·ÖÁ¿
	for (int i = 0; i < pFrame420P->height; i++)
		memcpy_s(pDestY + i * nStride, nSize * 3 / 2 - i*nStride, pFrame420P->data[0] + i * pFrame420P->linesize[0], pFrame420P->width);

	// ¸´ÖÆYUV420PµÄU·ÖÁ¿µ½Ä¿´åµÄYV12µÄU·ÖÁ¿
	for (int i = 0; i < pFrame420P->height / 2; i++)
		memcpy_s(pDestU + i * nStride / 2, nSizeofU - i*nStride / 2, pFrame420P->data[1] + i * pFrame420P->linesize[1], pFrame420P->width / 2);

	// ¸´ÖÆYUV420PµÄV·ÖÁ¿µ½Ä¿´åµÄYV12µÄV·ÖÁ¿
	for (int i = 0; i < pFrame420P->height / 2; i++)
		memcpy_s(pDestV + i * nStride / 2, nSizeofV - i*nStride / 2, pFrame420P->data[2] + i * pFrame420P->linesize[2], pFrame420P->width / 2);
}

void CIPCPlayer::ProcessYUVFilter(AVFrame *pAvFrame, LONGLONG nTimestamp)
{
	if (m_csYUVFilter.TryLock())
	{// ÔÚm_pfnYUVFileterÖÐ£¬ÓÃ»§ÐèÒª°ÑYUVÊý¾Ý´¦Àí·Ö£¬ÔÙ·Ö³ÉYUVÊý¾Ý
		if (m_pfnYUVFilter)
		{
			if (pAvFrame->format == AV_PIX_FMT_DXVA2_VLD)
			{// dxva Ó²½âÂëÖ¡
				CopyDxvaFrame(m_pYUV, pAvFrame);
				byte* pU = m_pYUV + pAvFrame->width*pAvFrame->height;
				byte* pV = m_pYUV + pAvFrame->width*pAvFrame->height / 4;
				m_pfnYUVFilter(this,
					m_pYUV,
					pU,
					pV,
					pAvFrame->width,
					pAvFrame->width / 2,
					pAvFrame->width,
					pAvFrame->height,
					nTimestamp,
					m_pUserYUVFilter);
			}
			else
				m_pfnYUVFilter(this,
				pAvFrame->data[0],
				pAvFrame->data[1],
				pAvFrame->data[2],
				pAvFrame->linesize[0],
				pAvFrame->linesize[1],
				pAvFrame->width,
				pAvFrame->height,
				nTimestamp,
				m_pUserYUVFilter);
		}
		m_csYUVFilter.Unlock();
	}
}

void CIPCPlayer::ProcessYUVCapture(AVFrame *pAvFrame, LONGLONG nTimestamp)
{
	if (m_csCaptureYUV.TryLock())
	{
		if (m_pfnCaptureYUV)
		{
			int nPictureSize = 0;
			if (pAvFrame->format == AV_PIX_FMT_DXVA2_VLD)
			{// Ó²½âÂë»·¾³ÏÂ,m_pYUVÄÚ´æÐèÒª¶ÀÁ¢ÉêÇë£¬¼ÆËã³ß´ç
				int nStrideY = 0;
				int nWidth = 0, nHeight = 0;
				CopyDxvaFrameNV12(&m_pYUV, nStrideY, nWidth, nHeight, pAvFrame);
				m_nYUVSize = nStrideY*nHeight * 3 / 2;
				m_pYUVPtr = shared_ptr<byte>(m_pYUV, _aligned_free);
				m_pfnCaptureYUV(this, m_pYUV, m_nYUVSize, nStrideY, nStrideY >> 1, nWidth, nHeight, nTimestamp, m_pUserCaptureYUV);
			}
			else
			{
				nPictureSize = pAvFrame->linesize[0] * pAvFrame->height;
				int nUVSize = nPictureSize / 2;
				if (!m_pYUV)
				{
					m_nYUVSize = nPictureSize * 3 / 2;
					m_pYUV = (byte *)_aligned_malloc(m_nYUVSize, 32);
					m_pYUVPtr = shared_ptr<byte>(m_pYUV, _aligned_free);
				}
				memcpy(m_pYUV, pAvFrame->data[0], nPictureSize);
				memcpy(&m_pYUV[nPictureSize], pAvFrame->data[1], nUVSize / 2);
				memcpy(&m_pYUV[nPictureSize + nUVSize / 2], pAvFrame->data[2], nUVSize / 2);
				m_pfnCaptureYUV(this, m_pYUV, m_nYUVSize, pAvFrame->linesize[0], pAvFrame->linesize[1], pAvFrame->width, pAvFrame->height, nTimestamp, m_pUserCaptureYUV);
			}
			//OutputMsg("%s m_pfnCaptureYUV = %p", __FUNCTION__, m_pfnCaptureYUV);
		}
		m_csCaptureYUV.Unlock();
	}
	if (m_csCaptureYUVEx.TryLock())
	{
		if (m_pfnCaptureYUVEx)
		{
			if (!m_pYUV)
			{
				m_nYUVSize = pAvFrame->width * pAvFrame->height * 3 / 2;
				m_pYUV = (byte *)av_malloc(m_nYUVSize);
				m_pYUVPtr = shared_ptr<byte>(m_pYUV, av_free);
			}
			if (pAvFrame->format == AV_PIX_FMT_DXVA2_VLD)
			{// dxva Ó²½âÂëÖ¡
				//CopyDxvaFrameNV12(m_pYUV, pAvFrame);
				byte *pY = NULL;
				byte *pUV = NULL;
				int nPitch = 0;
				LockDxvaFrame(pAvFrame, &pY, &pUV, nPitch);
				byte* pU = m_pYUV + pAvFrame->width*pAvFrame->height;
				byte* pV = m_pYUV + pAvFrame->width*pAvFrame->height / 4;

				m_pfnCaptureYUVEx(this,
					pY,
					pUV,
					NULL,
					nPitch,
					nPitch / 2,
					pAvFrame->width,
					pAvFrame->height,
					nTimestamp,
					m_pUserCaptureYUVEx);
				UnlockDxvaFrame(pAvFrame);
			}
			else
			{
				m_pfnCaptureYUVEx(this,
					pAvFrame->data[0],
					pAvFrame->data[1],
					pAvFrame->data[2],
					pAvFrame->linesize[0],
					pAvFrame->linesize[1],
					pAvFrame->width,
					pAvFrame->height,
					nTimestamp,
					m_pUserCaptureYUVEx);
			}
		}
		m_csCaptureYUVEx.Unlock();
	}
	if (m_csCaptureRGB.TryLock())
	{
		if (m_pCaptureRGB)
		{
			// 方案 D 可直接取得RGB数据，耗时4-5ms,CPU占用6-8%，只能纯软解，尚有优化空间
			// 其它方案详见 CDxSurface::GetRGBBuffer函数
			/*int nConverted = 0;
			if (m_pPixelConvert)
				nConverted = m_pPixelConvert->ConvertPixel(pAvFrame);
			else
			{
				m_pPixelConvert = make_shared<PixelConvert>(pAvFrame, D3DFMT_R8G8B8, GQ_FAST_BILINEAR);
				nConverted = m_pPixelConvert->ConvertPixel();
			}
			if (nConverted > 0)
				m_pCaptureRGB(this, m_pPixelConvert->pImage, pAvFrame->width, pAvFrame->height, nTimestamp, m_pUserCaptureRGB);*/
			byte *pRGBBuffer = nullptr;
			int  nRGBBufferSize = 0;
			if (m_pDxSurface && m_pDxSurface->GetRGBBuffer(&pRGBBuffer,nRGBBufferSize))
				m_pCaptureRGB(this, pRGBBuffer, pAvFrame->width, pAvFrame->height, nTimestamp, m_pUserCaptureRGB);
		}
			
		m_csCaptureRGB.Unlock();
	}
}

/// @brief			ÂëÁ÷Ì½²â¶ÁÈ¡Êý¾Ý°ü»Øµ÷º¯Êý
/// @param [in]		opaque		ÓÃ»§ÊäÈëµÄ»Øµ÷º¯Êý²ÎÊýÖ¸Õë
/// @param [in]		buf			¶ÁÈ¡Êý¾ÝµÄ»º´æ
/// @param [in]		buf_size	»º´æµÄ³¤¶È
/// @return			Êµ¼Ê¶ÁÈ¡Êý¾ÝµÄ³¤¶È
int CIPCPlayer::ReadAvData(void *opaque, uint8_t *buf, int buf_size)
{
	AvQueue *pAvQueue = (AvQueue *)opaque;
	CIPCPlayer *pThis = (CIPCPlayer *)pAvQueue->pUserData;

	int nReturnVal = buf_size;
	int nRemainedLength = 0;
	pAvQueue->pAvBuffer = buf;
	if (!pThis->m_pStreamProbe)
		return 0;
	int &nDataLength = pThis->m_pStreamProbe->nProbeDataRemained;
	byte *pProbeBuff = pThis->m_pStreamProbe->pProbeBuff;
	int &nProbeOffset = pThis->m_pStreamProbe->nProbeOffset;
	if (nDataLength > 0)
	{
		nRemainedLength = nDataLength - nProbeOffset;
		if (nRemainedLength > buf_size)
		{
			memcpy_s(buf, buf_size, &pProbeBuff[nProbeOffset], buf_size);
			nProbeOffset += buf_size;
			nDataLength -= buf_size;
		}
		else
		{
			memcpy_s(buf, buf_size, &pProbeBuff[nProbeOffset], nRemainedLength);
			nDataLength -= nRemainedLength;
			nProbeOffset = 0;
			nReturnVal = nRemainedLength;
		}
		return nReturnVal;
	}
	else
		return 0;
}

#ifndef _WIN64
UINT __stdcall CIPCPlayer::ThreadPlayAudioGSJ(void *p)
{
	CIPCPlayer *pThis = (CIPCPlayer *)p;
	int nAudioFrameInterval = pThis->m_fPlayInterval / 2;

	DWORD nResult = 0;
	int nTimeSpan = 0;
	StreamFramePtr FramePtr;
	int nAudioError = 0;
	byte *pPCM = nullptr;
	shared_ptr<CAudioDecoder> pAudioDecoder = make_shared<CAudioDecoder>();
	int nPCMSize = 0;
	int nDecodeSize = 0;
	__int64 nFrameEvent = 0;
	if (pThis->m_nAudioPlayFPS == 8)
		Sleep(250);
	// Ô¤¶ÁµÚÒ»Ö¡£¬ÒÔ³õÊ¼»¯ÒôÆµ½âÂëÆ÷
	while (pThis->m_bThreadPlayAudioRun)
	{
		if (!FramePtr)
		{
			CAutoLock lock(&pThis->m_csAudioCache, false, __FILE__, __FUNCTION__, __LINE__);
			if (pThis->m_listAudioCache.size() > 0)
			{
				FramePtr = pThis->m_listAudioCache.front();
				break;
			}
		}
		Sleep(10);
	}
	if (!FramePtr)
		return 0;
	if (pAudioDecoder->GetCodecType() == CODEC_UNKNOWN)
	{
		const IPCFrameHeaderEx *pHeader = FramePtr->FrameHeader();
		nDecodeSize = pHeader->nLength * 2;		//G711 Ñ¹ËõÂÊÎª2±¶
		switch (pHeader->nType)
		{
		case FRAME_G711A:			//711 AÂÉ±àÂëÖ¡
		{
			pAudioDecoder->SetACodecType(CODEC_G711A, SampleBit16);
			pThis->m_nAudioCodec = CODEC_G711A;
			pThis->OutputMsg("%s Audio Codec:G711A.\n", __FUNCTION__);
			break;
		}
		case FRAME_G711U:			//711 UÂÉ±àÂëÖ¡
		{
			pAudioDecoder->SetACodecType(CODEC_G711U, SampleBit16);
			pThis->m_nAudioCodec = CODEC_G711U;
			pThis->OutputMsg("%s Audio Codec:G711U.\n", __FUNCTION__);
			break;
		}

		case FRAME_G726:			//726±àÂëÖ¡
		{
			// ÒòÎªÄ¿Ç°IPCÏà»úµÄG726±àÂë,ËäÈ»²ÉÓÃµÄÊÇ16Î»²ÉÑù£¬µ«Ê¹ÓÃ32Î»Ñ¹Ëõ±àÂë£¬Òò´Ë½âÑ¹µÃÊ¹ÓÃSampleBit32
			pAudioDecoder->SetACodecType(CODEC_G726, SampleBit32);
			pThis->m_nAudioCodec = CODEC_G726;
			nDecodeSize = FramePtr->FrameHeader()->nLength * 8;		//G726×î´óÑ¹ËõÂÊ¿É´ï8±¶
			pThis->OutputMsg("%s Audio Codec:G726.\n", __FUNCTION__);
			break;
		}
		case FRAME_AAC:				//AAC±àÂëÖ¡¡£
		{
			pAudioDecoder->SetACodecType(CODEC_AAC, SampleBit16);
			pThis->m_nAudioCodec = CODEC_AAC;
			nDecodeSize = FramePtr->FrameHeader()->nLength * 24;
			pThis->OutputMsg("%s Audio Codec:AAC.\n", __FUNCTION__);
			break;
		}
		default:
		{
			assert(false);
			pThis->OutputMsg("%s Unspported audio codec.\n", __FUNCTION__);
			return 0;
			break;
		}
		}
	}
	if (nPCMSize < nDecodeSize)
	{
		pPCM = new byte[nDecodeSize];
		nPCMSize = nDecodeSize;
	}
#ifdef _DEBUG
	TimeTrace TimeAudio("AudioTime", __FUNCTION__);
#endif
	double dfLastPlayTime = GetExactTime();
	double dfPlayTimeSpan = 0.0f;
	UINT nFramesPlayed = 0;
	WaitForSingleObject(pThis->m_hEventDecodeStart, 1000);

	pThis->m_csAudioCache.Lock();
	pThis->m_nAudioCache = pThis->m_listAudioCache.size();
	pThis->m_csAudioCache.Unlock();
	pThis->OutputMsg("%s Audio Cache Size = %d.\n", __FUNCTION__, pThis->m_nAudioCache);
	time_t tLastFrameTime = 0;
	double dfDecodeStart = GetExactTime();
	DWORD dwOsMajorVersion = GetOsMajorVersion();
#ifdef _DEBUG
	int nSleepCount = 0;
	TimeTrace TraceSleepCount("SleepCount", __FUNCTION__, 25);
#endif
	while (pThis->m_bThreadPlayAudioRun)
	{
		if (pThis->m_bPause)
		{
			if (pThis->m_pDsBuffer->IsPlaying())
				pThis->m_pDsBuffer->StopPlay();
			Sleep(100);
			continue;
		}

		nTimeSpan = (int)((GetExactTime() - dfLastPlayTime) * 1000);
		if (pThis->m_fPlayRate != 1.0f)
		{// Ö»ÓÐÕý³£±¶ÂÊ²Å²¥·ÅÉùÒô
			if (pThis->m_pDsBuffer->IsPlaying())
				pThis->m_pDsBuffer->StopPlay();
			pThis->m_csAudioCache.Lock();
			if (pThis->m_listAudioCache.size() > 0)
				pThis->m_listAudioCache.pop_front();
			pThis->m_csAudioCache.Unlock();
			Sleep(5);
			continue;
		}

		if (nTimeSpan > 1000 * 3 / pThis->m_nAudioPlayFPS)			// Á¬Ðø3*ÒôÆµÂëÁ÷¼ä¸ôÃ»ÓÐÒôÆµÊý¾Ý£¬ÔòÊÓÎªÒôÆµÔÝÍ£
			pThis->m_pDsBuffer->StopPlay();
		else if (!pThis->m_pDsBuffer->IsPlaying())
			pThis->m_pDsBuffer->StartPlay();
		bool bPopFrame = false;
		if (pThis->m_bIpcStream && pThis->m_nAudioPlayFPS == 8)
		{
			if (pThis->m_pDsBuffer->IsPlaying())
				pThis->m_pDsBuffer->WaitForPosNotify();
		}

		pThis->m_csAudioCache.Lock();
		if (pThis->m_listAudioCache.size() > 0)
		{
			FramePtr = pThis->m_listAudioCache.front();
			pThis->m_listAudioCache.pop_front();
			bPopFrame = true;
		}
		pThis->m_nAudioCache = pThis->m_listAudioCache.size();
		pThis->m_csAudioCache.Unlock();

		if (!bPopFrame)
		{
			if (!pThis->m_bIpcStream)
				Sleep(10);
			continue;
		}

		if (nFramesPlayed < 50 && dwOsMajorVersion < 6)
		{// ÐÞÕýÔÚXPÏµÍ³ÖÐ£¬Ç°50Ö¡»á±»Ë²¼ä¶ªµôµÄÎÊÌâ
			if (((TimeSpanEx(dfLastPlayTime) + dfPlayTimeSpan) * 1000) < nAudioFrameInterval)
				Sleep(nAudioFrameInterval - (TimeSpanEx(dfLastPlayTime) * 1000));
		}

		if (pThis->m_pDsBuffer->IsPlaying() //||
			/*pThis->m_pDsBuffer->WaitForPosNotify()*/)
		{
			if (pAudioDecoder->Decode(pPCM, nPCMSize, (byte *)FramePtr->Framedata(pThis->m_nSDKVersion), FramePtr->FrameHeader()->nLength) != 0)
			{
				if (!pThis->m_pDsBuffer->WritePCM(pPCM, nPCMSize, !pThis->m_bIpcStream))
					pThis->OutputMsg("%s Write PCM Failed.\n", __FUNCTION__);
				//SetEvent(pThis->m_hAudioFrameEvent[nFrameEvent++ % 2]);
			}
			else
				pThis->OutputMsg("%s Audio Decode Failed Is.\n", __FUNCTION__);
		}
		nFramesPlayed++;
		if (pThis->m_nAudioPlayFPS == 8 && nFramesPlayed <= 8)
			Sleep(120);
		dfPlayTimeSpan = TimeSpanEx(dfLastPlayTime);
		dfLastPlayTime = GetExactTime();
		tLastFrameTime = FramePtr->FrameHeader()->nTimestamp;
	}
	if (pPCM)
		delete[]pPCM;
	return 0;
}

UINT __stdcall CIPCPlayer::ThreadPlayAudioIPC(void *p)
{
	CIPCPlayer *pThis = (CIPCPlayer *)p;
	int nAudioFrameInterval = pThis->m_fPlayInterval / 2;

	DWORD nResult = 0;
	int nTimeSpan = 0;
	StreamFramePtr FramePtr;
	int nAudioError = 0;
	byte *pPCM = nullptr;
	shared_ptr<CAudioDecoder> pAudioDecoder = make_shared<CAudioDecoder>();
	int nPCMSize = 0;
	int nDecodeSize = 0;
	__int64 nFrameEvent = 0;

	// Ô¤¶ÁµÚÒ»Ö¡£¬ÒÔ³õÊ¼»¯ÒôÆµ½âÂëÆ÷
	while (pThis->m_bThreadPlayAudioRun)
	{
		if (!FramePtr)
		{
			CAutoLock lock(&pThis->m_csAudioCache, false, __FILE__, __FUNCTION__, __LINE__);
			if (pThis->m_listAudioCache.size() > 0)
			{
				FramePtr = pThis->m_listAudioCache.front();
				break;
			}
		}
		Sleep(10);
	}
	if (!FramePtr)
		return 0;
	if (pAudioDecoder->GetCodecType() == CODEC_UNKNOWN)
	{
		const IPCFrameHeaderEx *pHeader = FramePtr->FrameHeader();
		nDecodeSize = pHeader->nLength * 2;		//G711 Ñ¹ËõÂÊÎª2±¶
		switch (pHeader->nType)
		{
		case FRAME_G711A:			//711 AÂÉ±àÂëÖ¡
		{
			pAudioDecoder->SetACodecType(CODEC_G711A, SampleBit16);
			pThis->m_nAudioCodec = CODEC_G711A;
			pThis->OutputMsg("%s Audio Codec:G711A.\n", __FUNCTION__);
			break;
		}
		case FRAME_G711U:			//711 UÂÉ±àÂëÖ¡
		{
			pAudioDecoder->SetACodecType(CODEC_G711U, SampleBit16);
			pThis->m_nAudioCodec = CODEC_G711U;
			pThis->OutputMsg("%s Audio Codec:G711U.\n", __FUNCTION__);
			break;
		}

		case FRAME_G726:			//726±àÂëÖ¡
		{
			// ÒòÎªÄ¿Ç°IPCÏà»úµÄG726±àÂë,ËäÈ»²ÉÓÃµÄÊÇ16Î»²ÉÑù£¬µ«Ê¹ÓÃ32Î»Ñ¹Ëõ±àÂë£¬Òò´Ë½âÑ¹µÃÊ¹ÓÃSampleBit32
			pAudioDecoder->SetACodecType(CODEC_G726, SampleBit32);
			pThis->m_nAudioCodec = CODEC_G726;
			nDecodeSize = FramePtr->FrameHeader()->nLength * 8;		//G726×î´óÑ¹ËõÂÊ¿É´ï8±¶
			pThis->OutputMsg("%s Audio Codec:G726.\n", __FUNCTION__);
			break;
		}
		case FRAME_AAC:				//AAC±àÂëÖ¡¡£
		{
			pAudioDecoder->SetACodecType(CODEC_AAC, SampleBit16);
			pThis->m_nAudioCodec = CODEC_AAC;
			nDecodeSize = FramePtr->FrameHeader()->nLength * 24;
			pThis->OutputMsg("%s Audio Codec:AAC.\n", __FUNCTION__);
			break;
		}
		default:
		{
			assert(false);
			pThis->OutputMsg("%s Unspported audio codec.\n", __FUNCTION__);
			return 0;
			break;
		}
		}
	}
	if (nPCMSize < nDecodeSize)
	{
		pPCM = new byte[nDecodeSize];
		nPCMSize = nDecodeSize;
	}
	double dfLastPlayTime = 0.0f;
	double dfPlayTimeSpan = 0.0f;
	UINT nFramesPlayed = 0;
	WaitForSingleObject(pThis->m_hEventDecodeStart, 1000);

	pThis->m_csAudioCache.Lock();
	pThis->m_nAudioCache = pThis->m_listAudioCache.size();
	pThis->m_csAudioCache.Unlock();

	pThis->OutputMsg("%s Audio Cache Size = %d.\n", __FUNCTION__, pThis->m_nAudioCache);
	time_t tLastFrameTime = 0;
	double dfDecodeStart = GetExactTime();
	DWORD dwOsMajorVersion = GetOsMajorVersion();
	while (pThis->m_bThreadPlayAudioRun)
	{
		if (pThis->m_bPause)
		{
			if (pThis->m_pDsBuffer->IsPlaying())
				pThis->m_pDsBuffer->StopPlay();
			Sleep(20);
			continue;
		}

		nTimeSpan = (int)((GetExactTime() - dfLastPlayTime) * 1000);
		if (pThis->m_fPlayRate != 1.0f)
		{// Ö»ÓÐÕý³£±¶ÂÊ²Å²¥·ÅÉùÒô
			if (pThis->m_pDsBuffer->IsPlaying())
				pThis->m_pDsBuffer->StopPlay();
			pThis->m_csAudioCache.Lock();
			if (pThis->m_listAudioCache.size() > 0)
				pThis->m_listAudioCache.pop_front();
			pThis->m_csAudioCache.Unlock();
			Sleep(5);
			continue;
		}

		if (nTimeSpan > 100)			// Á¬Ðø100msÃ»ÓÐÒôÆµÊý¾Ý£¬ÔòÊÓÎªÒôÆµÔÝÍ£
			pThis->m_pDsBuffer->StopPlay();
		else if (!pThis->m_pDsBuffer->IsPlaying())
			pThis->m_pDsBuffer->StartPlay();
		bool bPopFrame = false;

		// 			if (!pThis->m_pAudioPlayEvent->GetNotify(1))
		// 			{
		// 				continue;
		// 			}
		pThis->m_csAudioCache.Lock();
		if (pThis->m_listAudioCache.size() > 0)
		{
			FramePtr = pThis->m_listAudioCache.front();
			pThis->m_listAudioCache.pop_front();
			bPopFrame = true;
		}
		pThis->m_nAudioCache = pThis->m_listAudioCache.size();
		pThis->m_csAudioCache.Unlock();

		if (!bPopFrame)
		{
			Sleep(10);
			continue;
		}
		nFramesPlayed++;

		if (nFramesPlayed < 50 && dwOsMajorVersion < 6)
		{// ÐÞÕýÔÚXPÏµÍ³ÖÐ£¬Ç°50Ö¡»á±»Ë²¼ä¶ªµôµÄÎÊÌâ
			if (((TimeSpanEx(dfLastPlayTime) + dfPlayTimeSpan) * 1000) < nAudioFrameInterval)
				Sleep(nAudioFrameInterval - (TimeSpanEx(dfLastPlayTime) * 1000));
		}

		dfPlayTimeSpan = GetExactTime();
		if (pThis->m_pDsBuffer->IsPlaying())
		{
			if (pAudioDecoder->Decode(pPCM, nPCMSize, (byte *)FramePtr->Framedata(pThis->m_nSDKVersion), FramePtr->FrameHeader()->nLength) != 0)
			{
				if (!pThis->m_pDsBuffer->WritePCM(pPCM, nPCMSize))
					pThis->OutputMsg("%s Write PCM Failed.\n", __FUNCTION__);
				SetEvent(pThis->m_hAudioFrameEvent[nFrameEvent++ % 2]);
			}
			else
				pThis->OutputMsg("%s Audio Decode Failed Is.\n", __FUNCTION__);
		}
		dfPlayTimeSpan = TimeSpanEx(dfPlayTimeSpan);
		dfLastPlayTime = GetExactTime();
		tLastFrameTime = FramePtr->FrameHeader()->nTimestamp;
	}
	if (pPCM)
		delete[]pPCM;
	return 0;
}
#endif
bool CIPCPlayer::InitialziePlayer()
{
	if (m_nVideoCodec == CODEC_UNKNOWN ||		/// ÂëÁ÷Î´ÖªÔò³¢ÊÔÌ½²âÂë
		!m_nVideoWidth ||
		!m_nVideoHeight)
	{
		return false;
	}

	if (!m_pDecoder)
	{
		m_pDecoder = make_shared<CVideoDecoder>();
		if (!m_pDecoder)
		{
			OutputMsg("%s Failed in allocing memory for Decoder.\n", __FUNCTION__);
			return false;
		}
	}

	if (!InitizlizeDx())
	{
		assert(false);
		return false;
	}
	if (m_bD3dShared)
	{
		m_pDecoder->SetD3DShared(m_pDxSurface->GetD3D9(), m_pDxSurface->GetD3DDevice());
		m_pDxSurface->SetD3DShared(true);
	}

	// Ê¹ÓÃµ¥Ïß³Ì½âÂë,¶àÏß³Ì½âÂëÔÚÄ³´Ë±È½ÏÂýµÄCPUÉÏ¿ÉÄÜ»áÌá¸ßÐ§¹û£¬µ«ÏÖÔÚI5 2GHZÒÔÉÏµÄCPUÉÏµÄ¶àÏß³Ì½âÂëÐ§¹û²¢²»Ã÷ÏÔ·´¶ø»áÕ¼ÓÃ¸ü¶àµÄÄÚ´æ
	m_pDecoder->SetDecodeThreads(1);
	// ³õÊ¼»¯½âÂëÆ÷
	AVCodecID nCodecID = AV_CODEC_ID_NONE;
	switch (m_nVideoCodec)
	{
	case CODEC_H264:
		nCodecID = AV_CODEC_ID_H264;
		break;
	case CODEC_H265:
		nCodecID = AV_CODEC_ID_H265;
		break;
	default:
	{
		OutputMsg("%s You Input a unknown stream,Decode thread exit.\n", __FUNCTION__);
		return false;
		break;
	}
	}

#ifndef _WIN64
	m_bEnableHaccel = m_bEnableDDraw ? false : m_bEnableHaccel;
#endif
	if (!m_pDecoder->InitDecoder(nCodecID, m_nVideoWidth, m_nVideoHeight, m_bEnableHaccel))
	{
		OutputMsg("%s Failed in Initializing Decoder.\n", __FUNCTION__);
#ifdef _DEBUG
		OutputMsg("%s \tObject:%d Line %d Time = %d.\n", __FUNCTION__, m_nObjIndex, __LINE__, timeGetTime() - m_nLifeTime);
#endif
		return false;
	}
	return true;
}

struct DxDeallocator
{
public:
	CDxSurfaceEx *&m_pDxSurface;
#ifndef _WIN64
	CDirectDraw *&m_pDDraw;
	DxDeallocator(CDxSurfaceEx *&pDxSurface, CDirectDraw *&pDDraw)
		:m_pDxSurface(pDxSurface), m_pDDraw(pDDraw)
	{
	}
#else
	DxDeallocator(CDxSurfaceEx *&pDxSurface)
		: m_pDxSurface(pDxSurface)
	{
	}
#endif
	~DxDeallocator()
	{
#ifndef _WIN64
		DxTraceMsg("%s pSurface = %08X\tpDDraw = %08X.\n", __FUNCTION__, m_pDxSurface, m_pDDraw);
#else
		DxTraceMsg("%s pSurface = %08X.\n", __FUNCTION__, m_pDxSurface);
#endif
		if (m_pDxSurface)
		{
			if (strlen(m_pDxSurface->m_szAdapterID) > 0)
			{
				WCHAR szGUIDW[64] = { 0 };
				A2WHelper(m_pDxSurface->m_szAdapterID, szGUIDW, 64);
				for (int i = 0; i < g_pSharedMemory->nAdapterCount; i++)
				{
					if (wcscmp(g_pSharedMemory->HAccelArray[i].szAdapterGuid, szGUIDW) == 0)
					{
						if (!g_pSharedMemory->HAccelArray[i].hMutex)
							break;
						WaitForSingleObject(g_pSharedMemory->HAccelArray[i].hMutex, INFINITE);
						if (g_pSharedMemory->HAccelArray[i].nOpenCount > 0)
						{
							g_pSharedMemory->HAccelArray[i].nOpenCount--;
						}
#ifdef _DEBUG
						//else
						//{
						//	assert(false);
						//}
#endif
						ReleaseMutex(g_pSharedMemory->HAccelArray[i].hMutex);
					}
				}
			}
		}

		Safe_Delete(m_pDxSurface);
#ifndef _WIN64
		Safe_Delete(m_pDDraw);
#endif
	}
};
UINT __stdcall CIPCPlayer::ThreadDecode(void *p)
{
	CIPCPlayer* pThis = (CIPCPlayer *)p;
#ifdef _DEBUG
	pThis->OutputMsg("%s Timespan1 of First Frame = %f.\n", __FUNCTION__, TimeSpanEx(pThis->m_dfFirstFrameTime));
#endif

	DeclareRunTime(5);
	
#ifdef _DEBUG
	pThis->OutputMsg("%s \tObject:%d m_nLifeTime = %d.\n", __FUNCTION__, pThis->m_nObjIndex, timeGetTime() - pThis->m_nLifeTime);
#endif

	if (!pThis->m_hRenderWnd)
		pThis->OutputMsg("%s Warning!!!A Windows handle is Needed otherwith the video Will not showed..\n", __FUNCTION__);
	// ´´½¨¶àÃ½ÌåÊÂ¼þ
	//TimerEvent PlayEvent(1000 / pThis->m_nVideoFPS);
	int nIPCPlayInterval = 1000 / pThis->m_nVideoFPS;
	shared_ptr<CMMEvent> pRenderTimer = make_shared<CMMEvent>(pThis->m_hRenderAsyncEvent, nIPCPlayInterval);
	// Á¢¼´¿ªÊ¼äÖÈ¾»­Ãæ
	SetEvent(pThis->m_hRenderAsyncEvent);
	// µÈ´ýÓÐÐ§µÄÊÓÆµÖ¡Êý¾Ý
	long tFirst = timeGetTime();
	int nTimeoutCount = 0;
	while (pThis->m_bThreadDecodeRun)
	{
		AutoLock(pThis->m_csVideoCache.Get());
		if ((timeGetTime() - tFirst) > 5000)
		{// µÈ´ý³¬Ê±
			//assert(false);
			pThis->OutputMsg("%s Warning!!!Wait for frame timeout(5s),times %d.\n", __FUNCTION__,++nTimeoutCount);
			break;
		}
		if (pThis->m_listVideoCache.size() < 1)
		{
			Lock.Unlock();
			Sleep(20);
			continue;
		}
		else
			break;
	}
	SaveRunTime();
	if (!pThis->m_bThreadDecodeRun)
	{
		//assert(false);
		return 0;
	}

	// µÈ´ýIÖ¡
	// tFirst = timeGetTime();
	//		DWORD dfTimeout = 3000;
	// 		if (!pThis->m_bIpcStream)	// Ö»ÓÐIPCÂëÁ÷²ÅÐèÒª³¤Ê±¼äµÈ´ý
	// 			dfTimeout = 1000;
	
	AVCodecID nCodecID = AV_CODEC_ID_NONE;
	/* ½ûÓÃÌ½²âÂëÁ÷µÄ´úÂë£¬Ì½²âÂëÁ÷»áµ¼ÖÂÍ¼ÏñÑÓÊ±³ÊÏÖ
	int nDiscardFrames = 0;
	bool bProbeSucced = false;
	if (pThis->m_nVideoCodec == CODEC_UNKNOWN ||		/// ÂëÁ÷Î´ÖªÔò³¢ÊÔÌ½²âÂë
		!pThis->m_nVideoWidth ||
		!pThis->m_nVideoHeight)
	{
		bool bGovInput = false;
		while (pThis->m_bThreadDecodeRun)
		{
			if ((timeGetTime() - tFirst) >= pThis->m_nProbeStreamTimeout)
				break;
			CAutoLock lock(&pThis->m_csVideoCache, false, __FILE__, __FUNCTION__, __LINE__);
			if (pThis->m_listVideoCache.size() > 1)
				break;
			Sleep(25);
		}
		if (!pThis->m_bThreadDecodeRun)
		{
			assert(false);
			return 0;
		}
		
		auto itStart = pThis->m_listVideoCache.begin();
		while (!bProbeSucced && pThis->m_bThreadDecodeRun)
		{
#ifndef _DEBUG
			if ((timeGetTime() - tFirst) < pThis->m_nProbeStreamTimeout)
#else
			if ((timeGetTime() - tFirst) < INFINITE)
#endif
			{
				Sleep(5);
				CAutoLock lock(&pThis->m_csVideoCache, false, __FILE__, __FUNCTION__, __LINE__);
				//auto it = find_if(itPos, pThis->m_listVideoCache.end(), StreamFrame::IsIFrame);
				auto it = itStart;
				if (it != pThis->m_listVideoCache.end())
				{// Ì½²âÂëÁ÷ÀàÐÍ
					itStart = it;
					itStart++;
					pThis->OutputMsg("%s Probestream FrameType = %d\tFrameLength = %d.\n", __FUNCTION__, (*it)->FrameHeader()->nType, (*it)->FrameHeader()->nLength);
					if ((*it)->FrameHeader()->nType == FRAME_GOV)
					{
						if (bGovInput)
							continue;
						bGovInput = true;
						if (bProbeSucced = pThis->ProbeStream((byte *)(*it)->Framedata(pThis->m_nSDKVersion), (*it)->FrameHeader()->nLength))
							break;
					}
					else
						if (bProbeSucced = pThis->ProbeStream((byte *)(*it)->Framedata(pThis->m_nSDKVersion), (*it)->FrameHeader()->nLength))
							break;
				}
			}
			else
			{
#ifdef _DEBUG
				pThis->OutputMsg("%s Warning!!!\nThere is No an I frame in %d second.m_listVideoCache.size() = %d.\n", __FUNCTION__, (int)pThis->m_nProbeStreamTimeout / 1000, pThis->m_listVideoCache.size());
				pThis->OutputMsg("%s \tObject:%d Line %d Time = %d.\n", __FUNCTION__, pThis->m_nObjIndex, __LINE__, timeGetTime() - pThis->m_nLifeTime);
#endif
				if (pThis->m_hRenderWnd)
					::PostMessage(pThis->m_hRenderWnd, WM_IPCPLAYER_MESSAGE, IPCPLAYER_NOTRECVIFRAME, 0);
				assert(false);
				return 0;
			}
		}
		if (!pThis->m_bThreadDecodeRun)
		{
			assert(false);
			return 0;
		}

		if (!bProbeSucced)		// Ì½²âÊ§°Ü
		{
			pThis->OutputMsg("%s Failed in ProbeStream,you may input a unknown stream.\n", __FUNCTION__);
#ifdef _DEBUG
			pThis->OutputMsg("%s \tObject:%d Line %d Time = %d.\n", __FUNCTION__, pThis->m_nObjIndex, __LINE__, timeGetTime() - pThis->m_nLifeTime);
#endif
			if (pThis->m_hRenderWnd)
				::PostMessage(pThis->m_hRenderWnd, WM_IPCPLAYER_MESSAGE, IPCPLAYER_UNKNOWNSTREAM, 0);
			assert(false);
			return 0;
		}
		
		// °ÑffmpegµÄÂëÁ÷ID×ªÎªIPCµÄÂëÁ÷ID,²¢ÇÒÖ»Ö§³ÖH264ºÍHEVC
		// nCodecID = pThis->m_pStreamProbe->nProbeAvCodecID;
		if (nCodecID != AV_CODEC_ID_H264 &&
			nCodecID != AV_CODEC_ID_HEVC)
		{
			pThis->m_nVideoCodec = CODEC_UNKNOWN;
			pThis->OutputMsg("%s Probed a unknown stream,Decode thread exit.\n", __FUNCTION__);
			if (pThis->m_hRenderWnd)
				::PostMessage(pThis->m_hRenderWnd, WM_IPCPLAYER_MESSAGE, IPCPLAYER_UNKNOWNSTREAM, 0);
			assert(false);
			return 0;
		}
	}*/
	SaveRunTime();
	switch (pThis->m_nVideoCodec)
	{
	case CODEC_H264:
		nCodecID = AV_CODEC_ID_H264;
		break;
	case CODEC_H265:
		nCodecID = AV_CODEC_ID_H265;
		break;
	case CODEC_MJPEG:
		nCodecID = AV_CODEC_ID_MJPEG;
		break;
	default:
	{
		pThis->OutputMsg("%s You Input a unknown stream,Decode thread exit.\n", __FUNCTION__);
		if (pThis->m_hRenderWnd)	// ÔÚÏß³ÌÖÐ¾¡Á¿±ÜÃâÊ¹ÓÃSendMessage£¬ÒòÎª¿ÉÄÜ»áµ¼ÖÂ×èÈû
			::PostMessage(pThis->m_hRenderWnd, WM_IPCPLAYER_MESSAGE, IPCPLAYER_UNSURPPORTEDSTREAM, 0);
		//assert(false);
		return 0;
		break;
	}
	}

	int nRetry = 0;

	shared_ptr<CVideoDecoder>pDecodec = make_shared<CVideoDecoder>();
	if (!pDecodec)
	{
		pThis->OutputMsg("%s Failed in allocing memory for Decoder.\n", __FUNCTION__);
		assert(false);
		return 0;
	}
	SaveRunTime();
	CHAR szAdapterID[64] = { 0 };
	if (!pThis->m_bEnableDDraw && 
		(pThis->m_bEnableHaccel ||
		(g_pSharedMemory &&
		g_pSharedMemory->bHAccelPreferred)))
	{
		// ³¢ÊÔ´ò¿ªÓ²½âÉèÖÃ
		if (pThis->TryEnableHAccelOnAdapter(szAdapterID, 64))
		{
			pThis->m_bEnableHaccel = true;
			pThis->m_bD3dShared = true;
		}
		else
		{// ³¬³öÓ²½âÉèÖÃÊýÁ¿?½ûÓÃÓ²½â
			pThis->m_bEnableHaccel = false;
			pThis->m_bD3dShared = false;
		}
	}

	pThis->OutputMsg("%s Try to InitizlizeDx.\n", __FUNCTION__);
	if (!pThis->m_nVideoWidth)
		pThis->m_nVideoWidth = 1280;
	if (!pThis->m_nVideoHeight)
		pThis->m_nVideoHeight = 720;
	
	if (!pThis->InitizlizeDx())
	{
		assert(false);
		return 0;
	}
	if (pThis->m_bEnableHaccel && strlen(szAdapterID))
		strcpy_s(pThis->m_pDxSurface->m_szAdapterID, 64, szAdapterID);
	/*if (!pThis->m_pDxSurface)
	{
		assert(false);
		return 0;
	}*/
#ifdef _WIN64
	shared_ptr<DxDeallocator> DxDeallocatorPtr = make_shared<DxDeallocator>(pThis->m_pDxSurface);
#else
	shared_ptr<DxDeallocator> DxDeallocatorPtr = make_shared<DxDeallocator>(pThis->m_pDxSurface, pThis->m_pDDraw);
	pThis->m_bD3dShared = pThis->m_bEnableDDraw ? false : pThis->m_bD3dShared;
#endif
	SaveRunTime();
	

	if (pThis->m_bD3dShared && pThis->m_pDxSurface)
	{
		pDecodec->SetD3DShared(pThis->m_pDxSurface->GetD3D9(), pThis->m_pDxSurface->GetD3DDevice());
		pThis->m_pDxSurface->SetD3DShared(true);
	}

	// Ê¹ÓÃµ¥Ïß³Ì½âÂë,¶àÏß³Ì½âÂëÔÚÄ³´Ë±È½ÏÂýµÄCPUÉÏ¿ÉÄÜ»áÌá¸ßÐ§¹û£¬µ«ÏÖÔÚI5 2GHZÒÔÉÏµÄCPUÉÏµÄ¶àÏß³Ì½âÂëÐ§¹û²¢²»Ã÷ÏÔ·´¶ø»áÕ¼ÓÃ¸ü¶àµÄÄÚ´æ
	pDecodec->SetDecodeThreads(1);
	// ³õÊ¼»¯½âÂëÆ÷	
	while (pThis->m_bThreadDecodeRun)
	{// Ä³´ËÊ±ºò¿ÉÄÜ»áÒòÎªÄÚ´æ»ò×ÊÔ´²»¹»µ¼ÖÂ³õÊ¼»¯½âÂë²Ù×÷ÐÔ,Òò´Ë¿ÉÒÔÑÓ³ÙÒ»¶ÎÊ±¼äºóÔÙ´Î³õÊ¼»¯£¬Èô¶à´Î³õÊ¼»¯ÈÔ²»³É¹¦£¬ÔòÐèÍË³öÏß³Ì
		//DeclareRunTime(5);
		//SaveRunTime();
		if (!pDecodec->InitDecoder(nCodecID, pThis->m_nVideoWidth, pThis->m_nVideoHeight, pThis->m_bEnableHaccel))
		{
			pThis->OutputMsg("%s Failed in Initializing Decoder:CodeCID =%d,Width = %d,Height = %d,HWAccel = %d.\n", __FUNCTION__, nCodecID, pThis->m_nVideoWidth, pThis->m_nVideoHeight, pThis->m_bEnableHaccel);
#ifdef _DEBUG
			pThis->OutputMsg("%s \tObject:%d Line %d Time = %d.\n", __FUNCTION__, pThis->m_nObjIndex, __LINE__, timeGetTime() - pThis->m_nLifeTime);
#endif
			nRetry++;
			if (nRetry >= 3)
			{
				if (pThis->m_hRenderWnd)// ÔÚÏß³ÌÖÐ¾¡Á¿±ÜÃâÊ¹ÓÃSendMessage£¬ÒòÎª¿ÉÄÜ»áµ¼ÖÂ×èÈû
					::PostMessage(pThis->m_hRenderWnd, WM_IPCPLAYER_MESSAGE, IPCPLAYER_INITDECODERFAILED, 0);
				return 0;
			}
			Delay(2500, pThis->m_bThreadDecodeRun);
		}
		else
			break;
		//SaveRunTime();
	}
	SaveRunTime();
	if (!pThis->m_bThreadDecodeRun)
	{
		//assert(false);
		return 0;
	}

	if (pThis->m_pStreamProbe)
		pThis->m_pStreamProbe = nullptr;

	AVPacket *pAvPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
	shared_ptr<AVPacket>AvPacketPtr(pAvPacket, av_free);
	av_init_packet(pAvPacket);
	AVFrame *pAvFrame = av_frame_alloc();
	shared_ptr<AVFrame>AvFramePtr(pAvFrame, av_free);

	StreamFramePtr FramePtr;
	int nGot_picture = 0;
	DWORD nResult = 0;
	float fTimeSpan = 0;
	int nFrameInterval = pThis->m_nFileFrameInterval;
	pThis->m_dfTimesStart = GetExactTime();

	//		 È¡µÃµ±Ç°ÏÔÊ¾Æ÷µÄË¢ÐÂÂÊ,ÔÚ´¹Ö±Í¬²½Ä£Ê½ÏÂ,ÏÔÊ¾Æ÷µÄË¢ÐÂÂÊ¾ö¶¨ÁË£¬ÏÔÊ¾Í¼ÏñµÄ×î¸ßÖ¡Êý
	//		 Í¨¹ýÍ³¼ÆÃ¿ÏÔÊ¾Ò»Ö¡Í¼Ïñ(º¬½âÂëºÍÏÔÊ¾)ºÄ·ÑµÄÊ±¼ä
	// 		DEVMODE   dm;
	// 		dm.dmSize = sizeof(DEVMODE);
	// 		::EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);		
	// 		int nRefreshInvertal = 1000 / dm.dmDisplayFrequency;	// ÏÔÊ¾Æ÷Ë¢ÐÂ¼ä¸ô

	double dfDecodeStartTime = GetExactTime();
	double dfRenderTime = GetExactTime() - pThis->m_fPlayInterval;	// Í¼Ïñ±»ÏÔÊ¾µÄÊ±¼ä
	double dfRenderStartTime = 0.0f;
	double dfRenderTimeSpan = 0.000f;
	double dfTimeSpan = 0.0f;

#ifdef _DEBUG
	pThis->m_csVideoCache.Lock();
	pThis->OutputMsg("%s Video cache Size = %d .\n", __FUNCTION__, pThis->m_listVideoCache.size());
	pThis->m_csVideoCache.Unlock();
	pThis->OutputMsg("%s \tObject:%d Start Decoding.\n", __FUNCTION__, pThis->m_nObjIndex);
#endif
	//	    ÒÔÏÂ´úÂëÓÃÒÔ²âÊÔ½âÂëºÍÏÔÊ¾Õ¼ÓÃÊ±¼ä£¬½¨Òé²»ÒªÉ¾³ý		
	// 		TimeTrace DecodeTimeTrace("DecodeTime", __FUNCTION__);
	// 		TimeTrace RenderTimeTrace("RenderTime", __FUNCTION__);

	int nIFrameTime = 0;
	CStat FrameStat(pThis->m_nObjIndex);		// ½âÂëÍ³¼Æ
	int nFramesAfterIFrame = 0;		// Ïà¶ÔIÖ¡µÄ±àºÅ,IÖ¡ºóµÄµÚÒ»Ö¡Îª1£¬µÚ¶þÖ¡Îª2ÒÀ´ËÀàÍÆ
	int nSkipFrames = 0;
	bool bDecodeSucceed = false;
	
	pThis->m_tFirstFrameTime = 0;
	float fLastPlayRate = pThis->m_fPlayRate;		// ¼ÇÂ¼ÉÏÒ»´ÎµÄ²¥·ÅËÙÂÊ£¬µ±²¥·ÅËÙÂÊ·¢Éú±ä»¯Ê±£¬ÐèÒªÖØÖÃÖ¡Í³¼ÆÊý¾Ý

	if (pThis->m_dwStartTime)
	{
		pThis->OutputMsg("%s %d Render Timespan = %d.\n", __FUNCTION__, __LINE__, timeGetTime() - pThis->m_dwStartTime);
		pThis->m_dwStartTime = 0;
	}

	if (!pThis->m_hEventFlushDecoder)
	{
		pThis->m_hEventFlushDecoder = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	int nFramesPlayed = 0;			// ²¥·Å×Ü·«Êý
	double dfTimeStartPlay = GetExactTime();// ²¥·ÅÆðÊ¼Ê±¼ä
	int nTimePlayFrame = 0;		// ²¥·ÅÒ»Ö¡ËùºÄ·ÑÊ±¼ä£¨MS£©
	int nPlayCount = 0;
	int TPlayArray[100] = { 0 };
	double dfT1 = GetExactTime();
	int nVideoCacheSize = 0;
	LONG nTotalDecodeFrames = 0;
	dfDecodeStartTime = GetExactTime() - pThis->m_nPlayFrameInterval / 1000.0f;
	SaveRunTime();
	pThis->m_pDecoder = pDecodec;
	int nRenderTimes = 0;
	
	double dfTimeDecodeStart = 0.0f;
	int dfDecodeTimeSpan = 0;
	int nAvError = 0;
	char szAvError[1024] = { 0 };
	
#ifdef _DEBUG
	CStat  RenderInterval("RenderInterval", pThis->m_nObjIndex);
	CStat IFrameStat("I Frame Decode Time",0);		// IÖ¡½âÂëÍ³¼Æ
	CStat PopupTime("Frame Popup Time",pThis->m_nObjIndex);
	CStat TimeDecode("Decode Time", pThis->m_nObjIndex);
	CStat RenderTime("Frame Render Time", pThis->m_nObjIndex);
	DWORD dwFrameTimeInput;
	pThis->OutputMsg("%s Timespan2 of First Frame = %f.\n", __FUNCTION__, TimeSpanEx(pThis->m_dfFirstFrameTime));
#endif
	
	while (pThis->m_bThreadDecodeRun)
	{
		if (!pThis->m_bIpcStream &&
			pThis->m_bPause)
		{// Ö»ÓÐ·ÇIPCÂëÁ÷²Å¿ÉÒÔÔÝÍ£
			Sleep(40);
			continue;
		}
		pThis->m_csVideoCache.Lock();
		nVideoCacheSize = pThis->m_listVideoCache.size();
		pThis->m_csVideoCache.Unlock();
		if (WaitForSingleObject(pThis->m_hEventFlushDecoder, 0) == WAIT_OBJECT_0)
		{// Çå¿Õ½â»º´æ
			pDecodec->FlushDecodeBuffer();
			continue;
		}

		dfDecodeStartTime = GetExactTime();
		if (!pThis->m_bIpcStream)
		{// ÎÄ¼þ»òÁ÷Ã½Ìå²¥·Å£¬¿Éµ÷½Ú²¥·ÅËÙ¶È
			int nTimeSpan1 = (int)(TimeSpanEx(dfDecodeStartTime) * 1000);
			if (nVideoCacheSize < 2 &&
				(pThis->m_nPlayFrameInterval - nTimeSpan1) > 5)
			{
				Sleep(5);
				continue;
			}
			bool bPopFrame = false;
			// ²éÕÒÊ±¼äÉÏ×îÆ¥ÅäµÄÖ¡,²¢É¾³ý²»Æ¥ÅäµÄ·ÇIÖ¡
			int nSkipFrames = 0;
			CAutoLock lock(&pThis->m_csVideoCache, false, __FILE__, __FUNCTION__, __LINE__);
			if (!pThis->m_tFirstFrameTime &&
				pThis->m_listVideoCache.size() > 0)
				pThis->m_tFirstFrameTime = pThis->m_listVideoCache.front()->FrameHeader()->nTimestamp;
			for (auto it = pThis->m_listVideoCache.begin(); it != pThis->m_listVideoCache.end();)
			{
				time_t tFrameSpan = ((*it)->FrameHeader()->nTimestamp - pThis->m_tLastFrameTime) / 1000;
				if (StreamFrame::IsIFrame(*it))
				{
					bPopFrame = true;
					break;
				}
				if (pThis->m_fPlayRate < 16.0 && // 16±¶ËÙÒÔÏÂ£¬²Å¿¼ÂÇ°´Ê±ÌøÖ¡
					tFrameSpan / pThis->m_fPlayRate >= max(pThis->m_fPlayInterval, FrameStat.GetAvgValue() * 1000))
				{
					bPopFrame = true;
					break;
				}
				else
				{
					it = pThis->m_listVideoCache.erase(it);
					nSkipFrames++;
				}
			}
			if (nSkipFrames)
				pThis->OutputMsg("%s Skip Frames = %d bPopFrame = %s.\n", __FUNCTION__, nSkipFrames, bPopFrame ? "true" : "false");
			if (bPopFrame)
			{
				FramePtr = pThis->m_listVideoCache.front();
				pThis->m_listVideoCache.pop_front();
				//pThis->OutputMsg("%s Pop a Frame ,FrameID = %d\tFrameTimestamp = %d.\n", __FUNCTION__, FramePtr->FrameHeader()->nFrameID, FramePtr->FrameHeader()->nTimestamp);
			}
			pThis->m_nVideoCache = pThis->m_listVideoCache.size();
			if (!bPopFrame)
			{
				lock.Unlock();	// ÐëÌáÇ°½âËø£¬²»È»Sleepºó²Å»á½âËø£¬µ¼ÖÂÆäËüµØ·½±»Ëø×¡
				Sleep(10);
				continue;
			}
			lock.Unlock();
			pAvPacket->data = (uint8_t *)FramePtr->Framedata(pThis->m_nSDKVersion);
			pAvPacket->size = FramePtr->FrameHeader()->nLength;
			pThis->m_tLastFrameTime = FramePtr->FrameHeader()->nTimestamp;
			av_frame_unref(pAvFrame);
			int nFrameID = FramePtr->FrameHeader()->nFrameID;
#ifdef _DEBUG
			dfDecodeTimeSpan = TimeSpanEx(FramePtr->dfInputTime);
#endif
			nAvError = pDecodec->Decode(pAvFrame, nGot_picture, pAvPacket);
			av_log(nullptr, 1, "avError = %d.\n", nAvError);
			nTotalDecodeFrames++;
			av_packet_unref(pAvPacket);
			if (nAvError < 0)
			{
				av_strerror(nAvError, szAvError, 1024);
				pThis->OutputMsg("%s Frame :%d Decode Error:%s.\n", __FUNCTION__, FramePtr->FrameHeader()->nFrameID, szAvError);
				//dfDecodeStartTime = GetExactTime();
				continue;
			}
// 			avcodec_flush_buffers()			
// 			dfDecodeTimespan = TimeSpanEx(dfDecodeStartTime);
// 			if (StreamFrame::IsIFrame(FramePtr))			// Í³¼ÆIÖ¡½âÂëÊ±¼ä
// 				IFrameStat.Stat(dfDecodeTimespan);
// 			FrameStat.Stat(dfDecodeTimespan);	// Í³¼ÆËùÓÐÖ¡½âÂëÊ±¼ä
// 			if (fLastPlayRate != pThis->m_fPlayRate)
// 			{// ²¥·ÅËÙÂÊ·¢Éú±ä»¯£¬ÖØÖÃÍ³¼ÆÊý¾Ý
// 				IFrameStat.Reset();
// 				FrameStat.Reset();
// 			}
			fLastPlayRate = pThis->m_fPlayRate;
			fTimeSpan = (TimeSpanEx(dfRenderTime) + dfRenderTimeSpan) * 1000;
			int nSleepTime = 0;
			if (fTimeSpan < pThis->m_fPlayInterval)
			{
				nSleepTime = (int)(pThis->m_fPlayInterval - fTimeSpan);
				if (pThis->m_nDecodeDelay == -1)
					Sleep(nSleepTime);
				else if (pThis->m_nDecodeDelay)
					Sleep(pThis->m_nDecodeDelay);
				else
					continue;
			}
		}
		else
		{// IPC ÂëÁ÷£¬ÔòÖ±½Ó²¥·Å
// 			if (nAvError > 0)
// 			{
// 				int nWaitTime = nIPCPlayInterval - (int)(dfDecodeTimeSpan*1000);
// 				if (nWaitTime > 0)
// 					WaitForSingleObject(pThis->m_hRenderAsyncEvent, nWaitTime);		// ÓÃÓÚ¸ù¾ÝÖ¡ÂÊÀ´¿ØÖÆ²¥·ÅËÙ¶È£¬Ê¹»­ÃæÎÈ¶¨Á÷³©²¥·Å
// 				if (nVideoCacheSize >= 10)
// 				{
// 					int nMult = (nVideoCacheSize - 10) * 4 / 10;
// 					if (pRenderTimer->nPeriod != (nIPCPlayInterval - nMult))	// ²¥·Å¼ä¸ô½µµÍ40%,¿ÉÒÔÑ¸ËÙÇå¿Õ»ýÀÛÖ¡
// 						pRenderTimer->UpdateInterval((nIPCPlayInterval - nMult));
// 				}
// 				else if (pRenderTimer->nPeriod != nIPCPlayInterval)
// 					pRenderTimer->UpdateInterval(nIPCPlayInterval);
// 			}
// 				if (WaitForSingleObject(pThis->m_hRenderAsyncEvent, nIPCPlayInterval) == WAIT_TIMEOUT)
// 				{
// 					TraceMsgA("%s Wait time out.\n", __FUNCTION__);
// 				}
// 			if (nVideoCacheSize == 0)
// 			{
// 				if (WaitForSingleObject(pThis->m_hInputFrameEvent, 25) == WAIT_TIMEOUT)
// 					continue;
// 			}
		
			bool bPopFrame = false;
			AutoLockAgent(pThis->m_csVideoCache);
			if (pThis->m_listVideoCache.size() > 0)
			{
				FramePtr = pThis->m_listVideoCache.front();
				pThis->m_listVideoCache.pop_front();
				bPopFrame = true;
				nVideoCacheSize = pThis->m_listVideoCache.size();
			}
			UnlockAgent();
			if (!bPopFrame)
			{
				Sleep(10);
				continue;
			}
 #ifdef _DEBUG
// 			PopupTime.Stat((float)MMTimeSpan(FramePtr->FrameHeader()->nTimestamp));
// 			if (PopupTime.IsFull())
// 			{
// 				PopupTime.OutputStat();
// 				PopupTime.Reset();
// 			}
			dwFrameTimeInput = FramePtr->FrameHeader()->nTimestamp;
 #endif
			pAvPacket->data = (uint8_t *)FramePtr->Framedata(pThis->m_nSDKVersion);
			pAvPacket->size = FramePtr->FrameHeader()->nLength;
			pThis->m_tLastFrameTime = FramePtr->FrameHeader()->nTimestamp;
			av_frame_unref(pAvFrame);
			int nFrameSize = pAvPacket->size;
#ifdef _DEBUG
			int nNalType = pAvPacket->data[4] & 0x1F;
#endif
			dfDecodeStartTime = GetExactTime();
			// ½âÂëIÖ¡¿ÉÄÜÐèÒª30msÒÔÉÏµÄÊ±¼ä£¬¶ø½âPÖ¡»òBÖ¡¿ÉÄÜÖ»ÐèÒª²»µ½5msµÄÊ±¼ä
			nAvError = pDecodec->Decode(pAvFrame, nGot_picture, pAvPacket);
			nTotalDecodeFrames++;
			av_packet_unref(pAvPacket);
			if (nAvError < 0)
			{
				av_frame_unref(pAvFrame);
				av_strerror(nAvError, szAvError, 1024);
				continue;
			}
			dfDecodeTimeSpan = TimeSpanEx(dfDecodeStartTime);

#ifdef _DEBUG
// 			if (nNalType == 5)
// 			{
// 				IFrameStat.Stat(dfDecodeTimeSpan);
// 				if (IFrameStat.IsFull())
// 				{
// 					IFrameStat.OutputStat();
// 					IFrameStat.Reset();
// 				}
// 			}
// 			TimeDecode.Stat(dfDecodeTimeSpan);
// 			if (TimeDecode.IsFull())
// 			{
// 				TimeDecode.OutputStat(25);
// 				TimeDecode.Reset();
// 			}
#endif
		}
#ifdef _DEBUG
// 		if (pThis->m_bSeekSetDetected)
// 		{
// 			int nFrameID = FramePtr->FrameHeader()->nFrameID;
// 			int nTimeStamp = FramePtr->FrameHeader()->nTimestamp / 1000;
// 			pThis->OutputMsg("%s First Frame after SeekSet:ID = %d\tTimeStamp = %d.\n", __FUNCTION__, nFrameID, nTimeStamp);
// 			pThis->m_bSeekSetDetected = false;
// 		}
#endif	
		if (nGot_picture)
		{
			pThis->m_nDecodePixelFmt = (AVPixelFormat)pAvFrame->format;
			SetEvent(pThis->m_hEvnetYUVReady);
			SetEvent(pThis->m_hEventDecodeStart);
			pThis->m_nCurVideoFrame = FramePtr->FrameHeader()->nFrameID;
			pThis->m_tCurFrameTimeStamp = FramePtr->FrameHeader()->nTimestamp;
			pThis->ProcessYUVFilter(pAvFrame, (LONGLONG)pThis->m_nCurVideoFrame);
			if (!pThis->m_bIpcStream &&
				1.0f == pThis->m_fPlayRate  &&
				pThis->m_bEnableAudio &&
				pThis->m_hAudioFrameEvent[0] &&
				pThis->m_hAudioFrameEvent[1])
			{
				if (pThis->m_nDecodeDelay == -1)
					WaitForMultipleObjects(2, pThis->m_hAudioFrameEvent, TRUE, 40);
				else if (!pThis->m_nDecodeDelay)
					WaitForMultipleObjects(2, pThis->m_hAudioFrameEvent, TRUE, pThis->m_nDecodeDelay);
			}
			dfRenderStartTime = GetExactTime();
			pThis->RenderFrame(pAvFrame);

#ifdef _DEBUG
// 			RenderTime.Stat(MMTimeSpan(dwFrameTimeInput));
// 			if (RenderTime.IsFull())
// 			{
// 				RenderTime.OutputStat();
// 				RenderTime.Reset();
// 			}
			//RenderInterval.Stat(dfDecodeTimeSpan);
// 			RenderInterval.Stat(TimeSpanEx(dfRenderStartTime));
// 			if (RenderInterval.IsFull())
// 			{
// 				RenderInterval.OutputStat();
// 				RenderInterval.Reset();
// 			}
#endif
////////////////////////////////////////////////////////////////////////
//äÖÈ¾Ê±¼äÍ³¼Æ´úÂë£¬¿ÉÆÁ±Î
// 				float dfRenderTimespan = (float)(TimeSpanEx(dfRenderStartTime) * 1000);
// 				RenderInterval.Stat(dfRenderTimespan);
// 				if (RenderInterval.IsFull())
// 				{
// 					RenderInterval.OutputStat();
// 					RenderInterval.Reset();
// 				}
// 				nRenderTimes++;
////////////////////////////////////////////////////////////////////////
			if (!bDecodeSucceed)
			{
				bDecodeSucceed = true;
#ifdef _DEBUG
				pThis->OutputMsg("%s \tObject:%d  SetEvent Snapshot  m_nLifeTime = %d.\n", __FUNCTION__, pThis->m_nObjIndex, timeGetTime() - pThis->m_nLifeTime);
#endif
			}
			pThis->ProcessSnapshotRequire(pAvFrame);
			pThis->ProcessYUVCapture(pAvFrame, (LONGLONG)pThis->m_nCurVideoFrame);
			LineLockAgent(pThis->m_csFilePlayCallBack);
			if (pThis->m_pFilePlayCallBack)
				pThis->m_pFilePlayCallBack(pThis, pThis->m_pUserFilePlayer);
		}
		else
		{
			pThis->OutputMsg("%s \tObject:%d Decode Succeed but Not get a picture ,FrameType = %d\tFrameLength %d.\n", __FUNCTION__, pThis->m_nObjIndex, FramePtr->FrameHeader()->nType, FramePtr->FrameHeader()->nLength);
		}

		dfRenderTimeSpan = TimeSpanEx(dfRenderStartTime);
		nTimePlayFrame = (int)(TimeSpanEx(dfDecodeStartTime) * 1000);
		dfRenderTime = GetExactTime();
// 			if ((nTotalDecodeFrames % 100) == 0)
// 			{
// 				pThis->OutputMsg("%s nTotalDecodeFrames = %d\tnRenderTimes = %d.\n", __FUNCTION__,nTotalDecodeFrames, nRenderTimes);
// 			}
	}
	pThis->OutputMsg("%s Object %d Decode Frames:%d.\n", __FUNCTION__, pThis->m_nObjIndex, nTotalDecodeFrames);
	av_frame_unref(pAvFrame);
	SaveRunTime();
	pThis->m_pDecoder = nullptr;
	CloseHandle(pThis->m_hEventFlushDecoder);
	pThis->m_hEventFlushDecoder = nullptr;
	return 0;
}

UINT __stdcall CIPCPlayer::ThreadDecodeCache(void *p)
{
	CIPCPlayer* pThis = (CIPCPlayer *)p;
#ifdef _DEBUG
	pThis->OutputMsg("%s Timespan1 of First Frame = %f.\n", __FUNCTION__, TimeSpanEx(pThis->m_dfFirstFrameTime));
#endif

	DeclareRunTime(5);

#ifdef _DEBUG
	pThis->OutputMsg("%s \tObject:%d m_nLifeTime = %d.\n", __FUNCTION__, pThis->m_nObjIndex, timeGetTime() - pThis->m_nLifeTime);
#endif

	if (!pThis->m_hRenderWnd)
		pThis->OutputMsg("%s Warning!!!A Windows handle is Needed otherwith the video Will not showed..\n", __FUNCTION__);
	// ´´½¨¶àÃ½ÌåÊÂ¼þ
	//TimerEvent PlayEvent(1000 / pThis->m_nVideoFPS);
	int nIPCPlayInterval = 1000 / pThis->m_nVideoFPS;
	
	// µÈ´ýÓÐÐ§µÄÊÓÆµÖ¡Êý¾Ý
	long tFirst = timeGetTime();
	int nTimeoutCount = 0;
	while (pThis->m_bThreadDecodeRun)
	{
		AutoLock(pThis->m_csVideoCache.Get());
		if (pThis->m_listVideoCache.size() < 5)
		{
			Lock.Unlock();
			Sleep(20);
			continue;
		}
		else
			break;
	}
	SaveRunTime();
	if (!pThis->m_bThreadDecodeRun)
	{
		//assert(false);
		return 0;
	}

	AVCodecID nCodecID = AV_CODEC_ID_NONE;
	SaveRunTime();
	switch (pThis->m_nVideoCodec)
	{
	case CODEC_H264:
		nCodecID = AV_CODEC_ID_H264;
		break;
	case CODEC_H265:
		nCodecID = AV_CODEC_ID_H265;
		break;
	default:
	{
		pThis->OutputMsg("%s You Input a unknown stream,Decode thread exit.\n", __FUNCTION__);
		if (pThis->m_hRenderWnd)	// ÔÚÏß³ÌÖÐ¾¡Á¿±ÜÃâÊ¹ÓÃSendMessage£¬ÒòÎª¿ÉÄÜ»áµ¼ÖÂ×èÈû
			::PostMessage(pThis->m_hRenderWnd, WM_IPCPLAYER_MESSAGE, IPCPLAYER_UNSURPPORTEDSTREAM, 0);
		assert(false);
		return 0;
		break;
	}
	}

	int nRetry = 0;

	shared_ptr<CVideoDecoder>pDecodec = make_shared<CVideoDecoder>();
	if (!pDecodec)
	{
		pThis->OutputMsg("%s Failed in allocing memory for Decoder.\n", __FUNCTION__);
		assert(false);
		return 0;
	}
	SaveRunTime();
	pThis->OutputMsg("%s Try to InitizlizeDx.\n", __FUNCTION__);
	if (pThis->m_nVideoWidth && pThis->m_nVideoHeight)
	{
		if (!pThis->InitizlizeDx())
		{
			TraceFunction();
			assert(false);
			return 0;
		}
		pThis->OutputMsg("%s Try to Test m_pDxSurface.\n", __FUNCTION__);
		if (!pThis->m_pDxSurface)
		{
			TraceFunction();
			assert(false);
			return 0;
		}
	}

	CHAR szAdapterID[64] = { 0 };
	if (pThis->m_bEnableHaccel ||
		(g_pSharedMemory &&
		g_pSharedMemory->bHAccelPreferred))
	{
		// ³¢ÊÔÓ²½â
		if (pThis->m_pDxSurface &&
			pThis->TryEnableHAccelOnAdapter(szAdapterID, 64))
		{
			strcpy_s(pThis->m_pDxSurface->m_szAdapterID, 64, szAdapterID);
			pThis->m_bEnableHaccel = true;
			pThis->m_bD3dShared = true;
		}
		else
		{// ³¬³öÓ²½âÉèÖÃÊýÁ¿?½ûÓÃÓ²½â
			pThis->m_bEnableHaccel = false;
			pThis->m_bD3dShared = false;
		}
	}

#ifdef _WIN64
	shared_ptr<DxDeallocator> DxDeallocatorPtr = make_shared<DxDeallocator>(pThis->m_pDxSurface);
#else
	shared_ptr<DxDeallocator> DxDeallocatorPtr = make_shared<DxDeallocator>(pThis->m_pDxSurface, pThis->m_pDDraw);
	pThis->m_bD3dShared = pThis->m_bEnableDDraw ? false : pThis->m_bD3dShared;
#endif
	SaveRunTime();
	pThis->m_bD3dShared = pThis->m_bEnableDDraw ? false : pThis->m_bD3dShared;
	if (pThis->m_bD3dShared)
	{
		pDecodec->SetD3DShared(pThis->m_pDxSurface->GetD3D9(), pThis->m_pDxSurface->GetD3DDevice());
		pThis->m_pDxSurface->SetD3DShared(true);
	}

	// Ê¹ÓÃµ¥Ïß³Ì½âÂë,¶àÏß³Ì½âÂëÔÚÄ³´Ë±È½ÏÂýµÄCPUÉÏ¿ÉÄÜ»áÌá¸ßÐ§¹û£¬µ«ÏÖÔÚI5 2GHZÒÔÉÏµÄCPUÉÏµÄ¶àÏß³Ì½âÂëÐ§¹û²¢²»Ã÷ÏÔ·´¶ø»áÕ¼ÓÃ¸ü¶àµÄÄÚ´æ
	pDecodec->SetDecodeThreads(1);
	// ³õÊ¼»¯½âÂëÆ÷	
	while (pThis->m_bThreadDecodeRun)
	{// Ä³´ËÊ±ºò¿ÉÄÜ»áÒòÎªÄÚ´æ»ò×ÊÔ´²»¹»µ¼ÖÂ³õÊ¼»¯½âÂë²Ù×÷ÐÔ,Òò´Ë¿ÉÒÔÑÓ³ÙÒ»¶ÎÊ±¼äºóÔÙ´Î³õÊ¼»¯£¬Èô¶à´Î³õÊ¼»¯ÈÔ²»³É¹¦£¬ÔòÐèÍË³öÏß³Ì
		if (!pDecodec->InitDecoder(nCodecID, pThis->m_nVideoWidth, pThis->m_nVideoHeight, pThis->m_bEnableHaccel))
		{
			pThis->OutputMsg("%s Failed in Initializing Decoder:CodeCID =%d,Width = %d,Height = %d,HWAccel = %d.\n", __FUNCTION__, nCodecID, pThis->m_nVideoWidth, pThis->m_nVideoHeight, pThis->m_bEnableHaccel);
#ifdef _DEBUG
			pThis->OutputMsg("%s \tObject:%d Line %d Time = %d.\n", __FUNCTION__, pThis->m_nObjIndex, __LINE__, timeGetTime() - pThis->m_nLifeTime);
#endif
			nRetry++;
			if (nRetry >= 3)
			{
				if (pThis->m_hRenderWnd)// ÔÚÏß³ÌÖÐ¾¡Á¿±ÜÃâÊ¹ÓÃSendMessage£¬ÒòÎª¿ÉÄÜ»áµ¼ÖÂ×èÈû
					::PostMessage(pThis->m_hRenderWnd, WM_IPCPLAYER_MESSAGE, IPCPLAYER_INITDECODERFAILED, 0);
				return 0;
			}
			Delay(2500, pThis->m_bThreadDecodeRun);
		}
		else
			break;
	}
	SaveRunTime();
	if (!pThis->m_bThreadDecodeRun)
	{
		return 0;
	}

	if (pThis->m_pStreamProbe)
		pThis->m_pStreamProbe = nullptr;

	AVPacket *pAvPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
	shared_ptr<AVPacket>AvPacketPtr(pAvPacket, av_free);
	av_init_packet(pAvPacket);
	AVFrame *pAvFrame = av_frame_alloc();
	shared_ptr<AVFrame>AvFramePtr(pAvFrame, av_free);

	StreamFramePtr FramePtr;
	int nGot_picture = 0;
	DWORD nResult = 0;
	float fTimeSpan = 0;
	int nFrameInterval = pThis->m_nFileFrameInterval;
	pThis->m_dfTimesStart = GetExactTime();

	double dfDecodeStartTime = 0.0f;
	double dfRenderTime = GetExactTime() - pThis->m_fPlayInterval;	// Í¼Ïñ±»ÏÔÊ¾µÄÊ±¼ä
	double dfRenderStartTime = 0.0f;
	double dfRenderTimeSpan = 0.000f;
	double dfTimeSpan = 0.0f;

#ifdef _DEBUG
	pThis->m_csVideoCache.Lock();
	pThis->OutputMsg("%s Video cache Size = %d .\n", __FUNCTION__, pThis->m_listVideoCache.size());
	pThis->m_csVideoCache.Unlock();
	pThis->OutputMsg("%s \tObject:%d Start Decoding.\n", __FUNCTION__, pThis->m_nObjIndex);
#endif
	//	    ÒÔÏÂ´úÂëÓÃÒÔ²âÊÔ½âÂëºÍÏÔÊ¾Õ¼ÓÃÊ±¼ä£¬½¨Òé²»ÒªÉ¾³ý		
	// 		TimeTrace DecodeTimeTrace("DecodeTime", __FUNCTION__);
	// 		TimeTrace RenderTimeTrace("RenderTime", __FUNCTION__);

	int nIFrameTime = 0;
	CStat FrameStat(pThis->m_nObjIndex);		// ½âÂëÍ³¼Æ
	int nFramesAfterIFrame = 0;		// Ïà¶ÔIÖ¡µÄ±àºÅ,IÖ¡ºóµÄµÚÒ»Ö¡Îª1£¬µÚ¶þÖ¡Îª2ÒÀ´ËÀàÍÆ
	int nSkipFrames = 0;
	bool bDecodeSucceed = false;

	pThis->m_tFirstFrameTime = 0;
	float fLastPlayRate = pThis->m_fPlayRate;		// ¼ÇÂ¼ÉÏÒ»´ÎµÄ²¥·ÅËÙÂÊ£¬µ±²¥·ÅËÙÂÊ·¢Éú±ä»¯Ê±£¬ÐèÒªÖØÖÃÖ¡Í³¼ÆÊý¾Ý

	if (pThis->m_dwStartTime)
	{
		pThis->OutputMsg("%s %d Render Timespan = %d.\n", __FUNCTION__, __LINE__, timeGetTime() - pThis->m_dwStartTime);
		pThis->m_dwStartTime = 0;
	}

	if (!pThis->m_hEventFlushDecoder)
	{
		pThis->m_hEventFlushDecoder = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	int nFramesPlayed = 0;			// ²¥·Å×Ü·«Êý
	double dfTimeStartPlay = GetExactTime();// ²¥·ÅÆðÊ¼Ê±¼ä
	int nTimePlayFrame = 0;		// ²¥·ÅÒ»Ö¡ËùºÄ·ÑÊ±¼ä£¨MS£©
	int nPlayCount = 0;
	int TPlayArray[100] = { 0 };
	double dfT1 = GetExactTime();
	int nVideoCacheSize = 0;
	LONG nTotalDecodeFrames = 0;
	dfDecodeStartTime = GetExactTime() - pThis->m_nPlayFrameInterval / 1000.0f;
	SaveRunTime();
	pThis->m_pDecoder = pDecodec;
	int nRenderTimes = 0;

	double dfTimeDecodeStart = 0.0f;
	int dfDecodeTimeSpan = 0;
	int nAvError = 0;
	char szAvError[1024] = { 0 };

#ifdef _DEBUG
	CStat  RenderInterval("RenderInterval", pThis->m_nObjIndex);
	CStat  CacheStat("VideoCache Size", pThis->m_nObjIndex);
	pThis->OutputMsg("%s Timespan2 of First Frame = %f.\n", __FUNCTION__, TimeSpanEx(pThis->m_dfFirstFrameTime));
#endif
	shared_ptr<CMMEvent> pRenderTimer = make_shared<CMMEvent>(pThis->m_hRenderAsyncEvent, nIPCPlayInterval);
	SetEvent(pThis->m_hRenderAsyncEvent);				// Á¢¼´¿ªÊ¼äÖÈ¾»­Ãæ,ÏÂÒ»¸öÊÂ¼þÒªµ½ nIPCPlayInterval msÖ®ºó²Å»á·¢Éú

	while (pThis->m_bThreadDecodeRun)
	{
		pThis->m_csVideoCache.Lock();
		nVideoCacheSize = pThis->m_listVideoCache.size();
		pThis->m_csVideoCache.Unlock();
		if (WaitForSingleObject(pThis->m_hEventFlushDecoder, 0) == WAIT_OBJECT_0)
		{// Çå¿Õ½âÂëÆ÷»º´æ
			pDecodec->FlushDecodeBuffer();
			continue;
		}

		if (nAvError >= 0)	// ÉÏÒ»Ö¡½âÂë³É¹¦£¿
		{
			int nWaitTime = nIPCPlayInterval;
			if (nVideoCacheSize > 5)
			{
				int nMult = (nVideoCacheSize - 5)*5;
				if (nMult < (nIPCPlayInterval - 5))
				{
					nWaitTime = nIPCPlayInterval - nMult;
					if (pRenderTimer->nPeriod != nWaitTime)	// ²¥·Å¼ä¸ô½µµÍ,¿ÉÒÔÑ¸ËÙÇå¿Õ»ýÀÛÖ¡
						pRenderTimer->UpdateInterval(nWaitTime);
				}
				else
				{
					nWaitTime = 10;
					pRenderTimer->UpdateInterval(10);
				}
					
			}
			else if (pRenderTimer->nPeriod != nIPCPlayInterval)
			{
				nWaitTime = nIPCPlayInterval;
				pRenderTimer->UpdateInterval(nIPCPlayInterval);
			}
#ifdef _DEBUG
			if (WaitForSingleObject(pThis->m_hRenderAsyncEvent, nWaitTime) == WAIT_TIMEOUT)
			{
				TraceMsgA("%s Wait time out.\n", __FUNCTION__);
			}
#else
			WaitForSingleObject(pThis->m_hRenderAsyncEvent, nWaitTime);
#endif
			
#ifdef _DEBUG
		if (dfDecodeStartTime != 0.0f)
			RenderInterval.Stat(TimeSpanEx(dfDecodeStartTime));
		if (RenderInterval.IsFull())
		{
			RenderInterval.OutputStat();
			RenderInterval.Reset();
		}
		dfDecodeStartTime = GetExactTime();
#endif
		}
#ifdef _DEBUG
		CacheStat.Stat(nVideoCacheSize);
		if (CacheStat.IsFull())
		{
			CacheStat.OutputStat();
			CacheStat.Reset();
		}
#endif
		
		bool bPopFrame = false;
		AutoLock(pThis->m_csVideoCache.Get());
		if (pThis->m_listVideoCache.size() > 0)
		{
			FramePtr = pThis->m_listVideoCache.front();
			pThis->m_listVideoCache.pop_front();
			bPopFrame = true;
			nVideoCacheSize = pThis->m_listVideoCache.size();
		}
		else
		{
			Lock.Unlock();
			if (WaitForSingleObject(pThis->m_hInputFrameEvent, 20) == WAIT_TIMEOUT)
				continue;
		}
		
		pAvPacket->data = (uint8_t *)FramePtr->Framedata(pThis->m_nSDKVersion);
		pAvPacket->size = FramePtr->FrameHeader()->nLength;
		pThis->m_tLastFrameTime = FramePtr->FrameHeader()->nTimestamp;
		av_frame_unref(pAvFrame);

		dfDecodeStartTime = GetExactTime();
		// ½âÂëIÖ¡¿ÉÄÜÐèÒª20ms×óÓÒµÄÊ±¼ä£¬¶ø½âPÖ¡»òBÖ¡¿ÉÄÜÖ»ÐèÒª²»µ½5msµÄÊ±¼ä
		nAvError = pDecodec->Decode(pAvFrame, nGot_picture, pAvPacket);
		nTotalDecodeFrames++;
		av_packet_unref(pAvPacket);
		if (nAvError < 0)
		{
			av_frame_unref(pAvFrame);
			av_strerror(nAvError, szAvError, 1024);
			continue;
		}
		dfDecodeTimeSpan = TimeSpanEx(dfDecodeStartTime);
		if (nGot_picture)
		{
			pThis->m_nDecodePixelFmt = (AVPixelFormat)pAvFrame->format;
			SetEvent(pThis->m_hEvnetYUVReady);
			SetEvent(pThis->m_hEventDecodeStart);
			pThis->m_nCurVideoFrame = FramePtr->FrameHeader()->nFrameID;
			pThis->m_tCurFrameTimeStamp = FramePtr->FrameHeader()->nTimestamp;
			pThis->ProcessYUVFilter(pAvFrame, (LONGLONG)pThis->m_nCurVideoFrame);
			if (!pThis->m_bIpcStream &&
				1.0f == pThis->m_fPlayRate  &&
				pThis->m_bEnableAudio &&
				pThis->m_hAudioFrameEvent[0] &&
				pThis->m_hAudioFrameEvent[1])
			{
				if (pThis->m_nDecodeDelay == -1)
					WaitForMultipleObjects(2, pThis->m_hAudioFrameEvent, TRUE, 40);
				else if (!pThis->m_nDecodeDelay)
					WaitForMultipleObjects(2, pThis->m_hAudioFrameEvent, TRUE, pThis->m_nDecodeDelay);
			}
			dfRenderStartTime = GetExactTime();
			pThis->RenderFrame(pAvFrame);
			
			pThis->ProcessSnapshotRequire(pAvFrame);
			pThis->ProcessYUVCapture(pAvFrame, (LONGLONG)pThis->m_nCurVideoFrame);
			AutoLock(&pThis->m_csFilePlayCallBack);
			if (pThis->m_pFilePlayCallBack)
				pThis->m_pFilePlayCallBack(pThis, pThis->m_pUserFilePlayer);
		}
		else
		{
			pThis->OutputMsg("%s \tObject:%d Decode Succeed but Not get a picture ,FrameType = %d\tFrameLength %d.\n", __FUNCTION__, pThis->m_nObjIndex, FramePtr->FrameHeader()->nType, FramePtr->FrameHeader()->nLength);
		}

		dfRenderTimeSpan = TimeSpanEx(dfRenderStartTime);
		nTimePlayFrame = (int)(TimeSpanEx(dfDecodeStartTime) * 1000);
		dfRenderTime = GetExactTime();

	}
	pThis->OutputMsg("%s Object %d Decode Frames:%d.\n", __FUNCTION__, pThis->m_nObjIndex, nTotalDecodeFrames);
	av_frame_unref(pAvFrame);
	SaveRunTime();
	pThis->m_pDecoder = nullptr;
	CloseHandle(pThis->m_hEventFlushDecoder);
	pThis->m_hEventFlushDecoder = nullptr;
	return 0;
}

UINT __stdcall CIPCPlayer::ThreadAsyncRender(void *p)
{
	CIPCPlayer *pThis = (CIPCPlayer*)p;

	if (!pThis->m_hRenderWnd)
		pThis->OutputMsg("%s Warning!!!A Windows handle is Needed otherwise the video Will not showed..\n", __FUNCTION__);

	int nIPCPlayInterval = 1000 / pThis->m_nVideoFPS;
	
	if (!pThis->InitizlizeDx())
	{
		assert(false);
		return 0;
	}
	shared_ptr<CMMEvent> pRenderTimer = make_shared<CMMEvent>(pThis->m_hRenderAsyncEvent, nIPCPlayInterval);
	// Á¢¼´¿ªÊ¼äÖÈ¾»­Ãæ
	SetEvent(pThis->m_hRenderAsyncEvent);
#ifdef _WIN64
	shared_ptr<DxDeallocator> DxDeallocatorPtr = make_shared<DxDeallocator>(pThis->m_pDxSurface);
#else
	shared_ptr<DxDeallocator> DxDeallocatorPtr = make_shared<DxDeallocator>(pThis->m_pDxSurface, pThis->m_pDDraw);
	pThis->m_bD3dShared = pThis->m_bEnableDDraw ? false : pThis->m_bD3dShared;
#endif

	int nFameListSize = 0;
	int nRenderFrames = 0;
	CAVFramePtr pFrame;
	while (pThis->m_bThreadDecodeRun)
	{
		if (WaitForSingleObject(pThis->m_hRenderAsyncEvent, nIPCPlayInterval*2) == WAIT_TIMEOUT)
			continue;
		CAutoLock lock(pThis->m_cslistAVFrame.Get(), false, __FILE__, __FUNCTION__, __LINE__);
		nFameListSize = pThis->m_listAVFrame.size();
		if (nFameListSize < 1)
		{
			lock.Unlock();
			Sleep(20);
			continue;
		}
		lock.Unlock();
		if (pThis->m_tSyncTimeBase)
		{
			do 
			{
				CAutoLock lock(pThis->m_cslistAVFrame.Get());
				pFrame = pThis->m_listAVFrame.front();
				time_t nTimeSpan = pFrame->tFrame - pThis->m_tSyncTimeBase;
				if (nTimeSpan >= 60)	 // ÒÑ¾­Ô½¹ýÊ±¼äÖá»ùÏß
				{
					lock.Unlock();
					Sleep(20);
					continue;
				}
				else if (nTimeSpan <= -60)	// ÂäºóÊ±¼äÖá»ùÏßÌ«Ô¶
				{
					pThis->m_listAVFrame.pop_front();
					if (pThis->m_listAVFrame.size() < 1)
						break;
					else
						continue;
				}
			} while (pThis->m_bThreadDecodeRun);
		}
		else
		{
			CAutoLock lock(pThis->m_cslistAVFrame.Get());
			pFrame = pThis->m_listAVFrame.front();
			//pThis->m_listAVFrame.pop_front();
		}
		
		if (!pFrame)
			continue;
		
		CAVFramePtr pFrame = pThis->m_listAVFrame.front();
		pThis->m_listAVFrame.pop_front();
		lock.Unlock();
		pThis->RenderFrame(pFrame->pFrame);
		nRenderFrames++;
		if (nRenderFrames % 25 == 0)
			pThis->OutputMsg("%s nRenderFrames = %d.\n", __FUNCTION__, nRenderFrames);
		pFrame = nullptr;
	}
	return 0;
}

UINT __stdcall CIPCPlayer::ThreadSyncDecode(void *p)
{
	CIPCPlayer *pThis = (CIPCPlayer*)p;
	DeclareRunTime(5);
#ifdef _DEBUG
	pThis->OutputMsg("%s \tObject:%d  m_nLifeTime = %d.\n", __FUNCTION__, pThis->m_nObjIndex, timeGetTime() - pThis->m_nLifeTime);
	if (pThis->m_pSyncPlayer)
		pThis->OutputMsg("%s Salve Player Enter ThreadSyncDecode Time:%.3f.\n", __FUNCTION__, GetExactTime());
	else
		pThis->OutputMsg("%s Primary Player Enter ThreadSyncDecode Time:%.3f.\n", __FUNCTION__, GetExactTime());
#endif
	int nAvError = 0;
	char szAvError[1024] = { 0 };
	
	// µÈ´ýÓÐÐ§µÄÊÓÆµÖ¡Êý¾Ý
	long tFirst = timeGetTime();
// 	int nTimeoutCount = 0;
// 	while (pThis->m_bThreadDecodeRun)
// 	{
// 		Autolock(&pThis->m_csVideoCache);
// 		if (pThis->m_listVideoCache.size() < 1)
// 		{
// 			lock.Unlock();
// 			Sleep(20);
// 			continue;
// 		}
// 		else
// 			break;
// 	}
	SaveRunTime();
	if (!pThis->m_bThreadDecodeRun)
	{
		return 0;
	}

	// µÈ´ýIÖ¡
	tFirst = timeGetTime();
	//		DWORD dfTimeout = 3000;
	// 		if (!pThis->m_bIpcStream)	// Ö»ÓÐIPCÂëÁ÷²ÅÐèÒª³¤Ê±¼äµÈ´ý
	// 			dfTimeout = 1000;
	AVCodecID nCodecID = AV_CODEC_ID_NONE;
	int nDiscardFrames = 0;

	SaveRunTime();
	switch (pThis->m_nVideoCodec)
	{
	case CODEC_H264:
		nCodecID = AV_CODEC_ID_H264;
		break;
	case CODEC_H265:
		nCodecID = AV_CODEC_ID_H265;
		break;
	default:
	{
		pThis->OutputMsg("%s You Input a unknown stream,Decode thread exit.\n", __FUNCTION__);
		if (pThis->m_hRenderWnd)	// ÔÚÏß³ÌÖÐ¾¡Á¿±ÜÃâÊ¹ÓÃSendMessage£¬ÒòÎª¿ÉÄÜ»áµ¼ÖÂ×èÈû
			::PostMessage(pThis->m_hRenderWnd, WM_IPCPLAYER_MESSAGE, IPCPLAYER_UNSURPPORTEDSTREAM, 0);
		assert(false);
		return 0;
		break;
	}
	}

	int nRetry = 0;

	shared_ptr<CVideoDecoder>pDecodec = make_shared<CVideoDecoder>();
	if (!pDecodec)
	{
		pThis->OutputMsg("%s Failed in allocing memory for Decoder.\n", __FUNCTION__);
		assert(false);
		return 0;
	}
	SaveRunTime();

	if (!pThis->m_hRenderWnd)
		pThis->OutputMsg("%s Warning!!!A Windows handle is Needed otherwise the video Will not showed..\n", __FUNCTION__);

	int nIPCPlayInterval = 1000 / pThis->m_nVideoFPS;
// 	pThis->m_bEnableHaccel = true;
// 	pThis->m_bD3dShared = true;
	if (!pThis->m_bAsyncRender && 
		!pThis->InitizlizeDx())
	{
		assert(false);
		return 0;
	}
	shared_ptr<DxDeallocator> DxDeallocatorPtr;
	if (!pThis->m_bAsyncRender)
#ifdef _WIN64
		DxDeallocatorPtr = make_shared<DxDeallocator>(pThis->m_pDxSurface);
#else
	{
		DxDeallocatorPtr = make_shared<DxDeallocator>(pThis->m_pDxSurface, pThis->m_pDDraw);
		pThis->m_bD3dShared = pThis->m_bEnableDDraw ? false : pThis->m_bD3dShared;
	}
#endif
	pDecodec->SetD3DShared(pThis->m_pDxSurface->GetD3D9(), pThis->m_pDxSurface->GetD3DDevice());
	pThis->m_pDxSurface->SetD3DShared(true);
	
	// Ê¹ÓÃµ¥Ïß³Ì½âÂë,¶àÏß³Ì½âÂëÔÚÄ³´Ë±È½ÏÂýµÄCPUÉÏ¿ÉÄÜ»áÌá¸ßÐ§¹û£¬µ«ÏÖÔÚI5 2GHZÒÔÉÏµÄCPUÉÏµÄ¶àÏß³Ì½âÂëÐ§¹û²¢²»Ã÷ÏÔ·´¶ø»áÕ¼ÓÃ¸ü¶àµÄÄÚ´æ
	pDecodec->SetDecodeThreads(1);
	// ³õÊ¼»¯½âÂëÆ÷

	while (pThis->m_bThreadDecodeRun)
	{// Ä³´ËÊ±ºò¿ÉÄÜ»áÒòÎªÄÚ´æ»ò×ÊÔ´²»¹»µ¼ÖÂ³õÊ¼»¯½âÂë²Ù×÷ÐÔ,Òò´Ë¿ÉÒÔÑÓ³ÙÒ»¶ÎÊ±¼äºóÔÙ´Î³õÊ¼»¯£¬Èô¶à´Î³õÊ¼»¯ÈÔ²»³É¹¦£¬ÔòÐèÍË³öÏß³Ì
		//DeclareRunTime(5);
		//SaveRunTime();
		if (!pDecodec->InitDecoder(nCodecID, pThis->m_nVideoWidth, pThis->m_nVideoHeight, true))
		{
			pThis->OutputMsg("%s Failed in Initializing Decoder:CodeCID =%d,Width = %d,Height = %d,HWAccel = %d.\n", __FUNCTION__, nCodecID, pThis->m_nVideoWidth, pThis->m_nVideoHeight, pThis->m_bEnableHaccel);
#ifdef _DEBUG
			pThis->OutputMsg("%s \tObject:%d Line %d Time = %d.\n", __FUNCTION__, pThis->m_nObjIndex, __LINE__, timeGetTime() - pThis->m_nLifeTime);
#endif
			return 0;
		}
		else
			break;
		//SaveRunTime();
	}
	SaveRunTime();
	if (!pThis->m_bThreadDecodeRun)
	{
		//assert(false);
		return 0;
	}

	AVPacket *pAvPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
	shared_ptr<AVPacket>AvPacketPtr(pAvPacket, av_free);
	av_init_packet(pAvPacket);
	AVFrame *pAvFrame = av_frame_alloc();
	shared_ptr<AVFrame>AvFramePtr(pAvFrame, av_free);

	StreamFramePtr FramePtr;
	int nGot_picture = 0;
	DWORD nResult = 0;
	float fTimeSpan = 0;
	int nFrameInterval = pThis->m_nFileFrameInterval;
	pThis->m_dfTimesStart = GetExactTime();

	double dfDecodeStartTime = GetExactTime();
	double dfRenderTime = GetExactTime() - pThis->m_fPlayInterval;	// Í¼Ïñ±»ÏÔÊ¾µÄÊ±¼ä
	double dfRenderStartTime = 0.0f;
	double dfRenderTimeSpan = 0.000f;
	double dfTimeSpan = 0.0f;

// #ifdef _DEBUG
// 	pThis->m_csVideoCache.Lock();
// 	TraceMsgA("%s Video cache Size = %d .\n", __FUNCTION__, pThis->m_listVideoCache.size());
// 	pThis->m_csVideoCache.Unlock();
// 	pThis->OutputMsg("%s \tObject:%d Start Decoding.\n", __FUNCTION__, pThis->m_nObjIndex);
// #endif

	int nIFrameTime = 0;
	bool bDecodeSucceed = false;
	pThis->m_tFirstFrameTime = 0;
	float fLastPlayRate = pThis->m_fPlayRate;		// ¼ÇÂ¼ÉÏÒ»´ÎµÄ²¥·ÅËÙÂÊ£¬µ±²¥·ÅËÙÂÊ·¢Éú±ä»¯Ê±£¬ÐèÒªÖØÖÃÖ¡Í³¼ÆÊý¾Ý

	if (pThis->m_dwStartTime)
	{
		pThis->OutputMsg("%s %d Render Timespan = %d.\n", __FUNCTION__, __LINE__, timeGetTime() - pThis->m_dwStartTime);
		pThis->m_dwStartTime = 0;
	}

	int nFramesPlayed = 0;			// ²¥·Å×Ü·«Êý
	double dfTimeStartPlay = GetExactTime();// ²¥·ÅÆðÊ¼Ê±¼ä
	int nTimePlayFrame = 0;		// ²¥·ÅÒ»Ö¡ËùºÄ·ÑÊ±¼ä£¨MS£©
	int nPlayCount = 0;
	int TPlayArray[100] = { 0 };
	double dfT1 = GetExactTime();
	int nVideoCacheSize = 0;
	LONG nTotalDecodeFrames = 0;
	dfDecodeStartTime = GetExactTime() - pThis->m_nPlayFrameInterval / 1000.0f;
	SaveRunTime();
	pThis->m_pDecoder = pDecodec;
	int nRenderTimes = 0;
	CStat  RenderInterval("RenderInterval", pThis->m_nObjIndex);
	CStat  WaitStat("WaitStat", pThis->m_nObjIndex);
	DH_FRAME_INFO *pDHFrame = nullptr;
	double dfLastRenderTime = 0.0f;
	int nMaxFrameCache = pThis->m_nVideoFPS;
	if (pThis->m_bPlayOneFrame)
		nMaxFrameCache *= 4;
	
	while (pThis->m_bThreadDecodeRun)
	{
		pThis->m_csVideoCache.Lock();
		nVideoCacheSize = pThis->m_listVideoCache.size();
		pThis->m_csVideoCache.Unlock();

		if (nVideoCacheSize < 1)
		{
			Sleep(20);
			continue;
		}
		//TraceMsgA("%s ´ý½âÂëÖ¡ÊýÁ¿:%d.\n", __FUNCTION__,nVideoCacheSize);
		//////////////////////////////////////////////////////////////////////////
		if (pThis->m_pSyncPlayer)///ÓÐÍ¬²½²¥·ÅÆ÷£¿
		{
			int nWaitCount = 0;
			while (pThis->m_bThreadDecodeRun)
			{
				CAutoLock lock(pThis->m_csVideoCache.Get());
				FramePtr = pThis->m_listVideoCache.front();
				time_t nTimeSpan = FramePtr->FrameHeader()->nTimestamp - pThis->m_pSyncPlayer->m_tSyncTimeBase;
				if (nTimeSpan >= 20)	 // ÒÑ¾­Ô½¹ýÊ±¼äÖá»ùÏß
				{
					lock.Unlock();			
					nWaitCount++;
					Sleep(20);
					continue;
				}
				// ´Ó²¥·ÅÆ÷ÍêÈ«ÒÀÀµÖ÷²¥·ÅÆ÷µÄÊ±¼äÏß£¬ÒòÎªÃ»ÓÐ´ý¹ý³Ì£¬Òò´Ë²¥·ÅËÙ¶È»á±È½ÏÖ÷²¥·ÅÆ÷¿ì£¬Ö»»áÊÇµÈ´ýÖ÷²¥·ÅÆ÷£¬²»¿ÉÄÜ±ÈÖ÷²¥·ÅÆ÷Âý
// 				else if (nTimeSpan <= -40)	// ÂäºóÊ±¼äÖá»ùÏßÌ«Ô¶
// 				{
// 					if (!StreamFrame::IsIFrame(FramePtr))
// 					{
// 						pThis->m_listVideoCache.pop_front();
// 						if (pThis->m_listVideoCache.size() < 1)
// 							break;
// 						continue;
// 					}
// 					else
// 					{
// 						pThis->m_listVideoCache.pop_front();
// 						break;
// 					}
// 				}
				pThis->m_listVideoCache.pop_front();
				break;
			}
// 			WaitStat.Stat(nWaitCount);
// 			if (WaitStat.IsFull())
// 			{
// 				WaitStat.OutputStat();
// 				WaitStat.Reset();
// 			}
			if (!FramePtr)
			{
				Sleep(20);
				continue;
			}
				
		}
		else
		{
			CAutoLock lock(pThis->m_csVideoCache.Get());
			if (!pThis->m_bAsyncRender)	// ·ÇÒì²½½âÂë²ÅÐèÒªµÈ´ýäÖÈ¾ÊÂ¼þ
				if (WaitForSingleObject(pThis->m_hRenderAsyncEvent, INFINITE) == WAIT_TIMEOUT)
					continue;
			
			//TraceMsgA("%s µ¯³öÒ»Ö¡.\n", __FUNCTION__);
			FramePtr = pThis->m_listVideoCache.front();
			pThis->m_listVideoCache.pop_front();
		}
		//////////////////////////////////////////////////////////////////////////
		if (!pThis->m_tFirstFrameTime )
			pThis->m_tFirstFrameTime = FramePtr->FrameHeader()->nTimestamp;

		pAvPacket->data = (uint8_t *)FramePtr->Framedata(pThis->m_nSDKVersion);
		pAvPacket->size = FramePtr->FrameHeader()->nLength;
		pThis->m_tLastFrameTime = FramePtr->FrameHeader()->nTimestamp;
		av_frame_unref(pAvFrame);

		// ·ÇÒì²½äÖÈ¾ºÍÎÞÍ¬²½²¥·ÅÆ÷µÄÇé¿öÏÂ£¬²Å¿ÉÔÚ½âÂëÏß³ÌÖÐ¸Ä±äÍ¬²½»ù×¼Ê±ÖÓ
		if (!pThis->m_bAsyncRender && !pThis->m_pSyncPlayer)
			pThis->m_tSyncTimeBase = FramePtr->FrameHeader()->nTimestamp;
		
		int nFrameSize = pAvPacket->size;
		nAvError = pDecodec->Decode(pAvFrame, nGot_picture, pAvPacket);
		time_t tFrameTimestamp = FramePtr->FrameHeader()->nTimestamp;
		FramePtr = nullptr;

		nTotalDecodeFrames++;
		av_packet_unref(pAvPacket);
		if (nAvError < 0)
		{
			av_strerror(nAvError, szAvError, 1024);
			pThis->OutputMsg("%s %s\tFrameSize = %d.\n", __FUNCTION__, szAvError,nFrameSize);
			continue;
		}

		if (!nGot_picture)
			continue;
		
		pThis->m_nDecodePixelFmt = (AVPixelFormat)pAvFrame->format;
		if (pThis->m_bAsyncRender)
		{
			do
			{
				CAutoLock lock(pThis->m_cslistAVFrame.Get());
				if (pThis->m_listAVFrame.size() >= nMaxFrameCache)
				{
					lock.Unlock();
					Sleep(10);
					continue;
				}
				else
					break;
			} while (pThis->m_bThreadDecodeRun);
			CAutoLock lock(pThis->m_cslistAVFrame.Get());
			pThis->m_listAVFrame.push_back(make_shared<CAvFrame>(pAvFrame, tFrameTimestamp));
		}
		else
			pThis->RenderFrame(pAvFrame);
		if (dfLastRenderTime > 0.0f)
		{
// 			RenderInterval.Stat(TimeSpanEx(dfLastRenderTime));
// 			dfLastRenderTime = GetExactTime();
// 			if (RenderInterval.IsFull())
// 			{
// 				RenderInterval.OutputStat();
// 				RenderInterval.Reset();
// 			}
		}
		else
			dfLastRenderTime = GetExactTime();
	}
	av_frame_unref(pAvFrame);
	SaveRunTime();
	pThis->m_pDecoder = nullptr;
	
	return 0;
}

// UINT CIPCPlayer::ReversePlayRun()
// {
// 	while (true)
// 	{
// 
// 	}
// 
// }

/// @brief			ÆôÓÃÄæÏò²¥·Å
/// @remark			ÄæÏò²¥·ÅµÄÔ­ÀíÊÇÏÈ¸ßËÙ½âÂë£¬°ÑÍ¼Ïñ·ÅÈëÏÈÈëÏÈ³ö¶ÓÁÐµÄ»º´æ½øÐÐ²¥·Å£¬µ±ÐèÒªÄæÏò²¥·Å·Å£¬Ôò´Ó»º´æÎ²²¿ÏòÍ·²¿²¥·Å£¬ÐÎ³ÉÄæÏòÐ§¹û
/// @param [in]		bFlag			ÊÇ·ñÆôÓÃÄæÏò²¥·Å£¬ÎªtrueÊ±ÔòÆôÓÃ£¬ÎªfalseÊ±£¬Ôò¹Ø±ÕÍ¬Ê±Í¬¿Õ»º´æ
/*
void CIPCPlayer::EnableReversePlay(bool bFlag)
{
	if (bFlag)
	{// ´´½¨Òì²½äÖÈ¾Ïß³Ì
		m_bAsyncThreadRenderRun = true;
		m_hThreadAsyncReander = (HANDLE)_beginthreadex(nullptr, 128, ThreadAsyncRender, this, 0, 0);
	}
	else
	{// ¹Ø±ÕÒì²½äÖÈ¾Ïß³Ì
		if (m_hRenderAsyncEvent)
			SetEvent(m_hRenderAsyncEvent);
		m_bAsyncThreadRenderRun = false;
		if (m_hThreadAsyncReander)
		{
			WaitForSingleObject(m_hThreadAsyncReander, INFINITE);
			CloseHandle(m_hThreadAsyncReander);
			m_hThreadAsyncReander = nullptr;
		}
	}
	EnterCriticalSection(m_cslistAVFrame.Get());
	m_nListAvFrameMaxSize = nCacheFrames;
	m_listAVFrame.clear();
	LeaveCriticalSection(m_cslistAVFrame.Get());
}
*/

void CIPCPlayer::ProcessSwitchEvent()
{
	//m_csMapSwitcher.Lock();
	for (auto it = m_MapSwitcher.begin(); it != m_MapSwitcher.end(); it++)
	{
		it->second->cs.Lock();
		if (it->second->list.size() > 1)
		{
			it->second->list.pop_front();
		}
		it->second->cs.Unlock();
	}
	//m_csMapSwitcher.Unlock();
}