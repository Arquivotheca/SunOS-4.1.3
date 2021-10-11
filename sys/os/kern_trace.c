/*	@(#)kern_trace.c 1.1 92/07/30 SMI; from UCB 7.1 6/5/86 */


/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * kern_trace.c
 *
 * Code to support the vtrace system call and the
 * kernel-internal trace macro.
 */

#ifdef	TRACE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/trace.h>
#include <sys/kernel.h>
#include <sys/errno.h>

#include "hrc.h"
#if NHRC == 0
/* No HRC device is configured */
/* This is because stubs.c is only for Object distributions */
#define	hrc_time()	0
#define	hrc_acquire(x)	0
#else
extern u_long hrc_time();
#endif NHRC == 0

/*
 * Size, in entries, of tracebuffer.  Forced into
 * the data segment, so it can be patched.
 */
u_long 	tracetime();
void	tracetime_reset();
void	initialtraceevents();

int 	traceval;	/* Temp storage for returned macro values */
			/*   (see, e.g., sys/vnode.h) */
u_int	tracebufents = 3000;

fd_set	tracedevents = { /* FD_ISSET(ev, &tracedevents) ==> tracing event ev */
0, 0, 0, 0, 0, 0, 0, 0
};
u_int	eventstraced;	/* cumulative number of events traced */
struct	trace_rec *tracebuffer;	/* a pointer to the trace buffer */
struct	trace_rec *tracebufp;	/* the next trace buffer entry to be filled */

/*
 * Initialize the event tracing subsystem, allocating storage,
 * and so on.  Called at kernel startup time.
 */
void
inittrace()
{
	register u_int	entries = tracebufents;

	/*
	 * If tracebufents is nonzero (having been patched to
	 * some specific value), then allocate that much space
	 * at startup time.  This gives us a chance to get
	 * the space before the kernel memory pool has been
	 * fragmented from lots of allocation and deallocation.
	 */

	if (entries == 0)
		return;

	(void) realloctrace(entries);
}

/*
 * Reallocate the trace buffer, first freeing its previous
 * incarnation and then carving out the new area.  Reset
 * eventstraced to zero.  Return 0 on success, errno value
 * otherwise.
 *
 * We use kmem_alloc to obtain space.  We use KMEM_NOSLEEP so that
 * kmem_alloc fails if we ask for more than it can satisfy immediately,
 * since the system is likely to hang otherwise.
 */
int
realloctrace(entries)
	u_int	entries;
{
	/*
	 * Give back what we had.
	 */
	if (tracebuffer != NULL)
		kmem_free((caddr_t)tracebuffer,
			tracebufents * sizeof (struct trace_rec));
	tracebuffer = NULL;
	eventstraced = 0;

	if (entries == 0)
		return (0);

	/*
	 * Grab what we need.
	 */
	tracebuffer = (struct trace_rec *)
	    new_kmem_alloc(entries * sizeof (struct trace_rec), KMEM_NOSLEEP);
	if (tracebuffer == NULL)
		return (ENOMEM);

	tracebufents = entries;
	resettracebuf();
	return (0);
}

/*
 * Reset the trace buffer, discarding all events in it and
 * setting eventstraced to zero.
 */
void
resettracebuf()
{
	register int s = splclock();

	eventstraced = 0;
	tracebufp = tracebuffer;

	initialtraceevents();

	(void) splx(s);
}

/*
 * Called when tracing is reset to record one-time-only initial trace
 * events.
 */
void
initialtraceevents()
{
	initialtrace_page();
	initialtrace_dnlc();
}

/*
 * Record the event specified by the arguments, stashing away as well
 * a timestamp and the currently active process.  Assumes that ev has
 * already been validity-checked.
 */
int
traceit(ev, datum0, datum1, datum2, datum3, datum4, datum5)
	int	ev;
	u_long	datum0, datum1, datum2, datum3, datum4, datum5;
{
	extern struct proc *masterprocp;
	register struct trace_rec *tbp;
	register int s;

	/*
	 * Sanity check.
	 */
	if (tracebuffer == NULL)
		return (0);

	s = splclock();

	/*
	 * If there's a high-resolution timer available, it would
	 * be better to use it instead of the system time variable.
	 * HRC -- High Resolution Clock returns ticks which are
	 * .5 microseconds each.
	 * tracetime() is defined below, gives the HRC value if
	 * it is available, otherwise a timeval time value.
	 */

	tbp = tracebufp;
	tbp->tr_time = tracetime();
	tbp->tr_tag = ev;
	tbp->tr_pid = masterprocp ? masterprocp->p_pid : 0;
	tbp->tr_datum0 = datum0;
	tbp->tr_datum1 = datum1;
	tbp->tr_datum2 = datum2;
	tbp->tr_datum3 = datum3;
	tbp->tr_datum4 = datum4;
	tbp->tr_datum5 = datum5;

	if (++tracebufp >= tracebuffer + tracebufents)
		tracebufp = tracebuffer;
	eventstraced++;

	(void) splx(s);
	return (0);
}


/*
 * Vtrace system call interface.
 *
 * XXX:	Add request to (re)allocate the trace buffer.
 */
vtrace ()
{
	register struct a {
		int	request;
		int	nbits_or_value;
		fd_set	*bits;
	} *uap = (struct a *)u.u_ap;
	fd_set	bits;
	register int ni, request;
	register int s;

	/*
	 * Super-user only...
	 *
	 * XXX:	Liberalize?
	 */
	if (!suser()) {
		u.u_error = EPERM;
		return;
	}

	/*
	 * Do some preliminary checking and setup.  Note that using
	 * fd_sets for this interface requires that all event tags
	 * be less than FD_SETSIZE in value.
	 */
	request = uap->request;
	switch (request) {
	case VTR_DISABLE:
	case VTR_ENABLE:
	case VTR_VALUE:
	case VTR_EXEC:
		if (uap->nbits_or_value > FD_SETSIZE)
			uap->nbits_or_value = FD_SETSIZE;
		ni = howmany(uap->nbits_or_value, NFDBITS);
		break;
	case VTR_STAMP:
	case VTR_RESET:
		break;
	default:
		u.u_error = EINVAL;
		return;
	}

	/*
	 * Handle the request.
	 */
	switch (request) {
	/*
	 * Enable or disable a set of trace events.
	 */
	case VTR_DISABLE:
		tracetime_reset();	/* free up HRC device, if exists */

	case VTR_ENABLE:

		if (u.u_error = copyin((caddr_t)uap->bits, (caddr_t)&bits,
				(u_int)(ni * sizeof (fd_mask))))
			return;
		/*
		 * Lock out new trace events while changing the
		 * set of enabled events.
		 */
		s = splclock();
		while (--ni >= 0) {
			if (request == VTR_DISABLE)
				tracedevents.fds_bits[ni] &= ~bits.fds_bits[ni];
			else
				tracedevents.fds_bits[ni] |=  bits.fds_bits[ni];
		}
		(void) splx(s);
		break;

	/*
	 * Report the currently enabled set of trace events.
	 */
	case VTR_VALUE:
		u.u_error = copyout((caddr_t)&tracedevents, (caddr_t)uap->bits,
				(u_int)(ni * sizeof (fd_mask)));
		break;

	/*
	 * Cause a TR_STAMP trace event (subject to its being enabled).
	 */
	case VTR_STAMP:
		trace2(TR_STAMP, uap->nbits_or_value, u.u_procp->p_pid);
		break;

	/*
	 * Reset the trace buffer.
	 */
	case VTR_RESET:
		resettracebuf();
		tracetime_reset();
		break;

	case VTR_EXEC:
		if (u.u_error = copyin((caddr_t)uap->bits, (caddr_t)&bits,
				(u_int)(ni * sizeof (fd_mask))))
			return;
		vtrace_exec(&bits);
		break;
	}
}

int
vtrace_exec(bits)
	fd_set *bits;
{
	fd_set savebits;
	int s, ni;

	s = splclock();
	savebits = tracedevents;
	ni = howmany(FD_SETSIZE, NFDBITS);

	/* Turn on the bits designated to enable the requested trace points */
	while (--ni >= 0)
		tracedevents.fds_bits[ni] |=  bits->fds_bits[ni];
	(void) splx(s);

	/* Force the designated trace points to be executed */
#ifdef UFS
	if (FD_ISSET(TR_UFS_INODE, bits))
		trace_inode();
	if (FD_ISSET(TR_UFS_INSTATS, bits))
		trace_instats();
	if (FD_ISSET(TR_UFS_INSTATS_RESET, bits))
		trace_instats_reset();
#endif UFS
	s = splclock();
	tracedevents = savebits;
	(void) splx(s);
}




/*
 * tracetime()
 * returns the highest resolution realtime clock value
 * available.   Presenty this is HRC, when it is installed and working.
 * if HRC is not available, the system time value
 * is scaled to milliseconds and returned.
 * XXX Other Clocks may be used in the future for this routine.
 * XXX This routine could be made more generally available
 * for now, it is just for event tracing within the kernel
 */

#define	BT_UNUSED	0
#define	BT_HRC		1
#define	BT_SYSTIME	2

/* Only intereted in Milliseconds of system time variable XXXX */
#define	CEILING		2000000
#define	SYSTIME_MSEC	((time.tv_sec % CEILING) * 1000 + time.tv_usec / 1000)

int tracetime_source 	= BT_UNUSED;
int tracetime_ticks_sec = 0;

u_long
tracetime()
{

	register u_long result_ticks = 0;

	/* XXX: should we provide unique timestamps, similar to uniqtime() */
	switch (tracetime_source) {
	case BT_HRC:
			result_ticks = hrc_time();
			break;
	case BT_SYSTIME:
			result_ticks = SYSTIME_MSEC;
			break;
	case BT_UNUSED:
			if (hrc_acquire(1)) {
				/* Captured the HRC access privilege */
				tracetime_source = BT_HRC;
				tracetime_ticks_sec = 2000000;  /* 2 MHz ticks */
				result_ticks = hrc_time();
			} else {
				tracetime_source = BT_SYSTIME;
				tracetime_ticks_sec = 1000; /* msec */
				result_ticks = SYSTIME_MSEC;
			}
			break;
	default:
			printf("Bad tracetime_source: %d", tracetime_source);
			result_ticks = 0;
			break;
	}

	return (result_ticks);
}

/*
 * tracetime_reset()
 * called to relinquish HRC access (if it had been obtained)
 * allows later calls to tracetime() to acquire best clock device
 */
void
tracetime_reset()
{
	if (tracetime_source == BT_HRC) {
		hrc_acquire(0);	/* release hrc */
	}

	tracetime_source = BT_UNUSED;
	tracetime_ticks_sec = 0;
}

#endif TRACE
