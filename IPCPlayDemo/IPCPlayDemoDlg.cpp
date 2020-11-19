﻿
// DvoIPCPlayDemoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "IPCPlayDemo.h"
#include "IPCPlayDemoDlg.h"
#include "afxdialogex.h"
#include <MMSystem.h>
#pragma comment(lib,"winmm.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//CVca* g_pVca = NULL;
enum _SubItem
{
	Item_VideoInfo,
	Item_ACodecType,	
	Item_StreamRate,
	Item_FrameRate,
	Item_VFrameCache,
	Item_AFrameCache,
	Item_TotalStream,
	Item_ConnectTime,
	Item_PlayedTime,
	Item_RecFile,
	Item_RecTime,
	Item_FileLength,
};


#define  ID_PLAYEVENT 1024
#define _PlayInterval 250
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CDvoIPCPlayDemoDlg 对话框

#define _Row	1
#define _Col	1


CIPCPlayDemoDlg::CIPCPlayDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CIPCPlayDemoDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bPuased = false;
	m_bThreadStream = false;
	m_hThreadSendStream = NULL;
	m_hThreadPlayStream = NULL;
	m_nRow = 2;
	m_nCol = 2;
	m_nHotkeyID = 0;

	m_hFullScreen = NULL;
	m_nOriMonitorIndex = 0;
	m_hIOCP = NULL;
	m_bClickPlayerSlide = false;
	// 	m_pDDraw.reset();;
	// 	m_pYUVImage.reset();

	m_pVideoWndFrame = NULL;
	m_bRefreshPlayer = true;
	InitializeCriticalSection(&m_csYUVFrame);
	/*
	IPC_CODEC	nVideoCodec;	///< 视频编码格式,@see IPC_CODEC
	IPC_CODEC	nAudioCodec;	///< 音频编码格式,@see IPC_CODEC
	USHORT		nVideoWidth;	///< 视频图像宽度
	USHORT		nVideoHeight;	///< 视频图像高度
	BOOL		bAudioEnabled;	///< 是否已开启音频
	UINT		nTotalFrames;	///< 视频总帧数,只有文件播放时才有效
	time_t		tTotalTime;		///< 文件总时长(单位:毫秒),只有文件播放时才有效
	UINT		nCurFrameID;	///< 当前播放视频的帧ID,只有文件播放时才有效,nSDKVersion<IPC_IPC_SDK_VERSION_2015_12_16无效
	time_t		tFirstFrameTime;///< 收到的第一帧的时间(单位:毫秒)
	time_t		tCurFrameTime;	///< 返回当前播放视频的帧相对起点的时间(单位:毫秒)
	time_t		tTimeEplased;	///< 已播放时间(单位:毫秒)
	USHORT		nFPS;			///< 文件或码流中视频的原始帧率
	USHORT		nPlayFPS;		///< 当前播放的帧率
	WORD		nCacheSize;		///< 播放缓存
	WORD		nCacheSize2;	///< 音频缓存
	float		fPlayRate;		///< 播放速率,只有文件播放时才有效
	long		nSDKVersion;	///< SDK版本,详细定义参见@see IPC_MEDIAINFO
	bool		bFilePlayFinished;///< 文件播放完成标志,为true时，播放结束，为false时，则未结束
	byte		nReserver1[3];
	UINT		nReserver2[2];
	*/
// 	TraceMsgA("offsetof(nVideoCodec) = %d\r\n", offsetof(PlayerInfo, nVideoCodec));
// 	TraceMsgA("offsetof(nAudioCodec) = %d\r\n", offsetof(PlayerInfo, nAudioCodec));
// 	TraceMsgA("offsetof(nVideoWidth) = %d\r\n", offsetof(PlayerInfo, nVideoWidth));
// 	TraceMsgA("offsetof(nVideoHeight) = %d\r\n", offsetof(PlayerInfo, nVideoHeight));
// 	TraceMsgA("offsetof(bAudioEnabled) = %d\r\n", offsetof(PlayerInfo, bAudioEnabled));
// 	TraceMsgA("offsetof(nTotalFrames) = %d\r\n", offsetof(PlayerInfo, nTotalFrames));
// 	TraceMsgA("offsetof(tTotalTime) = %d\r\n", offsetof(PlayerInfo, tTotalTime));
// 	TraceMsgA("offsetof(nCurFrameID) = %d\r\n", offsetof(PlayerInfo, nCurFrameID));
// 	TraceMsgA("offsetof(tFirstFrameTime) = %d\r\n", offsetof(PlayerInfo, tFirstFrameTime));
// 	TraceMsgA("offsetof(tCurFrameTime) = %d\r\n", offsetof(PlayerInfo, tCurFrameTime));
// 	TraceMsgA("offsetof(tTimeEplased) = %d\r\n", offsetof(PlayerInfo, tTimeEplased));
// 	TraceMsgA("offsetof(nFPS) = %d\r\n", offsetof(PlayerInfo, nFPS));
// 	TraceMsgA("offsetof(nPlayFPS) = %d\r\n", offsetof(PlayerInfo, nPlayFPS));
// 	TraceMsgA("offsetof(nCacheSize) = %d\r\n", offsetof(PlayerInfo, nCacheSize));
// 	TraceMsgA("offsetof(nCacheSize2) = %d\r\n", offsetof(PlayerInfo, nCacheSize2));
// 	TraceMsgA("offsetof(fPlayRate) = %d\r\n", offsetof(PlayerInfo, fPlayRate));
// 	TraceMsgA("offsetof(nSDKVersion) = %d\r\n", offsetof(PlayerInfo, nSDKVersion));
// 	TraceMsgA("offsetof(bFilePlayFinished) = %d\r\n", offsetof(PlayerInfo, bFilePlayFinished));
// 	TraceMsgA("offsetof(nReserver1) = %d\r\n", offsetof(PlayerInfo, nReserver1));
// 	TraceMsgA("offsetof(nReserver2) = %d\r\n", offsetof(PlayerInfo, nReserver2));

}
void CIPCPlayDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CIPCPlayDemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CIPCPlayDemoDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, &CIPCPlayDemoDlg::OnBnClickedButtonDisconnect)
	ON_BN_CLICKED(IDC_BUTTON_PLAYSTREAM, &CIPCPlayDemoDlg::OnBnClickedButtonPlaystream)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CIPCPlayDemoDlg::OnBnClickedButtonRecord)
	ON_BN_CLICKED(IDC_BUTTON_PLAYFILE, &CIPCPlayDemoDlg::OnBnClickedButtonPlayfile)		
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_STREAMINFO, &CIPCPlayDemoDlg::OnNMCustomdrawListStreaminfo)
	ON_BN_CLICKED(IDC_CHECK_DISABLEAUDIO, &CIPCPlayDemoDlg::OnBnClickedCheckEnableaudio)
	ON_BN_CLICKED(IDC_CHECK_FITWINDOW, &CIPCPlayDemoDlg::OnBnClickedCheckFitwindow)	
	ON_WM_HSCROLL()
	ON_MESSAGE(WM_UPDATE_STREAMINFO, &CIPCPlayDemoDlg::OnUpdateStreamInfo)
	ON_MESSAGE(WM_UPDATE_PLAYINFO, &CIPCPlayDemoDlg::OnUpdatePlayInfo)
	ON_BN_CLICKED(IDC_BUTTON_SNAPSHOT, &CIPCPlayDemoDlg::OnBnClickedButtonSnapshot)
	ON_CBN_SELCHANGE(IDC_COMBO_PLAYSPEED, &CIPCPlayDemoDlg::OnCbnSelchangeComboPlayspeed)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &CIPCPlayDemoDlg::OnBnClickedButtonPause)
	ON_WM_TIMER()
	ON_WM_HOTKEY()
	ON_BN_CLICKED(IDC_BUTTON_TRACECACHE, &CIPCPlayDemoDlg::OnBnClickedButtonTracecache)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LIST_STREAMINFO, &CIPCPlayDemoDlg::OnLvnGetdispinfoListStreaminfo)
	ON_MESSAGE(WM_FRAME_LBUTTONDBLCLK,OnTroggleFullScreen)
	ON_BN_CLICKED(IDC_BUTTON_STOPBACKWORD, &CIPCPlayDemoDlg::OnBnClickedButtonStopbackword)
	ON_BN_CLICKED(IDC_BUTTON_STOPFORWORD, &CIPCPlayDemoDlg::OnBnClickedButtonStopforword)
	ON_BN_CLICKED(IDC_BUTTON_SEEKNEXTFRAME, &CIPCPlayDemoDlg::OnBnClickedButtonSeeknextframe)
	ON_BN_CLICKED(IDC_CHECK_ENABLELOG, &CIPCPlayDemoDlg::OnBnClickedCheckEnablelog)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_PLAYER, &CIPCPlayDemoDlg::OnNMReleasedcaptureSliderPlayer)
	//ON_BN_CLICKED(IDC_CHECK_REFRESHPLAYER, &CDvoIPCPlayDemoDlg::OnBnClickedCheckRefreshplayer)
	ON_BN_CLICKED(IDC_CHECK_ENABLEHACCEL, &CIPCPlayDemoDlg::OnBnClickedCheckEnablehaccel)
	ON_BN_CLICKED(IDC_CHECK_REFRESHPLAYER, &CIPCPlayDemoDlg::OnBnClickedCheckRefreshplayer)
	ON_BN_CLICKED(IDC_CHECK_SETBORDER, &CIPCPlayDemoDlg::OnBnClickedCheckSetborder)
	//ON_BN_CLICKED(IDC_CHECK_EXTERNDRAW, &CIPCPlayDemoDlg::OnBnClickedCheckExterndraw)
	ON_COMMAND(ID_FILE_ADDRENDERWND, &CIPCPlayDemoDlg::OnFileAddrenderwnd)
	ON_COMMAND(ID_FILE_REMOVERENDERWND, &CIPCPlayDemoDlg::OnFileRemoveRenderWnd)
	ON_BN_CLICKED(IDC_CHECK_DISPLAYRGB, &CIPCPlayDemoDlg::OnBnClickedCheckDisplayrgb)
	ON_MESSAGE(WM_UPDATEYUV, OnUpdateYUV)
	ON_COMMAND(ID_FILE_DISPLAYTRAN, &CIPCPlayDemoDlg::OnFileDisplaytran)
	ON_BN_CLICKED(IDC_BUTTON_DIVIDEFRAME, &CIPCPlayDemoDlg::OnBnClickedButtonDivideframe)
	ON_MESSAGE(WM_FRAME_LBUTTONDOWN,OnFrameLButtonDown)
	ON_MESSAGE(WM_FRAME_LBUTTONUP,OnFrameLButtonUp)
	ON_MESSAGE(WM_FRAME_MOUSEMOVE,OnFrameMouseMove)

	ON_BN_CLICKED(IDC_CHECK_ENABLETRANSPARENT, &CIPCPlayDemoDlg::OnBnClickedCheckEnabletransparent)
	ON_COMMAND(ID_FILE_DRAWLINE, &CIPCPlayDemoDlg::OnFileDrawline)
	ON_COMMAND_RANGE(ID_RENDER_D3D,ID_RENDER_DDRAW, &CIPCPlayDemoDlg::OnRender)
	ON_WM_MOVING()
	ON_WM_MOVE()
	ON_COMMAND(ID_ENABLE_OPASSIST, &CIPCPlayDemoDlg::OnEnableOpassist)
	ON_BN_CLICKED(IDC_CHECK_NODECODEDELAY, &CIPCPlayDemoDlg::OnBnClickedCheckNodecodedelay)
	ON_BN_CLICKED(IDC_BUTTON_OSD, &CIPCPlayDemoDlg::OnBnClickedButtonOsd)
	ON_BN_CLICKED(IDC_BUTTON_OSD2, &CIPCPlayDemoDlg::OnBnClickedButtonOsd2)
	ON_BN_CLICKED(IDC_BUTTON_REMOVEOSD, &CIPCPlayDemoDlg::OnBnClickedButtonRemoveosd)
	ON_BN_CLICKED(IDC_BUTTON_MERGESCREENS, &CIPCPlayDemoDlg::OnBnClickedButtonMergescreens)
END_MESSAGE_MAP()


CFile *CIPCPlayDemoDlg::m_pVldReport = NULL;

 int __cdecl VLD_REPORT(int reportType, wchar_t *message, int *returnValue)
{
	if (CIPCPlayDemoDlg::m_pVldReport)
	{
		CIPCPlayDemoDlg::m_pVldReport->Write(message, wcslen(message));
	}
	return 0;
}

// CDvoIPCPlayDemoDlg 消息处理程序
 
void CIPCPlayDemoDlg::OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2)
{
	if (nKey2 == VK_F12)
	{
		if (m_pVldReport)
		{
			delete m_pVldReport;
			m_pVldReport = NULL;
		}
		else
		{
			if (PathFileExists(_T("Vld_Report.txt")))
				DeleteFile(_T("Vld_Report.txt"));
			m_pVldReport = new CFile(_T("Vld_Report.txt"), CFile::modeCreate | CFile::modeWrite);
		}
		
// 		VLDSetReportOptions( VLD_OPT_REPORT_TO_DEBUGGER,NULL);
// 		VLDReportLeaks();
	}

	CDialogEx::OnHotKey(nHotKeyId, nKey1, nKey2);
}


BOOL CIPCPlayDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}
	CenterWindow(this);
	m_bitmapMask.LoadBitmap(IDB_BITMAP_MASK);

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	m_pPlayerInfo =make_shared<PlayerInfo>();
	m_wndStatus.SubclassDlgItem(IDC_STATIC_STATUS, this);

	

	SendDlgItemMessage(IDC_COMBO_PICTYPE, CB_SETCURSEL, 1, 0);		// 默认使用JPG截图
	SendDlgItemMessage(IDC_COMBO_PLAYSPEED, CB_SETCURSEL, 8, 0);	// 默认播放速度为1X
	//m_wndBrowseCtrl.SubclassDlgItem(IDC_MFCEDITBROWSE, this);
	m_wndStreamInfo.SubclassDlgItem(IDC_LIST_STREAMINFO, this);
	m_wndStreamInfo.SetExtendedStyle(m_wndStreamInfo.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	int nCol = 0;

	m_wndStreamInfo.InsertColumn(nCol++, _T("项目"), LVCFMT_LEFT, 60);
	m_wndStreamInfo.InsertColumn(nCol++, _T("内容"), LVCFMT_LEFT, 130);
	m_nDiapplayCard = 128;
	ipcplay_GetDisplayAdapterInfo(m_DisplayCard, m_nDiapplayCard);
	if (m_nDiapplayCard > 0)
	{
		for (int i = 0; i < m_nDiapplayCard; i++)
		{
			WCHAR szGUID[64] = { 0 };
			StringFromGUID2(m_DisplayCard[i].DeviceIdentifier, szGUID, 64);
			ipcplay_SetAdapterHAccelW(szGUID, 8);
		}
	}

	int nItem = 0;
	ZeroMemory(m_szListText, sizeof(ListItem) * 16);
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("视频信息"));
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("音频编码"));
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("码      率"));
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("帧      率"));
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("视频缓存"));
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("音频缓存"));
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("码流总长"));
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("连接时长"));
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("播放时长"));
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("录像文件"));
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("录像长度"));
	_tcscpy_s(m_szListText[nItem++].szItemName, 32, _T("录像时长"));
	m_wndStreamInfo.SetItemCount(nItem);
	SendDlgItemMessage(IDC_COMBO_STREAM, CB_SETCURSEL, 0, 0);
	SendDlgItemMessage(IDC_COMBO_ROCATE, CB_SETCURSEL, 0, 0);
	SetDlgItemText(IDC_EDIT_ACCOUNT, _T("admin"));
	SetDlgItemText(IDC_EDIT_PASSWORD, _T("admin"));
	LoadSetting();
	CRect rtClient;
	GetDlgItemRect(IDC_STATIC_FRAME, rtClient);
	m_pVideoWndFrame = new CVideoFrame;
	m_pVideoWndFrame->Create(1024, rtClient, _Row, _Col, this);
	BOOL bRedraw = FALSE;
	UINT nSliderIDArray[] = {
		IDC_SLIDER_SATURATION,
		IDC_SLIDER_BRIGHTNESS,
		IDC_SLIDER_CONTRAST,
		IDC_SLIDER_CHROMA,
		IDC_SLIDER_ZOOMSCALE,
		IDC_SLIDER_VOLUME,
		IDC_SLIDER_PLAYER};
	// 设置滑动块的取舍范围
	for (int i = 0; i < sizeof(nSliderIDArray) / sizeof(UINT); i++)
	{
		EnableDlgItem(nSliderIDArray[i], false);
		SendDlgItemMessage(nSliderIDArray[i],TBM_SETRANGEMIN, bRedraw, 0);
		SendDlgItemMessage(nSliderIDArray[i], TBM_SETRANGEMAX, bRedraw, 100);
	}
	SendDlgItemMessage(IDC_SLIDER_VOLUME, TBM_SETPOS, TRUE, 80);
	SendDlgItemMessage(IDC_SLIDER_ZOOMSCALE, TBM_SETPOS, TRUE, 100);
	SetDlgItemText(IDC_EDIT_PICSCALE, "100");
	EnableDlgItem(IDC_SLIDER_ZOOMSCALE, true);

	InitializeCriticalSection(&m_csListStream);
	m_nHotkeyID = GlobalAddAtom(_T("{A1C54F9F-A835-4fca-88DA-BF0C0DA0E6C5}"));
	
	if (m_nHotkeyID)
	{
		RegisterHotKey(m_hWnd, m_nHotkeyID, MOD_ALT | MOD_CONTROL, VK_F12);
	}
	//int nStatus = VLDSetReportHook(VLD_RPTHOOK_INSTALL, VLD_REPORT);
	SetDlgItemInt(IDC_EDIT_ROW, _Row);
	SetDlgItemInt(IDC_EDIT_COL, _Col);
	m_nPanelCapture = -1;
	CheckDlgButton(IDC_CHECK_REFRESHPLAYER, BST_CHECKED);
	UINT nIDArreayTop[] = {
		IDC_STATIC_ACCOUNT,
		IDC_EDIT_ACCOUNT,
		IDC_STATIC_PWD,
		IDC_EDIT_PASSWORD,
		IDC_STATIC_CAMERA,
		IDC_IPADDRESS,
		IDC_BUTTON_CONNECT,
		IDC_BUTTON_DISCONNECT,
		IDC_STATIC_STREAM,
		IDC_COMBO_STREAM,
		IDC_BUTTON_PLAYSTREAM,
		IDC_BUTTON_RECORD,
		IDC_STATIC_ROW,
		IDC_EDIT_ROW,
		IDC_STATIC_COL,
		IDC_EDIT_COL,
		IDC_CHECK_ENABLEHACCEL 
	};
	

	UINT nIDArrayRight[] = {
		IDC_STATIC_STREAMINFO,
		IDC_CHECK_REFRESHPLAYER,
		IDC_LIST_STREAMINFO,
		IDC_STATIC_COLOR,
		IDC_STATIC_SATURATION,
		IDC_SLIDER_SATURATION,
		IDC_EDIT_SURATION,
		IDC_STATIC_BRIGHTNESS,
		IDC_SLIDER_BRIGHTNESS,
		IDC_EDIT_BRIGHTNESS,
		IDC_STATIC_CONTRAST,
		IDC_SLIDER_CONTRAST,
		IDC_EDIT_CONTRAST,
		IDC_STATIC_CHROMA,
		IDC_SLIDER_CHROMA,
		IDC_EDIT_CHROMA,
		IDC_STATIC_GRAPH,
		IDC_STATIC_PICSCALE,
		IDC_COMBO_PICSCALE,
		IDC_STATIC_ZOOMSCALE,
		IDC_SLIDER_ZOOMSCALE,
		IDC_EDIT_PICSCALE,
		IDC_CHECK_FITWINDOW,
		IDC_BUTTON_SNAPSHOT,
		IDC_COMBO_PICTYPE,
		IDC_BUTTON_TRACECACHE,
		IDC_COMBO_ROCATE,
		IDC_CHECK_SETBORDER,
		IDC_CHECK_DISPLAYRGB,
		IDC_BUTTON_TRACECACHE,
		IDC_STATIC_ROCATEIMAGE
	};
	UINT nIDArrayBottom[] = {
			IDC_STATIC_FILE,
			IDC_EDIT_FILEPATH,
			IDC_BUTTON_PLAYFILE,
			IDC_BUTTON_PAUSE,
			IDC_STATIC_SPEED,
			IDC_STATIC_VIDEOCACHE,
			IDC_EDIT_PLAYCACHE,
			IDC_CHECK_STREAMPLAY,
			IDC_CHECK_ENABLELOG,
			IDC_STATIC_TOTALTIME,
			IDC_BUTTON_STOPBACKWORD,
			IDC_SLIDER_PLAYER,
			IDC_BUTTON_SEEKNEXTFRAME,
			IDC_STATIC_CURTIME,
			IDC_EDIT_PLAYTIME,
			IDC_STATIC_CURFRAME,
			IDC_EDIT_PLAYFRAME,
			IDC_STATIC_CURFPS,
			IDC_EDIT_FPS,
			IDC_CHECK_DISABLEAUDIO,
			IDC_STATIC_VOLUME,
			IDC_SLIDER_VOLUME,
			IDC_STATIC_STATUS,
			IDC_BUTTON_STOPFORWORD,
			IDC_COMBO_PLAYSPEED,
			IDC_CHECK_NODECODEDELAY,
			IDC_CHECK_GSJ,
			IDC_CHECK_ENABLETRANSPARENT
		};
	UINT nIDArrayCenter[] = { IDC_STATIC_FRAME };
	RECT rtDialog;
	GetClientRect(&rtDialog);
	CWnd *pItemWnd =  GetDlgItem(IDC_STATIC_ACCOUNT);
	InitializeCriticalSection(&m_csMapSubclassWnd);
#if _MSC_VER >= 1600
 	SaveWndPosition(nIDArreayTop, sizeof(nIDArreayTop) / sizeof(UINT), DockTop, rtDialog);
 	SaveWndPosition(nIDArrayRight, sizeof(nIDArrayRight) / sizeof(UINT), DockRigth, rtDialog);
 	SaveWndPosition(nIDArrayBottom, sizeof(nIDArrayBottom) / sizeof(UINT), DockBottom, rtDialog);
 	SaveWndPosition(nIDArrayCenter, sizeof(nIDArrayCenter) / sizeof(UINT), DockCenter, rtDialog);
#endif
	CMenu *pMenu = GetMenu()->GetSubMenu(0);
	CMenu *pSubMenu = pMenu->GetSubMenu(5);
	if (pSubMenu)
	{
		if (m_bEnableDDraw)
			pSubMenu->CheckMenuRadioItem(ID_RENDER_D3D, ID_RENDER_DDRAW, ID_RENDER_DDRAW, MF_CHECKED | MF_BYCOMMAND);
		else
			pSubMenu->CheckMenuRadioItem(ID_RENDER_D3D, ID_RENDER_DDRAW, ID_RENDER_D3D, MF_CHECKED | MF_BYCOMMAND);

	}
	CheckDlgButton(IDC_CHECK_FITWINDOW,BST_CHECKED);
	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CIPCPlayDemoDlg::SaveWndPosition(UINT *nDlgItemIDArray, UINT nItemCount, DockType nDock, RECT rtDialogClientRect)
{
	WndPostionInfo wndPos;
	wndPos.Dock = nDock;
	for (int i = 0; i < nItemCount; i++)
	{
		wndPos.hWnd = GetDlgItem(nDlgItemIDArray[i])->m_hWnd;
		wndPos.nID = nDlgItemIDArray[i];
		::GetWindowRect(wndPos.hWnd, &wndPos.rect);
		ScreenToClient(&wndPos.rect);
		switch (wndPos.Dock)
		{
		default:
		case DockTop:		// 无须变动
		{
			wndPos.DockDistance[0] = wndPos.rect.top - rtDialogClientRect.top;
			break;
		}
		case DockLeft:		// 无须变动
		{
			wndPos.DockDistance[0] = wndPos.rect.left - rtDialogClientRect.left;
			break;
		}
		case DockBottom:
		{
			wndPos.DockDistance[0] = rtDialogClientRect.bottom - wndPos.rect.bottom;
			break;
		}
		case DockRigth:
		{
			wndPos.DockDistance[0] = rtDialogClientRect.right - wndPos.rect.right;
			break;
		}
		case DockCenter:
		{
			wndPos.DockDistance[0] = wndPos.rect.left - rtDialogClientRect.left;
			wndPos.DockDistance[1] = wndPos.rect.top - rtDialogClientRect.top;
			wndPos.DockDistance[2] = rtDialogClientRect.right - wndPos.rect.right;
			wndPos.DockDistance[3] = rtDialogClientRect.bottom - wndPos.rect.bottom;
		}
			break;
		}
		
		m_vWndPostionInfo.push_back(wndPos);
	}
}
bool CIPCPlayDemoDlg::LoadSetting()
{
	TCHAR szPath[MAX_PATH] = { 0 };
	GetAppPath(szPath, MAX_PATH);
	_tcscat_s(szPath, MAX_PATH, _T("\\CameraSetting.xml"));
	if (!PathFileExists(szPath))
	{
		return false;
	}
	CMarkup xml;
	if (!xml.Load(szPath))
		return false;
	CString strCursel;
	CString strStreamType;
	CString strStream;
	HWND hCombox = GetDlgItem(IDC_IPADDRESS)->m_hWnd;

	if (xml.FindElem(_T("Configuration")))
	{
		if (xml.FindChildElem(_T("CurSel")))
		{
			xml.IntoElem();
			strCursel = xml.GetAttrib(_T("IP"));
			strStream = xml.GetAttrib(_T("Stream"));
			strStreamType = xml.GetAttrib(_T("StreamType"));
			xml.OutOfElem();
		}
		else
			return false;
		if (xml.FindChildElem(_T("CameraList")))
		{
			xml.IntoElem();

			while (xml.FindChildElem(_T("Camera")))
			{
				xml.IntoElem();
				ComboBox_AddString(hCombox, xml.GetAttrib(_T("IP")));
				xml.OutOfElem();
			}
			xml.OutOfElem();
		}
		else
			return false;
	}
	int nIndex = ComboBox_FindString(hCombox, 0, strCursel);
	if (nIndex == CB_ERR)
		ComboBox_SetCurSel(hCombox, 0);
	else
		ComboBox_SetCurSel(hCombox, nIndex);

	HWND hComboStrem = ::GetDlgItem(m_hWnd, IDC_COMBO_STREAM);
	nIndex = ComboBox_FindString(hComboStrem, 0, (LPCTSTR)strStream);
	ComboBox_SetCurSel(hComboStrem, nIndex);
	return true;
}

bool CIPCPlayDemoDlg::SaveSetting()
{
	TCHAR szPath[MAX_PATH] = { 0 };
	GetAppPath(szPath, MAX_PATH);
	_tcscat_s(szPath, MAX_PATH, _T("\\CameraSetting.xml"));

	HWND hComboBox = GetDlgItem(IDC_IPADDRESS)->m_hWnd;
	TCHAR szStreamType[16] = { 0 };
	TCHAR szText[32] = { 0 };
	TCHAR szStream[16] = { 0 };
	GetDlgItemText(IDC_IPADDRESS, szText, 32);

	GetDlgItemText(IDC_COMBO_STREAM, szStream, 16);
	CMarkup xml;
	TCHAR *szDoc = _T("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n");
	xml.SetDoc(szDoc);

	xml.AddElem(_T("Configuration"));
	xml.AddChildElem(_T("CurSel"));
	xml.IntoElem();
	xml.AddAttrib(_T("IP"), szText);
	xml.AddAttrib(_T("Stream"), szStream);
	xml.OutOfElem();

	xml.IntoElem();	  // Configuration
	xml.AddElem(_T("CameraList"));
	int nCount = ComboBox_GetCount(hComboBox);
	for (int i = 0; i < nCount; i++)
	{
		xml.AddChildElem(_T("Camera"));
		ComboBox_GetLBText(hComboBox, i, szText);
		xml.IntoElem();
		xml.AddAttrib(_T("IP"), szText);
		xml.OutOfElem();
	}
	xml.OutOfElem(); // Configuration

	xml.Save(szPath);
	return true;

}


void CIPCPlayDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CIPCPlayDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rt;
		GetClientRect(&rt);
		int x = (rt.Width() - cxIcon + 1) / 2;
		int y = (rt.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
		
	}
	else
	{
		CPaintDC dc(this); // 用于绘制的设备上下文
		CRect rt;
// 		m_pVideoWndFrame->GetClientRect(&rt);
// 		ClientToScreen(&rt);
// 		
// 		double dfT1 = GetExactTime();
// 		dc.MoveTo(rt.left, rt.right);
// 		int nCenterX = (rt.right + rt.left) / 2;
// 		int nCenterY = (rt.top + rt.bottom) / 2;
// 
// 		int w = 0, h = 0, left_w = 0, left_h = 0;
// 		int nCols = 32, nRows = 32;
// 		w = (rt.right - rt.left) / nCols;
// 		h = (rt.bottom - rt.top) / nRows;
// 
// 		left_w = (rt.right - rt.left) % nCols;
// 		left_h = (rt.bottom - rt.top) % nRows;
// 
// 		int c = 0, r = 0;
// 		CPen newPen(0, 1, RGB(250, 0, 0));
// 		CPen *oldPen = dc.SelectObject(&newPen);
// 		int nAddX = 1;
// 		for (c = 1; c < nRows; c++)
// 		{
// 			dc.MoveTo(0, h * c);
// 			dc.LineTo(rt.right, h * c);
// 		}
// 
// 		for (c = 1; c < nCols; c++)
// 		{
// 			dc.MoveTo(w * c, 0);
// 			dc.LineTo(w * c, rt.bottom);
// 		}
// 		dc.SelectObject(oldPen);
// 
// 		char *szText = "IPCIPCPlaySDK ";
// 		int nCount = strlen(szText);
// 		dc.SetBkColor(RGB(0, 0, 0));
// 		dc.SetTextColor(RGB(255, 0, 0));
// 		CSize sizet = dc.GetTextExtent(szText, nCount);
// 		dc.TextOutA(nCenterX - sizet.cx, nCenterY, szText, nCount);
// 		bool GridMask[32][32] = { 0 };
// 		for (int r = 0; r < nRows; r++)
// 		{
// 			for (int c = 0; c < nCols; c++)
// 			{
// 				if ((r * nCols + c) % 2 != 0)
// 					GridMask[r][c] = true;
// 			}
// 		}
// 
// 		CDC ImageDC, MaskDC;
// 		ImageDC.CreateCompatibleDC(&dc);
// 		ImageDC.SelectObject(&m_bitmapMask);
// 
// 
// 		int morex, morey;
// 		bool bFlag = false;
// 		for (r = 0; r < nRows; r++)
// 		{
// 			for (c = 0; c < nCols; c++)
// 			{
// 				if (GridMask[r][c] == bFlag)
// 				{
// 					dc.BitBlt(r * w, c * h, w, h, &ImageDC, 0, 0, SRCPAINT);
// 					//dc.BitBlt(j * w, i * h, w + morex, h + morey, &ImageDC, 0, 0, SRCPAINT);
// 
// 				}
// 			}
// 			bFlag = (bFlag == true) ? false : true;
// 		}
// 
// 		double dfTS = TimeSpanEx(dfT1);
// 		TraceMsgA("%s TimeSpan = %.3f.\n", __FUNCTION__, dfTS);
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CIPCPlayDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CIPCPlayDemoDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (GetDlgItem(IDC_EDIT_ACCOUNT)->GetSafeHwnd())
	{
		RECT rtDialog;
		GetClientRect(&rtDialog);
		for (vector<WndPostionInfo>::iterator it = m_vWndPostionInfo.begin(); it != m_vWndPostionInfo.end(); it++)
		{
			WndPostionInfo wndPos = (*it);
			RECT rt = wndPos.rect;
			switch (wndPos.Dock)
			{
			default:
			case DockTop:		// 无须变动
			case DockLeft:		// 无须变动
				break;
			case DockBottom:
			{
				if (nType == SIZE_MAXIMIZED)
				{
					rt.bottom = rtDialog.bottom - wndPos.DockDistance[0];
					rt.top = rt.bottom - (wndPos.rect.bottom - wndPos.rect.top);
					::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
				}
				else if (nType == SIZE_RESTORED)
				{
					::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
				}
			}
			break;
			case DockRigth:
			{
				if (nType == SIZE_MAXIMIZED)
				{
					rt.right = rtDialog.right - wndPos.DockDistance[0];
					rt.left = rt.right - (wndPos.rect.right - wndPos.rect.left);
					::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
				}
				else if (nType == SIZE_RESTORED)
				{
					::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
				}
				break;

			}
			case DockCenter:
			{
				if (nType == SIZE_MAXIMIZED)
				{
					rt.left = rtDialog.left + wndPos.DockDistance[0];
					rt.top = rtDialog.top + wndPos.DockDistance[1];
					rt.right = rtDialog.right - wndPos.DockDistance[2];
					rt.bottom = rtDialog.bottom - wndPos.DockDistance[3];					
					::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					if (wndPos.nID == IDC_STATIC_FRAME)
						m_pVideoWndFrame->MoveWindow(&rt, true);
				}
				else if (nType == SIZE_RESTORED)
				{
					::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					if (wndPos.nID == IDC_STATIC_FRAME)
						m_pVideoWndFrame->MoveWindow(&rt, true);
				}
				break;
			}
			}
		}
// 		if (m_pPlayContext)
// 			for (int i = 0; i < m_pPlayContext->nPlayerCount; i++)
// 			{
// 				ipcplay_Reset(m_pPlayContext->hPlayer[i]);
// 			}
	}
}

void CIPCPlayDemoDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	m_bThreadStream = false;
	if (m_hThreadPlayStream && m_hThreadSendStream)
	{
		HANDLE hArray[] = { m_hThreadPlayStream, m_hThreadSendStream };
		WaitForMultipleObjects(2, hArray, TRUE, INFINITE);
		CloseHandle(m_hThreadPlayStream);
		CloseHandle(m_hThreadSendStream);
		m_hThreadPlayStream = NULL;
		m_hThreadSendStream = NULL;
	}

	m_pPlayContext.reset();
	if (m_pVideoWndFrame)
	{
		m_pVideoWndFrame->DestroyWindow();
		delete m_pVideoWndFrame;
	}
	DeleteCriticalSection(&m_csListStream);
	//VLDSetReportHook(VLD_RPTHOOK_REMOVE, VLD_REPORT);
}

void CIPCPlayDemoDlg::OnBnClickedButtonConnect()
{
	CHAR szAccount[32] = { 0 };
	CHAR szPassowd[32] = { 0 };
	CHAR szIPAddress[32] = { 0 };
// 	TCHAR szAccountW[32] = { 0 };
// 	TCHAR szPassowdW[32] = { 0 };
// 	TCHAR szIPAddressW[32] = { 0 };

	GetDlgItemText(IDC_EDIT_ACCOUNT, szAccount, 32);
	GetDlgItemText(IDC_EDIT_ACCOUNT, szPassowd, 32);
	GetDlgItemText(IDC_IPADDRESS, szIPAddress, 32);
	int nStream = SendDlgItemMessage(IDC_COMBO_STREAM, CB_GETCURSEL);
	int nError = 0;
	if (!IsValidIPAddress(szIPAddress))
	{
		AfxMessageBox(_T("请输入一个有效的相机IP"), MB_OK | MB_ICONSTOP);
		return;
	}
	m_pPlayContext =make_shared<PlayerContext>();
	
	m_pPlayContext->pThis = this;
	bool bSucceed = false;

	if (bSucceed)
	{
		_tcscpy_s(m_pPlayContext->szIpAddress, 32, szIPAddress);
		SaveSetting();
		EnableDlgItem(IDC_BUTTON_DISCONNECT, true);
		EnableDlgItem(IDC_BUTTON_CONNECT, false);
		EnableDlgItem(IDC_EDIT_ACCOUNT, false);
		EnableDlgItem(IDC_EDIT_PASSWORD, false);
		EnableDlgItem(IDC_BUTTON_PLAYSTREAM, true);
		EnableDlgItem(IDC_BUTTON_RECORD, true);
		EnableDlgItem(IDC_IPADDRESS, false);
		EnableDlgItem(IDC_BUTTON_PLAYFILE, false);
		EnableDlgItem(IDC_BUTTON_PAUSE, false);
		EnableDlgItem(IDC_COMBO_PLAYSPEED, false);
		EnableDlgItem(IDC_SLIDER_PLAYER, false);
	}
}

void CIPCPlayDemoDlg::OnBnClickedButtonDisconnect()
{
	if (m_pPlayContext)
	{
		HWND hWnd = m_pPlayContext->hWndView;
		m_pPlayContext.reset();
		SetDlgItemText(IDC_BUTTON_PLAYSTREAM, _T("播放码流"));
		EnableDlgItem(IDC_BUTTON_DISCONNECT, false);
		EnableDlgItem(IDC_BUTTON_PLAYSTREAM, false);
		EnableDlgItem(IDC_BUTTON_RECORD, false);
		EnableDlgItem(IDC_BUTTON_CONNECT, true);
		EnableDlgItem(IDC_EDIT_ACCOUNT, true);
		EnableDlgItem(IDC_EDIT_PASSWORD, true);
		EnableDlgItem(IDC_IPADDRESS, true);
		EnableDlgItem(IDC_COMBO_STREAM, true);
		EnableDlgItem(IDC_BUTTON_PLAYFILE, true);
		EnableDlgItem(IDC_BUTTON_PLAYFILE, true);
		EnableDlgItem(IDC_BUTTON_PAUSE, true);
		EnableDlgItem(IDC_COMBO_PLAYSPEED, true);
		EnableDlgItem(IDC_SLIDER_PLAYER, true);
	}
}

void __stdcall CIPCPlayDemoDlg::ExternDCDraw(HWND hWnd, HDC hDc, RECT rt, void *pUserPtr)
{
	double dfT1 = GetExactTime();
	CIPCPlayDemoDlg *pThis = (CIPCPlayDemoDlg*)pUserPtr;
	int nWidth = RectWidth(rt);
	int nHeight = RectHeight(rt);
	CDC dc;
	dc.Attach(hDc);
	CDC ImageDC,MemDC;

	MemDC.CreateCompatibleDC(&dc);
	MemDC.SetBkColor(RGB(0, 0, 0));
	MemDC.FillSolidRect(0, 0, nWidth, nHeight, RGB(255, 255, 255));
	MemDC.SetTextColor(RGB(255, 0, 0));

	CBitmap *pOldMapMemory;
	CBitmap mapMemory;
	mapMemory.CreateCompatibleBitmap(&dc, nWidth, nHeight);
	pOldMapMemory = MemDC.SelectObject(&mapMemory);//加载兼容位图，只有制定了“桌布”尺寸之后，你才能在内存DC上面绘图


	MemDC.MoveTo(rt.left, rt.right);
	int nCenterX = (rt.right + rt.left) / 2;
	int nCenterY = (rt.top + rt.bottom) / 2;

	int w = 0, h = 0, left_w = 0, left_h = 0;
	int nCols = 256, nRows = 256;
	w = (rt.right - rt.left) / nCols;
	h = (rt.bottom - rt.top) / nRows;

	left_w = (rt.right - rt.left) % nCols;
	left_h = (rt.bottom - rt.top) % nRows;

	int c = 0, r = 0;
	CPen newPen(0, 1, RGB(250, 0, 0));
	CPen *oldPen = dc.SelectObject(&newPen);
	int nAddX = 1;
	for (c = 1; c < nRows; c++)
	{
		dc.MoveTo(0, h * c );
		dc.LineTo(rt.right, h * c);
	}

	for (c = 1; c < nCols; c++)
	{
		dc.MoveTo(w * c, 0);
		dc.LineTo(w * c, rt.bottom);
	}
	dc.SelectObject(oldPen);

	char *szText = "IPCIPCPlaySDK ";
	int nCount = strlen(szText);
	
	CSize sizet = dc.GetTextExtent(szText, nCount);
	dc.TextOutA(nCenterX - sizet.cx, nCenterY, szText, nCount);
	bool GridMask[256][256] = { 0 };
	for (int r = 0; r< nRows; r++)
	{
		for (int c = 0; c < nCols; c++)
		{
			if ((r * nCols + c) % 6 != 0)
				GridMask[r][c] = true;
		}
	}
	
	bool bStatus = false;
	bStatus = ImageDC.CreateCompatibleDC(&dc);
	ImageDC.SelectObject(&pThis->m_bitmapMask);

	BITMAP bitmapMap;
	pThis->m_bitmapMask.GetBitmap(&bitmapMap);

// 	bool bFlag = false;
// 	for (r = 0; r < nRows; r++)
// 	{
// 		for (c = 0; c < nCols; c++)
// 		{
// 			if (GridMask[r][ c] == bFlag)
// 			{
// 				bStatus = dc.StretchBlt(r * w, c * h, w, h, &ImageDC, 0, 0, bitmapMap.bmWidth, bitmapMap.bmHeight, SRCCOPY);
// 				//dc.BitBlt(j * w, i * h, w + morex, h + morey, &ImageDC, 0, 0, SRCPAINT);
// 
// 			}
// 		}
// 		//bFlag = (bFlag == true) ? false : true;
// 	}

	// 运行量大副增加
	//bStatus = dc.StretchBlt(0, 0, nWidth, nHeight, &MemDC, 0, 0,nWidth,nHeight, SRCPAINT);
	MemDC.SelectObject(pOldMapMemory);
	ImageDC.DeleteDC();
	MemDC.DeleteDC();
	dc.Detach();
	double dfTS = TimeSpanEx(dfT1);
	TraceMsgA("%s TimeSpan = %.3f.\n", __FUNCTION__, dfTS);
}

void CIPCPlayDemoDlg::OnBnClickedButtonPlaystream()
{
	
}

static TCHAR *szCodecString[] =
{
	_T("未知"),
	_T("H.264"),
	_T("MJPEG"),
	_T("H.265"),
	_T("null"),
	_T("null"),
	_T("null"),
	_T("G711-A"),
	_T("G711-U"),
	_T("G726"),
	_T("AAC")
};

void CIPCPlayDemoDlg::OnBnClickedButtonRecord()
{
// 	if (m_pPlayContext)
// 	{
// 		CWaitCursor Wait;		
// 		if (!m_pPlayContext->pRecFile)
// 		{
// 			int nStream = SendDlgItemMessage(IDC_COMBO_STREAM, CB_GETCURSEL);
// 			if (m_pPlayContext->hStream == -1)
// 			{
// 				int nError = 0;
// 				REAL_HANDLE hStreamHandle = DVO2_NET_StartRealPlay(m_pPlayContext->hUser,
// 					0,
// 					nStream,
// 					0,
// 					0,
// 					NULL,
// 					(fnDVOCallback_RealAVData_T)StreamCallBack,
// 					(void *)m_pPlayContext.get(),
// 					&nError);
// 				if (hStreamHandle == -1)
// 				{
// 					m_wndStatus.SetWindowText(_T("连接码流失败,无法录像"));
// 					m_wndStatus.SetAlarmGllitery();
// 					return;
// 				}
// 				m_pPlayContext->hStream = hStreamHandle;
// 				m_pPlayContext->pThis = this;
// 				EnableDlgItem(IDC_COMBO_STREAM, false);
// 			}
// 
// 			m_pPlayContext->bRecvIFrame = false;
// 			m_pPlayContext->nVideoFrameID = 0;
// 			m_pPlayContext->nAudioFrameID = 0;
// 			TCHAR szPath[MAX_PATH] = { 0 };
// 			TCHAR szFileDir[MAX_PATH] = { 0 };
// 			if (!PathFileExists(m_szRecordPath))
// 				GetAppPath(szFileDir, MAX_PATH);
// 			else
// 				_tcscpy_s(szFileDir, MAX_PATH, m_szRecordPath);
// 
// 			CTime tNow = CTime::GetCurrentTime();
// 			_stprintf_s(m_pPlayContext->szRecFilePath,
// 				_T("%s\\IPC_%s_CH%d_%s.MP4"),
// 				szFileDir,
// 				m_pPlayContext->szIpAddress,
// 				nStream,
// 				tNow.Format(_T("%y%m%d%H%M%S")));
// 			m_pPlayContext->StartRecord();
// 			SetDlgItemText(IDC_BUTTON_RECORD, _T("停止录像"));
// 		}
// 		else
// 		{
// 			m_pPlayContext->StopRecord();
// 			if (!m_pPlayContext->hPlayer[0] && m_pPlayContext->hStream != -1)
// 			{// 未播放码流,并且码流有效,则要断开码流
// 				DVO2_NET_StopRealPlay(m_pPlayContext->hStream);
// 				m_pPlayContext->hStream = -1;
// 				EnableDlgItem(IDC_COMBO_STREAM, true);
// 			}
// 			TraceMsgA("%s LastVideoFrameID = %d\tLastAudioFrameID = %d.\n", __FUNCTION__, m_pPlayContext->nVideoFrameID, m_pPlayContext->nAudioFrameID);
// 			TraceMsgA("%s VideoFrameCount = %d\tAudioFrameCount = %d.\n", __FUNCTION__, m_pPlayContext->pStreamInfo->nVideoFrameCount, m_pPlayContext->pStreamInfo->nAudioFrameCount );
// 			SetDlgItemText(IDC_BUTTON_RECORD, _T("开始录像"));
// 		}
// 	}
}


void CIPCPlayDemoDlg::OnBnClickedButtonPlayfile()
{
	//ipcplay_GetDisplayAdapterInfo();
	bool bEnableWnd = true;
	UINT bIsStreamPlay = IsDlgButtonChecked(IDC_CHECK_STREAMPLAY);
	if (!m_pPlayContext)
	{
		// sws_setColorspaceDetails()设置图像参数
		TCHAR szText[1024] = { 0 };		
		int nFreePanel = 0;		
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
		TCHAR  szFilter[] = _T("录像视频文件 (*.mp4)|*.mp4|H.264录像文件(*.H264)|*.H264|H.265录像文件(*.H265)|*.H265|All Files (*.*)|*.*||");
		TCHAR szExportLog[MAX_PATH] = { 0 };
		CTime tNow = CTime::GetCurrentTime();		
		CFileDialog OpenDataBase(TRUE, _T("*.mp4"), _T(""), dwFlags, (LPCTSTR)szFilter);
		OpenDataBase.m_ofn.lpstrTitle = _T("请选择播放的文件");
		CString strFilePath;
		RocateAngle nRocate = (RocateAngle)SendDlgItemMessage(IDC_COMBO_ROCATE, CB_GETCURSEL);
						
		if (OpenDataBase.DoModal() == IDOK)
		{
			strFilePath = OpenDataBase.GetPathName();
			try
			{
				CFile fpMedia((LPCTSTR)strFilePath, CFile::modeRead);
				IPC_MEDIAINFO MediaHeader;
				if (fpMedia.Read(&MediaHeader, sizeof(IPC_MEDIAINFO)) < sizeof(IPC_MEDIAINFO) ||
					(MediaHeader.nMediaTag != IPC_TAG &&	// 头标志 固定为   0x44564F4D 即字符串"MOVD"
					MediaHeader.nMediaTag != GSJ_TAG))
				{
					m_wndStatus.SetWindowText(_T("指定的文件不是一个有效的IPC录像文件"));
					m_wndStatus.SetAlarmGllitery();
					return;
				}
				fpMedia.GetPosition();
				fpMedia.Close();
				
				SetDlgItemText(IDC_EDIT_FILEPATH, strFilePath);
				m_pPlayContext =make_shared<PlayerContext>();
				m_pPlayContext->hWndView = m_pVideoWndFrame->GetPanelWnd(nFreePanel);
				m_pVideoWndFrame->SetPanelParam(nFreePanel, m_pPlayContext.get());
				
				OnBnClickedButtonDivideframe();
				
				m_pPlayContext->nPlayerCount = m_nCol*m_nRow;
				bool bEnableAudio = (bool)IsDlgButtonChecked(IDC_CHECK_DISABLEAUDIO);
				bool bFitWindow = (bool)IsDlgButtonChecked(IDC_CHECK_FITWINDOW);
				bool bEnableLog = (bool)IsDlgButtonChecked(IDC_CHECK_ENABLELOG);
				bool bEnableHaccel = (bool)IsDlgButtonChecked(IDC_CHECK_ENABLEHACCEL);
				bool bNoDecodeDelay = (bool)IsDlgButtonChecked(IDC_CHECK_NODECODEDELAY);
				bool bOnlyDeoce = (bool)IsDlgButtonChecked(IDC_CHECK_ONLYDECODE);
				if (bIsStreamPlay != BST_CHECKED)
				{
					//m_pPlayContext->hPlayer[0] = ipcplay_OpenFile(m_pPlayContext->hWndView, (CHAR *)(LPCTSTR)strFilePath,(FilePlayProc)PlayerCallBack,m_pPlayContext.get(),bEnableLog?"ipcplaysdk":nullptr);
					if (!bOnlyDeoce )
						m_pPlayContext->hPlayer[0] = ipcplay_OpenFile(m_pPlayContext->hWndView, (CHAR *)(LPCTSTR)strFilePath, NULL, m_pPlayContext.get(), bEnableLog ? "ipcplaysdk" : NULL);
					else
						m_pPlayContext->hPlayer[0] = ipcplay_OpenFile(nullptr, (CHAR *)(LPCTSTR)strFilePath, NULL, m_pPlayContext.get(), bEnableLog ? "ipcplaysdk" : NULL);
					if (!m_pPlayContext->hPlayer[0])
					{
						_stprintf_s(szText, 1024, _T("无法打开%s文件."), strFilePath);
						m_wndStatus.SetWindowText(szText);
						m_wndStatus.SetAlarmGllitery();
						m_pPlayContext.reset();
						return;
					}
 					//m_pYUVFile = new CFile(_T("axis1080p.yuv"), CFile::modeCreate | CFile::modeWrite);
 					//ipcplay_SetCallBack(m_pPlayContext->hPlayer[0], YUVCapture, CaptureYUVProc, this);
					PlayerInfo pi;
					if (ipcplay_GetPlayerInfo(m_pPlayContext->hPlayer[0], &pi) != IPC_Succeed)
					{
						m_wndStatus.SetWindowText(_T("获取文件信息失败."));
						m_wndStatus.SetAlarmGllitery();
					}

					//if (IsDlgButtonChecked(IDC_CHECK_EXTERNDRAW) == BST_CHECKED)
// 					{
// 						ipcplay_SetPixFormat(m_pPlayContext->hPlayer[0], R8G8B8);
// 						ipcplay_SetExternDrawCallBack(m_pPlayContext->hPlayer[0], ExternDCDraw, this);
// 					}
					ipcplay_EnableDDraw(m_pPlayContext->hPlayer[0],m_bEnableDDraw);
					time_t T1 = pi.tTotalTime / 1000;
					int nFloat = pi.tTotalTime - T1 * 1000;
					int nHour = T1 / 3600;
					int nMinute = (T1 - nHour * 3600) / 60;
					int nSecond = T1 % 60;
					TCHAR szPlayText[64] = { 0 };
					_stprintf_s(szPlayText, 64, _T("%02d:%02d:%02d"), nHour, nMinute, nSecond);
					SetDlgItemText(IDC_STATIC_TOTALTIME, szPlayText);
					//ipcplay_SetD3dShared(m_pPlayContext->hPlayer[0], bEnableHaccel);
					//if (bEnableHaccel)
					ipcplay_SetRocateAngle(m_pPlayContext->hPlayer[0], nRocate);
					if (bNoDecodeDelay)
						ipcplay_SetDecodeDelay(m_pPlayContext->hPlayer[0], 0);
					if (ipcplay_Start(m_pPlayContext->hPlayer[0], !bEnableAudio, bFitWindow, bEnableHaccel) != IPC_Succeed)
					{
						m_wndStatus.SetWindowText(_T("无法启动播放器"));
						m_wndStatus.SetAlarmGllitery();
						m_pPlayContext.reset();
						return;
					}
					
					SendDlgItemMessage(IDC_SLIDER_PLAYER, TBM_SETPOS, TRUE, 0);
					
					SetTimer(ID_PLAYEVENT,_PlayInterval, NULL);
					//this->OnBnClickedButtonPause();
				}
				else
				{
					// 创建文件解析句柄
					// 一般在流媒体服务端创建,用于向客户端提供媒体流数据
					m_pPlayContext->hPlayer[0] = ipcplay_OpenFile(NULL, (CHAR *)(LPCTSTR)strFilePath);					
					if (!m_pPlayContext->hPlayer[0])
					{
						_stprintf_s(szText, 1024, _T("无法打开%s文件."), strFilePath);
						m_wndStatus.SetWindowText(szText);
						m_wndStatus.SetAlarmGllitery();
						m_pPlayContext.reset();
						return;
					}
					PlayerInfo pi;
					if (ipcplay_GetPlayerInfo(m_pPlayContext->hPlayer[0], &pi) != IPC_Succeed)
					{
						m_wndStatus.SetWindowText(_T("获取文件信息失败."));
						m_wndStatus.SetAlarmGllitery();
					}

					time_t T1 = pi.tTotalTime / 1000;
					int nFloat = pi.tTotalTime - T1 * 1000;
					int nHour = T1 / 3600;
					int nMinute = (T1 - nHour * 3600) / 60;
					int nSecond = T1 % 60;
					TCHAR szPlayText[64] = { 0 };
					_stprintf_s(szPlayText, 64, _T("%02d:%02d:%02d"), nHour, nMinute, nSecond);
					SetDlgItemText(IDC_STATIC_TOTALTIME, szPlayText);			

					// 创建文件流播放句柄
					// 一般在客户端创建,用于播放服务端发送的媒体流数据
					// 设置较小的播放缓存,移动进度条时,可以及时更新
					m_pPlayContext->hPlayerStream = ipcplay_OpenStream(m_pPlayContext->hWndView, (byte*)&MediaHeader, sizeof(IPC_MEDIAINFO),4);
					if (!m_pPlayContext->hPlayerStream)
					{
						m_wndStatus.SetWindowText(_T("无法打开流播放器."));
						m_wndStatus.SetAlarmGllitery();
						m_pPlayContext.reset();
						return;
					}
					
					if (ipcplay_Start(m_pPlayContext->hPlayerStream, !bEnableAudio, bFitWindow) != IPC_Succeed)
					{
						m_wndStatus.SetWindowText(_T("无法启动流媒体播放器"));
						m_wndStatus.SetAlarmGllitery();
						m_pPlayContext.reset();
						return;
					}
					// 创建读文件帧线程
					m_bThreadStream = true;
					m_hThreadSendStream = (HANDLE)CreateThread(NULL, 0, ThreadSendStream, this, 0, NULL);
					m_hThreadPlayStream = (HANDLE)CreateThread(NULL, 0, ThreadPlayStream, this, 0, NULL);
				}
				int nCurSpeedIndex = 8;
				nCurSpeedIndex = SendDlgItemMessage(IDC_COMBO_PLAYSPEED, CB_GETCURSEL);
				if (nCurSpeedIndex <= 0 && nCurSpeedIndex > 16)
				{
					m_wndStatus.SetWindowText(_T("无效的播放速度,现以原始速度播放."));
					m_wndStatus.SetAlarmGllitery();
					nCurSpeedIndex = 8;
				}
				float fPlayRate = 1.0f;				
				switch (nCurSpeedIndex)
				{
				default:
				case 8:
					break;
				case 0:
					fPlayRate = 1.0 / 32;
					break;
				case 1:
					fPlayRate = 1.0 / 24;
					break;
				case 2:
					fPlayRate = 1.0 / 20;
					break;
				case 3:
					fPlayRate = 1.0 / 16;
					break;
				case 4:
					fPlayRate = 1.0 / 10;
					break;
				case 5:
					fPlayRate = 1.0 / 8;
					break;
				case 6:
					fPlayRate = 1.0 / 4;
					break;
				case 7:
					fPlayRate = 1.0 / 2;
					break;
				case 9:
					fPlayRate = 2.0f;
					break;
				case 10:
					fPlayRate = 4.0f;
					break;
				case 11:
					fPlayRate = 8.0f;
					break;
				case 12:
					fPlayRate = 10.0f;
					break;
				case 13:
					fPlayRate = 16.0f;
					break;
				case 14:
					fPlayRate = 20.0f;
					break;
				case 15:
					fPlayRate = 24.0f;
					break;
				case 16:
					fPlayRate = 32.0f;
					break;
				}
				ipcplay_SetRate(m_pPlayContext->hPlayer[0], fPlayRate);
				m_pPlayContext->pThis = this;
				SetDlgItemText(IDC_BUTTON_PLAYFILE, _T("停止播放"));
				bEnableWnd = false;
				m_dfLastUpdate = GetExactTime();
			}
			catch (CFileException* e)
			{
				TCHAR szErrorMsg[255] = { 0 };
				e->GetErrorMessage(szErrorMsg, 255);
				m_wndStatus.SetWindowText(szErrorMsg);
				m_wndStatus.SetAlarmGllitery();
				return;
			}
		}
	}
	else if (m_pPlayContext->hPlayer[0])
	{
		KillTimer(ID_PLAYEVENT);
		if (bIsStreamPlay == BST_CHECKED)
		{
			m_bThreadStream = false;
			HANDLE hArray[] = { m_hThreadPlayStream, m_hThreadSendStream };
			WaitForMultipleObjects(2,hArray, TRUE, INFINITE);
			CloseHandle(m_hThreadPlayStream);
			CloseHandle(m_hThreadSendStream);
			m_hThreadPlayStream = NULL;
			m_hThreadSendStream = NULL;
		}
		m_pPlayContext.reset();
		SetDlgItemText(IDC_BUTTON_PLAYFILE, _T("播放文件"));
		m_dfLastUpdate = 0.0f;
	}
	// 禁用所有其它码流播放相关按钮
	EnableDlgItem(IDC_BUTTON_DISCONNECT, bEnableWnd);
	EnableDlgItem(IDC_CHECK_STREAMPLAY, bEnableWnd);
	EnableDlgItem(IDC_BUTTON_PLAYSTREAM, bEnableWnd);
	EnableDlgItem(IDC_BUTTON_RECORD, bEnableWnd);
	EnableDlgItem(IDC_BUTTON_CONNECT, bEnableWnd);
	EnableDlgItem(IDC_EDIT_ACCOUNT, bEnableWnd);
	EnableDlgItem(IDC_EDIT_PASSWORD, bEnableWnd);
	EnableDlgItem(IDC_IPADDRESS, bEnableWnd);
	EnableDlgItem(IDC_COMBO_STREAM, bEnableWnd);
	EnableDlgItem(IDC_COMBO_ROCATE, bEnableWnd);
	EnableDlgItems(m_hWnd, !bEnableWnd, 7,
					IDC_SLIDER_SATURATION,
					IDC_SLIDER_BRIGHTNESS,
					IDC_SLIDER_CONTRAST,
					IDC_SLIDER_CHROMA,
					IDC_SLIDER_ZOOMSCALE,
					IDC_SLIDER_VOLUME, 
					IDC_SLIDER_PLAYER);
}

void CIPCPlayDemoDlg::PlayerCallBack(IPC_PLAYHANDLE hPlayHandle, void *pUserPtr)
{
	PlayerContext *pContext = (PlayerContext *)pUserPtr;
	if (pContext->pThis)
	{
		CIPCPlayDemoDlg *pDlg = (CIPCPlayDemoDlg *)pContext->pThis;
		if (TimeSpanEx(pDlg->m_dfLastUpdate) < 0.200f)
			return;
		pDlg->m_dfLastUpdate = GetExactTime();
		int nDvoError = ipcplay_GetPlayerInfo(hPlayHandle, pDlg->m_pPlayerInfo.get());
 		if (nDvoError == IPC_Succeed ||
 			nDvoError == IPC_Error_FileNotExist)
 			pDlg->PostMessage(WM_UPDATE_PLAYINFO, (WPARAM)pDlg->m_pPlayerInfo.get(), (LPARAM)nDvoError);
		if (pDlg->m_pPlayerInfo->bFilePlayFinished)
		{
			pDlg->OnBnClickedButtonPlayfile();
		}
	}

}

LRESULT CIPCPlayDemoDlg::OnUpdatePlayInfo(WPARAM w, LPARAM l)
{ 
	PlayerInfo  *fpi = (PlayerInfo *)w;
	if (fpi )
	{
		if (l != IPC_Error_FileNotExist)
		{
			int nSlidePos = 0;
			if (fpi->nTotalFrames > 0)
				nSlidePos = (int)(100 * (double)fpi->nCurFrameID / fpi->nTotalFrames);
			SendDlgItemMessage(IDC_SLIDER_PLAYER, TBM_SETPOS, TRUE, nSlidePos);
		}

		time_t T1 = fpi->tCurFrameTime / 1000;
		int nFloat = fpi->tCurFrameTime - T1 * 1000;
		int nHour = T1 / 3600;
		int nMinute = (T1 - nHour * 3600) / 60;
		int nSecond = T1 % 60;
		TCHAR szPlayText[64] = { 0 };
		_stprintf_s(szPlayText, 64, _T("%02d:%02d:%02d.%03d"), nHour, nMinute, nSecond, nFloat);
		SetDlgItemText(IDC_EDIT_PLAYTIME, szPlayText);
		_stprintf_s(szPlayText, 64, _T("%d"), fpi->nCurFrameID);
 		SetDlgItemText(IDC_EDIT_PLAYFRAME, szPlayText);

		_stprintf_s(szPlayText, 64, _T("%d"), fpi->nCacheSize);
		SetDlgItemText(IDC_EDIT_PLAYCACHE, szPlayText);

		_stprintf_s(szPlayText, 64, _T("%d"), fpi->nPlayFPS);
		SetDlgItemText(IDC_EDIT_FPS, szPlayText);
		m_dfLastUpdate = GetExactTime();

		if (fpi->nVideoCodec >= CODEC_UNKNOWN && fpi->nVideoCodec <= CODEC_AAC)
			_stprintf_s(m_szListText[Item_VideoInfo].szItemText, 256, _T("%s(%dx%d)"), szCodecString[fpi->nVideoCodec + 1], fpi->nVideoWidth, fpi->nVideoHeight);
		else
			_stprintf_s(m_szListText[Item_VideoInfo].szItemText, 256, _T("N/A"), szCodecString[fpi->nVideoCodec + 1]);

		if (fpi->nAudioCodec >= CODEC_UNKNOWN && fpi->nAudioCodec <= CODEC_AAC)
			_tcscpy(m_szListText[Item_ACodecType].szItemText, szCodecString[fpi->nAudioCodec + 1]);
		else
			_tcscpy(m_szListText[Item_ACodecType].szItemText, _T("N/A"));
		_stprintf_s(m_szListText[Item_VFrameCache].szItemText, 64, _T("%d"), fpi->nCacheSize);
		_stprintf_s(m_szListText[Item_AFrameCache].szItemText, 64, _T("%d"), fpi->nCacheSize2);

		if (fpi->tTimeEplased > 0)
		{
			CTimeSpan tSpan(fpi->tTimeEplased / 1000);
			_stprintf_s(m_szListText[Item_PlayedTime].szItemText, 64, _T("%02d:%02d:%02d"), tSpan.GetHours(), tSpan.GetMinutes(), tSpan.GetSeconds());
		}
		else
			_tcscpy_s(m_szListText[Item_PlayedTime].szItemText, 64, _T("00:00:00"));
		m_wndStreamInfo.Invalidate();
	}
	return 0;
}

LRESULT CIPCPlayDemoDlg::OnUpdateStreamInfo(WPARAM w, LPARAM l)
{
	if (!m_pPlayContext)
		return 0;
	TCHAR szText[128] = { 0 };
	
	IPC_CODEC nVideoCodec = CODEC_UNKNOWN;
	IPC_CODEC nAudioCodec = CODEC_UNKNOWN;
	PlayerInfo fpi;
	int nResult = ipcplay_GetPlayerInfo(m_pPlayContext->hPlayer[0], &fpi);
	if (nResult == IPC_Succeed ||
		nResult == IPC_Error_SummaryNotReady ||	// 文件播放概要信息尚未做准备好
		nResult == IPC_Error_FileNotExist)
	{
		if (fpi.nVideoCodec >= CODEC_UNKNOWN && fpi.nVideoCodec <= CODEC_AAC)
			_stprintf_s(m_szListText[Item_VideoInfo].szItemText, 256, _T("%s(%dx%d)"), szCodecString[fpi.nVideoCodec + 1], fpi.nVideoWidth, fpi.nVideoHeight);
		else
			_stprintf_s(m_szListText[Item_VideoInfo].szItemText, 256, _T("M/A"), szCodecString[fpi.nVideoCodec + 1]);
		
		if (fpi.nAudioCodec >= CODEC_UNKNOWN && fpi.nAudioCodec <= CODEC_AAC)
			_tcscpy(m_szListText[Item_ACodecType].szItemText, szCodecString[fpi.nAudioCodec + 1]);
		else
			_tcscpy(m_szListText[Item_ACodecType].szItemText, _T("N/A"));
		_stprintf_s(m_szListText[Item_VFrameCache].szItemText, 64, _T("%d"), fpi.nCacheSize);
		_stprintf_s(m_szListText[Item_AFrameCache].szItemText, 64, _T("%d"), fpi.nCacheSize2);
		
		if (fpi.tTimeEplased > 0)
		{
			CTimeSpan tSpan(fpi.tTimeEplased / 1000);
			_stprintf_s(m_szListText[Item_PlayedTime].szItemText, 64, _T("%02d:%02d:%02d"), tSpan.GetHours(), tSpan.GetMinutes(), tSpan.GetSeconds());
		}
		else
			_tcscpy_s(m_szListText[Item_PlayedTime].szItemText, 64, _T("00:00:00"));
	}
	
	int nStreamRate = m_pPlayContext->pStreamInfo->GetVideoCodeRate();
	_stprintf_s(m_szListText[Item_StreamRate].szItemText, 64, _T("%d Kbps"), nStreamRate);
	_stprintf_s(m_szListText[Item_FrameRate].szItemText, 64, _T("%d fps"), m_pPlayContext->nFPS);
	_stprintf_s(m_szListText[Item_TotalStream].szItemText, 64, _T("%d KB"), (m_pPlayContext->pStreamInfo->nVideoBytes + m_pPlayContext->pStreamInfo->nAudioBytes) / 1024);

	if (m_pPlayContext->pStreamInfo->tFirstTime > 0)
	{
		time_t nRecTime = time(0) * 1000 - m_pPlayContext->pStreamInfo->tFirstTime;
		CTimeSpan tSpan(nRecTime / 1000);
		_stprintf_s(m_szListText[Item_ConnectTime].szItemText, 64, _T("%02d:%02d:%02d"), tSpan.GetHours(), tSpan.GetMinutes(), tSpan.GetSeconds());
	}
	else
		_tcscpy_s(m_szListText[Item_ConnectTime].szItemText, 64, _T("00:00:00"));

	if (m_pPlayContext->pRecFile)
	{
		_tcscpy_s(m_szListText[Item_RecFile].szItemText, 256,m_pPlayContext->szRecFilePath);
		
		time_t nRecTime = time(0) - m_pPlayContext->tRecStartTime;
		CTimeSpan tSpan(nRecTime);
		_stprintf_s(m_szListText[Item_RecTime].szItemText, 64, _T("%02d:%02d:%02d"), tSpan.GetHours(), tSpan.GetMinutes(), tSpan.GetSeconds());
		_stprintf_s(m_szListText[Item_FileLength].szItemText, _T("%d KB"), m_pPlayContext->pRecFile->GetLength() / 1024);
	}
	else
	{
		_tcscpy_s(m_szListText[Item_RecFile].szItemText, 256, _T("暂未录像"));
		_tcscpy_s(m_szListText[Item_RecTime].szItemText, 256, _T("00:00:00"));
		_tcscpy_s(m_szListText[Item_FileLength].szItemText, 256, _T("暂未录像"));
	}
	m_dfLastUpdate = GetExactTime();
	m_wndStreamInfo.Invalidate();

	return 0;
}

void CIPCPlayDemoDlg::OnNMClickListConnection(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
}

void CIPCPlayDemoDlg::OnNMCustomdrawListStreaminfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW *pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
	*pResult = CDRF_DODEFAULT;
	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	}
	else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage)
	{
		int    nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);
		if (0 == pLVCD->iSubItem)
		{
			pLVCD->clrTextBk = RGB(224, 224, 224);
		}
		else
			pLVCD->clrTextBk = RGB(250, 250, 250);

		*pResult = CDRF_DODEFAULT;
	}
}

void CIPCPlayDemoDlg::OnBnClickedCheckEnableaudio()
{
	if (m_pPlayContext && m_pPlayContext->hPlayer[0])
	{
		bool bEnable = (bool)IsDlgButtonChecked(IDC_CHECK_DISABLEAUDIO);
		if (ipcplay_EnableAudio(m_pPlayContext->hPlayer[0], !bEnable) != IPC_Succeed)
		{
			m_wndStatus.SetWindowText(_T("开关音频失败."));
			m_wndStatus.SetAlarmGllitery();
		}
	}
}

void CIPCPlayDemoDlg::OnBnClickedCheckFitwindow()
{
	if (m_pPlayContext )
	{
		//for (int i = 0; i < m_pPlayContext->nPlayerCount; i++)
		{
			bool bFitWindow = (bool)IsDlgButtonChecked(IDC_CHECK_FITWINDOW);
// 			if (!m_pPlayContext->hPlayer[i])
// 				continue;
			if (ipcplay_FitWindow(m_pPlayContext->hPlayer[0], bFitWindow) != IPC_Succeed)
			{
				m_wndStatus.SetWindowText(_T("调整视频显示方式失败."));
				m_wndStatus.SetAlarmGllitery();
				return;
			}
			::InvalidateRect(m_pVideoWndFrame->GetPanelWnd(0), NULL, true);
		}
	}
}

#define _Var(P)	#P

void CIPCPlayDemoDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	UINT nSliderID = pScrollBar->GetDlgCtrlID();	
	nPos = SendDlgItemMessage(nSliderID, TBM_GETPOS, 0, 0l);
	switch (nSliderID)
	{
	case IDC_SLIDER_SATURATION:
		TraceMsgA("SliderID:%s\tPos = %d.\n", _Var(IDC_SLIDER_SATURATION),nPos);
		break;
	case IDC_SLIDER_BRIGHTNESS:
		TraceMsgA("SliderID:%s\tPos = %d.\n", _Var(IDC_SLIDER_BRIGHTNESS), nPos);
		break;
	case IDC_SLIDER_CONTRAST:
		TraceMsgA("SliderID:%s\tPos = %d.\n", _Var(IDC_SLIDER_CONTRAST), nPos);
		break;
	case IDC_SLIDER_CHROMA:
		TraceMsgA("SliderID:%s\tPos = %d.\n", _Var(IDC_SLIDER_CHROMA), nPos);
		break;
	case IDC_SLIDER_ZOOMSCALE:
	{
		CRect  rtVideoFrame;
		GetDlgItemRect(IDC_STATIC_FRAME, rtVideoFrame);
		int nWidth = rtVideoFrame.Width();
		int nHeight = rtVideoFrame.Height();
		int nCenterX = rtVideoFrame.left +  nWidth/ 2;
		int nCenterY = rtVideoFrame.top + nHeight / 2;
		int nNewWidth = nPos*nWidth / 100;
		int nNewHeight = nPos*nHeight / 100;
		rtVideoFrame.left = nCenterX - nNewWidth / 2;
		rtVideoFrame.right = rtVideoFrame.left + nNewWidth;
		rtVideoFrame.top = nCenterY - nNewHeight / 2;
		rtVideoFrame.bottom = rtVideoFrame.top + nNewHeight;
		m_pVideoWndFrame->MoveWindow(&rtVideoFrame);
		SetDlgItemInt(IDC_EDIT_PICSCALE, nPos);

		TraceMsgA("SliderID:%s\tPos = %d.\n", _Var(IDC_SLIDER_ZOOMSCALE), nPos);
		break;
	}
	case IDC_SLIDER_VOLUME:
	{
		if (!m_pPlayContext || !m_pPlayContext->hPlayer[0])
		{
			CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
			return;
		}
		ipcplay_SetVolume(m_pPlayContext->hPlayer[0], nPos);
	}
		break;
	case IDC_SLIDER_PLAYER:
	{
		if (!m_pPlayContext || !m_pPlayContext->hPlayer[0])
		{
			CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
			return;
		}
		DeclareRunTime();
		switch (nSBCode)
		{
		case SB_PAGEDOWN:
		case SB_PAGEUP:
		case SB_THUMBTRACK:
			KillTimer(ID_PLAYEVENT);
			break;
		case SB_THUMBPOSITION:
		case SB_ENDSCROLL:
		{
			SaveWaitTime();
			SaveRunTime();
			int nPos = SendDlgItemMessage(IDC_SLIDER_PLAYER, TBM_GETPOS);
			SaveRunTime();
			//TraceMsgA("%s Player Slide Pos = %d.\n", __FUNCTION__, nPos);
			int nTotalFrames = 0;
			PlayerInfo pi;
			UINT bIsStreamPlay = IsDlgButtonChecked(IDC_CHECK_STREAMPLAY);
			SaveRunTime();
			if (ipcplay_GetPlayerInfo(m_pPlayContext->hPlayer[0], &pi) == IPC_Succeed)
			{
				SaveRunTime();
				int nSeekFrame = pi.nTotalFrames*nPos / 100;
				bool bUpdate = true;
				if (bIsStreamPlay)
				{
					ipcplay_ClearCache(m_pPlayContext->hPlayerStream);
					SaveRunTime();
					CAutoLock lock(&m_csListStream);
					m_listStream.clear();
					bUpdate = false;		// 流播放无法通过这种方式刷新画面
				}
				SaveRunTime();
				ipcplay_SeekFrame(m_pPlayContext->hPlayer[0], nSeekFrame, bUpdate);
				SaveRunTime();
			}
			m_bClickPlayerSlide = true;
			SetTimer(ID_PLAYEVENT, _PlayInterval, NULL);
		}
			break;
		default:
			break;
		}
	}
		break;
	default:
		break;
	}

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CIPCPlayDemoDlg::OnBnClickedButtonSnapshot()
{
	if (m_pPlayContext && m_pPlayContext->hPlayer[0])
	{
		CWaitCursor Wait;
		TCHAR szPath[1024];
		GetAppPath(szPath, 1024);
		_tcscat_s(szPath, 1024, _T("\\ScreenSave"));
		if (!PathFileExists(szPath))
		{
			if (!CreateDirectory(szPath,NULL))
			{
				m_wndStatus.SetWindowText(_T("无法创建保存截图文件的目录,请确主是否有足够的权限."));
				m_wndStatus.SetAlarmGllitery();
				return;
			}
		}
		TCHAR szFileName[64] = { 0 };
		SYSTEMTIME systime;
		GetLocalTime(&systime);

		TCHAR *szFileExt[] = { _T("bmp"), _T("jpg"), _T("tga"), _T("png") };
		int nPicType = SendDlgItemMessage(IDC_COMBO_PICTYPE, CB_GETCURSEL);
		if (nPicType > 3 || nPicType < 0)
		{
			m_wndStatus.SetWindowText(_T("不支持的图片格式,请选择正确的图片格式."));
			m_wndStatus.SetAlarmGllitery();
			return;
		}
		// 生成文件名
		_stprintf_s(szFileName, 64,
			_T("\\SnapShot_%04d%02d%02d_%02d%02d%02d_%03d.%s"),
			systime.wYear, systime.wMonth, systime.wDay,
			systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds,
			szFileExt[nPicType]);
		_tcscat_s(szPath, 1024, szFileName);
		if (ipcplay_SnapShot(m_pPlayContext->hPlayer[0], szPath, (SNAPSHOT_FORMAT)nPicType) == IPC_Succeed)
		{
			TCHAR szText[1024];
			_stprintf_s(szText, 1024, _T("已经生成截图:%s."), szPath);
			m_wndStatus.SetWindowText(szText);
			m_wndStatus.SetOkGllitery();
		}
		else
		{
			m_wndStatus.SetWindowText(_T("播放器尚未启动,无法截图."));
			m_wndStatus.SetAlarmGllitery();
		}
	}
	else
	{
		m_wndStatus.SetWindowText(_T("播放器尚未启动."));
		m_wndStatus.SetAlarmGllitery();
	}
}


void CIPCPlayDemoDlg::OnCbnSelchangeComboPlayspeed()
{
	UINT bIsStreamPlay = IsDlgButtonChecked(IDC_CHECK_STREAMPLAY);
	if (m_pPlayContext && m_pPlayContext->hPlayer[0])
	{
		int nCurSecl = SendDlgItemMessage(IDC_COMBO_PLAYSPEED, CB_GETCURSEL);
		if (nCurSecl <= 0 && nCurSecl > 16)
		{
			m_wndStatus.SetWindowText(_T("无效的播放速度."));
			m_wndStatus.SetAlarmGllitery();
			return;
		}
		float fPlayRate = 1.0f;		
		switch (nCurSecl)
		{
		default:
		case 8:
			break;
		case 0:
			fPlayRate = 1.0 / 32;
			break;
		case 1:
			fPlayRate = 1.0 / 24;
			break;
		case 2:
			fPlayRate = 1.0 / 20;
			break;
		case 3:
			fPlayRate = 1.0 / 16;
			break;
		case 4:
			fPlayRate = 1.0 / 10;
			break;
		case 5:
			fPlayRate = 1.0 / 8;
			break;
		case 6:
			fPlayRate = 1.0 / 4;
			break;
		case 7:
			fPlayRate = 1.0 / 2;
			break;		
		case 9:
			fPlayRate = 2.0f;
			break;
		case 10:
			fPlayRate = 4.0f;
			break;
		case 11:
			fPlayRate = 8.0f;
			break;
		case 12:
			fPlayRate = 10.0f;
			break;
		case 13:
			fPlayRate = 16.0f;
			break;
		case 14:
			fPlayRate = 20.0f;
			break;
		case 15:
			fPlayRate = 24.0f;
			break;
		case 16:
			fPlayRate = 32.0f;
			break;
		}
		if (bIsStreamPlay)
			ipcplay_SetRate(m_pPlayContext->hPlayerStream, fPlayRate);
		else
			ipcplay_SetRate(m_pPlayContext->hPlayer[0], fPlayRate);
	}
	else
	{
		m_wndStatus.SetWindowText(_T("播放器尚未启动."));
		m_wndStatus.SetAlarmGllitery();
	}
}


void CIPCPlayDemoDlg::OnBnClickedButtonPause()
{
	UINT bIsStreamPlay = IsDlgButtonChecked(IDC_CHECK_STREAMPLAY);
	if (m_pPlayContext && m_pPlayContext->hPlayer[0])
	{
		if (bIsStreamPlay)
		{
			ipcplay_Pause(m_pPlayContext->hPlayerStream);
		}
		else
			ipcplay_Pause(m_pPlayContext->hPlayer[0]);
		m_bPuased = !m_bPuased;
		if (m_bPuased)
			SetDlgItemText(IDC_BUTTON_PAUSE, _T("继续播放"));
		else
			SetDlgItemText(IDC_BUTTON_PAUSE, _T("暂停播放"));
	}
	else
	{
		m_wndStatus.SetWindowText(_T("播放器尚未启动."));
		m_wndStatus.SetAlarmGllitery();
	}
}


void CIPCPlayDemoDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == ID_PLAYEVENT &&
		m_pPlayContext->hPlayer[0] && m_bRefreshPlayer)
	{
		PlayerInfo pi;
		PlayerInfo *fpi = &pi;
		int nError = ipcplay_GetPlayerInfo(m_pPlayContext->hPlayer[0] ,& pi);
		if (nError != IPC_Error_FileNotExist)
		{
			int nSlidePos = 0;
			if (fpi->nTotalFrames > 0)
				nSlidePos = (int)(100 * (double)fpi->nCurFrameID / fpi->nTotalFrames);
			if (m_bClickPlayerSlide)
			{
				//TraceMsgA("%s New Player Slider Pos = %d.\n", __FUNCTION__, nSlidePos);
				m_bClickPlayerSlide = false;
			}
			SendDlgItemMessage(IDC_SLIDER_PLAYER, TBM_SETPOS, TRUE, nSlidePos);
			//TraceMsgA("%s nSlidePos = %d.\n", __FUNCTION__,nSlidePos);
		}

		time_t T1 = fpi->tCurFrameTime / 1000;
		int nFloat = fpi->tCurFrameTime - T1 * 1000;
		int nHour = T1 / 3600;
		int nMinute = (T1 - nHour * 3600) / 60;
		int nSecond = T1 % 60;
		TCHAR szPlayText[64] = { 0 };
		_stprintf_s(szPlayText, 64, _T("%02d:%02d:%02d.%03d"), nHour, nMinute, nSecond, nFloat);
		SetDlgItemText(IDC_EDIT_PLAYTIME, szPlayText);
		_stprintf_s(szPlayText, 64, _T("%d"), fpi->nCurFrameID);
		SetDlgItemText(IDC_EDIT_PLAYFRAME, szPlayText);

		_stprintf_s(szPlayText, 64, _T("%d"), fpi->nCacheSize);
		SetDlgItemText(IDC_EDIT_PLAYCACHE, szPlayText);

		_stprintf_s(szPlayText, 64, _T("%d"), fpi->nPlayFPS);
		SetDlgItemText(IDC_EDIT_FPS, szPlayText);
		m_dfLastUpdate = GetExactTime();

		if (fpi->nVideoCodec >= CODEC_UNKNOWN && fpi->nVideoCodec <= CODEC_AAC)
			_stprintf_s(m_szListText[Item_VideoInfo].szItemText, 256, _T("%s(%dx%d)"), szCodecString[fpi->nVideoCodec + 1], fpi->nVideoWidth, fpi->nVideoHeight);
		else
			_stprintf_s(m_szListText[Item_VideoInfo].szItemText, 256, _T("M/A"), szCodecString[fpi->nVideoCodec + 1]);

		if (fpi->nAudioCodec >= CODEC_UNKNOWN && fpi->nAudioCodec <= CODEC_AAC)
			_tcscpy(m_szListText[Item_ACodecType].szItemText, szCodecString[fpi->nAudioCodec + 1]);
		else
			_tcscpy(m_szListText[Item_ACodecType].szItemText, _T("N/A"));
		_stprintf_s(m_szListText[Item_VFrameCache].szItemText, 64, _T("%d"), fpi->nCacheSize);
		_stprintf_s(m_szListText[Item_AFrameCache].szItemText, 64, _T("%d"), fpi->nCacheSize2);

		if (fpi->tTimeEplased > 0)
		{
			CTimeSpan tSpan(fpi->tTimeEplased / 1000);
			_stprintf_s(m_szListText[Item_PlayedTime].szItemText, 64, _T("%02d:%02d:%02d"), tSpan.GetHours(), tSpan.GetMinutes(), tSpan.GetSeconds());
		}
		else
			_tcscpy_s(m_szListText[Item_PlayedTime].szItemText, 64, _T("00:00:00"));
		m_wndStreamInfo.Invalidate();
		if (pi.bFilePlayFinished)
		{
			OnBnClickedButtonPlayfile();
		}

	}
	CDialogEx::OnTimer(nIDEvent);
}

void CIPCPlayDemoDlg::OnBnClickedButtonTracecache()
{
	if (m_pPlayContext && m_pPlayContext->nPlayerCount)
	{
		PlayerInfo pi;
		for (int i = 0; i < m_pPlayContext->nPlayerCount;i ++)
			if (m_pPlayContext->hPlayer[i])
			{
				ZeroMemory(&pi, sizeof(PlayerInfo));
				ipcplay_GetPlayerInfo(m_pPlayContext->hPlayer[i], &pi);
				TraceMsgA("%s Play[%d]:Size = %dx%d\tVideo cache = %d\tAudio Cache = %d.\n", __FUNCTION__, i, pi.nVideoWidth,pi.nVideoHeight,pi.nCacheSize, pi.nCacheSize2);
			}
	}
}

void CIPCPlayDemoDlg::OnLvnGetdispinfoListStreaminfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	if (pDispInfo->item.iItem >= 0)
	{
		int nItem = pDispInfo->item.iItem;
		int nSubItem = pDispInfo->item.iSubItem;
		switch (nSubItem)
		{
		case 0:
			pDispInfo->item.pszText = m_szListText[nItem].szItemName;
			break;
		case 1:
			pDispInfo->item.pszText = m_szListText[nItem].szItemText;
			break;
		default:
			break;
		}
	}
	*pResult = 0;
}


void CIPCPlayDemoDlg::OnBnClickedButtonStopbackword()
{
	// 步进5秒
	int nTotalFrames = 0;
	PlayerInfo pi;
	UINT bIsStreamPlay = IsDlgButtonChecked(IDC_CHECK_STREAMPLAY);
	if (ipcplay_GetPlayerInfo(m_pPlayContext->hPlayer[0], &pi) == IPC_Succeed)
	{
		if (pi.tCurFrameTime < 5000)
			return;
		int nSeekTime = pi.tCurFrameTime - 5000;		

		TraceMsgA("%s\nnTotalFrames = %d\ttTotalTime = %I64d\tnCurFrameID = %d\ttCurFrameTime = %I64d\ttTimeEplased = %I64d.\n",
			__FUNCTION__,
			pi.nTotalFrames,
			pi.tTotalTime,
			pi.nCurFrameID,
			pi.tCurFrameTime,
			pi.tTimeEplased);
		bool bUpdate = true;
		if (bIsStreamPlay)
		{
			ipcplay_ClearCache(m_pPlayContext->hPlayerStream);
			CAutoLock lock(&m_csListStream);
			m_listStream.clear();
			bUpdate = false;		// 流播放无法通过这种方式刷新画面
		}

		ipcplay_SeekTime(m_pPlayContext->hPlayer[0], nSeekTime, bUpdate);
	}
}


void CIPCPlayDemoDlg::OnBnClickedButtonStopforword()
{
	// 步进5秒
	int nTotalFrames = 0;
	PlayerInfo pi;
	UINT bIsStreamPlay = IsDlgButtonChecked(IDC_CHECK_STREAMPLAY);
	if (ipcplay_GetPlayerInfo(m_pPlayContext->hPlayer[0], &pi) == IPC_Succeed)
	{
		int nSeekTime = pi.tCurFrameTime + 5000;
		if (nSeekTime > pi.tTotalTime)
			return;
		
		TraceMsgA("%s\nnTotalFrames = %d\ttTotalTime = %I64d\tnCurFrameID = %d\ttCurFrameTime = %I64d\ttTimeEplased = %I64d.\n",
			__FUNCTION__,
			pi.nTotalFrames,
			pi.tTotalTime,
			pi.nCurFrameID,
			pi.tCurFrameTime,
			pi.tTimeEplased);
		bool bUpdate = true;
		if (bIsStreamPlay)
		{
			ipcplay_ClearCache(m_pPlayContext->hPlayerStream);
			CAutoLock lock(&m_csListStream);
			m_listStream.clear();
			bUpdate = false;		// 流播放无法通过这种方式刷新画面
		}

		ipcplay_SeekTime(m_pPlayContext->hPlayer[0], nSeekTime, bUpdate);
	}
}


void CIPCPlayDemoDlg::OnBnClickedButtonSeeknextframe()
{
	// 步进5秒
	int nTotalFrames = 0;
	PlayerInfo pi;
	UINT bIsStreamPlay = IsDlgButtonChecked(IDC_CHECK_STREAMPLAY);
	if (ipcplay_GetPlayerInfo(m_pPlayContext->hPlayer[0], &pi) == IPC_Succeed)
	{
		
		ipcplay_SeekNextFrame(m_pPlayContext->hPlayer[0]);
	}
}


void CIPCPlayDemoDlg::OnBnClickedCheckEnablelog()
{
	if (m_pPlayContext && m_pPlayContext->hPlayer[0])
	{
		if (IsDlgButtonChecked(IDC_CHECK_ENABLELOG) == BST_CHECKED)
			EnableLog(m_pPlayContext->hPlayer[0],"IPCIPCPlaySdk");
		else
			EnableLog(m_pPlayContext->hPlayer[0], NULL);
	}
}



void CIPCPlayDemoDlg::OnNMReleasedcaptureSliderPlayer(NMHDR *pNMHDR, LRESULT *pResult)
{
	//TraceMsgA(_T("Code = %d\thWndFrom = %08X\tidFrom = %d.\n"), pNMHDR->code, pNMHDR->hwndFrom, pNMHDR->idFrom);
	*pResult = 0;
}


// void CDvoIPCPlayDemoDlg::OnBnClickedCheckRefreshplayer()
// {
// 	m_bRefreshPlayer = IsDlgButtonChecked(IDC_CHECK_REFRESHPLAYER);
// }

void CIPCPlayDemoDlg::OnBnClickedCheckEnablehaccel()
{
	 
}

void CIPCPlayDemoDlg::OnBnClickedCheckRefreshplayer()
{
	m_bRefreshPlayer = IsDlgButtonChecked(IDC_CHECK_REFRESHPLAYER);
}

void CIPCPlayDemoDlg::OnBnClickedCheckSetborder()
{
	RECT rtBorder = { 80, 80, 80, 80 };
	if (m_pPlayContext && m_pPlayContext->hPlayer[0])
	{
// 		if (IsDlgButtonChecked(IDC_CHECK_SETBORDER) == BST_CHECKED)
// 			ipcplay_SetBorderRect(m_pPlayContext->hPlayer[0], m_pPlayContext->hWndView, rtBorder);
// 		else
// 			ipcplay_RemoveBorderRect(m_pPlayContext->hPlayer[0], m_pPlayContext->hWndView);
	}
}


// void CIPCPlayDemoDlg::OnBnClickedCheckExterndraw()
// {
// 	if (m_pPlayContext && m_pPlayContext->hPlayer[0])
// 	{
// 		if (IsDlgButtonChecked(IDC_CHECK_EXTERNDRAW) == BST_CHECKED)
// 			ipcplay_SetExternDrawCallBack(m_pPlayContext->hPlayer[0], ExternDCDraw, this);
// 		else
// 			ipcplay_SetExternDrawCallBack(m_pPlayContext->hPlayer[0], nullptr, nullptr);
// 	}
// }


void CIPCPlayDemoDlg::OnFileAddrenderwnd()
{
	if (m_pPlayContext && m_pPlayContext->hPlayer[0])
	{
		int nCount = 0;
		//ipcplay_GetRenderWndCount(m_pPlayContext->hPlayer[0], &nCount);
		if (nCount < m_pVideoWndFrame->GetPanelCount())
		{
			for (int i = 0; i < m_pVideoWndFrame->GetPanelCount(); i++)
			{
				if (!m_pVideoWndFrame->GetPanelParam(i))
				{
					//ipcplay_AddWnd(m_pPlayContext->hPlayer[0],m_pVideoWndFrame->GetPanelWnd(i));
					m_pVideoWndFrame->SetPanelParam(i, m_pPlayContext.get());
					break;
				}
			}
		}
	}
	else
	{
		m_wndStatus.SetWindowText(_T("播放器尚未启动."));
		m_wndStatus.SetAlarmGllitery();
	}
}


void CIPCPlayDemoDlg::OnFileRemoveRenderWnd()
{
	for (int i = m_pVideoWndFrame->GetPanelCount() - 1; i>=0; i--)
	{
		if (m_pVideoWndFrame->GetPanelParam(i))
		{
			//ipcplay_RemoveWnd(m_pPlayContext->hPlayer[0], m_pVideoWndFrame->GetPanelWnd(i));
			m_pVideoWndFrame->SetPanelParam(i, nullptr);
			break;
		}
	}
}


void CIPCPlayDemoDlg::OnBnClickedCheckDisplayrgb()
{
	if (IsDlgButtonChecked(IDC_CHECK_DISPLAYRGB) == BST_CHECKED)
	{
		if (!m_pPlayContext->hPlayer[0])
		{
			AfxMessageBox(_T("尚未播放图像像"));
			return;
		}
		if (!m_pDisplayRGB24)
		{
			m_pDisplayRGB24 = new CDialogDisplayRGB24;
			m_pDisplayRGB24->Create(IDD_DIALOG_DISPLAYRGB, this);
		}
		m_pDisplayRGB24->ShowWindow(SW_SHOW);
		ipcplay_SetYUVCaptureEx(m_pPlayContext->hPlayer[0], (void *)CaptureYUVExProc, this);
	}
	else
	{
		if (m_pDisplayRGB24)
			m_pDisplayRGB24->ShowWindow(SW_HIDE);
	}
}


void CIPCPlayDemoDlg::OnFileDisplaytran()
{
	//m_pVideoWndFrame->;
}


void CIPCPlayDemoDlg::OnBnClickedButtonDivideframe()
{
	m_nRow = GetDlgItemInt(IDC_EDIT_ROW);
	m_nCol = GetDlgItemInt(IDC_EDIT_COL);
	if (m_pVideoWndFrame->GetRows() != m_nRow ||
		m_pVideoWndFrame->GetCols() != m_nCol)
	{
		if (m_nRow*m_nCol <= 36)
		{
			m_pVideoWndFrame->AdjustPanels(m_nRow, m_nCol);
			//m_pPlayContext->nPlayerCount = m_nRow*m_nCol;
		}
		else
		{
			m_pVideoWndFrame->AdjustPanels(36);
			//m_pPlayContext->nPlayerCount = 36;
		}
	}
}


char* MessageArray[] =
{
	"WM_FRAME_MOUSEMOVE",
	"WM_FRAME_LBUTTONDOWN",
	"WM_FRAME_LBUTTONUP",
	"WM_FRAME_LBUTTONDBLCLK",
	"WM_FRAME_RBUTTONDOWN",
	"WM_FRAME_RBUTTONUP",
	"WM_FRAME_RBUTTONDBLCLK",
	"WM_FRAME_MBUTTONDOWN",
	"WM_FRAME_MBUTTONUP",
	"WM_FRAME_MBUTTONDBLCLK",
	"WM_FRAME_MOUSEWHEEL"
};
BOOL CIPCPlayDemoDlg::PreTranslateMessage(MSG* pMsg)
{	

	return CDialogEx::PreTranslateMessage(pMsg);
}

CRITICAL_SECTION CIPCPlayDemoDlg::m_csMapSubclassWnd;
map<HWND, SubClassInfoPtr> CIPCPlayDemoDlg::m_MapSubclassWnd;

LRESULT CIPCPlayDemoDlg::SubClassProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	
	EnterCriticalSection(&m_csMapSubclassWnd);
	auto itFind = m_MapSubclassWnd.find(hWnd);
	if (itFind == m_MapSubclassWnd.end())
	{
		LeaveCriticalSection(&m_csMapSubclassWnd);
		return 0L;
	}
	LeaveCriticalSection(&m_csMapSubclassWnd);
	RECT rtPartner;
	::GetWindowRect(itFind->second->hPartnerWnd, &rtPartner);
	switch (nMessage)
	{
	case WM_WINDOWPOSCHANGED:
	case WM_WINDOWPOSCHANGING:
	case WM_MOVE:
	case WM_MOVING:
	case WM_SIZE:
	{
		TraceMsgA("%s Message = %d.\n", __FUNCTION__, nMessage);
		::MoveWindow(hWnd, rtPartner.left, rtPartner.top, rtPartner.right - rtPartner.left, rtPartner.bottom - rtPartner.top, true);
	}
	}
	return itFind->second->pOldProcWnd(hWnd, nMessage, wParam, lParam);
}

void CIPCPlayDemoDlg::OnBnClickedCheckEnabletransparent()
{/*
	if (IsDlgButtonChecked(IDC_CHECK_ENABLETRANSPARENT) == BST_CHECKED)
	{
		int nPanel = m_pVideoWndFrame->GetCurPanel();
		if (nPanel == -1)
		{
			AfxMessageBox(_T("请选择一个窗格."));
			return;
		}
		
		RECT rtPanel =*(m_pVideoWndFrame->GetPanelRect(nPanel));
		//m_pVideoWndFrame->ClientToScreen(&rtPanel);
		//m_pTransparentDlg = new CTransparentDlg;
		
		CWnd *pTempWnd = CWnd::FromHandle(m_pVideoWndFrame->GetPanelWnd(nPanel));
		//m_pTransparentDlg->Create(IDD_DIALOG_TRANSPARENT, pTempWnd);
		
		m_pTransparentWnd->Create(nullptr, _T("TransparentWnd"), WS_CHILD, rtPanel, pTempWnd, 1024, nullptr);
		SubClassInfoPtr pSubClass = make_shared<SubClassInfo>(m_pVideoWndFrame->GetPanelWnd(nPanel), m_pTransparentWnd->GetSafeHwnd(), (WNDPROC)SubClassProc);
	
		auto itFind = m_MapSubclassWnd.find(m_pTransparentWnd->GetSafeHwnd());
		if (itFind == m_MapSubclassWnd.end())
		{
			m_pTransparentWnd->ShowWindow(SW_SHOW);
			//m_pTransparentWnd->MoveWindow(&rtPanel);
			m_MapSubclassWnd.insert(pair<HWND, SubClassInfoPtr>(m_pTransparentWnd->GetSafeHwnd(), pSubClass));
		}
		else
		{
			itFind->second = pSubClass;
		}
				
	}
	else
	{
		delete m_pTransparentWnd;
		m_pTransparentWnd = nullptr;
		m_MapSubclassWnd.erase(m_pTransparentWnd->GetSafeHwnd());
	}*/
}


void CIPCPlayDemoDlg::OnFileDrawline()
{
	if (!m_pPlayContext->hPlayer[0])
	{
		AfxMessageBox(_T("尚未播放图像像"));
		return;
	}
	POINT ptArray[2] = { 0 };
	ptArray[0].x = 0;
	ptArray[0].y = 0;
	ptArray[1].x = 1280;
	ptArray[1].y = 720;
	ipcplay_AddLineArray(m_pPlayContext->hPlayer[0], &ptArray[0], 2, 1, IPC_XRGB(255, 0, 0));
		
}


void CIPCPlayDemoDlg::OnRender(UINT nID)
{
	CMenu *pMenu = GetMenu()->GetSubMenu(0);
	CMenu *pSubMenu = pMenu->GetSubMenu(5);
	if (pSubMenu)
	{
		m_bEnableDDraw = (nID == ID_RENDER_DDRAW);
		if (m_bEnableDDraw)
			pSubMenu->CheckMenuRadioItem(ID_RENDER_D3D, ID_RENDER_DDRAW, ID_RENDER_DDRAW, MF_CHECKED | MF_BYCOMMAND);
		else
			pSubMenu->CheckMenuRadioItem(ID_RENDER_D3D, ID_RENDER_DDRAW, ID_RENDER_D3D, MF_CHECKED | MF_BYCOMMAND);
	}
}


void CIPCPlayDemoDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
	CDialogEx::OnMoving(fwSide, pRect);
// 	if (m_pTransparentDlg)
// 	{
// 		int nPanel = m_pVideoWndFrame->GetCurPanel();
// 		RECT rtPanel = *(m_pVideoWndFrame->GetPanelRect(nPanel));
// 		m_pVideoWndFrame->ClientToScreen(&rtPanel);
// 		m_pTransparentDlg->MoveWindow(&rtPanel);
// 	}
}


void CIPCPlayDemoDlg::OnMove(int x, int y)
{
	CDialogEx::OnMove(x, y);

// 	if (m_pTransparentDlg)
// 	{
// 		int nPanel = m_pVideoWndFrame->GetCurPanel();
// 		RECT rtPanel = *(m_pVideoWndFrame->GetPanelRect(nPanel));
// 		m_pVideoWndFrame->ClientToScreen(&rtPanel);
// 		m_pTransparentDlg->MoveWindow(&rtPanel);
// 	}
}


void CIPCPlayDemoDlg::OnEnableOpassist()
{
	if (!m_pPlayContext || !m_pPlayContext->hPlayer[0])
	{
		AfxMessageBox(_T("尚未播放图像像"));
		return;
	}
	CMenu *pSubMenu = GetMenu()->GetSubMenu(0);
	if (!pSubMenu)
		return;
	if (m_bOPAssistEnabled)
	{
		for (auto it = m_vecPolygon.begin(); it != m_vecPolygon.end(); it++)
			ipcplay_RemovePolygon(m_pPlayContext->hPlayer[0] ,*it);
	}
	else
	{
		int nWidth = 200;
		int nHeight = 150;

		int nThickVerical = 10;		// 垂直方向厚度
		int nThickHorizontal = 20;		// 水平方向厚度

		int nDistanceV = 100;
		int nDistanceH = 100;

		POINT ptArray[4][6];
		int nStartX = nDistanceH;
		int nStartY = nDistanceV;

		ptArray[0][0].x = nStartX;
		ptArray[0][0].y = nStartY;

		ptArray[0][1].x = nStartX + nWidth;
		ptArray[0][1].y = nStartY;

		ptArray[0][2].x = ptArray[0][1].x - nThickVerical;
		ptArray[0][2].y = ptArray[0][1].y + nThickVerical;

		ptArray[0][3].x = nStartX + nThickHorizontal;
		ptArray[0][3].y = nStartY + nThickVerical;

		ptArray[0][4].x = nStartX + nThickHorizontal;
		ptArray[0][4].y = nStartY + nHeight - nThickHorizontal;

		ptArray[0][5].x = nStartX;
		ptArray[0][5].y = nStartY + nHeight;


		nStartX = 800;
		nStartY = 100;

		ptArray[1][0].x = nStartX;
		ptArray[1][0].y = nStartY;

		ptArray[1][1].x = nStartX - nWidth;
		ptArray[1][1].y = nStartY;

		ptArray[1][2].x = ptArray[1][1].x + nThickVerical;
		ptArray[1][2].y = ptArray[1][1].y + nThickVerical;

		ptArray[1][3].x = nStartX - nThickHorizontal;
		ptArray[1][3].y = nStartY + nThickVerical;

		ptArray[1][4].x = ptArray[1][3].x;
		ptArray[1][4].y = nStartY + nHeight - nThickHorizontal;

		ptArray[1][5].x = nStartX;
		ptArray[1][5].y = nStartY + nHeight;


		nStartX = 800;
		nStartY = 600;

		ptArray[2][0].x = nStartX;
		ptArray[2][0].y = nStartY;

		ptArray[2][1].x = nStartX - nWidth;
		ptArray[2][1].y = nStartY;

		ptArray[2][2].x = ptArray[2][1].x + nThickVerical;
		ptArray[2][2].y = ptArray[2][1].y - nThickVerical;

		ptArray[2][3].x = nStartX - nThickHorizontal;
		ptArray[2][3].y = nStartY - nThickVerical;

		ptArray[2][4].x = ptArray[2][3].x;
		ptArray[2][4].y = nStartY - nHeight + nThickHorizontal;

		ptArray[2][5].x = nStartX;
		ptArray[2][5].y = nStartY - nHeight;


		nStartX = 100;
		nStartY = 600;

		ptArray[3][0].x = nStartX;
		ptArray[3][0].y = nStartY;

		ptArray[3][1].x = nStartX + nWidth;
		ptArray[3][1].y = nStartY;

		ptArray[3][2].x = ptArray[3][1].x - nThickVerical;
		ptArray[3][2].y = ptArray[3][1].y - nThickVerical;

		ptArray[3][3].x = nStartX + nThickHorizontal;
		ptArray[3][3].y = nStartY - nThickVerical;

		ptArray[3][4].x = ptArray[3][3].x;
		ptArray[3][4].y = nStartY - nHeight + nThickHorizontal;

		ptArray[3][5].x = nStartX;
		ptArray[3][5].y = nStartY - nHeight;

	
		WORD Index[2][6] = { { 0, 1, 2, 3,4,5 }, { 0, 5, 4, 3 ,2, 1 } };
		for (int i = 0; i < 4; i++)
		{
			long nPolygonIndex = ipcplay_AddPolygon(m_pPlayContext->hPlayer[0], ptArray[i], 6, Index[i %2], IPC_XRGB(255, 0, 0));
			if (nPolygonIndex)
				m_vecPolygon.push_back(nPolygonIndex);
		}
			
	}
	m_bOPAssistEnabled = !m_bOPAssistEnabled;
	pSubMenu->CheckMenuItem(ID_ENABLE_OPASSIST, MF_BYCOMMAND | MF_CHECKED);
}


void CIPCPlayDemoDlg::OnBnClickedCheckNodecodedelay()
{
	// TODO: Add your control notification handler code here
}


void CIPCPlayDemoDlg::OnBnClickedButtonOsd()
{
	// 正在播放中
	if (m_pPlayContext && m_pPlayContext->hPlayer[0])
	{
		IPC_PLAYHANDLE &hHandlePlay = m_pPlayContext->hPlayer[0];
		CFontDialog FontDlg;
		LOGFONT lf;
		ZeroMemory(&lf, sizeof(LOGFONT));
		if (FontDlg.DoModal() == IDOK)
		{
			memcpy(&lf, FontDlg.m_cf.lpLogFont, sizeof(LOGFONT));
			long hFontHandle = 0;
			if (ipcplay_CreateOSDFont(hHandlePlay, lf, &hFontHandle) == IPC_Succeed)
			{
				m_pPlayContext->nFontHandle[m_pPlayContext->nFontCount++] = hFontHandle;
				RECT rtPosition = { 0, 0, 1024, 256 };
				long nTextHandle = 0;
				COLORREF nColor = FontDlg.GetColor();

				if (ipcplay_DrawOSDText(hFontHandle,			// 字体句柄，一个字体句柄用输出多个叠加文本
										_T("OSD 字幕测试"),		// 文本内容
										-1,						// 文本长度，为-1时则自动计算长度
										rtPosition,				// 文本输出位置
										DT_LEFT|DT_TOP,				// 输出格式选择
										IPC_ARGB(0xFF/*字体透明度，0~FF,0xFF为完全不透明*/, GetRValue(nColor), GetGValue(nColor), GetBValue(nColor)), 
										&nTextHandle) != IPC_Succeed)
					AfxMessageBox(_T("叠加文字失败"));
				else
				{
					m_pPlayContext->nTextHandle[m_pPlayContext->nTextCount++] = nTextHandle;
				}
			}
			else
				AfxMessageBox(_T("创建OSD字体失败"));
		}
	}
	else
		AfxMessageBox(_T("尚未开始播放"));
}

void CIPCPlayDemoDlg::OnBnClickedButtonRemoveosd()
{
	// 移动文本时，可选择只移除文本，但保留字体，以备后后，但这演示时会把字体和文本一同移除
	// 正在播放中
	if (m_pPlayContext && m_pPlayContext->hPlayer[0])
	{
		int nResult = 0;
		TCHAR szErrorText[1024] = {0};
		if (m_pPlayContext->nTextHandle[0])
		{
			nResult = ipcplay_RmoveOSDText(m_pPlayContext->nTextHandle[0]);
			if (nResult != IPC_Succeed)
			{
				ipcplay_GetErrorMessage(nResult, szErrorText, 1024);
				AfxMessageBox(szErrorText);
				return;
			}
			m_pPlayContext->nTextHandle[0] = 0;
		}
			
		if (m_pPlayContext->nFontHandle[0])
		{
			nResult = ipcplay_DestroyOSDFont(m_pPlayContext->nFontHandle[0]);
			if (nResult != IPC_Succeed)
			{
				ipcplay_GetErrorMessage(nResult, szErrorText, 1024);
				AfxMessageBox(szErrorText);
				return;
			}
			m_pPlayContext->nFontHandle[0] = 0;
		}
	}
	else
		AfxMessageBox(_T("尚未开始播放"));
}

void CIPCPlayDemoDlg::OnBnClickedButtonOsd2()
{

}

void CIPCPlayDemoDlg::OnBnClickedButtonMergescreens()
{
	if (m_pPlayContext && m_pPlayContext->hPlayer[0])
	{
		IPC_PLAYHANDLE &hHandlePlay = m_pPlayContext->hPlayer[0];
		m_pVideoWndFrame->AdjustPanels(4);
		PlayerInfo pi;
		ipcplay_GetPlayerInfo(hHandlePlay, &pi);
		pi.nVideoWidth;
		pi.nVideoHeight;
		RECT rtBorder[4] = { 0 };
		int i = 0;
		rtBorder[i].left = 0;
		rtBorder[i].right = pi.nVideoWidth / 2;
		rtBorder[i].top = 0;
		rtBorder[i].bottom = pi.nVideoHeight / 2;
		i++;
		rtBorder[i].left = pi.nVideoWidth / 2;
		rtBorder[i].right = 0;
		rtBorder[i].top = 0;
		rtBorder[i].bottom = pi.nVideoHeight / 2;
		i++;
		rtBorder[i].left = 0;
		rtBorder[i].bottom = 0;
		rtBorder[i].right = pi.nVideoWidth / 2;
		rtBorder[i].top = pi.nVideoHeight / 2;
		i++;
		rtBorder[i].right = 0;
		rtBorder[i].bottom = 0;
		rtBorder[i].left = pi.nVideoWidth / 2;
		rtBorder[i].top = pi.nVideoHeight / 2;

		HWND hWndArray[4] = { 0 };
		for (int i = 0; i < 4; i++)
			hWndArray[i] = m_pVideoWndFrame->GetPanelWnd(i);
		ipcplay_SetBorderRect(hHandlePlay,hWndArray[0], &rtBorder[0]);
		for (int i = 1; i < 4; i++)
		{
			ipcplay_AddWindow(hHandlePlay, hWndArray[i], &rtBorder[i]);
		}
		
	}
	else
		AfxMessageBox(_T("尚未开始播放"));
}
