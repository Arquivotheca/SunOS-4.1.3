#ifndef __COND_H__
#define __COND_H__

/* @(#)cond.h 1.1 92/07/30 */

/*
 * cond.h: interface to condition variable primatives
 *
 * We are prevented from using "cv_*" and "condvar_t" by the
 * existing SunOS 4.1 LWP code.
 */

#include <os/mutex.h>

/*
 * This structure is entirely private to the module os/kern_cond.c
 */

typedef struct s_cond {
	mutex_t         lock[1];	/* access lock */
	int             waits;		/* count of cond_wait calls */
	int             waiting;	/* count of sleeping cond_wait calls */
	int             sigs;		/* count of cond_signal calls */
	int             broads;		/* count of cond_broadcast calls */
}               cond_t;

void            cond_init();
int             cond_wait();
void            cond_signal();
void            cond_broadcast();

#endif	__COND_H__
