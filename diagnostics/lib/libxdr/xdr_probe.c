#ifndef lint
static  char sccsid[] = "@(#)xdr_probe.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include "probe_sundiag.h"

/*
 * xdr_devinfo XDR encodes/decodes the gendev_info struct
 */
xdr_devinfo(xdrs, pd)
    XDR *xdrs;
    struct gendev_info *pd;
{
    if (!xdr_int(xdrs, &pd->status))
	return(0);
    return(1);
}

/*
 * xdr_meminfo XDR encodes/decodes the mem_info struct
 */
xdr_meminfo(xdrs, pd)
    XDR *xdrs;
    struct mem_info *pd;
{
    if (!xdr_int(xdrs, &pd->amt))
	return(0);
    return(1);
}

/*
 * xdr_tapeinfo XDR encodes/decodes the tape_info struct
 */
xdr_tapeinfo(xdrs, pd)
    XDR *xdrs;
    struct tape_info *pd;
{
    if (!xdr_int(xdrs, &pd->status))
	return(0);
    if (!xdr_wrapstring(xdrs, &pd->ctlr))
	return(0);
    if (!xdr_int(xdrs, &pd->ctlr_num))
	return(0);
    if (!xdr_short(xdrs, &pd->t_type))
	return(0);
    return(1);
}

/*
 * xdr_diskinfo XDR encodes/decodes the disk_info struct
 */
xdr_diskinfo(xdrs, pd)
    XDR *xdrs;
    struct disk_info *pd;
{
    if (!xdr_int(xdrs, &pd->amt))
	return(0);
    if (!xdr_int(xdrs, &pd->status))
	return(0);
    if (!xdr_wrapstring(xdrs, &pd->ctlr))
	return(0);
    if (!xdr_int(xdrs, &pd->ctlr_num))
	return(0);
    if (!xdr_int(xdrs, &pd->ctlr_type))
	return(0);
    return(1);
}

struct xdr_discrim u_tag_arms[5] = {
    { (int) GENERAL_DEV, xdr_devinfo },
    { (int) MEM_DEV, xdr_meminfo },
    { (int) TAPE_DEV, xdr_tapeinfo },
    { (int) DISK_DEV, xdr_diskinfo },
    { (int) MAXDEVS, NULL }
};

/*
 * xdr_u_tag XDR encodes/decodes the u_tag struct
 */
xdr_u_tag(xdrs, utp)
    XDR *xdrs;
    struct u_tag *utp;
{
    return(xdr_union(xdrs, &utp->utype, &utp->uval, u_tag_arms, NULL));
}

/*
 * xdr_found_dev XDR encodes/decodes the found_dev struct
 */
xdr_found_dev(xdrs, pd)
    XDR *xdrs;
    struct found_dev *pd;
{
    if (!xdr_wrapstring(xdrs, &pd->device_name))
	return(0);
    if (!xdr_int(xdrs, &pd->unit))
	return(0);
    if (!xdr_u_tag(xdrs, &pd->u_tag))
	return(0);
    return(1);
}

/*
 * xdr_f_devs XDR encodes/decodes the f_devs struct
 */
xdr_f_devs(xdrs, pds)
        XDR *xdrs;
        struct f_devs *pds;
{
	if (!xdr_int(xdrs, &pds->cputype))
		return(0);
	if (!xdr_wrapstring(xdrs, &pds->cpuname))
		return(0);
        return(xdr_array(xdrs, &pds->found_dev, &pds->num, MAXDEVS,
                sizeof(struct found_dev), xdr_found_dev));
}
