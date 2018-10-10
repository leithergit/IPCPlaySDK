
// RapidPlayDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RapidPlay.h"
#include "RapidPlayDlg.h"
#include "afxdialogex.h"

#pragma comment(lib,"ipcplaysdk.lib")


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
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


// CRapidPlayDlg dialog



CRapidPlayDlg::CRapidPlayDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRapidPlayDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRapidPlayDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FILE, m_ctlListFile);
}

BEGIN_MESSAGE_MAP(CRapidPlayDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CRapidPlayDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_START, &CRapidPlayDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &CRapidPlayDlg::OnBnClickedButtonPause)
	ON_BN_CLICKED(IDC_BUTTON_CAPTURE, &CRapidPlayDlg::OnBnClickedButtonCapture)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CRapidPlayDlg::OnBnClickedButtonStop)
END_MESSAGE_MAP()


// CRapidPlayDlg message handlers

BOOL CRapidPlayDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon


	m_ctlListFile.SetExtendedStyle(m_ctlListFile.GetExtendedStyle() | LVS_EX_FULLROWSELECT  | LVS_EX_CHECKBOXES);

	int nCols = 0;
	m_ctlListFile.InsertColumn(nCols++, _T("NO."), LVCFMT_LEFT, 60);
	m_ctlListFile.InsertColumn(nCols++, _T("File Name"), LVCFMT_LEFT, 240);
	m_hPlayHandle = nullptr;
	m_nCurFileIndex = -1;
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRapidPlayDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRapidPlayDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRapidPlayDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CRapidPlayDlg::OnBnClickedButtonBrowse()
{
	TCHAR szText[1024] = { 0 };

	DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ALLOWMULTISELECT;
	TCHAR  szFilter[] = _T("Viedo File (*.mp4)|*.mp4|Video File(*.dav)|*.dav|H.264 Video File(*.H264)|*.H264|H.265 Video File(*.H265)|*.H265|All Files (*.*)|*.*||");
	TCHAR szExportLog[MAX_PATH] = { 0 };
	CTime tNow = CTime::GetCurrentTime();
	CFileDialog OpenFile(TRUE, _T("*.mp4"), _T(""), dwFlags, (LPCTSTR)szFilter);
	OpenFile.m_ofn.lpstrTitle = _T("Please Select the files to play");
	CString strFilePath;

	const int nMaxFiles = 256;
	const int nMaxPathBuffer = (nMaxFiles * 1024) + 1;
	LPWSTR pPathBuffer = new WCHAR[nMaxPathBuffer * sizeof(WCHAR)];
	if (pPathBuffer)
	{
		ZeroMemory(pPathBuffer, sizeof(WCHAR)*nMaxPathBuffer);
		OpenFile.m_ofn.lpstrFile = pPathBuffer;
		
		OpenFile.m_ofn.nMaxFile = nMaxFiles;
		if (OpenFile.DoModal() == IDOK)
		{
			m_ctlListFile.DeleteAllItems();
			int nIndex = 0;
			TCHAR szText[256] = { 0 };
			POSITION posStart = OpenFile.GetStartPosition();
			while (posStart)
			{
				CString strFile = OpenFile.GetNextPathName(posStart);
				int nPos = strFile.ReverseFind(_T('\\'));
				int nLength = strFile.GetLength();
				m_strDirectory = strFile.Left(nPos);

				_stprintf_s(szText, 256, _T("%d"), nIndex + 1);
				m_ctlListFile.InsertItem(nIndex, szText);
				m_ctlListFile.SetItemText(nIndex, 1, strFile.Right(nLength - nPos - 1));
				nIndex++;
			}
			SetDlgItemText(IDC_EDIT_DIRECTORY, m_strDirectory);
			m_nCurFileIndex = 0;
		}
	}
	delete[]pPathBuffer;
}


void CRapidPlayDlg::OnBnClickedButtonStart()
{
	int nCount = m_ctlListFile.GetItemCount();
	if (nCount <= 0)
	{
		AfxMessageBox(_T("Please select some files to play."));
		return;
	}
	if (!m_hThreadParserFile)
	{
		m_bParserFile = true;
		if (m_nCurFileIndex == -1)
			m_nCurFileIndex = 0;
		m_hThreadParserFile = (HANDLE)_beginthreadex(nullptr, 0, ThreadParserFile, this, 0, 0);
	}
	else
		AfxMessageBox(_T("Player has beed started."));


}


void CRapidPlayDlg::OnBnClickedButtonPause()
{
	if (hPlayHandle)
		ipcplay_Pause(hPlayHandle);

}


void CRapidPlayDlg::OnBnClickedButtonCapture()
{

}


void CRapidPlayDlg::OnTimer(UINT_PTR nIDEvent)
{

	CDialogEx::OnTimer(nIDEvent);
}


void CRapidPlayDlg::OnBnClickedButtonStop()
{
	CWaitCursor Wait;
	m_bParserFile = false;

	EventDelay(m_hThreadParserFile, 5000);
	CloseHandle(m_hThreadParserFile);
	m_hThreadParserFile = nullptr;
}
