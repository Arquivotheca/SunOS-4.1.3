#ifndef lint
static	char sccsid[] = "@(#)ppt.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>

void	putppt();

/*ARGSUSED*/
int
main(argc, argv)
	int argc;
	char **argv;
{
	register int c;

	(void) printf("___________\n");
	while ((c = getchar()) != EOF)
		putppt(c);
	(void) printf("___________\n");
	return (0);
}

void
putppt(c)
	register int c;
{
	register int i;

	(void) putchar('|');
	for (i = 7; i >= 0; i--) {
		if (i == 2)
			(void) putchar('.');	/* feed hole */
		if ((c&(1<<i)) != 0)
			(void) putchar('o');
		else
			(void) putchar(' ');
	}
	(void) putchar('|');
	(void) putchar('\n');
}
