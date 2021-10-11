/*      @(#)reg.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Location of the users' stored
 * registers relative to R0.
 * Usage is u.u_ar0[XX].
 */

#ifndef _m68k_reg_h
#define _m68k_reg_h

#define	R0	(0)
#define	R1	(1)
#define	R2	(2)
#define	R3	(3)
#define	R4	(4)
#define	R5	(5)
#define	R6	(6)
#define	R7	(7)
#define	AR0	(8)
#define	AR1	(9)
#define	AR2	(10)
#define	AR3	(11)
#define	AR4	(12)
#define	AR5	(13)
#define	AR6	(14)
#define	AR7	(15)
#define	SP	(15)
#define	PS	(16)
#define	PC	(17)

/*
 * And now for something completely the same...
 */
#ifndef LOCORE
struct regs {	
	int	r_dreg[8];	/* data registers */
#define r_r0	r_dreg[0]	/* r0 for portability */
	int	r_areg[8];	/* address registers */
#define r_sp	r_areg[7]	/* user stack pointer */
	int	r_sr;		/* status register (actually a short) */
#define	r_ps	r_sr
	int	r_pc;		/* program counter */
};

struct stkfmt {
	int	f_stkfmt : 4;	/* stack format */
	int		 : 2;
	int	f_vector : 10;	/* vector offset */
	short	f_beibase;	/* start of bus error info (if any) */
};

#endif !LOCORE

#endif /*!_m68k_reg_h*/
