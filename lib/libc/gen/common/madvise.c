/* @(#)madvise.c 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/mman.h>

/*
 * Function to provide advise to vm system to optimize it's
 *   characteristics for a particular application
 */

/*LINTLIBRARY*/
madvise(addr, len, advice)
	caddr_t addr;
	u_int len;
	int advice;
{

	return (mctl(addr, len, MC_ADVISE, advice));
}
