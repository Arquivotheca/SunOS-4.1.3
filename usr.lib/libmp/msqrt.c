/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)msqrt.c 1.1 92/07/30 SMI"; /* from UCB 5.1 4/30/85 */
#endif

/* LINTLIBRARY */

#include <mp.h>
msqrt(a, b, r) 
	MINT *a, *b, *r;
{
	MINT a0, x, junk, y;
	int j;

	a0.len = x.len = junk.len = y.len = 0;
	if (a->len < 0)
		_mp_fatal("msqrt: neg arg");
	if (a->len == 0) {
		b->len = 0;
		r->len = 0;
		return (0);
	}
	if (a->len % 2 == 1)
		x.len = (1 + a->len) / 2;
	else
		x.len = 1 + a->len / 2;
	x.val = xalloc(x.len, "msqrt");
	for (j = 0; j < x.len; x.val[j++] = 0);
	if (a->len % 2 == 1)
		x.val[x.len - 1] = 0400;
	else
		x.val[x.len - 1] = 1;
	_mp_move(a, &a0);
	xfree(b);
	xfree(r);
loop:
	mdiv(&a0, &x, &y, &junk);
	xfree(&junk);
	madd(&x, &y, &y);
	sdiv(&y, 2, &y, (short *) &j);
	if (mcmp(&x, &y) > 0) {
		xfree(&x);
		_mp_move(&y, &x);
		xfree(&y);
		goto loop;
	}
	xfree(&y);
	_mp_move(&x, b);
	mult(&x, &x, &x);
	msub(&a0, &x, r);
	xfree(&x);
	xfree(&a0);
	return (r->len);
}
