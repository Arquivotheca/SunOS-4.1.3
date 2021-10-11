#ifndef lint
static  char sccsid[] = "@(#)xy_conf.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include "xy.h"

/*
 * Config dependent data structures for the Xylogics 450 driver.
 */
#include <sys/param.h>
#include <sys/buf.h>
#include <sundev/mbvar.h>
#include <sun/dkio.h>	/* for DKIOCWCHK define, which affects size of xyunit */
#include <sundev/xyvar.h>

int nxy = NXY;			/* So the driver can use these defines */
int nxyc = NXYC;

/*
 * Declare relevant structures if the devices exist
 */

/*
 * Unit structures.
 */
#if NXY > 0
struct xyunit xyunits[NXY];
#endif NXY

/*
 * Controller structures.
 */
#if NXYC > 0
struct xyctlr xyctlrs[NXYC];
#endif NXYC

/*
 * Generic structures.
 */
#if NXYC > 0
struct	mb_ctlr *xycinfo[NXYC];
#endif NXYC

#if NXY > 0
struct	mb_device *xydinfo[NXY];
#endif NXY
