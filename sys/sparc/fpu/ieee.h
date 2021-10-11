/*	@(#)ieee.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

	/*	Sparc IEEE floating-point support PUBLIC include file.  */

	/*	PUBLIC CONSTANTS	*/

	/*	PUBLIC TYPES 	*/

	/* IEEE Arithmetic types... numbered to correspond to fsr fields. */

enum fp_rounding_direction 		/* rounding direction */
	{
	fp_rd_nearest	= 0,
	fp_rd_zero	= 1,
	fp_rd_plus	= 2,
	fp_rd_minus	= 3
	} ;

enum fp_rounding_precision		/* extended rounding precision */
	{
	fp_rp_extended	= 0,
	fp_rp_single	= 1,
	fp_rp_double	= 2,
	fp_rp_3		= 3
	} ;

enum fp_exception_type			/* exceptions according to cexc bit number */
	{
	fp_inexact	= 0,
	fp_divide	= 1,
	fp_underflow	= 2,
	fp_overflow	= 3,
	fp_invalid	= 4
	} ;

enum fp_class_type			/* floating-point classes according to fclass */
	{
	fp_zero		= 0,
	fp_normal	= 1,		/* Includes subnormal. */
	fp_infinity   	= 2,
	fp_nan		= 3,		/* Includes quiet and signaling NaN. */
	} ;

	/* PUBLIC GLOBAL VARIABLES */

unsigned fp_accrued_exceptions ;	/* Sticky accumulated exceptions. */

	/* PUBLIC FUNCTIONS */

extern enum fp_rounding_direction swap_rounding_direction( /* rd */ ) ; 
/*
extern enum fp_rounding_direction rd ; 
 
/* Change rounding mode; return previous. */ 

extern int swap_accrued_exceptions ( /* x */ ) ;
/*
int x ;
 
/* Change accrued exceptions ; return previous. */
 

