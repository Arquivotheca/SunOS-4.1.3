/*	@(#)prom_handler.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

/*
 * XXX: The function types and args are different depending on romvec version
 */

int
prom_sethandler(v0_func, v2_func)
	void (*v0_func)();
	void (*v2_func)();
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		if (v0_func == 0)  {
			prom_printf("NOTICE: No OBP V0 CB handler registered!");
			return (-1);
		}
		OBP_CB_HANDLER = v0_func;
		return (0);

	default:
		if (v2_func == 0)  {
			prom_printf("NOTICE: No OBP V2 CB handler registered!");
			return (-1);
		}
		OBP_CB_HANDLER = v2_func;
		return (0);
	}
}
