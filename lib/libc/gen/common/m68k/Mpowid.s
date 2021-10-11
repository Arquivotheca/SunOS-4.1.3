	.data
|	.asciz	"@(#)Mpowid.s 1.1 92/07/30 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Mdefs.h"

RTENTRY(Mpowid)
	tstl	a0@
	bnes	6f		| Branch if i <> 0.
	movel	#0x3ff00000,d0	| Return 1.0 = x**0.
	clrl	d1
	bras	3f
6:
	FMOVEDIN		| fp0 gets x.
	movel	a0@,d0		| d0 gets integer power i.
	bpls	1f
	negl	d0		| d0 gets abs(i).
	bras	1f

dpowerloop:
	lsrl	#1,d0
	fmulx	fp0,fp0		| If there are n trailing 0's,
				| this loop computes fp0 = x**2**n. 
1:
	btst	#0,d0
	beqs	dpowerloop
	
	fmovex	fp0,fp1		| fp1 gets x**2**n.
	bras	6f

dmultloop:
	fmulx	fp1,fp1		| Square fp1.
	btst	#0,d0
	beqs	6f
	fmulx	fp1,fp0		| If a one bit, multiply result by fp1.
6:
	lsrl	#1,d0
	bnes	dmultloop	| Branch if there are more 1 bits.
5:
	tstl	a0@
	bpls	2f
	fmovecr	#0x32,fp1	| fp1 gets 1.0.
	fdivx 	fp0,fp1		| fp1 gets 1/x**|i|.
	fmovex	fp1,fp0		| Result is x**i.
2:	
	FMOVEDOUT		| Result is x**i.
3:
	RET
