#ifdef sccsid
static char     sccsid[] = "@(#)addsub.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <machine/fpu/fpu_simulator.h>
#include <machine/fpu/globals.h>

/*ARGSUSED*/
PRIVATE void
true_add(pfpsd, px, py, pz)
	fp_simd_type	*pfpsd;
	unpacked	*px, *py, *pz;
{
	unsigned 	c;
	unpacked	*pt;

	if ((int) px->fpclass < (int) py->fpclass) {	/* Reverse. */
		pt = py;
		py = px;
		px = pt;
	}
	/* Now class(x) >= class(y). */
	switch (px->fpclass) {
	case fp_quiet:		/* NaN + x -> NaN */
	case fp_signaling:	/* NaN + x -> NaN */
	case fp_infinity:	/* Inf + x -> Inf */
	case fp_zero:		/* 0 + 0 -> 0 */
		*pz = *px;
		return;
	default:
		if (py->fpclass == fp_zero) {
			*pz = *px;
			return;
		}
	}
	/* Now z is normal or subnormal. */
	/* Now y is normal or subnormal. */
	if (px->exponent < py->exponent) {	/* Reverse. */
		pt = py;
		py = px;
		px = pt;
	}
	/* Now class(x) >= class(y). */
	pz->fpclass = px->fpclass;
	pz->sign = px->sign;
	pz->exponent = px->exponent;
	pz->rounded = pz->sticky  = 0;

	if (px->exponent != py->exponent) {	/* pre-alignment required */
		fpu_rightshift(py, pz->exponent - py->exponent);
		pz->rounded = py->rounded;
		pz->sticky  = py->sticky;
	}
	c = 0;
	c = fpu_add3wc(&(pz->significand[3]), px->significand[3],
						py->significand[3], c);
	c = fpu_add3wc(&(pz->significand[2]), px->significand[2],
						py->significand[2], c);
	c = fpu_add3wc(&(pz->significand[1]), px->significand[1],
						py->significand[1], c);
	c = fpu_add3wc(&(pz->significand[0]), px->significand[0],
						py->significand[0], c);

	/* Handle carry out of msb. */
	if (pz->significand[0]>=0x20000) {
		fpu_rightshift(pz, 1);	/* Carried out bit. */
		pz->exponent++;		/* Renormalize. */
	}
	return;
}

PRIVATE void
true_sub(pfpsd, px, py, pz)
	fp_simd_type	*pfpsd;		/* Pointer to simulator data */
	unpacked	*px, *py, *pz;
{
	unsigned	*z, g, s, r, c;
	unpacked	*pt;

	if ((int) px->fpclass < (int) py->fpclass) {	/* Reverse. */
		pt = py;
		py = px;
		px = pt;
	}
	/* Now class(x) >= class(y). */
	*pz = *px;		/* Tentative difference: x. */
	switch (pz->fpclass) {
	case fp_quiet:		/* NaN - x -> NaN */
	case fp_signaling:	/* NaN - x -> NaN */
		return;
	case fp_infinity:	/* Inf - x -> Inf */
		if (py->fpclass == fp_infinity) {
			fpu_error_nan(pfpsd, pz);	/* Inf - Inf -> NaN */
			pz->fpclass = fp_quiet;
		}
		return;
	case fp_zero:		/* 0 - 0 -> 0 */
		pz->sign = (pfpsd->fp_direction == fp_negative);
		return;
	default:
		if (py->fpclass == fp_zero)
			return;
	}

	/* x and y are both normal or subnormal. */

	if (px->exponent < py->exponent) { /* Reverse. */
		pt = py;
		py = px;
		px = pt;
	}
	/* Now exp(x) >= exp(y). */
	pz->fpclass = px->fpclass;
	pz->sign = px->sign;
	pz->exponent = px->exponent;
	pz->rounded = 0;
	pz->sticky = 0;
	z = pz->significand;

	if (px->exponent == py->exponent) {	/* no pre-alignment required */
		c = 0;
		c = fpu_sub3wc(&z[3], px->significand[3],
				py->significand[3], c);
		c = fpu_sub3wc(&z[2], px->significand[2],
				py->significand[2], c);
		c = fpu_sub3wc(&z[1], px->significand[1],
				py->significand[1], c);
		c = fpu_sub3wc(&z[0], px->significand[0],
				py->significand[0], c);
		if ((z[0]|z[1]|z[2]|z[3])==0) {		/* exact zero result */
			pz->sign = (pfpsd->fp_direction == fp_negative);
			pz->fpclass = fp_zero;
			return;
		}
		if (z[0]>=0x20000) {	/* sign reversal occurred */
			pz->sign = py->sign;
			c = 0;
			c = fpu_neg2wc(&z[3], z[3], c);
			c = fpu_neg2wc(&z[2], z[2], c);
			c = fpu_neg2wc(&z[1], z[1], c);
			c = fpu_neg2wc(&z[0], z[0], c);
		}
		fpu_normalize(pz);
		return;
	} else {		/* pre-alignment required */
		fpu_rightshift(py, pz->exponent - py->exponent - 1);
		r = py->rounded; 	/* rounded bit */
		s = py->sticky;		/* sticky bit */
		fpu_rightshift(py, 1);
		g = py->rounded;	/* guard bit */
		if (s!=0) r = (r==0);
		if ((r|s)!=0) g = (g==0); /* guard and rounded bits of z */
		c = ((g|r|s)!=0);
		c = fpu_sub3wc(&z[3], px->significand[3],
				py->significand[3], c);
		c = fpu_sub3wc(&z[2], px->significand[2],
				py->significand[2], c);
		c = fpu_sub3wc(&z[1], px->significand[1],
				py->significand[1], c);
		c = fpu_sub3wc(&z[0], px->significand[0],
				py->significand[0], c);

		if (z[0]>=0x10000) { 	/* don't need post-shifted */
			pz->sticky = s|r;
			pz->rounded = g;
		} else {		/* post-shifted left 1 bit */
			pz->sticky = s;
			pz->rounded = r;
			pz->significand[0] = (z[0]<<1)|((z[1]&0x80000000)>>31);
			pz->significand[1] = (z[1]<<1)|((z[2]&0x80000000)>>31);
			pz->significand[2] = (z[2]<<1)|((z[3]&0x80000000)>>31);
			pz->significand[3] = (z[3]<<1)|g;
			pz->exponent -= 1;
			if (z[0]<0x10000) fpu_normalize(pz);
		}
		return;
	}
}

void
_fp_add(pfpsd, px, py, pz)
	fp_simd_type	*pfpsd;
	unpacked	*px, *py, *pz;
{
	if (px->sign == py->sign)
		true_add(pfpsd, px, py, pz);
	else
		true_sub(pfpsd, px, py, pz);
}

void
_fp_sub(pfpsd, px, py, pz)
	fp_simd_type	*pfpsd;
	unpacked	*px, *py, *pz;
{
	if ((int)py->fpclass < (int)fp_quiet) py->sign= 1- py->sign;
	if (px->sign == py->sign)
		true_add(pfpsd, px, py, pz);
	else
		true_sub(pfpsd, px, py, pz);
}
