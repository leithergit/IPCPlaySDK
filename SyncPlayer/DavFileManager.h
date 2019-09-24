#pragma once
#include "afxeditbrowsectrl.h"
#include "afxcmn.h"
#include "afxdtctl.h"
#include "atltime.h"


// CDavFileManager dialog

class CDavFileManager : public CDialogEx
{
	DECLARE_DYNAMIC(CDavFileManager)

public:
	CDavFileManager(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDavFileManager();

// Dialog Data
	enum { IDD = IDD_DIALOG_FILEMANAGER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:

	virtual BOOL OnInitDialog();
	CListCtrl m_ctlListFile;
	afx_msg void OnBnClickedButtonBrowse();
	CDateTimeCtrl m_ctlDateTimer;
	afx_msg void OnBnClickedButtonCut();
	afx_msg void OnBnClickedButtonMerge();
	afx_msg void OnNMClickListFile(NMHDR *pNMHDR, LRESULT *pResult);
	CTime m_tFirstFrame;
	BOOL m_bCheckStartTime;
};
