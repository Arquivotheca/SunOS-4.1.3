#ident	"@(#)kern_clock.c 1.1 92/07/30 SMI; from UCB 7.1 6/5/86"

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dk.h>
#include <sys/callout.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/trace.h>

#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/param.h>
#ifdef sun4
#include <machine/mmu.h>	/* to get MDEVBASE used in clock.h */
#endif
#include <machine/clock.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/rm.h>

#ifdef vax
#include <vax/mtpr.h>
#endif

#ifdef GPROF
#include <sys/gprof.h>
#endif

#ifdef MULTIPROCESSOR
#include "percpu.h"

extern	int	nmod;
#endif

/*
 * Clock handling routines.
 *
 * This code is written to operate with two timers which run
 * independently of each other. The main clock, running at hz
 * times per second, is used to do scheduling and timeout calculations.
 * The second timer does resource utilization estimation statistically
 * based on the state of the machine phz times a second. Both functions
 * can be performed by a single clock (ie hz == phz), however the
 * statistics will be much more prone to errors. Ideally a machine
 * would have separate clocks measuring time spent in user state, system
 * state, interrupt state, and idle state. These clocks would allow a non-
 * approximate measure of resource utilization.
 */

/*
 * TODO:
 *	time of day, system/user timing, timeouts, profiling on separate timers
 *	allocate more timeout table slots when table overflows.
 */
#ifdef notdef
/*
 * Bump a timeval by a small number of usec's.
 */
bumptime(tp, usec)
	register struct timeval *tp;
	int usec;
{

	tp->tv_usec += usec;
	if (tp->tv_usec >= 1000000) {
		tp->tv_usec -= 1000000;
		tp->tv_sec++;
	}
}
#endif notdef
#define	BUMPTIME(t, usec) { \
	register struct timeval *tp = (t); \
	\
	tp->tv_usec += (usec); \
	if (tp->tv_usec >= 1000000) { \
		tp->tv_usec -= 1000000; \
		tp->tv_sec++; \
	} \
}

#ifdef		TRACE
static		int	traceint = 0;
#define		TRACEINT	50
#endif		TRACE

extern struct user *uunix;

/*
 * The hz hardware interval timer.
 * We update the events relating to real time.
 * If this timer is also being used to gather statistics,
 * we run through the statistics gathering routine as well.
 */
#ifdef MEASURE_KERNELSTACK
/*
 * Instrumented to measure the stack pointer
 *
 */
int minkernstack = 0;
int minintrstack = 0;
int print_at_all = 0;
int print_minkernstack = 0;
int print_minintrstack = 0;

#include <machine/pte.h>

hardclock(pc, ps, sp)
	caddr_t pc;
	int ps;
	int sp;	/* Yes, should be a caddr_t, but making it an int is easier */
#else	MEASURE_KERNELSTACK
hardclock(pc, ps)
	caddr_t pc;
	int ps;
#endif	MEASURE_KERNELSTACK
{
	register struct callout *p1;
#ifndef MULTIPROCESSOR
	register struct proc *p;
	register int s;
#endif
	int needsoft = 0;
	extern int tickdelta;
	extern long timedelta;
	/*
	 * doresettodr values:
	 *   0 - 	no set pending
	 *   1 -	set when wanting to be sync'd
	 *   other -	set chip when time.tv_sec greater
	 *		than or equal to this value
	 */
	extern int doresettodr;

#ifdef MEASURE_KERNELSTACK
/*
 * Find the lowest stack pointer and print it out
 */
	if (! USERMODE(ps)) {
		if (sp > (int) Sysmap) {
			/* On kernel stack */
			if (sp < minkernstack) {
				minkernstack = sp;
				if (print_at_all) {
					print_minkernstack = 1;
					needsoft = 1;
				}
			}
		} else {
			/* On interrupt stack */
			if (sp < minintrstack) {
				minintrstack = sp;
				if (print_at_all) {
					print_minintrstack = 1;
					needsoft = 1;
				}
			}
		}
	}
#endif MEASURE_KERNELSTACK

#ifdef	TRACE
	/* Calibrate trace */
	if (++traceint == TRACEINT) {
		traceint = 0;
		trace3(TR_CALIBRATE, 0, time.tv_sec, time.tv_usec);
		trace3(TR_CALIBRATE, 1, time.tv_sec, time.tv_usec);
	}
#endif	TRACE

	/*
	 * Update real-time timeout queue.
	 * At front of queue are some number of events which are ``due''.
	 * The time to these is <= 0 and if negative represents the
	 * number of ticks which have passed since it was supposed to happen.
	 * The rest of the q elements (times > 0) are events yet to happen,
	 * where the time for each is given as a delta from the previous.
	 * Decrementing just the first of these serves to decrement the time
	 * to all events.
	 */
	p1 = calltodo.c_next;
	while (p1) {
		if (--p1->c_time > 0)
			break;
		needsoft = 1;
		if (p1->c_time == 0)
			break;
		p1 = p1->c_next;
	}

#ifndef MULTIPROCESSOR
	/*
	 * Charge the time out based on the mode the cpu is in.
	 * Here again we fudge for the lack of proper interval timers
	 * assuming that the current state has been around at least
	 * one tick.
	 */
	if (USERMODE(ps)) {
		if (u.u_prof.pr_scale)
			needsoft = 1;
		/*
		 * CPU was in user state.  Increment
		 * user time counter, and process process-virtual time
		 * interval timer.
		 */
		BUMPTIME(&u.u_ru.ru_utime, tick);
		if (timerisset(&u.u_timer[ITIMER_VIRTUAL].it_value) &&
		    itimerdecr(&u.u_timer[ITIMER_VIRTUAL], tick) == 0)
			psignal(u.u_procp, SIGVTALRM);
	} else {
		/*
		 * CPU was in system state.
		 */
		if (!noproc) {
			BUMPTIME(&u.u_ru.ru_stime, tick);
		}
	}

	/*
	 * If the cpu is currently scheduled to a process, then
	 * charge it with resource utilization for a tick, updating
	 * statistics which run in (user+system) virtual time,
	 * such as the cpu time limit and profiling timers.
	 * This assumes that the current process has been running
	 * the entire last tick.
	 */
	if (!noproc && !(u.u_procp->p_flag & SWEXIT)) {
		p = u.u_procp;
		if ((u.u_ru.ru_utime.tv_sec+u.u_ru.ru_stime.tv_sec+1) >
		    u.u_rlimit[RLIMIT_CPU].rlim_cur) {
			psignal(p, SIGXCPU);
			if (u.u_rlimit[RLIMIT_CPU].rlim_cur <
			    u.u_rlimit[RLIMIT_CPU].rlim_max)
				u.u_rlimit[RLIMIT_CPU].rlim_cur += 5;
		}
		if (timerisset(&u.u_timer[ITIMER_PROF].it_value) &&
		    itimerdecr(&u.u_timer[ITIMER_PROF], tick) == 0)
			psignal(p, SIGPROF);
		s = p->p_rssize = rm_asrss(p->p_as);
		u.u_ru.ru_idrss += s;
		if (s > u.u_ru.ru_maxrss)
			u.u_ru.ru_maxrss = s;
		/*
		* We adjust the priority of the current process.
		* The priority of a process gets worse as it accumulates
		* CPU time.  The cpu usage estimator (p_cpu) is increased here
		* and the formula for computing priorities (in kern_synch.c)
		* will compute a different value each time the p_cpu increases
		* by 4.  The cpu usage estimator ramps up quite quickly when the
		* process is running (linearly), and decays away exponentially,
		* at a rate which is proportionally slower when the system is
		* busy.  The basic principal is that the system will 90% forget
		* that a process used a lot of CPU time in 5*loadav seconds.
		* [See def. of nrscale in kern_synch.c for how this works.]
		* This causes the system to favor processes which haven't run
		* much recently, and to round-robin among other processes.
		* NOTE:
		* The scheduling alteration is a non sequitur to kernel
		* running lwp's. In fact, if p_pri changes while the
		* process is still on the run queue, a panic from remrq
		* may result
		*/
#ifdef LWP
		if (!runthreads) {
#endif LWP
			p->p_cpticks++;
			if (++p->p_cpu == 0)
				p->p_cpu--;
			if ((p->p_cpu&3) == 0) {
				(void) setpri(p);
				if (p->p_pri >= PUSER)
					p->p_pri = p->p_usrpri;
			}
#ifdef LWP
		}
#endif LWP
	}

#endif /* !MULTIPROCESSOR */

	/*
	 * If the alternate clock has not made itself known then
	 * we must gather the statistics.
	 */
	if (phz == 0)
		gatherstats(pc, ps);

#ifdef	MULTIPROCESSOR
	charge_time(&needsoft);
#endif

	/*
	 * Increment the time-of-day, and schedule
	 * processing of the callouts at a very low cpu priority,
	 * so we don't keep the relatively high clock interrupt
	 * priority any longer than necessary.
	 */

	if (timedelta == 0) {
		if (doresettodr == 1) {
			/*
			 * Oh dear- somebody zeroed timedelta out from
			 * underneath an adjustment that was in progress.
			 * Better zero doresettodr to be sure...
			 */
			doresettodr = 0;
		}
		BUMPTIME(&time, tick);
	} else {
		register delta;
		if (timedelta < 0) {
			delta = tick - tickdelta;
			timedelta += tickdelta;
		} else {
			delta = tick + tickdelta;
			timedelta -= tickdelta;
		}
		BUMPTIME(&time, delta);

		if (-tickdelta < timedelta && timedelta < tickdelta) {
			timedelta = 0;
			if (doresettodr == 1) {
				doresettodr = time.tv_sec + 1;
			}
		}
	}
	if (doresettodr > 1 && doresettodr <= time.tv_sec) {
		doresettodr = 0;
		resettodr();
	}

	if (needsoft) {
		if (BASEPRI(ps)) {
			/*
			 * Save the overhead of a software interrupt;
			 * it will happen as soon as we return, so do it now.
			 */
			(void) splsoftclock();
			softclock(ps);
		} else {
#ifdef sun
			int softclock();

			/*
			 * Pass softclock only the system mode bit
			 * to avoid the softcall table from overflowing
			 * from the various different calls to softclock.
			 */
			softcall(softclock, (caddr_t)(ps & SR_SMODE));
#endif
#ifdef vax
			setsoftclock();
#endif
		}
	}
}

#ifdef MULTIPROCESSOR

/*
 * Charge cpu time to all processes associated with a processor.
 * *needsoftp is set to 1 if a softcall is required.  This is set for
 * the calling routine's (hardclock()) benefit.
 */
charge_time(needsoftp)
	int            *needsoftp;
{
	register int    cix, s;
	register struct user *up;
	register struct proc *p;


	/*
	 * For each processor...
	 */

	for (cix = 0; cix < nmod; cix++) {
		if (!PerCPU[cix].cpu_exists ||
		    PerCPU[cix].noproc)
			continue;
		/*
		 * cpu_idle is set by the idle process when it is sure
		 * that nothing is going on.
		 */
		if (PerCPU[cix].cpu_idle)
			continue;
		up = PerCPU[cix].uunix;
		if (up == NULL)
			continue;
		p = up->u_procp;
		if (p == NULL)
			continue;
		if (p == &idleproc)
			continue;
		/*
		 * Charge the time out based on the mode the cpu is in. Here
		 * again we fudge for the lack of proper interval timers
		 * assuming that the current state has been around at least
		 * one tick.
		 *
		 * XXX - of course, Galaxy has proper interval timers; do we
		 * have the rocket-science time to gut all this stuff and make
		 * it all work?
		 */
		if (PerCPU[cix].cpu_supv) {
			BUMPTIME(&up->u_ru.ru_stime, tick);
		} else {
			if (up->u_prof.pr_scale)
				*needsoftp = 1;
			/*
			 * Increment user time counter, and process
			 * process-virtual time interval timer.
			 */
			BUMPTIME(&up->u_ru.ru_utime, tick);
			if ((timerisset(&up->u_timer[ITIMER_VIRTUAL].it_value)) &&
			    (itimerdecr(&up->u_timer[ITIMER_VIRTUAL], tick) == 0))
				psignal(p, SIGVTALRM);
		}

		/*
		 * If the cpu is currently scheduled to a process, then
		 * charge it with resource utilization for a tick, updating
		 * statistics which run in (user+system) virtual time, such
		 * as the cpu time limit and profiling timers. This assumes
		 * that the current process has been running the entire last
		 * tick.
		 */
		if (!(p->p_flag & SWEXIT)) {
			if ((up->u_ru.ru_utime.tv_sec + up->u_ru.ru_stime.tv_sec + 1) >
			    up->u_rlimit[RLIMIT_CPU].rlim_cur) {
				psignal(p, SIGXCPU);
				if (up->u_rlimit[RLIMIT_CPU].rlim_cur <
				    up->u_rlimit[RLIMIT_CPU].rlim_max)
					up->u_rlimit[RLIMIT_CPU].rlim_cur += 5;
			}
			if (timerisset(&up->u_timer[ITIMER_PROF].it_value) &&
			    itimerdecr(&up->u_timer[ITIMER_PROF], tick) == 0)
				psignal(p, SIGPROF);
			s = p->p_rssize = rm_asrss(p->p_as);
			up->u_ru.ru_idrss += s;
			if (s > up->u_ru.ru_maxrss)
				up->u_ru.ru_maxrss = s;
			/*
			 * We adjust the priority of the current process. The
			 * priority of a process gets worse as it accumulates
			 * CPU time.  The cpu usage estimator (p_cpu) is
			 * increased here and the formula for computing
			 * priorities (in kern_synch.c) will compute a
			 * different value each time the p_cpu increases by
			 * 4.  The cpu usage estimator ramps up quite quickly
			 * when the process is running (linearly), and decays
			 * away exponentially, at a rate which is
			 * proportionally slower when the system is busy.
			 * The basic principal is that the system will 90%
			 * forget that a process used a lot of CPU time in
			 * 5*loadav seconds. [See def. of nrscale in
			 * kern_synch.c for how this works.] This causes the
			 * system to favor processes which haven't run much
			 * recently, and to round-robin among other
			 * processes. NOTE: The scheduling alteration is a
			 * non sequitur to kernel running lwp's. In fact, if
			 * p_pri changes while the process is still on the
			 * run queue, a panic from remrq may result
			 */
#ifdef LWP
			if (!runthreads)
#endif	/* LWP */
			{
				/*
				 * It is possible that this process has been
				 * placed back on the run queue and that the
				 * CPU has not changed its uunix pointer, so
				 * do the right thing ...
				 */
				int	wasonq = onrq(p);
				p->p_cpticks++;
				if (++p->p_cpu == 0)
					p->p_cpu--;
				if ((p->p_cpu & 3) == 0) {
					(void) setpri(p);
					if (p->p_pri >= PUSER) {
						if (wasonq)
							remrq(p);
						p->p_pri = p->p_usrpri;
						if (wasonq)
							setrq(p);
					}
				}
			}
		}
	}
}

#endif /* MULTIPROCESSOR */

int	dk_ndrive = DK_NDRIVE;
/*
 * Gather statistics on resource utilization.
 *
 * We make a gross assumption: that the system has been in the
 * state it is in (user state, kernel state, interrupt state,
 * or idle state) for the entire last time interval, and
 * update statistics accordingly.
 *
 * In the MP version, only one processor per hardclock interrupt (level
 * 10) calls this routine.  Therefore, one
 * processor collects CPU time data for all of the processors present.
 * 
 */
/*ARGSUSED*/
#ifdef MULTIPROCESSOR

gatherstats(pc, ps)
	caddr_t         pc;
	int             ps;
{
	extern int procset;
	register int	cix, cs;
	register struct user *up;
	register struct proc *pp;

	/*
	 * Gather stats for each processor.
	 */
	for (cix=0; cix < nmod; ++cix)
		if ((procset & (1<<cix)) &&
		    (PerCPU[cix].cpu_enable) &&
		    (PerCPU[cix].cpu_exists)) {

			if (PerCPU[cix].cpu_idle)
				cs = CP_IDLE;
			else if (PerCPU[cix].cpu_supv)
				cs = CP_SYS;
			else if ((PerCPU[cix].noproc) ||
				 ((up = PerCPU[cix].uunix) == NULL) ||
				 (up == &idleuarea) ||
				 ((pp = PerCPU[cix].masterprocp) == NULL) ||
				 (pp == &idleproc))
				cs = CP_IDLE;
			else if (pp->p_nice > PZERO)
				cs = CP_NICE;
			else
				cs = CP_USER;

			mp_time[cix][cs]++;
			cp_time[cs]++;

#ifdef	XP_SPIN
			if ((cs == CP_SYS) &&
			    (PerCPU[cix].cpu_supv == 2))
				xp_time[cix][XP_SPIN] ++;
#endif	XP_SPIN

#ifdef	XP_DISK
			if ((cs == CP_IDLE) &&
			    (dk_busy != 0))
				xp_time[cix][XP_DISK] ++;
#endif	XP_DISK

#ifdef	XP_SERV
			if ((cs == CP_SYS) &&
			    (PerCPU[cix].cpu_supv == 3))
				xp_time[cix][XP_SERV] ++;
#endif	XP_SERV

		}

	/*
	 * Collect disk busy data.
	 */
	for (cix = 0; cix < dk_ndrive; cix++)
		if (dk_busy & (1 << cix))
			dk_time[cix]++;
}

#else /* !MULTIPROCESSOR */

gatherstats(pc, ps)
	caddr_t pc;
	int ps;
{
	register int cpstate, s;

	/*
	 * Determine what state the cpu is in.
	 */
	if (USERMODE(ps)) {
		/*
		 * CPU was in user state.
		 */
		if (u.u_procp->p_nice > NZERO)
			cpstate = CP_NICE;
		else
			cpstate = CP_USER;
	} else {
		/*
		 * CPU was in system state.  If profiling kernel
		 * increment a counter.  If no process is running
		 * then this is a system tick if we were running
		 * at a non-zero IPL (in a driver).  If a process is running,
		 * then we charge it with system time even if we were
		 * at a non-zero IPL, since the system often runs
		 * this way during processing of system calls.
		 * This is approximate, but the lack of true interval
		 * timers makes doing anything else difficult.
		 */
		cpstate = CP_SYS;
		if (noproc && BASEPRI(ps))
			cpstate = CP_IDLE;
#ifdef GPROF
#ifdef sun
		/*
		 * For Sun, this stuff is done in sun/kprof.s
		 */
#else
		s = pc - s_lowpc;
		if (profiling < 2 && s < s_textsize)
			kcount[s / (HISTFRACTION * sizeof (*kcount))]++;
#endif
#endif
	}
	/*
	 * We maintain statistics shown by user-level statistics
	 * programs:  the amount of time in each cpu state, and
	 * the amount of time each of dk_ndrive ``drives'' is busy.
	 */
	cp_time[cpstate]++;
	for (s = 0; s < dk_ndrive; s++)
		if (dk_busy & (1 << s))
			dk_time[s]++;
}
#endif MULTIPROCESSOR

#ifdef	MULTIPROCESSOR
static
do_SOWEUPC()
{
	register struct user *up;
	register struct proc *p;
	if ((up = uunix) && (p = up->u_procp)) {
		p->p_flag |= SOWEUPC;
		aston();
	}
}
#endif	MULTIPROCESSOR

/*
 * Software priority level clock interrupt.
 * Run periodic events from timeout queue.
 */
softclock(ps)
	int ps;
{
#ifdef	MULTIPROCESSOR
	int cix;
	struct user *up;
#endif	MULTIPROCESSOR

#ifdef MEASURE_KERNELSTACK
/*
 * Print out stack statistics, if necessary
 */
	if (print_minkernstack) {
		register int s;
		register int sp;

		s = splclock();
		print_minkernstack = 0;
		sp = minkernstack;
		(void) splx(s);
		printf("minkernstack = 0x%x\n", sp);
	}

	if (print_minintrstack) {
		register int s;
		register int sp;

		s = splclock();
		print_minintrstack = 0;
		sp = minintrstack;
		(void) splx(s);
		printf("minintrstack = 0x%x\n", sp);
	}
#endif MEASURE_KERNELSTACK

	for (;;) {
		register struct callout *p1;
		register caddr_t arg;
		register int (*func)();
		register int a, s;

		s = splclock();
		if (((p1 = calltodo.c_next) == 0) || (p1->c_time > 0)) {
			(void) splx(s);
			break;
		}
		arg = p1->c_arg; func = p1->c_func; a = p1->c_time;
		calltodo.c_next = p1->c_next;
		p1->c_next = callfree;
		callfree = p1;
		(void) splx(s);
		(*func)(arg, a);
	}

	/*
	 * If trapped user-mode and profiling, give it
	 * a profiling tick.
	 */
	if (USERMODE(ps)) {
		register struct proc *p = u.u_procp;

		if (u.u_prof.pr_scale) {
			p->p_flag |= SOWEUPC;
			aston();
		}
	}

#ifdef	MULTIPROCESSOR
	/*
	 * If any other cpu is profiling, give it a profiling
	 * tick if it is in user mode.
	 */
	for (cix = 0; cix < nmod; cix++) {
		if ((cix != cpuid) &&
		    (procset & (1<<cix)) &&
		    PerCPU[cix].cpu_exists &&
		    !PerCPU[cix].noproc &&
		    (up = PerCPU[cix].uunix) &&
		    (up != &idleuarea) &&
		    up->u_prof.pr_scale)
			xc_oncpu(0, 0, 0, 0,
				cix, do_SOWEUPC);
	}
#endif	MULTIPROCESSOR
}

/*
 * Arrange that (*fun)(arg) is called in t/hz seconds.
 */
timeout(fun, arg, t)
	int (*fun)();
	caddr_t arg;
	register int t;
{
	register struct callout *p1, *p2, *pnew;
	register int s = splclock();

	if (t <= 0)
		t = 1;
	pnew = callfree;
	if (pnew == NULL)
		panic("timeout table overflow");
	callfree = pnew->c_next;
	pnew->c_arg = arg;
	pnew->c_func = fun;
	for (p1 = &calltodo; (p2 = p1->c_next) && p2->c_time < t; p1 = p2)
		if (p2->c_time > 0)
			t -= p2->c_time;
	p1->c_next = pnew;
	pnew->c_next = p2;
	pnew->c_time = t;
	if (p2)
		p2->c_time -= t;
	(void) splx(s);
}

/*
 * untimeout is called to remove a function timeout call
 * from the callout structure.
 */
untimeout(fun, arg)
	int (*fun)();
	caddr_t arg;
{
	register struct callout *p1, *p2;
	register int s;

	s = splclock();
	for (p1 = &calltodo; (p2 = p1->c_next) != 0; p1 = p2) {
		if (p2->c_func == fun && p2->c_arg == arg) {
			if (p2->c_next && p2->c_time > 0)
				p2->c_next->c_time += p2->c_time;
			p1->c_next = p2->c_next;
			p2->c_next = callfree;
			callfree = p2;
			break;
		}
	}
	(void) splx(s);
}

/*
 * MICROFUDGE is used by "hzto" to relax its calculation
 * just a little bit; if an integer number of ticks take
 * us to less than MICROFUDGE microseconds before the
 * destination time, we call it "enough" and do not wait
 * the extra tick.
 */
#ifndef MICROFUDGE
#define	MICROFUDGE	10
#endif

/*
 * Compute number of hz until specified time.
 * Used to compute third argument to timeout() from an
 * absolute time.
 */
hzto(tv)
	struct timeval *tv;
{
	register long ticks, sec, usec;
	int s = splclock();

	/*
	 * Compute number of ticks we will see between now and
	 * the target time; returns "1" if the destination time
	 * is before the next tick, so we always get some delay,
	 * and returns 0x7fffffff ticks if we would overflow.
	 */
	sec = tv->tv_sec - time.tv_sec;
	usec = tv->tv_usec - time.tv_usec;
	if (usec < 0) {
		sec--;
		usec += 1000000;
	}
	ticks = usec * hz / 1000000;	/* integer ticks in usec delay */
	usec -= (ticks * 1000000) / hz;	/* ticks remaining */

	if (usec > MICROFUDGE)
		ticks++;
	/*
	 * Compute ticks, accounting for negative and overflow as above.
	 * Overflow protection kicks in at about 70 weeks for hz=50
	 * and at about 35 weeks for hz=100.
	 */
	if ((sec < 0) || ((sec == 0) && (ticks < 1)))
		ticks = 1;			/* protect vs nonpositive */
	else if (sec > (0x7fffffff - ticks) / hz)
		ticks = 0x7fffffff;		/* protect vs overflow */
	else
		ticks += sec * hz;		/* common case */

	(void) splx(s);
	return (ticks);
}

profil()
{
	register struct a {
		short	*bufbase;
		unsigned bufsize;
		unsigned pcoffset;
		unsigned pcscale;
	} *uap = (struct a *)u.u_ap;
	register struct uprof *upp = &u.u_prof;

	/*
	 * The low-order bit of the scale is irrelevant anyway, and a scale of
	 * 1 is supposed to be equivalent to a scale of 0 (both should turn
	 * profiling off), so we clear the low-order bit.
	 */
	upp->pr_base = uap->bufbase;
	upp->pr_size = uap->bufsize;
	upp->pr_off = uap->pcoffset;
	upp->pr_scale = uap->pcscale & ~01;
}

#if defined(SUN4_330) || defined(SUN4_470) || defined(sun4c) || defined(sun4m)
#define	HI_RES_CLOCK
#endif

uniqtime(tv)
	register struct timeval *tv;
{
	static struct timeval last;		/* last value returned */
	static struct timeval now;		/* current time value */
#ifdef	HI_RES_CLOCK
	register int cr;			/* nsec counter register */
	register int ib;			/* usec past last tick */
#endif
	int s;

	s = splclock();	/* protect accesses to now, last, time */
	now = time;
#ifdef	HI_RES_CLOCK
	/*
	 * Some models have a high resolution timer that we can use to
	 * figure out the microsecond-precision time.
	 *
	 * If the "limit reached" bit appears in the value register and is
	 * turned on, we have a pending interrupt; add a tick's worth of time
	 * to the clock.
	 *
	 * If the "limit reached" bit is not reflected in the counter value
	 * register, then this may be slow by 1/100 sec if there is a pending
	 * clock interrupt; reading the bit from the limit register would be a
	 * bad thing.
	 *
	 * The limit bit may turn on AS WE REACH the limit. Be ready
	 * to handle both cases here.
	 *
	 * XXX - it would be nice to be able to have this code figure
	 * out if the clock is supported and if it is there without
	 * looking at "cpu".
	 */
#if !defined(sun4c) && !defined(sun4m)
	switch (cpu) {
#ifdef SUN4_330
	case CPU_SUN4_330:
#endif
#ifdef SUN4_470
	case CPU_SUN4_470:
#endif
#endif	/* !sun4c && !sun4m */
		cr = COUNTER->counter10;
		ib = (cr & CTR_USEC_MASK) >> CTR_USEC_SHIFT;
		if (cr & CTR_LIMIT_BIT)
			ib = (ib % tick) + tick;
		BUMPTIME(&now, ib);
#if !defined(sun4c) && !defined(sun4m)
		break;
	default:	/* kernel supports, cpu does not */
		break;
	}
#endif	/* !sun4c && !sun4m */
#endif /* HI_RES_CLOCK */

	/*
	 * Try to keep the return value of uniqtime() monotonicly
	 * increasing in the face of repeated calls, but don't be
	 * obsessive about it in the face of large differences.
	 */
	if ((now.tv_sec > last.tv_sec) ||	/* higher second, or */
	    ((now.tv_sec == last.tv_sec) &&	/* same second and */
	    (now.tv_usec > last.tv_usec)) ||	/* higher microsecond, or */
	    ((last.tv_sec - now.tv_sec) > 5)) {	/* definitely back in time */
		last = now;
	} else {
		BUMPTIME(&last, 1);	/* keep it monotonicly ascending */
	}

	*tv = last;
	(void) splx(s);
}
