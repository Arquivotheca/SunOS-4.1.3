LL0:
	.data
	.text
| sccsid[] = "@(#)pw_test_set.s 1.1 92/07/30 Copyr 1986 Sun Micro";
|
|
|  Copyright (c) 1986 by Sun Microsystems, Inc.
|
|
|#PROC# 04
|/* return true if bit zero already set. */
|int
|pw_test_set(lock_flags)
|	int	*lock_flags;
|
	.globl	_pw_test_set
_pw_test_set:
	link	a6,#0
	addl	#-LF12,sp
	moveml	#LS12,sp@
	movl	a6@(0x8),a1
	bset	#0,a1@(3)
	jeq	L14
	moveq	#1,d0
	jra	LE12
L14:
	moveq	#0,d0
	jra	LE12
LE12:
	unlk	a6
	rts
	LF12 = 0
	LS12 = 0x0
	LFF12 = 0
	LSS12 = 0x0
	LP12 =	0x8
	.data
