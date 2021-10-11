	.data
|	.asciz	"@(#)Sremd.s 1.1 92/07/30 SMI"
	.even
	.text


|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Smodd)
	moveml	d0/d1/d2/a0/a2,sp@-	| Save arguments and scratch.
	JBSR(Fremd,a2)
	movel	sp@,d2			| d2 gets sign of x.
	eorl	d0,d2			| d2 gets sign of x eor sign of remainder r.
	bpls	ok			| Branch if same signs.
	movel	sp@(12),a0		| a0 gets address of y.
	movel	a0@,d2			| d2 gets sign of y.
	eorl	d0,d2			| d2 gets sign of y eor sign of r.
	bpls	sub			| Branch if same signs, implying subtract.
	JBSR(Saddd,a2)			| d0/d1 := r + y.
	bras	ok
sub:
	JBSR(Ssubd,a2)			| d0/d1 := r - y.
ok:
	addql	#8,sp			| Skip old d0/d1.
	moveml	sp@+,d2/a0/a2		| Restore 
	RET

ENTER(Sremd)
#ifdef PIC
	movl	a2,sp@-
	JBSR(Fremd,a2)
	movl	sp@+,a2
	RET
#else
	jmp	Fremd
#endif
