/*
 * @(#)get.c 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include <sys/types.h>
#include <mon/sunromvec.h>

#ifdef OPENPROMS
extern	int	prom_mayget();
#else
#define prom_mayget	(*romp->v_mayget)
#endif OPENPROPMS

getchar()
{
	register int c;

	while ((c = prom_mayget()) == -1)
		;
	if (c == '\r') {
		putchar(c);
		c = '\n';
	}
	if (c == 0177 || c == '\b') {
		putchar('\b');
		putchar(' ');
		c = '\b';
	}
	putchar(c);
	return (c);
}

gets(buf)
	char *buf;
{
	register char *lp;
	register c;

	lp = buf;
	for (;;) {
		c = getchar() & 0177;
		switch(c) {
		case '\n':
		case '\r':
			c = '\n';
			*lp++ = '\0';
			return;
		case '\b':
			lp--;
			if (lp < buf)
				lp = buf;
			continue;
		case 'u'&037:			/* ^U */
			lp = buf;
			putchar('\r');
			putchar('\n');
			continue;
		default:
			*lp++ = c;
		}
	}
}
