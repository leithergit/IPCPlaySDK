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
#include <afx.h>
#include <afxwin.h>
#include <math.h>
#include "AutoLock.h"
#include "Utility.h"
#include "CriticalSectionAgent.h"


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
	Style_ZoomOut = 0xFF
};
#define		_GRID_LINE_WIDTH	1
// CVideoFrame
#define RectWidth(rt)	(rt.right - rt.left)
#define RectHeight(rt)	(rt.bottom - rt.top)

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
	/*	int nCount = 0;
		cs.Lock();
		nPanelCount++;
		nCount = nPanelCount;
		cs.Unlock();
		TraceMsgW(L"%s nPanelCount = %d.\n", __FUNCTIONW__, nCount);*/
	}
	PanelInfo(int nRowIn, int nColIn)
	{
		ZeroMemory(this, sizeof(PanelInfo));
		nRow = nRowIn;
		nCol = nColIn;
		/*
		int nCount = 0;
		cs.Lock();
		nPanelCount++;
		nCount = nPanelCount;
		cs.Unlock();
		TraceMsgW(L"%s nPanelCount = %d.\n", __FUNCTIONW__, nCount);*/
	}
	
	~PanelInfo()
	{
	/*	int nCount = 0;
		cs.Lock();
		nPanelCount++;
		nCount = nPanelCount;
		cs.Unlock();
		TraceMsgW(L"%s nPanelCount = %d.\n", __FUNCTIONW__, nCount);*/

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
	void *pCustumData;/*
#ifdef _DEBUG
	static int nPanelCount;
	static CCriticalSectionProxy cs;
	
#endif // _DEBUG
*/
};
typedef shared_ptr<PanelInfo> PanelInfoPtr;

struct PanelFinder
{
	POINT pt;
	PanelFinder(POINT ptInput)
		:pt(ptInput)
	{}
	BOOL operator()(PanelInfoPtr pPanel)
	{
		return ::PtInRect(&pPanel->rect, pt);
	}
};

extern HWND g_hSwapWnd;      //当前需要交换窗口
extern HWND g_hOldPlaywnd;   //历史选中窗口

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
	FrameStyle  m_nStyleSaved;	// Zoom前的分割风格
	int		m_nPanelCountSaved;	// Zoom前的分割数量
	int		m_nCurPanelSaved;	// Zoom前的所选窗格

private:
	int m_nScreenID;                   //屏幕号

	HCURSOR  m_hCursorHand;
	HCURSOR	 m_hCursorArrow;
	static   UINT m_nMouseMessageArray[];

public:
	CPoint m_point;

	CPoint m_ptBegin;    //起始点
	CPoint m_ptOrigon;	//前一个点
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
		TraceMsg(_T("%s nIndex = %d.\n"), __FUNCTIONW__, nIndex);
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
		TraceMsg(_T("%s nRow = %d\tnCol = %d.\n"), __FUNCTIONW__, nRow, nCol);
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
			break;	
		}
		case WM_LBUTTONDOWN:                  //0x0201
		case WM_LBUTTONUP:                    //0x0202
		case WM_LBUTTONDBLCLK:                //0x0203，双击事件
		case WM_RBUTTONDOWN:                  //0x0204:
		case WM_RBUTTONUP:                    //0x0205
		case WM_RBUTTONDBLCLK:                //0x0206
		case WM_MBUTTONDOWN:                  //0x0207
		case WM_MBUTTONUP:                    //0x0208
		case WM_MBUTTONDBLCLK:                //0x0209
		{
			static CHAR* szMouseMessage[] = {
				"WM_MOUSEMOVE",                  //  0x0200
				"WM_LBUTTONDOWN",                //  0x0201
				"WM_LBUTTONUP",                  //  0x0202
				"WM_LBUTTONDBLCLK",              //  0x0203
				"WM_RBUTTONDOWN",                //  0x0204
				"WM_RBUTTONUP",                  //  0x0205
				"WM_RBUTTONDBLCLK",              //  0x0206
				"WM_MBUTTONDOWN",                //  0x0207
				"WM_MBUTTONUP",                  //  0x0208
				"WM_MBUTTONDBLCLK"				 //  0x0209
			};
			static UINT MessageArray[] =
			{
				WM_MOUSEMOVE,
				WM_LBUTTONDOWN,
				WM_LBUTTONUP,
				WM_LBUTTONDBLCLK,
				WM_RBUTTONDOWN,
				WM_RBUTTONUP,
				WM_RBUTTONDBLCLK,
				WM_MBUTTONDOWN,
				WM_MBUTTONUP,
				WM_MBUTTONDBLCLK,
			};
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);

			m_csPannelMap->Lock();
			// 查找窗口对应的父窗口
			map<HWND, HWND>::iterator it = m_PanelMap.find(hWnd);
			if (it != m_PanelMap.end() &&
				IsWindow(it->second))
			{
				TraceMsgA("%s PostMessage hWnd(%08X) %s.\n", __FUNCTION__, it->second, szMouseMessage[message - WM_MOUSEMOVE]);
				::PostMessage(it->second, CVideoFrame::m_nMouseMessageArray[message - WM_MOUSEMOVE], (WPARAM)hWnd, lParam);
			}

			m_csPannelMap->Unlock();
		}           
		break;
		case 0x020E:                   //0x020A	WM_MOUSEHWHEEL
		{
			m_csPannelMap->Lock();
			// 查找窗口对应的父窗口
			map<HWND, HWND>::iterator it = m_PanelMap.find(hWnd);
			if (it != m_PanelMap.end() &&
				IsWindow(it->second))
			{
				TraceMsgA("%s PostMessage hWnd(%08X) WM_FRAME_MOUSEWHEEL.\n", __FUNCTION__, it->second);
				::PostMessage(it->second, WM_FRAME_MOUSEWHEEL, (WPARAM)hWnd, lParam);
			}

			m_csPannelMap->Unlock();
		}
		break;	
		case WM_MOUSEMOVE:
		{
			m_csPannelMap->Lock();

			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			// 查找窗口对应的父窗口
			map<HWND, HWND>::iterator it = m_PanelMap.find(hWnd);
			if (it != m_PanelMap.end() &&
				IsWindow(it->second))
			{
				//::ClientToScreen(hWnd, &pt);
				TraceMsgA("%s PostMessage hWnd(%08X) WM_FRAME_MOUSEMOVE.\n", __FUNCTION__, it->second);
				::PostMessage(it->second, WM_FRAME_MOUSEMOVE, (WPARAM)hWnd, MAKELPARAM(pt.x, pt.y));
			}

			m_csPannelMap->Unlock();
		}
		case WM_DESTROY:
		default:
			return ::DefWindowProc(hWnd, message, wParam, lParam);		// 必须要有这一句，不然窗口可能无法创建成功
		}
		return 0l;
	}
	
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnFrameLButtonDBClick(WPARAM W, LPARAM L)
	{
		TraceMsgA("%s PostMessage ParentWnd(%08X).\n", __FUNCTION__, GetParent()->GetSafeHwnd());
		GetParent()->PostMessage(WM_FRAME_LBUTTONDBLCLK, W, L);
		return 0L;
	}

	afx_msg LRESULT OnFrameRButtonDown(WPARAM W, LPARAM L)
	{
		TraceMsgA("%s PostMessage ParentWnd(%08X).\n", __FUNCTION__, GetParent()->GetSafeHwnd());
		GetParent()->PostMessage(WM_FRAME_RBUTTONDOWN, NULL, L);
		return 0L;
	}

	afx_msg LRESULT OnFrameMouseMove(WPARAM W, LPARAM L)
	{
		TraceMsgA("%s PostMessage ParentWnd(%08X).\n", __FUNCTION__, GetParent()->GetSafeHwnd());
		GetParent()->PostMessage(WM_FRAME_MOUSEMOVE, NULL, L);
		return 0L;
	}

	afx_msg LRESULT OnFrameRButtonUP(WPARAM W, LPARAM L)
	{
		TraceMsgA("%s PostMessage ParentWnd(%08X).\n", __FUNCTION__, GetParent()->GetSafeHwnd());
		GetParent()->PostMessage(WM_FRAME_RBUTTONUP, NULL, L);
		return 0L;
	}
	
	afx_msg LRESULT OnFrameLButtonDown(WPARAM W, LPARAM L)      //2018-03-05 
	{
		TraceMsgA("%s PostMessage ParentWnd(%08X).\n", __FUNCTION__, GetParent()->GetSafeHwnd());
		GetParent()->PostMessage(WM_FRAME_LBUTTONDOWN, NULL, L);
		return 0L;
	}
	
	// 放大或还原指窗口空格
	// nIndex	窗格序号，0为第一窗格
	// bFlag	true为放大窗口，此时空格将占满Frame
	//			false为还原窗口，此时窗格占还原为原始位置和尺寸
	bool ZoomPanel(int nIndex, bool bFlag);

	// 放大或还原指窗口空格
	// nRow		窗格行序号,0为第一行
	// nCol		窗格列序号,0为第一列
	// bFlag	true为放大窗口，此时空格将占满Frame
	//			false为还原窗口，此时窗格占还原为原始位置和尺寸
	bool ZoomPanel(int nRow, int nCol, bool bFlag);

	// 新增函数 int GetPanelFromPoint(POINT pt),以获取所在鼠标位置在所窗格号
	// CVideoFrame类不再处理鼠标消息，转而由其父窗口处理,父窗口收到鼠标消息后，应使用GetPanel函数判断鼠标坐标是否在Frame窗口内
	int GetPanelFromPoint(POINT pt)
	{
		int nIndex = -1;
		for (auto it = m_vecPanel.begin(); it != m_vecPanel.end(); it++)
		{
			if (PtInRect(&(*it)->rect, pt))
			{
				nIndex = it - m_vecPanel.begin();
				break;
			}
		}
		return -1;
	}
	// 交换两个窗格的位置
	// nPanel1,nPanel2为要交换窗格的索位值
	bool SwapPanel(int nPanel1, int nPanel2);
// 	HWND GetPanel(POINT pt)
// 	{
// 		return NULL;
// 	}
// 	afx_msg LRESULT OnFrameRButtonDown(WPARAM W, LPARAM L)
// 	{
// 		POINT pt;
// 		pt.x = LOWORD(L);
// 		pt.y = HIWORD(L);
// 		for (int nIndex = 0; nIndex < m_vecPanel.size(); nIndex++)
// 		{
// 			if (::PtInRect(&m_vecPanel[nIndex]->rect, pt))
// 			{
// 				m_nCurPanel = nIndex;
// 				if (!m_pCurSelectRect)
// 				{
// 					m_pLastSelectRect = nullptr;
// 				}
// 				else if (m_pCurSelectRect != &m_vecPanel[nIndex]->rect)//从另一窗格切换到当前窗格
// 				{
// 					m_pLastSelectRect = m_pCurSelectRect;
// 				}
// 				m_pCurSelectRect = &m_vecPanel[nIndex]->rect;
// 
// 				RefreshSelectedPanel();
// 				m_pCurrentFrame = this;
// 				GetParent()->SendMessage(WM_FRAME_RBUTTONDOWN, (WPARAM)MAKEWPARAM(pt.x, pt.y), (LPARAM)m_nCurPanel);
// 			}
// 		}
// 		return 0L;
// 	}

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


