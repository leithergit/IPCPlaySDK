// TabSheet.cpp : implementation file
//

//#include "stdafx.h"
#include "TabSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabSheet

CTabSheet::CTabSheet()
{
	m_nNumOfPages = 0;
	m_nCurrentPage = 0;
	for (int i = 0;i < MAXPAGE; i ++)
		m_pPages[i] = NULL;
}

CTabSheet::~CTabSheet()
{
}


BEGIN_MESSAGE_MAP(CTabSheet, CTabCtrl)
	//{{AFX_MSG_MAP(CTabSheet)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabSheet message handlers

BOOL CTabSheet::AddPage(LPCTSTR title, CDialogEx *pDialog,UINT ID,BOOL bSameTemplate)
{
	if( MAXPAGE == m_nNumOfPages )
		return FALSE;
	
	if (!bSameTemplate)
		for (int i=0;i<m_nNumOfPages;i++)
		{
			if (m_IDD[i]==ID)
				return TRUE;
		}	
	m_pPages[m_nNumOfPages] = pDialog;
	m_IDD[m_nNumOfPages] = ID;
	_tcscpy_s(m_szTitle[m_nNumOfPages],127,title);
	m_nNumOfPages++;
	return TRUE;
}

BOOL CTabSheet::RemovePage(int nIndex)
{
	if( nIndex < 0 || nIndex >= m_nNumOfPages)
		return FALSE;
	CTabCtrl::DeleteItem(nIndex);
	m_pPages[nIndex]->DestroyWindow();
	for (int i = nIndex ; i < (m_nNumOfPages - 1); i ++)
	{//0 1 2 x 4 5 6 7 
		m_pPages[i] = m_pPages[i + 1];
		m_IDD[i] = m_IDD[i + i];
		_tcscpy_s(m_szTitle[i],127,m_szTitle[i + i]);
	}
	
	m_nNumOfPages --;
	if (m_nCurrentPage >= m_nNumOfPages)
		m_nCurrentPage = 0;
	m_pPages[m_nNumOfPages] = NULL;
	_tcscpy_s(m_szTitle[m_nNumOfPages],127,_T(""));
	m_IDD[m_nNumOfPages] = 0;
	return TRUE;
}

void CTabSheet::SetRect()
{
	CRect tabRect, itemRect;
	int nX, nY, nXc, nYc;

	GetClientRect(&tabRect);
	GetItemRect(0, &itemRect);

	nX=itemRect.left;
	nY=itemRect.bottom+1;
	nXc=tabRect.right-itemRect.left-2;
	nYc=tabRect.bottom-nY-2;

	m_pPages[0]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_SHOWWINDOW);
	for( int nCount=1; nCount < m_nNumOfPages; nCount++ )
		m_pPages[nCount]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_HIDEWINDOW);

}

void CTabSheet::Show(UINT nIndex)
{
	int i = 0;
	for(i=0; i < m_nNumOfPages; i++ )
	{
		if (m_pPages[i] != NULL && !IsWindow(m_pPages[i]->m_hWnd)) 
		{
			m_pPages[i]->Create( m_IDD[i], this );
			InsertItem( i, m_szTitle[i] );
		}		
	}
	for(i=1; i < m_nNumOfPages; i++)
		m_pPages[i]->ShowWindow(SW_HIDE);
	m_pPages[nIndex]->ShowWindow(SW_SHOW);			//每次显示都以第一个页面为默认页面
	SetCurSel(nIndex);
	SetCurFocus(nIndex);	
	SetRect();
}

void CTabSheet::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CTabCtrl::OnLButtonDown(nFlags, point);
	int Focus=GetCurFocus();
	if(m_nCurrentPage != Focus)
	{
		m_pPages[m_nCurrentPage]->ShowWindow(SW_HIDE);
		m_nCurrentPage=GetCurFocus();
		m_pPages[m_nCurrentPage]->ShowWindow(SW_SHOW);
		m_pPages[m_nCurrentPage]->SetFocus();
		SetCurSel(m_nCurrentPage);
	}
	UpdateData(TRUE);
}

int CTabSheet::SetCurSel(int nItem)
{
	if( nItem < 0 || nItem >= m_nNumOfPages)
		return -1;

	int ret = m_nCurrentPage;

	if(m_nCurrentPage != nItem )
	{
		m_pPages[m_nCurrentPage]->ShowWindow(SW_HIDE);
		m_nCurrentPage = nItem;
		m_pPages[m_nCurrentPage]->ShowWindow(SW_SHOW);
		m_pPages[m_nCurrentPage]->SetFocus();
		CTabCtrl::SetCurSel(nItem);
	}

	return ret;
}

int CTabSheet::GetCurSel()
{
	return CTabCtrl::GetCurSel();
}

CDialog * CTabSheet::GetPage(int Index)
{
	return m_pPages[Index];
}

int CTabSheet::GetPageCount()
{
	return m_nNumOfPages;
}

void CTabSheet::SetPageTitle(int nPageIndex, LPCTSTR lpszTitle)
{	
	if( nPageIndex < 0 || nPageIndex >= m_nNumOfPages)
		return ;
	TCITEM Item;
	Item.mask = TCIF_TEXT;
	//GetItem(nPageIndex,&Item);
	_tcscpy_s(m_szTitle[nPageIndex],127,lpszTitle);
	Item.pszText = m_szTitle[nPageIndex];
	SetItem(nPageIndex,&Item);

}
