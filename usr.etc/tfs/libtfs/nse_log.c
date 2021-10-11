#ifndef lint
static char sccsid[] = "@(#)nse_log.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <nse/util.h>

/*
 * Routines to print error messages to both stdout and stderr.  These
 * routines are useful for the NSE daemons, which have the console as
 * stderr and a log file as stdout.
 */

/*
 * Set up stderr to go to /dev/console.  This routine leaves the process
 * with no controlling terminal, which is correct for a daemon.
 * (A daemon shouldn't have had a controlling terminal to start with.)
 */
void
nse_stderr_to_console()
{
	int		fd;

	fd = open("/dev/console", O_WRONLY, 0);
	if (fd < 0) {
		if (nse_get_cmdname()) {
			fprintf(stderr, "%s: ", nse_get_cmdname());
		}
		perror("open /dev/console");
		exit (1);
	}
	(void) dup2(fd, 2);
	(void) close(fd);
	setlinebuf(stderr);

	/*
	 * Disassociate from controlling terminal.  (Which became
	 * /dev/console when we opened it above.)
	 */
	fd = open("/dev/tty", O_RDWR);
	if (fd > 0) {
		ioctl(fd, TIOCNOTTY, 0);
		(void) close(fd);
	}
}


/*
 * Print error message to both stdout and stderr.
 */
/* VARARGS1 */
void
nse_log_message(format, a1, a2, a3, a4, a5, a6, a7, a8, a9)
	char		*format;
	int		a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
	char		*name;

	name = nse_get_cmdname();
	if (name) {
		fprintf(stderr, "%s: ", name);
	}
	fprintf(stderr, format, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	if (name) {
		fprintf(stdout, "%s: ", name);
	}
	fprintf(stdout, format, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}


void
nse_log_err_print(err)
	Nse_err		err;
{
	if (err) {
		fprintf(stderr, "%s\n", err->str);
		fprintf(stdout, "%s\n", err->str);
	}
}
