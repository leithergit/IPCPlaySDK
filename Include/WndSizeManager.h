#pragma once
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展
#include <afxdisp.h>        // MFC 自动化类

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC 对 Internet Explorer 4 公共控件的支持
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC 对 Windows 公共控件的支持
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <vector>
using namespace std;
enum DockType
{
	DockLeft	 = 1,			// 无须变动坐标
	DockTop		 = 2,			// 无须变动坐标
	DockRight	 = 4,			// 保持和父窗口的右边的距离
	DockBottom	 = 8,			// 保持和父窗口的底部的距离
	DockCenter	 = 16,			// 保持和父窗口四边的的距离
	DockCtrlLeft = 32,			// 保持和指定控件左边的距离
	DockCtrlTop	 = 64,			// 保持和指定控件顶部的距离
	DockCtrlRight= 128,			// 保持和指定控件左边的距离
	DockCtrlBottom=256,			// 保持和指定控件底部的距离
};

enum DockIndex
{
	DILeft = 0,
	DITop  = 1,
	DIRight= 2,
	DIBottom=3
};
struct WndPostionInfo
{
	HWND	hWnd;
	UINT	nID;
	DockType nDockType;			// 停靠类型
	UINT	nDockCtrl[4];
	UINT	DockDistance[4];	// 停靠距离
	RECT	rect;				// 原始大小
	bool	bAutoMove;			// 是否自动移动
	RECT	rtCurrentPos;
	WndPostionInfo()
	{
		ZeroMemory(this,sizeof(WndPostionInfo));
		bAutoMove = true;
	}
};

//////////////////////////////////////////////////////////////////////////
// 子窗口位置自动停靠管理类
// 用于父窗口尺寸或位置发生变化时，自动管理子窗口的位置信息
// 作者:李雄高
// 2016.04.10
//////////////////////////////////////////////////////////////////////////
class CWndSizeManger
{
private:
	vector<WndPostionInfo> m_vWndPostionInfo;
	CWnd	*m_pParentWnd;
public:
	CWndSizeManger()
	{

	}
	CWndSizeManger(CWnd* ParentWnd)
	{
		m_pParentWnd = ParentWnd;

	}
	~CWndSizeManger(void)
	{
		m_vWndPostionInfo.clear();
	}
	// 设置子窗口的父窗口
	// 该函数必须要SaveWndPosition()函数前调用
	void SetParentWnd(CWnd* ParentWnd)
	{
		m_pParentWnd = ParentWnd;

	}

	// 批量保存窗口的起始位置，建议在窗口初始化后并且所有子窗口的位置都不再变动时调用
	// nDlgItemIDArray		窗口ID的数组
	// nIDCount				nDlgItemIDArray中窗口ID的数量
	// nDock				父窗口位置变化后子窗口的停靠位置
	// bAutoMove			父窗口位置变化后子窗口是自动移动到停靠位置
	bool SaveWndPosition(UINT *nDlgItemIDArray, UINT nIDCount, DockType nDock ,bool bAutoMove = true)
	{
		if (!m_pParentWnd)
			return false;
		RECT rtClientRect;
		m_pParentWnd->GetClientRect(&rtClientRect);
		WndPostionInfo wndPos;
		wndPos.bAutoMove = bAutoMove;
		wndPos.nDockType = nDock;
		CWnd *pTempWnd = NULL;
		for (int i = 0; i < nIDCount; i++)
		{
			wndPos.hWnd = m_pParentWnd->GetDlgItem(nDlgItemIDArray[i])->GetSafeHwnd();
			if (!wndPos.hWnd)
			{
				assert(false);
				continue;
			}
			wndPos.nID = nDlgItemIDArray[i];
			::GetWindowRect(wndPos.hWnd, &wndPos.rect);
			m_pParentWnd->ScreenToClient(&wndPos.rect);
			switch (wndPos.nDockType)
			{
			default:
				{
					if ((wndPos.nDockType & DockLeft) == DockLeft )
						wndPos.DockDistance[DILeft] = wndPos.rect.left - rtClientRect.left;
					if ((wndPos.nDockType & DockTop) == DockTop )
						wndPos.DockDistance[DITop] = wndPos.rect.top - rtClientRect.top;
					if ((wndPos.nDockType & DockRight) == DockRight )
						wndPos.DockDistance[DIRight] = rtClientRect.right - wndPos.rect.right;
					if ((wndPos.nDockType & DockBottom) == DockBottom )
						wndPos.DockDistance[DIBottom] = rtClientRect.bottom - wndPos.rect.bottom;
					if ((wndPos.nDockType & DockCenter) == DockCenter)
					{
						RECT rtNeighbor = {0};
						HWND hWndNeighbor = NULL;
						// 在控制件的左侧
						if ((wndPos.nDockType & DockCtrlLeft) == DockCtrlLeft)
						{
							hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DILeft])->m_hWnd;
							::GetWindowRect(hWndNeighbor, &rtNeighbor);
							m_pParentWnd->ScreenToClient(&rtNeighbor);
							wndPos.DockDistance[DILeft] = rtNeighbor.right- wndPos.rect.left;
						}
						else if ((wndPos.nDockType & DockCtrlTop) == DockCtrlTop)
						{
							hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DITop])->m_hWnd;
							::GetWindowRect(hWndNeighbor, &rtNeighbor);
							m_pParentWnd->ScreenToClient(&rtNeighbor);
							wndPos.DockDistance[DITop] = rtNeighbor.top - wndPos.rect.bottom ;
						}
						else if ((wndPos.nDockType & DockCtrlRight) == DockCtrlRight)
						{
							hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DIRight])->m_hWnd;
							::GetWindowRect(hWndNeighbor, &rtNeighbor);
							m_pParentWnd->ScreenToClient(&rtNeighbor);
							wndPos.DockDistance[DIRight] = wndPos.rect.left - rtNeighbor.right;

						}
						else if ((wndPos.nDockType & DockCtrlBottom) == DockCtrlBottom)
						{
							hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DILeft])->m_hWnd;
							::GetWindowRect(hWndNeighbor, &rtNeighbor);
							m_pParentWnd->ScreenToClient(&rtNeighbor);
							wndPos.DockDistance[DILeft] = wndPos.rect.top - rtNeighbor.bottom;
						}
					}
					
					break;
				}
			case DockTop:		// 无须变动
				{
					wndPos.DockDistance[DITop] = wndPos.rect.top - rtClientRect.top;
					break;
				}
			case DockLeft:		// 无须变动
				{
					wndPos.DockDistance[DILeft] = wndPos.rect.left - rtClientRect.left;
					break;
				}
			case DockBottom:
				{
					wndPos.DockDistance[DIBottom] = rtClientRect.bottom - wndPos.rect.bottom;
					break;
				}
			case DockRight:
				{
					wndPos.DockDistance[DIRight] = rtClientRect.right - wndPos.rect.right;
					break;
				}
			case DockCenter:
				{
					wndPos.DockDistance[DILeft] = wndPos.rect.left - rtClientRect.left;
					wndPos.DockDistance[DITop] = wndPos.rect.top - rtClientRect.top;
					wndPos.DockDistance[DIRight] = rtClientRect.right - wndPos.rect.right;
					wndPos.DockDistance[DIBottom] = rtClientRect.bottom - wndPos.rect.bottom;
				}
				break;
			}

			m_vWndPostionInfo.push_back(wndPos);
		}
		return true;
	}

	// 处理窗口位置变动, 若在调用SaveWndPosition()时，第五个参数为false,则该窗口不作移动，只更新其rtCurrentPos值
	// 可通过GetWndCurrentPostion()函数取得该参数值
	void OnSize(UINT nType, int cx, int cy)
	{
		if (!m_pParentWnd)
			return;
		RECT rtClientRect;
		m_pParentWnd->GetClientRect(&rtClientRect);
		for (vector<WndPostionInfo>::iterator it = m_vWndPostionInfo.begin(); it != m_vWndPostionInfo.end(); it++)
		{
			WndPostionInfo &wndPos = (*it);
			RECT rt = wndPos.rect;
			switch (wndPos.nDockType)
			{
			default:
				{
					if (nType == SIZE_MAXIMIZED)
					{
						if ((wndPos.nDockType & DockLeft) == DockLeft)
						{
							rt.left = rtClientRect.left +  wndPos.DockDistance[DILeft];
							if ((wndPos.nDockType & DockRight) != DockRight)
								rt.right = rt.left + wndPos.DockDistance[DIRight];
						}
						if ((wndPos.nDockType & DockTop) == DockTop)
						{
							rt.top = rtClientRect.top + wndPos.DockDistance[DITop];
							if ((wndPos.nDockType & DockBottom) != DockBottom)
								rt.bottom = rt.top + wndPos.DockDistance[DIBottom];
						}
							
						if ((wndPos.nDockType & DockRight) == DockRight)
						{
							rt.right = rtClientRect.right - wndPos.DockDistance[DIRight];
							if ((wndPos.nDockType & DockLeft) != DockLeft)
								rt.left = rt.right - RectWidth(wndPos.rect);
						}
						if ((wndPos.nDockType & DockBottom) == DockBottom)
						{
							rt.bottom = rtClientRect.bottom - wndPos.DockDistance[DIBottom];
							if ((wndPos.nDockType & DockTop) != DockTop)
								rt.top = rt.bottom - RectHeight(wndPos.rect);
						}
						if ((wndPos.nDockType & DockCenter) == DockCenter)
						{
							RECT rtNeighbor = {0};
							HWND hWndNeighbor = NULL;
							// 在控制件的左侧
							if ((wndPos.nDockType & DockCtrlLeft) == DockCtrlLeft)
							{
								hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DILeft])->m_hWnd;
								::GetWindowRect(hWndNeighbor, &rtNeighbor);
								m_pParentWnd->ScreenToClient(&rtNeighbor);
								wndPos.DockDistance[DILeft] = rtNeighbor.right- wndPos.rect.left;
							}
							if ((wndPos.nDockType & DockCtrlTop) == DockCtrlTop)
							{
								hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DITop])->m_hWnd;
								::GetWindowRect(hWndNeighbor, &rtNeighbor);
								m_pParentWnd->ScreenToClient(&rtNeighbor);
								wndPos.DockDistance[DITop] = rtNeighbor.top - wndPos.rect.bottom ;
							}
							if ((wndPos.nDockType & DockCtrlRight) == DockCtrlRight)
							{
								hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DIRight])->m_hWnd;
								::GetWindowRect(hWndNeighbor, &rtNeighbor);
								m_pParentWnd->ScreenToClient(&rtNeighbor);
								wndPos.DockDistance[DIRight] = wndPos.rect.left - rtNeighbor.right;
							}
							if ((wndPos.nDockType & DockCtrlBottom) == DockCtrlBottom)
							{
								hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DILeft])->m_hWnd;
								::GetWindowRect(hWndNeighbor, &rtNeighbor);
								m_pParentWnd->ScreenToClient(&rtNeighbor);
								wndPos.DockDistance[DILeft] = wndPos.rect.top - rtNeighbor.bottom;
							}
						}
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					else
					{
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					memcpy(&wndPos.rtCurrentPos,&rt,sizeof(RECT));
					break;
				}
			case DockTop:		// 无须变动
			case DockLeft:		// 无须变动
				{
					memcpy(&wndPos.rtCurrentPos,&rt,sizeof(RECT));
				}
				break;
			case DockBottom:
				{
					if (nType == SIZE_MAXIMIZED)
					{
						rt.bottom = rtClientRect.bottom - wndPos.DockDistance[DIBottom];
						rt.top = rt.bottom - (wndPos.rect.bottom - wndPos.rect.top);
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					else if (nType == SIZE_RESTORED)
					{
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					memcpy(&wndPos.rtCurrentPos,&rt,sizeof(RECT));
				}
				break;
			case DockRight:
				{
					if (nType == SIZE_MAXIMIZED)
					{
						rt.right = rtClientRect.right - wndPos.DockDistance[DIRight];
						rt.left = rt.right - (wndPos.rect.right - wndPos.rect.left);
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					else if (nType == SIZE_RESTORED)
					{
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					memcpy(&wndPos.rtCurrentPos,&rt,sizeof(RECT));
					break;

				}
			case DockCenter:
				{
					if (nType == SIZE_MAXIMIZED)
					{
						rt.left = rtClientRect.left + wndPos.DockDistance[DILeft];
						rt.top = rtClientRect.top + wndPos.DockDistance[DITop];
						rt.right = rtClientRect.right - wndPos.DockDistance[DIRight];
						rt.bottom = rtClientRect.bottom - wndPos.DockDistance[DIBottom];			
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					else if (nType == SIZE_RESTORED)
					{
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					memcpy(&wndPos.rtCurrentPos,&rt,sizeof(RECT));
					break;
				}
			}
		}
	}

	// 获取指定ID的窗口在父窗口尺寸变化的尺寸信息
	// 返回true时获取成功，否则失败
	bool GetWndCurrentPostion(UINT nID,RECT &rt)
	{
		for (vector<WndPostionInfo>::iterator it = m_vWndPostionInfo.begin(); it != m_vWndPostionInfo.end(); it++)
			if ((*it).nID == nID)
			{
				memcpy(&rt,&(*it).rtCurrentPos,sizeof(RECT));
				return true;
			}
			return false;
	}
};
