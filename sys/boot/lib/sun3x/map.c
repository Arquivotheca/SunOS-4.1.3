#ifndef lint
static  char sccsid[] = "@(#)map.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Additional memory mapping routines for use by standalone debugger.
 * The arg and return types are utter crocks.
 */

#include <sys/param.h>
#include <debug/debug.h>
#include <machine/vm_hat.h>

#define MON_DVMA_ADDR (0 - DVMA_MAP_SIZE)

getpgmap(addr)
	addr_t addr;
{
	int *pte;
	int index;

	if (addr < (addr_t)MAINMEM_MAP_SIZE) {
		pte = (int *)*romp->v_lomemptaddr;
		index = mmu_btop(addr);
	} else if (addr >= (addr_t)DEBUGSTART && addr < (addr_t)MONEND) {
		pte = (int *)*romp->v_monptaddr;
		index = mmu_btop(addr - (addr_t)DEBUGSTART);
	} else if (addr >= (addr_t)MON_DVMA_ADDR) {
		pte = (int *)*romp->v_dvmaptaddr;
		index = mmu_btop(addr - (addr_t)MON_DVMA_ADDR);
	} else
		return (0);
	return(pte[index]);
}

setpgmap(addr, val)
	addr_t addr;
	u_int val;
{
	int *pte;
	int index;

	if (addr < (addr_t)MAINMEM_MAP_SIZE) {
		pte = (int *)*romp->v_lomemptaddr;
		index = mmu_btop(addr);
		pte[index] = val;
	} else if (addr >= (addr_t)DEBUGSTART && addr < (addr_t)MONEND) {
		pte = (int *)*romp->v_monptaddr;
		index = mmu_btop(addr - (addr_t)DEBUGSTART);
		pte[index] = val;
	} else if (addr >= (addr_t)MON_DVMA_ADDR) {
		pte = (int *)*romp->v_dvmaptaddr;
		index = mmu_btop(addr - (addr_t)MON_DVMA_ADDR);
		pte[index] = val;
		/*
		 * Update the shadow copy too.
		 */
		pte = (int *)*romp->v_shadowpteaddr;
		pte[index] = val;
	} else
		return;
	atc_flush();
}
