
// ComboPlayDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ComboPlay.h"
#include "ComboPlayDlg.h"
#include "afxdialogex.h"

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


// CComboPlayDlg 对话框



CComboPlayDlg::CComboPlayDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CComboPlayDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CComboPlayDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CComboPlayDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE1, &CComboPlayDlg::OnBnClickedButtonBrowse1)
	ON_BN_CLICKED(IDC_BUTTON_STARTPLAY, &CComboPlayDlg::OnBnClickedButtonStartplay)
	ON_BN_CLICKED(IDC_BUTTON_STOPPLAY, &CComboPlayDlg::OnBnClickedButtonStopplay)
	ON_BN_CLICKED(IDC_BUTTON_MOVEUP, &CComboPlayDlg::OnBnClickedButtonMoveup)
	ON_BN_CLICKED(IDC_BUTTON_MOVEDOWN, &CComboPlayDlg::OnBnClickedButtonMovedown)
END_MESSAGE_MAP()


// CComboPlayDlg 消息处理程序

BOOL CComboPlayDlg::OnInitDialog()
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

	m_rtBorder.left = 0;
	m_rtBorder.top = 0;
	m_rtBorder.right = 0;
	m_rtBorder.bottom = 50;

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CComboPlayDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CComboPlayDlg::OnPaint()
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
HCURSOR CComboPlayDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CComboPlayDlg::OnBnClickedButtonBrowse1()
{
	TCHAR szText[1024] = { 0 };
	int nFreePanel = 0;
	DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	TCHAR  szFilter[] = _T("录像视频文件 (*.mp4)|*.mp4|H.264录像文件(*.H264)|*.H264|H.265录像文件(*.H265)|*.H265|All Files (*.*)|*.*||");
	TCHAR szExportLog[MAX_PATH] = { 0 };
	CTime tNow = CTime::GetCurrentTime();
	CFileDialog OpenDataBase(TRUE, _T("*.mp4"), _T(""), dwFlags, (LPCTSTR)szFilter);
	OpenDataBase.m_ofn.lpstrTitle = _T("请选择播放的文件");
	CString strFilePath;
		
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
				AfxMessageBox(_T("指定的文件不是一个有效的IPC录像文件"));
				return;
			}
			fpMedia.GetPosition();
			fpMedia.Close();
			
			SetDlgItemText(IDC_EDIT_FILEPATH, strFilePath);
		}
		catch (CFileException* e)
		{
			TCHAR szErrorMsg[255] = { 0 };
			e->GetErrorMessage(szErrorMsg, 255);
			AfxMessageBox(szErrorMsg);
			return;
		}
	}
	EnableDlgItem(IDC_BUTTON_STARTPLAY, true);
}


void CComboPlayDlg::OnBnClickedButtonStartplay()
{
	TCHAR szText[1024] = { 0 };
	m_hWndView  = GetDlgItem(IDC_STATIC_FRAME)->GetSafeHwnd();
	CString strFilePath;
	GetDlgItemText(IDC_EDIT_FILEPATH, strFilePath);
	m_hPlayer = ipcplay_OpenFile(m_hWndView, (TCHAR *)(LPCTSTR)strFilePath, NULL, NULL);
	if (!m_hPlayer)
	{
		_stprintf_s(szText, 1024, _T("无法播放%s文件."), strFilePath);
		AfxMessageBox(szText);
		return;
	}
	ipcplay_SetBorderRect(m_hPlayer, m_hWndView, &m_rtBorder, true);
	if (ipcplay_Start(m_hPlayer, false, true, false) != IPC_Succeed)
	{
		AfxMessageBox(_T("无法启动播放器"));
		return;
	}
	EnableDlgItems(m_hWnd, true, 3, IDC_BUTTON_STOPPLAY, IDC_BUTTON_MOVEUP, IDC_BUTTON_MOVEDOWN);
	EnableDlgItem(IDC_BUTTON_BROWSE1, false);
}


void CComboPlayDlg::OnBnClickedButtonStopplay()
{
	if (m_hPlayer)
	{
		ipcplay_Close(m_hPlayer);
		m_hPlayer = nullptr;
		EnableDlgItems(m_hWnd, false, 3, IDC_BUTTON_STOPPLAY, IDC_BUTTON_MOVEUP, IDC_BUTTON_MOVEDOWN);
		EnableDlgItem(IDC_BUTTON_BROWSE1, true);
	}
}


void CComboPlayDlg::OnBnClickedButtonMoveup()
{
	if (m_rtBorder.top > 0)
	{
		m_rtBorder.top--;
		m_rtBorder.bottom = 50 - m_rtBorder.top;
		ipcplay_SetBorderRect(m_hPlayer, m_hWndView, &m_rtBorder, true);
		TraceMsgA("%s Rect.top = %d\tRect.bottom = %d,Height = %d.\n",__FUNCTIONW__, m_rtBorder.top, m_rtBorder.bottom, m_rtBorder.top + m_rtBorder.bottom);
	}
	else
		MessageBeep(MB_ICONEXCLAMATION);
}


void CComboPlayDlg::OnBnClickedButtonMovedown()
{
	if (m_rtBorder.top < 50)
	{
		m_rtBorder.top++;
		m_rtBorder.bottom = 50 - m_rtBorder.top;
		ipcplay_SetBorderRect(m_hPlayer, m_hWndView, &m_rtBorder, true);
		TraceMsgA("%s Rect.top = %d\tRect.bottom = %d,Height = %d.\n",__FUNCTIONW__, m_rtBorder.top, m_rtBorder.bottom, m_rtBorder.top + m_rtBorder.bottom);
	}
	else
		MessageBeep(MB_ICONEXCLAMATION);
}
