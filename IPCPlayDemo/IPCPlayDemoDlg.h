
// IPCPlayDemoDlg.h : 头文件
//

#pragma once

#include "VideoFrame.h"
#include "AutoLock.h"

#ifdef _STD_SMARTPTR
#include <memory>
using namespace std;
using namespace std::tr1;
#else
#include <boost/smart_ptr.hpp>
using namespace boost;
#endif

#include "GlliteryStatic.h"
#include "fullscreen.h"
#include "SocketClient.h"
//#include "DirectDraw.h"
#include "../ipcnetsdk/ipcMsgHead.h"
#include "./DisplayYUV/YuvFrame.h"
#include "DialogDisplayRGB24.h"
#include "TransparentDlg.h"

using namespace std;

struct MSG_HEAD
{
	U8  Magic1[4];  // 	四字节 固定内容（0xF5，0x5A，0xA5，0x5F）
	U32	Pktlen;		//	类型U32 数据包总长度。
	U16	Version;	//  类型U16 协议版本号，高8位为主版本号，低8位为子版本号。
	U16	Hwtype;		//	类型U16 硬件类型(保留)
	U32	Sn;		    //  类型U32 帧序列号(保留)
	U16	CmdType;	//	类型U16 表示命令类型
	U16 CmdSubType;	//	类型U16 表示命令子类型
	U16 DataType;	//  类型U16 数据包类型1：命令包，2：视频包，3：音频包
	U16 Rev1;		//	类型U16 保留字段
	U32 Rev2;		//	类型U32 保留字段
	U32 Rev3;		//	类型U32 保留字段
};

#define _MaxTimeCount	125
struct TimeTrace
{
	char szName[128];
	char szFunction[128];
	double dfTimeArray[100];
	int	   nTimeCount;
	double dfInputTime;
	TimeTrace(char *szNameT, char *szFunctionT)
	{
		ZeroMemory(this, sizeof(TimeTrace));
		strcpy(szName, szNameT);
		strcpy(szFunction, szFunctionT);
	}
	void Zero()
	{
		ZeroMemory(this, sizeof(TimeTrace));
	}
	inline bool IsFull()
	{
		return (nTimeCount >= _MaxTimeCount);
	}
	inline void SaveTime()
	{
		if (dfInputTime != 0.0f)
			dfTimeArray[nTimeCount++] = GetExactTime() - dfInputTime;
		dfInputTime = GetExactTime();
	}
	inline void SaveTime(double dfTimeSpan)
	{
		dfTimeArray[nTimeCount++] = dfTimeSpan;
	}
	void OutputTime(float fMaxTime = 0.0f, bool bReset = true)
	{
		char szOutputText[1024] = { 0 };
		double dfAvg = 0.0f;
		int nCount = 0;
		if (nTimeCount < 1)
			return;
		TraceMsgA("%s %s Interval:\n", szFunction, szName);
		for (int i = 0; i < nTimeCount; i++)
		{
			if (fMaxTime > 0)
			{
				if (dfTimeArray[i] >= fMaxTime)
				{
					sprintf(&szOutputText[strlen(szOutputText)], "%.3f\t", dfTimeArray[i]);
					nCount++;
				}
			}
			else
			{
				sprintf(&szOutputText[strlen(szOutputText)], "%.3f\t", dfTimeArray[i]);
				nCount++;
			}

			dfAvg += dfTimeArray[i];
			if ((nCount + 1) % 25 == 0)
			{
				TraceMsgA("%s %s\n", szFunction, szOutputText);
				ZeroMemory(szOutputText, 1024);
			}
		}
		dfAvg /= nTimeCount;
		//if (dfAvg >= fMaxTime)
		TraceMsgA("%s Avg %s = %.6f.\n", szFunction, szName, dfAvg / nTimeCount);
		if (bReset)
			nTimeCount = 0;
	}
};

#define WM_UPDATE_STREAMINFO	(WM_USER + 1024)
#define WM_UPDATE_PLAYINFO		(WM_USER + 1025)
#define WM_UPDATEYUV			(WM_USER + 1026)

#define _Frame_PERIOD			30.0f		///< 一个帧率区间
struct FrameInfo
{
	time_t  tRecvTime;				///< 收到帧的时间 单位毫秒
	int		nFrameSize;
};

/// @brief 解析IPC私有帧结构体
struct FrameParser
{
	IPCFrameHeaderEx	*pHeaderEx;		///< IPC私有录像的帧数据
	UINT				nFrameSize;		///< pFrame的数据长度
	byte				*pLawFrame;		///< 原始码流数据
	UINT				nLawFrameSize;	///< 原始码流数据长度
};

struct StreamInfo
{
	UINT	nFrameID;
	bool	bRecvIFrame;		///< 是否收到第个I帧
	int		nVideoFrameCount;
	int		nAudioFrameCount;
	__int64 nVideoBytes;		///< 收到视频总字节数
	__int64 nAudioBytes;		///< 收到音频总字节数
	time_t  tFirstTime;			///< 接收数据的起始时间	单位毫秒
	time_t	tLastTime;			///< 最后一次接收数据时间 单位毫秒
	CRITICAL_SECTION	csFrameList;
	list<FrameInfo>FrameInfoList;
	StreamInfo()
	{
		ZeroMemory(this, offsetof(StreamInfo, FrameInfoList));
		::InitializeCriticalSection(&csFrameList);
	}
	~StreamInfo()
	{
		::DeleteCriticalSection(&csFrameList);
	}

	int GetFrameRate()
	{
		CAutoLock lock(&csFrameList);
		if (FrameInfoList.size() > 2)
		{
			time_t tSpan = FrameInfoList.back().tRecvTime - FrameInfoList.front().tRecvTime;
			if (!tSpan)
				tSpan = 1000;
			int nSize = FrameInfoList.size();
			return nSize * 1000 / (int)(tSpan);
		}
		else
			return 1;
	}

	int	GetVideoCodeRate(int nUnit = 1024)	///< 取得视频码率(Kbps)
	{
		time_t tSpan = 25;
		CAutoLock lock(&csFrameList);
		if (FrameInfoList.size() > 1)
			tSpan = FrameInfoList.back().tRecvTime - FrameInfoList.front().tRecvTime;
		__int64 nTotalBytes = 0;

		for (list<FrameInfo>::iterator it = FrameInfoList.begin();
			it != FrameInfoList.end();
			it++)
			nTotalBytes += (*it).nFrameSize;

		if (tSpan == 0)
			tSpan = 1;
		__int64 nRate = (nTotalBytes * 1000 * 8 / ((int)tSpan*nUnit));
		return (int)nRate;
	}
	int GetAudioCodeRate(int nUnit = 1024)
	{
		time_t tSpan = (tLastTime - tFirstTime);
		if (tSpan == 0)
			tSpan = 1;
		return (int)(nAudioBytes * 1000 / (tSpan*nUnit));
	}
	void inline Reset()
	{
		ZeroMemory(this, offsetof(StreamInfo, csFrameList));
		CAutoLock lock(&csFrameList);
		FrameInfoList.clear();
	}
	void PushFrameInfo(int nFrameSize)
	{
		time_t tNow = (time_t)(GetExactTime() * 1000);
		CAutoLock lock(&csFrameList);
		if (FrameInfoList.size() > 1)
		{
			for (list<FrameInfo>::iterator it = FrameInfoList.begin();
				it != FrameInfoList.end();)
			{
				if ((tNow - (*it).tRecvTime) > _Frame_PERIOD * 1000)
					it = FrameInfoList.erase(it);
				else
					it++;
			}
		}

		FrameInfo fi;
		fi.tRecvTime = tNow;
		fi.nFrameSize = nFrameSize;
		FrameInfoList.push_back(fi);
	}
};

struct PlayerContext
{
public:
	StreamInfo*	pStreamInfo;
	USER_HANDLE hUser;
	int nPlayerCount;
	bool	bHisiliconFrame;
	
	IPC_PLAYHANDLE	hPlayer[36];
	IPC_PLAYHANDLE	hPlayerStream;		// 流播放句柄
	TimeTrace		*m_pInputStreamTimeTrace ;
	double			m_dfLastInputstream ;
	REAL_HANDLE hStream;
	CSocketClient *pClient;
	volatile bool bThreadRecvIPCStream/* = false*/;
	HANDLE hThreadRecvStream/* = NULL*/;
	fnDVOCallback_RealAVData_T pStreamCallBack/* = NULL*/;
	shared_ptr<byte>pYuvBuffer /*= NULL*/;
	int nYuvBufferSize/* = 0*/;
	DWORD		nVideoFrames/* = 0*/;
	DWORD		nAudioFrames/* = 0*/;
	double		dfTimeRecv1/* = 0.0f*/;
	double		dfTimeRecv2/* = 0.0f*/;
	
	HWND		hWndView;
	int			nItem;
	void*		pThis;
	CFile		*pRecFile;
	bool		bRecvIFrame;
	time_t		tLastFrame;
	int			nFPS;
	UINT		nVideoFrameID;
	UINT		nAudioFrameID;
	int			nAudioCodec;
	bool		bWriteMediaHeader;
	time_t		tRecStartTime;
	UINT64		nRecFileLength;
	TCHAR		szIpAddress[32];
	TCHAR		szRecFilePath[MAX_PATH];
	__int64		nTimeStamp[100];
	int			nFirstID/* = 0*/;
	int			nTimeCount;

	CRITICAL_SECTION csRecFile;
public:
	PlayerContext(USER_HANDLE hUserIn,
		REAL_HANDLE hStreamIn = -1,
		IPC_PLAYHANDLE hPlayerIn = NULL,int nCount = 1)
	{
		ZeroMemory(this, sizeof(PlayerContext));
		pStreamInfo = new StreamInfo();
		bThreadRecvIPCStream = false;
		hThreadRecvStream = NULL;
		pStreamCallBack = NULL;
		pYuvBuffer.reset();
		nYuvBufferSize = 0;
		nVideoFrames = 0;
		nAudioFrames = 0;
		dfTimeRecv1 = 0.0f;
		dfTimeRecv2 = 0.0f;

		nTimeCount = 0;
		hUser = hUserIn;
		hStream = hStreamIn;
		nPlayerCount = nCount;
		hPlayer[0] = hPlayerIn;
		nAudioCodec = IPC_711_ALAW;
		InitializeCriticalSection(&csRecFile);
		m_pInputStreamTimeTrace = new TimeTrace("StreamCallBack", __FUNCTION__);

	}
	~PlayerContext()
	{
		StopRecord();
		if (hStream != -1)
		{
			DVO2_NET_StopRealPlay(hStream);
			hStream = -1;
		}
		TraceMsgA("%s Now() = %.5f.\n", __FUNCTION__, GetExactTime());
		for (int i = 0; i < nPlayerCount;i ++)
		if (hPlayer[i])
			ipcplay_Close(hPlayer[i]);
		ZeroMemory(hPlayer, sizeof(IPC_PLAYHANDLE));
		if (hPlayerStream)
		{
			ipcplay_Close(hPlayerStream);
			hPlayerStream = NULL;
		}
		if (pClient)
		{
			delete pClient;
			pClient = NULL;
		}
		if (hUser != -1)
		{
			DVO2_NET_Logout(hUser);
			hUser = -1;
		}
		if (pStreamInfo)
			delete pStreamInfo;

		if (m_pInputStreamTimeTrace)
			delete m_pInputStreamTimeTrace;
		
		DeleteCriticalSection(&csRecFile);
	}
	void StartRecv(fnDVOCallback_RealAVData_T pCallBack)
	{
		bThreadRecvIPCStream = true;
		pStreamCallBack = pCallBack;
		hThreadRecvStream = CreateThread(NULL, 0, ThreadRecvIPCStream, this, 0, NULL);
	}
	void StopRecv()
	{
		if (hThreadRecvStream)
		{
			bThreadRecvIPCStream = false;
			WaitForSingleObject(hThreadRecvStream, INFINITE);
			CloseHandle(hThreadRecvStream);
			hThreadRecvStream = NULL;
		}
	}
	

	static	DWORD WINAPI ThreadRecvIPCStream(void *p)
	{
		PlayerContext *pThis = (PlayerContext *)p;
		if (!pThis->pClient)
			return 0;
		CSocketClient *pClient = pThis->pClient;
		MSG_HEAD MsgHeader;
		DWORD nBytesRecved = 0;
		int nBufferSize = 64 * 1024;
		int nDataLength = 0;
		byte *pBuffer = new byte[nBufferSize];

		while (pThis->bThreadRecvIPCStream)
		{
			ZeroMemory(&MsgHeader, sizeof(MSG_HEAD));
			nBytesRecved = 0;
			if (pClient->Recv((char *)&MsgHeader, sizeof(MSG_HEAD), nBytesRecved) == 0 &&
				nBytesRecved == sizeof(MSG_HEAD))
			{
				int nPackLen = ntohl(MsgHeader.Pktlen) - sizeof(MSG_HEAD);
				if (nBufferSize < nPackLen)
				{
					delete[]pBuffer;
					while (nBufferSize < nPackLen)
						nBufferSize *= 2;
					pBuffer = new byte[nBufferSize];
				}
				nDataLength = 0;
				while (nDataLength < nPackLen)
				{
					if (!pClient->Recv((char *)&pBuffer[nDataLength], nPackLen - nDataLength, nBytesRecved) == 0)
						break;
					nDataLength += nBytesRecved;
				}
				app_net_tcp_enc_stream_head_t *pStreamHeader = (app_net_tcp_enc_stream_head_t *)pBuffer;
				pStreamHeader->chn = ntohl(pStreamHeader->chn);
				pStreamHeader->stream = ntohl(pStreamHeader->stream);
				pStreamHeader->frame_type = ntohl(pStreamHeader->frame_type);
				pStreamHeader->frame_num = ntohl(pStreamHeader->frame_num);
				pStreamHeader->sec = ntohl(pStreamHeader->sec);
				pStreamHeader->usec = ntohl(pStreamHeader->usec);

				pThis->pStreamCallBack(-1, -1, 0, (char *)pBuffer, nPackLen, pThis);
				ZeroMemory(pBuffer, nBufferSize);
			}
			Sleep(10);
		}
		if (pBuffer)
			delete[]pBuffer;
		return 0;
	}
	void StartRecord()
	{
		try
		{
			if (_tcslen(szRecFilePath) == 0)
				return;
			CAutoLock lock(&csRecFile);
			pRecFile = new CFile(szRecFilePath, CFile::modeCreate | CFile::modeWrite);
			tRecStartTime = time(NULL);
		}
		catch (/*std::exception* e*/CException *e)
		{
			TCHAR szError[256] = { 0 };
			e->GetErrorMessage(szError, 256);
			_tprintf(_T("%s %d Exception:%s.\n"), __FILE__, __LINE__, szError/*e->what()*/);
			e->Delete();
		}
	}

	void StopRecord()
	{
		if (!pRecFile)
			return;
		try
		{
			CAutoLock lock(&csRecFile);
			delete pRecFile;
			pRecFile = NULL;
			ZeroMemory(szRecFilePath, sizeof(szRecFilePath));
			tRecStartTime = 0;
		}
		catch (/*std::exception* e*/CException *e)
		{
			TCHAR szError[256] = { 0 };
			e->GetErrorMessage(szError, 256);
			_tprintf(_T("%s %d Exception:%s.\n"), __FILE__, __LINE__, szError/*e->what()*/);
			e->Delete();
			pRecFile = NULL;
		}
	}
};

struct ListItem
{
	TCHAR szItemName[32];
	TCHAR szItemText[256];
};

struct FullScreenWnd
{
	WINDOWPLACEMENT windowedPWP;
	DWORD dwExStyle;
	DWORD dwStyle;
	HMENU hMenu;
	HWND  hWnd;
	FullScreenWnd()
	{
		ZeroMemory(this, sizeof(FullScreenWnd));
	}

	void SetWnd(HWND hWnd)
	{
		this->hWnd = hWnd;
		GetWindowPlacement(hWnd, &windowedPWP);
	}
	void SwitchFullScreen()
	{
		dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
		dwStyle = GetWindowLong(hWnd, GWL_STYLE);
		dwStyle &= ~WS_MAXIMIZE & ~WS_MINIMIZE; // remove minimize/maximize style
		hMenu = GetMenu(hWnd);
		// Hide the window to avoid animation of blank windows
		ShowWindow(hWnd, SW_HIDE);
		// Set FS window style
		SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_SYSMENU | WS_CLIPCHILDREN);
		SetMenu(hWnd, NULL);
		ShowWindow(hWnd, SW_SHOWMAXIMIZED);
	}
	void Restore()
	{
		if (dwStyle != 0)
			SetWindowLong(hWnd, GWL_STYLE, dwStyle);

		if (dwExStyle != 0)
			SetWindowLong(hWnd, GWL_EXSTYLE, dwExStyle);

		if (hMenu != NULL)
			SetMenu(hWnd, hMenu);

		if (windowedPWP.length == sizeof(WINDOWPLACEMENT))
		{
			if (windowedPWP.showCmd == SW_SHOWMAXIMIZED)
			{
				ShowWindow(hWnd, SW_HIDE);
				windowedPWP.showCmd = SW_HIDE;
				SetWindowPlacement(hWnd, &windowedPWP);
				ShowWindow(hWnd, SW_SHOWMAXIMIZED);
				windowedPWP.showCmd = SW_SHOWMAXIMIZED;
			}
			else
				SetWindowPlacement(hWnd, &windowedPWP);
		}
	}
};
enum DockType
{
	DockTop = 0,
	DockBottom,
	DockLeft,
	DockRigth,
	DockCenter	//居中
};

struct WndPostionInfo
{
	HWND	hWnd;
	UINT	nID;
	DockType Dock;			// 停靠类型
	UINT DockDistance[4];	// 停靠距离
	RECT rect;				// 原始大小
};

struct YUVFrame
{
	int nWidth;
	int nHeight;
	int nStrideY;
	int nStrideUV;
	byte *pY;	
	byte *pU;	
	byte *pV;
	__int64	nDecodeTime;
	int nPlayedCount;
	CRITICAL_SECTION csCount;
	
	YUVFrame()
	{
		ZeroMemory(this, sizeof(YUVFrame));
		InitializeCriticalSection(&csCount);
	}
	YUVFrame(const unsigned char* pY,
		const unsigned char* pU,
		const unsigned char* pV,
		int nStrideY,
		int nStrideUV,
		int nWidth,
		int nHeight,
		INT64 nTime)
	{
		InitializeCriticalSection(&csCount);
		int nYSize = nStrideY*nHeight;
		int nUVSize = nStrideUV*(nHeight >> 1);
		this->pY = new byte[nYSize];
		this->pU = new byte[nUVSize];
		this->pV = new byte[nUVSize];
		memcpy(this->pY, pY, nYSize);
		memcpy(this->pU, pU, nUVSize);
		memcpy(this->pV, pV, nUVSize);
		this->nHeight = nHeight;
		this->nWidth = nWidth;
		this->nStrideUV = nStrideUV;
		this->nStrideY = nStrideY;
	}
	~YUVFrame()
	{
		if (pY)
			delete[]pY;
		if (pU)
			delete[]pU;
		if (pV)
			delete[]pV;
		DeleteCriticalSection(&csCount);
		ZeroMemory(this, sizeof(YUVFrame));
	}
	int GetPlayedCount()
	{
		CAutoLock lock(&csCount);
		return nPlayedCount;
	}
	void IncPlayedCount()
	{
		CAutoLock lock(&csCount);
		nPlayedCount++;
	}
};

typedef shared_ptr<YUVFrame> YUVFramePtr;

struct SubClassInfo
{
private:
	SubClassInfo()
	{

	}
public:
	HWND hMainWnd;
	HWND hPartnerWnd;
	WNDPROC	pOldProcWnd;
	

	SubClassInfo(HWND hWnd,HWND hPartnerWnd,WNDPROC pWndProc)
	{
		ZeroMemory(this, sizeof(SubClassInfo));
		hMainWnd = hWnd;
		pOldProcWnd = (WNDPROC)::GetWindowLong(hMainWnd, GWL_WNDPROC);
		this->hPartnerWnd = hPartnerWnd;
		SetWindowLong(hMainWnd, GWL_WNDPROC, (long)pWndProc);
	}
	~SubClassInfo()
	{
		if (pOldProcWnd && hMainWnd)
			SetWindowLong(hMainWnd, GWL_WNDPROC, (long)pOldProcWnd);
	}
};
typedef shared_ptr<SubClassInfo> SubClassInfoPtr;
// CIPCPlayDemoDlg 对话框
#include "TransparentWnd.h"

class CIPCPlayDemoDlg : public CDialogEx
{
	// 构造
public:
	CIPCPlayDemoDlg(CWnd* pParent = NULL);	// 标准构造函数

	// 对话框数据
	enum { IDD = IDD_IPCPLAYDEMO_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


	// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CBitmap		m_bitmapMask;
	bool		m_bEnableDDraw = false;
	CListCtrl	m_wndStreamInfo;
	CMFCEditBrowseCtrl	m_wndBrowseCtrl;
	int			m_nListWidth;			// List控件的宽度
	int			m_nListTop;				// List宽度的Top坐标
	TCHAR		m_szRecordPath[MAX_PATH];
	int			m_nMonitorCount;		//  当前已经连接显示器的数量
	CVideoFrame *m_pVideoWndFrame/* = NULL*/;
	CGlliteryStatic m_wndStatus;
	double	m_dfLastUpdate;
	bool	m_bRefreshPlayer/* = true*/;
	bool SaveSetting();
	bool LoadSetting();
	static CFile *m_pVldReport;
	ListItem m_szListText[16];
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonDisconnect();
	afx_msg void OnBnClickedButtonPlaystream();
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnBnClickedButtonPlayfile();
	list<shared_ptr<PlayerContext>>m_listPlayer;
	// 相机实时码流捕捉回调函数
	static void  __stdcall StreamCallBack(IN USER_HANDLE  lUserID,
		IN REAL_HANDLE lStreamHandle,
		IN int         nErrorType,
		IN const char* pBuffer,
		IN int         nDataLen,
		IN void*       pUser);
	static void __stdcall ExternDCDraw(HWND hWnd, HDC hDc, RECT rt, void *pUserPtr);
	static void __stdcall PlayerCallBack(IPC_PLAYHANDLE hPlayHandle, void *pUserPtr);
	shared_ptr<PlayerInfo>	m_pPlayerInfo;
	afx_msg void OnNMClickListConnection(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnUpdateStreamInfo(WPARAM, LPARAM);
	afx_msg LRESULT OnUpdatePlayInfo(WPARAM, LPARAM);
	afx_msg void OnNMCustomdrawListStreaminfo(NMHDR *pNMHDR, LRESULT *pResult);
	shared_ptr<PlayerContext>m_pPlayContext;
	bool m_bOPAssistEnabled = false;
	vector<long> m_vecPolygon;
	afx_msg void OnBnClickedCheckEnableaudio();
	afx_msg void OnBnClickedCheckFitwindow();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedButtonSnapshot();
	afx_msg void OnCbnSelchangeComboPlayspeed();
	afx_msg void OnBnClickedButtonPause();
	afx_msg void OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2);
	FullScreenWnd m_FullScreen;

	HWND m_hFullScreen/* = NULL*/;
	UINT m_nOriMonitorIndex/* = 0*/;
	void *m_hIOCP/* = NULL*/;
	bool m_bClickPlayerSlide/* = false*/;
	//shared_ptr<CDirectDraw> m_pDDraw/* = NULL*/;
	//shared_ptr<ImageSpace> m_pYUVImage/* = NULL*/;
	vector<WndPostionInfo> m_vWndPostionInfo;
	void SaveWndPosition(UINT *nDlgItemIDArray, UINT nItemCount, DockType nDock, RECT rtDialogClientRect);

	/// @brief		解码后YVU数据回调函数定义
	/// @param [in]		hPlayHandle	由ipcplay_OpenFile或ipcplay_OpenStream返回的播放句柄
	/// @param [in]		pYUV		YUV数据指针
	/// @param [in]		nSize		YUV数据的长度
	/// @param [in]		nWidth		YUV图像的宽度
	/// @param [in]		nHeight		YUV图像的高度
	/// @param [in]		nTime		产生YUV数据的时间
	/// @param [in]		pUserPtr	用户自定义指针
	CFile *m_pYUVFile = nullptr;

	//list<YUVFramePtr> m_listYUVFrame;
	CRITICAL_SECTION m_csYUVFrame;
	shared_ptr<CDisplayYUVFrame> m_pYUVFrame;
	LONGLONG		m_nYUVCount = 0;
	static void __stdcall CaptureYUVExProc(IPC_PLAYHANDLE hPlayHandle,
		const unsigned char* pY,
		const unsigned char* pU,
		const unsigned char* pV,
		int nStrideY,
		int nStrideUV,
		int nWidth,
		int nHeight,
		INT64 nTime,
		void *pUserPtr)
	{
		CIPCPlayDemoDlg *pThis = (CIPCPlayDemoDlg *)pUserPtr;
		//YUVFramePtr yuvFrame =make_shared<YUVFrame>(pY,pU,	pV,nStrideY,nStrideUV,nWidth,nHeight,nTime);
		//pThis->m_listYUVFrame.push_back(yuvFrame);
		if (!pThis->m_pYUVFrame)
		{
			pThis->m_nYUVCount = 0;
			pThis->m_pYUVFrame = make_shared<CDisplayYUVFrame>(pY, pU, pV, nStrideY, nStrideUV, nWidth, nHeight);
			pThis->m_nYUVCount++;
		}
		else
		{
			if (pThis->m_nYUVCount % 25 == 0)
			{
				if (TryEnterCriticalSection(&pThis->m_csYUVFrame))
				{
					pThis->m_pYUVFrame->UpdateYUV(pY, pU, pV);
					pThis->PostMessage(WM_UPDATEYUV, (WPARAM)hPlayHandle);
					LeaveCriticalSection(&pThis->m_csYUVFrame);
				}
			}
			pThis->m_nYUVCount++;
		}
	}
	CDialogDisplayRGB24 *m_pDisplayRGB24 = nullptr;
	afx_msg LRESULT OnUpdateYUV(WPARAM w, LPARAM l)
	{
		if (m_pDisplayRGB24 && m_pDisplayRGB24->GetSafeHwnd())
			m_pYUVFrame->Render(m_pDisplayRGB24->GetSafeHwnd());

		return 0;
	}

	volatile bool bThreadRun = false;
	HANDLE m_hRenderThread1 = nullptr;
	HANDLE m_hRenderThread2 = nullptr;
	// 	static UINT __stdcall RenderThread1(void *p)
	// 	{
	// 		CTestDisplayDlg *pThis = (CTestDisplayDlg *)p;
	// 		DDSURFACEDESC2 ddsd = { 0 };
	// 		int nWidth = 1280;
	// 		int nHeight = 720;
	// 		CDirectDraw *m_pDDraw = new CDirectDraw();
	// 		FormatYV12::Build(ddsd, nWidth, nHeight);
	// 		HWND hRenderhWnd = pThis->GetDlgItem(IDC_STATIC_FRAME1)->m_hWnd;
	// 		m_pDDraw->Create<FormatYV12>(hRenderhWnd, ddsd);
	// 		shared_ptr<ImageSpace> m_pYUVImage = NULL;
	// 		m_pYUVImage = make_shared<ImageSpace>();
	// 		m_pYUVImage->dwLineSize[0] = nWidth;
	// 		m_pYUVImage->dwLineSize[1] = nWidth >> 1;
	// 		m_pYUVImage->dwLineSize[2] = nWidth >> 1;
	// 
	// 		int nSize = nHeight*nWidth * 3 / 2;
	// 		byte *pBuffer = new byte[nSize];
	// 		
	// 		YuvFile.Read(pBuffer, nSize);
	// 		m_pYUVImage->pBuffer[0] = (PBYTE)pBuffer;
	// 		m_pYUVImage->pBuffer[1] = (PBYTE)pBuffer + nWidth * nHeight;
	// 		m_pYUVImage->pBuffer[2] = (PBYTE)pBuffer + nWidth * 5 * nHeight / 4;
	// 		long nCount = 0;
	// 		while (pThis->bThreadRun)
	// 		{
	// 			m_pDDraw->Draw(*m_pYUVImage, nullptr, nullptr, true);
	// 			nCount++;
	// 		}
	// 		pThis->SetDlgItemInt(IDC_STATIC_RENDER1, nCount);
	// 		return 0;
	// 	}
	// 	
	// 	static UINT __stdcall RenderThread2(void *p)
	// 	{
	// 		CTestDisplayDlg *pThis = (CTestDisplayDlg *)p;
	// 		CDxSurfaceEx* m_pDxSurface = new CDxSurfaceEx();
	// 		AVFrame *PFrame = ;
	// 	
	// 
	// 		return 0;
	// 	}
	static void __stdcall CaptureYUVProc(IPC_PLAYHANDLE hPlayHandle,
		const unsigned char* pYUV,
		int nYUVSize,
		int nStrideY,
		int nStrideUV,
		int nWidth,
		int nHeight,
		INT64 nTime,
		void *pUserPtr)
	{
		CIPCPlayDemoDlg *pThis = (CIPCPlayDemoDlg *)pUserPtr;
		// 以下代码片段用于把一帧YUV保存数据保存到文件中
		// 		if (pThis->m_pYUVFile)
		// 		{
		// 			pThis->m_pYUVFile->Write(pYUV, nYUVSize);
		// 			pThis->m_pYUVFile->Close();
		// 			delete pThis->m_pYUVFile;
		// 			pThis->m_pYUVFile = nullptr;
		// 		}	

		if (!pThis->m_pYUVFrame)
		{
			// 			pThis->m_nYUVCount = 0;
			// 			pThis->m_pYUVFrame =make_shared<CDisplayYUVFrame>(pYUV, nStrideY, nStrideY>>1, nWidth, nHeight);
			// 			pThis->m_nYUVCount++;
		}
		else
		{
			if (pThis->m_nYUVCount % 25 == 0)
			{
				// 				if (TryEnterCriticalSection(&pThis->m_csYUVFrame))
				// 				{
				// 					pThis->m_pYUVFrame->UpdateYUV(pYUV);
				// 					pThis->PostMessage(WM_UPDATEYUV, (WPARAM)hPlayHandle);
				// 					LeaveCriticalSection(&pThis->m_csYUVFrame);
				// 				}
			}
			pThis->m_nYUVCount++;
		}
	}
	byte *m_pRGBBuffer;
	long  m_nRGBSize;

	/// @brief		解码后YVU数据回调
	static void __stdcall YUVFilterProc(IPC_PLAYHANDLE hPlayHandle,
		const unsigned char* pY,
		const unsigned char* pU,
		const unsigned char* pV,
		int nStrideY,
		int nStrideUV,
		int nWidth,
		int nHeight,
		INT64 nTime,
		void *pUserPtr)
	{
		CIPCPlayDemoDlg *pThis = (CIPCPlayDemoDlg *)pUserPtr;
		if (!pThis->m_pPlayContext->hPlayer[0])
		{
			return;
		}

		// 		if (!pThis->m_pDDraw)
		// 		{
		// 			PlayerInfo pi;
		// 			if (ipcplay_GetPlayerInfo(pThis->m_pPlayContext->hPlayer[0], &pi) != IPC_Succeed)
		// 			{
		// 				assert(false);
		// 				return;
		// 			}
		// 			//构造表面  
		// 			DDSURFACEDESC2 ddsd = { 0 };
		// 			FormatYV12::Build(ddsd, pi.nVideoWidth, pi.nVideoHeight);
		// 			pThis->m_pDDraw = make_shared<CDirectDraw>();
		// 			pThis->m_pDDraw->Create<FormatYV12>(pThis->m_pVideoWndFrame->GetPanelWnd(1), ddsd);
		// 			pThis->m_pYUVImage = make_shared<ImageSpace>();
		// 			pThis->m_pYUVImage->dwLineSize[0] = nWidth;
		// 			pThis->m_pYUVImage->dwLineSize[1] = nWidth >> 1;
		// 			pThis->m_pYUVImage->dwLineSize[2] = nWidth >> 1;
		// 		}
		// 		pThis->m_pYUVImage->pBuffer[0] = (PBYTE)pY;
		// 		pThis->m_pYUVImage->pBuffer[1] = (PBYTE)pU;
		// 		pThis->m_pYUVImage->pBuffer[2] = (PBYTE)pV;
		// 		//g_pVca->ProcessYuv(pY, pU, pV, nStrideY, nStrideUV, nWidth, nHeight);
		// 
		// 		pThis->m_pDDraw->Draw(*pThis->m_pYUVImage,true);
		// todo:把YUV数据整合为YV12数据，并交给VCA引擎进行分析，把分析交给VCA Render渲染，再把渲染结果还到pY,pU,pV当中

	}

	afx_msg LRESULT OnTroggleFullScreen(WPARAM W, LPARAM L)
	{
		if (!m_pPlayContext || !m_pPlayContext->hPlayer[0])
			return 0l;
		if (!m_hFullScreen)
		{// 切到全屏
			m_hFullScreen = (HWND)W;
			HWND hDesktop = ::GetDesktopWindow();
			m_nOriMonitorIndex = GetMonitorIndexFromWnd(m_hWnd);
			m_FullScreen.SetWnd(m_hFullScreen);
			::SetParent(m_hFullScreen, ::GetDesktopWindow());
			POINT pt;
			pt.x = 0;
			pt.y = 0;
			MoveWnd2Monitor(m_hFullScreen, m_nOriMonitorIndex, pt);
			m_FullScreen.SwitchFullScreen();
			ipcplay_Reset(m_pPlayContext->hPlayer[0], m_hFullScreen);
			ShowWindow(SW_HIDE);
		}
		else
		{// 切回窗口
			m_FullScreen.Restore();
			::SetParent(m_hFullScreen, m_pVideoWndFrame->GetSafeHwnd());
			ipcplay_Reset(m_pPlayContext->hPlayer[0], m_hFullScreen);
			m_hFullScreen = NULL;
			ShowWindow(SW_SHOW);
		}
		return 0;
	}
	bool m_bPuased/* = false*/;
	volatile bool m_bThreadStream /*= false*/;
	HANDLE  m_hThreadSendStream/* = NULL*/;
	HANDLE  m_hThreadPlayStream /*= NULL*/;
	int		m_nRow/* = 1*/;
	int		m_nCol/* = 1*/;
	ATOM m_nHotkeyID/* = 0*/;
	struct DvoStream
	{
		DvoStream(byte *pBuffer, int nBufferSize)
		{
			pStream = new byte[nBufferSize];
			nStreamSize = nBufferSize;
			if (pStream)
				memcpy(pStream, pBuffer, nBufferSize);
		}
		~DvoStream()
		{
			if (pStream)
				delete[]pStream;
		}
		byte *pStream;
		int  nStreamSize;
	};
	typedef shared_ptr<DvoStream> DvoStreamPtr;
	CRITICAL_SECTION m_csListStream;
	list<DvoStreamPtr> m_listStream;	// 流播放队列
	// 发送数据流
	// 这里只是把数据放入流播放队列来模拟发送操作
	int SendStream(byte *pFrameBuffer, int nFrameSize)
	{
		CAutoLock lock(&m_csListStream);
		DvoStreamPtr pStream = make_shared<DvoStream>(pFrameBuffer, nFrameSize);
		m_listStream.push_back(pStream);
		return m_listStream.size();
	}
	// 接收数据流
	// 这里只是从流队列中取出数据来模拟接收操作
	int RecvStream(DvoStreamPtr &pStream, bool &bRecved)
	{
		CAutoLock lock(&m_csListStream);
		if (m_listStream.size() > 0)
		{
			pStream = m_listStream.front();
			m_listStream.pop_front();
			bRecved = true;
		}
		else
			bRecved = false;
		return m_listStream.size();
	}

	// 解码录像文件线程,一般在服务端,把一帧数的解析出来后放到流队列中
	// 在实际的流媒体服务器中，应该是把数据直接发送到播放客户端去
	static  DWORD WINAPI ThreadSendStream(void *p)
	{
		CIPCPlayDemoDlg *pThis = (CIPCPlayDemoDlg *)p;
		if (!pThis->m_pPlayContext ||
			!pThis->m_pPlayContext->hPlayer)
			return 0;
		shared_ptr<PlayerContext>pPlayCtx = pThis->m_pPlayContext;
		IPCFrameHeader Header;
		byte *pFrameBuffer = NULL;
		UINT nFrameSize = 0;

		int nDvoError = 0;
		int nStreamListSize = 0;
		int nFrames = 0;
		while (pThis->m_bThreadStream)
		{
			// 读取下一帧
			if (nStreamListSize < 8)
			{
				int nFrameType = 0;
				if (nDvoError = ipcplay_GetFrame(pPlayCtx->hPlayer[0], &pFrameBuffer, nFrameSize) != IPC_Succeed)
				{
					TraceMsgA("%s ipcplay_GetFrame Failed:%d.\n", __FUNCTION__, nDvoError);
					Sleep(5);
					continue;
				}
				nFrames++;
				nStreamListSize = pThis->SendStream(pFrameBuffer, nFrameSize);
			}
			else
			{
				CAutoLock lock(&pThis->m_csListStream);
				nStreamListSize = pThis->m_listStream.size();
			}
			Sleep(5);
		}
		return 0;
	}
	// 流播放线程,从流队列中取出数据进行播放
	// 实际开发中，应该是从服务端接数据进行播放
	static  DWORD WINAPI ThreadPlayStream(void *p)
	{
		CIPCPlayDemoDlg *pThis = (CIPCPlayDemoDlg *)p;
		if (!pThis->m_pPlayContext ||
			!pThis->m_pPlayContext->hPlayerStream)
			return 0;
		shared_ptr<PlayerContext>pPlayCtx = pThis->m_pPlayContext;
		int nDvoError = 0;
		int nStreamListSize = 0;
		DvoStreamPtr pStream;
		bool bRecved = false;
		int nLoopCount = 0;
		bool bInputList = true;
		while (pThis->m_bThreadStream)
		{
			if (bInputList)
			{
				nStreamListSize = pThis->RecvStream(pStream, bRecved);
				if (!bRecved)
				{
					Sleep(5);
					continue;
				}
			}

			nLoopCount++;
			if (nDvoError = ipcplay_InputStream(pPlayCtx->hPlayerStream, pStream->pStream, pStream->nStreamSize) != IPC_Succeed)
			{

				int nDvoError = ipcplay_GetPlayerInfo(pPlayCtx->hPlayerStream, pThis->m_pPlayerInfo.get());
				TraceMsgA("%s ipcplay_InputStream Failed:%d\tVideocache size = %d.\n", __FUNCTION__, nDvoError, pThis->m_pPlayerInfo->nCacheSize);
				bInputList = false;
				Sleep(5);
				continue;
			}

			int nDvoError = ipcplay_GetPlayerInfo(pPlayCtx->hPlayerStream, pThis->m_pPlayerInfo.get());
			if (nDvoError == IPC_Succeed ||
				nDvoError == IPC_Error_FileNotExist)
				pThis->PostMessage(WM_UPDATE_PLAYINFO, (WPARAM)pThis->m_pPlayerInfo.get(), (LPARAM)nDvoError);
			bInputList = true;
			Sleep(5);
		}
		return 0;
	}
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonTracecache();
	afx_msg void OnLvnGetdispinfoListStreaminfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonStopbackword();
	afx_msg void OnBnClickedButtonStopforword();
	afx_msg void OnBnClickedButtonSeeknextframe();
	afx_msg void OnBnClickedCheckEnablelog();
	afx_msg void OnNMReleasedcaptureSliderPlayer(NMHDR *pNMHDR, LRESULT *pResult);
	//afx_msg void OnBnClickedCheckRefreshplayer();
	afx_msg void OnBnClickedCheckEnablehaccel();
	afx_msg void OnBnClickedCheckRefreshplayer();
	afx_msg void OnBnClickedCheckSetborder();
	//afx_msg void OnBnClickedCheckExterndraw();
	afx_msg void OnFileAddrenderwnd();
	afx_msg void OnFileRemoveRenderWnd();
	int  nCurPannelID = 0;
	afx_msg void OnBnClickedCheckDisplayrgb();
	afx_msg void OnFileDisplaytran();
	afx_msg void OnBnClickedButtonDivideframe();

	int		m_nPanelCapture;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	POINT m_ptMove;
	int		m_nPanelID;

	POINT m_ptStart;
	POINT m_ptStop;
	long  m_nLineID = -1;
	bool  m_bLButtondown = false;

	afx_msg LRESULT OnFrameLButtonDown(WPARAM W, LPARAM L)
	{
		//		int nPanel = W;	
		// 		RECT rtPanel = *(m_pVideoWndFrame->GetPanelRect(nPanel));
		// 		m_pVideoWndFrame->ClientToScreen(&rtPanel);
		m_ptStart.x = LOWORD(L);
		m_ptStart.y = HIWORD(L);
		m_bLButtondown = true;

		return 0;
	}
	afx_msg LRESULT OnFrameLButtonUp(WPARAM W, LPARAM L)
	{
		if (!m_pPlayContext || !m_pPlayContext->hPlayer[0])
		{
			AfxMessageBox(_T("尚未播放图像像"));
			return 0;
		}
		m_ptStop.x = LOWORD(L);
		m_ptStop.y = HIWORD(L);
		POINT ptArray[2] = { 0 };
		ptArray[0] = m_ptStart;
		ptArray[1] = m_ptStop;

		if (m_nLineID != -1)
		{
			ipcplay_RemoveLineArray(m_pPlayContext->hPlayer[0], m_nLineID);
		}

		m_nLineID = ipcplay_AddLineArray(m_pPlayContext->hPlayer[0], &ptArray[0], 2, 2, IPC_XRGB(255, 0, 0));
		m_bLButtondown = false;
		return 0;
	}
	afx_msg LRESULT OnFrameMouseMove(WPARAM W, LPARAM L)
	{
		if (!m_bLButtondown ||
			!m_pPlayContext || 
			!m_pPlayContext->hPlayer[0])
			return 0;
		m_ptStop.x = LOWORD(L);
		m_ptStop.y = HIWORD(L);
		POINT ptArray[2] = { 0 };
		ptArray[0] = m_ptStart;
		ptArray[1] = m_ptStop;
		if (m_nLineID != -1)
		{
			ipcplay_RemoveLineArray(m_pPlayContext->hPlayer[0], m_nLineID);
		}
		
		m_nLineID = ipcplay_AddLineArray(m_pPlayContext->hPlayer[0], &ptArray[0], 2, 2, IPC_XRGB(255, 0, 0));
		
		return 0;
	}
	afx_msg void OnBnClickedCheckEnabletransparent();
	CTransparentDlg	*m_pTransparentDlg;
	afx_msg void OnFileDrawline();
	afx_msg void OnRender(UINT nID);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnMove(int x, int y);
	
	static CRITICAL_SECTION m_csMapSubclassWnd;
	static map<HWND, SubClassInfoPtr> m_MapSubclassWnd;
	static LRESULT SubClassProc(HWND, UINT, WPARAM, LPARAM);
	CTransparentWnd *m_pTransparentWnd = nullptr;
	afx_msg void OnEnableOpassist();
	afx_msg void OnBnClickedCheckNodecodedelay();
};
