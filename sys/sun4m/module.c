#ident	"@(#)module.c 1.1 92/07/30 SMI"

#include <sys/param.h>
#include <machine/module.h>
#include <machine/cpu.h>
#include <machine/mmu.h>

/* Generic pointer to specific routine related to modules */

/*
 * Module_setup is called from locore.s, very early.  It will look
 * at the implementation and version number of the module, and decide
 * which routines to use for modules specific tasks. Note that the
 * relation between impl/vers and the routine to be called is encoded
 * in a table in module_conf.c, which can be distributed in source
 * form if we wish and is intended to make porting to new modules
 * a lot easier -- think about "module drivers".
 */

extern void	mmu_flushpagectx();
extern void	vac_pagectxflush();

#ifdef	NOPROM
/* FIXME: should be replaced by cpuinfo */
/* note: autoconf also uses this, a little. */

/*
 * If we don't have a prom (some SAS runs are done
 * like this, before fakeproms are made available
 * for the platform being simulated) we need to figure
 * out what our "module_type" is.
 */
extern int	module_type;

#endif	NOPROM

#ifndef lint
extern int	srmmu_mmu_getcr();
extern int	srmmu_mmu_getctp();
extern int	srmmu_mmu_getctx();
extern int	srmmu_mmu_probe();
extern void	srmmu_mmu_setcr();
extern void	srmmu_mmu_setctp();
extern void	srmmu_mmu_setctx();
extern void	srmmu_mmu_flushall();
extern void	srmmu_mmu_flushctx();
extern void	srmmu_mmu_flushrgn();
extern void	srmmu_mmu_flushseg();
extern void	srmmu_mmu_flushpage();
extern void	srmmu_mmu_flushpagectx();
extern void	srmmu_mmu_getsyncflt();
extern void	srmmu_mmu_getasyncflt();
extern void	srmmu_mmu_chk_wdreset();
extern void	srmmu_mmu_sys_ovf();
extern void	srmmu_mmu_sys_unf();
extern void	srmmu_mmu_wo();
extern void	srmmu_mmu_wu();
#else
int	srmmu_mmu_getcr(){ return 0; }
int	srmmu_mmu_getctp(){ return 0; }
int	srmmu_mmu_getctx(){ return 0; }
int	srmmu_mmu_probe(){ return 0; }
void	srmmu_mmu_setcr(){}
void	srmmu_mmu_setctp(){}
void	srmmu_mmu_setctx(){}
void	srmmu_mmu_flushall(){}
void	srmmu_mmu_flushctx(){}
void	srmmu_mmu_flushrgn(){}
void	srmmu_mmu_flushseg(){}
void	srmmu_mmu_flushpage(){}
void	srmmu_mmu_flushpagectx(){}
void	srmmu_mmu_getsyncflt(){}
void	srmmu_mmu_getasyncflt(){}
void	srmmu_mmu_chk_wdreset(){}
void	srmmu_mmu_sys_ovf(){}
void	srmmu_mmu_sys_unf(){}
void	srmmu_mmu_wo(){}
void	srmmu_mmu_wu(){}
#endif
extern void	srmmu_mmu_log_module_err();
extern void	srmmu_mmu_print_sfsr();
extern u_int	srmmu_pte_offon();
extern void	module_no_wkaround();

int		(*v_mmu_getcr)() = srmmu_mmu_getcr;
int		(*v_mmu_getctp)() = srmmu_mmu_getctp;
int		(*v_mmu_getctx)() = srmmu_mmu_getctx;
int		(*v_mmu_probe)() = srmmu_mmu_probe;
void		(*v_mmu_setcr)() = srmmu_mmu_setcr;
void		(*v_mmu_setctp)() = srmmu_mmu_setctp;
void		(*v_mmu_setctx)() = srmmu_mmu_setctx;
void		(*v_mmu_flushall)() = srmmu_mmu_flushall;
void		(*v_mmu_flushctx)() = srmmu_mmu_flushctx;
void		(*v_mmu_flushrgn)() = srmmu_mmu_flushrgn;
void		(*v_mmu_flushseg)() = srmmu_mmu_flushseg;
void		(*v_mmu_flushpage)() = srmmu_mmu_flushpage;
void		(*v_mmu_flushpagectx)() = srmmu_mmu_flushpagectx;
void		(*v_mmu_getsyncflt)() = srmmu_mmu_getsyncflt;
void		(*v_mmu_getasyncflt)() = srmmu_mmu_getasyncflt;
void		(*v_mmu_chk_wdreset)() = srmmu_mmu_chk_wdreset;
void		(*v_mmu_sys_ovf)() = srmmu_mmu_sys_ovf;
void		(*v_mmu_sys_unf)() = srmmu_mmu_sys_unf;
void		(*v_mmu_wo)() = srmmu_mmu_wo;
void		(*v_mmu_wu)() = srmmu_mmu_wu;
void		(*v_mmu_log_module_err)() = srmmu_mmu_log_module_err;
void		(*v_mmu_print_sfsr)() = srmmu_mmu_print_sfsr;

u_int		(*v_pte_offon)() = srmmu_pte_offon;
void		(*v_module_wkaround)() = module_no_wkaround;
#ifdef	VAC
#ifndef	lint
extern void	vac_noop();
extern int	vac_inoop();
#else
void	vac_noop(){}
int	vac_inoop(){ return 0; }
#endif

void		(*v_vac_init)() = vac_noop;
void		(*v_vac_flushall)() = vac_noop;
void		(*v_vac_usrflush)() = vac_noop;
void		(*v_vac_ctxflush)() = vac_noop;
void		(*v_vac_rgnflush)() = vac_noop;
void		(*v_vac_segflush)() = vac_noop;
void		(*v_vac_pageflush)() = vac_noop;
void		(*v_vac_pagectxflush)() = vac_noop;
void		(*v_vac_flush)() = vac_noop;
void		(*v_cache_on)() = vac_noop;
int		(*v_vac_parity_chk_dis)() = vac_inoop;
#endif	VAC

void
module_setup(mcr)
	int	mcr;
{
	extern int module_info_size;
	int	i = module_info_size;
	struct module_linkage *p = module_info;

#ifdef	NOPROM
	register int    mt = mcr >> 24;

/*
 * Collapse 604-f into 604, and 605-b into 605.
 */
	if (mt == ROSS604f)
		mt = ROSS604;

	if (mt == ROSS605b)
		mt = ROSS605;

	module_type = mt;
#endif	NOPROM

#ifdef	lint
	v_mmu_getcr = v_mmu_getcr;
	v_mmu_getctp = v_mmu_getctp;
	v_mmu_getctx = v_mmu_getctx;
	v_mmu_probe = v_mmu_probe;
	v_mmu_setcr = v_mmu_setcr;
	v_mmu_setctp = v_mmu_setctp;
	v_mmu_setctx = v_mmu_setctx;
	v_mmu_flushall = v_mmu_flushall;
	v_mmu_flushctx = v_mmu_flushctx;
	v_mmu_flushrgn = v_mmu_flushrgn;
	v_mmu_flushseg = v_mmu_flushseg;
	v_mmu_flushpage = v_mmu_flushpage;
	v_mmu_flushpagectx = v_mmu_flushpagectx;
	v_mmu_getsyncflt = v_mmu_getsyncflt;
	v_mmu_getasyncflt = v_mmu_getasyncflt;
	v_mmu_chk_wdreset = v_mmu_chk_wdreset;
	v_mmu_sys_ovf = v_mmu_sys_ovf;
	v_mmu_sys_unf = v_mmu_sys_unf;
	v_mmu_wo = v_mmu_wo;
	v_mmu_wu = v_mmu_wu;
	v_mmu_log_module_err = v_mmu_log_module_err;
	v_mmu_print_sfsr = v_mmu_print_sfsr;

	v_pte_offon = v_pte_offon;

#ifdef	VAC
	v_vac_init = v_vac_init;
	v_vac_flushall = v_vac_flushall;
	v_vac_usrflush = v_vac_usrflush;
	v_vac_ctxflush = v_vac_ctxflush;
	v_vac_rgnflush = v_vac_rgnflush;
	v_vac_segflush = v_vac_segflush;
	v_vac_pageflush = v_vac_pageflush;
	v_vac_pagectxflush = v_vac_pagectxflush;
	v_vac_flush = v_vac_flush;
	v_cache_on = v_cache_on;
	v_vac_parity_chk_dis = v_vac_parity_chk_dis;
#endif	/* VAC */
#endif	/* lint */

	while (i-->0) {
		if ((mcr & p->mcr_mask) == p->mcr_chk)
			(*(p->setup_func))(mcr);
		++p;
	}
}

void
srmmu_mmu_log_module_err(afsr, afar0)
	u_int afsr;	/* Asynchronous Fault Status Register */
	u_int afar0;	/* Asynchronous Fault Address Register */
{
	if (afsr & MMU_AFSR_BE)
		printf("\tM-Bus Bus Error\n");
	if (afsr & MMU_AFSR_TO)
		printf("\tM-Bus Timeout Error\n");
	if (afsr & MMU_AFSR_UC)
		printf("\tM-Bus Uncorrectable Error\n");
	if (afsr & MMU_AFSR_AFV)
		printf("\tPhysical Address = (space %x) %x\n",
			(afsr&MMU_AFSR_AFA)>>MMU_AFSR_AFA_SHIFT, afar0);
}

void
srmmu_mmu_print_sfsr(sfsr)
	u_int	sfsr;
{
	printf("MMU sfsr=%x:", sfsr);
	switch (sfsr & MMU_SFSR_FT_MASK) {
	case MMU_SFSR_FT_NO: printf (" No Error"); break;
	case MMU_SFSR_FT_INV: printf (" Invalid Address"); break;
	case MMU_SFSR_FT_PROT: printf (" Protection Error"); break;
	case MMU_SFSR_FT_PRIV: printf (" Privilege Violation"); break;
	case MMU_SFSR_FT_TRAN: printf (" Translation Error"); break;
	case MMU_SFSR_FT_BUS: printf (" Bus Access Error"); break;
	case MMU_SFSR_FT_INT: printf (" Internal Error"); break;
	case MMU_SFSR_FT_RESV: printf (" Reserved Error"); break;
	default: printf (" Unknown Error"); break;
	}
	if (sfsr) {
		printf (" on %s %s %s at level %d",
			    sfsr & MMU_SFSR_AT_SUPV ? "supv" : "user",
			    sfsr & MMU_SFSR_AT_INSTR ? "instr" : "data",
			    sfsr & MMU_SFSR_AT_STORE ? "store" : "fetch",
			    (sfsr & MMU_SFSR_LEVEL) >> MMU_SFSR_LEVEL_SHIFT);
		if (sfsr & MMU_SFSR_BE)
			printf ("\n\tM-Bus Bus Error");
		if (sfsr & MMU_SFSR_TO)
			printf ("\n\tM-Bus Timeout Error");
		if (sfsr & MMU_SFSR_UC)
			printf ("\n\tM-Bus Uncorrectable Error");
	}
	printf("\n");
}

#include <machine/vm_hat.h>
#include <vm/as.h>

/*
 * srmmu_pte_offon: change some bits in a specified pte,
 * keeping the VAC and TLB consistant.
 *
 * This version is correct for uniprocessors that implement
 * the SPARC Reference MMU. It assumes that the cache can
 * do a copyback during a flush without a mapping available,
 * which is true for the Cy7C605 (for instance) but not
 * for the Cy7C604 (for instance). The alternative is to
 * reverse the order of the update, mmu flush and vac flush,
 * which would fail in the case of demapping a page that was
 * referenced between the flushing and the update.
 */
u_int
srmmu_pte_offon (ptpe, aval, oval)
	register union ptpe *ptpe;
	register u_int      aval;
	register u_int      oval;
{
	register u_int     *ppte;
	register u_int      rpte;
	register u_int      wpte;

	if (!ptpe)					/* if no pte, */
		return 0;				/*   nothing to do. */

	ppte = &(ptpe->ptpe_int);
	rpte = *ppte;					/* read original value */
	wpte = (rpte & ~aval) | oval;			/* calculate new value */

	if (rpte == wpte)				/* if no change, */
		return rpte;				/*   all done. */

	*ppte = wpte;					/* update the PTE */

	if ((rpte & PTE_ETYPEMASK) != MMU_ET_PTE)	/* if it was not valid, */
		return rpte;				/*   no flushing needed. */

	{	/* Yilch. All this to get the right context number??? */
		register addr_t     vaddr = ptetovaddr(&ptpe->pte);
		register struct as *as = ptetoptbl(&ptpe->pte)->ptbl_as;
		register struct ctx *cxp = as ? as->a_hat.hat_ctx : 0;
		register u_int      ctx = cxp ? cxp->c_num : 0;

		mmu_flushpagectx (vaddr, ctx);		/* flush mapping from TLB */
#ifdef	VAC
#define	REQUIRE_VAC_FLUSH	(PTE_PFN_MASK|PTE_CE_MASK|PTE_ETYPEMASK)
		if ((vac) &&				/* if using a VAC and */
		    (rpte & PTE_CE_MASK) &&		/*    page was cacheable and */
		    ((rpte & REQUIRE_VAC_FLUSH) !=	/*    we changed physical page, */
		     (wpte & REQUIRE_VAC_FLUSH)))	/*    decached, or demapped, */
			vac_pagectxflush (vaddr, ctx);	/*   flush data from VAC. */
#endif
	}

	return rpte;
}

/*
 * No  workarounds for modules by default.
 * If there are any, rearrange the vectors
 */
void
module_no_wkaround() {}
