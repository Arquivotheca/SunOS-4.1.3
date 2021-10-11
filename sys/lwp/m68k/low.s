.data
.asciz "@(#) low.s 1.1 92/07/30"
.even
.text

| Copyright (C) 1986 by Sun Microsystems, Inc.

#include <machlwp/low.h>
#include "assym.s"

.text
.globl	___schedule
.globl 	_splclock
.globl 	_splx
.globl	___asmdebug
.globl	___Curproc
.globl	___NuggetSP


|
| assembly language support routines
|

| a7 is the stack

| __swtch()
| switch context when not at interrupt level and the new process
| was not switched out asynchronously.
| Volatile registers are not restored (d0/d1, a0/a1).
|

ENTRY(swtch)
	jbsr	_splclock
	movl	___Curproc, a0		| pointer to client context
	moveml	a0@(REGOFFSET), #0xfcfc	| restore client regs and new sp
	movl	d0, sp@-
	jbsr	_splx
	addql	#4, sp
1:	rts				| back to client

| __checkpoint()
| Save context and reschedule.
| The context will have the return address of the caller of
| ___checkpoint on the stack so upon rescheduling it will look
| like checkpoint() just returned.

ENTRY(checkpoint)
	jbsr	_splclock
	movl	___Curproc, a0
	moveml	#0xfcfc, a0@(REGOFFSET)	| save volatile registers
	movl	___NuggetSP, sp		| don't use client stack now
	movl	d0, sp@-
	jbsr	_splx
	addql	#4, sp
	jra	___schedule
	| NOTREACHED

| __setstack()
| give the nugget a stack to use when no lwps are running

ENTRY(setstack)
	movl	sp@, a0			| save return address
	movl	___NuggetSP, sp		| switch stacks
	jra	a0@			| ...and return to caller
