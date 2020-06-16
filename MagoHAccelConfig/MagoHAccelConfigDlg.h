
// MagoHAccelConfigDlg.h : header file
//

#pragma once
#include "../include/CtrlsForListCtrl.h"
#include "../include/GlliteryStatic.h"

// CMagoHAccelConfigDlg dialog
class CMagoHAccelConfigDlg : public CDialogEx
{
// Construction
public:
	CMagoHAccelConfigDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MAGOHACCELCONFIG_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	CListCtrl m_listAdapter;
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonSave();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnNMClickListConfig(NMHDR *pNMHDR, LRESULT *pResult);
	LRESULT OnKillFocusCtrls(WPARAM w, LPARAM l);
	CListEdit* m_pWndItemEdit = nullptr;
	CGlliteryStatic m_ctlStatus;
	afx_msg void OnBnClickedCheckRefresh();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	void RefreshHACCel(bool bUpdateMaxHAccel = true);
};
