#ifndef __MUTEX_H__
#define __MUTEX_H__

#ifndef	LOCORE

/* @(#)mutex.h 1.1 92/07/30 Copyright 1989 Sun Microsystems */

#include <sys/time.h>
#include <os/atom.h>

/*
 * mutex.h: interface to mutual exclusion primatives
 */

/*
 * The first 32 bits of this structure shall be a lock word
 * that is zero when the lock is free and nonzero when the
 * lock is busy.
 *
 * The remainder of the structure shall only be examined
 * by modules in "os/kern_mutex.c".
 */
typedef struct {
	atom_t		atom;		/* atom for free/busy state */
	int		missct;		/* ldstub's that missed */
	int 	        spl;		/* spl level for this mutex */
	int		psr;		/* saved psr for "splx" */
}               mutex_t;

void            mutex_init();
void            mutex_enter();
void            mutex_exit();
int             mutex_tryenter();

#endif	/* LOCORE */

#endif	__MUTEX_H__
