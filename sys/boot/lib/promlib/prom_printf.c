/*	@(#)prom_printf.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>
#include <sys/varargs.h>

extern void prom_putchar();

/*VARARGS1*/
prom_printf(fmt, va_alist)
	char *fmt;
	va_dcl
{
	va_list adx;
	register int b, c, i, width;
	register char *s;
	void prom_printn();

	va_start(adx);

loop:
	width = 0;
	while ((c = *fmt++) != '%') {
		if (c == '\0')
			goto out;
		if (c == '\n')
			prom_putchar('\r');
		prom_putchar(c);
	}
again:
	c = *fmt++;
	if (c >= '2' && c <= '9') {
		width = c - '0';
		c = *fmt++;
	}
	switch (c) {

	case 'l':
		goto again;
	case 'x':
	case 'X':
		b = 16;
		goto number;
	case 'd':
	case 'D':
	case 'u':
		b = 10;
		goto number;
	case 'o':
	case 'O':
		b = 8;
number:
		prom_printn(va_arg(adx, u_long), b, width);
		break;
	case 'c':
		b = va_arg(adx, int);
		for (i = 24; i >= 0; i -= 8)
			if (c = (b >> i) & 0x7f) {
				if (c == '\n')
					prom_putchar('\r');
				prom_putchar(c);
			}
		break;
	case 's':
		s = va_arg(adx, char*);
		while (c = *s++) {
			if (c == '\n')
				prom_putchar('\r');
			prom_putchar(c);
		}
		break;

	case '%':
		prom_putchar('%');
		break;
	}
	goto loop;
out:
	va_end(x1);
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernel stacks.
 */
void
prom_printn(n, b, width)
	u_long n;
	int b, width;
{
	char prbuf[40];
	register char *cp;

	if (b == 10 && (int)n < 0) {
		prom_putchar('-');
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
		width--;
	} while (n);
	while (width-- > 0)
		*cp++ = '0';
	do {
		prom_putchar(*--cp);
	} while (cp > prbuf);
}
