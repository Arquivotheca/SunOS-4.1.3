#ifndef lint
static char     sccsid[] = "@(#)kern_mutex.c 1.1 92/07/30 SMI";
#endif

#include <sys/param.h>

#ifdef	MULTIPROCESSOR

#include <os/mutex.h>

#ifndef	ipltospl	/* from psl.h */
#define	ipltospl(x)	(((x)&0xf) << 8)
#endif

extern int	xc_runsplr;
/*
 * Semi-Portable MUTEX lock stuff
 * Requires the "atom" package.
 */

/*
 * mutex_init: initialize a mutual exclusion structure.
 * Must leave the structure unlocked and all other fields
 * initialized to sane values. "type" is ignored, but "arg"
 * should be either a NULL or a pointer to an SPL-ish function
 * that gets us to the right level to protect against an interrupt
 * that might also want to acquire the mutex.
 *
 * Contents of the structure:
 *	lock	word-long lock field. Last byte is nonzero if
 *		the mutex has been acquired; when released,
 *		the entire word is cleared.
 *	waits	sum of the time that people spent waiting
 *		for the lock to be released, most useful if
 *		uniqtime is microsecond precise.
 */
void
mutex_init(mp, pri)
	mutex_t        *mp;
	int             pri;	/* how high to spl */
{
	atom_init(mp->atom);
	mp->missct = 0;
	mp->spl = ipltospl(pri);
}

/*
 * mutex_enter: acquire the lock. When this routine returns,
 * the caller owns the specified lock. Optimized to take a
 * minimum amount of time if the lock is free, and to minimize
 * the MBUS traffic if the lock is busy. 
 *
 * Triffic limitation is done by spinning on testing the
 * atom with loads, not swaps, so the line may become
 * shared by all caches.
 *
 * If several processors are waiting on the lock, we get a burst
 * of mbus traffic at several times:
 *   -	when a new processor joins the group, he first checks
 *	the lock with a swap, causing one CRI followed by N-1
 *	CR transactions. (this goes away if mutex_tryenter uses
 *	a "test and test and set" algorithm).
 *   -	when the lock is freed, we get a CI or CRI followed by
 *	N CR transactions; at worst case, these are followed by
 *	another N CRI transactions as each processor in succession
 *	tries for the lock, followed by N-1 additioan CR transactions
 *	as everyone who did not get the lock goes back to spinning.
 *
 * In this kernel, N is at most NCPU-1, which is currently 3; if this
 * gets up above 8 or so, look at doing something to prevent the above
 * crazy transactions.
 */

void
mutex_enter(mp)
	mutex_t        *mp;
{
	int mc = 0;
	register int s;

	if (mp == (mutex_t *)0)
		return;

	s = splr(mp->spl);
	if (mp->spl >= xc_runsplr)
		xc_defer();
	if (!atom_tas(mp->atom))
		atom_wcsi(mp->atom, &mc);
	mp->missct += mc;
	mp->psr = s;
}

/*
 * mutex_exit: release the lock on the mutex. Do this with
 * a swap instead of a store to guarantee that all writes
 * done before the call to mutex_exit complete before the
 * lock is released.
 */
void
mutex_exit(mp)
	mutex_t        *mp;
{
	register int s;

	if (mp == (mutex_t *)0)
		return;

	s = mp->psr;
	atom_clr(mp->atom);
	if (mp->spl >= xc_runsplr)
		xc_accept();
	(void)splx(s);
}

/*
 * mutex_tryenter: attempt to acquire the lock. Returns true
 * if the lock was free. does not update missct.
 */
int
mutex_tryenter(mp)
	mutex_t        *mp;
{
	register int s;
	register int rv;

	if (mp == (mutex_t *)0)
		return 0;

	s = splr(mp->spl);
	if (mp->spl >= xc_runsplr)
		xc_defer();
	if (rv = atom_tas(mp->atom))
		mp->psr = s;
	else {
		if (mp->spl >= xc_runsplr)
			xc_accept();
		(void)splx(s);
	}
	return rv;
}

void
mutex_dump(s, mp)
	char   	       *s;
	mutex_t	       *mp;
{
	extern char *atom_dump();
	romprintf("%s mutex %s, with %d miss cycles; splr(0x%x) on lock, splx(0x%x) at next free\n",
		  s, atom_dump(mp->atom), mp->missct, mp->spl, mp->psr);
}
#endif
