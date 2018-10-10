#pragma once


// CTransparentWnd

class CTransparentWnd : public CWnd
{
	DECLARE_DYNAMIC(CTransparentWnd)

public:
	CTransparentWnd();
	virtual ~CTransparentWnd();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
};


