/* @(#)plock.c 1.1 92/07/30 */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * plock - lock "segments" in physical memory.
 *
 * Supports SVID-compatible plock, taking into account dynamically linked
 * objects (such as shared libraries).
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/lock.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <machine/param.h>
#include <machine/vmparam.h>
#include <a.out.h>
#include <link.h>
#include <errno.h>

/*
 * Globals we reference.
 */
extern	struct link_dynamic _DYNAMIC;
extern	int mlock();
extern	int munlock();

/*
 * Module-scope variables.
 */
static	int page_size = 0;		/* cached result of getpagesize() */
static	int lock_state = 0;		/* lock state */
static	int state_pid = -1;		/* pid to which state belongs */

/*
 * Local worker routine to lock text and data segments.  Handles
 * dynamically loaded objects.  This routine is highly dependent
 * on executable format and layout.
 */
static int
apply_lock(op, f)
	int op;				/* desired operation */
	int (*f)();			/* function to perform */
{
	int	e = 0;			/* return value */
	caddr_t	a;			/* address of operation */
	u_int	l;			/* length of operation */
	struct	link_map *lmp;		/* link map walker */
	struct	exec *eh;		/* exec header */
	extern	caddr_t sbrk();		/* find end of data segment */
	extern	caddr_t etext;		/* end of text segment */

	/*
	 * Operate on application segment first.
	 */
	switch (op) {
	case TXTLOCK:
		a = (caddr_t)USRTEXT;	/* note: old Sun-2 not handled */
		l = (u_int)&etext -  USRTEXT;
		break;
	case DATLOCK:
		a = (caddr_t)(((int)&etext + (SEGSIZ - 1)) & ~(SEGSIZ - 1));
		l = (u_int)(sbrk(0) - a);
		break;
	}
	l = (l + (page_size - 1)) & (u_int)~(page_size - 1);

	/*
	 * Perform the operation -- if failure, return immediately.
	 */
	if (e = (*f)(a, l))
		return (e);

	/*
	 * If we're not a dynamically linked program, we are finished.
	 */
	if (&_DYNAMIC == 0)
		return (0);

	/*
	 * Find the list of dynamically linked objects.  If we get
	 * dynamic linking formats we don't recognize, then punt.
	 */
	switch (_DYNAMIC.ld_version) {
	case 2:
#ifdef sparc
	case 3:
#endif sparc
		lmp = _DYNAMIC.ld_un.ld_2->ld_loaded;
		break;
	default:
		return (0);
	}

	/*
	 * Loop over all objects.  Extract the addresses and lengths as
	 * required, and perform the appropriate operation.
	 */

	while (lmp) {
		eh = (struct exec *)lmp->lm_addr;
		switch (op) {
		case TXTLOCK:
			a = (caddr_t)eh;
			l = (u_int)eh->a_text;
			break;
		case DATLOCK:
			a = (caddr_t)((u_int)eh + N_DATADDR(*eh) -
			    N_TXTADDR(*eh));
			l = (u_int)eh->a_data + (u_int)eh->a_bss;
			break;
		}
		l = (l + (page_size - 1)) & ~(page_size - 1);
		if (e = (*f)(a, l))
			return (e);
		lmp = lmp->lm_next;
	}
	return (0);
}

/*
 * plock 
 */
int
plock(op)
	int op;				/* desired operation */
{
	int 	e = 0;			/* return value */
	int	pid;			/* current pid */
	caddr_t	a1, a2;			/* loop variables */
	struct	rlimit rl;		/* resource limit */

	/*
	 * Initialize static caches.
	 */
	if (page_size == 0)
		page_size = getpagesize();

	/*
	 * Validate state of lock's.  If parent has forked, then
	 * the lock state needs to be reset (children do not inherit
	 * memory locks, and thus do not inherit their state).
	 */
	if ((pid = getpid()) != state_pid) {
		lock_state = 0;
		state_pid = pid;
	}

	/*
	 * Dispatch on operation.  Note: plock and its relatives depend
	 * upon "op" being bit encoded.
	 */
	switch (op) {

	/*
	 * UNLOCK: remove all memory locks.  Requires that some be set!
	 */
	case UNLOCK:
		if (lock_state == 0) {
			errno = EINVAL;
			return (-1);
		}
		if (e = munlockall())
			return (-1);
		else {
			lock_state = 0;
			return (0);
		}
		/*NOTREACHED*/

	/*
	 * TXTLOCK: locks text segments.  
	 */
	case TXTLOCK:

		/*
		 * If a text or process lock is already set, then fail.
		 */
		if ((lock_state & TXTLOCK) || (lock_state & PROCLOCK)) {
			errno = EINVAL;
			return (-1);
		}

		/*
		 * Try to apply the lock(s).  If a failure occurs,
		 * back them out.  On success, remember that a text
		 * lock was set.
		 */
		if (e = apply_lock(op, mlock))
			(void) apply_lock(op, munlock);
		else
			lock_state |= TXTLOCK;
		return (e);
		/*NOTREACHED*/

	/*
	 * DATLOCK: locks data segment(s), including the stack and all
	 * future growth in the address space.
	 */
	case DATLOCK:

		/*
		 * If a data or process lock is already set, then fail.
		 */
		if ((lock_state & DATLOCK) || (lock_state & PROCLOCK)) {
			errno = EINVAL;
			return (-1);
		}

		/*
		 * Try to lock the data segments.  On failure, back out
		 * the locks and return.
		 */
		if (e = apply_lock(op, mlock)) {
			(void) apply_lock(op, munlock);
			return (-1);
		}

		/*
		 * Try to lock the stack segment.  Find out the extent
		 * and start of the stack (there should be a function for
		 * this!) and then iterate over the pages of the stack
		 * locking them.  The stack *could* be sparely populated.
		 * Ignore lock failures resulting from the absence of a
		 * mapping.
		 */
		(void) getrlimit(RLIMIT_STACK, &rl);
		for (a1 = (caddr_t)USRSTACK - page_size;
		    a1 != (caddr_t)USRSTACK - rl.rlim_cur; a1 -= page_size)
			if (e = mlock(a1, page_size)) {
				if (errno == ENOMEM)
					e = 0;
				break;
			}

		/*
		 * If we were successful in locking the stack, then
		 * try to set a lock for all future mappings.
		 */
		if (!e)
			e = mlockall(MCL_FUTURE);

		/*
		 * If failures have occurred, back out the locks
		 * and return failure.
		 */
		if (e) {
			e = errno;
			(void) apply_lock(op, munlock);
			for (a2 = (caddr_t)USRSTACK - page_size; a2 != a1;
			    a2 -= page_size)
				(void) munlock(a2, page_size);
			errno = e;
			return (-1);
		}

		/*
		 * Data, stack, and growth have been locked.  Set state
		 * and return success.
		 */
		lock_state |= DATLOCK;
		return (0);
		/*NOTREACHED*/

	/*
	 * PROCLOCK: lock everything, and all future things as well.
	 * There should be nothing locked when this is called.
	 */
	case PROCLOCK:
		if (lock_state) {
			errno = EINVAL;
			return (-1);
		}
		if (mlockall(MCL_CURRENT | MCL_FUTURE) == 0) {
			lock_state |= PROCLOCK;
			return (0);
		} else
			return (-1);
		/*NOTREACHED*/

	/*
	 * Invalid operation.
	 */
	default:
		errno = EINVAL;
		return (-1);
		/*NOTREACHED*/
	}
}
