/*	@(#)vm_rm.c	1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * VM - resource manager
 * As you can see, it needs lots of work
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/proc.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/rm.h>
#include <vm/seg.h>
#include <vm/page.h>

/*ARGSUSED*/
struct page *
rm_allocpage(seg, addr, len, canwait)
	struct seg *seg;
	addr_t addr;
	u_int len;
	int canwait;
{

	return (page_get(len, canwait));
}

/*
 * This routine is called when we couldn't allocate an anon slot.
 * For now, we simply print out a message and kill of the process
 * who happened to have gotten burned.
 *
 * XXX - swap reservation needs lots of work so this only happens in
 * `nice' places or we need to have a method to allow for recovery.
 */
void
rm_outofanon()
{
	struct proc *p;

	p = u.u_procp;
	printf("Sorry, pid %d (%s) was killed due to lack of swap space\n",
	    p->p_pid, u.u_comm);
	/*
	 * To be sure no looping (e.g. in vmsched trying to
	 * swap out) mark process locked in core (as though
	 * done by user) after killing it so noone will try
	 * to swap it out.
	 */
	psignal(p, SIGKILL);
	p->p_flag |= SULOCK;
	/*NOTREACHED*/
}

void
rm_outofhat()
{

	panic("out of mapping resources");			/* XXX */
	/*NOTREACHED*/
}

/*
 * Yield the memory claim requirement for an address space.
 *
 * This is currently implemented as the number of active hardware
 * translations that have page structures.  Therefore, it can
 * underestimate the traditional resident set size, eg, if the
 * physical page is present and the hardware translation is missing;
 * and it can overestimate the rss, eg, if there are active
 * translations to a frame buffer with page structs.
 * Also, it does not take sharing into account.
 */
int
rm_asrss(as)
	struct as *as;
{

	return (as == (struct as *)NULL ? 0 : as->a_rss);
}
