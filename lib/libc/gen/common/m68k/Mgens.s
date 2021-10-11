        .data
|        .asciz  "@(#)Mgens.s 1.1 92/07/30 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Mdefs.h"

RTENTRY(Manints)
	fmovel	fpcr,sp@-	| Save old modes.
	movel	sp@,d1
	andb	#ROUNDMASK,d1
	orb	#RNEAREST,d1
	fmovel	d1,fpcr		| Force round toward nearest.
	fabss	d0,fp1		| fp1 := abs(x).
	fintx	fp1,fp0		| fp0 := arint(abs(x))=abs(arint(x)).
	fsubx	fp0,fp1		| fp1 := abs(x) - arint(abs(x)).
	fcmps	#0r0.5,fp1	| 
	fbneq	1f		| Branch if difference <> +0.5.
	faddl	#1,fp0		| Force biased round upward.
1:
	tstl	d0
	bpls	2f		| Branch if x was not negative sign.
	fnegx	fp0,fp0		| Negate result.
2:
	fmoves	fp0,d0
	fmovel	sp@+,fpcr	| Restore old modes.
	RET

RTENTRY(Maints)
	fintrzs	d0,fp0
	fmoves	fp0,d0
	RET

RTENTRY(Marints)
	fints	d0,fp0
	fmoves	fp0,d0
	RET

RTENTRY(Mceils)
	fmovel	fpcr,d1
	movel	d1,sp@-
	andb	#ROUNDMASK,d1
	orb	#RPLUS,d1
	fmovel	d1,fpcr
	fints	d0,fp0
	fmoves	fp0,d0
	fmovel	sp@+,fpcr
	RET
RTENTRY(Mfloors)
	fmovel	fpcr,d1
	movel	d1,sp@-
	andb	#ROUNDMASK,d1
	orb	#RMINUS,d1
	fmovel	d1,fpcr
	fints	d0,fp0
	fmoves	fp0,d0
	fmovel	sp@+,fpcr
	RET

RTENTRY(Msqrts)
	fsqrts	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Mlength2s)
	fmoves	d0,fp0
	fmulx	fp0,fp0
	fmoves	d1,fp1
	fmulx	fp1,fp1
	faddx	fp1,fp0
	fsqrtx	fp0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Mmods)
	fmoves	d0,fp0
	fmods	d1,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Mrems)
	fmoves	d0,fp0
	frems 	d1,fp0
	fmoves	fp0,d0
	RET
