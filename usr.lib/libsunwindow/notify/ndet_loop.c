#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_loop.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_loop.c - Notification loop.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>
#include <sunwindow/ndis.h>		/* For ndis_dispatch */
#include <syscall.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>		/* For temp debugging */

extern	int errno;

pkg_private_data u_int	ndet_flags = 0;
pkg_private_data NTFY_CLIENT *ndet_clients = 0;
pkg_private_data NTFY_CLIENT *ndet_client_latest = 0;
pkg_private_data u_int	ndet_sigs_received = 0;
pkg_private_data u_int	ndet_sigs_managing = 0;
pkg_private_data fd_set	ndet_fasync_mask = 0;  /* Initialized to 0 by the compiler*/
pkg_private_data fd_set	ndet_fndelay_mask = 0; /* Maintained, but really used? */

pkg_private_data struct	timeval ndet_polling_tv = {0,0};

pkg_private_data fd_set	ndet_ibits = 0, ndet_obits = 0, ndet_ebits = 0; /* = 0;  */
static	struct timeval ndet_signal_check;	/* Minimum select timeout */

pkg_private_data struct sigvec ndet_prev_sigvec[NSIG] = 0;

pkg_private void ndet_signal_catcher();
pkg_private_data struct sigvec ndet_sigvec = {ndet_signal_catcher, 0, 0};
static	int	ndet_signal_code;
static	struct	sigcontext *ndet_signal_context;

static	void ndet_update_itimer();
static	void ndet_send_async_sigs();
static	void ndet_fig_fd_change(), ndet_fig_wait3_change(),
	    ndet_fig_destroy_change(), ndet_fig_sig_change();
static	NTFY_ENUM ndet_sig_send(), ndet_poll_send();

static	NTFY_ENUM ndet_itimer_change();
static	NTFY_ENUM ndet_destroy_change();
static	NTFY_ENUM ndet_sig_change();
static	NTFY_ENUM ndet_fd_change();
static	NTFY_ENUM ndet_wait3_change();
static	NTFY_ENUM ndet_virtual_set_tv_update();
static	NTFY_ENUM ndet_async_sig_send();

#ifdef	lint
/* VARARGS */
/* ARGSUSED */
int	syscall(a) { return(0);} /* When syscall put in llib-lc then remove */
#endif	lint

extern Notify_error
notify_start()
{
	register struct	timeval *timer;
	fd_set	ibits, obits, ebits;
	register int	nfds;
	NDET_ENUM_SEND	enum_send;
	int errno_remember = errno;
	Notify_error return_code;

	FD_ZERO(&ibits); FD_ZERO(&obits); FD_ZERO(&ebits);
	/* Notify_start is not reentrant */
	if (ndet_flags & NDET_STARTED) {
		ntfy_set_errno(NOTIFY_INVAL);
		return(notify_errno);
	}
	/* Always go around the loop at least once */
	ndet_flags |= NDET_STARTED;
	do {
		NTFY_BEGIN_CRITICAL;
		/* Ndet_update_*_itimer (below) may set up NDET_ITIMER_ENQ*/
		ndet_flags &= ~NDET_ITIMER_ENQ;
		/* If nothing has changed then do no set up */
		if (ndet_flags & NDET_CONDITION_CHANGE) {
			if (ndet_flags & NDET_REAL_CHANGE)
				ndet_update_real_itimer();
			if (ndet_flags & NDET_VIRTUAL_CHANGE)
				ndet_update_virtual_itimer();
			if (ndet_flags & NDET_FD_CHANGE)
				ndet_fig_fd_change();
			if (ndet_flags & NDET_WAIT3_CHANGE)
				ndet_fig_wait3_change();
			if (ndet_flags & NDET_DESTROY_CHANGE)
				ndet_fig_destroy_change();
			/*
			 * Always handle signal changes last because other
			 * ndet_*_change and ndet_update_*_itimer calls may
			 * have set NDET_SIGNAL_CHANGE in ndet_flags.
			 */
			if (ndet_flags & NDET_SIGNAL_CHANGE)
				ndet_fig_sig_change();
		}
		/*
		 * Set up select parameters.  Wouldn't get changes from
		 * signal processing this time around.  Poll if stuff in
		 * the dispatch queue.
		 */
		timer = (
		   ndet_flags & (NDET_POLL | NDET_ITIMER_ENQ | NDET_NO_DELAY) ||
		   ndis_clients != NTFY_CLIENT_NULL)?
		      &ndet_polling_tv: NTFY_TIMEVAL_NULL;
		ibits = ndet_ibits;
		obits = ndet_obits;
		ebits = ndet_ebits;
		NTFY_END_CRITICAL;
		/*
		 * From the tests of ndet_sigs_received until get into
		 * pause or select below is a race condition.
		 * It is possible to get a signal during this time that wouldn't
		 * get serviced until something breaks us out of the block.
		 * Can't get away from this.  UNIX needs a version of select
		 * and pause that will simultaneously release blocked signals.
		 * We test ndet_sigs_received as late as possible in order
		 * to reduce this window of vulnerability.
		 */
		if (!ntfy_fd_anyset(&ibits) && !ntfy_fd_anyset(&obits) &&
		    !ntfy_fd_anyset(&ebits) && timer == NTFY_TIMEVAL_NULL && 
		    !ndet_sigs_received) {
			if (ndet_sigs_managing != 0) {
				/* Wait for interrupt */
				pause();
			} else {
				/* Not detecting ANY conditions */
				return_code = NOTIFY_NO_CONDITION;
				goto Finish;
			}
		} else {
			/*
			 * Hack to avoid race condition described below.
			 * notify_signal_check must be called explicitly
			 * to enable this mechanism.
			 */
			if ((timer == NTFY_TIMEVAL_NULL) &&
			    (timerisset(&ndet_signal_check)))
				timer = &ndet_signal_check;
			/*
			 * Wait for select to return.
			 *
			 * Do this ndet_sigs_received test at the last possible
			 * nano in order to reduce the unavoidable race
			 * condition between detecting signal arrival and
			 * making the call to select (which will return with an
			 * EINTR when a signal arrives while IN select,
			 * not ON THE WAY into select).
			 */
			nfds = syscall(SYS_select,
			    FD_SETSIZE, &ibits, &obits, &ebits,
			    (ndet_sigs_received)? &ndet_polling_tv: timer);
			errno_remember = errno;
			/* See if select returned unconventionally */
			if (nfds == -1) {
				/* Clear *bits when in an undefined situation */
				FD_ZERO(&ibits); FD_ZERO(&obits);
				FD_ZERO(&ebits);
				switch (errno) {
				case EINTR:
					/* Signals received */
					if (ndet_flags & NDET_STOP_ON_SIG)
						(void) notify_stop();
					break; /* Out of switch */
				case EBADF:
					ntfy_set_errno(NOTIFY_BADF);
					goto Error;
				default:
					ntfy_set_errno(NOTIFY_INVAL);
					goto Error;
				}
			} else {
				/*
				 * Terminate notification loop if ask to
				 * stop on sig and received signal before
				 * entering select.
				 */
				if (ndet_flags & NDET_STOP_ON_SIG &&
				    ndet_sigs_received) {
					(void) notify_stop();
					/*
					 * Modify errno to indicate that an
					 * signal was received during select.
					 */
					errno_remember = errno = EINTR;
				}
			/*
			 * (timer && (nfds == 0)) means select timer expired.
			 * Since will only go off in polling situation and
			 * we send poll notifications every time around loop,
			 * fall through.
			 */
			}
		}
		/* Enqueue detected notifications with the dispatcher */
		NTFY_BEGIN_CRITICAL;
		/* Set up enumeration record */
		enum_send.ibits = ibits;
		enum_send.obits = obits;
		enum_send.ebits = ebits;
		enum_send.wait3 = NTFY_WAIT3_DATA_NULL;
		timerclear(&(enum_send.current_tv));
		enum_send.sigs = ndet_sigs_received;
		ndet_sigs_received = 0;
		/* Check for fd related condition */
		/*if (ibits || obits || ebits) */
		if (ntfy_fd_anyset(&ibits) || ntfy_fd_anyset(&obits) ||
		    ntfy_fd_anyset(&ebits))
			if (ntfy_enum_conditions(ndet_clients, ndet_fd_send,
			    (NTFY_ENUM_DATA)&enum_send) == NTFY_ENUM_TERM)
				goto Protected_Error;
		/* Check for signal related condition */
		if (enum_send.sigs)
			/*
			 * Use paranoid enum because when get in to
			 * ndet_auto_sig_send will do another enumeration
			 * that can axe client/condition but not have the
			 * opportunity to return NTFY_ENUM_SKIP to the
			 * original enumeration (this one).
			 */
			if(ntfy_paranoid_enum_conditions(ndet_clients,
			    ndet_sig_send, (NTFY_ENUM_DATA)&enum_send) ==
			    NTFY_ENUM_TERM)
				goto Protected_Error;
		if (ndet_flags & NDET_POLL)
			if (ntfy_enum_conditions(ndet_clients, ndet_poll_send,
			    NTFY_ENUM_DATA_NULL) == NTFY_ENUM_TERM)
				goto Protected_Error;
		NTFY_END_CRITICAL;
		/* Dispatch any notification enqueued with the dispatcher */
		if (ndis_dispatch() != NOTIFY_OK)
			goto Error;
	} while (!(ndet_flags & (NDET_STOP | NDET_NO_DELAY)));
	return_code = NOTIFY_OK;
	goto Finish;
Protected_Error:
	NTFY_END_CRITICAL;
Error:
	return_code = notify_errno;
Finish:
	ndet_flags &= ~(NDET_STOP | NDET_STARTED);
	if (ndet_flags & NDET_EXIT_SOON) {
		/* Allow everyone to clean up */
		(void) notify_die(DESTROY_PROCESS_DEATH);
		exit(1);
	}
	if (ndet_flags & NDET_STOP_ON_SIG) {
#ifdef	NTFY_DEBUG_SELECT
		char sb[100];

		sprintf(sb,
		    "errno changed (old=%ld, cur=%ld); reset to old",
		    errno_remember, errno);
		ntfy_assert(errno == errno_remember, sb);
#endif	NTFY_DEBUG_SELECT
		errno = errno_remember;
	}
	return(return_code);
}

/*
 * Set flag indicating that should return from notify_start.
 */
extern Notify_error
notify_stop()
{
	if (ndet_flags & NDET_STARTED) {
		ndet_flags |= NDET_STOP;
		return(NOTIFY_OK);
	} else
		return(NOTIFY_NOT_STARTED);
}

/*
 * Some fd related condition has changed.  Reset all ndet_*bits.
 * Enable(disable) notifier auto signal catching of SIGIO and SIGURG.
 */
static void
ndet_fig_fd_change()
{
	u_int sigs_tmp;

	ndet_flags &= ~NDET_FD_CHANGE;
	/* Remember bits of notifier auto signal catcher */
	sigs_tmp = ndet_sigs_auto;
	/* Zero out bits */
	FD_ZERO(&ndet_ibits); FD_ZERO(&ndet_obits); FD_ZERO(&ndet_ebits); 
	ndet_sigs_auto &= ~(SIG_BIT(SIGIO) | SIG_BIT(SIGURG));
	/* Recompute all bits */
	(void) ntfy_enum_conditions(ndet_clients, ndet_fd_change,
	    NTFY_ENUM_DATA_NULL);
	/* Toggle notifier auto signal catching if situation changed */
	ndet_toggle_auto(sigs_tmp, SIGIO);
	ndet_toggle_auto(sigs_tmp, SIGURG);
}

/* ARGSUSED */
static NTFY_ENUM
ndet_fd_change(client, condition, context)
	NTFY_CLIENT *client;
	NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{

	switch (condition->type) {
	case NTFY_INPUT:
		if (FD_ISSET(condition->data.fd, &ndet_fasync_mask))
			ndet_sigs_auto |= SIG_BIT(SIGIO);
		else
			/*ndet_ibits |= bit; */
			FD_SET(condition->data.fd, &ndet_ibits);
		break;
	case NTFY_OUTPUT:
		if (FD_ISSET(condition->data.fd, &ndet_fasync_mask))
			ndet_sigs_auto |= SIG_BIT(SIGIO);
		else
			/*ndet_obits |= bit; */
			FD_SET(condition->data.fd, &ndet_obits);
		break;
	case NTFY_EXCEPTION:
		if (FD_ISSET(condition->data.fd, &ndet_fasync_mask))
			ndet_sigs_auto |= SIG_BIT(SIGURG);
		else
			/*ndet_ebits |= bit; */
			FD_SET(condition->data.fd, &ndet_ebits);
		break;
	default: {}
	}
	return(NTFY_ENUM_NEXT);
}

/*
 * Some wait3 condition has changed.
 * Enable(disable) notifier auto signal catching of SIGCHLD.
 */
static	void
ndet_fig_wait3_change()
{
	u_int sigs_tmp;

	ndet_flags &= ~NDET_WAIT3_CHANGE;
	/* Remember bits of notifier auto signal catcher */
	sigs_tmp = ndet_sigs_auto;
	/* Zero out bits */
	ndet_sigs_auto &= ~SIG_BIT(SIGCHLD);
	/* See if any wait conditions */
	(void) ntfy_enum_conditions(ndet_clients, ndet_wait3_change,
	    NTFY_ENUM_DATA_NULL);
	/* Toggle notifier auto signal catching if situation changed */
	ndet_toggle_auto(sigs_tmp, SIGCHLD);
}

/* ARGSUSED */
static NTFY_ENUM
ndet_wait3_change(client, condition, context)
	NTFY_CLIENT *client;
	NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	if (condition->type == NTFY_WAIT3)
		ndet_sigs_auto |= SIG_BIT(SIGCHLD);
	return(NTFY_ENUM_NEXT);
}

/*
 * Some virtual itimer related condition has changed.  Update all virtual
 * itimers.  Determine minimum wait and set virtual itimer.
 * Enable(disable) notifier auto signal catching of SIGVTALRM.
 */
pkg_private void
ndet_update_virtual_itimer()
{
	struct timeval ndet_virtual_min();
	struct itimerval current_itimer;
	NDET_ENUM_ITIMER enum_itimer;
	int n;

	ndet_flags &= ~(NDET_VIRTUAL_CHANGE | NDET_VIRTUAL_POLL);
	/* Initialize virtual itimer update probe */
	enum_itimer.type = NTFY_VIRTUAL_ITIMER;
	enum_itimer.polling_bit = NDET_VIRTUAL_POLL;
	enum_itimer.signal = SIGVTALRM;
	enum_itimer.which = ITIMER_VIRTUAL;
	enum_itimer.min_func = ndet_virtual_min;
	/* Virtual itimers are relative to current state of process itimer */
	n = getitimer(ITIMER_VIRTUAL, &current_itimer); /* SYSTEM CALL */
	ntfy_assert(n == 0, "Unexpected error: getitimer");
	enum_itimer.current_tv = current_itimer.it_value;
	/* enum_itimer.min_tv is initialized in ndet_update_itimer. */
	/*
	 * Update existing virtual itimer conditions and find what process
	 * itimer was set to.
	 */
	ndet_update_itimer(&enum_itimer);
	/* Update set_tv in existing virtual itimers */
	(void) ntfy_enum_conditions(ndet_clients, ndet_virtual_set_tv_update,
	    (NTFY_ENUM_DATA)&enum_itimer.min_tv);
}

/*
 * Some real itimer related condition has changed.
 * Determine minimum wait and set real itimer.
 * Enable(disable) notifier auto signal catching of SIGALRM.
 */
pkg_private void
ndet_update_real_itimer()
{
	struct timeval ndet_real_min();
	NDET_ENUM_ITIMER enum_itimer;
	int n;

	ndet_flags &= ~(NDET_REAL_CHANGE | NDET_REAL_POLL);
	/* Initialize virtual itimer update probe */
	enum_itimer.type = NTFY_REAL_ITIMER;
	enum_itimer.polling_bit = NDET_REAL_POLL;
	enum_itimer.signal = SIGALRM;
	enum_itimer.which = ITIMER_REAL;
	enum_itimer.min_func = ndet_real_min;
	/* Real itimers are relative to current time of day */
	n = gettimeofday(&enum_itimer.current_tv,
			(struct timezone *)0); /* SYSTEM CALL */
	ntfy_assert(n == 0, "Unexpected error: gettimeofday");
	/* enum_itimer.min_tv is initialized in ndet_update_itimer. */
	/* Determine and set process real itimer */
	ndet_update_itimer(&enum_itimer);
}

static struct timeval NDET_END_OF_TIME = {100000000,0};
	/* For explanation of why 100000000, see ndet_check_tv */

static void
ndet_update_itimer(enum_itimer)
	NDET_ENUM_ITIMER *enum_itimer;
{
	u_int sigs_tmp;
	struct	itimerval process_itimer;
	int n;

	/* Remember bits of notifier auto signal catcher */
	sigs_tmp = ndet_sigs_auto;
	/* Zero out polling bit */
	ndet_flags &= ~enum_itimer->polling_bit;
	ndet_sigs_auto &= ~SIG_BIT(enum_itimer->signal);
	/* Recompute interval timer */
	enum_itimer->min_tv = NDET_END_OF_TIME;
	(void) ntfy_enum_conditions(ndet_clients, ndet_itimer_change,
	    (NTFY_ENUM_DATA)enum_itimer);
	/* Toggle notifier auto signal catching if situation changed */
	ndet_toggle_auto(sigs_tmp, enum_itimer->signal);
	/* Set interval timer */
	timerclear(&process_itimer.it_interval);
	if (ndet_tv_equal(enum_itimer->min_tv, NDET_END_OF_TIME))
		/* No one reset min_tv */
		timerclear(&enum_itimer->min_tv);
	process_itimer.it_value = enum_itimer->min_tv;
	n = setitimer(enum_itimer->which, &process_itimer, /* SYSTEM CALL */
	    (struct itimerval *)0);
	ntfy_assert(n == 0, "Unexpected error: setitimer");
}

/*
 * Called to update global polling flags and find global minimum until next
 * itimer expiration.
 */
static NTFY_ENUM
ndet_itimer_change(client, condition, context)
	NTFY_CLIENT *client;
	NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	struct timeval local_min;
	register NDET_ENUM_ITIMER *enum_itimer = (NDET_ENUM_ITIMER *)context;
	register NTFY_ITIMER *n_itimer;

	switch (condition->type) {
	case NTFY_VIRTUAL_ITIMER:
	case NTFY_REAL_ITIMER:
		n_itimer = condition->data.ntfy_itimer;
		if (condition->type != enum_itimer->type)
			break;
		/* See if polling itimer */
		if (ndet_tv_polling(n_itimer->itimer.it_value))
			ndet_flags |= enum_itimer->polling_bit;
		else {
			/* Figure time to go until expiration for this client */
			local_min = enum_itimer->min_func(n_itimer,
			    enum_itimer->current_tv);
			/* See if expired */
			if (!timerisset(&local_min)) {
				/*
				 * Dispatch notification, reset itimer value,
				 * remove if nothing to wait for (returns !0).
				 */
				ndet_flags |= NDET_ITIMER_ENQ;
				if (ndet_itimer_expired(client, condition))
					/*
					 * Know can skip rest of clients
					 * conditions because only one itimer
					 * of each type is allowed per client.
					 */
					return(NTFY_ENUM_SKIP);
				/* Else update local_min and set time */
				local_min = n_itimer->itimer.it_value;
				n_itimer->set_tv = enum_itimer->current_tv;
			}
			/* Figure global minimum time to go until expiration */
			enum_itimer->min_tv = ndet_tv_min(local_min,
			    enum_itimer->min_tv);
			/*
			 * Tell automatic signal mechanism to watch for this
			 * kind of interval timer expiration.
			 */
			ndet_sigs_auto |= SIG_BIT(enum_itimer->signal);
		}
		/*
		 * Know can skip rest of clients conditions because only one
		 * itimer of each type is allowed per client.
		 */
		return(NTFY_ENUM_SKIP);
	default: {}
	}
	return(NTFY_ENUM_NEXT);
}

/* Update every virtual itimer's set_tv field */
/* ARGSUSED */
static NTFY_ENUM
ndet_virtual_set_tv_update(client, condition, context)
	NTFY_CLIENT *client;
	NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	struct timeval *set_tv = (struct timeval *)context;

	if (condition->type == NTFY_VIRTUAL_ITIMER) {
		condition->data.ntfy_itimer->set_tv = *set_tv;
		/*
		 * Know can skip rest of clients conditions because only one
		 * itimer of each type is allowed per client.
		 */
		return(NTFY_ENUM_SKIP);
	} else
		return(NTFY_ENUM_NEXT);
}

static void
ndet_fig_destroy_change()
{
	u_int sigs_auto_tmp;

	ndet_flags &= ~NDET_DESTROY_CHANGE;
	/* Remember what signals were catching for auto client */
	sigs_auto_tmp = ndet_sigs_auto;
	/* Zero out what used to collect the data for auto client */
	ndet_sigs_auto &= ~(SIG_BIT(SIGTERM));
	/* Recompute SIGTERM managing */
	(void) ntfy_enum_conditions(ndet_clients, ndet_destroy_change,
	    NTFY_ENUM_DATA_NULL);
	/* Toggle notifier auto signal catching if situation changed */
	ndet_toggle_auto(sigs_auto_tmp, SIGTERM);
}

static void
ndet_fig_sig_change()
{
	register u_int sig_bit, sigs_tmp, sigs_dif;
	register int sig;
	int n;

	ndet_flags &= ~NDET_SIGNAL_CHANGE;
	/* Remember what signals were catching */
	sigs_tmp = ndet_sigs_managing;
	/* Zero out what used to collect the data */
		/*
		 * Note: ndet_signal_catcher shouldn't look at
		 * ndet_sigs_managing when NTFY_IN_CRITICAL.
		 */
	ndet_sigs_managing = 0;
	/* Recompute signals managing */
	(void) ntfy_enum_conditions(ndet_clients, ndet_sig_change,
	    NTFY_ENUM_DATA_NULL);
	/* Update signal catching */
	sigs_dif = ndet_sigs_managing^sigs_tmp;
	for (sig = 1;sig < NSIG;sig++) {
		if ((sig_bit = SIG_BIT(sig) & sigs_dif)) {
			if (sig_bit & ndet_sigs_managing) {
				ndet_enable_sig(sig);
			} if (sig_bit & sigs_tmp) {
				/*
				 * Don't catch this signal,
				 * currently we are
				 */
				n = sigvec(sig, &ndet_prev_sigvec[sig],
				    (struct sigvec *)0); /* SYSTEM CALL */
				ntfy_assert(n == 0, "Unexpected error: sigvec");
			} else
				ntfy_set_errno(NOTIFY_INTERNAL_ERROR);
		}
	}
}

/*
 * Call this routine (other than from ndet_fig_sig_change) when you need
 * to make sure that a signal is being caught but don't want to go through
 * the whole process of globally finding out who else needs it.
 */
pkg_private void
ndet_enable_sig(sig)
	int sig;
{
	if (!(SIG_BIT(sig) & ndet_sigs_managing)) {
		int n;

		/* Arrange to catch this signal, currently we are not */
		n = sigvec(sig, &ndet_sigvec, &ndet_prev_sigvec[sig]);
		    /* SYSTEM CALL */
		ntfy_assert(n == 0, "Unexpected error: sigvec");
		ndet_sigs_managing |= SIG_BIT(sig);
	}
}

pkg_private_data int	ndet_track_sigs = 0;

pkg_private void	/* Should be static but there might be clients of it */
ndet_signal_catcher(sig, code, scp)
	int sig;
	int code;
	struct sigcontext *scp;
{
	int oldmask = sigblock((int)ndet_sigs_managing); /* SYSTEM CALL */
	void (*old_handler)() = ndet_prev_sigvec[sig].sv_handler;

	if (NTFY_IN_CRITICAL || ntfy_nodes_avail < NTFY_PRE_ALLOCED_MIN) {
		ntfy_sigs_delayed |= SIG_BIT(sig);
		(void) sigsetmask(oldmask); /* SYSTEM CALL */
#ifdef	NTFY_DEBUG
		if (ndet_track_sigs)
		    (void) fprintf(stdout, "SIG caught when CRITICAL %ld\n", sig);
#endif	NTFY_DEBUG
		goto Done;
	}
	NTFY_BEGIN_INTERRUPT;
	ndet_signal_code = code;
	ndet_signal_context = scp;
	ndet_send_async_sigs(SIG_BIT(sig));
	(void) sigsetmask(oldmask); /* SYSTEM CALL */
	NTFY_END_INTERRUPT;
Done:
	/*
	 * Call previous handler.  This feature is not part of the
	 * notifier definition but is included as a means of reducing
	 * compatibility problems.
	 */
	if (old_handler != SIG_DFL && old_handler != SIG_IGN)
		old_handler(sig, code, scp);
#ifdef	NTFY_DEBUG
	if (ndet_track_sigs)
		(void) fprintf(stdout, "SIG caught %ld\n", sig);
#endif	NTFY_DEBUG
	return;
}

pkg_private void
ndet_send_delayed_sigs()
{
	register int sigs;
	int oldmask;

	ntfy_assert(!NTFY_IN_INTERRUPT, "Tried send delayed sig in interrupt");
	ntfy_assert(!NTFY_IN_CRITICAL, "Tried send delayed sig when protected");
	/* Don't need to enter critical section because blocking signals. */
	/* Carefully reset ntfy_sigs_delayed so don't loose signal. */
	oldmask = sigblock((int)ndet_sigs_managing); /* SYSTEM CALL */
	sigs = ntfy_sigs_delayed;
	ntfy_sigs_delayed = 0;
	/* Send delayed signals */
	ndet_send_async_sigs(sigs);
	(void) sigsetmask(oldmask); /* SYSTEM CALL */
}

/* Don't need to enter critical section because blocking signals when called. */
static void
ndet_send_async_sigs(sigs)
	int sigs;
{

	ndet_sigs_received |= sigs;
	(void) ntfy_paranoid_enum_conditions(ndet_clients, ndet_async_sig_send,
	    (NTFY_ENUM_DATA)sigs);
}

static NTFY_ENUM
ndet_async_sig_send(client, condition, context)
	NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	u_int sigs = (u_int)context;

	if (condition->type == NTFY_ASYNC_SIGNAL &&
	    (SIG_BIT(condition->data.signal) & sigs)) {
		Notify_func func;

		/* Push condition on interposition stack */
		func = nint_push_callout(client, condition);
		/* The notifier doesn't catch any async sigs */
		(void) func(client->nclient,
		    condition->data.signal, NOTIFY_ASYNC);
		/* Pop condition from interposition stack */
		nint_unprotected_pop_callout();
		/* Note: condition/client may be undefined now! */
	}
	return(NTFY_ENUM_NEXT);
}

/* ARGSUSED */
static NTFY_ENUM
ndet_destroy_change(client, condition, context)
	NTFY_CLIENT *client;
	NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	if (condition->type == NTFY_DESTROY)
		/* Tell automatic signal mechanism to watch for SIGTERM */
		ndet_sigs_auto |= SIG_BIT(SIGTERM);
	return(NTFY_ENUM_NEXT);
}

/* ARGSUSED */
static NTFY_ENUM
ndet_sig_change(client, condition, context)
	NTFY_CLIENT *client;
	NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	if ((condition->type == NTFY_SYNC_SIGNAL) ||
	    (condition->type == NTFY_ASYNC_SIGNAL))
		ndet_sigs_managing |= SIG_BIT(condition->data.signal);
	return(NTFY_ENUM_NEXT);
}

pkg_private NTFY_ENUM
ndet_fd_send(client, condition, context)
	NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	register NDET_ENUM_SEND *enum_send = (NDET_ENUM_SEND *)context;

	switch (condition->type) {
	case NTFY_INPUT:
		if (FD_ISSET(condition->data.fd, &enum_send->ibits))
			goto EnQ;
		break;
	case NTFY_OUTPUT:
		if (FD_ISSET(condition->data.fd, &enum_send->obits))
			goto EnQ;
		break;
	case NTFY_EXCEPTION:
		if (FD_ISSET(condition->data.fd, &enum_send->ebits))
			goto EnQ;
		break;
	default: {}
	}
	return(NTFY_ENUM_NEXT);
EnQ:
	if (ndis_enqueue(client, condition) != NOTIFY_OK)
		/* Internal fatal error */
		return(NTFY_ENUM_TERM);
	return(NTFY_ENUM_NEXT);
}

static NTFY_ENUM
ndet_sig_send(client, condition, context)
	NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	register NDET_ENUM_SEND *enum_send = (NDET_ENUM_SEND *)context;

	if (condition->type == NTFY_SYNC_SIGNAL &&
	    (SIG_BIT(condition->data.signal) & enum_send->sigs)) {
		/* Intercept conditions that were set by the notifier */
		if (client->nclient == ndet_auto_nclient)
			return(ndet_auto_sig_send(client, condition, context));
		else {
			if (ndis_enqueue(client, condition) != NOTIFY_OK)
				ntfy_fatal_error("Error when enq condition");
		}
	}
	return(NTFY_ENUM_NEXT);
}

/* ARGSUSED */
static NTFY_ENUM
ndet_poll_send(client, condition, context)
	NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	if ((condition->type == NTFY_REAL_ITIMER ||
	    condition->type == NTFY_VIRTUAL_ITIMER) &&
	    ndet_tv_polling(condition->data.ntfy_itimer->itimer.it_value)) {
		/*
		 * Dispatch notification, reset itimer value,
		 * remove if nothing to wait for an return -1 else 0.
		 */
		if (!ndet_itimer_expired(client, condition)) {
			/*
			 * Avoid making system call in ndet_reset_itimer_set_tv
			 * if just only to be polling again.
			 */
			if (!ndet_tv_polling(
			    condition->data.ntfy_itimer->itimer.it_value))
				ndet_reset_itimer_set_tv(condition);
		}
		/*
		 * Know can skip rest of clients conditions because
		 * only one itimer of each type is allowed per client.
		 */
		return(NTFY_ENUM_SKIP);
	}
	return(NTFY_ENUM_NEXT);
}

extern void
notify_set_signal_check(tv)
	struct timeval tv;
{
	ndet_signal_check = tv;
}

extern struct timeval
notify_get_signal_check()
{
	return (ndet_signal_check);
}

extern int
notify_get_signal_code()
{
	/* Could put check to see if in interrupt (should be) */
	return (ndet_signal_code);
}

extern struct sigcontext *
notify_get_signal_context()
{
	/* Could put check to see if in interrupt (should be) */
	return (ndet_signal_context);
}

