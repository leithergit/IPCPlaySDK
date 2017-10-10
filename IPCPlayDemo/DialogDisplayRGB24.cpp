// DialogDisplayRGB24.cpp : implementation file
//

#include "stdafx.h"
#include "IPCPlayDemo.h"
#include "DialogDisplayRGB24.h"
#include "afxdialogex.h"


// CDialogDisplayRGB24 dialog

IMPLEMENT_DYNAMIC(CDialogDisplayRGB24, CDialogEx)

CDialogDisplayRGB24::CDialogDisplayRGB24(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDialogDisplayRGB24::IDD, pParent)
{

}

CDialogDisplayRGB24::~CDialogDisplayRGB24()
{
}

void CDialogDisplayRGB24::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDialogDisplayRGB24, CDialogEx)
END_MESSAGE_MAP()


// CDialogDisplayRGB24 message handlers
