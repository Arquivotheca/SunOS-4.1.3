/*	@(#)vm_mp.c	1.1	92/07/30	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * VM - multiprocessor/ing support.
 *
 * Currently the kmon_enter() / kmon_exit() pair implements a
 * simple monitor for objects protected by the appropriate lock.
 * The kcv_wait() / kcv_broadcast pait implements a simple
 * condition variable which can be used for `sleeping'
 * and `waking' inside a monitor if some resource is
 * is needed which is not available.
 *
 * XXX - this code is written knowing about the semantics
 * of sleep/wakeup and UNIX scheduling on a uniprocessor machine.
 */


#ifdef KMON_DEBUG

#include <sys/param.h>

#include <vm/mp.h>

#define	ISLOCKED	0x1
#define	LOCKWANT	0x2

/*
 * kmon_enter is used as a type of multiprocess semaphore
 * used to implement a monitor where the lock represents
 * the ability to operate on the associated object.
 * For now, the lock/object association is done
 * by convention only.
 */
void
kmon_enter(lk)
	kmon_t *lk;
{
	int s;

	s = spl6();
	while ((lk->dummy & ISLOCKED) != 0) {
#ifdef notnow
		lk->dummy |= LOCKWANT;
		(void) sleep((char *)lk, PSWP+1);
#else notnow
		panic("kmon_enter");
#endif notnow
	}
	lk->dummy |= ISLOCKED;
	(void) splx(s);
}

/*
 * Release the lock associated with a monitor,
 * waiting up anybody that has already decided
 * to wait for this lock (monitor).
 */
void
kmon_exit(lk)
	kmon_t *lk;
{
	int s;

	if ((lk->dummy & ISLOCKED) == 0)	/* paranoid */
		panic("kmon_exit not locked");

	s = spl6();
	lk->dummy &= ~ISLOCKED;
	if ((lk->dummy & LOCKWANT) != 0) {
		lk->dummy &= ~LOCKWANT;
		wakeup((char *)lk);
	}
	(void) splx(s);
}

/*
 * Wait for the named condition variable.
 * Must already have the monitor lock when kcv_wait is called.
 */
void
kcv_wait(lk, cond)
	kmon_t *lk;
	char *cond;
{
	int s;

	if ((lk->dummy & ISLOCKED) == 0)	/* paranoia */
		panic("kcv_wait not locked");

	s = spl6();
	lk->dummy &= ~ISLOCKED;			/* release lock */

	(void) sleep(cond, PSWP+1);

	if ((lk->dummy & ISLOCKED) != 0)	/* more paranoia */
		panic("kcv_wait locked");

	lk->dummy |= ISLOCKED;			/* reacquire lock */
	(void) splx(s);
}

/*
 * Wake up all processes waiting on the named condition variable.
 *
 * We just use current UNIX sleep/wakeup semantics to delay the actual
 * context switching until later after we have released the lock.
 */
void
kcv_broadcast(lk, cond)
	kmon_t *lk;
	char *cond;
{

	if ((lk->dummy & ISLOCKED) == 0)
		panic("kcv_broadcast");
	wakeup(cond);
}
#endif /* !KMON_DEBUG */
