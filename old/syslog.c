#ifndef lint
static	char sccsid[] = "@(#)syslog.c 1.1 92/08/05 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * syslog - make a syslog entry
 */

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>

char	*lnames[] = {
	"EMERG", "ALERT", "CRIT", "ERROR",
	"WARNING", "NOTICE", "INFO", "DEBUG", 0
};

int	level;		/* logging level */
int	flags;		/* flags for openlog */
char	*ident;		/* tag for log */
char	*name;		/* our name */

char	buf[BUFSIZ];

main(argc, argv)
	int argc;
	char **argv;
{
	int logstdin = 0;

	name = argv[0];
	while (--argc > 0 && (++argv)[0][0] == '-')
		switch (argv[0][1]) {

		case 'p':	/* log pid */
			flags |= LOG_PID;
			break;

		case 'i':	/* ident */
			if (argc > 1 && argv[1][0] != '-') {
				ident = argv[1];
				argv++;
				argc--;
			} else
				ident = name;
			break;

		case '\0':	/* log stdin */
			logstdin = 1;
			break;

		default:
			setlevel(&argv[0][1]);
			break;
		}
	(void) openlog(ident, flags, 0);
	if (logstdin) {
		while (fgets(buf, sizeof (buf) - 1, stdin) != NULL)
			syslog(level, buf);
	} else {
		while (argc-- > 0) {
			strcat(buf, " ");
			strcat(buf, *argv++);
		}
		syslog(level, buf+1);
	}
	exit(0);
	/* NOTREACHED */
}

setlevel(s)
	register char *s;
{
	register char **cp;

	if (isdigit(*s)) {
		level = atoi(s);
		if (level < LOG_EMERG || level > LOG_DEBUG) {
			fprintf(stderr, "%s: level must be in range %d - %d\n",
			    name, LOG_EMERG, LOG_DEBUG);
			exit(1);
		}
	} else {
		if (!isupper(*s)) {
			fprintf(stderr, "%s: unknown option '%s'\n", name, s);
			exit(1);
		}
		for (cp = lnames; *cp; cp++)
			if (strcmp(*cp, s) == 0)
				break;
		if (*cp)
			level = cp - lnames + 1;
		else {
			fprintf(stderr,
			    "%s: unknown level name, valid names are:\n", name);
			for (cp = lnames; *cp; cp++)
				fprintf(stderr, "%s%s",
				    cp == lnames ? "" : ", ", *cp);
			fprintf(stderr, "\n");
			exit(1);
		}
	}
	if (getuid() && level <= LOG_ALERT) {
		fprintf(stderr, "%s: must be superuser to log at level %d\n",
		    name, level);
		exit(1);
	}
}
