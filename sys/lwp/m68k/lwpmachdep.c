/* Copyright (C) 1986 Sun Microsystems Inc. */

#include <lwp/common.h>
#include <lwp/alloc.h>
#include <lwp/queue.h>
#include <machlwp/machdep.h>
#include <lwp/lwperror.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/monitor.h>
#include <machine/psl.h>
#ifndef lint
SCCSID(@(#) lwpmachdep.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * lwp_setregs(tid, machstate)
 * lwp_getregs(tid, machstate)
 */

/*
 * The routines here, in except.c, in machsig.c, in lwputil.c,
 * and those in low.s are machine-dependent parts that need to be changed
 * when porting to a different environment.
 * alloc.[ch] may also require modification.
 */


stkalign_t *__NuggetSP;			/* nugget stack */


/* generally useful printf for debugging low.s */
/* VARARGS0 */
/* ARGSUSED */
void
__asmdebug(a1, a2, a3, a4, a5, a6)
	int a1, a2, a3, a4, a5, a6;
{
	LWPTRACE(tr_ASM,
	    1, ("ASMDEBUG: %d %d %d %d %d %d\n", a1, a2, a3, a4, a5, a6));
}

/* dump the stack pointer. trick: not called with any args. */
/* VARARGS0 */
/* ARGSUSED */
void
__dumpsp(retadr)
	unsigned int *retadr;
{
#ifdef lint
	retadr = retadr;
#endif lint
	LWPTRACE(tr_ASM, 1, ("stack is %x\n", (int) &retadr));
}


/*
 * Create a stack for the nugget to use. This should not come from
 * paged memory.
 * Create a stack for interrupts to operate on.
 */
void
__init_machdep()
{
	__NuggetSP = (stkalign_t *)
	    STACKALIGN(MEM_ALLOC(NUGSTACKSIZE) + NUGSTACKSIZE);
}
