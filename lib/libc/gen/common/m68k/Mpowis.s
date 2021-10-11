	.data
|	.asciz	"@(#)Mpowis.s 1.1 92/07/30 Copyr 1986 Sun Micro"
	.even
	.text

|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Mdefs.h"

RTENTRY(Mpowis)
	tstl	d1
	bnes	6f		| Branch if i <> 0.
	movel	#0x3f800000,d0	| Return 1.0 = x**0.
	bras	3f
6:
	fmoves	d0,fp0		| fp0 gets x.
	movel	d1,d0		| d0 gets integer power i.
	bpls	1f
	negl	d0		| d0 gets abs(i).
	bras	1f

powerloop:
	lsrl	#1,d0
	fmulx	fp0,fp0		| If there are n trailing 0's,
				| this loop computes fp0 = x**2**n. 
1:
	btst	#0,d0
	beqs	powerloop
	
	fmovex	fp0,fp1		| fp1 gets x**2**n.
	bras	6f

multloop:
	fmulx	fp1,fp1		| Square fp1.
	btst	#0,d0
	beqs	6f
	fmulx	fp1,fp0		| If a one bit, multiply result by fp1.
6:
	lsrl	#1,d0
	bnes	multloop	| Branch if there are more 1 bits.
5:
	tstl	d1
	bpls	2f
	fmovecr	#0x32,fp1	| fp1 gets 1.0.
	fdivx fp0,fp1		| fp1 gets 1/x**|i|.  Same divide as Mdivs.
	fmoves	fp1,d0		| Result is 1/x**|i|.
	bras	3f
2:	
	fmoves	fp0,d0		| Result is x**i.
3:
	RET
