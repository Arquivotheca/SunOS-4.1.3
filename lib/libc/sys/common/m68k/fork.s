/* @(#)fork.s 1.1 92/07/30 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

SYSCALL(fork)
#if vax
	jlbc	r1,1f	/* parent, since r1 == 0 in parent, 1 in child */
	clrl	r0
1:
#endif
#if sun
	tstl	d1
	beqs	2$	/* parent, since ...  */
	clrl	d0
2$:
#endif
	RET		/* pid = fork() */
