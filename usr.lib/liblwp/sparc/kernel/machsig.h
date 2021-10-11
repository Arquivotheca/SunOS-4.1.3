/* @(#) machsig.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
#ifndef __MACHSIG_H__
#define	__MACHSIG_H__

/* resolution of clock in us */
#define	CLOCKRES	10000

/* Number of events: add 1 for LASTRITES "soft" events */
#define	NUMSIG		(2)

typedef enum sigdstat_t {
	SIG_ERROR	=0,	/* error in processing event */
	SIG_CONT	=1,	/* try to get more events from this signal */
	SIG_DONE	=2,	/* no more events from this signal */
} sigdstat_t;
#define	TRAPEVENT	((sigdstat_t (*)()) 0)	/* trap (vs. interrupt) */

typedef struct sigdesc_t {
	sigdstat_t (*int_start)();	/* interrupt info gathering routine */
	int int_cookie;			/* info descriptor size */
} sigdesc_t;

extern void __init_sig();
extern sigdesc_t __Eventinfo[];

/*
 * Decrement the reference count of an event and free it if no longer in use
 * by an agent. Must be done while disasbled since event memory is allocated
 * at interrupt time. (AGTMSG memory may be allocated while DISABLED but NOT at
 * interrupt time, so LOCK can protect it).
 */
#define	RECLAIM(ev) {								\
	DISABLE();								\
	if (((ev) != EVENTNULL) && (--(ev)->event_refcnt == 0))			\
		FREECHUNK(__Eventinfo[(ev)->event_signum].int_cookie, (ev));	\
	ENABLE();								\
}

#endif __MACHSIG_H__
