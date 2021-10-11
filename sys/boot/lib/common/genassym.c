#ifndef lint
static  char sccsid[] = "@(#)genassym.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <mon/sunromvec.h>

main()
{
	printf("#define\tINITGETKEY 0x%x\n", &romp->v_initgetkey);
	exit(0);
}

