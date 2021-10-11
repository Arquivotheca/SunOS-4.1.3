/*      @(#)nsquery.c 1.1 92/07/30 SMI      */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nsquery:nsquery.c	1.7.2.3"
#include  <sys/types.h>
#include  <rfs/sema.h>
#include  <rfs/rfsys.h>
#include  <rfs/rfs_misc.h>
#include  <rfs/comm.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <rfs/nserve.h>
#include  <errno.h>

void	exit();
static	char	*cmd;
extern	int	errno;
extern	int	ns_errno;
extern	int	optind,	opterr;
extern	char	*optarg;
extern	char	*dompart();
extern	char	*namepart();

main(argc,argv)
int	argc;
char	*argv[];
{

	int	chr;
	int	errflag = 0, hflag = 0;
	int	rtn;
	char	*name = "*";
	char 	*usage = "nsquery [-h] [<name>]";


	cmd = argv[0];

	if ((rtn = rfsys(RF_RUNSTATE)) < 0) {
		perror(cmd);
		exit(1);
	}

	if (rtn != DU_UP) {
		fprintf(stderr, "%s: RFS is not running\n", cmd);
		exit(1);
	}

	while ((chr = getopt(argc,argv,"h")) != EOF)
		switch(chr) {
	 	case 'h':
			if (hflag)
				errflag = 1;
			else
				hflag = 1;	
			break;
		case '?':
			errflag = 1;
			break;
		}

	if (errflag || argc > optind + 1) {
		fprintf(stderr,"Usage: %s\n",usage);
		exit(1);
	}

	if (argv[optind] != NULL) {
		name = argv[optind];
		verify_name(name);
	}

	if (!hflag)
		fprintf(stdout,"RESOURCE        ACCESS      SERVER                    DESCRIPTION\n\n");

	if (ns_info(name) == RFS_FAILURE) {
		if (ns_errno == R_SETUP) {
			fprintf(stderr, "%s: cannot set up communication with the name server\n", cmd);
			fprintf(stderr, "%s: possible cause: heavily loaded RFS activity\n", cmd);
		} else if (ns_errno == R_RCV) {
			fprintf(stderr, "%s: no information received from name server\n", cmd);
			fprintf(stderr, "%s: possible cause: unknown domain name specified or a connection\n", cmd);
			fprintf(stderr, "         to the name server could not be established\n");
		} else
			nserror(cmd);
		exit(1);
	}
	exit(0);
	/* NOTREACHED */
}

static
verify_name(name)
char	*name;
{

	char	*mach;
	char	*domain;
	int	qname = 0, dname = 0;

	if (name[strlen(name)-1] == SEPARATOR)
		dname = 1;

	if (*(domain = dompart(name)) != '\0') {
		qname = 1;
		if (strlen(domain) > SZ_DELEMENT) {
			fprintf(stderr,"%s: domain name %s<%s> exceeds <%d> characters\n",cmd,dname ? "":"in ",name,SZ_DELEMENT);
			exit(1);
		}

		if (v_dname(domain) != 0) {
			fprintf(stderr,"%s: domain name %s<%s> contains invalid characters\n",cmd,dname ? "":"in ",name);
			exit(1);
		}
	}

	if (*(mach = namepart(name)) != '\0') {
		if (strlen(mach) > SZ_MACH) {
			fprintf(stderr,"%s: nodename %s<%s> exceeds <%d> characters\n",cmd,qname ? "in ":"",name,SZ_MACH);
			exit(1);
		}

		if (v_uname(mach) != 0) {
			fprintf(stderr,"%s: nodename %s<%s> contains invalid characters\n",cmd,qname ? "in ":"",name);
			exit(1);
		}
	}
}
