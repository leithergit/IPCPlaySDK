// DialogVideo.cpp : implementation file
//

#include "stdafx.h"
#include "SyncPlayer.h"
#include "DialogVideo.h"
#include "afxdialogex.h"


// CDialogVideo dialog

IMPLEMENT_DYNAMIC(CDialogVideo, CDialogEx)

CDialogVideo::CDialogVideo(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDialogVideo::IDD, pParent)
{

}

CDialogVideo::~CDialogVideo()
{
}

void CDialogVideo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDialogVideo, CDialogEx)
END_MESSAGE_MAP()


// CDialogVideo message handlers
