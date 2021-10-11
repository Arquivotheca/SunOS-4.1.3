/* @(#)condvar.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

#ifndef _lwp_condvar_h
#define _lwp_condvar_h

/* definition of a condition variable */
typedef struct condvar_t {
	qheader_t cv_waiting;	/* queue of lwps awaiting condition */
	struct monitor_t *cv_mon;	/* which monitor owns this condition */
	int cv_lock;		/* lock for validity check */
	caddr_t cv_cm;		/* back ptr to cm list */
} condvar_t;
#define	CVNULL	((struct condvar_t *) 0)

/* for keeping track of all cv's in the system */
typedef struct cvps_t {
	struct cvps_t *cvps_next;	/* next in list */
	condvar_t *cvps_cvid;		/* id of the cv (its address) */
} cvps_t;
#define	CVPSNULL	((struct cvps_t *) 0)

extern qheader_t __CondvarQ;		/* list of all cv's in the system */
extern int cv_destroy();
extern void __init_cv();

#endif /*!_lwp_condvar_h*/
