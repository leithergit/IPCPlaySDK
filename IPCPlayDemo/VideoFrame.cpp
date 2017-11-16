// VideoFrame.cpp : implementation file
//

#include "stdafx.h"
#include "VideoFrame.h"


// CVideoFrame

IMPLEMENT_DYNAMIC(CVideoFrame, CWnd)

map<HWND, HWND>CVideoFrame::m_PanelMap;
CriticalSectionPtr CVideoFrame::m_csPannelMap = make_shared<CriticalSectionWrap>();

CVideoFrame::CVideoFrame()
{
	m_nCols = 1;
	m_nRows = 1;
	m_nCurSelected = -1;
	m_nPannelUsed = 0;		//  已用空格数量
	m_pSelectedPen = NULL;
	m_pUnSelectedPen = NULL;
	m_pRestorePen = NULL;
	m_pCurSelectRect = nullptr;
	m_pLastSelectRect =  nullptr;
}

CVideoFrame::~CVideoFrame()
{
}


BEGIN_MESSAGE_MAP(CVideoFrame, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_MESSAGE(WM_TROGGLEFULLSCREEN,OnTroggleFullScreen)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

// CVideoFrame message handlers

BOOL CVideoFrame::Create(UINT nID, const RECT& rect,int nRow,int nCol, CWnd* pParentWnd, CCreateContext* pContext)
{
	// can't use for desktop or pop-up windows (use CreateEx instead)
	ASSERT(pParentWnd != NULL);
	ASSERT(nID != 0);
	ASSERT(nRow != 0 && nCol != 0);
	m_nRows = nRow;
	m_nCols = nCol;

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


int CVideoFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	m_pSelectedPen = new CPen(PS_SOLID, 2, RGB(255, 0, 0));
	m_pUnSelectedPen = new CPen(PS_SOLID, _GRID_LINE_WIDTH, RGB(0, 255, 0));
	m_pRestorePen = new CPen(PS_SOLID, 2, RGB(0, 0, 0));
	
	AdjustPanels(m_nRows, m_nCols);

	return 0;
}

// 调整窗格数量
// 如果是新增窗格，则不需要删除原有的窗格
// 如果是缩减窗格，则仅删除多余的窗格
bool CVideoFrame::AdjustPanels(int nRow, int nCols)
{
	ASSERT(nRow != 0 && nCols != 0);
	if (!nRow || !nCols)
		return false;
	//缩减窗格
	if (nRow*nCols < m_vecPanel.size())
	{
		int nReduceCount = m_vecPanel.size() - nRow*nCols;
		m_csPannelMap->Lock();
		// 从后往前删除
		for (vector<PanelInfoPtr>::iterator it = m_vecPanel.begin() + nRow*nCols;it != m_vecPanel.end();)
		{
			map<HWND, HWND>::iterator itFind = m_PanelMap.find((*it)->hWnd);
			if (itFind != m_PanelMap.end())
				m_PanelMap.erase(itFind);
			it = m_vecPanel.erase(it);
		}
		m_csPannelMap->Unlock();
		// 调整窗格尺寸
	}
	else if (nRow*nCols > m_vecPanel.size())	// 新增窗格
	{
		int nAddCount = nRow*nCols - m_vecPanel.size();
		for (int i = 0; i < nAddCount; i++)
		{
			m_vecPanel.push_back(PanelInfoPtr(new PanelInfo(0, 0)));
		}
	}
	else
		return true;
	
	m_nRows = nRow;
	m_nCols = nCols;
	// 调整窗格的行列编号
	int nIndex = 0;
	for (int nRow = 0; nRow < m_nRows; nRow++)
	{
		for (int nCol = 0; nCol < m_nCols; nCol++)
		{
			m_vecPanel[nIndex]->nCol = nCol;
			m_vecPanel[nIndex]->nRow = nRow;
		}
	}
	// 重新计算每一个窗口的大小
	ResizePanel();
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
	Invalidate();
	return true;
}

bool CVideoFrame::AdjustPanels(int nCount)
{
	ASSERT(nCount != 0);
	if (nCount == 0)
		return false;

	float fsqroot = sqrt((float)nCount);
	int nRowCount = floor(fsqroot);
	int nColCount = nRowCount;

	if (nRowCount*nColCount < nCount)
	{
		nColCount++;
		if (nRowCount*nColCount < nCount)
			nRowCount++;
	}

	// 必须保证列数大于行数
	if (nRowCount > nColCount)
	{
		int nTemp = nRowCount;
		nRowCount = nColCount;
		nColCount = nTemp;
	}
	m_nRows = nRowCount;
	m_nCols = nColCount;
	_TraceMsgA("%s Rows = %d\tCols = %d.\n", __FUNCTION__, nRowCount, nColCount);
	return AdjustPanels(m_nRows, m_nCols);
}
void CVideoFrame::DrawGrid(CDC *pDc)
{
	CRect rtClient;
	CPen* pOldPen = (CPen *)pDc->SelectObject(m_pUnSelectedPen);
	GetClientRect(&rtClient);
	int nWidth = rtClient.Width();
	int nHeight = rtClient.Height();
	int nAvgColWidth = nWidth / m_nCols;
	int nAvgRowHeight = nHeight / m_nRows;

	int nRemainedWidth = nWidth - nAvgColWidth*m_nCols;		// 平均分配宽度有盈余
	int nRemainedHeight = nHeight - nAvgRowHeight*m_nRows;	// 平均分配高度有盈余

	int nStartX = rtClient.left;
	int nStartY = rtClient.top;

	// 画竖线
	for (int nCol = 0; nCol < m_nCols; nCol++)
	{
		if (nCol > 0 && nRemainedWidth > 0)
		{
			nStartX++;
			nRemainedWidth--;
		}
		if (nCol > 0)
		{
			pDc->MoveTo(nStartX, nStartY);
			pDc->LineTo(nStartX, rtClient.bottom);
		}
		
		nStartX += nAvgColWidth;
	}
	
	// 画横线
	for (int nRow = 0; nRow < m_nRows; nRow++)
	{
		if (nRow > 0 && nRemainedHeight > 0)
		{
			nStartY++;
			nRemainedHeight--;
		}
		if (nRow > 0)
		{
			pDc->MoveTo(rtClient.left, nStartY);
			pDc->LineTo(rtClient.right, nStartY);
		}	
		nStartY += nAvgRowHeight;
	}
	pDc->SelectObject(pOldPen);
	
//#ifdef _DEBUG
//	_TraceMsgA("Index\tLeft\tRight\tTop\t\tBottom.\n");
//	for (int nRow = 0; nRow < m_nRows; nRow++)	
//	{
//		for (int nCol = 0; nCol < m_nCols; nCol++)
//		{
//			int nIndex = nRow*m_nCols + nCol;
//			RECT rtWnd = m_vecPanel[nIndex]->rect;
//			_TraceMsgA("(%d,%d)\t%d\t\t%d\t\t%d\t\t%d\n",nRow,nCol, pRtWnd->left, pRtWnd->right, pRtWnd->top, pRtWnd->bottom);
//		}
//	}
//#endif
}

void CVideoFrame::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
	DrawGrid(&dc);
}

void CVideoFrame::OnDestroy()
{
	CWnd::OnDestroy();
	if (m_pSelectedPen)
		delete m_pSelectedPen;
	if (m_pUnSelectedPen)
		delete m_pUnSelectedPen;
	if (m_pRestorePen)
		delete m_pRestorePen;
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
	int nAvgColWidth = nWidth / m_nCols;
	int nAvgRowHeight = nHeight / m_nRows;

	int nRemainedWidth = nWidth - nAvgColWidth*m_nCols;		// 平均分配宽度有盈余
	int nRemainedHeight = nHeight - nAvgRowHeight*m_nRows;	// 平均分配高度有盈余

	int nStartX = 0;
	
	// 画竖线
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
			m_vecPanel[nRow*m_nCols + nCol]->rect.left = nStartX + _GRID_LINE_WIDTH;
			if (nCol > 0)
				m_vecPanel[nRow*m_nCols + nCol - 1]->rect.right = nStartX - _GRID_LINE_WIDTH;
		}
		nStartX += nAvgColWidth;
	}
	// 修正最右一列的右坐标
	for (int nRow = 0; nRow < m_nRows; nRow++)
		m_vecPanel[nRow*m_nCols + m_nCols - 1]->rect.right = nWidth /*- _GRID_LINE_WIDTH + 1*/;
	
	int nStartY = 0;
	// 画横线
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
			m_vecPanel[nRow*m_nCols + nCol]->rect.top = nStartY + _GRID_LINE_WIDTH;
			if (nRow > 0)
				m_vecPanel[(nRow - 1)*m_nCols + nCol]->rect.bottom = nStartY - _GRID_LINE_WIDTH;
		}
		
		nStartY += nAvgRowHeight;
	}
	// 修正最下一条的底坐标
	for (int nCol = 0; nCol < m_nCols; nCol++)
		m_vecPanel[(m_nRows - 1)*m_nCols + nCol]->rect.bottom = nHeight/* - _GRID_LINE_WIDTH*/;
	//所有矩形内缩一个象素
	for (vector<PanelInfoPtr>::iterator it = m_vecPanel.begin();it != m_vecPanel.end();it ++)
	{
		LPRECT pRect = &(*it)->rect;
		pRect->left += 1;
		pRect->top += 1;
		pRect->right -= 1;
		pRect->bottom -= 1;
	}

//#ifdef _DEBUG
//	_TraceMsgA("Index\tLeft\tRight\tTop\t\tBottom.\n");
//	for (int nRow = 0; nRow < m_nRows; nRow++)	
//	{
//		for (int nCol = 0; nCol < m_nCols; nCol++)
//		{
//			int nIndex = nRow*m_nCols + nCol;
//			RECT rtWnd = m_vecPanel[nIndex]->rect;
//			_TraceMsgA("(%d,%d)\t%d\t\t%d\t\t%d\t\t%d\n",nRow,nCol, pRtWnd->left, pRtWnd->right, pRtWnd->top, pRtWnd->bottom);
//		}
//	}
//#endif
}

void CVideoFrame::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	ResizePanel(cx, cy);
	for (vector<PanelInfoPtr>::iterator it = m_vecPanel.begin(); it != m_vecPanel.end(); it++)
		(*it)->UpdateWindow();
}

LRESULT CVideoFrame::OnTroggleFullScreen(WPARAM W, LPARAM L)
{
	HWND hRoot = ::GetAncestor(m_hWnd, GA_ROOT);
	if (hRoot)
	{
		::PostMessage(hRoot, WM_TROGGLEFULLSCREEN, W, L);
	}
	return 0;
}
void CVideoFrame::OnLButtonDown(UINT nFlags, CPoint point)
{
	_TraceMsgA("%s point(%d,%d).\n",__FUNCTION__,point.x,point.y);
	for (vector<PanelInfoPtr>::iterator it = m_vecPanel.begin();it != m_vecPanel.end();it ++)
	{
		if (::PtInRect(&(*it)->rect,point))
		{
			if (!m_pCurSelectRect)
			{
				m_pLastSelectRect = nullptr;
			}
			else if (m_pCurSelectRect != &(*it)->rect)//从另一窗格切换到当前窗格
			{
				m_pLastSelectRect = m_pCurSelectRect;
			}
			m_pCurSelectRect = &(*it)->rect;
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
	}
	CWnd::OnLButtonDown(nFlags, point);
}
