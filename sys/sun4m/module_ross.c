#ident	"@(#)module_ross.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <machine/module.h>

extern u_int	swapl();
extern void	mmu_flushpagectx();
extern void	vac_pagectxflush();

#ifndef	MULTIPROCESSOR
#define	xc_attention()
#define	xc_dismissed()
#else	MULTIPROCESSOR
extern	xc_attention();
extern	xc_dismissed();
#endif	MULTIPROCESSOR

int	ross_test_mcr = 0;

#define	ROSS_VERSE
#ifdef	ROSS_VERSE
/*
 * If ROSS_VERSE is defined, ross_verse shows whether
 * we are running on a vers=0xE (A.3) Cy7C605.
 * If so, we must not use copyback mode.
 */
int	ross_verse = 0;
#endif

#define	ROSS_NOASI
#ifdef	ROSS_NOASI
#define	ROSS_VERSD
#ifdef	ROSS_VERSD
/*
 * If ROSS_VERSD is defined, ross_verse shows whether
 * we are running on a vers=0xD (A.5) Cy7C605.
 * If so, we must not use ASI-based flushes.
 */
int	ross_versd = 0;
#endif	ROSS_VERSD

/*
 * If ross_noasi is turned on,
 * avoid using ASI-based flushes.
 */
int	ross_noasi = 0;
#endif	ROSS_NOASI

#define	ROSS_VERSC

/*
 * Support for modules based on the Cypress CY604/CY605
 * memory management unit.
 */
#ifndef lint
extern void	ross_mmu_getasyncflt();
extern u_int	ross_pte_rmw();
#else	lint
void	ross_mmu_getasyncflt(){}
u_int	ross_pte_rmw(p,a,o) u_int *p,a,o;
{ return swapl((*p&~a)|o,p);}
#endif	lint
extern u_int	ross_pte_offon();
extern void	ross_module_wkaround();

#ifdef	MULTIPROCESSOR
extern u_int  (*mp_pte_offon)();
#endif	MULTIPROCESSOR

void
ross_vac_init()
{
	static int calls = 0;
	static u_int setbits = 0;
	static u_int clrbits = 0;
	extern int vac_copyback;
	extern char *vac_mode;
	extern void ross_vac_init_asm();
	extern u_int mmu_getcr();

	if (!calls++) {
#ifdef	ROSS_VERSE
		if (ross_verse) {
			printf("WARNING: Rev A.3 cache controller detected\n");
			printf("WORKAROUND: avoiding copyback cache mode\n");
		}
#endif	ROSS_VERSE
		if (vac_copyback)
			setbits = 0x400;
		else
			clrbits = 0x400;
	}
	ross_vac_init_asm(clrbits, setbits);
	if (mmu_getcr() & 0x400)
		vac_mode = "COPYBACK";
	else
		vac_mode = "WRITETHRU";
}
#ifndef lint
extern void	ross_vac_flushall();
extern void	ross_vac_usrflush();
extern void	ross_vac_ctxflush();
extern void	ross_vac_rgnflush();
extern void	ross_vac_segflush();
extern void	ross_vac_pageflush();
extern void	ross_vac_pagectxflush();
extern void	ross_vac_flush();
extern void	ross_cache_on();
#else	lint
void	ross_vac_init_asm(c,s) u_int c, s; { vac_copyback = c ^ s; }
void	ross_vac_flushall(){}
void	ross_vac_usrflush(){}
void	ross_vac_ctxflush(){}
void	ross_vac_rgnflush(){}
void	ross_vac_segflush(){}
void	ross_vac_pageflush(){}
void	ross_vac_pagectxflush(){}
void	ross_vac_flush(){}
void	ross_cache_on(){}
#endif	lint

void	/*ARGSUSED*/
ross_module_setup(mcr)
	register int	mcr;
{
	register int vers;
#ifdef	ROSS_NOASI
	extern void ross_na_module_setup();
#endif	ROSS_NOASI
	extern int	cpuid;

	if (ross_test_mcr) {
		prom_printf("ross_module_setup: pretending mcr is %x, not %x\n",
			ross_test_mcr, mcr);
		mcr = ross_test_mcr;
	}

	vers = (mcr >> 24) & 0xF;

	v_mmu_getasyncflt = ross_mmu_getasyncflt;

	v_vac_init = ross_vac_init;
	v_vac_flushall = ross_vac_flushall;
	v_vac_usrflush = ross_vac_usrflush;
	v_vac_ctxflush = ross_vac_ctxflush;
	v_vac_rgnflush = ross_vac_rgnflush;
	v_vac_segflush = ross_vac_segflush;
	v_vac_pageflush = ross_vac_pageflush;
	v_vac_pagectxflush = ross_vac_pagectxflush;
	v_vac_flush = ross_vac_flush;
	v_cache_on = ross_cache_on;

	v_pte_offon = ross_pte_offon;
	v_module_wkaround = ross_module_wkaround;
#ifdef	MULTIPROCESSOR
	mp_pte_offon = ross_pte_offon;
#endif	MULTIPROCESSOR

#ifdef	ROSS_VERSE
	if (vers == 0xE) {
		ross_verse = 1;
		vac_copyback = 0;
	} else
		ross_verse = 0;
#endif	ROSS_VERSE

#ifdef	ROSS_NOASI
#ifdef	ROSS_VERSD
	if (vers == 0xD) {
		ross_versd = 1;
		ross_noasi = 1;
	} else
		ross_versd = 0;
#endif	ROSS_VERSD
	if (ross_noasi)
		ross_na_module_setup(mcr);
#endif	ROSS_NOASI
}

#ifdef	ROSS_NOASI
/************************************************************************
 *	Ross "no-asi" support
 *
 *	replace flush entries with routines that avoid using
 *	the ASI-based flush code.
 */
void
ross_noasi_vac_init()
{
	static int calls = 0;
	if (!calls++) {
#ifdef	ROSS_VERSD
		if (ross_versd)
			printf("WARNING: Rev A.5 cache controller detected\n");
#endif	ROSS_VERSD
		printf("WORKAROUND: avoiding ASI-based cache flushes\n");
	}
	ross_vac_init();
}

#ifndef lint
extern void	ross_noasi_vac_usrflush();
extern void	ross_noasi_vac_ctxflush();
extern void	ross_noasi_vac_rgnflush();
extern void	ross_noasi_vac_segflush();
extern void	ross_noasi_vac_pageflush();
extern void	ross_noasi_vac_pagectxflush();
extern void	ross_noasi_vac_flush();
#else	lint
void	ross_noasi_vac_usrflush(){}
void	ross_noasi_vac_ctxflush(){}
void	ross_noasi_vac_rgnflush(){}
void	ross_noasi_vac_segflush(){}
void	ross_noasi_vac_pageflush(){}
void	ross_noasi_vac_pagectxflush(){}
void	ross_noasi_vac_flush(){}
#endif	lint

void		/*ARGSUSED*/
ross_na_module_setup(mcr)
	register int	mcr;
{

	v_vac_init = ross_noasi_vac_init;
	v_vac_usrflush = ross_noasi_vac_usrflush;
	v_vac_ctxflush = ross_noasi_vac_ctxflush;
	v_vac_rgnflush = ross_noasi_vac_rgnflush;
	v_vac_segflush = ross_noasi_vac_segflush;
	v_vac_pageflush = ross_noasi_vac_pageflush;
	v_vac_pagectxflush = ross_noasi_vac_pagectxflush;
	v_vac_flush = ross_noasi_vac_flush;
}
#endif	ROSS_NOASI

#include <machine/vm_hat.h>
#include <vm/as.h>

/*
 * ross_pte_offon: change some bits in a specified pte,
 * keeping the VAC and TLB consistant.
 *
 * This is the bottom level of manipulation for PTEs,
 * and it knows all the right things to do to flush
 * all the right caches. Further, it is safe in the face
 * of an MMU doing a read-modify-write cycle to set
 * the REF or MOD bits.
 */
unsigned
ross_pte_offon(ptpe, aval, oval)
	union ptpe	   *ptpe;
	unsigned	    aval;
	unsigned	    oval;
{
	unsigned	    rpte;
	unsigned	    wpte;
	unsigned	   *ppte;
	struct pte	   *pte;
	addr_t		    vaddr;
	struct ptbl	   *ptbl;
	struct as	   *as;
	struct ctx	   *ctx;
	u_int		    cno;

	/*
	 * If the ptpe address is NULL, we have nothing to do.
	 */
	if (!ptpe)
		return 0;

	ppte = &ptpe->ptpe_int;
	rpte = *ppte;
	wpte = (rpte & ~aval) | oval;

	/*
	 * Early Exit Options: If no change, just return.
	 */
	if (wpte == rpte)
		return rpte;

	/*
	 * Fast Update Options: If the old PTE was not valid,
	 * then no flush is needed: update and return.
	 */
	if ((rpte & PTE_ETYPEMASK) != MMU_ET_PTE)
		return ross_pte_rmw(ppte, aval, oval);

	pte = &ptpe->pte;
	ptbl = ptetoptbl(pte);
	vaddr = ptetovaddr(pte);

	/*
	 * Decide which context this mapping
	 * is active inside. If the vaddr is a
	 * kernel address, force context zero;
	 * otherwise, peel the address space and
	 * context used by the ptbl.
	 *
	 * Fast Update Option:
	 *	If there is no address space or
	 *	there is no context, then this
	 *	mapping must not be active, so
	 *	we just update and return.
	 */
	if ((unsigned)vaddr >= (unsigned)KERNELBASE)
		cno = 0;
	else if (((as = ptbl->ptbl_as) == NULL) ||
		 ((ctx = as->a_hat.hat_ctx) == NULL))
		return ross_pte_rmw(ppte, aval, oval);
	else
		cno = ctx->c_num;

	/*
	 * The Cy7C605 has this silly bug: if it takes a TLB hit on a
	 * translation that is clean while doing a write, and during
	 * the M-bit update cycle discovers that the PTE is invalid or
	 * unwritable, it keeps some "hidden state" that it needs to
	 * do an update cycle; on the next TLB walk, it writes
	 * whatever PTE it finds back after reading it, then does one
	 * junk request cycle to the MBus and sends trash back to the
	 * CPU. So, we can't allow any CPU to take a TLB hit on a
	 * valid writable clean TLB entry if the memory image of that
	 * PTE has been changed to be invalid or nonwritable.
	 *
	 * So, if the PTE starts out clean/valid/writable and we are
	 * invalidating or decreasing access, we must prevent the TLB
	 * hit by capturing the remote CPUs.
	 *
	 * XXX - for some reason, if we do not always capture, the
	 * system watchdogs on occasion. methinks there are more
	 * problems here than we thought.
	 */
	xc_attention();
	rpte = ross_pte_rmw(ppte, aval, oval);
	mmu_flushpagectx(vaddr, cno);
#ifdef	VAC
#define	REQUIRE_VAC_FLUSH	(PTE_PFN_MASK|PTE_CE_MASK|PTE_ETYPEMASK)
	if ((vac) &&				/* if using a VAC and */
	    (rpte & PTE_CE_MASK) &&		/*    page was cacheable and */
	    ((rpte & REQUIRE_VAC_FLUSH) !=	/*    we changed physical page, */
	     (wpte & REQUIRE_VAC_FLUSH)))	/*    decached, or demapped, */
		vac_pagectxflush (vaddr, cno);	/*   flush data from VAC */
#endif
	xc_dismissed();
	return rpte;
}

/*
 * At this time no module specific workaround for ROSS
 * Heh, Heh!!!
 */
void
ross_module_wkaround() {}
