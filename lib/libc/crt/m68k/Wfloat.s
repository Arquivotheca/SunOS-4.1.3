
|	@(#)Wfloat.s 1.1 92/07/30 SMI
|       Copyright (c) 1988 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Wdefs.h"
#include "Mdefs.h"

RTENTRY(Wuns)
        cmpl    twoto31,d0
        bges    1f
	fpstol	d0,fpa0
	SREADR0
        bras    2f
1:
	fpmoves	d0,fpa0
	fpsubs	twoto31,fpa0		| Subtract 2**31.
	fpstol	fpa0,fpa0
	fpmoves	fpa0,d0
	bchg	#31,d0			| Add 2**31.
2:
 	RET

twoto31: .long 0x4f000000               | 2**31.
dtwoto31: .double 0r2.147483648000000000E+9 | 2**31

RTENTRY(Wund)
        cmpl    dtwoto31,d0
        bges    1f
	fpdtol	d0:d1,fpa0
	SREADR0
        bras    2f
1:
	fpmoved	d0:d1,fpa0
	fpsubd	dtwoto31,fpa0		| Subtract 2**31.
	fpdtol	fpa0,fpa0
	fpmoves	fpa0,d0
	bchg	#31,d0			| Add 2**31.
2:
 	RET

SMONADIC(Wflts,lto)
SMONADIC(Wsqrs,sqr)

SDYADIC(Wadds,add)
SDYADIC(Wsubs,sub)
SDYADIC(Wmuls,mul)
SDYADIC(Wdivs,div)

RTENTRY(Wints)
	fpstol	d0,fpa0
	SREADR0
	RET

RTENTRY(Wrints)
	fpmoves	d0,fpa0
	fpmove	fpamode,d0
	andl	#0xf,d0		| Clear garbage bits.
	movel	d0,d1
	bclr	#1,d0
	fpmove	d0,fpamode
	fpstol	fpa0,fpa0
	fpmove	d1,fpamode
	SREADR0
	RET

RTENTRY(Wnints)
	SWRITER0		| fpa0 := x.
	bpls	1f
	cmpl	mshalf,d0
	blts	3f
	cmpl	msthreshold,d0
	bges	2f		| No add/sub if big enough.
        fpsubs	shalf,fpa0	| fpa0 := x - 0.5.
	bras	2f
3:
	clrl	d0
	bras	4f
1:
	cmpl	shalf,d0
	blts	3b
	cmpl	sthreshold,d0
	bges	2f		| No add/sub if big enough.
        fpadds	shalf,fpa0	| fpa0 := x + 0.5.
2:
	fpstol	fpa0,fpa0	| fpa0 := nint(x).
	SREADR0
4:
	RET
shalf:	.single  0r0.5
mshalf:	.single  0r-0.5
sthreshold: .single 0r8388608
msthreshold: .single 0r-8388608

RTENTRY(Wcmps)
	SWRITER0
	SDYADOP(cmp)
	READFPACC
	RET

ENTER(Wscaleis)
        fmoves  d0,fp0
        fscalel d1,fp0
        fmoves  fp0,d0
        RET	

RTENTRY(Wstod)
	fpstod	d0,fpa0
	DREADR0
	RET

RTENTRY(Wdtos)
	fpdtos	d0:d1,fpa0
	SREADR0
	RET

RTENTRY(Wfltd)
	fpltod	d0,fpa0
	DREADR0
	RET

DMONADIC(Wsqrd,sqr)

DDYADIC(Waddd,add)
DDYADIC(Wsubd,sub)
DDYADIC(Wmuld,mul)
DDYADIC(Wdivd,div)

RTENTRY(Wintd)
	fpdtol	d0:d1,fpa0
	SREADR0
	RET

RTENTRY(Wrintd)
	fpmoved	d0:d1,fpa0
	fpmove	fpamode,d0
	andl	#0xf,d0		| Clear garbage bits.
	movel	d0,d1
	bclr	#1,d0
	fpmove	d0,fpamode
	fpdtol	fpa0,fpa0
	fpmove	d1,fpamode
	SREADR0
	RET

RTENTRY(Wnintd)
        movel   d1,FPABASEADDRESS+(0xc04)
        movel   d0,FPABASEADDRESS+(0xc00)
	bpls	1f
        cmpl	mdhalf,d0
	blts	3f
	cmpl	mdthreshold,d0
	bges	2f
	fpsubd	dhalf,fpa0
	bras	2f
3:
	clrl	d0
	bras	4f
1:
        cmpl	dhalf,d0
	blts	3b
        cmpl	dthreshold,d0
	bges	2f
        fpaddd	dhalf,fpa0
2:
	fpdtol	fpa0,fpa0		| r0 := int(r0).
	SREADR0
4:
	RET
dhalf:	.double  0r0.5
mdhalf:	.double  0r-0.5
dthreshold: .double 0r4.503599627370496000E+15
mdthreshold: .double 0r-4.503599627370496000E+15

RTENTRY(Wcmpd)
	DWRITER0
	DDYADOP(cmp)
	READFPACC
	RET


ENTER(Wscaleid)
        FMOVEDIN
        fscalel a0@,fp0
        fmoved  fp0,sp@
        moveml  sp@+,d0/d1
        RET

RTENTRY(Wstatus)
        movel   d0,d1
        fmovel  fpsr,d0
        fmovel  d1,fpsr
        btst	#3,d1
	bnes	1f			| Branch if accrued inexact on.
	movel	#1,FPABASEADDRESS+0xf14	| Enable FPA inexact mask.
1:
	RET

RTENTRY(Wmode)
        movel   d0,a0
        fmovel  fpcr,d0
        andl	#0xffffffcf,d0		| Clear round direction.
	fpmove	fpamode,d1		| d1 gets FPA mode.
        andl	#0xc,d1			| Clear all but round direction.
	lslb	#2,d1			| Align for 68881.
	orb	d1,d0			| Insert.
	btst	#5,d0
	beqs	1f			| Branch if rn or rz.
	bchg	#4,d0			| Reverse rm and rp.
1:
	exg	d0,a0			| a0 gets old mode.
					| d0 gets new mode.
	fmovel	d0,fpcr			| Load new mode to 68881.
	btst	#9,d0
	beqs	1f			| Branch if inexact disabled.
	movel	#1,FPABASEADDRESS+0xf14	| Enable FPA inexact mask.
1:
	fpmove	fpamode,d1
	andl	#3,d1			| Clear round direction.
	andl	#0x30,d0		| Get round bits.
	btst    #5,d0  
        beqs    1f                      | Branch if rn or rz.
        bchg    #4,d0                   | Reverse rm and rp.
1:
 	lsrb	#2,d0
	orb	d0,d1
	fpmove	d1,fpamode		| Set FPA mode bits.
	movel	a0,d0			| Set up old mode return value.
	RET

RTENTRY(_fpamode_)
        movel   PARAM,a0       		| Get address of long argument.
        movel   a0@,d1          	| Get long argument.
        fpmove  fpamode,d0
        andl	#0xf,d0			| Clear junk bits.
        andl	#0xf,d1			| Clear junk bits.
	fpmove  d1,fpamode
        RET

|	C entry points for use by fpa_handler and fpa_recompute to avoid circularity
|	of references via winitfp -> fpmode -> Vinit

RTENTRY(_Wmode)
	movel	PARAM,d0
	jsr	Wmode
	RET

RTENTRY(_Wstatus)
	movel	PARAM,d0
	jsr	Wstatus
	RET

