#ifndef lint
static	char sccsid[] = "@(#)tty.c 1.1 92/07/30 SMI"; /* from UCB 4.1 10/01/80 */
#endif
/*
 * Type tty name
 */

char	*ttyname();

main(argc, argv)
char **argv;
{
	register char *p;

	p = ttyname(0);
	if(argc==2 && !strcmp(argv[1], "-s"))
		;
	else
		printf("%s\n", (p? p: "not a tty"));
	exit(p? 0: 1);
	/* NOTREACHED */
}
