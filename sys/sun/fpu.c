/*	@(#)fpu.c 1.1 92/07/30 SMI; from UCB 4.3 83/06/14	*/

#include <sys/types.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/buserr.h>
#include <machine/trap.h>
#include <sun/fault.h>
#include <m68k/frame.h>
#include <sundev/fpareg.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/core.h>
#include "fpa.h"
#include <sys/ptrace.h>

#ifdef FPU
#if NFPA > 0
enum fpa_state	fpa_state = FPA_SINGLE_USER;
#endif NFPA > 0

fpu_fork_context(p, pnfp, pnew_context)
	struct proc *p;
	struct file **pnfp;
	int *pnew_context;
{

#if NFPA > 0
	if (u.u_pcb.u_fpa_flags)
		if (fpa_fork_context(p, pnfp, pnew_context) < 0) {
			/* no FPA context, inode, or file, error quit */
			u.u_error = EAGAIN;
			return (0);
		}
#endif NFPA > 0
	return (1);
}

fpu_newproc(nfp, new_context, childu)
	struct file *nfp;
	int new_context;
	struct user *childu;
{       

#if NFPA > 0
#define	u_fpas_state u_fpa_status.fpas_state
	if (childu->u_pcb.u_fpa_flags) {

		/*
		 * If the parent had the FPA open, but the FPA is not
		 * initialized, the child does not inherit FPA access
		 */
		if (fpa_state == FPA_SINGLE_USER)
		{
			childu->u_ofile[u.u_pcb.u_fpa_flags & U_FPA_FDBITS] = NULL;
			childu->u_pcb.u_fpa_flags = 0;
			return;
		}
		
		/*
		 * All FPA registers including the pipe are in
		 * child u and have been restored to the FPA board.
		 * Set up childu->u_ofile[] to the opened FPA file
		 * and childu->u_fpa_state to the new FPA context
		 * number for the child process.
		 */
		fpa_save(childu); /* make sure pipe is still ready */
		childu->u_ofile[childu->u_pcb.u_fpa_flags & U_FPA_FDBITS] = nfp;
		childu->u_pcb.u_fpas_state =
		    (childu->u_pcb.u_fpas_state & FPA_PBITS) | (long)new_context;
		fpa_dorestore(childu); /* state reg is written here */
	}
#endif NFPA > 0
}

fpu_core(corep)
	struct core *corep;
{

	u.u_pcb.u_fp_status.fps_flags = EXT_FPS_FLAGS(&u.u_pcb.u_fp_istate);
	corep->c_fpu.f_fpstatus = u.u_pcb.u_fp_status;

#if NFPA > 0
	/*
	 * Dump FPA regs only when u.u_fpa_flags is nonzero and
	 * FPA_LOAD_BIT is off.
	 * The reason is that if FPA_LOAD_BIT is on, it means that there
	 * is no microcode in FPA board, we cannot access FPA data regs.
	 */
	if ((corep->c_fpu.f_fparegs.fpar_flags = u.u_pcb.u_fpa_flags) &&
	    !(fpa->fp_state & FPA_LOAD_BIT)) {
		/* loop until FPA pipe become stable */
		CDELAY(fpa->fp_pipe_status & FPA_STABLE, 300);
		if (fpa->fp_pipe_status & FPA_STABLE) {
			corep->c_fpu.f_fparegs.fpar_status =
			    *((struct fpa_status *) &fpa->fp_state);
			fpa->fp_clear_pipe = 0; /* clear pipe to read data */
			lcopy((char *)fpa->fp_data,
			    (char *)corep->c_fpu.f_fparegs.fpar_data,
			    sizeof (fpa->fp_data)/4);
		} else {
			fpa_shutdown();
			printf("FPA not stable in core(), FPA is shutdown!\n");
		}
	}
#endif NFPA > 0
}

fpu_setregs()
{
	extern struct proc *fpprocp;

	/*
	 * Clear any external and internal fpp state.
	 * If this process was the last one to load its
	 * external fpp state, erase that fact also.
	 */
	bzero((caddr_t)&u.u_pcb.u_fp_status, sizeof (u.u_pcb.u_fp_status));
	bzero((caddr_t)&u.u_pcb.u_fp_istate, sizeof (u.u_pcb.u_fp_istate));
	if (u.u_procp == fpprocp)
		fpprocp = (struct proc *)0;
}

#ifdef sun3x
fpu_berror(be, locregs, fmt, mmube)
#else
fpu_berror(be, locregs, fmt)
#endif
	u_char be;
	struct stkfmt fmt;
	register struct regs *locregs;
#ifdef sun3x
	u_short	mmube;
#endif
{

#if NFPA > 0
	/*
	 * If enable register is not turned on, panic.
	 * In case of FPA bus error in the kernel mode, shutdown
	 * FPA, instead of panicing the system.
	 */
	if (be & BE_FPAENA)
		panic("FPA not enabled");
	if (be & BE_FPABERR) {
#ifdef sun3x
		showregs("FPA KERNEL BUS ERROR", fmt.f_vector, locregs,
		    &fmt, be, mmube);
#else
		showregs("FPA KERNEL BUS ERROR", fmt.f_vector, locregs,
		    &fmt, be);
#endif
		traceback((long)locregs->r_areg[6],
		    (long)locregs->r_sp);
		printf("FPA BUS ERROR: IERR == %x\n", fpa->fp_ierr);
		fpa_shutdown();
		return (SIGSEGV);
	}
#endif NFPA > 0
	return (0);
}

fpu_user_berror(be)
	u_char be;
{

#if NFPA > 0
	if (u.u_pcb.u_fpa_flags && (be & (BE_FPAENA | BE_FPABERR))) {
		u.u_code = (be & BE_FPAENA) ? FPE_FPA_ENABLE : FPE_FPA_ERROR;
		return (SIGFPE);
	}
#endif NFPA > 0
	return (0);
}

fpu_ptrace(req, p, ipc)
	enum ptracereq req;
	struct proc *p;
	struct ipc *ipc;
{
	switch (req) {

	case PTRACE_GETFPREGS:
		runchild(p);
		if (copyout((caddr_t)&(ipc->ip_fpu.f_fpstatus), ipc->ip_addr,
		    sizeof (struct fp_status)) != 0) {
			ipc->ip_error = 1;
		}
		break;

	case PTRACE_SETFPREGS:
		if (copyin(ipc->ip_addr, (caddr_t)&(ipc->ip_fpu.f_fpstatus),
		    sizeof (struct fp_status)) != 0) {
			ipc->ip_error = 1;
		}
		runchild(p);
		break;
#if NFPA > 0
	case PTRACE_GETFPAREGS:
		runchild(p);
		if (copyout((caddr_t)&ipc->ip_fpu.f_fparegs, ipc->ip_addr,
		    sizeof (struct fpa_regs)) != 0) {
			ipc->ip_error = 1;
		}
		break;

	case PTRACE_SETFPAREGS:
		if (copyin(ipc->ip_addr, (caddr_t)&ipc->ip_fpu.f_fparegs,
		    sizeof (struct fpa_regs)) != 0) {
			ipc->ip_error = 1;
		}
		runchild(p);
		break;
#endif NFPA > 0
	}
}

/*ARGSUSED*/
fpu_procxmt(req, ipc)
	enum ptracereq req;
	struct ipc *ipc;
{
	extern short fppstate;

	switch (req) {
	/*
	 * Read floating point registers -
	 * Copy from the u area to the ipc buffer,
	 * after setting the fps_flags
	 * from the internal fpp info.
	 */
	case PTRACE_GETFPREGS:
		if (fppstate == 0) {
			ipc->ip_error = 1;
		} else {
			u.u_pcb.u_fp_status.fps_flags =
			    EXT_FPS_FLAGS(&u.u_pcb.u_fp_istate);
			ipc->ip_fpu.f_fpstatus = u.u_pcb.u_fp_status;
		}
		break;

	/*
	 * Write floating point registers -
	 * Copy from the ipc buffer to the u area,
	 * and set u.u_code from the code in fp_status,
	 * then initialize the fpp registers.
	 */
	case PTRACE_SETFPREGS:
		if (fppstate == 0) {
			ipc->ip_error = 1;
		} else {
			u.u_pcb.u_fp_status = ipc->ip_fpu.f_fpstatus;
			fsave(&u.u_pcb.u_fp_istate);
			setfppregs();
			frestore(&u.u_pcb.u_fp_istate);
		}
		break;
#if NFPA > 0
	/* Read FPA registers from u area and FPA board to ipc buffer. */
	case PTRACE_GETFPAREGS:
		if ((!u.u_pcb.u_fpa_flags) || (fpa->fp_pipe_status & FPA_WAIT2))
			return (-1);
		ipc->ip_fpa_flags = u.u_pcb.u_fpa_flags;
		fpa_save(&u); /* force a save to u area */
		ipc->ip_fpa_status = u.u_pcb.u_fpa_status;
		fpa->fp_clear_pipe = 0; /* clear FPA pipe to read data regs */
		lcopy((char *)fpa->fp_data, (char *)ipc->ip_fpa_data,
			sizeof (fpa->fp_data)/4); /* copy from fpa */
		fpa_dorestore(&u); /* restore u area to FPA */
		break;

	/*
	 * Write FPA registers from ipc buffer to the U area and FPA
	 * board.  U.u_fpa_flags, FPA STATE register, and WSTATUS register
	 * are protected from being written.
	 * Fpa_restore() is needed, o/w u.u_fpa_status is erased by
	 * fpa context save during the coming context switch.
	 */
	case PTRACE_SETFPAREGS:
		if ((!u.u_pcb.u_fpa_flags) || (fpa->fp_pipe_status & FPA_WAIT2))
			return (-1);
		/* STATE reg is protected from being modified */
		ipc->ip_fpa_state = u.u_pcb.u_fpa_status.fpas_state;
		u.u_pcb.u_fpa_status = ipc->ip_fpa_status;
		fpa->fp_clear_pipe = 0; /* clear FPA pipe to read data regs */
		lcopy((char *)ipc->ip_fpa_data, (char *)fpa->fp_data,
			sizeof (fpa->fp_data)/4); /* copy to fpa */
		fpa_dorestore(&u);
		break;
#endif NFPA > 0
	}
	return (0);
}

fpu_ofile()
{

#if NFPA > 0
	/*
	 * F_count associated with /dev/fpa should not be
	 * incremented in the above for loop.
	 */
	if (u.u_pcb.u_fpa_flags)
		u.u_ofile[u.u_pcb.u_fpa_flags & U_FPA_FDBITS]->f_count--;
#endif NFPA > 0
}
#endif FPU
