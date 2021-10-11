/* @(#)cerror.s 1.1 92/07/30 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

	.globl	_errno
	.text
cerror:
#ifdef PIC  
	PIC_SETUP(a0);
	movl	a0@(_errno:w),a1
	movl	d0,a1@
#else
	movl	d0,_errno
#endif

	moveq	#-1,d0
	RET
