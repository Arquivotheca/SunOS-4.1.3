/*	@(#)kern_proc.c 1.1 92/07/30 SMI; from UCB 7.1 6/5/86	*/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <machine/reg.h>
#include <machine/pte.h>
#include <machine/psl.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/acct.h>
#include <sys/wait.h>
#include <sys/vm.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/mbuf.h>

struct	proc *
pfind(pid)
	register pid;
{
	register struct proc *p;

	for (p = &proc[pidhash[PIDHASH(pid)]];
	    p != &proc[0]; p = &proc[p->p_idhash]) {
		if (p->p_pid == pid)
			return (p);
	}
	return ((struct proc *)0);
}

/*
 * init the process queues
 */
pqinit()
{
	register struct proc *p;

#ifndef	MULTIPROCESSOR
	/*
	 * most procs are initially on freequeue
	 *	nb: we place them there in their "natural" order.
	 */

	freeproc = NULL;
	for (p = procNPROC; --p > proc; freeproc = p)
		p->p_nxt = freeproc;

	/*
	 * but proc[0] is special ...
	 */

	allproc = p;
	p->p_nxt = NULL;
	p->p_prev = &allproc;
#else MULTIPROCESSOR
	/*
	 * all procs are initially on freequeue
	 */
	freeproc = NULL;
	for (p = procNPROC; p-- > proc; freeproc = p)
		p->p_nxt = freeproc;
	allproc = NULL;
#endif MULTIPROCESSOR
	zombproc = NULL;
}

/*
 * insert p into the pgrp hash table on pgrp.
 *
 * N.B. p->p_pgrp must match pgrp now or soon.
 *	p->p_pgrp may not change without moving the process in this table.
 */
void
pgenter(p, pgrp)
	register struct proc *p;
{
	register i = PGRPHASH(pgrp);

	p->p_pglnk = pgrphash[i];
	pgrphash[i] = p;
}

/*
 * remove p from the pgrp hash table
 */
void
pgexit(p)
	register struct proc *p;
{
	register struct proc *pp, *follow;
	int i = PGRPHASH(p->p_pgrp);

	for (pp = pgrphash[i]; pp != p;  pp=pp->p_pglnk) {
	    if (!pp) {
		printf("%d not in pgrp hash table\n", p->p_pid);
		panic("pgexit");
	    }
	    follow = pp;
	}
	if (pp == pgrphash[i])
	    pgrphash[i] = pp->p_pglnk;
	else
	    follow->p_pglnk = p->p_pglnk;
	p->p_pglnk = NULL;	/* paranoia */
}

/*
 * find the first process in hash table with p_pgrp == pgrp
 */
struct	proc*
pgfind(pgrp)
	register pgrp;
{
	register struct proc *p;

	p = pgrphash[PGRPHASH(pgrp)];
	while (p != NULL && p->p_pgrp != pgrp)
		p = p->p_pglnk;
	return (p);
}

/*
 * return the next process in hash table with p_pgrp == p->p_pgrp
 *
 * XXX - this really should have a lock on the pgrphash slot.
 */
struct	proc*
pgnext(p)
	register struct proc *p;
{
	register pgrp;

	pgrp = p->p_pgrp;
	for (p = p->p_pglnk; p && p->p_pgrp != pgrp; p = p->p_pglnk)
		;
	return (p);
}
