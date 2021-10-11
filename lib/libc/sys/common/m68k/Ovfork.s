/* @(#)Ovfork.s 1.1 92/07/30 SMI; from UCB 4.1 12/21/80 */

/*
 * C library -- vfork
 */

#include "SYS.h"

#define	SYS_vfork	66

/*
 * pid = vfork();
 *
 * r1 == 0 in parent process, r1 == 1 in child process.
 * r0 == pid of child in parent, r0 == pid of parent in child.
 */

#if vax
/*
 * trickery here, due to keith sklower, uses ret to clear the stack,
 * and then returns with a jump indirect, since only one person can return
 * with a ret off this stack... we do the ret before we vfork!
 */

ENTRY(vfork)
	movl	16(fp),r2
	movab	here,16(fp)
	RET
here:
	chmk	$SYS_vfork
	bcc	vforkok
	jmp	verror
vforkok:
	tstl	r1		# child process ?
	bneq	child	# yes
	bcc 	parent		# if c-bit not set, fork ok
.globl	_errno
verror:
	movl	r0,_errno
	mnegl	$1,r0
	jmp	(r2)
child:
	clrl	r0
parent:
	jmp	(r2)
#endif

#if sun
ENTRY(vfork)
	movl	sp@+,a0
	movl	#SYS_vfork,sp@-
	trap	#0
	bcss	verror
vforkok:
	tstl	d1		| child process ?
	jne	child		| yes
	jmp	a0@
.globl	_errno
verror:
#ifdef PIC
	PIC_SETUP(a1)
	movl	a1@(_errno:w),a1
	movl	d0, a1@
#else
	movl	d0,_errno
#endif
	movl	#-1,d0
	jmp	a0@
child:
	clrl	d0
parent:
	jmp	a0@
#endif
