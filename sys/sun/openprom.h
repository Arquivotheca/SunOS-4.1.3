/*	@(#)openprom.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _sun_openprom_h
#define	_sun_openprom_h

/*
 * This file contains definitions related to the kernel structures
 * for dealing with the autoconfig structures built under the OPENPROM
 * interface.
 *
 */

#include <sys/types.h>
#include <sys/buf.h>
#include <machine/param.h>

/*
 *
 * devinfo structures - these are constructed out of information
 * built in the new OPENPROM proms.
 *
 */

/*
 * This structure represents a physical device on the machine. It is designed
 * to allow for an arbitrary width and depth tree to be constructed describing
 * the configuration of the machine.
 */
struct dev_info {
	struct dev_info *devi_parent;	/* parent of this device */
	struct dev_info *devi_next;	/* next sibling device */
	struct dev_info *devi_slaves;	/* list of child devices */
	char *devi_name;		/* name of this device */
	int devi_nreg;			/* number of regs */
	struct dev_reg *devi_reg;	/* array of regs */
	int devi_nintr;			/* number of interrupts */
	struct dev_intr *devi_intr;	/* array of possible interrupts */
	struct dev_ops *devi_driver;	/* link to device driver */
	caddr_t devi_data;		/* driver private data */
	int devi_nodeid;		/* id of this device - from proms */
	int devi_unit;			/* logical unit (driver specific) */
	int devi_nrng;			/* number of ranges */
	struct dev_rng *devi_rng;	/* array of ranges */
};

/*
 * This structure represents one piece of bus space occupied by a given
 * device. It is used in an array for devices with multiple address windows.
 */
struct dev_reg {
	int reg_bustype;		/* cookie for bus type it's on */
	addr_t reg_addr;		/* address of reg relative to bus */
	u_int reg_size;			/* size of this register set */
};

/*
 * This structure represents one piece of nexus bus space.
 * It is used in an array for nexi with multiple bus spaces
 * to define the childs offsets in the parents bus space.
 */
struct dev_rng {
	u_int rng_cbustype;		/* Child's address, hi order */
	u_int rng_coffset;		/* Child's address, lo order */
	u_int rng_bustype;		/* Parent's address, hi order */
	u_int rng_offset;		/* Parent's address, lo order */
	u_int rng_size;			/* size of space for this entry */
};

/*
 * This structure represents one interrupt possible from the given
 * device. It is used in an array for devices with multiple interrupts.
 */
struct dev_intr {
	int int_pri;			/* interrupt priority */
	int int_vec;			/* vector # (0 if none) */
};

/*
 * This structure identifies the driver entry points. All drivers must
 * declare one of these.
 */
struct dev_ops {
	int devo_rev;
	int (*devo_identify)();		/* confirm device id */
	int (*devo_attach)();		/* attach routine of driver */
	int (*devo_open)();
	int (*devo_close)();
	int (*devo_read)();
	int (*devo_write)();
	int (*devo_strategy)();
	int (*devo_dump)();
	int (*devo_psize)();
	int (*devo_ioctl)();
	int (*devo_reset)();
	int (*devo_mmap)();
};

/*
 * This structure is a wrapper for the dev_ops struct to make a list
 * of all the system's drivers. These are built by config.
 */
struct dev_opslist {
	struct dev_opslist *devl_next;	/* next element of list */
	struct dev_ops *devl_ops;	/* ptr to driver's dev_ops struct */
};

/*
 * replacement for instrumentation index found in old mb_device structure.
 * Each device that wishes to log instrumentation information will call
 * the routine 'newdk' in macine/autoconf.c with the arguments of
 * "name" and unit#. If there is space (i.e., DK_NDRIVE not exceeded)
 * an index is returned appropriate for indexing into dk_wds, etc..
 * and the corresponding location in dk_ivec is initialized from the
 * passed arguments, else -1 is returned.
 */

struct dk_ivec {
	char *dk_name;	/* name of device */
	short dk_unit;	/* unix unit #	*/
};


#if	defined(KERNEL) && defined(OPENPROMS)

/*
 * Kernel declarations and data references
 */
/*
 * Global pointer to the top of the dev_info tree.
 */
extern struct dev_info *top_devinfo;

/*
 * These routines are the driver interface to the property lists. They are
 * used as follows:
 *
 * getprop(devi, name, default) - returns the value of 'name' in the
 *	property list for 'devi'; returns 'default' if no value is found or
 *	the value is not 4 bytes in length.
 *
 * getlongprop(devi, name) - returns a pointer to a private copy of the
 *	value of 'name' in the property list for 'devi'; returns NULL
 *	if no value is found.
 */
extern int getprop();
extern addr_t getlongprop();

/*
 * New Instrumentation declarations
 */

extern int newdk();
extern struct dk_ivec dk_ivec[];

#endif	defined(KERNEL) && defined(OPENPROMS)
#endif	_sun_openprom_h
