#ifndef lint
static	char sccsid[] = "@(#)pwd.c 1.1 92/07/30 SMI"; /* from UCB 4.4 83/01/05 */
#endif
/*
 * pwd
 */
#include <stdio.h>
#include <sys/param.h>

char *getwd();

main()
{
	char pathname[MAXPATHLEN + 1];

	if (getwd(pathname) == NULL) {
		fprintf(stderr, "pwd: %s\n", pathname);
		exit(1);
	}
	printf("%s\n", pathname);
	exit(0);
	/* NOTREACHED */
}
