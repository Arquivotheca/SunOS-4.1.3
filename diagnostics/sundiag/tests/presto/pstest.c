#ifndef lint
static  char sccsid[] = "@(#)pstest.c 1.1 92/07/30 Copyright Sun Micro";
#endif
/****************************************************************************

  Legato Prestoserve board is served for NFS acceleration. The board is
  nothing but an extra memory cache on VME slot and can be used for
  multiple controllers all together.  This board is good for synchronous
  I/O access only; and useless, even worse, on async type of access.

  - Memory tests now read/write shorts and longs, as well as chars to test
    more board paths.
  - "PRRESET" done to reinit board properly.
  - Timing is done on filesystem writes only, for more reproducable results.
  - File locking is now used to insure the only one diag is trying to
    read/write memory on the board at a time.

***************************************************************************/

/*
 * Prestoserve diagnostic.
 *
 * Copyright (C) 1990, Legato Systems, Inc.
 * All rights reserved.
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include "sdrtns.h"
#include "../../../lib/include/libonline.h"
#include "pstest.h"
#include "pstest_msg.h"
#include "prestoioctl.h"

extern char *malloc();
extern char *memalign();
extern caddr_t mmap();
extern time_t time();
extern off_t lseek();
extern time_t iotest();

static int	dummy(){return FALSE;}
int	flock_created = 0;
int	testfile_created = 0;
int 	prfd;
double  ratio, lowratio = 1.5;
prstates  presto_statesave;
int sel_perf_warning = FALSE;

void
main(argc, argv)
	int argc;
	char *argv[];
{
        char *devname = PRDEV;
	int  excode;
        extern int process_presto_args(), routine_usage();

        versionid = "1.1";
        device_name = &devname[5];

        test_init(argc, argv, process_presto_args, routine_usage, test_usage_msg);

	if ((excode = pserve_test()) == -1) {
	   send_message (TESTEND_ERR, FATAL, testend_err_msg); 
	}
	else
	   send_message (0, VERBOSE, "PRESTOSERVE TESTS PASSED.\n");
				/* make sure displayed at command mode */

        test_end();		/* Sundiag normal exit */
}

process_presto_args(argv, arrcount)
char *argv[];
int arrcount;
{
    func_name = "process_presto_args";
    TRACE_IN
 
    if (strncmp(argv[arrcount], "e", 1) == 0)
    {
        sel_perf_warning = TRUE;
        TRACE_OUT
        return(TRUE);
    }
    else
    {
        TRACE_OUT
        return(FALSE);
    }
}

routine_usage()
{
    func_name = "routine_usage";
    TRACE_IN
 
    send_message(0, CONSOLE, routine_msg, test_name);
 
    TRACE_OUT
}

int
pserve_test()
{

        int res, iocount;
        register int i;
	int saverr;
	int subpass;
	int fatal_flag = FALSE;

        register char *prbase;
        struct presto_status prstatus;
	prstates tmp;


        time_t time_with, time_without;


	func_name = "pserve_test";
	TRACE_IN
	saverr = test_error;

        if ((prfd = open(PRDEV, O_RDWR)) == -1) {
		send_message (OPEN_ERR, FATAL, open1_err_msg, errmsg(errno));
                exit(OPEN_ERR);  /* force to exit no matter what */
        } /* end of if */

        /* Check state of the board. */
        if ((res = ioctl(prfd, PRGETSTATUS, &prstatus)) == -1)
                send_message(IOCTL_ERR, ERROR, 
			prgetstatus_err_msg, errmsg(errno));

        /* Check for error state. */
        if (prstatus.pr_state == PRERROR) {
		send_message(STATE_ERR, FATAL, state_err_msg); 
                exit(STATE_ERR);	/* force to exit no matter what */
        } /* end of if */

	/* Save the current prestoserve status to allow recover later. */
	presto_statesave = prstatus.pr_state;

	(void) send_message(0, VERBOSE, "prestoserve memory size= %d bytes",
		prstatus.pr_maxsize);

        /* Make sure all batteries are good. */
        for (i=0; i<prstatus.pr_battcnt; i++) {
                if (prstatus.pr_batt[i] != BAT_GOOD) {
                        send_message(BATTERY_ERR, FATAL, battery_err_msg);
                        exit(BATTERY_ERR);  /* force to exit no matter what */
                }
        }
        (void) send_message(0, VERBOSE, "All batteries are good.");

        /* Make sure that prestoserve is DOWN before locked. */
        tmp=PRDOWN;
        if ((res = ioctl(prfd, PRSETSTATE, &tmp)) == -1)
                send_message(IOCTL_ERR, ERROR,
                        prsetstate1_err_msg, errmsg(errno));
        (void) send_message(0, VERBOSE, "Turned Prestoserve DOWN.");

        /* Place a lock on the board, preventing simultaneous prestotest's */
        if ((res = flock(prfd, LOCK_EX|LOCK_NB)) == -1) 
                send_message(FLOCK_ERR, ERROR, flock1_err_msg, errmsg(errno));
	else
		flock_created = TRUE;

	(void) send_message(0, VERBOSE, "Prestoserve locked.");

 	subpass = PASS_COUNT;
	for (i=1; i<=subpass; i++) {
		if ((res = memory_check(prstatus.pr_maxsize)) == -1) {
		   fatal_flag = TRUE;
		   if (run_on_error) { 
			send_message(SUBTEST_ERR, FATAL, 
			   memcheck_err_msg, i);
			subpass = PASS_COUNT + RETRY_COUNT;  /* retry */
		   }
		}
		else
		   (void) send_message(0, VERBOSE, 
			"Memory read/write test passed.");
	}

	subpass = PASS_COUNT;
	for (i=1; i<=subpass; i++) {
		for (iocount = 0; iocount < 3; iocount++) {
		     res = fileio_check();
		     if (res == 0) break;
		     else if (res == -2) 
				continue;	/* try one more time */
		     else {
		   	fatal_flag = TRUE;
                   	if (run_on_error) {
                           send_message(SUBTEST_ERR, FATAL, 
                              filecheck_err_msg, i);
			   subpass = PASS_COUNT + RETRY_COUNT;   /* retry */
		        }
		     }
		 } /* end of for iocount */
		 if (iocount < 3)
		     (void) send_message(0, VERBOSE, "File I/O test passed.");
		 else {
		     fatal_flag = TRUE;
                     if (sel_perf_warning)
                         (void) send_message(0, WARNING,
                                performance_warning_msg, ratio);
                     else
                         (void) send_message(PERFORMANCE_ERR, ERROR,
                                performance_err_msg, ratio);
		 }
	}

	clean_up();

	TRACE_OUT
        if (test_error > saverr || fatal_flag)
                return (-1);
        else    
                return (0);
}


int
memory_check(size)
int	size;
{
	int res;
	int err;
	int saverr;
	register int i, iter;
	prstates tmp;
        struct presto_status prstatus;
	long xor;
	register char *prbase;


	func_name = "memory_check";
	TRACE_IN
	saverr = test_error;


        /* Check state of the board. */
        if ((res = ioctl(prfd, PRGETSTATUS, &prstatus)) == -1)
                send_message(IOCTL_ERR, ERROR, prgetstatus_err_msg,
                        errmsg(errno));
 
        /* make sure board is in DOWN state. */
        if (prstatus.pr_state == PRUP) {
                if ((res = ioctl(prfd, PRRESET, 0)) == -1)
                    send_message(IOCTL_ERR, ERROR,
                        prreset_err_msg, errmsg(errno));
                tmp=PRDOWN;
                if ((res = ioctl(prfd, PRSETSTATE, &tmp)) == -1)
                    send_message(IOCTL_ERR, ERROR,
                        prsetstate1_err_msg, errmsg(errno));
        }

	/* Map the board into our address space. */
	if ((prbase = (char *)mmap((char *)NULL, size,
	    PROT_READ|PROT_WRITE, MAP_SHARED, prfd, (off_t) 0)) == (char *)-1)
		send_message(MMAP_ERR, ERROR, mmap_err_msg, errmsg(errno));

	/* read/write test on memory. */
	xor = 0;
	for (iter = 1; iter <= MEM_TEST_CYCLES; iter++) {

		/* Read/write memory as bytes. */
		for (i=STARTLOC/sizeof (char), err=test_error; 
		     i<size/sizeof (char); i++) {
			((char *)prbase)[i] = (char)(xor^i);
			if (((char *)prbase)[i] != (char)(xor^i)) {
			   (void) send_message(COMPARE_ERR, ERROR,
				      compare1_err_msg,
				      i,((char *)prbase)[i],(char)(xor^i));
			}
		}
		send_message (0, VERBOSE, 
			"total byte access = %d, failed = %d", 
				i, test_error-err);

		/* Read/write memory as shorts. */
		for (i=STARTLOC/sizeof (short), err=test_error; 
		     i<size/sizeof (short); i++) {
			((short *)prbase)[i] = (short)(xor^i);
			if (((short *)prbase)[i] != (short)(xor^i)) {
			   (void) send_message(COMPARE_ERR, ERROR, 
				     compare2_err_msg,
				     i,((short *)prbase)[i],(short)(xor^i));
			}
		}
		send_message (0,VERBOSE,"total word access = %d, failed = %d",
				 i, test_error-err);

		/* Read/write memory as longs. */
		for (i=STARTLOC/sizeof (long), err=test_error; 
		     i<size/sizeof (long); i++) {
			((long *)prbase)[i] = (long)(xor^i);
			if (((long *)prbase)[i] != (long)(xor^i)) {
		           (void) send_message(COMPARE_ERR, ERROR,
				    compare3_err_msg,
				    i, ((long *)prbase)[i], (long)(xor^i));
			}
		}
                send_message (0, VERBOSE, 
			"total long access = %d, failed = %d", 
			i, test_error-err);

		if (quick_test) break; 	  /* no need to repeat */
		xor = ~xor;	/* reverse the pattern. */
	}

        /* Unmap the board off our address space. */
        if (munmap(prbase, size) == -1)
                send_message(MUNMAP_ERR, ERROR, munmap_err_msg, errmsg(errno));

	TRACE_OUT
	if (test_error > saverr)
		return (-1); 
	else	
		return (0);
}


int
fileio_check()
{
        int res;
	int saverr;
	int fatal_flag = FALSE;
        prstates tmp;   
        struct presto_status prstatus;
        time_t time_with, time_without;


	func_name = "fileio_check";
	TRACE_IN
	saverr = test_error;

        /* Check state of the board. */
        if ((res = ioctl(prfd, PRGETSTATUS, &prstatus)) == -1)
                send_message(IOCTL_ERR, ERROR, prgetstatus_err_msg,
			errmsg(errno));

        /* make sure board is in DOWN state. */
        if (prstatus.pr_state == PRUP) {
        	if ((res = ioctl(prfd, PRRESET, 0)) == -1)
                    send_message(IOCTL_ERR, ERROR, 
			prreset_err_msg, errmsg(errno));
        	tmp=PRDOWN;
        	if ((res = ioctl(prfd, PRSETSTATE, &tmp)) == -1)
		    send_message(IOCTL_ERR, ERROR, 
                	prsetstate1_err_msg, errmsg(errno));
        }

	/* time to run io test WITHOUT prestoserve (board is DOWN). */
	time_without = iotest();
	if (time_without > 0) 
	 	(void) send_message(0, VERBOSE, 
		  "File I/O test, elapsed time: %d seconds. (Prestoserve down)", 
		  time_without);
	else {
		fatal_flag = TRUE;
		(void) send_message(0, FATAL, 
		  "File I/O test failed! (Prestoserve down)"); 
	}

	/* Reinitializing (RESETing) prestoserve. */
	if ( (res = ioctl(prfd, PRRESET, 0)) == -1)
		(void) send_message(IOCTL_ERR, ERROR, 
			prreset_err_msg, errmsg(errno));
        (void) send_message(0, VERBOSE, "Reinitialized Prestoserve.");

	/* Turn prestoserve back UP. */
	tmp=PRUP;
	if ((res = ioctl(prfd, PRSETSTATE, &tmp)) == -1)
		(void) send_message(IOCTL_ERR, ERROR,
			prsetstate2_err_msg, errmsg(errno));
        (void) send_message(0, VERBOSE, "Turned Prestoserve UP.");

	/* time to run io test WITH prestoserve (board is now UP). */
	time_with = iotest();

        if (time_with > 0) 
                (void) send_message(0, VERBOSE,
                  "File I/O test, elapsed time: %d seconds. (Prestoserve up)",
                  time_with);
        else {
		fatal_flag = TRUE;
                (void) send_message(0, FATAL,  
                  "File I/O test failed! (Prestoserve up)");
 	}
	ratio = (double)time_without/(double)time_with;
	(void) send_message(0, VERBOSE, "Performance Ratio:  %4.2f:1", 
		ratio);

	TRACE_OUT
        if (test_error > saverr || fatal_flag)
                return (-1);		/* error */
        else if (ratio < lowratio)
		return (-2);		/* need to retry */   
	else
                return (0);		/* succeed */
}


/*
 * (REPS*SZ) should exceed the size of the
 * prestoserve cache (1M) to force flushing.  256*8192
 */

char testfile[100];


/*
 * Run a read/write test to the filesystem.
 * Time the write portion to measure improvement
 * in synchronous write performance.
 */
time_t
iotest() {
	int loop, fd, stat, pid;
	int saverr;
	register int count, i;
	time_t total, start, finish;
	char buf[SZ], bufcmp[SZ];

	func_name = "iotest";
	TRACE_IN
	saverr = test_error;

	/* init the buffer. */
	for (i=0; i<SZ; i++)
		buf[i] = bufcmp[i] = i;

	/* create a unique /tmp filename. */
	pid = getpid();
	(void) sprintf(testfile, "/tmp/prestotest.%d", pid);

	if ((fd = open(testfile, O_CREAT|O_RDWR|O_SYNC, 0666)) == -1) {
		send_message(OPEN_ERR, ERROR, open2_err_msg, testfile,
			errmsg(errno));
		testfile_created = FALSE;
	}
	else
		testfile_created = TRUE;

	total = 0;
	for (loop = 1; loop <= IO_TEST_CYCLES; loop++) {

		start = time((time_t *)0);

		for (count = 0; count < REPS; count++) {
			buf[0] = count;
			stat = write(fd, (char *)buf, SZ);
			if (stat != SZ)
				send_message(WRITE_ERR, ERROR, 
					write_err_msg, errmsg(errno));
		}
		finish = time((time_t *)0);


		total += (finish-start);


		if ((stat = lseek(fd, 0L, L_SET)) !=0)
			send_message(LSEEK_ERR, ERROR, lseek_err_msg,
				errmsg(errno));

		for (count = 0; count < REPS; count++) {

			if ((stat = read(fd, (char *)buf, SZ)) != SZ)
				send_message(READ_ERR, ERROR,
				    read_err_msg, errmsg(errno));

			/* make sure we read what we wrote. */
			bufcmp[0] = (char) count;
			if (bcmp(buf, bufcmp, SZ) != 0) {
				(void) send_message(COMPARE_ERR, ERROR,
					compare4_err_msg);
			}
		}

		if ((stat = lseek(fd, 0L, L_SET)) != 0)
			send_message(LSEEK_ERR, ERROR,"Lseek failed: %s",
				errmsg(errno));

		if (quick_test) break;		/* no need to repeat */

	} /* end of for */

	if (testfile_created) {
		close (fd);
		(void) unlink(testfile);
		testfile_created = FALSE;
	}

	TRACE_OUT
        if (test_error > saverr)
                return (-1);
        else
                return (total);
}


clean_up()
{
	int    res;
	prstates tmp;
	

        /* Restore board state  */
	if (prfd > 0) {
        	if ((res = ioctl(prfd, PRSETSTATE, &presto_statesave)) == -1) {
                   send_message(IOCTL_ERR, ERROR,
                        prsetstate3_err_msg, errmsg(errno));
	  	}
                if (flock_created) {
                        flock(prfd, LOCK_UN);
                        flock_created = FALSE;
                }
                close (prfd);
                prfd = 0;
	}

        if (testfile_created) {
                (void) unlink(testfile);
                 testfile_created = FALSE;
        }

	return(0);
}

