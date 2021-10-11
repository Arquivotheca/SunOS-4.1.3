	.data
|	.asciz	"	@(#)gmcount.s	1.1	92/07/30	Copyr 1986 Sun Micro"
|	Copyright (c) 1986 by Sun Microsystems, Inc.
_profiling:
	.byte	3
	.text
	.globl	mcount
mcount:
	tstb	_profiling	|are we recursivly called?
	jne	2$
	movb	#1,_profiling
	movl    sp@, d1		| snag our return address
	movl    a6@(4), d0	| snag HIS return address
	subl	_s_lowpc,d0	| subtract out profiling origin
	cmpl	_s_textsize,d0	| is it cool?
	jhi	3$
	andb	#0xfe,d0	| make sure halfword subscript even
	movl	_froms,a1
	addl	d0,a1		| frompc = &froms[(frompc-s_lowpc)/(HASHFRACTION*sizeof(*froms))]
	tstw	a1@
	jne	4$
	movl	_tos,a0		| bucket chain is empty.
	addqw	#1,a0@(8)
	moveq	#0,d0
	movw	a0@(8),d0
	movw	d0,a1@
	cmpl	_tolimit,d0	| are there too many now?
	jcc	overflow
	mulu	#10,d0
	addl	d0,a0		| form address of bucket.
	movl	d1,a0@		| callee address here
	movl	#1,a0@(4)	| called once.
	clrw	a0@(8)		| no links.
	jra	3$		| all done.
4$:
	movw	a1@,d0		| we've seen this caller before
	mulu	#10,d0
	addl	_tos,d0
	movl	d0,a0		| top = &tos[ *frompc ]
5$:
	| 			  we now have a list of who our caller has
	|			  called. If we're on it, just bump our count.
	|			  Else add us to the list.
	movl	a0@,d0
	cmpl	d1,d0
	jne	6$
	addql	#1,a0@(4)	| found !
	jra	3$
6$:
	tstw	a0@(8)
	jne	7$
	movl	_tos,a1		| fell off the end of the list -- add us.
	movw	a1@(8),d0
	addqw	#1,d0
	movw	d0,a1@(8)
	movw	a1@(8),a0@(8)
	moveq	#0,d0
	movw	a0@(8),d0
	cmpl	_tolimit,d0	| too many buckets?
	jcc	overflow
	mulu	#10,d0
	addl	_tos,d0
	movl	d0,a0
	movl	d1,a0@		| callee pc
	movl	#1,a0@(4)	| count of times called.
	clrw	a0@(8)		| link to next.
	jra	3$		| finished.
7$:
	movw	a0@(8),d0	| loop-the-loop
	mulu	#10,d0
	addl	_tos,d0
	movl	d0,a0
	jra	5$
3$:
	clrb	_profiling
2$:
	rts			| go home
overflow:
	.data1
8$:
	.asciz	"mcount: tos overflow\12"
	.text
	pea	22:w	| sizeof message
	pea	8$	| message
	pea	2:w	| stderr
	jsr	_write
	addw	#12,sp
	rts
