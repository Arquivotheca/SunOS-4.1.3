/* @(#)trace.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

#ifndef _lwp_trace_h
#define _lwp_trace_h

/* ex. usage: LWPTRACE(NUGGET, 3, ("message")); */
#ifdef LDEBUG
#define LWPTRACE(flag, lev, mesg) 				\
	if ((__Trace[(int)flag] >= lev)) {			\
		DISABLE(); 					\
		(void) printf mesg;				\
		ENABLE(); 					\
	}
#else
#define LWPTRACE(flag, lev, mesg)
#endif LDEBUG

/* debugging flags for the LWPTRACE macro */
typedef enum flag_type_t {
	tr_ABORT	=0,
	tr_EXCEPT	=1,
	tr_USER		=2,
	tr_SCHEDULE	=3,
	tr_PROCESS	=4,
	tr_ASM		=5,
	tr_ASYNCH	=6,
	tr_MESSAGE	=7,
	tr_SELECT	=8,
	tr_AGENT	=9,
	tr_CLOCK	=10,
	tr_CONDVAR	=11,
	tr_MONITOR	=12,
	tr_ALLOC	=13,
} flag_type_t;
#define MAXFLAGS	14		/* size of __Trace vector */

extern int __Trace[];

#endif /*!_lwp_trace_h*/
