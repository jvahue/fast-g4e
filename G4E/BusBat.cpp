// BusBat.cpp : implementation file
//

#include "stdafx.h"
#include "G4E.h"
#include "BusBat.h"


// BusBat dialog

IMPLEMENT_DYNAMIC(BusBat, CDialog)

BusBat::BusBat(float bus, float bat, CWnd* pParent /*=NULL*/)
	: CDialog(BusBat::IDD, pParent)
  , m_bus(bus)
  , m_bat(bat)
{
}

BusBat::~BusBat()
{
}

void BusBat::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_BUSV, m_bus);
  DDX_Text(pDX, IDC_BATV, m_bat);
}


BEGIN_MESSAGE_MAP(BusBat, CDialog)
END_MESSAGE_MAP()


// BusBat message handlers
