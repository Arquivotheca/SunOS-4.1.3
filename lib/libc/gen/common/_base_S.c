#ifndef lint
static char     sccsid[] = "@(#)_base_S.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "base_conversion.h"

/*	Fundamental utilities for base conversion that should be recoded as assembly language subprograms or as inline expansion templates. */

void
_fourdigitsquick(t, d)
	short unsigned  t;
	char            *d;

/* Converts t < 10000 into four ascii digits at *pc.     */

{
	register short  i;

	i = 3;
	do {
		d[i] = '0' + t % 10;
		t = t / 10;
	}
	while (--i != -1);
}

void
_multiply_base_two_vector(n, px, py, product)
	short unsigned  n, *py;
	_BIG_FLOAT_DIGIT *px, product[3];

{
	/*
	 * Given xi and yi, base 2**16 vectors of length n, computes dot
	 * product
	 * 
	 * sum (i=0,n-1) of x[i]*y[n-1-i]
	 * 
	 * Product may fill as many as three short-unsigned buckets. Product[0]
	 * is least significant, product[2] most.
	 */

	unsigned long   acc, p;
	short unsigned  carry;
	int             i;

	acc = 0;
	carry = 0;
	for (i = 0; i < n; i++) {
	p=_umac(px[i],py[n - 1 - i],acc);
		if (p < acc)
			carry++;
		acc = p;
	}
	product[0] = (_BIG_FLOAT_DIGIT) (acc & 0xffff);
	product[1] = (_BIG_FLOAT_DIGIT) (acc >> 16);
	product[2] = (_BIG_FLOAT_DIGIT) (carry);
}

void
_multiply_base_ten_vector(n, px, py, product)
	short unsigned  n, *py;
	_BIG_FLOAT_DIGIT *px, product[3];

{
	/*
	 * Given xi and yi, base 10**4 vectors of length n, computes dot
	 * product
	 * 
	 * sum (i=0,n-1) of x[i]*y[n-1-i]
	 * 
	 * Product may fill as many as three short-unsigned buckets. Product[0]
	 * is least significant, product[2] most.
	 */

#define ABASE	3000000000	/* Base of accumulator. */

	unsigned long   acc;
	short unsigned  carry;
	int             i;

	acc = 0;
	carry = 0;
	for (i = 0; i < n; i++) {
	acc=_umac(px[i],py[n - 1 - i],acc);
		if (acc >= (unsigned long) ABASE) {
			carry++;
			acc -= ABASE;
		}
	}
	/*
	 NOTE: because 
		acc * <= ABASE-1,
		acc/10000 <= 299999
	 which would overflow a short unsigned
	 */
	product[0] = (_BIG_FLOAT_DIGIT) (acc % 10000);
	acc /= 10000;
	product[1] = (_BIG_FLOAT_DIGIT) (acc % 10000);
	acc /= 10000;
	product[2] = (_BIG_FLOAT_DIGIT) (acc + (ABASE / 100000000) * carry);
}

