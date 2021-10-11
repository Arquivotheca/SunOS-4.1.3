#ifndef lint
static	char sccsid[] = "@(#)winlock.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1987-1989 Sun Microsystems, Inc.
 */

/*
 * SunWindows locking primitives.
 */

#include <sunwindowdev/wintree.h>

extern	int	ws_check_lock;		/* Check locks every #of hz if pid */
extern	int	ws_no_pid_check_lock;	/* Check locks every #of hz if no pid */
int	ws_set_pri;			/* Reschedule every time set priority */
int	ws_set_nice = 0;		/* Set nice every time lock */
int	ws_nice = NZERO;		/* Nice used when ws_set_nice */

extern	struct	timeval ws_check_time;	/* Check locks every this # usec */
int	ws_set_favor = 0;		/* Control favoring feature */

/*
 * ws_lock_limit is the process virtual time limit for a data or display
 * lock.  The check for this amount of virtual time doesn't start for
 * wlock->lok_max_time amount of real time after the lock is set.  This is
 * done to avoid the overhead of mapping the user structure for normal
 * short lock intervals.
 */
#if defined(AT386)
#define WS_LOCK_LIMIT_SEC	10	/* Break locks in this # of seconds */
#else
#define WS_LOCK_LIMIT_SEC	2	/* Break locks in this # of seconds */
#endif

#define WS_LOCK_LIMIT_USEC	0	/* plus this # of micro seconds */
struct	timeval ws_lock_limit = {WS_LOCK_LIMIT_SEC, WS_LOCK_LIMIT_USEC};

/*
 * Generic access control
 */
int
wlok_waitunlocked(wlock)
	Winlock *wlock;
{
	while ((wlock->lok_flags) && (*wlock->lok_flags & wlock->lok_bit) &&
	    (wlock->lok_pid != u.u_procp->p_pid))
		if (sleep((caddr_t)wlock->lok_wakeup, LOCKPRI|PCATCH))
			return (-1);
	return (0);
}

wlok_running_check(wlock)
	Winlock *wlock;
{
	char stat;

	/* lok_proc may not be set yet */
	if (!wlock->lok_proc)
		return;

	stat = wlock->lok_proc->p_stat;
	if ((stat != SRUN && stat != SSLEEP) ||
	    (wlock->lok_proc->p_pid != wlock->lok_pid)) {
		if (!(wlock->lok_options & WLOK_SILENT))
			printf(
			  "Window %s lock broken as process %D not running\n",
			  (wlock->lok_string) ? wlock->lok_string : "Null",
			  wlock->lok_pid);
		wlok_forceunlock(wlock);
	}
}

/*ARGSUSED*/
wlok_lockcheck(wlock, poll_rate, charge_sleep_against_limit)
	Winlock *wlock;
	int	poll_rate;
	int	charge_sleep_against_limit;
{
	extern	struct proc *pfind();
	register struct proc *process;
	register int pid;
	struct	timeval utime, stime;

	wlock->lok_time += poll_rate;
	if (wlock->lok_time <= wlock->lok_max_time)
		return;
	pid = wlock->lok_pid;

	/* See if process still exists */
	process = pfind(pid);
	if (process == (struct proc *)0) {
		/* Process does not exist */
		if (!(wlock->lok_options & WLOK_SILENT))
			printf(
			"Window %s lock broken after process %D went away\n",
			(wlock->lok_string) ? wlock->lok_string : "Null", pid);
		goto force;
	}

	/* See if process is blocked on an interruptible device */
	if ((process->p_stat == SSLEEP) && (process->p_pri >= PZERO)
		&& !(process->p_flag & SRPC)) {
		if (!charge_sleep_against_limit) {

			/* Only print message if not being traced (debugged) */
			if ((process->p_stat != SSTOP) &&
			  (!(wlock->lok_options & WLOK_SILENT)))
				printf(
		         "Window %s lock broken as process %D blocked\n",
			wlock->lok_string ? wlock->lok_string : "Null", pid);
			goto force;
		}

		/*
		 * Don't start timing process until gets loaded
		 * into core (see SLOAD below).
		 */
		if (process->p_flag & SLOAD) {
			/*
			 * Subtract ws_check_time from limit to give
			 * affect of charging as real time elasped.
			 */
			if (timercmp((&wlock->lok_limit), (&ws_check_time), >)) {
				timevalsub(&wlock->lok_limit, &ws_check_time);
			} else {
				timerclear(&wlock->lok_limit);
			}
		}
	}

	if ((process->p_stat != SSLEEP) && (process->p_stat != SRUN)) {
		if (!(wlock->lok_options & WLOK_SILENT))
			printf(
			"Window %s lock broken as process %D not running\n",
			(wlock->lok_string) ? wlock->lok_string : "Null", pid);
		goto force;
	}

	/*
	 * Don't start timing process until gets loaded into core.
	 */
	if ((~process->p_flag) & SLOAD)
		goto done;
	else
		/*
		 * We may have set the SULOCK bit already but go ahead and
		 * do it again.  When it finally gets into core then it should
		 * stay put.
		 *
		 * There is a 2.0 bug in which the scheduler can turn off the
		 * SULOCK.  If this happens then we just turn it on again.
		 */
		process->p_flag |= SULOCK;

	/*
	 * Do the process virtual time tracking.
	 */
 	if (process->p_uarea == (struct user *)0) {
 		if (!(wlock->lok_options & WLOK_SILENT))
 			printf(
 			"Window %s lock broken as process %D uarea 0\n",
 			(wlock->lok_string) ? wlock->lok_string : "Null", pid);
 		goto force;
 	}
 	utime = wlock->lok_utime = process->p_uarea->u_ru.ru_utime;
 	stime = wlock->lok_stime = process->p_uarea->u_ru.ru_stime;
	if ((!timerisset(&wlock->lok_utime) || !timerisset(&wlock->lok_stime)))
		goto done;

	/* Subtract saved time from current time */
	timevalsub(&utime, &wlock->lok_utime);
	timevalsub(&stime, &wlock->lok_stime);

	/* See if combined time exceeds threshold */
	timevaladd(&utime, &stime);
	if (wlock->lok_timeout_action &&
	  ((!timerisset(&wlock->lok_limit)) ||
	    timercmp((&utime),(&wlock->lok_limit), >))) {
		  /* Lock timed out */
		  if (!(wlock->lok_options & WLOK_SILENT))
			printf(
			  "Window %s lock broken: time exceeded by pid %D\n",
			  wlock->lok_string, pid);
		  wlock->lok_timeout_action(wlock);
		  goto force;
	}
done:
	/* Reset time until next check */
	wlock->lok_time = 0;
	return;
force:
	wlok_forceunlock(wlock);
	return;
}

wlok_forceunlock(wlock)
	Winlock *wlock;
{

	if (wlock->lok_count)
		*(wlock->lok_count) = 1;
	if (wlock->lok_force_action)
		wlock->lok_force_action(wlock);
	wlok_unlock(wlock);
}

/* Most callers will use u.u_procp for proc */
wlok_setlock(wlock, flags, lockbit, lock_count, process)
	register Winlock *wlock;
	int	*flags;
	int	lockbit;
	int	*lock_count;
	register struct	proc *process;
{

	/*
	 * If the lock bit is already set, assume that whoever set it
	 * will set the count also.
	 */
	if (!(*flags & lockbit)) {
		*lock_count = 1;
	}
	*flags |= lockbit;
	wlock->lok_flags = flags;
	wlock->lok_bit = lockbit;
	wlock->lok_count = lock_count;
	wlock->lok_time = 0;
	wlock->lok_limit = ws_lock_limit;	/* Default, can override */
	if (process) {
	    wlock->lok_max_time = ws_check_lock*2;
	    wlock->lok_pid = process->p_pid;
	    wlock->lok_proc = process;

	    /* Pin process so wouldn't swap (only if currently loaded) */
	    if (process->p_flag & SLOAD)
		    process->p_flag |= SULOCK;
	    if (ws_set_nice) {
		/* Boost priority */
		wlock->lok_nice = process->p_nice;
		process->p_nice = ws_nice;
		if (ws_set_pri)
			(void) setpri(process);
	    }
	} else
	    wlock->lok_max_time = ws_no_pid_check_lock;
	/* Will set up rest of structure when first check lock */
}

wlok_unlock(wlock)
	register Winlock *wlock;
{
	register struct	proc *process;
	caddr_t wakeup_handle;

	if ((wlock->lok_flags) && (*wlock->lok_flags & wlock->lok_bit)) {
		if (!(wlock->lok_count) || (*(wlock->lok_count) <= 1)) {
#ifdef WINDEVDEBUG
			if (!(wlock->lok_count))
			    printf("Warning: window lock count pointer NULL\n");
#endif

			/* Reset lock state */
			*wlock->lok_flags &= ~wlock->lok_bit;

			/*
			 * Reset process if process ids match and no other
			 * locks outstanding (they all should be on the same
			 * pid).
			 */
			process = wlock->lok_proc;
			if (process && (process->p_pid == wlock->lok_pid)) {

				/* see if this process has any other locks */
				if (!wlock->lok_other_check ||
				  !(wlock->lok_other_check)(wlock->lok_client)) {
					if (ws_set_nice) {
					    process->p_nice = wlock->lok_nice;
					    if (ws_set_pri)
						(void) setpri(process);
					}
					process->p_flag &= ~SULOCK;
				}
			}

			/* Do lock specific action */
		  	if (wlock->lok_unlock_action)
		  		wlock->lok_unlock_action(wlock);

			/*
			 * Zero wlock after noting who to wakeup,
			 * but preserve user_addr.
			 */
			wakeup_handle = wlock->lok_wakeup;
			wlok_clear(wlock);
			wakeup((caddr_t)wakeup_handle);
		} else
			*(wlock->lok_count) -= 1;
	}
}

wlok_clear(wlock)
	register Winlock *wlock;
{

	/*
	 * Zero wlock but preserve user_addr.
	 */
	if (wlock->lok_count)
		*(wlock->lok_count) = 0;
	bzero((caddr_t)wlock, sizeof (Winlock));
}

/*
 * Ws_favor will boost w->w_pid's favor if ws->ws_favor_pid is not
 * equal to w->w_pid.  A null w means that no process is in favor;
 * ws->ws_favor_pid will be zeroed in this case.
 */
ws_favor(ws, w)
	register Workstation *ws;
	register Window *w;
{
	if (w == WINDOW_NULL) {
		ws->ws_favor_pid = 0;
	} else {
		if ((w->w_pid != ws->ws_favor_pid) && ws_set_favor) {
			favor_pid_and_children(w->w_pid);
			ws->ws_favor_pid = w->w_pid;
		}
	}
}

/*
 * Process pid_to_favor now is the interactive process.
 * Adjust its favor and its children's favor up.
 * Adjust old process(es) (if any) favor down.
 *
 * The correct way to do this is to recursively favor all the children
 * of the tool that has the input focus.  But we know that the interesting
 * case is the C shell and its children.  Csh makes each of its children
 * a process group leader, and their children in turn keep that process
 * group.  It would be nice if we knew which was the foreground process
 * group, but the controlling pty is pointed by the u area which is not
 * only hard to get to, it may be swapped out.  So we punt and do all
 * the children.
 */

#define	FAVOR(p)	((p)->p_flag |= SFAVORD)

#define	UNFAVOR(p)	((p)->p_flag &= ~SFAVORD)

#define	ISFAVORED(p)	((p)->p_flag & SFAVORD)

#ifdef WINDEVDEBUG
int	favor_debug;
#endif

static
favor_pid_and_children(pid_to_favor)
	int pid_to_favor;
{
	register struct proc *p, *pp;
	int w_pid, sh_pid, pgrp;

	p = pfind(pid_to_favor);
#ifdef WINDEVDEBUG
	if (favor_debug)
		printf("%D favored (%X)\n", pid_to_favor, p);
#endif
	if (p == 0) {
		return;
	}
	FAVOR(p);
	w_pid = p->p_pid;
	sh_pid = 0;
	for (p = allproc; p != NULL; p = p->p_nxt) {
		/*
		 * Take away niceness from processes that got
		 * it before.
		 */
		if (ISFAVORED(p) && p->p_pid != w_pid) {
			UNFAVOR(p);
		}
		/*
		 * Check for direct child of window process.
		 */
		if (p->p_ppid == w_pid) {
			FAVOR(p);
			sh_pid = p->p_pid;
		}
	}
	if (sh_pid == 0) {
		return;
	}

	/*
	 * Assuming each child of the shell is a process group leader,
	 * do each process group.
	 * XXX inefficient O(nproc**2) algorithm. Should also use child
	 * pointer (cptr) stuff to get children.
	 */
	for (p = allproc; p != NULL; p = p->p_nxt) {
		if (p->p_ppid == sh_pid) {	/* child of shell */
			if (!ISFAVORED(p)) {	/* got it already? */
				FAVOR(p);	/* no, favor it and pgrp */
				pgrp = p->p_pgrp;
				for (pp = allproc; pp != NULL; pp = p->p_nxt) {
					if (pp->p_pgrp == pgrp) {
						FAVOR(pp);
					}
				}
			}
		}
	}
}
