/ 
/	.data
/	.asciz	"@(#)_swapFLAGS.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl  _swapTE
	.globl  _swapEX
	.globl  _swapRD
	.globl  _swapRP
_swapTE:
/ i386 Control Word
/ bit 0 - invalid mask
/ bit 1 - denormalize mask
/ bit 2 - zero divide mask
/ bit 3 - overflow mask
/ bit 4 - underflow mask
/ bit 5 - inexact mask
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp
	fnstcw	-4(%ebp)
	movw	-4(%ebp),%cx
	movw	%cx,%ax		/ ax = cw
	orw	$0x3f,%cx	/ cx = mask off all exception
	movw	8(%ebp),%dx	/ dx = input TRAP ENABLE value te
	andw	$0x3f,%dx	/ make sure bitn>5 is zero
	xorw	%dx,%cx		/ turn off the MASK bit accordingly
	movw	%cx,-8(%ebp)
	fldcw	-8(%ebp)	/ load new cw 
	andw	$0x3f,%ax	/ old cw exception MASK info
	xorw	$0x3f,%ax	/ return exception TRAP info
	leave
	ret

_swapEX:
/ i386 Status Word
/ bit 0 - invalid
/ bit 1 - denormalize 
/ bit 2 - zero divide
/ bit 3 - overflow
/ bit 4 - underflow
/ bit 5 - inexact
	pushl	%ebp
	movl	%esp,%ebp
	fnstsw	%ax		/ ax = sw
	movw	8(%ebp),%cx	/ cx = input ex
	andw	$0x3f,%cx
	cmpw	$0,%cx
	jne	L1
				/ input ex=0, clear all exception
	fnclex	
	andw	$0x3f,%ax
	leave
	ret
L1:
				/ input ex !=0, use fnstenv and fldenv
	subl	$0x70,%esp
	fnstenv	-0x70(%ebp)
	movw	%ax,%dx
	andw	$0xffc0,%dx
	orw	%dx,%cx
	movw	%cx,-0x6c(%ebp)	/ replace old sw by a new one (need to verify)
	fldenv	-0x70(%ebp)
	andw	$0x3f,%ax
	leave
	ret


_swapRP:
/ 00 - 24 bits
/ 01 - reserved
/ 10 - 53 bits
/ 11 - 64 bits
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp
	fstcw	-4(%ebp)
	movw	-4(%ebp),%ax
	andw	$0xfcff,-4(%ebp)
	movw	8(%ebp),%dx
	andw	$0x3,%dx
	shlw	$8,%dx
	orw	%dx,-4(%ebp)
	fldcw	-4(%ebp)
	shrw	$8,%ax
	andw	$0x3,%ax
	leave
	ret

_swapRD:
/ 00 - Round to nearest or even
/ 01 - Round down
/ 10 - Round up
/ 11 - Chop

	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp
	fstcw	-4(%ebp)
	movw	-4(%ebp),%ax
	andw	$0xf3ff,-4(%ebp)
	movw	8(%ebp),%dx
	andw	$0x3,%dx
	shlw	$10,%dx
	orw	%dx,-4(%ebp)
	fldcw	-4(%ebp)
	shrw	$10,%ax
	andw	$0x3,%ax
	leave
	ret
