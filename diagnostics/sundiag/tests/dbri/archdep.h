#ident   "@(#)archdep.h 1.1 92/07/30 SMI"

/*
 * Derived from:
 *  @(#)archdep.h 1.1 89/12/22 SMI
 */
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#ifndef _multimedia_archdep_h
#define	_multimedia_archdep_h

/*
 * Machine-dependent and implementation-dependent definitions
 * are placed here so that source code can be portable among a wide
 * variety of machines.
 */

/*
 * The following macros are used to generate architecture-specific
 * code for handling byte-ordering correctly.
 *
 * Note that these macros *do not* work for in-place transformations.
 */

#if defined (mc68000) || defined (sparc)
#define	DECODE_SHORT(from, to)	*((short *)(to)) = *((short *)(from))
#define	DECODE_LONG(from, to)	*((long *)(to)) = *((long *)(from))
#define	DECODE_FLOAT(from, to)	*((float *)(to)) = *((float *)(from))
#define	DECODE_DOUBLE(from, to)	*((double *)(to)) = *((double *)(from))
#endif /*big-endian*/

#if defined (i386)
#define	DECODE_SHORT(from, to)						\
			    ((char *)(to))[0] = ((char *)(from))[1];	\
			    ((char *)(to))[1] = ((char *)(from))[0];
#define	DECODE_LONG(from, to)						\
			    ((char *)(to))[0] = ((char *)(from))[3];	\
			    ((char *)(to))[1] = ((char *)(from))[2];	\
			    ((char *)(to))[2] = ((char *)(from))[1];	\
			    ((char *)(to))[3] = ((char *)(from))[0];

#define	DECODE_FLOAT(from, to)		DECODE_LONG((to), (from))

#define	DECODE_DOUBLE(from, to)						\
			    ((char *)(to))[0] = ((char *)(from))[7];	\
			    ((char *)(to))[1] = ((char *)(from))[6];	\
			    ((char *)(to))[2] = ((char *)(from))[5];	\
			    ((char *)(to))[3] = ((char *)(from))[4];	\
			    ((char *)(to))[4] = ((char *)(from))[3];	\
			    ((char *)(to))[5] = ((char *)(from))[2];	\
			    ((char *)(to))[6] = ((char *)(from))[1];	\
			    ((char *)(to))[7] = ((char *)(from))[0];
#endif /*little-endian*/


/* Most architectures are symmetrical with respect to conversions. */
#if defined (mc68000) || defined (sparc) || defined (i386)
#define	ENCODE_SHORT(from, to)		DECODE_SHORT((from), (to))
#define	ENCODE_LONG(from, to)		DECODE_LONG((from), (to))
#define	ENCODE_FLOAT(from, to)		DECODE_FLOAT((from), (to))
#define	ENCODE_DOUBLE(from, to)		DECODE_DOUBLE((from), (to))

/* Define types of specific length */
typedef char		i_8;
typedef short		i_16;
typedef int		i_32;
typedef unsigned char	u_8;
typedef unsigned short 	u_16;
typedef unsigned	u_32;

#endif /*Sun machines*/

#endif /*!_multimedia_archdep_h*/
