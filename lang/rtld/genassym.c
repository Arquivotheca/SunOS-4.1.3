/* @(#)genassym.c 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Derive assembly language symbols from system header files.
 */

#include <sys/param.h>
#include <a.out.h>

main()
{

	printf("#define\tPAGESIZE\t0x%x\n", PAGESIZE);
	printf("#define\tPAGEMASK\t0x%x\n", PAGEMASK);
	printf("#define\tTXTOFF\t0x%x\n", sizeof (struct exec));
	exit(0);
}
