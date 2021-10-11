#ifdef sccsid
static char     sccsid[] = "@(#)unpack.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */

/* Unpack procedures for Sparc FPU simulator. */

#include <machine/fpu/fpu_simulator.h>
#include <machine/fpu/globals.h>

PRIVATE void
unpackinteger(pu, x)
	unpacked	*pu;	/* unpacked result */
	int		x;	/* packed integer */
{
	unsigned ux;
	pu->sticky = pu->rounded = 0;
	if (x == 0) {
		pu->sign = 0;
		pu->fpclass = fp_zero;
	} else {
		(*pu).sign = x < 0;
		(*pu).fpclass = fp_normal;
		(*pu).exponent = INTEGER_BIAS;
		if (x<0) ux = -x; else ux = x;
		(*pu).significand[0] = ux>>15;
		(*pu).significand[1] = (ux&0x7fff)<<17;
		(*pu).significand[2] = 0;
		(*pu).significand[3] = 0;
		fpu_normalize(pu);
	}
}

void
unpacksingle(pfpsd, pu, x)
	fp_simd_type	*pfpsd;	/* simulator data */
	unpacked	*pu;	/* unpacked result */
	single_type	x;	/* packed single */
{
	unsigned U;
	pu->sticky = pu->rounded = 0;
	U = x.significand;
	(*pu).sign = x.sign;
	pu->significand[1] = 0;
	pu->significand[2] = 0;
	pu->significand[3] = 0;
	if (x.exponent == 0) {				/* zero or sub */
		if (x.significand == 0) {		/* zero */
			pu->fpclass = fp_zero;
			return;
		} else {				/* subnormal */
			pu->fpclass = fp_normal;
			pu->exponent = -SINGLE_BIAS-6;
			pu->significand[0]=U;
			fpu_normalize(pu);
			return;
		}
	} else if (x.exponent == 0xff) {		/* inf or nan */
		if (x.significand == 0) {		/* inf */
			pu->fpclass = fp_infinity;
			return;
		} else {				/* nan */
			if ((U & 0x400000) != 0) {	/* quiet */
				pu->fpclass = fp_quiet;
			} else {			/* signaling */
				pu->fpclass = fp_signaling;
				fpu_set_exception(pfpsd, fp_invalid);
			}
			pu->significand[0] = 0x18000 | (U >> 7);
			(*pu).significand[1]=((U&0x7f)<<25);
			return;
		}
	}
	(*pu).exponent = x.exponent - SINGLE_BIAS;
	(*pu).fpclass = fp_normal;
	(*pu).significand[0]=0x10000|(U>>7);
	(*pu).significand[1]=((U&0x7f)<<25);
}

void
unpackdouble(pfpsd, pu, x, y)
	fp_simd_type	*pfpsd;	/* simulator data */
	unpacked	*pu;	/* unpacked result */
	double_type	x;	/* packed double */
	unsigned	y;
{
	unsigned U;
	pu->sticky = pu->rounded = 0;
	U = x.significand;
	(*pu).sign = x.sign;
	pu->significand[1] = y;
	pu->significand[2] = 0;
	pu->significand[3] = 0;
	if (x.exponent == 0) {				/* zero or sub */
		if ((x.significand == 0) && (y == 0)) {	/* zero */
			pu->fpclass = fp_zero;
			return;
		} else {				/* subnormal */
			pu->fpclass = fp_normal;
			pu->exponent = -DOUBLE_BIAS-3;
			pu->significand[0] = U;
			fpu_normalize(pu);
			return;
		}
	} else if (x.exponent == 0x7ff) {		/* inf or nan */
		if ((U|y) == 0) {			/* inf */
			pu->fpclass = fp_infinity;
			return;
		} else {				/* nan */
			if ((U & 0x80000) != 0) {	/* quiet */
				pu->fpclass = fp_quiet;
			} else {			/* signaling */
				pu->fpclass = fp_signaling;
				fpu_set_exception(pfpsd, fp_invalid);
			}
			pu->significand[0] = 0x18000 | (U >> 4);
			(*pu).significand[1]=((U&0xf)<<28)|(y>>4);
			(*pu).significand[2]=((y&0xf)<<28);
			return;
		}
	}
	(*pu).exponent = x.exponent - DOUBLE_BIAS;
	(*pu).fpclass = fp_normal;
	(*pu).significand[0]=0x10000|(U>>4);
	(*pu).significand[1]=((U&0xf)<<28)|(y>>4);
	(*pu).significand[2]=((y&0xf)<<28);
}

PRIVATE void
unpackextended(pfpsd, pu, x, y, z, w)
	fp_simd_type	*pfpsd;	/* simulator data */
	unpacked	*pu;	/* unpacked result */
	extended_type	x;	/* packed extended */
	unsigned	y, z, w;
{
	unsigned U;
	pu->sticky = pu->rounded = 0;
	U = x.significand;
	(*pu).sign = x.sign;
	(*pu).fpclass = fp_normal;
	(*pu).exponent = x.exponent - EXTENDED_BIAS;
	(*pu).significand[0] = (x.exponent==0)? U:0x10000|U;
	(*pu).significand[1] = y;
	(*pu).significand[2] = z;
	(*pu).significand[3] = w;
	if (x.exponent < 0x7fff) {	/* zero, normal, or subnormal */
		if ((z|y|w|pu->significand[0]) == 0) {	/* zero */
			pu->fpclass = fp_zero;
			return;
		} else {			/* normal or subnormal */
			if (x.exponent==0) {
				fpu_normalize(pu);
				pu->exponent += 1;
			}
			return;
		}
	} else {					/* inf or nan */
		if ((U|z|y|w) == 0) {			/* inf */
			pu->fpclass = fp_infinity;
			return;
		} else {				/* nan */
			if ((U & 0x00008000) != 0) {	/* quiet */
				pu->fpclass = fp_quiet;
			} else {			/* signaling */
				pu->fpclass = fp_signaling;
				fpu_set_exception(pfpsd, fp_invalid);
			}
			pu->significand[0] |= 0x8000;	/* make quiet */
			return;
		}
	}
}

void
_fp_unpack(pfpsd, pu, n, dtype)
	fp_simd_type	*pfpsd;	/* simulator data */
	unpacked	*pu;	/* unpacked result */
	unsigned	n;	/* register where data starts */
	enum fp_op_type	dtype;	/* type of datum */
{
	freg_type	f, fy, fz, fw;

	switch ((int) dtype) {
	case fp_op_integer:
		pfpsd->fp_current_read_freg(&f, n, pfpsd);
		unpackinteger(pu, f.int_reg);
		break;
	case fp_op_single:
		pfpsd->fp_current_read_freg(&f, n, pfpsd);
		unpacksingle(pfpsd, pu, f.single_reg);
		break;
	case fp_op_double:
		pfpsd->fp_current_read_freg(&f, DOUBLE_E(n), pfpsd);
		pfpsd->fp_current_read_freg(&fy, DOUBLE_F(n), pfpsd);
		unpackdouble(pfpsd, pu, f.double_reg, fy.unsigned_reg);
		break;
	case fp_op_extended:
		pfpsd->fp_current_read_freg(&f,  EXTENDED_E(n), pfpsd);
		pfpsd->fp_current_read_freg(&fy, EXTENDED_F(n), pfpsd);
		pfpsd->fp_current_read_freg(&fz, EXTENDED_G(n), pfpsd);
		pfpsd->fp_current_read_freg(&fw, EXTENDED_H(n), pfpsd);
		unpackextended(pfpsd, pu, f.extended_reg, fy.unsigned_reg,
			fz.unsigned_reg, fw.unsigned_reg);
		break;
	}
}

void
_fp_unpack_word(pfpsd, pu, n)
	fp_simd_type	*pfpsd;	/* simulator data */
	unsigned	*pu;	/* unpacked result */
	unsigned	n;	/* register where data starts */
{
	pfpsd->fp_current_read_freg(pu, n, pfpsd);
}
