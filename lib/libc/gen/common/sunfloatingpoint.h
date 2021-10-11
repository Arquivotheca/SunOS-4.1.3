/*	@(#)sunfloatingpoint.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <floatingpoint.h>

	/*	Sun floating-point PRIVATE include file.  */

	/*	PRIVATE MACROS	*/

#ifdef DEBUG
#define PRIVATE
#else
#define PRIVATE static
#endif

#define	DOUBLE_E(n) (n & 0xfffe ) 	/* More significant word of double. */
#define DOUBLE_F(n) (1+DOUBLE_E(n)) 	/* Less significant word of double. */

	/*	PRIVATE CONSTANTS	*/

#define INTEGER_BIAS	   31
#define	SINGLE_BIAS	  127
#define DOUBLE_BIAS	 1023
#define EXTENDED_BIAS	16383

#define DECIMAL_BINARY_INTEGER_SIZE 3088	/* Size of decimal to binary integer 
						conversion buffer. */

#define SINGLE_MAXE	  97		/* Maximum decimal exponent we need to consider. */
#define DOUBLE_MAXE	 771		/* Maximum decimal exponent we need to consider. */
#define EXTENDED_MAXE  12330		/* Maximum decimal exponent we need to consider. */

	/* PRIVATE TYPES 	*/

typedef struct		/* Type for passing partial results of decimal to binary conversion. */
	{		/* Floating point of base B==2**16 with at least 80 significant bits. */
			/* Interpreted as B**exponent * 0.s0s1s2s3s4s5 */
 	int	exponent ;		/* Signed exponent. */
	unsigned significand[6] ;	/* Significand normalized so significand[0] >= 1 ; 
						significand[n] < 2**16. */
					/* Least significant bit can be sticky. */
	}
	big_float ;

typedef unsigned decimal_binary_integer_buffer[DECIMAL_BINARY_INTEGER_SIZE] ;

#ifdef i386
typedef struct					/* Most significant word formats. */
	{
	unsigned significand:	23 ;
	unsigned exponent:	8 ;
	unsigned sign:		1 ;
	}
	single_msw ;

typedef struct
	{
	unsigned significand:	20 ;
	unsigned exponent:	11 ;
	unsigned sign:		1 ;
	}
	double_msw ;

typedef struct
	{
	unsigned exponent:	15 ;
	unsigned sign:		1 ;
	unsigned unused:	16 ;
	}
	extended_msw ;

typedef struct					/* Floating-point formats in detail. */
	{
	single_msw msw ;
	}
	single_formatted ;

typedef struct
	{
	unsigned significand2 ;
	double_msw msw ;
	}
	double_formatted ;

typedef struct
	{
	unsigned significand2 ;
	unsigned significand ;
	extended_msw msw ;
	}
	extended_formatted ;
#else
typedef struct					/* Most significant word formats. */
	{
	unsigned sign:		1 ;
	unsigned exponent:	8 ;
	unsigned significand:	23 ;
	}
	single_msw ;

typedef struct
	{
	unsigned sign:		1 ;
	unsigned exponent:	11 ;
	unsigned significand:	20 ;
	}
	double_msw ;

typedef struct
	{
	unsigned sign:		1 ;
	unsigned exponent:	15 ;
	unsigned unused:	16 ;
	}
	extended_msw ;

typedef struct					/* Floating-point formats in detail. */
	{
	single_msw msw ;
	}
	single_formatted ;

typedef struct
	{
	double_msw msw ;
	unsigned significand2 ;
	}
	double_formatted ;

typedef struct
	{
	extended_msw msw ;
	unsigned significand ;
	unsigned significand2 ;
	}
	extended_formatted ;
#endif

typedef union					/* Floating-point formats equivalenced. */
	{
	single_formatted 	f ;
	single		 	x ;
	}
	single_equivalence ;

typedef union
	{
	double_formatted 	f ;
	double		 	x ;
	}
	double_equivalence ;

typedef union
	{
	extended_formatted  	f ;
	extended	 	x ;
	}
	extended_equivalence ;

typedef struct					/* Unpack floating-point internal format. */
        {
        int 		sign ;
        enum fp_class_type fpclass ;
        int     	exponent ;              /* Unbiased exponent. */
        unsigned 	significand[3] ;	/* Third word is round and sticky. */
        }
        unpacked ;
 
	/* PRIVATE GLOBAL VARIABLES */

fp_exception_field_type _fp_current_exceptions ; /* Current floating-point exceptions. */

enum fp_direction_type
		_fp_current_direction ;	/* Current rounding direction. */

enum fp_precision_type
		_fp_current_precision ; /* Current rounding precision. */

	/* PRIVATE FUNCTIONS */

extern void _fp_normalize (/* pu */) ;
/*        unpacked        *pu ;           /* unpacked operand and result */

extern void _fp_rightshift (/* pu, n */) ;
/*	unpacked *pu ; unsigned n ;	*/
/*      Right shift significand sticky by n bits.  */

extern void _fp_set_exception(/* ex */) ;
/*	enum fp_exception_type ex ;	/* exception to be set in curexcep */ 

extern void _unpack_integer () ; /* pu, px )     *pu gets *px unpacked
unpacked	*pu ;
integer		*px ;
*/

extern void _unpack_single () ; /* pu, px )
unpacked	*pu ;
integer		*px ;
*/

extern void _unpack_double () ; /* pu, px )
unpacked	*pu ;
integer		*px ;
*/

extern void _unpack_extended () ; /* pu, px )
unpacked	*pu ;
integer		*px ;
*/

extern void _pack_integer () ; /* pu, px )		*px gets packed *pu
unpacked	*pu ;
integer		*px ;
*/

extern void _pack_single () ; /* pu, px )
unpacked	*pu ;
integer		*px ;
*/

extern void _pack_double () ; /* pu, px )
unpacked	*pu ;
integer		*px ;
*/

extern void _pack_extended () ; /* pu, px )
unpacked	*pu ;
integer		*px ;
*/

extern enum fp_class_type class_single () ;
extern enum fp_class_type class_double () ;
extern enum fp_class_type class_extended () ;

