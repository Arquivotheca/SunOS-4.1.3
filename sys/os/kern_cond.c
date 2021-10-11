#ifndef lint
static char     sccsid[] = "@(#)kern_cond.c 1.1 92/07/30 SMI";
#endif

#include <sys/param.h>

#ifdef	MULTIPROCESSOR

#include <os/cond.h>

/*
 * Semi-Portable Condition Variable stuff
 *
 * SunOS 4.1 LWP mechanism prevents us from using the "cv_" prefix.
 */

/*
 * cond_init: initialize a condition variable.
 */
void
cond_init(cp, lvl)
	cond_t         *cp;
	int             lvl;
{
	mutex_init(cp->lock, lvl);
	mutex_enter(cp->lock);
	cp->waits = cp->waiting = cp->sigs = cp->broads = 0;
	mutex_exit(cp->lock);
}

/*
 * cond_wait: like MT "cv_wait"
 *
 * Sleep on the condition variable and release the mutex atomicly.
 * if sleep returns without error, reacquires the mutex.
 * returns whatever the sleep returns.
 */
int
cond_wait(cp, mp, pri)
	cond_t         *cp;	/* condition variable */
	mutex_t        *mp;	/* mutex to be released */
	int             pri;	/* sleep priority */
{
	int             rv;

	mutex_enter(cp->lock);
	cp->waits++;
	cp->waiting++;
	mutex_exit(cp->lock);
	if (!(rv = sleepexit((caddr_t) cp, pri, mp)))
		mutex_enter(mp);
	return rv;
}

/*
 * cond_signal: like MT "cv_signal"
 *
 * Wake up one person sleeping on the condition variable. Goes with cond_wait.
 */
void
cond_signal(cp)
	cond_t         *cp;
{
	mutex_enter(cp->lock);
	if (cp->waiting > 0) {
		wakeup_one((caddr_t) cp);
		cp->sigs++;
		cp->waiting --;
	}
	mutex_exit(cp->lock);
}

/*
 * cond_signal: like MT "cv_signal"
 *
 * Wake up everybody sleeping on the condition variable. Goes with cond_wait.
 *
 * SunOS 4.1 LWP mechanism prevents us from using the "cv_" prefix.
 */
void
cond_broadcast(cp)
	cond_t         *cp;
{
	mutex_enter(cp->lock);
	wakeup((caddr_t) cp);
	cp->broads++;
	cp->waiting = 0;
	mutex_exit(cp->lock);
}

void
cond_dump(s, cp)
	char   	       *s;
	cond_t	       *cp;
{
	extern void mutex_dump();
	mutex_dump(s, cp->lock);
	romprintf("%s cond had %d calls (%d pending), %d signals and %d broadcasts\n",
		  s, cp->waits, cp->waiting, cp->sigs, cp->broads);
}

#endif

