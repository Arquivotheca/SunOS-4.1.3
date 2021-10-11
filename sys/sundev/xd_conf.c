#ifndef lint
static	char sccsid[] = "@(#)xd_conf.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include "xd.h"

/*
 * Config dependent data structures for the Xylogics 751 driver.
 */
#include <sys/param.h>
#include <sys/buf.h>
#include <sundev/mbvar.h>
#include <sun/dkio.h>	/* for DKIOCWCHK define, which affects size of xyunit */
#include <sundev/xdvar.h>
#include <machine/cpu.h>

int nxd = NXD;			/* So the driver can use these defines */
int nxdc = NXDC;

/*
 * Declare relevant structures if the devices exist
 */

/*
 * Unit structures.
 */

#if NXD > 0
struct xdunit xdunits[NXD];
#endif NXD

/*
 * Controller structures.
 */

#if NXDC > 0
struct xdctlr xdctlrs[NXDC];
struct xd_w_info xd_watch_info[NXDC];
#endif NXDC

/*
 * Generic structures.
 */

#if NXDC > 0
struct	mb_ctlr *xdcinfo[NXDC];
#endif NXDC

#if NXD > 0
struct	mb_device *xddinfo[NXD];
#endif NXD

/*
 * These shorts define the controller parameters the 7053 will get set with.
 *
 * xd_ctlrpar0 is a short containing the bits for iopb bytes 0x8 and 0x9
 * (which is the sector count field under normal operations)
 *
 * xd_ctlrpar1 is a short containing the bits for iopb byte 0xa. The driver
 * will shift this by 8, and or it into the xd_throttle field of the iopb
 * (also the cylinder field).
 *
 * see the file xdreg.h for the definitions of these bits.
 */

u_short xd_ctlrpar0 = XD_LWRD|XD_DACF|(xd_tdtv(1))|XD_ROR;

/*
 * This *must* be a short, as it will be shifted by 8 to be folded into the
 * iopb throttle variable.
 */

u_short xd_ctlrpar1 = XD_OVS|XD_ASR|XD_RBC|XD_ECC2;

/*
 * These are the values put into byte 0xd for write controller command
 * The last entry in the list must have the first value of null.  This
 * last entry is used if no others fit.
 */

struct xd_ctlrpar2 xd_ctlrpar2[] = {
#if defined(sun3)
/* sun3 requires a delay of 20 */
	{ 0, 20 }
#endif
#if defined(CPU_SUN4_330)
	{ CPU_SUN4_330, 4 },
#endif
	{ 0, 0}
};
