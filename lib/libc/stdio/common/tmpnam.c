#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)tmpnam.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

extern char *mktemp(), *strcpy(), *strcat();
static char str[L_tmpnam], seed[] = { 'a', 'a', 'a', '\0' };

char *
tmpnam(s)
char	*s;
{
	register char *p, *q;
	register int cnt = 0;

	p = (s == NULL)? str: s;
	(void) strcpy(p, P_tmpdir);
	(void) strcat(p, seed);
	(void) strcat(p, "XXXXXX");

	q = seed;
	while(*q == 'z') {
		*q++ = 'a';
		cnt++;
	}
	if (cnt < 3)
		++*q;

	(void) mktemp(p);
	return(p);
}
