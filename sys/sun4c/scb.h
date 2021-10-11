/*	@(#)scb.h 1.1 92/07/30 SMI	*/

#ifndef	_sun4c_scb_h
#define	_sun4c_scb_h

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#define	VEC_MIN 64
#define	VEC_MAX 255
#define	AUTOBASE	16		/* base for autovectored ints */

#ifndef LOCORE

typedef	struct trapvec {
	int	instr[4];
} trapvec;

/*
 * Sparc System control block layout
 */

struct scb {
	trapvec reset;			/* 0 reset */
	trapvec inst_access;		/* 1 instruction access */
	trapvec illegal_inst;		/* 2 illegal instruction */
	trapvec priv_inst;		/* 3 privilegded instruction */
	trapvec fp_disabled;		/* 4 floating point desabled */
	trapvec window_overflow;	/* 5 window overflow */
	trapvec window_underflow;	/* 6 window underflow */
	trapvec alignment_excp;		/* 7 alignment */
	trapvec fp_exception;		/* 8 floating point exception */
	trapvec	data_acess;		/* 9 protection violation */
	trapvec	tag_overflow;		/* 10 tag overflow taddtv */
	trapvec reserved[5];		/* 11 - 15 */
	trapvec stray;			/* 16 spurious */
	trapvec	interrupts[15];		/* 17 - 31 autovectors */
	trapvec impossible[96];		/* 32 - 127 "cannot happen" */
	trapvec user_trap[128];		/* 128 - 255 */
};

#ifdef KERNEL
extern	struct scb scb;
#endif KERNEL

#endif !LOCORE

#endif	/* !_sun4c_scb_h */
