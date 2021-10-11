!
!	"@(#)fork.s 1.1 92/07/30"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
	.seg	"text"

#include "SYS.h"

/*
 * pid = fork();
 *
 * r1 == 0 in parent process, r1 == 1 in child process.
 * r0 == pid of child in parent, r0 == pid of parent in child.
 */

	SYSCALL(fork)
	tst	%o1		! test for child
	bnz,a	1f
	clr	%o0		! child, return (0)
1:
	RET			! parent, return (%o0 = child pid)
