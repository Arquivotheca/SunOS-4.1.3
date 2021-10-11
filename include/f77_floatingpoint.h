/*	@(#)f77_floatingpoint.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*	f77 IEEE arithmetic function type definitions.	*/

#ifndef _f77_floatingpoint_h
#define _f77_floatingpoint_h

#define fp_direction_type integer
#ifdef sunrise
#define	fp_rd_nearest	0
#define	fp_rd_zero	1
#define	fp_rd_plus	2
#define	fp_rd_minus	3
#endif
#ifdef mc68000
#define	fp_rd_nearest	0
#define	fp_rd_zero	1
#define	fp_rd_minus	2
#define	fp_rd_plus	3
#endif

#define fp_precision_type integer
#define	fp_rp_extended	0
#define	fp_rp_single	1
#define	fp_rp_double	2
#define	fp_rp_unused	3

#define N_IEEE_EXCEPTION 5
#define fp_exception_type integer
#define	fp_inexact	0		/* exceptions according to bit number */
#define	fp_divide	1
#define	fp_underflow	2
#define	fp_overflow	3
#define	fp_invalid	4

#define fp_exception_field_type integer

#define	fp_class_type integer
#define	fp_zero		0
#define	fp_subnormal	1
#define	fp_normal	2
#define	fp_infinity   	3
#define	fp_quiet	4
#define	fp_signaling	5

#define N_SIGFPE_CODE 	13
#define sigfpe_code_type integer
#define sigfpe_handler_type integer
#define sigfpe_default	0		/* default exception handling */
#define sigfpe_abort	1		/* abort on exception */

#endif /*!_f77_floatingpoint_h*/
