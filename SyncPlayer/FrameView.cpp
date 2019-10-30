// FrameView.cpp : implementation file
//

#include "stdafx.h"
#include "SyncPlayer.h"
#include "FrameView.h"
#include "afxdialogex.h"
#include "SyncPlayerDlg.h"


// CFrameView dialog

IMPLEMENT_DYNAMIC(CFrameView, CDialogEx)

CFrameView::CFrameView(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFrameView::IDD, pParent)
{

}

CFrameView::~CFrameView()
{
}

void CFrameView::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_ctlFrameList);
}


BEGIN_MESSAGE_MAP(CFrameView, CDialogEx)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LIST1, &CFrameView::OnLvnGetdispinfoList1)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CFrameView message handlers


void CFrameView::OnLvnGetdispinfoList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	((CSyncPlayerDlg *)theApp.m_pMainWnd)->vecFrameTime;
	LV_ITEM *pItem = &(pDispInfo)->item;
	int k = pItem->iItem;
	static TCHAR szText[32];
	if (pItem->mask & LVIF_TEXT)
	{
		if (((CSyncPlayerDlg *)theApp.m_pMainWnd)->vecFrameTime[0].size() < k)
				return;
		switch (pItem->iSubItem)
		{
		case 0:
		{
			_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%d"), k);
		}
		break;
		case 1:
		{
			if (((CSyncPlayerDlg *)theApp.m_pMainWnd)->vecFrameTime[0][k]->bIFrame)
				_tcscpy_s(pItem->pszText, pItem->cchTextMax, _T("true"));
		}
			
			break;
		case 2:
		{
			_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%I64d"), ((CSyncPlayerDlg *)theApp.m_pMainWnd)->vecFrameTime[0][k]->tFrameTime);
		}
		break;
		case 3:
		{
			_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%d"), ((CSyncPlayerDlg *)theApp.m_pMainWnd)->vecFrameTime[0][k]->nFrameSize);
		}

		break;

		default:
			break;
		}
	}

	*pResult = 0;
}


BOOL CFrameView::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	int nCols = 0;
	m_ctlFrameList.SetExtendedStyle(m_ctlFrameList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	m_ctlFrameList.InsertColumn(nCols++, _T("序号"), LVCFMT_LEFT, 60);
	m_ctlFrameList.InsertColumn(nCols++, _T("帧1"), LVCFMT_LEFT, 120);
	m_ctlFrameList.InsertColumn(nCols++, _T("帧1时间"), LVCFMT_LEFT, 120);
	m_ctlFrameList.InsertColumn(nCols++, _T("帧长"), LVCFMT_LEFT, 120);
// 	m_ctlFrameList.InsertColumn(nCols++, _T("帧2时间"), LVCFMT_LEFT, 120);
// 	m_ctlFrameList.InsertColumn(nCols++, _T("时间差"), LVCFMT_LEFT, 120);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CFrameView::OnClose()
{
	ShowWindow(SW_HIDE);
	CDialogEx::OnClose();
}
