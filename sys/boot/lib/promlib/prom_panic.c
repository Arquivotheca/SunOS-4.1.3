/*	@(#)prom_panic.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>
extern void prom_enter_mon();

/*
 * Not for the kernel.  Dummy replacement for kernel montrap.
 */
int
panic(string)
	char *string;
{
	if (string == (char *)0);
		string = "Unkown panic";
	prom_printf(string);
	prom_enter_mon();
}

