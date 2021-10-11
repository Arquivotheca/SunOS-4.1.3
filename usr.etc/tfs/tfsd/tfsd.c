#ifndef lint
static char sccsid[] = "@(#)tfsd.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

/*
 * TFS server (tfsd)
 *
 *	*** How to debug the tfsd ***
 *
 * The tfsd will log incoming requests if the 'tfsdebug' variable is
 * set to certain values.  (See the comment for the dprint() routine,
 * in tfs_printf.c)  The incoming requests will be logged to the
 * file /tmp/tfsd.logXXXXXX, where XXXXXX is a string containing the
 * process ID of the tfsd.  The tfsdebug variable's value can be changed
 * by sending a SIGUSR1 signal to the tfsd (with the 'kill -USR1 xxx'
 * command'.)  If the tfsd is running with tfsdebug = 0, and a
 * SIGUSR1 signal is received, then tfsdebug will be set to 4, and
 * the tfsd will subsequently log all TFS calls and their arguments.
 * A SIGUSR1 signal received when the tfsd is running with tfsdebug = 4
 * will cause the tfsd to set tfsdebug = 15, which will cause the tfsd
 * to log the arguments and results of all TFS calls, along with other
 * debugging messages.  A SIGUSR1 signal received when the tfsd is
 * running with tfsdebug = 15 will cause tfsdebug to be set to 0.
 *
 * Additionally, the tfsd will print its internal vnode and pnode tables
 * if the NULLPROC RPC request is received when tfsdebug > 0.  To effect
 * this, one would type the following commands:
 *	kill -USR1 xxx				(to set tfsdebug = 4)
 *	/usr/etc/rpcinfo -u my_machine 100037	(send NULLPROC request
 *						 to RPC program # of TFS)
 *
 * If the tfsd is running with tfsdebug = 7, then it will print its busy
 * and idle times every 100 requests.  (Busy time = total amount of wall
 * clock time that the tfsd spent servicing requests.)
 *
 * There is an alternate way to set the value of the 'tfsdebug' variable:
 * if the "TFSDEBUG" environment variable is set to a number, then the tfsd
 * will set 'tfsdebug' to that number.  This is useful if the user wants to
 * set tfsdebug to something other than 0, 4, or 15, which is all the
 * SIGUSR1 signal allows one to do.  The "TFSDEBUG" environment variable
 * has to be set before the inetd starts, so that it will be in the
 * environment inherited by the tfsd when the inetd forks it as a child.
 * One way to do this is to put the line "setenv TFSDEBUG 7" in the /etc/rc
 * file before the line that starts the inetd.
 */

#include "headers.h"
#include "vnode.h"
#include "tfs.h"
#include "subr.h"
#include <signal.h>
#include <nse/util.h>


SVCXPRT		*tfsd_xprt;
char		tfsd_log_name[] = "/tmp/tfsd.logXXXXXX";
char		tfsd_lock_name[] = "/tmp/tfsd.running";
time_t		tfsd_timestamp;
#ifdef TFSDEBUG
extern int	print_to_screen;
#endif TFSDEBUG

extern char	*getenv();
extern char	*mktemp();

int		main();
static void	segv_handler();
static void	sig_handler();
static void	term_handler();
static void	usr1_handler();


main(argc, argv)
	int		argc;
	char           *argv[];
{
	SVCXPRT		*transp;
	int		fd;
	struct stat	statb;
#ifdef TFSDEBUG
	struct sockaddr_in addr;
	int		len = sizeof(struct sockaddr_in);
	int		debug = 0;
	int		proto;
	char		*env_str;

	nse_set_cmdname(argv[0]);
	if (!debug && getsockname(0, (struct sockaddr *) &addr, &len) == -1) {
		/*
		 * Don't allow the tfsd to start if it wasn't started by
		 * inetd.  If you *really* want to start the tfsd by hand,
		 * you'll have to set debug = 1 before reaching this
		 * statement.
		 */
		if (errno == ENOTSOCK) {
			fprintf(stderr,
				"%s should only be started by inetd\n",
				nse_get_cmdname());
			exit (1);
		} else {
			perror("tfsd: getsockname");
			exit (1);
		}
	}

	if (debug) {
		if (argc != 2) {
			fprintf(stdout, "usage: %s -dX\n", argv[0]);
			exit (1);
		}
		if (argc > 1 && !strncmp(argv[1], "-d", 2)) {
			tfsdebug = atoi(&argv[1][2]);
			fprintf(stdout, "%s starting in debug mode %d\n",
				argv[0], tfsdebug);
		}
	} else {
		env_str = getenv("TFSDEBUG");
		if (env_str == NULL) {
			tfsdebug = 0;
		} else {
			tfsdebug = atoi(env_str);
		}
	}
#endif TFSDEBUG
	if (setpriority(PRIO_PROCESS, 0, -3) < 0) {
		perror("tfsd: set_priority");
		exit (1);
	}
	(void) umask(0);
#ifdef TFSDEBUG
	if (!debug) {
		print_to_screen = 0;
#endif
		(void) mktemp(tfsd_log_name);
		fd = open(tfsd_log_name, O_RDWR | O_CREAT | O_TRUNC, 0666);
		(void) dup2(fd, 1);
		(void) close(fd);
#ifdef TFSDEBUG
	}
#endif
	tfsd_timestamp = time((time_t *)0);
	setlinebuf(stdout);
	/*
	 * Reset stderr to go to the console.
	 */
	nse_stderr_to_console();

	if (stat(tfsd_lock_name, &statb) == 0) {
		fprintf(stderr,
		     "%s: warning: new server starting -- old server died?\n",
			nse_get_cmdname());
	} else {
		fd = creat(tfsd_lock_name, 0777);
		(void) close(fd);
	}

	/*
	 * Signals which cause core dumps.
	 */
	signal(SIGQUIT, segv_handler);
	signal(SIGILL, segv_handler);
	signal(SIGTRAP, segv_handler);
	signal(SIGEMT, segv_handler);
	signal(SIGFPE, segv_handler);
	signal(SIGBUS, segv_handler);
	signal(SIGSEGV, segv_handler);
	signal(SIGSYS, segv_handler);
	/*
	 * Signals which would cause the process to terminate if not caught.
	 */
	signal(SIGHUP, sig_handler);
	signal(SIGINT, sig_handler);

	signal(SIGTERM, term_handler);
	signal(SIGUSR1, usr1_handler);

	fd = 0;
	proto = 0;
#ifdef TFSDEBUG
	if (debug) {
		fd = RPC_ANYSOCK;
		proto = IPPROTO_UDP;
		pmap_unset(TFS_PROGRAM, TFS_VERSION);
		pmap_unset(TFS_PROGRAM, TFS_OLD_VERSION);
	}
#endif TFSDEBUG
	if ((transp = svcudp_create(fd)) == (SVCXPRT *) NULL) {
		nse_log_message("couldn't create TFS server\n");
		exit (1);
	}
	if (!svc_register(transp, TFS_PROGRAM, TFS_VERSION, tfs_dispatch,
			  proto)) {
		nse_log_message("couldn't register TFS service\n");
		exit (1);
	}
	if (!svc_register(transp, TFS_PROGRAM, TFS_OLD_VERSION, tfs_dispatch,
			  proto)) {
		nse_log_message("couldn't register old TFS service\n");
		exit (1);
	}
	if (!svc_register(transp, NFS_PROGRAM, NFS_VERSION, tfs_dispatch, 0)) {
		nse_log_message("couldn't register NFS service\n");
		exit (1);
	}
	tfsd_xprt = transp;
	init_dispatch_buffers();
	root_pnode = alloc_pnode((struct pnode *) NULL, "/");
	/*
	 * The root pnode is never freed.
	 */
	root_pnode->p_refcnt = 1;
	init_alarm();
	init_swap();
	init_dupreq_cache();
	init_fd_cache();
#ifdef TFSDEBUG
	dprint(tfsdebug, 1, "about to svc_run\n");
	trace_init();
#endif TFSDEBUG
	svc_run();
	nse_log_message("svc_run shouldn't have returned\n");
	exit (1);
}


/*
 * Force core to be dumped in /tmp when a signal occurs which would cause a
 * core dump.
 */
/* ARGSUSED */
static void
segv_handler(sig, code, scp)
	int		sig;
	int		code;
	struct sigcontext *scp;
{
	kill_me();
}


/* ARGSUSED */
static void
sig_handler(sig, code, scp)
	int		sig;
	int		code;
	struct sigcontext *scp;
{
	nse_log_message("warning: signal %d received\n", sig);
	sync_entire_wcache();
}


/*
 * shutdown(8) sends the TERM signal to all processes.  Make sure that we
 * sync the write cache before exiting.
 */
/* ARGSUSED */
static void
term_handler(sig, code, scp)
	int		sig;
	int		code;
	struct sigcontext *scp;
{
	sync_entire_wcache();
	exit (0);
}


/*
 * The SIGUSR1 signal causes the tfsd to change its debug logging level
 * (none/low/high)
 */
/* ARGSUSED */
static void
usr1_handler(sig, code, scp)
	int		sig;
	int		code;
	struct sigcontext *scp;
{
#ifdef TFSDEBUG
	if (tfsdebug == 0) {
		tfsdebug = 4;
	} else if (tfsdebug == 4) {
		tfsdebug = 15;
	} else {
		tfsdebug = 0;
	}
#endif TFSDEBUG
}
