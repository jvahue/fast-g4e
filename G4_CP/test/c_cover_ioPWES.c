/*
---------------------------------------
-- Copyright 2001 Vector Software    --
-- North Kingstown, Rhode Island USA --
---------------------------------------

Modified for use on the FAST system during V&V.

This file must be placed in the VectorCast code directory associated with the project that has
been setup.

e.g.,

A possible FAST_Testing dir structure is shown below along with the directory used to create a 
project for VectorCover (+) and the location of this file (*).  The dir Testing (-) is used to
save this file in CM.  It must be moved to the location (*) that the user has setup during
testing.

FAST_Testing
. dev
.. CP_Code
... application
... build
... drivers
... system
... Testing (-)
.. CP_Coverage (+)
... CP_Cov1 (*)
.. STE
... IOC
... PySTE
. scripts
.. bin
... Logs
.. TestScripts

This file's operation is tied to the inclusion of the TestPoints module for achieving code
coverage.
The TestPoint module provides the command used by the STE to retrieve the coverage data.

//=================================== PWC Modifications =======================================
Three functions:
  1. void ReportCoverage(void)
  2. void SendWait( const char* str)
  3. void KickWd(void)

Have been added to support FAST Testing.  ReportCoverage is called from the TestPoint action
cmd TP.DumpCov to handle dumping the coverage data, and SendWait is used to "throttle" the
output of data so no data is lost on the Serial Port.    KickWd is a callback fuunction to
ensure the WD does not expire when dumping data during shutdown.

Additionally the function VCAST_WRITE_TO_INST_FILE_COVERAGE has been modified to use SendWait.

The following compiler switches must be on to use this code.

	-DSTE_TP
	-DVCAST_USE_STATIC_MEMORY
	-DVCAST_COVERAGE_IO_BUFFERED
	-DVCAST_CUSTOM_OUTPUT
	
And you may want the following to allow for log creation, but it is not needed:
	-DCREATE_LOGS	

To speed up the execution this in-lines the function
    -OI=VCAST_SAVE_STATEMENT_REALTIME 
    
And of course this file must be added to the build.
*/



/* Defined variable usage for this file:

   Variable names that are indented apply only if the enclosing variable is set

   <<no defined variables set>>    : Output is written to the file TESTINSS.DAT

   VCAST_USE_STDOUT                : Output is written to stdout using puts
      VCAST_USE_PUTCHAR            : Output is written to stdout using putchar
      VCAST_USE_GH_SYSCALL         : Output is written to stdout using the GH syscall
                                     (For Green Hills INTEGRITY 178B)
   VCAST_USE_STD_STRING            : This define turns ON the use of memset from the
                                     system header string.h.

   VCAST_USE_STATIC_MEMORY         : No malloc is available, use alternate data.
      VCAST_MAX_MCDC_STATEMENTS    : The number of MCDC statement conditions 
                                     that can be reached when malloc is not 
                                     available. 
   VCAST_MAX_COVERED_SUBPROGRAMS   : The number of subprograms that may be
                                     covered. 
   VCAST_ATTRIBUTE_CODE            : Allows the user to specify an attribute
                                     that will be placed before the ascii, 
                                     binary and subprogram coverage pool 
                                     global variables. This is useful for 
                                     putting this data in specific places
                                     in memory.
   VCAST_DUMP_CALLBACK             : If this is defined to a function name, 
                                     then when the user calls 
                                     VCAST_DUMP_COVERAGE_DATA, the function
                                     this was defined to will be called. The 
                                     purpose is to allow the users main loop
                                     to be given a chance to run within a 
                                     certain time frame.
   VCAST_FLUSH_DATA                : Use the flush system call after each
                                     string is written with fprintf in
                                     VCAST_WRITE_TO_INST_FILE_COVERAGE. The
                                     default is disabled. Define to any value
                                     to enable.
   VCAST_APPEND_WIN32_PID          : Append the process id to the TESTINSS.DAT
                                     file. This uses windows specific system
                                     calls to get the pid.
   VCAST_APPEND_POSIX_PID          : Append the process id to the TESTINSS.DAT
                                     file. This uses Posix specific system
                                     calls to get the pid.
*/

#ifndef __C_COVER_H__
#define __C_COVER_H__

#ifdef __cplusplus
extern "C" {
#endif

//=================================== PWC Modifications =======================================
#include "GSE.h"
#include "UART.h"
#include "FPGA.h"
void SendWait( const char* str);
void KickWd(void);
void SendPacket(void);


#define VCAST_DUMP_CALLBACK KickWd
//=================================== PWC Modifications =======================================


#ifndef VCAST_CONDITION_TYP
#define VCAST_CONDITION_TYP int
#endif

/* AVL prototypes */
typedef struct vcast_mcdc_statement* VCAST_elementType;
struct AVLNode;
typedef struct AVLNode *VCAST_position;
typedef struct AVLNode *AVLTree;
VCAST_position find (VCAST_elementType VCAST_X, AVLTree VCAST_T);
AVLTree insert (VCAST_elementType VCAST_X, AVLTree VCAST_T);

struct vcast_mcdc_statement
{
  unsigned long mcdc_bits;
  unsigned long mcdc_bits_used;
};
typedef struct vcast_mcdc_statement *vcast_mcdc_statement_ptr;

struct vcast_statement_coverage
{
  /* Allocate N/8 bytes where N is the number of statements */
  char *coverage_bits;
  /* The number of statement statements */
  short vcast_num_statement_statements;
};
typedef struct vcast_statement_coverage *vcast_statement_coverage_ptr;

struct vcast_branch_coverage
{
  /* Allocate N/4 bytes where N is the number of statements */
  char *branch_bits_true;
  char *branch_bits_false;
  /* The number of branch statements */
  short vcast_num_branch_statements;
};
typedef struct vcast_branch_coverage *vcast_branch_coverage_ptr;

struct vcast_mcdc_coverage
{
  /* For each statement hit, a 'struct vcast_mcdc_statement'
   * will be inserted into this structure to represent the
   * statements that have been hit.
   * Allocate N tree's where N is the number of statements.
   */
  AVLTree *mcdc_data;
  /* The number of mcdc statements */
  short vcast_num_mcdc_statements;
  /* The temporary statement used. */
  struct vcast_mcdc_statement cur_vcast_mcdc_statement;
};
typedef struct vcast_mcdc_coverage *vcast_mcdc_coverage_ptr;

struct vcast_levela_coverage
{
  struct vcast_statement_coverage *statement_coverage;
  struct vcast_mcdc_coverage *mcdc_coverage;
};
typedef struct vcast_levela_coverage *vcast_levela_coverage_ptr;

struct vcast_levelb_coverage
{
  struct vcast_statement_coverage *statement_coverage;
  struct vcast_branch_coverage *branch_coverage;
};
typedef struct vcast_levelb_coverage *vcast_levelb_coverage_ptr;

enum vcast_coverage_kind
{
  VCAST_COVERAGE_STATEMENT = 1,
  VCAST_COVERAGE_BRANCH = 2,
  VCAST_COVERAGE_MCDC = 4,
  VCAST_COVERAGE_LEVEL_A = 8,
  VCAST_COVERAGE_LEVEL_B = 16
};

#ifndef VCAST_COVERAGE_KIND_TYPE
#define VCAST_COVERAGE_KIND_TYPE char
#endif

struct vcast_subprogram_coverage
{
  /* The unit id */
  short vcast_unit_id;
  /* The subprogram id */
  short vcast_subprogram_id;

  /* if coverage_kind == VCAST_COVERAGE_STATEMENT
   *   coverage_ptr is of type 'struct vcast_subprogram_statement_coverage *'
   * if coverage_kind == VCAST_COVERAGE_BRANCH
   *   coverage_ptr is of type 'struct vcast_subprogram_branch_coverage *'
   * if coverage_kind == VCAST_COVERAGE_MCDC
   *   coverage_ptr is of type 'struct vcast_subprogram_mcdc_coverage *'
   * if coverage_kind == VCAST_COVERAGE_LEVEL_A
   *   coverage_ptr is of type 'struct vcast_subprogram_levela_coverage *'
   * if coverage_kind == VCAST_COVERAGE_LEVEL_B
   *   coverage_ptr is of type 'struct vcast_subprogram_levelb_coverage *'
   */
  VCAST_COVERAGE_KIND_TYPE coverage_kind;
  void *coverage_ptr;
};

VCAST_CONDITION_TYP
VCAST_SAVE_MCDC_SUBCONDITION (struct vcast_subprogram_coverage *coverage,
                              int bit_index, VCAST_CONDITION_TYP condition);

VCAST_CONDITION_TYP
VCAST_SAVE_MCDC_LEVEL_A_SUBCONDITION (
                              struct vcast_subprogram_coverage *coverage,
                              int bit_index, VCAST_CONDITION_TYP condition);

void VCAST_SAVE_STATEMENT_REALTIME(struct vcast_subprogram_coverage *coverage, 
                             int statement);

VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_CONDITION_REALTIME(struct vcast_subprogram_coverage *coverage,
                               int statement,
                               VCAST_CONDITION_TYP condition);

VCAST_CONDITION_TYP 
VCAST_SAVE_MCDC_CONDITION_REALTIME(struct vcast_subprogram_coverage *coverage,
                             int statement, VCAST_CONDITION_TYP condition);

void VCAST_SAVE_STATEMENT_LEVEL_A_REALTIME(
                             struct vcast_subprogram_coverage *coverage, 
                             int statement);
VCAST_CONDITION_TYP 
VCAST_SAVE_MCDC_LEVEL_A_CONDITION_REALTIME(
                             struct vcast_subprogram_coverage *coverage,
                             int statement, VCAST_CONDITION_TYP condition);

void VCAST_SAVE_STATEMENT_LEVEL_B_REALTIME(
                             struct vcast_subprogram_coverage *coverage, 
                             int statement);

VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_LEVEL_B_CONDITION_REALTIME(
                               struct vcast_subprogram_coverage *coverage,
                               int statement,
                               VCAST_CONDITION_TYP condition);
void VCAST_SAVE_STATEMENT_ANIMATION(struct vcast_subprogram_coverage *coverage, 
                             int statement);

VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_CONDITION_ANIMATION(struct vcast_subprogram_coverage *coverage,
                               int statement,
                               VCAST_CONDITION_TYP condition);
VCAST_CONDITION_TYP 
VCAST_SAVE_MCDC_CONDITION_ANIMATION(struct vcast_subprogram_coverage *coverage,
                             int statement, VCAST_CONDITION_TYP condition);

void VCAST_SAVE_STATEMENT_LEVEL_A_ANIMATION(
                             struct vcast_subprogram_coverage *coverage, 
                             int statement);

VCAST_CONDITION_TYP 
VCAST_SAVE_MCDC_LEVEL_A_CONDITION_ANIMATION(
                             struct vcast_subprogram_coverage *coverage,
                             int statement, VCAST_CONDITION_TYP condition);

void VCAST_SAVE_STATEMENT_LEVEL_B_ANIMATION(
                             struct vcast_subprogram_coverage *coverage, 
                             int statement);

VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_LEVEL_B_CONDITION_ANIMATION(
                               struct vcast_subprogram_coverage *coverage,
                               int statement,
                               VCAST_CONDITION_TYP condition);

VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_CONDITION_BUFFERED(
                            struct vcast_subprogram_coverage *coverage,
                            int statement,
                            VCAST_CONDITION_TYP condition);

VCAST_CONDITION_TYP 
VCAST_SAVE_MCDC_CONDITION_BUFFERED(
                          struct vcast_subprogram_coverage *coverage,
                          int statement, VCAST_CONDITION_TYP condition);

VCAST_CONDITION_TYP 
VCAST_SAVE_MCDC_LEVEL_A_CONDITION_BUFFERED(
                          struct vcast_subprogram_coverage *coverage,
                          int statement, VCAST_CONDITION_TYP condition);

VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_LEVEL_B_CONDITION_BUFFERED(
                            struct vcast_subprogram_coverage *coverage,
                            int statement,
                            VCAST_CONDITION_TYP condition);

void VCAST_REGISTER_STATEMENT (struct vcast_subprogram_coverage **cov, 
   char *vcast_bits_statement,
   int vcast_unit_id, int vcast_subprogram_id, int vcast_size);
void VCAST_REGISTER_BRANCH (struct vcast_subprogram_coverage **cov, 
   int vcast_unit_id, int vcast_subprogram_id, int vcast_size);
void VCAST_REGISTER_MCDC (struct vcast_subprogram_coverage **cov, 
   int vcast_unit_id, int vcast_subprogram_id, int vcast_size);
void VCAST_REGISTER_LEVEL_A (struct vcast_subprogram_coverage **cov, 
   char *vcast_bits_statement,
   int vcast_unit_id, int vcast_subprogram_id, int vcast_size,
   int vcast_include_stmnt);
void VCAST_REGISTER_LEVEL_B (struct vcast_subprogram_coverage **cov, 
   char *vcast_bits_statement,
   int vcast_unit_id, int vcast_subprogram_id, int vcast_size,
   int vcast_include_stmnt);
void VCAST_DUMP_COVERAGE_DATA (void);
void VCAST_INITIALIZE (void);

#ifdef __cplusplus
}
#endif

#endif
#include "vcast_c_options.h"

#ifndef VCAST_UNIT_TEST_TOOL
#include <string.h> /* Needed for string operations */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
void *VCAST_memset(void *s,int c,unsigned int n){
  return memset(s, c, n);
}
#ifdef __cplusplus
}
#endif /* __cplusplus */

/*Allow file operations*/
#ifndef VCAST_USE_STDIO_OPS
#define VCAST_USE_STDIO_OPS
#endif /* VCAST_USE_STDIO_OPS */

#ifndef VCAST_USE_STATIC_MEMORY
#include <stdlib.h> /* Needed for malloc */
/* In the unit test tool, this will use the VCAST_malloc
   already defined in the unit test harness. In the coverage
   tool, malloc is currently used. */
#define VCAST_malloc malloc
#endif /* VCAST_USE_STATIC_MEMORY */
#endif /* VCAST_UNIT_TEST_TOOL */

/* #ifdef       __cplusplus
extern "C" {
#endif */

#ifndef VCAST_ATTRIBUTE_CODE
#define VCAST_ATTRIBUTE_CODE
#endif /* VCAST_ATTRIBUTE_CODE */

#ifdef VCAST_USE_BUFFERED_ASCII_DATA

/*Do not use file operations*/
#undef VCAST_USE_STDIO_OPS

/* This is where all the captured coverage data will be written in
 * human readable form. */
char VCAST_ATTRIBUTE_CODE vcast_ascii_coverage_data_pool[VCAST_MAX_CAPTURED_ASCII_DATA];
int vcast_ascii_coverage_data_pos = 0;

#endif /* VCAST_USE_BUFFERED_ASCII_DATA */

#ifndef VCAST_USE_GH_SYSCALL
#ifdef VCAST_USE_STDIO_OPS
#include <stdio.h>
#endif /* VCAST_USE_STDIO_OPS */
#endif /*VCAST_USE_GH_SYSCALL*/

#ifndef VCAST_USE_STDOUT
/* Normal case, uses a file for output */
#ifndef VCAST_UNIT_TEST_TOOL
#ifdef VCAST_USE_STDIO_OPS
FILE *vCAST_INST_FILE = NULL;
#endif /* VCAST_USE_STDIO_OPS */
#endif /* VCAST_UNIT_TEST_TOOL */
#endif /* VCAST_USE_STDOUT */

#if defined VCAST_UNIT_TEST_TOOL
#define VCAST_WRITE_COVERAGE_DATA(string) { vCAST_CREATE_INST_FILE(); vectorcast_fprint_string_with_cr(vCAST_INST_FILE, string); }
#define VCAST_WRITE_COVERAGE_DATA_FLUSH(string) { vCAST_CREATE_INST_FILE(); vectorcast_fprint_string_with_cr(vCAST_INST_FILE, string); }
#else /* VCAST_UNIT_TEST_TOOL */
#define VCAST_WRITE_COVERAGE_DATA(string) (VCAST_WRITE_TO_INST_FILE_COVERAGE(string, 0))
#define VCAST_WRITE_COVERAGE_DATA_FLUSH(string) (VCAST_WRITE_TO_INST_FILE_COVERAGE(string, 1))
#endif /* VCAST_UNIT_TEST_TOOL */

/* The instrumentation code depends on this data, usually found in S2.c in a
 * test harness.  For standalone coverage, they need to be supplied here: */
#ifndef LONG_MAX
#define LONG_MAX 2147483647L
#endif /* LONG_MAX */

#ifndef VCAST_UNIT_TEST_TOOL
#ifndef VCAST_USE_STDOUT

#ifdef VCAST_APPEND_WIN32_PID
#include <windows.h>
#elif VCAST_APPEND_POSIX_PID
#include <sys/types.h>
#include <unistd.h>
#endif /* end VCAST_APPEND_WIN32_PID */

static char *
VC_strcat (char *VC_S, const char *VC_T);
static char *
VC_INT_TO_STRING (char *buf, unsigned long vc_x);

void vCAST_CREATE_INST_FILE (void)
{
  char result_filename[32] = "TESTINSS.DAT";
#ifdef VCAST_APPEND_WIN32_PID
  DWORD pid = GetCurrentProcessId();
  sprintf (result_filename,
           "%s-%d.%s",
           "TESTINSS",
           pid, 
           "DAT");
#elif VCAST_APPEND_POSIX_PID
  result_filename[0] = 0;
  VC_strcat(result_filename, "TESTINSS-");
  VC_INT_TO_STRING(result_filename+9, (int)getpid());
  VC_strcat(result_filename, ".DAT");
#endif
#ifdef VCAST_USE_STDIO_OPS
   if (!vCAST_INST_FILE)
#ifdef VCAST_APPEND_TO_TESTINSS
      vCAST_INST_FILE = fopen (result_filename, "a");
#else /* VCAST_APPEND_TO_TESTINSS */
      vCAST_INST_FILE = fopen (result_filename, "w");
#endif /* VCAST_APPEND_TO_TESTINSS */
#endif /* VCAST_USE_STDIO_OPS */
}

void VCAST_CLOSE_FILE (void)
{
#ifdef VCAST_USE_STDIO_OPS
   if (vCAST_INST_FILE > (FILE*)0 )
      fclose(vCAST_INST_FILE);
   vCAST_INST_FILE = (FILE*)0;
#endif /* VCAST_USE_STDIO_OPS */
}
#endif /* VCAST_USE_STDOUT */
#endif /* VCAST_UNIT_TEST_TOOL */

#ifndef VCAST_UNIT_TEST_TOOL

#ifdef VCAST_USE_GH_SYSCALL

#ifdef __cplusplus
extern "C" int __ghs_syscall(int, ...);
#else /* __cplusplus */
extern int __ghs_syscall(int, ...);
#endif /* __cplusplus */

#endif /* end VCAST_USE_GH_SYS_CALL */

/* Original write_to_inst_file function.  Currently used by default when
 * coverage optimizations are turned off. */
void VCAST_WRITE_TO_INST_FILE_COVERAGE (const char S[], int flush)
{
#ifdef VCAST_USE_BUFFERED_ASCII_DATA
   while (*S) {
      vcast_ascii_coverage_data_pool[vcast_ascii_coverage_data_pos++] = *S;
      ++S;
   }
   vcast_ascii_coverage_data_pool[vcast_ascii_coverage_data_pos++] = '\n';
   vcast_ascii_coverage_data_pool[vcast_ascii_coverage_data_pos] = '\0';
#else /* VCAST_USE_BUFFERED_ASCII_DATA */
#ifdef VCAST_CUSTOM_OUTPUT
//=================================== PWC Modifications =======================================
  /* Insert your own call to an API to write "str" out of your target */
  /* For example: "uart_print (str);" */
  /* Also set the defined variable: VCAST_CUSTOM_OUTPUT */
  /*  in the VectorCAST Tool Options Dialog -> C/C++ tab */
    SendWait( S);
    SendWait( "\r\n");
//=================================== PWC Modifications =======================================
#else
#ifdef VCAST_USE_STDIO_OPS
#ifdef VCAST_USE_STDOUT
#ifdef VCAST_USE_PUTCHAR

   int i, len;
   len = strlen( S );
   for( i=0; i<len; ++i ) {
      putchar( S[i] );
   }
   putchar( '\n' );

#else /* ndef VCAST_USE_PUTCHAR */
#ifdef VCAST_USE_GH_SYSCALL

   /* worst case is 30 chars plus the end of line */
   char Str[31];
   int  Len = strlen (S);
   strcpy (Str, S);
   strcat (Str, "\n"); 
   /* we hardcode stdout (1) as the file handle */
   __ghs_syscall(0x40001, 1, Str, Len+1);
     
#else /* ndef VCAST_USE_GH_SYSCALL */    

   puts( S );
   
#endif /* end VCAST_USE_GH_SYS_CALL */
#endif /* end VCAST_USE_PUTCHAR */

#else /* ndef VCAST_USE_STDOUT */

   vCAST_CREATE_INST_FILE();
   fprintf ( vCAST_INST_FILE, "%s\n", S );
   if (flush) {
     fflush (vCAST_INST_FILE);
   }
#ifdef VCAST_FLUSH_DATA
   else {
     fflush (vCAST_INST_FILE);
   }
#endif /* VCAST_FLUSH_DATA */
   
#endif /* end VCAST_USE_STDOUT */
#endif /* VCAST_USE_STDIO_OPS */
#endif /* VCAST_CUSTOM_OUTPUT */
#endif /* VCAST_USE_BUFFERED_ASCII_DATA */
}
#endif /* end VCAST_UNIT_TEST_TOOL */

static char *
VC_strcat (char *VC_S, const char *VC_T)
{
   int vc_i,vc_j;
   vc_i = vc_j = 0;
   while ( VC_S[vc_i] != 0 )
      vc_i++;           /* find end of VC_S */
   while ( (VC_S[vc_i++] = VC_T[vc_j++]) != 0 ) ; /* copy VC_T */
   return VC_S;
}

static char *
VC_INT_TO_STRING (char *buf, unsigned long vc_x)
{
   int vc_i;
   int vc_cur = vc_x;
   int vc_size = 0;

   if (vc_x == 0) {
      buf[0] = '0';
      buf[1] = '\0';
      return buf;
   }

   while (vc_cur != 0) {
      vc_size++;
      vc_cur /= 10;
   }

   for (vc_i = 0; vc_x != 0; vc_i++){
      buf[vc_size-vc_i-1] = (vc_x % 10) + '0';
      vc_x /= 10;
   }

   buf[vc_size] = 0;

   return buf;
}

/*
   VCAST_STATEMENT_ID_BUF_SIZE     : The number of characters needed to print
                                     a statement id. The default is 5, which 
                                     supports 9,999 statements in a single
                                     function.
   VCAST_SUBPROGRAM_ID_BUF_SIZE    : The number of characters needed to print
                                     a subprogram id. The default is 5, which 
                                     supports 9,999 functions in a single
                                     unit.
   VCAST_UNIT_ID_BUF_SIZE          : The number of characters needed to print
                                     a unit id. The default is 5, which 
                                     supports 9,999 units in a project.
*/

/* The following defines will control the sizes of intermediate character 
 * strings that VectorCAST uses to output coverage data. Keeping these 
 * values close to the sizes needed results in less memory being used 
 * on the stack, during test execution.
 */
#define VCAST_STATEMENT_ID_BUF_SIZE 5
#define VCAST_SUBPROGRAM_ID_BUF_SIZE 5
#define VCAST_UNIT_ID_BUF_SIZE 5

/* The user can never go above the maximum of the supported 32 bit 
   limit, which is 4,294,967,296. */
#define VCAST_MCDC_ID_BUF_SIZE 11

/* The amount of space needed to write a line of statement data is,
 * The size of the unit id, plus a space, plus the size of the 
 * subprogram id, plus a space, plus the size of the statement id,
 * plus the NUL character.
 */
#define VCAST_STATEMENT_DATA_BUF_SIZE 18

/* The branch data is the same as the statement data, except it has
 * an extra space, and then an extra character (T or F) at the end. 
 * The statement case takes care of the NUL char.
 */
#define VCAST_BRANCH_DATA_BUF_SIZE 20

/* The mcdc data is the same as the statement data, except that
 * it has an extra space, plus a mcdc bits number, plus a space,
 * plus an mcdc bits used number.
 * The statement case takes care of the NUL char.
 */
#define VCAST_MCDC_DATA_BUF_SIZE 42

void
VCAST_WRITE_STATEMENT_DATA (short unit, short sub, int statement)
{
   char vcast_out_str[VCAST_STATEMENT_DATA_BUF_SIZE];
   char sub_buf[VCAST_SUBPROGRAM_ID_BUF_SIZE];
   char statement_buf[VCAST_STATEMENT_ID_BUF_SIZE];
   VC_INT_TO_STRING (vcast_out_str, unit);
   VC_INT_TO_STRING (sub_buf, sub);
   VC_strcat (vcast_out_str, " ");
   VC_strcat (vcast_out_str, sub_buf);
   VC_INT_TO_STRING (statement_buf, statement);
   VC_strcat (vcast_out_str, " ");
   VC_strcat (vcast_out_str, statement_buf);
   VCAST_WRITE_COVERAGE_DATA (vcast_out_str);
}

void
VCAST_WRITE_BRANCH_DATA (short unit, short sub, int statement, const char *TorF)
{
   char vcast_out_str[VCAST_BRANCH_DATA_BUF_SIZE];
   char sub_buf[VCAST_SUBPROGRAM_ID_BUF_SIZE];
   char statement_buf[VCAST_STATEMENT_ID_BUF_SIZE];
   VC_INT_TO_STRING (vcast_out_str, unit);
   VC_INT_TO_STRING (sub_buf, sub);
   VC_strcat (vcast_out_str, " ");
   VC_strcat (vcast_out_str, sub_buf);
   VC_INT_TO_STRING (statement_buf, statement);
   VC_strcat (vcast_out_str, " ");
   VC_strcat (vcast_out_str, statement_buf);
   VC_strcat (vcast_out_str, " ");
   VC_strcat (vcast_out_str, TorF);
   VCAST_WRITE_COVERAGE_DATA (vcast_out_str);
}

void
VCAST_WRITE_MCDC_DATA (short unit, short sub, int statement, 
      unsigned long mcdc_bits, unsigned long mcdc_bits_used)
{
   char vcast_out_str[VCAST_MCDC_DATA_BUF_SIZE];
   char sub_buf[VCAST_SUBPROGRAM_ID_BUF_SIZE];
   char statement_buf[VCAST_STATEMENT_ID_BUF_SIZE]; 
   char mcdc_bits_buf[VCAST_MCDC_ID_BUF_SIZE]; 
   char mcdc_bits_used_buf[VCAST_MCDC_ID_BUF_SIZE];
   VC_INT_TO_STRING (vcast_out_str, unit);
   VC_INT_TO_STRING (sub_buf, sub);
   VC_strcat (vcast_out_str, " ");
   VC_strcat (vcast_out_str, sub_buf);
   VC_INT_TO_STRING (statement_buf, statement);
   VC_strcat (vcast_out_str, " ");
   VC_strcat (vcast_out_str, statement_buf);
   VC_INT_TO_STRING (mcdc_bits_buf, mcdc_bits);
   VC_strcat (vcast_out_str, " ");
   VC_strcat (vcast_out_str, mcdc_bits_buf);
   VC_INT_TO_STRING (mcdc_bits_used_buf, mcdc_bits_used);
   VC_strcat (vcast_out_str, " ");
   VC_strcat (vcast_out_str, mcdc_bits_used_buf);
   VCAST_WRITE_COVERAGE_DATA (vcast_out_str);
}

void
fatalError (const char *msg)
{
  VCAST_WRITE_COVERAGE_DATA_FLUSH (msg);
}

#ifdef VCAST_USE_STATIC_MEMORY
#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A)
struct vcast_mcdc_statement mcdc_statement_pool[VCAST_MAX_MCDC_STATEMENTS];
struct vcast_mcdc_statement *mcdc_statement_pool_ptr = mcdc_statement_pool;
struct vcast_mcdc_statement *mcdc_statement_end = mcdc_statement_pool + VCAST_MAX_MCDC_STATEMENTS;
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A) */
static int vcast_max_covered_subprograms_exceeded = 0;
#endif /* VCAST_USE_STATIC_MEMORY */

#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A)
static int vcast_max_mcdc_statements_exceeded = 0;
struct vcast_mcdc_statement *
get_mcdc_statement (void)
{
  struct vcast_mcdc_statement *ptr;
#ifdef VCAST_USE_STATIC_MEMORY
  if (mcdc_statement_pool_ptr == mcdc_statement_end)
  {
    if (vcast_max_mcdc_statements_exceeded == 0)
	   fatalError ("VCAST_MAX_MCDC_STATEMENTS exceeded!");
    vcast_max_mcdc_statements_exceeded = 1;
    return NULL;
  }
  ptr = mcdc_statement_pool_ptr++;
#else /* VCAST_USE_STATIC_MEMORY */
  ptr = (struct vcast_mcdc_statement*) 
    VCAST_malloc (sizeof (struct vcast_mcdc_statement));
#endif /* VCAST_USE_STATIC_MEMORY */

  return ptr;
}

int
mcdc_statement_compare (struct vcast_mcdc_statement *ptr1,
                        struct vcast_mcdc_statement *ptr2)
{
  if (ptr1->mcdc_bits == ptr2->mcdc_bits)
  {
    if (ptr1->mcdc_bits_used == ptr2->mcdc_bits_used)
      return 0;
    else if (ptr1->mcdc_bits_used < ptr2->mcdc_bits_used)
      return -1;
    else
      return 1;
  } else if (ptr1->mcdc_bits < ptr2->mcdc_bits)
    return -1;
  else
    return 1;
}
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A) */

#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A)
VCAST_CONDITION_TYP
VCAST_SAVE_MCDC_SUBCONDITION (struct vcast_subprogram_coverage *coverage,
                              int bit_index, VCAST_CONDITION_TYP condition)
{
   /* This is a sub-condition for an MCDC expression, just record the bit */
  if (condition)
    (((struct vcast_mcdc_coverage*) coverage->coverage_ptr)->cur_vcast_mcdc_statement).mcdc_bits |= (1<<bit_index);

  (((struct vcast_mcdc_coverage*) coverage->coverage_ptr)->cur_vcast_mcdc_statement).mcdc_bits_used |= (1<<bit_index);

  return condition;
}
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) */

#ifdef VCAST_COVERAGE_TYPE_LEVEL_A
VCAST_CONDITION_TYP
VCAST_SAVE_MCDC_LEVEL_A_SUBCONDITION ( 
                              struct vcast_subprogram_coverage *coverage,
                              int bit_index, VCAST_CONDITION_TYP condition)
{
   /* This is a sub-condition for an MCDC expression, just record the bit */
  if (condition)
    (((struct vcast_levela_coverage*) coverage->coverage_ptr)->mcdc_coverage->cur_vcast_mcdc_statement).mcdc_bits |= (1<<bit_index);

  (((struct vcast_levela_coverage*) coverage->coverage_ptr)->mcdc_coverage->cur_vcast_mcdc_statement).mcdc_bits_used |= (1<<bit_index);

  return condition;
}

#endif /* VCAST_COVERAGE_TYPE_LEVEL_A */

#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A)
/* MCDC Global vars */
AVLTree *vcast_mcdc_data_ptr;
vcast_mcdc_statement_ptr vcast_cur_mcdc_statement_ptr;
struct vcast_mcdc_statement *vcast_mcdc_statement;
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A) */

#if defined(VCAST_COVERAGE_IO_REAL_TIME)
#ifdef VCAST_COVERAGE_TYPE_STATEMENT
void VCAST_SAVE_STATEMENT_REALTIME(struct vcast_subprogram_coverage *coverage, 
                             int statement)
{
   int index = (statement >> 3), shift = statement % 8;
   char *coverage_bits =
     ((struct vcast_statement_coverage*)coverage->coverage_ptr)->coverage_bits;

   /* JV always tag the line as executed for speed */
   coverage_bits[index] |= (1 << shift);
/*
   if ((coverage_bits[index] & (1 << shift)) == 0)
   {
     coverage_bits[index] |= (1 << shift);

     VCAST_WRITE_STATEMENT_DATA (coverage->vcast_unit_id,
         coverage->vcast_subprogram_id, statement+1);
   }
*/
}

#endif /* VCAST_COVERAGE_TYPE_STATEMENT */

#ifdef VCAST_COVERAGE_TYPE_BRANCH
VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_CONDITION_REALTIME(struct vcast_subprogram_coverage *coverage,
                               int statement,
                               VCAST_CONDITION_TYP condition)
{
   int index = (statement >> 3), shift = statement % 8;
   char *coverage_bits;

   if (condition)
     coverage_bits = ((struct vcast_branch_coverage*)coverage->coverage_ptr)->branch_bits_true;
   else
     coverage_bits = ((struct vcast_branch_coverage*)coverage->coverage_ptr)->branch_bits_false;

   if ((coverage_bits[index] & (1 << shift)) == 0)
   {
     coverage_bits[index] |= (1 << shift);

     VCAST_WRITE_BRANCH_DATA (coverage->vcast_unit_id, 
         coverage->vcast_subprogram_id, statement, 
         condition ? "T" : "F");
   }
 
  return condition;
}
#endif /* VCAST_COVERAGE_TYPE_BRANCH */

#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A)
VCAST_CONDITION_TYP 
VCAST_SAVE_MCDC_CONDITION_REALTIME(struct vcast_subprogram_coverage *coverage,
                             int statement, VCAST_CONDITION_TYP condition)
{
   vcast_mcdc_data_ptr = 
     ((struct vcast_mcdc_coverage*) coverage->coverage_ptr)->mcdc_data;
   vcast_cur_mcdc_statement_ptr =
     &(((struct vcast_mcdc_coverage*) coverage->coverage_ptr)->cur_vcast_mcdc_statement);

   /* Store the total expression */
   if (condition)
     vcast_cur_mcdc_statement_ptr->mcdc_bits |= 1;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used |= 1;

   /* Get the coverage data necessary */
   if (!find (vcast_cur_mcdc_statement_ptr, vcast_mcdc_data_ptr[statement]))
   {
      vcast_mcdc_statement = get_mcdc_statement ();
      if (!vcast_mcdc_statement)
        return condition;
      *vcast_mcdc_statement = *vcast_cur_mcdc_statement_ptr;

      vcast_mcdc_data_ptr[statement] = 
         insert (vcast_mcdc_statement, vcast_mcdc_data_ptr[statement]);

      VCAST_WRITE_MCDC_DATA (coverage->vcast_unit_id, 
            coverage->vcast_subprogram_id, statement,
            vcast_cur_mcdc_statement_ptr->mcdc_bits,
            vcast_cur_mcdc_statement_ptr->mcdc_bits_used);
   }

   vcast_cur_mcdc_statement_ptr->mcdc_bits = 0;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used = 0;

   return condition;
}
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) */

#ifdef VCAST_COVERAGE_TYPE_LEVEL_A
void VCAST_SAVE_STATEMENT_LEVEL_A_REALTIME(
                             struct vcast_subprogram_coverage *coverage, 
                             int statement)
{
   int index = (statement >> 3), shift = statement % 8;
   char *coverage_bits =
     ((struct vcast_levela_coverage*)coverage->coverage_ptr)->statement_coverage->coverage_bits;

   if ((coverage_bits[index] & (1 << shift)) == 0)
   {
     coverage_bits[index] |= (1 << shift);

     VCAST_WRITE_STATEMENT_DATA (coverage->vcast_unit_id,
       coverage->vcast_subprogram_id, statement+1);
   }
}

VCAST_CONDITION_TYP 
VCAST_SAVE_MCDC_LEVEL_A_CONDITION_REALTIME( 
                             struct vcast_subprogram_coverage *coverage,
                             int statement, VCAST_CONDITION_TYP condition)
{
   vcast_mcdc_data_ptr = 
     ((struct vcast_levela_coverage*) coverage->coverage_ptr)->mcdc_coverage->mcdc_data;
   vcast_cur_mcdc_statement_ptr =
     &(((struct vcast_levela_coverage*) coverage->coverage_ptr)->mcdc_coverage->cur_vcast_mcdc_statement);

   /* Store the total expression */
   if (condition)
     vcast_cur_mcdc_statement_ptr->mcdc_bits |= 1;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used |= 1;

   /* Get the coverage data necessary */
   if (!find (vcast_cur_mcdc_statement_ptr, vcast_mcdc_data_ptr[statement]))
   {
      vcast_mcdc_statement = get_mcdc_statement ();
      if (!vcast_mcdc_statement)
        return condition;
      *vcast_mcdc_statement = *vcast_cur_mcdc_statement_ptr;

      vcast_mcdc_data_ptr[statement] = 
         insert (vcast_mcdc_statement, vcast_mcdc_data_ptr[statement]);

      VCAST_WRITE_MCDC_DATA (coverage->vcast_unit_id, 
            coverage->vcast_subprogram_id, statement,
            vcast_cur_mcdc_statement_ptr->mcdc_bits,
            vcast_cur_mcdc_statement_ptr->mcdc_bits_used);
   }

   vcast_cur_mcdc_statement_ptr->mcdc_bits = 0;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used = 0;

   return condition;
}
#endif /* VCAST_COVERAGE_TYPE_LEVEL_A */

#ifdef VCAST_COVERAGE_TYPE_LEVEL_B
void VCAST_SAVE_STATEMENT_LEVEL_B_REALTIME(
                             struct vcast_subprogram_coverage *coverage, 
                             int statement)
{
   int index = (statement >> 3), shift = statement % 8;
   char *coverage_bits =
     ((struct vcast_levelb_coverage*)coverage->coverage_ptr)->statement_coverage->coverage_bits;

   if ((coverage_bits[index] & (1 << shift)) == 0)
   {
     coverage_bits[index] |= (1 << shift);

     VCAST_WRITE_STATEMENT_DATA (coverage->vcast_unit_id,
       coverage->vcast_subprogram_id, statement+1);
   }
}

VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_LEVEL_B_CONDITION_REALTIME(
                               struct vcast_subprogram_coverage *coverage,
                               int statement,
                               VCAST_CONDITION_TYP condition)
{
   int index = (statement >> 3), shift = statement % 8;
   char *coverage_bits; 

   if (condition)
     coverage_bits = (((struct vcast_levelb_coverage*)coverage->coverage_ptr)->branch_coverage)->branch_bits_true;
   else
     coverage_bits = (((struct vcast_levelb_coverage*)coverage->coverage_ptr)->branch_coverage)->branch_bits_false;

   if ((coverage_bits[index] & (1 << shift)) == 0)
   {
     coverage_bits[index] |= (1 << shift);

     VCAST_WRITE_BRANCH_DATA (coverage->vcast_unit_id, 
         coverage->vcast_subprogram_id, statement, 
         condition ? "T" : "F");
   }
 
  return condition;
}

#endif /* VCAST_COVERAGE_TYPE_LEVEL_B */
#endif /* VCAST_COVERAGE_IO_REAL_TIME */

#if defined(VCAST_COVERAGE_IO_ANIMATION)
#ifdef VCAST_COVERAGE_TYPE_STATEMENT
void VCAST_SAVE_STATEMENT_ANIMATION(struct vcast_subprogram_coverage *coverage, 
                             int statement)
{
  VCAST_WRITE_STATEMENT_DATA (coverage->vcast_unit_id,
    coverage->vcast_subprogram_id, statement+1);
}
#endif /* VCAST_COVERAGE_TYPE_STATEMENT */

#ifdef VCAST_COVERAGE_TYPE_BRANCH
VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_CONDITION_ANIMATION(struct vcast_subprogram_coverage *coverage,
                               int statement,
                               VCAST_CONDITION_TYP condition)
{
  VCAST_WRITE_BRANCH_DATA (coverage->vcast_unit_id, 
      coverage->vcast_subprogram_id, statement, 
      condition ? "T" : "F");
 
  return condition;
}
#endif /* VCAST_COVERAGE_TYPE_BRANCH */

#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A)
VCAST_CONDITION_TYP 
VCAST_SAVE_MCDC_CONDITION_ANIMATION(struct vcast_subprogram_coverage *coverage,
                             int statement, VCAST_CONDITION_TYP condition)
{
   vcast_cur_mcdc_statement_ptr =
     &(((struct vcast_mcdc_coverage*) coverage->coverage_ptr)->cur_vcast_mcdc_statement);

   /* Store the total expression */
   if (condition)
     vcast_cur_mcdc_statement_ptr->mcdc_bits |= 1;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used |= 1;

   VCAST_WRITE_MCDC_DATA (coverage->vcast_unit_id, 
         coverage->vcast_subprogram_id, statement,
         vcast_cur_mcdc_statement_ptr->mcdc_bits,
         vcast_cur_mcdc_statement_ptr->mcdc_bits_used);
   
   vcast_cur_mcdc_statement_ptr->mcdc_bits = 0;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used = 0;

   return condition;
}
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) */

#ifdef VCAST_COVERAGE_TYPE_LEVEL_A
void VCAST_SAVE_STATEMENT_LEVEL_A_ANIMATION(
                             struct vcast_subprogram_coverage *coverage, 
                             int statement)
{
  VCAST_WRITE_STATEMENT_DATA (coverage->vcast_unit_id,
    coverage->vcast_subprogram_id, statement+1);
}

VCAST_CONDITION_TYP
VCAST_SAVE_MCDC_LEVEL_A_CONDITION_ANIMATION(
                             struct vcast_subprogram_coverage *coverage,
                             int statement, VCAST_CONDITION_TYP condition)
{
   vcast_mcdc_data_ptr = 
     ((struct vcast_levela_coverage*) coverage->coverage_ptr)->mcdc_coverage->mcdc_data;
   vcast_cur_mcdc_statement_ptr =
     &(((struct vcast_levela_coverage*) coverage->coverage_ptr)->mcdc_coverage->cur_vcast_mcdc_statement);

   /* Store the total expression */
   if (condition)
     vcast_cur_mcdc_statement_ptr->mcdc_bits |= 1;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used |= 1;

   VCAST_WRITE_MCDC_DATA (coverage->vcast_unit_id, 
         coverage->vcast_subprogram_id, statement,
         vcast_cur_mcdc_statement_ptr->mcdc_bits,
         vcast_cur_mcdc_statement_ptr->mcdc_bits_used);

   vcast_cur_mcdc_statement_ptr->mcdc_bits = 0;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used = 0;

   return condition;
}
#endif /* VCAST_COVERAGE_TYPE_LEVEL_A */

#ifdef VCAST_COVERAGE_TYPE_LEVEL_B
void VCAST_SAVE_STATEMENT_LEVEL_B_ANIMATION(
                             struct vcast_subprogram_coverage *coverage, 
                             int statement)
{
  VCAST_WRITE_STATEMENT_DATA (coverage->vcast_unit_id,
    coverage->vcast_subprogram_id, statement+1);
}

VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_LEVEL_B_CONDITION_ANIMATION(
                               struct vcast_subprogram_coverage *coverage,
                               int statement,
                               VCAST_CONDITION_TYP condition)
{
  VCAST_WRITE_BRANCH_DATA (coverage->vcast_unit_id, 
      coverage->vcast_subprogram_id, statement, 
      condition ? "T" : "F");
 
  return condition;
}
#endif /* VCAST_COVERAGE_TYPE_LEVEL_B */
#endif /* VCAST_COVERAGE_IO_ANIMATION */


#if defined(VCAST_COVERAGE_IO_BUFFERED)
#ifdef VCAST_COVERAGE_TYPE_BRANCH
VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_CONDITION_BUFFERED(struct vcast_subprogram_coverage *coverage,
                            int statement,
                            VCAST_CONDITION_TYP condition)
{
   int index = (statement >> 3), shift = statement % 8;

   if (condition)
     ((struct vcast_branch_coverage*)coverage->coverage_ptr)->branch_bits_true[index] |= (1 << shift);
   else
     ((struct vcast_branch_coverage*)coverage->coverage_ptr)->branch_bits_false[index] |= (1 << shift);

   return condition;
}
#endif /* VCAST_COVERAGE_TYPE_BRANCH */

#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A)
VCAST_CONDITION_TYP 
VCAST_SAVE_MCDC_CONDITION_BUFFERED(struct vcast_subprogram_coverage *coverage,
                          int statement, VCAST_CONDITION_TYP condition)
{
   vcast_mcdc_data_ptr = 
     ((struct vcast_mcdc_coverage*) coverage->coverage_ptr)->mcdc_data;
   vcast_cur_mcdc_statement_ptr =
     &(((struct vcast_mcdc_coverage*) coverage->coverage_ptr)->cur_vcast_mcdc_statement);

   /* Store the total expression */
   if (condition)
     vcast_cur_mcdc_statement_ptr->mcdc_bits |= 1;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used |= 1;

   /* Get the coverage data necessary */
   if (!find (vcast_cur_mcdc_statement_ptr, vcast_mcdc_data_ptr[statement]))
   {
      vcast_mcdc_statement = get_mcdc_statement ();
      if (!vcast_mcdc_statement)
        return condition;
      *vcast_mcdc_statement = *vcast_cur_mcdc_statement_ptr;

      vcast_mcdc_data_ptr[statement] = 
         insert (vcast_mcdc_statement, vcast_mcdc_data_ptr[statement]);
   }

   vcast_cur_mcdc_statement_ptr->mcdc_bits = 0;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used = 0;

   return condition;
}
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) */

#ifdef VCAST_COVERAGE_TYPE_LEVEL_A
VCAST_CONDITION_TYP 
VCAST_SAVE_MCDC_LEVEL_A_CONDITION_BUFFERED(
                          struct vcast_subprogram_coverage *coverage,
                          int statement, VCAST_CONDITION_TYP condition)
{
   vcast_mcdc_data_ptr =
     ((struct vcast_levela_coverage*) coverage->coverage_ptr)->mcdc_coverage->mcdc_data;
   vcast_cur_mcdc_statement_ptr =
     &(((struct vcast_levela_coverage*) coverage->coverage_ptr)->mcdc_coverage->cur_vcast_mcdc_statement);

   /* Store the total expression */
   if (condition)
     vcast_cur_mcdc_statement_ptr->mcdc_bits |= 1;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used |= 1;

   /* Get the coverage data necessary */
   if (!find (vcast_cur_mcdc_statement_ptr, vcast_mcdc_data_ptr[statement]))
   {
      vcast_mcdc_statement = get_mcdc_statement ();
      if (!vcast_mcdc_statement)
        return condition;
      *vcast_mcdc_statement = *vcast_cur_mcdc_statement_ptr;

      vcast_mcdc_data_ptr[statement] = 
         insert (vcast_mcdc_statement, vcast_mcdc_data_ptr[statement]);
   }

   vcast_cur_mcdc_statement_ptr->mcdc_bits = 0;
   vcast_cur_mcdc_statement_ptr->mcdc_bits_used = 0;

   return condition;
}
#endif /* VCAST_COVERAGE_TYPE_LEVEL_A */

#ifdef VCAST_COVERAGE_TYPE_LEVEL_B
VCAST_CONDITION_TYP 
VCAST_SAVE_BRANCH_LEVEL_B_CONDITION_BUFFERED(
                            struct vcast_subprogram_coverage *coverage,
                            int statement,
                            VCAST_CONDITION_TYP condition)
{
   int index = (statement >> 3), shift = statement % 8;

   if (condition)
     (((struct vcast_levelb_coverage*)coverage->coverage_ptr)->branch_coverage)->branch_bits_true[index] |= (1 << shift);
   else
     (((struct vcast_levelb_coverage*)coverage->coverage_ptr)->branch_coverage)->branch_bits_false[index] |= (1 << shift);

   return condition;
}
#endif /* VCAST_COVERAGE_TYPE_LEVEL_B */
#endif /* VCAST_COVERAGE_IO_BUFFERED */

struct AVLNode
{
  VCAST_elementType element;
  AVLTree left;
  AVLTree right;
  int height;
};

#ifdef VCAST_USE_STATIC_MEMORY
#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A)
struct AVLNode avlnode_pool[VCAST_MAX_MCDC_STATEMENTS];
AVLTree avlnode_pool_ptr = avlnode_pool;
AVLTree avlnode_pool_end = avlnode_pool + VCAST_MAX_MCDC_STATEMENTS;
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A) */
#endif /* VCAST_USE_STATIC_MEMORY */

#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A)
AVLTree
getAVLTree (void)
{
  AVLTree ptr;
#ifdef VCAST_USE_STATIC_MEMORY
  if (avlnode_pool_ptr == avlnode_pool_end)
    return NULL;
  ptr = avlnode_pool_ptr++;
#else /* VCAST_USE_STATIC_MEMORY */
  ptr = (struct AVLNode *)VCAST_malloc (sizeof (struct AVLNode));
#endif /* VCAST_USE_STATIC_MEMORY */

  return ptr;
}

int vcast_find_val;
VCAST_position
find (VCAST_elementType VCAST_X, AVLTree VCAST_T)
{
  if (VCAST_T == NULL)
    return NULL;
  vcast_find_val = mcdc_statement_compare (VCAST_X, VCAST_T->element);
  if (vcast_find_val < 0)
    return find (VCAST_X, VCAST_T->left);
  else if (vcast_find_val > 0)
    return find (VCAST_X, VCAST_T->right);
  else
    return VCAST_T;
}

static int
height (VCAST_position P)
{
  if (P == NULL)
    return -1;
  else
    return P->height;
}

static int
avl_max (int Lhs, int Rhs)
{
  return Lhs > Rhs ? Lhs : Rhs;
}

	/* This function can be called only if K2 has a left child */
	/* Perform a rotate between a node (K2) and its left child */
	/* Update heights, then return new root */

static VCAST_position
singleRotateWithleft (VCAST_position K2)
{
  VCAST_position K1;

  K1 = K2->left;
  K2->left = K1->right;
  K1->right = K2;

  K2->height = avl_max (height (K2->left), height (K2->right)) + 1;
  K1->height = avl_max (height (K1->left), K2->height) + 1;

  return K1;			/* New root */
}

	/* This function can be called only if K1 has a right child */
	/* Perform a rotate between a node (K1) and its right child */
	/* Update heights, then return new root */

static VCAST_position
singleRotateWithright (VCAST_position K1)
{
  VCAST_position K2;

  K2 = K1->right;
  K1->right = K2->left;
  K2->left = K1;

  K1->height = avl_max (height (K1->left), height (K1->right)) + 1;
  K2->height = avl_max (height (K2->right), K1->height) + 1;

  return K2;			/* New root */
}

	/* This function can be called only if K3 has a left */
	/* child and K3's left child has a right child */
	/* Do the left-right double rotation */
	/* Update heights, then return new root */

static VCAST_position
doubleRotateWithleft (VCAST_position K3)
{
  /* Rotate between K1 and K2 */
  K3->left = singleRotateWithright (K3->left);

  /* Rotate between K3 and K2 */
  return singleRotateWithleft (K3);
}

	/* This function can be called only if K1 has a right */
	/* child and K1's right child has a left child */
	/* Do the right-left double rotation */
	/* Update heights, then return new root */

static VCAST_position
doubleRotateWithright (VCAST_position K1)
{
  /* Rotate between K3 and K2 */
  K1->right = singleRotateWithleft (K1->right);

  /* Rotate between K1 and K2 */
  return singleRotateWithright (K1);
}

int vcast_insert_val;
AVLTree
insert (VCAST_elementType VCAST_X, AVLTree VCAST_T)
{
  if (VCAST_T == NULL)
    {
      /* Create and return a one-node tree */
      VCAST_T = getAVLTree ();
      if (VCAST_T == NULL)
      {
        if (vcast_max_mcdc_statements_exceeded == 0)
	  fatalError ("VCAST_MAX_MCDC_STATEMENTS exceeded!");
        vcast_max_mcdc_statements_exceeded = 1;
      }
      else
	{
	  VCAST_T->element = VCAST_X;
	  VCAST_T->height = 0;
	  VCAST_T->left = VCAST_T->right = NULL;
	}
    }
  else if ((vcast_insert_val = mcdc_statement_compare (VCAST_X, VCAST_T->element)) < 0)
    {
      VCAST_T->left = insert (VCAST_X, VCAST_T->left);
      if (height (VCAST_T->left) - height (VCAST_T->right) == 2)
	{
	  if (mcdc_statement_compare (VCAST_X, VCAST_T->left->element) < 0)
	    VCAST_T = singleRotateWithleft (VCAST_T);
	  else
	    VCAST_T = doubleRotateWithleft (VCAST_T);
	}
    }
  else if (vcast_insert_val > 0)
    {
      VCAST_T->right = insert (VCAST_X, VCAST_T->right);
      if (height (VCAST_T->right) - height (VCAST_T->left) == 2)
	{
	  if (mcdc_statement_compare (VCAST_X, VCAST_T->right->element) > 0)
	    VCAST_T = singleRotateWithright (VCAST_T);
	  else
	    VCAST_T = doubleRotateWithright (VCAST_T);
	}
    }
  /* Else X is in the tree already; we'll do nothing */

  VCAST_T->height = avl_max (height (VCAST_T->left), height (VCAST_T->right)) + 1;
  return VCAST_T;
}
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A) */

/* All the coverage data necessary to capture 100% coverage 
 * This currently only works as a static array, but if needs be,
 * this could be replaced with data from the heap instead.
 */
#if defined (VCAST_COVERAGE_TYPE_STATEMENT) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) || defined (VCAST_COVERAGE_TYPE_LEVEL_B)
struct vcast_statement_coverage VCAST_ATTRIBUTE_CODE vcast_statement_coverage_data_pool[VCAST_NUMBER_STATEMENT_STRUCTS];
int vcast_statement_coverage_data_pos = 0;
#endif /* defined (VCAST_COVERAGE_TYPE_STATEMENT) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) || defined (VCAST_COVERAGE_TYPE_LEVEL_B) */

#if defined (VCAST_COVERAGE_TYPE_BRANCH) || defined (VCAST_COVERAGE_TYPE_LEVEL_B)
struct vcast_branch_coverage VCAST_ATTRIBUTE_CODE vcast_branch_coverage_data_pool[VCAST_NUMBER_BRANCH_STRUCTS];
int vcast_branch_coverage_data_pos = 0;
#endif /* defined (VCAST_COVERAGE_TYPE_BRANCH) || defined (VCAST_COVERAGE_TYPE_LEVEL_B) */

#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A)
struct vcast_mcdc_coverage VCAST_ATTRIBUTE_CODE vcast_mcdc_coverage_data_pool[VCAST_NUMBER_MCDC_STRUCTS];
int vcast_mcdc_coverage_data_pos = 0;
AVLTree VCAST_ATTRIBUTE_CODE vcast_avltree_data_pool[VCAST_NUMBER_AVLTREE_POINTERS];
int vcast_avltree_data_pos = 0;
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) */

#ifdef VCAST_COVERAGE_TYPE_LEVEL_A
struct vcast_levela_coverage VCAST_ATTRIBUTE_CODE vcast_levela_coverage_data_pool[VCAST_NUMBER_LEVEL_A_STRUCTS];
int vcast_levela_coverage_data_pos = 0;
#endif /* VCAST_COVERAGE_TYPE_LEVEL_A */

#ifdef VCAST_COVERAGE_TYPE_LEVEL_B
struct vcast_levelb_coverage VCAST_ATTRIBUTE_CODE vcast_levelb_coverage_data_pool[VCAST_NUMBER_LEVEL_B_STRUCTS];
int vcast_levelb_coverage_data_pos = 0;
#endif /* VCAST_COVERAGE_TYPE_LEVEL_B */

struct vcast_subprogram_coverage VCAST_ATTRIBUTE_CODE vcast_subprogram_coverage_data_pool[VCAST_NUMBER_SUBPROGRAM_STRUCTS];
int vcast_subprogram_coverage_data_pos = 0;

#if defined (VCAST_COVERAGE_TYPE_BRANCH) || defined (VCAST_COVERAGE_TYPE_LEVEL_B)
char VCAST_ATTRIBUTE_CODE vcast_binary_coverage_data_pool[VCAST_NUMBER_BINARY_BYTES];
int vcast_binary_coverage_data_pos = 0;
#endif /* defined (VCAST_COVERAGE_TYPE_BRANCH) || defined (VCAST_COVERAGE_TYPE_LEVEL_B) */

#ifdef VCAST_USE_STATIC_MEMORY
/* A staticall defined representation */
/* All the coverage data necessary to dump all covered subprograms */
struct vcast_subprogram_coverage VCAST_ATTRIBUTE_CODE *subprogram_coverage_pool[VCAST_NUMBER_SUBPROGRAM_STRUCTS];
int vcast_subprogram_coverage_pool_pos = 0;

#else /* VCAST_USE_STATIC_MEMORY */
struct vcast_subprog_list_ptr {
  struct vcast_subprogram_coverage *vcast_data;
  struct vcast_subprog_list_ptr *vcast_next;
} *vcast_subprog_root = 0, *vcast_subprog_cur = 0;

void VCAST_APPEND_SUBPROG_LIST_PTR (struct vcast_subprogram_coverage *coverage)
{
  struct vcast_subprog_list_ptr *vcast_new = (struct vcast_subprog_list_ptr*)
    VCAST_malloc (sizeof (struct vcast_subprog_list_ptr));
  vcast_new->vcast_data = coverage;
  vcast_new->vcast_next = NULL;
  if (vcast_subprog_root)
  {
    vcast_subprog_cur->vcast_next = vcast_new;
    vcast_subprog_cur = vcast_subprog_cur->vcast_next;
  }
  else
  {
    vcast_subprog_root = vcast_new;
    vcast_subprog_cur = vcast_new;
  }
}
#endif /* VCAST_USE_STATIC_MEMORY */

void 
VCAST_REGISTER_SUBPROGRAM (struct vcast_subprogram_coverage *coverage)
{
#ifdef VCAST_USE_STATIC_MEMORY
  if (vcast_subprogram_coverage_pool_pos >= VCAST_MAX_COVERED_SUBPROGRAMS)
  {
    if (vcast_max_covered_subprograms_exceeded == 0)
      fatalError ("VCAST_MAX_COVERED_SUBPROGRAMS exceeded!");
    vcast_max_covered_subprograms_exceeded = 1;
    return;
  }
  subprogram_coverage_pool[vcast_subprogram_coverage_pool_pos++] = coverage;
#else /* VCAST_USE_STATIC_MEMORY */
  VCAST_APPEND_SUBPROG_LIST_PTR (coverage);
#endif /* VCAST_USE_STATIC_MEMORY */
}

#if defined VCAST_COVERAGE_TYPE_STATEMENT
/* PWES */
void VCAST_REGISTER_STATEMENT (
   struct vcast_subprogram_coverage **cov, 
   char *vcast_bits_statement,
   int vcast_unit_id,
   int vcast_subprogram_id,
   int vcast_size)
{
  struct vcast_statement_coverage *statement_data;
  int vcast_num_bytes = (vcast_size%8 == 0)?vcast_size/8:vcast_size/8+1;
  int vcast_cur;

  for (vcast_cur = 0; vcast_cur < vcast_num_bytes; ++vcast_cur) {
     vcast_bits_statement[vcast_cur] = 0;
  }

  statement_data = &vcast_statement_coverage_data_pool[vcast_statement_coverage_data_pos++];
  statement_data->coverage_bits = vcast_bits_statement;
  statement_data->vcast_num_statement_statements = vcast_size;
#if defined(VCAST_COVERAGE_IO_BUFFERED)
  statement_data->coverage_bits[0] |= 1;
#endif /* VCAST_COVERAGE_IO_BUFFERED */

  *cov = &vcast_subprogram_coverage_data_pool[vcast_subprogram_coverage_data_pos++];
  (*cov)->vcast_unit_id = vcast_unit_id;
  (*cov)->vcast_subprogram_id = vcast_subprogram_id;
  (*cov)->coverage_kind = VCAST_COVERAGE_STATEMENT;
  (*cov)->coverage_ptr = statement_data;

  VCAST_REGISTER_SUBPROGRAM (*cov);
}
#endif /* VCAST_COVERAGE_TYPE_STATEMENT */

#if defined (VCAST_COVERAGE_TYPE_BRANCH)
void VCAST_REGISTER_BRANCH (
   struct vcast_subprogram_coverage **cov, 
   int vcast_unit_id,
   int vcast_subprogram_id,
   int vcast_size)
{
  int vcast_num_bytes, vcast_cur;
  struct vcast_branch_coverage *branch_data;

  vcast_num_bytes = (vcast_size%8 == 0)?vcast_size/8:vcast_size/8+1;

  branch_data = &vcast_branch_coverage_data_pool[vcast_branch_coverage_data_pos++];
  branch_data->branch_bits_true = &vcast_binary_coverage_data_pool[vcast_binary_coverage_data_pos];
  branch_data->branch_bits_false = &vcast_binary_coverage_data_pool[vcast_binary_coverage_data_pos + vcast_num_bytes];
  vcast_binary_coverage_data_pos += 2*vcast_num_bytes;

  for (vcast_cur = 0; vcast_cur < vcast_num_bytes; ++vcast_cur) {
     branch_data->branch_bits_true[vcast_cur] = 0;
     branch_data->branch_bits_false[vcast_cur] = 0;
  }

  branch_data->vcast_num_branch_statements = vcast_size;
#if defined(VCAST_COVERAGE_IO_BUFFERED)
  branch_data->branch_bits_true[0] |= 1;
#endif /* VCAST_COVERAGE_IO_BUFFERED */

  *cov = &vcast_subprogram_coverage_data_pool[vcast_subprogram_coverage_data_pos++];
  (*cov)->vcast_unit_id = vcast_unit_id;
  (*cov)->vcast_subprogram_id = vcast_subprogram_id;
  (*cov)->coverage_kind = VCAST_COVERAGE_BRANCH;
  (*cov)->coverage_ptr = branch_data;

  VCAST_REGISTER_SUBPROGRAM (*cov);
}
#endif /* VCAST_COVERAGE_TYPE_STATEMENT */

#if defined (VCAST_COVERAGE_TYPE_MCDC)
void VCAST_REGISTER_MCDC (
   struct vcast_subprogram_coverage **cov, 
   int vcast_unit_id,
   int vcast_subprogram_id,
   int vcast_size)
{
  struct vcast_mcdc_coverage *mcdc_data;
  int vcast_cur;

  mcdc_data = &vcast_mcdc_coverage_data_pool[vcast_mcdc_coverage_data_pos++];
  mcdc_data->mcdc_data = &vcast_avltree_data_pool[vcast_avltree_data_pos];
  vcast_avltree_data_pos += vcast_size;
  mcdc_data->vcast_num_mcdc_statements = vcast_size;
  mcdc_data->cur_vcast_mcdc_statement.mcdc_bits = 0;
  mcdc_data->cur_vcast_mcdc_statement.mcdc_bits_used = 0;
  for (vcast_cur = 0; vcast_cur < vcast_size; ++vcast_cur)
    mcdc_data->mcdc_data[vcast_cur] = 0;

  *cov = &vcast_subprogram_coverage_data_pool[vcast_subprogram_coverage_data_pos++];
  (*cov)->vcast_unit_id = vcast_unit_id;
  (*cov)->vcast_subprogram_id = vcast_subprogram_id;
  (*cov)->coverage_kind = VCAST_COVERAGE_MCDC;
  (*cov)->coverage_ptr = mcdc_data;

  VCAST_REGISTER_SUBPROGRAM (*cov);

#if defined(VCAST_COVERAGE_IO_BUFFERED)
  VCAST_SAVE_MCDC_CONDITION_BUFFERED (*cov, 0, 1);
#endif /* VCAST_COVERAGE_IO_BUFFERED */
}
#endif /* VCAST_COVERAGE_TYPE_MCDC */

#if defined (VCAST_COVERAGE_TYPE_LEVEL_A)
void VCAST_REGISTER_LEVEL_A (
   struct vcast_subprogram_coverage **cov, 
   char *vcast_bits_statement,
   int vcast_unit_id,
   int vcast_subprogram_id,
   int vcast_size,
   int vcast_include_stmnt)
{
  struct vcast_statement_coverage *statement_data;
  struct vcast_mcdc_coverage *mcdc_data;
  struct vcast_levela_coverage *levela_data;
  int vcast_cur;
  int vcast_num_bytes = (vcast_size%8 == 0)?vcast_size/8:vcast_size/8+1;

  for (vcast_cur = 0; vcast_cur < vcast_num_bytes; ++vcast_cur) {
     vcast_bits_statement[vcast_cur] = 0;
  }

  statement_data = &vcast_statement_coverage_data_pool[vcast_statement_coverage_data_pos++];
  statement_data->coverage_bits = vcast_bits_statement;
  statement_data->vcast_num_statement_statements = vcast_size;
#if defined(VCAST_COVERAGE_IO_BUFFERED)
  if (vcast_include_stmnt)
    statement_data->coverage_bits[0] |= 1;
#endif /* VCAST_COVERAGE_IO_BUFFERED */

  mcdc_data = &vcast_mcdc_coverage_data_pool[vcast_mcdc_coverage_data_pos++];
  mcdc_data->mcdc_data = &vcast_avltree_data_pool[vcast_avltree_data_pos];
  vcast_avltree_data_pos += vcast_size;
  mcdc_data->vcast_num_mcdc_statements = vcast_size;
  mcdc_data->cur_vcast_mcdc_statement.mcdc_bits = 0;
  mcdc_data->cur_vcast_mcdc_statement.mcdc_bits_used = 0;
  for (vcast_cur = 0; vcast_cur < vcast_size; ++vcast_cur)
    mcdc_data->mcdc_data[vcast_cur] = 0;

  levela_data = &vcast_levela_coverage_data_pool[vcast_levela_coverage_data_pos++];
  levela_data->statement_coverage = statement_data;
  levela_data->mcdc_coverage = mcdc_data;

  *cov = &vcast_subprogram_coverage_data_pool[vcast_subprogram_coverage_data_pos++];
  (*cov)->vcast_unit_id = vcast_unit_id;
  (*cov)->vcast_subprogram_id = vcast_subprogram_id;
  (*cov)->coverage_kind = VCAST_COVERAGE_LEVEL_A;
  (*cov)->coverage_ptr = levela_data;

  VCAST_REGISTER_SUBPROGRAM (*cov);

#if defined(VCAST_COVERAGE_IO_BUFFERED)
  VCAST_SAVE_MCDC_LEVEL_A_CONDITION_BUFFERED (*cov, 0, 1);
#endif /* VCAST_COVERAGE_IO_BUFFERED */
}
#endif /* VCAST_COVERAGE_TYPE_LEVEL_A */

#if defined (VCAST_COVERAGE_TYPE_LEVEL_B)
void VCAST_REGISTER_LEVEL_B (
   struct vcast_subprogram_coverage **cov, 
   char *vcast_bits_statement,
   int vcast_unit_id,
   int vcast_subprogram_id,
   int vcast_size,
   int vcast_include_stmnt)
{
  int vcast_num_bytes, vcast_cur;
  struct vcast_statement_coverage *statement_data;
  struct vcast_branch_coverage *branch_data;
  struct vcast_levelb_coverage *levelb_data;

  vcast_num_bytes = (vcast_size%8 == 0)?vcast_size/8:vcast_size/8+1;
  for (vcast_cur = 0; vcast_cur < vcast_num_bytes; ++vcast_cur) {
     vcast_bits_statement[vcast_cur] = 0;
  }

  statement_data = &vcast_statement_coverage_data_pool[vcast_statement_coverage_data_pos++];
  statement_data->coverage_bits = vcast_bits_statement;
  statement_data->vcast_num_statement_statements = vcast_size;
#if defined(VCAST_COVERAGE_IO_BUFFERED)
  if (vcast_include_stmnt)
    statement_data->coverage_bits[0] |= 1;
#endif /* VCAST_COVERAGE_IO_BUFFERED */

  branch_data = &vcast_branch_coverage_data_pool[vcast_branch_coverage_data_pos++];
  branch_data->branch_bits_true = &vcast_binary_coverage_data_pool[vcast_binary_coverage_data_pos];
  branch_data->branch_bits_false = &vcast_binary_coverage_data_pool[vcast_binary_coverage_data_pos + vcast_num_bytes];
  vcast_binary_coverage_data_pos += 2*vcast_num_bytes;

  for (vcast_cur = 0; vcast_cur < vcast_num_bytes; ++vcast_cur) {
     branch_data->branch_bits_true[vcast_cur] = 0;
     branch_data->branch_bits_false[vcast_cur] = 0;
  }

  branch_data->vcast_num_branch_statements = vcast_size;
#if defined(VCAST_COVERAGE_IO_BUFFERED)
  branch_data->branch_bits_true[0] |= 1;
#endif /* VCAST_COVERAGE_IO_BUFFERED */

  levelb_data = &vcast_levelb_coverage_data_pool[vcast_levelb_coverage_data_pos++];
  levelb_data->statement_coverage = statement_data;
  levelb_data->branch_coverage = branch_data;

  *cov = &vcast_subprogram_coverage_data_pool[vcast_subprogram_coverage_data_pos++];
  (*cov)->vcast_unit_id = vcast_unit_id;
  (*cov)->vcast_subprogram_id = vcast_subprogram_id;
  (*cov)->coverage_kind = VCAST_COVERAGE_LEVEL_B;
  (*cov)->coverage_ptr = levelb_data;

  VCAST_REGISTER_SUBPROGRAM (*cov);
}
#endif /* VCAST_COVERAGE_TYPE_LEVEL_B */

#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A)
struct vcast_subprogram_coverage *vcast_dump_mcdc_coverage;
short vcast_dump_mcdc_statement;

void
VCAST_DUMP_MCDC_COVERAGE_DATA (AVLTree tree)
{
  /* Walk the tree, and output the data */
  if (tree)
  {
    VCAST_DUMP_MCDC_COVERAGE_DATA (tree->left);
    VCAST_WRITE_MCDC_DATA (
         vcast_dump_mcdc_coverage->vcast_unit_id, 
         vcast_dump_mcdc_coverage->vcast_subprogram_id, 
         vcast_dump_mcdc_statement,
         tree->element->mcdc_bits,
         tree->element->mcdc_bits_used);
    VCAST_DUMP_MCDC_COVERAGE_DATA (tree->right);
  }
}
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) */

#if defined (VCAST_COVERAGE_TYPE_STATEMENT) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) || defined (VCAST_COVERAGE_TYPE_LEVEL_B)
void
VCAST_DUMP_STATEMENT_COVERAGE (struct vcast_subprogram_coverage *cur)
{
   int vcast_i, index, shift;
   struct vcast_statement_coverage *statement_coverage;
   if (cur->coverage_kind == VCAST_COVERAGE_STATEMENT)
     statement_coverage = (struct vcast_statement_coverage*)cur->coverage_ptr;
   else if (cur->coverage_kind == VCAST_COVERAGE_LEVEL_A)
     statement_coverage = ((struct vcast_levela_coverage*)cur->coverage_ptr)->statement_coverage;
   else if (cur->coverage_kind == VCAST_COVERAGE_LEVEL_B)
     statement_coverage = ((struct vcast_levelb_coverage*)cur->coverage_ptr)->statement_coverage;

   for (vcast_i = 0; 
        vcast_i < statement_coverage->vcast_num_statement_statements;
        ++vcast_i)
   {
     index = (vcast_i >> 3);
     shift = vcast_i%8;
     if (statement_coverage->coverage_bits[index] & (1<<shift))
     {
       VCAST_WRITE_STATEMENT_DATA (cur->vcast_unit_id,
            cur->vcast_subprogram_id, vcast_i+1);
     }
   }
}
#endif /* defined (VCAST_COVERAGE_TYPE_STATEMENT) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) || defined (VCAST_COVERAGE_TYPE_LEVEL_B) */

#if defined (VCAST_COVERAGE_TYPE_BRANCH) || defined (VCAST_COVERAGE_TYPE_LEVEL_B)
void
VCAST_DUMP_BRANCH_COVERAGE (struct vcast_subprogram_coverage *cur)
{
   int vcast_i, index, shift;
   struct vcast_branch_coverage *branch_coverage;

   if (cur->coverage_kind == VCAST_COVERAGE_BRANCH)
     branch_coverage = ((struct vcast_branch_coverage*)cur->coverage_ptr);
   else if (cur->coverage_kind == VCAST_COVERAGE_LEVEL_B)
     branch_coverage = ((struct vcast_levelb_coverage*)cur->coverage_ptr)->branch_coverage;

   for (vcast_i = 0; 
        vcast_i < branch_coverage->vcast_num_branch_statements;
        ++vcast_i)
   {
     index = (vcast_i >> 3);
     shift = vcast_i%8;
     if (branch_coverage->branch_bits_true[index] & (1<<shift))
     {
       VCAST_WRITE_BRANCH_DATA (cur->vcast_unit_id, cur->vcast_subprogram_id,
         vcast_i, "T");
     }

     if (branch_coverage->branch_bits_false[index] & (1<<shift))
     {
       VCAST_WRITE_BRANCH_DATA (cur->vcast_unit_id, cur->vcast_subprogram_id,
         vcast_i, "F");
     }
   }
}
#endif /* defined (VCAST_COVERAGE_TYPE_BRANCH) || defined (VCAST_COVERAGE_TYPE_LEVEL_B) */

#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A)
void
VCAST_DUMP_MCDC_COVERAGE (struct vcast_subprogram_coverage *cur)
{
   int vcast_i;
   struct vcast_mcdc_coverage *mcdc_coverage;

   if (cur->coverage_kind == VCAST_COVERAGE_MCDC)
     mcdc_coverage = (struct vcast_mcdc_coverage*) cur->coverage_ptr;
   else if (cur->coverage_kind == VCAST_COVERAGE_LEVEL_A)
     mcdc_coverage = ((struct vcast_levela_coverage*) cur->coverage_ptr)->mcdc_coverage;
   
   for (vcast_i = 0; 
        vcast_i < mcdc_coverage->vcast_num_mcdc_statements; ++vcast_i)
   {
     vcast_dump_mcdc_coverage = cur;
     vcast_dump_mcdc_statement = vcast_i;
     VCAST_DUMP_MCDC_COVERAGE_DATA (mcdc_coverage->mcdc_data[vcast_i]);
   }
}
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) */

void 
VCAST_DUMP_COVERAGE_DATA (void)
{
#if defined(VCAST_COVERAGE_IO_BUFFERED)
  struct vcast_subprogram_coverage *cur;
#ifdef VCAST_USE_STATIC_MEMORY
  int vcast_i;
  for (vcast_i = 0; vcast_i < vcast_subprogram_coverage_pool_pos; ++vcast_i)
  {
    cur = subprogram_coverage_pool[vcast_i];
#else /* VCAST_USE_STATIC_MEMORY */
  struct vcast_subprog_list_ptr *vcast_cur;
  for (vcast_cur = vcast_subprog_root; vcast_cur != 0; vcast_cur = vcast_cur->vcast_next)
  {
    cur = vcast_cur->vcast_data;
#endif /* VCAST_USE_STATIC_MEMORY */

    switch (cur->coverage_kind)
    {
      case VCAST_COVERAGE_STATEMENT:
#ifdef VCAST_COVERAGE_TYPE_STATEMENT
        VCAST_DUMP_STATEMENT_COVERAGE (cur);
#endif /* VCAST_COVERAGE_TYPE_STATEMENT */
        break;
      case VCAST_COVERAGE_BRANCH:
#ifdef VCAST_COVERAGE_TYPE_BRANCH
        VCAST_DUMP_BRANCH_COVERAGE (cur);
#endif /* VCAST_COVERAGE_TYPE_BRANCH */
        break;
      case VCAST_COVERAGE_MCDC:
#ifdef VCAST_COVERAGE_TYPE_MCDC
        VCAST_DUMP_MCDC_COVERAGE (cur);
#endif /* VCAST_COVERAGE_TYPE_MCDC */
        break;
      case VCAST_COVERAGE_LEVEL_A:
#ifdef VCAST_COVERAGE_TYPE_LEVEL_A
        VCAST_DUMP_STATEMENT_COVERAGE (cur);
        VCAST_DUMP_MCDC_COVERAGE (cur);
#endif /* VCAST_COVERAGE_TYPE_LEVEL_A */
        break;
      case VCAST_COVERAGE_LEVEL_B:
#ifdef VCAST_COVERAGE_TYPE_LEVEL_B
        VCAST_DUMP_STATEMENT_COVERAGE (cur);
        VCAST_DUMP_BRANCH_COVERAGE (cur);
#endif /* VCAST_COVERAGE_TYPE_LEVEL_B */
        break;
    };
    
#ifdef VCAST_DUMP_CALLBACK
    VCAST_DUMP_CALLBACK ();
#endif /* VCAST_DUMP_CALLBACK */
  }
#endif /* VCAST_COVERAGE_IO_BUFFERED */
}

void
VCAST_INITIALIZE (void)
{
#ifdef VCAST_USE_BUFFERED_ASCII_DATA
  vcast_ascii_coverage_data_pos = 0;
#endif /* VCAST_USE_BUFFERED_ASCII_DATA */
  
#ifndef VCAST_USE_STDOUT
#ifndef VCAST_UNIT_TEST_TOOL
#ifdef VCAST_USE_STDIO_OPS
  vCAST_INST_FILE = NULL;
#endif /* VCAST_USE_STDIO_OPS */
#endif /* VCAST_UNIT_TEST_TOOL */
#endif /* VCAST_USE_STDOUT */
  
#ifdef VCAST_USE_STATIC_MEMORY
#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A)
  mcdc_statement_pool_ptr = mcdc_statement_pool;
  mcdc_statement_end = mcdc_statement_pool + VCAST_MAX_MCDC_STATEMENTS;
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A) */
  vcast_max_covered_subprograms_exceeded = 0;
#endif /* VCAST_USE_STATIC_MEMORY */
  
#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A)
  vcast_max_mcdc_statements_exceeded = 0;
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A) */
  
#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A)
  /* MCDC Global vars */
  vcast_mcdc_data_ptr = NULL;
  vcast_cur_mcdc_statement_ptr = NULL;
  vcast_mcdc_statement = NULL;
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A) */
  
#ifdef VCAST_USE_STATIC_MEMORY
#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A)
  avlnode_pool_ptr = avlnode_pool;
  avlnode_pool_end = avlnode_pool + VCAST_MAX_MCDC_STATEMENTS;
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A) */
#endif /* VCAST_USE_STATIC_MEMORY */
  
#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A)
  vcast_find_val = 0;
  vcast_insert_val = 0;
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined(VCAST_COVERAGE_TYPE_LEVEL_A) */
  
#if defined (VCAST_COVERAGE_TYPE_STATEMENT) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) || defined (VCAST_COVERAGE_TYPE_LEVEL_B)
  vcast_statement_coverage_data_pos = 0;
#endif /* defined (VCAST_COVERAGE_TYPE_STATEMENT) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) || defined (VCAST_COVERAGE_TYPE_LEVEL_B) */
  
#if defined (VCAST_COVERAGE_TYPE_BRANCH) || defined (VCAST_COVERAGE_TYPE_LEVEL_B)
  vcast_branch_coverage_data_pos = 0;
#endif /* defined (VCAST_COVERAGE_TYPE_BRANCH) || defined (VCAST_COVERAGE_TYPE_LEVEL_B) */
  
#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A)
  vcast_mcdc_coverage_data_pos = 0;
  vcast_avltree_data_pos = 0;
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) */
  
#ifdef VCAST_COVERAGE_TYPE_LEVEL_A
  vcast_levela_coverage_data_pos = 0;
#endif /* VCAST_COVERAGE_TYPE_LEVEL_A */
  
#ifdef VCAST_COVERAGE_TYPE_LEVEL_B
  vcast_levelb_coverage_data_pos = 0;
#endif /* VCAST_COVERAGE_TYPE_LEVEL_B */
  
  vcast_subprogram_coverage_data_pos = 0;
  
#if defined (VCAST_COVERAGE_TYPE_BRANCH) || defined (VCAST_COVERAGE_TYPE_LEVEL_B)
  vcast_binary_coverage_data_pos = 0;
#endif /* defined (VCAST_COVERAGE_TYPE_BRANCH) || defined (VCAST_COVERAGE_TYPE_LEVEL_B) */
  
#ifdef VCAST_USE_STATIC_MEMORY
  /* A statically defined representation */
  /* All the coverage data necessary to dump all covered subprograms */
  vcast_subprogram_coverage_pool_pos = 0;
#else /* VCAST_USE_STATIC_MEMORY */
  vcast_subprog_root = 0;
  vcast_subprog_cur = 0;
#endif /* VCAST_USE_STATIC_MEMORY */
  
#if defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A)
  vcast_dump_mcdc_statement = 0;
#endif /* defined (VCAST_COVERAGE_TYPE_MCDC) || defined (VCAST_COVERAGE_TYPE_LEVEL_A) */
}

//=================================== PWC Modifications =======================================

// K-Modem Constants (Knowlogic-Modem ... a cheap imitation of the good stuff)
#define PAYLOAD_SIZE 500
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

#pragma pack(1)
typedef struct {
    UINT32 header;
    UINT16 seq;
    UINT16 size;
    BYTE   payload[PAYLOAD_SIZE];
    UINT16 checksum;
} CoveragePacket;
#pragma pack()

CoveragePacket covData;
UINT16 buffered;
BOOLEAN cancel;

/*--------------------------------------------------------------------------------------------------
  Function: ReportCoverage
  This function is used to report coverage data at the end of a coverage run.
  
  Need to define VCAST_DUMP_CALLBACK as a KickWd to pulse the WD when we are dumping
  Coverage data from the shutdown function.

  The Monitor Task must be disabled when this is called so that we can respond directly to the 
  Coverage data receiver if the packets were transmitted successfully.
--------------------------------------------------------------------------------------------------*/
void ReportCoverage(void)
{
    memset( &covData, 0, sizeof( covData));
    buffered = 0;
    cancel = FALSE;

    VCAST_DUMP_COVERAGE_DATA();

    // indicate this is the last packet
    if (!cancel)
    {
        covData.seq = 0xfffe;
        SendPacket();
    }
}

//------------------------------------------------------------------------------
// send the coverage data out
void SendWait( const char* str)
{
    UINT16 cnt;
    UINT16 sent = 0;
    UINT16 size = strlen( str);
    UINT16 free = PAYLOAD_SIZE - buffered;

    while (size > 0 && !cancel)
    {
        free = PAYLOAD_SIZE - buffered;

        // send the lesser of free/size
        cnt = size < free ? size : free;
        memcpy( &covData.payload[buffered], &str[sent], cnt);

        // keep track of what we buffered
        sent += cnt;
        buffered += cnt;
        size -= cnt;

        // if we are full send it out
        if (buffered == PAYLOAD_SIZE)
        {
            SendPacket();

            // reset covData
            memset( &covData.payload, 0, sizeof( covData.payload));
            covData.size = 0;
            covData.checksum = 0;
            buffered = 0;
        }
    }
}

//------------------------------------------------------------------------------
// send a packet and verify it was received correctly
void SendPacket(void)
{
    BYTE*  data;
    UINT16 i;
    UINT16 size;
    UINT16 bytesSent = 0;
    INT16 rsp = -1;
    INT16 canCount = 0;

    // fill in packet
    covData.header = 0x48454144; // HEAD
    covData.seq += 1;
    covData.size = buffered;

    // compute the checksum, includes all but the checksum
    size = sizeof(covData);
    data = (BYTE*)&covData;
    covData.checksum = 0;
    for (i = 0; i < (size - sizeof(covData.checksum)); i++)
    {
        covData.checksum += *data;
        data++;
    }

    do {
        // clear the input buffer
        rsp = GSE_getc();
        while rsp != -1: rsp = GSE_getc();

        // send all the data
        UART_Transmit( GSE_UART_PORT, (INT8*)(&covData), size, &bytesSent);

        // While the GSE Tx reg is not empty (DNW)
        while ( !(MCF_PSC_SR(0) & MCF_PSC_SR_TXEMP_URERR))
        { 
            // kick the WD
            KickWd();
        }

        // Set Dir to Rx
        DIO_SetPin(GSE_TxEnb,  DIO_SetLow);

        rsp = -1;
        while (rsp == -1 && canCount < 2)
        {
            KickWd();
            rsp = GSE_getc();

            // ignore all but ACK/NAK/CAN
            if (rsp == ACK || rsp == NAK)
            {
                canCount = 0;
                break;
            }
            else if ( rsp == CAN)
            {
                rsp = -1;
                canCount++;
            }
            else
            {
                rsp = -1;
                canCount = 0;
            }
        }
    } while (rsp == NAK && canCount < 2);

    if (canCount == 2)
    {
        cancel = TRUE;
    }
}

//------------------------------------------------------------------------------
// Keep the WD alive during Coverage data dump
void KickWd( void)
{
    FPGA_W( FPGA_WD_TIMER, FPGA_WD_TIMER_VAL);
    FPGA_W( FPGA_WD_TIMER_INV, FPGA_WD_TIMER_INV_VAL);
}
//=================================== PWC Modifications =======================================

/* #ifdef       __cplusplus
}
#endif */
