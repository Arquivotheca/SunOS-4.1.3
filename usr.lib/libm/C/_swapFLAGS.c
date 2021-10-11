
#ifndef lint
static	char sccsid[] = "@(#)_swapFLAGS.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* 
 * _swapRD(rd) 	exchanges rd with the current rounding direction.
 * _swapRP(rp) 	exchanges rp with the current rounding precision.
 * _swapTE(ex)  exchanges ex with the current exception trap enable bits.
 * _swapEX(ex) 	exchanges ex with the current accrued exception. 
 *		Here ex is a five bit value where each bit (cf. enum 
 *		fp_exception_type in <sys/ieeefp.h>) corresponds 
 *		to either an exception-occurred accrued status flag or an
 *		exception trap enable flag (0 off,1 on):
 * 			bit 0 :  inexact flag
 * 			bit 1 :  division by zero flag
 * 			bit 2 :  underflow flag
 * 			bit 3 :  overflow flag
 * 			bit 4 :  invalid flag
 */

#include <math.h>

enum fp_direction_type _swapRD(rd)
enum fp_direction_type rd;
{
	/* 
	   swap rd with the current rounding direction
	 */
	return (enum fp_direction_type) -1;	/* not available */
}

enum fp_precision_type _swapRP(rp)
enum fp_precision_type rp;
{
	/* 
	   swap rp with the current rounding precision
	 */
	return (enum fp_precision_type) -1;	/* not available */
}

int _swapEX(ex)
int ex;
{
	/* 
	   swap ex with the current accrued exception mode
	 */
	return -1;	/* not available */
}

int _swapTE(ex)
int ex;
{
	/* 
	   swap ex with the current exception trap enable bits
	 */
	return -1;	/* not available */
}
