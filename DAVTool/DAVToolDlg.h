
// DAVToolDlg.h : header file
//

#pragma once
#include "afxshelltreectrl.h"
#include "afxbutton.h"
#include "IPCPlaySDK.h"
#pragma comment(lib,"IPCplaySDK")

enum ButtonIndex
{
	Btn_BackWord,
	Btn_Play,
	Btn_Pause,
	Btn_Stop,
	Btn_Forword,
	Btn_Save
};

class CMyListCtrl : public CMFCListCtrl
{
	virtual COLORREF OnGetCellTextColor(int nRow, int nColum);
	virtual COLORREF OnGetCellBkColor(int nRow, int nColum);
	virtual HFONT OnGetCellFont(int nRow, int nColum, DWORD dwData = 0);

	virtual int OnCompareItems(LPARAM lParam1, LPARAM lParam2, int iColumn);

public:
	BOOL m_bColor;
	BOOL m_bModifyFont;
};

// CDAVToolDlg dialog
class CDAVToolDlg : public CDialogEx
{
// Construction
public:
	CDAVToolDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_DAVTOOL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonBrowse();
	CMFCShellTreeCtrl m_wndShellTree;
	CString m_strSelectedFolder;
	afx_msg void OnBnClickedBackword();
	CMFCButton m_btnArray[6];
	HICON m_hIconArray[6];
	UINT	m_nButtonIDArray[6];
	CString m_strButtonTips[6];
	CMyListCtrl m_ctlFileList;
	int m_nCurSelected = -1;
	IPC_PLAYHANDLE	m_hPlayHandle = nullptr;
	afx_msg void OnNMDblclkListFile(NMHDR *pNMHDR, LRESULT *pResult);
	
};
