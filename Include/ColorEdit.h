#if !defined(AFX_COLOREDIT_H__B65ACD44_D13C_4951_9B37_02D0153E930E__INCLUDED_)
#define AFX_COLOREDIT_H__B65ACD44_D13C_4951_9B37_02D0153E930E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ColorEdit.h : header file
//
#include "stdafx.h"
/////////////////////////////////////////////////////////////////////////////
// CColorEdit window

class CColorEdit : public CEdit
{
// Construction
public:
	CColorEdit();

// Attributes
public:
	short	m_nCurItem,m_nCurSubItem;
// Operations
public:
	
private:
	COLORREF m_clrText,m_clrBack;
	CBrush m_brBkgnd;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL	m_bListEdit;
	void SetColor(COLORREF clrText,COLORREF clrBack);
	void SetInListEdit(BOOL bListEdit = TRUE);
	virtual ~CColorEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorEdit)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLOREDIT_H__B65ACD44_D13C_4951_9B37_02D0153E930E__INCLUDED_)
