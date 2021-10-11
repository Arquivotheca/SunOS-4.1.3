#ifndef lint
static	char sccsid[] = "@(#)ttysoftcar.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <strings.h>
#include <ttyent.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

char *cmdname;

enum opt { OFF, ON, PRINT };

enum verbose { QUIET, VERBOSE };

main(argc, argv)
	int argc;
	char **argv;
{
	register int c, i;
	enum opt opt = PRINT;
	extern int optind;
	extern char *optarg;

	cmdname = rindex(argv[0], '/');
	if (cmdname)
		cmdname++;
	else
		cmdname = argv[0];
	while ((c = getopt(argc, argv, "yna")) != EOF)
		switch (c) {
		case 'y':
			opt = ON;
			break;
		case 'n':
			opt = OFF;
			break;
		case 'a':
			if (argv[optind] != NULL)
				fprintf(stderr,
				    "%s: ignoring extra arguments after -a\n",
				    cmdname);
			setall();
			exit(0);
		default:
			fprintf(stderr,
			    "usage: %s [-y] [-n] tty ...\n\t%s -a\n",
			    cmdname, cmdname);
			exit(1);
		}

	for (i = optind; i < argc; i++)
		dotty(argv[i], opt, VERBOSE);
	exit(0);
	/* NOTREACHED */
}

setall()
{
	register struct ttyent *ty;

	/*
	 * Set up each tty mentioned in /etc/ttytab, but don't
	 * complain if the device entry doesn't exist or the
	 * device can't be opened, since we often have entries
	 * in ttytab and/or /dev for devices that don't exist
	 * on a particular machine.
	 */
	while ((ty = getttyent()) != NULL) {
		if (ty->ty_status & TTY_LOCAL)
			dotty(ty->ty_name, ON, QUIET);
		else
			dotty(ty->ty_name, OFF, QUIET);
	}
}

dotty(name, opt, verbose)
	char *name;
	enum opt opt;
	enum verbose verbose;
{
	char tty[100];
	int fd, val;
	char **pre;
	static int on = 1, off = 0;
	static char *prelist[] = { "", "/dev/", "/dev/tty", NULL };

	for (pre = prelist; *pre; pre++) {
		strcpy(tty, *pre);
		strcat(tty, name);
		if ((fd = open(tty, O_RDONLY|O_NDELAY)) < 0) {
			if (errno == ENOENT)
				continue;
			if ((errno != ENODEV && errno != ENXIO) ||
			    verbose == VERBOSE) {
				fprintf(stderr, "%s: ", cmdname);
				perror(tty);
			}
			return;
		} else
			goto ok;
	}
	if (verbose == VERBOSE)
		fprintf(stderr, "%s: %s: no such device\n", cmdname, name);
	return;

ok:
	switch (opt) {
	case ON:
		(void) ioctl(fd, TIOCSSOFTCAR, &on);
		break;
	case OFF:
		(void) ioctl(fd, TIOCSSOFTCAR, &off);
		break;
	case PRINT:
		if (ioctl(fd, TIOCGSOFTCAR, &val) < 0) {
			fprintf(stderr, "%s: ", cmdname);
			perror(tty);
		} else
			printf("%s is %c\n", name, val ? 'y' : 'n');
	}
	close(fd);
}
