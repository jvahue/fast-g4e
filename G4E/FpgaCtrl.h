#pragma once


// CFpgaCtrl dialog

class CFpgaCtrl : public CDialog
{
	DECLARE_DYNAMIC(CFpgaCtrl)

public:
	CFpgaCtrl(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFpgaCtrl();

// Dialog Data
	enum { IDD = IDD_FPGA_CTRL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
