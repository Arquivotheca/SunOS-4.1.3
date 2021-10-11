/*	@(#)mp.h	1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _vm_mp_h
#define	_vm_mp_h

/*
 * VM - multiprocessor/ing support.
 *
 * Currently the kmon_enter() / kmon_exit() pair implements a
 * simple monitor for objects protected by the appropriate lock.
 * The kcv_wait() / kcv_broadcast pait implements a simple
 * condition variable which can be used for `sleeping'
 * and `waking' inside a monitor if some resource
 * is needed which is not available.
 */

typedef struct kmon_t {
	u_int	dummy;
} kmon_t;


#define	lock_init(lk)	(lk)->dummy = 0

#ifndef KMON_DEBUG
#define	kmon_enter(a)
#define	kmon_exit(a)
#define	kcv_wait(lk, cond)	(void) sleep(cond, PSWP+1)
#define	kcv_broadcast(lk, cond)	wakeup(cond)
#else
void	kmon_enter(/* lk */);
void	kmon_exit(/* lk */);
void	kcv_wait(/* lk, cond */);
void	kcv_broadcast(/* lk, cond */);
#endif /*!KMON_DEBUG*/
#endif /*!_vm_mp_h*/
