#ifndef lint
static	char sccsid[] = "@(#)consfb.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Console frame buffer driver for Sun.
 *
 * Indirects to fbdev found during autoconf.
 */

#include <machine/pte.h>

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/vmmac.h>
#include <sys/uio.h>
#include <sys/proc.h>

dev_t	fbdev = -1;

/*
 * If we have a frame buffer device, then its major and minor device
 * numbers have been placed in the global variable "fbdev" allocated
 * above. So, when someone opens /dev/fb, redirect the device reference
 * to the "primary" frame buffer.
 */

/*ARGSUSED*/
consfbopen(dev, flag, ndev)
	dev_t dev, *ndev;
{
	if ((fbdev == -1) || (fbdev == dev))
		return (ENXIO);
	*ndev = fbdev;
	return (EAGAIN);
}

/*
 * The remaining entry points should never be called, but are
 * provided so that consfb.o can be linked in to kernels that
 * requre them. If they get called, then spec_open did not
 * properly redirect the device open to the new device.
 */

/*ARGSUSED*/
consfbclose(dev, flag)
	dev_t dev;
{
	panic ("consfbclose");
}

/*ARGSUSED*/
consfbioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	panic ("consfbioctl");
}

/*ARGSUSED*/
consfbmmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	panic ("consfbmmap");
}
