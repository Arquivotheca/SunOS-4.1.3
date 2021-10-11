/* @(#)ptrace.s 1.1 92/07/30 SMI; from UCB 4.2 83/06/30 */

#include "SYS.h"

#define	SYS_ptrace	26

ENTRY(ptrace)
#ifdef PIC
	PIC_SETUP(a0)
	movl	a0@(_errno:w),a0
	clrl	a0@
#else
	clrl	_errno
#endif

#if vax
	chmk	$SYS_ptrace
#endif
#if sun
	pea	SYS_ptrace
	trap	#0
#endif
	jcs	err
	RET
err:
	CERROR(a0)
