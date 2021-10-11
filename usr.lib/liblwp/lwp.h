/* @(#)lwp.h 1.1 92/07/30 */
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
#ifndef _lwp_lwp_h
#define _lwp_lwp_h

#include <sys/types.h>
#include <sys/time.h>

/*
 * Definitions available to the client
 */

#define	MINPRIO		1			/* lowest lwp priority */

#define	INFINITY	((struct timeval *)0)	/* infinite timeout */
extern struct timeval *__Poll;			/* polling timeout */
#define	POLL	__Poll

#define	CATCHALL	-2			/* catch any exception pattern */
#define	LASTRITES	32			/* thread termination agent */

/* flags for thread options */
#define	LWPSUSPEND		0x1	/* create suspended */
#define	LWPNOLASTRITES		0x2	/* don't deliver last rites on death */
#define	LWPSERVER		0x4	/* treat as server thread */

#define	AGENTMEMORY	256	/* maximum number of unreplied-to agent msgs */

/*
 * Receive any message: use ?: trick to allow side-effect so this
 * macro can behave like a function.
 */
#define	MSG_RECVALL(sender, abuf, asize, rbuf, rsize, timeout)		\
	(((sender)->thread_id = 0) == 0) ? 				\
	msg_recv((sender), (abuf), (asize), (rbuf), (rsize), (timeout)) : -1

/*
 * Types available to the client
 */

typedef struct thread_t {
	caddr_t thread_id;
	int thread_key;
} thread_t;
extern thread_t __ThreadNull;
#define	THREADNULL	__ThreadNull
#define	SELF		THREADNULL
#define	SAMETHREAD(t1, t2) (((t1).thread_id == (t2).thread_id) &&	\
	((t1).thread_key == (t2).thread_key))

typedef struct mon_t {
	caddr_t monit_id;
	int monit_key;
} mon_t;
#define	SAMEMON(m1, m2) (((m1).monit_id == (m2).monit_id) &&	\
	((m1).monit_key == (m2).monit_key))

typedef struct cv_t {
	caddr_t cond_id;
	int cond_key;
} cv_t;
#define	SAMECV(c1, c2) (((c1).cond_id == (c2).cond_id) &&	\
	((c1).cond_key == (c2).cond_key))

/* fixed-length header for agent messages */
typedef struct eventinfo_t {
	int eventinfo_signum;		/* identifies the particular event */
	int eventinfo_code;		/* parameter of certain signals */
	caddr_t eventinfo_addr;		/* address causing fault */
	thread_t eventinfo_victimid;	/* thread running when interrupted */
} eventinfo_t;

/* canonical way to do monitors (1 monitor per procedure) */
extern void mon_exit1();
#define	MONITOR(mid){						\
	(void) exc_on_exit(mon_exit1, (caddr_t)&(mid));		\
	(void) mon_enter(mid);					\
}

/*
 * Types of object a thread can block on
 */
typedef enum lwp_type_t {
	NO_TYPE		=0,	/* No object */
	CV_TYPE		=1,	/* Condition Variable */
	MON_TYPE	=2,	/* Monitor */
	LWP_TYPE	=3,	/* Thread or agent */
} lwp_type_t;

/*
 * Arbitrary lwp object: mon_t, cv_t, thread_t.
 */
typedef	struct anylwp_t {
	lwp_type_t any_kind;	/* type of object blocked on */
	union {
		cv_t		any_cv;		/* any_kind = CV_TYPE */
		mon_t		any_mon;	/* any_kind = MON_TYPE */
		thread_t	any_thread;	/* any_kind = LWP_TYPE */
	} any_object;
} anylwp_t;

typedef	struct statvec_t {
	int stat_prio;			/* scheduling priority */
	anylwp_t stat_blocked;		/* object a thread may be blocked on */
} statvec_t;
#define l_id	stat_blocked.any_object.any_thread.thread_id
#define l_key	stat_blocked.any_object.any_thread.thread_key
#define c_id	stat_blocked.any_object.any_cv.cond_id
#define c_key	stat_blocked.any_object.any_cv.cond_key
#define m_id	stat_blocked.any_object.any_mon.monit_id
#define m_key	stat_blocked.any_object.any_mon.monit_key

extern int exc_handle();
/*
 * routines that call exc_handle have strange control flow graphs,
 * since a call to a routine that calls exc_raise will eventually
 * return at the exc_handle site, not the original call site.  This
 * utterly wrecks control flow analysis.
 */
#pragma unknown_control_flow(exc_handle)

#ifdef mc68000
/*
* on the 68000, there is the additional problem that registers
* are restored to their values at the time of the call to exc_handle;
* they should be set to their values at the time of the call to
* whoever eventually called exc_raise.  Thus, on routines that call
* exc_handle, automatic register allocation must be suppressed.
*/
#pragma makes_regs_inconsistent(exc_handle)
#endif mc68000

#endif /*!_lwp_lwp_h*/
