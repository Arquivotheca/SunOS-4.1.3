	.data
|	.asciz	"@(#)rel2support.s 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
        
#include "fpcrtdefs.h"
	
/*	OBSOLETE FEATURES FROM RELEASE 2.X	*/
#ifdef PIC
#define JJMP(label,free_reg) \
	movel	free_reg,sp@-; \
	JBSR(label,free_reg); \
	movel	sp@+,free_reg; \
	RET
#else
#define JJMP(label,dummy) \
	jmp 	label
#endif


OBSOLETE(Fsinglei)
	JJMP(Fdtos,a2)
OBSOLETE(Fdoublei)
	JJMP(Fstod,a2)
OBSOLETE(Fflti)
	JJMP(Ffltd,a2)
OBSOLETE(Ffixi)
	JJMP(Fintd,a2)
OBSOLETE(Faddi)
	JJMP(Faddd,a2)
OBSOLETE(Fsubi)
	JJMP(Fsubd,a2)
OBSOLETE(Fmuli)
	JJMP(Fmuld,a2)
OBSOLETE(Fdivi)
	JJMP(Fdivd,a2)
OBSOLETE(Ffltis)
	JJMP(Fflts,a2)
OBSOLETE(Ffixis)
	JJMP(Fints,a2)
OBSOLETE(Faddis)
	JJMP(Fadds,a2)
OBSOLETE(Fsubis)
	JJMP(Fsubs,a2)
OBSOLETE(Fmulis)
	JJMP(Fmuls,a2)
OBSOLETE(Fdivis)
	JJMP(Fdivs,a2)
OBSOLETE(Ssinglei)
	JJMP(Sdtos,a2)
OBSOLETE(Sdoublei)
	JJMP(Sstod,a2)
OBSOLETE(Sflti)
	JJMP(Sfltd,a2)
OBSOLETE(Sfixi)
	JJMP(Sintd,a2)
OBSOLETE(Saddi)
	JJMP(Saddd,a2)
OBSOLETE(Ssubi)
	JJMP(Ssubd,a2)
OBSOLETE(Smuli)
	JJMP(Smuld,a2)
OBSOLETE(Sdivi)
	JJMP(Sdivd,a2)
OBSOLETE(Sfltis)
	JJMP(Sflts,a2)
OBSOLETE(Sfixis)
	JJMP(Sints,a2)
OBSOLETE(Saddis)
	JJMP(Sadds,a2)
OBSOLETE(Ssubis)
	JJMP(Ssubs,a2)
OBSOLETE(Smulis)
	JJMP(Smuls,a2)
OBSOLETE(Sdivis)
	JJMP(Sdivs,a2)

OBSOLETE(_vfloat_)
OBSOLETE(_vfunc_)
	JJMP(Vswitch,a2)

OBSOLETE(_ffloat_)
OBSOLETE(_ffunc_)
	JJMP(Fswitch,a2)

OBSOLETE(skyvector)
OBSOLETE(_sfloat_)
OBSOLETE(_sfunc_)
	JJMP(Sswitch,a2)

OBSOLETE(__skyinit)
	JJMP(Sinit,a2)

/*	OLD STYLE COMPARE ROUTINES */

/*
 *	ieee single floating compare
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 29 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0
 *	    second argument in d1
 *	exit conditions:
 *	    result in condition code
 *	    d0/d1 trashed
 *
 *	register conventions:
 *	    d0		operand 1
 *	    d1		operand 2
 *	    d2		scratch
 */
	NSAVED	= 4
	CODE	= NSAVED

OBSOLETE(Fcmpis)
OBSOLETE(Scmpis)
RTOBSOLETE(fvcmpis)
#ifdef PROF
	unlk	a6		| don't get in the way of the cc.
#endif
	subqw	#2,sp		| save space for condition code return
	movl	d2,sp@-		| save register

	movl	d1,d2
	andl	d0,d2		| compare signs
	|bmis	nbothmi
	bpls	1$
	exg	d0,d1		| both minus
1$:	cmpl	d1,d0		| main compare
	andb	#0xe,cc		| clear carry
	movw	cc,sp@(CODE)
	lsll	#1,d0
	lsll	#1,d1
	cmpl	d1,d0
	bccs	2$
	exg	d0,d1	| find larger
2$:	cmpl	#0xff000000,d0
	blss	3$
	| nan
	movw	#1,sp@(CODE)	| c for unordered
3$:	tstl	d0
	bnes	4$
	movw	#4,sp@(CODE)	| -0 == 0
	| result is in sp@(CODE)
4$:	| restore saved register and go
	movl	sp@+,d2
	rtr


/*
 *	ieee double floating compare
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 30 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0/d1
 *	    second argument on stack
 *	exit conditions:
 *	    result in cc -- carry flag set if either a NAN
 *	problems:
 *	    unordered cases (e.e.: projective infinities and NANs)
 *	    produce random results.
 *	    A NAN, however, does compare not equal to anything.
 *
 *	register conventions:
 *	    d0/d1	first operand
 *	    d2/d3	second operand
 *	    d4		scratch
 */

	ARG2PTR = a0
	SAVEMASK = 0x3800	| registers d2-d4
	RESTMASK = 0x1c
	NSAVED   = 3*4		| 6 registers * sizeof(register)
	CODE	 = NSAVED

OBSOLETE(Fcmpi)
OBSOLETE(Scmpi)
RTOBSOLETE(fvcmpi)
	subqw	#2,sp	| save room for result
|	save registers and load operands into registers
	moveml	#SAVEMASK,sp@-
	movl	ARG2PTR@+,d2
	movl	ARG2PTR@ ,d3
	| we are now set up.
	movl	d2,d4
	andl	d0,d4		| compare signs
	|bmis	nbothmi
	bpls	nbothmi
	exg	d0,d2		| both minus
	exg	d1,d3
nbothmi:cmpl	d2,d0		| main compare
	bnes	gotcmp		| got the answer
	movl	d1,d4
	subl	d3,d4		| compare lowers
	beqs	gotcmp		| entirely equal
	roxrl	#1,d4
	andb	#0xa,cc		| clear z, in case differ by 1 ulp
gotcmp:	andb	#0xe,cc		| clear carry
	movw	cc,sp@(CODE)
	lsll	#1,d0
	lsll	#1,d2
	cmpl	d2,d0
	bccs	4$
	exg	d0,d2		| find larger in magnitude
4$:	cmpl	#0xffe00000,d0
	blss	6$		| no nan
	movw	#1,sp@(CODE)	| c, nz
	bras	8$		| one was a nan
6$:	orl	d1,d0
	orl	d2,d0
	orl	d3,d0
	bnes	8$
	movw	#4,sp@(CODE)	| -0 == 0
	| done, now go
8$:	moveml	sp@+,#RESTMASK	| put back saved registers
	movw	sp@+,cc		| install condition code
	RET
