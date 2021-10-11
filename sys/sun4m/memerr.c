#ident "@(#)memerr.c 1.1 92/07/30 SMI"

/*LINTLIBRARY*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <machine/memerr.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <vm/page.h>
#include <sys/vnode.h>

extern void	enable_dvma();
extern void	disable_dvma();
extern void	hat_pagesync();
extern void	hat_pageunload();

int		report_ce_console = 0;	/* don't print messages on console */
int		report_ce = 1;
u_int		efervalue = 0x101;

/*
 * Memory error handling for sun4m
 */

memerr_init()
{
#ifndef SAS
	if (report_ce)		/* Enables ECC checking, reports on CEs */
		efervalue = efervalue | EFER_EE | EFER_EI;
	else			/* Else just turn on checking */
		efervalue = efervalue | EFER_EE;
#ifdef DEBUG
	printf("efer set to 0x%x\n", efervalue);
#endif
	*(u_int *) EFER_VADDR = *(u_int *)EFER_VADDR | efervalue;

#endif
}


int		correctable = 1;
u_int		testloc[2];

int
testecc()
{
	/*
	 * Try generating an ECC error by turning on the diagnostic register
	 */
	/* Assume uniprocessor and dvma has finished */
	/* Also assume cache is off */

	disable_dvma();
	if (correctable)
		*(u_int *) EFDR_VADDR = EFDR_GENERATE_MODE | 0x85;
	else
		*(u_int *) EFDR_VADDR = EFDR_GENERATE_MODE | 0x61;

	testloc[0] = 0x00000000;
	testloc[1] = 0x00000000;
#if 0
	vac_flush(&testloc[0], 4);
	vac_flush(&testloc[1], 4);
#endif
	*(u_int *) EFDR_VADDR = 0;
	enable_dvma();
	if ((testloc[0] != 0x80000000) && (testloc[1] != 0))
		u.u_r.u_rv.R_val1 = -1;
	else
		u.u_r.u_rv.R_val1 = 0;
}

int
fix_nc_ecc(efsr, efar0, efar1)
	u_int		efsr, efar0, efar1;
{
	/* Fix a non-correctable ECC error by "retiring" the page */

	u_int		pagenum;
	struct page    *pp;

	pagenum = (((efar0 & EFAR0_PA) << EFAR0_MID_SHFT) |
			((efar1 & EFAR1_PGNUM) >> 4));

	if (efsr & EFSR_ME) {
		printf("Multiple ECC Errors\n");
		return (-1);
	}
	if (efar0 & EFAR0_S) {
		printf("ECC error recovery: Supervisor mode\n");
		return (-1);
	}
	pp = (struct page *) page_numtopp(pagenum);
	if (pp = (struct page *) 0) {
		printf("Ecc error recovery: no page structure\n");
		return (-1);
	}
	if (pp->p_vnode == 0) {
		printf("ECC error recovery: no vnode\n");
		return (-1);
	}
	if (pp->p_intrans) {
		printf("Ecc error recovery: i/o in progress\n");
		return (-1);
	}
	hat_pagesync(pp);

	if (pp->p_mod && hat_kill_procs(pp, pagenum))
		return (-1);

	page_giveup(pagenum, pp);
	return (0);
}

page_giveup(pagenum, pp)
	u_int		pagenum;
	struct page    *pp;
{

	hat_pageunload(pp);
	page_abort(pp);
	if (pp->p_free)
		page_unfree(pp);
	pp->p_lock = 1;
	printf("page %x marked out of service.\n",
	    ptob(pagenum));
}
