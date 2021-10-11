#ifndef lint
static	char sccsid[] = "@(#)open_sccs_log.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>

#include <fcntl.h>

extern int open();
extern int close();

FILE *
open_sccs_log()
{
	register int fd;
	register FILE *fp;

	/*
	 * Do not create the file if it does not exist.  Append to it
	 * if it does.
	 */
	if ((fd = open("/var/adm/sccslog", O_WRONLY|O_APPEND)) < 0)
		return (NULL);

	if ((fp = fdopen(fd, "a")) == NULL) {
		(void) close(fd);
		return (NULL);
	}

	return (fp);
}
