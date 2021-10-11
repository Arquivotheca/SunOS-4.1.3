#ifndef lint
static	char sccsid[] = "@(#)accton.c 1.1 92/07/30 SMI"; /* from UCB 4.1 */
#endif

#include <stdio.h>
#include <errno.h>

main(argc, argv)
	int argc;
	char **argv;
{
	extern int errno;
	int status;

	if (--argc > 1) {
		fprintf(stderr, "usage: accton [ filename ]\n");
		exit(1);
	}
	status = acct((argc != 0) ? argv[1] : (char *)0);
	if (status == -1) {
		if (errno == EINVAL)
			fprintf(stderr, 
		"accton: the kernel is not configured for accounting\n");
		else
			perror("accton");
		exit(1);
	} else
		exit(0);
	/* NOTREACHED */
}
