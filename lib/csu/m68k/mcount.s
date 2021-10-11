	.data
	.asciz	"	@(#)mcount.s	1.1	92/07/30	Copyr 1983 Sun Micro"
|	Copyright (c) 1983 by Sun Microsystems, Inc.
	.even
_profiling:
	.long	3
_countbase:
	.long	0
_numctrs:
	.long	0
_cntrs:
	.long	0
	.text
	.globl	mcount
mcount:
	tstl	_profiling | check recursion
	jne	3$
	movl	#1,_profiling
	tstl	a0@	   | have we passed this way before?
	jne	2$
	tstl	_countbase | no: check initialization
	jeq	1$
	movl	_cntrs,d0  | is there space left?
	addql	#1,_cntrs
	cmpl	_numctrs,d0
	jeq	4$	   | no space in the place
	movl	_countbase,a1
	movl	sp@,a1@	   | snag return address
	addql	#4,a1
	movl	a1,a0@	   | initialize indirect cell with count cell address
	addql	#1,a1@	   | then increment count cell
	addql	#8,_countbase
1$:
	clrl	_profiling
3$:
	rts
2$:
	movl	a0@,a0     | we've been here before:
	addql	#1,a0@     | just increment indirect
	clrl	_profiling
	rts
4$:
	.data1
5$:
	.ascii	"mcount\72 counter overflow\12\0"
	.text
	pea	26:w
	pea	5$
	pea	2:w
	jsr	_write
	addw	#12,sp
	rts
