#pragma once

#include <assert.h>
#include <vector>
#include <map>
#include <list>
//#include <memory>
#ifdef _STD_SMARTPTR				// 使用STL的智能指针
#include <memory>
using namespace std;
using namespace std::tr1;
#else
#include <boost/shared_ptr.hpp>
using namespace boost;
#endif

#include <math.h>
#include "AutoLock.h"
#include "Utility.h"
#include "ipcplaysdk.h"
#include "CriticalSectionProxy.h"

using namespace  std;

#pragma warning(disable:4244 4018)
enum FrameStyle
{
	StyleNormal = 0,
	Style_2P1,
	Style_5P1,
	Style_7P1,
	Style_9P1,
	Style_11P1,
	Style_13P1,
	Style_15P1,
};
#define		_GRID_LINE_WIDTH	1
// CVideoFrame


// #define WM_SELWINDOW_LBUTTON        WM_USER+4300   //单击左键选中窗口消息
// #define WM_SELWINDOW_LBUTTONUP      WM_USER+4301   //单击左键鼠标松开消息
// #define WM_MIN_SHOW                 WM_USER+4302   //屏幕最小化显示消息 
// #define WM_MAX_SHOW                 WM_USER+4303   //屏幕最小化显示消息
// #define WM_NORMAL_SHOW              WM_USER+4304   //恢复正常显示屏幕消息
// #define WM_FULLSCREEN_SHOW          WM_USER+4305   //双击左键全屏显示消息
// #define WM_SHOW_MAINWND             WM_USER+4307   //显示主窗口位置消息
#define WM_SWAP_SCREEN              WM_USER+4308   //窗口交换处理消息

// 通知父窗口右键消息,WPARAM 为鼠标所在坐标,x = LOWROD(wParam),y = HIWORD(wParam),LPARAM为窗格索引
#define WM_FRAME_MOUSEMOVE                     (WM_USER + 4096)
#define WM_FRAME_LBUTTONDOWN                   (WM_FRAME_MOUSEMOVE + 1)
#define WM_FRAME_LBUTTONUP                     (WM_FRAME_MOUSEMOVE + 2)
#define WM_FRAME_LBUTTONDBLCLK                 (WM_FRAME_MOUSEMOVE + 3)
#define WM_FRAME_RBUTTONDOWN                   (WM_FRAME_MOUSEMOVE + 4)
#define WM_FRAME_RBUTTONUP                     (WM_FRAME_MOUSEMOVE + 5)
#define WM_FRAME_RBUTTONDBLCLK                 (WM_FRAME_MOUSEMOVE + 6)
#define WM_FRAME_MBUTTONDOWN                   (WM_FRAME_MOUSEMOVE + 7)
#define WM_FRAME_MBUTTONUP                     (WM_FRAME_MOUSEMOVE + 8)
#define WM_FRAME_MBUTTONDBLCLK                 (WM_FRAME_MOUSEMOVE + 9)
#define WM_FRAME_MOUSEWHEEL                    (WM_FRAME_MOUSEMOVE + 10)

struct PanelInfo
{
	PanelInfo()
	{
		ZeroMemory(this, sizeof(PanelInfo));
	}
	PanelInfo(int nRowIn, int nColIn)
	{
		ZeroMemory(this, sizeof(PanelInfo));
		nRow = nRowIn;
		nCol = nColIn;
	}
	
	~PanelInfo()
	{
		DestroyWindow(hWnd);
	}
	void UpdateWindow()
	{
		MoveWindow(hWnd, rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top), TRUE);	
	}
	HWND hWnd;
	RECT rect;
	int nIndex;
	int nRow;
	int nCol;
	void *pCustumData;
};
typedef shared_ptr<PanelInfo> PanelInfoPtr;

struct PanelFinder
{
	POINT pt;
	PanelFinder(POINT ptInput)
		:pt(ptInput)
	{}
	bool operator()(PanelInfoPtr pPanel)
	{
		return ::PtInRect(&pPanel->rect, pt);
	}
};

class CVideoFrame : public CWnd
{
	DECLARE_DYNAMIC(CVideoFrame)
public:
	CVideoFrame();
	virtual ~CVideoFrame();
	static map<HWND, HWND> m_PanelMap;
	static CCriticalSectionProxyPtr m_csPannelMap;
	
protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
private:
	static CVideoFrame *m_pCurrentFrame;
	UINT	m_nCols /*= 1*/, m_nRows/* = 1*/;
	UINT	m_nNewCols,m_nNewRows;
	vector	<PanelInfoPtr>m_vecPanel;
	CCriticalSectionProxyPtr m_csvecPanel;
	list	<PanelInfoPtr>m_listRecyclePanel;
	CCriticalSectionProxyPtr m_cslistRecyclePanel;
	int		m_nPannelUsed/* = 0*/;		//  已用空格数量
	LPRECT  m_pCurSelectRect;
	LPRECT  m_pLastSelectRect;
	int		m_nCurPanel;		// 当前被选中的面板序号
	FrameStyle	m_nFrameStyle;

private:
	int m_nScreenID;                   //屏幕号

	HCURSOR  m_hCursorHand;
	HCURSOR	 m_hCursorArrow;
	static   long m_nMouseMessageArray[];

public:
	CPoint m_point;

	CPoint m_ptBegin;    //起始点
	CPoint m_ptOrigon;	//前一个点

	IPC_PLAYHANDLE m_handle;
private:
	void ResizePanel(int nWidth = 0, int nHeight = 0);

	// 绘制窗口
	void DrawGrid(CDC *pdc);

	// 创建创建面板窗口
	HWND CreatePanel(int nRow, int nCol)
	{
		WNDCLASSEX wcex;
		TCHAR *szWindowClass = _T("PanelWnd");
		TCHAR szWndName[256] = { 0 };
		_stprintf_s(szWndName, 256, _T("Panel(%d,%d)"), nRow, nCol);
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wcex.lpfnWndProc = PanelWndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = NULL;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = szWindowClass;
		wcex.hIconSm = NULL;
		RegisterClassEx(&wcex);
		RECT* pRtWnd = GetPanelRect(nRow, nCol);
		//TraceMsgA("Rect(%d,%d) = (%d,%d,%d,%d).\n", nRow, nCol, pRtWnd->left, pRtWnd->right, pRtWnd->top, pRtWnd->bottom);
		HWND hPannel =  ::CreateWindow(szWindowClass,	// 窗口类
									szWndName,							// 窗口标题 
									WS_CHILD,							// 窗口风格
									pRtWnd->left, 						// 窗口左上角X坐标
									pRtWnd->top, 						// 窗口左上解Y坐标
									(pRtWnd->right - pRtWnd->left), 	// 窗口宽度
									(pRtWnd->bottom - pRtWnd->top), 	// 窗口高度
									m_hWnd, 							// 父窗口句柄
									NULL,								// 菜单句柄
									NULL,
									this);
		if (hPannel)
		{
			DWORD dwExStype = ((DWORD)GetWindowLong(hPannel, GWL_EXSTYLE));
			dwExStype |=WS_EX_TRANSPARENT;
			SetWindowLong(hPannel, GWL_EXSTYLE,dwExStype);
			ModifyWndStyle(hPannel, GWL_STYLE, 0, WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 0);
			m_csPannelMap->Lock();
			map<HWND, HWND>::iterator it = m_PanelMap.find(hPannel);
			if (it != m_PanelMap.end())
				m_PanelMap.erase(it);
			m_PanelMap.insert(pair<HWND, HWND>(hPannel, m_hWnd));
			m_csPannelMap->Unlock();
		}
		return hPannel;
	}
	HWND CreatePanel(int nIndex)
	{
		WNDCLASSEX wcex;
		TCHAR *szWindowClass = _T("PanelWnd");
		TCHAR szWndName[256] = { 0 };
		_stprintf_s(szWndName, 256, _T("Panel(%d)"), nIndex);
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wcex.lpfnWndProc = PanelWndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = NULL;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = szWindowClass;
		wcex.hIconSm = NULL;
		RegisterClassEx(&wcex);
		RECT* pRtWnd = GetPanelRect(nIndex);
		TraceMsgA("Rect[%d] = (%d,%d,%d,%d).\n", nIndex, pRtWnd->left, pRtWnd->right, pRtWnd->top, pRtWnd->bottom);
		HWND hPannel =  ::CreateWindow(szWindowClass,	// 窗口类
			szWndName,							// 窗口标题 
			WS_CHILD,							// 窗口风格
			pRtWnd->left, 						// 窗口左上角X坐标
			pRtWnd->top, 						// 窗口左上解Y坐标
			(pRtWnd->right - pRtWnd->left), 	// 窗口宽度
			(pRtWnd->bottom - pRtWnd->top), 	// 窗口高度
			m_hWnd, 							// 父窗口句柄
			NULL,								// 菜单句柄
			NULL,
			this);
		if (hPannel)
		{
			DWORD dwExStype = ((DWORD)GetWindowLong(hPannel, GWL_EXSTYLE));
			dwExStype |=WS_EX_TRANSPARENT;
			SetWindowLong(hPannel, GWL_EXSTYLE,dwExStype);
			ModifyWndStyle(hPannel, GWL_STYLE, 0, WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 0);
			m_csPannelMap->Lock();
			map<HWND, HWND>::iterator it = m_PanelMap.find(hPannel);
			if (it != m_PanelMap.end())
				m_PanelMap.erase(it);
			m_PanelMap.insert(pair<HWND, HWND>(hPannel, m_hWnd));
			m_csPannelMap->Unlock();
		}
		return hPannel;
	}
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//afx_msg void OnPaint();
	CPen *m_pSelectedPen/* = NULL*/;
	CPen *m_pUnSelectedPen/* = NULL*/;
	CPen *m_pRestorePen/* = NULL*/;


public:

	void SetCurSelIndex(int nIndex);     //设置选中的窗口
	void SetScreenWnd(int nScreenID);
	
	int GetFrameStyle();

	afx_msg void OnDestroy();
	virtual BOOL Create(UINT nID, const RECT& rect,int nRow,int nCol,CWnd* pParentWnd,  CCreateContext* pContext = NULL);
	virtual BOOL Create(UINT nID, const RECT& rect,int nCount,CWnd* pParentWnd,  CCreateContext* pContext = NULL);
	// 此处的nStyle函数不能取NormalStyle,否则创建无法成功
	virtual BOOL Create(UINT nID, const RECT& rect,FrameStyle nStyle,CWnd* pParentWnd,  CCreateContext* pContext = NULL );

	void CalcRowCol(int nCount,UINT &nRows,UINT &nCols)
	{
		assert(nCount != 0);
		if (nCount == 0)
			return ;

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
		nRows = nRowCount;
		nCols = nColCount;
	}
	void RefreshSelectedPanel();
	LPRECT GetPanelRect(int nRow, int nCol)
	{
		if ((nRow*m_nCols + nCol) < m_vecPanel.size())		
			return &m_vecPanel[nRow*m_nCols + nCol]->rect;
		else
			return NULL;
	}

	const LPRECT GetPanelRect(int nIndex)
	{
		if (nIndex < m_vecPanel.size())
			return &m_vecPanel[nIndex]->rect;
		else
			return NULL;
	}

	HWND GetPanelWnd(int nRow, int nCol)
	{
		if ((nRow*m_nCols + nCol) < m_vecPanel.size())
			return m_vecPanel[nRow*m_nCols + nCol]->hWnd;
		else
			return NULL;
	}

	HWND GetPanelWnd(int nIndex)
	{
		if (nIndex < m_vecPanel.size())
		{
			return m_vecPanel[nIndex]->hWnd;
		}
		else
			return NULL;
	}

	inline bool SetPanelParam(int nIndex, void *pParam)
	{
		if (nIndex < m_vecPanel.size())
		{
			m_vecPanel[nIndex]->pCustumData = pParam;
			return true;
		}
		else
			return false;
	}

	bool SetPanelParam(int nRow, int nCol, void *pParam)
	{
		return SetPanelParam(nRow*m_nCols + nCol, pParam);
	}
	
	inline void *GetPanelParam(int nIndex)
	{
		if (m_vecPanel.size() > 0 && nIndex < m_vecPanel.size())
			return m_vecPanel[nIndex]->pCustumData;
		else
			return NULL;
	}
	void *GetPanelParam(int nRow, int nCol)
	{
		return GetPanelParam(nRow*m_nCols + nCol);
	}
	
	void SetCurPanel(int nIndex)
	{
		if (nIndex < m_vecPanel.size())
		{
			m_pLastSelectRect = m_pCurSelectRect;
			m_pCurSelectRect = &m_vecPanel[nIndex]->rect;
			RefreshSelectedPanel();
		}
	}

	int GetCurPanel()
	{
		if (m_vecPanel.size() > 0 && m_nCurPanel < m_vecPanel.size())
			return m_nCurPanel;
		else
			return -1;
	}
	int GetPanelCount()
	{
		return m_vecPanel.size();
	}
	int GetRows(){ return m_nRows; }
	int GetCols(){ return m_nCols; }
	bool AdjustPanels(int nRow, int nCols,FrameStyle fs = StyleNormal);
	bool AdjustPanels(int nCount,FrameStyle fs = StyleNormal);
	static CVideoFrame *GetCurrentFrame()
	{
		return m_pCurrentFrame;
	};


	static LRESULT CALLBACK PanelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_KEYDOWN:
		{
			switch (wParam)	// 忽略ESC和回车键
			{
			case VK_ESCAPE:
			case VK_RETURN:
			{
				//printf(_T("Capture Escape or Return Key.\n"));
				return 0;
			}
			break;
			}
		}

		break;
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
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			TraceMsgA("%s A Mouse Message:%04X from hWnd(%08x) point(%d,%d).\n", __FUNCTION__,message,hWnd,pt.x,pt.y);
// 			m_csPannelMap->Lock();
// 			// 查找窗口对应的父窗口
// 			map<HWND, HWND>::iterator it = m_PanelMap.find(hWnd);
// 			if (it != m_PanelMap.end() &&
// 				IsWindow(it->second))
// 				::PostMessage(it->second, WM_FRAME_LBUTTONDBLCLK, (WPARAM)hWnd, lParam);
// 			m_csPannelMap->Unlock();
//			return ::DefWindowProc(hWnd, message, wParam, lParam);		// 必须要有这一句，不然窗口可能无法创建成功
		}
		case WM_MOUSEMOVE :                   //0x0200
		case WM_MOUSEWHEEL:                   //0x020A
		case WM_DESTROY:
		default:
			return ::DefWindowProc(hWnd, message, wParam, lParam);		// 必须要有这一句，不然窗口可能无法创建成功
		}
		return 0l;
	}
	
	afx_msg void OnSize(UINT nType, int cx, int cy);
	// 新增函数 int GetPanel(POINT pt),以获取所在鼠标位置在所窗格号
	// CVideoFrame类不再处理鼠标消息，转而由其父窗口处理,父窗口收到鼠标消息后，应使用GetPanel函数判断鼠标坐标是否在Frame窗口内
	int GetPanel(POINT pt)
	{
		return 0;
	}

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg LRESULT OnMouseWheel(WPARAM W, LPARAM L)
	{
		HWND hParentWnd = GetParent()->GetSafeHwnd();
		if (hParentWnd)
			return ::SendMessage(hParentWnd, WM_FRAME_MOUSEWHEEL, W, L);
		else
			return 0;
	}
};


