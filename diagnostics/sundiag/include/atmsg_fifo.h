/*      @(#)atmsg_fifo.h 1.1 92/07/30 SMI          */
 
/* 
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#define MAXTRIES        10      /* number of open retries            	   */
#define NUM_FIFOS	3	/* number of fifos			   */

#define EXIT_FIFO       0       /* SYSEX_SUNDIAG_CANT_RUN message type     */
#define ANSWER_FIFO     1       /* SYSEX_MEXEC_PROMPT_ANSWER message type  */
#define TEST_FIFO       2       /* SYSEX_MEXEC_TEST_FAILURE message type   */
				/* SYSEX_MEXEC_UNKNOWN_TST message type    */
				/* SYSEX_MEXEC_START_TST_FAIL message type */
struct fifo {
    int key; 
    int fd; 
};

#ifndef MAXNAMLEN
#define   MAXNAMLEN     255
#endif
#define MAXANSWER	40
#define MAXTESTNAM	40
#define MAXDEVNAM	20

struct exit_msg {
  int procnum;
};

struct answer_msg {
    int procnum;
    int strlen;
    char answer[MAXANSWER];
};

struct test_msg {
    int procnum;
    int teststatus;
    char testname[MAXTESTNAM];
    char devname[MAXDEVNAM];
    char err_logname[MAXNAMLEN];
};
