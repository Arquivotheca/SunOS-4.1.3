/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 *
 * ns (noswap) pseudo-device is used to fake out the VM system.
 */

#include "ns.h"

#if NNS > 0


#include <sys/param.h>		/* Includes "../h/types.h" */
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/vmsystm.h>
#include <sundev/mbvar.h>

#ifndef	lint
static	char sccsid[] = "@(#)ns.c	1.1 92/07/30";
#endif

#ifndef OPENPROMS
struct	mb_driver nsdriver = {
	0,		/* see if a driver is really there */
	0,		/* see if a slave is there */
	0,		/* setup driver for a slave */
	0,		/* routine to start transfer */
	0,		/* routine to finish transfer */
	0,		/* polling interrupt routine */
	0,		/* amount of memory space needed */
	"ns",		/* name of a device */
	0,		/* backpointers to mbdinit structs */
	"ns",		/* name of a controller */
	0,		/* backpointers to mbcinit structs */
	MDR_PSEUDO,	/* Mainbus usage flags */
	0,		/* interrupt routine linked list */
};
#else OPENPROMS
/*
 * nothing go here for snazzy autoconfig
 */
#endif !OPENPROMS



/*ARGSUSED*/
nsopen(dev, wrtflag)
	dev_t           dev;
	int             wrtflag;
{
	return(0);
}

/* nssize - the size of this device is equal to the amount of physical
 *		memory available when the device is sized.
 */
/*ARGSUSED*/
nssize(dev)
	dev_t           dev;
{
	return (ctod(freemem));
}

#include <sys/uio.h>

/*ARGSUSED*/
nsread(dev, uio) dev_t dev; struct uio *uio; { panic("nsread"); }

/*ARGSUSED*/
nswrite(dev, uio) dev_t dev; struct uio *uio; { panic("nswrite"); }

/*
 * new way of MUNIX - this returns an error, so the page is known not to
 * go to disk.  vm_pageout:checkpage() will then just punt.
 */
/*ARGSUSED*/
nsstrategy(bp) struct buf *bp;
{
	bp->b_flags |= B_ERROR;
	iodone( bp );
}

#endif
