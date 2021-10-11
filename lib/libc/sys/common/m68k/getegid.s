/* @(#)getegid.s 1.1 92/07/30 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

PSEUDO(getegid,getgid)
#if vax
	movl	r1,r0
#endif
#if sun
	movl	d1,d0
#endif
	RET		/* egid = getegid(); */
