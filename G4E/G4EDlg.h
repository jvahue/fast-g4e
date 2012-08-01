// G4EDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

#include "IocSocket.h"
#include "FPGAemulation.h"
#include "DioEmulation.h"
#include "DPRAMemul.h"

#if defined(_DEBUG)
#define BUILD "Debug "__DATE__ " " __TIME__
#else
#define BUILD "Release "__DATE__ " " __TIME__
#endif


// CG4EDlg dialog
class CG4EDlg : public CDialog
{
// Construction
public:
	CG4EDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_G4E_DIALOG };

    enum Constants {
        UIupdateTime = 10
    };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    BOOL m_powerOnG4;
    BOOL m_runSimulation;
    bool m_loadingCfg;
    int  m_clearGseTxRequest;
    CString m_gseTxDisplay;
    CString m_gseTx;
    CString m_gseRx;
    CString m_cfgFilename;
    CStdioFile m_cfgFile;
    CFile m_logFile;
    IocSocket m_ioc;
    bool m_uiCmdRequest;
    time_t m_startTime;
    BOOL m_steConn;
    CComboBox m_interruptRequest;
    CString m_IRQsent;
    CString m_status;
    CStatic m_statusWnd;
    CComboBox m_din;
    BOOL m_dinSts;
    UINT32 m_dinValues;
    CComboBox m_dout;
    BOOL m_doutSts;
    UINT32 m_doutValues;
    UINT32 m_wdResetRequest;

    FPGAemulation* m_fpga;
    DioEmulation*  m_dio;
    DPRamEmulation* m_dpr;

    float m_busV;
    float m_batV;
    boolean m_shutDownStarted;


    afx_msg void OnClose();
    //afx_msg void OnBnClickedRunG4();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedLdCfg();
    afx_msg void OnBnClickedSend();
    afx_msg void OnBnClickedClear();
    afx_msg void OnBnClickedDinSet();
    afx_msg void OnCbnSelchangeDin();
    afx_msg void OnCbnSelchangeDout();

protected:
    void UpdateGseTxDisplay(CString& lineBuffer);
    bool ProcessG4eInterface();
    bool ProcessIocInterface();
    CString LastNlines(CString inStr, int N);
    CString TimeRep( UINT32 elapsed);
    void PowerUpG4E();
    //void PowerDownG4E();
    void ShutItDown();
public:
    CButton m_dinStsCtrl;
    CButton m_doutStsCtrl;
    afx_msg void OnBnClickedEsc();
    afx_msg void OnBnClickedBusbat();
    CButton m_pwr;
    CButton m_lnk;
    CButton m_dat;
    CButton m_cfg;
    CButton m_xfr;
    CButton m_sts;
    CButton m_cfm;
    CButton m_flt;
    CButton m_lss0;
    CButton m_lss1;
    CButton m_lss2;
    CButton m_lss3;
};
