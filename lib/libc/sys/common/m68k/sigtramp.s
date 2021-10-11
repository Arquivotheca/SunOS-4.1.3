/*	.asciz	"@(#)sigtramp.s 1.1 92/07/30 Copyr 1985 Sun Micro" */
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "PIC.h"

	.globl	__sigtramp, __sigfunc
__sigtramp:
	moveml	a0/a1/d0/d1,sp@-	/* save C scratch regs */
	JBSR(fp_save,a0)		| Save floating point state.
	movl	sp@(16+FPSAVESIZE),d0	/* get signal number */
	lsll	#2,d0		/* scale for index */
#ifdef PIC
	PIC_SETUP(a0)
	movl	a0@(__sigfunc:w),a0	/* get array of func ptrs */
#else
	movl	#__sigfunc,a0	
#endif
	movl	a0@(0,d0:l),a0	/* get func */
	movl	sp@(0+28+FPSAVESIZE),sp@-	/* push addr */
	movl	sp@(4+24+FPSAVESIZE),sp@-	/* push scp address */
	movl	sp@(8+20+FPSAVESIZE),sp@-	/* push code */
	movl	sp@(12+16+FPSAVESIZE),sp@-	/* push signal number */
	jsr	a0@
	addl	#16,sp		/* pop args */
	JBSR(fp_restore,a0)	| Restore floating point state.
	moveml	sp@+,a0/a1/d0/d1	/* restore regs */
	addl	#8,sp		/* pop signo and code */
	pea	139
	trap	#0
	/*NOTREACHED*/
