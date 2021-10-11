#ifndef __BATON_H__
#define __BATON_H__

#ifndef	LOCORE

/* @(#)baton.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 * The first 32 bits of this structure shall be a cookie
 * that represents the owning processor.
 *
 * The second 32 bits of this structure shall be the nesting
 * depth at which the lock is held, where zero represents a
 * free lock.
 *
 * The remainder of the structure is private to os/kern_lock.c
 */

#include <os/cond.h>

typedef void	      (*who_t)();
typedef unsigned	why_t;

#define	ANY_WHO	((who_t)0xFFFFFFFF)
#define	ANY_WHY	((why_t)0xFFFFFFFF)

typedef struct {
	int             owner;		/* owning processor */
	int             depth;		/* nested lock depth */
	mutex_t         access[1];	/* baton access lock */
	cond_t          waiting[1];	/* waiting procs */
}               baton_t;

void	baton_init();
void	baton_lock();
void	baton_free();
void	baton_spin();
int	baton_chklock();
int	baton_intlock();
void	baton_intfree();
void	baton_swtch();
void	baton_require();

#endif	/* LOCORE */

#endif	 __BATON_H__
