/*      @(#)DEFS.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifdef PROF
	.global  .mcount
#define WINDOWSIZE	(16*4)
#define MCOUNT(x)	save	%sp, -WINDOWSIZE, %sp;\
			sethi	%hi(L_/**/x/**/1), %o0;\
			call	.mcount;\
			or	%o0, %lo(L_/**/x/**/1), %o0;\
			restore;\
			.reserve	L_/**/x/**/1, 4, "data"

#define RTMCOUNT(x)	save	%sp, -WINDOWSIZE, %sp;\
			sethi	%hi(L/**/x/**/1), %o0;\
			call	.mcount;\
			or	%o0, %lo(L/**/x/**/1), %o0;\
			restore;\
			.reserve L/**/x/**/1, 4, "data"

#else not PROF

#define MCOUNT(x)
#define RTMCOUNT(x)

#endif not PROF

#define RET		retl; nop
#define	ENTRY(x)	.global _/**/x; _/**/x: MCOUNT(x)
#define	RTENTRY(x)	.global x; x: RTMCOUNT(x)
