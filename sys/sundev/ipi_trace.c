#ifndef lint
static char sccsid[] = "@(#)ipi_trace.c	1.1 7/30/92 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Tracing support for IPI drivers.
 *
 * When system-wide tracing is defined, the macros that would call this
 * support call that instead.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sundev/ipi_trace.h>

#ifdef	IPI_TRACE_SOLO

fd_set			ipi_traced;	/* bitmap of traced events */

static	u_int	ipi_ntrace = 1024;	/* size of trace buffer */

struct ipi_trace_ctl {
	struct trace_rec	*tc_first;	/* first entry */
	struct trace_rec	*tc_in;		/* next entry to write */
	struct trace_rec	*tc_out;	/* next entry to read */
	struct trace_rec	*tc_limit;	/* points past the end */
};

static struct ipi_trace_ctl ipi_trace_ctl;

/*
 * Initialize tracing.
 */
void
ipi_inittrace()
{
	struct ipi_trace_ctl *tc;
	int	i;

	tc = &ipi_trace_ctl;
	tc->tc_first = (struct trace_rec *)
		kmem_zalloc(ipi_ntrace * sizeof(struct trace_rec));
	if (tc->tc_first == NULL) {
		printf("ipi_inittrace: no space for trace\n");
		return;
	}
	tc->tc_in = tc->tc_out = tc->tc_first;
	tc->tc_limit = tc->tc_first + ipi_ntrace;

	/*
	 * Turn everything on for starts
	 */
	for (i = 0; i < FD_SETSIZE; i++)
		FD_SET(i, &ipi_traced);
}

/*
 * Make a trace entry.
 */
void
ipi_traceit(ev, d0, d1, d2, d3, d4, d5)
	u_long ev, d0, d1, d2, d3, d4, d5;
{
	register struct ipi_trace_ctl *tc = &ipi_trace_ctl;
	register struct trace_rec *t;
	extern struct timeval time;
	register int	s;

	if (panicstr)
		return;		/* don't let panic dump pollute the trace */
	s = splhigh();
	t = tc->tc_in;
	if (++(tc->tc_in) >= tc->tc_limit)
		tc->tc_in = tc->tc_first;
	(void) splx(s);
	t->tr_tag = ev;
	t->tr_datum0 = d0;	t->tr_datum1 = d1;	t->tr_datum2 = d2;
	t->tr_datum3 = d3;	t->tr_datum4 = d4;	t->tr_datum5 = d5;
	t->tr_time = (time.tv_sec << 16) | ((unsigned) time.tv_usec >> 16);
}
#endif /* IPI_TRACE_SOLO */
