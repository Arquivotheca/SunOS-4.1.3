#ifndef lint
static  char sccsid[] = "@(#)genassym.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sun4/pte.h>
#include <sun4/psl.h>
#include <sun4/mmu.h>
#include <sun4/cpu.h>
#include <mon/sunromvec.h>
#include "allregs.h"
#include "../../debug/debug.h"

main()
{
	register struct allregs *rp = (struct allregs *)0;

	printf("#define\tR_PSR %d\n", &rp->r_psr);
	printf("#define\tR_PC %d\n", &rp->r_pc);
	printf("#define\tR_NPC %d\n", &rp->r_npc);
	printf("#define\tR_TBR %d\n", &rp->r_tbr);
	printf("#define\tR_WIM %d\n", &rp->r_wim);
	printf("#define\tR_Y %d\n", &rp->r_y);
	printf("#define\tR_G1 %d\n", &rp->r_globals[0]);
	printf("#define\tR_G2 %d\n", &rp->r_globals[1]);
	printf("#define\tR_G3 %d\n", &rp->r_globals[2]);
	printf("#define\tR_G4 %d\n", &rp->r_globals[3]);
	printf("#define\tR_G5 %d\n", &rp->r_globals[4]);
	printf("#define\tR_G6 %d\n", &rp->r_globals[5]);
	printf("#define\tR_G7 %d\n", &rp->r_globals[6]);
	printf("#define\tR_WINDOW %d\n", rp->r_window);
	printf("#define\tPSR_PIL_BIT %d\n", bit(PSR_PIL));
	printf("#define\tPG_S_BIT %d\n", bit(PG_S));
	printf("#define\tROMP_MEMAVAIL 0x%x\n", &romp->v_memoryavail);
	printf("#define\tMONSTART 0x%x\n", MONSTART);
	printf("#define\tMONEND 0x%x\n", MONEND);
	printf("#define\tDEBUGSTART 0x%x\n", DEBUGSTART);
	printf("#define\tDEBUGEND 0x%x\n", DEBUGEND);
	exit(0);
}

bit(mask)
	register long mask;
{
	register int i;

	for (i = 0; i < 32; i++) {
		if (mask & 1)
			return (i);
		mask >>= 1;
	}
	exit (1);
}
