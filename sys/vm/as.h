/*	@(#)as.h 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef _vm_as_h
#define	_vm_as_h

#include <vm/faultcode.h>

/*
 * VM - Address spaces.
 */

/*
 * Each address space consists of a list of sorted segments
 * and machine dependent address translation information.
 *
 * All the hard work is in the segment drivers and the
 * hardware address translation code.
 */
struct as {
	u_int	a_lock: 1;
	u_int	a_want: 1;
	u_int	a_paglck: 1;	/* lock mappings into address space */
	u_int	a_ski:  1;	/* enables recording of page info for ski */
	u_int	a_hatcallback:  1; /* enables recording of page info for ski */
	u_int	: 11;
	u_short	a_keepcnt;	/* number of `keeps' */
	struct	seg *a_segs;	/* segments in this address space */
	struct	seg *a_seglast;	/* last segment hit on the address space */
	int	a_rss;		/* memory claim for this address space */
	struct	hat a_hat;	/* hardware address translation */
};

#ifdef KERNEL
/*
 * Types of failure for several various address space operations.
 */
enum as_res {
	A_SUCCESS,		/* operation successful */
	A_BADADDR,		/* illegal address encountered */
	A_OPFAIL,		/* segment operation failure */
	A_RESOURCE,		/* resource exhaustion */
};

/*
 * Flags for as_hole.
 */
#define	AH_DIR		0x1	/* direction flag mask */
#define	AH_LO		0x0	/* find lowest hole */
#define	AH_HI		0x1	/* find highest hole */
#define	AH_CONTAIN	0x2	/* hole must contain `addr' */

/*
 * Flags for as_hatsync
 */
#define	AHAT_UNLOAD	0x01	/* Translation being unloaded */

struct	seg *as_segat(/* as, addr */);
struct	as *as_alloc();
void	as_free(/* as */);
struct	as *as_dup(/* as */);
enum	as_res as_addseg(/* as, seg */);
faultcode_t as_fault(/* as, addr, size, type, rw */);
faultcode_t as_faulta(/* as, addr, size */);
enum	as_res as_setprot(/* as, addr, size, prot */);
enum	as_res as_checkprot(/* as, addr, size, prot */);
enum	as_res as_unmap(/* as, addr, size */);
int	as_map(/* as, addr, size, crfp, crargsp */);
enum	as_res as_hole(/* as, minlen, basep, lenp, flags, addr */);
enum	as_res as_memory(/* as, addrp, sizep */);
u_int	as_swapout(/* as */);
enum	as_res as_incore(/* as, addr, size, vecp, sizep */);
enum	as_res as_ctl(/* as, addr, size, func, arg */);
void	as_hatsync(/* as, addr, ref, mod, flags */);
#endif KERNEL
#endif /*!_vm_as_h*/
