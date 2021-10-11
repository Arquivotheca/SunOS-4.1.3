        .data
|        .asciz  "@(#)Mfloat.s 1.1 92/07/30 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Mdefs.h"

RTENTRY(Muns)
	cmpl	twoto31,d0
	bges	1f
	fintrzs	d0,fp0 			| fp0 gets x.
	fmovel	fp0,d0
	bras	2f
1:
	fmoves	d0,fp0
	fsubs	twoto31,fp0		| Subtract 2**31.
	fintrzx	fp0,fp0 		| fp0 gets x.
	fmovel	fp0,d0
	bchg	#31,d0			| Add 2**31.
2:
 	RET

twoto31: .long 0x4f000000		| 2**31.

RTENTRY(Mund)
	moveml  d0/d1,sp@-
	cmpl	#0x41e00000,d0
	bges	1f
        fintrzd sp@+,fp0
	fmovel	fp0,d0
	bras	2f
1:
	fmoved	sp@+,fp0
	fsubs	twoto31,fp0		| Subtract 2**31.
	fintrzx	fp0,fp0 		| fp0 gets x.
	fmovel	fp0,d0
	bchg	#31,d0			| Add 2**31.
2:
 	RET

RTENTRY(Mdtos)
	FMOVEDINC
	fmoves	fp0,d0
	RET
RTENTRY(Mstod)
	fmoves	d0,fp0
	FMOVEDEC
	RET

RTENTRY(Mflts)
	fmovel	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Mrints)
	fmoves	d0,fp0 			| fp0 gets x.
	fmovel	fp0,d0
	RET
RTENTRY(Mints)
	fintrzs	d0,fp0 			| fp0 gets x.
	fmovel	fp0,d0
	RET
RTENTRY(Mnints)
	fmoves	d0,fp0			| fp0 gets x.
	movel	#0x3f000000,d1		| d1 gets 0.5
	tstl	d0
	bpls	1f			| Branch if x is negative
	bset	#31,d1			| d1 gets -0.5 if x is -.
1:	
	fadds	d1,fp0			| Add +-0.5.
	fintrzx	fp0,fp0
	fmovel	fp0,d0
	RET
RTENTRY(Msqrs)
	fmoves	d0,fp0
	fmulx	fp0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Madds)
	fmoves	d0,fp0
	fadds	d1,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Msubs)
	fmoves	d0,fp0
	fsubs	d1,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Mmuls)
	fmoves	d0,fp0
	fmuls d1,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Mdivs)
	fmoves	d0,fp0
	fdivs d1,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Mscaleis)
	fmoves	d0,fp0
	fscalel d1,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Mfltd)
	fmovel	d0,fp0
	FMOVEDEC
	RET
RTENTRY(Mrintd)
	FMOVEDINC
	fmovel	fp0,d0
	RET
RTENTRY(Mintd)
	moveml  d0/d1,sp@-
        fintrzd sp@+,fp0
	fmovel	fp0,d0
	RET
RTENTRY(Mnintd)
	FMOVEDINC
	movel	#0x3f000000,d1		| d1 gets 0.5
	tstl	d0
	bpls	1f			| Branch if x is negative
	bset	#31,d1			| d1 gets -0.5 if x is -.
1:	
	fadds	d1,fp0			| Add +-0.5.
	fintrzx	fp0,fp0
	fmovel	fp0,d0
	RET
RTENTRY(Msqrd)
	FMOVEDIN
	fmulx	fp0,fp0
	FMOVEDOUT
	RET
RTENTRY(Maddd)
	FMOVEDIN
	faddd	a0@,fp0
	FMOVEDOUT
	RET
RTENTRY(Msubd)
	FMOVEDIN
	fsubd	a0@,fp0
	FMOVEDOUT
	RET
RTENTRY(Mmuld)
	FMOVEDIN
	fmuld	a0@,fp0
	FMOVEDOUT
	RET
RTENTRY(Mdivd)
	FMOVEDIN
	fdivd	a0@,fp0
	FMOVEDOUT
	RET
RTENTRY(Mscaleid)
	FMOVEDIN
	fscalel a0@,fp0
	FMOVEDOUT
	RET

RTENTRY(Mmode)
	movel	d0,d1
	fmovel	fpcr,d0
	fmovel	d1,fpcr
	RET

RTENTRY(Mstatus)
	movel	d0,d1
	fmovel	fpsr,d0
	fmovel	d1,fpsr
	RET

RTENTRY(Mcmps)
	fmoves	d0,fp0
	fcmps	d1,fp0
	fmovel	fpsr,d0
	roll	#8,d0
	andw	#0xf,d0
	lsll	#1,d0
	movew	pc@(comparetable:b,d0:W),cc
	RET

comparetable:
	.word	FGT,FUN,FGT,FUN
	.word	FEQ,FUN,FEQ,FUN
	.word	FLT,FUN,FLT,FUN
	.word	FEQ,FUN,FEQ,FUN

RTENTRY(Mcmpd)
	FMOVEDINC
	fcmpd	a0@,fp0
	fmovel	fpsr,d0
	roll	#8,d0
	andw	#0xf,d0
	lsll	#1,d0
	movew	pc@(comparetable:b,d0:W),cc
	RET

