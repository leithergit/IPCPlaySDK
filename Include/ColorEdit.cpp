// ColorEdit.cpp : implementation file
//

#include "../MagoACS/stdafx.h"
#include "ColorEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorEdit

CColorEdit::CColorEdit()
{
	m_clrText = ::GetSysColor(COLOR_WINDOWTEXT);	
	m_clrBack = ::GetSysColor(COLOR_3DFACE);	
	m_brBkgnd.CreateSolidBrush(m_clrBack);
	m_bListEdit = FALSE;
}

CColorEdit::~CColorEdit()
{
	m_brBkgnd.DeleteObject();
}

void CColorEdit::SetInListEdit(BOOL bListEdit)
{
	m_bListEdit = bListEdit;
}

BEGIN_MESSAGE_MAP(CColorEdit, CEdit)
	//{{AFX_MSG_MAP(CColorEdit)
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
	ON_WM_KILLFOCUS()
	ON_WM_CREATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorEdit message handlers

HBRUSH CColorEdit::CtlColor(CDC* pDC, UINT nCtlColor) 
{
	pDC->SetTextColor(m_clrText);
	pDC->SetBkColor(m_clrBack);
	return (HBRUSH) m_brBkgnd;

}

void CColorEdit::SetColor(COLORREF clrText,COLORREF clrBack)
{
	m_clrText = clrText;
	m_clrBack = clrBack;
	m_brBkgnd.DeleteObject();
	m_brBkgnd.CreateSolidBrush(clrBack);
	this->Invalidate();
}

void CColorEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	if (!m_bListEdit)
		return ;
	TCHAR szText[255] = {0};
	GetWindowText(szText,255);
	((CListCtrl *)GetParent())->SetItemText(m_nCurItem,m_nCurSubItem,szText);
	ShowWindow(SW_HIDE);

	
}

int CColorEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_bListEdit)
		return 0;
	SetFont(GetParent()->GetFont());
	SetFocus();
	//SetTextAlign(::GetDC(m_hWnd),TA_LEFT|TA_BOTTOM);

	return 0;
}
