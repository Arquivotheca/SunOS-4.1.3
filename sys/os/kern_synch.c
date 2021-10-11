/*	@(#)kern_synch.c 1.1 92/07/30 SMI; from UCB 4.26 83/05/21	*/

#include <machine/pte.h>
#include <machine/psl.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/vm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/trace.h>

#ifdef LWP
#include <lwp/lwp.h>
#endif LWP

#ifdef	MULTIPROCESSOR
#include <percpu.h>

extern	int	nmod;
#endif	MULTIPROCESSOR

#define	SQSIZE 0100	/* Must be power of 2 */
#define	HASH(x)	(((int)x >> 5) & (SQSIZE-1))

struct prochd slpque[SQSIZE];

/*
 * rr_ticks is the number of clock ticks per time slice,
 * controlling how frequently we switch among tasks on the
 * same segment of the run queue (approximately the same
 * overall process priority).
 *
 * If zero, it will be set to hz/10 (1/10 sec) as that seems
 * to be small enough for good interactive response and large
 * enough to keep the roundrobin overhead down.
 */
int	rr_ticks = 0;

#ifdef	MULTIPROCESSOR
struct proc *topproc();		/* machdep.c */
int bestcpu();			/* machdep.c */
int roundrobin_remote();
#endif	MULTIPROCESSOR

/*
 * Force switch among equal priority processes periodicly.
 */
roundrobin()
{
	if (rr_ticks < 1)
		rr_ticks = hz / 10;
	timeout(roundrobin, (caddr_t)0, rr_ticks);
#ifdef	MULTIPROCESSOR
	(void)xc_oncpu(0,0,0,0,
		       bestcpu(topproc()),
		       roundrobin_remote);
}

roundrobin_remote()
{
#endif	MULTIPROCESSOR
	runrun++;
	if (uunix != (struct user *)0) {
		aston();
	}
}

/*
 * constants for digital decay and forget
 * nrscale:	90% of (p_cpu) usage in 5*loadav time
 * ccpu:	95% of (p_pctcpu) usage in 60 seconds (load insensitive)
 *		Note that, as ps(1) mentions, this can let percentages
 *		    total over 100% (I've seen 137.9% for 3 processes)
 *
 * Note that hardclock updates p_cpu and p_cpticks independently.
 *
 * How nrscale could have been chosen:
 *
 *	We wish to decay away 90% of p_cpu in (5 * loadavg) seconds.
 *	That is, the system wants to compute a value of decay such
 *	that the following for loop:
 *		for (i = 0; i < (5 * loadavg); i++)
 *			p_cpu *= decay;
 *	will compute
 *		p_cpu *= 0.1;
 *	for all values of loadavg.
 *
 *	Mathematically this loop can be expressed by saying:
 *		decay ** (5 * loadavg) ~= .1
 *
 *	The system computes decay as:
 *		decay = (nrscale * loadavg) / (nrscale * loadavg + 1)
 *	(with scaled integers, so the actual code is different).
 *
 *	We wish to prove that if nrscale == 2, then the system's
 *	computation of decay will always fulfill the equation:
 *		decay ** (5 * loadavg) ~= .1
 *
 *	If we compute b as:
 *		b = nrscale * loadavg
 *	then
 *		decay = b / (b + 1)
 *
 *	We now need to show two things:
 *	[S1] Given factor ** (5 * loadavg) ~= .1, show factor == b/(b+1)
 *	[S2] Given b/(b+1) ** power ~= .1, show power == (5 * loadavg)
 *
 *	We will use the following facts (and trivial approximations):
 *	[F1] exp(x) = 0! + x**1/1! + x**2/2! + ... .
 *	[F2] ln(1+x) = x - x**2/2 + x**3/3 - ... 	-1 < x < 1
 *	[F3] ln(.1) ~= -2.30
 *	[F4] 5/2.30 ~= 2
 *	[F5] (b-1)/b ~= b/(b+1)				b >> 0
 *	[F6] 4.6*t + 2.3 ~= 5*t
 *
 *	With these facts, we can derive the following approximations:
 *
 *	[A1] exp(x) ~= 1 + x				x ~= 0
 *		apply [F1], and ignore higher order terms
 *	[A2] exp(-1/b) ~= 1 - (1/b) = (b-1)/b		b >> 0
 *		apply [A1] and some algebra
 *
 *	[A3] ln(1+x) ~= x				x ~= 0
 *		apply [F2], and ignore higher order terms
 *	[A4] ln(b/(b+1)) = ln(1 - 1/(b+1)) ~= -1/(b+1)	b >> 0
 *		apply some algebra and [A3]
 *
 *
 *	Again, the basic equations are
 *	[E1] (factor)**(power) ~= .1
 *	[E2] b = 2 * loadav
 *
 *	To show [S1], we can solve for factor as follows:
 *	    take the ln of both sides of [E1], and apply [F3]:
 *		ln(factor) ~= (-2.30/5*loadav)
 *	    take exp of both sides, and apply [F4], [E2], [A2]:
 *		factor ~= exp(-1/((5/2.30)*loadav) ~= exp(-1/(2*loadav)) =
 *		    exp(-1/b) ~= (b-1)/b ~= b/(b+1).			QED
 *
 *	To show [S2], we can solve for power as follows:
 *	    take the ln of both sides of [E1]:
 *		power*ln(b/(b+1)) ~= -2.30
 *	    apply [A4], multiply thru by -(b+1), apply [E2], [F6]:
 *		power ~= 2.3 * (b + 1) = 4.6*loadav + 2.3 ~= 5*loadav.	QED
 *
 *	Even after so many approximations, the results are still fairly close
 *	as shown by the actual power values for the implemented algorithm:
 *
 *	loadav:		1	2	3	4
 *
 *	desired:	5	10	15	20
 *	actual:		5.68	10.32	14.94	19.55
 *
 *
 * These computations avoid floating point calculations by manipulating integer
 * values that are FSCALE times the true value.  Thus calculations that
 * scale a value by conceptually multiplying it by ccpu must adjust the
 * result by dividing by FSCALE (or equivalently, by shifting right FSHIFT).
 */
int	nrscale = 2;
#ifdef vax
/* vax C compiler won't convert float to int for initialization */
long	ccpu = 95*FSCALE/100;				/* exp(-1/20) */
#else
long	ccpu = 0.95122942450071400909 * FSCALE;		/* exp(-1/20) */
#endif

/*
 * Recompute process priorities, once a second
 */
schedcpu()
{
	register struct proc *p, *crp;
	register int s, a;
		/* NB: avenrun is FSCALEd, so b is FSCALEd */
	register long b = avenrun[0] * nrscale;
#ifdef LWP
	extern int runthreads;
	extern struct proc *oldproc;
#endif LWP

	wakeup((caddr_t)&lbolt);
	for (p = allproc; p != NULL; p = p->p_nxt) {
		if (p->p_time != 127)
			p->p_time++;
		if (p->p_stat == SSLEEP || p->p_stat == SSTOP)
			if (p->p_slptime != 127)
				p->p_slptime++;
		/*
		 * If the process has slept the entire second,
		 * stop recalculating its priority until it wakes up.
		 *
		 * p->pctcpu is only for ps.
		 */
		if (p->p_slptime > 1) {
			/*
			 * Decay one tick's worth of cpu percentage.
			 */
			p->p_pctcpu = (p->p_pctcpu * ccpu) >> FSHIFT;
			continue;
		}
		/*
		 * Decay old value and fold in new value.
		 */
		p->p_pctcpu = (ccpu * p->p_pctcpu +
		    (FSCALE - ccpu) * (p->p_cpticks*FSCALE/hz)) >> FSHIFT;
		p->p_cpticks = 0;
		a = ((p->p_cpu & 0377) * b) / (b + FSCALE) + p->p_nice - NZERO;
		if (a < 0)
			a = 0;
		if (a > 255)
			a = 255;
		p->p_cpu = a;
		s = spl6();	/* prevent state changes */
		(void) setpri(p);
#ifdef LWP
		crp = (runthreads ? oldproc : (noproc ? NULL : u.u_procp));
#else
		crp = noproc ? NULL : u.u_procp;
#endif LWP

		if (
#ifdef	MULTIPROCESSOR
/*
 * this process may not actually
 * be on the run queue.
 */
		    onrq(p) &&
#endif	MULTIPROCESSOR
		    (p->p_pri >= PUSER)) {
		    if ((p != crp) &&		/* %%% is this still needed? */
			(p->p_stat == SRUN) &&	/* %%% is this still needed? */
			(p->p_flag & SLOAD) &&
			(p->p_pri != p->p_usrpri)) {
			remrq(p);
			p->p_pri = p->p_usrpri;
			setrq(p);
		    } else
			p->p_pri = p->p_usrpri;
		}
		(void) splx(s);
	}
	vmmeter();
	if (runin != 0) {
		runin = 0;
		wakeup((caddr_t)&runin);
	}
	if (bclnlist != NULL || freemem < desfree) {
		trace1(TR_PAGEOUT_CALL, 2);
		wakeup((caddr_t)&proc[2]);
	}
	timeout(schedcpu, (caddr_t)0, hz);
}

/*
 * Recalculate the priority of a process after it has slept for a while.
 */
updatepri(p)
	register struct proc *p;
{
	register int a = p->p_cpu & 0377;
	register long b = avenrun[0] * nrscale;

	p->p_slptime--;		/* the first time was done in schedcpu */
	while (a && --p->p_slptime)
		a = (a * b) / (b + FSCALE) /* + p->p_nice - NZERO */;
	if (a < 0)
		a = 0;
	if (a > 255)
		a = 255;
	p->p_cpu = a;
	(void) setpri(p);
}

/*
 * Give up the processor till a wakeup occurs
 * on chan, at which time the process
 * enters the scheduling queue at priority pri.
 * The most important effect of pri is that when
 * pri<=PZERO a signal cannot disturb the sleep;
 * if pri>PZERO signals will be processed.
 * If pri&PCATCH is set, signals will cause sleep
 * to return 1, rather than longjmp.
 * Callers of this routine must be prepared for
 * premature return, and check that the reason for
 * sleeping has gone away.
 */
int
sleep(chan, pri)
	caddr_t chan;
	int pri;
{
	register struct proc *rp;
	register struct prochd *hp;
	register int s;
	extern int servicing_interrupt();
#ifdef LWP
	extern int runthreads;

	if (runthreads)
		return (cmsleep(chan, pri));
#endif LWP
	rp = u.u_procp;

	s = splr(pritospl(5));
	if (panicstr) {
		/*
		* After a panic, just give interrupts a chance,
		* then just return; don't run any other procs
		* or panic below, in case this is the idle process
		* and already asleep.
		*/
		(void) spl0();
		goto out;
	}

#ifdef	MULTIPROCESSOR
				/* The Idle Process Never Sleeps. */
	if (rp == &idleproc) {
		(void)spl0();
		swtch();
		goto out;
	}
#endif	MULTIPROCESSOR

	if (servicing_interrupt())
		panic("sleep from interrupt service");
	if (chan==0)
		panic("sleep on zero channel");
	if (rp->p_stat != SRUN)
		panic("sleeping a nonrunning process");
	rp->p_wchan = chan;
	rp->p_slptime = 0;
	rp->p_pri = pri & PMASK;
	/*
	 * put at end of sleep queue
	 */
	hp = &slpque[HASH(chan)];
	insque(rp, hp->ph_rlink);
	rp->p_stat = SSLEEP;
	u.u_ru.ru_nvcsw++;
	if ((pri & PMASK) > PZERO) {
		if (ISSIG(rp, 1)) {			/* ISSIG might swtch!!! */
			if (rp->p_wchan)
				unsleep(rp);
			rp->p_stat = SRUN;
			(void) spl0();
			goto psig;
		}
		if (rp->p_wchan == 0) {
			goto out;
		}
	}

	(void) spl0();
	swtch();					/* Just Do It. */

	if ((pri & PMASK) > PZERO) {
		if (ISSIG(rp, 1))
			goto psig;
	}
out:
	(void) splx(s);
	return (0);

	/*
	 * If priority was low (>PZERO) and
	 * there has been a signal, execute non-local goto through
	 * u.u_qsave, aborting the system call in progress (see trap.c)
	 * unless PCATCH is set, in which case we just return 1 so our
	 * caller can release resources and unwind the system call.
	 */
psig:
	(void) splx(s);
	if (pri & PCATCH)
		return (1);
	longjmp(&u.u_qsave);
	/*NOTREACHED*/
}

/*
 * Remove a process from its wait queue
 */
unsleep(p)
	register struct proc *p;
{
	register s;

	s = spl6();
	if (p->p_wchan) {
		remque(p);
		p->p_rlink = 0;
		p->p_wchan = 0;
	}
	(void) splx(s);
}

/*
 * Wake up all processes sleeping on chan.
 */
wakeup(chan)
	register caddr_t chan;
{
	register struct proc *p;
	register struct prochd *hp;
	int s;

	s = splr(pritospl(5));
#ifdef LWP
	cmwakeup(chan, 0);	/* wakeup any threads sleeping on chan */
#endif LWP
	hp = &slpque[HASH(chan)];
restart:
	for (p = hp->ph_link; p != (struct proc *)hp; ) {
		if (p->p_stat != SSLEEP && p->p_stat != SSTOP)
			panic("wakeup");
		if (p->p_wchan==chan) {
			remque(p);
			p->p_rlink = 0;
			p->p_wchan = 0;
			if (p->p_stat == SSLEEP) {
				/* OPTIMIZED INLINE EXPANSION OF setrun(p) */
				if (p->p_slptime > 1)
					updatepri(p);
				p->p_slptime = 0;
				p->p_stat = SRUN;
				if (p->p_flag & SLOAD)
					setrq(p);
#ifndef	MULTIPROCESSOR
				if (p->p_pri < curpri) {
					runrun++;
					aston();
				}
#else	MULTIPROCESSOR
				(void)xc_oncpu(0,0,0,0,
					       bestcpu(p),
					       roundrobin_remote);
#endif	MULTIPROCESSOR
				if ((p->p_flag&SLOAD) == 0) {
					trace4(TR_PR_WAKEUP, p->p_time,
					       p->p_cpu, p->p_slptime,
					       runout);
					if (runout != 0) {
						runout = 0;
						wakeup((caddr_t)&runout);
					}
					wantin++;
				}
				/* END INLINE EXPANSION */
				goto restart;
			}
			p->p_slptime = 0;
		} else
			p = p->p_link;
	}
	(void) splx(s);
}

/*
 * Wake up the first process sleeping on chan.
 *
 * Be very sure that the first process is really
 * the right one to wakeup.
 */
wakeup_one(chan)
	register caddr_t chan;
{
	register struct proc *p;
	register struct prochd *hp;
	int s;

	s = spl6();
#ifdef LWP
	cmwakeup(chan, 1);	/* wakeup a thread sleeping on chan */
#endif LWP
	hp = &slpque[HASH(chan)];
	for (p = hp->ph_link; p != (struct proc *)hp; ) {
		if (p->p_stat != SSLEEP && p->p_stat != SSTOP)
			panic("wakeup_one");
		if (p->p_wchan==chan) {
			remque(p);
			p->p_rlink = 0;
			p->p_wchan = 0;
			if (p->p_stat == SSLEEP) {
				/* OPTIMIZED INLINE EXPANSION OF setrun(p) */
				if (p->p_slptime > 1)
					updatepri(p);
				p->p_slptime = 0;
				p->p_stat = SRUN;
				if (p->p_flag & SLOAD)
					setrq(p);
#ifndef	MULTIPROCESSOR
				if (p->p_pri < curpri) {
					runrun++;
					aston();
				}
#else	MULTIPROCESSOR
				(void)xc_oncpu(0,0,0,0,
					       bestcpu(p),
					       roundrobin_remote);
#endif	MULTIPROCESSOR
				if ((p->p_flag&SLOAD) == 0) {
					trace4(TR_PR_WAKEUP, p->p_time,
						p->p_cpu, p->p_slptime,
						runout);
					if (runout != 0) {
						runout = 0;
						wakeup((caddr_t)&runout);
					}
					wantin++;
				}
				/* END INLINE EXPANSION */
				goto done;
			}
			p->p_slptime = 0;
		} else
			p = p->p_link;
	}
 done:
	(void) splx(s);
}

/*
 * Initialize the (doubly-linked) run queues and sleep queues
 * to be empty.
 */
rqinit()
{
	register int i;
	for (i = 0; i < NQS; i++)
		qs[i].ph_link = qs[i].ph_rlink = (struct proc *)&qs[i];
	for (i = 0; i < SQSIZE; i++)
		slpque[i].ph_link = slpque[i].ph_rlink =
		    (struct proc *)&slpque[i];
}

/*
 * Set the process running;
 * arrange for it to be swapped in if necessary.
 */

/* Callers must hold pslock !!! */
setrun(p)
	register struct proc *p;
{
	register int s;

	s = spl6();
	switch (p->p_stat) {

	case 0:
	case SWAIT:
	case SRUN:
	case SZOMB:
	default:
		panic("setrun");

	case SSTOP:
	case SSLEEP:
		unsleep(p);	/* e.g. when sending signals */
		break;

	case SIDL:
		break;
	}
	if (p->p_slptime > 1)
		updatepri(p);
	p->p_stat = SRUN;
	if (p->p_flag & SLOAD)
		setrq(p);
#ifndef	MULTIPROCESSOR
	if (p->p_pri < curpri) {
		runrun++;
		aston();
	}
#else	MULTIPROCESSOR
	(void)xc_oncpu(0,0,0,0,
		       bestcpu(p),
		       roundrobin_remote);
#endif	MULTIPROCESSOR
	if ((p->p_flag&SLOAD) == 0) {
		trace4(TR_PR_WAKEUP, p->p_time, p->p_cpu, p->p_slptime,
		       runout);
		if (runout != 0) {
			runout = 0;
			wakeup((caddr_t)&runout);
		}
		wantin++;
	}
	(void) splx(s);
}

int fav_nice = -10;

/*
 * Set user priority.
 * The rescheduling flag (runrun)
 * is set if the priority is better
 * than the currently running process.
 *
#ifdef	MULTIPROCESSOR
 *	carefully select which processor's
 *	curpri is used for comparison, and
 *	which processor's runrun is set,
 *	and to whom we direct the level10.
 *	NOTE: aston() attacks the process
 *	on the LOCAL processor.
#endif	MULTIPROCESSOR
 */
setpri(pp)
	register struct proc *pp;
{
	register int p;

	p = (pp->p_cpu & 0377)/4;
	p += PUSER + 2*(pp->p_nice - NZERO);
	if (pp->p_flag & SFAVORD)
		p += 2*fav_nice;
	if (pp->p_rssize > pp->p_maxrss && freemem < desfree)
		p += 2*4;	/* effectively, nice(4) */
	if (p > 127)
		p = 127;
	if (p < 1)
		p = 1;

	pp->p_usrpri = p;
#ifndef	MULTIPROCESSOR
	if (p < curpri) {
		runrun++;
		aston();
	}
#else	MULTIPROCESSOR
	(void)xc_oncpu(0,0,0,0,
		       bestcpu(pp),
		       roundrobin_remote);
#endif	MULTIPROCESSOR
	return pp->p_usrpri;
}
