#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)execute.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)execute.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		execute()
 *
 *	Description:	Execute a suninstall program in TOOL_DIR.
 *		Returns 1 if the process was successful and 0 otherwise.
 */

#include <sys/wait.h>
#include <errno.h>
#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		sprintf();




int
execute(argv)
	char **		argv;
{
	char		pathname[MAXPATHLEN];	/* path to program to exec */
	int		pid;			/* child's process id */
	int		ret_code;		/* return code */
	union wait	status;			/* child's return status */


#ifdef CATCH_SIGNALS
	/*
	 *	Ignore keyboard signals while child is running.
	 */
	(void) signal(SIGINT, SIG_IGN);
#endif
	(void) signal(SIGQUIT, SIG_IGN);

	(void) sprintf(pathname, "%s/%s", TOOL_DIR, argv[0]);
	pid = fork();
	if (pid == -1) {
		menu_log("%s: cannot create child process.", progname);
		menu_abort(1);
	}

	else if (pid == 0) {			/* this is the child */
		execv(pathname, argv);
		menu_log("%s: %s: %s.", progname, pathname, err_mesg(errno));
		menu_log("\tCannot execute install tool.");
		menu_abort(1);
	}
	else {					/* this is the parent */
		while ((ret_code = wait(&status)) != pid) {
			if (ret_code == -1) {
				menu_log("%s: %s: %s.", progname, pathname,
					 err_mesg(errno));
			        menu_log(
				      "Error while waiting for install tool.");
				menu_abort(1);
			}
		}
	}

#ifdef CATCH_SIGNALS
	/*
	 *	Restore signal values.
	 */
	(void) signal(SIGINT, SIG_DFL);
#endif
	(void) signal(SIGQUIT, SIG_IGN);

	return(status.w_status ? 0 : 1);
} /* end execute() */
