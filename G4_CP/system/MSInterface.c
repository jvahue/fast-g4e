#define SYS_MSINTERFACE_BODY
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

  File:          MSInterface.c

  Description:   Communication interface to the Micro-Server by the dual-port


  Requires:      ResultCodes.h
                 DPRAM.h
                 TaskManager.h

  VERSION
      $Revision: 65 $  $Date: 12-11-12 10:14a $

******************************************************************************/
/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "mcf548x.h"
#include <string.h>


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "alt_basic.h"
#include "Utility.h"
#include "ResultCodes.h"
#include "DPRAM.h"
#include "TaskManager.h"
#include "MSInterface.h"
#include "GSE.h"  //To output debug strings to the GSE, remove after debug
#include "FaultMgr.h"
#include "ClockMgr.h"
#include "MSStsCtl.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct  {
  UINT16           Id;
  UINT32           Seq;
  MSI_RSP_CALLBACK RspHandler;
  INT32            Timeout;
}MSI_PENDING_RSP;

typedef struct {
  UINT16           Id;
  MSI_CMD_CALLBACK CmdHandler;
} MSI_CMD_HANDLER;


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static MSI_PENDING_RSP MSI_PendingMSResponseList[MSI_MAX_PENDING_RSP];
static MSI_CMD_HANDLER MSI_MSCmdHandlerList[MSI_NUM_OF_CMD_HANDLERS];
static UINT32 MSI_CmdSeqNum;
static BOOLEAN DpramFailLogged;
static BOOLEAN m_MSIntSvcTimeout;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
BOOLEAN MSI_AddPendingRsp(UINT16 Id, UINT32 Seq, MSI_RSP_CALLBACK RspHandler, INT32 TOmS);
MSI_RSP_CALLBACK  MSI_FindAndRemovePendingRsp(UINT16 Id, UINT32 Seq);
void              MSI_MSPacketTask(void* TaskParam);
RESULT            MSI_ValidatePacket(const MSCP_CMDRSP_PACKET* Packet, UINT32 SizeRead);
void              MSI_HandleMSResponse(MSCP_CMDRSP_PACKET* Packet);
void              MSI_HandleMSCommand(MSCP_CMDRSP_PACKET* Packet);
void              MSI_CheckMsgTimeouts(void);
void              MSI_CheckDPRAM(void);
void              MSI_MSCommandFailed(MSCP_CMDRSP_PACKET* Packet);
void              dbgstr(INT8* str);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    MSI_Init
 *
 * Description: Initial setup for the micro-server interface module. Clear and
 *              initialize the response and command handler lists.  Create a
 *              micro server interface task to process packets from the
 *              micro server.
 *
 * Parameters:  none
 *
 * Returns:     none
 * Notes:       none
 *
 *****************************************************************************/
void MSI_Init(void)
{
  UINT32 i;
  TCB taskBlock;

  //Initialize global vars
  memset(MSI_PendingMSResponseList,0,sizeof(MSI_PendingMSResponseList));
  memset(MSI_MSCmdHandlerList,0,sizeof(MSI_MSCmdHandlerList));
  DpramFailLogged = FALSE;
  m_MSIntSvcTimeout = FALSE;

  MSI_CmdSeqNum = 0;
  for(i = 0; i < MSI_MAX_PENDING_RSP; i++)
  {
    MSI_PendingMSResponseList[i].Id = (UINT16)CMD_ID_NONE;
  }
  for(i = 0 ; i < MSI_NUM_OF_CMD_HANDLERS ; i++)
  {
    MSI_MSCmdHandlerList[i].Id = (UINT16)CMD_ID_NONE;
  }

  // Create the micro-server packet handler task
  memset(&taskBlock, 0, sizeof(taskBlock));
  strncpy_safe(taskBlock.Name,sizeof(taskBlock.Name),"MSI Packet Handler",_TRUNCATE);
  taskBlock.TaskID         = (TASK_INDEX)MSI_Packet_Handler;
  taskBlock.Function       = MSI_MSPacketTask;
  taskBlock.Priority       = taskInfo[MSI_Packet_Handler].priority;
  taskBlock.Type           = taskInfo[MSI_Packet_Handler].taskType;
  taskBlock.modes          = taskInfo[MSI_Packet_Handler].modes;
  taskBlock.MIFrames       = taskInfo[MSI_Packet_Handler].MIFframes;
  taskBlock.Rmt.InitialMif = taskInfo[MSI_Packet_Handler].InitialMif;
  taskBlock.Rmt.MifRate    = taskInfo[MSI_Packet_Handler].MIFrate;
  taskBlock.Enabled        = TRUE;
  taskBlock.Locked         = FALSE;
  taskBlock.pParamBlock    = NULL;
  TmTaskCreate (&taskBlock);

}



/******************************************************************************
 * Function:    MSI_PutCommand
 *
 * Description: Calls 'Ex function with NoCheck parameter false.
 *
 * Parameters:  [in]  Id      : Command Id
 *              [in]  data*   : Pointer to the data buffer to transmit
 *              [in]  size    : Size, in bytes, of the data
 *              [in]  TO      : Message timeout in milliseconds.  If a
 *                              response is not returned by the micro-server,
 *                              within this timeout, a "timeout" status is
 *                              sent to the response handler.
 *                              A value of -1 will disable
 *              [in]  handler*: Pointer to a response handler for the
 *                              micro-server response.  If this parameter
 *                              is null, the response packet is discarded
 *                              when received.
 *              [in]  NoCheck  : Allow command to be sent, even if the
 *                               MSSIM is not alive
 *
 * Returns:     SYS_OK: Message successfully sent to the queue
 *              SYS_SEND_BUF_FULL: Send buffer is full, try later
 *              else See ResultCodes.h
 * Notes:      The RspHandler callback is executed in the context of the MS
 *             Interface task, and therefore needs to be coded to execute
 *             quickly so the MS Interface task does not overrun its timeslot.
 *
 *             Ex function created to allow certian commands through w/o
 *             heartbeat, and maintain call signature other PutCommand
 *             callers.
 *
 *****************************************************************************/
RESULT MSI_PutCommand(UINT16 Id,const void* data,UINT32 size,INT32 TOmS,
                      MSI_RSP_CALLBACK RspHandler)
{
  return MSI_PutCommandEx(Id,data,size,TOmS,RspHandler,FALSE);
}



/******************************************************************************
 * Function:    MSI_PutCommandEx
 *
 * Description: Queues a command message for transmission to
 *              the micro server.  Once the micro
 *              server processes the command it should issue a response.  The
 *              response is passed to the function pointed to by RspHandler
 *
 *              Multiple tasks may be writing to the micro-server so
 *              mutual exclusion is necessary.
 *
 * Parameters:  [in]  Id      : Command Id
 *              [in]  data*   : Pointer to the data buffer to transmit
 *              [in]  size    : Size, in bytes, of the data
 *              [in]  TO      : Message timeout in milliseconds.  If a
 *                              response is not returned by the micro-server,
 *                              within this timeout, a "timeout" status is
 *                              sent to the response handler.
 *                              A value of -1 will disable
 *              [in]  handler*: Pointer to a response handler for the
 *                              micro-server response.  If this parameter
 *                              is null, the response packet is discarded
 *                              when received.
 *              [in]  NoCheck  : Allow command to be sent, even if the
 *                               MSSIM is not alive
 *
 * Returns:     SYS_OK: Message successfully sent to the queue
 *              SYS_SEND_BUF_FULL: Send buffer is full, try later
 *              else See ResultCodes.h
 * Notes:      The RspHandler callback is executed in the context of the MS
 *             Interface task, and therefore needs to be coded to execute
 *             quickly so the MS Interface task does not overrun its timeslot.
 *             Ex function created to allow certian commands through w/o
 *             heartbeat, and maintain call signature other PutCommand
 *             callers.
 *
 *****************************************************************************/
RESULT MSI_PutCommandEx(UINT16 Id,const void* data,UINT32 size,INT32 TOmS,
                      MSI_RSP_CALLBACK RspHandler, BOOLEAN NoCheck)
{
  CHAR resultStr[RESULTCODES_MAX_STR_LEN];
  MSCP_CMDRSP_PACKET* packet;
  DPRAM_WRITE_BLOCK dpramBlock;
  RESULT result;
  UINT32 thisCmdSeqNum;
  INT32 is;

  // Test Point for failing MSI_PutCommand
  TPS1(Id, eTpMsiPutCmd);

  //Reject commands except for the heartbeat until the MS is declared "alive"
  //Reject all commands if the MS is not servicing the DPRAM interrupt, this
  //prevent commands from queuing up while the MS is not listening.
  if ( ((MSSC_GetIsAlive() == TRUE) || NoCheck) &&
        !m_MSIntSvcTimeout )
  {
    //Create local copy of command sequence number for reentrancy
    is = __DIR();
    thisCmdSeqNum = MSI_CmdSeqNum++;
    __RIR(is);

    //Point dpram data block to the structure with the packet format.
    packet = (void*) dpramBlock.data;

    //Determine total size of the data to be written to DPRAM
    dpramBlock.size       = size + sizeof(*packet) - sizeof(packet->data);

    //Build the MS-CP packet
    packet->id             = Id;
    packet->nDataSize      = (UINT16)size;
    packet->UmRspSource    = (UINT8)MSCP_MS;
    packet->type           = (UINT16)MSCP_PACKET_COMMAND;
    packet->sequence       = thisCmdSeqNum;
    packet->status         = (UINT16)MSCP_RSP_STATUS_DATA;

    //Copy data into the packet data field
    memcpy(packet->data,data,size);

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
    packet->checksum = ChecksumBuffer(&packet->type,
                                      dpramBlock.size-sizeof(packet->checksum),
                                      0xFFFFFFFF);
#pragma ghs endnowarning

    // default to full and then determine status
    result = SYS_MSI_SEND_BUF_FULL;

    //Add pending response and put data in the queue.  If DPRAM call fails,
    //remove pending response from the queue.
    if(MSI_AddPendingRsp(Id,thisCmdSeqNum,RspHandler,TOmS))
    {
      result = DPRAM_WriteBlock(&dpramBlock);
      if(result != DRV_OK)
      {
        /*Return value unchecked. Reason:
          If unable to write block to DPRAM, attempt to remove from Pending
          Rsp Table.  If the entry is not in the table for some reason
          (ret == FALSE), this is okay because it is intended that it
          should be removed anyway. */
        MSI_FindAndRemovePendingRsp(Id,thisCmdSeqNum);
      }
    }

    GSE_DebugStr(VERBOSE,TRUE,"MSI: Sent cmd %d seq %d result %s",Id,
        thisCmdSeqNum, RcGetResultCodeString(result,resultStr));
  }
  else
  {
   //GSE_DebugStr(VERBOSE,TRUE,"MSI: Ignore cmd %d seq %d",Id, MSI_CmdSeqNum);
   result = SYS_MSI_INTERFACE_NOT_READY;
  }

  return result;
}



/******************************************************************************
 * Function:    MSI_PutResponse
 *
 * Description: Queues a response packet for transmission to
 *              the micro server.  The message will be transmitted once
 *              the interface is ready for transmission.
 *
 * Parameters:  [in]  Id:     Packet ID enumeration
 *              [in]  data*:  Pointer to the data buffer to transmit
 *              [in]  status: Response status(MSCP_RSP_STATUS_FAIL,
 *                                            MSCP_RSP_STATUS_SUCCESS,
 *                                            MSCP_RSP_STATUS_BUSY)
 *              [in]  size:   Size, in bytes, of the data
 *              [in]  seq:    Dual purpose depending if the message is
 *                              a command or response.
 *                              Command: SendMessage will assign a sequence
 *                                       number to the command, the caller
 *                                       will use this sequence id to retrieve
 *                                       the response from GetResponse
 *
 *
 *
 * Returns:     SYS_OK: Response added to message queue successfully
 *              MSI_SEND_BUF_FULL: Cannot send command now, send buffer is
 *                                 full, try again later.
 *              else:   See ResultCodes.h
 * Notes:
 *
 *****************************************************************************/
RESULT MSI_PutResponse(UINT16 Id, const void* data, UINT16 status, UINT32 size, UINT32 seq)
{
  MSCP_CMDRSP_PACKET* packet;
  DPRAM_WRITE_BLOCK dpramBlock;
  RESULT result;

  //Point dpram data block to the structure with the packet format.
  packet = (void*) dpramBlock.data;

  //Fill in packet fields Id, Size and Sequence.
  dpramBlock.size       = size + sizeof(*packet) - sizeof(packet->data);

  result = SYS_MSI_SEND_BUF_FULL;

  //Verify free space before adding the packet to the buffer
  //or pending response list.
  if( DPRAM_WriteFreeCnt() >= (UINT32) dpramBlock.size)
  {
    packet->id          = Id;
    packet->nDataSize   = (UINT16)size;
    packet->UmRspSource = (UINT8)MSCP_MS;
    packet->type        = (UINT16)MSCP_PACKET_RESPONSE;
    packet->sequence    = seq;
    packet->status      = status;

    //Copy data into the packet
    memcpy(packet->data,data,size);
#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
    packet->checksum = ChecksumBuffer(&packet->type,
                                      dpramBlock.size-sizeof(packet->checksum),
                                      0xFFFFFFFF);
#pragma ghs endnowarning

    result = DPRAM_WriteBlock(&dpramBlock);

  }

  return result;
}



/******************************************************************************
 * Function:    MSI_AddCmdHandler
 *
 * Description: Add a callback handler for command type messages from the
 *              micro server.  Command messages from the micro server typically
 *              happen asynchronously to any control processor process, so the
 *              handlers should be designed to handle or buffer messages at
 *              any time.
 *
 * Parameters:  [in] Id: Packet Id of the command to handle
 *              [in] CmdHandler: Pointer to a function that will handle the
 *                               command
 *
 * Returns:     SYS_OK: Command handler added successfully
 *              SYS_MSI_CMD_HANDLER_NOT_ADDED: Command handler could not be
 *                                             added, it is likely because
 *                                             the command list is full
 *
 * Notes:       The callback is executed in the context of the MS Interface
 *              task, and therefore needs to be coded to execute quickly so
 *              the MS Interface task does not overrun its timeslot.
 *
 *****************************************************************************/
RESULT MSI_AddCmdHandler(UINT16 Id, MSI_CMD_CALLBACK CmdHandler)
{
  UINT32 i;
  RESULT result = SYS_MSI_CMD_HANDLER_NOT_ADDED;

  for(i = 0; i < MSI_NUM_OF_CMD_HANDLERS; i++)
  {
    if ( MSI_MSCmdHandlerList[i].Id == CMD_ID_NONE )
    {
      MSI_MSCmdHandlerList[i].Id         = Id;
      MSI_MSCmdHandlerList[i].CmdHandler = CmdHandler;
      result = SYS_OK;
      break;
    }
  }
  return result;
}



/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    MSI_MSPacketTask
 *
 * Description: Task; Checks the Rx queue for responses from the micro server,
 *              verifies the checksum and size, then calls HandleRsp or
 *              HandleCmd to further process the packet.
 *
 *              This is the only consumer task for DPRAM_Read.
 *
 * Parameters:  void *TaskParam
 *
 * Returns:     none
 * Notes:       none
 *
 *****************************************************************************/
void MSI_MSPacketTask(void* TaskParam)
{
  UINT32 SizeRead;
  CHAR  ResultStr[RESULTCODES_MAX_STR_LEN];
  MSCP_CMDRSP_PACKET Packet;
  RESULT DPRAMReadResult;
  RESULT ValidatePacketResult;

  // Check for DPRAM Failure
  if (!DpramFailLogged)
  {
    // Check for PBIT Fail
    if ( DPRAM_InitStatus() )
    {
      SYS_APP_ID appId = DRV_ID_DPRAM_PBIT_RAM_TEST_FAIL;
      if (DRV_DPRAM_PBIT_REG_INIT_FAIL == DPRAM_InitStatus())
      {
        appId = DRV_ID_DPRAM_PBIT_REG_INIT_FAIL;
      }
      // Set the sys condition to the configured PBIT value for the MS
      Flt_SetStatus(MSSC_GetConfigSysCond(MSSC_PBIT_SYSCOND), appId, NULL, 0);

      DpramFailLogged = TRUE;

      GSE_DebugStr(NORMAL, TRUE, "\r\nMSI_MSPacketTask: DPRAM Failure: %0x04x.\r\n", appId);
    }
    // check for excessive DPRAM interrupt fail
    else if ( m_DPRAM_Status.intrFail )
    {
      DPRAM_INTR_FAIL_LOG intrLog;

      // log error and set system condition
      intrLog.minIntrTime = m_DPRAM_Status.minIntrTime;
      intrLog.maxExeTime  = m_DPRAM_Status.maxExeTime;

      // Set the sys condition to the configured CBIT value for the MS
      Flt_SetStatus(MSSC_GetConfigSysCond(MSSC_CBIT_SYSCOND),
                    DRV_ID_DPRAM_INTERRUPT_FAIL, &intrLog,
                    sizeof(DPRAM_INTR_FAIL_LOG));

      DpramFailLogged = TRUE;

      GSE_DebugStr(NORMAL, TRUE,
        "\r\nMSI_MSPacketTask: Interrupt Failure (%d uSec). Interrupts Disabled\r\n",
        intrLog.minIntrTime);
    }
  }

  //Check the pending response list for message timeouts.
  MSI_CheckMsgTimeouts();

  //Check for "stuck" DPRAM condition
  MSI_CheckDPRAM();

  //Read a packet from the read queue, ensure driver returns an okay code
  DPRAMReadResult = DPRAM_ReadBlock((INT8*)&Packet, sizeof(Packet), &SizeRead);

  if( (DPRAMReadResult == DRV_OK) && (SizeRead != 0) )
  {
    ValidatePacketResult = MSI_ValidatePacket(&Packet,SizeRead);
    if( ValidatePacketResult == SYS_OK)
    {
      //Valid packet received.
      switch (Packet.type)
      {
        case MSCP_PACKET_RESPONSE:
          MSI_HandleMSResponse(&Packet);
          break;

        case MSCP_PACKET_COMMAND:
          MSI_HandleMSCommand(&Packet);
          break;

        default:
          FATAL ("Unexpected MSCP_PACKET_TYPE = %d", Packet.type);
          break;
      }
    }
    else
    {
     GSE_DebugStr(NORMAL,TRUE,"MSI: ValidatePacket error %s",
         RcGetResultCodeString(ValidatePacketResult,ResultStr));
    }
  }
}



/******************************************************************************
 * Function:    MSI_HandleMSResponse
 *
 * Description: Routine handles response type packets from the micro-server.
 *              First, checks if the response is expected.  If the response
 *              is expected (i.e. it is found in the table of expected
 *              responses) the response handler callback is executed.
 *              Response handler callback is responsible for validating
 *              the data and length for the specific type of response returned.
 *
 * Parameters:  [in/out] Packet: Packet received from the micro-server.
 *                               The data portion is handed to the response
 *                               callback.  Because it is not declared const,
 *                               the callback may modify the data section.
 *
 * Returns:
 *
 * Notes:      The callback is executed in the context of the MS Interface
 *             task, and therefore needs to be coded to execute quickly so
 *             the MS Interface task does not overrun its timeslot.
 *
 *****************************************************************************/
void MSI_HandleMSResponse(MSCP_CMDRSP_PACKET* Packet)
{
  MSI_RSP_CALLBACK RspFunc;
  MSI_RSP_STATUS RspStatus;

  //Lookup response in the expected response table and get response handler
  RspFunc = MSI_FindAndRemovePendingRsp(Packet->id, Packet->sequence);
  if(RspFunc != NULL)
  {
    switch(Packet->status)
    {
      case MSCP_RSP_STATUS_SUCCESS:
        RspStatus = MSI_RSP_SUCCESS;
      break;

      case MSCP_RSP_STATUS_FAIL:
        RspStatus = MSI_RSP_FAILED;
      break;

      case MSCP_RSP_STATUS_BUSY:
        RspStatus = MSI_RSP_BUSY;
      break;

      default: //Default for unrecognized response statuses.
        RspStatus = MSI_RSP_FAILED;
      break;

    }

    RspFunc(Packet->id,
            Packet->data,
            Packet->nDataSize,
            RspStatus);
  }
  else //Response Id and Sequence do not match any expected responses, or no
  {    //callback was defined.
    GSE_DebugStr(VERBOSE,TRUE,
    "MSI: HandleMSResponse: No function to handle response id 0x%X seq %d",
    Packet->id, Packet->sequence);
  }
}


/******************************************************************************
 * Function:    MSI_HandlerMSCommand
 *
 * Description: Handles asynchronous commands originating from the micro-server
 *              and routes them to a command handler callback to process the
 *              command
 *
 *
 * Parameters: [in/out] Packet: Packet received from the microserver.  The data
 *                      portion is passed by reference to the command handler
 *                      that may modify the data portion.
 *
 * Returns:
 *
 * Notes:      The callback is executed in the context of the MS Interface
 *             task, and therefore needs to be coded to execute quickly so
 *             the MS Interface task does not overrun its time slot.
 *
 *****************************************************************************/
void MSI_HandleMSCommand(MSCP_CMDRSP_PACKET* Packet)
{
  UINT32 i = 0;

  //Search for matching command id
  while( (i < MSI_NUM_OF_CMD_HANDLERS) &&
         (MSI_MSCmdHandlerList[i].Id  != Packet->id ) )
  {
    i++;
  }

  if((i < MSI_NUM_OF_CMD_HANDLERS) &&
     (MSI_MSCmdHandlerList[i].Id == Packet->id) &&
     (MSI_MSCmdHandlerList[i].CmdHandler != NULL))
  {
    if( MSI_MSCmdHandlerList[i].CmdHandler( Packet->data,
                                            Packet->nDataSize,
                                            Packet->sequence ) == FALSE)
    {
      MSI_MSCommandFailed(Packet);
    }
  }
  else
  {
    MSI_MSCommandFailed(Packet);
  }

}



/******************************************************************************
 * Function:    MSI_CheckMsgTimeout
 *
 * Description: Checks the timeout values of pending responses from the micro-
 *              server.  If any expected responses timeout, the command is
 *              removed from the expected responses table and the callback
 *              function is called with a "TIMEOUT" status.
 *
 * Parameters:  none
 *
 * Returns:
 *
 * Notes:
 *
 *****************************************************************************/
void MSI_CheckMsgTimeouts(void)
{
  UINT32 i;
  UINT32 TimeElapsed;
  static UINT32 TimeLast;
  MSI_RSP_CALLBACK RspFunc;
  UINT16 Id;

  TimeElapsed = CM_GetTickCount() - TimeLast;

  //Check all possible pending response entries
  for( i = 0 ; i < MSI_MAX_PENDING_RSP ; i++)
  {
    //If a command is pending and it is using a timeout
    //value
    if( (CMD_ID_NONE != MSI_PendingMSResponseList[i].Id) &&
        (-1 != MSI_PendingMSResponseList[i].Timeout) )
    {
      MSI_PendingMSResponseList[i].Timeout -= TimeElapsed;
      //If the timeout has elapsed, remove it from the expected responses
      //and call the callback with a Timeout status.
      if(MSI_PendingMSResponseList[i].Timeout < 0)
      {
        Id = MSI_PendingMSResponseList[i].Id;
        RspFunc = MSI_FindAndRemovePendingRsp(Id,
                                             MSI_PendingMSResponseList[i].Seq);
        if(RspFunc != NULL)
        {
          RspFunc(Id,
                  NULL,
                  0,
                  MSI_RSP_TIMEOUT);
        }
      }
    }
  }

  TimeLast = CM_GetTickCount();
}


/******************************************************************************
 * Function:    MSI_CheckDPRAM
 *
 * Description: Sub-task of MSI_PacketTask that monitors the byte count of the
 *              DPRAM transmit queue to determine if the micro-server has
 *              missed an interrupt.  If this is determined, the interrupt
 *              is re-issued
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       This is a workaround for a CP<->MS interface issue.
 *
 *****************************************************************************/
void MSI_CheckDPRAM(void)
{
  static UINT32 TxCntLast;
  static UINT32 TimeElapsed;
  static UINT32 TimeLast;
  UINT32 TxCnt;
  UINT32 TimeNow;
  BOOLEAN MsgPending;

  TimeNow = CM_GetTickCount();

  TimeElapsed +=  TimeNow - TimeLast;

  DPRAM_GetTxInfo(&TxCnt,&MsgPending);

  //If a message is waiting and the message count is not changing, examine the timeout
  if( MsgPending && (TxCnt == TxCntLast))
  {
    if( TimeElapsed > MSI_MSG_IN_DPRAM_TIMEOUT_mS)
    {
      m_MSIntSvcTimeout = TRUE;
      DPRAM_KickMS();
      GSE_DebugStr(VERBOSE,TRUE,"MSI: Issued repeat interrupt to MS");
      TimeElapsed = 0;
    }
  }
  //else, everything is moving along, reset the timeout.
  else
  {
    TimeElapsed = 0;
    m_MSIntSvcTimeout = FALSE;
  }
  TxCntLast = TxCnt;
  TimeLast = TimeNow;
}


/******************************************************************************
 * Function:    MSI_MSCommandFailed
 *
 * Description: Called when a micro-server command packet cannot be processed
 *              either because the command handler callback returned an error,
 *              or because there is not command handler available to process
 *              the command.  An empty response packet is sent back to the
 *              micro-server with the status field set to FAIL.
 *
 * Parameters:  [in] Packet: Packet received from the micro-server that cannot
 *                           be processed
 *
 * Returns:
 *
 * Notes:
 *
 *****************************************************************************/
void MSI_MSCommandFailed(MSCP_CMDRSP_PACKET* Packet)
{
  if (MSI_PutResponse(Packet->id,
                      (void*)MSCP_RSP_STATUS_FAIL,
                      NULL,
                      0,
                      Packet->sequence ) != SYS_OK)
  {
    GSE_DebugStr(NORMAL,TRUE,"MS Command failed, could not return failed response");
  }
}



/******************************************************************************
 * Function:    MSI_FindAndRemovePendingRsp
 *
 * Description: When a response type packet is received from the micro-server
 *              this routine should be called to verify that the response
 *              Id and sequence was expected and to remove it from the list
 *              of expected responses.  A pointer to the registered response
 *              handler is returned.
 *
 * Parameters:  Id:  Id number of the response received
 *              Seq: Sequence number of the response received
 *
 * Returns:     != NULL: Pointer to the message response handler to call
 *              == NULL: Response Id and Sequence were not expected from the
 *                       micro server.
 * Notes:
 *
 *****************************************************************************/
MSI_RSP_CALLBACK MSI_FindAndRemovePendingRsp(UINT16 Id, UINT32 Seq)
{
  UINT32 i = 0;
  MSI_RSP_CALLBACK result = NULL;

  //Search for matching id and sequence
  while( (i < MSI_MAX_PENDING_RSP) &&
         (!((MSI_PendingMSResponseList[i].Id == Id) &&
            (MSI_PendingMSResponseList[i].Seq == Seq))) )
  {
    i++;
  }
  //If this is a match, return handler clear expected response
  if( (i < MSI_MAX_PENDING_RSP)                &&
      (MSI_PendingMSResponseList[i].Id  == Id) &&
      (MSI_PendingMSResponseList[i].Seq == Seq) )
  {
    result = MSI_PendingMSResponseList[i].RspHandler;
    MSI_PendingMSResponseList[i].Id = CMD_ID_NONE;
  }

  return result;
}



/******************************************************************************
 * Function:    MSI_AddPendingRsp
 *
 * Description: When sending a command to the micro-server, this routine
 *              should be called to add a Id and sequence number to the
 *              list of response packets expected from the micro server.
 *
 *
 * Parameters:  Id: Command id number of the expected response
 *              Seq: Sequence number expected in the response
 *              RspHandler: Ptr to Response Handler for the microserver response
 *              TOmS: TimeOut in mS
 *
 * Returns:     TRUE: Added to response list
 *              FALSE: Failed to add to response list
 *
 * Notes:       Caller must use CanSendCmd first to verify an open slot
 *              is available
 *
 *****************************************************************************/
BOOLEAN MSI_AddPendingRsp(UINT16 Id, UINT32 Seq, MSI_RSP_CALLBACK RspHandler,
                       INT32 TOmS)
{
  BOOLEAN result = FALSE;
  UINT32 i = 0;
  UINT32 intrLevel;


  intrLevel = __DIR();
  //Search for an open slot
  while( (i < MSI_MAX_PENDING_RSP) &&
         (MSI_PendingMSResponseList[i].Id != CMD_ID_NONE) )
  {
    i++;
  }

  //If this is an open slot, add the Id and Seq
  if((i < MSI_MAX_PENDING_RSP) &&
     (MSI_PendingMSResponseList[i].Id == CMD_ID_NONE) )
  {
    MSI_PendingMSResponseList[i].Id         = Id;
    MSI_PendingMSResponseList[i].Seq        = Seq;
    MSI_PendingMSResponseList[i].Timeout    = TOmS;
    MSI_PendingMSResponseList[i].RspHandler = RspHandler;
    result = TRUE;
  }
  __RIR(intrLevel);

  return result;
}



/******************************************************************************
 * Function:    MSI_ValidatePacket
 *
 * Description: Verifies the packet size reflected the SizeRead from the
 *              DPRAM buffer.  Verifies the checksum of the packet is correct
 *
 * Parameters:  [in] Packet:   Command/Response packet to validate
 *              [in] SizeRead: Number of bytes read from the DPRAM buffer
 *
 * Returns:     RESULT
 *
 * Notes:       None
 *
 *****************************************************************************/
RESULT MSI_ValidatePacket(const MSCP_CMDRSP_PACKET* Packet, UINT32 SizeRead)
{
  RESULT result;
  UINT32 ChksumSize;
  UINT32 chkSum;

  //Validate packet size as read from DPRAM
  if( (SizeRead >= sizeof(*Packet) - sizeof(Packet->data)) &&
      (SizeRead < DPRAM_SIZE) )
  {
    //Validate packet data field size (0 to MSI_CMDRSP_MAX_DATA_SIZE)
    if(Packet->nDataSize <= MSCP_CMDRSP_MAX_DATA_SIZE)
    {
      //Size valid, validate checksum
      //Number of bytes added to compute the checksum
      ChksumSize = Packet->nDataSize +
                   ( sizeof(*Packet) - sizeof(Packet->checksum)
                                     - sizeof(Packet->data) );
#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
      chkSum = ChecksumBuffer(&Packet->type, ChksumSize, 0xFFFFFFFF);
#pragma ghs endnowarning
      if( chkSum == Packet->checksum )
      {
        result = SYS_OK;
      }
      else
      {
        result = SYS_MSI_MS_PACKET_BAD_CHKSUM;
        GSE_DebugStr(NORMAL,TRUE,
                "MSI: CS fail, calculated: %x, received: %x, checksum size: %d",
                chkSum,
                Packet->checksum,
                ChksumSize);
      }
    }
    else
    {
      result = SYS_MSI_MS_PACKET_BAD_SIZE;
    }
  }
  else
  {
    result = SYS_MSI_MS_PACKET_BAD_SIZE;
  }

  return result;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: MSInterface.c $
 *
 * *****************  Version 65  *****************
 * User: John Omalley Date: 12-11-12   Time: 10:14a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 64  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 1:00p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 63  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 62  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 61  *****************
 * User: Jeff Vahue   Date: 11/03/10   Time: 7:03p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 60  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 7:50p
 * Updated in $/software/control processor/code/system
 * SCR# 975 - Code Coverage refactor
 *
 * *****************  Version 59  *****************
 * User: Jim Mood     Date: 11/01/10   Time: 4:06p
 * Updated in $/software/control processor/code/system
 * SCR 689 Nusance DPRAM interrupt failures
 *
 * *****************  Version 58  *****************
 * User: Jim Mood     Date: 10/26/10   Time: 5:12p
 * Updated in $/software/control processor/code/system
 * SCR 958 ResultCodes reentrancy
 *
 * *****************  Version 57  *****************
 * User: Jeff Vahue   Date: 9/23/10    Time: 5:34p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 56  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:01a
 * Updated in $/software/control processor/code/system
 * SCR 858 - Added FATAL to default cases
 *
 * *****************  Version 55  *****************
 * User: Peter Lee    Date: 8/31/10    Time: 5:47p
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 *
 * *****************  Version 54  *****************
 * User: Jim Mood     Date: 8/12/10    Time: 2:35p
 * Updated in $/software/control processor/code/system
 * SCR 699 Handle TODO comments
 *
 * *****************  Version 53  *****************
 * User: Jim Mood     Date: 8/12/10    Time: 2:23p
 * Updated in $/software/control processor/code/system
 * SCR 699 Added comment
 *
 * *****************  Version 52  *****************
 * User: Contractor2  Date: 8/05/10    Time: 11:19a
 * Updated in $/software/control processor/code/system
 * SCR #243 Implementation: CBIT of mssim SRS-3625
 *
 * *****************  Version 51  *****************
 * User: Contractor2  Date: 8/02/10    Time: 5:31p
 * Updated in $/software/control processor/code/system
 * SCR #762 Error: Micro-Server Interface Logic doesn't work
 *
 * *****************  Version 50  *****************
 * User: Jeff Vahue   Date: 8/02/10    Time: 4:53p
 * Updated in $/software/control processor/code/system
 * SCR# 764 - Unintialized variable 'ThisCmdSeqNum'
 *
 * *****************  Version 49  *****************
 * User: Contractor2  Date: 7/30/10    Time: 6:28p
 * Updated in $/software/control processor/code/system
 * SCR #243 Implementation: CBIT of mssim
 * Disable MS Interface if No Heartbeat with the exception of the
 * heartbeat command.
 *
 * *****************  Version 48  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 *
 * *****************  Version 47  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #711 Error: Dpram Int Failure incorrectly forces STA_FAULT.
 *
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:35p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Remove Code Coverage TPs
 *
 * *****************  Version 45  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Add TestPoints for code coverage.
 *
 * *****************  Version 44  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #8 Implementation: External Interrupt Monitors
 *
 * *****************  Version 43  *****************
 * User: Contractor3  Date: 7/06/10    Time: 10:38a
 * Updated in $/software/control processor/code/system
 * SCR #672 - Changes based on Code Review.
 *
 * *****************  Version 42  *****************
 * User: Jim Mood     Date: 7/02/10    Time: 4:06p
 * Updated in $/software/control processor/code/system
 * SCR 671 Added "busy" status to the MS-CP response packet
 *
 * *****************  Version 41  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:09p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 * Code standard fixes
 *
 * *****************  Version 40  *****************
 * User: Jim Mood     Date: 5/24/10    Time: 11:55a
 * Updated in $/software/control processor/code/system
 * SCR 606, Additional debug strings
 *
 * *****************  Version 39  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 37  *****************
 * User: Jim Mood     Date: 4/05/10    Time: 5:17p
 * Updated in $/software/control processor/code/system
 * SCR #511 change "no function to handle" debug message to verbose
 *
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 34  *****************
 * User: Jim Mood     Date: 3/12/10    Time: 11:03a
 * Updated in $/software/control processor/code/system
 * SCR #465 modfix for reentrancy of PutCommand
 *
 * *****************  Version 33  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 32  *****************
 * User: Jim Mood     Date: 2/26/10    Time: 10:45a
 * Updated in $/software/control processor/code/system
 * SCR #465
 *
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:58p
 * Updated in $/software/control processor/code/system
 * SCR# 455 - remove LINT issues
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 12/14/09   Time: 2:43p
 * Updated in $/software/control processor/code/system
 * SCR# 364
 *
 * *****************  Version 27  *****************
 * User: Jim Mood     Date: 10/15/09   Time: 9:15a
 * Updated in $/software/control processor/code/system
 * SCR# 286
 *
 * *****************  Version 26  *****************
 * User: Jim Mood     Date: 10/08/09   Time: 6:13p
 * Updated in $/software/control processor/code/system
 * SCR #283
 *
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 10/01/09   Time: 10:11a
 * Updated in $/software/control processor/code/system
 * SCR #283 Misc Code Review Updates
 *
 * *****************  Version 24  *****************
 * User: Jim Mood     Date: 12/16/08   Time: 5:17p
 * Updated in $/control processor/code/system
 * SCR-125.  Fixed MSI_PutResponse()
 *
 * *****************  Version 23  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 9:48a
 * Updated in $/control processor/code/system
 * SCR #87
 *
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 3:44p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 *
 *
 ***************************************************************************/
 
