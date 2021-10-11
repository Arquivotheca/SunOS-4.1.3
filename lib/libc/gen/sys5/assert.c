#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)assert.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 *	called from "assert" macro; prints without printf or stdio.
 */

#define WRITE(s, n)	(void) write(2, (s), (n))
#define WRITESTR(s1, n, s2)	WRITE((s1), n), \
				WRITE((s2), (unsigned) strlen(s2))

#define	LINESTR ", line NNNNN\n"
char	*malloc();

_assert(assertion, filename, line_num)
char *assertion;
char *filename;
int line_num;
{
	static char *linestr;
	register char *p;
	register int div, digit;

	if (!linestr) {
		linestr = malloc(strlen(LINESTR)+1);
		strcpy(linestr, LINESTR);
	}
	p = &linestr[7];

	WRITESTR("Assertion failed: ", 18, assertion);
	WRITESTR(", file ", 7, filename);
	for (div = 10000; div != 0; line_num %= div, div /= 10)
		if ((digit = line_num/div) != 0 || p != &linestr[7] || div == 1)
			*p++ = digit + '0';
	*p++ = '\n';
	*p = '\0';
	WRITE(linestr, (unsigned) strlen(linestr));
	(void) abort();
}
