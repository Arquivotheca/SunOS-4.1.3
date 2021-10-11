/*	@(#)libm.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#define TRUE  1
#define FALSE 0

extern double SVID_libm_err();

/* 
 * IEEE double precision constants 
 */
static double align__double = 0.0;
#ifdef i386
static long ln2x[] = 		{ 0xfefa39ef, 0x3fe62e42 };
static long ln2hix[] = 		{ 0xfee00000, 0x3fe62e42 };
static long ln2lox[] = 		{ 0x35793c76, 0x3dea39ef };
static long NaNx[] = 		{ 0xffffffff, 0x7fffffff };
static long sNaNx[] = 		{ 0x00000001, 0x7ff00000 };
static long Infx[] = 		{ 0x00000000, 0x7ff00000 };
static long two52x[] = 		{ 0x00000000, 0x43300000 };
static long twom52x[] = 	{ 0x00000000, 0x3cb00000 };
static long PI_RZx[] = 		{ 0x54442d18, 0x400921fb };
static long sqrt2x[] = 		{ 0x667f3bcd, 0x3ff6a09e };/* rounded up */
static long sqrt2p1_hix[] = 	{ 0x333f9de6, 0x4003504f };
static long sqrt2p1_lox[] = 	{ 0xf626cdd5, 0x3ca21165 };
static long fmaxx[] = 		{ 0xffffffff, 0x7fefffff };
static long fminx[] = 		{ 0x00000001, 0x00000000 };
static long fminnx[] = 		{ 0x00000000, 0x00100000 };
static long fmaxsx[] = 		{ 0xffffffff, 0x000fffff };
static long lnovftx[] = 	{ 0xfefa39ef, 0x40862e42 };/* chopped */
static long lnunftx[] = 	{ 0xd52d3052, 0xc0874910 };/* ln(minf/2) chop*/
static long invln2x[] = 	{ 0x652b82fe, 0x3ff71547 };
#else
static long ln2x[] = 		{ 0x3fe62e42, 0xfefa39ef };
static long ln2hix[] = 		{ 0x3fe62e42, 0xfee00000 };
static long ln2lox[] = 		{ 0x3dea39ef, 0x35793c76 };
static long NaNx[] = 		{ 0x7fffffff, 0xffffffff };
static long sNaNx[] = 		{ 0x7ff00000, 0x00000001 };
static long Infx[] = 		{ 0x7ff00000, 0x00000000 };
static long two52x[] = 		{ 0x43300000, 0x00000000 };
static long twom52x[] = 	{ 0x3cb00000, 0x00000000 };
static long PI_RZx[] = 		{ 0x400921fb, 0x54442d18 };
static long sqrt2x[] = 		{ 0x3ff6a09e, 0x667f3bcd };/* rounded up */
static long sqrt2p1_hix[] = 	{ 0x4003504f, 0x333f9de6 };
static long sqrt2p1_lox[] = 	{ 0x3ca21165, 0xf626cdd5 };
static long fmaxx[] = 		{ 0x7fefffff, 0xffffffff };
static long fminx[] = 		{ 0x00000000, 0x00000001 };
static long fminnx[] = 		{ 0x00100000, 0x00000000 };
static long fmaxsx[] = 		{ 0x000fffff, 0xffffffff };
static long lnovftx[] = 	{ 0x40862e42, 0xfefa39ef };/* chopped */
static long lnunftx[] = 	{ 0xc0874910, 0xd52d3052 };/* ln(minf/2) chop*/
static long invln2x[] = 	{ 0x3ff71547, 0x652b82fe };
#endif
#define ln2 		(*(double*)ln2x)
#define NaN 		(*(double*)NaNx)
#define sNaN 		(*(double*)sNaNx)
#define Inf 		(*(double*)Infx)
#define two52 		(*(double*)two52x)
#define twom52 		(*(double*)twom52x)
#define ln2hi 		(*(double*)ln2hix)
#define ln2lo 		(*(double*)ln2lox)
#define PI_RZ 		(*(double*)PI_RZx)
#define sqrt2 		(*(double*)sqrt2x)
#define sqrt2p1_hi 	(*(double*)sqrt2p1_hix)
#define sqrt2p1_lo 	(*(double*)sqrt2p1_lox)
#define fmax 		(*(double*)fmaxx)
#define fmin 		(*(double*)fminx)
#define fminn 		(*(double*)fminnx)
#define fmaxs 		(*(double*)fmaxsx)
#define lnovft 		(*(double*)lnovftx)
#define lnunft 		(*(double*)lnunftx)
#define invln2 		(*(double*)invln2x)
