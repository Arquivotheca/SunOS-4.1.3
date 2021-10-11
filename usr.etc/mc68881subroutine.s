 	.data
|	.asciz	"@(#)mc68881subroutine.s 1.1 92/07/30 Copyr 1986 Sun Micro"
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
