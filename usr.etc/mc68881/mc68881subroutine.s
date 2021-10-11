 	.data
|       .asciz  "@(#)mc68881subroutine.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

|	mc68881version subroutines

	.globl	_clear81
					| Initializes 68881 for default modes.
_clear81:
        fmoveml twozeros,fpcr/fpsr
        rts
twozeros: .long	0,0

|	A93N()  returns 1 if 68881 is an A93N, 0 if not
|	tests loge(1+eps) and denorm/norm boundary

	.globl 	_A93N
_A93N:
	flognd	onepluseps,fp0		| fp0 = loge(1+2**-52) = 2**-52 approx
	fmoves	fp0,d0
	cmpl	#0x25800000,d0
	bnes	2f
	fmoved	justdenorm,fp0
	fmoves	fp0,d0
	cmpl	#0x00800000,d0
	bnes	2f
	moveq	#1,d0
	bras	1f
2:
	clrl	d0
1:
	rts

onepluseps: .long 0x3FF00000,0x00000001	| 1+2^-52
justdenorm: .long 0x380FFFFF,0xFE000000 | Should round to least single normal.

|	M3A93N()  returns 1 if 6888x is an 3A93N, 0 if not
|	tests tricky rounding case

	.globl 	_M3A93N
_M3A93N:
	fmoved	d1p52,fp0
	fsubd	d6143m13,fp0
	fmoved	fp0,sp@-
	moveml	sp@+,d0/d1
	cmpl	d1p52d2,d0
	bnes	2f
	cmpl	d1p52d2+4,d1
	bnes	2f
	moveq	#1,d0
	bras	1f
2:
	clrl	d0
1:
	rts

d1p52:		.long	0x43300000,0		| 2^52
d6143m13:	.long	0x3FE7FF00,0		| 6143m13 = 2**-13 * (2**12 + 2**11 - 1)	
d1p52d2:	.long	0x432fffff,0xfffffffe	| correct result 1p52d2

|	bigrem81 computes a very big remainder 1024 times for timing purposes

	.globl	_bigrem81

_bigrem81:
	movew	#1024,d0
1:
	fmovex	dividend,fp0
	fremx	divisor,fp0
	dbf	d0,1b
	rts

dividend: .long	0x7ffe0000,0xffffffff,0xfffffffe
divisor:  .long 0x00000001,0xffffffff,0xffffffff


| slowsaxpy and fastsaxpy compute the same loop, but fastsaxpy should be
| faster on the 68882.

	.globl	_slowsaxpy
_slowsaxpy:
	link	a6,#-56
	moveml	#12480,sp@
	movl	a6@(8),a5
	movl	a6@(12),a4
	moveq	#0,d7
	fmoved	a6@(16),fp7
	movl	a6@(24),d6
	jra	5f
2:
	fmovex	fp7,fp0
	fmuld	a4@+,fp0
	faddd	a5@,fp0
	fmoved	fp0,a5@+
	fmovex	fp7,fp0
	fmuld	a4@+,fp0
	faddd	a5@,fp0
	fmoved	fp0,a5@+
	addql	#2,d7
5:
	cmpl	d6,d7
	jlt	2b
	moveml	a6@(-56),#12480
	unlk	a6
	rts

	.globl	_fastsaxpy
_fastsaxpy:
	link	a6,#-56
	moveml	#12480,sp@
	movl	a6@(8),a5
	movl	a6@(12),a4
	moveq	#0,d7
	fmoved	a6@(16),fp7
	movl	a6@(24),d6
	fmovex	fp7,fp0
	fmuld	a4@+,fp0
	jra	5f
2:
	faddd	a5@,fp0
	fmovex	fp7,fp1
	fmuld	a4@+,fp1
	fmoved	fp0,a5@+

	faddd	a5@,fp1
	fmovex	fp7,fp0
	fmuld	a4@+,fp0
	fmoved	fp1,a5@+
	
	addql	#2,d7
5:
	cmpl	d6,d7
	jlt	2b
	moveml	a6@(-56),#12480
	unlk	a6
	rts
