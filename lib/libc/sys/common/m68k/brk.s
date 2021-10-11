/* @(#)brk.s 1.1 92/07/30 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

#define	SYS_brk		17

	.globl	curbrk

SYSCALL(brk)
#ifdef vax
	movl	4(ap),curbrk
#endif
#ifdef sun
#ifdef PIC
	PIC_SETUP(a0)
	movl	a0@(curbrk:w),a1
	movl	PARAM,a1@
#else
	movl	PARAM,curbrk
#endif
#endif
	RET
