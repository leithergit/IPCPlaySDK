
// RapidPlay.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CRapidPlayApp:
// See RapidPlay.cpp for the implementation of this class
//

class CRapidPlayApp : public CWinApp
{
public:
	CRapidPlayApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CRapidPlayApp theApp;