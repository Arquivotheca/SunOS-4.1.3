/*      @(#)psl.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _sparc_psl_h
#define _sparc_psl_h

/*
 * Definition of bits in the Sun-4 PSR (Processor Status Register)
 *  ________________________________________________________________________
 * | IMPL | VER |      ICC      | resvd | EC | EF | PIL | S | PS | ET | CWP |
 * |      |     | N | Z | V | C |       |    |    |     |   |    |    |     |
 * |------|-----|---|---|---|---|-------|----|----|-----|---|----|----|-----|
 *  31  28 27 24  23  22  21  20 19   14  13   12  11  8   7   6    5  4   0
 *
 * Reserved bits are defined to be initialized to zero and must
 * be preserved if written, for compatibility with future revisions.
 */

#define PSR_CWP		0x0000001F	/* current window pointer */
#define PSR_ET		0x00000020	/* enable traps */
#define PSR_PS		0x00000040	/* previous supervisor mode */
#define PSR_S		0x00000080	/* supervisor mode */
#define PSR_PIL		0x00000F00	/* processor interrupt level */
#define PSR_EF		0x00001000	/* enable floating point unit */
#define PSR_EC		0x00002000	/* enable coprocessor */
#define PSR_RSV		0x000FC000	/* reserved */
#define PSR_ICC		0x00F00000	/* integer condition codes */
#define PSR_C		0x00100000	/* carry bit */
#define PSR_V		0x00200000	/* overflow bit */
#define PSR_Z		0x00400000	/* zero bit */
#define PSR_N		0x00800000	/* negative bit */
#define PSR_VER		0x0F000000	/* mask version */
#define PSR_IMPL	0xF0000000	/* implementation */

#define PSL_ALLCC	PSR_ICC		/* for portability */
#define SR_SMODE	PSR_PS

/*
 * Handy psr values.
 */
#define PSL_USER	(PSR_S)		/* initial user psr */
#define PSL_USERMASK	(PSR_ICC)	/* user variable psr bits */
#define PSR_PIL1	0x100		/* for rounding up interrupt pri */

/*
 * Macros to decode psr.
 */
#define USERMODE(ps)	(((ps) & PSR_PS) == 0)
#define BASEPRI(ps)	(((ps) & PSR_PIL) == 0)

/*
 * Convert system interrupt priorities (0-7) into a psr for splx.
 * In general, the processor priority (0-15) should be 2 times
 * the system pririty.
 */
#define pritospl(n)	((n) << 9)

/*
 * Convert a hardware interrupt priority level (0-15) into a psr for splx.
 * Also are macros to convert back.
 */
#define ipltospl(n)	(((n) & 0xf) << 8)
#define spltoipl(n)	(((n) >> 8) & 0xf)
#define spltopri(n)	((((n) + (1 << 8)) >> 9) & 0xf)

#endif /*!_sparc_psl_h*/
