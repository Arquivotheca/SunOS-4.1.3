/*	@(#)kern_fork.c 1.1 92/07/30 SMI; from UCB 7.1 6/5/86	*/

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
#include <sys/vnode.h>
#include <sys/vm.h>
#include <sys/file.h>
#include <sys/acct.h>
#include <sys/trace.h>
#include <sys/mman.h>
#include <sys/debug.h>

#ifdef sun386
#include <machine/cpu.h>
#endif

#ifdef sparc
#include <machine/asm_linkage.h>
#include <machine/frame.h>
extern void fork_rtt();
#endif

#include <machine/reg.h>
#include <machine/pte.h>
#include <machine/psl.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_u.h>
#include <vm/page.h>
#include <vm/rm.h>
#include <machine/seg_kmem.h>

#ifndef	MULTIPROCESSOR
static int mpid = 0;	/* used to generate pids */
#else	MULTIPROCESSOR
static int mpid = -1;	/* used to generate pids */
#endif	MULTIPROCESSOR

#ifdef	MULTIPROCESSOR
#include "percpu.h"
#endif

int newproc();

/*
 * fork system call.
 */
fork()
{

	fork1(0);
}

vfork()
{

	fork1(1);
}

fork1(isvfork)
	int isvfork;
{
	register struct proc *p1, *p2;
	register int a;
	struct as *newas = NULL;
	struct seguser *seg;
	extern int maxuprc;
#if defined(sun3) || defined(sun3x)
	extern void userstackset();
#endif sun3 || sun3x

	/*
	 * These only needed for FPU-sun support. Define always
	 * to simplify interface to newproc.
	 */
	int new_context = 0;	/* FPA context of child process */
	struct file *nfp;	/* to be assigned to u.u_ofile[] of child */

	/* Count the number of processes owned by client in a */
	a = 0;
	if ((u.u_uid != 0) || (u.u_ruid != 0)) {
		for (p1 = allproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u.u_ruid)
				a++;
		for (p1 = zombproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u.u_ruid)
				a++;
	}
	p2 = freeproc;
	if (p2 == NULL) {
		/*
		 * System process table is full.
		 */
		tablefull("proc");
		u.u_error = EAGAIN;
		goto out;
	}
	freeproc = p2->p_nxt;		/* remove from free list */
	if (u.u_uid != 0 && u.u_ruid != 0) {
		/*
		 * User is not privileged (neither their real nor their
		 * effective UID is super-user); don't allow them to take
		 * the last process slot or to take more than "maxuprc"
		 * process slots.
		 */
		if (p2->p_nxt == NULL || a > (maxuprc - 1)) {
			u.u_error = EAGAIN;
			goto out;
		}
	}
#ifdef sun
#ifdef FPU
	if (!fpu_fork_context(p2, &nfp, &new_context)) {
		/*
		 * u.u_error set as side effect (maybe).
		 *
		 * But, on the other hand, we may not be able to
		 * guarantee that new_context was left untouched.
		 */
		new_context = 0;
		goto out;
	}
#endif FPU
#endif sun
	p1 = u.u_procp;
	if (p1->p_as != NULL) {
		/*
		 * Duplicate address space of current process. For
		 * vfork we just share the address space structure.
		 */
		if (isvfork) {
			newas = p1->p_as;
		} else {
			newas = as_dup(p1->p_as);
			if (newas == NULL) {
				u.u_error = ENOMEM;
				goto out;
			}
		}
	}
	seg = (struct seguser *) segu_get();
	if (seg == (struct seguser *)0) {
		u.u_error = ENOMEM;
		goto out;
	}
#ifdef sparc
	{
		struct frame *fp = (struct frame *)KSTACK(seg);
		fp->fr_local[6] = (u_int)fork_rtt;
		fp->fr_local[5] = 0;	/* means fork a user process */
	}
#endif sparc
#if defined(sun3) || defined(sun2) || defined(sun3x)
	userstackset(seg);
#endif
	newproc(isvfork, newas, new_context, nfp, seg, p2);
	u.u_r.r_val1 = p2->p_pid;
out:
	if (u.u_error) {
		if (p2) {
			p2->p_nxt = freeproc;
			freeproc = p2;
		}
#ifdef	sun
#ifdef	FPU
#ifdef	sparc
		/*
		 * If we had an error, and a fpu context was
		 * successfully allocated, we had better remember
		 * to free it. This only applies to sparc machines.
		 * Yes. This is icky.
		 */
		if (new_context)
			kmem_free((caddr_t) new_context, sizeof (struct fpu));
#endif	sparc
#endif	FPU
#endif	sun
	}
	u.u_r.r_val2 = 0;
}

/*
 * Create a new process-- the internal version of sys fork.
 * Note: newproc duplicates only the process slot, not the address space.
 * Processes are created starting in slot 1, a fact used by init_main
 * to create init, pageout and sched in well-known slots.
 * newproc always returns as the parent.
 */

/* ARGSUSED */
newproc(isvfork, newas, new_context, nfp, seg, pp)
	int isvfork;
	struct as *newas;
	int new_context;	/* FPA context of child process */
	struct file *nfp;	/* to be assigned to u.u_ofile[] of child */
	struct seguser *seg;
	struct proc *pp;
{
	register struct proc *rpp, *rip;
	register int n;
	register struct file *fp;
	static int pidchecked = 0;
#ifdef VPIX
	extern char v86procflag;
#endif

	/*
	 * First, just locate a slot for a process
	 * and copy the useful info from this process into it.
	 * The panic "cannot happen" because fork has already
	 * checked for the existence of a slot.
	 */
	mpid++;
retry:
#ifdef	MULTIPROCESSOR
	if (allproc == NULL){               /* first time around */
		rpp = proc;
		freeproc = rpp->p_nxt;
		rpp->p_nxt = allproc;
		rpp->p_prev = &allproc;
		allproc = rpp;
		goto skip;
	}
#endif	MULTIPROCESSOR
	if (mpid >= MAXPID) {
		mpid = 100;	/* don't trample pageout, sched, or init */
		pidchecked = 0;
	}
	if (mpid >= pidchecked) {
		int doingzomb = 0;

		pidchecked = MAXPID;
		/*
		 * Scan the proc table to check whether this pid
		 * is in use.  Remember the lowest pid that's greater
		 * than mpid, so we can avoid checking for a while.
		 */
		rpp = allproc;
again:
		for (; rpp != NULL; rpp = rpp->p_nxt) {
			while (rpp->p_pid == mpid || rpp->p_pgrp == mpid) {
				mpid++;
				if (mpid >= pidchecked)
					goto retry;
			}
			if (rpp->p_pid > mpid && pidchecked > rpp->p_pid)
				pidchecked = rpp->p_pid;
			if (rpp->p_pgrp > mpid && pidchecked > rpp->p_pgrp)
				pidchecked = rpp->p_pgrp;
		}
		if (!doingzomb) {
			doingzomb = 1;
			rpp = zombproc;
			goto again;
		}
	}
	rpp = pp;
	rpp->p_nxt = allproc;			/* onto allproc */
	rpp->p_nxt->p_prev = &rpp->p_nxt;	/*   (allproc is never NULL) */
	rpp->p_prev = &allproc;
	allproc = rpp;

	/*
	 * Make a proc table entry for the new process.
	 */
#ifdef	MULTIPROCESSOR
skip:
#endif	MULTIPROCESSOR
	rip = u.u_procp;
	rpp->p_stat = SIDL;
	timerclear(&rpp->p_realtimer.it_value);
	rpp->p_flag = SLOAD | (rip->p_flag &(SFAVORD|SPAGI|SNOCLDSTOP|SORPHAN));
	if (isvfork)
		rpp->p_flag |= SVFORK;
	rpp->p_uid = rip->p_uid;
	rpp->p_suid = rip->p_suid;
#ifdef VPIX
	rpp->p_v86 = NULL;
	if (v86procflag)
		rpp->p_suid = 0;
#endif
	rpp->p_sgid = rip->p_sgid;
	rpp->p_pgrp = rip->p_pgrp;
	rpp->p_cred = rip->p_cred;
	rpp->p_nice = rip->p_nice;
	rpp->p_pid = mpid;
	rpp->p_ppid = rip->p_pid;
	rpp->p_pptr = rip;
	rpp->p_osptr = rip->p_cptr;
	if (rip->p_cptr)
		rip->p_cptr->p_ysptr = rpp;
	rpp->p_ysptr = NULL;
	rpp->p_cptr = NULL;
	rip->p_cptr = rpp;
	rpp->p_time = 0;
	rpp->p_cpu = 0;
	rpp->p_sigmask = rip->p_sigmask;
	rpp->p_sigcatch = rip->p_sigcatch;
	rpp->p_sigignore = rip->p_sigignore;
	/*
	 * should we take along any pending signals like stops?
	 * POSIX says not to take them, so we don't (1003.1, page 49, item 8)
	 */
	rpp->p_tsize = rip->p_tsize;
	rpp->p_dsize = rip->p_dsize;
	rpp->p_ssize = rip->p_ssize;
	if (isvfork) {
		forkstat.sizvfork += rip->p_dsize + rip->p_ssize;
		forkstat.cntvfork++;
	} else {
		forkstat.sizfork += rip->p_dsize + rip->p_ssize;
		forkstat.cntfork++;
	}
	rpp->p_rssize = 0;
	rpp->p_maxrss = rip->p_maxrss;
	rpp->p_wchan = 0;
	rpp->p_slptime = 0;
	rpp->p_pctcpu = 0;
	rpp->p_cpticks = 0;
	rpp->p_swlocks = 0;
	rpp->p_as = newas;	/* give child her address space */
	rpp->p_segu = seg;	/* give child her u segment */
#ifdef LWP
	rpp->p_threadcnt = 1;	/* 1 lwp (this guy) using this address space */
#ifdef ASYNCHIO
	rpp->p_aio_forw = rpp->p_aio_back = 0;
	rpp->p_aio_count = 0;
#endif ASYNCHIO
#endif LWP
#if defined(MULTIPROCESSOR)
        if (rip == &idleproc) {
                rpp->p_pptr = &proc[0];
                rpp->p_ppid = 0;
		rpp->p_pgrp = 0;
        }
#endif /* MULTIPROCESSOR */
	n = PIDHASH(rpp->p_pid);
	rpp->p_idhash = pidhash[n];
	pidhash[n] = rpp - proc;
	multprog++;
	pgenter(rpp, rpp->p_pgrp);
	ASSERT(rip->p_sessp);
	(rpp->p_sessp = rip->p_sessp)->s_members++;

	/*
	 * Increase reference counts on shared objects.
	 */
#ifdef	SYSAUDIT
	cwincr(u.u_cwd);
#endif	SYSAUDIT
	for (n = 0; n <= u.u_lastfile; n++) {
		fp = u.u_ofile[n];
		if (fp == NULL)
			continue;
		fp->f_count++;
	}
#ifdef sun386
	/* ZZZ: is this REALLY necessary? */
	fpsave(FPCHECK);	/* save floating point chip state */
#endif
#ifdef sun
#ifdef FPU
	fpu_ofile();
#endif FPU
#endif sun
	VN_HOLD(u.u_cdir);
	if (u.u_rdir)
		VN_HOLD(u.u_rdir);
	crhold(u.u_cred);

	trace5(TR_PROC_FORK, mpid, trs(u.u_comm, 0), trs(u.u_comm, 1),
		trs(u.u_comm, 2), trs(u.u_comm, 3));
	/*
	 * Copy parent's state to child and start child running.
	 * Note that u-area is not swappable now, so
	 * no need for SKEEP.
	 */
	start_child(rip, uunix, rpp, nfp, new_context, seg);
	if (isvfork) {	/* wait for child to exit or exec */
		while (rpp->p_flag & SVFORK) {
			(void) sleep((caddr_t)rpp, PZERO - 1);
		}
		rpp->p_flag |= SVFDONE;

		/*
		 * Copy back sizes to parent; child may have grown
		 * We hope that this is the only info outside the
		 * struct as that needs to be shared like this!
		 */
		rip->p_tsize = rpp->p_tsize;
		rip->p_dsize = rpp->p_dsize;
		rip->p_ssize = rpp->p_ssize;
		wakeup((caddr_t)rpp);	/* wakeup child */
	}
}

/*
 * in locore.s:
 * start_child(parentp, parentu, childp, nfp, new_context, seg)
 * 	struct proc *parentp, *childp;
 * 	struct user *parentu;
 * 	int new_context;
 * 	struct file *nfp;
 *	struct seguser *seg;
 * This saves the parent's state such that when resume'd, the
 * parent will return to the caller of startchild.
 * It then calls conschild to create the child process.
 */

/*
 * Create child and start it running since in most cases, the parent
 * just waits for child to exit or exec anyway.
 * Called from start_child in locore.s.
 */
/* ARGSUSED */
cons_child(parentp, parentu, childp, nfp, new_context, seg)
	struct proc *parentp, *childp;
	struct user *parentu;
	struct file *nfp;	/* to be assigned to u.u_ofile[] of child */
	int new_context;
	struct seguser *seg;
{
	register int a;
	register struct user *childu;

#ifdef	MULTIPROCESSOR
	struct sbus_vme_mm *mmp, *smmp;
	extern struct sbus_vme_mm *mm_find_proc();
	extern struct sbus_vme_mm *mm_base;
#endif	MULTIPROCESSOR

	childp->p_uarea = childu = KUSER(seg);

#if defined(sun4) || defined (sun4c) || defined (sun4m)
	/*
	 * We must flush the user windows out before doing the
	 * bcopy of the parent's u to the child's u.  If this
	 * flush were done later, then only the parent's u would
	 * have its UWM bits updated, and possibly registers saved
	 * in its register window save area (part of u-area).
	 */
	flush_windows();
#endif sun4m || sun4c || sun4m

	/* copy parent's state to child */
	bcopy((caddr_t)parentu, (caddr_t)childu, sizeof (struct user));

#ifdef	MULTIPROCESSOR
	if (parentu->u_pcb.pcb_flags & MM_DEVS) {
		smmp = mm_base;
		while ((mmp = mm_find_proc(parentp, smmp)) != NULL) {
			mm_add(childp, mmp->mm_type, mmp->mm_paddr,
				mmp->mm_len);
			smmp = mmp->mm_next;
		}
	}
#endif	MULTIPROCESSOR


	/* copy saved bus error state */
#if defined(sun3) || defined(sun3x)
	if (parentu->u_pcb.u_berr_stack != NULL) {
		childu->u_pcb.u_berr_stack = (struct user_buserr_stack *)
			new_kmem_alloc(sizeof (struct user_buserr_stack),
					KMEM_SLEEP);
		bcopy((caddr_t)parentu->u_pcb.u_berr_stack,
			(caddr_t)childu->u_pcb.u_berr_stack,
			sizeof (struct user_buserr_stack));
	}
#endif sun3 || sun3x
	/*
	 * If the parent's high water mark for open files is over
	 * the threshold, its u_ofile and u_pofile arrays are stored
	 * outside of its u-area, so we have to allocate space for
	 * them in the child and copy them over.
	 */
	if (parentu->u_ofile != parentu->u_ofile_arr) {
		/*
		 * XXX:	Doesn't handle allocation failure properly.
		 *	The fix is to revise the interface to cons_child
		 *	to take a pointer to a child_resource (or some
		 *	such) structure, and to allocate higher in the
		 *	call chain, only hooking resources into place
		 *	here.
		 */
		childu->u_ofile = (struct file **)
			new_kmem_alloc(NOFILE * sizeof *(childu->u_ofile),
					KMEM_SLEEP);
		if (childu->u_ofile == NULL)
			panic("cons_child: no u_ofile");
		childu->u_pofile = (char *)
			new_kmem_alloc(NOFILE * sizeof *(childu->u_pofile),
					KMEM_SLEEP);
		if (childu->u_pofile == NULL)
			panic("cons_child: no u_pofile");

		bcopy((caddr_t)parentu->u_ofile, (caddr_t)childu->u_ofile,
			NOFILE * sizeof *(childu->u_ofile));
		bcopy((caddr_t)parentu->u_pofile, (caddr_t)childu->u_pofile,
			NOFILE * sizeof *(childu->u_pofile));
	} else {
		/*
		 * Fix child u_ofile and u_pofile values to point
		 * within the child's u-area.
		 */
		childu->u_ofile = childu->u_ofile_arr;
		childu->u_pofile = childu->u_pofile_arr;
	}

	/*
	 * clear vm statistics of new process
	 * inefficient: we just bcopied this.
	 * However, the small savings aren't worth it for now
	 * to partition the u area into copy-on-fork and non-copy-on-fork
	 * stuff.
	 * Eventually we will have pagable kernel state attached to the
	 * proc (e.g., dynamically growing numbers of descriptors).
	 * It is only necessary to have a kernel stack pre-arranged to
	 * take faults on.
	 * On system call or page fault, a kernel stack should be dynamically
	 * given the process, by donating the current kernel stack and replacing
	 * the "default" kernel stack.
	 * Once this mechanism is in place, there is no requirement on
	 * kernel data structures to be wired down and they can be paged.
	 * Things like proc which contain frequently-used lists, should
	 * be advised to be wired down.
	 *
	 * XXX should allocate pagesize multiple for u-area
	 * KERNSTACK is no longer accurate.
	 */
	bzero((caddr_t)&childu->u_ru, sizeof (struct rusage));
	bzero((caddr_t)&childu->u_cru, sizeof (struct rusage));

	/* give child a kernel stack */
	childp->p_stack = KSTACK(seg);

#ifdef sun
#ifdef FPU
	fpu_newproc(nfp, new_context, childu);
#endif FPU
#endif sun

	/* These fields are also bcopied needlessly */
	childu->u_start = time;
	childu->u_acflag = AFORK;
	childu->u_ioch = 0;
	childu->u_procp = childp;

	/*
	 * If process has no user address space (kernel-only process) the
	 * saved user registers pointer has no meaning.
	 */
	if (childp->p_as == NULL)
		childu->u_ar0 = NULL;
#ifdef sparc
	else
		childu->u_ar0 = (int *)(childp->p_stack + MINFRAME);
#endif sparc
		

	/*
	 * Child must not inherit file-descriptor-oriented locks
	 * (i.e., System-V style record-locking)
	 */
	for (a = 0; a < childu->u_lastfile; a++) {
		childu->u_pofile[a] &= ~UF_FDLOCK;
	}

	/*
	 * Make parent runnable and add to run queue
	 * Since u.u_procp, etc. doesn't change until we yield_child,
	 * a clock interrupt could change the parent's priority.
	 * Then, the setrq actually lied and a subsequent context switch
	 * will find the parent's priority and queue inconsistent.
	 */
	(void) splclock();
	parentp->p_stat = SRUN;
	childp->p_stat = SRUN;
#ifdef	MULTIPROCESSOR
	childp->p_cpuid = cpuid;
	childp->p_pam = parentp->p_pam;	/* inherit affinity mask */
#endif	MULTIPROCESSOR
	setrq(parentp);

	/*
	 * yield to child, restoring original parent user-level regs
	 * and child return values (parent pid, 1).
	 * Also, switches masterprocp and uunix.
	 * In locore.s.
	 */
	yield_child(parentu->u_ar0, childp->p_stack, childp,
			parentp->p_pid, childu, seg);
	panic("cons_child: yield_child returned");
	/* NOTREACHED */
}

/*
 * Release virtual memory.
 * Called by exit and getxfile (via execve).
 * Also called for "user" processes thar live forever in the
 * kernel and don't need user memory: async_daemon and nfs_svc.
 */
relvm(p)
	register struct proc *p;	/* process exiting or exec'ing */
{

	if ((p->p_flag & SVFORK) == 0) {
		if (p->p_as != NULL) {
			struct  as *as = p->p_as;

#ifdef	MULTIPROCESSOR
			register struct user *pu = p->p_uarea;

			if (pu->u_pcb.pcb_flags & MM_DEVS)
				mm_delete_proc(p);
#endif	MULTIPROCESSOR
			p->p_as = NULL;	/* First detach as from proc, then */
			as_free(as);	/* blow it away (avoids races) */
		}
	} else {
		p->p_flag &= ~SVFORK;	/* no longer a vforked process */
		p->p_as = NULL;		/* no longer using parent's adr space */
		wakeup((caddr_t)p);	/* wake up parent */
		while ((p->p_flag & SVFDONE) == 0) {	/* wait for parent */
			(void) sleep((caddr_t)p, PZERO - 1);
		}
		p->p_flag &= ~SVFDONE;	/* continue on to die or exec */
	}
}
