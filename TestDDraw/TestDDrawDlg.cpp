
// TestDDrawDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "TestDDraw.h"
#include "TestDDrawDlg.h"
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


// CTestDDrawDlg 对话框



CTestDDrawDlg::CTestDDrawDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTestDDrawDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestDDrawDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTestDDrawDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_DISPLAY, &CTestDDrawDlg::OnBnClickedButtonDisplay)
END_MESSAGE_MAP()


// CTestDDrawDlg 消息处理程序

BOOL CTestDDrawDlg::OnInitDialog()
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

	SendDlgItemMessage(IDC_COMBO_PIXELFMT, CB_SETCURSEL, 0, 0);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTestDDrawDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CTestDDrawDlg::OnPaint()
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
HCURSOR CTestDDrawDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

enum _Pixfmt
{
	FmtYV12 = 0, 
	FmtYUY2, 
	FmtUYVY, 
	FmtPAL8, 
	FmtRGB555, 
	FmtRGB565, 
	FmtBGR24, 
	FmtRGB32, 
	FmtBGR32
};

void CTestDDrawDlg::OnBnClickedButtonDisplay()
{
	if (m_pDDraw)
		delete m_pDDraw;
	m_pDDraw = new CDirectDraw();
	//构造表面    
	DDSURFACEDESC2 ddsd = { 0 };
	int nWidth = 640;
	int nHeight = 480;
	
	_Pixfmt nPixfmt = (_Pixfmt)SendDlgItemMessage(IDC_COMBO_PIXELFMT, CB_GETCURSEL);
	switch (nPixfmt)
	{
	case FmtYV12:
		FormatYV12::Build(ddsd, nWidth, nHeight);
		m_pDDraw->Create<FormatYV12>(::GetDlgItem(m_hWnd, IDC_STATIC_FRAME), ddsd);
		break;
	case FmtYUY2:
		FormatYUY2::Build(ddsd, nWidth, nHeight);
		m_pDDraw->Create<FormatYUY2>(::GetDlgItem(m_hWnd, IDC_STATIC_FRAME), ddsd);
		break;
	case FmtUYVY:
		FormatUYVY::Build(ddsd, nWidth, nHeight);
		m_pDDraw->Create<FormatUYVY>(::GetDlgItem(m_hWnd, IDC_STATIC_FRAME), ddsd);
		break;
	case FmtPAL8:
		FormatPAL8::Build(ddsd, nWidth, nHeight);
		m_pDDraw->Create<FormatRGB555>(::GetDlgItem(m_hWnd, IDC_STATIC_FRAME), ddsd);
		break;
	case FmtRGB555:
		FormatRGB555::Build(ddsd, nWidth, nHeight);
		m_pDDraw->Create<FormatRGB555>(::GetDlgItem(m_hWnd, IDC_STATIC_FRAME), ddsd);
		break;
	case FmtRGB565:
		FormatRGB565::Build(ddsd, nWidth, nHeight);
		m_pDDraw->Create<FormatRGB565>(::GetDlgItem(m_hWnd, IDC_STATIC_FRAME), ddsd);
		break;
	case FmtBGR24:
		FormatBGR24::Build(ddsd, nWidth, nHeight);
		m_pDDraw->Create<FormatBGR24>(::GetDlgItem(m_hWnd, IDC_STATIC_FRAME), ddsd);
		break;
	case FmtRGB32:
		FormatRGB32::Build(ddsd, nWidth, nHeight);
		m_pDDraw->Create<FormatRGB32>(::GetDlgItem(m_hWnd, IDC_STATIC_FRAME), ddsd);
		break;
	case FmtBGR32:
		FormatBGR32::Build(ddsd, nWidth, nHeight);
		m_pDDraw->Create<FormatBGR32>(::GetDlgItem(m_hWnd, IDC_STATIC_FRAME), ddsd);
		break;
	default:
		break;
	}
}
