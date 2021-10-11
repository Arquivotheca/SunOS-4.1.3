/* @(#)monitor.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

#ifndef _lwp_monitor_h
#define _lwp_monitor_h

/* definition of a monitor */
typedef struct monitor_t {
	qheader_t mon_waiting;		/* queue of waiting lwps */
	struct lwp_t *mon_owner;	/* lwp owning mon (LWPNULL if unowned) */
	struct monitor_t *mon_set;		/* prev mon in nested call */
	int mon_nestlevel;		/* # of times same lwp reenters mon */
	int mon_lock;			/* lock for validity check */
} monitor_t;
#define	MONNULL	((struct monitor_t *) 0)

/* for keeping list of all monitors in the system */
typedef struct monps_t {
	struct monps_t *monps_next;	/* next monitor in list */
	monitor_t *monps_monid;		/* monitor identity */
} monps_t;
#define MONPSNULL	((monps_t *) 0)

extern qheader_t __MonitorQ;	/* list of all monitors in the system */
extern void __init_mon();

#endif /*!_lwp_monitor_h*/
