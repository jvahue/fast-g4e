// FpgaCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "G4E.h"
#include "FpgaCtrl.h"


// CFpgaCtrl dialog

IMPLEMENT_DYNAMIC(CFpgaCtrl, CDialog)

CFpgaCtrl::CFpgaCtrl(CWnd* pParent /*=NULL*/)
	: CDialog(CFpgaCtrl::IDD, pParent)
{

}

CFpgaCtrl::~CFpgaCtrl()
{
}

void CFpgaCtrl::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CFpgaCtrl, CDialog)
END_MESSAGE_MAP()


// CFpgaCtrl message handlers
