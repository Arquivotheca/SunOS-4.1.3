#ifndef lint
static	char sccsid[] = "@(#)mkclocale.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <locale.h>

/*ARGSUSED*/
int
main(argc, argv)
	int argc;
	char **argv;
{
	register struct lconv *lcp;
	register char *p;
	register char c;

	lcp = localeconv();	/* get C locale's values */

	(void) printf("%s\n", lcp->decimal_point);
	(void) printf("%s\n", lcp->thousands_sep);
	p = lcp->grouping;
	while ((c = *p++) != '\0') {
		if (c == '\377')
			putchar('X');
		else
			putchar(c + '0');
	}
	putchar('\n');

	if (ferror(stdout)) {
		perror("mkclocale");
		return (1);
	} else
		return (0);
}
