#pragma once
#include "afxcmn.h"


// CFrameView dialog

class CFrameView : public CDialogEx
{
	DECLARE_DYNAMIC(CFrameView)

public:
	CFrameView(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFrameView();

// Dialog Data
	enum { IDD = IDD_FRAME_VIEW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_ctlFrameList;
	afx_msg void OnLvnGetdispinfoList1(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
};
