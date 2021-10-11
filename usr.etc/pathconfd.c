#ifndef lint
static  char sccsid[] = "@(#)pathconfd.c 1.1 92/07/30 SMI"; /* from UCB 5.6 5/26/86 */
#endif not lint

#include <stdio.h>
#include <sys/limits.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <rpc/rpc.h>

/* pathconfd.h */
struct pathcnf {
	int pc_dir_entries;
	int pc_link_max;
	short pc_max_canon;
	short pc_max_input;
	short pc_name_max;
	short pc_path_max;
	short pc_pipe_buf;
	u_char pc_vdisable;
	char pc_xxx;
	short pc_mask;
	short pc_refcnt;
	struct pathcnf *pc_next;
};
typedef struct pathcnf pathcnf;
bool_t xdr_pathcnf();


#define PATHCONFD ((u_long)30962)
#define PATHCONFDVERS ((u_long)1)
#define PATHCONF ((u_long)1)
#define NORMAL_PATHCONF
extern pathcnf *pathconf_1();

/* server.c */

/* 
 * Pathconf daemon remote procedure
 *
 * This is a kludge to make pathconf work with old (NFS2) servers.
 * These servers must have this pathconf daemon running in order to
 * provide an approximation of the pathconf() information.
 *
 * The path passed in is the mount point so you may assume that it is 
 * directory.  This allows me to find out about directory entry counts.
 */
pathcnf        *
pathconf_1(pathp)
	char          **pathp;
{
	static pathcnf  r;
	extern          errno;
	register char  *p = *pathp;
	char            s[MAXPATHLEN+1];
	char	       *strcpy(), *strcat();

	errno = 0;
	bzero((caddr_t)&r, sizeof(r));

#if defined(NORMAL_PATHCONF)
	/*
	 * Use this if you have an existing pathconf that returns
	 * one value per call
	 *
	 * The daemon will create (briefly) an empty file to get 
	 * file link counts.
	 */
	r.pc_dir_entries = pathconf(p, _PC_LINK_MAX);
	if (errno)
	    r.pc_mask |= _PC_DIR_ENTRIES;
	(void)strcpy(s, p);
	(void)strcat(s, "/._-_pathconf");	/* unlikely file name */
	(void)close(creat(s, 0666));
	r.pc_link_max = pathconf(s, _PC_LINK_MAX);
	(void)unlink(s);
	if (errno)
	    r.pc_mask |= _PC_LINK_MAX;
	r.pc_max_canon = pathconf(p, _PC_MAX_CANON);
	if (errno)
	    r.pc_mask |= _PC_MAX_CANON;
	r.pc_max_input = pathconf(p, _PC_MAX_INPUT);
	if (errno)
	    r.pc_mask |= _PC_MAX_INPUT;
	r.pc_name_max = pathconf(p, _PC_NAME_MAX);
	if (errno)
	    r.pc_mask |= _PC_NAME_MAX;
	r.pc_path_max = pathconf(p, _PC_PATH_MAX);
	if (errno)
	    r.pc_mask |= _PC_PATH_MAX;
	r.pc_pipe_buf = pathconf(p, _PC_PIPE_BUF);
	if (errno)
	    r.pc_mask |= _PC_PIPE_BUF;
	if (pathconf(p, _PC_CHOWN_RESTRICTED) == 1)
	    r.pc_mask |= _PC_CHOWN_RESTRICTED;
	if (pathconf(p, _PC_NO_TRUNC) == 1)
	    r.pc_mask |= _PC_NO_TRUNC;
	if (pathconf(p, _PC_VDISABLE) == 1)
	    r.pc_mask |= _PC_VDISABLE;

#elif defined(TEST)
	/*
	 * This one is just for testing
	 */
	r.pc_dir_entries = 1;
	r.pc_link_max = -2;
	r.pc_max_canon = -3;
	r.pc_max_input = 4;
	r.pc_name_max = 5;
	r.pc_path_max = -6;
	r.pc_pipe_buf = -7;
	r.pc_vdisable = 8;
	r.pc_mask = 0;

#else
	/*
	 * roll another one... just like the other one...
	 */
	r.pc_dir_entries = -1;
	r.pc_link_max = 32767;
	r.pc_max_canon = 256;
	r.pc_max_input = -1;
	r.pc_name_max = 255;
	r.pc_path_max = 1024;
	r.pc_pipe_buf = 4096;
	r.pc_vdisable = 0;
	r.pc_mask = _PC_CHOWN_RESTRICTED | _PC_NO_TRUNC;
#endif
	return &r;
}

/* pathconfd_svc.c */
static void pathconfd_1();

main()
{
	SVCXPRT *transp;

	(void)pmap_unset(PATHCONFD, PATHCONFDVERS);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		(void)fprintf(stderr, "cannot create udp service.\n");
		exit(1);
	}
	if (!svc_register(transp, PATHCONFD, PATHCONFDVERS, pathconfd_1, IPPROTO_UDP)) {
		(void)fprintf(stderr, "unable to register (PATHCONFD, PATHCONFDVERS, udp).\n");
		exit(1);
	}

	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL) {
		(void)fprintf(stderr, "cannot create tcp service.\n");
		exit(1);
	}
	if (!svc_register(transp, PATHCONFD, PATHCONFDVERS, pathconfd_1, IPPROTO_TCP)) {
		(void)fprintf(stderr, "unable to register (PATHCONFD, PATHCONFDVERS, tcp).\n");
		exit(1);
	}
	svc_run();
	(void)fprintf(stderr, "svc_run returned\n");
	exit(1);
	/*NOTREACHED*/
}

static void
pathconfd_1(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	union {
		char *pathconf_1_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void)svc_sendreply(transp, xdr_void, (char *)NULL);
		return;

	case PATHCONF:
		xdr_argument = xdr_wrapstring;
		xdr_result = xdr_pathcnf;
		local = (char *(*)()) pathconf_1;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	bzero((char *)&argument, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, &argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, &argument)) {
		(void)fprintf(stderr, "unable to free arguments\n");
		exit(1);
	}
}

/* pathconfd_xdr.c */

bool_t
xdr_pathcnf(xdrs, objp)
	XDR *xdrs;
	pathcnf *objp;
{
	if (!xdr_int(xdrs, &objp->pc_dir_entries)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->pc_link_max)) {
		return (FALSE);
	}
	if (!xdr_short(xdrs, &objp->pc_max_canon)) {
		return (FALSE);
	}
	if (!xdr_short(xdrs, &objp->pc_max_input)) {
		return (FALSE);
	}
	if (!xdr_short(xdrs, &objp->pc_name_max)) {
		return (FALSE);
	}
	if (!xdr_short(xdrs, &objp->pc_path_max)) {
		return (FALSE);
	}
	if (!xdr_short(xdrs, &objp->pc_pipe_buf)) {
		return (FALSE);
	}
	if (!xdr_u_char(xdrs, &objp->pc_vdisable)) {
		return (FALSE);
	}
	if (!xdr_char(xdrs, &objp->pc_xxx)) {
		return (FALSE);
	}
	if (!xdr_short(xdrs, &objp->pc_mask)) {
		return (FALSE);
	}
	if (!xdr_short(xdrs, &objp->pc_refcnt)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->pc_next, sizeof(pathcnf), xdr_pathcnf)) {
		return (FALSE);
	}
	return (TRUE);
}


