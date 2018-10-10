#pragma once
#include <afxwin.h>         // MFC core and standard components

#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls

#include <atlbase.h>
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// TextProgressCtrl.h : header file
//
// Written by Chris Maunder (chrismaunder@codeguru.com)
// Copyright 1998.
/////////////////////////////////////////////////////////////////////////////
// CTextProgressCtrl window

class CTextProgressCtrl : public CProgressCtrl
{
// Construction
public:
    CTextProgressCtrl();

// Attributes
public:

// Operations
public:
    int         SetPos(int nPos);
    int         StepIt();
    void        SetRange(int nLower, int nUpper);
    int         OffsetPos(int nPos);
    int         SetStep(int nStep);
    void        SetForeColour(COLORREF col);
    void        SetBkColour(COLORREF col);
    void        SetTextForeColour(COLORREF col);
    void        SetTextBkColour(COLORREF col);
    COLORREF    GetForeColour();
    COLORREF    GetBkColour();
    COLORREF    GetTextForeColour();
    COLORREF    GetTextBkColour();
    void        SetShowText(BOOL bShow);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTextProgressCtrl)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CTextProgressCtrl();

    // Generated message map functions
protected:
    int         m_nPos, 
                m_nStepSize, 
                m_nMax, 
                m_nMin;
    CString     m_strText;
    BOOL        m_bShowText;
    int         m_nBarWidth;
    COLORREF    m_colFore,
                m_colBk,
                m_colTextFore,
                m_colTextBk;

    //{{AFX_MSG(CTextProgressCtrl)
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    afx_msg LRESULT OnSetText(WPARAM  wParam,LPARAM lParam);
    afx_msg LRESULT OnGetText(WPARAM  wParam,LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXTPROGRESSCTRL_H__4C78DBBE_EFB6_11D1_AB14_203E25000000__INCLUDED_)

