#ifndef lint
static	char sccsid[] = "@(#)rpc.showfhd.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * The server side of the showfh which uses "find"
 * to find the name of the file. This scheme will fail
 * on non-unix machines which do not support "find".
 */

#include <stdio.h>
#include <signal.h>
#include <rpc/rpc.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <mntent.h>
#include <syslog.h>
#include <rpcsvc/showfh.h>

/*
 * Output of rpcgen -I -s tcp showfh.x
 */
#ifdef DEBUG
#define RPC_SVC_FG
#endif
#define _RPCSVC_CLOSEDOWN 120

static void filehandle_prog_1();
static void closedown();

static int _rpcpmstart;		/* Started by a port monitor ? */
static int _rpcsvcdirty;	/* Still serving ? */

main()
{
	register SVCXPRT *transp;
	int sock;
	int proto;
	struct sockaddr_in saddr;
	int asize = sizeof(saddr);
	static int _rpcfdtype;		/* Whether Stream or Datagram ? */

	if (getsockname(0, (struct sockaddr *)&saddr, &asize) == 0) {
		int ssize = sizeof(int);

		if (saddr.sin_family != AF_INET)
			exit(1);
		if (getsockopt(0, SOL_SOCKET, SO_TYPE,
				(char *)&_rpcfdtype, &ssize) == -1)
			exit(1);
		sock = 0;
		_rpcpmstart = 1;
		proto = 0;
		openlog("rpc.showfhd", LOG_PID, LOG_DAEMON);
	} else {
#ifndef RPC_SVC_FG
		int i, pid;

		pid = fork();
		if (pid < 0) {
			perror("cannot fork");
			exit(1);
		}
		if (pid)
			exit(0);
		for (i = 0 ; i < 20; i++)
			(void) close(i);
		i = open("/dev/console", 2);
		(void) dup2(i, 1);
		(void) dup2(i, 2);
		i = open("/dev/tty", 2);
		if (i >= 0) {
			(void) ioctl(i, TIOCNOTTY, (char *)NULL);
			(void) close(i);
		}
		openlog("rpc.showfhd", LOG_PID, LOG_DAEMON);
#endif
		sock = RPC_ANYSOCK;
		(void) pmap_unset(FILEHANDLE_PROG, FILEHANDLE_VERS);
	}

	if ((_rpcfdtype == 0) || (_rpcfdtype == SOCK_STREAM)) {
		transp = svctcp_create(sock, 0, 0);
		if (transp == NULL) {
			_msgout("cannot create tcp service.");
			exit(1);
		}
		if (!_rpcpmstart)
			proto = IPPROTO_TCP;
		if (!svc_register(transp, FILEHANDLE_PROG, FILEHANDLE_VERS,
				filehandle_prog_1, proto)) {
			_msgout("unable to register (FILEHANDLE_PROG, FILEHANDLE_VERS, tcp).");
			exit(1);
		}
	}

	if (transp == (SVCXPRT *)NULL) {
		_msgout("could not create a handle");
		exit(1);
	}
	if (_rpcpmstart) {
		(void) signal(SIGALRM, closedown);
		(void) alarm(_RPCSVC_CLOSEDOWN);
	}
	svc_run();
	_msgout("svc_run returned");
	exit(1);
	/* NOTREACHED */
}

static void
filehandle_prog_1(rqstp, transp)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	union {
		nfs_handle getrhandle_1_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();

	_rpcsvcdirty = 1;
	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply(transp, xdr_void, (char *)NULL);
		_rpcsvcdirty = 0;
		return;

	case GETRHANDLE:
		xdr_argument = xdr_nfs_handle;
		xdr_result = xdr_res_handle;
		local = (char *(*)()) getrhandle_1;
		break;

	default:
		svcerr_noproc(transp);
		_rpcsvcdirty = 0;
		return;
	}
	bzero((char *)&argument, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, &argument)) {
		svcerr_decode(transp);
		_rpcsvcdirty = 0;
		return;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, &argument)) {
		_msgout("unable to free arguments");
		exit(1);
	}
	_rpcsvcdirty = 0;
	return;
}

static
_msgout(msg)
	char *msg;
{
#ifdef RPC_SVC_FG
	if (_rpcpmstart)
		syslog(LOG_ERR, msg);
	else
		(void) fprintf(stderr, "%s\n", msg);
#else
	syslog(LOG_ERR, msg);
#endif
}

static void
closedown()
{
	if (_rpcsvcdirty == 0) {
		extern fd_set svc_fdset;
		static int size;
		int i, openfd;

		if (size == 0)
			size = getdtablesize();
		for (i = 0, openfd = 0; i < size && openfd < 2; i++)
			if (FD_ISSET(i, &svc_fdset))
				openfd++;
		if (openfd <= 1)
			exit(0);
	}
	(void) alarm(_RPCSVC_CLOSEDOWN);
}

/*
 * The function call implementation
 */
extern char *strdup();
extern char *sprintf();

/* VARARGS USED */
static res_handle *
getrhandle_1(handle)
	nfs_handle *handle;
{
	struct fhandle {	/* file handle */
		int fsidval[2]; 	/* file system id's */
		short rec_length;	/* record length */
		short filler1;
		int file_inode;		/* inode number */
		int file_gen;		/* generation count */
		short mount_length;
		short filler2;
		int mount_inode;	/* mount point inode number */
		int mount_gen;		/* generation count */
	} *fh = (struct fhandle *)handle->cookie.cookie_val;
	static res_handle res;		/* result */
	FILE *fin;			/* input file desc */
	FILE *mtabf;			/* mtab file desc */
	int tempfd;			/* temporary file desc */
	char cmd[MAXNAMELEN + 256]; 	/* for the find command */
	static char buf[MAXNAMELEN];	/* for sending the result */
	struct mntent *mnt;
	struct statfs statbuf;

	res.result = SHOWFH_FAILED;
	res.file_there = buf;
	if (handle->cookie.cookie_len != FHSIZE) {
		(void) sprintf(buf, "showfhd: illegal number of parameters\n");
		return (&res);
	}

	/* 
	 * Find out from the file /etc/mtab, the mount point for
	 * this stale handle.
	 */
	if ((mtabf = setmntent(MOUNTED, "r")) == NULL) {
		(void) sprintf(buf, "showfhd: could not read /etc/mtab\n");
		return (&res);
	}
	while (mnt = getmntent(mtabf)) {
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0)
			continue;
		if (statfs(mnt->mnt_dir, &statbuf))
			continue;
		if ((statbuf.f_fsid.val[0] == fh->fsidval[0]) &&
			(statbuf.f_fsid.val[1] == fh->fsidval[1])) {
#ifdef DEBUG
			printf("Mount point-> %s\n", mnt->mnt_fsname);
#endif
			break;
		}
	}
	(void) endmntent(mtabf);

	if (mnt == NULL) {
		sprintf(buf, "showfhd: could not get mount point\n");
		return (&res);
	}

	(void) sprintf(cmd, "find %s -inum %d -xdev -print",
				mnt->mnt_dir, fh->file_inode);

	/*
	 * To ignore the junk created by find call due to permissions
	 * being denied for searching the tree, we have to close stderr.
	 */
	tempfd = dup(2);
	(void) close(2);
	(void) fopen("/dev/null", "w"); /* This gets fid 2 */

	if ((fin = popen(cmd, "r")) == NULL) {
		(void) sprintf(buf, "showfhd: could not open pipe.\n");
	} else if (read(fileno(fin), buf, MAXNAMELEN) == 0) {
		/* The pipe was empty, hence no output from "find" */
		(void) sprintf(buf, "showfhd: no file with this file handle.\n");
	} else {
		/* buf has the file name here */
		res.result = SHOWFH_SUCCESS;
	}

	(void) pclose(fin);
	(void) dup2(tempfd, 2);	/* Reallocate 2 to stderr */
#ifdef DEBUG
	fprintf(stderr, "%s", buf);
#endif
	return (&res);
}
