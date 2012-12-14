#define UTILITY_BODY
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:           Utility.c

    Description:    Utility functions to perform simple tasks common to many
                    modules.

    VERSION
      $Revision: 54 $  $Date: 12-12-14 1:32p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <string.h>
#include <errno.h>
#include <stdlib.h>
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "alt_basic.h"
#include "utility.h"
#include "GSE.h"
#include "TTMR.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define BYTES_PER_WORD 4

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

/* The following CRC lookup table was generated automagically
   by the Rocksoft^tm Model CRC Algorithm Table Generation
   Program V1.0 using the following model parameters:

    Width   : 2 bytes.
    Poly    : 0x8005
    Reverse : TRUE.

   For more information on the Rocksoft^tm Model CRC Algorithm,
   see the document titled "A Painless Guide to CRC Error
   Detection Algorithms" by Ross Williams
   (ross@guest.adelaide.edu.au.). This document is likely to be
   in the FTP archive "ftp.adelaide.edu.au/pub/rocksoft".      */
static const UINT16 crc16Tbl[]  =
                     {0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,
                      0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,
                      0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,
                      0x0A00,0xCAC1,0xCB81,0x0B40,0xC901,0x09C0,0x0880,0xC841,
                      0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,
                      0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,
                      0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,
                      0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,
                      0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,
                      0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,
                      0x3C00,0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,
                      0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,
                      0x2800,0xE8C1,0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,
                      0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
                      0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,
                      0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,0x2080,0xE041,
                      0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,
                      0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
                      0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,
                      0xAA01,0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,
                      0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,
                      0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,
                      0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,0xB681,0x7640,
                      0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,
                      0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,0x5280,0x9241,
                      0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,
                      0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,
                      0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
                      0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,
                      0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,
                      0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,
                      0x8201,0x42C0,0x4380,0x8341,0x4100,0x81C1,0x8081,0x4040};

/* CRC LOOKUP TABLE
 ================
 The following CRC lookup table was generated automagically
 by the Rocksoft^tm Model CRC Algorithm Table Generation
 Program V1.0 using the following model parameters:

    Width   : 2 bytes.
    Poly    : 0x1021
    Reverse : FALSE.                                            */
static const UINT16 crc_CCITT_TBL[256] =
{
 0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
 0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
 0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
 0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
 0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
 0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
 0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
 0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
 0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
 0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
 0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
 0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
 0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
 0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
 0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
 0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
 0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};
/* CRC LOOKUP TABLE
 The following CRC lookup table was generated automagically
 by the Rocksoft^tm Model CRC Algorithm Table Generation
 Program V1.0 using the following model parameters:

    Width   : 4 bytes.
    Poly    : 0x04C11DB7L
    Reverse : TRUE.
 */
static const UINT32 crc32Tbl[] =
{0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL, 0x076DC419L, 0x706AF48FL,
 0xE963A535L, 0x9E6495A3L, 0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
 0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L, 0x1DB71064L, 0x6AB020F2L,
 0xF3B97148L, 0x84BE41DEL, 0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
 0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL, 0x14015C4FL, 0x63066CD9L,
 0xFA0F3D63L, 0x8D080DF5L, 0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L,
 0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL, 0x35B5A8FAL, 0x42B2986CL,
 0xDBBBC9D6L, 0xACBCF940L, 0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
 0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L, 0x21B4F4B5L, 0x56B3C423L,
 0xCFBA9599L, 0xB8BDA50FL, 0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
 0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL, 0x76DC4190L, 0x01DB7106L,
 0x98D220BCL, 0xEFD5102AL, 0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
 0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L, 0x7F6A0DBBL, 0x086D3D2DL,
 0x91646C97L, 0xE6635C01L, 0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL,
 0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L, 0x65B0D9C6L, 0x12B7E950L,
 0x8BBEB8EAL, 0xFCB9887CL, 0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
 0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L, 0x4ADFA541L, 0x3DD895D7L,
 0xA4D1C46DL, 0xD3D6F4FBL, 0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
 0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L, 0x5005713CL, 0x270241AAL,
 0xBE0B1010L, 0xC90C2086L, 0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
 0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L, 0x59B33D17L, 0x2EB40D81L,
 0xB7BD5C3BL, 0xC0BA6CADL, 0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
 0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L, 0xE3630B12L, 0x94643B84L,
 0x0D6D6A3EL, 0x7A6A5AA8L, 0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
 0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL, 0xF762575DL, 0x806567CBL,
 0x196C3671L, 0x6E6B06E7L, 0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
 0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L, 0xD6D6A3E8L, 0xA1D1937EL,
 0x38D8C2C4L, 0x4FDFF252L, 0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
 0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L, 0xDF60EFC3L, 0xA867DF55L,
 0x316E8EEFL, 0x4669BE79L, 0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
 0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL, 0xC5BA3BBEL, 0xB2BD0B28L,
 0x2BB45A92L, 0x5CB36A04L, 0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
 0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL, 0x9C0906A9L, 0xEB0E363FL,
 0x72076785L, 0x05005713L, 0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
 0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L, 0x86D3D2D4L, 0xF1D4E242L,
 0x68DDB3F8L, 0x1FDA836EL, 0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
 0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL, 0x8F659EFFL, 0xF862AE69L,
 0x616BFFD3L, 0x166CCF45L, 0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
 0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL, 0xAED16A4AL, 0xD9D65ADCL,
 0x40DF0B66L, 0x37D83BF0L, 0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
 0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L, 0xBAD03605L, 0xCDD70693L,
 0x54DE5729L, 0x23D967BFL, 0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
 0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL};

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
* Function:     Supper
*
* Description:  convert a string to upper case
*
* Parameters:   [in/out] str: String to be converted to upper case letters
*
* Returns:      None
*
* Notes:        None
******************************************************************************/
void Supper(INT8 * str)
{
  INT8 *pTmp;

  pTmp = str;

  while (*pTmp != '\0')
  {
    if ((*pTmp >= 'a') && (*pTmp <= 'z'))
    {
      *pTmp -= 32;
    }
    pTmp++;
  }
}



/******************************************************************************
* Function:     Slower
*
* Description:  [in/out] str: String to be converted to lower case characters
*
* Parameters:   INT8 *str - input string
*
* Returns:      None
*
* Notes:        None
******************************************************************************/
void Slower(INT8 * str)
{
  INT8 *pTmp;

  pTmp = str;

  while (*pTmp != '\0')
  {
    if ((*pTmp >= 'A') && (*pTmp <= 'Z'))
    {
      *pTmp += 32;
    }
    pTmp++;
  }
}

/******************************************************************************
* Function:     StripString
*
* Description:  Removes leading and trailing non-printable and whitespace
*               characters from a string
*
* Parameters:   [in/out]  Str: String to remove leading and trailing
*                         non-printable chars from
*
* Returns:      void
*
* Notes:
******************************************************************************/
void StripString(INT8* Str)
{
  INT8 *scanPtr = Str;

  //Scan through the input string until a printable char is found
  while( ((*scanPtr < '!')  || (*scanPtr > '~')) && (*scanPtr != '\0') )
  {
    scanPtr++;
  }

  //shift string left to remove any white space if required
  if(Str != scanPtr)
  {
    strcpy(Str, scanPtr);
  }

  // move p_char ptr to end of the string
  while(*scanPtr != '\0')
  {
    scanPtr++;
  }

  //Scan through the input string until a printable char is found.
  //Test that the string isn't null as a result of stripping the leading blanks.
  while( ((*scanPtr < '!')  || (*scanPtr > '~')) && (strlen(Str) > 0))
  {
    *scanPtr = '\0';
    scanPtr--;
  }
}



/******************************************************************************
* Function:     PadString
*
* Description:  Pads the end of a string with spaces to bring the total length
*               to the length passed into this function.  If the string
*               is already longer than Len, no action is performed
*
* Parameters:   [in/out]  Str: String to be padded with spaces
*               [in]      DesiredLen: Overall length to make the string
* Returns:      void
*
* Notes:
******************************************************************************/
void PadString(INT8* Str, UINT32 DesiredLen)
{
  UINT32 strLen;

  strLen = strlen(Str);

  for(;strLen < DesiredLen; strLen++)
  {
    Str[strLen] = ' ';
  }

  Str[DesiredLen-1] = '\0';
}

/******************************************************************************
* Function:     ChecksumBuffer
*
* Description:  Computes the byte-wise checksum of a buffer, up to 32 bits.
*               The mask parameter is used to limit the width of the result.
*
* Parameters:   [in] Buf: Memory region to checksum
*               [in] Size: Size of the region to checksum in bytes
*               [in] ChkMask: Significant bits to return.  Ex. use 0xFF
*                             for an 8-bit checksum, 0xFFFF for a 16-bit and
*                             0xFFFFFFFF for a 32-bit checksum.
* Returns:      UINT32
*
* Notes:        None
******************************************************************************/
UINT32 ChecksumBuffer(const void* Buf, UINT32 Size, UINT32 ChkMask)
{
  UINT32 i;
  UINT32 chksum = 0;
  UINT8* ptr;

  ptr = (UINT8*)Buf;

  for(i = 0; i < Size; i++)
  {
    chksum += *ptr++;
  }

  return chksum & ChkMask;
}


/******************************************************************************
* Function:     Timeout
*
* Description:  Calls TimeoutEx and append the "default" parameter value NULL,
*               for the last param to TimeoutEx.
*
* Parameters:   [in] Op:
*                       START: Set the StartTime to the current timer value
*                       CHECK: Check elapsed time against TimeoutVal duration
*
*               [in] TimeoutVal:Timeout duration in 10ns ticks.  This
*                               needs to be set for every "CHECK" op call.
*           [in/out] StartTime: Working location to store the start time for
*                               the timeout.  This must not be modified by the
*                               caller.
*
* Returns:      TRUE: TimeoutVal has expired
*               FALSE: TimeoutVal has NOT expired
*
* Notes:        Function must always be called with the Op set to START first
*               to initialize the timeout, then called with CHECK.  StartTime
*               must be static or in-scope for the duration that Timeout() is
*               used.
*
******************************************************************************/
BOOLEAN Timeout(TIMEOUT_OP Op, UINT32 TimeoutVal, UINT32* StartTime)
{
    return TimeoutEx( Op, TimeoutVal, StartTime, NULL);
}

/*vcast_dont_instrument_start*/
/******************************************************************************
* Function:     TimeoutEx
*
* Description:  Implements the Timeout function allows the caller to set a timeout
*               of 0 to 42.94 seconds in 10ns increments.  First, the StartTime
*               is set with the desired delay. Then, the elapsed time since
*               START is tested by polling the function with the CHECK.
*
* Parameters:   [in] Op:
*                       START: Set the StartTime to the current timer value
*                       CHECK: Check elapsed time against TimeoutVal duration
*
*               [in] TimeoutVal:Timeout duration in 10ns ticks.  This
*                               needs to be set for every "CHECK" op call.
*           [in/out] StartTime: Working location to store the start time for
*                               the timeout.  This must not be modified by the
*                               caller.
*              [out] lastRead:  The last time read when detecting the timeout.
*                               When timeout returns true this value can be used
*                               to determine the actual timeout duration.
*
* Returns:      TRUE: TimeoutVal has expired
*               FALSE: TimeoutVal has NOT expired
*
* Notes:        Function must always be called with the Op set to START first
*               to initialize the timeout, then called with CHECK.  StartTime
*               must be static or in-scope for the duration that Timeout() is
*               used.
*
*               Because this timeout function is used repeatedly it cause too
*               much overhead when instrumented.  Therefore instrumentation is
*               disabled.
*
******************************************************************************/
BOOLEAN TimeoutEx(TIMEOUT_OP Op, UINT32 TimeoutVal, UINT32* StartTime, UINT32* lastRead)
{
  UINT32 time;
  BOOLEAN result;

  time = TTMR_GetHSTickCount();
  // Process all recognized TIMEOUT_OP types, issue FATAL if not recognized.
  switch(Op)
  {
    case TO_START:
       *StartTime = time;
       result = TRUE;
       break;

    case TO_CHECK:
      result = TimeoutVal < (time - *StartTime) ? TRUE : FALSE;
      break;

    default:
      FATAL("Invalid TIMEOUT_OP", Op);
      break;
  }

  if (lastRead != NULL)
  {
      *lastRead = time;
  }

  return result;
}
/*vcast_dont_instrument_end*/

/******************************************************************************
* Function:     CRC16
*
* Description:  Computes a 16 bit CRC sum on a buffer using the CRC16 table
*               defined in the local var section of this file.
*
*               Init Val: 0xFFFF
*               Width   : 2 bytes.
*               Poly    : 0x8005
*               Reverse : TRUE.
*
* Parameters:   [in] Buf: Memory region to checksum
*               [in] Size: Size of the region to checksum in bytes
*
* Returns:      CRC16: UINT16 value representing the CRC computation on the
*                             input buffer.
*
* Notes:
******************************************************************************/
UINT16 CRC16(const void* Buf, UINT32 Size)
{
  const UINT16* pCRC16Tbl = crc16Tbl;
  const UINT8* ptr = Buf;
  UINT16 crc = 0xFFFF;

  while (Size--)
  {
    crc = pCRC16Tbl[(crc ^ *ptr++) & 0xFFL] ^ (crc >> 8);
  }
  return crc;
}



/******************************************************************************
* Function:     CRC_CCITT
*
* Description:  Computes a 16 bit CRC sum using the XMODEM/CCITT poly on
*               a buffer using the CRC_CCITT_TBL table defined in the local var
*               section of this file.
*
*               Init Val: 0x0000
*               Width   : 2 bytes.
*               Poly    : 0x1021
*               Reverse : FALSE.
*
* Parameters:   [in] Buf: Memory region to checksum
*               [in] Size: Size of the region to checksum in bytes
*
* Returns:      CRC16: UINT16 value representing the CRC computation on the
*                             input buffer.
*
* Notes:
******************************************************************************/
UINT16 CRC_CCITT(const void* Buf, UINT32 Size)
{
  const UINT16* pCRC_CCITT_TBL = crc_CCITT_TBL;
  const UINT8* ptr = Buf;
  UINT16 crc = 0;

  while (Size--)
  {
    crc = pCRC_CCITT_TBL[(crc>>8 ^ *ptr++) & 0xFFL] ^ (crc << 8);
  }
  return crc;
}


/******************************************************************************
 * Function:    CRC32
 *
 * Description: Calculate the crc32 value on a block of data.  This uses the
 *              crctable and with the following crc parameters:
 *
 *              Width         : 32bit
 *              Poly          : 04C11DB7
 *              Init Val      : FFFFFFFF
 *              Reflected In  : True
 *              Reflected Out : True
 *              XorOut        : FFFFFFFF
 *
 *              This method produces the same CRC outputs as the crc32
 *              algorithm used in PKZip, Ethernet, and FDDI
 *
 *              The function allows CRC computation on a block of data to be
 *              broken in to many chunks and fed to this routine in multiple
 *              calls.  The running CRC value is stored with the caller
 *              so this function can be re-entrant.  When computing
 *              the CRC in chunks, the chunks of the data block MUST be
 *              kept in order or the CRC calculation will not return a valid
 *              result.
 *
 * Parameters:  [in]     Data:  Pointer to the block of data to compute the
 *                              crc32 on
 *              [in]     Size:   length of the data block in bytes.
 *              [in/out] pCRC:   32-bit location to store the CRC result to
 *              [in]     Func:  CRC computing function to perform on this
 *                              call.  Enumerated values are:
 *
 *                               Single: The entire block of data is passed
 *                                       in in one call
 *
 *                               Start:  The first chunk of a larger block
 *                                       of data is passed in.
 *                               Next:   Subsequent chunks of a larger block
 *                                       of data is passed in
 *                               End:    The final chunk of a larger block of
 *                                       data is passed in.  The value
 *                                       returned to the CRC location is the
 *                                       final CRC of all the data chunks
 *                                       passed in.
 * Returns:     void
 *
 * Notes:
 *
 *****************************************************************************/
/*vcast_dont_instrument_start*/
void CRC32(const void *Data, UINT32 Size, UINT32* pCRC, CRC_FUNC Func)
{
  const INT8* blk_adr = Data;
  const UINT32* pCRC32Tbl = crc32Tbl;   // convert to pointer (faster)
  UINT32 crc = *pCRC;                   // convert to local (faster)

  //When starting a CRC calculation, set the initial CRC value to 0xFFFFFFFF
  //The initial value is defined by this CRC algorithm
  if((Func == CRC_FUNC_SINGLE)||(Func == CRC_FUNC_START))
  {
    crc = 0xFFFFFFFFL;
  }

  //For all cases, add all bytes to the CRC calculation
  while(Size--)
  {
    crc = (pCRC32Tbl[(crc ^ *blk_adr++) & 0xFFL] ^ (crc >> 8));
  }

  //When finishing a CRC calculation, invert the bits in the final result
  //This operation is defined by this CRC algorithm
  if((Func == CRC_FUNC_SINGLE)||(Func == CRC_FUNC_END))
  {
    crc ^= 0xFFFFFFFFL;
  }

  *pCRC = crc;
}
/*vcast_dont_instrument_end*/

/******************************************************************************
 * Function:    CalculateCheckSum
 *
 *  Description: Returns the CRC or CheckSum for the passed buffer
 *
 *
 * Parameters:  [in] method: CHK_METHOD to be used for calculating the CRC
 *              [in] Addr: starting address of buffer to be encoded into the crc
 *              [in] Size: size in bytes of the buffer to be encoded into the crc
 *
 *
 * Returns:     UINT32 containing the checksum for the address range.
 *
 * Notes:
 *
 *****************************************************************************/
UINT16 CalculateCheckSum(CHECK_METHOD method, void* Addr, UINT32 Size )
{
  UINT16 crc = 0x0000;
  switch (method)
  {
  case CM_CRC16:
    crc = CRC16(Addr, Size);
    break;

  case CM_CSUM16:
    crc = (UINT16)ChecksumBuffer(Addr, Size, 0xFFFF);
    break;

  default:
    FATAL("Unsupported CheckSum Method: %d",method);
    break;
  };
  return crc;
}


/******************************************************************************
* Function:     CompareVersions
*
* Description:  Compares two software version strings in the format
*               n.n.n.  Only 3 levels of revisions can be processed, however
*               processing will stop at the first revision level that is not
*               equal to the other and the function will return the result of
*               that comparison.
*
* Parameters:   [in] v1 - First version string to compare
*               [in] v2 - Second version string to compare
*
* Returns:      0:  The versions are identical
*               1:  v1 is greater than v2
*               2:  v2 is less than v1
*              -1: error interpeting v1 or v2
*
* Notes:
*
*****************************************************************************/
INT32 CompareVersions(const char* v1,const char* v2)
{
  INT32 v1_val;
  const CHAR  *v1_pos,*v2_pos;
  CHAR  *end;
  INT32 v2_val;
  INT32 i;
  INT32 retval = 0;

  v1_pos = v1;
  v2_pos = v2;

  //Convert v1 and v2 to their 3-integer representations
  for(i = 0; (i < 3) && (retval == 0); i++)
  {
    v1_val = strtol(v1_pos, &end, 10);
    //confirm strtol ate some characters
    if(end == v1_pos)
    {
      retval = -1;
      break;
    }
    //Advance to next number (pass '.') if not at the end of the string
    v1_pos = *end == '\0' ? end : end  + 1;


    v2_val = strtol(v2_pos, &end, 10);
    //confirm strtol ate some characters
    if(end == v2_pos)
    {
      retval = -1;
      break;
    }
    //Advance to next number (pass '.') if not at the end of the string
    v2_pos = *end == '\0' ? end : end + 1;

    if(v1_val > v2_val)
    {
      retval = 1;
    }
    else if(v1_val < v2_val)
    {
      retval = 2;
    }
  }

  return retval;
}


/******************************************************************************
* Function:     UTIL_MinuteTimerDisplay
*
* Description:  The UTIL_MinuteTimerDisplay will
*
* Parameters:   BOOLEAN bStartTimer
*
* Returns:      UINT32 Minutes Elapsed since start
*
* Notes:        The first Time this function is called it must Start the timer
*               by setting the bStartTimer to TRUE. Subsequent calls should
*               set this flag to FALSE.
*
*****************************************************************************/
UINT32 UTIL_MinuteTimerDisplay ( BOOLEAN bStartTimer )
{
  // Local Data
  static UINT32 secondCounter;
  static UINT32 minuteCounter;
  static UINT32 startTime;

  // If the first time called then reset the counters
  if (TRUE == bStartTimer)
  {
    // Initialize the StartTime, Second and Minute Counters
    startTime     = TTMR_GetHSTickCount();
    secondCounter = 0;
    minuteCounter = 0;
    // Start displaying on a new line
    GSE_StatusStr(NORMAL,"\r\n");
  }

  // Check if we have reached one second
  if ((TTMR_GetHSTickCount() - startTime) > TICKS_PER_Sec)
  {
    // Increment the second counter
    secondCounter++;
    // Save new start time
    startTime  = TTMR_GetHSTickCount();
    // Check which type of character to print
    // For Even minutes print 60 "."
    // For Odd  minutes print 60 "*"
    if (minuteCounter % 2)
    {
      GSE_StatusStr(NORMAL,".");
    }
    else
    {
      GSE_StatusStr(NORMAL,"*");
    }

    // Check if a minute has been reached and display the minute
    if ((secondCounter % 60) == 0)
    {
      minuteCounter++;
      GSE_StatusStr(NORMAL, "%d\r", minuteCounter);
    }
  }

  return (minuteCounter);
}
#ifdef ENABLE_DUMP_MEMORY_FUNC
/*vcast_dont_instrument_start*/
/******************************************************************************
* Function:     DumpMemory
*
* Description:  Displays a location in memory for a specified number of bytes.
*
* Parameters:   [in] UINT8* addr The starting address to display
*
*               [in] UINT16 size The number of bytes to display. If the size is not
*                    a multiple of 16, the size is rounded up to the next 16 byte
*                    interval.
*
*               [in] CHAR* string A user-supplied label to indentify the calling
*                    location in the the program.
*
* Returns:      void
*
* Notes:        None
*
*****************************************************************************/
void DumpMemory(UINT8* addr, UINT32 size, CHAR* string)
{
  CHAR Str[1024];
  UINT32 i;
  UINT32 remainder = (size % 16);
  BOOLEAN bRounding = FALSE;

  // If the caller supplied a size not divisible by 16, round-up
  // size to the next multiple of 16.
  if (remainder != 0)
  {
    size += (16 - remainder);
    bRounding = TRUE;
  }

  snprintf(Str,sizeof(Str),"%s DUMPING ADDR: x%08x for x%08x Bytes%s" NL,
           string == NULL ? "":string,
           addr,
           size,
           bRounding ? "(Auto-rounded)":" ");
  GSE_PutLine(Str);

  for(i = 0; i < size; i+=16)
  {
    snprintf( Str, sizeof(Str), "x%08x: %02X %02X %02X %02X %02X %02X %02X %02X "
                                "%02X %02X %02X %02X %02X %02X %02X %02X" NL,
                                &addr[ i+0],
                                addr[ i+0],
                                addr[ i+1],
                                addr[ i+2],
                                addr[ i+3],
                                addr[ i+4],
                                addr[ i+5],
                                addr[ i+6],
                                addr[ i+7],
                                addr[ i+8],
                                addr[ i+9],
                                addr[ i+10],
                                addr[ i+11],
                                addr[ i+12],
                                addr[ i+13],
                                addr[ i+14],
                                addr[ i+15]);
    GSE_PutLineBlocked(Str,TRUE);
  }

}
/*vcast_dont_instrument_end*/
#endif


/******************************************************************************
* Function:     strncpy_safe
*
* Description:  A safe string copy that ensures the dest string is null terminated.
*               or the operation will not be performed.
*
*
* Parameters:   [in/out] dest: string
*               [in]     sizeDest: s/b sizeof(dest)
*               [in]     source: string to append to dest
*               [in]     count: the number of bytes to be moved or _TRUNCATE
*
*
* Returns:      BOOLEAN  True if the copy was performed, otherwise false
*
* Notes:        If count is not _TRUNCATE, then the lesser of count and the
*               source-length will be moved to dest on the condition that
*               sizeDest is large enough to accept the string with room for a
*               null-terminator. If the move cannot be performed, dest is set to
*               null and false is returned.
*
*               If count is _TRUNCATE, then as many characters will be moved as
*               possible while still ensuring dest will be null-terminated.
*               TRUE is returned unless sizeDest is 1.
******************************************************************************/
BOOLEAN strncpy_safe(CHAR* dest, INT32 sizeDest, const CHAR* source, INT32 count )
{
  INT32 copiedCount = 0;
  INT32 sizeSource  = 0;
  INT32 numberOfBytesToMove = 0;
  BOOLEAN  status = TRUE;
  const CHAR* pSource = source;
  CHAR* pDest = dest;
  ASSERT((source != NULL) && (dest != NULL));
  //Determine size restrictions based on mode.

  // If caller has not requested to truncated the copy,
  // determine the length of the source
  if (_TRUNCATE != count )
  {
    while(sizeSource <= count && *pSource++ != '\0')
    {
      ++sizeSource;
    }

    // The number of chars to move is the lesser of
    // count and source size
    numberOfBytesToMove = (sizeSource < count) ? sizeSource : count;

    // if the # chars to move is too big to fit in the dest,
    // (with room remaining for null-term), set false and clear dest
    if(numberOfBytesToMove >= sizeDest)
    {
      status = FALSE;
      dest[0] = '\0';
    }
  }
  else // TRUNCATE_MODE
  {
    // Specify one less than the dest size(leave room for
    // null terminator.)
    // If the dest size is 1, return false.
    if(sizeDest > 1)
    {
      numberOfBytesToMove = sizeDest -1;
    }
    else
    {
      status = FALSE;
    }
  }

  // Perform the copy
  pSource = source;
  pDest   = dest;
  // Copy characters until destination size is reached or dest has been
  // assigned a null character.
  while ( status && copiedCount <= numberOfBytesToMove && (*pDest++ = *pSource++) )
  {
    ++copiedCount;
  }

  // If the destination size was reached before the string was
  // null-terminated, append it
  if( status && pDest[-1] != '\0')
  {
    pDest[-1] = '\0';
  }
  return status;
}

/******************************************************************************
* Function:     SuperStrcat
*
* Description:  A strcat that appends SOURCE to DEST, but limits the
*               total length DEST to the length passed in len.
*               If the new size of dest exceeds len, FALSE is returned.
*               dest is always returned null terminated, even if
*
* Parameters:   [in/out] dest: string to append to
*               [in]     source: string to append to dest
*               [in]     len: s/b sizeof(dest)
*
* Returns:      TRUE: source cat'd to dest, all chars copied
*               FALSE: strlen(dest+source) >= len.  No more chars can be copied to
*                      dest, the result is truncated if strlen(source+dest) >
*                      len.
*
* Notes:
******************************************************************************/
BOOLEAN SuperStrcat(INT8* dest, const INT8* source, UINT32 len)
{
  /*search for end of dest*/
  while((len > 0) && (*dest != '\0'))
  {
    len--;
    dest++;
  }

  /*End found, start appending str.  Break out if len = 0.*/
  if(*dest == '\0')
  {
    while((len > 0) && (*dest++ = *source++))
    {
      len--;
    }
  }

  /*Ensure null termination in case len reached zero before source copied to dest*/
  *--dest = '\0';

  return len == 0 ? FALSE : TRUE;
}

/******************************************************************************
 * Function:     GetBit
 *
 * Description:  A function which gets a bit state from an UINT32 array
 *               implementing a bit mask.
 *
 * Parameters:   [in] bitOffset: The bit offset into the array.
 *               [in] array:     An array of 32bit words.
 *               [in] arraySizeBytes: The length of array in bytes.
 *
 * Returns:      TRUE:  The value of the bit at bitOffset is binary 1
 *               FALSE: The value of the bit at bitOffset is binary 0 OR
 *                      the specified bit is outside the range of the array
 *
 * Notes:        0 <= bitOffset < (arraySizeBytes x 8)
 *               Otherwise function returns FALSE
 *
 ******************************************************************************/
BOOLEAN GetBit(INT32 bitOffset, UINT32 array[], INT32 arraySizeBytes)
{
  UINT32 result;
  ASSERT (array != NULL);
  ASSERT (bitOffset >= 0 && bitOffset < (arraySizeBytes * 8));
  result = array[bitOffset / 32] & ( 1U << (bitOffset % 32) );

  return (result != 0);
}


/******************************************************************************
 * Function:     SetBit
 *
 * Description:  A function which sets a bit in an UINT32 array implementing
 *               a bit mask.
 *
 * Parameters:   [in] bitOffset: The bit offset into the array.
 *               [in/out] array: An array of 32bit words.
 *               [in] arraySizeBytes: The length of array in bytes.
 *
 * Returns:      None
 *
 * Notes:        0 <= bitOffset < (arraySizeBytes x 8)
 *               Otherwise call is ignored.
 *
 ******************************************************************************/
void SetBit(INT32 bitOffset, UINT32 array[], INT32 arraySizeBytes)
{
  UINT32 wordOffset;
  UINT32 wordCnt  = (UINT32)arraySizeBytes / BYTES_PER_WORD;

  ASSERT( ((bitOffset >= 0) && ((UINT32)bitOffset < (wordCnt * 32)) ) );
  wordOffset = (UINT32)bitOffset / 32U;

  array[wordOffset] = array[wordOffset] | (1U << ((UINT32)bitOffset % 32U));

}

/******************************************************************************
 * Function:     ResetBit
 *
 * Description:  A function which clears a bit in an UINT32 array implementing
 *               a bit mask/field.
 *
 * Parameters:   [in] bitOffset: The bit offset into the array.
 *               [in/out] array: An array of 32bit words.
 *               [in] arraySizeBytes: The length of array in bytes.
 *
 * Returns:      None
 *
 * Notes:        0 <= bitOffset < (arraySizeBytes x 8)
 *               Otherwise system will ASSERT.
 *
 ******************************************************************************/
void ResetBit(INT32 bitOffset, UINT32 array[], INT32 arraySizeBytes)
{
  UINT32 wordOffset;
  UINT32 wordCnt  = (UINT32)(arraySizeBytes / BYTES_PER_WORD);

  ASSERT( ((bitOffset >= 0) && ((UINT32)bitOffset < (wordCnt * 32U)) ) );
  wordOffset = (UINT32)bitOffset / 32U;

  array[wordOffset] = ( array[wordOffset] & ~(1U << ((UINT32)bitOffset % 32U) ) );
}

/******************************************************************************
 * Function:     ResetBits
 *
 * Description:  A function which clears bits in an UINT32 array implementing
 *               a bit mask/field.
 *
 * Parameters:   [in] mask: to be tested against array.
 *               [in] maskSizeBytes:  the size of the mask in bytes
 *               [in] array: An array of 32bit words.
 *               [in] arraySizeBytes: The length of array in bytes.
 *
 * Returns:      None
 *
 * Notes:
 *
 ******************************************************************************/
void ResetBits(UINT32 mask[], INT32 maskSizeBytes, UINT32 array[], INT32 arraySizeBytes)
{
  // Local Data
   UINT16 i;

  // Check that the sizes are the same
  ASSERT(maskSizeBytes == arraySizeBytes );

  for ( i = 0; i < (arraySizeBytes / BYTES_PER_WORD); i++ )
  {
     array[i] &= ~(mask[i]);
  }

}

/******************************************************************************
 * Function:     TestBits
 *
 * Description:  A function which compares an array representing a bit mask against
 *               an equal length array representing a field of bit flags.
 *
 * Parameters:   [in] mask: to be tested against array.
 *               [in] maskSizeBytes:  the size of the mask in bytes
 *               [in] data: An array of 32bit words.
 *               [in] dataSizeBytes: The length of array in bytes.
 *               [in] exact: true -  The function will test if ALL bits
 *                                   set in mask are also set in data;
 *                           false - The function will test if ANY bits
 *                                   set in mask are also set in data;
 *
 *
 * Returns:      BOOLEAN
 *
 * Notes:        MaskSizeBytes must equals dataSizeBytes.
 *               Otherwise system will ASSERT.
 *
 ******************************************************************************/
BOOLEAN TestBits( UINT32 mask[], INT32 maskSizeBytes, UINT32 data[], INT32 dataSizeBytes,
                  BOOLEAN exact )
{
  INT32 i;
  INT32 wordCnt  = dataSizeBytes / BYTES_PER_WORD;

  INT32 matchedCnt = 0;
  INT32 expectedCnt;

  // Check that the sizes are the same
  ASSERT(maskSizeBytes == dataSizeBytes );

  // If exact matching, expect as many matches as words in array
  // else, a matched bit in any one word will suffice.
  expectedCnt = exact ? wordCnt : 1;

  for (i = 0; (i < wordCnt) && (matchedCnt < expectedCnt); ++i)
  {
    // If this word is zeros in both the mask and data,
    // consider it matched and skip to next word.
    if ( exact && mask[i] == 0 && data[i] == 0)
    {
      matchedCnt++;
      continue;
    }

    if (exact)
    {
      // EXACT: Perform XOR on this word... All must match
      matchedCnt += (mask[i] ^ data[i]) == 0 ? 1 : 0;
    }
    else
    {
      // ANY: Perform AND on this word. Any matching bit is success
      matchedCnt += (mask[i] & data[i]) > 0 ? 1 : 0;
    }
  }
  return matchedCnt == expectedCnt;
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/



/*************************************************************************
 *  MODIFICATIONS
 *    $History: Utility.c $
 * 
 * *****************  Version 54  *****************
 * User: John Omalley Date: 12-12-14   Time: 1:32p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates by Dave B
 * 
 * *****************  Version 53  *****************
 * User: Contractor V&v Date: 12/10/12   Time: 1:44p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review
 *
 * *****************  Version 52  *****************
 * User: John Omalley Date: 12-12-02   Time: 1:03p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 51  *****************
 * User: John Omalley Date: 12-10-23   Time: 2:49p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 50  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 49  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 48  *****************
 * User: John Omalley Date: 12-07-13   Time: 3:50p
 * Updated in $/software/control processor/code/system
 * SCR 1107 Added BIT processing for 128 bit array
 *
 * *****************  Version 47  *****************
 * User: Contractor V&v Date: 3/21/12    Time: 6:49p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 relocated evaluator to drivers
 *
 * *****************  Version 46  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:52p
 * Updated in $/software/control processor/code/system
 * FAST 2 Support trigger/event
 *
 * *****************  Version 45  *****************
 * User: Contractor2  Date: 4/11/11    Time: 2:25p
 * Updated in $/software/control processor/code/system
 * SCR 1013: Code Coverage: Removed unused function BCDToBinaryConvert
 *
 * *****************  Version 44  *****************
 * User: Contractor2  Date: 10/01/10   Time: 12:59p
 * Updated in $/software/control processor/code/system
 * SCR #915 Loop termination error in SuperStrcat
 *
 * *****************  Version 43  *****************
 * User: Peter Lee    Date: 8/04/10    Time: 1:49p
 * Updated in $/software/control processor/code/system
 * SCR #765 Remove unused func UpdateCheckSum().
 *
 * *****************  Version 42  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 *
 * *****************  Version 41  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 *
 * *****************  Version 40  *****************
 * User: Contractor V&v Date: 6/16/10    Time: 6:11p
 * Updated in $/software/control processor/code/system
 * SCR #636 add the ability to read/write EEPROM/RTC via the sys.g
 *
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:55p
 * Updated in $/software/control processor/code/system
 * SCR #615 Showcfg/Long msg enhancement
 *
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 5/24/10    Time: 6:57p
 * Updated in $/software/control processor/code/system
 * SCR #585 Cfg File CRC changes from load to load
 *
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:52p
 * Updated in $/software/control processor/code/system
 * SCR #572 - VectorCast changes
 *
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 4/12/10    Time: 7:22p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:12p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 7:02p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy fixing comment
 *
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 31  *****************
 * User: Contractor2  Date: 3/22/10    Time: 4:00p
 * Updated in $/software/control processor/code/system
 * SRC 490 - Check program CRC on startup.
 * Optimized performance.
 *
 * *****************  Version 30  *****************
 * User: Contractor2  Date: 3/17/10    Time: 3:32p
 * Updated in $/software/control processor/code/system
 * SRC 490 - Check program CRC on startup.
 * Relocated CRC tables from initialized ram to flash.
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR #483 - Function Naming
 * SCR# 452 - Code Coverage Improvements
 *
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 3/11/10    Time: 11:05a
 * Updated in $/software/control processor/code/system
 * SCR# 481 - add Comments to inhibit VectorCast Coverage statements when
 * instrumenting code
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:05p
 * Updated in $/software/control processor/code/system
 * SCR 67 Changed type ret on CalculateCheckSum , added DumpMemory
 *
 * *****************  Version 26  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:59p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 11:24a
 * Updated in $/software/control processor/code/system
 * SCR# 467 - move minute timer display to Utitlities and rename TTMR to
 * UTIL in function name
 *
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 2:18p
 * Updated in $/software/control processor/code/system
 * SCR# 176 - change BitToBitMask
 *
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:58p
 * Updated in $/software/control processor/code/system
 * SCR 330
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 2/01/10    Time: 1:34p
 * Updated in $/software/control processor/code/system
 * SCR# 314 - fix strange user response when CTRL or blank characters are
 * sent to user command processing
 *
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 1/28/10    Time: 10:32a
 * Updated in $/software/control processor/code/system
 * SCR# 142: Error: Faulted EEPROM "Write In Progress" bit causes infinite
 * loop.
 *
 * Extended the Timeout function by creating TimeoutEx that returns the
 * last time read when the timeout was decalred so the caller may know how
 * much over the limit they were.
 *
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 1/27/10    Time: 5:24p
 * Updated in $/software/control processor/code/system
 * SCR 142
 *
 * *****************  Version 19  *****************
 * User: Jim Mood     Date: 1/20/10    Time: 3:44p
 * Updated in $/software/control processor/code/system
 * Added a Timeout() utility function
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:28p
 * Updated in $/software/control processor/code/system
 * SCR 316 Added instrumented version of CRC32
 *
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 12/07/09   Time: 4:26p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - typos
 *
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 11/30/09   Time: 9:31a
 * Updated in $/software/control processor/code/system
 * Added XMODEM/CCITT compatable CRC routine
 *
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 11/05/09   Time: 3:04p
 * Updated in $/software/control processor/code/system
 * Added SuperStrcat as part of scr # 320
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 10/23/09   Time: 9:35a
 * Updated in $/software/control processor/code/system
 * SCR #315 Add CmpBytes() routine to support CBIT / SEU Reg Processing
 *
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 10/13/09   Time: 2:44p
 * Updated in $/software/control processor/code/system
 * SCR #184
 *
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 8/18/09    Time: 2:28p
 * Updated in $/software/control processor/code/system
 * SCR #246 StripString function
 *
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 11/24/08   Time: 2:32p
 * Updated in $/control processor/code/system
 * Added CRC32 function and prototype
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 4:25p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 *
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 9/23/08    Time: 10:15a
 * Updated in $/control processor/code/system
 *
 *
 ***************************************************************************/
