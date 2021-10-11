/*      @(#)libtest.h	1.1 7/30/92 Copyright Sun Micro     */

/********************************************************************
  libtest.h
  This include file is used for internal library rouintes, and should be
  transparent to general Sundiag test programmers.
*********************************************************************/
 
#ifndef FALSE
#define FALSE   0 
#endif

#ifndef TRUE
#define TRUE    ~FALSE
#endif

#define RETRY_SEC               120
#define TOTAL_TIMEOUT_SEC       0       /* RPC reply timeout seconds */
#define TOTAL_TIMEOUT_USEC      0

/* RPC program number and version number */
#define SUNDIAG_PROG            0x20000002
#define SUNDIAG_VERS            1

/* RPC procedure numbers */
#define INFO_MSG                1
#define WARNING_MSG             2
#define FATAL_MSG               3
#define ERROR_MSG               4
#define DEFAULT_MSG             5
#define LOGFILE_MSG             6
#define PROBE_DEVS              7
#define CONSOLE_MSG             8

extern void trace_before();
extern void trace_after();
