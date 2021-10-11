/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)startdaemon.c 1.1 92/07/30 SMI"; /* from UCB 5.1 6/6/85 */
#endif not lint

/*
 * Tell the printer daemon that there are new files in the spool directory.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <strings.h>
#include "lp.local.h"

startdaemon(printer)
	char *printer;
{
	struct sockaddr_un s_un;
	register int s, n;
	char buf[BUFSIZ];

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		perr("socket");
		return (0);
	}
	s_un.sun_family = AF_UNIX;
	strcpy(s_un.sun_path, SOCKETNAME);
	if (connect(s, (struct sockaddr *)&s_un, strlen(s_un.sun_path)+2) < 0) {
		perr("connect");
		(void) close(s);
		return (0);
	}
	(void) sprintf(buf, "\1%s\n", printer);
	n = strlen(buf);
	if (write(s, buf, n) != n) {
		perr("write");
		(void) close(s);
		return (0);
	}
	if (read(s, buf, 1) == 1) {
		if (buf[0] == '\0') {		/* everything is OK */
			(void) close(s);
			return (1);
		}
		putchar(buf[0]);
	}
	while ((n = read(s, buf, sizeof buf)) > 0)
		fwrite(buf, 1, n, stdout);
	(void) close(s);
	return (0);
}

static
perr(msg)
	char *msg;
{
	extern char *name;
	extern int sys_nerr;
	extern char *sys_errlist[];
	extern int errno;

	printf("%s: %s: ", name, msg);
	fputs(errno < sys_nerr ? sys_errlist[errno] : "Unknown error", stdout);
	putchar('\n');
}
