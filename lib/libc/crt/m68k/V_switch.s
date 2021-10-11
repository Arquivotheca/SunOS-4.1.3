	.data
|	.asciz	"@(#)V_switch.s 1.1 92/07/30 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(V_switch)
	moveml	a0/a1/a2/d0/d1,sp@-
6:
#ifdef PIC
	PIC_SETUP(a0)  
	movel a0@(_fp_switch:w), a0
	movel	a0@,d0
#else
	movel	_fp_switch,d0
#endif
	cmpw	#FP_SUNFPA,d0
	bnes	1f
	movw	#WOFFSET,d0
	bras	5f
1:
	cmpw	#FP_MC68881,d0
	bnes	2f
	movw	#MOFFSET,d0
	bras	5f
2:	
	cmpw	#FP_SKYFFP,d0
	bnes	3f
	movw	#SOFFSET,d0
	bras	5f
3:	
	cmpw	#FP_SOFTWARE,d0
	bnes	4f
	movw	#FOFFSET,d0
	bras	5f
4:
	JBSR(Vinit,a1)		| Floating point device not decided yet: go decide.
	bras	6b
5:	
	movel	sp@(20),a0	| a0 gets float_switch return address == Vxxx+6.
	subl	#6,a0		| a0 gets Vxxx.
#ifdef PIC
	PIC_SETUP(a1)
	movl	a1@(Vlist:w),a1
#else
	lea	Vlist,a1	| a1 gets address of list of Vxxx entry points.
#endif
1:
	movel	a1@+,a2		| a2 gets next Vxxx entry point.
	cmpl	a0,a2
	beqs	2f		| Branch if a0 is already in list.
	cmpl	#0,a2
	bnes	1b		| Branch if more entries in list.
	movel	a0,a1@(-4)	| Add this Vxxx entry point to the list.
2:
	movew	#0x4ef9,a0@+	| Convert jsr to jmp.
	movel	a0@(-2,d0:w),a0@ | Place proper address after jmp.
	movel	a0@,sp@(20)	| Place same address for float_switch return.
	CLEARCACHE
	moveml	sp@+,a0/a1/a2/d0/d1
	rts
