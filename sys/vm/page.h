/*	@(#)page.h 1.1 92/07/30 SMI 	*/


/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef _vm_page_h
#define	_vm_page_h
/*
 * VM - Ram pages.
 *
 * Each physical page has a page structure, which is used to maintain
 * these pages as a cache.  A page can be found via a hashed lookup
 * based on the [vp, offset].  If a page has an [vp, offset] identity,
 * then it is entered on a doubly linked circular list off the
 * vnode using the vpnext/vpprev pointers.   If the p_free bit
 * is on, then the page is also on a doubly linked circular free
 * list using next/prev pointers.  If the p_intrans bit is on,
 * then the page is currently being read in or written back.
 * In this case, the next/prev pointers are used to link the
 * pages together for a consecutive IO request.  If the page
 * is in transit and the the page is coming in (pagein), then you
 * must wait for the IO to complete before you can attach to the page.
 *
 */
struct page {
	u_int	p_lock: 1,		/* locked for name manipulation */
		p_want: 1,		/* page wanted */
		p_free: 1,		/* on free list */
		p_intrans: 1,		/* data for [vp, offset] intransit */
		p_gone: 1,		/* page has been released */
		p_mod: 1,		/* software copy of modified bit */
		p_ref: 1,		/* software copy of reference bit */
		p_pagein: 1,		/* being paged in, data not valid */
		p_nc: 1,		/* do not cache page */
		p_age: 1;		/* on age free list */
	u_int	p_nio : 6;		/* # of outstanding io reqs needed */
	u_short	p_keepcnt;		/* number of page `keeps' */
	struct	vnode *p_vnode;		/* logical vnode this page is from */
	u_int	p_offset;		/* offset into vnode for this page */
	struct	page *p_hash;		/* hash by [vnode, offset] */
	struct	page *p_next;		/* next page in free/intrans lists */
	struct	page *p_prev;		/* prev page in free/intrans lists */
	struct	page *p_vpnext;		/* next page in vnode list */
	struct	page *p_vpprev;		/* prev page in vnode list */
	caddr_t	p_mapping;		/* hat specific translation info */
	u_short	p_lckcnt;		/* number of locks on page data */
	u_short	p_pad;			/* steal bits from here */
};

/*
 * Each segment of physical memory is described by a memseg struct. Within
 * a segment, memory is considered contiguous. The segments from a linked
 * list to describe all of physical memory. The list is ordered by increasing
 * physical addresses.
 */
struct memseg {
	struct page *pages, *epages;	/* [from, to) in page array */
	u_int pages_base, pages_end;	/* [from, to) in page numbers */
	struct memseg *next;		/* next segment in list */
};

#ifdef KERNEL
#define	PAGE_HOLD(pp)	(pp)->p_keepcnt++
#define	PAGE_RELE(pp)	page_rele(pp)

#define	PAGE_HASHSZ	page_hashsz

extern	int page_hashsz;
extern	struct page **page_hash;

extern	struct	page *pages;		/* array of all page structures */
extern	struct	page *epages;		/* end of all pages */
extern	struct	memseg *memsegs;	/* list of memory segments */

/*
 * Variables controlling locking of physical memory.
 */
extern	u_int	pages_pp_locked;	/* physical pages actually locked */
extern	u_int	pages_pp_claimed;	/* physical pages reserved */
extern	u_int	pages_pp_maximum;	/* tuning: lock + claim <= max */

/*
 * Page frame operations.
 */
void	page_init(/* pp, num, base */);
void	page_reclaim(/* pp */);
struct	page *page_find(/* vp, off */);
struct	page *page_exists(/* vp, off */);
struct	page *page_lookup(/* vp, off */);
int	page_enter(/* pp, vp, off */);
void	page_abort(/* pp */);
void	page_free(/* pp */);
void	page_unfree(/* pp */);
struct	page *page_get();
void	page_rele(/* pp */);
void	page_lock(/* pp */);
void	page_unlock(/* pp */);
int	page_pp_lock(/* pp, claim, check_resv */);
void	page_pp_unlock(/* pp, claim */);
int	page_addclaim(/* claim */);
void	page_subclaim(/* claim */);
void	page_hashout(/* pp */);
void	page_add(/* ppp, pp */);
void	page_sub(/* ppp, pp */);
void	page_sortadd(/* ppp, pp */);
void	page_wait(/* pp */);
u_int	page_pptonum(/* pp */);
struct	page *page_numtopp(/* pfnum */);
struct	page *page_numtookpp(/* pfnum */);
#endif KERNEL

/*
 * Page hash table is a power-of-two in size, externally chained
 * through the hash field.  PAGE_HASHAVELEN is the average length
 * desired for this chain, from which the size of the page_hash
 * table is derived at boot time and stored in the kernel variable
 * page_hashsz.  In the hash function it is given by PAGE_HASHSZ.
 * PAGE_HASHVPSHIFT is defined so that 1 << PAGE_HASHVPSHIFT is
 * the approximate size of a vnode struct.
 */
#define	PAGE_HASHAVELEN		4
#define	PAGE_HASHVPSHIFT	6
#define	PAGE_HASHFUNC(vp, off) \
	((((off) >> PAGESHIFT) + ((int)(vp) >> PAGE_HASHVPSHIFT)) & \
		(PAGE_HASHSZ - 1))

/*
 * Macros for setting reference and modify bit values.  These exist as macros
 * so that tracing code has the opportunity to note the new values.
 */
#ifdef	TRACE

#ifdef	lint
#define	pg_setref(pp, val) \
	if (pp) { \
		trace2(TR_PG_SETREF, (pp), (val)); \
		(pp)->p_ref = (val); \
	} else
#define	pg_setmod(pp, val) \
	if (pp) { \
		trace2(TR_PG_SETMOD, (pp), (val)); \
		(pp)->p_mod = (val); \
	} else
#else	lint
#define	pg_setref(pp, val) \
	if (1) { \
		trace2(TR_PG_SETREF, (pp), (val)); \
		(pp)->p_ref = (val); \
	} else
#define	pg_setmod(pp, val) \
	if (1) { \
		trace2(TR_PG_SETMOD, (pp), (val)); \
		(pp)->p_mod = (val); \
	} else
#endif	lint

#else	TRACE

#define	pg_setref(pp, val)	(pp)->p_ref = (val)
#define	pg_setmod(pp, val)	(pp)->p_mod = (val)

#endif	TRACE

#endif /*!_vm_page_h*/
