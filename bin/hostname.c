#ifndef lint
static	char sccsid[] = "@(#)hostname.c 1.1 92/07/30 SMI"; /* from UCB 1.3 83/03/30 */
#endif
/*
 * hostname -- get (or set hostname)
 */
#include <stdio.h>
#include <sys/param.h>

char hostname[MAXHOSTNAMELEN];
extern int errno;

main(argc,argv)
	char *argv[];
{
	int	myerrno;

	argc--;
	argv++;
	if (argc) {
		if (sethostname(*argv,strlen(*argv)))
			perror("sethostname");
		myerrno = errno;
	} else {
		gethostname(hostname,sizeof(hostname));
		myerrno = errno;
		printf("%s\n",hostname);
	}
	exit(myerrno);
	/* NOTREACHED */
}
