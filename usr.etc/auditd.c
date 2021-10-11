#ifndef lint
static	char *sccsid = "@(#)auditd.c 1.1 92/07/30 SMI"; /* Copywr SunMicro */
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

/* Audit daemon server */
/****************************************************************************
 * These routines make up the audit daemon server.  This daemon, called
 * auditd, handles the user level parts of auditing.  For the most part
 * auditd will be in the audit_svc system call; otherwise, it is handling
 * the interrupts and errors from the system call and handling the audit
 * log files.
 *
 * The major interrupts are SIGHUP (start over), SIGTERM (start shutting
 * down), SIGALRM (quit), and SIGUSR1 (start a new audit log file).
 *
 * The major errors are EBUSY (auditing is already in use), EINVAL (someone
 * has turned auditing off), EINTR (one of the above signals was received),
 * and EDQUOT (the directory is out of space).  All other errors are treated
 * the same as EDQUOT.
 *
 * Much of the work of the audit daemon is taking care of when the
 * directories fill up.  In the audit_control file, there is a value
 * min_free which determines a "soft limit" for what percentage of space
 * to reserve on the file systems.  This code deals with the cases where
 * one file systems is at this limit (soft), all the file systems are at
 * this limit (allsoft), one of the file systems is completely full (hard),
 * and all of the file systems are completely full (allhard).  The
 * audit daemon warns the administrator about these and other problems
 * using the auditwarn shell script.
 ***************************************************************************/

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <sys/wait.h>
#include <stdio.h>
#include <pwd.h>
#include <signal.h>
#include <setjmp.h>
#include <tzfile.h>


/*
 * DEFINES:
 */

#define MACH_NAME_SZ	64
#define AUDIT_DATE_SZ	14
#define AUDIT_FNAME_SZ	2 * AUDIT_DATE_SZ + 2 + MACH_NAME_SZ

#define MYMASK  sigmask(~SIGTERM)	/* catch only SIGTERM */

#define	SOFT_LIMIT	0	/* used by the limit flag */
#define HARD_LIMIT	1

#define SOFT_SPACE	0	/* used in the dirlist_s structure for	*/
#define HARD_SPACE	1	/*   how much space there is in the	*/
#define SPACE_FULL	2	/*   filesystem				*/
#define STAY_FULL	3

#define AVAIL_MIN	50	/* If there are less that this number	*/
				/* of blocks avail, the filesystem is	*/
				/* presumed full.  May need tweeking.	*/

/*
 * After we get a SIGTERM, we want to set a timer for 2 seconds and
 * call do_auditing() to let the auditsvc syscall write as many records
 * as it can until the timer goes off (at which point it returns to
 * auditd with SIGALRM.
 */
#define ALRM_TIME	2

#define SLEEP_TIME	20	/* # of seconds to sleep in all hard loop */

/* DATA STRUCTURES: */

/*
 * The directory list is a circular linked list.  It is pointed into by
 * startdir and thisdir.  Each element contains the pointer to the next
 * element, the directory pathname and a flag for how much space there is
 * in the directory's filesystem.
 */
struct dirlist_s {
        struct dirlist_s        *dl_next;
	int			dl_space;
        char 			*dl_name;
};
typedef struct dirlist_s dirlist_t;

/*
 * The logfile contains a file descriptor, a directory pathname,
 * and a filename.
 */
struct logfile_s {
        int                     l_fd;
        char                    *l_name;
};
typedef struct logfile_s        logfile_t;
 
/*
 * GLOBALS: 
 *	startdir	pointer to the "start" of the dir list
 *	thisdir		pointer into dir list for the current directory
 *	logfile		points to struct with info of current log file
 *	force_close	true if the user explicitly asked to close the log file
 *	minfree		the lower bound percentage read from audit_control
 *	minfreeblocks	tells audit_svc what the soft limit is in blocks
 *	limit		whether we are using soft or hard limits
 *	errno		error returned by audit_svc (and other syscalls)
 *	sig_num		number for which signal was received
 *	ljmp		used to close a "window" for signals.  comment below.
 *	dbflag		debug flag (option to auditd)
 *	nflag		no fork flag (option to auditd) used for dbx
 *	hung_count	count for how many times sent message for all hard
 *	opened		whether the open was successful
 *	the files	these are the four files auditing uses in addition
 *				to the audit log files
 */

dirlist_t *startdir;
dirlist_t *thisdir;
logfile_t *logfile;
int force_close;
int minfree;
int minfreeblocks;
int limit = SOFT_LIMIT;
extern int errno;
int sig_num = 0;
jmp_buf ljmp;
int dbflag;
int nflag;
int hung_count;
int opened = 1;
char auditcontrol[] = "/etc/security/audit/audit_control";
char auditdata[] = "/etc/security/audit/audit_data";
char tempfile[] = "/etc/security/audit/audit_tmp";
char auditwarn[] = "/usr/etc/audit_warn";

char *malloc();
char *memcpy();
char *strcpy();
char *sprintf();

/*
 * SIGNAL HANDLERS:
 */

/************************************************************************
 * catch() is the signal handler used for SIGHUP, SIGTERM, and SIGALRM
 * which returns the code to the _setjmp() inside do_auditing().
 ***********************************************************************/
void
catch(sig)
{

        sig_num = sig;
        (void)sigsetmask(MYMASK);
	if (dbflag)
		(void)fprintf(stderr, "auditd: caught signal %d\n", sig_num);
        _longjmp(ljmp, 1);
}

/************************************************************************
 * finish() is the signal handler for SIGTERM that is used to
 * terminate the audit daemon gracefully.  It will return us
 * to the auditsvc syscall for ALRM_TIME seconds (defined above).
 ***********************************************************************/
void
finish()
{
	int audit_break = 0;
	int reset_list = 0;

	(void)sigsetmask(MYMASK);
	if (dbflag)
		(void)fprintf(stderr, "auditd: processing SIGTERM\n");
	if (hung_count > 0) {
		/*
		 * There is no space to write audit records
		 * so just shutdown here.
		 */
		if (dbflag)
			(void)fprintf(stderr, "auditd: terminating the daemon.\n");
		auditon(AUC_NOAUDIT);
		exit(0);
	} 
	if (logfile == (logfile_t *)0) {
		/* This should never happen. */
		if (dbflag)
			(void)fprintf(stderr,
			    "auditd: terminating daemon - no logfile open.\n");
		auditon(AUC_NOAUDIT);
		exit(0);
	}
	(void)alarm(ALRM_TIME);	/* sets timer to signal auditsvc */
	if (dbflag)
		(void)fprintf(stderr, "auditd: alarm set for %d secs\n",
			ALRM_TIME);
	minfreeblocks = 0;
	do_auditing(minfreeblocks, &audit_break, &reset_list);
	/*
	 * if do_auditing() returns to here, we came out of
	 * auditsvc() for some other reason than SIGALRM.  For
	 * now, just exit.
	 */
	(void)dowarn("postsigterm", "", 0);
	auditon(AUC_NOAUDIT);
	exit(0);
}



main(argc, argv)
int argc;
char *argv[];
{
	char*s;			/* used for processing arguments */
	struct sigvec sv;	/* used to set up signal handling */
	int fd;			/* used to remove the control tty */
	int reset_list;		/* 1 if user wants to re-read audit_control */

        /*
         *  Make sure I'm not being audited, and won't be in the future.
         */
        if (setauid(AU_NOAUDITID) < 0)
		(void)fprintf(stderr, "auditd: setauid error %d", errno);

	/*
	 * Check and process arguments.
	 */
	while (--argc && *(s = *++argv) == '-') {
		switch(*++s) {
		case 'd':
			dbflag++;
			break;
		case 'n':
			nflag++;
			break;
		default:
			(void)fprintf(stderr,
				"auditd: Unknown flag '%c'\n", *s);
			exit(1);
		}
	}
        if (argc > 0) {
                (void)fprintf(stderr,
                        "auditd: usage: auditd [-dn]\n", argv[0]);
                exit(1);
        }


	/*
         * Only allow super-user to invoke this program
	 */
        if (getuid() != 0) {
                (void)fprintf(stderr,
                        "auditd:  must be super-user to run this program\n");
                exit(1);
        }

	/*
	 * Set the process group to something safe
	 */
	if (!nflag && setpgrp(0, getpid()) < 0) {
		/* this should never happen */
                (void)fprintf(stderr,
                        "auditd:  unable to set process group\n");
                exit(1);
	}

	/*
         * Make sure that we are running on a secure system.
	 */
	if (!issecure()) {
		/*
		 * set audit state to NOAUDIT so people can do fchroot() or
		 * fchdir() if they wish.  The return is ignored since the only
		 * reason auditon() would fail is if the state is FCHDONE which
		 * is unexpected but fine.
		 */
		auditon(AUC_NOAUDIT);
		exit(0);
	}

	/*
	 * Set the audit state flag to AUDITING.  If we can't, fchroot() or
	 * fchdir() has been done.  Auditing can't start until the system is
	 * rebooted.  Use auditwarn to inform the administrator.
	 */
	if (auditon(AUC_AUDITING) < 0) {
		if (dbflag)
			perror("auditd");
		(void)dowarn("nostart", "", 0);
		exit(0);
	}

	/*
	 * Remove the control tty
	 */
	if ((fd = open("/dev/tty", 0, O_RDWR)) > 0) {
		(void) ioctl(fd, TIOCNOTTY, 0);
		(void) close(fd);
	}

	/*
	 * Fork child process and abandon it (it gets inherited by init)
	 */
	if (!nflag && fork())
		exit(0);


        if (dbflag) {
                (void)fprintf(stderr, "auditd: Effective uid is: %d\n",
				geteuid());
                (void)fprintf(stderr, "auditd: Real uid is:      %d\n",
				getuid());
        }

	
	setlinebuf(stderr);
	setlinebuf(stdout);

	/*
	 * Set the umask so that only audit or other users in the audit group
	 * can get to the files created by auditd.
	 */
	(void)umask(027);

	/*
	 * Set up signal handling.
	 */
	(void)sigsetmask(MYMASK);       /* Block all signals except SIGTERM  */
	sv.sv_handler = catch;
	sv.sv_mask = -1;
	sv.sv_flags = SV_INTERRUPT;

	/* Catch signals */
	(void)sigvec(SIGHUP, &sv, (struct sigvec *)0);
	(void)sigvec(SIGUSR1, &sv, (struct sigvec *)0);
	(void)sigvec(SIGALRM, &sv, (struct sigvec *)0);

	/* Special handler for SIGTERM */
	sv.sv_handler = finish;
	(void)sigvec(SIGTERM, &sv, (struct sigvec *)0);

	/*
	 * Here is the main body of the audit daemon.  This while loop is
	 * where the code will return if a SIGHUP is received.
	 */
	while (1) {
		loadauditlist();	/* read in the directory list */
					/* sets thisdir to the first dir */
		force_close = 0;
		hung_count = 0;
		limit = HARD_LIMIT;

		/*
		 * Call process_audit to do the auditing and the handling
		 * of space overflows until reset_list is true which
		 * means the user wants to reread the audit_control file.
		 */
		reset_list = 0;
		while (!reset_list)
			(void)process_audit(&reset_list);

	} /* end of while (1) loop*/
} /* end of main */

					

/*******************************************************************
 * process_audit - handle auditing and space overflows
 ******************************************************************/
process_audit(reset_list)
	int *reset_list;
{
	int audit_break = 0; /* whether we were broken out of do_auditing() */
	/*
	 * allhard determines whether all of the filesystems are full.
	 * It is set to 1 (true) until a filesystem is found which has
	 * space, at which point it is set to 0.
	 */
	int allhard = 1;
	int firsttime;	/* used when searching for hard space */

	if (dbflag)
		(void)fprintf(stderr, "auditd: inside process_audit\n");
	startdir = thisdir;

	do {
		/*
		 * test whether thisdir is under the soft limit
		 */
		if (testspace(thisdir, SOFT_SPACE)) {
			limit = SOFT_LIMIT;
			/*
			 * open_log calls close_log.  If the open fails, it
			 * sets thisdir to the next directory in the list
			 */
			if (open_log()) {
				/*
				 * We finally call audit_svc here.
				 */
				allhard = 0;
				do_auditing(minfreeblocks, &audit_break,
						reset_list);
			}
		} else {
			if (thisdir->dl_space == HARD_SPACE)
				allhard = 0;
			/*
			 * Go to the next directory
			 */
			thisdir = thisdir->dl_next;
		}

	} while (thisdir != startdir
		&& !(audit_break || *reset_list));

	if (*reset_list || audit_break)
		return;

	if (thisdir == startdir) {
		if (allhard) {
			handleallhard(reset_list);
		} else {
			if (limit == SOFT_LIMIT)
				(void)dowarn("allsoft", "", 0);

			/*
			 * Find the first directory that has
			 * hard space and open a new logfile.
			 *
			 * Complications:
			 * Open may fail which may mean that
			 * there is no more space anywhere.
			 * Also, if they explicitly want to
			 * change files, we need to do so.
			 */
			opened = 0;
			firsttime = 1;

			while (firsttime || thisdir != startdir) {
				firsttime = 0;
				if (thisdir->dl_space == HARD_SPACE) {
					if (thisdir == startdir &&
					    logfile != (logfile_t *)0 &&
					    !force_close) {
						/* use the file already open */
						if (dbflag)
							(void)fprintf(stderr, "auditd: using the same logfile %s\n", thisdir->dl_name);
						opened = 1;
						break;
					} else if (opened = open_log()) {
						break;
					} else {
						if (dbflag)
							(void)fprintf(stderr, "can't open %s", thisdir->dl_name);
						thisdir = thisdir->dl_next;
					}
				} else {
					thisdir = thisdir->dl_next;
				}
			}
			limit = HARD_LIMIT;
			if (opened) {
				minfreeblocks = 0;
				do_auditing(minfreeblocks, &audit_break,
						reset_list);
			} else {	
				/* allhard is true */
				if (dbflag)
					(void)fprintf(stderr,
					    "auditd: allhard because open failed\n");
				handleallhard(reset_list);
			}
		} /* end of else (if (allhard)) */
	} /* end of if (thisdir == startdir) */
} /* end of process_audit() */



/**********************************************************************
 * do_auditing - set up for calling audit_svc and handle it's various
 * 		 returns.
 * globals:
 *	ljmp    - used for _setjmp and _longjmp (see comment below)
 **********************************************************************/
do_auditing(minfreeblocks, audit_break, reset_list)
	int minfreeblocks;
	int *audit_break;
	int *reset_list;

{
	int error = 0;		/* used to save the errno */

	*audit_break = 1;
	*reset_list = 0;
	force_close = 0;	/* this is a global variable */

	if (limit == HARD_LIMIT) {
		if (dbflag)
			(void)fprintf(stderr,
				"auditd: hard limit, set minfreeblocks to 0\n");
		minfreeblocks = 0;
	}

	/*
	 * audit_svc is put inside an if (!_setjmp(ljmp)) because there
	 * is a window in between turning on signalling and actually getting
	 * inside the system call (where we really want the signal catching
	 * to happen) that we need to close.  If a signal comes in at this
	 * time, the last line of the signal handler, catch(), will do a
	 * _longjmp back to this test of _setjmp with ljmp set to 1.  This
	 * will get us to the else case, and we are all set.
	 */
	if (!_setjmp(ljmp)) {
		if (dbflag)
			(void)fprintf(stderr, "auditd: Begin audit service with %s; limit = %d blocks.\n",
				logfile->l_name, minfreeblocks);
		(void)sigsetmask(0);
		auditsvc(logfile->l_fd, minfreeblocks);
		error = errno;
		(void)sigsetmask(MYMASK);
	} else {
		if (dbflag)
			(void)fprintf(stderr,
				"auditd: Caught signal outside of auditsvc\n");
		error = EINTR;
	}

	switch (error) {
	case EBUSY:
		if (dbflag)
			(void)fprintf(stderr, "auditd: already in use.\n");
		(void)dowarn("ebusy", "", 0);
		exit(1);
	
	case EINVAL:
		/*
		 * who knows what we should do here - someone has turned
		 * auditing off unexpectedly - for now we will exit
		 */
		if (dbflag)
			(void)fprintf(stderr, "auditd: audit state is not AUDITING.\n");
		(void)dowarn("auditoff", "", 0);
		exit(1);

	case EINTR:
		switch (sig_num) {
		case SIGTERM:
			/*
			 * Should not get here.  The signal handler does not
			 * return.
			 */
			if (dbflag)
				(void)fprintf(stderr, "auditd: SIGTERM badly handled.\n");
			exit(1);

		case SIGALRM:
			/*
			 * We have returned from our timed entrance into
			 * auditsvc().  We need to really shut down here.
			 */
			if (dbflag)
				(void)fprintf(stderr, "auditd: processing SIGALRM\n");

			if (logfile) {
				(void)close_log(logfile, "", "");
			}
			/*
			 * Set audit state to NOAUDIT so that people can do
			 * fchroot() or fchdir() if they wish.  I don't check
			 * the return since the only reason auditon() would
			 * fail is if the state is FCHDONE which
			 * is unexpected but fine.
			 */
			auditon(AUC_NOAUDIT);
			if (dbflag)
				(void)fprintf(stderr, "auditd: daemon terminating\n");
			exit(0);

		case SIGHUP:
			/*
			 * They want to reread the audit_control file.  Set
			 * reset_list which will return us to the main while
			 * loop in the main routine.
			 */
			if (dbflag)
				(void)fprintf(stderr, "auditd: processing SIGHUP.\n");
			*reset_list = 1;
			break;

		case SIGUSR1:
			/*
			 * In every normal case, we could ignore this case
			 * because the normal behavior of the rest of the
			 * code is to switch to a new log file before getting
			 * back into audit_svc.  There is one exception -
			 * if all the filesystems are at the soft limit,
			 * the normal behavior is to keep using the same
			 * log file in audit_svc until this file system
			 * fills up.  Since our user is explicitly asking 
			 * for a new file, we must make sure in that case
			 * that a new file is used.
			 */
			if (dbflag)
				(void)fprintf(stderr, "auditd: processing SIGUSR1.\n");
			force_close = 1;
			break;

		default:
			/*
			 * Should not get here since we only catch SIGTERM,
			 * SIGALRM, SIGHUP, and SIGUSR1.
			 */
			if (dbflag)
				(void)fprintf(stderr, "auditd: Signal %d ignored.\n",
					sig_num);
			break;
		}
		break;

	case EDQUOT:
		/*
		 * Update the structure for this directory to have the
		 * correct space value (HARD_SPACE or SPACE_FULL).  Nothing
		 * further needs to be done here, the main routine will
		 * worry about finding space (here or elsewhere).
		 */
		if (dbflag)
			(void)fprintf(stderr, "auditd: processing EDQUOT.\n");
		thisdir->dl_space =
			(limit == SOFT_LIMIT) ? HARD_SPACE : SPACE_FULL;
		break;

	case EIO:
	case ENETDOWN:
	case ENETUNREACH:
	case ECONNRESET:
	case ETIMEDOUT:
	case EHOSTDOWN:
	case EHOSTUNREACH:
	case ESTALE:
		/*
		 * If any of the errors are returned, the filesystem is
		 * unusable.  Make sure that it does not try to use it
		 * the next time that testspace is called.  After that,
		 * it will try to use it again since the problem may have
		 * been solved.
		 */
		if (dbflag)
			(void)fprintf(stderr, "auditd: error %d: %s unusable\n",
				error, thisdir->dl_name);
		thisdir->dl_space = STAY_FULL;
		break;

	default:
		/*
		 * Correct the space portain of the structure for this
		 * directory.  Throw away the return value from testspace.
		 */
		(void)testspace(thisdir, SPACE_FULL);
		if (dbflag) {
			errno = error;
			perror("auditd: Error ignored");
		}
		break;
	}

	/*
	 * We treat any of the signals or errors that get us here except
	 * for SIGHUP and SIGUSR1 as if we have run out of space.
	 */
	if (!*reset_list && !force_close) {
		if (limit == SOFT_LIMIT)
			(void)dowarn("soft", logfile->l_name, 0);
		else
			(void)dowarn("hard", logfile->l_name, 0);
	}

	hung_count = 0;
}



/***********************************************************************
 * handleallhard - handle the calling of dowarn, the sleeping, etc
 *		   for the allhard case
 * globals:
 *	hung_count - how many times dowarn has been called for the
 *		     allhard case.
 *	logfile - the current logfile to close and reinitialize.
 * arguments:
 * 	reset_list - set if SIGHUP was received in my_sleep
 **********************************************************************/
handleallhard(reset_list)
	int *reset_list;
{

	++hung_count;
	if (hung_count == 1) {
		(void)close_log(logfile, "", "");
		logfile = (logfile_t *)0;
		(void)logpost(getpid(), "");
	}
	(void)dowarn("allhard", "", hung_count);
	my_sleep(reset_list);
}



/**********************************************************************
 * my_sleep - sleep for SLEEP_TIME seconds but only accept the signals
 *	      that we want to accept
 *
 * arguments:
 * 	reset_list - set if SIGHUP was received
 **********************************************************************/
my_sleep(reset_list)
	int *reset_list;
{

	if (!_setjmp(ljmp)) {
		if (dbflag)
			(void)fprintf(stderr, "auditd: sleeping for 20 seconds\n");
		(void)alarm(SLEEP_TIME);	/* set timer to "sleep" */
		sigpause(0);		/* the 0 says to allow all signals */
		/* We should never get here!! */
		(void)fprintf(stderr,
			"auditd: We should never get here (after sigpause)!\n");
		exit(1);
	} else {
		/* Handle the signal from sigpause */
		switch (sig_num) {
		case SIGTERM:
			/* We should never get here!! */
			if (dbflag)
				(void)fprintf(stderr,
					"auditd: SIGTERM mishandled\n");
			(void)alarm(0);
			exit(1);

		case SIGHUP:
			/* They want to reread the audit_control file */
			if (dbflag)
				(void)fprintf(stderr,
					"auditd: processing SIGHUP.\n");
			(void)alarm(0);
			*reset_list = 1;
			break;

		case SIGALRM:
			/* We slept for SLEEP_TIME seconds */
			break;

		case SIGUSR1:
			/*
			 * We can ignore this since the current
			 * logfile has already been closed.
			 */
			(void)alarm(0);
			break;

		default:
			/* We shouldn't get here, ignore it */
			(void)alarm(0);
			break;
		}
	}
}



/**********************************************************************
 * open_log - open a new logfile in the current directory.  If a
 *	      logfile is already open, close it.
 *
 * globals:
 * 	thisdir - used to determine where to put the new file
 *	logfile - used to get the oldfile name (and change it),
 *		  to close the oldfile and then is set to the newfile
 **********************************************************************/
int 
open_log()
{
	char auditdate[AUDIT_DATE_SZ+1];
	char host[MACH_NAME_SZ+1];
	char oldname[AUDIT_FNAME_SZ+1];
	char *name;			/* pointer into oldname */
	char *rindex();
	logfile_t *newlog = (logfile_t *)0;
	int opened;
	int error = 0;
	struct timeval tv;
	register audit_header_t *ah;
	register unsigned len;
	register unsigned ahlen;

	if (dbflag)
		(void)fprintf(stderr, "auditd: inside open_log for dir %s\n",
			thisdir->dl_name);
	/* make ourselves a new logfile structure */
	newlog = (logfile_t *)malloc(sizeof(logfile_t));
	if (newlog == NULL) {
		perror("auditd");
		exit(1);
	}
	newlog->l_name = (char *)malloc(AUDIT_FNAME_SZ);
	if (newlog->l_name == NULL) {
		perror("auditd");
		exit(1);
	}

	(void)gethostname(host, MACH_NAME_SZ);
	/* loop to get a filename which does not already exist */
	opened = 0;
	while (!opened) {
		getauditdate(auditdate);
		(void)sprintf(newlog->l_name, "%s/%s.not_terminated.%s",
					thisdir->dl_name, auditdate, host);
		newlog->l_fd = open(newlog->l_name,
					O_RDWR|O_APPEND|O_CREAT|O_EXCL, 0600);
		if (newlog->l_fd < 0) {
			switch (errno) {
			case EEXIST:
				sleep(1);
				break;
			default:
				/* open failed */
				if (dbflag)
					perror("auditd");
				thisdir->dl_space = SPACE_FULL;
				thisdir = thisdir->dl_next;
				return (0);
			} /* switch */
		}
		else
			opened = 1;
	} /* while */

	/*
	 * When we get here, we have opened our new log file.
	 * Now we need to update the name of the old file to
	 * store in this file's header
	 */
	if (logfile) {
		/* set up oldname, but do not change logfile */
		(void)strcpy(oldname, logfile->l_name);
		name = rindex(oldname, '/') + 1;
		(void)memcpy(name + AUDIT_DATE_SZ + 1, auditdate, AUDIT_DATE_SZ);
		if (dbflag)
			(void)fprintf(stderr, "auditd: revised oldname is %s\n", 
					oldname);
	}
	else
		(void)strcpy(oldname, "");

	/*
	 * Write Audit Log header record
	 */
	(void)gettimeofday(&tv, (struct timezone *)0);
        len = (strlen(oldname) + 2) & ~1;
        ahlen = sizeof(audit_header_t) + len;
        ah = (audit_header_t *)malloc(ahlen);
	if (ah == NULL) {
		perror("auditd");
		exit(1);
	}
        ah->ah_magic = AUDITMAGIC;
        ah->ah_time = tv.tv_sec;
        ah->ah_namelen = len;
        (void)strcpy((char *)&ah[1], oldname);

        if (write(newlog->l_fd, (caddr_t)ah, (int)ahlen) < 0)
                error = errno;
        free((caddr_t)ah);
        if (error) {
		if (dbflag)
			perror("auditd");
                (void)close(newlog->l_fd);
		free((caddr_t)newlog->l_name);
                free((caddr_t)newlog);
		thisdir->dl_space = SPACE_FULL;
		thisdir = thisdir->dl_next;
                return(0);
        }

	(void)close_log(logfile, oldname, newlog->l_name);
	logfile = newlog;
	(void)logpost(getpid(), logfile->l_name);
	startdir = thisdir;
	if (dbflag)
		(void)fprintf(stderr, "auditd: Log opened: %s\n", logfile->l_name);
	return (1);
}



/*********************************************************************
 * close_log - close the logfile if open.  Also put the name of the
 *	       new log file in the trailer, and rename the old file
 *	       to oldname
 * arguments -
 *	logfile - the logfile structure to close (and free)
 *	oldname - the new name for the file to be closed
 *	newname - the name of the new log file (for the trailer)
 *********************************************************************/
close_log(logfile, oname, newname)
	register logfile_t      *logfile;
	char			*oname;
	char			*newname;
{
	char auditdate[AUDIT_DATE_SZ+1];
	register audit_trailer_t *at;
	unsigned len;
	unsigned atlen;
	struct timeval tv;
	char *name;
	char *rindex();
	char oldname[AUDIT_FNAME_SZ+1];

	if (dbflag)
		(void)fprintf(stderr, "auditd: inside close_log\n");
	/*
	 * If there is not a file open, return.
	 */
	if (!logfile) {
		return;
	}

	/*
	 * If oldname is blank, we were called by the main routine
	 * instead of by open_log, so we need to update our name.
	 */
	(void)strcpy(oldname, oname);
	if (strcmp(oldname, "") == 0) {
		getauditdate(auditdate);
		(void)strcpy(oldname, logfile->l_name);
		name = rindex(oldname, '/') + 1;
		(void)memcpy(name + AUDIT_DATE_SZ + 1, auditdate, AUDIT_DATE_SZ);
		if (dbflag)
			(void)fprintf(stderr, "auditd: revised oldname is %s\n", 
					oldname);
	}

	/*
	 * Write the trailer record and rename and close the file.
	 * If any of the write, rename, or close fail, ignore it
	 * since there is not much else we can do.
	 */
	(void)gettimeofday(&tv, (struct timezone *)0);
	len = (strlen(newname) + 2) & ~1;
	atlen = sizeof(audit_trailer_t) + len;
	at = (audit_trailer_t *)malloc(atlen);
	if (at == NULL) {
		perror("auditd");
		exit(1);
	}
	at->at_record_size = atlen;
	at->at_record_type = AUR_TRAILER;
	at->at_time = tv.tv_sec;
	at->at_namelen = len;
	(void)strcpy((char *)&at[1], newname);
	(void)write(logfile->l_fd, (caddr_t)at, (int)atlen);
	free((caddr_t)at);

	(void)close(logfile->l_fd);
	(void)rename(logfile->l_name, oldname);
	if (dbflag)
		(void)fprintf(stderr, "auditd: Log closed %s\n", logfile->l_name);
	free((caddr_t)logfile->l_name);
	free((caddr_t)logfile);
}



/********************************************************************
 * getauditdate - get the current time (GMT) and put it in the form
 *		  yyyymmddHHMMSS .
 ********************************************************************/
getauditdate(date)
	char *date;
{
	struct timeval tp;
	struct timezone tzp;
	struct tm tm, *gmtime();

	if (dbflag)
		(void)fprintf(stderr, "auditd: inside getauditdate\n");
	(void)gettimeofday(&tp, &tzp);
	tm = *gmtime(&tp.tv_sec);
	/*
	 * NOTE:  if we want to use gmtime, we have to be aware that the
	 *	structure only keeps the year as an offset from TM_YEAR_BASE.
	 *      I have used TM_YEAR_BASE in this code so that if they change
	 *	this base from 1900 to 2000, it will hopefully mean that this
	 *	code does not have to change.  TM_YEAR_BASE is defined in
	 *	tzfile.h .
	 */
	(void)sprintf(date, "%.4d%.2d%.2d%.2d%.2d%.2d",
			tm.tm_year + TM_YEAR_BASE, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (dbflag)
		(void)fprintf(stderr, "auditd: auditdate = %s\n", date);
	}



/**************************************************************************
 * loadauditlist - read the directory list from the audit_control file.
 *		   Creates a circular linked list of directories.  Also
 *		   gets the minfree value from the file.
 * globals -
 *	thisdir and startdir - both are left pointing to the start of the
 *			       directory list
 *	minfree - the soft limit value to use when filling the directories 
 **************************************************************************/
loadauditlist()
{
	char		buf[200];
	dirlist_t	*node;
	dirlist_t	**nodep;
	int		acresult;
	int		temp;
	int		wait_count;

	if (dbflag)
		(void)fprintf(stderr, "auditd: Loading audit list from %s\n",
				auditcontrol);

	/*
	 * Free up the old directory list
	 */
	if (startdir) {
		thisdir = startdir->dl_next;
		while (thisdir != startdir) {
			node = thisdir->dl_next;
			free((caddr_t)thisdir);
			thisdir = node;
		}
		free((caddr_t)startdir);
	}

	/*
	 * Build new directory list
	 */
	nodep = &startdir;
	wait_count = 0;
	while ((acresult = getacdir(buf, sizeof(buf))) != -1) {
		if (acresult != 0) {
			/*
			 * there was a problem getting the directory
			 * list from the audit_control file
			 */
			if (dbflag && ++wait_count == 1)
				(void)fprintf(stderr, "auditd: problem getting directory list from audit_control.\n");
			(void)dowarn("getacdir", "", wait_count);
			/*
			 * sleep for SLEEP_TIME seconds - temp would
			 * be set if the user asked us to re-read the
			 * audit_control file during the sleep.  Since
			 * that is exactly what we are trying to do,
			 * this value can be ignored.
			 */
			if (wait_count == 1)
				(void)logpost(getpid(), "");
			my_sleep(&temp);
			continue;
		}
		node = (dirlist_t *)malloc(sizeof(dirlist_t));
		if (node == NULL) {
			perror("auditd");
			exit(1);
		}
		node->dl_name = (char *)malloc((unsigned)strlen(buf) + 1);
		if (node->dl_name == NULL) {
			perror("auditd");
			exit(1);
		}
		(void)strcpy(node->dl_name, buf);
		node->dl_next = 0;
		*nodep = node;
		nodep = &(node->dl_next);
	}
	node->dl_next = startdir;

	if (dbflag) {
		/* print out directory list */
		(void)fprintf(stderr, "Directory list:\n%s\n", startdir->dl_name);
		thisdir = startdir->dl_next;
		while (thisdir != startdir) {
			(void)fprintf(stderr, "%s\n", thisdir->dl_name);
			thisdir = thisdir->dl_next;
		}
	}
	thisdir = startdir;

	/*
	 * Get the minfree value
	 */
	if (getacmin(&minfree) != 0)
                minfree = 0;
        endac();
        if (minfree < 0 || minfree > 100)
                minfree = 0;
        if (dbflag)
                (void)fprintf(stderr, "auditd: new minfree: %d\n", minfree);
}



/***********************************************************************
 * logpost - post the new audit log file name to audit_data.
 ***********************************************************************/
logpost(pid, name)
        int             pid;
        char            *name;
{
        register short  fd;
        register FILE   *f;
        register int    error   = 0;

	if (dbflag)
		(void)fprintf(stderr, "auditd: Posting %d, %s to %s\n",
			pid, name, auditdata);
	/*
	 * Ignore the result of unlink since we don't care whether it
	 * already existed or not.
	 */
	(void)unlink(tempfile);
        if ((fd = open(tempfile, O_RDWR|O_CREAT|O_EXCL, 0600)) < 0) {
		(void)fprintf(stderr, "auditd: can't open %s.  Auditd exiting!\n",
			tempfile);
		perror("auditd");
		(void)dowarn("tmpfile", "", 0);
                exit(1);
        }
        f = fdopen(fd, "w");
	if (f == NULL) {
		/* this should not happen */
		(void)fprintf(stderr, "auditd: can't fdopen %s,%d.  Auditd exiting!\n",
			tempfile, fd);
		(void)dowarn("tmpfile", "", 0);
		exit(1);
	}
        (void)fprintf(f, "%d:%s\n", pid, name);
        if (fflush(f) == EOF) {
                (void)fprintf(stderr, "auditd: Can't write audit_data\n");
                error = -1;
        }
	if (!error && fsync(fd) < 0) {
		perror("auditd");
		error = -1;
	}
        (void)fclose(f);
        if (!error && rename(tempfile, auditdata) < 0) {
                perror("auditd");
                error = -1;
        }
        if (error) {
		if (dbflag)
			perror("auditd");
                (void)unlink(tempfile);
	}
        return(error);
}




/*********************************************************************
 * testspace - determine whether the given directorie's filesystem
 *	       has the given amount of space.  Also set the space
 *	       value in the directory list structure.
 * globals -
 *	minfree - what the soft limit is (% of filesystem to reserve)
 *	limit - whether we are using soft or hard limits
 ********************************************************************/
testspace(thisdir, test_limit)
	dirlist_t *thisdir;
	int test_limit;
{
	struct statfs sb;

	if (dbflag)
		(void)fprintf(stderr, "auditd: checking %s for space limit %d\n",
			thisdir->dl_name, test_limit);

	if (thisdir->dl_space == STAY_FULL) {
		thisdir->dl_space = SPACE_FULL;
		minfreeblocks = 0;
	} else if (statfs(thisdir->dl_name, &sb) < 0) {
		if (dbflag)
			perror("auditd");
		thisdir->dl_space = SPACE_FULL;
		minfreeblocks = 0;
	} else {
		minfreeblocks = minfree * (sb.f_blocks / 100);
		if (dbflag)
			(void)fprintf(stderr, "auditd: bavail = %d, minblocks = %d\n",
				sb.f_bavail, minfreeblocks);
		if (sb.f_bavail < AVAIL_MIN)
			thisdir->dl_space = SPACE_FULL;
		else if (sb.f_bavail < minfreeblocks)
			thisdir->dl_space = HARD_SPACE;
		else
			thisdir->dl_space = SOFT_SPACE;

	}
	
	return(thisdir->dl_space == test_limit);
}



/************************************************************************
 * dowarn - invoke the shell script auditwarn to notify the adminstrator
 *	    about the given problem.
 * parameters -
 *	option - what the problem is
 *	logfile - used with options soft and hard: which file was being
 *		   used when the filesystem filled up
 *	count - used with option allhard: how many times auditwarn has
 *		been called for this problem
 ***********************************************************************/
dowarn(option, filename, count)
	char *option;
	char *filename;
	int count;
{
	register int pid;
	int st;
	char countstr[5];
	char warnstring[80];
        register short  fd;
        register FILE   *f;
        register int    error   = 0;

	if (dbflag)
		(void)fprintf(stderr, "auditd: calling %s with %s %s %d\n",
			auditwarn, option, filename, count);

	if (!nflag) {
		if ((pid = fork()) == -1) {
			(void)fprintf(stderr, "auditd: fork failed\n");
			return;
		} else if (pid != 0) {
			pid = wait((union wait *)&st);
			return;
		}
	}
	/*
	 * Set our effective uid back to root so that audit_warn can 
	 * write to the console if it needs to.
	 */
	if (setreuid(0, 0) < 0)
		perror("auditd");
	if (strcmp(option, "soft") == 0 || strcmp(option, "hard") == 0) {
		(void)sprintf(warnstring, "%s limit exceeded in %s.\n",
			option, filename);
		execl(auditwarn, auditwarn, option, filename, 0);
	} else if (strcmp(option, "allhard") == 0) {
		(void)sprintf(warnstring, "All audit filesystems are full.\n");
		(void)sprintf(countstr, "%d", count);
		execl(auditwarn, auditwarn, option, countstr, 0);
	} else if (strcmp(option, "getacdir") == 0) {
		(void)sprintf(warnstring, "Problem getting directories from audit_control.\n");
		(void)sprintf(countstr, "%d", count);
		execl(auditwarn, auditwarn, option, countstr, 0);
	} else {
		(void)sprintf(warnstring, "Need attention for error %s.\n",
			option);
		execl(auditwarn, auditwarn, option, 0);
	}

	/*
	 * If get here, execls above failed.
	 */
	perror("auditd");
        if ((fd = open("/dev/console", O_WRONLY)) < 0) {
		if (dbflag)
			perror("auditd");
		exit(1);
        }
        f = fdopen(fd, "w");
	if (f == NULL) {
		/* this should not happen */
		if (dbflag)
			(void)fprintf(stderr, "auditd: couldn't fdopen /dev/console");
		exit(1);
	}
        (void)fprintf(f, "auditd: Could not execute %s\n", auditwarn);
        (void)fprintf(f, "auditd: %s\n", warnstring);
        if (fflush(f) == EOF) {
		if (dbflag)
			(void)fprintf(stderr, "auditd: Can't write audit_data\n");
                error = -1;
        }
	if (!error && fsync(fd) < 0) {
		if (dbflag)
			perror("auditd");
		error = -1;
	}
        (void)fclose(f);
	if (close(fd) < 0)
		if (dbflag)
			perror("auditd");
	exit(1);
}
