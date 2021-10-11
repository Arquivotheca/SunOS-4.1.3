#ifdef sccsid
static char     sccsid[] = "@(#)utility.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* Utility functions for Sparc FPU simulator. */

#include <machine/fpu/fpu_simulator.h>
#include <machine/fpu/globals.h>

void
_fp_read_vfreg(pf, n, pfpsd)
	FPU_REGS_TYPE	*pf;	/* Old freg value. */
	unsigned	n;	/* Want to read register n. */
	fp_simd_type	*pfpsd;
{
	*pf = pfpsd->fp_current_pfregs->fpu_regs[n];
}

void
_fp_write_vfreg(pf, n, pfpsd)
	FPU_REGS_TYPE	*pf;	/* New freg value. */
	unsigned	n;	/* Want to read register n. */
	fp_simd_type	*pfpsd;
{
	pfpsd->fp_current_pfregs->fpu_regs[n] = *pf;
}

/*
 * Normalize a number.  Does not affect zeros, infs, or NaNs.
 * The number will be normalized to 113 bit extended:
 *		0x0001####, 0x########, 0x########, 0x########.
 */
void
fpu_normalize(pu)
	unpacked	*pu;
{
	unsigned U, u0, u1, u2, u3, m, n, k;
	u0 = pu->significand[0];
	u1 = pu->significand[1];
	u2 = pu->significand[2];
	u3 = pu->significand[3];
	if ((*pu).fpclass == fp_normal) {
		if ((u0|u1|u2|u3)==0) {
			(*pu).fpclass = fp_zero;
			return;
		}
		while (u0 == 0) {
			u0 = u1; u1=u2; u2=u3; u3=0;
			(*pu).exponent = (*pu).exponent - 32;
		}
		if (u0 >= 0x20000) { 	/* u3 should be zero */
			n=1; U = u0 >> 1;
			while (U >= 0x20000) {
				U >>= 1;
				n += 1;
			}
			m = (1 << n)-1;
			k = 32-n;
			(*pu).exponent += n;
			u3 = ((u2&m)<<k)|(u3>>n);
			u2 = ((u1&m)<<k)|(u2>>n);
			u1 = ((u0&m)<<k)|(u1>>n);
			u0 = U;
		} else if (u0 < 0x10000) {
			n=1; U = u0<<1;
			while (U < 0x10000) {
				U <<= 1;
				n += 1;
			}
			k = 32-n;
			m = -(1 << k);
			(*pu).exponent -= n;
			u0 = (u0<<n)|((u1&m)>>k);
			u1 = (u1<<n)|((u2&m)>>k);
			u2 = (u2<<n)|((u3&m)>>k);
			u3 = (u3<<n);
		}
		pu->significand[0] = u0;
		pu->significand[1] = u1;
		pu->significand[2] = u2;
		pu->significand[3] = u3;
	}
}

/*
 * Right shift significand sticky by n bits.
 */
void
fpu_rightshift(pu, n)
	unpacked	*pu;
	int		n;
{
	unsigned m, k, j, u0, u1, u2, u3;
	if (n > 113) {		/* drastic */
		if (((*pu).significand[0] | (*pu).significand[1]
			| (*pu).significand[2] | (*pu).significand[3]) == 0){
						/* really zero */
			pu->fpclass = fp_zero;
			return;
		} else {
			pu->rounded = 0;
			pu->sticky  = 1;
			pu->significand[3] = 0;
			pu->significand[2] = 0;
			pu->significand[1] = 0;
			pu->significand[0] = 0;
			return;
		}
	}
	while (n >= 32) {	/* big shift */
		pu->sticky  |= pu->rounded | (pu->significand[3]&0x7fffffff);
		pu->rounded  = (*pu).significand[3] >> 31;
		(*pu).significand[3] = (*pu).significand[2];
		(*pu).significand[2] = (*pu).significand[1];
		(*pu).significand[1] = (*pu).significand[0];
		(*pu).significand[0] = 0;
		n -= 32;
	}
	if (n > 0) {		/* small shift */
		u0 = pu->significand[0];
		u1 = pu->significand[1];
		u2 = pu->significand[2];
		u3 = pu->significand[3];
		m = (1<<n)-1;
		k = 32 - n;
		j = (1<<(n-1))-1;
		pu->sticky |= pu->rounded | (u3&j);
		pu->rounded = (u3&m)>>(n-1);
		pu->significand[3] = ((u2&m)<<k)|(u3>>n);
		pu->significand[2] = ((u1&m)<<k)|(u2>>n);
		pu->significand[1] = ((u0&m)<<k)|(u1>>n);
		pu->significand[0] = u0>>n;
	}
}


/*
 * Set the exception bit in the current exception register.
 */
void
fpu_set_exception(pfpsd, ex)
	fp_simd_type	*pfpsd;	/* Pointer to simulator data */
	enum fp_exception_type ex;
{
	pfpsd->fp_current_exceptions |= 1 << (int) ex;
}

/*
 * Set invalid exception and error nan in *pu
 */
void
fpu_error_nan(pfpsd, pu)
	fp_simd_type	*pfpsd;	/* Pointer to simulator data */
	unpacked	*pu;
{
	fpu_set_exception(pfpsd, fp_invalid);
	pu->sign = 0;
	pu->significand[0] = 0x7fffffff;
	pu->significand[1] = 0xffffffff;
	pu->significand[2] = 0xffffffff;
	pu->significand[3] = 0xffffffff;
}

/*
 * the following fpu_add3wc should be inlined as
 *	.inline	_fpu_add3wc, 3
 *	ld	[%o1], %o4		! sum = x
 *	addcc	-1, %o3, %g0		! restore last carry in cc reg
 *	addxcc	%o4, %o2, %o4		! sum = sum + y + last carry
 *	st	%o4, [%o0]		! *z  = sum
 *	addx	%g0, %g0, %o0		! return new carry
 *	.end
 */

#ifndef lint
unsigned
#endif !lint
fpu_add3wc(z, x, y, carry)
	unsigned *z, x, y, carry;
{				/*  *z = x + y + carry, set carry; */
	if (carry==0) {
		*z = x+y;
		return (*z < y);
	} else {
		*z = x+y+1;
		return (*z <= y);
	}
}

/* the following fpu_sub3wc should be inlined as
 *	.inline	_fpu_sub3wc, 3
 *	ld	[%o1], %o4		! sum = *x
 *	addcc	-1, %o3, %g0		! restore last carry in cc reg
 *	subxcc	%o4, %o2, %o4		! sum = sum - y - last carry
 *	st	%o4, [%o0]		! *x  = sum
 *	addx	%g0, %g0, %o0		! return new carry
 *	.end
 */
#ifndef lint
unsigned
#endif !lint

fpu_sub3wc(z, x, y, carry)
	unsigned *z, x, y, carry;
{				/*  *z = x - y - carry, set carry; */
	if (carry==0) {
		*z = x-y;
		return (*z > x);
	} else {
		*z = x-y-1;
		return (*z >= x);
	}
}

/*
 * the following fpu_neg2wc should be inlined as
 *	.inline	_fpu_neg2wc, 2
 *	ld	[%o1], %o3		! tmp = *x
 *	addcc	-1, %o2, %g0		! restore last carry in cc reg
 *	subxcc	%g0, %o3, %o3		! sum = 0 - tmp - last carry
 *	st	%o3, [%o0]		! *x  = sum
 *	addx	%g0, %g0, %o0		! return new carry
 *	.end
 */

#ifndef lint
unsigned
#endif !lint

fpu_neg2wc(z, x, carry)
	unsigned *z, x, carry;
{				/*  *x = 0 - *x - carry, set carry; */
	if (carry==0) {
		*z = -x;
		return ((*z) != 0);
	} else {
		*z = -x-1;
		return (1);
	}
}

int
fpu_cmpli(x, y, n)
	unsigned x[], y[]; int n;
{				/* compare two unsigned array */
	int i;
	i=0;
	while (i < n) {
		if (x[i] > y[i]) return 1;
		else if (x[i] < y[i]) return -1;
		i++;
	}
	return (0);
}

#ifdef DEBUG
/*
 * Print out unpacked record.
 */
void
display_unpacked(pu)
	unpacked	*pu;
{
	(void) printf(" unpacked ");
	if (pu->sign)
		(void) printf("-");
	else
		(void) printf("+");

	switch (pu->fpclass) {
	case fp_zero:
		(void) printf("0     ");
		break;
	case fp_normal:
		(void) printf("normal");
		break;
	case fp_infinity:
		(void) printf("Inf   ");
		break;
	case fp_quiet:
	case fp_signaling:
		(void) printf("nan   ");
		break;
	}
	(void) printf(" %X %X %X %X (%X, %X) exponent %X \n",
		pu->significand[0], pu->significand[1], pu->significand[2],
		pu->significand[3], (pu->rounded!=0),
		(pu->sticky!=0), pu->exponent);
}
#endif
