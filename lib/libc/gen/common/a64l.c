#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)a64l.c 1.1 92/07/30 SMI"; /* from S5R2 1.5 */
#endif

/*LINTLIBRARY*/
/*
 * convert base 64 ascii to long int
 * char set is [./0-9A-Za-z]
 *
 */

#define BITSPERCHAR	6 /* to hold entire character set */

long
a64l(s)
register char *s;
{
	register int i, c;
	long lg = 0;

	for (i = 0; (c = *s++) != '\0'; i += BITSPERCHAR) {
		if (c > 'Z')
			c -= 'a' - 'Z' - 1;
		if (c > '9')
			c -= 'A' - '9' - 1;
		lg |= (long)(c - ('0' - 2)) << i;
	}
	return (lg);
}
