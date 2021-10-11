#ifndef lint
static char sccsid[] = "@(#)cg8_wrapper.c 1.1 92/07/30 SMI";
#endif

#include <sys/types.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sundev/mbvar.h>
#include <sun/autoconf.h>
#include <sun/vddrv.h>

/*
 * Driver character device entry points and mb device definition
 */
#if !defined(sun4c) && !defined(sun4m) 
	extern tconeopen(), tconeclose(), nulldev();
	extern tconeioctl(), nodev(), seltrue();
	extern tconemmap();
	extern struct mb_driver tconedriver;

	static struct cdevsw cdev = {
		tconeopen, tconeclose, nulldev, nulldev, tconeioctl, 
		nulldev, seltrue, tconemmap, 0
	};

	static struct mb_device xxxtconecdevice = {
		/* driver,  unit, ctlr, slave,       address,       pri, dk, flag, irq, space */
		&tconedriver,  0,  -1,   0,    (caddr_t)0x0a0200000, 0,   0,  0x0,  0, SP_OBMEM
	};

#else sun4c
	extern cgeightopen(), cgeightclose(), nulldev();
	extern cgeightioctl(), nodev(), seltrue();
	extern cgeightmmap();
	extern int spec_segmap();
	static struct cdevsw cdev = {
		cgeightopen, cgeightclose, nulldev, nulldev, cgeightioctl, 
		nulldev, seltrue, cgeightmmap, 0,
		spec_segmap
	};

	extern struct dev_ops cgeight_ops;		/* Pick up tv from driver. */
#endif sun4c

/*
 * Module linkage information for the kernel.
 */
static struct vdldrv vd = { 
	VDMAGIC_DRV, "cgeight", 
#if !defined(sun4c) && !defined(sun4m) 
	NULL, &tconedriver, &xxxtconecdevice, 
	0, 1, 
#else sun4c
	&cgeight_ops,
#endif sun4c
	NULL, &cdev, 0, 0
};

/*
 * This is the driver entry point routine.  Its address was specified
 * to the linker on the link command line as "-e xxxinit".
 *
 * inputs:  function code - VDLOAD, VDUNLOAD, VDSTAT.
 *	    pointer to kernel vddrv structure for this module. 
 * 	    pointer to appropriate vdioctl structure for this function.
 *	    pointer to vdstat structure (for VDSTAT only)
 *	
 * return:  0 for success.  VDLOAD function must set vdp->vdd_vdtab.
 *	    non-zero error code (from errno.h) if error.
 *
 */

xxxinit(function_code, vdp, vdi, vds)
	unsigned int function_code;
	struct vddrv *vdp;
	addr_t vdi;
	struct vdstat *vds;
{
	switch (function_code) {
		case VDLOAD:
			vdp->vdd_vdtab = (struct vdlinkage *)&vd;

		case VDUNLOAD:
		case VDSTAT:
			return (0);

		default:
			return (EIO);
	}
}
