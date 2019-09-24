#pragma once


// CDialogVideo dialog

class CDialogVideo : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogVideo)

public:
	CDialogVideo(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogVideo();

// Dialog Data
	enum { IDD = IDD_DIALOG_VIDEO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
