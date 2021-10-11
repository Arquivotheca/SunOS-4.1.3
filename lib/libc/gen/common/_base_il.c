#ifndef lint
static char     sccsid[] = "@(#)_base_il.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "base_conversion.h"

/*	The following should be coded as inline expansion templates.	*/

/*
 * Fundamental utilities that multiply two shorts into a unsigned long, add
 * carry, compute quotient and remainder in underlying base, and return
 * quo<<16 | rem as  a unsigned long.
 */

/*
 * C compilers tend to generate bad code - forcing full unsigned long by
 * unsigned long multiplies when what is really wanted is the unsigned long
 * product of half-long operands. Similarly the quotient and remainder are
 * all half-long. So these functions should really be implemented by inline
 * expansion templates.
 */

unsigned long
_umac(x, y, c)		/* p = x * y + c ; return p */
	_BIG_FLOAT_DIGIT x, y;
	unsigned long c;
{
	return x * (unsigned long) y + c;
}

unsigned long
_carry_in_b10000(x, c)		/* p = x + c ; return (p/10000 << 16 |
				 * p%10000) */
	_BIG_FLOAT_DIGIT x;
	long unsigned c;
{
	unsigned long   p = x + c ;

	return ((p / 10000) << 16) | (p % 10000);
}

void
_carry_propagate_two(carry, psignificand)
	unsigned long carry;
	_BIG_FLOAT_DIGIT *psignificand;
{
	/*
	 * Propagate carries in a base-2**16 significand.
	 */

	long unsigned   p;
	int             j;

	j = 0;
	while (carry != 0) {
	p = _carry_in_b65536(psignificand[j],carry);
		psignificand[j++] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
}

void
_carry_propagate_ten(carry, psignificand)
	unsigned long carry;
	_BIG_FLOAT_DIGIT *psignificand;
{
	/*
	 * Propagate carries in a base-10**4 significand.
	 */

	int             j;
	unsigned long p;

	j = 0;
	while (carry != 0) {
	p = _carry_in_b10000(psignificand[j],carry);
		psignificand[j++] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
}

