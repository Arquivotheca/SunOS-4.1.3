/* @(#) process.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this source code without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * THIS PROGRAM CONTAINS SOURCE CODE COPYRIGHTED BY SUN MICROSYSTEMS, INC.
 * AND IS LICENSED TO SUNSOFT, INC., A SUBSIDIARY OF SUN MICROSYSTEMS, INC.
 * SUN MICROSYSTEMS, INC., MAKES NO REPRESENTATIONS ABOUT THE SUITABLITY
 * OF SUCH SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT
 * EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  SUN MICROSYSTEMS, INC. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO SUCH SOURCE CODE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
 * NO EVENT SHALL SUN MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM USE OF SUCH SOURCE CODE, REGARDLESS OF THE THEORY OF LIABILITY.
 * 
 * This source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction, 
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS
 * SOURCE CODE OR ANY PART THEREOF.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California 94043
 */
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
	struct msg_t lwp_tag;		/* message sent by this thread */
	qheader_t lwp_exchandles;	/* stack of exception handlers */
	qheader_t lwp_joinq;		/* threads awaiting this thread's death */
	int lwp_prio;			/* scheduling priority, higher favored */
	int lwp_flags;			/* LWPSUSPEND|LWPSERVER|LWPNOLASTRITES */
	int lwp_stack;			/* stack arg, passed to LASTRITES agt */
	int lwp_lock;			/* for validity checks */
	int lwp_cancel;			/* for kernel: TRUE if unwanted */
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
	(__Curproc->lwp_lwperrno != LE_NOERROR) 			\

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
		__HighPrio = prio;			\
		WAIT();					\
	} else {					\
		UNLOCK();				\
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
			ASSERT((dest) != __Curproc);				\
			INS_QUEUE(&__SuspendQ, dest);				\
			(dest)->lwp_run = RS_SUSPEND;				\
		} else {							\
			INS_QUEUE(&__RunQ[dest->lwp_prio], dest);		\
			__HighPrio = MAX(__HighPrio, (dest)->lwp_prio);		\
			(dest)->lwp_run = RS_UNBLOCKED;				\
		}								\
		(dest)->lwp_blockedon = NOBODY;					\
	}									\
	__Nrunnable++;								\
}

extern qheader_t __SuspendQ;	/* Queue of suspended processes */
extern qheader_t __SleepQ;	/* place to put processes sleeping on an alarm */
extern qheader_t __ZombieQ;	/* Queue of zombies (killed during send) */
extern int __LwpRunCnt;		/* Number of lwp's in the system */
extern thread_t __Slaves[];	/* list of server threads */
extern int __SlaveThreads;	/* index of free server thread */
extern int __NActiveSlaves;	/* count of living slave threads */
extern int __Nrunnable;		/* number of runnable threads */
extern void __freeproc(/* lwp */);	/* free lwp resources */
extern void __rem_corpse(/* queue, corpse, error */);	/* remove a dead lwp */
extern void __init_lwp();	/* initialize lwp library */
extern void __maincreate();	/* turn main into a l wp */

#endif /*!_lwp_process_h*/
