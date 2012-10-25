// G4EDlg.cpp : implementation file
//

#include "stdafx.h"
#include "G4E.h"
#include "G4EDlg.h"
#include "BusBat.h"

#include "HW_Emul.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define BIT( word, bit)  ((word)>>(bit) & 1U)
#define FIELD( w, b ,s) ( ((w) & ( (1U << (s)) - 1U)) << (b) )

////////////////////////////////////////////////////////////////////////////////////////////////////
// Connections to the G4 code
extern "C"
{

extern unsigned int _cdecl RunMain( void* pParam );

SYS_CTRL G4sysCtrl;

// get access to DIO names
extern DIO_CONFIG DIO_OutputPins[DIO_MAX_OUTPUTS];
extern DIO_CONFIG DIO_InputPins[DIO_MAX_INPUTS]; 

};

////////////////////////////////////////////////////////////////////////////////////////////////////
char* IRQ_Names[] = {
    "None",
    "FPGA",
    "DPRAM",
    "Power Fail",
    "GSE",
    "SP0",
    "SP1",
    "SP2",
    "",
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////////////////////////////////////
// CG4EDlg dialog
CG4EDlg::CG4EDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CG4EDlg::IDD, pParent)
    , m_powerOnG4(FALSE)
    , m_gseTx(_T(""))
    , m_gseTxDisplay(_T(""))
    , m_runSimulation(FALSE)
    , m_gseRx(_T(""))
    , m_cfgFilename( _T(""))
    , m_loadingCfg( false)
    , m_clearGseTxRequest( 0)
    , m_ioc( IocSocket::eClient)
    , m_uiCmdRequest(false)
    , m_steConn(FALSE)
    , m_IRQsent(_T(""))
    , m_status(_T(""))
    , m_dinSts(FALSE)
    , m_doutSts(FALSE)
    , m_dinValues(0)
    , m_doutValues(0)
    , m_wdResetRequest(0)
    , m_busV(28.0f)
    , m_batV(28.0f)
    , m_shutDownStarted( false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_fpga = (FPGAemulation*)GetFpgaObject();
    m_dio = (DioEmulation*)GetDioObject();
    m_dpr = (DPRamEmulation*)GetDprObject();
}

void CG4EDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //DDX_Check(pDX, IDC_RUN_G4, m_powerOnG4);
    DDX_Text(pDX, IDC_GSE_TX, m_gseTxDisplay);
    //DDX_Check(pDX, IDC_RUN_SIM, m_runSimulation);
    DDX_Text(pDX, IDC_GSE_RX, m_gseRx);
    DDX_Check(pDX, IDC_STE_CONN, m_steConn);
    DDX_Control(pDX, IDC_IRQST, m_interruptRequest);
    DDX_Text(pDX, IDC_IRQ, m_IRQsent);
    DDX_Control(pDX, IDC_STATUS, m_statusWnd);
    DDX_Control(pDX, IDC_DIN, m_din);
    DDX_Check(pDX, IDC_DIN_STS, m_dinSts);
    DDX_Control(pDX, IDC_DOUT, m_dout);
    DDX_Check(pDX, IDC_DOUT_STS, m_doutSts);
    DDX_Control(pDX, IDC_DIN_STS, m_dinStsCtrl);
    DDX_Control(pDX, IDC_DOUT_STS, m_doutStsCtrl);
    DDX_Control(pDX, IDC_PWR, m_pwr);
    DDX_Control(pDX, IDC_LNK, m_lnk);
    DDX_Control(pDX, IDC_DAT, m_dat);
    DDX_Control(pDX, IDC_CFG, m_cfg);
    DDX_Control(pDX, IDC_XFR, m_xfr);
    DDX_Control(pDX, IDC_STS, m_sts);
    DDX_Control(pDX, IDC_CFM, m_cfm);
    DDX_Control(pDX, IDC_FLT, m_flt);
    DDX_Control(pDX, IDC_LSS0, m_lss0);
    DDX_Control(pDX, IDC_LSS1, m_lss1);
    DDX_Control(pDX, IDC_LSS2, m_lss2);
    DDX_Control(pDX, IDC_LSS3, m_lss3);
}

BEGIN_MESSAGE_MAP(CG4EDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_WM_TIMER()
    ON_WM_CLOSE()        
    //ON_BN_CLICKED(IDC_RUN_G4, &CG4EDlg::OnBnClickedRunG4)
    ON_BN_CLICKED(IDOK, &CG4EDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_LD_CFG, &CG4EDlg::OnBnClickedLdCfg)
    ON_BN_CLICKED(IDC_SEND, &CG4EDlg::OnBnClickedSend)
    ON_BN_CLICKED(IDC_CLEAR, &CG4EDlg::OnBnClickedClear)
    ON_BN_CLICKED(IDC_DIN_SET, &CG4EDlg::OnBnClickedDinSet)
    ON_CBN_SELCHANGE(IDC_DIN, &CG4EDlg::OnCbnSelchangeDin)
    ON_CBN_SELCHANGE(IDC_DOUT, &CG4EDlg::OnCbnSelchangeDout)
    ON_BN_CLICKED(IDC_ESC, &CG4EDlg::OnBnClickedEsc)
    ON_BN_CLICKED(IDC_BUSBAT, &CG4EDlg::OnBnClickedBusbat)
END_MESSAGE_MAP()


// CG4EDlg message handlers
void CG4EDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CG4EDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CG4EDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

BOOL CG4EDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    CRect rect;
    GetWindowRect(&rect);
    SetWindowPos( NULL, 0, 0, rect.Width(), rect.Height(), SWP_SHOWWINDOW);

    CString title;
    AfxGetMainWnd()->GetWindowText(title);
    title = title + _T(" ")  + BUILD;    
    AfxGetMainWnd()->SetWindowText(title);

    // Fill the DIN drop downs
    for ( int i = 0; i < DIO_MAX_INPUTS; ++i)
    {
        m_din.AddString(DIO_InputPins[i].Name);
    }
    m_din.SetCurSel(20);
    for ( int i = 0; i < DIO_MAX_OUTPUTS; ++i)
    {
        m_dout.AddString(DIO_OutputPins[i].Name);
    }
    m_dout.SetCurSel(6);

    // Add interrupt requests
    for (int i = 0; IRQ_Names[i][0] != 0; ++i)
    {
        m_interruptRequest.AddString(IRQ_Names[i]);
    }
    m_interruptRequest.SetCurSel(0);
    
    printf("Open file\n");
    CString temp;
    temp.Format("Startup at %d\n", clock());
    m_logFile.Open(_T("gse.log"), CFile::modeCreate|CFile::modeWrite|CFile::shareDenyWrite);
    m_logFile.Write((LPCTSTR)temp, temp.GetLength());
    
#if 0
    // test sleep timer
    for (int x=0; x < 10; ++x)
    {
        time_t start = clock();
        for (int i=0; i < 100; ++i)
        {
            Sleep(1);
        }
        TRACE( "Sleep Tick = %f\n", float(clock() - start)/100.0f);
    }
#endif

    // Init the Interface Structure Names
    G4sysCtrl.runG4thread = G4sysCtrl.g4eInitComplete = G4sysCtrl.shutDownComplete = 0;
    G4sysCtrl.holdOnConnection = 1;
    G4sysCtrl.runSec = G4sysCtrl.irqId = G4sysCtrl.irqRqst = 0;
    G4sysCtrl.irqHandled = G4sysCtrl.timerDF = 0;

    G4sysCtrl.g4i.SetName( "G4E2CP");
    G4sysCtrl.g4o.SetName( "CP2G4E");

    G4sysCtrl.busV = 28.0f;
    G4sysCtrl.batV = 28.0f;

    // Launch all the G4 thread
    PowerUpG4E();

    // Connect to the IOC
    m_ioc.GetConnection();

    UpdateData(FALSE);

    //ShowWindow(SW_MINIMIZE);
    m_pwr.SetCheck(1);

    // Start the timer
    SetTimer( 1, UIupdateTime, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}


//--------------------------------------------------------------------------------------------------
void CG4EDlg::PowerUpG4E()
{
    m_fpga->Reset(); // this must come first as there is DIO in the FPGA
    m_dio->Reset();
    m_dpr->Reset();

    G4sysCtrl.g4eInitComplete = FALSE;
    G4sysCtrl.runSec = 0;
    G4sysCtrl.runG4thread = TRUE;
    m_powerOnG4 = TRUE;
    m_runSimulation = TRUE;
    m_startTime = clock();

    AfxBeginThread( RunMain, &G4sysCtrl);
    UpdateData(FALSE);
}

//--------------------------------------------------------------------------------------------------
// Ok Button processing ... assume the user is done with the GSE command and send it
void CG4EDlg::OnBnClickedOk()
{
    OnBnClickedSend();
}

//--------------------------------------------------------------------------------------------------
void CG4EDlg::OnTimer(UINT_PTR nIDEvent)
{
    bool changedIoc = false;
    bool changedG4e = false;
    UpdateData();

    // process data from the IOC, unless we are shutting down
    if (!m_shutDownStarted)
    {
      changedIoc = ProcessIocInterface();
      changedG4e = ProcessG4eInterface();
    }

    if ( changedIoc || changedG4e)
    {
        //TRACE("IOC:%d G4E:%d\n", changedIoc?1:0, changedG4e?1:0);
        UpdateData(FALSE);
    }

    time_t sec = (clock() - m_startTime)/1000;
    CString elapsed = TimeRep((UINT32)sec);
    CString simTime = TimeRep(G4sysCtrl.runSec);
    time_t delta = sec - G4sysCtrl.runSec;
    CString deltaStr = TimeRep( (UINT32)delta);

    m_status.Format(_T("G4 Status Elapsed[%s] Sim[%s] Delta[%s]"), elapsed, simTime, deltaStr);
    m_statusWnd.SetWindowText(m_status);

    SetTimer( 1, UIupdateTime, NULL);

    CDialog::OnTimer(nIDEvent);
}

CString CG4EDlg::TimeRep( UINT32 elapsed)
{
    CString rep;

    UINT32 sec = elapsed;
    int hours = sec/3600;
    sec = sec - (hours*3600);
    int min = sec/60;
    sec = sec - (min*60);

    rep.Format("%02d:%02d:%02d", hours, min, sec);
    return rep;
}

//--------------------------------------------------------------------------------------------------
// 
bool CG4EDlg::ProcessIocInterface()
{
    int curSel;
    bool changed = false;
    static int hb = 0;
    static unsigned int lastIocHb = 0;

    static int subFrame = 3;

    if ( m_ioc.IsConnected() != (m_steConn == 1 ? true : false))
    {
        m_steConn = m_ioc.IsConnected() ? 1 : 0;
        changed = true;

        // DIN Ctrl is only available when no STE connection
        m_dinStsCtrl.EnableWindow(!m_steConn);

        G4sysCtrl.holdOnConnection = 0;
    }

    if ( !m_ioc.IsConnected())
    {
        static clock_t lastAttempt = clock();

        if ( clock() - lastAttempt > 2000)
        {
            // try to establish connection to a data source
            m_ioc.Client();
            lastAttempt = clock();
        }

        // turn on internal DINs 
        m_dinValues |= 1 << WLAN_WOW_Enb;
        m_dinValues |= 1 << FFD_Enb;

        // share our PS value with the G4 code
        G4sysCtrl.busV = m_busV;
        G4sysCtrl.batV = m_batV;
    }
    else
    {
        //G4O_Data g4o;
        G4I_Data g4i;

        //------------------------------------------------------------------------------------------
        // GSE Rx Data from the IOC
        while ( m_ioc.m_g4i.Read(g4i))
        {
            if ( lastIocHb+1 != g4i.hb)
            {
                CString temp;
                temp.Format("Missed IOC HB(%d) Local(%d)\n", g4i.hb,lastIocHb);
                m_logFile.Write((LPCTSTR)temp, temp.GetLength());
            }
            lastIocHb = g4i.hb;

            // terminate the data for handling as CString
            g4i.gseRx[g4i.gseRxSize] = '\0';

            //TRACE("I(%d)\n", g4i.hb);
            if (strlen(g4i.gseRx) > 0)
            {
                m_gseRx = g4i.gseRx;
                m_uiCmdRequest = true;
            }

            if ( m_busV < g4i.busV)
            {
                G4sysCtrl.busV = m_busV;
                G4sysCtrl.batV = m_batV;
            }
            else
            {
                G4sysCtrl.busV = g4i.busV;
                G4sysCtrl.batV = g4i.batV;
            }

            // don't give any 429/717 data to the G4 code until init is complete
            // because it will screw up BIT
            if ( G4sysCtrl.g4eInitComplete)
            {
                // pass it down to the G4E once init is done
                G4sysCtrl.g4i.Write(g4i);

                // before we put any A429 data in the FIFO lets make sure our speeds match
                BYTE matches = m_fpga->A429SpeedMatch( g4i.a429Speeds);

                for (int i = 0; i < g4i.sensorCount; ++i)
                {
                    // Handle an Arinc 429 input
                    UINT16 chan = g4i.chan[i] & 0xF;
                    // if speed matches
                    if ( BIT(matches, chan) == 1)
                    {
                        // and the msg is allowed
                        if ( m_fpga->MessageFilter( chan, g4i.sensors[i]))
                        {
                            m_fpga->m_rxFifo[chan].Push( g4i.sensors[i]);
                        }
                    }
                }

                // handle QAR data
                if ( (g4i.QARstatus & QAR_DATA_PRESENT) == 0) // is the bus running
                {
                    m_fpga->QarOff();
                }
                else if ( (g4i.QARstatus & QAR_NEW_SF) != 0)
                {
                    UINT16 sfx = g4i.QARstatus & 0xF;
                    m_fpga->WriteQarData( sfx, g4i.QARwords, g4i.QAR);

                    // this must be done after the words are written as WriteQarData clear BE
                    if ( (g4i.QARstatus & BARKER_ERR) != 0)
                    {
                        // turn on the barker error bit in the status reg
                        m_fpga->BarkerError( (g4i.QARstatus & NO_FRAME_SYNC) != 0);
                    }
                }
            }

            // copy the packed DIN values, even before init done
            m_dinValues = g4i.din;

            // update the displays of the selected DIN/DOUT
            curSel = m_din.GetCurSel();
            if ( m_dinStsCtrl.GetCheck() != BIT(m_dinValues, curSel))
            {
                m_dinStsCtrl.SetCheck( BIT(m_dinValues, curSel));
                changed = true;
            }            
        }

        //------------------------------------------------------------------------------------------
        // G4E Output to the IOC
        // GSE Tx to the IOC was done during G4E processing, it just forwarded on g4o packet

        curSel = m_dout.GetCurSel();
        if ( m_doutStsCtrl.GetCheck() != BIT(m_doutValues, curSel))
        {
            m_doutStsCtrl.SetCheck( BIT(m_doutValues, curSel));
            changed = true;
        }
    }

    return changed;
}

//--------------------------------------------------------------------------------------------------
// SendG4Cmds: Handle sending commands to the GSE from the UI or a Cfg file load, or the IOC.
//
bool CG4EDlg::ProcessG4eInterface()
{
#define WRAP 28

    static time_t loadTimer = 0;
    G4O_Data g4o;
    G4I_Data g4i;
    CString lineBuffer;
    int x = 0;

    bool changed = false;
    UINT32 dinZ = m_dinValues;
    UINT32 doutZ = m_doutValues;

    memset( &g4o, 0, sizeof(g4o));
    memset( &g4i, 0, sizeof(g4i));

    if ( !m_shutDownStarted)
    {
        //------------------------------------------------------------------------------------------
        // Handle G4 Outputs
        while ( G4sysCtrl.g4o.Read(g4o))
        {
            //TRACE("G:%d\n", g4o.hb);
            g4o.gseTx[g4o.gseTxSize] = '\0';
            lineBuffer += g4o.gseTx;
            if ( !lineBuffer.IsEmpty())
            {
                //TRACE("LB%02d: <%s>\n", x, lineBuffer);
            }
            x++;

            // DOUTs are set via the call to ProcessDio (see below)

            m_wdResetRequest = g4o.wdResetRequest;

            // forward this on to the IOC
            m_ioc.m_g4o.Write(g4o);
            m_ioc.TxData();
        }
        UpdateGseTxDisplay(lineBuffer);

        //------------------------------------------------------------------------------------------
        // Handle G4I Processing

        // is the G4E UI loading a Cfg file?
        if (m_loadingCfg)
        {
            if (clock() - loadTimer > 100)
            {
                loadTimer = clock();

                // send another line, check for completion
                if ( !m_cfgFile.ReadString(m_gseRx))
                {
                    m_loadingCfg = false;
                    m_cfgFile.Close();
                }
                else
                {
                    m_gseRx.Trim();
                    if ( !m_gseRx.IsEmpty())
                    {
                        m_uiCmdRequest = true;
                    }
                }
            }
        }
        else if ( !m_cfgFilename.IsEmpty())
        {
            m_cfgFile.Open( m_cfgFilename, CFile::modeRead);
            m_cfgFilename.Empty();
            m_loadingCfg = true;
            loadTimer = 0;
        }

        if ( m_uiCmdRequest)
        {
            CString temp( m_gseRx);
            if ( temp.GetLength() == 0 || 
                (temp[temp.GetLength()-1] != '\n' && temp[temp.GetLength()-1] != '\r'))
            {
                temp += '\n';
            }
            m_logFile.Write((LPCTSTR)temp, temp.GetLength());
            strncpy( g4i.gseRx, (LPCTSTR)temp, sizeof(g4i.gseRx));
            g4i.gseRxSize = temp.GetLength();
            G4sysCtrl.g4i.Write(g4i);
            m_uiCmdRequest = false;
            changed = true;
        }

        // perform hardware timing for the H/W devices (i.e., write/erase timing for the FLASH)
        if ( G4sysCtrl.timerDF > 0)
        {
            --G4sysCtrl.timerDF;
        }

        // check for an interrupt request
        int irq = m_interruptRequest.GetCurSel();
        if (irq != 0 && m_interruptRequest.GetDroppedState() == FALSE)
        {
            if (G4sysCtrl.irqRqst == G4sysCtrl.irqHandled)
            {
                G4sysCtrl.irqId = irq;
                m_interruptRequest.SetCurSel(0);
                m_IRQsent.Format(" Sent IRQ: %s", IRQ_Names[irq]);
                ++G4sysCtrl.irqRqst;
                changed = true;
            }
        }

        m_fpga->Process();
        m_dio->ProcessDio(m_dinValues, m_doutValues);

        if (dinZ != m_dinValues || doutZ != m_doutValues)
        {
            UINT32 curSel = m_din.GetCurSel();
            m_dinSts = BIT( m_dinValues, curSel);
            OnCbnSelchangeDout();

            // check the lamp status
            m_lnk.SetCheck(m_dpr->msAlive?1:0);
            m_dat.SetCheck(m_dpr->msWrite?1:0);
            m_cfg.SetCheck(BIT(m_doutValues,LED_CFG));
            m_xfr.SetCheck(BIT(m_doutValues,LED_XFR));
            m_sts.SetCheck(BIT(m_doutValues,LED_STS));
            m_cfm.SetCheck(0);
            m_flt.SetCheck(BIT(m_doutValues,LED_FLT));

            m_lss0.SetCheck(BIT(m_doutValues,LSS0));
            m_lss1.SetCheck(BIT(m_doutValues,LSS1));
            m_lss2.SetCheck(BIT(m_doutValues,LSS2));
            m_lss3.SetCheck(BIT(m_doutValues,LSS3));
        }

        // process the DPRAM if we did anything we may need to interrupt the CP
        if (m_dpr->ProcessDPR())
        {
            // force the issue even if the G4E has not done the other interrupt
            G4sysCtrl.irqId = 2; // sorry for the magic# but 2 is the DPRAM IRQ
            m_IRQsent.Format(" Sent IRQ: %s", IRQ_Names[irq]);
            ++G4sysCtrl.irqRqst;
        }

        if (G4sysCtrl.shutDownComplete && !m_shutDownStarted)
        {
            m_shutDownStarted = true;
            PostMessageA(WM_CLOSE);
        }
    }

    return changed;
}

//--------------------------------------------------------------------------------------------------
// Update the GSE TX display window 
void CG4EDlg::UpdateGseTxDisplay( CString& lineBuffer)
{
    if ( !lineBuffer.IsEmpty())
    {
        m_gseTx += lineBuffer;

        m_gseTxDisplay += lineBuffer;
        m_gseTxDisplay = LastNlines( m_gseTxDisplay, WRAP);

        m_logFile.Write( (LPCTSTR)lineBuffer, lineBuffer.GetLength());
        UpdateData( FALSE);
    }
}

//--------------------------------------------------------------------------------------------------
// Recursively walk up a string from the end find the last 'N' lines.  N is specified on the 
// initial call and counted down.
CString CG4EDlg::LastNlines(CString inStr, int N)
{
    if (inStr.IsEmpty())
    {
        return inStr;
    }
    else if (N == 0)
    {
        CString empty;
        return empty;
    }
    else
    {
        int at = inStr.ReverseFind('\r');
        if (at != -1)
        {
            CString front;
            CString endLine;

            int len = inStr.GetLength();
            endLine = inStr.Right(len - at);
            front = LastNlines( inStr.Left( at), N-1);
            return front + endLine;
        }
        else
        {
            return inStr;
        }
    }
}

//--------------------------------------------------------------------------------------------------
// Load a Cfg file to the unit
void CG4EDlg::OnBnClickedLdCfg()
{
    CFileDialog open( TRUE, _T("cfg"));

    if ( open.DoModal() == IDOK)
    {
        m_cfgFilename = open.GetPathName();
    }
}

//--------------------------------------------------------------------------------------------------
void CG4EDlg::OnClose()
{
    ShutItDown();
    CDialog::OnClose();
}

//--------------------------------------------------------------------------------------------------
void CG4EDlg::ShutItDown()
{
    int timeout = 0;
    G4sysCtrl.runG4thread = FALSE;
    m_shutDownStarted = true;
    while (!G4sysCtrl.shutDownComplete && timeout < 3) // wait up to 3 sec for the G4E to end
    {
        // G4E will turn it back on when it's done
        ProcessIocInterface();
        ProcessG4eInterface();
        Sleep(1000);
        ++timeout;
    }

    m_logFile.Close();
}

//--------------------------------------------------------------------------------------------------
void CG4EDlg::OnBnClickedSend()
{
    // ask to send the GSE Rx data down to the G4E
    m_uiCmdRequest = true;
}

//--------------------------------------------------------------------------------------------------
void CG4EDlg::OnBnClickedClear()
{
    CString dummy;
    m_gseTx = "";         // clear the full buffer
    m_gseTxDisplay = "";  // clear the display buffer

    UpdateGseTxDisplay( dummy);
    UpdateData( FALSE);
}

//--------------------------------------------------------------------------------------------------
void CG4EDlg::OnBnClickedDinSet()
{
    UpdateData();
    UINT32 curSel = m_din.GetCurSel();
    if ( BIT(m_dinValues,curSel))
    {
        m_dinValues &= ~(1 << curSel);
    }
    else
    {
        m_dinValues |= FIELD( 1, curSel, 1);
    }
}

//--------------------------------------------------------------------------------------------------
void CG4EDlg::OnCbnSelchangeDin()
{
    UINT32 curSel = m_din.GetCurSel();
    m_dinSts = BIT( m_dinValues, curSel);
    UpdateData( FALSE);
}

//--------------------------------------------------------------------------------------------------
void CG4EDlg::OnCbnSelchangeDout()
{
    UINT32 curSel = m_dout.GetCurSel();
    m_doutSts = BIT( m_doutValues, curSel);
    UpdateData( FALSE);
}

void CG4EDlg::OnBnClickedEsc()
{
    m_gseRx = "\033";

    // ask to send the GSE Rx data down to the G4E
    m_uiCmdRequest = true;
    UpdateData( FALSE);
}

void CG4EDlg::OnBnClickedBusbat()
{
  BusBat dlg(m_busV, m_batV);
  dlg.DoModal();

  m_busV = dlg.m_bus;
  m_batV = dlg.m_bat;
}

