#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_select.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_select.c - Notifier's version of select.  Will do notification cycle
 *		   if not already in the middle of it.
 */
#include <sys/types.h>
#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/ndis.h>	/* For ndis_client == NTFY_CLIENT_NULL check */
#include <errno.h>

static	Notify_value	ndet_select_in_func(), ndet_select_out_func(),
		    ndet_select_except_func(), ndet_select_itimer_func();

static	fd_set	ndet_select_ibits, ndet_select_obits, ndet_select_ebits;
static	int	ndet_select_nfds, ndet_select_timeout;

static	Notify_client ndet_select_nclient = (Notify_client)&ndet_select_ibits;

extern	errno;

extern int
select(nfds, readfds, writefds, exceptfds, timeout)
	register int nfds;
	fd_set *readfds, *writefds, *exceptfds;
	struct timeval *timeout;
{
	struct  itimerval itimer;
	register int fd;
	int errno_remember; /* Trying figure why select returns for no reason */

	/*
	 * Do real select if in middle of notification loop or
	 * no other clients and no notifications pending.
	 * Also, don't dispatch events if haven't started notifier and
	 * "background" dispatching hasn't been turned on.
	 */
	if ((ndet_flags & NDET_STARTED) ||
	    ((!(ndet_flags & NDET_STARTED)) &&
	      (!(ndet_flags & NDET_DISPATCH))) ||
	    ((ndet_clients == NTFY_CLIENT_NULL) &&
	      (ndis_clients == NTFY_CLIENT_NULL)))
		return(notify_select(nfds, readfds, writefds, exceptfds,
		    timeout));
	/* Set up fd related conditions */
	for (fd = 0; fd < nfds; fd++) {
		if (readfds && FD_ISSET(fd, readfds))
			(void) notify_set_input_func(ndet_select_nclient,
			    ndet_select_in_func, fd);
		if (writefds && FD_ISSET(fd, writefds))
			(void) notify_set_output_func(ndet_select_nclient,
			    ndet_select_out_func, fd);
		if (exceptfds && FD_ISSET(fd, exceptfds))
			(void) notify_set_exception_func(ndet_select_nclient,
			    ndet_select_except_func, fd);
	}
	/* Set up timeout condition */
	if (timeout) {
		timerclear(&(itimer.it_interval));
		/* Transform select's polling value to itimer's */
		itimer.it_value = (timerisset(timeout))? *timeout:
		    NOTIFY_POLLING_ITIMER.it_value;
		(void) notify_set_itimer_func(ndet_select_nclient,
		    ndet_select_itimer_func, ITIMER_REAL, &itimer,
		    NTFY_ITIMER_NULL);
	}
	/* Set up variables that will collect the notifications */
	FD_ZERO(&ndet_select_ibits); FD_ZERO(&ndet_select_obits);
	FD_ZERO(&ndet_select_ebits);
	ndet_select_nfds = ndet_select_timeout = 0;
	/*
	 * Set up flag so that break out of select on a signal.
	 * Note: We could setup an async signal condition for every
	 * signal but this is too expensive
	 * Question: Should we set NDET_NO_DELAY (like for read) for select
	 * if any of the fd are non-blocking?  Answer: Via experimentation,
	 * a select of a non-blocking file descriptor does not return until
	 * activity occurs on the file descriptor.  Thus, we should not set
	 * NDET_NO_DELAY for select.
	 */
	ndet_flags |= NDET_STOP_ON_SIG;
	/*
	 * Start notifier.
	 * Note: Is errno from the real select preserved well enough in
	 * order to make it out to the caller of select?
	 */
	(void) notify_start();
	errno_remember = errno;
	/* Reset break out flag */
	ndet_flags &= ~NDET_STOP_ON_SIG;
	/* Clear select conditions from notifier */
	for (fd = 0; fd < nfds; fd++) {
		if (readfds && FD_ISSET(fd, readfds))
			(void) notify_set_input_func(ndet_select_nclient,
			    NOTIFY_FUNC_NULL, fd);
		if (writefds && FD_ISSET(fd, writefds))
			(void) notify_set_output_func(ndet_select_nclient,
			    NOTIFY_FUNC_NULL, fd);
		if (exceptfds && FD_ISSET(fd, exceptfds))
			(void) notify_set_exception_func(ndet_select_nclient,
			    NOTIFY_FUNC_NULL, fd);
	}
	if (timeout) {
		itimer = NOTIFY_NO_ITIMER;
		(void) notify_set_itimer_func(ndet_select_nclient,
		    NOTIFY_FUNC_NULL, ITIMER_REAL, &itimer, NTFY_ITIMER_NULL);
	}
	/* Set up return values */
	if (readfds)
		*readfds = ndet_select_ibits;
	if (writefds)
		*writefds = ndet_select_obits;
	if (exceptfds)
		*exceptfds = ndet_select_ebits;
	/*
	 * If no fd related notifications and no timeout occurred then
	 * assume error return from real select.
	 */
	if (ndet_select_nfds == 0 && !(ndet_select_timeout && timeout))
		ndet_select_nfds = -1;
	ntfy_assert(errno == errno_remember, "errno changed in select");
	return(ndet_select_nfds);
}

/* ARGSUSED */
static Notify_value
ndet_select_in_func(nclient, fd)
	Notify_client nclient;
	int fd;
{
	FD_SET(fd, &ndet_select_ibits);
	ndet_select_nfds++;
	(void)notify_stop();
	return(NOTIFY_DONE);
}

/* ARGSUSED */
static Notify_value
ndet_select_out_func(nclient, fd)
	Notify_client nclient;
	int fd;
{
	FD_SET(fd, &ndet_select_obits);
	ndet_select_nfds++;
	(void)notify_stop();
	return(NOTIFY_DONE);
}

/* ARGSUSED */
static Notify_value
ndet_select_except_func(nclient, fd)
	Notify_client nclient;
	int fd;
{
	FD_SET(fd, &ndet_select_ebits);
	ndet_select_nfds++;
	(void)notify_stop();
	return(NOTIFY_DONE);
}

/* ARGSUSED */
static Notify_value
ndet_select_itimer_func(nclient, which)
	Notify_client nclient;
	int which;
{
	ndet_select_timeout = 1;
	(void)notify_stop();
	return(NOTIFY_DONE);
}

