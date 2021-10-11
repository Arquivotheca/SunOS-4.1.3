/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)pow.c 1.1 92/07/30 SMI"; /* from UCB 5.1 4/30/85 */
#endif

/* LINTLIBRARY */

#include <mp.h>

pow(a, b, c, d) 
	MINT *a, *b, *c, *d;
{
	int i, j, n;
	MINT x, y;
	MINT a0, b0, c0;

	a0.len = b0.len = c0.len = x.len = y.len = 0;
	_mp_move(a, &a0);
	_mp_move(b, &b0);
	_mp_move(c, &c0);
	xfree(d);
	d->len = 1;
	d->val = xalloc (1, "pow");
	*d->val = 1;
	for (j = 0; j < b0.len; j++) {
		n = b0.val[b0.len - j - 1];
		for (i = 0; i < 15; i++) {
			mult(d, d, &x);
			mdiv(&x, &c0, &y, d);
			if ((n = n << 1) & 0100000) {
				mult(&a0, d, &x);
				mdiv(&x, &c0, &y, d);
			}
		}
	}
	xfree(&x);
	xfree(&y);
	xfree(&a0);
	xfree(&b0);
	xfree(&c0);
}


rpow (a, n, b) 
	MINT *a, *b;
{
	MINT x, y;
	int     i;

	x.len = 1;
	x.val = xalloc(1, "rpow");
	*x.val = n;
	y.len = n * a->len + 4;
	y.val = xalloc(y.len, "rpow2");
	for (i = 0; i < y.len; i++)
		y.val[i] = 0;
	y.val[y.len - 1] = 010000;
	pow(a, &x, &y, b);
	xfree(&x);
	xfree(&y);
}
