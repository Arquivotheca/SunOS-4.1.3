#ifdef sccsid
static char     sccsid[] = "@(#)pack.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* Pack procedures for Sparc FPU simulator. */

#include <machine/fpu/fpu_simulator.h>
#include <machine/fpu/globals.h>

/*
 * Returns 1 if overflow should go to infinity, 0 if to max finite.
 */
PRIVATE int
overflow_to_infinity(pfpsd, sign)
	fp_simd_type	*pfpsd;		/* Pointer to simulator data */
	int		sign;
{
	int		inf;

	switch (pfpsd->fp_direction) {
	case fp_nearest:
		inf = 1;
		break;
	case fp_tozero:
		inf = 0;
		break;
	case fp_positive:
		inf = !sign;
		break;
	case fp_negative:
		inf = sign;
		break;
	}
	return (inf);
}

/*
 * Round according to current rounding mode.
 */
PRIVATE void
round(pfpsd, pu)
	register fp_simd_type	*pfpsd;	/* Pointer to simulator data */
	register unpacked	*pu;
{
	int		increment;	/* boolean to indicate round up */
	int		sr;

	sr = pu->sticky|pu->rounded;

	if (sr == 0)
		return;
	fpu_set_exception(pfpsd, fp_inexact);
	switch (pfpsd->fp_direction) {
	case fp_nearest:
		increment = pu->rounded;
		break;
	case fp_tozero:
		increment = 0;
		break;
	case fp_positive:
		increment = (pu->sign == 0) & (sr != 0);
		break;
	case fp_negative:
		increment = (pu->sign != 0) & (sr != 0);
		break;
	}
	if (increment) {
	    pu->significand[3]++;
	    if (pu->significand[3] == 0) {
		pu->significand[2]++;
		if (pu->significand[2] == 0) {
		    pu->significand[1]++;
		    if (pu->significand[1] == 0) {
			pu->significand[0]++;	/* rounding carried out */
			if (pu->significand[0] == 0x20000) {
			    pu->exponent++;
			    pu->significand[0] = 0x10000;
			}
		    }
		}
	    }
	}
	if ((pfpsd->fp_direction == fp_nearest) &&
	    (pu->sticky == 0) && increment!=0) {	/* ambiguous case */
		pu->significand[3] &= 0xfffffffe; /* force round to even */
	}
}

PRIVATE void
packinteger(pfpsd, pu, px)
	fp_simd_type	*pfpsd;	/* Pointer to simulator data */
	unpacked	*pu;	/* unpacked result */
	int		*px;	/* packed integer */
{
	switch (pu->fpclass) {
	case fp_zero:
		*px = 0;
		break;
	case fp_normal:
		if (pu->exponent >= 32)
			goto overflow;
		fpu_rightshift(pu, 112 - pu->exponent);
		round(pfpsd, pu);
		if (pu->significand[3] >= 0x80000000)
			if ((pu->sign == 0)||(pu->significand[3] > 0x80000000))
				goto overflow;
		*px = pu->significand[3];
		if (pu->sign)
			*px = -*px;
		break;
	case fp_infinity:
	case fp_quiet:
	case fp_signaling:
overflow:
		if (pu->sign)
			*px = 0x80000000;
		else
			*px = 0x7fffffff;
		pfpsd->fp_current_exceptions &= ~(1 << (int) fp_inexact);
		fpu_set_exception(pfpsd, fp_invalid);
		break;
	}
}

PRIVATE void
packsingle(pfpsd, pu, px)
	fp_simd_type	*pfpsd;	/* Pointer to simulator data */
	unpacked	*pu;	/* unpacked result */
	single_type	*px;	/* packed single */
{
	px->sign = pu->sign;
	switch (pu->fpclass) {
	case fp_zero:
		px->exponent = 0;
		px->significand = 0;
		break;
	case fp_infinity:
infinity:
		px->exponent = 0xff;
		px->significand = 0;
		break;
	case fp_quiet:
	case fp_signaling:
		fpu_rightshift(pu, 113-24);
		px->exponent = 0xff;
		px->significand = 0x400000|(0x3fffff&pu->significand[3]);
		break;
	case fp_normal:
		fpu_rightshift(pu, 113-24);
		pu->exponent += SINGLE_BIAS;
		if (pu->exponent <= 0) {
			px->exponent = 0;
			fpu_rightshift(pu, 1 - pu->exponent);
			round(pfpsd, pu);
			if (pu->significand[3] == 0x800000) {	/*
								 * rounded
								 * back up to
								 * normal
								 */
				px->exponent = 1;
				px->significand = 0;
				fpu_set_exception(pfpsd, fp_inexact);
			} else
				px->significand = 0x7fffff & pu->significand[3];

			if (pfpsd->fp_current_exceptions & (1 << fp_inexact))
				fpu_set_exception(pfpsd, fp_underflow);
			if (pfpsd->fp_fsrtem & (1<<fp_underflow)) {
				fpu_set_exception(pfpsd, fp_underflow);
				pfpsd->fp_current_exceptions &=
						~(1 << (int) fp_inexact);
			}
			return;
		}
		round(pfpsd, pu);
		if (pu->significand[3] == 0x1000000) {	/* rounding overflow */
			pu->significand[3] = 0x800000;
			pu->exponent += 1;
		}
		if (pu->exponent >= 0xff) {
			fpu_set_exception(pfpsd, fp_overflow);
			fpu_set_exception(pfpsd, fp_inexact);
			if (pfpsd->fp_fsrtem & (1<<fp_overflow)) {
				pfpsd->fp_current_exceptions &=
						~(1 << (int) fp_inexact);
			}
			if (overflow_to_infinity(pfpsd, pu->sign))
				goto infinity;
			px->exponent = 0xfe;
			px->significand = 0x7fffff;
			return;
		}
		px->exponent = pu->exponent;
		px->significand = 0x7fffff & pu->significand[3];
	}
}

PRIVATE void
packdouble(pfpsd, pu, px, py)
	fp_simd_type	*pfpsd;	/* Pointer to simulator data */
	unpacked	*pu;	/* unpacked result */
	double_type	*px;	/* packed double */
	unsigned	*py;
{
	px->sign = pu->sign;
	switch (pu->fpclass) {
	case fp_zero:
		px->exponent = 0;
		px->significand = 0;
		*py = 0;
		break;
	case fp_infinity:
infinity:
		px->exponent = 0x7ff;
		px->significand = 0;
		*py = 0;
		break;
	case fp_quiet:
	case fp_signaling:
		fpu_rightshift(pu, 113-53);
		px->exponent = 0x7ff;
		px->significand = 0x80000 | (0x7ffff & pu->significand[2]);
		*py = pu->significand[3];
		break;
	case fp_normal:
		fpu_rightshift(pu, 113-53);
		pu->exponent += DOUBLE_BIAS;
		if (pu->exponent <= 0) {	/* underflow */
			px->exponent = 0;
			fpu_rightshift(pu, 1 - pu->exponent);
			round(pfpsd, pu);
			if (pu->significand[2] == 0x100000) {	/*
								 * rounded
								 * back up to
								 * normal
								 */
				px->exponent = 1;
				px->significand = 0;
				*py = 0;
				fpu_set_exception(pfpsd, fp_inexact);
			} else {
				px->exponent = 0;
				px->significand = 0xfffff & pu->significand[2];
				*py = pu->significand[3];
			}
			if (pfpsd->fp_current_exceptions & (1 << fp_inexact))
				fpu_set_exception(pfpsd, fp_underflow);
			if (pfpsd->fp_fsrtem & (1<<fp_underflow)) {
				fpu_set_exception(pfpsd, fp_underflow);
				pfpsd->fp_current_exceptions &=
						~(1 << (int) fp_inexact);
			}
			return;
		}
		round(pfpsd, pu);
		if (pu->significand[2] == 0x200000) {	/* rounding overflow */
			pu->significand[2] = 0x100000;
			pu->exponent += 1;
		}
		if (pu->exponent >= 0x7ff) {	/* overflow */
			fpu_set_exception(pfpsd, fp_overflow);
			fpu_set_exception(pfpsd, fp_inexact);
			if (pfpsd->fp_fsrtem & (1<<fp_overflow)) {
				pfpsd->fp_current_exceptions &=
						~(1 << (int) fp_inexact);
			}
			if (overflow_to_infinity(pfpsd, pu->sign))
				goto infinity;
			px->exponent = 0x7fe;
			px->significand = 0xfffff;
			*py = 0xffffffff;
			return;
		}
		px->exponent = pu->exponent;
		px->significand = 0xfffff & pu->significand[2];
		*py = pu->significand[3];
		break;
	}
}

PRIVATE void
packextended(pfpsd, pu, px, py, pz, pw)
	fp_simd_type	*pfpsd;	/* Pointer to simulator data */
	unpacked	*pu;	/* unpacked result */
	extended_type	*px;	/* packed extended */
	unsigned	*py, *pz, *pw;
{
	px->sign = pu->sign;
	switch (pu->fpclass) {
	case fp_zero:
		px->exponent = 0;
		px->significand = 0;
		*pz = 0;
		*py = 0;
		*pw = 0;
		break;
	case fp_infinity:
infinity:
		px->exponent = 0x7fff;
		px->significand = 0;
		*pz = 0;
		*py = 0;
		*pw = 0;
		break;
	case fp_quiet:
	case fp_signaling:
		px->exponent = 0x7fff;
		px->significand = 0x8000 | pu->significand[0];	/*
								 * Insure quiet
								 * nan.
								 */
		*py = pu->significand[1];
		*pz = pu->significand[2];
		*pw = pu->significand[3];
		break;
	case fp_normal:
		pu->exponent += EXTENDED_BIAS;
		if (pu->exponent <= 0) {	/* underflow */
			fpu_rightshift(pu, 1-pu->exponent);
			round(pfpsd, pu);
			if (pu->significand[0] < 0x00010000) {	/*
								 * not rounded
								 * back up
								 * to normal
								 */
				px->exponent = 0;
			} else {
				px->exponent = 1;
				fpu_set_exception(pfpsd, fp_inexact);
			}
			if (pfpsd->fp_current_exceptions & (1 << fp_inexact))
				fpu_set_exception(pfpsd, fp_underflow);
			if (pfpsd->fp_fsrtem & (1<<fp_underflow)) {
				fpu_set_exception(pfpsd, fp_underflow);
				pfpsd->fp_current_exceptions &=
						~(1 << (int) fp_inexact);
			}
			px->significand = pu->significand[0];
			*py = pu->significand[1];
			*pz = pu->significand[2];
			*pw = pu->significand[3];
			return;
		}
		round(pfpsd, pu); /* rounding overflow handled in round() */
		if (pu->exponent >= 0x7fff) {	/* overflow */
			fpu_set_exception(pfpsd, fp_overflow);
			fpu_set_exception(pfpsd, fp_inexact);
			if (pfpsd->fp_fsrtem & (1<<fp_overflow)) {
				pfpsd->fp_current_exceptions &=
						~(1 << (int) fp_inexact);
			}
			if (overflow_to_infinity(pfpsd, pu->sign))
				goto infinity;
			px->exponent = 0x7ffe;	/* overflow to max norm */
			px->significand = 0xffff;
			*py = 0xffffffff;
			*pz = 0xffffffff;
			*pw = 0xffffffff;
			return;
		}
		px->exponent = pu->exponent;
		px->significand = pu->significand[0];
		*py = pu->significand[1];
		*pz = pu->significand[2];
		*pw = pu->significand[3];
		break;
	}
}

void
_fp_pack(pfpsd, pu, n, type)
	fp_simd_type	*pfpsd;	/* Pointer to simulator data */
	unpacked	*pu;	/* unpacked operand */
	unsigned	n;	/* register where datum starts */
	enum fp_op_type type;	/* type of datum */

{
	switch (type) {
	case fp_op_integer:
		{
			int		x;
			packinteger(pfpsd, pu, &x);
			if (!(pfpsd->fp_current_exceptions & pfpsd->fp_fsrtem))
				pfpsd->fp_current_write_freg(&x, n, pfpsd);
			break;
		}
	case fp_op_single:
		{
			single_type	x;
			packsingle(pfpsd, pu, &x);
			if (!(pfpsd->fp_current_exceptions & pfpsd->fp_fsrtem))
				pfpsd->fp_current_write_freg(&x, n, pfpsd);
			break;
		}
	case fp_op_double:
		{
			double_type	x;
			unsigned	y;
			packdouble(pfpsd, pu, &x, &y);
			if (!(pfpsd->fp_current_exceptions &
			    pfpsd->fp_fsrtem)) {
				pfpsd->fp_current_write_freg(&x, DOUBLE_E(n),
								pfpsd);
				pfpsd->fp_current_write_freg(&y, DOUBLE_F(n),
								pfpsd);
			}
			break;
		}
	case fp_op_extended:
		{
			extended_type	x;
			unsigned	y, z, w;
			unpacked	U;
			int		k;
			switch (pfpsd->fp_precision) {	/*
							 * Implement extended
							 * rounding precision
							 * mode.
							 */
			case fp_single:
				{
					single_type	tx;
					packsingle(pfpsd, pu, &tx);
					pu = &U;
					unpacksingle(pfpsd, pu, tx);
					break;
				}
			case fp_double:
				{
					double_type	tx;
					unsigned	ty;
					packdouble(pfpsd, pu, &tx, &ty);
					pu = &U;
					unpackdouble(pfpsd, pu, tx, ty);
					break;
				}
			case fp_precision_3:	/* rounded to 64 bits */
				{
					k = pu->exponent + EXTENDED_BIAS;
					if (k>=0) k = 113-64;
					else	k = 113-64-k;
					fpu_rightshift(pu, 113-64);
					round(pfpsd, pu);
					pu->sticky=pu->rounded=0;
					pu->exponent += k;
					fpu_normalize(pu);
					break;
				}
			}
			packextended(pfpsd, pu, &x, &y, &z, &w);
			if (!(pfpsd->fp_current_exceptions &
			    pfpsd->fp_fsrtem)) {
				pfpsd->fp_current_write_freg(&x, EXTENDED_E(n),
								pfpsd);
				pfpsd->fp_current_write_freg(&y, EXTENDED_F(n),
								pfpsd);
				pfpsd->fp_current_write_freg(&z, EXTENDED_G(n),
								pfpsd);
				pfpsd->fp_current_write_freg(&w, EXTENDED_H(n),
								pfpsd);
			}

			break;
		}
	}
}

void
_fp_pack_word(pfpsd, pu, n)
	fp_simd_type	*pfpsd;	/* Pointer to simulator data */
	unsigned	*pu;	/* unpacked operand */
	unsigned	n;	/* register where datum starts */
{
	pfpsd->fp_current_write_freg(pu, n, pfpsd);
}
