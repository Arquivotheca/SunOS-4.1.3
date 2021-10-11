/*	@(#)vm_sched.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/trace.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_u.h>
#include <vm/faultcode.h>


int	maxslp = MAXSLP;

/*
 * The main loop of the scheduling (swapping) process.
 *
 * The basic idea is:
 *	see if anyone wants to be swapped in;
 *	swap out processes until there is room;
 *	swap him in;
 *	repeat.
 * If the paging rate is too high, or the average free memory
 * is very low, then we do not consider swapping anyone in,
 * but rather look for someone to swap out.
 *
 * The runout flag is set whenever someone is swapped out.
 * Sched sleeps on it awaiting work.
 *
 * Sched sleeps on runin whenever it cannot find enough
 * core (by swapping out or otherwise) to fit the
 * selected swapped process.  It is awakened when the
 * core situation changes and in any case once per second.
 */

/*
 * Kernel processes shouldn't be swapped.  Such processes have no
 * associated address space and may also have the SSYS flag set
 * (thereby insulating themselves from signals).
 * XXX - But what if we want to be able to swap out unused daemons?
 */
#define	swappable(p) \
	(((p)->p_flag & \
	    (SSYS|SULOCK|SLOAD|SPAGE|SKEEP|SWEXIT|SPHYSIO))==SLOAD && \
	(p)->p_as != NULL && (p)->p_swlocks == 0 && (p)->p_threadcnt < 2)

/* insure non-zero */
#define	nz(x)	(x != 0 ? x : 1)

#define	NBIG	4
#define	MAXNBIG	10
int	nbig = NBIG;

struct bigp {
	struct	proc *bp_proc;
	int	bp_pri;
	struct	bigp *bp_link;
} bigp[MAXNBIG], bplist;

int	nosched = 0;		/* XXX - temp until swapping fixed */
int	swapdebug = 0;		/* XXX */

void sched()
{
	register struct proc *rp, *p, *inp;
	int outpri, inpri, rppri;
	int sleeper, desperate, deservin, needs, divisor;
	register struct bigp *bp, *nbp;
	int biggot;

#ifdef	MULTIPROCESSOR
	proc[0].p_flag |= SORPHAN | SLOAD | SSYS;
#endif

	if (nosched)			/* XXX - temp until swapping fixed */
		(void) sleep((caddr_t)&nosched, PSWP);

loop:
	wantin = 0;
	deservin = 0;
	sleeper = 0;
	p = 0;
	/*
	 * See if paging system is overloaded; if so swap someone out.
	 * Conditions for hard outswap are:
	 *	1. if there are at least 2 runnable processes (on the average)
	 * and	2. the paging rate is excessive or memory is now VERY low.
	 * and	3. the short (5-second) and longer (30-second) average
	 *	   memory is less than desirable.
	 */
	trace6(TR_SWAP_LOOP, mapwant(kernelmap), avenrun[0],
		avefree, avefree30, rate.v_pgin, rate.v_pgout);
	if ((avenrun[0] >= 2*FSCALE && imax(avefree, avefree30) < desfree &&
	    (rate.v_pgin + rate.v_pgout > maxpgio || avefree < minfree))) {
		desperate = 1;
	} else {
		desperate = 0;
		/*
		 * Not desperate for core,
		 * look for someone who deserves to be brought in.
		 */
		outpri = -20000;


		for (rp = allproc; rp != NULL; rp = rp->p_nxt)
#ifdef	MULTIPROCESSOR
		if (rp->p_cpuid < 0)
#endif	MULTIPROCESSOR
		switch (rp->p_stat) {

		case SRUN:
			if ((rp->p_flag&SLOAD) == 0) {
				/*
				 * The process is swapped out; compute how deserving
				 * it is to be brought back in.  The higher the
				 * resulting value, the more deserving.  Larger
				 * processes are less deserving.  Long time sleepers
				 * are more deserving.  Niceness is amplified
				 * before being taken into account.
				 */
				rppri = rp->p_time -
					rp->p_swrss / nz(maxpgio/2) +
						rp->p_slptime - (rp->p_nice-NZERO)*8;
				if (rppri > outpri) {
					p = rp;
					outpri = rppri;
				}
			}
			continue;

		case SSLEEP:
		case SSTOP:
			/*
			 * XXX:	Check for swrss == 0?
			 */
			trace6(TR_SWAP_OUT_CHECK0, rp, freemem, rp->p_rssize,
			       rp->p_slptime, maxslp, swappable(rp));
			if (rp->p_slptime > maxslp && swappable(rp)) {
				/*
				 * Kick out deadwood.
				 */
				(void) spl6();
				rp->p_flag &= ~SLOAD; /* XXX */
				if (rp->p_stat == SRUN)
					remrq(rp);
				(void) spl0();
				if (freemem < desfree)
					(void) swapout(rp, 1);
				else
					(void) swapout(rp, 0);
				trace3(TR_SWAP_OUT, rp, 0, rp->p_swrss);
				goto loop;
			}
			continue;
		}

		/*
		 * No one wants in, so nothing to do.
		 */
		if (outpri == -20000) {
			(void) spl6();
			if (wantin) {
				wantin = 0;
				trace4(TR_SWAP_SLEEP, &lbolt, runin, runout, wantin);
				(void) sleep((caddr_t)&lbolt, PSWP);
			} else {
				runout++;
				trace4(TR_SWAP_SLEEP, &runout, runin, runout, wantin);
				(void) sleep((caddr_t)&runout, PSWP);
			}
			(void) spl0();
			goto loop;
		}
		/*
		 * Decide how deserving this guy is.  If he is deserving
		 * we will be willing to work harder to bring him in.
		 * Needs is an estimate of how much core he will need.
		 * If he has been out for a while, then we will
		 * bring him in with 1/2 the core he will need, otherwise
		 * we are conservative.
		 */

		deservin = 0;
		divisor = 1;
		if (outpri > maxslp/2) {
			deservin = 1;
			divisor = 2;
		}
		needs = p->p_swrss;
		needs = imin(needs, lotsfree);
		trace6(TR_SWAP_IN_CHECK, p, freemem, deficit, needs, outpri, maxslp);
		if (freemem - deficit > needs / divisor) {
			deficit += needs;
			if (swapin(p))
				goto loop;
			deficit -= imin(needs, deficit);
		}
	}
	/********************************************************
	 *			HARDSWAP
	 *
	 * Need resources (kernel map or memory), swap someone out.
	 * Select the nbig largest jobs, then the oldest of these
	 * is ``most likely to get booted.''
	 */
	inp = p;
	sleeper = 0;
	if (nbig > MAXNBIG)
		nbig = MAXNBIG;
	if (nbig < 1)
		nbig = 1;
	biggot = 0;
	bplist.bp_link = 0;

	for (rp = allproc; rp != NULL; rp = rp->p_nxt)
#ifdef	MULTIPROCESSOR
	if (rp->p_cpuid < 0)
#endif	MULTIPROCESSOR
	{
		if (!swappable(rp))
			continue;
		if (rp == inp)
			continue;
		/* XXX - used to skip if text segment locked */
		if ((rp->p_slptime > maxslp) &&
		    (((rp->p_stat == SSLEEP) &&
		      (rp->p_pri > PZERO)) ||
		     (rp->p_stat == SSTOP))) {
			if (sleeper < rp->p_slptime) {
				p = rp;
				sleeper = rp->p_slptime;
			}
		} else if ((sleeper == 0) &&
			   ((rp->p_stat == SRUN) ||
			    (rp->p_stat == SSLEEP))) {
			rppri = rp->p_rssize;
			if (biggot < nbig)
				nbp = &bigp[biggot++];
			else {
				nbp = bplist.bp_link;
				if (nbp->bp_pri > rppri)
					continue;
				bplist.bp_link = nbp->bp_link;
			}
			for (bp = &bplist; bp->bp_link; bp = bp->bp_link)
				if (rppri < bp->bp_link->bp_pri)
					break;
			nbp->bp_link = bp->bp_link;
			bp->bp_link = nbp;
			nbp->bp_pri = rppri;
			nbp->bp_proc = rp;
		}
	}
	if (!sleeper) {
		p = NULL;
		inpri = -1000;
		for (bp = bplist.bp_link; bp; bp = bp->bp_link) {
			rp = bp->bp_proc;
			rppri = rp->p_time + rp->p_nice-NZERO;
			if (rppri >= inpri) {
				p = rp;
				inpri = rppri;
			}
		}
	}
	/*
	 * If we found a long-time sleeper, or we are desperate and
	 * found anyone to swap out, or if someone deserves to come
	 * in and we didn't find a sleeper, but found someone who
	 * has been in core for a reasonable length of time, then
	 * we kick the poor luser out.
	 */
	trace6(TR_SWAP_OUT_CHECK, p, sleeper, desperate, deservin, inpri,
		maxslp);
	if (sleeper ||
	    (desperate && p) ||
	    (deservin && (inpri > maxslp))) {
		(void) spl6();
		p->p_flag &= ~SLOAD;				/* XXX */
		if (p->p_stat == SRUN)
			remrq(p);
		(void) spl0();
		trace3(TR_SWAP_OUT, p, 1, p->p_swrss);
		if (swapout(p, 1) != 0) {
			/*
			 * If we're desperate, we want to give the space
			 * obtained by swapping this process out to the
			 * rest of the processes in core, so we give them
			 * a chance by increasing the deficit.
			 */
			if (desperate)
				deficit += imin(p->p_swrss, lotsfree);
		}
		goto loop;
	}
	/*
	 * Want to swap someone in, but can't
	 * so wait on runin.
	 */
	(void) spl6();
	runin++;
	trace4(TR_SWAP_SLEEP, &runin, runin, runout, wantin);
	(void) sleep((caddr_t)&runin, PSWP);
	(void) spl0();
	goto loop;
}

/*
 * XXX - this still needs to be made real, and as_swapin written.
 */

/*
 * Swap in process p.  To the extent we can, keep track of the
 * number of pages brought back in, updating the global statistics
 * information with this value.
 */
int
swapin(p)
	register struct proc *p;
{
	register int s;
	register u_int bytesin;
	register faultcode_t fc;

	trace2(TR_SWAP_IN, p, p->p_swrss);
	if (swapdebug)
		printf("swapin: pid %d\n", p->p_pid);

	p->p_rssize = rm_asrss(p->p_as);

	bytesin = ptob(SEGU_PAGES - 1);
	fc = segu_fault(segu, (addr_t)p->p_segu, bytesin, F_SOFTLOCK, S_OTHER);
	if (fc != 0) {
		/*
		 * XXX:	Should kill off this process rather than panicing.
		 */
		panic("error in swapping in u-area");
	}
#ifdef	notdef
	/*
	 * For now, rely on faulting the process back in.  (We
	 * currently don't have any way to remember which pages
	 * were in the process's resident set when it was swapped
	 * out.)
	 */
	bytesin += as_swapin(p->p_as);
#endif	notdef
	s = spl6();
	if (p->p_stat == SRUN)
		setrq(p);
	p->p_flag |= SLOAD;
	(void) splx(s);
	p->p_time = 0;
	multprog++;
	cnt.v_swpin++;
	sum.v_pswpin += btop(bytesin);
	return (1);
}

/*
 * Swap out process p.  Keep track of the number of pages swapped out,
 * updating the global statistics information with this value.
 */
int
swapout(p, hardswap)
	register struct proc *p;
	short hardswap;
{
#ifdef	notdef
	register struct pte *map;
	register struct user *utl;
#endif	notdef
	register int a;
	register u_int bytesout = 0;
	faultcode_t fc;
	int rc = 1;

	if (swapdebug)
		printf("swapout: pid %d\n", p->p_pid);

	/*
	 * Make the process unrunnable and thus commit to swapping
	 * it out.
	 *
	 * XXX:	Why must this assignment be protected?
	 */
	a = spl6();		/* hack memory interlock XXX */
	p->p_flag &= ~SLOAD; 	/* redundant? XXX */
	(void) splx(a);

	/*
	 * Note that we record the amount of memory actually swapped
	 * out, not the process's resident set size.  The rationale
	 * is that the remainder of the process's resident set is shared
	 * and will likely still be resident when the process is swapped
	 * back in.
	 */
	/*
	 * Swap the process before the u-area because some architectures
	 * keep the process's vm info in the u-area.
	 */
	if (hardswap) {
		bytesout = as_swapout(p->p_as, 1);
		fc = segu_fault(segu, (addr_t)p->p_segu, ptob(SEGU_PAGES - 1),
				F_SOFTUNLOCK, S_WRITE);
		cnt.v_swpout++;
	} else {
		fc = segu_fault(segu, (addr_t)p->p_segu, ptob(SEGU_PAGES - 1),
				F_SOFTUNLOCK, S_OTHER);
		rc = as_swapout(p->p_as, 0);
	}
	if (fc != 0) {
		/*
		 * XXX:	Should kill off this process rather than panicing.
		 */
		printf("segu_fault --> %d\n", fc);
		panic("error in swapping out u-area");
	}
	bytesout += ptob(SEGU_PAGES - 1);
	p->p_swrss = btop(bytesout);

	p->p_time = 0;

	multprog--;
	sum.v_pswpout += p->p_swrss;

	if (runout) {
		runout = 0;
		wakeup((caddr_t)&runout);
	}
	/*
	 * check for error when soft swapping
	 * (but not yet...) XXX
	 */
#if 0
	if (!hardswap && rc == 0) {
		a = spl6();
		p->p_flag |= SLOAD;
		if (p != u.u_procp && p->p_stat == SRUN)
			setrq(p);
		(void) splx(a);
	}
#endif
	return (rc);
}
