#ifndef lint
static char sccsid[] = "@(#)rpc.ypupdated.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif
/*
 * Copyright (C) 1990, Sun Microsystems, Inc.
 */

/*
 * NIS update service
 */
#include <stdio.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/file.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <rpc/auth_des.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/termio.h>
#include <strings.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/ypupdate_prot.h>
#include <netdb.h>


#define RPC_INETDSOCK	0	/* socket descriptor if using inetd */
#define debug(msg)	/* turn off debugging */

char YPDIR[] = "/var/yp";
char UPDATEFILE[] = "updaters";

void ypupdate_prog();
int insecure;

main(argc, argv)
	int argc;
	char *argv[];
{
	SVCXPRT *transp;
	int proto;
	int sock;
	char *cmd;

	cmd = argv[0];
	switch (argc) {
	case 0:
		cmd = "ypupdated";
		break;
	case 1:
		break;
	case 2:
		if (strcmp(argv[1], "-i") == 0) {
			insecure++;
			break;
		}
		else if (strcmp(argv[1], "-s") == 0) {
			insecure = 0;
			break;
		}
	default:
		fprintf(stderr, "%s: warning -- options ignored\n", cmd);
		break;
	}
	if (issock(0)) {
		sock = RPC_INETDSOCK;
		proto = 0;
	} else {
		detachfromtty();
		sock = RPC_ANYSOCK;
		pmap_unset(YPU_PROG, YPU_VERS);
		proto = IPPROTO_TCP;
	}
	if (chdir(YPDIR) < 0) {
		fprintf(stderr,"%s: can't chdir to ", cmd);
		perror(YPDIR);
		exit(1);	
	}
	transp = svctcp_create(sock, 0, 0);
	if (transp == NULL) {
		fprintf(stderr, "%s: cannot create tcp service\n", cmd);
		exit(1);	
	}
	if (!svc_register(transp, YPU_PROG, YPU_VERS, ypupdate_prog, proto)) {
		fprintf(stderr, "%s: couldn't register service\n", cmd);
		exit(1);	
	}
	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		fprintf(stderr, "%s: cannot create udp service\n", cmd);
		exit(1);	
	}
	proto = IPPROTO_UDP;
	if (!svc_register(transp, YPU_PROG, YPU_VERS, ypupdate_prog, proto)) {
		fprintf(stderr, "%s: couldn't register service\n", cmd);
		exit(1);	
	}
	svc_run();
	abort();
	/* NOTREACHED */
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
	 * SunOS returns S_IFIFO for sockets, while 4.3 returns 0 and
	 * does not even have an S_IFIFO mode.  Since there is confusion 
	 * about what the mode is, we check for what it is not instead of 
	 * what it is.
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

detachfromtty()
{
	int tt;

	close(0);
	close(1);
	close(2);
	switch (fork()) {
	case -1:
		perror("fork");
		break;
	case 0:
		break;
	default:
		exit(0);
	}
	tt = open("/dev/tty", O_RDWR, 0);
	if (tt >= 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	}
	open("/dev/null", O_RDWR, 0);
	dup(0);
	dup(0);
}
 
void
ypupdate_prog(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypupdate_args args;
	u_int rslt;	
	u_int op;
	char *netname;
	char namebuf[MAXNETNAMELEN+1];
	struct authunix_parms *aup;

	switch (rqstp->rq_proc) {
	case NULLPROC:
		svc_sendreply(transp, xdr_void, NULL);
		return;
	case YPU_CHANGE:
		op = YPOP_CHANGE;
		break;
	case YPU_DELETE:
		op = YPOP_DELETE;
		break;
	case YPU_INSERT:
		op = YPOP_INSERT;
		break;
	case YPU_STORE:
		op = YPOP_STORE;
		break;
	default:
		svcerr_noproc(transp);
		return;
	}	
	switch (rqstp->rq_cred.oa_flavor) {
	case AUTH_DES:
		netname = ((struct authdes_cred *)
			rqstp->rq_clntcred)->adc_fullname.name;
		break;
	case AUTH_UNIX:
		if (insecure) {
			aup = (struct authunix_parms *)rqstp->rq_clntcred;
			if (aup->aup_uid == 0) {
				addr2netname(namebuf, svc_getcaller(transp));
			} else {
				user2netname(namebuf, aup->aup_uid, NULL);
			}
			netname = namebuf;
			break;
		}
	default:
		svcerr_weakauth(transp);
		return;
	}
	bzero(&args, sizeof(args));
	if (!svc_getargs(transp, xdr_ypupdate_args, &args)) {
		svcerr_decode(transp);
		return;
	}
	rslt = update(netname,
		args.mapname, op, args.key.yp_buf_len, args.key.yp_buf_val,
		args.datum.yp_buf_len, args.datum.yp_buf_val);
	if (!svc_sendreply(transp, xdr_u_int, &rslt)) {
		debug("svc_sendreply failed");
	}
	if (!svc_freeargs(transp, xdr_ypupdate_args, &args)) {
		debug("svc_freeargs failed");
	}
}

/*
 * Determine if requester is allowed to update the given map,
 * and update it if so. Returns the NIS status, which is zero
 * if there is no access violation.
 */
update(requester, mapname, op, keylen, key, datalen, data)
	char *requester;
	char *mapname;
	u_int op;
	u_int keylen;	
	char *key;
	u_int datalen;
	char *data;
{
	char updater[MAXMAPNAMELEN + 40]; 	
	FILE *childargs;
	FILE *childrslt;
	union wait status;
	int yperrno;
	int pid;

	sprintf(updater, "make -s -f %s %s", UPDATEFILE, mapname);
	pid = _openchild(updater, &childargs, &childrslt);
	if (pid < 0) {
		debug("openpipes failed");
		return (YPERR_YPERR);
	}

	/*
	 * Write to child
	 */
	fprintf(childargs, "%s\n", requester);
	fprintf(childargs, "%u\n", op);
	fprintf(childargs, "%u\n", keylen);
	fwrite(key, keylen, 1, childargs);
	fprintf(childargs,"\n");
	fprintf(childargs, "%u\n", datalen);
	fwrite(data, datalen, 1, childargs);
	fprintf(childargs,"\n");
	fclose(childargs);

	/*
	 * Read from child
	 */
	fscanf(childrslt, "%d", &yperrno);
	fclose(childrslt);

	wait(&status);
	if (status.w_retcode != 0) {
		return (YPERR_YPERR);
	}
	return (yperrno);
}

addr2netname(namebuf, addr)
	char *namebuf;
	struct sockaddr_in *addr;
{
	struct hostent *h;

	h = gethostbyaddr(&addr->sin_addr, sizeof(addr->sin_addr), AF_INET);
	if (h == NULL) {
		host2netname(namebuf, inet_ntoa(addr->sin_addr), NULL);
	} else {
		host2netname(namebuf, h->h_name, NULL);
	}
}
