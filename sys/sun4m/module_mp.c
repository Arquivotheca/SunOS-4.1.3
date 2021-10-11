#ident	"@(#)module_mp.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <machine/module.h>

#ifndef	lint
extern void mp_mmu_flushall();
extern void mp_mmu_flushctx();
extern void mp_mmu_flushrgn();
extern void mp_mmu_flushseg();
extern void mp_mmu_flushpage();
extern void mp_mmu_flushpagectx();
#else
void mp_mmu_flushall(){}
void mp_mmu_flushctx(){}
void mp_mmu_flushpage(){}
void mp_mmu_flushpagectx(){}
void mp_mmu_flushrgn(){}
void mp_mmu_flushseg(){}
#endif

void	      (*s_mmu_flushall)() = 0;
void	      (*s_mmu_flushctx)() = 0;
void	      (*s_mmu_flushrgn)() = 0;
void	      (*s_mmu_flushseg)() = 0;
void	      (*s_mmu_flushpage)() = 0;
void	      (*s_mmu_flushpagectx)() = 0;

extern u_int	srmmu_mp_pte_offon();

u_int	      (*mp_pte_offon)() = srmmu_mp_pte_offon;
u_int	      (*s_pte_offon)() = 0;

#ifdef	VAC
#ifndef lint
extern void mp_vac_flushall();
extern void mp_vac_usrflush();
extern void mp_vac_ctxflush();
extern void mp_vac_rgnflush();
extern void mp_vac_segflush();
extern void mp_vac_pageflush();
extern void mp_vac_pagectxflush();
extern void mp_vac_flush();
#else
void mp_vac_flushall(){}
void mp_vac_usrflush(){}
void mp_vac_ctxflush(){}
void mp_vac_rgnflush(){}
void mp_vac_segflush(){}
void mp_vac_pageflush(){}
void mp_vac_pagectxflush(){}
void mp_vac_flush(){}
#endif

void	      (*s_vac_flushall)() = 0;
void	      (*s_vac_usrflush)() = 0;
void	      (*s_vac_ctxflush)() = 0;
void	      (*s_vac_rgnflush)() = 0;
void	      (*s_vac_segflush)() = 0;
void	      (*s_vac_pageflush)() = 0;
void	      (*s_vac_pagectxflush)() = 0;
void	      (*s_vac_flush)() = 0;
#endif	VAC

/*
 * Support for multiple processors
 */

mp_init()
{
#define	TAKE(name)	s_/**/name = v_/**/name; v_/**/name = mp_/**/name

	TAKE(mmu_flushall);
	TAKE(mmu_flushctx);
	TAKE(mmu_flushrgn);
	TAKE(mmu_flushseg);
	TAKE(mmu_flushpage);
	TAKE(mmu_flushpagectx);

	TAKE(pte_offon);

#ifdef	VAC
	TAKE(vac_flushall);
	TAKE(vac_usrflush);
	TAKE(vac_ctxflush);
	TAKE(vac_rgnflush);
	TAKE(vac_segflush);
	TAKE(vac_pageflush);
	TAKE(vac_pagectxflush);
	TAKE(vac_flush);
#endif
#undef	TAKE
}

#include <machine/pte.h>

/*
 * srmmu_mp_pte_offon: MP wrapper used when all you have
 * is a uniprocessor-save pte_offon routine. Small optimisation
 * included to shortcircuit if there is no net change.
 *
 * Note that this assumes that the update service routine
 * calls the proper MP distributed flush routines if it
 * needs to flush caches ...
 */
u_int
srmmu_mp_pte_offon(ptpe, aval, oval)
	register union ptpe     *ptpe;
	register u_int        aval;
	register u_int        oval;
{
	register u_int rpte;
	register u_int wpte;

	if (!ptpe)
		return 0;

	rpte = ptpe->ptpe_int;
	wpte = (rpte &~ aval) | oval;

	if (rpte == wpte)
		return rpte;

	xc_attention();
	rpte = (*s_pte_offon)(ptpe, aval, oval);
	xc_dismissed();
	return rpte;
}
