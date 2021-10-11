#ifndef lint
static	char sccsid[] = "@(#)scheck.c 1.1 92/07/30 SMI"; /* from Arthur Olson's 8.9 */
#endif /* !defined lint */

/*LINTLIBRARY*/

#include <stdio.h>
#include <ctype.h>

#ifdef __STDC__

#define P(s)		s

#else /* !defined __STDC__ */

#define ASTERISK	*
#define P(s)		(/ASTERISK s ASTERISK/)

#define const

#endif /* !defined __STDC__ */

extern char *	imalloc P((int n));
extern void	ifree P((char * p));

char *
scheck(string, format)
const char * const	string;
const char * const	format;
{
	register char *		fbuf;
	register const char *	fp;
	register char *		tp;
	register int		c;
	register char *		result;
	char			dummy;

	result = "";
	if (string == NULL || format == NULL)
		return result;
	fbuf = imalloc(2 * strlen(format) + 4);
	if (fbuf == NULL)
		return result;
	fp = format;
	tp = fbuf;
	while ((*tp++ = c = *fp++) != '\0') {
		if (c != '%')
			continue;
		if (*fp == '%') {
			*tp++ = *fp++;
			continue;
		}
		*tp++ = '*';
		if (*fp == '*')
			++fp;
		while (isascii(*fp) && isdigit(*fp))
			*tp++ = *fp++;
		if (*fp == 'l' || *fp == 'h')
			*tp++ = *fp++;
		else if (*fp == '[')
			do *tp++ = *fp++;
				while (*fp != '\0' && *fp != ']');
		if ((*tp++ = *fp++) == '\0')
			break;
	}
	*(tp - 1) = '%';
	*tp++ = 'c';
	*tp = '\0';
	if (sscanf(string, fbuf, &dummy) != 1)
		result = (char *) format;
	ifree(fbuf);
	return result;
}
