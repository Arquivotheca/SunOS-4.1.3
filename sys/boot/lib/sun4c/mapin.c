#if !defined(lint) && !defined(BOOTBLOCK)
static char sccsid[] = "@(#)mapin.c 1.1 92/07/30 SMI";
#endif

#include <sys/types.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <machine/mmu.h>

/*
 * We must map in the devices ourselves on a Sun-4c.
 */

mapin(vaddr, paddr, pte, pmeg)
	caddr_t vaddr, paddr;
	u_int *pte;
	int *pmeg;
{
	int lopm, i, pm;

	*pmeg = pm = getsegmap(vaddr);
	/*
	 * If the EEPROM segment isn't mapped in, find a pmeg to use:
	 * assume the monitor uses the last pmegs (before INVALID). Find
	 * the first unused pmeg below the monitor's and borrow it.
	 */
	if (pm == PMGRP_INVALID) {
		lopm = PMGRP_INVALID;
		for (i = MONSTART; i < MONEND; i += PMGRPSIZE)
			if (((pm = getsegmap(i)) < lopm) && pm)
				lopm = pm;
		setsegmap(vaddr, --lopm);
	}

	*pte = getpgmap((caddr_t)vaddr);
	setpgmap((caddr_t)vaddr,
	    PG_V|PG_NC|PG_KW|PGT_OBIO|btop(paddr));
}
