/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)mdiv.c 1.1 92/07/30 SMI"; /* from UCB 5.1 4/30/85 */
#endif

/* LINTLIBRARY */

#include <mp.h>
#include <stdio.h>

mdiv(a, b, q, r) 
	MINT *a, *b, *q, *r;
{
	MINT x, y;
	int signq, signr;

	signq = 1;
	signr = 1;
	x.len = y.len = 0;
	_mp_move(a, &x);
	_mp_move(b, &y);
	if (x.len < 0) {
		signq = -1;
		signr = -1;
		x.len = -x.len;
	}
	if (y.len < 0) {
		signq = -signq;
		y.len = -y.len;
	}
	xfree(q);
	xfree(r);
	m_div(&x, &y, q, r);
	if (signq == -1) {
		q->len = -q->len;
	}
	if (signr == -1) {
		r->len = -r->len;
	}
	xfree(&x);
	xfree(&y);
}


m_dsb (qx, n, a, b) 
	register short qx;
	int n;
	short *a, *b;
{
	register long borrow;
	register int j;
	register short fifteen = 15;
	register short *aptr, *bptr;

	borrow = 0;
	aptr = a;
	bptr = b;
	for (j = n; j > 0; j--) {
		borrow -= (*aptr++) * qx - *bptr;
		*bptr++ = borrow & 077777;
		borrow >>= fifteen;
	}
	borrow += *bptr;
	*bptr = borrow & 077777;
	if (borrow >> fifteen == 0) {
		return(0);
	}
	borrow = 0;
	aptr = a;
	bptr = b;
	for (j = n; j > 0; j--) {
		borrow += *aptr++ + *bptr;
		*bptr++ = borrow & 077777;
		borrow >>= fifteen;
	}
	return(1);
}



m_trq (v1, v2, u1, u2, u3) 
	register short v1;
	register short v2;
	short u1;
	short u2;
	short u3;
{
	register short d;
	register long x1;
	register long c1;

	c1 = u1 * 0100000 + u2;
	if (u1 == v1) {
		d = 077777;
	} else {
		d = c1 / v1;
	}
	do {	
		x1 = c1 - v1 * d;
		x1 = x1 * 0100000 + u3 - v2 * d;
		 --d;
	} while (x1 < 0);
	return(d + 1);
}


m_div (a, b, q, r) 
	MINT *a, *b, *q, *r;
{
	MINT u, v, x, w;
	short d;
	register short *qval;
	register short *uval;
	register int j;
	register int qq;
	register int n;
	register int v1;
	register int v2;

	u.len = v.len = x.len = w.len = 0;
	if (b->len == 0) {
		_mp_fatal("mdiv divide by zero");
		return;
	}
	if (b->len == 1) {
		r->val = xalloc(1, "m_div1");
		sdiv (a, b->val[0], q, r->val);
		if (r->val[0] == 0) {
			free ((char *) r->val);
			r->len = 0;
		} else {
			r->len = 1;
		}
		return;
	}
	if (a -> len < b -> len) {
		q->len = 0;
		r->len = a->len;
		r->val = xalloc(r->len, "m_div2");
		for (qq = 0; qq < r->len; qq++) {
			r->val[qq] = a->val[qq];
		}
		return;
	}
	x.len = 1;
	x.val = &d;
	n = b->len;
	d = 0100000L / (b->val[n - 1] + 1L);
	mult(a, &x, &u);	/* subtle: relies on mult allocing extra space */
	mult(b, &x, &v);
	v1 = v.val[n - 1];
	v2 = v.val[n - 2];
	qval = xalloc(a -> len - n + 1, "m_div3");
	uval = u.val;
	for (j = a->len - n; j >= 0; j--) {
		qq = m_trq(v1, v2, uval[j + n], uval[j + n - 1], uval[j + n - 2]);
		if (m_dsb(qq, n, v.val, uval + j))
			qq -= 1;
		qval[j] = qq;
	}
	x.len = n;
	x.val = u.val;
	_mp_mcan(&x);
	sdiv(&x, d, &w, &d);
	r->len = w.len;
	r->val = w.val;
	q->val = qval;
	qq = a->len - n + 1;
	if (qq > 0 && qval[qq - 1] == 0)
		qq -= 1;
	q->len = qq;
	if (qq == 0)
		free ((char *) qval);
	if (x.len != 0)
		xfree (&u);
	xfree (&v);
}
