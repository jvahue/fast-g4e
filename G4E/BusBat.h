#pragma once


// BusBat dialog

class BusBat : public CDialog
{
	DECLARE_DYNAMIC(BusBat)

public:
	BusBat(float bus, float bat, CWnd* pParent = NULL);   // standard constructor
	virtual ~BusBat();

// Dialog Data
	enum { IDD = IDD_BUSBAT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
  float m_bus;
  float m_bat;
};
