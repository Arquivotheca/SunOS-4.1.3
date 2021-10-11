#ident "@(#)trap.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/kernel.h>
#include <sys/msgbuf.h>
#include <sys/trace.h>
#include <sun/fault.h>
#include <machine/frame.h>
#include <machine/asm_linkage.h>
#include <machine/buserr.h>
#include <machine/mmu.h>
#include <machine/pcb.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/vm_hat.h>
#include <machine/seg_kmem.h>
#include <machine/memerr.h>
#include <vm/as.h>
#include <vm/seg.h>

#include <sys/reboot.h>
#include <debug/debug.h>

#define	USER	0x10000		/* user-mode flag added to type */

#ifdef SYSCALLTRACE
extern char* syscallnames[];
#endif /* SYSCALLTRACE */

extern struct sysent sysent[];
extern int nsysent;

char	*trap_type[] = {
	"Zero",
	"Text fault",
	"Illegal instruction",
	"Privileged instruction",
	"Floating point unit disabled",
	"Window overflow",
	"Window underflow",
	"Memory address alignment",
	"Floating point exception",
	"Data fault",
	"Tag overflow",
	"Trap 0x0B",
	"Trap 0x0C",
	"Trap 0x0D",
	"Trap 0x0E",
	"Trap 0x0F",
	"Spurious interrupt",
	"Interrupt level 1",
	"Interrupt level 2",
	"Interrupt level 3",
	"Interrupt level 4",
	"Interrupt level 5",
	"Interrupt level 6",
	"Interrupt level 7",
	"Interrupt level 8",
	"Interrupt level 9",
	"Interrupt level A",
	"Interrupt level B",
	"Interrupt level C",
	"Interrupt level D",
	"Interrupt level E",
	"Interrupt level F",
	"AST",
};

#define	TRAP_TYPES	(sizeof (trap_type) / sizeof (trap_type[0]))

int tudebug = 0;
int tudebugbpt = 0;
int tudebugfpe = 0;
int aligndebug = 0;
int alignfaults = 0;

#if defined(TRAPDEBUG) || defined(lint)
int tdebug = 0;
int lodebug = 0;
int faultdebug = 0;
#else
#define	tdebug	0
#define	lodebug	0
#define	faultdebug	0
#endif defined(TRAPDEBUG) || defined(lint)

#ifdef SUN4_110
int fault_type;
#endif

#define	SIMU_SUCCESS 1		/* simulation worked */
#define	SIMU_ILLEGAL 0		/* tried to simulate an illegal instruction */
#define	SIMU_FAULT -1		/* simulation generated an illegal access */
#define	SIMU_DZERO -2		/* divided by zero detected */

u_int beval = 0;		/* used by fp_traps() to setup a data pg fault*/
/*
 * Called from the trap handler when a processor trap occurs.
 * Addr, be and rw only are passed for text and data faults.
 */
/*VARARGS2*/
void
trap(type, rp, addr, be, rw)
	register unsigned type;
	register struct regs *rp;
	register addr_t addr;
	register u_int be;
	register enum seg_rw rw;
{
	register int i = 0;
	register struct proc *p;
	struct timeval syst;
	int lofault;
	faultcode_t pagefault(), res;
	int mask;

	p = u.u_procp;
	cnt.v_trap++;
	syst = u.u_ru.ru_stime;
	if (tdebug)
		showregs(type, rp, addr, be, rw);
	if (USERMODE(rp->r_psr)) {
		type |= USER;
		u.u_ar0 = (int *)rp;
	}



	/*
	 * take any pending floating point exceptions now
	 */
	syncfpu(rp);

	switch (type) {

	default:
		/*
		 * Check for user software trap.
		 */
		if (type & USER) {
			if (tudebug)
				showregs(type, rp, (addr_t)0, 0, S_OTHER);
			if (((type & ~USER) >= T_SOFTWARE_TRAP) ||
			    ((type & ~USER) & CP_BIT)) {
				u.u_addr = (char *)rp->r_pc;
				u.u_code = type &~ USER;
				i = SIGILL;
				break;
			}
		}
die:
		(void) spl7();
		printf("BAD TRAP\n");
		showregs(type, rp, addr, be, rw);
		traceback((caddr_t)rp->r_sp);
		if (type < TRAP_TYPES)
			panic(trap_type[type]);
		panic("trap");
		/* NOTREACHED */

	case T_TEXT_FAULT:		/* system text access fault */
		if (be & SE_MEMERR) {
			if (lodebug)
				showregs(type, rp, addr, be, rw);
			memerr(MERR_SYNC, be, addr, type, rp);
			/* memerr returns if recoverable, panics if not */
			return;
		}
		goto die;

	case T_DATA_FAULT:		/* system data access fault */
		/* may have been expected by C (e.g., bus probe) */
		if (nofault) {
			label_t *ftmp;

			ftmp = nofault;
			nofault = 0;
			longjmp(ftmp);
		}

		if (be & SE_MEMERR) {
			if (lodebug)
				showregs(type, rp, addr, be, rw);
			memerr(MERR_SYNC, be, addr, type, rp);
			/* memerr returns if recoverable, panics if not */
			return;
		}

#ifndef lint
		if (be & SE_SIZERR) {
			if (lodebug)
				showregs(type, rp, addr, be, rw);
		}
		/* may be fault caused by timeout on read of VME vector */
		if (be & SE_TIMEOUT) {
		/*
		 * XXX  FIX ME FIX ME- need to put in an SBUS
		 * spurious interrupt routine
		 */
#ifndef	sun4c
			extern int vme_read_vector(), spurious();

			if (lodebug)
				showregs(type, rp, addr, be, rw);
			if ((addr_t) rp->r_pc == (addr_t) vme_read_vector) {
				rp->r_pc = (int) spurious;
				rp->r_npc = (int) spurious + 4;
				return;
			}
#else
			/* Ignore timeout if lofault set */
			if (u.u_lofault) {
				if (lodebug) {
					showregs(type, rp, addr, be, rw);
					traceback((caddr_t)rp->r_sp);
				}
				rp->r_g1 = EFAULT;
				rp->r_pc = u.u_lofault;
				rp->r_npc = u.u_lofault + 4;
				return;
			}
			printf("KERNEL DATA ACCESS TIMEOUT\n");
			showregs(type, rp, addr, be, rw);
			traceback((caddr_t)rp->r_sp);
			panic("system bus error timeout\n");
			/*NOTREACHED*/
#endif
		}
#endif !lint
		/*
		 * There is a bug in Campus and Calvin, where a
		 * protection error on a LDSTUB (and perhaps SWAP?)
		 * will not set the RW bit in the SER.
		 * bugid 1040150 (hw), 1040151 (this workaround)
		 */
		if (be & SE_PROTERR && rw == S_READ && is_atomic(rp))
			rw = S_WRITE;

		/*
		 * See if we can handle as pagefault. Save lofault
		 * across this. Here we assume that an address
		 * less than KERNELBASE + sizeof (int) is a user fault.
		 * We can do this as copy.s routines verify that the
		 * starting address is less than KERNELBASE before
		 * starting and because we know that we always have
		 * KERNELBASE mapped as invalid to serve as a "barrier".
		 * Because of SF9010 bug (see below), we must validate
		 * the bus error register when more than one bit is on.
		 */
		if ((be & (be - 1)) != 0) {	/* more than one bit set? */
			struct pte pte;

			mmu_getpte(addr, &pte);
			if (pte_valid(&pte)) {
				if (be & SE_PROTERR &&
				    rw == S_WRITE && pte_ronly(&pte))
					be = SE_PROTERR;
				else
					be &= ~SE_PROTERR;
			} else {
				be = SE_INVALID;
			}
		} else if ((be & ~(SE_RW|SE_INVALID|SE_PROTERR)) != 0) {
			/*
			 * In this case, we might have a SBus Error.
			 * In any case, this is a gross hardware
			 * error that we should trap. In any case,
			 * we need to skip over the crap below which
			 * is trying to resolve page faults, etc..
			 */
			lofault = u.u_lofault;
			res = FC_HWERR;
			goto skip;
		}

		lofault = u.u_lofault;
		u.u_lofault = 0;

		if (addr < (addr_t)KERNELBASE + sizeof (int)) {
			if (lofault == 0) {
				goto die;
			}
			res = pagefault(addr,
				(be & SE_PROTERR)? F_PROT: F_INVAL, rw, 0);
			if (res != 0 && grow((int)addr))
				res = 0;
		} else {
			res = pagefault(addr,
				(be & SE_PROTERR)? F_PROT: F_INVAL, rw, 1);
		}
		/*
		 * Restore lofault. If we resolved the fault, exit.
		 * If we didn't and lofault wasn't set, die.
		 */
		u.u_lofault = lofault;
		if (res == 0)
			return;

#ifndef	XXXX	/* Bogus kernel page faults (P1_5 only?) */
		/*
		 * If the pme says this is a good page, print bogosity
		 * message and ignore it.
		 */
		if (good_addr(addr)) {
			struct pte pte;

			mmu_getpte(addr, &pte);
			if (pte_valid(&pte) && (rw != S_WRITE ||
			    !pte_ronly(&pte))) {
				printf("BOGUS page fault on valid page: ");
				showregs(type, rp, addr, be, rw);
				return;
			}
		}
#endif	XXXX

skip:
		if (lofault == 0)
			goto die;

		/*
		 * Cannot resolve fault.  Return to lofault.
		 */
		if (lodebug) {
			showregs(type, rp, addr, be, rw);
			traceback((caddr_t)rp->r_sp);
		}
		if (FC_CODE(res) == FC_OBJERR)
			res = FC_ERRNO(res);
		else
			res = EFAULT;
		rp->r_g1 = res;
		rp->r_pc = u.u_lofault;
		rp->r_npc = u.u_lofault + 4;
		return;

	case T_DATA_FAULT + USER:	/* user data access fault */
	case T_TEXT_FAULT + USER:	/* user text access fault */
		/*
		 * The SPARC processor prefetches instructions.
		 * The bus error register may also reflect faults
		 * that occur during prefetch in addition to the one
		 * that caused the current fault. For example:
		 *	st	[proterr]	! end of page
		 *	...			! invalid page
		 * will cause both SE_INVALID and SE_PROTERR.
#ifdef notdef
		 * If any of the unusual bus error register bits
		 * (SE_TIMEOUT, SE_SBBERR, SE_SIZERR)
		 * are on, we kill the user even though these errors
		 * could have occurred during some previous (possibly
		 * annulled) fetch. We do this because it is simple
		 * and the user had to have done something weird in
		 * order to get this. The downside of this is that
		 * the current pc is not necessarily the instruction
		 * that caused the bad fault.
#endif notdef
		 * The gate array version of sparc (SF9010) has a bug
		 * which works as follows: When the chip does a prefetch
		 * to an invalid page the board loads garbage into the
		 * chip. If this garbage looks like a branch instruction
		 * a prefetch will be generated to some random address
		 * even though the branch is annulled. This can cause
		 * bits in the bus error register to be set. In this
		 * case we have to validate the bus error register bits.
		 * We only handle true SE_INVALID and SE_PROTERR faults.
		 * SE_MEMERR is given to memerr(), which will handle
		 * recoverable parity errors.
		 * All others cause the user to die.
		 */

		if (be & SE_MEMERR) {
			memerr(MERR_SYNC, be, addr, type & ~USER, rp);
			/* memerr returns if recoverable, panics if not */
			goto out;
		}

		/*
		 * There is a bug in Campus and Calvin, where a
		 * protection error on a LDSTUB (and perhaps SWAP?)
		 * will not set the RW bit in the SER.
		 * bugid 1040150 (hw), 1040151 (this workaround)
		 */
		if (be & SE_PROTERR && rw == S_READ && is_atomic(rp))
			rw = S_WRITE;

		if ((be & (be - 1)) != 0) {	/* more than one bit set? */
			struct pte pte;

			mmu_getpte(addr, &pte);
			if (pte_valid(&pte)) {
				if (be & SE_PROTERR && (pte_konly(&pte) ||
					(rw == S_WRITE && pte_ronly(&pte))))
					be = SE_PROTERR;
				else
					be &= ~(SE_PROTERR|SE_INVALID);
			} else {
				be = SE_INVALID;
			}
		}
		if ((be & ~(SE_RW|SE_INVALID|SE_PROTERR)) != 0) {
			/*
			 * There was an error in the buserr register besides
			 * invalid and protection - we cannot handle it.
			 */
			res = FC_HWERR;
		} else {
#ifdef notdef
			/*
			 * If SE_INVALID and SE_PROTERR were both on
			 * figure out which one it really was.
			 */
			if (be == (SE_INVALID|SE_PROTERR)) {
				struct pte pte;

				mmu_getpte(addr, &pte);
				if (pte_valid(&pte))
					be = SE_PROTERR;
				else
					be = SE_INVALID;
			}
#endif notdef
			if (faultdebug) {
				char *fault_str;

				switch (rw) {
				case S_READ:
					fault_str = "read";
					break;
				case S_WRITE:
					fault_str = "write";
					break;
				case S_EXEC:
					fault_str = "exec";
					break;
				default:
					fault_str = "";
					break;
				}
				printf("user %s fault:  addr=0x%x be=0x%x\n",
				    fault_str, addr, be);
			}
#ifdef SUN4_110
			if (rw == S_EXEC)
				fault_type = T_TEXT_FAULT;
			else
				fault_type = T_DATA_FAULT;
#endif

			res = pagefault(addr,
				(be & SE_PROTERR)? F_PROT: F_INVAL, rw, 0);
			if (res == 0)
				return;

			if (type == T_DATA_FAULT + USER) {
				/*
				 * Grow the stack automatically.
				 */
				if (grow((int)addr))
					goto out;
			}

		}

		if (tudebug)
			showregs(type, rp, addr, be, rw);
		/*
		 * In the case where both pagefault and grow fail,
		 * set the code to the value provided by pagefault.
		 * We map errors relating to the underlying object
		 * to SIGBUS, others to SIGSEGV.
		 */
		u.u_addr = (char *)addr;
		u.u_code = res;
		switch (FC_CODE(res)) {
		case FC_HWERR:
		case FC_OBJERR:
			i = SIGBUS;
			break;
		default:
			i = SIGSEGV;
		}
		break;

	case T_ALIGNMENT + USER:	/* user alignment error */
		if (tudebug)
			showregs(type, rp, (addr_t)0, 0, S_OTHER);
		/*
		 * if the user has to do unaligned references
		 * the ugly stuff gets done here
		 */
		alignfaults++;
		if (u.u_pcb.pcb_flags & FIX_ALIGNMENT) {
			if (do_unaligned(rp)) {
				rp->r_pc = rp->r_npc;
				rp->r_npc += 4;
				return;
			}
		}
		u.u_addr = (char *)rp->r_pc;	/* not quite right */
		u.u_code = FC_ALIGN;
		i = SIGBUS;
		break;

	case T_PRIV_INSTR + USER:	/* privileged instruction fault */
		if (tudebug)
			showregs(type, rp, (addr_t)0, 0, S_OTHER);
		u.u_addr = (char *)rp->r_pc;
		u.u_code = ILL_PRIVINSTR_FAULT;
		i = SIGILL;
		break;

	case T_UNIMP_INSTR + USER:	/* illegal instruction fault */
		if (tudebug)
			showregs(type, rp, (addr_t)0, 0, S_OTHER);
		switch (simulate_unimp(rp)) {
		case SIMU_SUCCESS:
			/* skip the successfully simulated instruction */
			rp->r_pc = rp->r_npc;
			rp->r_npc += 4;
			return;
		case SIMU_FAULT:
			/*
			 * tried to reference an illegal address
			 * u.u_{addr, code} set in simulation called above
			 */
			i = SIGSEGV;
			break;
		case SIMU_DZERO:
			/* a simulated division had a divisor of zero */
			u.u_addr = (char *)rp->r_pc;
			u.u_code = (int)FPE_INTDIV_TRAP;
			i = SIGFPE;
			break;
		case SIMU_ILLEGAL:
		default:
			/* no such instruction */
			u.u_addr = (char *)rp->r_pc;
			u.u_code = ILL_ILLINSTR_FAULT;
			i = SIGILL;
			break;
		}
		break;

	case T_DIV0 + USER:		/* integer divide by zero */
		if (tudebug && tudebugfpe)
			showregs(type, rp, (addr_t)0, 0, S_OTHER);
		rp->r_pc = rp->r_npc;	/* skip trap instruction */
		rp->r_npc += 4;
		u.u_addr = (char *)rp->r_pc;
		u.u_code = (int)FPE_INTDIV_TRAP;
		i = SIGFPE;
		break;

	case T_INT_OVERFLOW + USER:	/* integer overflow */
		if (tudebug && tudebugfpe)
			showregs(type, rp, (addr_t)0, 0, S_OTHER);
		rp->r_pc = rp->r_npc;	/* skip trap instruction */
		rp->r_npc += 4;
		u.u_addr = (char *)rp->r_pc;
		u.u_code = (int)FPE_INTOVF_TRAP;
		i = SIGFPE;
		break;

	case T_FP_EXCEPTION + USER:	/* FPU arithmetic exception */
	case T_FP_EXCEPTION:
		if (tudebug && tudebugfpe)
			showregs(type, rp, addr, 0, S_OTHER);
		u.u_code = be;
		u.u_addr = (char *)addr;
		i = SIGFPE;
		break;

	case T_BREAKPOINT + USER:	/* breakpoint trap (t 1) */
		if (tudebug && tudebugbpt)
			showregs(type, rp, (addr_t)0, 0, S_OTHER);
		i = SIGTRAP;
		break;

	case T_TAG_OVERFLOW + USER:	/* tag overflow (taddcctv, tsubcctv) */
		if (tudebug)
			showregs(type, rp, (addr_t)0, 0, S_OTHER);
		u.u_addr = (char *)rp->r_pc;
		u.u_code = EMT_TAG;
		i = SIGEMT;
		break;

	case T_WIN_OVERFLOW + USER:	/* finish user window overflow */
		/*
		 * This trap is entered from sys_rtt in locore.s when, upon
		 * return to user is is found that there are user windows in
		 * u.u_wbuf. This happens because they could not be saved on
		 * the user stack, either because it wasn't resident or because
		 * it was misaligned. We flush the user's windows to make
		 * sure pcb_wbcnt does not change while we are doing this.
		 */
	    {
		register int j;

		flush_user_windows();
		for (j = 0; j < u.u_pcb.pcb_wbcnt; j++) {
			register char *sp;

			sp = u.u_pcb.pcb_spbuf[j];
			if (((int)sp & (STACK_ALIGN-1)) ||
				copyout((caddr_t)&u.u_pcb.pcb_wbuf[j], sp,
					sizeof (struct rwindow)) != 0) {
				u.u_addr = (char *)rp->r_pc;
				u.u_code = ILL_STACK;
				i = SIGILL;
				break;
			}
		}
		if (i == 0) {
			u.u_pcb.pcb_wbcnt = 0;
			return;
		}
		if (tudebug)
			showregs(type, rp, (addr_t)0, 0, S_OTHER);
	    }
		break;

	case T_AST + USER:		/* profiling or resched psuedo trap */
		astoff();
		if ((p->p_flag & SOWEUPC) && u.u_prof.pr_scale) {
			addupc(rp->r_pc, &u.u_prof, 1);
			p->p_flag &= ~SOWEUPC;
		}
		goto out;
	}

	/*
	 * Before trying to send a signal to the process, make
	 * sure it's not masked, otherwise we'll loop taking
	 * synchronous traps until someone notices and kills the
	 * process.
	 */
	mask = sigmask(i);
	if ((mask & p->p_sigmask) && (i != SIGFPE)) {
		u.u_signal[i] = SIG_DFL;
		p->p_sigignore &= ~mask;
		p->p_sigcatch &= ~mask;
		p->p_sigmask &= ~mask;
	}
	psignal(p, i);
out:
	if (((p->p_flag & STRC) == 0 && p->p_cursig) || ISSIG(p, 0))
		psig();
	p->p_pri = p->p_usrpri;
	if (runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		(void) spl6();
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
		(void) spl0();
	}
	if (u.u_prof.pr_scale) {
		int ticks;
		struct timeval *tv = &u.u_ru.ru_stime;

		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks)
			addupc(rp->r_pc, &u.u_prof, ticks);
	}
	curpri = p->p_pri;

}

#ifdef SYSCALLTRACE
int syscalltrace = 0;
#endif

/*
 * Called from the trap handler when a system call occurs
 */
void
syscall(rp)
	register struct regs *rp;
{
	register unsigned code;
	register struct sysent *callp;
	register struct proc *p;
	register syst_flag;
	struct timeval syst;
	register short pid;


	code = rp->r_g1;
	cnt.v_syscall++;
	if (u.u_prof.pr_scale) {
		syst = u.u_ru.ru_stime;
		syst_flag = 1;
		pid = u.u_procp->p_pid;
	} else
		syst_flag = 0;
#ifdef notdef
	if (!USERMODE(rp->r_psr))
		panic("syscall");
#endif
	/*
	 * take any pending floating point exceptions now
	 */
	syncfpu(rp);

	u.u_ar0 = (int *)rp;
	if (code >= nsysent)
		callp = &sysent[63];
	else
		callp = &sysent[code];
	u.u_error = 0;
	if ((callp->sy_narg > 6) || (u.u_procp->p_flag & STRCSYS)) {
		/*
		 * Copy registers and stack arguments into u.u_arg array.
		 */
		u.u_arg[0] = rp->r_o0; u.u_arg[1] = rp->r_o1;
		u.u_arg[2] = rp->r_o2; u.u_arg[3] = rp->r_o3;
		u.u_arg[4] = rp->r_o4; u.u_arg[5] = rp->r_o5;
		if (copyin((addr_t)rp->r_sp + MINFRAME, (addr_t)&u.u_arg[6],
			(u_int)((callp->sy_narg - 6) * sizeof (int)))) {
			u.u_error = EFAULT;
			goto bad;
		}
		u.u_ap = u.u_arg;
	} else {
		/*
		 * Use the registers as arguments.
		 */
		u.u_ap = &rp->r_o0;
	}
	u.u_r.r_val1 = 0;
	u.u_r.r_val2 = rp->r_o1;
	if (setjmp(&u.u_qsave)) {
		if (u.u_error == 0 && u.u_eosys == NORMALRETURN)
			u.u_error = EINTR;
	} else {
		u.u_eosys = NORMALRETURN;

	trace6(TR_SYSCALL, code, u.u_arg[0], u.u_arg[1], u.u_arg[2],
			u.u_arg[3], u.u_arg[4]);
#ifdef SYSCALLTRACE
		if (syscalltrace) {
			register int i;
			char *cp;

			printf("%d: ", u.u_procp->p_pid);
			if (code >= nsysent)
				printf("0x%x", code);
			else
				printf("%s", syscallnames[code]);
			cp = "(";
			for (i = 0; i < callp->sy_narg; i++) {
				printf("%s%x", cp, u.u_ap[i]);
				cp = ", ";
			}
			if (i)
				printf(")");
			printf("\n");
		}
#endif
		/*
		 * If tracing sys calls signal a trap.
		 * System call code is slipped into u_arg[7].
		 * No system call has 8 args (at the present time!).
		 */
		if ((u.u_procp->p_flag & STRCSYS) && code != 0) {
			int s, save_sigmask;
			u.u_procp->p_flag &= ~STRCSYS;
			u.u_arg[7] = code;
			psignal(u.u_procp, SIGTRAP);
			s = splhigh();
			save_sigmask = u.u_procp->p_sigmask;
			u.u_procp->p_sigmask = ~sigmask(SIGTRAP);
			(void) issig(0);
			u.u_procp->p_sigmask = save_sigmask;
			(void) splx(s);
		}

		(*(callp->sy_call))(u.u_ap);
	}
	if (u.u_eosys == NORMALRETURN) {
		if (u.u_error) {
bad:
#ifdef SYSCALLTRACE
			if (syscalltrace)
				printf("syscall: error=%d\n", u.u_error);
#endif
			rp->r_o0 = u.u_error;
			rp->r_psr |= PSR_C;	/* set carry bit */
		} else {
			rp->r_psr &= ~PSR_C;	/* reset carry bit */
			rp->r_o0 = u.u_r.r_val1;
			rp->r_o1 = u.u_r.r_val2;
		}
		/*
		 * The default action is to redo the trap instruction.
		 * We increment the pc and npc past it for NORMALRETURN.
		 * JUSTRETURN has set up a new pc and npc already.
		 * RESTARTSYS automatically restarts by leaving pc and npc
		 * alone.
		 */
		rp->r_pc = rp->r_npc;
		rp->r_npc += 4;
	}

	/* If tracing sys calls signal a trap */
	if (u.u_procp->p_flag & STRCSYS) {
		u.u_procp->p_flag &= ~STRCSYS;
		u.u_arg[7] = 0;
		psignal(u.u_procp, SIGTRAP);
	}

	p = u.u_procp;
	if (((p->p_flag & STRC) == 0 && p->p_cursig) || ISSIG(p, 0))
		psig();
	p->p_pri = p->p_usrpri;
	if (runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		(void) spl6();
		setrq(p);
		u.u_ru.ru_nivcsw++;
	
		swtch();
		(void) spl0();
	}
	if (syst_flag && u.u_prof.pr_scale) {
		register int ticks;
		struct timeval *tv = &u.u_ru.ru_stime;

		/*
		 * If pid != pp->p_pid, then we are the child returning from
		 * a "fork" or "vfork" system call.  In this case, reset
		 * "syst", since our time was reset in fork.
		 */
		if (pid != p->p_pid)
			timerclear(&syst);
		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks)
			addupc(rp->r_pc, &u.u_prof, ticks);
	}
	curpri = p->p_pri;
}

/*
 * Indirect system call.
 * Used to be handled above, in syscall, but then everyone
 * was paying a performance penalty for this rarely-used
 * (and questionable) feature.
 */
indir()
{
	register unsigned code;
	register int *sp, *dp;
	register struct sysent *callp;
	register int a;

	code = (unsigned) u.u_ap[0];
	if (code == 0 || code >= nsysent)
		callp = &sysent[63];
	else
		callp = &sysent[code];
	/*
	 * Shift the valid args down 1 and copy into u.u_arg array.
	 */
	a = MIN(callp->sy_narg, 5);
	sp = &u.u_ap[1];
	dp = &u.u_arg[0];
	while (sp <= (int *)&u.u_ap[a]) {
		*dp++ = *sp++;
	}
	/*
	 * Copy in any new args.
	 */
	if (callp->sy_narg > 5) {
		if (copyin((addr_t)u.u_ar0[SP] + MINFRAME,
			(addr_t)&u.u_arg[5],
			(u_int)((callp->sy_narg - 5) * sizeof (int)))) {
			u.u_error = EFAULT;
			return;
		}
	}
	u.u_ap = u.u_arg;
#ifdef SYSCALLTRACE
	if (syscalltrace) {
		register int i;
		char *cp;

		printf("%d: ", u.u_procp->p_pid);
		if (code >= nsysent)
			printf(" -> 0x%x", code);
		else
			printf(" -> %s", syscallnames[code]);
		cp = "(";
		for (i = 0; i < callp->sy_narg; i++) {
			printf("%s%x", cp, u.u_ap[i]);
			cp = ", ";
		}
		if (i)
			printf(")");
		printf("\n");
	}
#endif
	if (u.u_procp->p_flag & STRCSYS) {
		int s, save_sigmask;
		u.u_procp->p_flag &= ~STRCSYS;
		u.u_arg[7] = code;
		psignal(u.u_procp, SIGTRAP);
		s = splhigh();
		save_sigmask = u.u_procp->p_sigmask;
		u.u_procp->p_sigmask = ~sigmask(SIGTRAP);
		(void) issig(0);
		u.u_procp->p_sigmask = save_sigmask;
		(void) splx(s);
	}

	(*(callp->sy_call))(u.u_ap);
}

/*
 * nonexistent system call-- signal process (may want to handle it)
 * flag error if process won't see signal immediately
 * Q: should we do that all the time ??
 */
nosys()
{

	if (u.u_signal[SIGSYS] == SIG_IGN || u.u_signal[SIGSYS] == SIG_HOLD)
		u.u_error = EINVAL;
	psignal(u.u_procp, SIGSYS);
}

/*
 * Print out a traceback for kernel traps
 */
traceback(sp)
	caddr_t sp;
{
	register u_int tospage;
	register struct frame *fp;
	static int done = 0;

	if (panicstr && done++ > 0)
		return;

	if ((int)sp & (STACK_ALIGN-1)) {
		printf("traceback: misaligned sp = %x\n", sp);
		return;
	}
	flush_windows();
	tospage = (u_int)btoc(sp);
	fp = (struct frame *)sp;
	printf("Begin traceback... sp = %x\n", sp);
	while (btoc((u_int)fp) == tospage) {
		if (fp == fp->fr_savfp) {
			printf("FP loop at %x", fp);
			break;
		}
		printf("Called from %x, fp=%x, args=%x %x %x %x %x %x\n",
		    fp->fr_savpc, fp->fr_savfp,
		    fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2],
		    fp->fr_arg[3], fp->fr_arg[4], fp->fr_arg[5]);
#ifdef notdef
		printf("\tl0-l7: %x, %x, %x, %x, %x, %x, %x, %x\n",
		    fp->fr_local[0], fp->fr_local[1],
		    fp->fr_local[2], fp->fr_local[3],
		    fp->fr_local[4], fp->fr_local[5],
		    fp->fr_local[6], fp->fr_local[7]);
#endif
		fp = fp->fr_savfp;
		if (fp == 0)
			break;
	}
	printf("End traceback...\n");
#ifndef SAS
#ifdef VAC
	vac_flush((caddr_t)&msgbuf, sizeof (msgbuf));	/* push msgbuf to mem */
#endif VAC
	DELAY(2000000);
#endif SAS
}

/*
 * General system stack backtrace
 */
tracedump()
{
	label_t l;

#ifdef lint
	int x;
	x = setjmp(&l);
	x = x;
#else
	setjmp(&l);
#endif
	traceback((caddr_t)l.val[1]);
}

#ifdef TRAPWINDOW
long trap_window[25];
#endif TRAPWINDOW

/*
 * Print out debugging info.
 */
showregs(type, rp, addr, be, rw)
	register unsigned type;
	register struct regs *rp;
	addr_t addr;
	u_int be;
	enum seg_rw rw;
{
	int s;
	extern u_int map_getctx();

	s = spl7();
	type &= ~USER;
	if (masterprocp == NULL || peekc(u.u_comm) == -1)
		printf("(unknown): ");
	else
		printf("pid %d, `%s': ", masterprocp->p_pid, u.u_comm);
	if (type < TRAP_TYPES)
		printf("%s\n", trap_type[type]);
	else switch (type) {
	case T_SYSCALL:
		printf("syscall trap:\n");
		break;
	case T_BREAKPOINT:
		printf("breakpoint trap:\n");
		break;
	case T_DIV0:
		printf("zero divide trap:\n");
		break;
	case T_FLUSH_WINDOWS:
		printf("flush windows trap:\n");
		break;
	case T_SPURIOUS:
		printf("spurious interrupt:\n");
		break;
	case T_AST:
		printf("AST\n");
		break;
	default:
		if (type >= T_SOFTWARE_TRAP && type <= T_ESOFTWARE_TRAP)
			printf("software trap 0x%x\n", type - T_SOFTWARE_TRAP);
		else
			printf("bad trap = %d\n", type);
		break;
	}
	if (type == T_DATA_FAULT || type == T_TEXT_FAULT) {
		if (good_addr(addr)) {
			struct pte pte;

			mmu_getpte(addr, &pte);
			printf("%s %s fault at addr=0x%x, pme=0x%x\n",
			    (USERMODE(rp->r_psr)? "user": "kernel"),
			    (rw == S_WRITE? "write": "read"),
			    addr, *(int *)&pte);
		} else {
			printf("bad %s %s fault at addr=0x%x\n",
			    (USERMODE(rp->r_psr)? "user": "kernel"),
			    (rw == S_WRITE? "write": "read"),
			    addr);
		}
		printf("Sync Error Reg %b\n", be, SYNCERR_BITS);
	} else if (addr) {
		printf("addr=0x%x\n", addr);
	}
	printf("pc=0x%x, sp=0x%x, psr=0x%x, context=0x%x\n",
	    rp->r_pc, rp->r_sp, rp->r_psr, map_getctx());
	if (USERMODE(rp->r_psr)) {
		printf("o0-o7: %x, %x, %x, %x, %x, %x, %x, %x\n",
		    rp->r_o0, rp->r_o1, rp->r_o2, rp->r_o3,
		    rp->r_o4, rp->r_o5, rp->r_o6, rp->r_o7);
	}
	printf("g1-g7: %x, %x, %x, %x, %x, %x, %x\n",
	    rp->r_g1, rp->r_g2, rp->r_g3,
	    rp->r_g4, rp->r_g5, rp->r_g6, rp->r_g7);
#ifdef TRAPWINDOW
	printf("trap_window: wim=%x\n", trap_window[24]);
	printf("o0-o7: %x, %x, %x, %x, %x, %x, %x, %x\n",
	    trap_window[0], trap_window[1], trap_window[2], trap_window[3],
	    trap_window[4], trap_window[5], trap_window[6], trap_window[7]);
	printf("l0-l7: %x, %x, %x, %x, %x, %x, %x, %x\n",
	    trap_window[8], trap_window[9], trap_window[10], trap_window[11],
	    trap_window[12], trap_window[13], trap_window[14], trap_window[15]);
	printf("i0-i7: %x, %x, %x, %x, %x, %x, %x, %x\n",
	    trap_window[16], trap_window[17], trap_window[18], trap_window[19],
	    trap_window[20], trap_window[21], trap_window[22], trap_window[23]);
#endif TRAPWINDOW
	vac_flush((caddr_t)&msgbuf, sizeof (msgbuf));	/* push msgbuf to mem */
#ifndef SAS
	if (tudebug > 1 && (boothowto & RB_DEBUG)) {
		CALL_DEBUG();
	}
#endif SAS
	(void) splx(s);
}

/*ARGSUSED*/
sys_rttchk(p, l7, sp)
	struct proc *p;
	caddr_t l7, sp;
{
	if (runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		(void) spl6();
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
		(void) spl0();
	}
#ifdef notdef
	/* XXX - need to do something about this */
	if (syst_flag && u.u_prof.pr_scale) {
		register int ticks;
		struct timeval *tv = &u.u_ru.ru_stime;

		/*
		 * If pid != pp->p_pid, then we are the child returning from
		 * a "fork" or "vfork" system call.  In this case, reset
		 * "syst", since our time was reset in fork.
		 */
		if (pid != p->p_pid)
			timerclear(&syst);
		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks)
			addupc(rp->r_pc, &u.u_prof, ticks);
	}
#endif
	curpri = p->p_pri;
}

/*
 * unimplemented and unaligned instruction simulation support
 */

static int
getreg(rgs, rw, reg, val)
        u_int   *rgs, *rw, reg;
        u_int   *val;
{
        if (reg == 0)
                *val = 0;
        else if (reg < 16)
                *val = rgs[reg];
        else {
                if ((*val = fuword((int *)&rw[reg - 16])) == -1) {
                        if (fubyte((caddr_t)&rw[reg - 16]) == -1) {
                                return (-1);
                        }
                }
        }
        return (0);
}

static int
putreg(data, rgs, rw, reg)
	u_int data, *rgs, *rw, reg;
{
	if (reg == 0)
		return (0);	/* can't write g0 */

	if (reg < 16)
		rgs[reg] = data;
	else {
		if (suword((caddr_t)&rw[reg - 16], (int)data) != 0)
			return (-1);
	}
	return (0);
}

u_int dev_null = 0;		/* a place to store junk */

/*
 * calculate the address of a register that will eventually get
 * restored for a user so it can be changed.
 */
u_int *
getregaddr(rgs, rw, reg)
	u_int rgs[], rw[];
	u_int reg;
{
	if (reg == 0)
		return (&dev_null);
	if (reg < 16)
		return (&rgs[reg]);
	else
		return (&rw[reg - 16]);
}

/*
 * For the sake of those who must be compatible with unaligned
 * architectures, users can link their programs to use a
 * corrective trap handler that will fix unaligned references
 * a special trap #6 (T_FIX_ALIGN) enables this 'feature'.
 */
char *sizestr[] = {"word", "byte", "halfword", "double"};

do_unaligned(rp)
	register struct regs *rp;
{
	register u_int inst;
	register u_int rd, rs1, rs2;
	register u_int *rgs;
	register u_int *rw;
	register u_int addr;
	register int sz;
	register int floatflg;
	register int immflg;
	u_int val;
	extern void _fp_read_pfreg();
	extern void _fp_write_pfreg();
	union ud {
		double	d;
		u_int	i[2];
		u_short	s[4];
		u_char	c[8];
	} data;

	inst = fuword((caddr_t)rp->r_pc);	/* get the instruction */
	rd = (inst >> 25) & 0x1f;
	rs1 = (inst >> 14) & 0x1f;
	rs2 = inst & 0x1f;
	floatflg = (inst >> 24) & 1;
	immflg = (inst >> 13) & 1;

	switch ((inst >> 19) & 3) {	/* map size bits to a number */
	case 0: sz = 4; break;
	case 1: printf("alignment botch\n"); return (0);
	case 2: sz = 2; break;
	case 3: sz = 8; break;
	}

	if (aligndebug) {
		printf("unaligned access at 0x%x, instruction: 0x%x\n",
			rp->r_pc, inst);
		printf("type %s %s %s\n",
			(((inst >> 21) & 1) ? "st" : "ld"),
			(((inst >> 22) & 1) ? "signed" : "unsigned"),
			sizestr[((inst >> 19) & 3)]);
		printf("rd = %d, rs1 = %d, rs2 = %d, imm13 = 0x%x\n",
			rd, rs1, rs2, (inst & 0x1fff));
	}

	/* if not load or store, or to alternate space do nothing */
	if (((inst >> 30) != 3) ||
	    (immflg == 0 && ((inst >> 5) & 0xff)))
		return (0);

	flush_user_windows_to_stack();		/* flush windows into memory */
	rgs = (u_int *)&rp->r_y;		/* globals and outs */
	rw = (u_int *)rp->r_sp;			/* ins and locals */

	/* calculate address, get first register */
	if (getreg(rgs, rw, rs1, &val) == -1)
		goto badret;
	addr = val;

	/* check immediate bit and use immediate field or reg (rs2) */
	if (immflg) {
		register int imm;
		imm  = inst & 0x1fff;		/* mask out immediate field */
		imm <<= 19;			/* sign extend it */
		imm >>= 19;
		addr += imm;			/* compute address */
	} else {
		if (getreg(rgs, rw, rs2, &val) == -1)
			goto badret;
		addr += val;
	}

	if (aligndebug)
		printf("addr = 0x%x\n", addr);

	/* a single bit differentiates ld and st */
	if ((inst >> 21) & 1) {			/* store */
		if (floatflg) {			/* if fp read fpu reg */
			_fp_read_pfreg((u_int *)&data.i[0], rd);
			if (sz == 8)
				_fp_read_pfreg((u_int *)&data.i[1], rd+1);
		} else {
			if (getreg(rgs, rw, rd, &val) == -1)
				goto badret;
			data.i[0] = val;
			if (sz == 8) {
				if (getreg(rgs, rw, rd+1, &val) == -1)
					goto badret;
				data.i[1] = val;
			}
		}

		if (aligndebug) {
			printf("data %x %x %x %x %x %x %x %x\n",
				data.c[0], data.c[1], data.c[2], data.c[3],
				data.c[4], data.c[5], data.c[6], data.c[7]);
		}

		if (sz == 2) {
			if (copyout((caddr_t)&data.s[1], (caddr_t)addr,
			    (u_int)sz) == -1)
				goto badret;
		} else {
			if (copyout((caddr_t)&data.i[0], (caddr_t)addr,
			    (u_int)sz) == -1)
				goto badret;
		}
	} else {				/* load */
		if (sz == 2) {
			if (copyin((caddr_t)addr, (caddr_t)&data.s[1],
			    (u_int)sz) == -1)
				goto badret;
			/* if signed and the sign bit is set extend it */
			if (((inst >> 22) & 1) && ((data.s[1] >> 15) & 1))
				data.s[0] = -1;	/* extend sign bit */
			else
				data.s[0] = 0;	/* clear upper 16 bits */
		} else
			if (copyin((caddr_t)addr, (caddr_t)&data.i[0],
			    (u_int)sz) == -1)
				goto badret;

		if (aligndebug) {
			printf("data %x %x %x %x %x %x %x %x\n",
				data.c[0], data.c[1], data.c[2], data.c[3],
				data.c[4], data.c[5], data.c[6], data.c[7]);
		}

		if (floatflg) {		/* if fp, write fpu reg */
			_fp_write_pfreg((u_int *)&data.i[0], rd);
			if (sz == 8)
				_fp_write_pfreg((u_int *)&data.i[1], rd+1);
		} else {
			if (putreg(data.i[0], rgs, rw, rd) == -1)
				goto badret;
			if (sz == 8)
				if (putreg(data.i[1], rgs, rw, rd+1) == -1)
					goto badret;
		}
	}
	return (SIMU_SUCCESS);
badret:
	return (SIMU_FAULT);
}

/*
 * simulate the swap instruction as best as possible
 */
do_swap(rp, inst)
	register struct regs *rp;
	register u_int inst;
{
	register u_int rd, rs1, rs2, addr;
	register u_int *rgs, *rw;
	register int immflg, s;
	u_int val, newval;

	rd =  (inst >> 25) & 0x1f;
	rs1 = (inst >> 14) & 0x1f;
	rs2 =  inst & 0x1f;
	immflg = (inst >> 13) & 1;

	/*
	 * It assumed that the register windows have been flushed
	 * to the stack.  Getreg, putreg, suword, and fuword all
	 * depend upon this.
	 */
	rgs = (u_int *)&rp->r_y;		/* globals and outs */
	rw = (u_int *)rp->r_sp;			/* ins and locals */

	/* calculate address, get first register */
	if (getreg(rgs, rw, rs1, &val) == -1)
		goto badret;
	addr = val;

	/* check immediate bit and use immediate field or reg (rs2) */
	if (immflg) {
		s  = inst & 0x1fff;		/* mask out immediate field */
		s <<= 19;			/* sign extend it */
		s >>= 19;
		addr += s;			/* compute address */
	} else {
		if (getreg(rgs, rw, rs2, &val) == -1)
			goto badret;
		addr += val;
	}

	/* raise priority (atomic swap operation) */
	s = spl6();

	/* read memory at source address */
	if ((val = fuword((caddr_t)addr)) == -1) {
		if ((val = fubyte((caddr_t)addr)) == -1)
			goto badret;
		else
			val = -1;
	}

	newval = val;

	/* write src address with value from destination reg */
	if (getreg(rgs, rw, rd, &val) == -1)
		goto badret;
	if (suword((caddr_t)addr, (int) val) == -1)
		goto badret;

	/* update destination reg with value from memory */
	if (putreg(newval, rgs, rw, rd) == -1) {
		addr = (int)getregaddr(rgs, rw, rd);
		goto badret;
	}

	/* restore priority and return success or failure */
	(void) splx(s);
	return (SIMU_SUCCESS);
badret:
	u.u_addr = (char *)addr;
	u.u_code = FC_NOMAP;
	(void) splx(s);
	return (SIMU_FAULT);
}

/*
 * simulate unimplemented instructions (swap, mul, div)
 */
simulate_unimp(rp)
	struct regs *rp;
{
	register u_int inst, rv;
	u_int val, wasmul = 1;

	if ((inst = fuword((caddr_t)rp->r_pc)) == -1){
		/*
		 * -1 is an illegal instruction
		 * or a error in fuword, give up now
		 */
		return (SIMU_ILLEGAL);
	}

	/*
	 * simulation depends on register windows being stack resident
	 */
	flush_user_windows_to_stack();

	/*
	 * check for the unimplemented swap instruction
	 * note: swapa is not used anywhere and is not currently supported
	 */
	if ((inst & 0xc1f80000) == 0xc0780000) {
		return (do_swap(rp, inst));
	}

	/*
	 * for mul/div instruction switch on op3 field of instruction
	 * if the two bit op field is 0x2
	 */
	if ((inst >> 30) == 0x2) {
		register u_int rs1, rs2, rd;
		register u_int *rgs;		/* pointer to struct regs */
		register u_int *rw;		/* pointer to frame */
		u_int	dest[2];		/* destination for simulator */

		rd =  (inst >> 25) & 0x1f;
		rgs = (u_int *)&rp->r_y;	/* globals and outs */
		rw = (u_int *)rp->r_sp;		/* ins and locals */

		/* generate first operand rs1 */
		if (getreg(rgs, rw, (inst >> 14) & 0x1f, &val) != 0)
			return (SIMU_FAULT);
		rs1 = val;

		/* check immediate bit and use immediate field or reg (rs2) */
		if ((inst >> 13) & 1) {
			register int imm;
			imm  = inst & 0x1fff;	/* mask out immediate field */
			imm <<= 19;		/* sign extend it */
			imm >>= 19;
			rs2 = imm;		/* compute address */
		} else {
			if (getreg(rgs, rw, inst & 0x1f, &val) != 0)
				return (SIMU_FAULT);
			rs2 = val;
		}


		switch ((inst & 0x01f80000) >> 19) {
		case 0xa:
			rv = _ip_umul(rs1, rs2, dest, &rp->r_y, &rp->r_psr);
			break;
		case 0xb:
			rv = _ip_mul(rs1, rs2, dest, &rp->r_y, &rp->r_psr);
			break;
		case 0xe:
			rv = _ip_udiv(rs1, rs2, dest, &rp->r_y, &rp->r_psr);
			wasmul = 0;
			break;
		case 0xf:
			rv = _ip_div(rs1, rs2, dest, &rp->r_y, &rp->r_psr);
			wasmul = 0;
			break;
		case 0x1a:
			rv = _ip_umulcc(rs1, rs2, dest, &rp->r_y, &rp->r_psr);
			break;
		case 0x1b:
			rv = _ip_mulcc(rs1, rs2, dest, &rp->r_y, &rp->r_psr);
			break;
		case 0x1e:
			rv = _ip_udivcc(rs1, rs2, dest, &rp->r_y, &rp->r_psr);
			wasmul = 0;
			break;
		case 0x1f:
			rv = _ip_divcc(rs1, rs2, dest, &rp->r_y, &rp->r_psr);
			wasmul = 0;
			break;
		default:
			return (SIMU_ILLEGAL);
		}
		if (rv != SIMU_SUCCESS) {
			return (rv);
		}
		if (putreg(dest[0], rgs, rw, rd) != 0) {
			return (SIMU_FAULT);
		}
		if (wasmul)
			rp->r_y = dest[1];
		return (SIMU_SUCCESS);
	}

	/*
	 * Otherwise, we can't simulate instruction, its illegal.
	 */
	return (SIMU_ILLEGAL);
}
/*
 * Check for an atomic (LDST, SWAP) instruction
 * returns 1 if yes, 0 if no.
 */
int
is_atomic(rp)
	struct regs	*rp;
{
	u_int inst;

	inst = USERMODE(rp->r_ps) ? fuword((caddr_t)rp->r_pc) :
		*(u_int *)rp->r_pc;

	/* True for LDSTUB(A) and SWAP(A) */
	return ((inst & 0xc1680000) == 0xc0680000);
}
