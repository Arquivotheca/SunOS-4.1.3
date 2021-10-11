/*	@(#)mmu.c	1.1 92/07/30	SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * VM - Sun-3 low-level routines.
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
#include <vm/hat.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/seg_kmem.h>

#include <sundev/mbvar.h>

extern	struct ctx ctxs[];
extern	struct pmgrp pmgrps[];

struct	pte mmu_pteinvalid = { 0 };

static	void mmu_setkpmg(/* base, pmgnum */);

struct ctx *
mmu_getctx()
{

	return (&ctxs[sun_getcontrol_byte(CONTEXT_REG, (addr_t)0) & CTXMASK]);
}

void
mmu_setctx(ctx)
	struct ctx *ctx;
{

	sun_setcontrol_byte(CONTEXT_REG, (addr_t)0, ctx->c_num);
}

void
mmu_setpmg(base, pmg)
	addr_t base;
	struct pmgrp *pmg;
{

	sun_setcontrol_byte(SEGMENT_MAP, base, pmg->pmg_num);
	if (base >= (addr_t)KERNELBASE)
		(void) mmu_setkpmg(base, pmg->pmg_num);
}

void
mmu_settpmg(base, pmg)
	addr_t base;
	struct pmgrp *pmg;
{

	sun_setcontrol_byte(SEGMENT_MAP, base, pmg->pmg_num);
}

struct pmgrp *
mmu_getpmg(base)
	addr_t base;
{

	return (&pmgrps[sun_getcontrol_byte(SEGMENT_MAP, base)]);
}

void
mmu_setpte(base, pte)
	addr_t base;
	struct pte pte;
{

	sun_setcontrol_long(PAGE_MAP, base, *(u_int *)&pte);
}

void
mmu_getpte(base, ppte)
	addr_t base;
	struct pte *ppte;
{

	*(u_int *)ppte = sun_getcontrol_long(PAGE_MAP, base);
}

void
mmu_getkpte(base, ppte)
	addr_t base;
	struct pte *ppte;
{

	*(u_int *)ppte = sun_getcontrol_long(PAGE_MAP, base);
}

void
mmu_pmginval(base)
	addr_t base;
{

	sun_setcontrol_byte(SEGMENT_MAP, base, PMGRP_INVALID);
	if (base >= (addr_t)KERNELBASE)
		(void) mmu_setkpmg(base, PMGRP_INVALID);
}

/*
 * Copy pmgnum to all contexts but this one.
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

	my_c = sun_getcontrol_byte(CONTEXT_REG, (addr_t)0) & CTXMASK;
	for (c = 0; c < NCTXS; c++) 
		if (c != my_c) {
			sun_setcontrol_byte(CONTEXT_REG, (addr_t)0, c);
			sun_setcontrol_byte(SEGMENT_MAP, base, pmgnum);
		}
	sun_setcontrol_byte(CONTEXT_REG, (addr_t)0, my_c);
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
		my_c = sun_getcontrol_byte(CONTEXT_REG, (addr_t)0) & CTXMASK;
		for (c = 0; c < NCTXS; c++) {
			sun_setcontrol_byte(CONTEXT_REG, (addr_t)0, c);
			vac_ctxflush();
		}

		/*
		 * For all valid kernel pmgrps, flush them.
		 */
		for (v = (addr_t)KERNELBASE; v < (addr_t)CTXSIZE; 
		    v += PMGRPSIZE)
			if (mmu_getpmg(v) != pmgrp_invalid)
				vac_segflush(v);

		/*
		 * Restore starting context.
		 */
		sun_setcontrol_byte(CONTEXT_REG, (addr_t)0, my_c);
	}
}
#endif VAC
