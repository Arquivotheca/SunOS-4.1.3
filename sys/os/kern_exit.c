/*	@(#)kern_exit.c 1.1 92/07/30 SMI; from UCB 7.1 6/5/86	*/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/session.h>
#include <sys/buf.h>
#include <sys/wait.h>
#include <sys/vm.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/vnode.h>
#include <sys/asynch.h>
#include <sys/trace.h>
#include <sys/debug.h>

#include <machine/reg.h>
#include <machine/pte.h>
#include <machine/psl.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_u.h>
#include <vm/page.h>
#include <machine/seg_kmem.h>

/*
 * Exit system call: pass back caller's arg
 */
rexit()
{
	register struct a {
		int	rval;
	} *uap;

	uap = (struct a *)u.u_ap;
	exit((uap->rval & 0xff) << 8);
}

/*
 * Release resources.
 * Save u. area info for parent to look at.
 * Enter zombie state.
 * Wake up parent and init processes,
 * and dispose of children.
 */
exit(rv)
	int rv;
{
	register int i;
	register struct proc *p, *q;
	register int x;
	struct mbuf *m;
	extern void astop();
#ifdef sun386
	extern struct proc *fp_proc;
#endif

	p = u.u_procp;
#ifdef SHOWSTAGE
	simcshowexit(p);
#endif
	p->p_xstat = rv;
#ifdef LWP
#ifdef ASYNCHIO
	if (p->p_aio_forw) {
		/*
		 * Cancel (sleep if nec.) asynch IO
		 */
		astop(ALL_AIO, (struct aio_result_t *)ALL_AIO);
	}
#endif ASYNCHIO
#endif LWP
	m = m_getclr(M_WAIT, MT_ZOMBIE);

#ifdef PGINPROF
	vmsizmon();
#endif
#ifdef VPIX
	v86exit(p);
#endif
	p->p_flag &= ~(STRC|SULOCK);
	p->p_flag |= SWEXIT;
	p->p_sigignore = ~0;
	p->p_cpticks = 0;
	p->p_pctcpu = 0;
	for (i = 0; i < NSIG; i++)
		u.u_signal[i] = SIG_IGN;
	untimeout(realitexpire, (caddr_t)p);
	relvm(p);
	/*
	 * if session leader
	 *	if there's a foreground pgrp then SIGHUP them
	 *	mark the tty as available
	 */
	if (p->p_pid == p->p_sessp->s_sid) {
		register struct sess *sp = p->p_sessp;

		if (sp->s_ttyp != NULL && *sp->s_ttyp != 0) {
			gsignal(*sp->s_ttyp, SIGHUP);
			*sp->s_ttyp = 0;
		}
		SESS_VN_RELE(sp);
	}
	/*
	 * Close all files and deallocate space for them if the
	 * process previously allocated any.
	 */
	for (i = 0; i <= u.u_lastfile; i++) {
		register struct file *f;

		if ((f = u.u_ofile[i]) != NULL) {
			/* Release all System-V style record locks, if any */
			(void) vno_lockrelease(f);
			closef(f);
			u.u_ofile[i] = NULL;
		}
		u.u_pofile[i] = 0;
	}
	if (u.u_ofile != u.u_ofile_arr) {
		kmem_free((caddr_t)u.u_ofile, NOFILE * sizeof (*(u.u_ofile)));
		kmem_free((caddr_t)u.u_pofile, NOFILE * sizeof (*(u.u_pofile)));
	}
	VN_RELE(u.u_cdir);
	if (u.u_rdir) {
		VN_RELE(u.u_rdir);
	}
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;

	/*
	 * free saved bus error stack frame
	 */
#if defined(sun3) || defined(sun3x)
	if (u.u_pcb.u_berr_stack != NULL) {
		kmem_free((caddr_t)u.u_pcb.u_berr_stack,
		    sizeof (struct user_buserr_stack));
		u.u_pcb.u_berr_stack = NULL;
	}
#endif sun3 || sun3x

	/*
	 * disown 80387 chip
	 */
#ifdef i386
	if (p == fp_proc) {
		fp_proc = NULL;
	}
#endif

	/* calls to "exitfunc" functions */
#ifdef IPCSEMAPHORE
	semexit();		/* clean up SystemV IPC semaphores */
#endif
#ifdef SYSACCT
	acct(rv);
#endif
	/*
	 * Give up the saved directory
	 */
#ifdef	SYSAUDIT
	cwfree(u.u_cwd);
#endif	SYSAUDIT
#if defined(sun3) || defined(sun3x)
	{
		extern struct proc *fpprocp;

		if (p == fpprocp)
			fpprocp = (struct proc *)0;
	}
#endif sun3 || sun3x
#if defined(sun4) || defined(sun4c) || defined(sun4m)
	/* XXX - three different version of fpu stuff should be merged */
	fpu_ctxfree();
#endif sun4 || sun4c || sun4m
	crfree(u.u_cred);
	if (*p->p_prev = p->p_nxt)			/* off allproc queue */
		p->p_nxt->p_prev = p->p_prev;
	if (p->p_nxt = zombproc)			/* onto zombproc */
		p->p_nxt->p_prev = &p->p_nxt;
	p->p_prev = &zombproc;
	zombproc = p;
	multprog--;
	p->p_stat = SZOMB;
	noproc = 1;
	if (p->p_pid == 1)
		panic("init died");
	i = PIDHASH(p->p_pid);
	x = p - proc;
	if (pidhash[i] == x) {
		pidhash[i] = p->p_idhash;
	} else {
		for (i = pidhash[i]; i != 0; i = proc[i].p_idhash)
			if (proc[i].p_idhash == x) {
				proc[i].p_idhash = p->p_idhash;
				goto done;
			}
		panic("exit");
	}
done:
	p->p_ru = mtod(m, struct rusage *);
	*p->p_ru = u.u_ru;
	ruadd(p->p_ru, &u.u_cru);
	if (p->p_flag & STRACNG) {
		/*
		 * We were tracing at least one process at some point in
		 * our history.  Track down all the processes we're
		 * currently tracing, un-trace them, and kill them (if
		 * they're not zombies), since their existence means
		 * someone is screwing up.  If they are zombies, notify
		 * their real parent.
		 * Note that we can trace a non-related process.
		 */
		for (q = allproc; q != NULL; q = q->p_nxt) {
			if (q->p_tptr == p) {
				q->p_flag &= ~STRC;
				q->p_tptr = 0;	/* not being traced any more */
				psignal(q, SIGKILL);
			}
		}
		for (q = zombproc; q != NULL; q = q->p_nxt) {
			if (q->p_tptr == p) {
				q->p_flag &= ~STRC;
				q->p_tptr = 0;	/* not being traced any more */
				psignal(q->p_pptr, SIGCHLD);
				wakeup((caddr_t)q->p_pptr);
			}
		}
	}
	if (p->p_cptr)		/* only need this if any child is SZOMB */
		wakeup((caddr_t)&proc[1]);
	/*
	 * Traverse the list, making init the parent,
	 * then move the whole list at once to init.
	 */
	if ((q = p->p_cptr) != NULL) {
		register struct proc* init = &proc[1];

		for (;;) {
			register int zap;

			/*
			 * orphan_chk() looks at p_pptr, so this has to be here
			 */
			zap = orphan_chk(q->p_pgrp);
			q->p_pptr = init;
			q->p_ppid = 1;
			if (zap) {
				register struct proc *z;
				/*
				 * When a process group becomes orphaned and
				 * there is a member that is stopped (but not
				 * traced) we HUP and CONT the pgrp.
				 *
				 * This is designed to be ``safe'' for setuid
				 * processes since they must be willing to
				 * tolerate hangups anyways.
				 *
				 * If the process is being traced, we shouldn't
				 * mess with it, unless we were the one tracing
				 * in, in which case we will zap it below.
				 *
				 * This used to do a gsignal() but that might
				 * zap traced processes so we do it by hand.
				 */
				for (z = pgfind(q->p_pgrp); z; z = pgnext(z)) {
					if ((z->p_flag & STRC) == 0) {
						psignal(z, SIGHUP);
						psignal(z, SIGCONT);
					}
				}
			}
			if (q->p_osptr == NULL)
				break;
			q = q->p_osptr;
		}
		if ((q->p_osptr = init->p_cptr) != NULL)
			init->p_cptr->p_ysptr = q;
		init->p_cptr = p->p_cptr;
	}
	p->p_cptr = NULL;
	psignal(p->p_pptr, SIGCHLD);
	wakeup((caddr_t)p->p_pptr);

	trace4(TR_PROC_EXIT, trs(u.u_comm,0), trs(u.u_comm,1),
	    trs(u.u_comm,2), trs(u.u_comm,3));
	/*
	 * Free stack and uarea.  The finishexit routine first (atomically)
	 * switches to a different stack before calling segu_release.
	 * This is necessary because the stack that we're currently
	 * running on is one of the things that segu_release frees.
	 * finishexit() then calls swtch() to get a new process.
	 */
	finishexit((addr_t)p->p_segu);
	/*NOTREACHED*/
}


int wait4();

/*
 * Old wait() and wait3() system calls to support old (pre-4.0) binaries.
 *	u.u_ar0[R0] == options;
 *	u.u_ar0[R1] == &rusage;
 *	u.u_r.r_val1 == pid;
 *	u.u_r.r_val2 == status;
 */
owait()
{
	struct wait4_args args;

	args.pid = 0;
	args.status = (union wait *)&u.u_r.r_val2;
#ifdef sparc
	args.options = 0;
	args.rusage = 0;
#else !sparc
#ifdef i386
	if ((u.u_ar0[PS] & PS_C) == 0) {
		args.options = 0;
		args.rusage = 0;
	} else {
		args.options = u.u_ar0[ECX];
		args.rusage = (struct rusage *)u.u_ar0[EDX];
	}
#endif i386
	if ((u.u_ar0[PS] & PSL_ALLCC) != PSL_ALLCC) {
		args.options = 0;
		args.rusage = 0;
	} else {
		args.options = u.u_ar0[R0];
		args.rusage = (struct rusage *)u.u_ar0[R1];
	}
#endif !i386 and !sparc
	wait4(&args);
}

#ifdef sparc
/* XXX temp to make pre-4.0 binaries run */
owait3(uap)
	register struct a {
		union wait *status;
		int options;
		struct rusage *rup;
	} *uap;
{
	struct wait4_args args;

	args.pid = 0;
	args.status = (union wait *)&u.u_r.r_val2;
	args.options = uap->options;
	args.rusage = uap->rup;
	wait4(&args);
}
#endif sparc

/*
 * Wait4 system call.
 * Search for a terminated (zombie) child,
 * finally lay it to rest, and collect its status.
 * Look also for stopped (traced) children,
 * and pass back status from them.
 * Also look for non-children that we are tracing.
 *
 * POSIX changes:
 *	if !pid then wait for anyone in my process group
 */
wait4(uap)
	struct wait4_args *uap;
{
	register f, pgrp;
	register struct proc *p, *q;

	f = 0;
	if (uap->pid < 0)
		pgrp = -uap->pid;
	else
		pgrp = -1;

	if (uap->options & ~(WNOHANG|WUNTRACED)) {
		u.u_error = EINVAL;
		return;
	}
loop:
	q = u.u_procp;
	if (q->p_flag & STRACNG) {
		/*
		 * We were tracing at least one process at some point
		 * in our history.  Check whether any process that isn't a
		 * child of ours, but which we're tracing, has exited or
		 * stopped.
		 */
		for (p = allproc; p; p = p->p_nxt) {
			/*
			 * Check non-zombie process list.
			 */
			if (p->p_tptr != q)
				continue;
			if (uap->pid && (p->p_pid != uap->pid) &&
			    (pgrp == -1 || p->p_pgrp != pgrp))
				continue;
			f++;
			if (p->p_stat == SSTOP && (p->p_flag&SWTED)==0 &&
			    (p->p_flag&STRC || uap->options&WUNTRACED)) {
				stoppedproc(uap, p);
				return;
			}
		}
		for (p = zombproc; p; p = p->p_nxt) {
			/*
			 * Check zombie process list.
			 */
			if (p->p_tptr != q)
				continue;
			if (uap->pid && (p->p_pid != uap->pid) &&
			    (pgrp == -1 || p->p_pgrp != pgrp))
				continue;
			zombieproc(uap, p);
			return;
		}
	}
	for (p = q->p_cptr; p != NULL; p = p->p_osptr) {
		/*
		 * Check all children
		 */
		if (p->p_tptr) {
			continue; /* traced by somebody other than q */
		}
		if (uap->pid && (p->p_pid != uap->pid) &&
		    (pgrp == -1 || p->p_pgrp != pgrp))
			continue;
		f++;
		if (p->p_stat == SZOMB) {
			zombieproc(uap, p);
			return;
		}
		if ((p->p_stat == SSTOP) && ((p->p_flag&SWTED) == 0) &&
		    (p->p_flag&STRC || uap->options&WUNTRACED)) {
			stoppedproc(uap, p);
			return;
		}
	}
	if (f == 0) {
		u.u_error = ECHILD;
		return;
	}
	if (uap->options&WNOHANG) {
		u.u_r.r_val1 = 0;
		return;
	}
	if (setjmp(&u.u_qsave)) {
		p = u.u_procp;
		if ((u.u_sigintr & sigmask(p->p_cursig)) != 0) {
			u.u_error = EINTR;
			return;
		}
		u.u_eosys = RESTARTSYS;
		return;
	}
	(void) sleep((caddr_t)u.u_procp, PWAIT);
	goto loop;
}

/*
 * Return status of a zombie process and remove it from the process table.
 */
zombieproc(uap, p)
	struct wait4_args *uap;
	register struct proc *p;
{
	register struct proc *q = u.u_procp;

	u.u_error = statusout((int)p->p_xstat, uap->status);
	if (u.u_error == 0 && uap->rusage != NULL) {
		u.u_error = copyout((caddr_t)p->p_ru, (caddr_t)uap->rusage,
		    sizeof (struct rusage));
	}
	if (u.u_error)
		return;

	if (*p->p_prev = p->p_nxt)	/* off zombproc */
		p->p_nxt->p_prev = p->p_prev;

	u.u_r.r_val1 = p->p_pid;
	p->p_xstat = 0;
	ruadd(&u.u_cru, p->p_ru);
	(void) m_free(dtom(p->p_ru));
	p->p_ru = 0;
	p->p_stat = NULL;
	pgexit(p);
	SESS_EXIT(p);
	p->p_pid = 0;
	p->p_ppid = 0;
	if (q = p->p_ysptr)
		q->p_osptr = p->p_osptr;
	if (q = p->p_osptr)
		q->p_ysptr = p->p_ysptr;
	if ((q = p->p_pptr)->p_cptr == p)
		q->p_cptr = p->p_osptr;
	p->p_pptr = 0;
	p->p_tptr = 0;
	p->p_ysptr = 0;
	p->p_osptr = 0;
	p->p_cptr = 0;
	p->p_sig = 0;
	p->p_sigcatch = 0;
	p->p_sigignore = 0;
	p->p_sigmask = 0;
	p->p_pgrp = 0;
	p->p_flag = 0;
	p->p_wchan = 0;
	p->p_cursig = 0;

	p->p_nxt = freeproc;		/* onto freeproc */
	freeproc = p;
}

/*
 * Return status of a stopped process.
 */
stoppedproc(uap, p)
	struct wait4_args *uap;
	register struct proc *p;
{

	u.u_error = statusout((int)(p->p_cursig<<8 | WSTOPPED), uap->status);
	if (u.u_error)
		return;
	p->p_flag |= SWTED;
	u.u_r.r_val1 = p->p_pid;
}

/*
 * Copy the exit status back to userland
 */
statusout(kstatus, ustatusp)
	int kstatus;
	union wait *ustatusp;
{

	if (ustatusp == (union wait *)&u.u_r.r_val2) {
		/*
		 * Compatibility for old wait and wait3
		 */
		u.u_r.r_val2 = kstatus;
		return (0);
	} else {
		/*
		 * wait4
		 */
		if (ustatusp == NULL) {
			return (0);
		}
		return (copyout((caddr_t)&kstatus,
		    (caddr_t)&ustatusp->w_status, sizeof (kstatus)));
	}
}
