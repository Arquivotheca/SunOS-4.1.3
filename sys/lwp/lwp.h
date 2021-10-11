/* @(#)lwp.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

#ifndef _lwp_lwp_h
#define _lwp_lwp_h

#include <sys/types.h>
#include <sys/time.h>

/*
 * Definitions available to the client
 */

#define	MINPRIO		1			/* lowest lwp priority */


/* flags for thread options */
#define	LWPSUSPEND		0x1	/* create suspended */


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


#endif /*!_lwp_lwp_h*/
