#ident "@(#)mmu.c 1.1 92/07/30 SMI"

/*
 * Copyright 1987-1989 Sun Microsystems, Inc.
 */

/*
 * VM - Sun-4 low-level routines.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/vm.h>
#include <sys/mman.h>
#include <sys/kernel.h>
#include <sys/map.h>

#include <vm/page.h>
#include <vm/seg.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/vm_hat.h>
#include <machine/seg_kmem.h>

#include <sundev/mbvar.h>

extern	struct ctx *ctxs;
extern	struct pmgrp *pmgrps;
extern  struct hwpmg *hwpmgs;

extern	u_int map_getpgmap();
extern	u_int map_getsgmap();
extern	u_int map_getctx();

struct	pte mmu_pteinvalid = { 0 };

static void mmu_setkpmg(/* base, pmgnum */);

struct ctx *
mmu_getctx()
{

	return (&ctxs[map_getctx()]);
}

void
mmu_setctx(ctx)
	struct ctx *ctx;
{

	/*
	 * We must make sure that there are no user windows
	 * in the register file when we switch contexts.
	 * Otherwise the flushed windows will go to the
	 * wrong place.
	 */
	flush_user_windows();
	map_setctx(ctx->c_num);
}

void
mmu_setpmg(base, pmg)
	addr_t base;
	struct pmgrp *pmg;
{
	if (pmg->pmg_num == PMGNUM_SW)
		panic("pmg_num == PMGNUM_SW");

	if (base >= (addr_t)KERNELBASE)
		mmu_setkpmg(base, pmg->pmg_num);
	else
		map_setsgmap(base, pmg->pmg_num);
}

void
mmu_settpmg(base, pmg)
	addr_t base;
	struct pmgrp *pmg;
{

	map_setsgmap(base, pmg->pmg_num);
}

struct pmgrp *
mmu_getpmg(base)
	addr_t base;
{
	struct pmgrp    *retval;

	retval = hwpmgs[map_getsgmap(base)].hwp_pmgrp;

	if (retval == NULL)
		panic("mmu_getpmg");

	return (retval);
}

void
mmu_setpte(base, pte)
	addr_t base;
	struct pte pte;
{
	extern void map_setpgmap();
	/*
	 * Make sure user windows are flushed before
	 * possibly taking a mapping away.
	 */
	if (!pte_valid(&pte))
		flush_user_windows();
	map_setpgmap(base, *(u_int *)&pte);
}

void
mmu_getpte(base, ppte)
	addr_t base;
	struct pte *ppte;
{

	*(u_int *)ppte = map_getpgmap(base);
}

void
mmu_getkpte(base, ppte)
	addr_t base;
	struct pte *ppte;
{

	*(u_int *)ppte = map_getpgmap(base);
}

void
mmu_pmginval(base)
	addr_t base;
{

	/*
	 * Make sure user windows are flushed before
	 * possibly taking a mapping away.
	 */
	flush_user_windows();
	if (base >= (addr_t)KERNELBASE)
		mmu_setkpmg(base, PMGRP_INVALID);
	else
		map_setsgmap(base, PMGRP_INVALID);
}

/*
 * Copy pmgnum to all contexts.
 * Keeps kernel up-to-date in all contexts.
 */
static void
mmu_setkpmg(base, pmgnum)
	addr_t base;
	u_int pmgnum;
{
	register u_int c;
	register u_int my_c;

	if (base < (addr_t)KERNELBASE)
		panic("mmu_setkpmg");

	flush_user_windows();		/* flush before changing ctx */
	my_c = map_getctx();
	for (c = 0; c < NCTXS; c++) {
		map_setctx(c);
		map_setsgmap(base, pmgnum);
	}
	map_setctx(my_c);
}
