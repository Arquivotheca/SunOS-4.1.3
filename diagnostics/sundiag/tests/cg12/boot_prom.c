
#ifndef lint
static  char sccsid[] = "@(#)boot_prom.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <esd.h>

/**********************************************************************/
char *
boot_prom()
/**********************************************************************/

{
    int result;

    if(!open_gp()) {
	return(errmsg_list[1]);
    }

    result = post_diag_test(GP1_DIAG, BPROM_TEST, 0, 0, 0, 0, 0, 0, 0, 0);
    if (result) {
	return(errmsg_list[18]);
    }

    return(char *)0;
}
