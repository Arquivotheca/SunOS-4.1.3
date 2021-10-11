#ifndef lint
static  char sccsid[] = "@(#)genassym.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <machine/reg.h>
#include <mon/sunromvec.h>
#include <mon/keyboard.h>

main()
{
	register struct regs *rp = (struct regs *)0;

	printf("#define\tCACR_CLEAR 0x%x\n", 0x8);
	printf("#define\tCACR_ENABLE 0x%x\n", 0x1);
	printf("#define\tV_TRANSLATION 0x%x\n", &romp->v_translation);
	printf("#define\tR_SP 0x%x\n", &rp->r_sp);
	printf("#define\tR_SR 0x%x\n", ((int)&rp->r_sr) + sizeof (short));
	exit(0);
}

