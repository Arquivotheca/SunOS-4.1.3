/*
  @(#)ATS dtm_tcm.h 1.1 92/07/30 Copyright Sun Microsystems Inc.
*/
/*
 * dtm_tcm.h
 *
 *	This file lists the rpc proc nums for messages from dtm to tcm.
 *
 */

struct failure {		/* when SUNDIAG_ATS_FAILURE/SUNDIAG_ATS_ERROR */
  long hostid;			/* old: int tcm_index */
  int  run_status;		/* old: int teststatus */
  char testname[80];		/* old: char *testname */
  char devname[80];		/* old: char *devname */
  char message[600];		/* 600 = MESSAGE_SIZE + time stamp */
};

struct test {			/* when SUNDIAG_ATS_STOP, SUNDIAG_ATS_START */
  long hostid;			/* old: int tcm_index */
  char testname[80];            /* testname is started/stopped */
  char devname[80];		/* devname */
  int  pass_count;		/* pass count */
};

extern xdr_test();
extern xdr_failure();
