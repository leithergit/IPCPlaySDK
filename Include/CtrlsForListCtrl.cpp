
#include "CtrlsForListCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CListEdit

CListEdit::CListEdit()
{
	m_clrText = ::GetSysColor(COLOR_WINDOWTEXT);	
	m_clrBack = ::GetSysColor(COLOR_3DFACE);	
	m_brBkgnd.CreateSolidBrush(m_clrBack);
}

CListEdit::~CListEdit()
{
	m_brBkgnd.DeleteObject();
}

BEGIN_MESSAGE_MAP(CListEdit, CEdit)
	//{{AFX_MSG_MAP(CListEdit)
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
	ON_WM_KILLFOCUS()
	ON_WM_CREATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListEdit message handlers

HBRUSH CListEdit::CtlColor(CDC* pDC, UINT nCtlColor) 
{
	pDC->SetTextColor(m_clrText);
	pDC->SetBkColor(m_clrBack);
	return (HBRUSH) m_brBkgnd;

}

void CListEdit::SetColor(COLORREF clrText,COLORREF clrBack)
{
	m_clrText = clrText;
	m_clrBack = clrBack;
	m_brBkgnd.DeleteObject();
	m_brBkgnd.CreateSolidBrush(clrBack);
	InvalidateRect(nullptr);
}

void CListEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	if (m_bSkipFocusOnce)
	{
		m_bSkipFocusOnce = false;
		return;
	}
	if (m_nCurItem < 0 || m_nCurSubItem <0)
		return ;
	TCHAR szText[255] = {0};
	GetWindowText(szText,255);

	TraceMsg(_T("%s:m_nCurItem = %d,m_nCurSubItem = %d,szText = %s\r\n"),__FUNCTIONW__,m_nCurItem,m_nCurSubItem,szText);

	CListCtrl *pList = (CListCtrl *)GetParent();
	WORD nListID = (WORD)pList->GetDlgCtrlID();
	WORD nCtrlID = (WORD)GetDlgCtrlID();
	WORD nItem = m_nCurItem;
	WORD nSubItem = m_nCurSubItem;
	pList->GetParent()->PostMessage(WM_CTRLS_KILLFOCUS,MAKEWPARAM(nCtrlID,nListID),MAKELPARAM(nSubItem,nItem));
	ShowWindow(SW_HIDE);
	
}

int CListEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;
	SetFont(GetParent()->GetFont());
	SetFocus();	
	return 0;
}

// CheckComboBox.cpp 
//
// Written by Magnus Egelberg (magnus.egelberg@lundalogik.se)
//
// Copyright (C) 1999, Lundalogik AB, Sweden. All rights reserved.
// 
//


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static WNDPROC m_pWndProc = 0;
static CListCheckComboBox *m_pComboBox = 0;


BEGIN_MESSAGE_MAP(CListCheckComboBox, CComboBox)
	//{{AFX_MSG_MAP(CListCheckComboBox)
	ON_MESSAGE(WM_CTLCOLORLISTBOX, OnCtlColorListBox)
	ON_MESSAGE(WM_GETTEXT, OnGetText)
	ON_MESSAGE(WM_GETTEXTLENGTH, OnGetTextLength)
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropDown)
	ON_CONTROL_REFLECT(CBN_CLOSEUP, &CListCheckComboBox::OnCbnCloseup)
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// The subclassed COMBOLBOX message handler
//
extern "C" LRESULT FAR PASCAL ComboBoxListBoxProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{

	switch (nMsg) {

	case WM_RBUTTONDOWN: 
		{
		// If you want to select all/unselect all using the
		// right button, remove this ifdef. Personally, I don't really like it
#if FALSE

		if (m_pComboBox != 0) {
			INT nCount = m_pComboBox->GetCount();
			INT nSelCount = 0;

			for (INT i = 0; i < nCount; i++) 
			{
				if (m_pComboBox->GetCheck(i))
					nSelCount++;
			}
			m_pComboBox->SelectAll(nSelCount != nCount);

			// Make sure to invalidate this window as well
			InvalidateRect(hWnd, 0, FALSE);
			m_pComboBox->GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetWindowLong(m_pComboBox->m_hWnd, GWL_ID), CBN_SELCHANGE), (LPARAM)m_pComboBox->m_hWnd);

		}
#endif

		break;
	 }

						 // Make the combobox always return -1 as the current selection. This
						 // causes the lpDrawItemStruct->itemID in DrawItem() to be -1
						 // when the always-visible-portion of the combo is drawn
	case LB_GETCURSEL: 
		{
			return -1;
		 }
	case WM_CHAR: 
		{
		if (wParam == VK_SPACE) 
		{
			// Get the current selection
			INT nIndex = CallWindowProc(m_pWndProc, hWnd, LB_GETCURSEL, wParam, lParam);

			CRect rcItem;
			SendMessage(hWnd, LB_GETITEMRECT, nIndex, (LONG)(VOID *)&rcItem);
			InvalidateRect(hWnd, rcItem, FALSE);

			// Invert the check mark
			m_pComboBox->SetCheck(nIndex, !m_pComboBox->GetCheck(nIndex));

			// Notify that selection has changed
			m_pComboBox->GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetWindowLong(m_pComboBox->m_hWnd, GWL_ID), CBN_SELCHANGE), (LPARAM)m_pComboBox->m_hWnd);
			return 0;
		}

		break;
	}


	case WM_LBUTTONDOWN: 
		{

		CRect rcClient;
		GetClientRect(hWnd, rcClient);

		CPoint pt;
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);


		if (PtInRect(rcClient, pt)) 
		{
			INT nItemHeight = SendMessage(hWnd, LB_GETITEMHEIGHT, 0, 0);
			INT nTopIndex   = SendMessage(hWnd, LB_GETTOPINDEX, 0, 0);

			// Compute which index to check/uncheck
			INT nIndex = nTopIndex + pt.y / nItemHeight;

			CRect rcItem;
			SendMessage(hWnd, LB_GETITEMRECT, nIndex, (LONG)(VOID *)&rcItem);

			if (PtInRect(rcItem, pt)) 
			{
				// Invalidate this window
				InvalidateRect(hWnd, rcItem, FALSE);
				m_pComboBox->SetCheck(nIndex, !m_pComboBox->GetCheck(nIndex));

				// Notify that selection has changed
				m_pComboBox->GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetWindowLong(m_pComboBox->m_hWnd, GWL_ID), CBN_SELCHANGE), (LPARAM)m_pComboBox->m_hWnd);

			}
		}

		// Do the default handling now (such as close the popup
		// window when clicked outside)
		break;
	}

	case WM_LBUTTONUP: 
		{
		// Don't do anything here. This causes the combobox popup
		// windows to remain open after a selection has been made
		return 0;
		}
	}

	return CallWindowProc(m_pWndProc, hWnd, nMsg, wParam, lParam);
}


void CListCheckComboBox::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);
	if (m_nCurItem < 0 || m_nCurSubItem <0)
		return ;
	TCHAR szText[255] = {0};
	GetWindowText(szText,255);
	LVITEM lvi;
	lvi.iSubItem = m_nCurSubItem;
	lvi.pszText = (LPTSTR) szText;
	::SendMessage(m_hListCtrl, LVM_SETITEMTEXT, m_nCurItem, (LPARAM)&lvi);
	ShowWindow(SW_HIDE);	
}


CListCheckComboBox::CListCheckComboBox()
{
	m_hListBox       = 0;
	m_hListCtrl		= NULL;
	m_bTextUpdated   = FALSE;
	m_bItemHeightSet = FALSE;
}


CListCheckComboBox::~CListCheckComboBox()
{
}


BOOL CListCheckComboBox::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{

	// Remove the CBS_SIMPLE and CBS_DROPDOWN styles and add the one I'm designed for
	dwStyle &= ~0xF;
	dwStyle |= CBS_DROPDOWNLIST;

	// Make sure to use the CBS_OWNERDRAWVARIABLE style
	dwStyle |= CBS_OWNERDRAWVARIABLE;

	// Use default strings. We need the itemdata to store checkmarks
	dwStyle |= CBS_HASSTRINGS;

	return CComboBox::Create(dwStyle, rect, pParentWnd, nID);
}


LRESULT CListCheckComboBox::OnCtlColorListBox(WPARAM wParam, LPARAM lParam) 
{
	// If the listbox hasn't been subclassed yet, do so...
	if (m_hListBox == 0) {
		HWND hWnd = (HWND)lParam;

		if (hWnd != 0 && hWnd != m_hWnd) {
			// Save the listbox handle
			m_hListBox = hWnd;

			// Do the subclassing
			m_pWndProc = (WNDPROC)GetWindowLong(m_hListBox, GWL_WNDPROC);
			SetWindowLong(m_hListBox, GWL_WNDPROC, (LONG)ComboBoxListBoxProc);
		}
	}


	return DefWindowProc(WM_CTLCOLORLISTBOX, wParam, lParam);
}


void CListCheckComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	HDC dc = lpDrawItemStruct->hDC;

	CRect rcBitmap = lpDrawItemStruct->rcItem;
	CRect rcText   = lpDrawItemStruct->rcItem;

	CString strText;

	// 0 - No check, 1 - Empty check, 2 - Checked
	INT nCheck = 0;

	// Check if we are drawing the static portion of the combobox
	if ((LONG)lpDrawItemStruct->itemID < 0) {
		// Make sure the m_strText member is updated
		RecalcText();

		// Get the text
		strText = m_strText;

		// Don't draw any boxes on this item
		nCheck = 0;
	}

	// Otherwise it is one of the items
	else {
		GetLBText(lpDrawItemStruct->itemID, strText);
		nCheck = 1 + (GetItemData(lpDrawItemStruct->itemID) != 0);

		TEXTMETRIC metrics;
		GetTextMetrics(dc, &metrics);

		rcBitmap.left    = 0;
		rcBitmap.right   = rcBitmap.left + metrics.tmHeight + metrics.tmExternalLeading + 6;
		rcBitmap.top    += 1;
		rcBitmap.bottom -= 1;

		rcText.left = rcBitmap.right;
	}	 

	if (nCheck > 0) {
		SetBkColor(dc, GetSysColor(COLOR_WINDOW));
		SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));

		UINT nState = DFCS_BUTTONCHECK;

		if (nCheck > 1)
			nState |= DFCS_CHECKED;

		// Draw the checkmark using DrawFrameControl
		DrawFrameControl(dc, rcBitmap, DFC_BUTTON, nState);
	}

	if (lpDrawItemStruct->itemState & ODS_SELECTED) {
		SetBkColor(dc, GetSysColor(COLOR_HIGHLIGHT));
		SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else {
		SetBkColor(dc, GetSysColor(COLOR_WINDOW));
		SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
	}

	// Erase and draw
	
	//CFont *pFont = dc.SelectObject(GetParent()->GetFont());
	::SelectObject(dc, GetParent()->GetFont()->GetSafeHandle());

	ExtTextOut(dc, 0, 0, ETO_OPAQUE, &rcText, 0, 0, 0);
	DrawText(dc, _T(' ') + strText, strText.GetLength() + 1, &rcText, DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);

	if ((lpDrawItemStruct->itemState & (ODS_FOCUS|ODS_SELECTED)) == (ODS_FOCUS|ODS_SELECTED))
		DrawFocusRect(dc, &rcText);

}


void CListCheckComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	CClientDC dc(this);
	
	CFont *pFont = dc.SelectObject(GetParent()->GetFont());

	if (pFont != 0) {

		TEXTMETRIC metrics;
		dc.GetTextMetrics(&metrics);

		lpMeasureItemStruct->itemHeight = metrics.tmHeight + metrics.tmExternalLeading;

		// An extra height of 2 looks good I think. 
		// Otherwise the list looks a bit crowded...
		lpMeasureItemStruct->itemHeight += 2;


		// This is needed since the WM_MEASUREITEM message is sent before
		// MFC hooks everything up if used in i dialog. So adjust the
		// static portion of the combo box now
		if (!m_bItemHeightSet) {
			m_bItemHeightSet = TRUE;
			SetItemHeight(-1, lpMeasureItemStruct->itemHeight);
		}

		dc.SelectObject(pFont);
	}
}


//
// Make sure the combobox window handle is updated since
// there may be many CListCheckComboBox windows active
//
void CListCheckComboBox::OnDropDown() 
{
	m_pComboBox = this;

}


//
// Selects/unselects all items in the list
//
void CListCheckComboBox::SelectAll(BOOL bCheck)
{
	INT nCount = GetCount();

	for (INT i = 0; i < nCount; i++)
		SetCheck(i, bCheck);

}


//
// By adding this message handler, we may use CWnd::GetText()
//
LRESULT CListCheckComboBox::OnGetText(WPARAM wParam, LPARAM lParam)
{
	// Make sure the text is updated
	RecalcText();

	if (lParam == 0)
		return 0;

	// Copy the 'fake' window text
	lstrcpyn((LPTSTR)lParam, m_strText, (INT)wParam);
	return m_strText.GetLength();
}


//
// By adding this message handler, we may use CWnd::GetTextLength()
//
LRESULT CListCheckComboBox::OnGetTextLength(WPARAM, LPARAM)
{
	// Make sure the text is updated
	RecalcText();
	return m_strText.GetLength();
}


//
// This routine steps thru all the items and builds
// a string containing the checked items
//
void CListCheckComboBox::RecalcText()
{
	if (!m_bTextUpdated) {
		CString strText;

		// Get the list count
		INT nCount    = GetCount();

		// Get the list separator
// 		TCHAR szBuffer[10] = {0};
// 		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SLIST, szBuffer, sizeof(szBuffer));

		CString strSeparator = _T(";");

// 		// If none found, the the ';'
// 		if (strSeparator.GetLength() == 0)
//			strSeparator =

		// Trim extra spaces
//		strSeparator.TrimRight();

		// And one...
		//strSeparator += _T(' ');

		for (INT i = 0; i < nCount; i++) {

			if (GetItemData(i)) {
				CString strItem;
				GetLBText(i, strItem);

				if (!strText.IsEmpty())
					strText += strSeparator;

				strText += strItem;
			}
		}

		// Set the text
		m_strText = strText;

		m_bTextUpdated = TRUE;
	}
}

INT CListCheckComboBox::SetCheck(INT nIndex, BOOL bFlag)
{
	INT nResult = SetItemData(nIndex, bFlag);

	if (nResult < 0)
		return nResult;

	// Signal that the text need updating
	m_bTextUpdated = FALSE;

	// Redraw the window
	Invalidate(FALSE);

	return nResult;
}

BOOL CListCheckComboBox::GetCheck(INT nIndex)
{
	return GetItemData(nIndex);
}

void CListCheckComboBox::SetListHwnd(HWND hListCtrl)
{
	if (hListCtrl && ::IsWindow(hListCtrl))
		m_hListCtrl = hListCtrl;
}

void CListCheckComboBox::OnCbnCloseup()
{
	TCHAR szText[255] = {0};
	int nCurSel = GetCurSel();	
	GetWindowText(szText,255);	
	LVITEM lvi;
	lvi.iSubItem = m_nCurSubItem;
	lvi.pszText = (LPTSTR) szText;
	::SendMessage(m_hListCtrl, LVM_SETITEMTEXT, m_nCurItem, (LPARAM)&lvi);
	ShowWindow(SW_HIDE);
}



IMPLEMENT_DYNAMIC(CListComboBox, CComboBox)

	CListComboBox::CListComboBox()
{
	m_hListCtrl = NULL;
	m_nCurItem = -1;
	m_nCurSubItem = -1;

}

CListComboBox::~CListComboBox()
{
}

void CListComboBox::SetListHwnd(HWND hListCtrl)
{
	if (hListCtrl && IsWindow(hListCtrl))
		m_hListCtrl = hListCtrl;
}

BEGIN_MESSAGE_MAP(CListComboBox, CComboBox)
	ON_WM_KILLFOCUS()
	ON_WM_CREATE()
	ON_CONTROL_REFLECT(CBN_KILLFOCUS, &CListComboBox::OnCbnKillfocus)
	ON_CONTROL_REFLECT(CBN_CLOSEUP, &CListComboBox::OnCbnCloseup)
	//ON_CONTROL_REFLECT(CBN_SELENDOK, &CListComboBox::OnCbnSelendOK)
END_MESSAGE_MAP()

// CListComboBox message handlers

void CListComboBox::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);
	if (m_nCurItem < 0 || m_nCurSubItem <0)
		return ;
	if (!m_hListCtrl)
		return ;
	
	CListCtrl *pList = (CListCtrl *)GetParent();
	// 不再主动更新父窗口的表格内容，改由通知父窗口的方式，让其处自身更新
// 	pList->SetItemText(m_nCurItem,m_nCurSubItem,szText);

	WORD nListID = (WORD)pList->GetDlgCtrlID();
	WORD nCtrlID = (WORD)GetDlgCtrlID();
	WORD nItem = m_nCurItem;
	WORD nSubItem = m_nCurSubItem;
	pList->GetParent()->SendMessage(WM_CTRLS_KILLFOCUS,MAKEWPARAM(nCtrlID,nListID),MAKELPARAM(nSubItem,nItem));
	ShowWindow(SW_HIDE);
}

int CListComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_hListCtrl)
		return 0;
	SetFont(GetParent()->GetFont());
	SetFocus();
	return 0;
}

void CListComboBox::OnCbnKillfocus()
{	

}

void CListComboBox::OnCbnCloseup()
{
	if (!m_hListCtrl)
		return ;
	TCHAR szText[255] = {0};
	int nCurSel = GetCurSel();	
	GetWindowText(szText,255);
	LVITEM lvi;
	lvi.iSubItem = m_nCurSubItem;
	lvi.pszText = (LPTSTR) szText;
	::SendMessage(m_hListCtrl, LVM_SETITEMTEXT, m_nCurItem, (LPARAM)&lvi);

 	//CGeneralToolDlg *pDlg =  (CGeneralToolDlg *)(GetParent()->GetParent());
	int nID = GetDlgCtrlID();
	AfxTrace(_T("Ctrl ID = %d\r\n"),nID);
 	//pDlg->OnCbnSelendokComboCommand(nCurSel,nID);

	ShowWindow(SW_HIDE);
}

void CListComboBox::OnCbnSelendOK()
{
	if (!m_hListCtrl)
		return ;
	TCHAR szText[255] = {0};
	int nCurSel = GetCurSel();
	GetWindowText(szText,255);	
	((CListCtrl *)GetParent())->SetItemText(m_nCurItem,m_nCurSubItem,szText);	
	ShowWindow(SW_HIDE);
}


BOOL CListEdit::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		GetParent()->SetFocus();
		return TRUE;
	}
	return CEdit::PreTranslateMessage(pMsg);
}


BOOL CListComboBox::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		GetParent()->SetFocus();
		return TRUE;
	}

	return CComboBox::PreTranslateMessage(pMsg);
}


BOOL CListCheckComboBox::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		GetParent()->SetFocus();
		return TRUE;
	}

	return CComboBox::PreTranslateMessage(pMsg);
}


// CListIPAddressCtrl

IMPLEMENT_DYNAMIC(CListIPAddressCtrl, CIPAddressCtrl)

	CListIPAddressCtrl::CListIPAddressCtrl()
{

}

CListIPAddressCtrl::~CListIPAddressCtrl()
{
}


BEGIN_MESSAGE_MAP(CListIPAddressCtrl, CIPAddressCtrl)
	ON_WM_KILLFOCUS()
	ON_WM_CREATE()
	ON_EN_KILLFOCUS(CListIPAddressCtrl::m_nCtrlID,OnEnKillFocus)
	ON_EN_SETFOCUS(CListIPAddressCtrl::m_nCtrlID,OnEnSetFocus)
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

UINT CListIPAddressCtrl::m_nCtrlID = 0;

// CListIPAddressCtrl message handlers
void CListIPAddressCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CIPAddressCtrl::OnKillFocus(pNewWnd);
	if (m_nCurItem < 0 || m_nCurSubItem <0)
		return ;

	if (!m_hListCtrl)
		return ;
	TCHAR szText[255] = {0};
	GetWindowText(szText,255);
	TraceMsg(_T("CListIPAddressCtrl::OnKillFocus:m_nCurItem = %d,m_nCurSubItem = %d,szText = %s\r\n"),m_nCurItem,m_nCurSubItem,szText);
	((CListCtrl *)GetParent())->SetItemText(m_nCurItem,m_nCurSubItem,szText);
	//ShowWindow(SW_HIDE);
	AfxTrace(_T("WM_KILLFOCUS.\r\n"));
}


void CListIPAddressCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CIPAddressCtrl::OnSetFocus(pOldWnd);

	AfxTrace(_T("WM_SETFOCUS.\r\n"));
}

void CListIPAddressCtrl::OnEnKillFocus()
{
	if (!m_hListCtrl)
		return ;
	const MSG *pMsg  = GetCurrentMessage();
	//if (pMsg->lParam == (LPARAM)m_hField[3])
	{
		TCHAR szText[255] = {0};
		GetWindowText(szText,255);
		//TraceMsg(_T("CListIPAddressCtrl::OnKillFocus:m_nCurItem = %d,m_nCurSubItem = %d,szText = %s\r\n"),m_nCurItem,m_nCurSubItem,szText);
		AfxTrace(_T("CListIPAddressCtrl::OnEnKillFocus.\r\n"));
		 AfxTrace(_T("hWnd = %08X,message = %d,wParam = %x,lParam = %08X.\r\n"),
		 		pMsg->hwnd,pMsg->message,pMsg->wParam,pMsg->lParam);
		((CListCtrl *)GetParent())->SetItemText(m_nCurItem,m_nCurSubItem,szText);
		//ShowWindow(SW_HIDE);
	}
}

void CListIPAddressCtrl::OnEnSetFocus()
{
	const MSG *pMsg  = GetCurrentMessage();
	// pMsg->lParam为子域窗口句柄
	UINT nID = GetWindowLong((HWND)pMsg->lParam,GWL_ID);
	// 此ID仍为主窗口ID，很奇怪为什么会这样
	// m_hLastFocusField = (HWND)pMsg->lParam;

	AfxTrace(_T("CListIPAddressCtrl::OnEnSetFocus.\r\n"));
	AfxTrace(_T("hWnd = %08X,message = %d,wParam = %x,lParam = %08X.\r\n"),
				pMsg->hwnd,pMsg->message,pMsg->wParam,pMsg->lParam,nID);
}

BOOL CListIPAddressCtrl::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		GetParent()->SetFocus();
		return TRUE;
	}

	return CIPAddressCtrl::PreTranslateMessage(pMsg);
}


int CListIPAddressCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CIPAddressCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_hListCtrl)
		return 0;
	
	ZeroMemory(m_hField,sizeof(m_hField));
	int nIndex = 0;
	m_hField[nIndex] = ::GetWindow(m_hWnd, GW_CHILD);
	UINT nID = GetWindowLong(m_hField[nIndex],GWL_ID);
	AfxTrace(_T("nID = %04X\r\n"),nID);
	HWND hChild = NULL;
	while(m_hField[nIndex])
	{
		::SetWindowText(m_hField[nIndex],_T("111"));
		hChild = ::GetWindow(m_hField[nIndex], GW_HWNDNEXT);
		m_hField[++nIndex] = hChild;
		nID = GetWindowLong(m_hField[nIndex],GWL_ID);
		AfxTrace(_T("nID = %04X\r\n"),nID);
	}
	AfxTrace(_T("m_hWnd = %08x\r\n"),(long)m_hWnd);
	return 0;
}

void CListIPAddressCtrl::SetListHwnd(HWND hListCtrl)
{
	if (hListCtrl && IsWindow(hListCtrl))
		m_hListCtrl = hListCtrl;
}

// CListButton

/*
IMPLEMENT_DYNAMIC(CListMFCButton, CMFCButton)

CListMFCButton::CListMFCButton()
{

}

CListMFCButton::~CListMFCButton()
{
}


BEGIN_MESSAGE_MAP(CListMFCButton, CMFCButton)
	ON_WM_CREATE()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


// CListButton message handlers

int CListMFCButton::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMFCButton::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetFont(GetParent()->GetFont());
	SetFocus();	
	return 0;
}


BOOL CListMFCButton::PreTranslateMessage(MSG* pMsg)
{
	// 	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	// 	{
	// 		GetParent()->SetFocus();
	// 		return TRUE;
	// 	}
	return CMFCButton::PreTranslateMessage(pMsg);
}


void CListMFCButton::OnKillFocus(CWnd* pNewWnd)
{
	CMFCButton::OnKillFocus(pNewWnd);
		
}

*/

CListEdit* CreateEditForList(CListCtrl *pListCtrl, UINT nEditCtrlID)
{
	CRect rt;
	CListEdit* pItemEdit = new CListEdit;
	pItemEdit->m_bListEdit = TRUE;
	pItemEdit->Create(WS_CHILD | WS_BORDER | ES_LEFT, CRect(0, 0, 1, 1), pListCtrl, nEditCtrlID);

	pItemEdit->SetColor(GetSysColor(COLOR_BTNTEXT), GetSysColor(COLOR_BTNFACE));
	pItemEdit->ShowWindow(SW_HIDE);
	pListCtrl->GetSubItemRect(1, 1, LVIR_BOUNDS, rt);
	rt.bottom += 3;
	rt.top -= 1;
	pItemEdit->MoveWindow(&rt);

	pItemEdit->m_nCurItem = -1;
	pItemEdit->m_nCurSubItem = -1;
	pItemEdit->SetSel(0, -1);
	return pItemEdit;
}

CListComboBox *CreateComboBoxForList(CListCtrl *pListCtrl, UINT nID)
{
	// 创建组合框
	CListComboBox *pItemComboBox = new CListComboBox;
	pItemComboBox->m_hListCtrl = pListCtrl->GetSafeHwnd();
	pItemComboBox->Create(WS_CHILD | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL | CBS_NOINTEGRALHEIGHT, CRect(0, 0, 1, 1), pListCtrl, nID);
	CRect rt;
	pListCtrl->GetSubItemRect(1, 1, LVIR_BOUNDS, rt);
	rt.bottom += 3;
	rt.top -= 3;

	rt.bottom = rt.bottom + 480;
	pItemComboBox->MoveWindow(&rt);
	pItemComboBox->ShowWindow(SW_HIDE);
	return pItemComboBox;
}