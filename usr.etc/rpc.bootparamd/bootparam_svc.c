#ifndef lint
static  char sccsid[] = "@(#)bootparam_svc.c 1.1 92/07/30 SMI";
#endif

/*
 * Main program of the bootparam server.
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <rpc/rpc.h>
#include <rpcsvc/bootparam.h>

extern int debug;

static void background();
static void bootparamprog_1();

main(argc, argv)
	int argc;
	char **argv;
{
	SVCXPRT *transp;

	if (argc > 1)  {
		if (strncmp(argv[1], "-d", 2) == 0) {
			debug++;
			fprintf(stderr, "In debug mode\n");
		} else {
			fprintf(stderr, "usage: %s [-d]\n", argv[0]);
			exit(1);
		}
	}

	if (! issock(0)) {
		/*
		 * Started by user.
		 */
		if (!debug)
			background();

		pmap_unset(BOOTPARAMPROG, BOOTPARAMVERS);
		if ((transp = svcudp_create(RPC_ANYSOCK)) == NULL) {
			fprintf(stderr, "cannot create udp service.\n");
			exit(1);
		}
		if (! svc_register(transp, BOOTPARAMPROG, BOOTPARAMVERS,
		    bootparamprog_1, IPPROTO_UDP)) {
			fprintf(stderr, "unable to register (BOOTPARAMPROG, BOOTPARAMVERS, udp).\n");
			exit(1);
		}
	} else {
		/*
		 * Started by inetd.
		 */
		if ((transp = svcudp_create(0)) == NULL) {
			fprintf(stderr, "cannot create udp service.\n");
			exit(1);
		}
		if (!svc_register(transp, BOOTPARAMPROG, BOOTPARAMVERS,
		    bootparamprog_1, 0)) {
			fprintf(stderr, "unable to register (BOOTPARAMPROG, BOOTPARAMVERS, udp).\n");
			exit(1);
		}
	}

	/*
	 * Start serving
	 */
	init_nlist();		/* nlist the kernel */
	svc_run();
	fprintf(stderr, "svc_run returned\n");
	exit(1);

	/* NOTREACHED */
}

static void
background()
{
#ifndef DEBUG
	if (fork())
		exit(0);
	{ int s;
	for (s = 0; s < 10; s++)
		(void) close(s);
	}
	(void) open("/", O_RDONLY);
	(void) dup2(0, 1);
	(void) dup2(0, 2);
#endif
	{ int tt = open("/dev/tty", O_RDWR);
	  if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	  }
	}
}

static void
bootparamprog_1(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	union {
		bp_whoami_arg bootparamproc_whoami_1_arg;
		bp_getfile_arg bootparamproc_getfile_1_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();
	extern bp_whoami_res *bootparamproc_whoami_1();
	extern bp_getfile_res *bootparamproc_getfile_1();

	switch (rqstp->rq_proc) {
	case NULLPROC:
		svc_sendreply(transp, xdr_void, NULL);
		return;

	case BOOTPARAMPROC_WHOAMI:
		xdr_argument = xdr_bp_whoami_arg;
		xdr_result = xdr_bp_whoami_res;
		local = (char *(*)()) bootparamproc_whoami_1;
		break;

	case BOOTPARAMPROC_GETFILE:
		xdr_argument = xdr_bp_getfile_arg;
		xdr_result = xdr_bp_getfile_res;
		local = (char *(*)()) bootparamproc_getfile_1;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	bzero(&argument, sizeof(argument));
	if (! svc_getargs(transp, xdr_argument, &argument)) {
		svcerr_decode(transp);
		return;
	}
	if ((result = (*local)(&argument)) != NULL) {
		if (! svc_sendreply(transp, xdr_result, result)) {
			svcerr_systemerr(transp);
		}
	}
	if (! svc_freeargs(transp, xdr_argument, &argument)) {
		fprintf(stderr,"unable to free arguments\n");
		exit(1);
	}
}
