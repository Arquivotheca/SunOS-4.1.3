
#ifndef lint
static  char sccsid[] = "@(#)exec_tests.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/* Include files */
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <esd.h>

extern char *sprintf();
extern void pmessage();

/* Static variables */
static
int		pid;

/**********************************************************************/
exec_tests(fildes, test_type, blck_cnt, brd_cnt, test_select)
/**********************************************************************/
int fildes[], test_type, blck_cnt, brd_cnt, test_select;

{

    extern char *fork_test();

    char errtxt[256];
    char *errmsg;

    if ((errmsg=fork_test(fildes, test_type, blck_cnt, brd_cnt,
					    test_select, &pid))) {
	(void)sprintf(errtxt, errmsg_list[8], errmsg);
	pmessage(errtxt);
	return(-1);
    }

    /* hang up here untill test died */
    do {
	rd_stpd_pipe(fildes);
    } while (pid);
    
    return(0);
  
}

/*ARGSUSED*/
/**********************************************************************/
register_signal(npid)
/**********************************************************************/
int npid;

{
    extern void test_died();
    extern void (*signal())();

    /* catch signal when test(s) die(s) */
    (void)signal(SIGCHLD, test_died);
}


/**********************************************************************/
rd_stpd_pipe(fildes)
/**********************************************************************/
int fildes[];

{
    int cc, bcount;
    char text[512];

    /* read pending message in the pipe */
    while ((ioctl(fildes[0], FIONREAD, &cc) != -1) && cc > 0) {
        bcount = read(fildes[0], text, (cc <= 511) ? cc : 511);
	text[bcount] = '\0';
        pmessage(text);
    }
}

/*ARGSUSED*/
/**********************************************************************/
void
test_died(sig, code, scp, addr)
/**********************************************************************/
int sig, code;
struct sigcontext *scp;
char *addr;
 
{

    extern void (*signal())();
    int cpid;
    union wait status;
    char errtxt[256];
 
    /* unregister SIGCHLD */
    (void)signal(SIGCHLD, SIG_DFL);

    cpid = wait4(pid, &status, WNOHANG, 0);

    switch (cpid) {

	case -1 :
	    perror("wait4 subroutine");
	    return;

	case 0 :
	    (void)fprintf(stderr, "Spurious SIGCHLD caught.\n");
	    return;

	default :
	    if (WIFEXITED(status)) { /* normal exit */
		pid = 0;
		return;
	    }
	    if (WIFSTOPPED(status)) { /* stopped by a signal */
		pid = 0;
		(void)sprintf(errtxt,  "Test has been stopped by signal %d.\n", status.w_stopsig);
		pmessage(errtxt);
		return;
	    }
	    if (WIFSIGNALED(status)) { /* test was terminated */
		pid = 0;
		(void)sprintf(errtxt,  "Test has been terminated by signal %d.\n", status.w_termsig);
		pmessage(errtxt);
		return;
	    }
    }
}
