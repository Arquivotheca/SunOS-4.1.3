/*	@(#)vm_hat.h  1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * VM - Hardware Address Translation management.
 *
 * This file describes the contents of the machine specific
 * hat data structures and the machine specific hat procedures.
 * The machine independent interface is described in <vm/hat.h>.
 */

#ifndef _sun_vm_hat_h
#define	_sun_vm_hat_h

struct ctx {
	u_int	c_lock: 1;
	u_int	c_clean: 1;		/* this ctx has no lines in cache */
	u_int	: 6;
	u_char	c_num;			/* hardware info (context number) */
	u_short	c_time;			/* pseudo-time for ctx lru */
	struct	as *c_as;		/* back pointer to address space */
};

#ifdef MMU_3LEVEL
struct smgrp {
	u_int	smg_lock: 1;
	u_int	smg_num: 15;		/* hardware info (smgrp #) */
	u_short	smg_keepcnt;		/* `keep' count */
	struct	as *smg_as;		/* address space using this smgrp */
	addr_t	smg_base;		/* base address within address space */
	struct	smgrp *smg_next;	/* next smgrp for this as */
	struct	smgrp *smg_prev;	/* previous smgrp for this as */
	struct	sment *smg_sme;		/* ptr to sment array for this smgrp */
};

struct sment {
	struct	pmgrp *sme_pmg;		/* what pmeg this maps */
	int	sme_valid;
};
#endif /* MMU_3LEVEL */

struct pmgrp {
	u_int	pmg_lock: 1;
	u_int	pmg_num: 15;		/* hardware info (pmgrp #) */
	u_short	pmg_keepcnt;		/* `keep' count */
	struct	as *pmg_as;		/* address space using this pmgrp */
	addr_t	pmg_base;		/* base address within address space */
	struct	pmgrp *pmg_next;	/* next pmgrp for this as */
	struct	pmgrp *pmg_prev;	/* previous pmgrp for this as */
	struct	pment *pmg_pme;		/* ptr to pment array for this pmgrp */
	struct	sment *pmg_sme;		/* ptr to the sment for this pmgrp */
};

struct pment {
	struct	page *pme_page;		/* what page this maps */
	u_short	pme_next;		/* other pments mapping same page */
	u_short	pme_intrep : 1;		/* interrupt replaceable flag */
	u_short	pme_nosync : 1;		/* ghost unload flag */
	u_short	pme_valid : 1;		/* pme loaded flag */
};

/*
 * These macros manipulate the next index of the pme struct.
 */
#define	PMENXT_PTR(x)		((x) == PMENXT_NULL ? \
				(struct pment *)NULL : &pments[(x)])
#define	PMENXT_INDEX(x)		((struct pment *)(x) == (struct pment *)NULL ? \
				PMENXT_NULL : (struct pment *)(x) - pments)
#define	PMENXT_NULL		0xFFFF

/*
 * The hat structure is the machine dependent hardware address translation
 * structure kept in the address space structure to show the translation.
 *
 * We maintain that any time an ctx structure exists,
 * that all pmgrps on the hat are loaded into the hardware.
 */
struct hat {
	struct	ctx *hat_ctx;		/* hardware ctx info (if any) */
	struct	smgrp *hat_smgrps;	/* smgrps belonging to this hat */
	struct	pmgrp *hat_pmgrps;	/* pmgrps belonging to this hat */
};

/*
 * Flags to pass to hat_pteunload() and hat_ptesync().
 */
#define	HAT_NOSYNC	0x00
#define	HAT_RMSYNC	0x01
#define	HAT_NCSYNC	0x02
#define	HAT_INVSYNC	0x04
#define	HAT_VADDR	0x08

/*
 * Flags to pass to hat_pteload() and segkmem_mapin().
 */
#define	PTELD_LOCK	0x01
#define	PTELD_INTREP	0x02
#define	PTELD_NOSYNC	0x04
#define	PTELD_IOCACHE	0x08

#ifdef KERNEL
/*
 * For the Sun MMU hat code, we don't need to do any initialization at
 * hat creation time since we know that the structure is guaranteed to be
 * all zeros.  So we can define the machine independent interface routine
 * hat_alloc() to be a nop here (<vm/hat.h> is set up to deal with this).
 */
#define	hat_alloc(as)

/*
 * These routines are all MACHINE-SPECIFIC interfaces to the hat routines.
 * These routines are called from machine specific places to do the
 * dirty work for things like boot initialization, mapin/mapout and
 * first level fault handling.
 */

/*
 * Boot time pmgrp initialization routines
 */
void	hat_pmginit();
void	hat_pmgreserve(/* seg, addr */);

#ifdef MMU_3LEVEL
void	hat_smginit();
void	hat_smgreserve(/* seg, addr */);
extern	struct smgrp *smgrp_invalid;
#else
#define		hat_smginit()
#define		hat_smgreserve(seg, addr)
#endif /* MMU_3LEVEL */

/*
 * Allocate a ctx for the specified as.
 */
void	hat_getctx(/* as */);

/*
 * Reserve hat resources for the address range. In this implementation, this
 * means allocating and locking the pmegs. This currently only works with
 * kernel addresses.
 */
void	hat_reserve(/* seg, addr, len */);

/*
 * Called by UNIX during pagefault to insure a context is allocated
 * for this address space and that the ctx time is updated.  Also
 * used internally before all operations involving the translation
 * hardware if you need to be in the correct context for the operation.
 * Done inline for performance reasons.
 */
#ifndef sun2
#define	hat_setup(x) { \
	register struct ctx *ctx = (x)->a_hat.hat_ctx; \
	extern u_short ctx_time; \
	if (ctx != kctx) { \
		if (ctx == NULL) { \
			hat_getctx(x); \
		} else { \
			ctx->c_time = ctx_time++; \
			mmu_setctx(ctx); \
		} \
	} \
	}
#else sun2
#define	hat_setup(x) { \
	register struct ctx *ctx = (x)->a_hat.hat_ctx; \
	extern u_short ctx_time; \
	if (ctx == NULL) { \
		hat_getctx(x); \
	} else { \
		ctx->c_time = ctx_time++; \
		mmu_setctx(ctx); \
	} \
	}
#endif sun2

/*
 * Load up a particular translation
 */
void	hat_pteload(/* seg, addr, pp, pte, lock */);

/*
 * Convert from mman protections to a protection value to be loaded in a pte
 */
u_int	hat_vtop_prot(/* vprot */);

/*
 * Construct a memory pte using the page pointer and mman protections specified
 */
void	hat_mempte(/* pp, vprot, ppte */);

#ifdef sun4c
/*
 * Flush all translations to a given physical page from the cache.
 */
void	hat_vacsync(/* pfnum */);
#endif sun4c

extern	struct pmgrp *pmgrp_invalid;
#ifdef MMU_3LEVEL
extern	struct smgrp *smgrp_invalid;
#endif /* MMU_3LEVEL */
#endif KERNEL

#endif /*!_sun_vm_hat_h*/
