/*	@(#)anon.h	1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _vm_anon_h
#define	_vm_anon_h

/*
 * VM - Anonymous pages.
 */

/*
 * Each page which is anonymous, either in memory or in swap,
 * has an anon structure.  The structure's primary purpose is
 * to hold a reference count so that we can detect when the last
 * copy of a multiply-referenced copy-on-write page goes away.
 * When on the free list, un.next gives the next anon structure
 * in the list.  Otherwise, un.page is a ``hint'' which probably
 * points to the current page.  This must be explicitly checked
 * since the page can be moved underneath us.  This is simply
 * an optimization to avoid having to look up each page when
 * doing things like fork.
 */
struct anon {
	int	an_refcnt;
	union {
		struct	page *an_page;	/* ``hint'' to the real page */
		struct	anon *an_next;	/* free list pointer */
	} un;
};

struct anoninfo {
	u_int	ani_max;	/* maximum anon pages available */
	u_int	ani_free;	/* number of anon pages currently free */
	u_int	ani_resv;	/* number of anon pages reserved */
};

#ifdef KERNEL
/*
 * Flags for anon_private.
 */
#define	STEAL_PAGE	0x01	/* page can be stolen */
#define	LOCK_PAGE	0x02	/* page must be ``logically'' locked */

extern	struct anoninfo anoninfo;

struct	anon *anon_alloc();
void	anon_dup(/* old, new, size */);
void	anon_free(/* app, size */);
int	anon_getpage(/* app, protp, pl, sz, seg, addr, rw, cred */);
struct	page *anon_private(/* app, seg, addr, opp, oppflags */);
struct	page *anon_zero(/* seg, addr, app */);
void	anon_unloadmap(/* ap, ref, mod */);
int	anon_resv(/* size */);
void	anon_unresv(/* size */);
#endif KERNEL

#endif /*!_vm_anon_h*/
