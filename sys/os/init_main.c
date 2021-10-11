/*	@(#)init_main.c 1.1 92/07/30 SMI; from UCB 4.53 83/07/01	*/

#include <machine/pte.h>

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/vfs.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/session.h>
#include <sys/buf.h>
#include <sys/vm.h>
#include <sys/clist.h>
#include <sys/vnode.h>
#ifdef QUOTA
#include <ufs/quota.h>
#endif
#ifdef TRACE
#include <sys/trace.h>
#endif
#include <sys/systm.h>
#include <sys/bootconf.h>
#include <sys/reboot.h>
#include <machine/mmu.h>
#include <machine/reg.h>
#ifdef sun386
#include <machine/cpu.h>
#endif

#include <vm/swap.h>

#ifndef	MULTIPROCESSOR
#define	cpuid	(0)
#else	MULTIPROCESSOR
#include "percpu.h"
#endif	MULTIPROCESSOR

extern	void	sched();

#ifdef	MULTIPROCESSOR
int	initing;
#endif	MULTIPROCESSOR

extern	int	nmod;

int	swap_present = 0;
int	cmask = CMASK;
/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 1 execute bootstrap
 *	     - process 2 to page out
 */
#if defined(vax) || defined(sun386)
main(firstaddr)
	int firstaddr;
#endif
#if defined(sun2) || defined(sun3) || defined(sun3x)
main(regs)
	struct regs regs;
#endif

#if defined(sun4) || defined(sun4c) || defined(sun4m)
main(rp)
	struct regs *rp;
#endif sun4 || sun4c
{
	register int i;
	register struct proc *p;
	int s;
	extern struct seguser ubasic;
	extern void pageout();
	extern void icode1();
#ifdef MULTIPROCESSOR
        extern dnode_t prom_childnode();
        extern dnode_t prom_nextnode();
        extern addr_t map_regs();
#endif
	extern struct proc *masterprocp;
#ifdef SYSAUDIT
	register char *cp;
#endif

#if defined(AT386) || defined(SUN386)
	extern   int interupts_are_on, patchmein, patchmeout, kadbpatchme;
#endif

#ifdef AT386
	*(short *)0x472 = 0x1234;	/* prevent mem test on each boot */
	Vidsetm();			/* set up ega */
#endif

#if defined(AT386) || defined(SUN386)
	patchmein = 1;			/* get  intput from console */
	patchmeout = 1;			/* send output to console */
#endif

#ifdef sun4m
	atom_setup();
#endif

	rqinit();
#include "loop.h"
#if defined(vax) || defined(sun386)
	startup(firstaddr);
#elif !defined(MULTIPROCESSOR)
	startup();
#endif

	/*
	 * set up system process 0 (swapper)
	 * uunix is already set up.
	 */
#ifndef	MULTIPROCESSOR
	p = &proc[0];
#else	MULTIPROCESSOR
	p = &idleproc;

	/*
	 * set up some pseudoconstants
	 * in the per-cpu area
	 */
	cpuid = getprocessorid();
	cpu_exists = 1;
	cpu_enable = 1;

	initing = 1;
	p->p_pid = -1;
	p->p_pgrp = -1;
	p->p_cpuid = cpuid;
	p->p_pam = ~0;		/* can run anywhere */
#endif	MULTIPROCESSOR
	p->p_segu = &ubasic;
	p->p_stack = KSTACK(p->p_segu);
	p->p_uarea = KUSER(p->p_segu);

	p->p_stat = SRUN;
	p->p_flag |= SORPHAN|SLOAD|SSYS;
	p->p_nice = NZERO;
	p->p_cpu = 0;
	p->p_wchan = 0;
	masterprocp = p;

#ifdef	MULTIPROCESSOR
	startup();
#endif

	u.u_procp = p;
#if defined(sun2) || defined(sun3) || defined(sun3x)
	u.u_ar0 = &regs.r_r0;
#endif
#if defined(sun4) || defined(sun4c) || defined(sun4m)
	u.u_ar0 = (int *)rp;
#endif sun4 || sun4c
	u.u_cmask = cmask;
	u.u_lastfile = -1;
	u.u_ofile = u.u_ofile_arr;
	u.u_pofile = u.u_pofile_arr;
	for (i = 0; i < sizeof (u.u_rlimit) / sizeof (u.u_rlimit[0]); i++)
		u.u_rlimit[i].rlim_cur = u.u_rlimit[i].rlim_max =
		    RLIM_INFINITY;
	u.u_rlimit[RLIMIT_STACK].rlim_cur = DFLSSIZ;
	u.u_rlimit[RLIMIT_STACK].rlim_max = MAXSSIZ;
	u.u_rlimit[RLIMIT_DATA].rlim_cur = DFLDSIZ;
	u.u_rlimit[RLIMIT_DATA].rlim_max = MAXDSIZ;
	u.u_rlimit[RLIMIT_NOFILE].rlim_max = NOFILE;
	u.u_rlimit[RLIMIT_NOFILE].rlim_cur = NOFILE_IN_U;
	p->p_maxrss = RLIM_INFINITY/NBPG;
	p->p_sessp = &sess0;
	sess0.s_members = 1;		/* all other fields are NULL */
	pgenter(p, p->p_pgrp);

	/*
	 * Set up credentials
	 */
	u.u_cred = crget();
	for (i = 1; i < NGROUPS; i++)
		u.u_groups[i] = NOGROUP;

	/* Set this up so kernel processes are not audited */
	u.u_auid = -2;

	/*
	 * Build cwd structure
	 */
#ifdef	SYSAUDIT
	/* Size of CWD structure + 2 null strings */
	i = sizeof (struct ucwd) + 2;
	u.u_cwd = (struct ucwd *)new_kmem_alloc((unsigned)i, KMEM_SLEEP);
	u.u_cwd->cw_ref = 1;
	u.u_cwd->cw_len = i;
	u.u_cwd->cw_dir = cp = (char *)&(u.u_cwd[1]);
	*cp++ = '\0';
	u.u_cwd->cw_root = cp;
	*cp = '\0';
#else
	u.u_cwd = (struct ucwd *)0;
#endif	SYSAUDIT

#ifdef	LWP
	lwpinit();
#endif	LWP

#ifdef IPCMESSAGE
	msginit();		/* init SysV IPC message facility */
#endif

#ifdef IPCSEMAPHORE
	seminit();		/* init SysV IPC semaphore facility */
#endif

#ifdef QUOTA
	qtinit();
#endif
	startrtclock();
#ifdef vax
#include "kg.h"
#if NKG > 0
	startkgclock();
#endif
#endif

	/*
	 * Initialize tables, protocols
	 */
	mbinit();
	cinit();			/* needed by dmc-11 driver */
#ifdef INET
#if NLOOP > 0
	loattach();			/* XXX */
#endif
	/*
	 * Block reception of incoming packets
	 * until protocols have been initialized.
	 */
	s = splimp();
	ifinit();
#endif
	domaininit();
#ifdef INET
	(void) splx(s);
#endif
	pqinit();	/* initialize allproc: p is the only guy on this q */
	bhinit();
	binit();
	dnlc_init();
	strinit();

#ifdef TRACE
	/*
	 * Get initial event trace buffer.  (This will only
	 * grab space if the tracebufents variable has been
	 * patched nonzero.)
	 */
	inittrace();
#endif TRACE

#ifdef GPROF
	kmstartup();
#endif

	/*
	 * Set the scan rate and other parameters of the paging subsystem.
	 */
	setupclock();

	/*
	 * kick off timeout driven events by calling first time
	 */
	roundrobin();
	schedcpu();

	/*
	 * mount the root, gets rootdir
	 */
	vfs_mountroot();
	boottime = time;

	/*
	 * now that we have the rootvp, initialize swap and dump information
	 */
	swapconf();
	if (!swap_present)
		halt("Trying To Swap On A non-Existent Device");
	if (swap_init(swapfile.bo_vp))
		panic("swap_init");
	dumpinit();

#ifdef	sun
	consconfig();
#endif

	schedpaging();

	/*
	 * The next two calls to kern_proc must be in this order
	 * in order to place them in well-known slots 1 and 2.
	 * We make process 1 (init) and process 2 (pageout).
	 * process 0 (sched) and pageout kernel-only processes.
	 * init is called via icode.
	 * icode is called by icode1, which simulates a system call
	 * environment so icode can use execve (which in turn uses
	 * setregs via getxfile to set an entry point).
	 * icode must also allocate an as for process 1.
	 */
#ifdef	MULTIPROCESSOR
	kern_proc(sched, 0);	/* make sched process (0) */
#endif	MULTIPROCESSOR

	kern_proc(icode1, 0);	/* make init process (1) */
	kern_proc(pageout, 0);	/* make pageout daemon (2) */

#ifndef	MULTIPROCESSOR
	sched();		/* process 0 continues */
	/*NOTREACHED*/
#else	MULTIPROCESSOR
	initing = 0;
		
	idlework();
#endif	MULTIPROCESSOR
	/*NOTREACHED*/
}

/*
 * Initialize hash links for buffers.
 */
bhinit()
{
	register int i;
	register struct bufhd *bp;

	for (bp = bufhash, i = 0; i < BUFHSZ; i++, bp++)
		bp->b_forw = bp->b_back = (struct buf *)bp;
}

/*
 * Initialize the buffer I/O system by
 * setting all device buffer lists to empty.
 */
binit()
{
	register struct buf *dp;

	for (dp = bfreelist; dp < &bfreelist[BQUEUES]; dp++) {
		dp->b_forw = dp->b_back = dp->av_forw = dp->av_back = dp;
		dp->b_flags = B_HEAD;
	}
}

/*
 * Initialize the dump vnode.
 */
dumpinit()
{
	extern int dump_max;

	/*
	 * set up dump file
	 * XXX this should be done somewhere else
	 */
	if (dumpfile.bo_vp == NULL) {
		dumpfile = swapfile;
	}
	dumpvp = dumpfile.bo_vp;
	VN_HOLD(dumpvp);
	if (dumplo < 0)
		dumplo = 0;
	dumphdr_init();
	printf("dump on %s fstype %s size %dK\n",
		dumpfile.bo_name, dumpfile.bo_fstype,
		(dump_max * PAGESIZE)/1024);
}

/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
cinit()
{
	register int ccp;
	register struct cblock *cp;

	ccp = (int)cfree;
	ccp = (ccp+CROUND) & ~CROUND;
	for (cp = (struct cblock *)ccp; cp < &cfree[nclist-1]; cp++) {
		cp->c_next = cfreelist;
		cfreelist = cp;
		cfreecount += CBSIZE;
	}
}


#ifdef	MULTIPROCESSOR

extern	idle();
extern	int whichqs;
extern	char qrunflag;

#ifdef	LWP
extern int	__Nrunnable;
#endif	LWP

idlework()
{
	register int	delay = 0;

	cpu_exists = 1;
	cpu_enable = 1;

	if (cpuid != 0) {
		klock_exit();
		while (initing)
			;	/* sit and spin */
	}

	while (1) {
		/*
		 * Remember, any trap we take could land us
		 * back inside the klock, so if we are truely
		 * idle, verify that we don't hold it.
		 */
		klock_exit();

		(void)spl0();
		if (!cpu_idle)
			cpu_idle = 1;	/* we really are idle! */
		if (cpu_supv)
			cpu_supv = 0; 	/* shut off the SUPV light. */
		if (!cpu_enable)
			continue; /* someone disabled this processor! */

		/*
		 * If we recently entered the lock,
		 * avoid trying to enter it again.
		 */
		if (delay>0) {
			delay--;
			continue;
		}

		if ((
#ifdef	LWP
		     __Nrunnable ||
#endif	LWP
		     whichqs) &&
		    klock_knock()) {
			idle();	/* the real dispatcher */
			delay = 1000;
		}

		if (qrunflag && klock_knock()) {
			klock_enter();
			if (cpu_idle)
				cpu_idle = 0; /* not idle while streams are moving */
			runqueues();
			delay = 1000;
		}

#if defined(SAS)
		if (!whichqs)
			mpsas_idle_trap();
#endif
	}
}
#endif	MULTIPROCESSOR
