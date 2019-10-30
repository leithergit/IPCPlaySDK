
// ComboPlayDlg.h : 头文件
//

#pragma once


// CComboPlayDlg 对话框
class CComboPlayDlg : public CDialogEx
{
// 构造
public:
	CComboPlayDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_COMBOPLAY_DIALOG };
	IPC_PLAYHANDLE  m_hPlayer = nullptr;
	HWND m_hWndView = nullptr;
	RECT m_rtBorder;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonBrowse1();
	afx_msg void OnBnClickedButtonStartplay();
	afx_msg void OnBnClickedButtonStopplay();
	afx_msg void OnBnClickedButtonMoveup();
	afx_msg void OnBnClickedButtonMovedown();
};
