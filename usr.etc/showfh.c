#ifndef lint
static	char sccsid[] = "@(#)showfh.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * showfh.c
 * Client side : Prints the complete path name for the given file handle
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/showfh.h>

#define MAXLINELEN 256
/*
 * The time in seconds within which the request should be handled.
 */
#define TOTTIME	3600

long getnum();
long strtol();
char *malloc();

main(argc, argv)
	int argc;
	char **argv;
{
	CLIENT *cl;		/* Client for the rpc calls */
	nfs_handle rhan;	/* Parameters for the rpc call */
	res_handle *rval;	/* Result returned from the rpc call */
	char *hostp;		/* Remote host */
	int i;
	int arglen;

	if (argc <= 2)
		usage();
	
	hostp = argv[1];
	argv += 2;	/* Skip cmd name and host name */

	/* Now get the file handle parameters. */
	arglen = argc - 2;
	rhan.cookie.cookie_len = arglen;
	rhan.cookie.cookie_val = (u_int *)malloc(arglen * sizeof(int));
	for (i = 0 ; i < arglen; i++)
		rhan.cookie.cookie_val[i] = (u_int) getnum(argv[i]);

	if ((cl = clnt_create(hostp, FILEHANDLE_PROG,
				FILEHANDLE_VERS, "tcp")) == NULL) {
		fprintf(stderr, "showfh: ");
		clnt_pcreateerror(hostp);
		exit (1);
	}

	if ((rval = getrhandle_1(&rhan, cl)) == NULL) {
		fprintf(stderr, "showfh: ");
		clnt_perror(cl, hostp);
		exit (1);
	}
	if (rval->result == SHOWFH_FAILED) {
		fprintf(stderr, "showfh: %s", rval->file_there);
		exit (1);
	}
	printf("%s", rval->file_there);
	exit (0);
	/* NOTREACHED */
}

static
usage()
{
	fprintf(stderr, "usage: showfh server_name num1 num2 ..\n");
	exit (1);
}

static long
getnum(arg)
	char *arg;
{
	char *strptr;
	register long num;

	num = strtol(arg, &strptr, 16);
	if (strptr == arg || *strptr) {
		fprintf(stderr, "showfh: %s: illegal file handle number\n", arg);
		exit (1);
	}
	return (num);
}

/*
 * Output of rpcgen.
 */

static struct timeval TIMEOUT = {TOTTIME, 0};

static res_handle *
getrhandle_1(argp, clnt)
	nfs_handle *argp;
	CLIENT *clnt;
{
	static res_handle res;

	bzero((char *) &res, sizeof(res));
	(void) clnt_control(clnt,CLSET_TIMEOUT,&TIMEOUT);
	if (clnt_call(clnt, GETRHANDLE, xdr_nfs_handle, argp, xdr_res_handle, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}
