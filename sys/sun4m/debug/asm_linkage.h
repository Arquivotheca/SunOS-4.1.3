/*      @(#)asm_linkage.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _REG_
#include "reg.h"
#endif

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

/*
 * constants defining a stack frame.
 */
#define WINDOWSIZE	(16*4)
#define ARGPUSHSIZE	(6*4)		/* size of arg dump area */
#define ARGPUSH		(WINDOWSIZE+4)	/* arg dump area offset */
#define MINFRAME	(WINDOWSIZE+ARGPUSHSIZE+8) /* min frame */

/*
 * Entry macros for assembler subroutines.
 * NAME prefixes the underscore before a symbol.
 * ENTRY provides a way to insert the calls to mcount for profiling.
 * RTENTRY is similar to the above but provided for run-time routines
 *	whose names should not be prefixed with an underscore.
 */
#define	NAME(x) _/**/x

#ifdef GPROF
#define ENTRY(x)  \
	.global	NAME(x); \
NAME(x): \
	mov	%i7, %g1; \
	call	mcount; \
	save	%sp, -MINFRAME, %sp; \
	restore	;
	
#define ENTRY2(x,y)  \
	.global	NAME(x), NAME(y); \
NAME(x): ; \
NAME(y):   \
	mov	%i7, %g1; \
	call	mcount; \
	save	%sp, -MINFRAME, %sp;
	restore ;

#define RTENTRY(x) \
	.global	x;  x: ; \
	mov	%i7, %g1; \
	call	mcount; \
	save	%sp, -MINFRAME, %sp; \
	restore	;

#else GPROF
#define ENTRY(x)  \
	.global	NAME(x); \
NAME(x):

#define ENTRY2(x,y)  \
	ENTRY(x)  \
	ENTRY(y)  

#define RTENTRY(x) \
	.global	x; x:

#endif GPROF
