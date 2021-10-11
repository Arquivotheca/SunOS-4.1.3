/*	@(#)psl.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987, 1988, 1989 by Sun Microsystems, Inc.
 */

/*
 * Definition of bits in the 68020 status register (SR)
 * We always use the 68020 interrupt stack because the
 * pain of fixing up the master stack when returning
 * off the interrupt stack is too high.
 */

#ifndef _sun3_psl_h
#define	_sun3_psl_h

#define	SR_T1		0x8000		/* trace on any intruction */
#define	SR_T0		0x4000		/* trace on change flow */
#define	SR_SMODE	0x2000		/* system mode */
#define	SR_MASTER	0x1000		/* master mode (non-interrupt) */
#define	SR_INTPRI	0x0700		/* interrupt priority bits */
#define	SR_CC		0x001F		/* all condition code bits */

/* Handy values for SR */
#define	SR_HIGH		0x2700		/* supervisor (interrupt) high pri */
#define	SR_LOW		0x2000		/* supervisor (interrupt) low pri */
#define	SR_USER		0x0000		/* user, low priority */
#define	SR_USERCLR	0xFF00		/* system bits */
#define	SR_TRACE	SR_T1		/* trace mode mask - use T1 (any) */


#define	PSL_USER	SR_USER		/* must set for user */
#define	PSL_USERMASK	~SR_USERCLR	/* must clear for user */
#define	PSL_ALLCC	SR_CC		/* condition code bits */
#define	PSL_T		SR_TRACE	/* trace bit */


/* convert a interrupt priority to a spl (SR) cookie */
#define	ipltospl(n)	(SR_SMODE|((n)<<8))

/* convert a system prority level to a spl (SR) cookie */
#define	pritospl(n)	ipltospl(n)

/* convert an spl (SR) cookie back to either a level or interrupt priority */
#define	spltopri(n)	(((n) & SR_INTPRI) >> 8)
#define	spltoipl(n)	spltopri(n)

/*
 * Note that on the sun3, interrupt priority and system priority are
 * identical. On other architectures this is not necessarily the case.
 */

#endif /*!_sun3_psl_h*/
