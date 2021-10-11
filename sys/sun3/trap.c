/*	@(#)trap.c 1.1 92/07/30 SMI 	*/


/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
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
#include <machine/buserr.h>
#include <machine/memerr.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/seg_kmem.h>

#include "fpa.h"
#include <sundev/fpareg.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>

#include <sys/reboot.h>
#include <debug/debug.h>

#define	USER	0x400		/* user-mode flag added to type */

#ifdef SYSCALLTRACE
extern char *syscallnames[];
#endif

extern struct sysent sysent[];
extern int nsysent;

#ifdef FPU
extern char *fpiar();
#endif FPU

char	*trap_type[] = {
	"Vector address 0x0",
	"Vector address 0x4",
	"Bus error",
	"Address error",
	"Illegal instruction",
	"Divide by zero",
	"CHK, CHK2 instruction",
	"TRAPV, cpTRAPcc, cpTRAPcc instruction",
	"Privilege violation",
	"Trace",
	"1010 emulator trap",
	"1111 emulator trap",
	"Vector address 0x30",
	"Coprocessor protocol error",
	"Stack format error",
	"Unitialized interrupt",
	"Vector address 0x40",
	"Vector address 0x44",
	"Vector address 0x48",
	"Vector address 0x4c",
	"Vector address 0x50",
	"Vector address 0x54",
	"Vector address 0x58",
	"Vector address 0x5c",
	"Spurious interrupt",
};
#define	TRAP_TYPES	(sizeof trap_type / sizeof trap_type[0])

int tudebug = 0;

#if defined(DEBUG) || defined(lint)
int tdebug  = 0;
int lodebug = 0;
int bedebug = 0;
#else
#define	tdebug	0
#define	lodebug	0
#define	bedebug	0
#endif defined(DEBUG) || defined(lint)

/*
 * Called from the trap handler when a processor trap occurs.
 * Returns amount to adjust the stack:  > 0 removes bus error
 * info, == 0 does nothing.
 */
int
trap(type, regs, fmt)
	int type;
	struct regs regs;
	struct stkfmt fmt;
{
	register struct regs *locregs = &regs;
	register struct stkfmt *stkfmt = &fmt;
	register int i = 0;
	register struct proc *p = u.u_procp;
	struct timeval syst;
	int nosig = 0;
	int besize = 0;
	u_char be = (type == T_BUSERR) ? getbuserr() : 0;
	int lofault;
	int dfaultaddr;
	int mask;
	faultcode_t pagefault(), res;

	/*
	 * The vector can never really be zero, so if it is we must have
	 * a padded stack. Move the whole structure up by 2 bytes to where
	 * it really is.
	 */
	if (stkfmt->f_vector == 0)
		stkfmt = (struct stkfmt *)((u_int)stkfmt + 2);
	cnt.v_trap++;
	syst = u.u_ru.ru_stime;
	if (tdebug) {
		i = type/sizeof (int);
		if ((u_int)i < TRAP_TYPES)
			printf("trap: %s\n", trap_type[i]);
		showregs("trap", type, locregs, stkfmt, be);
	}
	if (USERMODE(locregs->r_sr)) {
		type |= USER;
		u.u_ar0 = &locregs->r_dreg[0];
	} else {
		/* reset sp value to adjusted system sp */
		locregs->r_sp = (int)&stkfmt->f_beibase;
		switch (stkfmt->f_stkfmt) {
		case SF_NORMAL:
		case SF_THROWAWAY:
			break;
		case SF_NORMAL6:
			locregs->r_sp += sizeof (struct bei_normal6);
			break;
		case SF_COPROC:
			locregs->r_sp += sizeof (struct bei_coproc);
			break;
		case SF_MEDIUM:
			locregs->r_sp += sizeof (struct bei_medium);
			break;
		case SF_LONGB:
			locregs->r_sp += sizeof (struct bei_longb);
			break;
		default:
			panic("bad system stack format");
			/* NOTREACHED */
		}
	}

	switch (type) {

	case T_ADDRERR:			/* kernel address error */
		/*
		 * On 68020, addresses errors can only happen
		 * when executing instructions on an odd boundary.
		 * Therefore, we cannot have gotten here as a
		 * result of copyin/copyout request - so panic.
		 */
	default:
	die:
		(void) spl7();
		showregs("", stkfmt->f_vector, locregs, stkfmt, be);
		traceback((long)locregs->r_areg[6], (long)locregs->r_sp);
		i = stkfmt->f_vector/sizeof (int);
		if (i < TRAP_TYPES)
			panic(trap_type[i]);
		panic("trap");
		/* NOTREACHED */

	case T_BUSERR:
#ifdef SUN3_260
		if (cpu != CPU_SUN3_260)
#endif SUN3_260
			if (be & BE_TIMEOUT)
				DELAY(2000);	/* allow for refresh recovery */

		/* may have been expected by C (e.g., VMEbus probe) */
		if (nofault) {
			label_t *ftmp;

			ftmp = nofault;
			nofault = 0;
			longjmp(ftmp);
		}
		/*
		 * No use trying to resolve a timeout error. Jump right
		 * to the lofault handling.
		 */
		if (be & BE_TIMEOUT) {
			if (stkfmt->f_stkfmt == SF_MEDIUM)
				besize = sizeof (struct bei_medium);
			else if (stkfmt->f_stkfmt == SF_LONGB)
				besize = sizeof (struct bei_longb);
			else
				panic("bad bus error stack format");
			res = FC_HWERR;
			goto kloout;
		}
#ifdef FPU
		if ((i = fpu_berror(be, locregs, *stkfmt)) == SIGSEGV)
			break;
#endif FPU

		/*
		 * See if we can handle as pagefault.  Save lofault
		 * across this.  Here we assume that an address
		 * less than KERNELBASE + sizeof (int) is a user fault.
		 * We can do this as locore.s routines verify that the
		 * starting address is less than KERNELBASE before
		 * starting and because we know that we always have
		 * KERNELBASE mapped as invalid to serve as a "barrier".
		 */
		lofault = u.u_lofault;
		u.u_lofault = 0;

		switch (stkfmt->f_stkfmt) {
		case SF_MEDIUM: {
			struct bei_medium *beip =
			    (struct bei_medium *)&stkfmt->f_beibase;

			besize = sizeof (struct bei_medium);
			if (beip->bei_dfault) {
				/*
				 * We look at function codes here since
				 * we might have taken a fault during a
				 * movs instruction.  In this case, we
				 * need to mask off the extra bits.
				 */
				if (beip->bei_fcode == FC_MAP)
					dfaultaddr = beip->bei_fault &
							ADDRESS_MASK;
				else
					dfaultaddr = beip->bei_fault;
				if (dfaultaddr < KERNELBASE + sizeof (int)) {
					if (lofault == 0)
						goto die;
					res = pagefault(dfaultaddr,
					    beip->bei_rw ? S_READ : S_WRITE, 0);
				} else {
					res = pagefault(dfaultaddr,
					    beip->bei_rw ? S_READ : S_WRITE, 1);
				}
			}
			if (beip->bei_faultc || beip->bei_faultb) {
				/*
				 * For now, we assume no kernel
				 * text faults are allowed
				 */
				goto die;
			}
			break;
			}
		case SF_LONGB: {
			struct bei_longb *beip =
			    (struct bei_longb *)&stkfmt->f_beibase;

			besize = sizeof (struct bei_longb);
			if (beip->bei_dfault) {
				/*
				 * We look at function codes here since
				 * we might have taken a fault during a
				 * movs instruction.  In this case, we
				 * need to mask off the extra bits.
				 */
				if (beip->bei_fcode == FC_MAP)
					dfaultaddr = beip->bei_fault &
							ADDRESS_MASK;
				else
					dfaultaddr = beip->bei_fault;
				if (dfaultaddr < KERNELBASE + sizeof (int)) {
					if (lofault == 0)
						goto die;
					res = pagefault(dfaultaddr,
					    beip->bei_rw ? S_READ : S_WRITE, 0);
				} else {
					res = pagefault(dfaultaddr,
					    beip->bei_rw ? S_READ : S_WRITE, 1);
				}
			}
			if (beip->bei_faultc || beip->bei_faultb) {
				/*
				 * For now, we assume no kernel
				 * text faults are allowed
				 */
				goto die;
			}
			break;
			}
		default:
			panic("bad bus error stack format");
		}

		/*
		 * Restore lofault.  If we resolved the fault exit,
		 * else if we didn't and lofault wasn't set, die.
		 */
		u.u_lofault = lofault;
		if (res == 0)
			return (0);
kloout:
		if (u.u_lofault == 0)
			goto die;

		/*
		 * Cannot resolve fault.  Return to lofault after
		 * cleaning up stack.
		 */
		if (lodebug) {
			showregs("lofault", type, locregs, stkfmt, be);
			traceback((long)locregs->r_areg[6],
			    (long)locregs->r_sp);
		}
		locregs->r_pc = u.u_lofault;
		if (FC_CODE(res) == FC_OBJERR)
			locregs->r_r0 = FC_ERRNO(res);
		else
			locregs->r_r0 = EFAULT;
		return (besize);

	case T_ADDRERR + USER:		/* user address error */
		if (tudebug)
			showregs("USER ADDRESS ERROR", type, locregs,
			    stkfmt, be);
		u.u_addr = (char *)locregs->r_pc;
		u.u_code = FC_ALIGN;
		i = SIGBUS;
		switch (stkfmt->f_stkfmt) {
		case SF_MEDIUM:
			besize = sizeof (struct bei_medium);
			break;
		case SF_LONGB:
			besize = sizeof (struct bei_longb);
			break;
		default:
			panic("bad address error stack format");
		}
		break;

	case T_SPURIOUS:
	case T_SPURIOUS + USER:		/* spurious interrupt */
		i = spl7();
		if ((i & 0x700) != 0x300) 
			printf("spurious level %d interrupt\n", (i & SR_INTPRI) >> 8);
		(void) splx(i);
		return (0);

	case T_PRIVVIO + USER:		/* privileged instruction fault */
		if (tudebug)
			showregs("USER PRIVILEGED INSTRUCTION", type, locregs,
			    stkfmt, be);
		u.u_addr = (char *)locregs->r_pc;
		u.u_code = stkfmt->f_vector;
		i = SIGILL;
		break;

	case T_COPROCERR + USER:	/* coprocessor protocol error */
		/*
		 * Dump out obnoxious info to warn user
		 * that something isn't right w/ the 68881
		 */
		showregs("USER COPROCESSOR PROTOCOL ERROR", type, locregs,
		    stkfmt, be);
		u.u_addr = (char *)locregs->r_pc;
		u.u_code = stkfmt->f_vector;
		i = SIGILL;
		break;

	case T_M_BADTRAP + USER:	/* (some) undefined trap */
	case T_ILLINST + USER:		/* illegal instruction fault */
	case T_FMTERR + USER:		/* format error fault */
		if (tudebug)
			showregs("USER ILLEGAL INSTRUCTION", type, locregs,
			    stkfmt, be);
		if  (type == T_M_BADTRAP + USER)
			u.u_addr = (char *)locregs->r_pc - 2;
		else
			u.u_addr = (char *)locregs->r_pc;
		u.u_code = stkfmt->f_vector;
		i = SIGILL;
		break;

	case T_M_FLOATERR + USER:
		/*
		 * Some floating point error trap.
		 */
#ifdef FPU
		{
			register struct fp_istate *isp;

			isp = &u.u_pcb.u_fp_istate;
			(void) fsave(isp);
			isp->fpis_buf[isp->fpis_bufsiz - 4] |= FPP_EXC_PEND;
			u.u_addr = fpiar();
			frestore(isp);
		}
#else
		u.u_addr = (char *)locregs->r_pc;
#endif FPU
		u.u_code = stkfmt->f_vector;
		i = SIGFPE;
		break;

	case T_ZERODIV + USER:		/* divide by zero */
	case T_CHKINST + USER:		/* CHK [CHK2] instruction */
	case T_TRAPV + USER:		/* TRAPV [cpTRAPcc TRAPcc] instr */
		if (stkfmt->f_stkfmt != SF_NORMAL6)
			panic("expected normal6 stack format");
		u.u_addr = (char *)
		    ((struct bei_normal6 *)&stkfmt->f_beibase)->bei_pc;
		u.u_code = stkfmt->f_vector;
		i = SIGFPE;
		break;

	/*
	 * User bus error.  Try to handle FPA, then pagefault, and
	 * failing that, grow the stack automatically if a data fault.
	 */
	case T_BUSERR + USER:
#ifdef SUN3_260
		if (cpu != CPU_SUN3_260)
#endif SUN3_260
			if (be & BE_TIMEOUT)
				DELAY(2000);	/* allow for refresh recovery */

		/*
		 * Copy the "rerun" bits to the "fault" bits.
		 *
		 * This is what is going on here (don't believe
		 * the 2nd edition 68020 description in section
		 * 6.4.1, it is full of errors).  A rerun bit
		 * being on means that the prefetch failed.  A
		 * fault bit being on means the processor tried
		 * to use bad prefetch data.  Upon return via
		 * the RTE instruction, the '20 will retry the
		 * instruction access only if BOTH the rerun and
		 * the corresponding fault bit is on.
		 *
		 * We need to do guarantee that any time we have a
		 * fault that we have actually just run the cycle,
		 * otherwise the current external state (i.e. the
		 * bus error register) might not anything to do with
		 * what really happened to cause the prefetch to fail.
		 * For example the prefetch might have occured previous
		 * to an earlier bus error exception whose handling
		 * might have resolved the prefetch problem.  Thus by
		 * copying the "rerun" bits, we force the `20 to rerun
		 * every previously faulted prefetch upon return from
		 * this bus error.  This way we are guaranteed that we
		 * never get a "bogus" '20 internal bus error when it
		 * attempts to use a previously faulted prefetch.  On
		 * the downside, this hack might make the kernel fix up
		 * a prefetch fault that the '20 was not going to use.
		 * What we really need is a "don't know anything about
		 * a prefetch bit".  If we had something like that then
		 * the '20 could know enough to rerun the prefetch, but
		 * only if it turns out that it really needs it.
		 *
		 * RISC does have its advantages.
		 *
		 * N.B.  This code depends on not having an executable
		 * where the last instruction in the text segment is
		 * too close the end of a page.  We don't want to get
		 * ourselves in trouble trying to fix up a fault beyond
		 * the end of the text segment.  But because the loader
		 * already pads out by an additional page when it sees
		 * this problem due to microcode bugs with the first
		 * year or so worth of '20 chips, we shouldn't get be
		 * in trouble here.
		 */

		switch (stkfmt->f_stkfmt) {
		case SF_MEDIUM: {
			struct bei_medium *beip =
			    (struct bei_medium *)&stkfmt->f_beibase;

			besize = sizeof (struct bei_medium);
			beip->bei_faultc = beip->bei_rerunc;
			beip->bei_faultb = beip->bei_rerunb;
			break;
			}
		case SF_LONGB: {
			struct bei_longb *beip =
			    (struct bei_longb *)&stkfmt->f_beibase;

			besize = sizeof (struct bei_longb);
			beip->bei_faultc = beip->bei_rerunc;
			beip->bei_faultb = beip->bei_rerunb;
			break;
			}
		default:
			panic("bad bus error stack format");
		}

#ifdef FPU
		/*
		 * Check to see if it was an FPU error.
		 */
		if ((i = fpu_user_berror(be)) == SIGFPE) {
			besize = (stkfmt->f_stkfmt == SF_MEDIUM) ?
				 sizeof (struct bei_medium) : sizeof (struct bei_longb);
			save_user_state(locregs, stkfmt);
			break;
		}
#endif FPU
		if ((be & ~(BE_INVALID|BE_PROTERR)) != 0) {
			/*
			 * There was an error in the buserr register besides
			 * invalid and protection - we cannot handle it.
			 */
			res = FC_HWERR;
		} else if (stkfmt->f_stkfmt == SF_MEDIUM) {
			struct bei_medium *beip =
			    (struct bei_medium *)&stkfmt->f_beibase;

			if ((bedebug &&
			    (beip->bei_faultb || beip->bei_faultc)) ||
			    (bedebug > 1 && beip->bei_fault))
				printf(
				    "medium fault b %d %x, c %d %x, d %d %x\n",
				    beip->bei_faultb, locregs->r_pc + 4,
				    beip->bei_faultc, locregs->r_pc + 2,
				    beip->bei_dfault, beip->bei_fault);

			if (beip->bei_dfault) {
				if ((res = pagefault(beip->bei_fault,
				    beip->bei_rw ? S_READ : S_WRITE, 0)) == 0)
					return (0);
				if (grow(beip->bei_fault)) {
					nosig = 1;
					besize = 0;
					goto out;
				}
				u.u_addr = (char *)beip->bei_fault;
			} else if (beip->bei_faultc) {
				if ((res = pagefault(locregs->r_pc + 2,
				    S_EXEC, 0)) == 0)
					return (0);
				u.u_addr = (char *)locregs->r_pc + 2;
			}
			if (beip->bei_faultb) {
				if ((res = pagefault(locregs->r_pc + 4,
				    S_EXEC, 0)) == 0)
					return (0);
				u.u_addr = (char *)locregs->r_pc + 4;
			}
		} else {
			struct bei_longb *beip =
			    (struct bei_longb *)&stkfmt->f_beibase;

			if ((bedebug &&
			    (beip->bei_faultb || beip->bei_faultc)) ||
			    (bedebug > 1 && beip->bei_fault))
				printf("long fault b %d %x, c %d %x, d %d %x\n",
				    beip->bei_faultb, beip->bei_stageb,
				    beip->bei_faultc, beip->bei_stageb - 2,
				    beip->bei_dfault, beip->bei_fault);

			if (beip->bei_dfault) {
				if ((res = pagefault(beip->bei_fault,
				    beip->bei_rw ? S_READ : S_WRITE, 0)) == 0)
					return (0);
				if (grow(beip->bei_fault)) {
					nosig = 1;
					besize = 0;
					goto out;
				}
				u.u_addr = (char *)beip->bei_fault;
			} else if (beip->bei_faultc) {
				if ((res = pagefault(beip->bei_stageb - 2,
				    S_EXEC, 0)) == 0)
					return (0);
				u.u_addr = (char *)beip->bei_stageb - 2;
			}
			if (beip->bei_faultb) {
				if ((res = pagefault(beip->bei_stageb,
				    S_EXEC, 0)) == 0)
					return (0);
				u.u_addr = (char *)beip->bei_stageb;
			}
		}
		if (tudebug)
			showregs("USER BUS ERROR", type, locregs, stkfmt, be);
		/*
		 * In the case where both pagefault and grow fail,
		 * set the code to the value provided by pagefault.
		 * We map errors relating to the underlying object
		 * to SIGBUS, others to SIGSEGV.
		 */
		u.u_code = res;
		besize = (stkfmt->f_stkfmt == SF_MEDIUM) ?
			 sizeof(struct bei_medium) : sizeof(struct bei_longb);
		save_user_state(locregs, stkfmt);
		switch (FC_CODE(res)) {
		case FC_HWERR:
		case FC_OBJERR:
			i = SIGBUS;
			break;
		default:
			i = SIGSEGV;
		}
		break;

	case T_TRACE:			/* caused by tracing trap instr */
		u.u_pcb.pcb_flags |= TRACE_PENDING;
		return (0);

	case T_TRACE + USER:		/* trace trap */
		dotrace(locregs);
		goto out;

	case T_BRKPT + USER:		/* bpt instruction (trap #15) fault */
		i = SIGTRAP;
		break;

	case T_EMU1010 + USER:		/* 1010 emulator trap */
	case T_EMU1111 + USER:		/* 1111 emulator trap */
		u.u_addr = (char *)locregs->r_pc;
		u.u_code = stkfmt->f_vector;
		i = SIGEMT;
		break;
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
	if (u.u_pcb.pcb_flags & TRACE_PENDING)
		dotrace(locregs);
	if (((p->p_flag & STRC) == 0 && p->p_cursig) || ISSIG(p, 0)) {
		if (nosig)
			u.u_pcb.pcb_flags |= AST_STEP;	/* get back here soon */
		else
			psig();
	}
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
			addupc(locregs->r_pc, &u.u_prof, ticks);
	}
	curpri = p->p_pri;
	return (besize);
}

#ifdef SYSCALLTRACE
int syscalltrace = 0;
#endif

/*
 * Called from the trap handler when a system call occurs
 */
void
syscall(code, regs)
	int code;
	struct regs regs;
{
	struct timeval syst;
	short int syst_flag;
	short pid;

	cnt.v_syscall++;
	if (u.u_prof.pr_scale) {
		syst = u.u_ru.ru_stime;
		syst_flag = 1;
		pid = u.u_procp->p_pid;
	} else
		syst_flag = 0;
#ifdef notdef
	if (!USERMODE(regs.r_sr))
		panic("syscall");
#endif
	{
	/*
	 * At this point we declare a number of register variables.
	 * syscall_setjmp (called below) does not preserve the values
	 * of register variables, so we limit their scope to this block.
	 */
	register struct regs *locregs;
	register struct sysent *callp;

	locregs = &regs;
	u.u_ar0 = &locregs->r_dreg[0];
	if (code < 0)
		code = 63;
	u.u_error = 0;
	callp = (code >= nsysent) ? &sysent[63] : &sysent[code];
	if (callp->sy_narg) {
		if (fulwds((caddr_t)locregs->r_sp + 2 * NBPW, (caddr_t)u.u_arg,
		    callp->sy_narg)) {
			u.u_error = EFAULT;
			goto bad;
		}
	}
	u.u_ap = u.u_arg;
	u.u_r.r_val1 = 0;
	u.u_r.r_val2 = regs.r_dreg[1];
	/*
	 * Syscall_setjmp is a special setjmp that only saves a6 and sp.
	 * The result is a significant speedup of this critical path,
	 * but meanwhile all the register variables have the wrong
	 * values after a longjmp returns here.
	 * This is the reason for the limited scope of the register
	 * variables in this routine - the values may go away here.
	 */
	if (syscall_setjmp(&u.u_qsave)) {
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
			for (i= 0; i < callp->sy_narg; i++) {
				printf("%s%x", cp, u.u_arg[i]);
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
	/* end of scope of register variables above */
	}
	if (u.u_eosys == NORMALRETURN) {
		regs.r_sp += sizeof (int);	/* pop syscall # */
		if (u.u_error) {
bad:
#ifdef SYSCALLTRACE
			if (syscalltrace)
				printf("syscall: error=%d\n", u.u_error);
#endif
			regs.r_dreg[0] = u.u_error;
			regs.r_sr |= SR_CC;	/* carry bit */
		} else {
			regs.r_sr &= ~SR_CC;
			regs.r_dreg[0] = u.u_r.r_val1;
			regs.r_dreg[1] = u.u_r.r_val2;
		}
	} else if (u.u_eosys == RESTARTSYS)
		regs.r_pc -= 2;
	/* else if (u.u_eosys == JUSTRETURN) */
		/* nothing to do */

	/* If tracing sys calls signal a trap */

	if (u.u_procp->p_flag & STRCSYS) {
		u.u_procp->p_flag &= ~STRCSYS;
		u.u_arg[7] = 0;
		psignal(u.u_procp, SIGTRAP);
	}

	if (u.u_pcb.pcb_flags & TRACE_PENDING)
		dotrace(&regs);
	{
	/* scope for use of register variable p */
	register struct proc *p;

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
		int ticks;
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
			addupc(regs.r_pc, &u.u_prof, ticks);
	}
	curpri = p->p_pri;
	}
}

/*
 * Indirect system call.
 * Used to be handled above, in syscall, but then everyone
 * was paying a performance penalty for this rarely-used
 * (and questionable) feature.
 */
indir()
{
	register int code, i;
	register struct sysent *callp;

	code = u.u_arg[0];
	callp = (code < 1 || code >= nsysent) ?
		&sysent[63] : &sysent[code];
	if (i = callp->sy_narg) {
		if (fulwds((caddr_t)u.u_ar0[SP] + 3*NBPW, (caddr_t)u.u_arg,
		    i)) {
			u.u_error = EFAULT;
			return;
		}
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
 * Handle trace traps, both real and delayed.
 */
dotrace(locregs)
	struct regs *locregs;
{
	register int r, s;
	struct proc *p = u.u_procp;

	s = spl6();
	r = u.u_pcb.pcb_flags&AST_CLR;
	u.u_pcb.pcb_flags &= ~AST_CLR;
	u.u_ar0[PS] &= ~PSL_T;
	(void) splx(s);
	if (r & TRACE_AST) {
		if ((p->p_flag&SOWEUPC) && u.u_prof.pr_scale) {
			addupc(locregs->r_pc, &u.u_prof, 1);
			p->p_flag &= ~SOWEUPC;
		}
		if ((r & TRACE_USER) == 0)
			return;
	}
	psignal(p, SIGTRAP);
}

/*
 * Print out a traceback for kernel traps
 */
traceback(afp, sp)
	long afp, sp;
{
	u_int tospage = btoc(sp);
	struct frame *fp = (struct frame *)afp;
	static int done = 0;

	if (panicstr && done++ > 0)
		return;

	printf("Begin traceback...fp = %x, sp = %x\n", fp, sp);
	while (btoc(fp) == tospage) {
		if (fp == fp->fr_savfp) {
			printf("FP loop at %x", fp);
			break;
		}
		printf("Called from %x, fp=%x, args=%x %x %x %x\n",
		    fp->fr_savpc, fp->fr_savfp,
		    fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3]);
		fp = fp->fr_savfp;
	}
	printf("End traceback...\n");
	vac_flush((caddr_t)&msgbuf, sizeof (msgbuf));	/* push msgbuf to mem */
	DELAY(2000000);
}

showregs(str, type, locregs, fmtp, be)
	char *str;
	int type;
	struct regs *locregs;
	struct stkfmt *fmtp;
	u_char be;
{
	int *r, s;
	int fcode, accaddr;
	char *why;
	struct pte pte;
	struct pmgrp *pmgrp;

	s = spl7();
	if (masterprocp == NULL || peekc(u.u_comm) == -1)
		printf("(unknown): %s\n", str);
	else
		printf("pid %d, `%s': %s\n", masterprocp->p_pid, u.u_comm, str);
	printf("vector 0x%x, pc 0x%x, sr 0x%x, stkfmt 0x%x, context 0x%x\n",
	    fmtp->f_vector, locregs->r_pc, locregs->r_sr, fmtp->f_stkfmt,
	    sun_getcontrol_byte(CONTEXT_REG, (addr_t)0) & CTXMASK);
	type &= ~USER;
	if (type == T_BUSERR)
		printf("Bus Error Reg %b\n", be, BUSERR_BITS);
	if (type == T_BUSERR || type == T_ADDRERR) {
		switch (fmtp->f_stkfmt) {
		case SF_MEDIUM: {
			struct bei_medium *beip =
			    (struct bei_medium *)&fmtp->f_beibase;

			fcode = beip->bei_fcode;
			if (beip->bei_dfault) {
				why = "data";
				accaddr = beip->bei_fault;
			} else if (beip->bei_faultc) {
				why = "stage c";
				accaddr = locregs->r_pc+2;
			} else if (beip->bei_faultb) {
				why = "stage b";
				accaddr = locregs->r_pc+4;
			} else {
				why = "unknown";
				accaddr = 0;
			}
			printf("%s fault address %x faultc %d faultb %d ",
			    why, accaddr, beip->bei_faultc, beip->bei_faultb);
			printf("dfault %d rw %d size %d fcode %d\n",
			    beip->bei_dfault, beip->bei_rw,
			    beip->bei_size, fcode);
			break;
			}
		case SF_LONGB: {
			struct bei_longb *beip =
			    (struct bei_longb *)&fmtp->f_beibase;

			fcode = beip->bei_fcode;
			if (beip->bei_dfault) {
				why = "data";
				accaddr = beip->bei_fault;
			} else if (beip->bei_faultc) {
				why = "stage c";
				accaddr = beip->bei_stageb-2;
			} else if (beip->bei_faultb) {
				why = "stage b";
				accaddr = beip->bei_stageb;
			} else {
				why = "unknown";
				accaddr = 0;
			}
			printf("%s fault address %x faultc %d faultb %d ",
			    why, accaddr, beip->bei_faultc, beip->bei_faultb);
			printf("dfault %d rw %d size %d fcode %d\n",
			    beip->bei_dfault, beip->bei_rw,
			    beip->bei_size, fcode);
			break;
			}
		default:
			panic("bad bus error stack format");
		}
		if (fcode == FC_SD || fcode == FC_SP)
			printf("KERNEL MODE\n");
		mmu_getpte((addr_t)accaddr, &pte);
		pmgrp = mmu_getpmg((addr_t)accaddr);
		printf("page map %x pmgrp %x\n", pte, pmgrp->pmg_num);
	}
	r = &locregs->r_dreg[0];
	printf("D0-D7  %x %x %x %x %x %x %x %x\n",
	    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
	r = &locregs->r_areg[0];
	printf("A0-A7  %x %x %x %x %x %x %x %x\n",
	    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
	vac_flush((caddr_t)&msgbuf, sizeof (msgbuf));	/* push msgbuf to mem */
	if (tudebug > 1 && (boothowto & RB_DEBUG)) {
		CALL_DEBUG();
	} else {
		DELAY(2000000);
	}
	(void) splx(s);
}

/*
 * Save processor state at time of bus error to allow user to resume
 * execution following return from signal handler.  The bus error PC
 * is saved in u_berr_pc, and the current bus error stack frame in
 * u_berr_stack.  Saved information is: the bus error exception frame,
 * a short of frame type and vector offset, processor registers, a long
 * of AST bits, and the size of the stack frame.  Appended to this
 * is the fsaved internal 68882 state.
 * 
 * The stack frame is restored to the kernel stack in locore following
 * return from syscall() when either pc equals saved PC on kernel
 * stack.  The 68882 internal state is frestored at the end of sigcleanup()
 * under the same conditions.
 *
 * side effect: if the trap frame is an SF_COPROC,
 *		then set the reg[PC] to the real pc from frame.
 */

save_user_state(locregs, stkfmt)
	register struct regs *locregs;
	struct stkfmt *stkfmt;
{       
	register struct user_buserr_stack *fp;
	u_int size;
	register struct buserr_state *bs = &u.u_pcb.u_buserr_state;

	if (bs->u_buserr_stack == NULL) {
		/* First user bus error, alloc space */
		bs->u_buserr_stack =  (struct user_buserr_stack *)
			kmem_alloc(sizeof (struct user_buserr_stack));
	}
	fp = bs->u_buserr_stack;
	fp->uberr_ast = u.u_pcb.pcb_flags & AST_CLR;

	/*
	 * what's on the stack:
	 * 
	 *		<high addr>
	 *		struct bei_(medium, longb, coproc) exception frame
	 * stkfmt -->	short stack frame format & vector offset
	 *		possible short stack pad
	 * locregs -->	struct regs
	 *		<low addr>
	 *
	 * except for the pad, this all gets saved in *(bs->stack)
	 */

	bcopy((caddr_t)locregs, (caddr_t)&fp->uberr_regs, sizeof(struct regs));
	/* don't bcopy the whole thing, b/c a stack pad may be present */
	switch(stkfmt->f_stkfmt){
	 case SF_MEDIUM:{
		fp->uberr_medium = *(struct bei_medium *) &stkfmt->f_beibase;
		size = sizeof (struct bei_medium);
		break;
	 }
	 case SF_LONGB:{
		struct bei_longb *beip = (struct bei_longb *)&stkfmt->f_beibase;
		size = sizeof (struct bei_longb);
		bcopy((caddr_t)beip, (caddr_t)&fp->uberr_longb, sizeof(struct bei_longb));
		break;
	 }
	 case SF_COPROC:{
		struct bei_coproc *beip = (struct bei_coproc *)&stkfmt->f_beibase;
		fp->uberr_coproc = *(struct bei_coproc *) &stkfmt->f_beibase;
		size = sizeof (struct bei_coproc);
		locregs->r_pc = beip->bei_pc;	/* put real pc in regs[PC] */
		break;
	 }
	 default:
		printf("stkfmt 0x%x fmt 0x%x, vector 0x%x\n",
			stkfmt, stkfmt->f_stkfmt,stkfmt->f_vector);
		panic("save_user_state() -- bad trap frame.\n");
	}
	fp->uberr_save_size = sizeof (struct regs) + sizeof (short) + size;
	fp->uberr_stkfmt = *(short *)stkfmt;

	bs->u_buserr_pc = locregs->r_pc;

	/*
	 * Grab the internal '882 state, so it can be restored with
	 * the above saved stack frame.
	 */
#ifdef FPU
	fsave(&fp->fp_istate);
#endif FPU
}

	/*
	 * what's on the stack:
	 * 
	 *		<high addr>
	 *		struct bei_(medium, longb, coproc) exception frame
	 * stkfmt -->	short stack frame format & vector offset
	 *		possible short stack pad
	 * locregs -->	struct regs
	 *		<low addr>
	 *
	 * except for the pad, this all gets saved in *(bs->stack)
	 */

/* wrapper for save_user_state so it can be called from sendsig()
 *
 * Check if there is a SF_COPROC on the stack.
 * If so, save it via save_user_state();
 */


save_fpe_state()
{       
	register struct regs *locregs = (struct regs *)u.u_ar0;
	register struct stkfmt *stkfmt;

	stkfmt = (struct stkfmt *) ((u_int)locregs + sizeof(struct regs));
	if (stkfmt->f_vector == 0)	/* if stack pad present */
		stkfmt = (struct stkfmt *)((u_int)stkfmt + 2);
	if (stkfmt->f_stkfmt != SF_COPROC)	/* if not a mid-instruction stack frame */
		return;
	save_user_state(locregs, stkfmt);
}

