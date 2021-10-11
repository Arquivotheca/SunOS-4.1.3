/*	@(#)sys_process.c 1.1 92/07/30 SMI; from UCB 5.10 83/07/01	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/vm.h>
#include <sys/buf.h>
#include <sys/acct.h>
#include <sys/uio.h>
#include <sys/ptrace.h>
#include <sys/mman.h>

#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>

#include <vm/hat.h>
#include <vm/as.h>

#include <machine/seg_kmem.h>

/*
 * Priority for tracing
 */
#define	IPCPRI	PZERO

#ifdef vax
#define	NIPCREG 16
#endif
#ifdef mc68000
#define	NIPCREG 17
#endif
#ifdef sparc
#define	NIPCREG 17
#endif
#ifdef i386
#define	NIPCREG 8
#endif

int ipcreg[NIPCREG] =
#ifdef vax
	{R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, AP, FP, SP, PC};
#endif
#ifdef mc68000
	{R0, R1, R2, R3, R4, R5, R6, R7, AR0, AR1, AR2, AR3, AR4,
	AR5, AR6, AR7, PC};
#endif
#ifdef sparc
	{PC, nPC, G1, G2, G3, G4, G5, G6, G7, O0, O1, O2, O3, O4, O5, O6, O7};
#endif
#ifdef i386
	{EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI};	/* ZZZ */
#endif

#define	PHYSOFF(p, o) \
	((physadr)(p)+((o)/sizeof (((physadr)0)->r[0])))

struct ipc ipc;

/*
 * sys-trace system call.
 */
ptrace()
{
	register struct proc *p;
	register struct a {
		enum ptracereq req;
		int	pid;
		caddr_t	addr;
		int	data;
		caddr_t addr2;
	} *uap;

	uap = (struct a *)u.u_ap;
	if (uap->req == PTRACE_TRACEME) {
		u.u_procp->p_flag |= STRC;
		u.u_procp->p_tptr = u.u_procp->p_pptr;
		u.u_procp->p_pptr->p_flag |= STRACNG;
		return;
	}
	p = pfind(uap->pid);
	if (p == NULL || p->p_as == NULL) {
		u.u_error = ESRCH;
		return;
	}
	if (uap->req == PTRACE_ATTACH) {
		/*
		 * Can only attach if:
		 *	process is not already being traced, and
		 *	process does not have any privileges beyond
		 *	what our effective privileges already imply
		 *	(i.e., our effective [ug]id matches his real
		 *	and saved [ug]ids, or we're superuser).
		 */
		if ((p->p_flag & STRC) ||
		    ((u.u_uid != p->p_suid || u.u_uid != p->p_uid ||
		    u.u_gid != p->p_sgid || u.u_gid != p->p_cred->cr_rgid) &&
		    !suser())) {
			u.u_error = EPERM;
			return;
		}
		/*
		 * Tell him we're tracing him and stop him where he is.
		 */
		p->p_flag |= STRC;
		p->p_tptr = u.u_procp;
		/* if the traced process already stop, wake it up first */
		if (p->p_stat == SSTOP) {
			int s;

			psignal(p, SIGCONT);
			/*
			 * We need to force the process to sleep because
			 * psignal() ignores all signals while p_flag = STRC.
			 */
			s = splhigh();
			if (p->p_stat == SSTOP)
				p->p_stat = SSLEEP;
			(void) splx(s);
		}
		/* clear any pending STOP or CONT signal */
		if (p->p_cursig == SIGSTOP || p->p_cursig == SIGCONT)
			p->p_cursig = 0;
		psignal(p, SIGSTOP);
		u.u_procp->p_flag |= STRACNG;
		return;
	}
	if (p->p_tptr != u.u_procp || !(p->p_flag & STRC)) {
		u.u_error = ESRCH;
		return;
	}
	if (uap->req == PTRACE_DETACH && p->p_stat != SSTOP) {
		/* If not stopped, ignore addr and sig */
		p->p_tptr = 0;
		p->p_flag &= ~(STRC | STRCSYS);
		return;
	}
	if (p->p_stat != SSTOP) {
		u.u_error = ESRCH;
		return;
	}
#ifdef sparc
	/*
	 * For sparc, the pc must be aligned to a four byte boundary.
	 * Special case: addr == 1, continue where we were.
	 */
	if ((uap->req == PTRACE_CONT) && ((int)uap->addr & 0x3) &&
	    ((int)uap->addr != 1)) {
		u.u_error = EINVAL;
		return;
	}
#endif sparc
	while (ipc.ip_lock)
		(void) sleep((caddr_t)&ipc, IPCPRI);
	ipc.ip_lock = p->p_pid;
	ipc.ip_error = 0;
	ipc.ip_data = uap->data;
	ipc.ip_addr = uap->addr;
	ipc.ip_req = uap->req;
	ipc.ip_addr2 = uap->addr2;
	p->p_flag &= ~SWTED;
	u.u_r.r_val1 = 0;

	switch (uap->req) {
	case PTRACE_GETREGS:
		runchild(p);
		if (copyout((caddr_t)&ipc.ip_regs, ipc.ip_addr,
		    sizeof (ipc.ip_regs)) != 0) {
			ipc.ip_error = 1;
		}
		break;

	case PTRACE_SETREGS:
		if (copyin(ipc.ip_addr, (caddr_t)&ipc.ip_regs,
		    sizeof (ipc.ip_regs)) != 0) {
			ipc.ip_error = 1;
		}
#ifdef sparc
		if ((ipc.ip_regs.r_pc & 03) || (ipc.ip_regs.r_npc & 03)) {
			ipc.ip_error = 1;
			break;
		}
#endif sparc
		runchild(p);
		break;

#ifdef sun
#ifdef FPU
	case PTRACE_GETFPREGS:
	case PTRACE_SETFPREGS:
	case PTRACE_GETFPAREGS:
	case PTRACE_SETFPAREGS:
		fpu_ptrace(uap->req, p, &ipc);
		break;
#endif FPU
#endif sun

	case PTRACE_READDATA:
	case PTRACE_READTEXT:
		ipc.ip_bigbuf = (caddr_t)new_kmem_alloc(CLBYTES, KMEM_SLEEP);
		while (ipc.ip_data > 0) {
			ipc.ip_req = uap->req;
			ipc.ip_nbytes = MIN(ipc.ip_data, CLBYTES);
			runchild(p);
			if (copyout(ipc.ip_bigbuf, ipc.ip_addr2,
			    (u_int)ipc.ip_nbytes) != 0) {
				ipc.ip_error = 1;
				break;
			}
			ipc.ip_addr += CLBYTES;
			ipc.ip_data -= CLBYTES;
			ipc.ip_addr2 += CLBYTES;
		}
		kmem_free(ipc.ip_bigbuf, CLBYTES);
		ipc.ip_bigbuf = NULL;
		break;

	case PTRACE_WRITEDATA:
	case PTRACE_WRITETEXT:
		ipc.ip_bigbuf = new_kmem_alloc(CLBYTES, KMEM_SLEEP);
		while (ipc.ip_data > 0) {
			ipc.ip_req = uap->req;
			ipc.ip_nbytes = MIN(ipc.ip_data, CLBYTES);
			if (copyin(ipc.ip_addr2, ipc.ip_bigbuf,
			    (u_int)ipc.ip_nbytes) != 0) {
				ipc.ip_error = 1;
				break;
			}
			runchild(p);
			ipc.ip_addr += CLBYTES;
			ipc.ip_data -= CLBYTES;
		}
		kmem_free(ipc.ip_bigbuf, CLBYTES);
		ipc.ip_bigbuf = NULL;
		break;

	case PTRACE_DUMPCORE: {
		struct vattr vattr;

		vattr_null(&vattr);
		vattr.va_type = VREG;
		vattr.va_mode = 0644;
		u.u_error =
		    vn_create(ipc.ip_addr, UIO_USERSPACE, &vattr, NONEXCL,
		    VWRITE, &ipc.ip_vp);
		if (u.u_error) {
			ipc.ip_error = 1;
			break;
		}
		if (vattr.va_nlink != 1) {
			u.u_error = EFAULT;
			VN_RELE(ipc.ip_vp);
			break;
		}
		runchild(p);
		}
		break;

	default:
		runchild(p);
		u.u_r.r_val1 = ipc.ip_data;
		break;
	}

	if (ipc.ip_error != 0)
		u.u_error = EIO;

	ipc.ip_lock = 0;
	wakeup((caddr_t)&ipc);
}

/*
 * Set the child as runnable and go to sleep waiting for him
 * to do his part.
 */
runchild(p)
struct proc *p;
{

	while ((int)ipc.ip_req > (int)PTRACE_CHILDDONE) {
		if (p->p_stat == SSTOP)
			setrun(p);
		(void) sleep((caddr_t)&ipc, IPCPRI);
	}
}

/*
 * Code that the child process
 * executes to implement the command
 * of the parent process in tracing.
 */
procxmt()
{
	register enum ptracereq req;
	register int i, c;
	register int *p;
	register char *cp;
	register caddr_t a;
	register caddr_t addr;
	int chgprot;
	struct as *as = u.u_procp->p_as;
#ifdef i386
	long debugr7;
#endif

	if (ipc.ip_lock != u.u_procp->p_pid)
		return (0);
	u.u_procp->p_slptime = 0;
	req = ipc.ip_req;
	ipc.ip_req = PTRACE_CHILDDONE;
	addr = ipc.ip_addr;
	switch (req) {

	/* read user I */
	case PTRACE_PEEKTEXT:
		if (!useracc(addr, NBPW, B_READ))
			goto error;
		ipc.ip_data = fuiword(addr);
		break;

	/* read user D */
	case PTRACE_PEEKDATA:
		if (!useracc(addr, NBPW, B_READ))
			goto error;
		ipc.ip_data = fuword(addr);
		break;

	/* read u */
	case PTRACE_PEEKUSER:
		/*
		 * Make sure that the requested access is within this
		 * user's u-area and that it's properly accessible.
		 */
		i = (int)addr;
		if (i < 0 || i > (sizeof (struct user) - sizeof (int)))
			goto error;
		a = (caddr_t)PHYSOFF(uunix, i);
		if (as_checkprot(&kas, (addr_t)a, sizeof (int), PROT_READ) !=
		    A_SUCCESS)
			goto error;
		ipc.ip_data = *(int *)a;
		break;

	/* write user I */
	case PTRACE_POKETEXT:
		if ((i = suiword(addr, ipc.ip_data)) < 0) {
			/*
			 * Must change the permissions to allow writing.
			 * The act of writing should cause a copy-on-write
			 * for a ZMAGIC file mapped with PRIVATE text.
			 */
			if (as_setprot(as, addr, NBPW, PROT_ALL) == A_SUCCESS)
				i = suiword(addr, ipc.ip_data);
			/* XXX - this assumes the old permissions */
			(void) as_setprot(as, addr, NBPW,
			    PROT_ALL & ~PROT_WRITE);
		}
		if (i < 0)
			goto error;
		break;

	/* write user D */
	case PTRACE_POKEDATA:
		if (suword(addr, 0) < 0)
			goto error;
		(void) suword(addr, ipc.ip_data);
		break;

	/* write u */
	case PTRACE_POKEUSER:
		i = (int)addr;
		p = (int *)PHYSOFF(&u, i);
		for (i = 0; i < NIPCREG; i++)
			if (p == &u.u_ar0[ipcreg[i]])
				goto ok;
		if (p == &u.u_ar0[PS]) {
			ipc.ip_data =
			    (u.u_ar0[PS] & ~PSL_USERMASK) |
			    (ipc.ip_data & PSL_USERMASK);
			goto ok;
		}
		goto error;

	ok:
		/* make a final paranoid check. */
		if (as_checkprot(&kas, (addr_t)p, sizeof (int), PROT_WRITE) !=
		    A_SUCCESS)
			goto error;
		*p = ipc.ip_data;
		break;

	/* set signal and continue */
	/* one version causes a trace-trap */
	case PTRACE_SINGLESTEP:
	/* one version stops tracing */
#ifndef PSL_T
		goto	error;
#endif !PSL_T
	case PTRACE_DETACH:
	case PTRACE_CONT:
	case PTRACE_SYSCALL:
		if ((int)addr != 1) {
			u.u_ar0[PC] = (int)addr;
#ifdef sparc
			u.u_ar0[nPC] = (int)addr + 4;
#endif sparc
		}
		if ((unsigned)ipc.ip_data > NSIG)
			goto error;
		u.u_procp->p_cursig = ipc.ip_data;	/* see issig */
#ifdef PSL_T
		if (req == PTRACE_SINGLESTEP)
			u.u_ar0[PS] |= PSL_T;
		else
#endif PSL_T
		if (req == PTRACE_DETACH) {
			u.u_procp->p_flag &= ~STRC;
			u.u_procp->p_flag &= ~STRCSYS;
			u.u_procp->p_tptr = 0;
		} else if (req == PTRACE_SYSCALL)
			u.u_procp->p_flag |= STRCSYS;

		wakeup((caddr_t)&ipc);
		return (1);

	/* force exit */
	case PTRACE_KILL:
		u.u_procp->p_flag &= ~(STRC|STRCSYS);
		u.u_procp->p_tptr = 0;
		wakeup((caddr_t)&ipc);
		exit(u.u_procp->p_cursig);

	/* read registers - copy from the u area to the ipc buffer */
	case PTRACE_GETREGS:
		ipc.ip_regs = *(struct regs *) u.u_ar0;
		break;

	/* write registers - copy from the ipc buffer to the u area */
	case PTRACE_SETREGS:
		ipc.ip_regs.r_ps =
		    (u.u_ar0[PS] & ~PSL_USERMASK) |
		    (ipc.ip_regs.r_ps & PSL_USERMASK);
		*(struct regs *) u.u_ar0 = ipc.ip_regs;
		break;

	/* read data segment buffer */
	case PTRACE_READDATA:
		if (copyin(addr, ipc.ip_bigbuf, (u_int)ipc.ip_nbytes) != 0)
			goto error;
		break;

	/* write data segment buffer */
	case PTRACE_WRITEDATA:
		if (copyout(ipc.ip_bigbuf, addr, (u_int)ipc.ip_nbytes) != 0)
			goto error;
		break;

	/* read text segment buffer */
	case PTRACE_READTEXT:
		if (!useracc(addr, (u_int)ipc.ip_nbytes, B_READ))
			goto error;

		a = addr;			/* user address */
		c = 0;				/* total bytes moved */

		for (cp = ipc.ip_bigbuf; ((int)a & (NBPW - 1)) != 0;
		    c++, a++, cp++)
			*cp = fuibyte(a);

		for (p = (int *)cp; c < ipc.ip_nbytes - NBPW + 1;
		    c += NBPW, a += NBPW, p++)
			*p = fuiword(a);

		for (cp = (char *)p; c < ipc.ip_nbytes; c++, a++, cp++)
			*cp = fuibyte(a);

		break;

	/* write text segment buffer */
	case PTRACE_WRITETEXT:
		if (useracc(addr, (u_int)ipc.ip_nbytes, B_WRITE)) {
			chgprot = 0;		/* permissions ok */
		} else {
			/*
			 * Must change the permissions to allow writing.
			 * The act of writing should cause a copy-on-write
			 * for a ZMAGIC file mapped with PRIVATE text.
			 */
			if (as_setprot(as, addr, (u_int)ipc.ip_nbytes,
			    PROT_ALL) != A_SUCCESS)
				goto error;
			chgprot = 1;		/* changed permissions */
		}

		a = addr;			/* user address */
		c = 0;				/* total bytes moved */

		for (cp = ipc.ip_bigbuf; ((int)a & (NBPW - 1)) != 0;
		    c++, a++, cp++) {
			i = suibyte(a, *cp);
			if (i < 0)
				goto writetextout;
		}

		for (p = (int *)cp; c < ipc.ip_nbytes - NBPW + 1;
		    c += NBPW, a += NBPW, p++) {
			i = suiword(a, *p);
			if (i < 0)
				goto writetextout;
		}

		for (cp = (char *)p; c < ipc.ip_nbytes; c++, a++, cp++) {
			i = suibyte(a, *cp);
			if (i < 0)
				break;
		}

writetextout:
		if (chgprot) {
			/* XXX - this assumes the old permissions */
			(void) as_setprot(as, addr, (u_int)ipc.ip_nbytes,
			    PROT_ALL & ~PROT_WRITE);
		}
		if (i < 0)
			goto error;
		break;

#ifdef sun
#ifdef FPU
	case PTRACE_GETFPREGS:
	case PTRACE_SETFPREGS:
	case PTRACE_GETFPAREGS:
	case PTRACE_SETFPAREGS:
		if (-1 == fpu_procxmt(req, &ipc))
			goto error;
		break;
#endif FPU
#endif sun

#ifdef	i386
#define	BPWRITE		1
#define	BPACCESS	3
	/*
	 * Set write and access breakpoints.  Also sets necessary bits
	 * in DR7; data is the breakpoint number, and addr2 is the
	 * length of the operand.
	 */
	case PTRACE_SETWRBKPT:
	case PTRACE_SETACBKPT:
		if ((i = (int)ipc.ip_addr2) != 1 && i != 2 && i != 4) {
			ipc.ip_error = 1;
			break;
		}
		switch (ipc.ip_data) {
		case 0:
			_wdr0(ipc.ip_addr);
			break;
		case 1:
			_wdr1(ipc.ip_addr);
			break;
		case 2:
			_wdr2(ipc.ip_addr);
			break;
		case 3:
			_wdr3(ipc.ip_addr);
			break;
		default:
			ipc.ip_error = 1;
			break;
		}
		if (ipc.ip_error)
			break;
		debugr7 = _dr7();
		i = ipc.ip_data;
		debugr7 &= ~(0xf << (16 + i * 4) | 3 << (i * 2));
		if (req == PTRACE_SETWRBKPT)
			c = BPWRITE;
		else
			c = BPACCESS;
		debugr7 |= c << (16 + i * 4) |
		    ((int)ipc.ip_addr2 - 1) << (18 + i * 4) |
		    1 << (i * 2) | 0x100;
		_wdr7(debugr7);
		break;

	/* Clear debug register 7 */
	case PTRACE_CLRDR7:
		_wdr7(0);
		break;
#endif	i386

	/* Dump core image to file */
	case PTRACE_DUMPCORE:
		if (core(ipc.ip_vp))
			break;

	/* get u.u_code */
	case PTRACE_GETUCODE:
		ipc.ip_data = u.u_code;
		break;

	default:
	error:
		ipc.ip_error = 1;
	}
	wakeup((caddr_t)&ipc);
	return (0);
}
