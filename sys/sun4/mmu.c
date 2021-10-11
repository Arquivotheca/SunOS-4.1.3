#ifndef lint
static	char sccsid[] = "@(#)mmu.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
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

#ifdef	MMU_3LEVEL
extern	struct smgrp *smgrps;
#endif	MMU_3LEVEL

extern	u_int map_getpgmap();
extern	u_int map_getsgmap();
extern	u_int map_getctx();

struct	pte mmu_pteinvalid = { 0 };

static	void mmu_setkpmg(/* base, pmgnum */);

#ifdef MMU_3LEVEL
static	void mmu_setksmg(/* base, smgnum */);
#endif MMU_3LEVEL

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

#ifdef MMU_3LEVEL
void
mmu_setsmg(base, smg)
	addr_t base;
	struct smgrp *smg;
{

	if (base >= (addr_t)KERNELBASE)
		mmu_setksmg(base, smg->smg_num);
	else
		map_setrgnmap(base, smg->smg_num);
}

void
mmu_settsmg(base, smg)
	addr_t base;
	struct smgrp *smg;
{

	map_setrgnmap(base, smg->smg_num);
}

struct smgrp *
mmu_getsmg(base)
	addr_t base;
{

	return (&smgrps[map_getrgnmap(base)]);
}

void
mmu_smginval(base)
	addr_t base;
{

	/*
	 * Make sure user windows are flushed before
	 * possibly taking a mapping away.
	 */
	flush_user_windows();
	if (base >= (addr_t)KERNELBASE)
		mmu_setksmg(base, SMGRP_INVALID);
	else
		map_setrgnmap(base, SMGRP_INVALID);
}

/*
 * Copy smgnum to all contexts.
 * Keeps kernel up-to-date in all contexts.
 */
static void
mmu_setksmg(base, smgnum)
	addr_t base;
	u_int smgnum;
{
	register u_int c;
	register u_int my_c;

	if (base < (addr_t)KERNELBASE)
		panic("mmu_setksmg");

	flush_user_windows();		/* flush before changing ctx */
	my_c = map_getctx();
	for (c = 0; c < NCTXS; c++) {
		map_setctx(c);
		map_setrgnmap(base, smgnum);
	}
	map_setctx(my_c);
}
#endif MMU_3LEVEL

void
mmu_setpmg(base, pmg)
	addr_t base;
	struct pmgrp *pmg;
{
	if (pmg->pmg_num == PMGNUM_SW)
		panic("pmg_num == PMGNUM_SW");

#ifdef	MMU_3LEVEL
	if (base >= (addr_t)KERNELBASE && !mmu_3level)
#else	MMU_3LEVEL
	if (base >= (addr_t)KERNELBASE)
#endif	MMU_3LEVEL
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
	struct pmgrp	*retval;

	retval = hwpmgs[map_getsgmap(base)].hwp_pmgrp;

	if (retval == NULL) {
		panic("mmu_getpmg");
	}

	return (retval);
}

void
mmu_setpte(base, pte)
	addr_t base;
	struct pte pte;
{

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
#ifdef	MMU_3LEVEL
	if (base >= (addr_t)KERNELBASE && !mmu_3level)
#else	MMU_3LEVEL
	if (base >= (addr_t)KERNELBASE)
#endif	MMU_3LEVEL
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

#ifdef VAC
/*
 * Flush the entire VAC.  Flush all user contexts and flush all valid
 * kernel segments.  It is only used by dumpsys() before we dump
 * the entire physical memory.
 */
void
vac_flushall()
{
	register u_int c;
	register u_int my_c;
	register addr_t v;

	/*
	 * Do nothing if no VAC active.
	 */
	if (vac) {
		/*
		 * Save starting context, step over all contexts,
		 * flushing them.
		 */
		flush_user_windows();		/* flush before changing ctx */
		my_c = map_getctx();

#ifdef MMU_3LEVEL
		if (mmu_3level)
			vac_usrflush();
		else
#endif MMU_3LEVEL
		{
			for (c = 0; c < NCTXS; c++) {
				map_setctx(c);
				vac_ctxflush();
			}
		}

		/*
		 * For all valid kernel pmgrps, flush them.
		 */
#ifdef MMU_3LEVEL
		if (mmu_3level) {
			for (v = (addr_t)KERNELBASE; v != NULL; v += SMGRPSIZE)
				if (mmu_getsmg(v) != smgrp_invalid)
					vac_rgnflush(v);
		} else
#endif MMU_3LEVEL
		{
			for (v = (addr_t)KERNELBASE; v != NULL; v += PMGRPSIZE)
				if (mmu_getpmg(v) != pmgrp_invalid)
					vac_segflush(v);
		}

		/*
		 * Restore starting context.
		 */
		map_setctx(my_c);
	}
}
#endif VAC
