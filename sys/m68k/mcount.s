	.data
	.asciz	"@(#)mcount.s 1.1 92/07/30 Copyr 1983 Sun Micro"
	.even
	.text

|	Copyright (c) 1983 by Sun Microsystems, Inc.

	.globl	mcount
mcount:
	tstl	_tolimit 	| have we run out?
	jeq	L58
	tstl	_profiling	|are we recursivly called?
	jne	L58
	addl	#1,_profiling
	movl    sp@, d1		| snag our return address
	movl    a6@(4), d0	| snag HIS return address
	subl	_s_lowpc,d0	| subtract out profiling origin
	cmpl	_s_textsize,d0	| is it cool?
	jhi	L61
	lsrl	#1,d0
	andb	#0xfe,d0	| make sure halfword subscript even
	movl	_froms,a1
	addl	d0,a1		| frompc = &froms[(frompc-s_lowpc)>>1]
	tstw	a1@
	jne	L62
	movl	_tos,a0		| bucket chain is empty.
	addqw	#1,a0@(8)
	moveq	#0,d0
	movw	a0@(8),d0
	movw	d0,a1@
	cmpl	_tolimit,d0	| are there too many now?
	jcc	L64
	mulu	#10,d0
	addl	d0,a0		| form address of bucket.
	movl	d1,a0@		| callee address here
	movl	#1,a0@(4)	| called once.
	clrw	a0@(8)		| no links.
	jra	L61		| all done.
L62:
	movw	a1@,d0		| we've seen this caller before
	mulu	#10,d0
	addl	_tos,d0
	movl	d0,a0		| top = &tos[ *frompc ]
L65:
	| 			  we now have a list of who our caller has
	|			  called. If we're on it, just bump our count.
	|			  Else add us to the list.
	movl	a0@,d0
	cmpl	d1,d0
	jne	L69
	addql	#1,a0@(4)	| found !
	jra	L61
L69:
	tstw	a0@(8)
	jne	L70
	movl	_tos,a1		| fell off the end of the list -- add us.
	moveq	#0,d0
	movw	a1@(8),d0
	addqw	#1,d0
	movw	d0,a1@(8)
	movw	a1@(8),a0@(8)
	movw	a0@(8),d0
	cmpl	_tolimit,d0	| too many buckets?
	jcc	L64
	mulu	#10,d0
	addl	_tos,d0
	movl	d0,a0
	movl	d1,a0@		| callee pc
	movl	#1,a0@(4)	| count of times called.
	clrw	a0@(8)		| link to next.
	jra	L61		| finished.
L70:
	movw	a0@(8),d0	| loop-the-loop
	mulu	#10,d0
	addl	_tos,d0
	movl	d0,a0
	jra	L65
L61:
	subl	#1,_profiling
L58:
	rts			| go home
L64:
	clrl	_tolimit	| oops!
	pea	L72		| message
	jsr	_printf
	addw	#4,sp
	rts
	.data
L72:
	.asciz	"mcount: tos overflow\12"
