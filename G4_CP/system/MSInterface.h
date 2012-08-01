#ifndef SYS_MSINTERFACE_H
#define SYS_MSINTERFACE_H

/******************************************************************************
            Copyright (C) 2008-2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

  File:          MSInterface.h
 
  Description: 
  
  VERSION
      $Revision: 19 $  $Date: 7/19/12 11:07a $ 
 
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/ 
#include "mcf548x.h"
#include "DPRAM.h"
#include "MSCPInterface.h"
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define MSI_MAX_PENDING_RSP     10   //Max number of CP commands that can be
                                     // awaiting responses from the MS
#define MSI_NUM_OF_CMD_HANDLERS 10   //The number of entries in the command
                                     //handler table needed to define every
                                     //command that can originate from the
                                     //microserver.
#define MSI_MSG_IN_DPRAM_TIMEOUT_mS 1000
/******************************************************************************
                                 Package Typedefs
******************************************************************************/
//Callback prototypes for micro server response handlers and command handlers
typedef enum {
  MSI_RSP_SUCCESS,
  MSI_RSP_BUSY,
  MSI_RSP_FAILED,
  MSI_RSP_TIMEOUT
}MSI_RSP_STATUS;

//MSI_RSP_CALLBACK processes the micro server response of a command sent
//from the control processor
//
//Parameters:
//            Id:         Message Id of the response (MSCP_CMD_ID)
//            PacketData: Pointer to the data returned from the microserver.
//            Size:       Length (in bytes) of the data returned from the
//                        microserver
//            Status:     Response status from the micro server
typedef void (*MSI_RSP_CALLBACK)(UINT16 Id, void* PacketData, UINT16 Size,
                                 MSI_RSP_STATUS Status);

//MSI_CMD_CALLBACK processes a command originating from the micro-server.
//The command handling function shall process the command, then return a 
//response packet to the microserver by calling MSI_PutResponse.  The function
//must hold the return the "Sequence" value to MSI_PutResponse. 
//
//Parameters:
//            PacketData: Pointer to the data returned from the microserver.
//            Size:       Length (in bytes) of the data returned from the
//                        microserver
//            Sequence:   Command sequence number assigned by the microserver.
//                        This value must be returned in the response, it is
//                        the command handlers duty to copy the sequence number
//                        to the response.
typedef BOOLEAN (*MSI_CMD_CALLBACK)(void* PacketData, UINT16 Size, UINT32 Sequence);

#pragma pack(1)

// DPRAM interface interrupt failure log
typedef struct {
  UINT32  minIntrTime; 
  UINT32  maxExeTime; 
} DPRAM_INTR_FAIL_LOG; 

#pragma pack()

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( SYS_MSINTERFACE_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT RESULT MSI_PutCommand(UINT16 Id,const void* data,UINT32 size,INT32 TOmS,
                      MSI_RSP_CALLBACK RspHandler);
EXPORT RESULT MSI_PutResponse(UINT16 Id, const void* data, UINT16 status, UINT32 size,
                       UINT32 seq );
EXPORT RESULT MSI_AddCmdHandler(UINT16 Id, MSI_CMD_CALLBACK CmdHandler);

EXPORT void MSI_Init(void); 

EXPORT RESULT MSI_PutCommandEx(UINT16 Id,const void* data,UINT32 size,INT32 TOmS,
                               MSI_RSP_CALLBACK RspHandler, BOOLEAN NoCheck);
/*************************************************************************
 *  MODIFICATIONS
 *    $History: MSInterface.h $
 * 
 * *****************  Version 19  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 * 
 * *****************  Version 18  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 17  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #8 Implementation: External Interrupt Monitors
 * 
 * *****************  Version 16  *****************
 * User: Contractor3  Date: 7/06/10    Time: 10:38a
 * Updated in $/software/control processor/code/system
 * SCR #672 - Changes based on Code Review.
 * 
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 7/02/10    Time: 4:06p
 * Updated in $/software/control processor/code/system
 * SCR 671 Added "busy" status to the MS-CP response packet
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 10/01/09   Time: 10:11a
 * Updated in $/software/control processor/code/system
 * SCR #283 Misc Code Review Updates
 * 
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 9/11/09    Time: 10:50a
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 3:47p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 * 
 *
 *************************************************************************/
 
#endif // SYS_MSINTERFACE_H


