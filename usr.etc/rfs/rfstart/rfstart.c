/*	@(#)rfstart.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)dustart:dustart.c	1.21" */
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <sys/signal.h>
#include <rfs/nserve.h>
#include <rfs/rfsys.h>

#define NSERVE  	NSDIRQ/nserve"
#define NETSPEC  	NSDIRQ/netspec"

#define WAIT_TIME	120
#define MAX_ARGS	100
#define ERROR(str)	fprintf(stderr,"%s: %s\n", cmd_name, str)
#define NODENAMESIZE	100

static	char   *cmd_name;
static	int	ok_flag = 0;
static	int	ns_process;

extern	int	errno;

main( argc, argv )
int   argc;
char *argv[];
{
	char   *n_argv[MAX_ARGS];

	int	error = 0, pflag = 0, vflag = 0, cflag = 0;
	int	parent;
	int	i, c, rec;
	int	indx;
	int	cr_pass = 0;
	int	ok_rtn(), error_rtn(), intrp_rtn();
	char    newpass[100];
	char   *rtn;
	char   mach_name[NODENAMESIZE];
	char   *netspec;
	char   *dname;
	char   *getdname();
	char   *getnetspec();
	char   *getpass();
	char   *ns_getpass();
	char   *ns_verify();

	extern int optind;

	cmd_name = argv[0];

	/*
	 *	Process arguments.
	 *	Dustart will exit with one of three exit codes:
	 *		0 - success
	 *		1 - error because name server failed - try
	 *			again later.
	 *		2 - error because something else failed.
	 */

	while ((c = getopt(argc, argv, "vp:l:f:c")) != EOF) {
		switch (c) {
			case 'v':
				if (vflag)
					error = 1;
				else
					vflag = 1;
				break;
			case 'p':
				if (pflag)
					error = 1;
				else
					pflag = 1;
				break;
			case 'c':
				if (cflag)
					error = 1;
				else
					cflag = 1;
				break;
			case '?':
				error = 1;
		}
	}

	if (optind < argc) {
		ERROR("extra arguments given");
		error = 1;
	}

	if (error) {
		ERROR("usage: rfstart [-v] [-p primary_name_server_address]");
		exit(2);
	}

	if (getuid() != 0) {
		ERROR("must be super-user");
		exit(2);
	}

	chdir("/");

	/*
	 *	Get the network specification to send to the name server.
	 */

	if ((netspec = getnetspec()) == NULL) {
		ERROR("network specification not set");
		exit(2);
	}

	/*
	 *	Set up the argument list of the name server.  The
	 *	"-k" option is always given to the name server (the
	 *	option tells the name server to signal the parent
	 *	when it is set up correctly).
	 *	The "-c" option is not recognized by the name server.
	 */

	n_argv[0] = NSERVE;
	n_argv[1] = "-k";
	indx = 2;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-c") == 0)
			continue;
		n_argv[indx++] = argv[i];
	}
	n_argv[indx++] = netspec;
	n_argv[indx++] = NULL;;

	/*
	 *	Get the machine name of the host.
	 */

	if (gethostname(mach_name, NODENAMESIZE - 1) == -1) {
		ERROR("cannot get machine name");
		exit(2);
	}

	/*
	 *	Check to make sure the node name is valid.
	 */

	if (pv_uname(cmd_name, mach_name, 0, "node") != 0) {
		exit(1);
	}

	/*
	 *	Get the domain name and pass it into the kernel.
	 */

	if ((dname = getdname()) == NULL) {
		ERROR("domain name information not set");
		exit(2);
	}

	if (rfsys(RF_SETDNAME, dname, strlen(dname)+1) < 0) {
		if (errno == EEXIST)
			ERROR("RFS is already running");
		else
			perror(cmd_name);
		exit(2);
	}

	/*
	 *	Issue the rfstart(2) system call to initiate the
	 *	kernel functions of Distributed UNIX.
	 */

	if (rfstart() < 0) {
		if (errno == EAGAIN) {
			ERROR("Tunable parameter MAXGDP is not set to a large enough value");
			ERROR("Consult Administrator's Guide");
		} else {
			perror(cmd_name);
		}
		exit(2);
	}

	/*
	 *	Catch the SIGUSR1 and SIGUSR2 signals from the
	 *	child processes which will be spawned.
	 *	SIGUSR1 will signify everything within the
	 *	child is OK and SIGUSR2 will signify that something
	 *	went wrong.
	 *	If any other interrupt, stop the name server and exit.
	 */

	signal(SIGUSR1, ok_rtn);
	signal(SIGUSR2, error_rtn);
	signal(SIGHUP,  intrp_rtn);
	signal(SIGINT,  intrp_rtn);
	signal(SIGQUIT, intrp_rtn);

#ifdef NAMESERVER
	/*
	 *	Fork off a child which will eventually start off
	 *	the name server process.
	 */

	if ((parent = ns_process = fork()) == -1) {
		ERROR("cannot fork to start name server");
		perror(cmd_name);
		rfstop();
		exit(2);
	}

	if (parent) {
		/*
		 *	The parent expects the name server to send
		 *	a SIGUSR1 signal or a SIGUSR2 signal.
		 *	If the parent sleeps for WAIT_TIME seconds and
		 *	does not get a signal (i.e., ok_flag is still
	 	 *	not set), it assumes something
		 *	went wrong and exits.
		 */

		if (!ok_flag) {
			sleep(WAIT_TIME);
			if (!ok_flag) {
				ERROR("timed out waiting for name server signal");
				rfstop();
				killns();
				exit(2);
			}
		}
	} else {
		setpgrp();
		if (execv(NSERVE, n_argv) == -1) {
			ERROR("cannot exec name server");
			perror(cmd_name);
			kill(getppid(), SIGUSR2);
			exit(2);
		}
	}

	/*
	 *	Call ns_initaddr(), which sends the address of
	 *	this machine to the primary name server and tells the
	 *	primary name server to unadvertise any resources
	 *	currently advertised by this machine.
	 *	If the "-c" flag is specified, don't kill the name
	 *	server on failure, simply exit.
	 */

	if (ns_initaddr(mach_name) == RFS_FAILURE) {
		if (cflag) {
			ERROR("warning: could not send address to primary name server");
			system("/usr/bin/setpgrp /usr/bin/rfudaemon &");
			exit(2);
		} else {
			ERROR("could not send address to primary name server");
			nserror(cmd_name);
			rfstop();
			killns();
			exit(1);
		}
	}

	/*
	 *	Get the password of this machine.
	 *	If no password exists for this machine, prompt for
	 *	password.  The password is verified by the primary
	 *	name server.
	 */

	if ((rec = open(PASSFILE, O_RDONLY)) == -1) {
		strncpy(newpass, getpass("rfstart: Please enter machine password:"), sizeof(newpass));
		cr_pass = 1;
	} else {
		int num = read(rec, newpass, sizeof(newpass) - 1);
		if (num > 0)
			newpass[num] = '\0';
		else
			newpass[0] = '\0';
		close(rec);
	}

	rtn = ns_verify(mach_name, newpass);
	if (rtn == (char *)NULL) {
		ERROR("warning: no entry for this host in domain passwd file on current name server");
		cr_pass = 0;
	} else {
		if (strcmp(rtn, INCORRECT) == 0) {
			ERROR("warning: host password does not match registered password on current name server");
			cr_pass = 0;
		}
	}

	if (cr_pass == 1) {
		if ((rec = creat(PASSFILE, 0600)) < 0) { 
			ERROR("warning: cannot create password file");
		} else if (write(rec, newpass, strlen(newpass)) < 0) {
			ERROR("warning cannot write password");
			close(rec);
			unlink(PASSFILE);
		}
	}

	/*
	 *	Start up a daemon that will wait for a messages from
	 *	other systems (for fumount, etc.).
	 */

	system("/usr/bin/rfudaemon &");

	/*
	 *	Finally, clean up the /etc/advtab file.
	 */

#endif NAMESERVER
	close(open("/etc/advtab", O_TRUNC));
	exit(0);
	
	/* NOTREACHED */
}

static
ok_rtn()
{
	/*
	 *	everyting went smoothly in the name server startup
	 */

	ok_flag = 1;
}

static
error_rtn()
{
	rfstop();
	exit(2);
}

static
intrp_rtn()
{
	rfstop();
	killns();
	exit(2);
}

static
killns()
{
	/*
	 *	kill the name server process.
	 */

	if (kill(ns_process, SIGKILL) < 0) {
		ERROR("error in killing name server");
		perror(cmd_name);
	}
}

static
char	*
getdname()
{
	static char dname[MAXDNAME];
	FILE	*fp;

	if ((rfsys(RF_GETDNAME, dname, MAXDNAME) < 0) || (dname[0] == '\0')) {
		if (((fp = fopen(NSDOM,"r")) == NULL)
		|| (fgets(dname,MAXDNAME,fp) == NULL))
			return(NULL);
		/*
		 *	get rid of trailing newline, if there
		 */
		if (dname[strlen(dname)-1] == '\n')
			dname[strlen(dname)-1] = '\0';
		fclose(fp);
	}
	return(dname);
}

static
char	*
getnetspec()
{
	static char netspec[BUFSIZ];
	FILE	*fp;

	if (((fp = fopen(NETSPEC,"r")) == NULL)
	|| (fgets(netspec,BUFSIZ,fp) == NULL))
		return(NULL);
	/*
	 *	get rid of trailing newline, if there
	 */
	if (netspec[strlen(netspec)-1] == '\n')
		netspec[strlen(netspec)-1] = '\0';

	fclose(fp);
	return(netspec);
}
