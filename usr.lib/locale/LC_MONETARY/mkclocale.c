#ifndef lint
static	char sccsid[] = "@(#)mkclocale.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <locale.h>

static void	printnum(/*char num*/);
static void	printbool(/*char bool*/);

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

	(void) printf("%s\n", lcp->int_curr_symbol);
	(void) printf("%s\n", lcp->currency_symbol);
	(void) printf("%s\n", lcp->mon_decimal_point);
	(void) printf("%s\n", lcp->mon_thousands_sep);
	p = lcp->mon_grouping;
	while ((c = *p++) != '\0') {
		if (c == '\377')
			putchar('X');
		else
			putchar(c + '0');
	}
	putchar('\n');
	(void) printf("%s\n", lcp->positive_sign);
	(void) printf("%s\n", lcp->negative_sign);
	printnum(lcp->frac_digits);
	printbool(lcp->p_cs_precedes);
	printbool(lcp->p_sep_by_space);
	printbool(lcp->n_cs_precedes);
	printbool(lcp->n_sep_by_space);
	printnum(lcp->p_sign_posn);
	printnum(lcp->n_sign_posn);

	if (ferror(stdout)) {
		perror("mkclocale");
		return (1);
	} else
		return (0);
}

static void
printnum(num)
	char num;
{
	if (num == '\377')
		(void) printf("\n");
	else
		(void) printf("%d\n", num);
}

static void
printbool(bool)
	char bool;
{
	if (bool == '\377')
		(void) printf("\n");
	else
		(void) printf("%c\n", (bool ? 'y' : 'n'));
}
