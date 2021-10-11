/* @(#)printf.c 1.1 92/07/30 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 */

#ifdef printf
#  undef printf
#endif

#include <sys/types.h>
#include <varargs.h>
#include <mon/sunromvec.h>

static void printn();

/*
 * Scaled down version of kernel printf (which, of course, is itself
 * a scaled down version of the libc printf).
 * Used to print diagnostic information directly on console tty.
 * Since it is not interrupt driven, all system activities are
 * suspended.  Printf should not be used for chit-chat.
 *
 * Does not support:
 *	f, e, E, g, G formats
 *	-, +, blank, # adjustment flags
 *	* runtime field width or precision specification
 *
 * One additional format: %b is supported to decode error registers.
 * Usage is:
 *	printf("reg=%b\n", regval, "<base><arg>*");
 * Where <base> is the output base expressed as a control character,
 * e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the next characters (up to a control character, i.e.
 * a character <= 32), give the name of the register.  Thus
 *	printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * would produce output:
 *	reg=3<BITTWO,BITONE>
 */

/*VARARGS1*/
printf(fmt, va_alist)
	register char *fmt;
	va_dcl
{
	register va_list adx;
	register int b, c, i;
	register char *s;
	int any, width, pad;

	va_start(adx);

	for ( ;; ) {
		while ((c = *fmt++) != '%') {
			if (c == '\0') {
				va_end(adx);
				return;
			}
			if (c == '\n')
				putchar('\r');
			putchar(c);
		}

		c = *fmt++;		/* got a '%' */
		width = pad = 0;
		while (c == '0') {
			pad = 1;
			c = *fmt++;
		}
		while (c >= '0' && c <= '9') {
			width = (c - '0') + (width * 10);
			c = *fmt++;
		}
again:
		switch (c) {

		case 'l':
			c = *fmt++;
			goto again;

		case 'x': case 'X':
			b = 16;
			goto number;

		case 'd': case 'D':
		case 'u':		/* what a joke */
			b = 10;
			goto number;

		case 'o': case 'O':
			b = 8;
number:
			printn(va_arg(adx, u_long), b, width, pad);
			break;

		case 'c':
			b = va_arg(adx, int);
			for (i = 24; i >= 0; i -= 8)
				if (c = (b >> i) & 0x7f) {
					if (c == '\n')
						putchar('\r');
					putchar(c);
				}
			break;

		case 'b':
			b = va_arg(adx, int);
			s = va_arg(adx, char*);
			printn((u_long)b, *s++, 0);
			any = 0;
			if (b) {
				while (i = *s++) {
					if (b & (1 << (i-1))) {
						putchar(any? ',' : '<');
						any = 1;
						for (; (c = *s) > 32; s++)
							putchar(c);
					} else
						for (; *s > 32; s++)
							;
				}
				if (any)
					putchar('>');
			}
			break;

		case 's':
			s = va_arg(adx, char*);
			while (c = *s++) {
				if (c == '\n')
					putchar('\r');
				putchar(c);
			}
			break;

		case '%':
		default:
			putchar(c);
			break;
		}
	}
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernel stacks.
 */
static void
printn(n, b, width, pad)
	u_long n;
	int b, width, pad;
{
	char prbuf[128];
	register char *cp;

	if (b == 10 && (int)n < 0) {
		putchar('-');
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
		width--;
	} while (n);
	while (width-- > 0)
		*cp++ = (pad ? '0' : ' ');
	do {
		putchar(*--cp);
	} while (cp > prbuf);
}
