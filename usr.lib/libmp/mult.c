/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)mult.c 1.1 92/07/30 SMI"; /* from UCB 5.1 4/30/85 */
#endif

/* LINTLIBRARY */

#include <mp.h>

mult(a,b,c) 
	register struct mint *a,*b,*c;
{	
	struct mint x,y;
	int sign;

	_mp_mcan(a);
	_mp_mcan(b);
	if (a->len == 0 || b->len == 0) {
		xfree(c);	
		return;
	}
	sign = 1;
	x.len = y.len = 0;
	_mp_move(a, &x);
	_mp_move(b, &y);
	if (a->len < 0) {	
		x.len = -x.len;
		sign = -sign;
	}
	if (b->len < 0) {	
		y.len = -y.len;
		sign = -sign;
	}
	xfree(c);
	if (x.len < y.len) {
		m_mult(&x,&y,c);
	} else {
		m_mult(&y,&x,c);
	}
	if (sign < 0) 
		c->len = -c->len;
	if (c->len == 0) 
		xfree(c);
	xfree(&x);	
	xfree(&y);
}

/* 
 * Knuth  4.3.1, Algorithm M 
 */
m_mult(a,b,c) 
	struct mint *a;
	struct mint *b;
	struct mint *c;
{	
	register int i,j;
	register long sum; 
	register short bcache;
	register short *aptr;
	register short *bptr;
	register short *cptr;
	register unsigned short fifteen = 15;
	register int alen;
	int blen;

#	define BASEBITS	(8*sizeof(short)-1)
#	define BASE		(1 << BASEBITS)
#	define LOWBITS 	(BASE - 1)

	alen = a->len;
	blen = b->len;

	c->len = alen + blen;
	c->val = xalloc(c->len,"m_mult");

	aptr = a->val;
	bptr = b->val;
	cptr = c->val;

	sum = 0;
	bcache = *bptr++;
	for (i = alen; i > 0; i--) {
 		sum += *aptr++ * bcache;
		*cptr++ = sum & LOWBITS;
		sum >>= fifteen;
	}
	*cptr = sum;
	aptr -= alen;
	cptr -= alen; 
	cptr++;

	for (j = blen - 1; j > 0; j--) {
		sum = 0;
		bcache = *bptr++;
		for (i = alen; i > 0; i--) {
			sum += *aptr++ * bcache + *cptr;
			*cptr++ = sum & LOWBITS;
			sum >>= fifteen;	
		}
		*cptr = sum;
		aptr -= alen;
		cptr -= alen; 
		cptr++;
	}
	if (c->val[c->len-1] == 0) {
		c->len--;
	}
}
