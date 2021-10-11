static char     version[] = "Version 1.1";
static char     sccsid[] = "@(#)recon.c 1.1 92/07/30 Copyright(c) 1987, Sun Microsystems, Inc.";
/*
 * recon.c
 * 
 * Copyright(c) 1987, Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/dir.h>
#include <sys/time.h>
#include <errno.h>
#include "sdrtns.h"		/* sundiag standard header file */
#include "tapetest.h"
#include "tapetest_msg.h"

extern char 	*device;	/* /dev/<tape_dev> defined in tapetest.c*/

static char 	sd_buf[512 + 2], /* Buffer to store SCSI disk reads */
		sd_name[12];	/*  /dev/<SCSI dev name> */

static int      st_done=0,	/* SCSI retension done flag */
		pf_flag=0,	/* Child returned status flag */
		ppid=0,		/* Parent (st) process id */
		sd_fd=0;	/* File descriptor to SCSI disk */

static struct 	mtop mtop_arg = {MTRETEN, 1};    /* retension command */

/*
 * Main routine that invokes the test
 * and retries for MAX_RETRIES number
 * of times in order to provide for
 * scheduling delays. Persistent delays
 * ie., > MAX_RETRIES is flagged off as
 * an error and SCSI Reconnect/Disconnect
 * failed message is displayed.
 */
recon()
{
    send_message(0, VERBOSE, recon_start_msg);
    nice(-40);    /* set the max. priority, in order to minimize delays */
    send_message(0, VERBOSE, tape_name_msg, device);

    while (rec_test() == TRY_AGAIN)
        ;	/* if child sleep too much */

    send_message(0, VERBOSE, recon_stop_msg);
}

/*************************************************************************/

/*
 * Signal handler in child.
 * Traps the SIGUSR1 signal 
 * received FROM the parent.
 * On receiving this signal
 * the child process proceeds
 * with the SCSI read.
 */
sig_st_done()
{			       /* to catch the "retension  done signal */
	st_done = 1;
}

/*
 * Signal handler in parent.
 * Traps various signals from the child
 * depending on success or failure and
 * sets the pf_flag.
 */
fromchild(sig)			/* to catch signals from  child(sd) process */
int sig;
{
    switch(sig) {
    	case SIGUSR1:		/* Sent from child on success */
		pf_flag = 1;
		break;
    	case SIGUSR2:		/* Sent from child on failure */
		pf_flag = 2;
		break;
    	case SIGVTALRM:		/* time to do the retry */
		pf_flag = 4;
		break;
    	case SIGALRM:
		pf_flag = -READDISK_ERROR;
		break;
    	default:		/* spurious signals */
		pf_flag = 3;
    }
}

/*
 * This is the actual test.
 * We fork a process.
 * The parent does the tape retension.
 * The child does the SCSI disk read.
 * Communication between parent (st) and child (sd) processes is via 
 * kill() system call.
 * sleep system call is used for syncronizing the processes.
 * In case sleep takes too much time due to scheduling delays, we
 * retry five times.
 */

rec_test()
{
    int  
			omask,	/* signal mask */
			pid,	/* child process id */
			st_fd,	/* File descriptor to SCSI tape dev */
			trycounter,	/* Stores the number of retries */
			log_time;	/* Stores time in secs since 1970 */
    DIR			*dirfp;
    struct direct 	*dp;
    struct timeval 	tv;


    dirfp = opendir("/dev");
    for (dp = readdir(dirfp); dp != NULL; dp = readdir(dirfp))
    {	/* Read through the directory structure for raw SCSI device */
	if (dp->d_namlen == 5) { /* Only if namelength matched our device */
	    if (!strncmp(dp->d_name,"rsd",3) && 
	    ((dp->d_name[dp->d_namlen-1] == 'a') || 
	    (dp->d_name[dp->d_namlen-1] == 'g'))) /* Partition 'a' or 'g' */
		{	/* Look only for /dev/rsd?a OR /dev/rsd?g */
    			strcpy(sd_name, "/dev/");
			strcat(sd_name,dp->d_name);
			send_message(0, VERBOSE, disk_name_msg, sd_name);
			if ((sd_fd = open(sd_name, O_RDONLY)) > 0) { 
				if ( read(sd_fd, sd_buf, 512) == 512 )
					break;	/* The partition exists ! */
				else { /* the search continues.. */
					close_dev(sd_fd);
					sd_fd = 0;
				}
			}
		}
	}
    }
    closedir(dirfp); /* Finished looking for  raw SCSI disk device */

    if (sd_fd < 0) /* Did not find it, FATAL ERROR */
	send_message(-OPENDISK_ERROR, ERROR, device_err_msg);

    if (sd_name[0] == NULL || device[0] == NULL)
	send_message(-ENVIRON_ERROR, ERROR, environ_err_msg);

    pf_flag = 0;
    signal(SIGUSR1, fromchild);		/* so that parent is always ready */
    pid = fork();			       /* create the child(sd)
						* process */

    if (pid == 0) {			       /* child(sd) process */
	st_done = 0;
	signal(SIGUSR1, sig_st_done);	       /* setup to catch the signal */

	ppid = getppid();	     /* get the pid of parent(st process) */

	gettimeofday(&tv, (struct timezone *)NULL);
	log_time = tv.tv_sec;

	kill(ppid, SIGUSR1);		       /* notify the parent to get
						* started */

	while (st_done == 0)
	    sleep(1);			       /* wait until parent(st
						* process)  is about to start */

	st_done = 0;
	sleep(1);			       /* make sure parent(st
						* process) has started */
	gettimeofday(&tv, (struct timezone *)NULL);
	if ( (tv.tv_sec - log_time) >= MAX_TIME )
	{	/* if it takes ages to wakeup */
		kill(ppid, SIGVTALRM); 
		exit(0);
	}
	else /* If alls well, do the read */
		rdsk();
	sleep(1);   
	/* do the raw SCSI disk access again */
	rdsk();


	sleep(1);			       /* to synchronize */

	if (st_done == 0)    /* passed */
	    kill(ppid, SIGUSR1);
	else /* failed */
	    kill(ppid, SIGUSR2);

	while (!st_done)			/* wait until st is done too */
	  sleep(1);

	exit(0);			       /* ..exit quietly !!! */

    } else {				       /* parent(st) process */

    	signal(SIGVTALRM, fromchild);	/* setup to catch the signal */
	signal(SIGUSR2, fromchild);
	signal(SIGALRM, fromchild);
	/* used to simulate spurious signal error */

	omask = enter_critical();
        if ((st_fd = open(device, O_RDONLY)) == -1) 
		send_message(-OPENTAPE_ERROR, ERROR,open_err_msg,device,errmsg(errno));
	exit_critical(omask);

	while (pf_flag == 0)
	    sleep(1);			       /* wait until child is ready */

	pf_flag = 0;
	kill(pid, SIGUSR1);		       /* notify the child to get
						* started */

	if (ioctl(st_fd, MTIOCTOP, &mtop_arg) == -1) {	/* retension tape */
	    kill(pid, SIGINT);
	    send_message(-RETEN_ERROR, ERROR, reten_err_msg, device);
	}
	kill(pid, SIGUSR1);		       /* notify the child */

	close_dev(st_fd);
	close_dev(sd_fd);

	while (pf_flag == 0)	  /* wait until sd process(child) is done */
	  sleep(1);

	switch(pf_flag) {	/* Check status and take appropriate action */
	case  1:	/* SIGUSR1, all fine */
		send_message(0, VERBOSE,"recon : Passed.");
		break;			/* Passed recon test */
	case -READDISK_ERROR:
		send_message(-READDISK_ERROR, ERROR, file_err_msg, sd_name);
		break;
	case  4:	/* Retry signal */
		if (trycounter++ <= MAX_RETRIES) {	
			send_message(0, INFO, retry_msg);
			return(TRY_AGAIN);
		}
	case  2:	/* SIGUSR2, Fail signal */
	    send_message(-FAIL_ERROR, ERROR, connect_err_msg);
	    break;
	default:
	    send_message(-BADSIG_ERROR, ERROR, spurious_msg);
	}
    } /* end of if parent */

    return(0);
}

/* 
 * Read the SCSI disk.
 */
static
rdsk()
{
	if (read(sd_fd, sd_buf, 512) != 512) {	/* get host out of the loop */
		kill(ppid, SIGALRM);	
		send_message(-READDISK_ERROR, ERROR, file_err_msg, sd_name);
	}
}

/*
 * This function is called just before executing
 * some critical part of the code. The SIGINT
 * signal when/if received while executing this
 * part of the code will be temporarily blocked
 * and serviced at a later stage(in exit_critical()).
 */
enter_critical()
{
	int oldmask=0;
#       ifdef SVR4
        oldmask = sighold(SIGINT);
#       else
        oldmask = sigblock(sigmask(SIGINT));
	return(oldmask);
#       endif
}
 
/*
 * This function is called just after executing
 * some critical part of the code. The SIGINT
 * signal will be  unblocked and the old signal
 * mask will be restored, so that the interrupts
 * may be serviced as originally intended.
 */
exit_critical(omask)
int omask;
{
#       ifdef SVR4
        sigrelse(SIGINT);
#       else
        (void) sigsetmask(omask);
#       endif
}

