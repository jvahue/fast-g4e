#include "stdafx.h"

#include "HW_Emul.h"
#include "DPRAMemul.h"

extern "C"
{
#include "alt_basic.h"
#include "ClockMgr.h"
#include "Utility.h"

//CP transmit to MS buffer definitions
#define DPRAM_CPtoMS_Buf          ((INT8*)(DPRAM_CP_TO_MS_BUF_ADDR))
#define DPRAM_CPtoMS_Int          (*(volatile UINT16*)DPRAM_CP_TO_MS_INT_FLAG)
#define DPRAM_CPtoMS_Own          (*(volatile UINT16*)DPRAM_CP_TO_MS_OWN_FLAG)
#define DPRAM_CPtoMS_Cnt          (*(volatile UINT16*)DPRAM_CP_TO_MS_COUNT)

//MS transmit to CP buffer definitions
#define DPRAM_MStoCP_Buf          ((INT8*)(DPRAM_MS_TO_CP_BUF_ADDR))
#define DPRAM_MStoCP_Int          (*(volatile UINT16*)DPRAM_MS_TO_CP_INT_FLAG)
#define DPRAM_MStoCP_Own          (*(volatile UINT16*)DPRAM_MS_TO_CP_OWN_FLAG)
#define DPRAM_MStoCP_Cnt          (*(volatile UINT16*)DPRAM_MS_TO_CP_COUNT)

#define DPRAM_INT_PATTERN         0xA5A5

};

// zero all the DPR memory
DPRamEmulation::DPRamEmulation()
    : m_file(0)
    , msAlive(false)
    , msWrite(false)
{
    Reset();
}

void DPRamEmulation::Reset()
{
    memset(hw_DPRAM, 0, sizeof(hw_DPRAM));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Act like the MS return true if you want to interrupt the CP
bool DPRamEmulation::ProcessDPR()
{
    const int msStartUpSeconds = 5;

    bool status = false;
    static time_t start = 0;

    // see if the CP wants us to do anything - check the CP2MS Int location
    if ( DPRAM_INT_PATTERN == DPRAM_CPtoMS_Int)
    {
        DPRAM_CPtoMS_Int = 0;             // reset the interrupt (the emulation way)

        // start the clock the first time we see a request
        if (start == 0)
        {
            start = clock();
        }
        else if ( clock() - start > (msStartUpSeconds*1000))
        {
            msAlive = true;
            // delay 0:40s before returning the HB
            // determine what the message is
            memcpy( &m_msgIn, DPRAM_CPtoMS_Buf, DPRAM_CPtoMS_Cnt);
            PrepResp();

            if ( m_msgIn.id != CMD_ID_HEARTBEAT)
            {
                TRACE( "CP2MS Cmd: %d\n", m_msgIn.id);
            }

            // process the message
            switch ( m_msgIn.id)
            {
            case CMD_ID_HEARTBEAT:
                status = HbResponse();
                break;
            case CMD_ID_START_LOG_DATA:
                status = StartLog();
                break;
            case CMD_ID_LOG_DATA:
                status = LogData();
                break;
            case CMD_ID_END_LOG_DATA:
                status = EndLog();
                break;
            case CMD_ID_REMOVE_LOG_FILE:
                status = RemoveLogFile();
                break;
            case CMD_ID_CHECK_FILE:
                status = CheckFile();
                break;
            case CMD_ID_SETUP_GSM:
                status = GsmSetup();
                break;
            case CMD_ID_SHELL_CMD:
                status = MsShellCmd();
                break;
            default:
                status = true;
                break;
            }

            if ( status)
            {
                DPRAM_CPtoMS_Own = DPRAM_TX_OWNS; // give the buffer back to the 
                DPRAM_CPtoMS_Cnt = 0;
            }
        }
    }
    else
    {
        // do we have anything to send to the CP ?
    }
            

    return status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DPRamEmulation::PrepResp()
{
    m_msgOut.id = m_msgIn.id;
    m_msgOut.sequence = m_msgIn.sequence;
    m_msgOut.type = MSCP_PACKET_RESPONSE;
    m_msgOut.status = MSCP_RSP_STATUS_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool DPRamEmulation::MsShellCmd()
{
    MSCP_CP_SHELL_CMD* cmd = (MSCP_CP_SHELL_CMD*)m_msgIn.data;
    MSCP_MS_SHELL_RSP rsp; 
    
    if (strstr( (char*)cmd->Str, "ls") != NULL)
    {
        strcpy( (CHAR*)rsp.Str, "-rw-r--r--    1 root     root          338 Wed Jun  2 22:47:57 2010 FAST-SYS-000001-700608091.dtu.bz2.bfe.corrupt\r-rw-r--r--    1 root     root          338 Wed Jun  2 22:47:15 2010 FAST-SYS-000001-700608045.dtu.bz2.bfe\r");
    }
    else
    {
        strcpy( (CHAR*)rsp.Str, "#");
    }

    rsp.Len = strlen( (CHAR*)rsp.Str);
    
    return Send( &rsp, sizeof(rsp));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool DPRamEmulation::HbResponse()
{
    MSCP_MS_HEARTBEAT_RSP rsp;
    TIMESTRUCT Time;

    CM_GetRTCClock(&Time);

    rsp.Time.Year        = Time.Year;
    rsp.Time.Month       = Time.Month;
    rsp.Time.Day         = Time.Day;
    rsp.Time.Hour        = Time.Hour;
    rsp.Time.Minute      = Time.Minute;
    rsp.Time.Second      = Time.Second;
    rsp.Time.MilliSecond = Time.MilliSecond;

    rsp.ClientConnected     = TRUE;
    rsp.CompactFlashMounted = TRUE;
    
    rsp.TimeSynched = FALSE;
    rsp.Sta         = MSCP_STS_OFF;
    rsp.Xfr         = MSCP_XFR_NONE;

    return Send( &rsp, sizeof(rsp));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool DPRamEmulation::StartLog()
{
    MSCP_LOGFILE_INFO fileName;
    UINT32 handle = 42;
    CString name;

    memcpy( fileName.FileName, m_msgIn.data, m_msgIn.nDataSize);
    TRACE("StartLog: %s\n", fileName.FileName);

    // cover a BUG in CP code - is is a bug could it have opened the file if it is not running?
    // how did we do it?  Is the DPRAM cmd still present when we come up?
    // seems we are getting a double call to open the file??
    if (m_file.m_hFile != CFile::hFileNull)
    {
      m_file.Close();
    }

    name.Format("%s", fileName.FileName);
    // put all the files in the persist dir
    m_file.Init( name, 0, false);

    m_seq = 1;

    msWrite = true;

    return Send( &handle, sizeof(handle));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool DPRamEmulation::LogData()
{
    UINT16 result = 0;

    typedef struct  {
        MSCP_FILE_PACKET_INFO FileInfo;
        INT8 FileDataBuf[MSCP_CMDRSP_MAX_DATA_SIZE];
    } DataBuf;

    DataBuf* data = (DataBuf*)m_msgIn.data;

    if ( data->FileInfo.seq != m_seq)
    {
        TRACE("*** Error Log File Sequence: Got %d expected %d\n", data->FileInfo.seq, m_seq);
        m_seq = data->FileInfo.seq;
        result = 1;
    }
    else
    {
        TRACE("Log File Sequence: Got %d expected %d\n", data->FileInfo.seq, m_seq);
        m_file.Write( data->FileDataBuf, m_msgIn.nDataSize - sizeof(MSCP_FILE_PACKET_INFO));
        ++m_seq;
    }

    // toggle this while we write the data
    msWrite = msWrite ? false : true;

    return Send( &result, sizeof(result));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool DPRamEmulation::EndLog()
{
    UINT16 result = 0;
    if (m_file.m_hFile != CFile::hFileNull)
    {
        m_file.Close();
    }
    TRACE("*** Close Log(%d)\n", m_seq);
    msWrite = false;
    return Send( &result, sizeof(result));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool DPRamEmulation::RemoveLogFile()
{
    CString name;
    MSCP_LOGFILE_INFO fileName;
    MSCP_REMOVE_FILE_RSP data;

    // BUG: should delete the file
    memcpy( fileName.FileName, m_msgIn.data, m_msgIn.nDataSize);
    name.Format("%s", fileName.FileName);

    data.Result = MSCP_REMOVE_FILE_OK;
    return Send( &data, sizeof(data));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool DPRamEmulation::CheckFile()
{
    MSCP_CHECK_FILE_CMD filename;
    MSCP_CHECK_FILE_RSP rsp;

    // BUG: should look up and see if the file is there
    memcpy( filename.FN, m_msgIn.data, m_msgIn.nDataSize);

    rsp.FileSta = MSCP_FILE_STA_MSVALID;
    return Send( &rsp, sizeof(rsp));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool DPRamEmulation::GsmSetup()
{
    memcpy( &gsmSetup, m_msgIn.data, m_msgIn.nDataSize);
    return Send( NULL, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool DPRamEmulation::Send( void* data, UINT16 size)
{
    bool status = false;
    UINT32 TotalSize;

    // make sure we have the buffer
    if (DPRAM_RX_OWNS == DPRAM_MStoCP_Own)
    {
        Sleep(1000);
    }

    if (DPRAM_RX_OWNS != DPRAM_MStoCP_Own)
    {
        //Fill in packet fields Id, Size and Sequence.
        TotalSize = size + (sizeof(m_msgOut) - sizeof(m_msgOut.data));

        if ( size > 0)
        {
            memcpy( &m_msgOut.data, data, size);
        }

        m_msgOut.nDataSize = size;
        m_msgOut.checksum = ChecksumBuffer(&m_msgOut.type,
                                           TotalSize-sizeof(m_msgOut.checksum),
                                           0xFFFFFFFF);

        memcpy( DPRAM_MStoCP_Buf, &m_msgOut, sizeof( m_msgOut));
        DPRAM_MStoCP_Cnt = sizeof( m_msgOut);
        DPRAM_MStoCP_Own = DPRAM_RX_OWNS;
        status = true;
    }
    return status;
}

