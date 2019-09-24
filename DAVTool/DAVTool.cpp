
// DAVTool.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "DAVTool.h"
#include "DAVToolDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDAVToolApp

BEGIN_MESSAGE_MAP(CDAVToolApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinAppEx::OnHelp)
END_MESSAGE_MAP()


// CDAVToolApp construction

CDAVToolApp::CDAVToolApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CDAVToolApp object

CDAVToolApp theApp;


// CDAVToolApp initialization

BOOL CDAVToolApp::InitInstance()
{
	AfxEnableControlContainer();
	AfxOleInit();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	SetRegistryKey(_T("Microsoft\\MFC\\Samples"));
	SetRegistryBase(_T("Settings"));

	InitCommonControls();
	InitContextMenuManager();
	InitShellManager();

	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	CMFCButton::EnableWindowsTheming();

	CDAVToolDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

