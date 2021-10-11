#ifndef	lint
static	char sccsid[] = "@(#)spec_clone.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif	lint

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Clone device driver.  Forces a clone open of some other
 * character device.  Since its purpose in life is to force
 * some other device to clone itself, there's no need for
 * anything other than the open routine here.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>

/*
 * Do a clone open.  The (major number of the) device to be cloned
 * is specified by minor(dev).  We tell spec_open to do the work
 * by returning EEXIST after naming the device to clone.
 */
/* ARGSUSED */
cloneopen(dev, flag, newdevp)
	dev_t	dev;
	int	flag;
	dev_t	*newdevp;
{
	/* Convert to the device to be cloned. */
	*newdevp = makedev(minor(dev), 0);

	return (EEXIST);
}
