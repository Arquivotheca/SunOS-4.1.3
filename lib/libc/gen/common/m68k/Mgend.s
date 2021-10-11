        .data
|        .asciz  "@(#)Mgend.s 1.1 92/07/30 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Mdefs.h"

RTENTRY(Maintd)
	moveml	d0/d1,sp@-
	fintrzd	sp@,fp0
	FMOVEDOUT
	RET

RTENTRY(Marintd)
	moveml	d0/d1,sp@-
	fintd	sp@,fp0
	FMOVEDOUT
	RET

RTENTRY(Mceild)
	fmovel	fpcr,sp@-	| Save old modes.
	moveml	d0/d1,sp@-
	movel	sp@(8),d0
	andb	#ROUNDMASK,d0
	orb	#RPLUS,d0
	fmovel	d0,fpcr		| Force round.
	fintd	sp@,fp0
	FMOVEDOUT
	fmovel	sp@+,fpcr	| Restore old modes.
	RET

RTENTRY(Mfloord)
	fmovel	fpcr,sp@-	| Save old modes.
	moveml	d0/d1,sp@-
	movel	sp@(8),d0
	andb	#ROUNDMASK,d0
	orb	#RMINUS,d0
	fmovel	d0,fpcr		| Force round.
	fintd	sp@,fp0
	FMOVEDOUT
	fmovel	sp@+,fpcr	| Restore old modes.
	RET

RTENTRY(Manintd)
	fmovel	fpcr,sp@-	| Save old modes.
	moveml	d0/d1,sp@-
	movel	sp@(8),d1
	andb	#ROUNDMASK,d1
	orb	#RNEAREST,d1
	fmovel	d1,fpcr		| Force round toward nearest.
	fabsd	sp@,fp1		| fp1 := abs(x).
	fintx	fp1,fp0		| fp0 := arint(abs(x))=abs(arint(x)).
	fsubx	fp0,fp1		| fp1 := abs(x) - arint(abs(x)).
	fcmps	#0r0.5,fp1	| 
	fbneq	1f		| Branch if difference <> +0.5.
	faddl	#1,fp0		| Force biased round upward.
1:
	tstw	sp@
	bpls	2f		| Branch if x was not negative sign.
	fnegx	fp0,fp0		| Negate result.
2:
	FMOVEDOUT
	fmovel	sp@+,fpcr	| Restore old modes.
	RET

RTENTRY(Msqrtd)
	moveml	d0/d1,sp@-
	fsqrtd	sp@,fp0
	FMOVEDOUT
	RET
RTENTRY(Mlength2d)
	FMOVEDIN
	fmulx	fp0,fp0
	fmoved	a0@,fp1
	fmulx	fp1,fp1
	faddx	fp1,fp0
	fsqrtx	fp0,fp0
	FMOVEDOUT
	RET
RTENTRY(Mmodd)
	FMOVEDIN
	fmodd	a0@,fp0
	FMOVEDOUT
	RET
RTENTRY(Mremd)
	FMOVEDIN
	fremd	a0@,fp0
	FMOVEDOUT
	RET
