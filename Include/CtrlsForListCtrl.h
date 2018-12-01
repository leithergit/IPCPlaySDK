
#pragma once

#include "../MagoHAccelConfig/stdafx.h"
#include "Utility.h"

#define IDC_CTRL_INLIST			4096
// LIST 内控件失去焦点	WPARAM 高位为List窗口ID,低位为控件窗口ID,LPARAM 高位为Item值，低位为subItem值
#define WM_CTRLS_KILLFOCUS	WM_USER + 1920

/*
class CListMFCButton : public CMFCButton
{
	DECLARE_DYNAMIC(CListMFCButton)
public:
	CListMFCButton();
	short	m_nCurItem,m_nCurSubItem;
	virtual ~CListMFCButton();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};
*/


/////////////////////////////////////////////////////////////////////////////
// CListEdit window

class CListEdit : public CEdit
{
	// Construction
public:
	CListEdit();

	// Attributes
public:
	short	m_nCurItem,m_nCurSubItem;
	BOOL	m_bListEdit;
	CString	m_strDisableText;
	CString m_strEnableText;
	bool	m_bSkipFocusOnce;
	// Operations
public:

private:
	COLORREF m_clrText,m_clrBack;
	CBrush m_brBkgnd;
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListEdit)
	//}}AFX_VIRTUAL

	// Implementation
public:

	void SetColor(COLORREF clrText,COLORREF clrBack);
	virtual ~CListEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CListEdit)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	void	SetSkillFocusOnce(bool bFlag = true)
	{
		m_bSkipFocusOnce = bFlag;
	}
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


class CListCheckComboBox : public CComboBox
{
public:
	CListCheckComboBox();
	virtual ~CListCheckComboBox();

	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

	// Selects all/unselects the specified item
	INT SetCheck(INT nIndex, BOOL bFlag);

	// Returns checked state
	BOOL GetCheck(INT nIndex);

	short	m_nCurItem,m_nCurSubItem;
	HWND	m_hListCtrl;
	void SetListHwnd(HWND hListCtrl);

	// Selects all/unselects all
	void SelectAll(BOOL bCheck = TRUE);


protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListCheckComboBox)
protected:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CListCheckComboBox)
	afx_msg LRESULT OnCtlColorListBox(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetTextLength(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDropDown();
	afx_msg void OnCbnCloseup();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

protected:
	// Routine to update the text
	void RecalcText();

	// The subclassed COMBOLBOX window (notice the 'L')
	HWND m_hListBox;

	// The string containing the text to display
	CString m_strText;
	BOOL m_bTextUpdated;

	// A flag used in MeasureItem, see comments there
	BOOL m_bItemHeightSet;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


class CListComboBox : public CComboBox
{
	DECLARE_DYNAMIC(CListComboBox)

public:
	CListComboBox();

	short	m_nCurItem,m_nCurSubItem;
	HWND	m_hListCtrl;
	virtual ~CListComboBox();

protected:
	DECLARE_MESSAGE_MAP()
public:

	void SetListHwnd(HWND hListCtrl);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnCbnKillfocus();
	afx_msg void OnCbnCloseup();
	afx_msg void OnCbnSelendOK();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

// CListIPAddressCtrl
class CListIPAddressCtrl : public CIPAddressCtrl
{
	DECLARE_DYNAMIC(CListIPAddressCtrl)

public:
	CListIPAddressCtrl();
	virtual ~CListIPAddressCtrl();
	// Attributes
public:
	short	m_nCurItem,m_nCurSubItem;
	HWND	m_hListCtrl;
	static	UINT m_nCtrlID;
	HWND	m_hField[4];
	HWND	m_hLastFocusField;

protected:
	DECLARE_MESSAGE_MAP()
public:
	void SetListHwnd(HWND hListCtrl);
	void OnEnKillFocus();
	void OnEnSetFocus();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
};

CListEdit* CreateEditForList(CListCtrl *pListCtrl, UINT nEditCtrlID);
CListComboBox *CreateComboBoxForList(CListCtrl *pListCtrl, UINT nID);