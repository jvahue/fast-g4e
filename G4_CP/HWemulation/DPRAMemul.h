#pragma once

#include "PersistFile.h"

extern "C"
{
#include "DPRAM.h"
#include "MSCPInterface.h"
};

class DPRamEmulation
{
public:
    DPRamEmulation();
    void Reset();
    bool ProcessDPR();
    bool msAlive;
    bool msWrite;

protected:
    MSCP_CMDRSP_PACKET m_msgIn;
    MSCP_CMDRSP_PACKET m_msgOut;
    PersistFile m_file;
    int m_seq;

    void PrepResp();
    bool Send( void* data, UINT16 size);

    // specific message handlers
    bool HbResponse();
    bool RemoveLogFile();
    bool StartLog();
    bool LogData();
    bool EndLog();
    bool CheckFile();
    bool GsmSetup();
    bool MsShellCmd();
    bool GetInfo();
    MSCP_GSM_CONFIG_CMD gsmSetup;
};

