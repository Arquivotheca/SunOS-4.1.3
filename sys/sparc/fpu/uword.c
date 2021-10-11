#ifdef sccsid
static char	sccsid[] = "@(#)uword.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* Read/write user memory procedures for Sparc FPU simulator. */
#include <machine/param.h>
#include <machine/fpu/fpu_simulator.h>
#include <machine/fpu/globals.h>

#include <vm/seg.h>

extern int fuword();
extern int suword();
extern int fubyte();

enum ftt_type
_fp_read_word(address, pvalue, pfpsd)
	caddr_t		address;
	int		*pvalue;
	fp_simd_type	*pfpsd;
{
	int		w;
	int		b;

	if (((int) address & 0x3) != 0)
		return (ftt_alignment);	/* Must be word-aligned. */
	w = fuword(address);
	if (w == -1) {
		b = fubyte(address);
		if (b == -1) {
			pfpsd->fp_trapaddr = address;
			pfpsd->fp_traprw = S_READ;
			return (ftt_fault);
		}
	}
	*pvalue = w;
	return (ftt_none);
}

enum ftt_type
_fp_write_word(address, value, pfpsd)
	caddr_t		address;
	int		value;
	fp_simd_type	*pfpsd;
{
	int		w;

	if (((int) address & 0x3) != 0)
		return (ftt_alignment);	/* Must be word-aligned. */
	w = suword(address, value);
	if (w == -1) {
		pfpsd->fp_trapaddr = address;
		pfpsd->fp_traprw = S_WRITE;
		return (ftt_fault);
	} else {
		return (ftt_none);
	}
}

enum ftt_type
read_iureg(n, pregs, pwindow, pvalue, pfpsd)
	unsigned	n;
	struct regs	*pregs;	/* Pointer to PCB image of registers. */
	struct rwindow	*pwindow;/* Pointer to locals and ins. */
	int		*pvalue;
	fp_simd_type	*pfpsd;

{				/* Reads integer unit's register n. */
	register int	*pint;

	if (n <= 15) {
		if (n == 0) {
			*pvalue = 0;
			return (ftt_none);	/* Read global register 0. */
		}
		if (n <= 7) {	/* globals */
			pint = &(pregs->r_g1) + (n - 1);
			*pvalue = *pint;
			return (ftt_none);
		} else {	/* outs */
			pint = &(pregs->r_o0) + (n - 8);
			*pvalue = *pint;
			return (ftt_none);
		}
	} else {		/* locals and ins */
		if (n <= 23)
			pint = &pwindow->rw_local[n - 16];
		else
			pint = &pwindow->rw_in[n - 24];
		if ((int) pint > KERNELBASE){
			*pvalue =  *pint;
			return (ftt_none);
		}
		return _fp_read_word((char *) pint, pvalue, pfpsd);
	}
}
