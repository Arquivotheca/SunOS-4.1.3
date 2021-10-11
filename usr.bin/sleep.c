#ifndef lint
static	char sccsid[] = "@(#)sleep.c 1.1 92/07/30 SMI"; /* from UCB 4.1 10/01/80 */
#endif

#include	<stdio.h>
char	*cmdname;
char	*rindex();
#define	basename(s) (rindex(s,'/')?rindex(s,'/')+1:s)

main(argc, argv)
	int	argc;
	char	**argv;
{
	int c, n;
	char *s;

	cmdname = basename(argv[0]);
	if(argc != 2) usage();

	n = 0;
	s = argv[1];
	while(c = *s++) {
		if(c<'0' || c>'9') usage();
		n = n*10 + c - '0';
	}
	sleep(n);
	exit (0);
	/* NOTREACHED */
}

usage()
{
	fprintf (stderr, "Usage: %s time-in-seconds\n", cmdname);
	exit (1);
}
