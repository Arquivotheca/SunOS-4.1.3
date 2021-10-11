/* @(#) monitor.h 1.1 92/07/30 Copyr 1987 Sun Micro */
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

/* set monitor ownership */
#define	MON_SET(p, m)				\
{						\
	m->mon_owner = p;			\
	m->mon_set = p->lwp_curmon;		\
	p->lwp_curmon = m;			\
	m->mon_nestlevel = p->lwp_monlevel;	\
	p->lwp_monlevel = 0;			\
}

/* restore monitor ownership */
#define MON_RESTORE(p, m)			\
{						\
	p->lwp_curmon = m->mon_set;		\
	m->mon_set = MONNULL;			\
	m->mon_nestlevel = 0;			\
}

extern qheader_t __MonitorQ;	/* list of all monitors in the system */
extern void __moncleanup(/* corpse */);
extern void __init_mon();

#endif /*!_lwp_monitor_h*/
