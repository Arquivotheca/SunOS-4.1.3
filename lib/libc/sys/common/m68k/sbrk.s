/* @(#)sbrk.s 1.1 92/07/30 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

#define	SYS_brk		17

	.globl	curbrk
	.globl	_end
	.data
curbrk:	.long	_end
	.text

ENTRY(sbrk)
#if vax
	addl3	curbrk,4(ap),-(sp)
	pushl	$1
	movl	ap,r3
	movl	sp,ap
	chmk	$SYS_brk
	jcs 	err
	movl	curbrk,r0
	addl2	4(r3),curbrk
#endif
#if sun

	movl	PARAM,d0
	addql	#3,d0		| round up request to a multiple of wordsize
	moveq	#~3,d1
	andl	d1,d0
#ifdef PIC
	PIC_SETUP(a0)
	movl	a0@(curbrk:w),a0
	addl	a0@,d0
#else
	addl	curbrk,d0
#endif
	movl	d0,PARAM
	pea	SYS_brk
	trap	#0
	jcs	err
#ifdef PIC
	PIC_SETUP(a0)
	movl	a0@(curbrk:w),a0
	movl	a0@,d0
	movl	PARAM,a0@
#else
	movl	curbrk,d0
	movl	PARAM,curbrk
#endif

#endif
	RET
err:
	CERROR(a1)
