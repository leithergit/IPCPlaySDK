
// DAVToolDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DAVTool.h"
#include "DAVToolDlg.h"
#include "afxdialogex.h"
#include <vector>
#include <algorithm>
using namespace std;

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



COLORREF CMyListCtrl::OnGetCellTextColor(int nRow, int nColum)
{
	if (!m_bColor)
	{
		return CMFCListCtrl::OnGetCellTextColor(nRow, nColum);
	}

	return(nRow % 2) == 0 ? RGB(128, 37, 0) : RGB(0, 0, 0);
}

COLORREF CMyListCtrl::OnGetCellBkColor(int nRow, int nColum)
{
	if (!m_bColor)
	{
		return CMFCListCtrl::OnGetCellBkColor(nRow, nColum);
	}

	if (m_bMarkSortedColumn && nColum == m_iSortedColumn)
	{
		return(nRow % 2) == 0 ? RGB(233, 221, 229) : RGB(176, 218, 234);
	}

	return(nRow % 2) == 0 ? RGB(253, 241, 249) : RGB(196, 238, 254);
}

HFONT CMyListCtrl::OnGetCellFont(int nRow, int nColum, DWORD /*dwData* = 0*/)
{
	if (!m_bModifyFont)
	{
		return NULL;
	}

	if (nColum == 2 && (nRow >= 4 && nRow <= 8))
	{
		return afxGlobalData.fontDefaultGUIBold;
	}

	return NULL;
}

int CMyListCtrl::OnCompareItems(LPARAM lParam1, LPARAM lParam2, int iColumn)
{
	CString strItem1 = GetItemText((int)(lParam1 < lParam2 ? lParam1 : lParam2), iColumn);
	CString strItem2 = GetItemText((int)(lParam1 < lParam2 ? lParam2 : lParam1), iColumn);

	if (iColumn == 0)
	{
		int nItem1 = _ttoi(strItem1);
		int nItem2 = _ttoi(strItem2);
		return(nItem1 < nItem2 ? -1 : 1);
	}
	else
	{
		int iSort = _tcsicmp(strItem1, strItem2);
		return(iSort);
	}
}

// CDAVToolDlg dialog

CDAVToolDlg::CDAVToolDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDAVToolDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hIconArray[0] = AfxGetApp()->LoadIcon(IDI_ICON_BACKWORD);
	m_hIconArray[1] = AfxGetApp()->LoadIcon(IDI_ICON_PLAY);
	m_hIconArray[2] = AfxGetApp()->LoadIcon(IDI_ICON_PAUSE);
	m_hIconArray[3] = AfxGetApp()->LoadIcon(IDI_ICON_STOP);
	m_hIconArray[4] = AfxGetApp()->LoadIcon(IDI_ICON_FORWORD);
	m_hIconArray[5] = AfxGetApp()->LoadIcon(IDI_ICON_SAVEMEDIA);
	m_nButtonIDArray[0] = IDC_BACKWORD;
	m_nButtonIDArray[1] = IDC_PLAY;
	m_nButtonIDArray[2] = IDC_PAUSE;
	m_nButtonIDArray[3] = IDC_STOP;
	m_nButtonIDArray[4] = IDC_FORWORD;
	m_nButtonIDArray[5] = IDC_SAVE;
	m_strButtonTips[0] = _T("Play Backword");
	m_strButtonTips[1] = _T("Play");
	m_strButtonTips[2] = _T("Pause");
	m_strButtonTips[3] = _T("Stop");
	m_strButtonTips[4] = _T("Play Forword");
	m_strButtonTips[5] = _T("Save File");
}

void CDAVToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_DIRECTROY, m_strSelectedFolder);
	for (int i = 0; i < 6;i ++)
		DDX_Control(pDX, m_nButtonIDArray[i], m_btnArray[i]);
}

BEGIN_MESSAGE_MAP(CDAVToolDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CDAVToolDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BACKWORD, &CDAVToolDlg::OnBnClickedBackword)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_FILE, &CDAVToolDlg::OnNMDblclkListFile)
END_MESSAGE_MAP()


// CDAVToolDlg message handlers

BOOL CDAVToolDlg::OnInitDialog()
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

	for (int i = 0; i < 6; i++)
	{
		m_btnArray[i].SetIcon(m_hIconArray[i]);
		m_btnArray[i].SetTooltip(m_strButtonTips[i]);
	}
	
	m_ctlFileList.SubclassDlgItem(IDC_LIST_FILE, this);
	int nCols = 0;
	m_ctlFileList.InsertColumn(nCols++, _T("No."), LVCFMT_LEFT, 50);
	m_ctlFileList.InsertColumn(nCols++, _T("No."), LVCFMT_LEFT, 160);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDAVToolDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CDAVToolDlg::OnPaint()
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
HCURSOR CDAVToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CDAVToolDlg::OnBnClickedButtonBrowse()
{
	if (theApp.GetShellManager()->BrowseForFolder(m_strSelectedFolder, this, m_strSelectedFolder))
	{
		UpdateData(FALSE);
		CFileFind finder;
		CString strFilePath = m_strSelectedFolder + _T("\\*.dav");
		BOOL bWorking = finder.FindFile(strFilePath);
		vector <CString> vecFilePath;
		while (bWorking)
		{
			bWorking = finder.FindNextFile();
			if (finder.IsDots())
				continue;
			if (finder.IsDirectory())
				continue;
			 CString strSrcFile1 = finder.GetFileName();
			vecFilePath.push_back(strSrcFile1);
		}
		sort(vecFilePath.begin(), vecFilePath.end());
		CString strText;
		int nIndex = 0;
		m_ctlFileList.DeleteAllItems();
		for (auto it = vecFilePath.begin(); it != vecFilePath.end(); it++)
		{
			strText.Format(_T("%d"), nIndex);
			m_ctlFileList.InsertItem(nIndex, strText);
			m_ctlFileList.SetItemText(nIndex ++, 1, *it);
		}
		m_nCurSelected = -1;
	}
}


void CDAVToolDlg::OnBnClickedBackword()
{
	
}


void CDAVToolDlg::OnNMDblclkListFile(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMItemActivate->iItem == -1)
		return;
	m_nCurSelected = pNMItemActivate->iItem;

	*pResult = 0;
}
