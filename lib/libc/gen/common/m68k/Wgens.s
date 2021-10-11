        .data
|        .asciz  "@(#)Wgens.s 1.1 92/07/30 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Wdefs.h"

RTENTRY(Wsqrts)
        fsqrts  d0,fp0
        fmoves  fp0,d0
        RET
RTENTRY(Wlength2s)
        fpstod	d0,fpa0
        fpsqrd	fpa0,fpa0
	fpstod	d1,fpa1
	fpmad	fpa1,fpa1,fpa0,fpa0
	fpmoved	fpa0,sp@-
        fsqrtd  sp@+,fp0
        fmoves  fp0,d0
        RET
RTENTRY(Wmods)
        fmoves  d0,fp0
        fmods   d1,fp0
        fmoves  fp0,d0
        RET
RTENTRY(Wrems)
        fmoves  d0,fp0
        frems   d1,fp0
        fmoves  fp0,d0
        RET

#define PREAINTS \
	movel	d0,d1 ; \
	andl	#0x7fffffff,d1 ; \
	cmpl	threshold,d1 ; \
	blts	7f		/* Branch if possibly non-integral. */ ; \
	cmpl	inf,d1 ; \
	bles	9f		/* Branch if not a nan. */ ; \
7: ; \
	LOADFPABASE ; \
	fpmove@1 fpamode,d1	/* Save old FPA mode. */ ; \
	andl	#0xf,d1		/* Clear garbage bits. */

#define MIDAINTS \
	fpmoves@1	threshold,fpa1 ; \
	fpmoves@1	d0,fpa0 ; \
	bmis	1f ; \
	fpadds@1	fpa1,fpa0 ; \
	fpsubs@1	fpa1,fpa0 ; \
	fpmoves@1	fpa0,d0 ; \
	cmpl	#0x80000000,d0 ; \
	bnes	2f ; \
	clrl	d0 ; \
	bras	2f ; \
1: ; \
	fpsubs@1	fpa1,fpa0 ; \
	fpadds@1	fpa1,fpa0 ; \
	fpmoves@1	fpa0,d0 ; \
	bnes	2f ; \
	movel	#0x80000000,d0 ; \
2: ; \
	fpmove@1 d1,fpamode	/* Restore old FPA mode. */

#define POSTAINTS \
9: ; \
	RET

#define AINTS(RMODE) \
	PREAINTS ; \
	fpmove@1	#RMODE,fpamode ; \
	MIDAINTS ; \
	POSTAINTS

threshold: .single 0r8388608
half:	.single	0r0.5
one:	.single	0r1
inf:	.long	0x7f800000

RTENTRY(Waints)
	AINTS(WRZ)
RTENTRY(Wceils)
	AINTS(WRP)
RTENTRY(Wfloors)
	AINTS(WRM)
RTENTRY(Warints)
	PREAINTS
	MIDAINTS
	POSTAINTS
RTENTRY(Wanints)
	PREAINTS
	fpmove@1	#WRZ,fpamode
	fpmoves@1	threshold,fpa1
	fpmoves@1	d0,fpa0
	bmis	1f
	fpadds@1	half,fpa0
	fpadds@1	fpa1,fpa0
	fpsubs@1	fpa1,fpa0
	fpmoves@1	fpa0,d0
	cmpl	#0x80000000,d0
	bnes	2f
	clrl	d0
	bras	2f
1:
	fpsubs@1	half,fpa0
	fpsubs@1	fpa1,fpa0
	fpadds@1	fpa1,fpa0
	fpmoves@1	fpa0,d0
	bnes	2f
	movel	#0x80000000,d0
2:
	fpmove@1 d1,fpamode	| Restore old FPA mode.
	POSTAINTS
