/*	@(#)ipi_trace.h	1.1 7/30/92 SMI */

#ifndef	_ipi_trace_h
#define	_ipi_trace_h

/* #define IPI_TRACE	/* XXX - Temporary? */


#include <sys/trace.h>

/*
 * Trace event codes for IPI.
 */
#define TRC_IPI_CMD	160	/* a0 Command sent to channel driver */
/* "csf %x ref %x pkt %x %x %x % x " */
#define TRC_IPI_RUPT	161	/* a1 Interrupt received from channel driver */
/* "csf %x ref %x" */
#define	TRC_IPI_GET_XFR	162	/* a2 get transfer settings */
/* "csf %x ref %x" */
#define	TRC_IPI_SET_XFR	163	/* a3 set transfer settings */
/* "csf %x ref %x" */
#define	TRC_IPI_RETRY	164	/* a4 retry */
/* "csf %x ref %x pkt %x %x %x %x" */
#define TRC_IPI_TST_BD	170	/* aa test board issued */
/* "csf %x ref %x */
#define TRC_IPI_RST_BD	171	/* ab Reset board issued */
/* "csf %x ref %x */
#define TRC_IPI_RST_CH	172	/* ac Reset channel issued */
/* "csf %x ref %x */
#define TRC_IPI_RST_SL	173	/* ad Reset slave issued */
/* "csf %x ref %x */

/*
 * Trace events for the IS string controller adapter driver.
 */
#define TRC_IS_CMD	176	/* b0 command sent by string ctlr driver */
/* "ctlr %x ref %x cmdaddr %x opcode %x" */
#define TRC_IS_RUPT_OK	177	/* b1 Interrupt with success indicated */
/* "ctlr %x stat %x resp %x" */
#define TRC_IS_RUPT_RESP 178	/* b2 Interrupt with response */
/* "ctlr %x stat %x resp %x majstat %x" */
#define TRC_IS_RUPT_ERR	179	/* b3 Interrupt with error */
/* "ctlr %x stat %x resp %x" */

#ifdef	KERNEL

#ifdef TRACE
/* #define IPI_TRACE		/* IPI trace using system trace support */

#define ipi_trace(ev, d0, d1, d2, d3, d4, d5) 
	trace(ev, d0, d1, d2, d3, d4, d5)
#define ipi_inittrace()
#else 
#ifdef	IPI_TRACE
/* #define	IPI_TRACE_SOLO		/* IPI tracing only */
#endif	/* IPI_TRACE */
#endif	/* TRACE */


#ifdef	IPI_TRACE_SOLO
/*
 * Tracing on but just for IPI stuff.
 */
extern fd_set	ipi_traced;		/* bitmap for IPI tracing */
extern void	ipi_inittrace();	/* initialization routine */
extern void	ipi_traceit();		/* make a trace entry */

/*
 * Lint doesn't believe that there are valid reasons for comparing
 * constants to each other...
 */
#ifdef	lint
#define ipi_trace(ev, d0, d1, d2, d3, d4, d5) \
	if (FD_ISSET((ev), &ipi_traced)) \
		ipi_traceit((u_long)(ev), (u_long)(d0), (u_long)(d1), \
			(u_long)(d2), (u_long)(d3), (u_long)(d4), (u_long)(d5))
#else	/* lint */
#define ipi_trace(ev, d0, d1, d2, d3, d4, d5) \
	if ((u_int)(ev) < FD_SETSIZE && FD_ISSET((ev), &ipi_traced)) \
		ipi_traceit((u_long)(ev), (u_long)(d0), (u_long)(d1), \
			(u_long)(d2), (u_long)(d3), (u_long)(d4), (u_long)(d5))
#endif	/* lint */
#endif	/* IPI_TRACE_SOLO */


#ifdef	IPI_TRACE

#define ipi_trace6(ev, d0, d1, d2, d3, d4, d5) \
	ipi_trace(ev, d0, d1, d2, d3, d4, d5)
#define ipi_trace5(ev, d0, d1, d2, d3, d4) \
	ipi_trace(ev, d0, d1, d2, d3, d4, 0)
#define ipi_trace4(ev, d0, d1, d2, d3)	ipi_trace(ev, d0, d1, d2, d3, 0, 0)
#define ipi_trace3(ev, d0, d1, d2)	ipi_trace(ev, d0, d1, d2, 0, 0, 0)
#define ipi_trace2(ev, d0, d1)		ipi_trace(ev, d0, d1, 0, 0, 0, 0)
#define ipi_trace1(ev, d0)		ipi_trace(ev, d0, 0, 0, 0, 0, 0)

#else	/* TRACE */

/*
 * Neither kind of tracing is supported 
 */

#define pack(a, b)

#define ipi_trace	ipi_trace6
#define ipi_trace6(ev, d0, d1, d2, d3, d4, d5)
#define ipi_trace5(ev, d0, d1, d2, d3, d4)
#define ipi_trace4(ev, d0, d1, d2, d3)
#define ipi_trace3(ev, d0, d1, d2)
#define ipi_trace2(ev, d0, d1)
#define ipi_trace1(ev, d0)

#endif	/* TRACE */
#endif	/* KERNEL */

#endif /* ! _ipi_trace_h */
