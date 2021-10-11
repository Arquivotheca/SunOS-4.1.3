#ifndef lint
static	char sccsid[] = "@(#)trap.c 1.1 92/07/30";
#endif
/* syncd to sun3/trap.c 1.64 */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/kernel.h>
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
#ifdef PARITY
int put_parity = 0;
#endif PARITY

extern struct sysent sysent[];
extern int nsysent;

#ifdef FPU
extern char	*fpiar();
#endif FPU

extern u_short	mmu_geterror();

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
#define	tdebug  0
#define	lodebug 0
#define	bedebug 0
#endif defined(DEBUG) || defined(lint)
#define	LONG_ALIGN(x)	(x & 0xFFFFFFFC)

/*
 * Called from the trap handler when a processor trap occurs.
 * Returns amount to adjust the stack:  > 0 removes bus error
 * info, == 0 does nothing.
 *
 * XXX - This code does NOT handle RMW faults correctly - see
 * section 6.3.1.7 of the 68851 manual.
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
	struct	timeval syst;
	int	nosig = 0;
	int	besize = 0;
	u_char	be = 0;
	u_short	mmube = 0;
	int	lofault;
	u_int	faultaddr, errtype;
	char	usr, isdf;
	enum	seg_rw rw;
	faultcode_t pagefault(), res;
	struct	bei_medium *beimp;
	struct	bei_longb *beilp;
	int	mask;

	/*
	 * The vector can never really be zero, so if it is we must have
	 * a padded stack. Move the whole structure up by 2 bytes to where
	 * it really is.
	 */
	if (stkfmt->f_vector == 0)
		stkfmt = (struct stkfmt *)((u_int)stkfmt + 2);
	cnt.v_trap++;
	syst = u.u_ru.ru_stime;
	/*
	 * We need to pull out information about whether this fault
	 * was an mmu error. This is necessary so we can properly set/reset
	 * the bus error register copy, since it doesn't reset itself.
	 * First, see if we were in user/supervisor mode.
	 */
	if (USERMODE(locregs->r_sr)) {
		usr = 1;
		u.u_ar0 = &locregs->r_dreg[0];
	} else {
		usr = 0;
		/*
		 * Reset sp value to adjusted system sp.
		 */
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
			/*NOTREACHED*/
		}
	}
	/*
	 * Next, we look into the bus error frame for all pertinent info.
	 */
	if (type == T_BUSERR) {
		switch (stkfmt->f_stkfmt) {
		    case SF_MEDIUM:
			beimp = (struct bei_medium *)&stkfmt->f_beibase;
			besize = sizeof (struct bei_medium);
			/*
			 * Fault occurred during data cycle.
			 */
			if (beimp->bei_dfault) {
				isdf = 1;
				faultaddr = beimp->bei_fault;
				if (beimp-> bei_rmw)
					rw= S_WRITE;
				else
					rw = beimp->bei_rw ? S_READ : S_WRITE;
			} else if (beimp->bei_faultc || beimp->bei_faultb) {
				isdf = 0;
				/*
				 * We don't allow kernel text faults. If
				 * it's a user fault, get the pipeline
				 * address.
				 */
				if (usr == 0) {
					goto die;
				} else if (beimp->bei_faultc)
					faultaddr = locregs->r_pc + 2;
				else
					faultaddr = locregs->r_pc + 4;
				/*
				 * Program faults are execs.
				 */
				rw = S_EXEC;
			}
			break;
		    case SF_LONGB:
			beilp = (struct bei_longb *)&stkfmt->f_beibase;
			besize = sizeof (struct bei_longb);
			/*
			 * Fault occurred during data cycle.
			 */
			if (beilp->bei_dfault) {
				isdf = 1;
				faultaddr = beilp->bei_fault;
				if (beilp-> bei_rmw)
					rw= S_WRITE;
				else
					rw = beilp->bei_rw ? S_READ : S_WRITE;
			} else if (beilp->bei_faultc || beilp->bei_faultb) {
				isdf = 0;
				/*
				 * We don't allow kernel text faults. If
				 * it's a user fault, get the pipeline
				 * address.
				 */
				if (usr == 0) {
					goto die;
				} else if (beilp->bei_faultc)
					faultaddr = beilp->bei_stageb - 2;
				else
					faultaddr = beilp->bei_stageb;
				/*
				 * Program faults must be reads.
				 */
				rw = S_READ;
			}
			break;
		    default:
			panic("bad bus error stack format");
			/*NOTREACHED*/
		}
		/*
		 * Ok, now we can call the mmu routine to see if it was an
		 * mmu error. If it was, we clear the buserr copy, otherwise
		 * we set it from the actual reg. We also establish which type
		 * of mmu error occurred, this is passed onto pagefault.
		 * Also, if the mmu was the source, we flush the bad entry
		 * out of the ATC.
		 */
		mmube = mmu_geterror(faultaddr, usr ? FC_UD : FC_SD,
		    rw == S_READ ? 0 : 1);
		if (mmube & (MMU_LIMIT | MMU_TRANS)) {
			panic("trap: mmu violation");
		} else if (mmube & MMU_BERR) {
			atc_flush_entry((caddr_t)faultaddr);
			be = *(u_char *)BUSERRREG;
			mmube = 0;
		} else if (mmube & MMU_SUPER) {
			atc_flush_entry((caddr_t)faultaddr);
			be = 0;
			errtype = MMUE_SUPER;
		} else if (mmube & MMU_INVLD) {
			atc_flush_entry((caddr_t)faultaddr);
			be = 0;
			errtype = MMUE_INVALID;
		} else if (mmube & MMU_WPROT && rw == S_WRITE) {
			atc_flush_entry((caddr_t)faultaddr);
			be = 0;
			errtype = MMUE_PROTERR;
		} else {
			be = *(u_char *)BUSERRREG;
			mmube = 0;
		}
	}
#ifdef SUN3X_80
		/* Check for Parity error */
		if (cpu == CPU_SUN3X_80) {
			if (BUSERRREG->be_parityerr &&
			    (type == T_BUSERR)) {
				unsigned char mcntl;

				mcntl = MEMREG->mr_per;
				parity_report((addr_t) faultaddr, mcntl);
				MEMREG->mr_paddr = 0;
				*(u_int *)LONG_ALIGN(faultaddr) = 0;
				if (usr) {
					if (user_parity_recover(
					    (addr_t)faultaddr, rw))
						goto parity_out;
					else
						return (0);
				} else {
					kernel_parity_error();
				}
			}
		}
#endif SUN3X_80
	/*
	 * Now we can meld the user bit into the type.
	 */
	if (usr)
		type |= USER;
	if (tdebug) {
		i = type/sizeof (int);
		if ((u_int)i < TRAP_TYPES)
			printf("trap: %s\n", trap_type[i]);
		showregs("trap", type, locregs, stkfmt, be, mmube);
	}
	switch (type) {

	    case T_COPROCERR:		/* coprocessor protocol error */
#ifdef FPU
		printf("Response CIR: 0x%x\n", get_cir());
#endif FPU
		/* fall through to die: */

	    case T_ADDRERR:
		/*
		 * Kernel address error.
		 * On 68030, address errors can only happen
		 * when executing instructions on an odd boundary.
		 * Therefore, we cannot have gotten here as a
		 * result of copyin/copyout request - so panic.
		 */
default:
die:
		(void) spl7();
		showregs("", stkfmt->f_vector, locregs, stkfmt, be, mmube);
		traceback((long)locregs->r_areg[6], (long)locregs->r_sp);
		i = stkfmt->f_vector/sizeof (int);
		if (i < TRAP_TYPES)
			panic(trap_type[i]);
		panic("trap");
		/*NOTREACHED*/
	    case T_BUSERR:
		/*
		 * Kernel bus error.
		 * The fault may have been expected (eg - a probe).
		 */
		if (nofault) {
			label_t *ftmp;

			ftmp = nofault;
			nofault = 0;
			longjmp(ftmp);
		}
#ifdef FPU
		/*
		 * Check to see if it was an FPU error.
		 */
		if ((i = fpu_berror(be, locregs, *stkfmt, mmube)) == SIGSEGV)
			break;
#endif FPU
		/*
		 * See if it was an mmu error.
		 * If so, try to resolve it as a pagefault. Save lofault
		 * across this.  Here we assume that an address
		 * less than KERNELBASE + sizeof (int) is a user fault.
		 * We can do this as locore.s routines verify that the
		 * starting address is less than KERNELBASE before
		 * starting and because we know that we always have
		 * KERNELBASE mapped as invalid to serve as a "barrier".
		 */
		if (mmube) {
			lofault = u.u_lofault;
			u.u_lofault = 0;
			if (faultaddr < KERNELBASE + sizeof (int)) {
				if (lofault == 0)
					goto die;
				res = pagefault((addr_t)faultaddr, rw, 0, errtype);
			} else {
				res = pagefault((addr_t)faultaddr, rw, 1, errtype);
			}
			u.u_lofault = lofault;
		} else
			res = FC_HWERR;
		/*
		 * If we resolved the fault exit,
		 * else if we didn't and lofault wasn't set, die.
		 */
		if (res == 0)
			return (0);
		if (u.u_lofault == 0)
			goto die;
		/*
		 * Cannot resolve fault.  Return to lofault after
		 * cleaning up stack.
		 */
		if (lodebug) {
			showregs("lofault", type, locregs, stkfmt, be, mmube);
			traceback((long)locregs->r_areg[6],
			    (long)locregs->r_sp);
		}
		locregs->r_pc = u.u_lofault;
		if (FC_CODE(res) == FC_OBJERR)
			locregs->r_r0 = FC_ERRNO(res);
		else
			locregs->r_r0 = EFAULT;
		return (besize);
	    case T_ADDRERR + USER:
		/*
		 * User address error.
		 */
		if (tudebug)
			showregs("USER ADDRESS ERROR", type, locregs,
			    stkfmt, be, mmube);
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
	    case T_SPURIOUS + USER:
		/*
		 * Spurios interrupt.
		 */
		i = spl7();
		if ((i & 0x700) != 0x300)
			printf("spurious level %d interrupt\n", (i & SR_INTPRI) >> 8);
		(void) splx(i);
		return (0);
	    case T_PRIVVIO + USER:
		/*
		 * Privileged instruction fault.
		 */
		if (tudebug)
			showregs("USER PRIVILEGED INSTRUCTION", type, locregs,
			    stkfmt, be, mmube);
		u.u_addr = (char *)locregs->r_pc;
		u.u_code = stkfmt->f_vector;
		i = SIGILL;
		break;
	    case T_COPROCERR + USER:
		/*
		 * Coprocessor protocol error.
		 * Dump out obnoxious info to warn user
		 * that something isn't right w/ the 68881
		 */
		showregs("USER COPROCESSOR PROTOCOL ERROR", type, locregs,
		    stkfmt, be, mmube);
#ifdef FPU
		printf("Response CIR: 0x%x\n", get_cir());
#endif FPU
		u.u_addr = (char *)locregs->r_pc;
		u.u_code = stkfmt->f_vector;
		i = SIGILL;
		break;
	    case T_M_BADTRAP + USER:
	    case T_ILLINST + USER:
	    case T_FMTERR + USER:
		/*
		 * Some undefined trap, illegal instruction fault, or
		 * format error fault.
		 */
		if (tudebug)
			showregs("USER ILLEGAL INSTRUCTION", type, locregs,
			    stkfmt, be, mmube);
		if (type == T_M_BADTRAP + USER)
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
		{
			register struct fp_istate *isp;

			isp = &u.u_pcb.u_fp_istate;
			(void) fsave(isp);
			isp->fpis_buf[isp->fpis_bufsiz - 4] |= FPP_EXC_PEND;
			u.u_addr = fpiar();
			frestore(isp);
		}
		u.u_code = stkfmt->f_vector;
		i = SIGFPE;
		break;
	case T_ZERODIV + USER:
	case T_CHKINST + USER:
	case T_TRAPV + USER:
		/*
		 * Divide by zero, CHK [CHK2] instr,
		 * TRAPV [cpTRAPcc TRAPcc] instr, or fall from above.
		 */
		if (stkfmt->f_stkfmt != SF_NORMAL6)
			panic("expected normal6 stack format");
		u.u_addr = (char *)
		    ((struct bei_normal6 *)&stkfmt->f_beibase)->bei_pc;
		u.u_code = stkfmt->f_vector;
		i = SIGFPE;
		break;
	case T_BUSERR + USER:
		/*
		 * User bus error.  Try to handle FPA, then pagefault, and
		 * failing that, grow the stack automatically if a data fault.
		 */
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
		if (stkfmt->f_stkfmt == SF_MEDIUM) {
			beimp->bei_faultc = beimp->bei_rerunc;
			beimp->bei_faultb = beimp->bei_rerunb;
		} else {
			beilp->bei_faultc = beilp->bei_rerunc;
			beilp->bei_faultb = beilp->bei_rerunb;
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

		/*
		 * Check to see if it was an mmu error.
		 */
		if (mmube) {
			if ((bedebug &&
			    (beimp->bei_faultb || beimp->bei_faultc)) ||
			    (bedebug > 1 && beimp->bei_fault)) {
				if (stkfmt->f_stkfmt == SF_MEDIUM) {
					printf("med fault b %d %x, c %d %x, ",
					    beimp->bei_faultb,
					    locregs->r_pc + 4,
					    beimp->bei_faultc,
					    locregs->r_pc + 2);
					printf("d %d %x\n", beimp->bei_dfault,
					    beimp->bei_fault);
				} else {
					printf("long fault b %d %x, c %d %x, ",
					    beilp->bei_faultb,
					    beilp->bei_stageb,
					    beilp->bei_faultc,
					    beilp->bei_stageb - 2);
					printf("d %d %x\n", beilp->bei_dfault,
					    beilp->bei_fault);
				}
			}
			res = pagefault((addr_t)faultaddr, rw, 0, errtype);
#ifdef PARITY
			if (put_parity)
				put_parity_error((addr_t)faultaddr);
#endif PARITY
			if (res == 0)
				return (0);
			/*
			 * XXX ugly cast from u_int to int.
			 * grow() should really use u_int XXX
			 */
#ifdef SUN3X_80
parity_out:
#endif SUN3X_80
			if (isdf && grow((int)faultaddr)) {
					nosig = 1;
					besize = 0;
					goto out;
			}
			u.u_addr = (char *)faultaddr;
		} else {
			/*
			 * There was an error other than from the mmu.
			 * We cannot handle it.
			 */
			res = FC_HWERR;
		}
		if (tudebug)
			showregs("USER BUS ERROR", type, locregs, stkfmt, be, mmube);
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
	    case T_TRACE:
		/*
		 * Caused by tracing trap instr.
		 */
		u.u_pcb.pcb_flags |= TRACE_PENDING;
		return (0);
	    case T_TRACE + USER:
		/*
		 * Trace trap.
		 */
		dotrace(locregs);
		/*
		 * The only way to get here with a SF_COPROC on the stack
		 * was to interrupt a 68882 instruction, and to force a
		 * trace trap for rescheduling.  Since we can't send a
		 * signal with an SF_COPROC on the stack, and the
		 * save_user_state() scheme may already be in use, just
		 * skip signal processing under these conditions.
		 */
		if (stkfmt->f_stkfmt == SF_COPROC)
			goto out_no_signal;
		goto out;
	    case T_BRKPT + USER:
		/*
		 * Bpt instruction (trap #15) fault
		 */
		i = SIGTRAP;
		break;
	    case T_EMU1010 + USER:
	    case T_EMU1111 + USER:
		/*
		 * 1010 emulator or 1111 emulator trap.
		 */
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
out_no_signal:
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
	DELAY(2000000);
}

showregs(str, type, locregs, fmtp, be, mmube)
	char *str;
	int type;
	struct regs *locregs;
	struct stkfmt *fmtp;
	u_char be;
	u_short mmube;
{
	int *r, s;
	int fcode;
	u_int accaddr;
	char *why;
	struct pte *pte = (struct pte *)NULL;

	s = spl7();
	if (masterprocp == NULL || peekc(u.u_comm) == -1)
		printf("(unknown): %s\n", str);
	else
		printf("pid %d, `%s': %s\n", masterprocp->p_pid, u.u_comm, str);
	printf("vector 0x%x, pc 0x%x, sr 0x%x, stkfmt 0x%x, root ptr 0x%x\n",
	    fmtp->f_vector, locregs->r_pc, locregs->r_sr, fmtp->f_stkfmt,
	    mmu_getrootptr());
	type &= ~USER;
	if (type == T_BUSERR) {
		printf("Bus Error Reg %b, ", be, BUSERR_BITS);
		printf("MMU Status Reg %x\n", mmube);
	}
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
		if (u.u_procp)
			pte = hat_ptefind(u.u_procp->p_as, (addr_t)accaddr);
		if (pte == NULL)
			printf("pte not found\n");
		else
			printf("pte %x\n", *(u_int *)pte);
	}
	r = &locregs->r_dreg[0];
	printf("D0-D7  %x %x %x %x %x %x %x %x\n",
	    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
	r = &locregs->r_areg[0];
	printf("A0-A7  %x %x %x %x %x %x %x %x\n",
	    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
	if (tudebug > 1 && (boothowto & RB_DEBUG)) {
		CALL_DEBUG();
	} else {
		DELAY(2000000);
	}
	(void) splx(s);
}

#ifdef PARITY
put_parity_error(addr)
unsigned char *addr;
{

	struct pte *pte;
	int s;
	unsigned char instr;
	unsigned char *raddr;
	register struct proc *p;
	register struct as *as;
	extern int debug_parity;
	unsigned int *tp = (unsigned int *) &time;


	if ((addr != (unsigned char *)0x2020) || (u.u_exdata.ux_mag != ZMAGIC))
		return;
	p = u.u_procp;
	as = p->p_as;
	pte = hat_ptefind(as, addr);
	pte->pte_readonly = PG_KW;
	raddr = addr + (*tp % 0x4);
	instr = *raddr;		/* magical */

	if (debug_parity)
		printf(" Put a parity error at %x Byte  %x\n", raddr, instr);
	MEMREG->mr_per &= ~(PER_CHECK | PER_INTENA);
	s = spl7();
	pperr(raddr);
	(void)splx(s);
	MEMREG->mr_per |= PER_CHECK | PER_INTENA;
	pte->pte_readonly = PG_UR;
}
#endif PARITY

#ifdef SUN3X_80
kernel_parity_error()
{

	panic(" synchronous parity error - kernel");
	/* NOT REACHED */
}
#endif SUN3X_80

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

