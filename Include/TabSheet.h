#if !defined(AFX_TABSHEET_H__42EE262D_D15F_46D5_8F26_28FD049E99F4__INCLUDED_)
#define AFX_TABSHEET_H__42EE262D_D15F_46D5_8F26_28FD049E99F4__INCLUDED_
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxwin.h>         // MFC core and standard components

#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls

#include <atlbase.h>
#include <afxdialogex.h>
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CTabSheet window
#define MAXPAGE 16

class CTabSheet : public CTabCtrl
{
// Construction
public:
	CTabSheet();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTabSheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetPageTitle(int nPageIndex,LPCTSTR lpszTitle);
	int  GetPageCount();
	CDialog * GetPage(int Index);	
	int  GetCurSel();
	int  SetCurSel(int nItem);
	void Show(UINT nIndex = 0);
	void SetRect();
	BOOL  RemovePage(int nIndex);
	BOOL  AddPage(LPCTSTR title, CDialogEx *pDialog, UINT ID,BOOL bSameTemplate = FALSE/*允许多人相同的模板*/);
	virtual ~CTabSheet();

	// Generated message map functions
protected:
	TCHAR m_szTitle[MAXPAGE][127];
	UINT  m_IDD[MAXPAGE];
	CDialogEx* m_pPages[MAXPAGE];
	int   m_nNumOfPages;
	int   m_nCurrentPage;
	//{{AFX_MSG(CTabSheet)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABSHEET_H__42EE262D_D15F_46D5_8F26_28FD049E99F4__INCLUDED_)
