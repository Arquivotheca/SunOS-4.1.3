/* @(#) process.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

#ifndef _lwp_process_h
#define _lwp_process_h

/*
 * Which queue or state a thread belongs to (if not on any queue)
 * A thread never belongs to more than one queue.
 */
typedef enum lwp_state_t {
	RS_UNBLOCKED	=0,	/* Runnable (__RunQ) */
	RS_SEND		=1,	/* Blocked on a send (__SendQ) */
	RS_RECEIVE	=2,	/* Blocked on a receive (__ReceiveQ) */
	RS_SLEEP	=3,	/* Sleeping for finite time (__SleepQ) */
	RS_MONITOR	=4,	/* Blocked on a monitor (__MonitorQ) */
	RS_CONDVAR	=5,	/* Blocked on a condition var (__CondvarQ) */
	RS_SUSPEND	=6,	/* Suspended (__SuspendQ) */
	RS_ZOMBIE	=7,	/* Dead but memory kept around until msg reply */
	RS_JOIN		=8,	/* Blocked on a join */
	RS_UNINIT	=9,	/* Not on any queue, uninitialized */
} lwp_state_t;

/*
 * Context block for lightweight process.
 */
typedef struct lwp_t {
	struct lwp_t *lwp_next;		/* list of processes blocked together */
	object_type_t lwp_type;		/* type (agt/lwp) (MUST BE 2'nd) */
	machstate_t lwp_machstate;	/* machine-dependent state */
	qheader_t lwp_ctx;		/* special context save/restore actions */
	bool_t lwp_full;		/* TRUE if rti resume, else coroutine */
	bool_t lwp_suspended;		/* TRUE if to be suspended */
	enum lwp_state_t lwp_run;	/* which queue holds the process */
	enum lwp_err_t lwp_lwperrno;	/* errors in primitives */
	qheader_t lwp_messages;		/* list of unreceived messages */
	caddr_t lwp_blockedon; 		/* entity waited upon */
	struct monitor_t *lwp_curmon;	/* currently-held monitor */
	int lwp_monlevel;		/* nesting level of reentrant monitor */
	qheader_t lwp_exchandles;	/* stack of exception handlers */
	qheader_t lwp_joinq;		/* threads awaiting this thread's death */
	int lwp_prio;			/* scheduling priority, higher favored */
	int lwp_flags;			/* LWPSUSPEND|LWPSERVER|LWPNOLASTRITES */
	int lwp_stack;			/* stack arg, passed to LASTRITES agt */
	int lwp_lock;			/* for validity checks */
	int lwp_cancel;			/* for kernel: TRUE if unwanted */
	caddr_t lwp_ref;		/* o.s. reference key (owner proc) */
} lwp_t;

#define	LWPNULL ((lwp_t *) 0)
#define	NOBODY	CHARNULL	/* lwp_blockedon: not waiting on any object */

/*
 * Manipulate client error setting/clearing
 */
#define	SET_ERROR(proc, err)	{					\
	if ((proc != LWPNULL)) {					\
		(proc)->lwp_lwperrno = err;				\
	}								\
}

#define	IS_ERROR()							\
	__Curproc ? (__Curproc->lwp_lwperrno != LE_NOERROR) :		\
		(panic("Curproc is null"), 0)

#define	CLR_ERROR()	{						\
	if ((__Curproc != LWPNULL)) {					\
		__Curproc->lwp_lwperrno = LE_NOERROR;			\
	}								\
}

#define	NUGPRIO		0	/* idle priority (no other lwp this low) */

/*
 * Conditionally give up the processor if "prio" warrants it.
 * Always UNLOCKED when it returns, either via WAIT or UNLOCK.
 */
#define	YIELD_CPU(prio) {				\
	if (prio > __Curproc->lwp_prio) {		\
		WAIT();					\
	}						\
}

/*
 * Remove the specified process from the run
 * queue and set it's new state:
 * which queue it will be blocked on and which object can unblock it
 * (if applicable).
 */
#define	BLOCK(pro, state, waiton)	{				\
	REMX_ITEM(&__RunQ[pro->lwp_prio], pro);				\
	pro->lwp_run = state;						\
	pro->lwp_blockedon = (caddr_t)waiton;				\
	__Nrunnable--;							\
}

/*
 * Put a process back on the run queue. lwp_run prevents
 * a wakeup from unblocking an unblocked process.
 * Do this AFTER deueueing from whatever queue the process was on before.
 * If the (blocked) process is suspended, we now alter its status
 * to the suspended state (and __HighPrio remains the same since no new
 * runnable process was added).
 */
#define	UNBLOCK(dest) {								\
	if ((dest)->lwp_run != RS_UNBLOCKED) {					\
		if ((dest)->lwp_suspended) {					\
			INS_QUEUE(&__SuspendQ, dest);				\
			(dest)->lwp_run = RS_SUSPEND;				\
		} else {							\
			INS_QUEUE(&__RunQ[dest->lwp_prio], dest);		\
			(dest)->lwp_run = RS_UNBLOCKED;				\
		}								\
		(dest)->lwp_blockedon = NOBODY;					\
	}									\
	__Nrunnable++;								\
}

extern qheader_t __SuspendQ;	/* Queue of suspended processes */
extern int __LwpRunCnt;		/* Number of lwp's in the system */
extern int __Nrunnable;		/* number of runnable threads */
extern void __freeproc();	/* free lwp resources */
extern void __init_lwp();	/* initialize lwp library */

#endif /*!_lwp_process_h*/
