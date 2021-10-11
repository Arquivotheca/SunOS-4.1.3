#ifdef sccsid
static char     sccsid[] = "@(#)compare.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */

#include <machine/fpu/fpu_simulator.h>
#include <machine/fpu/globals.h>

enum fcc_type
_fp_compare(pfpsd, px, py, strict)
	fp_simd_type	*pfpsd;	/* simulator data */
	unpacked	*px, *py;
	int		strict;	/*
				 * 0 if quiet NaN unexceptional, 1 if
				 * exceptional
				 */
{
	enum fcc_type   cc;
	int  n;

	if ((px->fpclass == fp_quiet) || (py->fpclass == fp_quiet) ||
	    (px->fpclass == fp_signaling) || (py->fpclass == fp_signaling)) {
		if (strict)				/* NaN */
			fpu_set_exception(pfpsd, fp_invalid);
		cc = fcc_unordered;
	} else if ((px->fpclass == fp_zero) && (py->fpclass == fp_zero))
		cc = fcc_equal;
	/* both zeros */
	else if (px->sign < py->sign)
		cc = fcc_greater;
	else if (px->sign > py->sign)
		cc = fcc_less;
	else {			/* signs the same, compute magnitude cc */
		if ((int) px->fpclass > (int) py->fpclass)
			cc = fcc_greater;
		else if ((int) px->fpclass < (int) py->fpclass)
			cc = fcc_less;
		else
		/* same classes */ if (px->fpclass == fp_infinity)
			cc = fcc_equal;	/* same infinity */
		else if (px->exponent > py->exponent)
			cc = fcc_greater;
		else if (px->exponent < py->exponent)
			cc = fcc_less;
		else {	/* equal exponents */
			n = fpu_cmpli(px->significand, py->significand, 4);
			if (n>0) cc = fcc_greater;
			else if (n<0) cc = fcc_less;
			else cc = fcc_equal;
		}
		if (px->sign)
			switch (cc) {	/* negative numbers */
			case fcc_less:
				cc = fcc_greater;
				break;
			case fcc_greater:
				cc = fcc_less;
				break;
			}
	}
	return (cc);
}
