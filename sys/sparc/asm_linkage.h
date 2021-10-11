/*	@(#)asm_linkage.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * A stack frame looks like:
 *
 * %fp->|				|
 *	|-------------------------------|
 *	|  Locals, temps, saved floats	|
 *	|-------------------------------|
 *	|  outgoing parameters past 6	|
 *	|-------------------------------|-\
 *	|  6 words for callee to dump	| |
 *	|  register arguments		| |
 *	|-------------------------------|  > minimum stack frame
 *	|  One word struct-ret address	| |
 *	|-------------------------------| |
 *	|  16 words to save IN and	| |
 * %sp->|  LOCAL register on overflow	| |
 *	|-------------------------------|-/
 */

#ifndef _sparc_asm_linkage_h
#define _sparc_asm_linkage_h

/*
 * Constants defining a stack frame.
 */
#define WINDOWSIZE	(16*4)		/* size of window save area */
#define ARGPUSHSIZE	(6*4)		/* size of arg dump area */
#define ARGPUSH		(WINDOWSIZE+4)	/* arg dump area offset */
#define MINFRAME	(WINDOWSIZE+ARGPUSHSIZE+4) /* min frame */

/*
 * Stack alignment macros.
 */
#define STACK_ALIGN	8
#define SA(X)	(((X)+(STACK_ALIGN-1)) & ~(STACK_ALIGN-1))


/*
 * Profiling causes defintions of the MCOUNT and RTMCOUNT
 * particular to the type.  Note: the nop in the following
 * macros allows them to be used in a delay slot.
 */
#ifdef GPROF

#define MCOUNT(x) \
	nop; \
	save	%sp, -SA(MINFRAME), %sp; \
	call	mcount; \
	nop ; \
	restore	;

#define RTMCOUNT(x) \
	nop; \
	save	%sp, -SA(MINFRAME), %sp; \
	call	mcount; \
	nop ; \
	restore	;

#endif GPROF


#ifdef PROF

#define MCOUNT(x) \
	nop; \
	save	%sp, -SA(MINFRAME), %sp; \
	sethi	%hi(L_/**/x/**/1), %o0; \
	call	mcount; \
        or      %o0, %lo(L_/**/x/**/1), %o0; \
        restore; \
        .reserve	L_/**/x/**/1, 4, "data", 4

#define RTMCOUNT(x) \
	nop; \
	save	%sp, -SA(MINFRAME), %sp; \
        sethi	%hi(L/**/x/**/1), %o0; \
        call	mcount; \
        or	%o0, %lo(L/**/x/**/1), %o0; \
        restore; \
        .reserve	L/**/x/**/1, 4, "data", 4

#endif PROF

/*
 * if we are not profiling,
 * MCOUNT and RTMCOUNT should be defined to nothing
 */
#if !defined(PROF) && !defined(GPROF)
#define MCOUNT(x)
#define RTMCOUNT(x)
#endif !PROF && !GPROF

/*
 * Entry macros for assembler subroutines.
 * NAME prefixes the underscore before a symbol.
 * ENTRY provides a way to insert the calls to mcount for profiling.
 * RTENTRY is similar to the above but provided for run-time routines
 *	whose names should not be prefixed with an underscore.
 */

#define	NAME(x) _/**/x

#define ENTRY(x) \
	.global	NAME(x); \
	NAME(x): MCOUNT(x)

#define ENTRY2(x,y) \
	.global	NAME(x), NAME(y); \
	NAME(x): ; \
	NAME(y): MCOUNT(x)

#define RTENTRY(x) \
	.global	x; x: RTMCOUNT(x)

/*
 * For additional entry points.
 */
#define ALTENTRY(x) \
	.global NAME(x); \
NAME(x):

#ifdef KERNEL
/*
 * Macros for saving/restoring registers.
 */

#define SAVE_GLOBALS(RP) \
	st	%g1, [RP + G1*4]; \
	std	%g2, [RP + G2*4]; \
	std	%g4, [RP + G4*4]; \
	std	%g6, [RP + G6*4]; \
	mov	%y, %g1; \
	st	%g1, [RP + Y*4]

#define RESTORE_GLOBALS(RP) \
	ld	[RP + Y*4], %g1; \
	mov	%g1, %y; \
	ld	[RP + G1*4], %g1; \
	ldd	[RP + G2*4], %g2; \
	ldd	[RP + G4*4], %g4; \
	ldd	[RP + G6*4], %g6;

#define SAVE_OUTS(RP) \
	std	%i0, [RP + O0*4]; \
	std	%i2, [RP + O2*4]; \
	std	%i4, [RP + O4*4]; \
	std	%i6, [RP + O6*4]

#define RESTORE_OUTS(RP) \
	ldd	[RP + O0*4], %i0; \
	ldd	[RP + O2*4], %i2; \
	ldd	[RP + O4*4], %i4; \
	ldd	[RP + O6*4], %i6;

#define SAVE_WINDOW(SBP) \
	std	%l0, [SBP + (0*4)]; \
	std	%l2, [SBP + (2*4)]; \
	std	%l4, [SBP + (4*4)]; \
	std	%l6, [SBP + (6*4)]; \
	std	%i0, [SBP + (8*4)]; \
	std	%i2, [SBP + (10*4)]; \
	std	%i4, [SBP + (12*4)]; \
	std	%i6, [SBP + (14*4)]

#define RESTORE_WINDOW(SBP) \
	ldd	[SBP + (0*4)], %l0; \
	ldd	[SBP + (2*4)], %l2; \
	ldd	[SBP + (4*4)], %l4; \
	ldd	[SBP + (6*4)], %l6; \
	ldd	[SBP + (8*4)], %i0; \
	ldd	[SBP + (10*4)], %i2; \
	ldd	[SBP + (12*4)], %i4; \
	ldd	[SBP + (14*4)], %i6;

#define STORE_FPREGS(FCP) \
	std	%f0, [FCP + FPCTX_REGS]; \
	std	%f2, [FCP + FPCTX_REGS + 8]; \
	std	%f4, [FCP + FPCTX_REGS + 16]; \
	std	%f6, [FCP + FPCTX_REGS + 24]; \
	std	%f8, [FCP + FPCTX_REGS + 32]; \
	std	%f10, [FCP + FPCTX_REGS + 40]; \
	std	%f12, [FCP + FPCTX_REGS + 48]; \
	std	%f14, [FCP + FPCTX_REGS + 56]; \
	std	%f16, [FCP + FPCTX_REGS + 64]; \
	std	%f18, [FCP + FPCTX_REGS + 72]; \
	std	%f20, [FCP + FPCTX_REGS + 80]; \
	std	%f22, [FCP + FPCTX_REGS + 88]; \
	std	%f24, [FCP + FPCTX_REGS + 96]; \
	std	%f26, [FCP + FPCTX_REGS + 104]; \
	std	%f28, [FCP + FPCTX_REGS + 112]; \
	std	%f30, [FCP + FPCTX_REGS + 120]

#define LOAD_FPREGS(FCP) \
	ldd	[FCP + FPCTX_REGS ], %f0; \
	ldd	[FCP + FPCTX_REGS + 8], %f2; \
	ldd	[FCP + FPCTX_REGS + 16], %f4; \
	ldd	[FCP + FPCTX_REGS + 24], %f6; \
	ldd	[FCP + FPCTX_REGS + 32], %f8; \
	ldd	[FCP + FPCTX_REGS + 40], %f10; \
	ldd	[FCP + FPCTX_REGS + 48], %f12; \
	ldd	[FCP + FPCTX_REGS + 56], %f14; \
	ldd	[FCP + FPCTX_REGS + 64], %f16; \
	ldd	[FCP + FPCTX_REGS + 72], %f18; \
	ldd	[FCP + FPCTX_REGS + 80], %f20; \
	ldd	[FCP + FPCTX_REGS + 88], %f22; \
	ldd	[FCP + FPCTX_REGS + 96], %f24; \
	ldd	[FCP + FPCTX_REGS + 104], %f26; \
	ldd	[FCP + FPCTX_REGS + 112], %f28; \
	ldd	[FCP + FPCTX_REGS + 120], %f30; \

#endif KERNEL

#endif /*!_sparc_asm_linkage_h*/
