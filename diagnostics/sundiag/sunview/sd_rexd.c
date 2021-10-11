#ifndef lint
static  char sccsid[] = "@(#)sd_rexd.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * sd_rexd - Sundiag's remote execution daemon based on rexd.
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mntent.h>
#include <errno.h>
#include <rpcsvc/rex.h>
#include "sundiag_rpcnums.h"

# define ListnerTimeout 300	/* seconds listner stays alive */
# define WaitLimit 10		/* seconds to wait after io is closed */
# define TempDir "/tmp_rex"	/* directory to hold temp mounts */
# define TempName "/tmp_rex/sd_rexdXXXXXX"
				/* name template for temp mount points */
# define TempMatch 13		/* unique prefix of above */
#define PROGRAMNAME	"sd_rexd"

SVCXPRT *ListnerTransp;		/* non-null means still a listner */
static char **Argv;		/* saved argument vector (for ps) */
static char *LastArgv;		/* saved end-of-argument vector */
int OutputSocket;		/* socket for stop/cont notification */
int MySocket;			/* transport socket */
int HasHelper = 0;		/* must kill helpers (interactive mode) */
int debug = 0;

extern int Master;		/* half of the pty */

main(argc, argv)
	int argc;
	char **argv;
{
	  /*
	   * the server is a typical RPC daemon, except that we only
	   * accept TCP connections.
	   */
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	int dorex(), ListnerTimer(), CatchChild();
	char c;
	int  fdes = 0;

	/*
	 * Remember the start and extent of argv for setproctitle().
	 * Open the console for error printouts, but don't let it be
	 * our controlling terminal.
	 */
	
	if (geteuid()) {
		fprintf(stderr, "sd_rexd: must be super-user to run\n");
		exit(1);
	}

	while ((c=getopt(argc, argv, "d")) != EOF) {
		switch(c) {
		case 'd':
			debug = 1;
			break;
		}
	}
	if (debug)
		printf("%s started.\n", PROGRAMNAME);
	Argv = argv;
	if (argc > 0)
		LastArgv = argv[argc-1] + strlen(argv[argc-1]);
	else
		LastArgv = NULL;
	close(1); /* close stdout */
	close(2); /* close stderr */
		/*
		 * redirect stdout (file descriptor 1) to console
		 * since open returns the lowest number fd available.
		 */
	/* fdes = open("/dev/console", 1); */
	fdes = open("/dev/tty", 1);
	if (debug)
		printf("opening /dev/console returned fd = %d\n", fdes);
	fdes = dup(1);
	if (debug)
		printf("dup(1) returned fd = %d\n", fdes);
	/* NoControl(); */
	signal(SIGCHLD, CatchChild);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, ListnerTimer);
	if (issock(0)) {
		if (debug)
			printf("started from inetd.\n");
		/*
		 * Started from inetd: use fd 0 as socket
		 */
		if ((ListnerTransp = svctcp_create(0, 0, 0)) == NULL) {
			fprintf(stderr, "sd_rexd: svctcp_create error\n");
			exit(1);
		}
		if (!svc_register(ListnerTransp, SD_REXPROG, SD_REXVERS, 
			dorex, 0)) {
			fprintf(stderr, "sd_rexd: service register error\n");
			exit(1);
		}
		alarm(ListnerTimeout);
	} else {
		/*
		 * Started from shell, background thyself and run forever.
		 */
		int pid = fork();

		if (pid < 0) {
			perror("rpc.rexd: can't fork");
			exit(1);
		}
		if (pid) {
			if (debug) printf("parent exiting. pid = %d.\n", pid);
			exit(0);
		}

		if ((ListnerTransp = svctcp_create(RPC_ANYSOCK, 0, 0)) 
				== NULL) {
			fprintf(stderr, "sd_rexd: svctcp_create: error\n");
			exit(1);
		}
		if (debug)
			printf("started from shell.  pid = %d.\n", pid);
		pmap_unset(SD_REXPROG, SD_REXVERS);
		if (!svc_register(ListnerTransp, SD_REXPROG, SD_REXVERS, 
				dorex, IPPROTO_TCP)) {
			fprintf(stderr, "sd_rexd: service rpc register: error\n");
			exit(1);
		}
		if (debug)
			printf("dorex registered.\n");
	}

	/*
	 * Create a private temporary directory to hold sd_rexd's mounts
	 */
	if (mkdir (TempDir, 0777) < 0) {
		if (errno != EEXIST) {
			perror ("sd_rexd: mkdir");
			fprintf (stderr, 
				 "sd_rexd: can't create temp directory %s\n",
				 TempDir);
			exit (1);
		} else if (debug)
			printf("temp directory %s already exists.\n", TempDir);
	} else if (debug)
		printf("made temp directory %s.\n", TempDir);

/*
 * normally we would call svc_run() at this point, but we need to be informed
 * of when the RPC connection is broken, in case the other side crashes.
 */
	while (TRUE) {
	    extern int errno;
	    fd_set readfds;

	    if (MySocket && !FD_ISSET(MySocket, &svc_fdset) ) {
		char *waste;
		   /*
		    * This is when the connection dies for some 
		    * random reason, e.g. client crashes.
		    */
		(void) rex_wait(&waste);
		rex_cleanup();
		exit(1);
	    }
	    readfds = svc_fdset;
	    switch (select(FD_SETSIZE, &readfds, (int *)0, (int *)0, 0)) {
	      case -1:  if (errno == EINTR) continue;
	      		perror("sd_rexd: select failed");
			exit(1);
	      case 0: 
	      		fprintf(stderr,"sd_rexd: Select returned zero\r\n");
			continue;
	      default:
	      		if (HasHelper) HelperRead(&readfds);
			svc_getreqset(&readfds);
	    }
	}
}


/*
 * This function gets called after the listner has timed out waiting
 * for any new connections coming in.
 */
ListnerTimer()
{
  svc_destroy(ListnerTransp);
  exit(0);
}


/*
 * dorex - handle one of the rex procedure calls, dispatching to the 
 *	correct function.
 */
dorex(rqstp, transp)
	register struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	struct rex_start *rst;
	struct rex_result result;

	
	if (ListnerTransp) {
		  /*
		   * First call - fork a server for this connection
		   */
		int fd, pid, count;

		if (debug)
			printf("entering dorex. still a listner.\n");
		for (count=0; (pid = fork()) < 0; count++) {
			if (count > 4) {
				perror("sd_rexd: cannot fork");
				break;
			}
			sleep(5);
		}
		if (pid != 0) {
		    /*
		     * Parent - return to service loop to accept further
		     * connections.
		     */
			if (debug)
			    printf("dorex: parent; return to service loop.\n");
			/* alarm(ListnerTimeout); */
			svc_destroy (transp);
			return;
		}
		  /*
		   * child - close listner transport to avoid confusion
		   * Also need to close all other service transports
		   * besides the one we are interested in.
		   * Save ours so that we know when it goes away.
		   */
		alarm(0);
		if (transp != ListnerTransp) {
			close(ListnerTransp->xp_sock);
			xprt_unregister(ListnerTransp);
			if (debug)
				printf("transp != ListnerTransp\n");
		}
		ListnerTransp = NULL;
		MySocket = transp->xp_sock;
		for (fd=0; fd<FD_SETSIZE; fd++)
		  if (fd != transp->xp_sock && FD_ISSET(fd, &svc_fdset) ) {
			close(fd);
			FD_CLR(fd, &svc_fdset);
		  }
		if (debug)
			printf("dorex: child; closed listner transport.\n");
	}

	switch (rqstp->rq_proc) {
	case NULLPROC:
		if (svc_sendreply(transp, xdr_void, 0) == FALSE) {
			fprintf(stderr, "sd_rexd: nullproc err");
			exit(1);
		}
		return;

	case REXPROC_START:
		rst = (struct rex_start *)malloc(sizeof (struct rex_start));
		bzero((char *)rst, sizeof *rst);
		if (svc_getargs(transp, xdr_rex_start, rst) == FALSE) {
			svcerr_decode(transp);
			exit(1);
		}
		if (rqstp->rq_cred.oa_flavor != AUTH_UNIX) {
			svcerr_auth(transp);
			exit(1);
		}
		result.rlt_stat = rex_start(rst,
			(struct authunix_parms *)rqstp->rq_clntcred,
			&result.rlt_message, transp->xp_sock);
		if (svc_sendreply(transp, xdr_rex_result, &result) == FALSE) {
			fprintf(stderr, "sd_rexd: reply failed\n");
			rex_cleanup();
			exit(1);
		}
		if (result.rlt_stat) {
			rex_cleanup();
			exit(0);
		}
		return;

	case REXPROC_MODES:
		{
		    struct rex_ttymode mode;
		    if (svc_getargs(transp, xdr_rex_ttymode, &mode)==FALSE) {
			svcerr_decode(transp);
			exit(1);
		    }
		    SetPtyMode(&mode);
		    if (svc_sendreply(transp, xdr_void, 0) == FALSE) {
			fprintf(stderr, "sd_rexd: mode reply failed");
			exit(1);
		    }
		}
		return;

	case REXPROC_WINCH:
		{
		    struct ttysize size;
		    if (svc_getargs(transp, xdr_rex_ttysize, &size)==FALSE) {
			svcerr_decode(transp);
			exit(1);
		    }
		    SetPtySize(&size);
		    if (svc_sendreply(transp, xdr_void, 0) == FALSE) {
			fprintf(stderr, "sd_rexd: window change reply failed");
			exit(1);
		    }
		}
		return;

	case REXPROC_SIGNAL:
		{
		    int sigNumber;

		    if (svc_getargs(transp, xdr_int, &sigNumber)==FALSE) {
			svcerr_decode(transp);
			exit(1);
		    }
		    SendSignal(sigNumber);
		    if (svc_sendreply(transp, xdr_void, 0) == FALSE) {
			fprintf(stderr, "sd_rexd: signal reply failed");
			exit(1);
		    }
		}
		return;

	case REXPROC_WAIT:
		result.rlt_stat = rex_wait(&result.rlt_message);
		if (svc_sendreply(transp, xdr_rex_result, &result) == FALSE) {
			fprintf(stderr, "sd_rexd: reply failed\n");
			exit(1);
		}
		rex_cleanup();
		exit(0);
		/* NOTREACHED */

	default:
		svcerr_noproc(transp);
		exit(1);
	}
}

int child = 0;			/* pid of the executed process */
int ChildStatus = 0;		/* saved return status of child */
int ChildDied = 0;		/* true when above is valid */
char nfsdir[MAXPATHLEN];	/* file system we mounted */
char *tmpdir;			/* where above is mounted, NULL if none */

/*
 * signal handler for SIGCHLD - called when user process dies or is stopped
 */
CatchChild()
{
  int pid;
  union wait status;
  
  while ((pid = wait3(&status, WNOHANG|WUNTRACED, NULL)) > 0) {
    if (pid==child) {
      if (WIFSTOPPED(status)) {
	  send(OutputSocket, "", 1, MSG_OOB);	/* tell remote client to stop */
	  sigpause(0);				/* wait for SIGURG */
	  killpg(child, SIGCONT);		/* restart child */
	  return;
      }

      ChildStatus = status.w_retcode;
      ChildDied = 1;

      if (HasHelper && !FD_ISSET (Master, &svc_fdset)) {
	  KillHelper(child);
	  HasHelper = 0;
	}
    }
  }
}

/*
 * oob -- called when we should restart the stopped child.
 */
oob()
{
	int atmark;
	char waste[BUFSIZ], mark;

	for (;;) {
		if (ioctl(OutputSocket, SIOCATMARK, &atmark) < 0) {
			perror("ioctl");
			break;
		}
		if (atmark)
			break;
		(void) read(OutputSocket, waste, sizeof (waste));
	}
	(void) recv(OutputSocket, &mark, 1, MSG_OOB);
}

/*
 * rex_wait - wait for command to finish, unmount the file system,
 * and return the exit status.
 * message gets an optional string error message.
 */
rex_wait(message)
	char **message;
{
	static char error[1024];
	int count;

	*message = error;
	strcpy("",error);
	if (child == 0) {
		errprintf(error,"No process to wait for!\n");
		rex_cleanup();
		return (1);
	}
	kill(child, SIGHUP);
	for (count=0;!ChildDied && count<WaitLimit;count++)
		sleep(1);
	if (ChildStatus & 0xFF)
		return (ChildStatus);
	return (ChildStatus >> 8);
}


/*
 * cleanup - unmount and remove our temporary directory
 */
rex_cleanup()
{
	if (tmpdir) {
		if (child && !ChildDied) {
		    fprintf(stderr,
		    "sd_rexd: child killed to unmount %s\r\n", nfsdir);
		    kill(child, SIGKILL);
		}
		chdir("/");
		if (nfsdir[0] && umount_nfs(nfsdir, tmpdir))
			fprintf(stderr,
				 "sd_rexd: couldn't umount %s from %s\r\n", 
				   nfsdir, tmpdir);
		if (rmdir(tmpdir) < 0)
		   if (errno != EBUSY) perror("rmdir");
		tmpdir = NULL;
	}
      if (HasHelper) KillHelper(child);
      HasHelper = 0;
}


/*
 * This function does the server work to get a command executed
 * Returns 0 if OK, nonzero if error
 */
rex_start(rst, ucred, message, sock)
	struct rex_start *rst;
	struct authunix_parms *ucred;
	char **message;
	int sock;
{
	char *index(), *mktemp();
	char hostname[255];
	char *p, *wdhost, *fsname, *subdir;
	char dirbuf[1024];
	static char error[1024];
	char defaultShell[1024];	/* command executed if none given */
	char defaultDir[1024];		/* directory used if none given */
	struct sockaddr_in sin;
	int len;
	int fd0, fd1, fd2;
	extern char **environ;
	int icount = 0;

	if (child) {	/* already started */
		killpg(child, SIGKILL);
		return (1);
	}
	*message = error;
	(void) strcpy(error, "");
	signal(SIGCHLD, CatchChild);

	if (ValidUser(ucred->aup_machname, ucred->aup_uid,error,	
			defaultShell, defaultDir, rst->rst_cmd[0]))
		return(1);
	if (rst->rst_fsname && strlen(rst->rst_fsname)) {
		fsname = rst->rst_fsname;
		subdir = rst->rst_dirwithin;
		wdhost = rst->rst_host;
	} else {
		fsname = defaultDir;
		subdir = "";
		wdhost = hostname;
	}		
	gethostname(hostname, 255);
	if (strcmp(wdhost, hostname) == 0) {
		  /*
		   * The requested directory is local to our machine,
		   * so just change to it.
		   */
		strcpy(dirbuf,fsname);
	} else {
		static char wanted[1024];
		static char mountedon[1024];

		strcpy(wanted,wdhost);
		strcat(wanted,":");
		strcat(wanted,fsname);
		if (AlreadyMounted(wanted,mountedon)) {
		  /*
		   * The requested directory is already mounted.  If the
		   * mount is not by another rexd, just change to it. 
		   * Otherwise, mount it again.  If just changeing to
		   * the mounted directy, be careful. It might be mounted 
		   * in a different place.
		   * (dirbuf is modified in place!)
		   */
		  if (strncmp(mountedon, TempName, TempMatch) == 0) {
		    /* */
		    tmpdir = mktemp (TempName);
		    if (mkdir (tmpdir, 0777)) {
		      perror (tmpdir);
		      audit_note (2, tmpdir);
		      return (1);
		    }
		    strcpy(nfsdir, wanted);
		    if (mount_nfs (wanted, tmpdir, error)) {
		      audit_note (1,error);
		      return (1);
		    }
		    strcpy (dirbuf, tmpdir);
		  } else
		    strcpy(dirbuf, mountedon);
		}
		else {
		  /*
		   * The requested directory is not mounted anywhere,
		   * so try to mount our own copy of it.  We set nfsdir
		   * so that it gets unmounted later, and tmpdir so that
		   * it also gets removed when we are done.
		   */
			tmpdir = mktemp(TempName);
			if (mkdir(tmpdir, 0777)) {
				perror(tmpdir);
				audit_note(1, tmpdir );
				return(1);
			}
			strcpy(nfsdir, wanted);
			if (mount_nfs(wanted, tmpdir, error)) {
				audit_note(1, error);
				return(1);
			}
			strcpy(dirbuf, tmpdir);
		}
	}
	  /*
	   * "dirbuf" now contains the local mount point, so just tack on
	   * the subdirectory to get the pathname to which we "chdir"
	   */
	strcat(dirbuf, subdir);

	len = sizeof sin;
	if (getpeername(sock, &sin, &len)) {
		perror("getpeername");
		audit_note(1, "getpeername" );
		return(1);
	}
	fd0 = socket(AF_INET, SOCK_STREAM, 0);
	fd0 = doconnect(&sin, rst->rst_port0,fd0);
	OutputSocket = fd0;
	   /*
	    * Arrange for fd0 to send the SIGURG signal when out-of-band data
	    * arrives, which indicates that we should send the stopped child a
	    * SIGCONT signal so that we can resume work.
	    */
	(void) fcntl(fd0, F_SETOWN, getpid());
	signal(SIGURG, oob);
	if (rst->rst_port0 == rst->rst_port1) {
	  /*
	   * use the same connection for both stdin and stdout
	   */
		fd1 = fd0;
	}
	if (rst->rst_flags & REX_INTERACTIVE) {
	  /*
	   * allocate a pseudo-terminal if necessary
	   */
	   if (AllocatePtyMaster(fd0,fd1)) {
	   	errprintf(error,"sd_rexd: cannot allocate a pty\n");
		audit_note(1, error);
		return (1);
	   }
	   HasHelper = 1;
	}
	child = fork();
	if (child < 0) {
		errprintf(error, "sd_rexd: can't fork\n");
		audit_note(1, error);
		return (1);
	}
	if (child) {
	    /*
	     * parent sd_rexd: close network connections if needed,
	     * then return to the main loop.
	     */
		if ((rst->rst_flags & REX_INTERACTIVE)==0) {
			close(fd0);
			close(fd1);
		}
		return (0);
	}

	/* child rexd */
	if (rst->rst_flags & REX_INTERACTIVE)
	  AllocatePtySlave();

	if (rst->rst_port0 != rst->rst_port1) {
		fd1 = socket(AF_INET, SOCK_STREAM, 0);
		shutdown(fd0, 1);
		fd1 = doconnect(&sin, rst->rst_port1,fd1);
		shutdown(fd1, 0);
	}
	if (rst->rst_port1 == rst->rst_port2) {
	  /*
	   * Use the same connection for both stdout and stderr
	   */
		fd2 = fd1;
	} else {
		fd2 = socket(AF_INET, SOCK_STREAM, 0);
		fd2 = doconnect(&sin, rst->rst_port2,fd2);
		shutdown(fd2, 0);
	}
	if (rst->rst_flags & REX_INTERACTIVE) {
	  /*
	   * use ptys instead of sockets in interactive mode
	   */
	   DoHelper(&fd0, &fd1, &fd2);
	}
	dup2(fd0, 0);
	dup2(fd1, 1);
	dup2(fd2, 2);
	for (fd0 = 3; fd0 < getdtablesize(); fd0++)
		close(fd0);
	environ = rst->rst_env;
	setgid(ucred->aup_gid);
	setgroups(ucred->aup_len,ucred->aup_gids);
	/*
	 * write audit record before making uid switch  
	 */
	audit_note(0, "successful"); 
	setuid(ucred->aup_uid);
	if (debug) {
		printf("ucred->aup_machname = %s\n", ucred->aup_machname);
		printf("ucred->aup_uid = %d\n", ucred->aup_uid);
		printf("ucred->aup_gid = %d\n", ucred->aup_gid);
	}
	if (chdir(dirbuf)) {
		perror("rex_start");
	  	fprintf(stderr, "sd_rexd: can't chdir to %s\n", dirbuf);
		exit(1);
	}
	signal( SIGINT, SIG_DFL);
	signal( SIGHUP, SIG_DFL);
	signal( SIGQUIT, SIG_DFL);
	if (rst->rst_cmd[0]==NULL) {
	  /*
	   * Null command means execute the default shell for this user
	   */
	    char *args[2];

	    args[0] = defaultShell;
	    args[1] = NULL;
	    execvp(defaultShell, args);
	    fprintf(stderr, "sd_rexd: can't exec shell %s\n", defaultShell);
	    exit(1);
	}
	if (debug) {
		printf("command line:  ");
		icount = 0;
		while (rst->rst_cmd[icount] != NULL) {
			printf("%s ", rst->rst_cmd[icount]);
			icount++;
		}
		printf("\n");
	}
	execvp(rst->rst_cmd[0], rst->rst_cmd);
	perror("sd_rexd: rex_start:");
	fprintf(stderr, "sd_rexd: can't exec %s\n", rst->rst_cmd[0]);
	exit(1);
}

AlreadyMounted(fsname, mountedon)
    char *fsname;
    char *mountedon;
{
  /*
   * Search the mount table to see if the given file system is already
   * mounted.  If so, return the place that it is mounted on.
   */
   FILE *table;
   register struct mntent *mt;

   table = setmntent(MOUNTED,"r");
   if (debug)
	printf("MOUNTED = %s; fsname = %s; mountedon = %s\n",
		MOUNTED, fsname, mountedon);
   if (table == NULL)
     return (0);
   while ( (mt = getmntent(table)) != NULL) {
   	if (strcmp(mt->mnt_fsname,fsname) == 0) {
	    if (debug)
		printf("mt->mnt_fsname = %s; mt->mnt_dir = %s\n",
			mt->mnt_fsname, mt->mnt_dir);
	    strcpy(mountedon,mt->mnt_dir);
	    endmntent(table);
	    return(1);
	}
   }
   endmntent(table);
   return(0);
}


/*
 * connect to the indicated IP address/port, and return the 
 * resulting file descriptor.  
 */
doconnect(sin, port, fd)
	struct sockaddr_in *sin;
	short port;
	int fd;
{
	sin->sin_port = ntohs(port);
	if (connect(fd, sin, sizeof *sin)) {
		perror("sd_rexd: connect");
		exit(1);
	}
	return (fd);
}

/*
*  SETPROCTITLE -- set the title of this process for "ps"
*
*	Does nothing if there were not enough arguments on the command
* 	line for the information.
*
*	Side Effects:
*		Clobbers argv[] of our main procedure.
*/

setproctitle(user, host)
	char *user, *host;
{
	register char *tohere;

	tohere = Argv[0];
	if (LastArgv == NULL || 
	    strlen(user)+strlen(host)+3 > (LastArgv - tohere))
		return;
	*tohere++ = '-';	/* So ps prints (rpc.rexd) */
	sprintf(tohere, "%s@%s", user, host);
	while (*tohere++) ;		/* Skip to end of printf output */
	while (tohere < LastArgv) *tohere++ = ' ';  /* Avoid confusing ps */
}


/*
 * Determine if a descriptor belongs to a socket or not 
 */
issock(fd)
	int fd;
{
	struct stat st;

	if (fstat(fd, &st) < 0) {
		return (0);
	}
	/*
	 * SunOS returns S_IFIFO for sockets, while 4.3 returns 0 and does not
	 * even have an S_IFIFO mode.  Since there is confusion about what the
	 * mode is, we check for what it is not instead of what it is. 
	 */
	switch (st.st_mode & S_IFMT) {
	case S_IFCHR:
	case S_IFREG:
	case S_IFLNK:
	case S_IFDIR:
	case S_IFBLK:
		return (0);
	default:
		return (1);
	}
}
