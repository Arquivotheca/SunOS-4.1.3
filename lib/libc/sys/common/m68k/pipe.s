/* @(#)pipe.s 1.1 92/07/30 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

SYSCALL(pipe)
#if vax
	movl	4(ap),r2
	movl	r0,(r2)+
	movl	r1,(r2)
	clrl	r0
#endif
#if sun
	movl	PARAM,a0
	movl	d0,a0@+
	movl	d1,a0@
	clrl	d0
#endif
	RET
