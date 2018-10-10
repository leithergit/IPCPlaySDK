// TransparentWnd.cpp : implementation file
//

#include "stdafx.h"
#include "IPCPlayDemo.h"
#include "TransparentWnd.h"


// CTransparentWnd

IMPLEMENT_DYNAMIC(CTransparentWnd, CWnd)

CTransparentWnd::CTransparentWnd()
{

}

CTransparentWnd::~CTransparentWnd()
{
}


BEGIN_MESSAGE_MAP(CTransparentWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
END_MESSAGE_MAP()



// CTransparentWnd message handlers




int CTransparentWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE, GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_TRANSPARENT | WS_EX_LAYERED);
	//LWA_COLORKEY(RGB有效，可绘图，鼠标无响应) LWA_ALPHA（鼠标有响应，半透明时动态绘图可见）
	DWORD nColorWindow = GetSysColor(COLOR_WINDOW);
	DWORD nColorBtn = GetSysColor(COLOR_BTNFACE);
	SetLayeredWindowAttributes(nColorBtn, 0, LWA_COLORKEY);

	//::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOOWNERZORDER);

	return 0;
}


void CTransparentWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(rect);
	//dc.FillSolidRect(rect, RGB(255, 0, 255));
	int nWidth = rect.Width();
	int nHeight = rect.Height();
	int nGridWidth = nWidth / 20;
	int nGridHeight = nHeight / 20;
	int nStartX = 0, nStartY = 0;
	int nEndX = 0, nEndY = 0;
	CPen pen(PS_SOLID, 1, RGB(255, 0, 0));
	CPen *pOldPen = dc.SelectObject(&pen);
	for (int i = 0; i < 20; i++)
	{
		nStartX = i*nGridWidth;
		nStartY = rect.top;

		nEndX = nStartX;
		nEndY = rect.bottom;
		dc.MoveTo(nStartX, nStartY);
		dc.LineTo(nEndX, nEndY);

		nStartX = rect.left;
		nStartY = i*nGridHeight;
		nEndX = rect.right;
		nEndY = nStartY;

		dc.MoveTo(nStartX, nStartY);
		dc.LineTo(nEndX, nEndY);
	}
}
