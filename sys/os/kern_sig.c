/*	@(#)kern_sig.c 1.1 92/07/30 SMI; from UCB 5.23 83/06/24	*/

#include <machine/pte.h>
#include <machine/psl.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vm.h>
#include <sys/acct.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/core.h>
#include <sys/audit.h>
#include <sys/syslog.h>
#ifdef	sun3x
#include <vm/as.h>
#endif	sun3x

#ifdef	MULTIPROCESSOR
#include "percpu.h"

extern	int	nmod;
#endif	MULTIPROCESSOR

#define	stopsigmask	(sigmask(SIGSTOP)|sigmask(SIGTSTP)| \
			sigmask(SIGTTIN)|sigmask(SIGTTOU))

/*
 * Generalized interface signal handler.
 */
sigvec()
{
	register struct a {
		int	signo;
		struct	sigvec *nsv;
		struct	sigvec *osv;
	} *uap = (struct a  *)u.u_ap;
	struct sigvec vec;
	register struct sigvec *sv;
	register int sig;
	int bit;

	sig = uap->signo;
	if (sig <= 0 || sig >= NSIG || 
	    ((sig == SIGKILL || sig == SIGSTOP) && uap->nsv)) {
		u.u_error = EINVAL;
		return;
	}
	sv = &vec;
	if (uap->osv) {
		sv->sv_handler = u.u_signal[sig];
		sv->sv_mask = u.u_sigmask[sig];
		bit = sigmask(sig);
		sv->sv_flags = 0;
		if ((u.u_sigonstack & bit) != 0)
			sv->sv_flags |= SV_ONSTACK;
		if ((u.u_sigintr & bit) != 0)
			sv->sv_flags |= SV_INTERRUPT;
		if ((u.u_sigreset & bit) != 0)
			sv->sv_flags |= SV_RESETHAND;
		if (sig == SIGCLD && u.u_procp->p_flag & SNOCLDSTOP)
			sv->sv_flags |= SV_NOCLDSTOP;
		u.u_error =
		    copyout((caddr_t)sv, (caddr_t)uap->osv, sizeof (vec));
		if (u.u_error)
			return;
	}
	if (uap->nsv) {
		u.u_error =
		    copyin((caddr_t)uap->nsv, (caddr_t)sv, sizeof (vec));
		if (u.u_error)
			return;
		setsigvec(sig, sv);
	}
}

setsigvec(sig, sv)
	int sig;
	register struct sigvec *sv;
{
	register struct proc *p;
	register int bit;

	bit = sigmask(sig);
	p = u.u_procp;
	/*
	 * Change setting atomically.
	 */
	(void) splhigh();
	u.u_signal[sig] = sv->sv_handler;
	u.u_sigmask[sig] = sv->sv_mask &~ cantmask;
	if (sv->sv_flags & SV_INTERRUPT)
		u.u_sigintr |= bit;
	else
		u.u_sigintr &= ~bit;
	if (sv->sv_flags & SV_ONSTACK)
		u.u_sigonstack |= bit;
	else
		u.u_sigonstack &= ~bit;
	if (sv->sv_flags & SV_RESETHAND)
		u.u_sigreset |= bit;
	else
		u.u_sigreset &= ~bit;
	if (sig == SIGCHLD) {
		if (sv->sv_flags & SV_NOCLDSTOP)
			u.u_procp->p_flag |= SNOCLDSTOP;
		else
			u.u_procp->p_flag &= ~SNOCLDSTOP;
	}
	/* 
         * Pending SIGCHLD is ignored if the handler is set to default  
         * because the default behavior is to ignore the signal 
         */
        if (sv->sv_handler == SIG_IGN ||
            (sv->sv_handler == SIG_DFL && sig == SIGCHLD)) {
		p->p_sig &= ~bit;		/* never to be seen again */
		p->p_sigignore |= bit;
		p->p_sigcatch &= ~bit;
	} else {
		p->p_sigignore &= ~bit;
		if (sv->sv_handler == SIG_DFL)
			p->p_sigcatch &= ~bit;
		else
			p->p_sigcatch |= bit;
	}
	(void) spl0();
}

/*
 * posix syscall to return list of pending & blocked signals.
 */
sigpending()
{
	struct a {
		int	*mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
	int mask;

	mask = p->p_sig & p->p_sigmask;
	u.u_error = copyout((caddr_t)&mask,
	    (caddr_t)uap->mask, sizeof (uap->mask));
}

sigblock()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	(void) splhigh();
	u.u_r.r_val1 = p->p_sigmask;
	p->p_sigmask |= uap->mask &~ cantmask;
	(void) spl0();
}

sigsetmask()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	(void) splhigh();
	u.u_r.r_val1 = p->p_sigmask;
	p->p_sigmask = uap->mask &~ cantmask;
	(void) spl0();
}

sigpause()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	/*
	 * When returning from sigpause, we want
	 * the old mask to be restored after the
	 * signal handler has finished.  Thus, we
	 * save it here and mark the proc structure
	 * to indicate this (should be in u.).
	 */
	u.u_oldmask = p->p_sigmask;
	p->p_flag |= SOMASK;
	p->p_sigmask = uap->mask &~ cantmask;
	for (;;)
		(void) sleep((caddr_t)&u, PSLEP);
	/*NOTREACHED*/
}

sigstack()
{
	register struct a {
		struct	sigstack *nss;
		struct	sigstack *oss;
	} *uap = (struct a *)u.u_ap;
	struct sigstack ss;

	if (uap->oss) {
		u.u_error = copyout((caddr_t)&u.u_sigstack, (caddr_t)uap->oss,
		    sizeof (struct sigstack));
		if (u.u_error)
			return;
	}
	if (uap->nss) {
		u.u_error =
		    copyin((caddr_t)uap->nss, (caddr_t)&ss, sizeof (ss));
		if (u.u_error == 0)
			u.u_sigstack = ss;
	}
}

kill()
{
	register struct a {
		int	pid;
		int	signo;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;
	register signo = uap->signo;

	if (signo < 0 || signo > NSIG) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->pid > 0) {
		/* kill single process */
		p = pfind(uap->pid);
		if (p == 0) {
			u.u_error = ESRCH;
			return;
		}
		if (!(u.u_uid == 0 ||
		    u.u_uid == p->p_uid ||
		    u.u_ruid == p->p_uid ||
		    u.u_uid == p->p_suid ||
		    u.u_ruid == p->p_suid ||
		    (signo == SIGCONT && u.u_procp->p_sessp == p->p_sessp)))
			u.u_error = EPERM;
		else if (signo)
			psignal(p, signo);
		return;
	}
	switch (uap->pid) {
	case -1:		/* broadcast signal */
		u.u_error = killpg1(signo, 0, 1);
		break;
	case 0:			/* signal own process group */
		u.u_error = killpg1(signo, 0, 0);
		break;
	default:		/* negative explicit process group */
		u.u_error = killpg1(signo, -uap->pid, 0);
		break;
	}
	return;
}

killpg()
{
	register struct a {
		int	pgrp;
		int	signo;
	} *uap = (struct a *)u.u_ap;

	if (uap->signo < 0 || uap->signo > NSIG) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = killpg1(uap->signo, uap->pgrp, 0);
}

/* KILL CODE SHOULDNT KNOW ABOUT PROCESS INTERNALS !?! */

killpg1(signo, pgrp, all)
	int signo, pgrp, all;
{
	register struct proc *p;
	int f, error = 0;

	if (!all && pgrp == 0) {
		/*
		 * Zero process id means send to my process group.
		 */
		pgrp = u.u_procp->p_pgrp;
		if (pgrp == 0)
			return (ESRCH);
	}
	for (f = 0, p = allproc; p != NULL; p = p->p_nxt) {
		/*
		 * If not sending to all processes, only send to processes
		 * in the specified process group.
		 * Don't send to "init" (child of process 0).
		 * Don't send to system processes.
		 * If sending to all processes, don't send to ourselves.
		 */
		if ((p->p_pgrp != pgrp && !all) || p->p_ppid == 0 ||
		    (p->p_flag&SSYS) || (all && p == u.u_procp))
			continue;
		if (!(u.u_uid == 0 ||
		    u.u_uid == p->p_uid ||
		    u.u_ruid == p->p_uid ||
		    u.u_uid == p->p_suid ||
		    u.u_ruid == p->p_suid ||
		    (signo == SIGCONT && u.u_procp->p_sessp == p->p_sessp))) {
			if (!all)
				error = EPERM;	/* XXX - SVID says ESRCH? */
			continue;
		}
		f++;
		if (signo)
			psignal(p, signo);
	}
	return (error ? error : (f == 0 ? ESRCH : 0));
}

/*
 * Send the specified signal to
 * all processes with 'pgrp' as
 * process group.
 */
gsignal(pgrp, sig)
	register int pgrp;
{
	register struct proc *p;

	if (pgrp == 0)
		return;
	for (p = pgfind(pgrp); p != NULL; p = pgnext(p))
		psignal(p, sig);
}

#ifdef	MULTIPROCESSOR
int
psignal_remote()
{
/*
 * force this cpu to drop through the AST handler so
 * we trap and process our signals. Thanks ...
 * (and don't forget the sanity check!!!!)
 */
	if (!noproc && (uunix != (struct user *)0)) {
		aston();
	}
}
#endif	MULTIPROCESSOR


/*
 * Send the specified signal to
 * the specified process.
 */
psignal(p, sig)
	register struct proc *p;
	register int sig;
{
	register int s;
	register void (*action)();
	int mask;

	if ((unsigned)sig >= NSIG)
		return;
	mask = sigmask(sig);

	/*
	 * If proc is traced, always give parent a chance.
	 */
	if (p->p_flag & STRC)
		action = SIG_DFL;
	else {
		/*
		 * SIGCONT can be now blocked/ignored due to POSIX.
		 * It always restarts a stopped process but the handler
		 * doesn't get called until it is unblocked.
		 */
		if (sig == SIGCONT)
			action = p->p_sigcatch & mask ? SIG_CATCH : SIG_DFL;
		/*
		 * If the signal is being ignored,
		 * then we forget about it immediately.
		 */
		else if (p->p_sigignore & mask)
			return;
		else if (p->p_sigmask & mask)
			action = SIG_HOLD;
		else if (p->p_sigcatch & mask)
			action = SIG_CATCH;
		else
			action = SIG_DFL;
	}
	if (sig) {
		p->p_sig |= mask;
		switch (sig) {

		case SIGTERM:
			if ((p->p_flag&STRC) || action != SIG_DFL)
				break;
			/* fall into ... */

		case SIGKILL:
			if (p->p_nice > NZERO)
				p->p_nice = NZERO;
			break;

		case SIGCONT:
			p->p_sig &= ~stopsigmask;
			break;

		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
		case SIGSTOP:
			p->p_sig &= ~sigmask(SIGCONT);
			break;
		}
	}
	/*
	 * Defer further processing for signals which are held.
	 */
	if (action == SIG_HOLD)
		return;
	s = splhigh();
	switch (p->p_stat) {

	case SSLEEP:
		/*
		 * If process is sleeping at negative priority
		 * we can't interrupt the sleep... the signal will
		 * be noticed when the process returns through
		 * trap() or syscall().
		 */
		if (p->p_pri <= PZERO)
			goto out;
		/*
		 * Process is sleeping and traced... make it runnable
		 * so it can discover the signal in issig() and stop
		 * for the parent.
		 */
		if (p->p_flag&STRC)
			goto run;
		switch (sig) {

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * These are the signals which by default
			 * stop a process.
			 */
			if (action != SIG_DFL)
				goto run;
			/*
			 * Orphans drop tty signals.
			 * This may clog the system if children aren't
			 * eventually killed.
			 */
			if (sig != SIGSTOP && (p->p_flag & SORPHAN)) {
				psignal(p, SIGKILL);
				p->p_sig &= ~mask;
				(void) splx(s);
				return;
			}
			/*
			 * If a child in vfork(), stopping could
			 * cause deadlock.
			 */
			if (p->p_flag&SVFORK)
				goto out;
			p->p_sig &= ~mask;
			p->p_cursig = sig;
			stop(p);
			goto out;

		case SIGIO:
		case SIGURG:
		case SIGCHLD:
		case SIGWINCH:
			/*
			 * These signals are special in that they
			 * don't get propagated... if the process
			 * isn't interested, forget it.
			 */
			if (action != SIG_DFL)
				goto run;
			p->p_sig &= ~mask;		/* take it away */
			goto out;

		default:
			/*
			 * All other signals cause the process to run
			 */
			goto run;
		}
		/*NOTREACHED*/

	case SSTOP:
		/*
		 * If traced process is already stopped,
		 * then no further action is necessary.
		 */
		if (p->p_flag&STRC)
			goto out;
		switch (sig) {

		case SIGKILL:
			/*
			 * Kill signal always sets processes running.
			 */
			goto run;

		case SIGCONT:
			/*
			 * If the process catches SIGCONT, let it handle
			 * the signal itself.  If it isn't waiting on
			 * an event, then it goes back to run state.
			 * Otherwise, process goes back to sleep state.
			 */
			if (action != SIG_DFL || p->p_wchan == 0)
				goto run;
			p->p_stat = SSLEEP;
			goto out;

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * Already stopped, don't need to stop again.
			 * (If we did the shell could get confused.)
			 */
			p->p_sig &= ~mask;		/* take it away */
			goto out;

		default:
			/*
			 * If process is sleeping interruptibly, then
			 * unstick it so that when it is continued
			 * it can look at the signal.
			 * But don't setrun the process as its not to
			 * be unstopped by the signal alone.
			 */
			if (p->p_wchan && p->p_pri > PZERO)
				unsleep(p);
			goto out;
		}
		/*NOTREACHED*/

	default:
		/*
		 * SRUN, SIDL, SZOMB do nothing with the signal,
		 * other than kicking ourselves if we are running.
		 * It will either never be noticed, or noticed very soon.
		 */
		if (p == u.u_procp && !noproc) {
#ifdef	vax
#include <vax/mtpr.h>
#endif	vax
			aston();
		}
#ifdef	MULTIPROCESSOR
		else if (p && (p != &idleproc)) {
			register int cix;
			extern int procset;

			for (cix = 0; cix < nmod; ++cix) {
				if ((procset & (1<<cix)) &&
				    (PerCPU[cix].masterprocp == p) &&
				    (!PerCPU[cix].noproc)) {
					xc_oncpu(0,0,0,0,cix,psignal_remote);
				}
			}
		}
#endif	MULTIPROCESSOR
		goto out;
	}
	/*NOTREACHED*/
run:
	/*
	 * Raise priority to at least PUSER.
	 */
	if (p->p_pri > PUSER)
		p->p_pri = PUSER;
	setrun(p);
out:
	(void) splx(s);
}

/*
 * Returns true if the current
 * process has a signal to process.
 * The signal to process is put in p_cursig.
 * This is asked at least once each time a process enters the
 * system (though this can usually be done without actually
 * calling issig by checking the pending signal masks.)
 * A signal does not do anything
 * directly to a process; it sets
 * a flag that asks the process to
 * do something to itself.
 */
issig(flag)
	int flag;
{
	register struct proc *p;
	register int sig;
	int sigbits, mask;

	p = u.u_procp;
	for (;;) {
		sigbits = p->p_sig &~ p->p_sigmask;
		if ((p->p_flag&STRC) == 0)
			sigbits &= ~p->p_sigignore;
		if (p->p_flag&SVFORK)
			sigbits &= ~stopsigmask;
		if (sigbits == 0)
			break;
		sig = ffs((long)sigbits);
		/*
		 * The following test fixes a dbx bug.
		 * The problem arises when a signal interrupts a slow system
		 * call.  If the following test were not done, dbx would catch
		 * the signal, but then issig would return 0 to sleep which
		 * would just continue to sleep.  But sometimes dbx has set the
		 * pc because it wants the child to do something.  This fix
		 * causes sleep to longjmp which eventually leads to issig
		 * being called from syscall.  After dbx handles the signal,
		 * syscall goes back to the user to resume the interrupted
		 * system call.  Since dbx has reset the pc, the user process
		 * then does what dbx wants.
		 *
		 * It's important to set p_cursig here so that procedures
		 * that are interrupted by the signal can check p_cursig
		 * against u.u_sigintr to determine whether they should
		 * restart the system call or not.  This must work even
		 * when we take the early return for the case described
		 * above.
		 */
		p->p_cursig = sig;
		if (p->p_flag&STRC && (p->p_flag&SVFORK) == 0 && flag)
			return (sig);

		mask = sigmask(sig);
		p->p_sig &= ~mask;		/* take the signal! */
		if (p->p_flag&STRC && (p->p_flag&SVFORK) == 0) {
			/*
			 * If traced, always stop, and stay
			 * stopped until released by the parent.
			 */
			do {
				stop(p);
				swtch();
#ifdef	sun3x
				hat_setup(p->p_as);
#endif	sun3x
			} while (!procxmt() && p->p_flag&STRC);

			/*
			 * If the traced bit got turned off,
			 * then put the signal taken above back into p_sig
			 * and go back up to the top to rescan signals.
			 * This ensures that p_sig* and u_signal are
			 * consistent.
			 */
			if ((p->p_flag&STRC) == 0) {
/*
				p->p_sig |= mask;
*/
				continue;
			}

			/*
			 * If parent wants us to take the signal,
			 * then it will leave it in p->p_cursig;
			 * otherwise we just look for signals again.
			 */
			sig = p->p_cursig;
			if (sig == 0)
				continue;

			/*
			 * If signal is being masked put it back
			 * into p_sig and look for other signals.
			 */
			mask = sigmask(sig);
			if (p->p_sigmask & mask) {
				p->p_sig |= mask;
				continue;
			}
		}
		switch ((int)u.u_signal[sig]) {

		case (int)SIG_DFL:
			/*
			 * Don't take default actions on system processes.
			 */
			if (p->p_ppid == 0)
				break;
			switch (sig) {

			case SIGTSTP:
			case SIGTTIN:
			case SIGTTOU:
				/*
				 * Orphans aren't allowed to stop
				 * on signals from the keyboard.
				 */
				if (p->p_flag & SORPHAN) {
					psignal(p, SIGKILL);
					continue;
				}
				/* fall into ... */

			case SIGSTOP:
				if (p->p_flag&STRC)
					continue;
				stop(p);
				swtch();
				continue;

			case SIGCONT:
			case SIGCHLD:
			case SIGURG:
			case SIGIO:
			case SIGWINCH:
				/*
				 * These signals are normally not
				 * sent if the action is the default.
				 */
				continue;		/* == ignore */

			default:
				goto send;
			}
			/*NOTREACHED*/

		case (int)SIG_HOLD:
		case (int)SIG_IGN:
			/*
			 * Masking above should prevent us
			 * ever trying to take action on a held
			 * or ignored signal, unless process is traced.
			 */
			if ((p->p_flag&STRC) == 0)
				printf("issig\n");
			continue;

		default:
			/*
			 * This signal has an action, let
			 * psig process it.
			 */
			goto send;
		}
		/*NOTREACHED*/
	}
	/*
	 * Didn't find a signal to send.
	 */
	p->p_cursig = 0;
	return (0);

send:
	/*
	 * Let psig process the signal.
	 */
	return (sig);
}

/*
 * Put the argument process into the stopped
 * state and notify the parent via wakeup and/or signal.
 */
stop(p)
	register struct proc *p;
{
	p->p_stat = SSTOP;
	p->p_flag &= ~SWTED;
	/*
	 * always do the wakeup.
	 */
	wakeup((caddr_t)p->p_tptr);
	/*
	 * If we are not traced and the parent wants to now about
	 * these things, send him a signal and wakeup the parent
	 */
	if (((p->p_flag & STRC) == 0) &&
	    ((p->p_pptr->p_flag & SNOCLDSTOP) == 0)) {	/* posix */
		psignal(p->p_pptr, SIGCHLD);
		wakeup((caddr_t)p->p_pptr);
	}

}

/*
 * Perform the action specified by
 * the current signal.
 * The usual sequence is:
 *	if (issig())
 *		psig();
 * The signal bit has already been cleared by issig,
 * and the current signal number stored in p->p_cursig.
 */
psig()
{
	register struct proc *p = u.u_procp;
	register int sig = p->p_cursig;
	int mask = sigmask(sig), returnmask;
	register void (*action)();


	if (sig == 0)
		panic("psig");
	action = u.u_signal[sig];
	if (action != SIG_DFL) {
		if (action == SIG_IGN)
			panic("psig action");
		if (p->p_sigmask & mask) {
			u.u_comm[MAXCOMLEN] = 0; /* just in case */
			log(LOG_WARNING,
			    "psig: \"%s\" signal %d was masked, put back.\n",
			    u.u_comm, sig);
			u.u_error = 0;
			p->p_cursig = 0;
			p->p_sig |= mask;
			return;
		}
		u.u_error = 0;
		/*
		 * Set the new mask value and also defer further
		 * occurences of this signal (unless we're simulating
		 * the old signal facilities).
		 *
		 * Special case: user has done a sigpause.  Here the
		 * current mask is not of interest, but rather the
		 * mask from before the sigpause is what we want restored
		 * after the signal processing is completed.
		 */
		(void) splhigh();
		if (u.u_sigreset & mask) {
			if (sig != SIGILL && sig != SIGTRAP) {
				u.u_signal[sig] = SIG_DFL;
				p->p_sigcatch &= ~mask;
			}
			mask = 0;
		}
		if (p->p_flag & SOMASK) {
			p->p_flag &= ~SOMASK;
			returnmask = u.u_oldmask;
		} else
			returnmask = p->p_sigmask;
		p->p_sigmask |= u.u_sigmask[sig] | mask;
		(void) spl0();
		u.u_ru.ru_nsignals++;
		sendsig(action, sig, returnmask);
		p->p_cursig = 0;
		return;
	}
	u.u_acflag |= AXSIG;
	switch (sig) {

	case SIGILL:
	case SIGIOT:
	case SIGBUS:
	case SIGQUIT:
	case SIGTRAP:
	case SIGEMT:
	case SIGFPE:
	case SIGSEGV:
	case SIGSYS:
		u.u_arg[0] = sig;
		if (core((struct vnode *)0))
			sig += 0200;
	}
	exit(sig);
}

/*
 * Create a core image on the file "core"
 * If you are looking for protection glitches,
 * there are probably a wealth of them here
 * when this occurs to a suid command.
 *
 * It writes a struct core
 * followed by the entire
 * data+stack segments
 * and user area.
 */
core(vp)
	struct vnode *vp;
{
	struct vattr vattr;
	struct core *corep;
	int len;
	extern char *strncpy();
	struct proc *p = u.u_procp, *tracer = p->p_tptr;
	int offset = 0;
	int ssize = u.u_rlimit[RLIMIT_STACK].rlim_cur;
#ifdef	MORECORE
	char *corefile = "core.more";
#else
	char *corefile = "core";
#endif	MORECORE
	struct ucred *cred;

	/* test for setuid program */
	if (u.u_uid != u.u_ruid || u.u_gid != u.u_rgid) {
		if (tracer == NULL)
			return (0);
		if (tracer->p_suid != 0 &&
		    (tracer->p_suid != u.u_uid || tracer->p_sgid != u.u_gid))
			return (0);
	}
	cred = (tracer == NULL) ? u.u_cred : tracer->p_cred;
	if ((ctob(p->p_dsize) + ssize + sizeof (struct core)) >=
	    u.u_rlimit[RLIMIT_CORE].rlim_cur)
		return (0);
	u.u_error = 0;

#ifdef	sparc
	flush_user_windows_to_stack();
#endif	sparc
	if (vp == (struct vnode *)0) { /* create vnode for core file */
		vattr_null(&vattr);
		vattr.va_type = VREG;
		vattr.va_mode = 0644;
		u.u_error =
		    vn_create(corefile, UIO_SYSSPACE, &vattr, NONEXCL, VWRITE,
		    &vp);

#ifdef	 SYSAUDIT
		au_sysaudit(AUR_CORE, AU_DWRITE, u.u_error, !u.u_error, 2,
		    (caddr_t)0,	(caddr_t)corefile,
		    (caddr_t)0,	(caddr_t)u.u_cwd->cw_dir);
#endif	SYSAUDIT

		if (u.u_error)
			return (0);
		if (vattr.va_nlink != 1) {
			u.u_error = EFAULT;
			goto out;
		}
	}
	vattr_null(&vattr);
	vattr.va_size = 0;
	VOP_SETATTR(vp, &vattr, cred);
	u.u_acflag |= ACORE;

	/*
	 * Dump the specific areas of the u area into the new
	 * core structure for examination by debuggers.  The
	 * new format is now independent of the user structure and
	 * only the information needed by the debuggers is included.
	 */
	corep = (struct core *)new_kmem_alloc(sizeof (struct core), KMEM_SLEEP);
	bzero((caddr_t)corep, sizeof (struct core));
	corep->c_magic = CORE_MAGIC;
	corep->c_len = sizeof (struct core);
	corep->c_regs = *(struct regs *)u.u_ar0;
#ifdef	FPU
	fpu_core(corep);
#endif	FPU
	corep->c_ucode = u.u_code;
	corep->c_aouthdr = u.u_exdata.Ux_A;
	corep->c_signo = u.u_arg[0];
	corep->c_tsize = ctob(p->p_tsize);
	corep->c_dsize = ctob(p->p_dsize);
	corep->c_ssize = ssize;
	len = min(MAXCOMLEN, CORE_NAMELEN);
	(void) strncpy(corep->c_cmdname, u.u_comm, len);
	corep->c_cmdname[len] = '\0';

	u.u_error = corefile_rdwr(UIO_WRITE, vp, /* core file header */
	    (caddr_t)corep, sizeof (struct core),
	    0, UIO_SYSSPACE, IO_UNIT, (int *)0, cred);

	offset += sizeof (struct core);
	kmem_free((caddr_t)corep, sizeof (struct core));

#ifdef	MORECORE
	/*
	 * XXX - Dump out more info about the mappings?
	 */
#endif	MORECORE

	if (u.u_error == 0)			/* UNIX data segment */
		u.u_error = core_seg(vp, offset,
		    (caddr_t)ctob(dptov(p, 0)), ctob(p->p_dsize), cred);
	offset += ctob(p->p_dsize);

	if (u.u_error == 0)			/* UNIX stack segment */
		u.u_error = core_seg(vp, offset,
		    (caddr_t)ctob(sptov(p, btop(ssize - 1))), ssize, cred);
	offset += ssize;

	if (u.u_error == 0) {			/* uarea (for those who care) */
		/*
		 * uarea may be less than 1 page; to play safe,
		 * copy to a temp area
		 */
		caddr_t uareap;

		uareap = (caddr_t) new_kmem_alloc(ctob(UPAGES), KMEM_SLEEP);
		(void) bzero(uareap, ctob(UPAGES));
		bcopy((caddr_t) &u, uareap, sizeof (struct user));
		u.u_error = corefile_rdwr(UIO_WRITE, vp, uareap, ctob(UPAGES),
		    offset, UIO_SYSSPACE, IO_UNIT, (int *)0, cred);
		(void) kmem_free(uareap, ctob(UPAGES));
	}

out:
	VN_RELE(vp);
	return (u.u_error == 0);
}

/*
 * write to a core file using creditial, same as vn_rdwr + cred
 */
int
corefile_rdwr(rw, vp, base, len, offset, seg, ioflag, aresid, cred)
	enum uio_rw rw;
	struct vnode *vp;
	caddr_t base;
	int len;
	int offset;
	int seg;
	int ioflag;
	int *aresid;
	struct ucred *cred;
{
	struct uio auio;
	struct iovec aiov;
	int error;

	if ((rw == UIO_WRITE) && isrofile(vp)) {
		return (EROFS);
	}


	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_segflg = seg;
	auio.uio_resid = len;
	error = VOP_RDWR(vp, &auio, rw, ioflag, cred);
	if (aresid)
		*aresid = auio.uio_resid;
	else
		if (auio.uio_resid)
			error = EIO;
	return (error);
}
