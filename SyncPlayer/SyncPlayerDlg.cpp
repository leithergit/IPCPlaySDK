
// SyncPlayerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SyncPlayer.h"
#include "SyncPlayerDlg.h"
#include "afxdialogex.h"

#include "DHStream.hpp"
#include "DhStreamParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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


// CSyncPlayerDlg 对话框


CSyncPlayerDlg::CSyncPlayerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSyncPlayerDlg::IDD, pParent)
	, m_nTestTimes(200)
	, m_nPlayerCount(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pVideoFrame = new CVideoFrame;
	m_pWndSizeManager = new CWndSizeManger;
	
// 	m_strFile[0] = _T("I:\\1\\3301061000001$0\\3301061000001$0_2019-04-25 10;20;50.dat\\0-0.dav");
// 	m_strFile[1] = _T("I:\\1\\3301061000002$0\\3301061000002$0_2019-04-25 10;20;50.dat\\0-1.dav");
// 	m_strFile[2] = _T("I:\\1\\3301061000003$0\\3301061000003$0_2019-04-25 10;20;51.dat\\0-2.dav");
// 	m_strFile[3] = _T("I:\\1\\3301061000004$0\\3301061000004$0_2019-04-25 10;20;51.dat\\0-3.dav");
	LoadConfig();
	m_nTestTimes = 200;
}

void CSyncPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_FILE1, m_strFile[0]);
	DDX_Text(pDX, IDC_EDIT_FILE2, m_strFile[1]);
	DDX_Text(pDX, IDC_EDIT_FILE3, m_strFile[2]);
	DDX_Text(pDX, IDC_EDIT_FILE4, m_strFile[3]);
	DDX_Text(pDX, IDC_EDIT_FILE5, m_strFile[4]);
	DDX_Text(pDX, IDC_EDIT_TEST, m_nTestTimes);
	DDX_Text(pDX, IDC_EDIT_PLAYERS, m_nPlayerCount);
}

BEGIN_MESSAGE_MAP(CSyncPlayerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, &CSyncPlayerDlg::OnBnClickedButtonStart)
	ON_WM_SIZE()
	ON_COMMAND_RANGE(IDC_BUTTON_BROWSE1, IDC_BUTTON_BROWSE5, &CSyncPlayerDlg::OnBrowse)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CSyncPlayerDlg::OnBnClickedButtonStop)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_TEST, &CSyncPlayerDlg::OnBnClickedButtonTest)
	ON_BN_CLICKED(IDC_BUTTON_STOPTEST, &CSyncPlayerDlg::OnBnClickedButtonStoptest)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_VIDEOFILEMANAGER, &CSyncPlayerDlg::OnBnClickedButtonVideofilemanager)
END_MESSAGE_MAP()


// CSyncPlayerDlg 消息处理程序

BOOL CSyncPlayerDlg::OnInitDialog()
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	ZeroMemory(m_pPlayer, sizeof(CDialogVideo*) * 16);
	CRect rtClient;
	GetDlgItemRect(IDC_STATIC_FRAME, rtClient);
	m_pVideoFrame->Create(1024, rtClient, 4, this);
	//SetDlgItemInt(IDC_EDIT_PLAYERS, 5);

	UINT nIDArrayBottom[] = { IDC_STATIC1, IDC_EDIT_FILE1, IDC_BUTTON_BROWSE1,
		IDC_STATIC2, IDC_EDIT_FILE2, IDC_BUTTON_BROWSE2,
		IDC_STATIC3, IDC_EDIT_FILE3, IDC_BUTTON_BROWSE3,
		IDC_STATIC4, IDC_EDIT_FILE4, IDC_BUTTON_BROWSE4,
		IDC_BUTTON_START,IDC_BUTTON_STOP };

// 	UINT nIDArrayRight[] = {  };
// 	UINT nIDArrayRightBottom[] = {};
	UINT nIDArrayCenter[] = { IDC_STATIC_FRAME };

	RECT rtDialog;
	GetClientRect(&rtDialog);

	// 	SaveWndPosition(nIDArreayTop, sizeof(nIDArreayTop) / sizeof(UINT), DockTop, rtDialog);
	// 	SaveWndPosition(nIDArrayRight, sizeof(nIDArrayRight) / sizeof(UINT), DockRigth, rtDialog);
	m_pWndSizeManager->SetParentWnd(this);
	m_pWndSizeManager->SaveWndPosition(nIDArrayBottom, sizeof(nIDArrayBottom) / sizeof(UINT), DockBottom);
	m_pWndSizeManager->SaveWndPosition(nIDArrayCenter, sizeof(nIDArrayCenter) / sizeof(UINT), DockCenter, false);
	//m_pWndSizeManager->SaveWndPosition(nIDArrayRight, sizeof(nIDArrayRight) / sizeof(UINT), DockRight);
	//m_pWndSizeManager->SaveWndPosition(nIDArrayRightBottom, sizeof(nIDArrayRightBottom) / sizeof(UINT), (DockType)(DockRight | DockBottom));

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CSyncPlayerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CSyncPlayerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CSyncPlayerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSyncPlayerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (GetDlgItem(IDC_BUTTON_START)->GetSafeHwnd())
	{
		m_pWndSizeManager->OnSize(nType, cx, cy);
		RECT rt;
		m_pWndSizeManager->GetWndCurrentPostion(IDC_STATIC_FRAME, rt);
		m_pVideoFrame->MoveWindow(&rt, true);

	}
}

void CSyncPlayerDlg::OnBrowse(UINT nID)
{
	int nIndex = nID - IDC_BUTTON_BROWSE1;
	if (nIndex >= 0 && nIndex <= 4)
	{
		UINT nEditID[] = { IDC_EDIT_FILE1, IDC_EDIT_FILE2, IDC_EDIT_FILE3, IDC_EDIT_FILE4, IDC_EDIT_FILE5 };
		TCHAR szText[1024] = { 0 };
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ALLOWMULTISELECT;
		TCHAR  szFilter[] = _T("Video File(*.dav)|*.dav|Viedo File (*.mp4)|*.mp4|H.264 Video File(*.H264)|*.H264|H.265 Video File(*.H265)|*.H265|All Files (*.*)|*.*||");
		TCHAR szExportLog[MAX_PATH] = { 0 };
		CTime tNow = CTime::GetCurrentTime();
		CFileDialog OpenFile(TRUE, _T("*.mp4"), _T(""), dwFlags, (LPCTSTR)szFilter);
		OpenFile.m_ofn.lpstrTitle = _T("Please Select the files to play");
		CString strFilePath;

		if (OpenFile.DoModal() == IDOK)
		{
			SetDlgItemText(nEditID[nIndex], OpenFile.GetPathName());
		}
	}
}


bool CSyncPlayerDlg::LoadConfig( )
{
	TCHAR szPath[MAX_PATH] = { 0 };
	GetAppPath(szPath, MAX_PATH);
	_tcscat_s(szPath, MAX_PATH, _T("\\Configuration.xml"));
	if (!PathFileExists(szPath))
	{
		return false;
	}
	CMarkup xml;
	if (!xml.Load(szPath))
		return false;
	/*
	<?xml version="1.0" encoding="utf-8"?>
	<Configuration >
	<CameraList>
	<Camera IP="192.168.1.26"/>
	<Camera IP="192.168.1.30"/>
	</CameraList>
	</Configuration>
	*/

	int nCount = 0;
	TCHAR szItemText[64] = { 0 };

	CString strValue;
	m_nPlayerCount = 0;

	CString strIP, strAcount, strPassword, strPort;
	CString strURL;
	if (xml.FindElem(_T("Configuration")))
	{
		if (xml.FindChildElem(_T("FileList")))
		{
			xml.IntoElem();
			while (xml.FindChildElem(_T("File")))
			{
				xml.IntoElem();
				m_strFile[m_nPlayerCount++] = xml.GetAttrib(_T("Name"));

				xml.OutOfElem();
			}
			xml.OutOfElem();
		}
		//nCount = 0;	
	}
	return true;
}

bool CSyncPlayerDlg::SaveConfig()
{
	TCHAR szPath[MAX_PATH] = { 0 };
	GetAppPath(szPath, MAX_PATH);
	_tcscat_s(szPath, MAX_PATH, _T("\\Configuration.xml"));

	TCHAR szItemText[1024] = { 0 };
	CMarkup xml;
	TCHAR *szDoc = _T("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n");
	xml.SetDoc(szDoc);
	/*
	<?xml version="1.0" encoding="utf-8"?>
	<Configuration >
	<FileList>
	<File Name="192.168.1.26"/>
	<File Name="192.168.1.26"/>
	</FileList>
	</Configuration>
	*/

	xml.AddElem(_T("Configuration"));
	xml.IntoElem();	// Configuration	
	xml.AddElem(_T("FileList"));	//CameraList

	xml.IntoElem();	// CameraList	
	int nCount = GetDlgItemInt(IDC_EDIT_PLAYERS);
	bool bFlag = false;
	for (int i = 0; i < nCount; i++)
	{
		bFlag = xml.AddElem(_T("File"));
		bFlag = xml.AddAttrib(_T("Name"), m_strFile[i]);
	}

	xml.OutOfElem();// CameraList
	xml.OutOfElem();// Configuration
	xml.Save(szPath);
	return true;
}
void CSyncPlayerDlg::OnBnClickedButtonStart()
{
	UpdateData();
	SaveConfig();
	bool bExistSameFile = false;
	CString strText;
	
	int nCount = GetDlgItemInt(IDC_EDIT_PLAYERS);

// 	for (int i = 0; i < nCount; i++)
// 	{
// 		CString strTemp = m_strFile[i];
// 		for (int k = 0; k < nCount; k++)
// 		{
// 			if (k == i)
// 				continue;
// 			if (strTemp = m_strFile[k])
// 			{
// 				bExistSameFile = true;
// 				strText.Format(_T("%d号和%d号文件名相同，同步播放不能播放同一个文件。"), i, k);
// 				
// 				break;
// 			}
// 		}
// 	}
// 	if (bExistSameFile)
// 		AfxMessageBox(strText);
// 	else
	{
		TCHAR szTitle[256] = { 0 };
		for (int i = 0; i < nCount; i++)
		{
			ThreadParam *pTP = new ThreadParam;
			pTP->pDialog = this;
			m_pPlayer[i] = new CDialogVideo;
			m_pPlayer[i]->Create(IDD_DIALOG_VIDEO, this);
			_stprintf_s(szTitle, 256, _T("VideoPlayer(%d)"), i + 1);
			m_pPlayer[i]->SetWindowText(szTitle);
			m_pPlayer[i]->ShowWindow(SW_SHOW);
			pTP->nID = i;
			m_bThreadRun = true;
			//m_nTimeEvent = timeSetEvent(100, 1, (LPTIMECALLBACK)MMTIMECALLBACK, (DWORD_PTR)this, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
			m_hReadFile[i] = (HANDLE)_beginthreadex(nullptr, 0, ThreadReadFile, pTP, CREATE_SUSPENDED, 0);
		}
		for (int i = 0; i < nCount; i++)
			ResumeThread(m_hReadFile[i]);
		EnableDlgItem(IDC_BUTTON_START, false);
		EnableDlgItem(IDC_BUTTON_STOP, true);
	}
}


UINT CSyncPlayerDlg::ReadFileRun(UINT nIndex)
{
	int nBufferSize = 128 * 1024;
	byte *pFileBuffer = new byte[nBufferSize];
	ZeroMemory(pFileBuffer, nBufferSize);
	DhStreamParser* pStreamParser = nullptr;	
	try
	{
		CFile file(m_strFile[nIndex], CFile::modeRead | CFile::shareDenyWrite);
		int nWidth = 0, nHeight = 0, nFramerate = 0;		
//		ipcplay_EnableStreamParser(hPlayHandle, CODEC_H264);
//		ipcplay_SetDecodeDelay(hPlayHandle, 0);
//		同步播放
// 		if (nIndex == 0)
// 			ipcplay_StartSyncPlay(hPlayHandle, true,nullptr,25);
// 		else
// 			ipcplay_StartSyncPlay(hPlayHandle, true, m_hSyncSource, 25);
		time_t tTimeStamp1st = 0;
		time_t tFrameTimeStamp = 0;
		int nFrameInterval = 40;
		bool bInput1stFrame = false;
		int nFrames = 0;
		while (m_bThreadRun)
		{
			int nReadLength = file.Read(pFileBuffer, nBufferSize);
			if (nReadLength > 0)
			{
				if (!pStreamParser)
					pStreamParser = new DhStreamParser();
				pStreamParser->InputData(pFileBuffer, nReadLength);
				while (true)
				{
					DH_FRAME_INFO *pFrame = pStreamParser->GetNextFrame();
					if (!pFrame)
						break;
					else if (!m_hAsyncPlayHandle[nIndex])
					{
						IPC_MEDIAINFO MediaHeader;
						MediaHeader.nVideoCodec = CODEC_H264;
						MediaHeader.nAudioCodec = CODEC_UNKNOWN;
						MediaHeader.nVideoWidth = pFrame->nWidth;
						MediaHeader.nVideoHeight = pFrame->nHeight;
						MediaHeader.nFps = pFrame->nFrameRate;
						m_hAsyncPlayHandle[nIndex] = ipcplay_OpenStream(m_pPlayer[nIndex]->m_hWnd, (byte *)&MediaHeader, sizeof(IPC_MEDIAINFO), pFrame->nFrameRate);
						ipcplay_EnableAsyncRender(m_hAsyncPlayHandle);
						//ipcplay_EnablePlayOneFrame(m_hAsyncPlayHandle);
						if (nIndex == 0)
						{
							TraceMsgA("%s Start Play[0]:%.3f\n", __FUNCTION__, GetExactTime());
							ipcplay_StartSyncPlay(m_hAsyncPlayHandle[nIndex], true, nullptr, pFrame->nFrameRate ? pFrame->nFrameRate : 25);
						}
						else
						{
							TraceMsgA("%s Start Play[%d]:%.3f\n", __FUNCTION__,nIndex, GetExactTime());
							ipcplay_StartSyncPlay(m_hAsyncPlayHandle[nIndex], true, m_hAsyncPlayHandle[0], pFrame->nFrameRate ? pFrame->nFrameRate : 25);
						}
							
					}
					if (pFrame->nLength < 64)
						continue;
					if (pFrame->nType == DH_FRAME_TYPE_VIDEO)
					{
						if (!tTimeStamp1st)
						{
							tTimeStamp1st = pFrame->nTimeStamp;
							tTimeStamp1st *= 1000;
						}
						
						
						nFrames++;
						//TraceMsgA("%s Frame[%d] Length = %d.\n", __FUNCTION__, nFrames, pFrame->nLength);
						
						bool bKeyFrame = false;
						
						if (pFrame->nFrameRate)
							nFrameInterval = 1000 / pFrame->nFrameRate;
						else
							nFrameInterval = 40;
						if (pFrame->nSubType == DH_FRAME_TYPE_VIDEO_I_FRAME)
						{
							tFrameTimeStamp = tTimeStamp1st;
							bKeyFrame = true;
						}
// 							
// 						else
						tTimeStamp1st += nFrameInterval;

						if (nIndex == 0 && tFirstFrameTime == 0)
							tFirstFrameTime = tTimeStamp1st;
						if (nIndex != 0)
						{
							if ((tFirstFrameTime != 0) && (tTimeStamp1st >= tFirstFrameTime))
								vecFrameTime[nIndex].push_back(make_shared<FrameTime>(tTimeStamp1st,pFrame->nLength, bKeyFrame));
						}
						else
							vecFrameTime[nIndex].push_back(make_shared<FrameTime>(tTimeStamp1st,pFrame->nLength, bKeyFrame));
							
						if (bKeyFrame && m_pFrameView)
						{
							m_pFrameView->m_ctlFrameList.SetItemCount(vecFrameTime[0].size());
							m_pFrameView->m_ctlFrameList.Invalidate();
						}
						do
						{
							//TraceMsgA("%s Player[%d] Frame Time[%I64d]\n", __FUNCTION__, nIndex, tFrameTimeStamp);
							int nStatus = ipcplay_InputIPCStream(m_hAsyncPlayHandle[nIndex], pFrame->pContent,
								pFrame->nSubType == DH_FRAME_TYPE_VIDEO_I_FRAME ? IPC_I_FRAME : IPC_P_FRAME,
								pFrame->nLength,
								pFrame->nRequence,
								tTimeStamp1st);
							if (IPC_Succeed == nStatus)
							{
// 								if (pFrame->nLength < 80)
// 								{
// 									TraceMsgA("%s Player[%d] Input1stFrame Time:%.3f,FrameTime = %I64d\n", __FUNCTION__, nIndex, GetExactTime(), tFrameTimeStamp);
// 									bInput1stFrame = true;
// 								}
								break;
							}
							else if (IPC_Error_FrameCacheIsFulled == nStatus)
							{
								Sleep(10);
								continue;
							}
						} while (m_bThreadRun);
					}
				}
			}
			else
				break;
			Sleep(5);
		}
		ipcplay_Stop(m_hAsyncPlayHandle);
		ipcplay_Close(m_hAsyncPlayHandle);
	}
	catch (CFileException* e)
	{
		TCHAR szError[1024] = { 0 };
		e->GetErrorMessage(szError, 1024, nullptr);
		TraceMsg(_T("%s Exception:%s.\n"), __FUNCTIONW__, szError);
	}
	delete[]pFileBuffer;
	if (nIndex == 0)
	{
		SetTimer(ID_RESTART, 2000, nullptr);
	}
	return 0;
}


#include "mp4v2/mp4v2.h"
#pragma comment(lib,"libmp4v2.lib")

struct CMP4File
{
	MP4FileHandle	hMP4File;
	int		nTimeScale;	
	int		nFrameRate;
	int		nVideoID;
	int		nWidth;
	int		nHeight;
	bool	bAddSPS;
	CMP4File(TCHAR *szFile, int nWidth = 0, int nHeight = 0, int nTimeScale = 90000, int nFrarate = 25)
	{
		ZeroMemory(this, sizeof(CMP4File));
		if (szFile && strlen(szFile))
		{
			hMP4File = MP4CreateEx(szFile, 0, 1, 1, 0, 0, 0, 0);
			if (hMP4File == MP4_INVALID_FILE_HANDLE)
			{
				char szException[1024] = { 0 };
				_stprintf_s(szException, 1024, "Failed in createing file %s.\n", szFile);
				throw exception(szException);
			}
			this->nTimeScale = nTimeScale;
			this->nFrameRate = nFrarate;
			this->nWidth = nWidth;
			this->nHeight = nHeight;
			MP4SetTimeScale(hMP4File, nTimeScale);
		}
		else
		{
			char szException[1024] = { 0 };
			_stprintf_s(szException, 1024, "Input a invalid filename .\n");
			throw exception(szException);
		}
	}
	~CMP4File()
	{
		if (hMP4File)
		{
			MP4Close(hMP4File, 0);
			ZeroMemory(this, sizeof(CMP4File));
		}
	}

	bool WriteData(unsigned char *pBuffer, int nSize, int nFrameWidth = 0, int nFrameHeight = 0, int nFrameTimeScale = 0, int nFrameRate = 0)
	{
		if (!hMP4File)
			return false;
		TraceMsg("%s FrameType = %d\n", __FUNCTION__, (pBuffer[4] & 0x1F));
		int index = -1;
		if (pBuffer[0] != 0 ||
			pBuffer[1] != 0 ||
			pBuffer[2] != 0 ||
			pBuffer[3] != 1)
			return false;

		unsigned char *pNalu = &pBuffer[4];
		int nNaluType = pNalu[0] & 0x1F;
		int nNaluLen = nSize - 4;
		switch (nNaluType)
		{
		case 0x07: // SPS
			TraceMsg("sps(%d)\n", nNaluLen);
			if (!bAddSPS)
			{
				int nScale = nFrameTimeScale ? nFrameTimeScale : nTimeScale;
				int nRate = nFrameRate ? nFrameRate : this->nFrameRate;
				nVideoID = MP4AddH264VideoTrack
					(hMP4File,
					nScale,								// 一秒钟多少timescale
					nScale / nRate,						// 每个帧有多少个timescale
					nFrameWidth?nFrameWidth:nWidth,     // width
					nFrameHeight?nFrameHeight:nHeight,  // height
					pBuffer[1],               // sps[1] AVCProfileIndication
					pBuffer[2],               // sps[2] profile_compat
					pBuffer[3],               // sps[3] AVCLevelIndication
					3);						  // 4 bytes length before each NAL unit
				if (nVideoID == MP4_INVALID_TRACK_ID)
				{
					TraceMsg("Error:Can't add track.\n");
					return false;
				}

				MP4SetVideoProfileLevel(hMP4File, 0x7F);

				bAddSPS = true;
				MP4AddH264SequenceParameterSet(hMP4File, nVideoID, (uint8_t *)pBuffer, nNaluLen);
			}
			else
				MP4WriteSample(hMP4File, nVideoID, (uint8_t *)pBuffer, nNaluLen + 4, MP4_INVALID_DURATION, 0, 1);


			break;

		case 0x08: // PPS
			TraceMsg("pps(%d)\n", nNaluLen);
			MP4AddH264PictureParameterSet(hMP4File, nVideoID, (uint8_t *)pBuffer, nNaluLen);
			break;

		default:
			TraceMsg("slice(%d)\n", nNaluLen);
			pBuffer[0] = (nNaluLen >> 24) & 0xFF;
			pBuffer[1] = (nNaluLen >> 16) & 0xFF;
			pBuffer[2] = (nNaluLen >> 8) & 0xFF;
			pBuffer[3] = (nNaluLen >> 0) & 0xFF;

			MP4WriteSample(hMP4File, nVideoID, (uint8_t *)pBuffer, nNaluLen + 4, MP4_INVALID_DURATION, 0, 1);

			break;
		}
		return true;
	}
	
};



UINT CSyncPlayerDlg::ReadFileRun2(UINT nIndex)
{
	DhStreamParser* pStreamParser = nullptr;
	int nBufferSize =  16*1024;
	byte *pFileBuffer = new byte[nBufferSize];
	ZeroMemory(pFileBuffer, nBufferSize);
	int nTimeScale = 90000;
	bool bAddSample = true;
	CMP4File *pMP4File = nullptr;
	try
	{
		TCHAR szMP4[256] = { 0 };
		_stprintf_s(szMP4, 256, _T("%s.mp4"), (LPCTSTR)m_strFile[nIndex].Left(m_strFile[nIndex].GetLength() - 4));
		
		
			
		CFile fileRead(m_strFile[nIndex], CFile::modeRead | CFile::shareDenyWrite);
		//CFile fileWrite(_T("FileSave.dav"), CFile::modeCreate | CFile::modeWrite);
		int nWidth = 0, nHeight = 0, nFramerate = 0;
		int nOffsetStart = 0;
		int nOffsetMatched = 0;
		DHFrame *pDHFrame = nullptr;
		int nFrames = 0;
		int nVideoID = -1;
		while (m_bThreadRun)
		{
			int nReadLength = fileRead.Read(pFileBuffer, nBufferSize);
			if (nReadLength > 0)
			{
				if (!pStreamParser)
					pStreamParser = new DhStreamParser();
				pStreamParser->InputData(pFileBuffer, nReadLength);
				while (true)
				{
					DH_FRAME_INFO *pFrame = pStreamParser->GetNextFrame();
					if (!pFrame)
						break;
					nFrames++;
						
					if (pFrame->nType == DH_FRAME_TYPE_VIDEO)
					{
						try
						{
							if (!pMP4File)
								pMP4File = new CMP4File(szMP4, pFrame->nWidth, pFrame->nHeight, nTimeScale, pFrame->nFrameRate);
							else
								pMP4File->WriteData(pFrame->pContent, pFrame->nFrameLength);
						}
						catch (std::exception &e)
						{
							TraceMsgA("%s Catch a exception:%s.\n", __FUNCTION__, e.what());
							return 0;
						}
					}
				}
			}
			else
				break;
		}
		if (pMP4File)
			delete pMP4File;

	}
	catch (CFileException* e)
	{
	}
	delete[]pFileBuffer;
	return 0;
}
void CSyncPlayerDlg::OnBnClickedButtonStop()
{
	CWaitCursor Wait;
	m_bThreadRun = false;
	::MsgWaitForMultipleObjects(4, m_hReadFile, true, INFINITE, QS_ALLINPUT);
	for (int i = 0; i < 4; i++)
	{
		if (m_pPlayer[i])
		{
			m_pPlayer[i]->DestroyWindow();
			delete m_pPlayer[i];
			m_pPlayer[i] = nullptr;
		}
		CloseHandle(m_hReadFile[i]);
		m_hReadFile[i] = nullptr;
	}
	EnableDlgItem(IDC_BUTTON_START, true);
	EnableDlgItem(IDC_BUTTON_STOP, false);
}


void CSyncPlayerDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	OnBnClickedButtonStop();
	delete m_pVideoFrame;
	delete m_pWndSizeManager;
}


void CSyncPlayerDlg::OnBnClickedButtonTest()
{
	EnableDlgItem(IDC_EDIT_TEST, FALSE);
	UpdateData();
	m_nTestTimer = SetTimer(1024, 1000, nullptr);
}


void CSyncPlayerDlg::OnBnClickedButtonStoptest()
{
	if (m_nTestTimer)
		KillTimer(m_nTestTimer);
	EnableDlgItem(IDC_EDIT_TEST, TRUE);
	if (m_pFrameView)
	{
		m_pFrameView->ShowWindow(SW_SHOW);
	}
	else
	{
		m_pFrameView = new CFrameView;
		m_pFrameView->Create(IDD_FRAME_VIEW, this);
		m_pFrameView->ShowWindow(SW_SHOW);
	}
}


void CSyncPlayerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1024)
	{
		if (m_nTestTimes > 0)
		{
			if (m_bThreadRun)
			{
				OnBnClickedButtonStop();
			}
			else
				OnBnClickedButtonStart();
			m_nTestTimes--;
			UpdateData(FALSE);
		}
	}
	if (nIDEvent == ID_RESTART)
	{
		OnBnClickedButtonStop();
		OnBnClickedButtonStart();
		KillTimer(ID_RESTART);
			
	}

	CDialogEx::OnTimer(nIDEvent);
}

#include "DavFileManager.h"
void CSyncPlayerDlg::OnBnClickedButtonVideofilemanager()
{
	CDavFileManager dlg;
	dlg.DoModal();
}

