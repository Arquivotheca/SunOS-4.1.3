	.data
|	.asciz	"@(#)float_switch.s 1.1 92/07/30 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

| Vlist is used to construct a run-time
| list of switched entry points that are
| actually used in this program.  
| By not listing all Vxxx entry points
| we avoid linking in those that are not used.

| Switched entry points arise in libc/crt, libc/gen, and libm.

	.globl	Vlist
	.lcomm	Vlist,127*4 		| Room for 127 switched entry points.

	.text

RTENTRY(float_switch)
	moveml	a0/a1/d0/d1,sp@-
#ifdef PIC
	PIC_SETUP(a0)
	movl	a0@(_fp_switch:w),a0
	movl	a0@,d0
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
5:	
#ifdef PIC
	PIC_SETUP(a0)
	movl	a0@(Vlist:w),a0
#else
        lea     Vlist,a0		| a0 gets address of list of entry points.
#endif
	bras	2f
1:      
	movel	a1@(0,d0:w),a1@(2)	| Change offset of jmp.
2:
	movel	a0@+,a1			| a1 gets listed entry point.
        cmpl    #0,a1
        bnes    1b			| Branch if more entries on list.
	CLEARCACHE			| Wrote instruction so clear cache.
 	moveml	sp@+,a0/a1/d0/d1
	RET
