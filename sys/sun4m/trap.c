#ifndef lint
static char sccsid[] = "@(#)trap.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
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
#include <vm/as.h>
#include <vm/seg.h>

#include <sys/reboot.h>
#include <debug/debug.h>

#include <os/atom.h>

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

#ifndef	OPENPROMS
#define	prom_write	???
#else	OPENPROMS
#ifndef	MULTIPROCESSOR
extern void	prom_write();
#else	MULTIPROCESSOR
#define	prom_write	mpprom_write
#endif	MULTIPROCESSOR
#endif	OPENPROMS

#if defined(TRAPDEBUG) || defined(lint)
int tdebug = 0;
int lodebug = 0;
int faultdebug = 0;
#else
#define	tdebug	0
#define	lodebug	0
#define	faultdebug	0
#endif defined(TRAPDEBUG) || defined(lint)

#define	SIMU_SUCCESS 1		/* simulation worked */
#define	SIMU_ILLEGAL 0		/* tried to simulate an illegal instruction */
#define	SIMU_FAULT -1		/* simulation generated an illegal access */
#define	SIMU_DZERO -2		/* divided by zero detected */

u_int beval = FT_INVALID_ADDR << FS_FTSHIFT;	/* used by fp_traps to setup
						 * a datafault trap
						 */

#ifdef	TRAPTRACE
int	traptrace = 0;
#endif	TRAPTRACE

void
badtrap(type, rp, addr, mmu_fsr, rw)
	register unsigned type;
	register struct regs *rp;
	register addr_t addr;
	register u_int mmu_fsr;
	register enum seg_rw rw;
{
	extern int cpuid;

	trigger_logan();
	(void) spl7();
	printf("BAD TRAP: cpu=%d type=%x rp=%x addr=%x mmu_fsr=%x rw=%x\n",
			cpuid, type, rp, addr, mmu_fsr, rw);
	mmu_print_sfsr(mmu_fsr);
#if defined(MULTIPROCESSOR) && defined(PROM_PARANOIA)
	mpprom_eprintf("PROM_PARANOIA: inside badtrap()\n");
#endif
	printf("regs at %x:\n", rp);
	printf("\tpsr=%x pc=%x npc=%x\n",
		rp->r_psr, rp->r_pc, rp->r_npc);
	printf("\ty: %x g1: %x g2: %x g3: %x\n",
		rp->r_y, rp->r_g1, rp->r_g2, rp->r_g3);
	printf("\tg4: %x g5: %x g6: %x g7: %x\n",
		rp->r_g4, rp->r_g5, rp->r_g6, rp->r_g7);
	printf("\to0: %x o1: %x o2: %x o3: %x\n",
		rp->r_o0, rp->r_o1, rp->r_o2, rp->r_o3);
	printf("\to4: %x o5: %x sp: %x ra: %x\n",
		rp->r_o4, rp->r_o5, rp->r_o6, rp->r_o7);
	showregs(type, rp, addr, mmu_fsr, rw);
	traceback((caddr_t)rp->r_sp);
	if (type < TRAP_TYPES)
		panic(trap_type[type]);
	panic("trap");
}

enum fault_type
    get_faulttype(type, rp, addr, mmu_fsr, rw)
register unsigned type;
register struct regs *rp;
register addr_t addr;
register u_int mmu_fsr;
register enum seg_rw rw;
{
	enum fault_type fault_type;

#ifdef  TRAPTRACE
	int pid;

	if (u.u_procp)
		pid = u.u_procp->p_pid;
	else
		pid = -1;
#endif  TRAPTRACE

	switch (X_FAULT_TYPE(mmu_fsr)) {
		/*
		 * FT_NONE can't happen, locore stops us from getting here
		 * if sfsr has not ft_type
		 */

		case FT_TRANS_ERROR:
		case FT_ACC_BUSERR:
		case FT_INTERNAL:
		default:
			printf("Unexpected trap %d fault %x\n", type,
				X_FAULT_TYPE(mmu_fsr));
			badtrap(type, rp, addr, mmu_fsr, rw);
			/* NO RETURN */

		case FT_INVALID_ADDR:
#ifdef  TRAPTRACE
			if (traptrace) {
				printf("%d: data fault at 0x%x: ", pid, addr);
				printf("address not mapped\n");
			}
#endif
			if (tdebug)
				printf("FT_INV\n");
			fault_type = F_INVAL;
			break;

		case FT_PROT_ERROR:
		case FT_PRIV_ERROR:
#ifdef  TRAPTRACE
			if (traptrace) {
				printf("%d: data fault at 0x%x: ", pid, addr);
				printf("protection violation\n");
			}
#endif
			if (tdebug)
				printf("FT_PROT\n");
			fault_type = F_PROT;
			break;
		}
		return (fault_type);

}

void
check_fsr(type, rp, addr, mmu_fsr, rw)
	register unsigned type;
	register struct regs *rp;
	register addr_t addr;
	register u_int mmu_fsr;
	register enum seg_rw rw;
{
	extern int vac_parity_chk_dis();

	if ((mmu_fsr & MMU_SFSR_FATAL) || (mmu_fsr & MMU_SFSR_UD)) {

		/* bad bits in the SFSR */
		if (lodebug)
			showregs(type, rp, addr, mmu_fsr, rw);
		/*
		 * If this trap was caused by a MBUS Uncorrectable
		 * error, it's an ECC uncorrectable error on read.
		 * This requires to print out the detailed info (
		 * including the SIMM number) before calling panic
		 */
		if ((mmu_fsr & MMU_SFSR_UC) ||
		    (mmu_fsr & MMU_SFSR_UD)) {
			struct pte pte;
			addr_t paddr;
			int parity_err;

			(void) spl7();
			parity_err = vac_parity_chk_dis(mmu_fsr, 0);
			if (parity_err)
				printf("module parity error:\n");
			else
				printf("fatal system fault:\n");
			mmu_getpte(addr, &pte);
			if (pte_valid(&pte)) {
				paddr = (caddr_t) (pte.PhysicalPageNumber <<
					MMU_PAGESHIFT)
					+ ((int) addr & MMU_PAGEOFFSET);
				log_mem_err(mmu_fsr, (u_int) 0,
					(u_int) paddr, (u_int) 1);
			} else
				printf("addr %x is not valid\n", addr);
			printf("Control Registers:\n");
			printf("\tsfsr = 0x%x, %s fault ", mmu_fsr,
				(rw == S_WRITE? "write": "read"));
			printf("at vaddr 0x%x\n", addr);
			if (lodebug)
				mmu_print_sfsr(mmu_fsr);
			panic("memory error");
			/* NOTREACHED */
		}
	}
}


#ifdef  VME
/*
 * If "vme_int_ack_warn" is set, we will print
 * a warning message whenever we drop a VME INT-ACK.
 */
int     vme_int_ack_warn = 0;
/*
 * "vme_int_ack_drops" is a quick count of how many
 * times we have dropped a VME INT-ACK cycle. It should
 * stay at zero.
 */
int     vme_int_ack_drops = 0;
#endif  VME
/*
 * Called from the trap handler when a processor trap occurs.
 * Addr, mmu_fsr and rw only are passed for text and data faults.
 */


/*VARARGS2*/
void
trap(type, rp, addr, mmu_fsr, rw)
	register unsigned type;
	register struct regs *rp;
	addr_t addr;
	register u_int mmu_fsr;
	register enum seg_rw rw;
{
	register int i = 0;
	register struct proc *p;
	struct timeval syst;
	int lofault;
	faultcode_t pagefault(), res;
	u_int inst;
	enum fault_type fault_type;
	int mask;
#ifdef	TRAPTRACE
	int pid;
#endif	TRAPTRACE
	extern void module_wkaround();

#ifdef	MULTIPROCESSOR
	klock_enter();
#endif	MULTIPROCESSOR

	p = u.u_procp;
#ifdef	TRAPTRACE
	if (p) pid = p->p_pid;
	else pid = -1;
#endif	TRAPTRACE
	cnt.v_trap++;
	syst = u.u_ru.ru_stime;
	if (tdebug)
		showregs(type, rp, addr, mmu_fsr, rw);

	if (USERMODE(rp->r_psr)) {
		if (tdebug)
			printf("Setting user bit\n");
		type |= USER;
		u.u_ar0 = (int *)rp;
	}


	/*
	 * take any pending floating point exceptions now
	 */
	syncfpu(rp);


	switch (type) {

	case T_ALIGNMENT:	/* supv alignment error */
/*		printf("kernel alignment fault\n"); */
		goto die;

	case T_DATA_STORE:		/* store buffer exception */
	case T_DATA_STORE + USER:	/* store buffer exception */

		(void) check_fsr(type, rp, addr, mmu_fsr, rw);
		/*
		 * FIXME: currently we panic the system when store
		 * buffer exception occurs.  We should try to recover
		 * from it.
		 */
		if (tdebug) {
			printf("T_DATA_STORE mmu_fsr %x ", mmu_fsr);
			showregs(type, rp, addr, mmu_fsr, rw);
		}
		goto die;
		/* NOTREACHED */
	default:
		/*
		 * Check for user software trap.
		 */
		if (type & USER) {
#ifdef	TRAPTRACE
			if (traptrace) {
				printf("%d: bad user software trap 0x%x\n",
					pid, type);
			}
#endif
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
		badtrap(type, rp, addr, mmu_fsr, rw);
		/* NOTREACHED */

	case T_TEXT_FAULT:		/* system text access fault */

		(void) check_fsr(type, rp, addr, mmu_fsr, rw);

		/*
		 * 605 overwrite bit is not checked here.
		 * whether set or not kernel is going to die any way
		 *
		 */
		/* do the lda of sup/user inst space for completeness */
#ifdef	TRAPTRACE
		if (traptrace) {
			printf("%d: text fault at 0x%x\n", pid, addr);
		}
#endif

		if (lodebug)
			showregs(type, rp, addr, mmu_fsr, rw);
		goto die;

	case T_DATA_FAULT:		/* system data access fault */
		/* may have been expected by C (e.g., bus probe) */
		/*
		 * 605 overwrite bit story
		 * if set says the second fault could have been a
		 * a translation error on an instruction fault only.
		 * In any case translation errors are panics and hence
		 * we print info and giveup
		 */

		if (tdebug)
			printf("got to DATAFAULT\n");
		if (nofault) {
			label_t *ftmp;

			ftmp = nofault;
			nofault = 0;
			longjmp(ftmp);
		}
#ifdef  VME
                /*
                 * If this trap was caused by a VME device not
                 * completing an interrupt acknowledge cycle,
                 * print a warning message and continue.
                 * NOTE: leaves the destination register of the load
                 * entirely unmodified ...
                 */
                if ((type == T_DATA_FAULT) && (mmu_fsr & MMU_SFSR_FATAL) &&
                    ((int)addr & ~0xF) == V_VMEBUS_VEC) {
                        vme_int_ack_drops ++;
                        if (vme_int_ack_warn) {
                                printf("Warning: VME dropped an INT-ACK cycle\n");             
                                mmu_print_sfsr(mmu_fsr);
                        }
                        rp->r_pc = rp->r_npc;
                        rp->r_npc = rp->r_npc + 4;
                        return;
                }
#endif  VME
		/* do module specific workarounds if any */
 		module_wkaround(&addr, rp, rw, mmu_fsr);

		(void) check_fsr(type, rp, addr, mmu_fsr, rw);

		/*  get_faulttype can panic if bad trap */
		fault_type = get_faulttype(type, rp, addr, mmu_fsr, rw);


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
		lofault = u.u_lofault;
		u.u_lofault = 0;

		if (addr < (addr_t)KERNELBASE + sizeof (int)) {
			if (lofault == 0) {
				goto die;
			}
			res = pagefault(addr, rw, 0, (u_int) fault_type);
			if (res != 0 && grow((int)addr))
				res = 0;
		} else {
			res = pagefault(addr, rw, 1, (u_int) fault_type);
		}
		/*
		 * Restore lofault. If we resolved the fault, exit.
		 * If we didn't and lofault wasn't set, die.
		 */
		u.u_lofault = lofault;
		if (res == 0)
			return;

		if (lofault == 0)
			goto die;

		/*
		 * Cannot resolve fault.  Return to lofault.
		 */
		if (lodebug) {
			showregs(type, rp, addr, mmu_fsr, rw);
			traceback((caddr_t)rp->r_sp);
		}
		if (FC_CODE(res) == FC_OBJERR)
			res = FC_ERRNO(res);
		else
			res = EFAULT;
		rp->r_g1 = res;
		rp->r_pc = lofault;
		rp->r_npc = lofault + 4;
		return;

	case T_DATA_FAULT + USER:	/* user data access fault */
		/* module specific workarounds if any */	
		/* we know the vikings need workaround in the case of
		 * data faults. See vik_module_wkaround() for deatails
		 */
		module_wkaround(&addr, rp, rw, mmu_fsr);
		/* FALL THROUGH */
	case T_TEXT_FAULT + USER:	/* user text access fault */

		/*
		 * 605 fault overwrite story.
		 * Instruction-instruction: Resolve at PC and let the
		 * other one happen again. The second fault could be
		 * for a prefetch of instructions. Hence we handle the
		 * fault pc  only.
		 * instruction-translation: Translation is treated as
		 * bad thing by the kernel. panic
		 * translation-otherfault: bad panic
		 * data - data: CANNOT happen on 601 IU
		 * data - translation: panic because of trans error
		 */


		if ((mmu_fsr&MMU_SFSR_OW) &&
			(X_FAULT_TYPE(mmu_fsr) == FT_TRANS_ERROR)) {
			printf("cpu %d: Multiple faults occurred\n", cpuid);
			printf("\t first fault %s at pc %x \n", cpuid,
				(type == (T_DATA_FAULT+USER)?
					"User Data ":"User text "));
			printf("\t second fault: translation error ");
			printf("at addr %x\n", addr);
		}

		if (tdebug) {
			printf("T_DATA_FAULT | USER mmu_fsr %x ", mmu_fsr);
			printf("X_FAULT_TYPE %x\n",
				(u_int)X_FAULT_TYPE(mmu_fsr));
		}

		if ((mmu_fsr & (MMU_SFSR_TO | MMU_SFSR_BE)) &&
			!(mmu_fsr & MMU_SFSR_UC)) {
			res = FC_HWERR;
			goto badbus;
		}


		(void) check_fsr(type, rp, addr, mmu_fsr, rw);

		/* get_faulttype can panic if bad trap */
		fault_type = get_faulttype(type, rp, addr, mmu_fsr, rw);

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
			printf("user %s fault:  addr=0x%x mmu_fsr=0x%x\n",
				fault_str, addr, mmu_fsr);
		}
		if (tdebug) {
			printf("Calling pagefault: ");
			printf("addr %x rw %x fault_type %x\n",
				addr, rw, fault_type);
		}


		res = pagefault(addr, rw, 0, (u_int) fault_type);
		if (res == 0)
			return;


		if (type == T_DATA_FAULT + USER) {
			/*
			 * Grow the stack automatically.
			 */
			if (grow((int)addr))
				goto out;
		}

		if (tudebug)
			showregs(type, rp, addr, mmu_fsr, rw);
		/*
		 * In the case where both pagefault and grow fail,
		 * set the code to the value provided by pagefault.
		 * We map errors relating to the underlying object
		 * to SIGBUS, others to SIGSEGV.
		 */
badbus:
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
#ifdef	TRAPTRACE
		if (traptrace) {
			printf("%d: alignment error at 0x%x\n",
				pid, addr);
		}
#endif
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
#ifdef	TRAPTRACE
		if (traptrace) {
			printf("%d: privileged instruction at 0x%x\n",
				pid, rp->r_pc);
		}
#endif
		if (tudebug)
			showregs(type, rp, (addr_t)0, 0, S_OTHER);
		u.u_addr = (char *)rp->r_pc;
		u.u_code = ILL_PRIVINSTR_FAULT;
		i = SIGILL;
		break;

	case T_UNIMP_INSTR + USER:	/* illegal instruction fault */
#ifdef	TRAPTRACE
		if (traptrace) {
			printf("%d: unimplemented instruction at 0x%x\n",
				pid, rp->r_pc);
		}
#endif
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
#ifdef	TRAPTRACE
		if (traptrace) {
			printf("%d: divide by zero at 0x%x\n",
				pid, rp->r_pc);
		}
#endif
		if (tudebug && tudebugfpe)
			showregs(type, rp, (addr_t)0, 0, S_OTHER);
		rp->r_pc = rp->r_npc;	/* skip trap instruction */
		rp->r_npc += 4;
		u.u_addr = (char *)rp->r_pc;
		u.u_code = (int)FPE_INTDIV_TRAP;
		i = SIGFPE;
		break;

	case T_INT_OVERFLOW + USER:	/* integer overflow */
#ifdef	TRAPTRACE
		if (traptrace) {
			printf("%d: integer overflow at 0x%x\n",
				pid, rp->r_pc);
		}
#endif
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
#ifdef	TRAPTRACE
		if (traptrace) {
			printf("%d: flotating point exception at 0x%x\n",
				pid, rp->r_pc);
		}
#endif
		if (tudebug && tudebugfpe)
			showregs(type, rp, addr, 0, S_OTHER);
		u.u_code = mmu_fsr;	/* this is actually fp trap code
					 * from fp_traps()
					 */
		u.u_addr = (char *)addr;
		i = SIGFPE;
		break;

	case T_BREAKPOINT + USER:	/* breakpoint trap (t 1) */
#ifdef	TRAPTRACE
		if (traptrace) {
			printf("%d: breakpoint at 0x%x\n",
				pid, rp->r_pc);
		}
#endif
		if (tudebug && tudebugbpt)
			showregs(type, rp, (addr_t)0, 0, S_OTHER);
		i = SIGTRAP;
		break;

	case T_TAG_OVERFLOW + USER:	/* tag overflow (taddcctv, tsubcctv) */
#ifdef	TRAPTRACE
		if (traptrace) {
			printf("%d: tag overflow at 0x%x\n",
				pid, rp->r_pc);
		}
#endif
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

			/*
			 * XXX - do we still need PARTIAL_ALIGN?
			 */
#ifdef	PARTIAL_ALIGN
			if ((partial_align? ((int)sp & 0x3): ((int)sp & 0x7))) {
#else
			if (((int)sp & (STACK_ALIGN-1))) {
#endif	PARTIAL_ALIGN
				u.u_addr = (char *)rp->r_pc;
				u.u_code = ILL_STACK;
				i = SIGILL;
				break;
			} else {
#ifdef	SAS
				if (tudebug) {
					printf("trap: copyout(%x, %x, %x) ",
						(caddr_t)&u.u_pcb.pcb_wbuf[j],
						sp, sizeof (struct rwindow));
					printf("j %x\n", j);
				}
#endif	SAS
				inst = copyout((caddr_t)&u.u_pcb.pcb_wbuf[j],
					sp, sizeof (struct rwindow));
				if (inst != 0) {
#ifdef	SAS
					printf("%d: ", u.u_procp->p_pid);
					printf("bad copyout ret %x ",
						inst);
					printf("pc %x sp %x ",
						rp->r_pc, sp);
					printf("wbuf %x j %x\n",
						u.u_pcb.pcb_wbcnt, j);
#endif	SAS
					u.u_addr = (char *)rp->r_pc;
					u.u_code = ILL_STACK;
					i = SIGILL;
					break;
				}
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
		p->p_pri = p->p_usrpri;
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
		(void) spl0();
	} else{
		p->p_pri = p->p_usrpri;
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


#ifdef	SYSCALL_TRACE_STRINGS
int	syscall_strs[] = {		/* which args to print as ascii */
	0,				/*   0 = indir */
	0,				/*   1 = exit */
	0,				/*   2 = fork */
	0,				/*   3 = read */
	0,				/*   4 = write */
	01,				/*   5 = open */
	0,				/*   6 = close */
	0,				/*   7 = wait4 */
	01,				/*   8 = creat */
	03,				/*   9 = link */
	01,				/*  10 = unlink */
	01,				/*  11 = execv */
	01,				/*  12 = chdir */
	0,				/*  13 = old time */
	01,				/*  14 = mknod */
	01,				/*  15 = chmod */
	01,				/*  16 = chown; now 3 args */
	0,				/*  17 = brk */
	01,				/*  18 = old stat */
	0,				/*  19 = lseek */
	0,				/*  20 = getpid */
	0,				/*  21 = old mount */
#ifdef	UFS
	01,				/*  22 = old umount */
#else
	0,				/*  22 = old umount */
#endif
	0,				/*  23 = old setuid */
	0,				/*  24 = getuid */
	0,				/*  25 = old stime */
	0,				/*  26 = ptrace */
	0,				/*  27 = old alarm */
	0,				/*  28 = old fstat */
	0,				/*  29 = opause */
	01,				/*  30 = old utime */
	0,				/*  31 = was stty */
	0,				/*  32 = was gtty */
	01,				/*  33 = access */
	0,				/*  34 = old nice */
	0,				/*  35 = old ftime */
	0,				/*  36 = sync */
	0,				/*  37 = kill */
	01,				/*  38 = stat */
	0,				/*  39 = old setpgrp */
	01,				/*  40 = lstat */
	0,				/*  41 = dup */
	0,				/*  42 = pipe */
	0,				/*  43 = old times */
	0,				/*  44 = profil */
	0,				/*  45 = nosys */
	0,				/*  46 = old setgid */
	0,				/*  47 = getgid */
	0,				/*  48 = old sig */
	0,				/*  49 = reserved for USG */
	0,				/*  50 = reserved for USG */
#ifdef SYSACCT
	0,				/*  51 = turn acct off/on */
#else
	0,				/*  51 = not configured */
#endif SYSACCT
	0,				/*  52 = old set phys addr */
	0,				/*  53 = memory control */
	0,				/*  54 = ioctl */
	02,				/*  55 = reboot */
#ifdef sparc
	0,				/*  56 = wait3 */
#else
	0,				/*  56 = old mpxchan */
#endif sparc
	03,				/*  57 = symlink */
	01,				/*  58 = readlink */
	01,				/*  59 = execve */
	0,				/*  60 = umask */
	01,				/*  61 = chroot */
	0,				/*  62 = fstat */
	0,				/*  63 = used internally */
	0,				/*  64 = getpagesize */
	0,				/*  65 = old msync */
	0,				/*  66 = vfork */
	0,				/*  67 = old vread */
	0,				/*  68 = old vwrite */
	0,				/*  69 = sbrk */
	0,				/*  70 = sstk */
	0,				/*  71 = mmap */
	0,				/*  72 = old vadvise */
	0,				/*  73 = munmap */
	0,				/*  74 = mprotect */
	0,				/*  75 = old madvise */
	0,				/*  76 = vhangup */
	0,				/*  77 = old vlimit */
	0,				/*  78 = mincore */
	0,				/*  79 = getgroups */
	0,				/*  80 = setgroups */
	0,				/*  81 = getpgrp */
	0,				/*  82 = setpgrp */
	0,				/*  83 = setitimer */
	0,				/*  84 = old wait & wait3 */
	01,				/*  85 = swapon */
	0,				/*  86 = getitimer */
	0,				/*  87 = gethostname */
	01,				/*  88 = sethostname */
	0,				/*  89 = getdtablesize */
	0,				/*  90 = dup2 */
	0,				/*  91 = getdopt */
	0,				/*  92 = fcntl */
	0,				/*  93 = select */
	0,				/*  94 = setdopt */
	0,				/*  95 = fsync */
	0,				/*  96 = setpriority */
	0,				/*  97 = socket */
	0,				/*  98 = connect */
	0,				/*  99 = accept */
	0,				/* 100 = getpriority */
	0,				/* 101 = send */
	0,				/* 102 = recv */
	0,				/* 103 = old socketaddr */
	0,				/* 104 = bind */
	0,				/* 105 = setsockopt */
	0,				/* 106 = listen */
	0,				/* 107 = old vtimes */
	0,				/* 108 = sigvec */
	0,				/* 109 = sigblock */
	0,				/* 110 = sigsetmask */
	0,				/* 111 = sigpause */
	0,				/* 112 = sigstack */
	0,				/* 113 = recvmsg */
	0,				/* 114 = sendmsg */
#ifdef	TRACE
	0,				/* 115 = vtrace */
#else	TRACE
	0,				/* 115 = nosys */
#endif	TRACE
	0,				/* 116 = gettimeofday */
	0,				/* 117 = getrusage */
	0,				/* 118 = getsockopt */
#ifdef vax
	0,				/* 119 = resuba */
#else
	0,				/* 119 = nosys */
#endif
	0,				/* 120 = readv */
	0,				/* 121 = writev */
	0,				/* 122 = settimeofday */
	0,				/* 123 = fchown */
	0,				/* 124 = fchmod */
	0,				/* 125 = recvfrom */
	0,				/* 126 = setreuid */
	0,				/* 127 = setregid */
	03,				/* 128 = rename */
	01,				/* 129 = truncate */
	0,				/* 130 = ftruncate */
	0,				/* 131 = flock */
	0,				/* 132 = nosys */
	0,				/* 133 = sendto */
	0,				/* 134 = shutdown */
	0,				/* 135 = socketpair */
	01,				/* 136 = mkdir */
	01,				/* 137 = rmdir */
	01,				/* 138 = utimes */
	0,				/* 139 = signalcleanup */
	0,				/* 140 = adjtime */
	0,				/* 141 = getpeername */
	0,				/* 142 = gethostid */
	0,				/* 143 = old sethostid */
	0,				/* 144 = getrlimit */
	0,				/* 145 = setrlimit */
	0,				/* 146 = killpg */
	0,				/* 147 = nosys */
	0,		/* XXX */	/* 148 = old quota */
	0,		/* XXX */	/* 149 = old qquota */
	0,				/* 150 = getsockname */
	0,				/* 151 = getmsg */
	0,				/* 152 = putmsg */
	0,				/* 153 = poll */
#ifdef NFSSERVER
	0,				/* 154 = old nfs_mount */
	0,				/* 155 = nfs_svc */
#else
	0,				/* 154 = nosys */
	0,				/* 155 = errsys */
#endif
	0,				/* 156 = getdirentries */
	01,				/* 157 = statfs */
	0,				/* 158 = fstatfs */
	01,				/* 159 = unmount */
#ifdef NFSCLIENT
	0,				/* 160 = async_daemon */
#else
	0,				/* 160 = errsys */
#endif
#ifdef NFSSERVER
	0,				/* 161 = get file handle */
#else
	0,				/* 161 = nosys */
#endif
	0,				/* 162 = getdomainname */
	01,				/* 163 = setdomainname */
#ifdef RT_SCHEDULE
	0,				/* 164 = rtschedule */
#else
	0,				/* 164 = not configured */
#endif RT_SCHEDULE
#ifdef QUOTA
	0,				/* 165 = quotactl */
#else
	0,				/* 165 = not configured */
#endif QUOTA
#ifdef NFSSERVER
	0,				/* 166 = exportfs */
#else
	0,				/* 166 = not configured */
#endif
	03,				/* 167 = mount */
	0,				/* 168 = ustat */
#ifdef IPCSEMAPHORE
	0,				/* 169 = semsys */
#else
	0,				/* 169 = not configured */
#endif
#ifdef IPCMESSAGE
	0,				/* 170 = msgsys */
#else
	0,				/* 170 = not configured */
#endif
#ifdef IPCSHMEM
	0,				/* 171 = shmsys */
#else
	0,				/* 171 = not configured */
#endif
#ifdef SYSAUDIT
	0,				/* 172 = auditsys (audit control) */
#else
	0,				/* 172 = not configured */
#endif SYSAUDIT
#ifdef RFS
	0,				/* 173 = RFS calls */
#else
	0,				/* 173 = not configured */
#endif
	0,				/* 174 = getdents */
	0,				/* 175 = setsid & s5 setpgrp() */
	0,				/* 176 = fchdir */
	0,				/* 177 = fchroot */
#ifdef VPIX
	0,				/* 178 = VP/ix system calls */
#else
	0,				/* 178 = not configured */
#endif
#ifdef ASYNCHIO
	0,				/* 179 = aioread */
	0,				/* 180 = aiowrite */
	0,				/* 181 = aiowait */
	0,				/* 182 = aiocancel */
#else
	0,				/* 179 not configured */
	0,				/* 180 not configured */
	0,				/* 181 not configured */
	0,				/* 182 not configured */
#endif ASYNCHIO
	0,				/* 183 = sigpending */
	0,				/* 184 = AVAILABLE */
	0,				/* 185 = setpgid */
	01,				/* 186 = pathconf */
	0,				/* 187 = fpathconf */
	0,				/* 188 = sysconf */
	01,				/* 189 = uname */
#ifdef VDDRV
	0,				/* 190 = reserved - Loadable syscalls */
	0,				/* 191 = reserved - Loadable syscalls */
	0,				/* 192 = reserved - Loadable syscalls */
	0,				/* 193 = reserved - Loadable syscalls */
	0,				/* 194 = reserved - Loadable syscalls */
	0,				/* 195 = reserved - Loadable syscalls */
	0,				/* 196 = reserved - Loadable syscalls */
	0,				/* 197 = reserved - Loadable syscalls */
#endif
};
#endif	SYSCALL_TRACE_STRINGS

#ifdef SYSCALLTRACE
#ifdef	MULTIPROCESSOR
int		syscalltracelockok = 0;
atom_t		syscalltracelock;
#endif
char		syscalltracebuf[4096];
static char *
eostr(bp)
char *bp;
{
	while (*bp) ++bp;
	return (bp);
}

static void
prsct(code, na, fmt, arg)
	unsigned	code;
	char		*fmt;
{
	extern char *sprintf();
	register int i;
	char *cp;
	char *bp = syscalltracebuf;

	*bp = 0;

#ifdef	MULTIPROCESSOR
/*
 * Yes, this could be a race, but what are the
 * chances of both processes coming through here
 * at the same time before we initialize
 * this lock? Zero.
 */
	if (!syscalltracelockok) {
		atom_init(syscalltracelock);
		syscalltracelockok = 1;
	}
	atom_wcs(syscalltracelock);
	(void) sprintf(bp = eostr(bp), "%d/", cpuid);
#endif
	(void) sprintf(bp = eostr(bp), "%d: ", u.u_procp->p_pid);
		(void) sprintf(bp = eostr(bp), "sys 0x%x", code);
	if (code < nsysent)
		(void) sprintf(bp = eostr(bp), " %s",
				syscallnames[code]);
	else
		(void) sprintf(bp = eostr(bp), " ???");
	cp = "(";
	for (i = 0; i < na; i++) {
#ifdef	SYSCALL_TRACE_STRINGS
		if ((code < nsysent) &&
		    (syscall_strs[code] & (1<<i))) {
			register int l, c, a;
			(void) sprintf(bp = eostr(bp), "%s", cp);
			a = u.u_ap[i];
			if (!a)
				(void) sprintf(bp = eostr(bp), "NULL");
			else if (fubyte((caddr_t) a) < 0)
				(void) sprintf(bp = eostr(bp), "%x (?)",
						a);
			else {
				(void) sprintf(bp = eostr(bp), "\"");
				for (l = 0; l < 80; ++l) {
					c = fubyte((caddr_t) a);
					if (c <= 0) break;
					(void) sprintf(bp = eostr(bp),
							"%c", c);
					a++;
				}
				if (c < 0) sprintf(bp = eostr(bp),
							"\" (?)");
				else if (c) sprintf(bp = eostr(bp),
							"\"...");
				else sprintf(bp = eostr(bp), "\"");
			}
		} else
#endif	SYSCALL_TRACE_STRINGS
			(void) sprintf(bp = eostr(bp), "%s%x", cp,
				u.u_ap[i]);
		cp = ", ";
	}
	if (i)
		(void) sprintf(bp = eostr(bp), ")");
	(void) sprintf(bp = eostr(bp), fmt, arg);
	bp = eostr(bp);
	*bp++ = '\n';
	prom_writestr(syscalltracebuf, bp - syscalltracebuf);
#ifdef	MULTIPROCESSOR
	atom_clr(syscalltracelock);
#endif
}
#endif	SYSCALLTRACE

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

#ifdef	MULTIPROCESSOR
	klock_enter();
#endif	MULTIPROCESSOR

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
		if (syscalltrace & 1)
			prsct(code, callp->sy_narg,
				" ...", 0);
#endif	SYSCALLTRACE
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
			if (syscalltrace & 2)
				prsct(code, callp->sy_narg,
					" error %d", u.u_error);
#endif
			rp->r_o0 = u.u_error;
			rp->r_psr |= PSR_C;	/* set carry bit */
		} else {
#ifdef SYSCALLTRACE
			if (syscalltrace & 4)
				prsct(code, callp->sy_narg,
					" returns 0x%x", u.u_r.r_val1);
#endif
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
		p->p_pri = p->p_usrpri;
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
		(void) spl0();
	} else {
		p->p_pri = p->p_usrpri;
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

	u.u_error = 0;


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
	if (syscalltrace & 8)
		prsct(code, callp->sy_narg, " ...", 0);
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

	(void) setjmp(&l);
	traceback((caddr_t)l.val[1]);
}

#ifdef TRAPWINDOW
long trap_window[25];
#endif TRAPWINDOW

/*
 * Print out debugging info.
 */
showregs(type, rp, addr, mmu_fsr, rw)
	register unsigned type;
	register struct regs *rp;
	addr_t addr;
	u_int mmu_fsr;
	enum seg_rw rw;
{
	int s;
	extern u_int mmu_getctx();

	trigger_logan();
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
		struct pte pte;

		mmu_getpte(addr, &pte);
		printf("%s %s fault at addr=0x%x, pme=0x%x\n",
			(USERMODE(rp->r_psr)? "user": "kernel"),
			(rw == S_WRITE? "write": "read"),
			addr, *(u_int *)&pte);
		mmu_print_sfsr(mmu_fsr);
	} else if (addr) {
		printf("addr=0x%x\n", addr);
	}
	printf("rp=0x%x, pc=0x%x, sp=0x%x, psr=0x%x, context=0x%x\n",
	    rp, rp->r_pc, rp->r_sp, rp->r_psr, mmu_getctx());
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
#ifdef	PROM_PARANOIA
	mpprom_eprintf("PROM_PARANOIA: inside showregs()\n");
#endif	PROM_PARANOIA
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
#ifdef	MULTIPROCESSOR
		klock_enter();
#endif
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
		(void) spl0();
	}

#if 0
	/* XXX - need to do something about this */
	if (syst_flag && u.u_prof.pr_scale) {
		register int ticks;
		struct timeval *tv = &u.u_ru.ru_stime;

#ifdef	MULTIPROCESSOR
		klock_enter();
#endif
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
                if ((*val = fuword((caddr_t)&rw[reg - 16])) == -1) {
                        if (fubyte((caddr_t)&rw[reg - 16]) == -1) {
                                return (-1);
                        }
                }
        }
        return (0);
}
static int
putreg(data, rgs, rw, reg)
        u_int   data, *rgs, *rw, reg;
{
        if (reg == 0)
                return (0);
        if (reg < 16)
                rgs[reg] = data;
        else {
                if (suword((caddr_t)&rw[reg - 16], (int)data) != 0) {
                        return (-1);
                }
        }
        return (0);
}

#ifndef lint
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
#endif

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
	if (getreg(rgs, rw, rs1, &val) != 0)
		return (SIMU_FAULT);
	addr = val;

	/* check immediate bit and use immediate field or reg (rs2) */
	if (immflg) {
		register int imm;
		imm  = inst & 0x1fff;		/* mask out immediate field */
		imm <<= 19;			/* sign extend it */
		imm >>= 19;
		addr += imm;			/* compute address */
	} else {
		if (getreg(rgs, rw, rs2, &val) != 0)
			return (SIMU_FAULT);
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
			if (getreg(rgs, rw, rd, &val) != 0)
				return (SIMU_FAULT);
			data.i[0] = val;
			if (sz == 8) {
				if (getreg(rgs, rw, rd+1, &val) != 0)
					return (SIMU_FAULT);
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
				return (SIMU_FAULT);
		} else {
			if (copyout((caddr_t)&data.i[0], (caddr_t)addr,
			    (u_int)sz) == -1)
				return (SIMU_FAULT);
		}
	} else {				/* load */
		if (sz == 2) {
			if (copyin((caddr_t)addr, (caddr_t)&data.s[1],
			    (u_int)sz) == -1)
				return (SIMU_FAULT);
			/* if signed and the sign bit is set extend it */
			if (((inst >> 22) & 1) && ((data.s[1] >> 15) & 1))
				data.s[0] = -1;	/* extend sign bit */
			else
				data.s[0] = 0;	/* clear upper 16 bits */
		} else
			if (copyin((caddr_t)addr, (caddr_t)&data.i[0],
			    (u_int)sz) == -1)
				return (SIMU_FAULT);

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
				return (SIMU_FAULT);
			if (sz == 8)
				if (putreg(data.i[1], rgs, rw, rd+1) == -1)
					return (SIMU_FAULT);
		}
	}
	return (SIMU_SUCCESS);
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
	 * check for the unimplemented iflush instruction
	 * the iflush support is only for th ROSS CPU. Instead of doing a 
	 * NOP, the ROSS does a illegal instruction trap. Just ignore
	 * it here and return
	 * The sun4s/sun4cs do a nop
	 * Vikings implement the iflush instruction
	 */
	if ((inst & 0xc1f80000) == 0x81d80000) {
		return (SIMU_SUCCESS);
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
			rp->r_y = dest[1];	/* put high mul in y */
		return (SIMU_SUCCESS);
	}

	/*
	 * Otherwise, we can't simulate instruction, its illegal.
	 */
	return (SIMU_ILLEGAL);
}

/*
 * fix_addr(): used by viking bug workaround. It is here because of getreg
 * Picks apart the instruction and computes the effective address used by
 * the instruction.
 */

addr_t 
fix_addr (rp, mfsr) 
struct regs  *rp;
register u_int mfsr;
{
	
	register u_int inst;
	u_int rs1, rs2;
	register u_int *rgs;		/* pointer to struct regs */
	register u_int *rw;		/* pointer to frame */
	register u_int err;

	/* make sure the fix_addr is being done for protection violation
	 * or privilege violations only
	 * All other cases we don't fix up the address
	 */

	err = (mfsr&MMU_SFSR_FT_MASK); 
	if ((err != MMU_SFSR_FT_PROT) && (err != MMU_SFSR_FT_PRIV)) 
		return((addr_t) -1);

	/* Our case, fix it */
	if ((inst = fuword((caddr_t)rp->r_pc)) == -1){
	     return ((addr_t)-1);
	}

	/* memory operation lds, sts, swap, or ldstub 
	 * The MSB 2 bits must be 3, and bits 5:4 out of op3(0:5) 
	 * must be 0
	 */ 
	if (((inst >> 30) !=  0x3) || (((inst >> 22) & 0x3) != 0))
		return ((addr_t)-1);
	
	rgs = (u_int *)&rp->r_y;	/* globals and outs */
	rw = (u_int *)rp->r_sp;		/* ins and locals */
	
	/* generate first operand rs1 */
	if (getreg(rgs, rw, (inst >> 14) & 0x1f, &rs1) != 0)
	     return ((addr_t)-1);
	/* check immediate bit and use immediate field or reg (rs2) */
	if ((inst >> 13) & 1) {
	     rs2  = inst & 0x1fff;	/* mask out immediate field */
	     rs2 <<= 19;		/* sign extend it */
	     rs2 >>= 19;
	} else  {
	     if (getreg(rgs, rw, inst & 0x1f, &rs2) != 0)
		  return ((addr_t)-1);
	}

	return((addr_t)rs1+rs2);
   }
