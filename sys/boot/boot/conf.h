/*
 * @(#)conf.h 1.1 92/07/30 SMI
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

/*
 * Declaration of block device list. This is vaguely analogous
 * to the block device switch bdevsw[] in the kernel's conf.h,
 * except that boot uses a boottab to collect the driver entry points.
 * Like bdevsw[], the bdevlist provides the link between the main unix
 * code and the driver.  The initialization of the device switches is in
 * the file <arch>/conf.c.
 */

#include <mon/sunromvec.h>

struct  bdevlist {
	char	*bl_name;
	struct	boottab	*bl_driver;
	dev_t	bl_root;
	int	bl_flags;
};

#ifdef OPENPROMS
struct ndevlist {
	char 	*nl_name;
	struct	boottab	*nl_driver;
	int	nl_root;
};

struct binfo {
	int	ihandle;
	char	*name;
};
#endif OPENPROMS
