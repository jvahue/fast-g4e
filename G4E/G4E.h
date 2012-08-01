// G4E.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CG4EApp:
// See G4E.cpp for the implementation of this class
//

class CG4EApp : public CWinApp
{
public:
	CG4EApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CG4EApp theApp;