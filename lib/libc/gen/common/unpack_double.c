#if !defined(lint) && defined(SCCSIDS)
static char     sccsid[] = "@(#)unpack_double.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc. 
 */

#include "sunfloatingpoint.h"

void
_fp_rightshift(pu, n)
	unpacked       *pu;
	int             n;

/* Right shift significand sticky by n bits.  */

{
	if (n >= 66) {		/* drastic */
		if (((*pu).significand[0] | (*pu).significand[1] | (*pu).significand[2]) == 0) {	/* really zero */
			pu->fpclass = fp_zero;
			return;
		} else {
			pu->significand[2] = 0x20000000;
			pu->significand[1] = 0;
			pu->significand[0] = 0;
			return;
		}
	}
	while (n >= 32) {	/* big shift */
		if (pu->significand[2] != 0)
			pu->significand[1] |= 1;
		(*pu).significand[2] = (*pu).significand[1];
		(*pu).significand[1] = (*pu).significand[0];
		(*pu).significand[0] = 0;
		n -= 32;
	}
	while (n > 0) {		/* small shift */
		if (pu->significand[2] & 1)
			pu->significand[2] |= 2;
		pu->significand[2] = pu->significand[2] >> 1;
		if (pu->significand[1] & 1)
			pu->significand[2] |= 0x80000000;
		pu->significand[1] = pu->significand[1] >> 1;
		if (pu->significand[0] & 1)
			pu->significand[1] |= 0x80000000;
		pu->significand[0] = pu->significand[0] >> 1;
		n -= 1;
	}
}

void
_fp_normalize(pu)
	unpacked       *pu;

/* Normalize a number.  Does not affect zeros, infs, or NaNs. */

{
	if ((*pu).fpclass == fp_normal) {
		if (((*pu).significand[0] | (*pu).significand[1] | (*pu).significand[2]) == 0) {	/* true zero */
			(*pu).fpclass = fp_zero;
			return;
		}
		while ((*pu).significand[0] == 0) {
			(*pu).significand[0] = (*pu).significand[1];
			(*pu).significand[1] = (*pu).significand[2];
			(*pu).significand[2] = 0;
			(*pu).exponent = (*pu).exponent - 32;
		}
		while ((*pu).significand[0] < 0x80000000) {
			pu->significand[0] = pu->significand[0] << 1;
			if (pu->significand[1] >= 0x80000000)
				pu->significand[0] += 1;
			pu->significand[1] = pu->significand[1] << 1;
			if (pu->significand[2] >= 0x80000000)
				pu->significand[1] += 1;
			pu->significand[2] = pu->significand[2] << 1;
			(*pu).exponent = (*pu).exponent - 1;
		}
	}
}

void
_fp_set_exception(ex)
	enum fp_exception_type ex;

/* Set the exception bit in the current exception register. */

{
	_fp_current_exceptions |= 1 << (int) ex;
}

enum fp_class_type 
class_double(x)
	double         *x;
{
	double_equivalence kluge;

	kluge.x = *x;
	if (kluge.f.msw.exponent == 0) {	/* 0 or sub */
		if ((kluge.f.msw.significand == 0) && (kluge.f.significand2 == 0))
			return fp_zero;
		else
			return fp_subnormal;
	} else if (kluge.f.msw.exponent == 0x7ff) {	/* inf or nan */
		if ((kluge.f.msw.significand == 0) && (kluge.f.significand2 == 0))
			return fp_infinity;
		else if (kluge.f.msw.significand >= 0x40000)
			return fp_quiet;
		else
			return fp_signaling;
	} else
		return fp_normal;
}

void
_fp_leftshift(pu, n)
	unpacked       *pu;
	unsigned        n;

/* Left shift significand by n bits.  Affect all classes.	 */

{
	while (n >= 32) {	/* big shift */
		(*pu).significand[0] = (*pu).significand[1];
		(*pu).significand[1] = (*pu).significand[2];
		(*pu).significand[2] = 0;
		n -= 32;
	}
	while (n > 0) {		/* small shift */
		pu->significand[0] = pu->significand[0] << 1;
		if (pu->significand[1] & 0x80000000)
			pu->significand[0] |= 1;
		pu->significand[1] = pu->significand[1] << 1;
		if (pu->significand[2] & 0x80000000)
			pu->significand[1] |= 1;
		pu->significand[2] = pu->significand[2] << 1;
		n -= 1;
	}
}

void
_unpack_double(pu, px)
	unpacked       *pu;	/* unpacked result */
	double         *px;	/* packed double */
{
	double_equivalence x;

	x.x = *px;
	(*pu).sign = x.f.msw.sign;
	pu->significand[1] = x.f.significand2;
	pu->significand[2] = 0;
	if (x.f.msw.exponent == 0) {	/* zero or sub */
		if ((x.f.msw.significand == 0) && (x.f.significand2 == 0)) {	/* zero */
			pu->fpclass = fp_zero;
			return;
		} else {	/* subnormal */
			pu->fpclass = fp_normal;
			pu->exponent = 12 - DOUBLE_BIAS;
			pu->significand[0] = x.f.msw.significand;
			_fp_normalize(pu);
			return;
		}
	} else if (x.f.msw.exponent == 0x7ff) {	/* inf or nan */
		if ((x.f.msw.significand == 0) && (x.f.significand2 == 0)) {	/* inf */
			pu->fpclass = fp_infinity;
			return;
		} else {	/* nan */
			if ((x.f.msw.significand & 0x80000) != 0) {	/* quiet */
				pu->fpclass = fp_quiet;
			} else {/* signaling */
				pu->fpclass = fp_quiet;
				_fp_set_exception(fp_invalid);
			}
			pu->significand[0] = 0x80000 | x.f.msw.significand;
			_fp_leftshift(pu, 11);
			return;
		}
	}
	(*pu).exponent = x.f.msw.exponent - DOUBLE_BIAS;
	(*pu).fpclass = fp_normal;
	(*pu).significand[0] = 0x100000 | x.f.msw.significand;
	_fp_leftshift(pu, 11);
}
