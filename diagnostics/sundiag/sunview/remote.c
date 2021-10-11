# ifndef lint
static char sccsid[] = "@(#)remote.c 1.1 92/07/30 Copyright Sun Micro";
# endif lint

/*
 * remote - interface program for remote execution service, based on
 *          the program "on."
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 *
 * Compilation:  cc -o remote remote.c -lrpcsvc
 */

#include <stdio.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <errno.h>
#include <mntent.h>
#include <rpcsvc/rex.h>
#include "sundiag_rpcnums.h"

extern int errno;

/*
 * Note - the following must be long enough for at least two portmap
 * timeouts on the other side.
 * 
 * The following value is 360 seconds to allow for net problems.
 */
struct timeval LongTimeout = { 360, 0 };

int Debug = 0;			/* print extra debugging information */
int Only2 = 0;			/* stdout and stderr are the same */

int InOut;			/* socket for stdin/stdout */
int Err;			/* socket for stderr */
char *rhost;			/* remote host name (made global: NY) */

CLIENT *Client;			/* RPC client handle */
struct ttysize WindowSize;	/* saved window size */

  

/*
 * oob -- called when the command invoked by the rexd server is stopped
 *	  with a SIGTSTP or SIGSTOP signal.
 */
oob()
{
	int atmark;
	char waste[BUFSIZ], mark;

	for (;;) {
		if (ioctl(InOut, SIOCATMARK, &atmark) < 0) {
			perror("ioctl");
			break;
		}
		if (atmark)
			break;
		(void) read(InOut, waste, sizeof (waste));
	}
	(void) recv(InOut, &mark, 1, MSG_OOB);
	kill(getpid(), SIGSTOP);
}

cont()
{
}

connect_timeout()
{
	fprintf(stderr, "remote: cannot connect to server on %s\n", rhost);
	exit(1);
}

main(argc, argv)
	int argc;
	char **argv;
{
/*	char *rhost, **cmdp; */
	char **cmdp;
	char curdir[MAXPATHLEN];
	char wdhost[MAXPATHLEN];
	char fsname[MAXPATHLEN];
	char dirwithin[MAXPATHLEN];
	struct rex_start rst;
	struct rex_result result;
	extern char **environ, *rindex();
	enum clnt_stat clstat;
	struct hostent *hp;
	struct sockaddr_in server_addr;
	int sock = RPC_ANYSOCK;
	int selmask, zmask, remmask;
	int nfds, cc;
	static char buf[4096];
	int attempted_con = 0;

	    /*
	     * we check the invoked command name to see if it should
	     * really be a host name.
	     */
	if ( (rhost = rindex(argv[0],'/')) == NULL) {
		rhost = argv[0];
	}
	else {
		rhost++;
	}

	signal(SIGALRM, connect_timeout);
	while (argc > 1 && argv[1][0] == '-') {
	    switch (argv[1][1]) {
	      case 'd': Debug = 1;
	      		break;
	      default:
	      		printf("Unknown option %s\n",argv[1]);
	    }
	    argv++;
	    argc--;
	}
	if (argc < 2)
		usage();
	rhost = argv[1];
	cmdp = &argv[2];

	/*
	 * Can only have one of these
	 */

	if ((hp = gethostbyname(rhost)) == NULL) {
		fprintf(stderr, "remote: unknown host %s\n", rhost);
		exit(1);
	}
	bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = 0;	/* use pmapper */
	if (Debug) printf("Got the host named %s (%s)\n",
			rhost,inet_ntoa(server_addr.sin_addr));
	alarm(300);  /* allow 5 minutes to create a rpc connection */
	while ((Client = clnttcp_create(&server_addr,
					SD_REXPROG, SD_REXVERS,
					&sock, 0, 0)) == NULL) {
		if (attempted_con == 0) {
		  fprintf(stderr,
		    "remote: cannot connect to server on %s.  still trying.\n",
		    rhost);
		  attempted_con++;
		}
	}
	alarm(0);    /* turn off alarm */
	attempted_con = 0;
	if (Debug) printf("TCP RPC connection created\n");
	Client->cl_auth = authunix_create_default();
	  /*
	   * Now that we have created the TCP connection, we do some
	   * work while the server daemon is being swapped in.
	   */
	  /* tell rexd on remote machine we're in root directory regardless
	   * of where we really are.
	   */
	strcpy(curdir, "/");
	if (findmount(curdir,wdhost,fsname,dirwithin) == 0) {
	      fprintf(stderr,"remote: can't locate mount point for %s (%s)\n",
		      curdir, dirwithin);
		exit(1);
	}
	if (Debug) printf("wd host %s, fs %s, dir within %s\n",
		wdhost, fsname, dirwithin);

	Only2 = samefd(1,2);
	rst.rst_cmd = cmdp;
	rst.rst_env = environ;
	rst.rst_host = wdhost;
	rst.rst_fsname = fsname;
	rst.rst_dirwithin = dirwithin;
	rst.rst_port0 = makeport(&InOut);
	rst.rst_port1 =  rst.rst_port0;		/* same port for stdin */
	rst.rst_flags = 0;
	if (Debug)
		printf("environ is %s\n", *environ);
	if (Only2) {
		rst.rst_port2 = rst.rst_port1;
	} else {
		rst.rst_port2 = makeport(&Err);
	}
	if (Debug)
	  printf("rst.rst_port0 = %d; rst.rst_port1 = %d; rst.rst_port2 = %d\n",
        	  rst.rst_port0, rst.rst_port1, rst.rst_port2);
	if (clstat = clnt_call(Client, REXPROC_START,
	    xdr_rex_start, &rst, xdr_rex_result, &result, LongTimeout)) {
		fprintf(stderr, "remote %s: ", rhost);
		clnt_perrno(clstat);
		fprintf(stderr, "\n");
		exit(1);
	}
	if (result.rlt_stat != 0) {
		fprintf(stderr, "remote %s: %s\r",rhost,result.rlt_message);
		exit(1);
	}
	if (Debug) printf("Client call was made\r\n");
 	signal(SIGCONT, cont);
	signal(SIGURG, oob);
	doaccept(&InOut);
	(void) fcntl(InOut, F_SETOWN, getpid());
	remmask = (1 << InOut);
	if (Debug) printf("accept on stdout\r\n");
	if (!Only2) {
		doaccept(&Err);
		shutdown(Err, 1);
		if (Debug) printf("accept on stderr\r\n");
		remmask |= (1 << Err);
	}
	zmask = 1;
	if (Debug) printf("remmask = %d\n", remmask);
	while (remmask) {
		selmask = remmask | zmask;
		nfds = select(32, &selmask, 0, 0, 0);
		if (nfds <= 0) {
			if (errno == EINTR) continue;
			perror("remote: select");
			exit(1);
		}
		if (selmask & (1<<InOut)) {
			cc = read(InOut, buf, sizeof buf);
			if (cc > 0)
				write(1, buf, cc);
			else
				remmask &= ~(1<<InOut);
		}
		if (!Only2 && selmask & (1<<Err)) {
			cc = read(Err, buf, sizeof buf);
			if (cc > 0)
				write(2, buf, cc);
			else
				remmask &= ~(1<<Err);
		}
		if (selmask & (1<<0)) {
			cc = read(0, buf, sizeof buf);
			if (cc > 0)
				write(InOut, buf, cc);
			else {
				/*
				 * End of standard input - shutdown outgoing
				 * direction of the TCP connection.
				 */
				if (Debug)
			printf("Got EOF - shutting down connection\n");
				zmask = 0;
				shutdown(InOut, 1);
			}
		}
	}
	close(InOut);
	if (!Only2)
	    close(Err);
	if (Debug) printf("last clnt_call of main()\n");
	if (clstat = clnt_call(Client, REXPROC_WAIT,
	    xdr_void, 0, xdr_rex_result, &result, LongTimeout)) {
		fprintf(stderr, "remote: ");
		clnt_perrno(clstat);
		fprintf(stderr, "\r\n");
		exit(1);
	}
	exit(result.rlt_stat);
}


remstop()
{
	exit(23);
}

  /*
   * returns true if we can safely say that the two file descriptors
   * are the "same" (both are same file).
   */
samefd(a,b)
{
    struct stat astat, bstat;
    if (fstat(a,&astat) || fstat(b,&bstat)) return(0);
    if (astat.st_ino == 0 || bstat.st_ino == 0) return(0);
    return( !bcmp( &astat, &bstat, sizeof(astat)) );
}


/*
 * accept the incoming connection on the given
 * file descriptor, and return the new file descritpor
 */
doaccept(fdp)
	int *fdp;
{
	int fd;

	fd = accept(*fdp, 0, 0);
	if (fd < 0) {
		perror("accept");
		remstop();
		exit(1);
	}
	close(*fdp);
	*fdp = fd;
}

/*
 * create a socket, and return its the port number.
 */
makeport(fdp)
	int *fdp;
{
	struct sockaddr_in sin;
	int fd, len;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}
	bzero((char *)&sin, sizeof sin);
	sin.sin_family = AF_INET;
	bind(fd, &sin, sizeof sin);
	len = sizeof sin;
	getsockname(fd, &sin, &len);
	listen(fd, 1);
	*fdp = fd;
	return (htons(sin.sin_port));
}

usage()
{
    fprintf(stderr, "Usage: remote [-i|-n] [-d] machine cmd [args]...\n");
    exit(1);
}


/*
 * findmount(qualpn, host, fsname, within)
 *
 * Searches the mount table to find the appropriate file system
 * for a given absolute path name.
 * host gets the name of the host owning the file system,
 * fsname gets the file system name on the host,
 * within gets whatever is left from the pathname
 *
 * Returns: 0 on failure, 1 on success.
 */
findmount(qualpn, host, fsname, within)
	char *qualpn;
	char *host;
	char *fsname;
	char *within;
{
	FILE *mfp;
	char bestname[MAXPATHLEN];
	int bestlen = 0;
	struct mntent *mnt;
   	char *endhost;			/* points past the colon in name */
	extern char *index();
	int i, len;

	for (i = 0; i < 10; i++) {
		mfp = setmntent("/etc/mtab", "r");
		if (mfp != NULL)
			break;
		sleep(1);
	}
	if (mfp == NULL) {
		sprintf(within, "mount table problem");
		return (0);
	}
	while ((mnt = getmntent(mfp)) != NULL) {
		len = preflen(qualpn, mnt->mnt_dir);
		if (qualpn[len] != '/' && qualpn[len] != '\0' && len > 1)
		   /*
		    * If the last matching character is neither / nor 
		    * the end of the pathname, not a real match
		    * (except for matching root, len==1)
		    */
			continue;
		if (len > bestlen) {
			bestlen = len;
			strcpy(bestname, mnt->mnt_fsname);
		}
	}
	endmntent(mfp);
	endhost = index(bestname,':');
	  /*
	   * If the file system was of type NFS, then there should already
	   * be a host name, otherwise, use ours.
	   */
	if (endhost) {
		*endhost++ = 0;
		strcpy(host,bestname);
		strcpy(fsname,endhost);
		  /*
		   * special case to keep the "/" when we match root
		   */
		if (bestlen == 1)
			bestlen = 0;
	} else {
		gethostname(host, 255);
		strncpy(fsname,qualpn,bestlen);
		fsname[bestlen] = 0;
	}
	strcpy(within,qualpn+bestlen);
	return 1;
}

/*
 * Returns: length of second argument if it is a prefix of the
 * first argument, otherwise zero.
 */
preflen(str, pref)
	char *str, *pref;
{
	int len; 

	len = strlen(pref);
	if (strncmp(str, pref, len) == 0)
		return (len);
	return (0);
}
