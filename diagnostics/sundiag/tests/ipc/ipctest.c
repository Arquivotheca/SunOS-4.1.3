/*
 * @(#)ipctest.c 1.1 7/30/92
 *
 * ipctest.c:  Sundiag diagnostics for the IPC (Integrated PC) board.
 *
 * Ipctest is Sundiag's interface to the IPC tests.  Ipctest will fork/exec
 * pctool to run a set of tests in the /usr/pctool/sysex directory.  All
 * testing is done by the IPC software.    The ipctest must work together
 * with pctool-listener, which is invoked by pc.bat whenever pc.bat writes
 * to LPT1 (i.e., ipctest sets the LPT1 environment variable so that
 * pctool-listener would get invoked.)
 *
 * compile:  cc -g ipctest.c -o ipctest -I../include ../lib/libtest.a
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <signal.h>
#include "sdrtns.h"			/* sundiag standard include */
#include "../../../lib/include/libonline.h"	/* online library include */
#include "ipctest.h"

int	timeout_code = 0;		/* reason for timeout (alarm signal) */
int	pctool_pid = 1000;		/* process id of pctool		*/
char	location[12][4] = {		/* location of pctool window	*/
		"-Wp","0","810",	/*	board 0			*/
		"-Wp","0","850",	/*	board 1			*/
		"-Wp", "650","810",	/*	board 2			*/
		"-Wp","650","850"	/*	board 3			*/
	};
char	sdmsg[MESSAGE_SIZE];	/* msg buffer for throwup()	*/
char	sockname[20];		/* unix socket name (SOCKNAME + ipcnum */
int	floppytest=FALSE;
int	pportest=FALSE;
int	descibetest = FALSE;
int	ipcnum=0;		/* ipc board number 		*/
char	env_buf[80];		/* environment variable buffer	*/

main(argc, argv)
	int	argc;
	char	*argv[];
{
	extern int process_ipctest_args();
	extern int routine_usage();
	char	*test_usage_msg = "[ipc#] [D] [P]";
	int	timeout();		/* routine to handle alarm signals   */
	int	messagetype;		/* message type received from pctool */
	char	pcdev[30];		/* string for /dev/pc#		*/
	char	pctoolargs[80];		/* args to pass to pctool on startup */
	char	currentpath[80];	/* abs path where sundiag resides    */

	versionid = "1.1";		/* SCCS version id */
        strcpy(device_name, "");
				/* begin Sundiag test */
	test_init (argc, argv, process_ipctest_args, routine_usage, 
		test_usage_msg);
	(void)signal(SIGALRM, timeout);
	bzero(env_buf, 80);

	if (descibetest) {
	    test_description();
	    exit(0);
	}
	sprintf(device_name, "/dev/pc%d", ipcnum);
	sprintf(sockname, "%s%d", SOCKNAME, ipcnum);
	if (strlen(env_buf) == 0) { /* if not user specified, set to default */
	    getwd(currentpath);
	    sprintf(env_buf, "LPT1=%s/%s", currentpath, LISTNER);
	}
	putenv(env_buf);
	sprintf(pcdev, "%s%s/dev/pc%d",
	    (hostname==NULL)?"":hostname, (hostname==NULL)?"":":", ipcnum);
	sprintf(pctoolargs, "cd %s; pc %d sd %s%s%s", SYSEXDIR, ipcnum,
			floppytest ? " f" : "",
			pportest   ? " pp": "",
			verbose    ? " d" : "");
	if ((pctool_pid = vfork()) == -1) {
		perror("ipctest: vfork() failed");
		sprintf(sdmsg, "ipctest: vfork() failed");
		writemsg(1, ERROR, sdmsg);	/* exit */
	}
	if (pctool_pid != 0) {
	    /* parent */
	    if (verbose) {
		sprintf(sdmsg,
"child will execlp:\n     pctool -d %s %s %s %s -c \"%s\"\n", pcdev,
location[ipcnum*3], location[ipcnum*3+1], location[ipcnum*3+2], pctoolargs);
		writemsg(0, VERBOSE, sdmsg);
	    }
	    messagetype = pctool_listen();
	} else {
	    /* child */
	    execlp("./pctool", "./pctool", "-d", pcdev, location[ipcnum*3],
		location[ipcnum*3+1], location[ipcnum*3+2],
		"-c", pctoolargs, (char *)0);
	    perror("ipctest: execlp() pctool failed");
	    sprintf(sdmsg, "execlp pctool failed");
	    writemsg(1, ERROR, sdmsg);	/* exit */
	}
	wait(0);	/* collect zombie process */
	switch(messagetype) {
	case VERBOS:
	case DONE:
	case TERMINATED:
		test_end(); 		/* Sundiag normal exit */
	case LISTEN_ERROR:
	case READ_FAILED:
	case PC_ERROR:
	case INVALID:
	default:
		exit(1);
	}
}

/*
 * Pctool_listen() receives the messages sent out by pc.bat (in pctool).
 *   The messages are actually received 1 line at a time by a separate
 *   process, pctool-listener, which then sends it to ipctest via a unix
 *   socket.  Pc.bat sends the messages to LPT1:, which we set as an
 *   environment variable (setenv LPT1 pctool-listener), and hence
 *   pctool-listener is invoked for each line of output be pc.bat.
 *   Note that pctool will send out only 1 message when not in debug
 *   mode (either ERROR or "completed").  In debug mode, pctool will
 *   send a "beginning" and "completed" message for each test, and
 *   a "Completed" message when all done.
 *
 * Problem in pctool:  in the source code, pctool calls a libsuntool routine
 *   seln_create() that will fail ocassionally whenever the system is very 
 *   busy.    In the libsuntool code, seln_create() calls other routines
 *   which will eventually call the RPC routine clnt_call();  this is the
 *   call that is actually failing because it has a time-out value of
 *   10 seconds.  This probably won't be fixed as there is no plan of
 *   releasing any more IPC software after 1.2.  Therefore, we'll ignore
 *   the condition of pctool dying prematurely.
 *
 * return value:  (int) message-type.  The type of message received from
 *		  pctool just before it exited.
 */

pctool_listen()
{
	int	sock;			/* socket descrip to listen on	*/
	int	ns;			/* sock desc to talk on		*/
	int	addrlen;		/* length of client address	*/
	struct sockaddr_un	name;	/* unix socket name		*/
	struct sockaddr		addr;	/* addr of client process	*/
	char	msgbuf[2*INFOSIZ];
	char	pcinfo[INFOSIZ];
	int	msgtype;		/* type of msg received from pctool */
	int	nread;
	int	timelimit;		/* alarm clock value		*/

	if ( (sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1 ) {
	    perror("ipctest socket() failed");
	    exit(1);
	}
	name.sun_family = AF_UNIX;
	strcpy(name.sun_path, sockname);
	if (bind(sock, (struct sockaddr *)&name,
		 sizeof(struct sockaddr_un)) == -1) {
	    perror("ipctest bind() failed");
	    exit(1);
	}
	addrlen = sizeof(struct sockaddr);
	listen(sock, 1);  /* take only 1 connection at a time */
	/* if not in debug mode, pctool sends only 1 message upon completion.
	 * if debug, then get message upon start and end of each test.
	 */
	timelimit = (verbose) ? VERBOSE_TIMELIM : NORMAL_TIMELIM; 
	/*
	 * We will get a new connection for each line sent by pctool.
	 * Continue to receive the messages until we get an ERROR or DONE.
	 */
	for (;;) {
	    alarm(timelimit); /* don't wait forever to accept socket connect */
	    if ((ns = accept(sock, &addr, &addrlen)) == -1) {
	    	if (timeout_code == CONNECT_FAILED) {
	    	   msgtype = READ_FAILED;
		   if (verbose) {
		       sprintf(sdmsg,
"Pctool never wrote completion message. Pctool may have terminated abnormally.\
 This condition is ignored.");
		       writemsg(INTERRUPT_RETURN, INFO, sdmsg);	/* exit */
		   }
		   clean_up();	/* remove socket from file system */
		   exit(INTERRUPT_RETURN);
	        } else if (timeout_code == STILL_RUNNING)
		   continue;	/* try accept() again */
		else
		   perror("ipctest accept() failed");
		unlink(sockname);
		exit(1);
	    }
	    alarm(0);
	    bzero(pcinfo, sizeof(pcinfo));
	    alarm(25);  /* if problem, don't want to block forever in read() */
	    nread = read(ns, pcinfo, sizeof(pcinfo)); /* read from socket */
	    alarm(0);	/* turn off after returning from read() */
	    if (timeout_code == CONNECT_FAILED) {
		if (verbose) {
		    msgtype = READ_FAILED;
		    sprintf(sdmsg, "Timed out waiting to read from pctool, \
which may have terminated abnormally.  This condition is ignored.\n");
		    writemsg(INTERRUPT_RETURN, INFO, sdmsg);	/* exit */
		}
		clean_up();  /* remove socket from file system */
		exit(INTERRUPT_RETURN);
	    }
	    /*
	     * Null terminate buffer.  The ^M from pctool does strange things
	     * to our buffer if we try to sprintf it.  Better to be rid of it.
	     */
	    if (nread > 0) {
		if ((nread == INFOSIZ) || (pcinfo[nread-1] == ''))
		    pcinfo[nread-1] = '\0';
	    }
	    close(ns);
	    if (verbose) {
		sprintf(msgbuf,
			"ipctest read from pc%d: '%s'\n", ipcnum, pcinfo);
		write(1, msgbuf, strlen(msgbuf));
	    }
	    msgtype = chk_ipc_msg(pcinfo); /* check each line received */
	    if (msgtype != VERBOS)
		break;
	}
	if (msgtype == PC_ERROR) {
	   sprintf(sdmsg, "read from pctool: '%s'", pcinfo);
	   writemsg(1, ERROR, sdmsg);
	} else if (msgtype == INVALID) {
	   sprintf(sdmsg,
		"received unknown msgtype (%d) from pctool. read '%s'",
		msgtype, pcinfo);
	   writemsg(1, ERROR, sdmsg);	/* exit */
	}
	unlink(sockname);	/* remove socket from file system */
	return(msgtype);
}


/*
 * chk_ipc_msg() compares 1 line of input (gotten from pctool) and looks for
 *   the key words "ERROR:" and "INFO:".  If "INFO:", then we look for the
 *   words "Completed" and "Terminated" in the message, telling us that
 *   pctool is done;  otherwise, it's just an information message.
 *   Note that in debug mode, the key word that tells us pctool is done
 *   is "Completed" while in non-debug mode, it's "completed".  Also,
 *   in non-debug mode, only 1 message is sent by pctool, after all tests
 *   are completed or an error condition occured.
 *
 * return value:  an integer (VERBOS, DONE, ERROR, TERMINATED, or INVALID)
 *		  specifying the type of message received.
 */
chk_ipc_msg(msg)
	char	*msg;
{
	char	*strtok();
	char	*token, *msgptr;
	char	*complete_string;

	complete_string = (verbose) ? "Complete" : "complete";
	msgptr = (char *)malloc(strlen(msg));
	strcpy(msgptr, msg);
	if (strncmp(msg, "ERROR:", 6) == 0)
	    return(PC_ERROR);
	else if (strncmp(msg, "INFO:", 5) == 0) {
	    token = strtok(msgptr, " ");
	    while (token != NULL) {
		if (strncmp(token, "Completed", 9) == 0)
		    return(DONE);
		else if (strncmp(token, "Terminated", 10) == 0)
		    return(TERMINATED);
		else
		    token = strtok((char *)0, " ");  /* get next word in msg */
	    }
	} else if (strncmp(msg, LISTEN_ERRMSG) == 0)
		return(LISTEN_ERROR);
	else	/* only ERROR:, INFO: and LISTEN_ERRMSG messages are valid. */
	    return(INVALID);
	return(VERBOS);
}

/*
 * timeout() is the handler for SIGALRM.  We use alarm() to time out after
 *   blocking on accept() system call waiting for the pctool-listener
 *   process to make a connection to the socket.  We also use alarm() to
 *   time out waiting to read from the socket.
 */
timeout()
{
	if (kill(pctool_pid, 0) == 0)
		/* pctool still running, system probably very loaded */
	    timeout_code = STILL_RUNNING;
	else
	    timeout_code = CONNECT_FAILED;
}

/*
 * clean_up() is called by finish() (in sdrtns library) upon receipt of
 *   SIGHUP, SIGTERM, or SIGINT.  Clean_up() will kill pctool and then
 *   remove the unix socket from the file system.
 */
clean_up()
{
	if (kill(pctool_pid, 0) == 0)
		/* send signal to kill pctool */
	    kill(pctool_pid, SIGKILL);
	unlink(sockname);
}

writemsg(retcode, messgtype, message)
	int	retcode, messgtype;
	char	*message;
{
	if (exec_by_sundiag)
	    send_message(retcode, messgtype, message);
	else {
	    write(1, message, strlen(message));
	    if (retcode != 0) {
		clean_up();
		exit(retcode);
	    }
	}
	return(1);
}

process_ipctest_args(argv, arrcount)
   char         *argv[];
   int          arrcount;
{
	if (strncmp(argv[arrcount], "ipc", 3) == 0) {
	    ipcnum = atoi(&argv[arrcount][3]);
	    if ((ipcnum < 0) || (ipcnum > 3)) {
	        printf("%s: invalid ipc number %d.  Using ipc0.\n",
		       test_name, ipcnum);
	        ipcnum = 0;
	    }
	} else if (strncmp(argv[arrcount], "D", 1) == 0) {
	    floppytest = TRUE;
	} else if (strncmp(argv[arrcount], "P", 1) == 0) {
	    pportest = TRUE;
	} else if (strncmp(argv[arrcount], "LPT1", 4) == 0) {
	    strcpy(env_buf, argv[arrcount]);
	} else if (strncmp(argv[arrcount], "U", 1) == 0) {
	    descibetest = TRUE;
	} else {
	    return(FALSE);
	}
	return(TRUE);
}

routine_usage()
{
   (void) printf("Vmem arguments:\n\
        U      = test description.\n\
        ipc#   = the IPC board number to test (default is ipc0)\n\
        D      = include floppy disk test (floppy must be in drive B: only)\n\
        P      = include parallel port test (must have loopback connector)\n");
}

test_description()
{
   (void) printf("\
Test description:\n\
   Ipctest is Sundiag's interface to the IPC tests, which are a part of the\n\
   IPC software.  Ipctest will fork/exec pctool to run a set of tests in the\n\
   /usr/pctool/sysex directory.  All testing is done by the IPC software.\n\
   The ipctest must work together with pctool-listener, which is invoked\n\
   by pc.bat whenever pc.bat writes to LPT1 (i.e., ipctest sets the LPT1\n\
   environment variable so that pctool-listener would get invoked).\n");
   (void) printf("\
   The user can also choose to test the floppy disk drive and/or the\n\
   parallel port.  If chosen, a floppy disk must be inserted in drive B: and\n\
   a loopback connector be connected to the parallel port prior to testing.\n\
   Only drive B: can be tested because any floppy left in drive A: prior\n\
   to starting PCTOOL will cause it to assume the system disk is in drive\n\
   A:.\n");
}
