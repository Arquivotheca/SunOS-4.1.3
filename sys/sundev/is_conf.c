#ifndef lint
static	char sccsid[] = "@(#)is_conf.c	1.1 7/30/92 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Configuration dependent data structures for the Panther IPI string
 * controller channel driver.
 */

#include "is.h"
#include <sys/types.h>
#include <sundev/ipi_chan.h>
#include <sundev/isvar.h>


#if NISC > 0
struct is_ctlr		is_ctlr[NISC];
struct mb_device 	*iscinfo[NISC];
struct mb_ctlr		*isdinfo[NISC];
int			nisc = NISC;
#endif
