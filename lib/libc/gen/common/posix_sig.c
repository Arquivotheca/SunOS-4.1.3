#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)posix_sig.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

/*
 * posix signal package
 */
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#define cantmask        (sigmask(SIGKILL)|sigmask(SIGSTOP))

extern errno;

/*
 * sigemptyset - all known signals
 */
sigemptyset(sigp)
    sigset_t *sigp;
{
    if (!sigp)
	return errno = EINVAL, -1;
    *sigp = 0;
    return 0;
}
    
/*
 * sigfillset - all known signals
 */
sigfillset(sigp)
    sigset_t *sigp;
{
    if (!sigp)
	return errno = EINVAL, -1;
    *sigp = sigmask(NSIG) - 1;
    return 0;
}
    
/*
 * add the signal to the set
 */
sigaddset(sigp,signo)	
    sigset_t* sigp;
{
    if (!sigp  ||  signo <= 0  ||  signo >= NSIG)
	return errno = EINVAL, -1;
    *sigp |= sigmask(signo);
    return 0;
}

/*
 * remove the signal from the set
 */
sigdelset(sigp,signo)
    sigset_t* sigp;
{
    if (!sigp  ||  signo <= 0  ||  signo >= NSIG)
	return errno = EINVAL, -1;
    *sigp &= ~sigmask(signo);
    return 0;
}

/*
 * return true if the signal is in the set (return is 0 or 1)
 */
sigismember(sigp,signo)
    sigset_t* sigp;
{
    if (!sigp  ||  signo <= 0  ||  signo >= NSIG)
	return errno = EINVAL, -1;
    return (*sigp & sigmask(signo)) != 0;
}

/*
 * sigsuspend - wait for sig
 */
sigsuspend(sigp)
    sigset_t* sigp;
{
    if (!sigp)
	return errno = EINVAL, -1;
    return sigpause(*sigp);
}

/*
 * sigaction - install a handler
 */
sigaction(signo, act, oact)
    struct sigaction* act;
    struct sigaction* oact;
{
    struct sigvec sv;
    struct sigvec nsv;
    int ret;

#ifdef POSIX
    if (signo <= 0 || signo >= NSIG || ((cantmask & sigmask(signo)) && act))
#else
    if (signo <= 0 || signo >= NSIG || (cantmask & sigmask(signo)))
#endif POSIX
	return errno = EINVAL, -1;
    if (act) {
	sv.sv_mask = act->sa_mask;
	sv.sv_handler = act->sa_handler;
#ifdef S5EMUL
	sv.sv_flags = act->sa_flags | SA_INTERRUPT;
#else
	sv.sv_flags = act->sa_flags;
#endif
	if (signo != SIGCHLD)
	    sv.sv_flags &= ~SV_NOCLDSTOP;
    }
    if (act  &&  oact)
	ret = sigvec(signo, &sv, &nsv);
    else if (act)
	ret = sigvec(signo, &sv, (struct sigvec*)0);
    else
	ret = sigvec(signo, (struct sigvec*)0, &nsv);
    if (oact) {
	oact->sa_flags = nsv.sv_flags;
	oact->sa_mask = nsv.sv_mask;
	oact->sa_handler = nsv.sv_handler;
    }
    return ret;
}

/*
 * setprocmask - set the signal mask
 */
sigprocmask(how, set, oset)
    sigset_t* set;
    sigset_t* oset;
{
    sigset_t retmask, Set;

    if (!set) {
	retmask = sigblock(0);
	if (oset)
	    *oset = retmask;
	return 0;
    }
    Set = *set;
    switch (how) {
    default:
	errno = EINVAL;
	return -1;
    case SIG_BLOCK:
	retmask = sigblock(Set);
	break;
    case SIG_UNBLOCK:
	sigfillset(&retmask);
	retmask = sigblock(retmask) & ~Set;
	retmask = sigsetmask(retmask);
	break;
    case SIG_SETMASK:
	retmask = sigsetmask(Set);
	break;
    }
    if (oset)
	*oset = retmask;
    return 0;
}
