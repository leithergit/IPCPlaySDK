#pragma once
#include "stdafx.h"
#include "IPCPlaySDK.h"
#include "VideoFrame.h"
#include "Resource.h"
//#include "YuvFrame.h"

// CDialogDisplayRGB24 dialog

class CDialogDisplayRGB24 : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogDisplayRGB24)

public:
	
	CDialogDisplayRGB24(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogDisplayRGB24();
	

// Dialog Data
	enum { IDD = IDD_DIALOG_DISPLAYRGB };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
