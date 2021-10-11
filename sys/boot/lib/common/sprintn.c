/*	@(#)sprintn.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


/*
 * sprintn formats a number n in base b and puts output to callers buffer.
 * We don't use recursion to avoid deep kernel stacks.
 *
 * Null terminates the buffer and returns the number of non-null bytes
 * added to the output buffer. (minimum of one zero).
 */

int
sprintn(p, n, b)
	char *p;
	register unsigned long n;
	register int b;
{
	char prbuf[40];
	register char *cp;
	register char *bp = p;

	if (b == 10 && (int)n < 0) {
		*bp++ = '-';
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
	} while (n);
	do {
		*bp++ = *--cp;
	} while (cp > prbuf);
	*bp = (char)0;
	return (bp - p);
}
