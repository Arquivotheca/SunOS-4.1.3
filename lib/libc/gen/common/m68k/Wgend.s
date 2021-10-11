        .data
|        .asciz  "@(#)Wgend.s 1.1 92/07/30 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Wdefs.h"

RTENTRY(Wmodd)
        moveml	d0/d1,sp@-
        fmoved	sp@,fp0
        fmodd   a0@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
        RET
RTENTRY(Wremd)
        moveml	d0/d1,sp@-
        fmoved	sp@,fp0
        fremd   a0@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
        RET
RTENTRY(Wsqrtd)
        moveml	d0/d1,sp@-
	fmovel	fpcr,d0			| Fetch old mode.
	movel	d0,d1			| d1 saves old fp mode.
	andb	#0x3f,d0		| Clear rounding precision.
	orb	#0x80,d0		| Set double precision.
	fmovel	d0,fpcr			| Set new mode.
	fsqrtd	sp@,fp0
	fmoved	fp0,sp@
	fmovel	d1,fpcr			| Restore old mode.
	moveml	sp@+,d0/d1
        RET
RTENTRY(Wlength2d)
        moveml	d0/d1,sp@-
        fmoved	sp@,fp0
        fmulx   fp0,fp0
        fmoved  a0@,fp1
        fmulx   fp1,fp1
        faddx   fp1,fp0
        fsqrtx  fp0,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
        RET

RTENTRY(Waintd)
	moveml	d0/d1,sp@-
	fintrzd	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET

#define PREAINTS \
	movel	d0,a0 ; \
	andl	#0x7fffffff,d0 ; \
	cmpl	threshold,d0 ; \
	blts	7f 		/* Branch if possibly nonintegral. */ ; \
	cmpl	inf,d0 ; \
	jlt	8f 		/* Branch if not NaN. */ ; \
	bgt	7f 		/* Branch if NaN. */ ; \
	tstl	d1 ; \
	jeq	8f 		/* Branch if infinity. */ ; \
7: ; \
	LOADFPABASE ; \
	fpmove@1 fpamode,d0	/* a0 save old fpa mode */ ; \
	andl	#0xf,d0		/* Clear garbage bits. */ ; \
	exg	a0,d0		/* a0 gets old mode, d0 gets argument. */

#define MIDAINTS \
	fpmoved@1	threshold,fpa1 ; \
	movel		d1,a1@(0xc04) ; \
	movel		d0,a1@(0xc00) ; \
	bmis	1f ; \
	fpaddd@1	fpa1,fpa0 ; \
	fpsubd@1	fpa1,fpa0 ; \
	movel	a1@(0xc00),d0 ; \
	cmpl	#0x80000000,d0 ; \
	bnes	2f ; \
	clrl	d0 ; \
	clrl	d1 ; \
	bras	3f ; \
1: ; \
	fpsubd@1	fpa1,fpa0 ; \
	fpaddd@1	fpa1,fpa0 ; \
	movel	a1@(0xc00),d0 ; \
	bnes	2f ; \
	movel	#0x80000000,d0 ; \
	clrl	d1 ; \
	bras	3f ; \
2: ; \
	movel	a1@(0xc04),d1 ; \
3: ; \
	fpmove@1 a0,fpamode	/* restore old fpa mode */

#define POSTAINTS \
	bras	9f ; \
8: ; \
	movel	a0,d0 ; \
9: ; \
	RET

#define AINTS(RMODE) \
	PREAINTS ; \
	fpmove@1	#RMODE,fpamode ; \
	MIDAINTS ; \
	POSTAINTS

threshold: .double 0r4.503599627370496000E+15
half:	.double	0r0.5
one:	.double	0r1
inf:	.long	0x7ff00000		| Smallest NaN.

RTENTRY(Wceild)
	AINTS(WRP)
RTENTRY(Wfloord)
	AINTS(WRM)
RTENTRY(Warintd)
	PREAINTS
	MIDAINTS
	POSTAINTS
RTENTRY(Wanintd)
	PREAINTS
	fpmove@1	#WRZ,fpamode
	fpmoved@1	threshold,fpa1
|	fpmoved@1	d0:d1,fpa0
	movel		d1,a1@(0xc04)
	movel		d0,a1@(0xc00)
	bmis	1f
	fpaddd@1	half,fpa0
	fpaddd@1	fpa1,fpa0
	fpsubd@1	fpa1,fpa0
	movel	a1@(0xc00),d0
	cmpl	#0x80000000,d0
	bnes	2f
	clrl	d0
	clrl	d1
	bras	3f
1:
	fpsubd@1	half,fpa0
	fpsubd@1	fpa1,fpa0
	fpaddd@1	fpa1,fpa0
	movel	a1@(0xc00),d0
	bnes	2f
	movel	#0x80000000,d0
	clrl	d1
	bras	3f
2:
	movel	a1@(0xc04),d1
3:
	fpmove@1	a0,fpamode	| Restore FPA mode.
	POSTAINTS
