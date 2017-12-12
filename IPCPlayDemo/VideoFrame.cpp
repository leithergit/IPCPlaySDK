// VideoFrame.cpp : implementation file
//

#include "stdafx.h"
#include "VideoFrame.h"
#include <algorithm>
// #include "..\MgMonitor\MgMonitorDlg.h"
// #include "..\MgMonitor\Resource.h"
//#include "..\MgMonitor\Redraw.h"
//#include "..\interface\CImgButton.h"

// CVideoFrame

IMPLEMENT_DYNAMIC(CVideoFrame, CWnd)

map<HWND, HWND>CVideoFrame::m_PanelMap;
CCriticalSectionProxyPtr CVideoFrame::m_csPannelMap = make_shared<CCriticalSectionProxy>();
CVideoFrame *CVideoFrame::m_pCurrentFrame = nullptr;



LONG CVideoFrame::m_nMouseMessageArray[] =
{
	WM_FRAME_MOUSEMOVE,                   //0x0200
	WM_FRAME_LBUTTONDOWN,                 //0x0201
	WM_FRAME_LBUTTONUP,                   //0x0202
	WM_FRAME_LBUTTONDBLCLK,               //0x0203
	WM_FRAME_RBUTTONDOWN,                 //0x0204
	WM_FRAME_RBUTTONUP,                   //0x0205
	WM_FRAME_RBUTTONDBLCLK,               //0x0206
	WM_FRAME_MBUTTONDOWN,                 //0x0207
	WM_FRAME_MBUTTONUP,                   //0x0208
	WM_FRAME_MBUTTONDBLCLK,               //0x0209
	WM_FRAME_MOUSEWHEEL					  //0x020A
};

CVideoFrame::CVideoFrame()
{
	m_nCols = m_nRows = 0;
	m_nNewCols = m_nNewRows = 0;
	m_nPannelUsed = 0;		//  已用窗格数量
	m_pSelectedPen = NULL;
	m_pUnSelectedPen = NULL;
	m_pRestorePen = NULL;
	m_pCurSelectRect = nullptr;
	m_pLastSelectRect =  nullptr;
	m_nCurPanel = -1;
	m_nFrameStyle = StyleNormal;
	m_csvecPanel = make_shared<CCriticalSectionProxy>();
	m_cslistRecyclePanel = make_shared<CCriticalSectionProxy>();

}

CVideoFrame::~CVideoFrame()
{
	if (m_pSelectedPen)
		delete m_pSelectedPen;
	if (m_pUnSelectedPen)
		delete m_pUnSelectedPen;
	if (m_pRestorePen)
		delete m_pRestorePen;
}

BEGIN_MESSAGE_MAP(CVideoFrame, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()
	//ON_WM_MOUSEWHEEL()
	ON_MESSAGE(WM_MOUSEHWHEEL, OnMouseWheel)
END_MESSAGE_MAP()

// CVideoFrame message handlers

BOOL CVideoFrame::Create(UINT nID, const RECT& rect,int nCount,CWnd* pParentWnd,  CCreateContext* pContext)
{
	UINT nRows = 0,nCols = 0;
	CalcRowCol(nCount,nRows,nCols);
	return Create(nID,rect,nRows,nCols,pParentWnd,pContext);
}

BOOL CVideoFrame::Create(UINT nID, const RECT& rect,int nRow,int nCol, CWnd* pParentWnd, CCreateContext* pContext)
{
	// can't use for desktop or pop-up windows (use CreateEx instead)
	ASSERT(pParentWnd != NULL);
	ASSERT(nID != 0);
	ASSERT(nRow != 0 && nCol != 0);
	m_nNewRows = nRow;
	m_nNewCols = nCol;

	LPCTSTR szWndClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH) ::GetStockObject(BLACK_BRUSH), NULL);
	return CreateEx(0, szWndClass, _T("VideoFrame"),
		WS_CHILD | WS_VISIBLE,
		rect.left, rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		pParentWnd->GetSafeHwnd(),
		(HMENU)(UINT_PTR)nID,
		(LPVOID)pContext);
}

BOOL CVideoFrame::Create(UINT nID, const RECT& rect,FrameStyle nStyle,CWnd* pParentWnd,  CCreateContext* pContext)
{
	ASSERT(pParentWnd != NULL);
	ASSERT(nID != 0);
	//ASSERT(nRow != 0 && nCol != 0);
	ASSERT(nStyle != StyleNormal);
	if (nStyle == StyleNormal)
		return FALSE;
	m_nFrameStyle = nStyle;
	switch(nStyle)
	{
	case StyleNormal:
	default:
		break;
	case Style_5P1:
		m_nRows = m_nCols = 3;
		break;
	case Style_7P1:
		m_nRows = m_nCols = 4;
		break;
	case Style_9P1:
		m_nRows = m_nCols = 5;
		break;
	}

	LPCTSTR szWndClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH) ::GetStockObject(BLACK_BRUSH), NULL);
	return CreateEx(0, szWndClass, _T("VideoFrame"),
		WS_CHILD | WS_VISIBLE,
		rect.left, rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		pParentWnd->GetSafeHwnd(),
		(HMENU)(UINT_PTR)nID,
		(LPVOID)pContext);
	
}
BOOL CVideoFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style &= ~WS_BORDER | ~WS_CAPTION | ~WS_SYSMENU | ~WS_THICKFRAME | ~WS_VSCROLL | ~WS_HSCROLL;
	cs.style |= WS_CHILD | WS_VISIBLE;
	
	return CWnd::PreCreateWindow(cs);
}

#define BackColor	RGB(0,0,0)
#define SelectColor	RGB(255,255,0)
int CVideoFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	m_pSelectedPen = new CPen(PS_SOLID, 1, SelectColor);
	m_pUnSelectedPen = new CPen(PS_SOLID, _GRID_LINE_WIDTH, BackColor);
	m_pRestorePen = new CPen(PS_SOLID, 1, BackColor);
	m_hCursorArrow = ::LoadCursor(NULL, IDC_ARROW);
	m_hCursorHand = ::LoadCursor(NULL, IDC_HAND);
	
	AdjustPanels(m_nNewRows, m_nNewCols);

	return 0;
}

// 调整窗格数量
// 如果是新增窗格，则不需要删除原有的窗格
// 如果是缩减窗格，则仅删除多余的窗格
bool CVideoFrame::AdjustPanels(int nRows, int nCols,FrameStyle fs)
{
	ASSERT(nRows != 0 && nCols != 0);
	if (!nRows || !nCols)
		return false;
	int nPanelCount = nRows*nCols;
	if (fs == StyleNormal)
	{
		// 行列数相同，不作调整
		if (fs == m_nFrameStyle && 
			nRows == m_nRows && 
			nCols == m_nCols )
			return true;
	}
		
	m_nFrameStyle = fs;
	switch(m_nFrameStyle)
	{
	case StyleNormal:
	default:
		break;
	case Style_2P1:
		nPanelCount = 3;
		nRows = 2;
		nCols = 2;
		break;
	case Style_5P1:
		nRows = nCols = 3;
		nPanelCount = 6;
		break;
	case Style_7P1:
		nRows = nCols = 4;
		nPanelCount = 8;
		break;
	case Style_9P1:
		nRows = nCols = 5;
		nPanelCount = 10;
		break;
	case Style_11P1:
		nRows = nCols = 6;
		nPanelCount = 12;
		break;
	case Style_13P1:
		nRows = nCols = 7;
		nPanelCount = 14;
		break;
	case Style_15P1:
		nRows = nCols = 8;
		nPanelCount = 16;
		break;
	}
	
	//缩减窗格
	if (nPanelCount < m_vecPanel.size())
	{
		int nReduceCount = m_vecPanel.size() - nPanelCount;
		int nReduced = 0;
		// 从后往前删除
		for (vector<PanelInfoPtr>::reverse_iterator rit = m_vecPanel.rbegin() ;rit != m_vecPanel.rend() && nReduced < nReduceCount;)
		{
			::ShowWindow((*rit)->hWnd,SW_HIDE);
			//::InvalidateRect((*rit)->hWnd, NULL, TRUE);
			m_listRecyclePanel.push_back(*rit);
			rit = vector<PanelInfoPtr>::reverse_iterator(m_vecPanel.erase((++rit).base()));
			nReduced ++;
		}
		// 调整窗格尺寸
	}
	else if (nPanelCount > m_vecPanel.size())	// 新增窗格
	{
		int nAddCount = nPanelCount - m_vecPanel.size();
		// 优先从回收窗口表中取回窗口
		int nSize = m_listRecyclePanel.size();
		if (nAddCount >= nSize)
		{// 回收窗口数大于需要窗口数
			for (list<PanelInfoPtr>::reverse_iterator rit = m_listRecyclePanel.rbegin();rit != m_listRecyclePanel.rend();)
			{
				::ShowWindow((*rit)->hWnd,SW_SHOW);
				//::InvalidateRect((*rit)->hWnd, NULL, TRUE);
				m_vecPanel.push_back(*rit);
				rit = list<PanelInfoPtr>::reverse_iterator(m_listRecyclePanel.erase((++rit).base()));
			}
			nAddCount -= nSize;
			for (int i = 0; i < nAddCount; i++)
			{
				m_vecPanel.push_back(PanelInfoPtr(new PanelInfo(0, 0)));
			}
		}
		else
		{
			int nAdded = 0;
			for (list<PanelInfoPtr>::reverse_iterator rit = m_listRecyclePanel.rbegin();
				rit != m_listRecyclePanel.rend() && nAdded < nAddCount;
				nAdded ++)
			{
				::ShowWindow((*rit)->hWnd,SW_SHOW);
				//::InvalidateRect((*rit)->hWnd, NULL, TRUE);
				m_vecPanel.push_back(*rit);
				rit = list<PanelInfoPtr>::reverse_iterator(m_listRecyclePanel.erase((++rit).base()));
			}
		}
	}
// 	else
// 		return true;
	
	m_nRows = nRows;
	m_nCols = nCols;
	// 调整窗格的行列编号
	int nIndex = 0;
	if (m_nFrameStyle == StyleNormal)
	{
		for (int nRow = 0; nRow < m_nRows; nRow++)
		{
			for (int nCol = 0; nCol < m_nCols; nCol++)
			{
				m_vecPanel[nIndex]->nCol = nCol;
				m_vecPanel[nIndex]->nRow = nRow;
				nIndex ++;
			}
		}
		// 重新计算每一个窗口的大小
		ResizePanel();
		// 调整每个窗口的位置
		nIndex = 0;
		for (int nRow = 0; nRow < m_nRows; nRow++)
		{
			for (int nCol = 0; nCol < m_nCols; nCol++)
			{
				PanelInfoPtr pPanel = m_vecPanel[nIndex ++];
				if (!pPanel->hWnd ||
					!IsWindow(pPanel->hWnd))
				{
					pPanel->hWnd = CreatePanel(nRow, nCol);
					::ShowWindow(pPanel->hWnd, SW_SHOW);
				}
				else
					::MoveWindow(pPanel->hWnd,pPanel->rect.left,pPanel->rect.top,RectWidth(pPanel->rect),RectHeight(pPanel->rect),FALSE);
			}
		}
	}
	else
	{
		// 调整窗格的行列编号
		m_vecPanel[nIndex]->nCol = 0;
		m_vecPanel[nIndex]->nRow = 0;
		if (m_nFrameStyle == Style_2P1)
		{
			m_vecPanel[1]->nCol = 1;
			m_vecPanel[1]->nRow = 0;

			m_vecPanel[2]->nCol = 1;
			m_vecPanel[2]->nRow = 1;

		}
		else
		{
			for (int nRow = 0; nRow < m_nRows; nRow++)
			{
				m_vecPanel[nIndex]->nRow = nRow;
				m_vecPanel[nIndex]->nCol = m_nCols - 1;
				nIndex ++;
			}
			for (int nCol = 0; nCol < m_nCols; nCol++)
			{
				m_vecPanel[nIndex]->nRow = m_nRows - 1;
				m_vecPanel[nIndex]->nCol = nCol;
				nIndex ++;
			}
		}

		// 重新计算每一个窗口的大小
		ResizePanel();
		// 先设置最大的窗口，需要重新调整尺寸
		PanelInfoPtr pPanel = m_vecPanel[0];
		if (!pPanel->hWnd ||
			!IsWindow(pPanel->hWnd))
		{
			pPanel->hWnd = CreatePanel(0, 0);
			::ShowWindow(pPanel->hWnd, SW_SHOW);
		}
		else
			::MoveWindow(pPanel->hWnd,pPanel->rect.left,pPanel->rect.top,RectWidth(pPanel->rect),RectHeight(pPanel->rect),FALSE);

		nIndex = 1;
		// 再调整右侧一列的窗口
		int nRows = m_nRows - 1;
		int nCols = m_nCols;
		if (m_nFrameStyle == Style_2P1)
		{
			nRows = m_nRows;
			nCols = 0;
		}
		{
			for (int nRow = 0; nRow < nRows; nRow++)
			{
				PanelInfoPtr pPanel = m_vecPanel[nIndex ];
				if (!pPanel->hWnd ||
					!IsWindow(pPanel->hWnd))
				{
					pPanel->hWnd = CreatePanel(nIndex);
					::ShowWindow(pPanel->hWnd, SW_SHOW);
				}
				else
					::MoveWindow(pPanel->hWnd,pPanel->rect.left,pPanel->rect.top,RectWidth(pPanel->rect),RectHeight(pPanel->rect),FALSE);
				nIndex ++;
			}

			// 调整底层一行窗口		
			for (int nCol = 0; nCol < nCols; nCol++)
			{
				PanelInfoPtr pPanel = m_vecPanel[nIndex];
				if (!pPanel->hWnd ||
					!IsWindow(pPanel->hWnd))
				{
					pPanel->hWnd = CreatePanel(nIndex );
					::ShowWindow(pPanel->hWnd, SW_SHOW);
				}
				else
					::MoveWindow(pPanel->hWnd,pPanel->rect.left,pPanel->rect.top,RectWidth(pPanel->rect),RectHeight(pPanel->rect),FALSE);
				nIndex ++;
			}
		}

	}
	
	Invalidate();
	return true;
}

int CVideoFrame::GetFrameStyle()
{
	return m_nFrameStyle;
}


bool CVideoFrame::AdjustPanels(int nCount,FrameStyle fs)
{
	if (fs != StyleNormal && fs == m_nFrameStyle)
		return true;

	CalcRowCol(nCount,m_nNewRows,m_nNewCols);
	TraceMsgA("%s Rows = %d\tCols = %d.\n", __FUNCTION__, m_nNewRows, m_nNewCols);
	return AdjustPanels(m_nNewRows, m_nNewCols,fs);
}

void CVideoFrame::DrawGrid(CDC *pDc)
{

}

// void CVideoFrame::OnPaint()
// {
// 	CPaintDC dc(this); // device context for painting
// 	
// 	DrawGrid(&dc);
// }

void CVideoFrame::OnDestroy()
{
	CWnd::OnDestroy();

}

void CVideoFrame::ResizePanel(int nWidth,int nHeight)
{
	CRect rtClient;
	m_pLastSelectRect = nullptr;
	m_pCurSelectRect = nullptr;
	if (!nWidth || !nHeight )
	{
		GetClientRect(&rtClient);
		if (!nWidth)
			nWidth = rtClient.Width();
		if (!nHeight)
			nHeight = rtClient.Height();
	}
	int nAvgColWidth = (nWidth - _GRID_LINE_WIDTH*(m_nCols + 1))/ m_nCols;
	int nAvgRowHeight = (nHeight - _GRID_LINE_WIDTH*(m_nRows + 1)) / m_nRows;

	int nRemainedWidth = nWidth - nAvgColWidth*m_nCols  ;	// 平均分配宽度有盈余
	int nRemainedHeight = nHeight - nAvgRowHeight*m_nRows;	// 平均分配高度有盈余
	

	vector<RECT>vecRect;
	RECT rtSample = {0,0,0,0};
	for (int nRow = 0;nRow < m_nRows;nRow ++)
		for (int nCol = 0;nCol < m_nCols;nCol ++)
		{
			vecRect.push_back(rtSample);
		}
	if (m_nRows == 1 && m_nCols == 1)
	{
		vecRect[0].left =  1;
		vecRect[0].right = nWidth - 1;
		vecRect[0].top =  1;
		vecRect[0].bottom = nHeight - 1;
	}
	else
	{
		// 先计算标准表情况下每个格子的尺寸
		int nStartX = _GRID_LINE_WIDTH;
		int nIndex = 0;
		for (int nCol = 0; nCol < m_nCols; nCol++)
		{
			if (nCol > 0 && nRemainedWidth > 0)
			{
				nStartX++;
				nRemainedWidth--;
			}
			// 计算每个窗格的左右边界
			for (int nRow = 0; nRow < m_nRows; nRow++)
			{
				if ( nCol > 0)
				{
					vecRect[nRow*m_nCols + nCol - 1].right = nStartX - _GRID_LINE_WIDTH;
					vecRect[nRow*m_nCols + nCol].left = nStartX + _GRID_LINE_WIDTH;
				}
				else
					vecRect[nRow*m_nCols + nCol].left = nStartX ;
			}
			nStartX += nAvgColWidth;
		}
		// 修正最右一列的右坐标
		for (int nRow = 0; nRow < m_nRows; nRow++)
			vecRect[nRow*m_nCols + m_nCols - 1].right = nWidth - _GRID_LINE_WIDTH ;

		int nStartY = _GRID_LINE_WIDTH;
		for (int nRow = 0; nRow < m_nRows; nRow++)
		{
			if (nRow > 0 && nRemainedHeight > 0)
			{
				nStartY++;
				nRemainedHeight--;
			}
			// 计算每个空格的上下边界		
			for (int nCol = 0; nCol < m_nCols; nCol++)
			{
				if (nRow > 0)
				{
					vecRect[nRow*m_nCols + nCol].top = nStartY + _GRID_LINE_WIDTH;
					vecRect[(nRow - 1)*m_nCols + nCol].bottom = nStartY - _GRID_LINE_WIDTH;
				}
				else
					vecRect[nRow*m_nCols + nCol].top = nStartY;
			}
			nStartY += nAvgRowHeight;
		}
	 	// 修正最下一行的底坐标
		for (int nCol = 0; nCol < m_nCols; nCol++)
			vecRect[(m_nRows - 1)*m_nCols + nCol].bottom = nHeight - _GRID_LINE_WIDTH;
	}
	

	if (m_nFrameStyle == StyleNormal)
	{
		for (int i = 0;i < m_vecPanel.size();i ++)
			m_vecPanel[i]->rect = vecRect[i];
	}
	else
	{
		if (m_nFrameStyle == Style_2P1)
		{
			m_vecPanel[0]->rect.left	 = vecRect[0].left;
			m_vecPanel[0]->rect.top		 = vecRect[0].top;
			m_vecPanel[0]->rect.right	 = vecRect[0].right;
			m_vecPanel[0]->rect.bottom	 = vecRect[2].bottom;

			m_vecPanel[1]->rect = vecRect[1];
			m_vecPanel[2]->rect = vecRect[3];
		}
		else
		{
			m_vecPanel[0]->rect.left	 = vecRect[0].left;
			m_vecPanel[0]->rect.top		 = vecRect[0].top;
			int nRightBottom = (m_nRows-1)*m_nCols  - 2;
			m_vecPanel[0]->rect.right	 = vecRect[nRightBottom].right;
			m_vecPanel[0]->rect.bottom	 = vecRect[nRightBottom].bottom;
			int nIndex = 1;
			for (int nRow = 0;nRow < m_nRows - 1;nRow ++)
				m_vecPanel[nIndex ++]->rect = vecRect[(nRow + 1)*m_nCols - 1];

			for (int nCol = 0;nCol < m_nCols;nCol ++)
				m_vecPanel[nIndex ++]->rect = vecRect[(m_nRows-1)*m_nCols + nCol];
		}
	}
	int nIndex = 0;
	for (auto it = m_vecPanel.begin(); it != m_vecPanel.end(); it++)
	{
		TraceMsgA("Rect[%d] = (%d,%d),(%d,%d).\n",nIndex++, (*it)->rect.left, (*it)->rect.top, (*it)->rect.right, (*it)->rect.bottom);
	}
}

void CVideoFrame::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	ResizePanel(cx, cy);
	for (vector<PanelInfoPtr>::iterator it = m_vecPanel.begin(); it != m_vecPanel.end(); it++)
		(*it)->UpdateWindow();
	RefreshSelectedPanel();
}

void CVideoFrame::SetCurSelIndex(int nIndex)
{
	m_nCurPanel = nIndex;
}   

void CVideoFrame::RefreshSelectedPanel()
{
	if (m_nCurPanel == -1)
		return ;
	CClientDC  dc(this);
	RECT rtTemp;
	CPen* pOldPen = nullptr;
	CBrush *pBrush= CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH));
	CBrush *pOldBrush=dc.SelectObject(pBrush);

	if (m_pLastSelectRect)
	{
		::CopyRect(&rtTemp,m_pLastSelectRect);
		rtTemp.left -=1;
		rtTemp.top -= 1;
		rtTemp.right += 1;
		rtTemp.bottom += 1;
		pOldPen = (CPen *)dc.SelectObject(m_pRestorePen);	

		dc.Rectangle(&rtTemp);
	}
	if (m_nCurPanel < m_vecPanel.size())
		m_pCurSelectRect = &m_vecPanel[m_nCurPanel]->rect;
	if (m_pCurSelectRect)
	{
		::CopyRect(&rtTemp,m_pCurSelectRect);
		rtTemp.left -=1;
		rtTemp.top -= 1;
		rtTemp.right += 1;
		rtTemp.bottom += 1;
		if (!pOldPen)
			pOldPen = (CPen*)dc.SelectObject(m_pSelectedPen);
		else
			dc.SelectObject(m_pSelectedPen);
		dc.Rectangle(&rtTemp);
	}
	dc.SelectObject(pOldBrush);
	dc.SelectObject(pOldPen);
}

void CVideoFrame::SetScreenWnd(int nScreenID)
{
	m_nScreenID = nScreenID;
}

HBRUSH CVideoFrame::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
 	HBRUSH hbr =  CreateSolidBrush(BackColor); //创建背景刷;  
 	pDC->SetBkMode(TRANSPARENT);  
	return hbr;  
}

BOOL CVideoFrame::OnEraseBkgnd(CDC* pDC)
{
	CBrush back(BackColor);  
	CBrush* pold=pDC->SelectObject(&back);  
	CRect rect;  
	pDC->GetClipBox (&rect);  
	pDC->PatBlt (rect.left,rect.top,rect.Width(),rect.Height(),PATCOPY);  
	pDC->SelectObject(pold);  
	RefreshSelectedPanel();
	return TRUE;   
}


BOOL CVideoFrame::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
// 	if (m_bRawFlag)
// 	{
// 		m_cursor = ::LoadCursor(NULL, IDC_ARROW);
// 		::SetCursor(m_cursor);
// 	}
// 	else
// 	{
// 
// 		m_cursor = ::LoadCursor(NULL, IDC_HAND);
// 		::SetCursor(m_cursor);
// 	}
	return TRUE;
}


BOOL CVideoFrame::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
		case WM_LBUTTONDOWN:                  //0x0201
		case WM_LBUTTONUP:                    //0x0202
		case WM_LBUTTONDBLCLK:                //0x0203，双击事件
		case WM_RBUTTONDOWN:                  //0x0204
		case WM_RBUTTONUP:                    //0x0205
		case WM_RBUTTONDBLCLK:                //0x0206
		case WM_MBUTTONDOWN:                  //0x0207
		case WM_MBUTTONUP:                    //0x0208
		case WM_MBUTTONDBLCLK:                //0x0209
		{
			POINT pt;
			pt.x = GET_X_LPARAM(pMsg->lParam);
			pt.y = GET_Y_LPARAM(pMsg->lParam);
			POINT ptClient = pMsg->pt;
			ScreenToClient(&ptClient);
			//TraceMsgA("%s Mouse Message Panel_point(%d,%d),Screen_point(%d,%d),Frame_point(%d,%d)\n", __FUNCTION__, pt.x, pt.y, pMsg->pt.x, pMsg->pt.y,ptClient.x,ptClient.y);
			auto it = find_if(m_vecPanel.begin(), m_vecPanel.end(), PanelFinder(ptClient));
			if (it != m_vecPanel.end())
			{
				m_nCurPanel = it - m_vecPanel.begin();
				if (!m_pCurSelectRect)
				{
					m_pLastSelectRect = nullptr;
				}
				else if (m_pCurSelectRect != &(*it)->rect)	//从另一窗格切换到当前窗格
				{
					m_pLastSelectRect = m_pCurSelectRect;
				}
				m_pCurSelectRect = &(*it)->rect;

				RefreshSelectedPanel();
				m_pCurrentFrame = this;
				HWND hParentWnd = GetParent()->GetSafeHwnd();
				if (hParentWnd)
					::SendMessage(hParentWnd, m_nMouseMessageArray[pMsg->message - WM_MOUSEMOVE],(WPARAM)m_nCurPanel, pMsg->lParam);
			}
			
		}
			break;
		
		case WM_MOUSEMOVE:
		{
			HWND hParentWnd = GetParent()->GetSafeHwnd();
			if (hParentWnd)
				::SendMessage(hParentWnd, WM_FRAME_MOUSEMOVE, pMsg->wParam, pMsg->lParam);
			break;
		}
		case WM_MOUSEHWHEEL:
		{
			HWND hParentWnd = GetParent()->GetSafeHwnd();
			if (hParentWnd)
				::SendMessage(hParentWnd, WM_FRAME_MOUSEWHEEL, pMsg->wParam, pMsg->lParam);
			break;
		}
		default:
			break;
	}

	return CWnd::PreTranslateMessage(pMsg);
}
