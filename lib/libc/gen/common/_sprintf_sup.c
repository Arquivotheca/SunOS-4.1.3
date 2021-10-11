#ifndef lint
static char     sccsid[] = "@(#)_sprintf_sup.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "base_conversion.h"

/*
 * Fundamental utilities of base conversion required for sprintf - but too
 * complex or too seldom used to be worth assembly language coding.
 */

unsigned long
_prodc_b10000(x, y, c)		/* p = x * y + c ; return (p/10000 << 16 |
				 * p%10000) */
	_BIG_FLOAT_DIGIT x, y;
	unsigned long   c;
{
	unsigned long   p = x * (unsigned long) y + c;

	return ((p / 10000) << 16) | (p % 10000);
}

unsigned long
_prod_b65536(x, y)		/* p = x * y ; return p */
	_BIG_FLOAT_DIGIT x, y;
{
	return x * (unsigned long) y;
}

unsigned long
_prod_b10000(x, y)		/* p = x * y ; return (p/10000 << 16 |
				 * p%10000) */
	_BIG_FLOAT_DIGIT x, y;
{
	unsigned long   p = x * (unsigned long) y;

	return ((p / 10000) << 16) | (p % 10000);
}

unsigned long
_lshift_b10000(x, n, c)		/* p = x << n + c ; return (p/10000 << 16 |
				 * p%10000) */
	_BIG_FLOAT_DIGIT x;
	short unsigned  n;
	long unsigned   c;
{
	unsigned long   p = (((unsigned long) x) << n) + c;

	return ((p / 10000) << 16) | (p % 10000);
}

unsigned long
_prod_10000_b65536(x, c)	/* p = x * 10000 + c ; return p */
	_BIG_FLOAT_DIGIT x;
	long unsigned   c;
{
	return x * (unsigned long) 10000 + c;
}

unsigned long
_prod_65536_b10000(x, c)	/* p = x << 16 + c ; return (p/10000 << 16 |
				 * p%10000) */
	_BIG_FLOAT_DIGIT x;
	long unsigned   c;
{
	unsigned long   p = (((unsigned long) x) << 16) + c;

	return ((p / 10000) << 16) | (p % 10000);
}

unsigned long
_carry_out_b10000(c)		/* p = c ; return (p/10000 << 16 | p%10000) */
	unsigned long   c;
{
	return ((c / 10000) << 16) | (c % 10000);
}

void
_left_shift_base_ten(pbf, multiplier)
	_big_float     *pbf;
	short unsigned  multiplier;

{
	/*
	 * Multiply a base-10**4 significand by 2<<multiplier.  Extend length
	 * as necessary to accommodate carries.
	 */

	short unsigned  length = pbf->blength;
	int             j;
	unsigned long   carry;
	long            p;

	carry = 0;
	for (j = 0; j < length; j++) {
		p = _lshift_b10000((_BIG_FLOAT_DIGIT) pbf->bsignificand[j], multiplier, carry);
		pbf->bsignificand[j] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
	while (carry != 0) {
		p = _carry_out_b10000(carry);
		pbf->bsignificand[j++] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
	pbf->blength = j;
}

void
_left_shift_base_two(pbf, multiplier)
	_big_float     *pbf;
	short unsigned  multiplier;
{
	/*
	 * Multiply a base-2**16 significand by 2<<multiplier.  Extend length
	 * as necessary to accommodate carries.
	 */

	short unsigned  length = pbf->blength;
	long unsigned   p;
	int             j;
	unsigned long   carry;

	carry = 0;
	for (j = 0; j < length; j++) {
		p = _lshift_b65536(pbf->bsignificand[j], multiplier, carry);
		pbf->bsignificand[j] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
	if (carry != 0) {
		pbf->bsignificand[j++] = (_BIG_FLOAT_DIGIT) (_carry_out_b65536(carry) & 0xffff);
	}
	pbf->blength = j;
}

void
_right_shift_base_two(pbf, multiplier, sticky)
	_big_float     *pbf;
	short unsigned  multiplier;
	_BIG_FLOAT_DIGIT *sticky;

{
	/* *pb = *pb / 2**multiplier	to normalize.	15 <= multiplier <= 1 */
	/* Any bits shifted out got to *sticky. */

	long unsigned   p;
	int             j;
	unsigned long   carry;

	carry = 0;
	for (j = pbf->blength - 1; j >= 0; j--) {
		p = _rshift_b65536(pbf->bsignificand[j], multiplier, carry);
		pbf->bsignificand[j] = (_BIG_FLOAT_DIGIT) (p >> 16);
		carry = p & 0xffff;
	}
	*sticky = (_BIG_FLOAT_DIGIT) carry;
}

void
_multiply_base_ten(pbf, multiplier)
	_BIG_FLOAT_DIGIT multiplier;
	_big_float     *pbf;
{
	/*
	 * Multiply a base-10**4 significand by multiplier.  Extend length as
	 * necessary to accommodate carries.
	 */

	int             j;
	unsigned long   carry;
	long            p;

	carry = 0;
	for (j = 0; j < pbf->blength; j++) {
		p = _prodc_b10000((_BIG_FLOAT_DIGIT) pbf->bsignificand[j], multiplier, carry);
		pbf->bsignificand[j] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
	while (carry != 0) {
		p = _carry_out_b10000(carry);
		pbf->bsignificand[j++] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
	pbf->blength = j;
}

void
_multiply_base_two(pbf, multiplier, carry)
	_big_float     *pbf;
	_BIG_FLOAT_DIGIT multiplier;
	long unsigned   carry;
{
	/*
	 * Multiply a base-2**16 significand by multiplier.  Extend length as
	 * necessary to accommodate carries.
	 */

	short unsigned  length = pbf->blength;
	long unsigned   p;
	int             j;

	for (j = 0; j < length; j++) {
		p = _prodc_b65536(pbf->bsignificand[j], multiplier, carry);
		pbf->bsignificand[j] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
	if (carry != 0) {
		pbf->bsignificand[j++] = (_BIG_FLOAT_DIGIT) (_carry_out_b65536(carry) & 0xffff);
	}
	pbf->blength = j;
}

void
_multiply_base_ten_by_two(pbf, multiplier)
	short unsigned  multiplier;
	_big_float     *pbf;
{
	/*
	 * Multiply a base-10**4 significand by 2**multiplier.  Extend length
	 * as necessary to accommodate carries.
	 */

	short unsigned  length = pbf->blength;
	int             j;
	long unsigned   carry, p;

	carry = 0;
	for (j = 0; j < length; j++) {
		p = _lshift_b10000(pbf->bsignificand[j], multiplier, carry);
		pbf->bsignificand[j] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
	while (carry != 0) {
		p = _carry_out_b10000(carry);
		pbf->bsignificand[j++] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
	pbf->blength = j;
}

void
_unpacked_to_big_float(pu, pb, pe)
	unpacked       *pu;
	_big_float     *pb;
	int            *pe;

{
	/*
	 * Converts pu into a bigfloat *pb of minimal length; exponent *pe
	 * such that pu = *pb * 2 ** *pe
	 */

	int             iz, it;

	for (iz = (UNPACKED_SIZE - 2); pu->significand[iz] == 0; iz--);	/* Find lsw. */

	for (it = 0; it <= iz; it++) {
		pb->bsignificand[2 * (iz - it)] = pu->significand[it] & 0xffff;
		pb->bsignificand[2 * (iz - it) + 1] = pu->significand[it] >> 16;
	}

	pb->blength = 2 * iz + 2;
	if (pb->bsignificand[0] == 0) {
		for (it = 1; it < pb->blength; it++)
			pb->bsignificand[it - 1] = pb->bsignificand[it];
		pb->blength--;
	}
	*pe = pu->exponent + 1 - 16 * pb->blength;
	pb->bexponent = 0;

#ifdef DEBUG
	printf(" unpacked to big float 2**%d * ", *pe);
	_display_big_float(pb, 2);
#endif
}

void
_mul_65536short(pbf, carry)
	_big_float     *pbf;
	unsigned long   carry;
{
	/* *pbf *= 65536 ; += carry ; */

	long unsigned   p;
	int             j;

	for (j = 0; j < pbf->blength; j++) {
		p = _prod_65536_b10000(pbf->bsignificand[j], carry);
		pbf->bsignificand[j] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
	while (carry != 0) {
		p = _carry_out_b10000(carry);
		pbf->bsignificand[j++] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
	pbf->blength = j;
}

void
_big_binary_to_big_decimal(pb, pd)
	_big_float     *pb, *pd;

{
	/* Convert _big_float from binary form to decimal form. */

	int             i;

	pd->bsignificand[0] = pb->bsignificand[pb->blength - 1] % 10000;
	if (pd->bsignificand[0] == pb->bsignificand[pb->blength - 1]) {
		pd->blength = 1;
	} else {
		pd->blength = 2;
		pd->bsignificand[1] = pb->bsignificand[pb->blength - 1] / 10000;
	}
	for (i = pb->blength - 2; i >= 0; i--) {	/* Multiply by 2**16 and
							 * add next significand. */
		_mul_65536short(pd, (unsigned long) pb->bsignificand[i]);
	}
	for (i = 0; i <= (pb->bexponent - 16); i += 16) {	/* Multiply by 2**16 for
								 * each trailing zero. */
		_mul_65536short(pd, (unsigned long) 0);
	}
	if (pb->bexponent > i)
		_left_shift_base_ten(pd, (short unsigned) (pb->bexponent - i));
	pd->bexponent = 0;

#ifdef DEBUG
	printf(" _big_binary_to_big_decimal ");
	_display_big_float(pd, 10);
#endif
}
