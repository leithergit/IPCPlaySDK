
// MagoHAccelConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MagoHAccelConfig.h"
#include "MagoHAccelConfigDlg.h"
#include "afxdialogex.h"
#include <string>
#include <map>
using namespace std;

#include "../IPCPlaySDK/IPCPlaySDK.h"
#include "../include/utility.h"
#ifdef _DEBUG
#pragma comment(lib,"../Debug/IPCPlaySDK.lib")
#else
#pragma comment(lib,"../release/IPCPlaySDK.lib")
#endif
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

enum Adapter_Item
{
	Item_No,
	Item_Adapter,
	Item_Guid,
	Item_MaxHAccel,
	Item_HaccelUsed
};

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


// CMagoHAccelConfigDlg dialog



CMagoHAccelConfigDlg::CMagoHAccelConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMagoHAccelConfigDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMagoHAccelConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMagoHAccelConfigDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_SAVE, &CMagoHAccelConfigDlg::OnBnClickedButtonSave)
	ON_BN_CLICKED(IDCANCEL, &CMagoHAccelConfigDlg::OnBnClickedCancel)
	ON_NOTIFY(NM_CLICK, IDC_LIST_CONFIG, &CMagoHAccelConfigDlg::OnNMClickListConfig)
	ON_MESSAGE(WM_CTRLS_KILLFOCUS, &CMagoHAccelConfigDlg::OnKillFocusCtrls)
	ON_BN_CLICKED(IDC_CHECK_REFRESH, &CMagoHAccelConfigDlg::OnBnClickedCheckRefresh)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CMagoHAccelConfigDlg message handlers

BOOL CMagoHAccelConfigDlg::OnInitDialog()
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

	CenterWindow(this);
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	m_ctlStatus.SubclassDlgItem(IDC_STATIC_STATUS, this);

	m_listAdapter.SubclassDlgItem(IDC_LIST_CONFIG, this);
	m_listAdapter.SetExtendedStyle(m_listAdapter.GetExtendedStyle() | LVS_EX_FULLROWSELECT  );

	int nCols = 0;
	CDC *pDC = m_listAdapter.GetHeaderCtrl()->GetDC();
	TCHAR* strItemName[] = { _T("No."), _T("Display Adapter"), _T("Guid"),/* _T("Display"),*/ /*_T("Physical No."),*/ _T("MaxHAccel"), _T("HAccel Used") };
	SIZE sz;
	for (int nCol = 0; nCol < sizeof(strItemName) / sizeof(TCHAR*); nCol++)
	{
		sz = pDC->GetOutputTextExtent(strItemName[nCol], _tcslen(strItemName[nCol]));
		m_listAdapter.InsertColumn(nCol, strItemName[nCol], LVCFMT_LEFT, sz.cx + 10);
		//m_listAdapter.SetColumnWidth(nCol, LVSCW_AUTOSIZE_USEHEADER);
	}
	ReleaseDC(pDC);

	m_pWndItemEdit = CreateEditForList(&m_listAdapter, 1024);
	AdapterInfo AdapterArray[64] = { 0 };
	int nAdapterCount = 64;
	ipcplay_GetDisplayAdapterInfo(AdapterArray, nAdapterCount);
	TCHAR szItemText[64] = { 0 };
	map<string, int> mapAdapter;
	
	m_listAdapter.SetRedraw(FALSE);
	AdapterHAccel *pHAConfig;
	int nAdapterCount2 = 0;
	ipcplay_GetHAccelConfig(&pHAConfig, nAdapterCount2);
	for (int i = 0; i < nAdapterCount; i++)
	{
		WCHAR szGuidW[64] = { 0 };
		StringFromGUID2(AdapterArray[i].DeviceIdentifier, szGuidW, 64);
		CHAR szGuidA[64] = { 0 };
		W2AHelper(szGuidW, szGuidA, 64);
		if (mapAdapter.size() > 0)
		{
			auto itFind = mapAdapter.find(szGuidA);
			if (itFind != mapAdapter.end())
				continue;
			else
				mapAdapter.insert(pair<string, int>(szGuidA, 1));
		}
		else
			mapAdapter.insert(pair<string, int>(szGuidA, 1));
		int nItem = mapAdapter.size() -1;
		_stprintf_s(szItemText, 64, _T("%d"), i +1);
		m_listAdapter.InsertItem(i, szItemText);
		int nSubitem = 1;
		m_listAdapter.SetItemText(i, nSubitem++, (LPCTSTR)AdapterArray[i].Description);
		m_listAdapter.SetItemText(i, nSubitem++, (LPCTSTR)szGuidA);
		//m_listAdapter.SetItemText(i, nSubitem++, (LPCTSTR)AdapterArray[i].DeviceName);
		//m_listAdapter.SetItemText(i, nSubitem++, (LPCTSTR)_T("0"));
		int nMaxHaccel = 0;
		for (int nIndex = 0; nIndex < nAdapterCount2; nIndex++)
		{
			if (strcmp(pHAConfig[nIndex].szAdapterGuid, szGuidA) == 0)
			{
				nMaxHaccel = pHAConfig[nIndex].nMaxHaccel;
				break;
			}
		}
		_stprintf_s(szItemText, 64, _T("%d"), nMaxHaccel);
		m_listAdapter.SetItemText(i, nSubitem++, szItemText);
	}

	for (int nCol = 0; nCol < sizeof(strItemName) / sizeof(TCHAR*); nCol++)
	{
		m_listAdapter.SetColumnWidth(nCol, LVSCW_AUTOSIZE);
		int nColumnWidth = m_listAdapter.GetColumnWidth(nCol);
		m_listAdapter.SetColumnWidth(nCol, LVSCW_AUTOSIZE_USEHEADER);
		int nHeaderWidth = m_listAdapter.GetColumnWidth(nCol);
		m_listAdapter.SetColumnWidth(nCol, max(nColumnWidth, nHeaderWidth));
	}
	
	m_listAdapter.SetRedraw(TRUE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMagoHAccelConfigDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMagoHAccelConfigDlg::OnPaint()
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
HCURSOR CMagoHAccelConfigDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMagoHAccelConfigDlg::OnBnClickedButtonSave()
{
	DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	TCHAR szPath[1024] = { 0 };
	GetAppPath(szPath, 1024);
	_tcscat_s(szPath, _T("\\RenderOption.ini"));

	int nAdapterCount = m_listAdapter.GetItemCount();
	TCHAR szGUID[64] = { 0 };
	TCHAR szAdapter[32] = { 0 };
	TCHAR szHaccel[16] = { 0 };
	TCHAR szValue[128] = { 0 };
	TCHAR szKey[32] = { 0 };
	for (int i = 0; i < nAdapterCount; i++)
	{
		m_listAdapter.GetItemText(i, 2, szGUID,64);
		m_listAdapter.GetItemText(i, 3, szHaccel,16);
		_stprintf_s(szValue, 128, _T("%s,%s"), szHaccel, szGUID);
		_stprintf_s(szKey, 32, _T("Adapter%d"), i+1);
		WritePrivateProfileString(_T("HAccel"), szKey, szValue, szPath);
	}
	if (IsDlgButtonChecked(IDC_CHECK_HAPREFERED) == BST_CHECKED)
		WritePrivateProfileString(_T("HAccel"), _T("Prefered"), "1", szPath);
	else
		WritePrivateProfileString(_T("HAccel"), _T("Prefered"), "0", szPath);
	TCHAR szText[128] = { 0 };
	_stprintf_s(szText, 128, _T("The Configuration was saved to file:%s.\n"), szPath);
	m_ctlStatus.SetWindowText(szText);
	m_ctlStatus.SetOkGllitery();

}


void CMagoHAccelConfigDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnCancel();
}


void CMagoHAccelConfigDlg::OnNMClickListConfig(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMItemActivate->iItem == -1)
		return;
	CRect rt;
	m_listAdapter.GetSubItemRect(pNMItemActivate->iItem, pNMItemActivate->iSubItem, LVIR_BOUNDS, rt);
	rt.bottom += 3;
	rt.top -= 1;
	CString strItemText = m_listAdapter.GetItemText(pNMItemActivate->iItem, pNMItemActivate->iSubItem);
	if (pNMItemActivate->iSubItem == 3 
		/*||pNMItemActivate->iSubItem == 5*/)
	{
		m_pWndItemEdit->SetWindowText(strItemText);
		m_pWndItemEdit->MoveWindow(&rt);
		m_pWndItemEdit->SetFocus();
		m_pWndItemEdit->m_nCurItem = pNMItemActivate->iItem;
		m_pWndItemEdit->m_nCurSubItem = pNMItemActivate->iSubItem;
		m_pWndItemEdit->SetSel(0, -1);
		m_pWndItemEdit->ShowWindow(SW_SHOW);
	}
	
	m_listAdapter.EnsureVisible(pNMItemActivate->iItem, FALSE);
	*pResult = 0;
}


// 响应WM_CTRLS_KILLFOCUS消息
// LIST 内控件失去焦点
// WPARAM 高位为List窗口ID,低位为控件窗口ID,LPARAM 高位为Item值，低位为subItem值

LRESULT CMagoHAccelConfigDlg::OnKillFocusCtrls(WPARAM w, LPARAM l)
{
	UINT nListID = HIWORD(w);
	UINT nCtrlID = LOWORD(w);
	int nItem = HIWORD(l);
	int nSubItem = LOWORD(l);
	CString strItemText;
	
	if (nCtrlID == m_pWndItemEdit->GetDlgCtrlID())
	{
		m_pWndItemEdit->GetWindowText(strItemText);
		if (nSubItem == 3 
			/*||nSubItem == 5*/)
		{
			if (strItemText.GetLength() == 0)
				m_listAdapter.SetItemText(nItem, nSubItem, _T("0"));
			else
				m_listAdapter.SetItemText(nItem, nSubItem, strItemText);
		}		
	}

	return 0;
}


void CMagoHAccelConfigDlg::OnBnClickedCheckRefresh()
{
	if (IsDlgButtonChecked(IDC_CHECK_REFRESH) == BST_CHECKED)
	{
		SetTimer(1024, 200, nullptr);
	}
	else
	{
		KillTimer(1024);
	}
}


void CMagoHAccelConfigDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1024)
	{
		AdapterHAccel *pHAConfig;
		int nAdapterCount2 = 0;
		ipcplay_GetHAccelConfig(&pHAConfig, nAdapterCount2);
		int nItemCount = m_listAdapter.GetItemCount();
		char szGuid[10][64] = { 0 };
		for (int nItem = 0; nItem < nItemCount; nItem++)
			m_listAdapter.GetItemText(nItem, Item_Guid, szGuid[nItem],64);

		TCHAR szItemText[128] = { 0 };
		for (int nAdapter = 0; nAdapter < nAdapterCount2; nAdapter++)
		{
			for (int nItem = 0; nItem < nItemCount; nItem++)
			{
				if (strcmp(pHAConfig[nAdapter].szAdapterGuid, szGuid[nItem]) == 0)
				{
					_stprintf_s(szItemText, 128,_T("%d"), pHAConfig[nAdapter].nOpenCount);
					m_listAdapter.SetItemText(nItem,Item_HaccelUsed, szItemText);
					break;
				}
			}
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}
