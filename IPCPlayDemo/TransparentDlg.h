#pragma once


// CTransparentDlg dialog

class CTransparentDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CTransparentDlg)

public:
	CTransparentDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTransparentDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_TRANSPARENT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
};
