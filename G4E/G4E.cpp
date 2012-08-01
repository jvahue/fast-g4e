// G4E.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "G4E.h"
#include "G4EDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CG4EApp

BEGIN_MESSAGE_MAP(CG4EApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

HANDLE hModulMutex;

BOOL FindPreviousInstance()
{
    // Check whether an instance of this program is still running
    // therefor create a global uniquely named mutex; if the mutex exist, GetLastError() returns ALREADY_EXIST
    hModulMutex = CreateMutex(NULL, FALSE, "OnlyOneG4ePlease");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        return TRUE; // previous instance available
    }
    return FALSE; // no previous instance
}


// CG4EApp construction

CG4EApp::CG4EApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CG4EApp object

CG4EApp theApp;


// CG4EApp initialization

BOOL CG4EApp::InitInstance()
{
//TODO: call AfxInitRichEdit2() to initialize richedit2 library.
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

    if ( !FindPreviousInstance())
    {
        if (!AfxSocketInit())
        {
            AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
            return FALSE;
        }

	    AfxEnableControlContainer();

	    // Standard initialization
	    // If you are not using these features and wish to reduce the size
	    // of your final executable, you should remove from the following
	    // the specific initialization routines you do not need
	    // Change the registry key under which our settings are stored
	    // TODO: You should modify this string to be something appropriate
	    // such as the name of your company or organization
	    SetRegistryKey(_T("Knowlogic"));

	    CG4EDlg dlg;
	    m_pMainWnd = &dlg;
	    INT_PTR nResponse = dlg.DoModal();
        ReleaseMutex( hModulMutex);
    }

    // Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
