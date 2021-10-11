#ident "@(#)autoconf.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1987-1989 by Sun Microsystems, Inc.
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the Mainbus
 * device tables and the memory controller monitoring. Available
 * devices are determined by PROM and passed into configure(), where
 * the drivers are initialized.
 */

#include <sys/param.h>
#define	OPENPROMS
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vfs_stat.h>
#include <sys/vnode.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/dk.h>
#include <sys/vm.h>
#include <sys/file.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/ttold.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/kernel.h>
#include <sys/uio.h>
#include <sys/mman.h>
#undef NFS
#include <sys/mount.h>
#include <sys/bootconf.h>
#include <specfs/snode.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <vm/seg.h>

#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/scb.h>
#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/seg_kmem.h>
#include <sun/autoconf.h>
#include <sun/consdev.h>
#include <machine/devr.h>
#include <sundev/mbvar.h>
#include <sundev/kbio.h>
#include <sundev/zsvar.h>
#include <mon/idprom.h>
#include <sys/debug.h>
#include <sys/reboot.h>


extern int swap_present;
/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */

int	dkn;		/* number of iostat dk numbers assigned so far */
/*
 * array of "name"/unit pairs that map to dk numbers
 */
struct	dk_ivec dk_ivec[DK_NDRIVE];

/*
 * pointer to DVMA resource map
 */
struct map *dvmamap;

struct dev_opslist *dev_opslist = 0;
extern struct dev_opslist av_opstab[];
struct dev_info *top_devinfo;
struct dev_info *options_devinfo;

#ifndef SAS
/*
 * This routine fills in the common information for a given dev_info struct.
 * Using the nodeid that is already initialized, it pulls the following
 * information out of the PROMS:
 *	name, reg, intr
 * If any of these properties are not found, the value of that field is
 * left at its initial value.
 *
 * nreg && nintr are inferred from the property length of reg && intr resp.
 */
static void
fill_devinfo(dev_info)
	struct dev_info *dev_info;
{
	int nodeid;
	int (*decode_func)();
	static int curzs = 0;

	nodeid = dev_info->devi_nodeid;
	dev_info->devi_name = (char *)getlongprop(nodeid, "name");
	dev_info->devi_nintr = getproplen(nodeid, "intr") /
					sizeof (struct dev_intr);
	/*
	 * We try to match stdin/stdout while attaching the first "zs".
	 * (Try a more generic approach like -1.)
	 */

	if (strcmp("zs", dev_info->devi_name) == 0)
		dev_info->devi_unit = curzs++;
	
	if (dev_info->devi_nintr > 0)
		dev_info->devi_intr =
		    (struct dev_intr *)getlongprop(nodeid, "intr");
	/*
	 * V.2 Open Boot proms report "reg" addresses for children of
	 * hierarchical devices in the address space of the parent
	 * device. If necessary, call a bus-specific decoding routine
	 * to unpack the reg struct.
	 */
	if ((prom_getversion() > 0) &&
	    (decode_func = path_getdecodefunc(dev_info->devi_parent))) {
		dev_info->devi_nreg = decode_func(dev_info, nodeid);
		return;
	}
	dev_info->devi_nreg = getproplen(nodeid, "reg")/sizeof (struct dev_reg);
	if (dev_info->devi_nreg > 0)
		dev_info->devi_reg =
		    (struct dev_reg *)getlongprop(nodeid, "reg");
};

/*
 * This routine attempts to match a device name to a device driver. It
 * walks the list of driver ops_vectors, calling the identify routines
 * until one of the drivers claims the device.
 */
static struct dev_ops *
match_driver(name)
	char *name;
{
	struct dev_opslist *ops;

	for (ops = dev_opslist; ops->devl_ops != NULL; ops = ops->devl_next) {
		if (ops->devl_ops->devo_identify(name))
			return (ops->devl_ops);
	}
	return (NULL);
}

/*
 * This routine checks to see if the given nodeid is already listed as a
 * child of the given device. It is used to prevent duplicate dev_info
 * structs from being created.
 */
static int
check_dupdev(dev_info, nodeid)
	struct dev_info *dev_info;
	int nodeid;
{
	struct dev_info *dev;

	for (dev = dev_info->devi_slaves; dev != NULL; dev = dev->devi_next) {
		if (dev->devi_nodeid == nodeid)
			return (1);
	}
	return (0);
}

/*
 * This routine appends the second argument onto the slave list for the
 * first argument. It always appends it to the end of the list.
 */
static void
append_dev(dev_parent, dev_slave)
	struct dev_info *dev_parent, *dev_slave;
{
	register struct dev_info *dev;

	if (dev_parent->devi_slaves == NULL)
		dev_parent->devi_slaves = dev_slave;
	else {
		for (dev = dev_parent->devi_slaves; dev->devi_next != NULL;
		    dev = dev->devi_next)
			;
		dev->devi_next = dev_slave;
	}
}

#ifndef VDDRV
/*
 * This routine removes the second argument from the slave list for the
 * first argument. It returns -1 if the slave isn't found on the list.
 */
static int
remove_dev(dev_parent, dev_slave)
	struct dev_info *dev_parent, *dev_slave;
{
	register struct dev_info *dev;

	if (dev_parent->devi_slaves == dev_slave) {
		dev_parent->devi_slaves = dev_slave->devi_next;
		return (0);
	} else
		for (dev = dev_parent->devi_slaves; dev != NULL;
		    dev = dev->devi_next) {
			if (dev->devi_next == dev_slave) {
				dev->devi_next = dev_slave->devi_next;
				return (0);
			}
		}
	return (-1);
}
#endif VDDRV

/*
 * Create a list of ops vectors out of the array config produced.
 */
static void
init_opslist()
{
	register struct dev_opslist *dp;
	register int i = 0;

	while ((dp = &av_opstab[i])->devl_ops != NULL)
		dp->devl_next = &av_opstab[++i];
	dev_opslist = av_opstab;
}

#else SAS
/*
 * STEVE- you'll have to fix this...
 */

struct dev_info sbus_info;
struct dev_info sas_info;

struct dev_info sas_info = {
	NULL,	/* devi_parent; */
	NULL,	/* devi_next; */
	&sbus_info,	/* devi_slaves; */
	"campus-sas",	/* devi_name; */
	0,	/* devi_nreg; */
	NULL,	/* devi_reg; */
	0,	/* devi_nintr; */
	NULL,	/* devi_intr; */
	NULL,	/* devi_driver; */
	NULL,	/* devi_data; */
	0,	/* devi_nodeid; */
	0,	/* devi_unit; */
};

struct dev_info sbus_info = {
	&sas_info,	/* devi_parent; */
	NULL,	/* devi_next; */
	NULL,	/* devi_slaves; */
	"sbus",	/* devi_name; */
	0,	/* devi_nreg; */
	NULL,	/* devi_reg; */
	0,	/* devi_nintr; */
	NULL,	/* devi_intr; */
	NULL,	/* devi_driver; */
	NULL,	/* devi_data; */
	1,	/* devi_nodeid; */
	0,	/* devi_unit; */
};


#endif SAS

/*
 * Maximum interrupt priority used by Mainbus DMA.
 * This value is determined as each driver that uses DMA calls adddma
 * with the hardware interrupt priority level that they use.  Adddma
 * increases splvm_val as necessary, and configure computes SPLMB based
 * on it.
 *
 * XXX SPLMB is for backward compatibility only.
 * XXX Drivers should call splvm() instead of splr(pritospl(SPLMB)).
 */
int SPLMB = 4;				/* reasonable default */

/*
 * We use splvmval to implement splvm().
 */
int splvm_val = pritospl(4);		/* reasonable default */

/*
 * Machine type we are running on.
 */
int cpu;

/*
 * Determine mass storage and memory configuration for a machine.
 * Get cpu type, and then switch out to machine specific procedures
 * which will determine what is out there.
 */
configure()
{
	extern int fpu_exists;		/* set or cleared by fpu_probe */

	idprom();
	fpu_probe();
	if (!fpu_exists)
		printf("No FPU in configuration\n");
	/*
	 * Configure the I/O devices.
	 */
	mbconfig();
	/*
	 * Attach pseudo-devices
	 */
	pseudoconfig();
}

/*
 * Initialize devices on the machine.
 * Uses configuration tree built by the PROMs to determine what
 * is present. The attach_devs() routine is used in conjuction with
 * the sbus routines to cause the appropriate initialization
 * to occur. All we do here is start the ball rolling.
 */
mbconfig()
{

#ifndef SAS
	dnode_t	prom_nextnode();
	/*
	 * chain together the opslist that we're built with
	 */
	init_opslist();
	/*
	 * build the dev_info tree, attaching devices along the way
	 */
	top_devinfo = (struct dev_info *) kmem_zalloc(sizeof (*top_devinfo));
	/*
	 * XXX - This call ALSO patches the DMA+ bug in Campus-B and
	 * Phoenix. The prom uncaches the traptable page as a side-effect
	 * of devr_next(0), so this *must* be executed early on.
	 */
	top_devinfo->devi_nodeid = (int)prom_nextnode((dnode_t)0);
	fill_devinfo(top_devinfo);
	printf("cpu = %s\n", top_devinfo->devi_name);
	attach_devs(top_devinfo);
#endif !SAS
}

#ifndef SAS
static int nsbus = 0;		/* number of sbus's */
static int cursbus = 0;		/* current unit number for attach */
int sbus_identify();
int sbus_attach();
struct dev_ops sbus_ops = {
	1,
	sbus_identify,
	sbus_attach,
};

int
sbus_identify(name)
	char *name;
{
	if (strcmp(name, "sbus") == 0) {
		nsbus++;
		return (1);
	}
	return (0);
}

int
sbus_attach(devi)
	struct dev_info *devi;
{

	/*
	 * We increment the unit number for each sbus we see.
	 */
	devi->devi_unit = cursbus++;
	report_dev(devi);
	attach_devs(devi);
	return (0);
}

/*
 * This routine identifies and initializes all the slaves of the given
 * device. A dev_info struct is created for each slave, and the common
 * values initialized. It is tested against the device driver
 * identify routines, and a match causes the appropriate dev_ops pointer
 * to be linked to the device. After all the slaves have been identified,
 * the appropriate attach routines are called for each one. Doing it in this
 * order allows a driver to count how many devices it will have before the
 * first attach routine is called. If an attach fails for any reason, the
 * dev_info struct is marked unattached, by setting the dev_ops pointer
 * to NULL.
 * If no identify match is made for a device, it is also marked
 * unattached and an error message is printed.
 * This routine should not be called after the system is running to initialize
 * a device that was added on the fly; add_drv should be used for that
 * purpose.
 *
 * It carefully checks each device the
 * prom returns to avoid creating duplicate dev_info entries,
 * but we may not need to do that anymore since we don't use this to add
 * devices after boot.
 */
attach_devs(dev_info)
	struct dev_info *dev_info;
{
	register struct dev_info *dev;
	struct dev_info *newdevs = NULL;
	register dnode_t curnode;
	dnode_t prom_nextnode(), prom_childnode();

	curnode = prom_childnode((dnode_t)(dev_info->devi_nodeid));
	if (curnode == 0)
		return;
	for (; curnode != 0;
	    curnode = prom_nextnode(curnode)) {
		/*
		 * If it's already on the list, skip it.
		 */
		if (check_dupdev(dev_info, (int)curnode) != 0)
			continue;
		/*
		 * Allocate it, fill it in and add it to the list.
		 */
		dev = (struct dev_info *)kmem_zalloc(sizeof (*dev));
		append_dev(dev_info, dev);
		dev->devi_nodeid = (int)curnode;
		dev->devi_parent = dev_info;
		fill_devinfo(dev);
		dev->devi_driver = match_driver(dev->devi_name);
		/*
		 * Note the first new struct on the list.
		 */
		if (newdevs == NULL)
			newdevs = dev;
	}
	/*
	 * Try to attach the new devs. If any of the identifies or attaches
	 * fail, throw out the dev_info struct.
	 * We track this loop funny because structs can get deallocated in
	 * the middle.
	 */
	for (dev = newdevs; dev != NULL; dev = newdevs) {
		newdevs = dev->devi_next;
		if (dev->devi_driver == NULL) {
#ifdef	DAVE_DOESNT_WANT_THIS_ANYMORE
			printf("Warning: kernel doesn't support device '%s'\n",
			    dev->devi_name);
#endif
#ifndef VDDRV
			if (remove_dev(dev_info, dev))
				panic("attach_devs remove_dev 1");
			(void) kmem_free((caddr_t) dev, sizeof (*dev));
#endif VDDRV
			continue;
		}
		if (dev->devi_driver->devo_attach(dev) < 0) {
#ifdef VDDRV
			dev->devi_driver = NULL;
#else VDDRV
			if (remove_dev(dev_info, dev))
				panic("attach_devs remove_dev 2");
			(void) kmem_free((caddr_t) dev, sizeof (*dev));
#endif VDDRV
		}
	}
}

#endif !SAS

#ifdef VDDRV
struct add_drv_info {
	struct dev_ops	*adi_ops;
	int		 adi_count;
};

/*
 * Pick up devices for a just-loaded driver:
 * 1. Initialize the add_drv_info structure.
 * 2. Kick off add_drv_layer on the top layer of devices.
 * 3. Return the number of devices attached.
 */
int
add_drv(dev_ops)
	struct dev_ops *dev_ops;
{
	struct add_drv_info	adi;
	void			add_drv_layer();

	adi.adi_ops = dev_ops;
	adi.adi_count = 0;

	walk_slaves(top_devinfo,
		(int (*)()) add_drv_layer, (caddr_t) &adi);
	return (adi.adi_count);
}

struct new_devlist {
	struct new_devlist	*nd_next;
	struct dev_info		*nd_dev;
	int			 nd_attachme;
};

struct new_devlist_head {
	struct new_devlist	*ndh_head;
	struct new_devlist	*ndh_tail;
};

/*
 * Pick up devices in a given layer for a just-loaded driver:
 * 1. Walk the layer, finding any unattached devices.
 * 2. Call the identify routine for each unattached device, and mark
 *    those that we should attempt to attach.
 * 3. Attach those that have been so marked.
 * 4. Free the link of new_devlist structures.
 * 5. Update the count of attached devices.
 * 6. Recurse over any slaves of this layer.
 */
static void
add_drv_layer(dev, adip)
	struct dev_info	*dev;
	struct add_drv_info	*adip;
{
	struct new_devlist_head new_devlist_head;
	register struct new_devlist *ndp;
	void	add_a_device();
	int	attached = 0;

	new_devlist_head.ndh_head = new_devlist_head.ndh_tail = NULL;
	walk_layer(dev,
		(int (*)()) add_a_device, (caddr_t) &new_devlist_head);
	for (ndp = new_devlist_head.ndh_head; ndp; ndp = ndp->nd_next)
		ndp->nd_attachme =
			(*adip->adi_ops->devo_identify)(ndp->nd_dev->devi_name);
	for (ndp = new_devlist_head.ndh_head; ndp;
		ndp = new_devlist_head.ndh_head) {
		if (ndp->nd_attachme) {
			ndp->nd_dev->devi_driver = adip->adi_ops;
			if ((*adip->adi_ops->devo_attach)(ndp->nd_dev) < 0)
				ndp->nd_dev->devi_driver = NULL;
			else
				attached++;
		}
		new_devlist_head.ndh_head = ndp->nd_next;
		(void) kmem_free((caddr_t) ndp, sizeof (*ndp));
	}
	adip->adi_count += attached;

	/* Now see if any slaves of this layer need attaching */
	walk_slaves(dev, (int (*)()) add_drv_layer, (caddr_t) adip);
}

/*
 * If a dev_info is unattached, add it to the new_devlist being built.
 * Keep the list in FIFO order.
 */
static void
add_a_device(dev, ndhp)
	struct dev_info	*dev;
	struct new_devlist_head	*ndhp;
{
	struct new_devlist	*ndp;

	if (dev->devi_driver == NULL) {
		ndp = (struct new_devlist *) kmem_alloc(sizeof (*ndp));
		ndp->nd_dev = dev;
		if (ndhp->ndh_head == NULL)
			ndhp->ndh_head = ndp;
		else /* if head is set, so is tail */
			ndhp->ndh_tail->nd_next = ndp;
		ndp->nd_next = NULL;
		ndhp->ndh_tail = ndp;
	}
}

/*
 * Add a driver to the list of ops vectors.  Don't allow duplicates.
 */
int
add_ops(ops)
	struct dev_ops *ops;
{
	register struct dev_opslist *dp;

	for (dp = dev_opslist; dp->devl_ops != NULL; dp = dp->devl_next) {
		if (dp->devl_ops == ops)
			return (EINVAL);
	}

	dp = (struct dev_opslist *) kmem_alloc(sizeof (*dp));
	dp->devl_ops = ops;
	dp->devl_next = dev_opslist;
	dev_opslist = dp;

	return (0);
}

/*
 * Unattach any devices attached to this driver.
 */
void
rem_drv(dev_ops)
	struct dev_ops *dev_ops;
{
	void	rem_a_device();

	walk_devs(top_devinfo,
		(int (*)()) rem_a_device, (caddr_t) dev_ops);
}

/*
 * If this dev is linked to this driver, remove it.
 */
static void
rem_a_device(dev, ops)
	struct dev_info	*dev;
	struct dev_ops	*ops;
{

	if (dev->devi_driver == ops)
		dev->devi_driver = NULL;
}

/*
 * Remove a driver from the list of ops vectors.
 */
int
rem_ops(ops)
	struct dev_ops *ops;
{
	register struct dev_opslist *dp;
	register struct dev_opslist *prev = NULL;

	for (dp = dev_opslist; dp->devl_ops != NULL;
		prev = dp, dp = dp->devl_next)
		if (dp->devl_ops == ops) {
			if (prev == NULL)
				dev_opslist = dp->devl_next;
			else
				prev->devl_next = dp->devl_next;
			(void) kmem_free((caddr_t) dp, sizeof (*dp));
			return (0);
		}

	return (EINVAL);
}
#endif VDDRV

/*
 * This general-purpose routine walks down the tree of devices, calling
 * the given function for each one it finds, and passing the pointer arg
 * which can point to a structure of information that the function
 * needs.
 * It is useful when searches need to be made.
 * It needs to walk all devices, attached or not, as it is used before
 * any devices are attached.  The function called should check if the
 * device is attached if it cares about such things.
 * It does the walk a layer at a time, not depth-first.
 */
walk_devs(dev_info, f, arg)
	struct dev_info *dev_info;
	int (*f)();
	caddr_t arg;
{
	register struct dev_info *dev;

	/*
	 * Call the function for all the devices on this layer.
	 * Then, for each device that has a slave, call walk_devs on
	 * the slave.
	 */
	walk_layer(dev_info, f, arg);
	for (dev = dev_info; dev != (struct dev_info *)NULL;
	    dev = dev->devi_next) {
		if (dev->devi_slaves)
			walk_devs(dev->devi_slaves, f, arg);
	}
}

/*
 * This general-purpose routine walks one layer of the dev_info tree,
 * calling the given function for each dev_info it finds, and passing
 * the pointer arg which can point to a structure of information that
 * the function needs.
 * It is useful when searches need to be made.
 * It walks all devices, attached or not.  The function called should
 * check if the device is attached if it cares about such things.
 */
walk_layer(dev_info, f, arg)
	struct dev_info *dev_info;
	int (*f)();
	caddr_t arg;
{
	register struct dev_info *dev;

	/*
	 * Call the function for this dev_info.
	 * Go to the next device.
	 */
	for (dev = dev_info; dev != (struct dev_info *)NULL;
	    dev = dev->devi_next) {
		(*f)(dev, arg);
	}
}

/*
 * This general-purpose routine walks one layer of the dev_info tree,
 * calling the given function for each dev_info that has slaves, passing
 * it the slave layer and the pointer arg which can point to a
 * structure of information that the function needs.
 * It is useful when searches need to be made.
 * It needs to walk all devices, attached or not, as it is used before
 * any devices are attached.  The function called should check if the
 * device is attached if it cares about such things.
 */
walk_slaves(dev_info, f, arg)
	struct dev_info *dev_info;
	int (*f)();
	caddr_t arg;
{
	register struct dev_info *dev;

	/*
	 * For each device on this layer that has a slave, call the
	 * function on that slave layer.
	 */
	for (dev = dev_info; dev != (struct dev_info *)NULL;
	    dev = dev->devi_next) {
		if (dev->devi_slaves)
			(*f)(dev->devi_slaves, arg);
	}
}

#ifndef SAS
/*
 * Given a physical address, a bus type, and a size, return a virtual
 * address that maps the registers.
 * Returns NULL on any error.
 */
addr_t
map_regs(addr, size, space)
	addr_t addr;
	u_int size;
	int space;
{
	register addr_t reg = NULL;
	u_int pageval;
	int s;

	if (space == SPO_VIRTUAL)
		reg = addr;
	else
		pageval = MAKE_PGT(space) | btop(addr);

	if (reg == NULL) {
		int offset = (int)addr & PGOFSET;
		long a = 0;
		int extent;

		extent = btoc(size + offset);
		if (extent == 0)
			extent = 1;
		s =splhigh();
		a = rmalloc(kernelmap, (long)extent);
		(void)splx(s);
		if (a == 0)
			panic("out of kernelmap for devices");
		reg = (addr_t)((int)kmxtob(a) | offset);
		segkmem_mapin(&kseg, (addr_t)((int)reg&PAGEMASK),
			(u_int)mmu_ptob(extent), PROT_READ | PROT_WRITE,
			pageval, 0);
	}
	return (reg);
}

#ifdef notused
/*
 * Supposedly, this routine is the only place that enumerates details
 * of the encoding of the "bustype" part of the physical address.
 * This routine depends on details of the physical address map, and
 * will probably be somewhat different for different machines.
 */
decode_address(space, addr)
int space, addr;
{
	char *name;

	/* Here we encode the knowledge that the SBUS is part of OBIO space */
	if (space == OBIO && addr >= SBUS_BASE) {
		printf(" SBus slot %d 0x%x", (addr - SBUS_BASE)/SBUS_SIZE,
			(addr % SBUS_SIZE) & (SBUS_SIZE-1));
		return;
	}

	/* Here we enumerate the set of physical address types */
	switch (space) {
	case	SPO_VIRTUAL:	name = "virtual";	break;
	case	OBMEM:		name = "obmem";		break;
	case	OBIO:		name = "obio";		break;
	default:		name = "unknown";	break;
	}

	printf(" %s 0x%x", name, addr);
}
#endif

/*
 * Say that a device is here
 */

void
report_dev(dev)
	register struct dev_info *dev;
{
	register struct dev_reg *rp;
	register struct dev_intr *ip;
	int i;

	printf("%s%d", dev->devi_name, dev->devi_unit);
	for (i = 0, rp = dev->devi_reg; i < dev->devi_nreg; i++, rp++) {
		if (i == 0)
			printf(" at");
		else
			printf(" and");
		decode_address(rp->reg_bustype, (int) rp->reg_addr);
	}
	for (i = 0, ip = dev->devi_intr; i < dev->devi_nintr; i++, ip++) {
		if (i == 0)
			printf(" ");
		else
			printf(", ");
		printf("pri %d", ip->int_pri);
		if (ip->int_vec)
			printf(" vec 0x%x", ip->int_vec);
	}
	printf("\n");
}

/*
 * Unmap the virtual address range from addr to addr+size.
 * panics on any error (but at a lower level).
 */
void
unmap_regs(addr, size)
	addr_t addr;
	u_int size;
{
	long a;
	int extent;
	int offset;
	int s;

	offset = (int)addr & PGOFSET;
	extent = btoc(size + offset);
	if (extent == 0)
		extent = 1;
	segkmem_mapout(&kseg, (addr_t)((int)addr&PAGEMASK),
		(u_int)mmu_ptob(extent));
	a=btokmx(addr);
	s = splhigh();
	rmfree(kernelmap, (long)extent, (u_long)a);
	(void)splx(s);
}
#endif !SAS

/*
 * This routine returns the length of the property value for the named
 * property.
 */
int
getproplen(id, name)
	int id;
	char *name;
{
	return (prom_getproplen((dnode_t)id, (caddr_t)name));
}

/*
 * This routine allows drivers to easily retrieve the value of a property
 * for int-sized values. The value of 'name' in the property list for 'devi'
 * is returned. If no value is found, 'def' is returned.
 *
 * As a special case, if a zero length property is found, 1 is returned.
 */
int
getprop(id, name, def)
	int id;
	char *name;
	int def;
{
	int value;

	switch (getproplen(id, name)) {
	case 0:
		return (1);

	case sizeof (int):
		(void) prom_getprop((dnode_t)id, (caddr_t)name, 
		    (caddr_t)&value);
		return (value);

	case -1:
	default:
		return (def);
	}
	/*NOTREACHED*/
}

/*
 * This routine allows drivers to easily retrieve the value of a property
 * for arbitrary sized properties. A pointer to the value of 'name' in
 * the property list for 'devi' is returned. If no value is found, NULL
 * is returned. The space is allocated using kmem_zalloc, so it is assumed
 * that this routine is NOT called from an interrupt routine.
 */
addr_t
getlongprop(id, name)
	int id;
	char *name;
{
	int size;
	addr_t value;

	size = getproplen(id, name);
	if (size <= 0)
		return (NULL);
	value = kmem_zalloc((u_int) size);
	(void)prom_getprop((dnode_t)id, (caddr_t)name, (caddr_t)value);
	return (value);
}

/*
 * Spurious interrupt messages
 */

#define	SPURIOUS	-1		/* recognized in locore.s */

static
not_serviced(counter, level)
	int *counter, level;
{
	if ((*counter)++ % 100 == 1)
		printf("iobus level %d interrupt not serviced\n", level);
	return (SPURIOUS);
}

int level1_spurious;
not_serviced1() { return not_serviced(&level1_spurious, 1); }

int level2_spurious;
not_serviced2() { return not_serviced(&level2_spurious, 2); }

int level3_spurious;
not_serviced3() { return not_serviced(&level3_spurious, 3); }

int level5_spurious;
not_serviced5() { return not_serviced(&level5_spurious, 5); }

int level6_spurious;
not_serviced6() { return not_serviced(&level6_spurious, 6); }

int level7_spurious;
not_serviced7() { return not_serviced(&level7_spurious, 7); }

int level8_spurious;
not_serviced8() { return not_serviced(&level8_spurious, 8); }

int level9_spurious;
not_serviced9() { return not_serviced(&level9_spurious, 9); }

int level11_spurious;
not_serviced11() { return not_serviced(&level11_spurious, 11); }

int level13_spurious;
not_serviced13() { return not_serviced(&level13_spurious, 13); }


typedef int (*func)();
extern softlevel1();

/*
 * These structures are used in locore.s to jump to device interrupt routines.
 * They also provide vmstat assistance.
 * They will index into the string table generated by autoconfig
 * but in the exact order addintr sees them. This allows IOINTR to quickly
 * find the right counter to increment.
 * (We use the fact that the arrays are initialized to 0 by default).
 */
struct	autovec level1[NVECT] = {{softlevel1}, {not_serviced1}};
struct	autovec level2[NVECT] = {{not_serviced2}};
struct	autovec level3[NVECT] = {{not_serviced3}};
struct	autovec level4[NVECT] = {{NULL}}; /* special - see locore */
struct	autovec level5[NVECT] = {{not_serviced5}};
struct	autovec level6[NVECT] = {{not_serviced6}};
struct	autovec level7[NVECT] = {{not_serviced7}};
struct	autovec level8[NVECT] = {{not_serviced8}};
struct	autovec level9[NVECT] = {{not_serviced9}};
struct	autovec level11[NVECT] = {{not_serviced11}};
struct	autovec level13[NVECT] = {{not_serviced13}};

/*
 * vmstat -i support for autovectored interrupts.
 * add the autovectored interrupt to the table of mappings of
 * interrupt counts to device names. We can't do much about
 * a device that has multiple units, so we print '?' for such.
 * Otherwise, we try to use the real unit number.
 */
#ifndef oldstuff
/*ARGSUSED */
#endif oldstuff
void
addioname(levelp, name, unit)
	register struct autovec *levelp;
	char *name;
	int unit;
{
#ifdef oldstuff
	register int k;
	register int index = 0;
	register char **nt;
	char devunit;
	int namelen;
#endif oldstuff

	if (levelp->av_name == NULL)
		levelp->av_name = name;
	else if (strcmp(levelp->av_name, name)) {
			printf("addioname: can't change %s to %s!\n",
				levelp->av_name, name);
			panic("addioname");
	}

	levelp->av_devcnt++;

#ifdef oldstuff
	/* FIXME - we don't use av_nametab anymore */
	/*
	 * Point levelp->name at the dev_info name (need the devinfo
	 * name!  or is "name" it?), and increment levelp->device_count
	*/
	devunit = (char)((int)'0' + unit);
	namelen = strlen(name);
	if (levelp->av_names != 0) {
		av_nametab[levelp->av_names - 1][namelen] = '?';
		return;
	}
	for (nt = av_nametab, k = 0; nt < av_end; nt++, k++) {
		if (strncmp(name, *nt, namelen) == 0) {
			index = k + 1;	/* 0 ==> uninitialized */
			if ((*nt)[namelen] == devunit)
				break;
		}
	}
	if (index != 0) {
		levelp->av_names = index;
		av_nametab[index - 1][namelen] = devunit;
	}
#endif oldstuff
}

/*
 * Arrange for a driver to be called when a particular
 * auto-vectored interrupt occurs.
 * NOTE: if a device can generate interrupts on more than
 * one level, or if a driver services devices that interrupt
 * on more than one level, then the driver should install
 * itself on each of those levels.
 */
addintr(lvl, xxintr, name, unit)
	func xxintr;
	char *name;
	int unit;
{
	register func f;
	register struct autovec *levelp;
	register int i;

	switch (lvl) {
	case 1: levelp = level1; break;
	case 2: levelp = level2; break;
	case 3: levelp = level3; break;
	case 4: levelp = level4; break;
	case 5: levelp = level5; break;
	case 6: levelp = level6; break;
	case 7: levelp = level7; break;
	case 8: levelp = level8; break;
	case 9: levelp = level9; break;
	case 11: levelp = level11; break;
	case 13: levelp = level13; break;
	default:
		printf(
"addintr: cannot set up polling interrupt on processor level %d\n", lvl);
		panic("addintr");
		/*NOTREACHED*/
	}
	if ((f = xxintr) == NULL)
		return;

	/* If devcnt == -1, settrap() claimed this trap */
	if (levelp->av_devcnt == -1) {
		/* XXX - addintr() should return success/failure */
		panic("addintr: interrupt trap vector busy\n");
	}

	for (i = 0; i < NVECT; i++) {
		if (levelp->av_vector == NULL)	/* end of list found */
			break;
		if (levelp->av_vector == f) {	/* already in list */
			addioname(levelp, name, unit);
			return;
		}
		levelp++;
	}
	if (i >= NVECT)
		panic("addintr: too many devices");
	/* if nobody there (like on level4) fake a dummy */
	if (i == 0)
		levelp++;
	levelp[0] = levelp[-1];		/* move not_serviced to end */
	/* must clear the new entry */
	bzero((caddr_t) &levelp[-1], sizeof (*levelp));
	levelp[-1].av_vector = f;		/* add f to list */
	addioname(levelp - 1, name, unit);
}


/*
 * Opcodes for instructions in scb entries
 */
#define	MOVPSRL0	0xa1480000		/* mov	%psr, %l0	*/
#define	MOVL4		0xa8102000		/* mov	(+/-0xfff), %l4	*/
#define	MOVL6		0xac102000		/* mov	(+/-0xfff), %l6	*/
#define	BRANCH		0x10800000		/* ba	(+/-0x1fffff)*4	*/
#define	SETHIL3		0x27000000		/* sethi %hi(0xfffffc00), %l3 */
#define	JMPL3		0x81c4e000		/* jmp	%l3+%lo(0x3ff)	*/
#define	NO_OP		0x01000000		/* nop			*/

#define	BRANCH_RANGE	0x001fffff		/* max branch displacement */
#define	BRANCH_MASK	0x003fffff		/* branch displacement mask */
#define	SETHI_HI(x)	(((x) >> 10) & 0x003fffff)	/* %hi(x) */
#define	JMP_LO(x)	((x) & 0x000003ff)		/* %lo(x) */

/*
 * Dynamically set a hard trap vector in the scb table.
 * This routine allows device drivers, such as fd and audio_79C30,
 * to dynamically configure assembly-language interrupt handlers.
 * If 'xxintr' is NULL, the trap vector is set back to the default system trap.
 *
 * If this trap can be autovectored, set the av_devcnt field to -1 when a
 * direct trap vector is set, so that two devices can't claim the same
 * interrupt.  If av_devcnt > 0, this trap is already autovectored, so don't
 * give the trap vector away.
 *
 * Return 0 if the trap vector could not be set according to the request.
 * Otherwise, return 1.
 */
int
settrap(lvl, xxintr)
	int		lvl;
	func		xxintr;
{
	struct autovec	*levelp;
	trapvec		vec;
	int		offset;
	extern		sys_trap();
	extern int	nwindows;		/* number of register windows */

	/* Find the appropriate autovector table */
	switch (lvl) {
	case 1: levelp = level1; break;
	case 2: levelp = level2; break;
	case 3: levelp = level3; break;
	case 4: levelp = level4; break;
	case 5: levelp = level5; break;
	case 6: levelp = level6; break;
	case 7: levelp = level7; break;
	case 8: levelp = level8; break;
	case 9: levelp = level9; break;
	case 11: levelp = level11; break;
	case 13: levelp = level13; break;
	case 10: case 12: case 14: case 15: levelp = NULL; break;
	default:
		printf(
		    "settrap: cannot set trap for processor level %d\n", lvl);
		return (0);
	}

	/*
	 * Check to see whether trap is available.
	 * If devcnt == 0, this is a system trap with no autovectors.
	 * If devcnt > 0, this is a system trap with autovectors set.
	 * If devcnt == -1, this is a direct trap (addintr() will complain).
	 */
	if (levelp != NULL) {
		if (xxintr != NULL) {
			/* If trying to set a direct trap handler */
			if (levelp->av_devcnt == -1) {
				printf("settrap: level %d trap busy\n", lvl);
				return (0);

			} else if (levelp->av_devcnt != 0) {
		    printf("settrap: level %d trap is autovectored\n", lvl);
				return (0);
			}
			levelp->av_devcnt = -1;		/* don't autovector */
		} else {
			/* Setting back to system trap */
			levelp->av_devcnt = 0;
		}
	}

	if (xxintr == NULL) {
		/* Reset the trap vector to a default system trap */
		vec.instr[0] = MOVPSRL0;

		/* Construct a load instruction */
		vec.instr[1] = MOVL4 | T_INTERRUPT | lvl;

		/* Branch to 'sys_trap' */
		vec.instr[2] = BRANCH | (BRANCH_MASK & (((int)sys_trap -
		    (int)&scb.interrupts[lvl - 1].instr[2]) >> 2));

		vec.instr[3] = MOVL6 | (nwindows - 1);

	} else {
		vec.instr[0] = MOVPSRL0;

		/* Construct a branch instruction with the given trap address */
		offset = (((int)xxintr -
		    (int)&scb.interrupts[lvl - 1].instr[1]) >> 2);

		/* Check that branch displacement is within range */
		if (((offset & ~BRANCH_RANGE) == 0) ||
		    ((offset & ~BRANCH_RANGE) == ~BRANCH_RANGE)) {
			vec.instr[1] = BRANCH | (offset & BRANCH_MASK);
			vec.instr[2] = MOVL6 | (nwindows - 1);
		} else {
			/* Branch is too far, so use a jump */
			vec.instr[1] = SETHIL3 | SETHI_HI((int)xxintr);
			vec.instr[2] = JMPL3 | JMP_LO((int)xxintr);
			vec.instr[3] = MOVL6 | (nwindows - 1);
		}
	}

	write_scb_int(lvl, &vec);		/* set the new vector */
	return (1);
}

/*
 * Increase splvm_val, if necessary, for a device that uses DMA.
 * Level is usually dev->devi_intr->int_pri.
 *
 * XXX keep SPLMB in step.
 */
adddma(level)
	int level;
{
	int spl;

	spl = ipltospl(level);
	if (spl > splvm_val)
		splvm_val = spl;
	SPLMB = spltopri(splvm_val);	/* XXX this may round, */
	splvm_val = pritospl(SPLMB);	/* XXX so convert back */
}

/*
 * store dk name && unit number info
 * reuse empty slots, if possible (loadable drivers, or
 * the case of drives coming and going off line)
 */
int
newdk(name, unit)
	char *name;
	int unit;
{
	int i;
	int j = -1;

	for (i = 0; i < dkn; i++) {
		char *p;

		if ((p = dk_ivec[i].dk_name) == NULL || *p == '\0') {
			j = i;
			break;
		}
	}

	if (j < 0) {
		if (dkn >= DK_NDRIVE) {
			return (-1);
		} else {
			j = dkn++;
		}
	}

	dk_ivec[j].dk_name = name;
	dk_ivec[j].dk_unit = unit;
	return (j);
}

/*
 * remove dk name && unit number info
 */
int
remdk(num)
	int num;
{
	if (num >= dkn || num < 0)
		return (-1);
	else {
		dk_ivec[num].dk_name = "";
		dk_ivec[num].dk_unit = 0;
		dk_time[num] = dk_seek[num] = dk_xfer[num] =
			dk_wds[num] = dk_bps[num] = dk_read[num] = 0;
		/* free up the last one */
		if ((num+1) == dkn)
			dkn = num;
		return (0);
	}
}

#ifdef VDDRV
/*
 * These routines are the inverse of the previous three routines;
 * they allow a driver to be unloaded.
 */

/*
 * remove the autovectored interrupt from the table of mappings of
 * interrupt counts to device names.
 */
/*ARGSUSED*/
void
remioname(levelp)
	struct autovec *levelp;
{

	/*
	 * We could try to remove the name from the av_nametab list, but
	 * that wouldn't buy us much.  We're about to overwrite this
	 * entry anyway, so there is nothing to do here.
	 * So do nothing.
	 */
}

/*
 * Remove a driver from the autovector list.
 * NOTE: if a driver has installed itself on more than one level,
 * then the driver should remove itself from each of those levels.
 */
remintr(lvl, xxintr)
	func xxintr;
{
	register func f;
	register struct autovec *levelp;
	register int i;

	switch (lvl) {

	case 1: levelp = level1; break;
	case 2: levelp = level2; break;
	case 3: levelp = level3; break;
	case 4: levelp = level4; break;
	case 5: levelp = level5; break;
	case 6: levelp = level6; break;
	case 7: levelp = level7; break;
	case 8: levelp = level8; break;
	case 9: levelp = level9; break;
	case 11: levelp = level11; break;
	case 13: levelp = level13; break;
	default:
		printf(
"remintr: cannot remove polling interrupt on processor level %d\n", lvl);
		panic("remintr");
	}
	if ((f = xxintr) == NULL)
		return (0);

	for (i = 0; i < NVECT; i++, levelp++) {
		if (levelp->av_vector == NULL) {
			/* end of list reached */
			break;
		}
		if (levelp->av_vector == f) {
			/* found it */
			remioname(levelp);
			/* move them all down one */
			for (i++; i < NVECT; i++, levelp++)
				levelp[0] = levelp[1];
			return (0);
		}
	}

	/* one way or another, it isn't in the list */
	printf("remintr: driver not installed on level %d\n", lvl);
	return (-1);
}

#endif VDDRV

/*
 * Configure swap space and related parameters.
 */
swapconf()
{
#ifndef SAS
	register struct bootobj *swp;
	register int nblks;
	struct vattr vattr;
	struct vfs *vfsp;
	struct vfssw *vsw;
	int error;
	extern char *strcpy();
	extern u_int swapwarn;		/* from param.c	*/

	swp = &swapfile;
	error = ENODEV;
	vfsp = NULL;
	/*
	 * No filesystem type specified, try to get default
	 */
	if (((boothowto & RB_ASKNAME) || (*(swp->bo_fstype) == '\0')) &&
	    (vsw = getfstype("swap", swp->bo_fstype))) {
		(void) strcpy(swp->bo_fstype, vsw->vsw_name);
		vfsp = (struct vfs *)kmem_alloc(sizeof (*vfsp));
		VFS_INIT(vfsp, vsw->vsw_ops, 0);
		error = VFS_SWAPVP(vfsp, &swp->bo_vp, swp->bo_name);
	} else if (*swp->bo_name == '/') {
		/*
		 * file name begins with a slash, try to look it up
		 * as a pathname starting at the root.
		 */
		error = lookupname(swp->bo_name, UIO_SYSSPACE,
		    FOLLOW_LINK, (struct vnode **)0, &swp->bo_vp);
		printf("swapconf: looked up %s, got error %d\n",
		    swp->bo_name, error);
		if (!error) {
			(void) strcpy(swp->bo_fstype, rootfs.bo_fstype);
		}
	} else {
		/*
		 * Look through all supported filesystem types for one
		 * that matches the type of swp, or if swp has no
		 * fstype just take the first filesystem type that
		 * will supply a swap vp
		 */
		for (vsw = &vfssw[0]; vsw <= &vfssw[MOUNT_MAXTYPE];
		    vsw++) {
			if (vsw->vsw_name == 0) {
				continue;
			}
			if (*(swp->bo_fstype) == '\0' ||
			    (strcmp(swp->bo_fstype, vsw->vsw_name) == 0)) {
				vfsp = (struct vfs *)kmem_alloc(sizeof (*vfsp));
				VFS_INIT(vfsp, vsw->vsw_ops, 0);
				error = VFS_SWAPVP(vfsp, &swp->bo_vp,
				    swp->bo_name);
				if (error == 0) {
					break;
				}
			}
		}
	}
	if (error || swp->bo_vp == NULL) {
		if (error == ENODEV)
			printf("swap device '%s': Not Present\n", swp->bo_name);
		else
			printf("bad swap device '%s': error %d\n", swp->bo_name,
				error);
bad:
		if (swp->bo_vp) {
			VN_RELE(swp->bo_vp);
			swp->bo_vp = NULL;
		}
		if (vfsp) {
			kmem_free((char *)vfsp, sizeof (*vfsp));
		}
		if (error)
			return;
	} else {
		error = VOP_OPEN(&swp->bo_vp, FREAD|FWRITE, u.u_cred);
		if (error) {
			printf("error %d in opening swap file %s\n",
			    error, swp->bo_name);
			goto bad;
		}
		error = VOP_GETATTR(swp->bo_vp, &vattr, u.u_cred);
		if (error) {
			printf("error %d getting size of swap file %s\n",
			    error, swp->bo_name);
			/*
			 * Let the release of the vnode above cause the call
			 * of the close routine for the device.
			 */
			goto bad;
		}
		nblks = btodb(vattr.va_size);
		if (swp->bo_size == 0 || swp->bo_size > nblks)
			swp->bo_size = nblks;
		if (*swp->bo_fstype == '\0') {
			(void) strcpy(swp->bo_fstype, vsw->vsw_name);
		}
#ifdef VFSSTATS
		if (vfs_add(swp->bo_vp, vfsp, 0) == 0 &&
		    vfsp->vfs_stats == 0) {
			struct vfsstats *vs;

			vs = (struct vfsstats *)
			    kmem_zalloc(sizeof (struct vfsstats));
			vs->vs_time = time.tv_sec;
			vfsp->vfs_stats = (caddr_t)vs;
		}
#endif
		printf("swap on %s fstype %s size %dK\n",
		    swp->bo_name, swp->bo_fstype,
		    dbtob(swp->bo_size) / 1024);
		if (nblks < swapwarn)
			printf("WARNING: swap space probably too small\n");
		swp->bo_flags |= BO_VALID;
		swap_present = 1;
	}
#else SAS
	register struct bootobj *swp;
	struct vattr vattr;
	int error;

	swp = &swapfile;
	if (strcmp(swp->bo_name, "sim0") != 0) {
		printf("bad swap name '%s', should be 'sim0'\n",
		    swp->bo_name);
		panic("bad swap device");
	}
	/* swp->bo_vp = makespecvp(makedev(1, 1), VBLK); */
	swp->bo_vp = bdevvp(makedev(1, 1));
	error = VOP_OPEN(&swp->bo_vp, FREAD+FWRITE, u.u_cred);
	if (error) {
		printf("error %d opening swap\n",
		    error);
		panic("cannot open swap device");
	}
	error = VOP_GETATTR(swp->bo_vp, &vattr, u.u_cred);
	if (error) {
		printf("error %d getting size of swap\n",
		    error);
		panic("cannot get swap size");
	}
	swp->bo_size = btodb(vattr.va_size);
	if (*swp->bo_fstype == '\0') {
		(void) strcpy(swp->bo_fstype, "spec");
	}
	printf("swap on %s fstype %s size %dK\n",
	    swp->bo_name, swp->bo_fstype,
	    dbtob(swp->bo_size) / 1024);
	swp->bo_flags |= BO_VALID;
	swap_present = 1;
#endif SAS
}

dev_t	kbddev = NODEV;
dev_t	mousedev = NODEV;
#define	NOUNIT		-1

#ifdef SAS
#define	SIMCMAJOR	12	/* simulated console */
#else !SAS
#define	WSCONSMAJOR	1	/* "workstation console" */
#define	ZSMAJOR		12	/* serial */
#define	KBDMINOR 	0
#define	MOUSEMINOR	1
#define	CONSKBDMAJOR	29	/* console keyboard driver */
#define	CONSMOUSEMAJOR	13	/* console mouse driver */
#endif !SAS

extern	struct fileops vnodefops;

struct consinfo {
	int kmunit;	/* keyboard/mouse unit number */
	dev_t fbdev;	/* frame buffer device */
	struct dev_info *options; /* options dev_info (for keyclick et al.) */
};

/*
 * Configure keyboard, mouse, and frame buffer using
 *	monitor provided values
 *	configuration flags
 * N.B. some console devices are statically initialized by the
 * drivers to NODEV.
 */
consconfig()
{
	int e;
#ifndef SAS
	struct consinfo consinfo;

	struct termios termios;
	int kbdtranslatable = TR_CANNOT;
	int kbdspeed = B9600;
	struct vnode *kbdvp;
	struct vnode *conskbdvp;
	struct file *fp;
	int i;
	char	devname[OBP_MAXDEVNAME];
	int	unit;
	int findcons(/* (struct dev_info *), (struct consinfo *) */);
	int zsminor;
	char *p;
	extern char *prom_get_stdin_subunit();

	stop_mon_clock();	/* turn off monitor polling clock */

	consinfo.kmunit = NOUNIT;
	consinfo.fbdev = NODEV;
	consinfo.options = NULL;

	/*
	 * check for console on same ascii port to allow full speed
	 * output by using the UNIX driver and avoiding the monitor.
	 */

	if (prom_stdin_stdout_equivalence() == 0)
		goto namedone;

	if (prom_get_stdin_dev_name(devname, sizeof devname) != 0)
		goto namedone;

	if (strcmp("zs", devname) != 0)
		goto namedone;

	if ((unit = prom_get_stdin_unit()) == -1)
		goto namedone;

	zsminor = 0;
	if ((p = prom_get_stdin_subunit()) != (char *)0)
		if ((*p) != (char)0)
			zsminor = (*p - 'a') & 1;

	zsminor = (unit << 1) + zsminor;

#ifdef	XPRINTF
	XPRINTF("consconfig: rconsdev zs minor device number <%d>\n", zsminor);
#endif	XPRINTF

	rconsdev = makedev(ZSMAJOR, zsminor);

#else SAS
	rconsdev = makedev(SIMCMAJOR, 0);
#endif SAS

namedone:

	if (rconsdev) {
		/*
		 * Console is a CPU serial port.
		 */

#ifdef	XPRINTF
		XPRINTF("Console is a serial device\n");
#endif	XPRINTF

		rconsvp = makespecvp(rconsdev, VCHR);
		kbddev = rconsdev;
		/*
		 * Opening causes interrupts, etc. to be initialized.
		 * Console device drivers must be able to do output
		 * after being closed!
		 */
		if (e = VOP_OPEN(&rconsvp, FREAD|FWRITE|FNOCTTY|FNDELAY, u.u_cred))
			printf("console open failed: error %d\n", e);
		/* now we must close it to make console logins happy */
		VOP_CLOSE(rconsvp, FREAD|FWRITE, 1, u.u_cred);
		consdev = rconsdev;
		consvp = rconsvp;
#ifndef SAS
		/* Still need to find options dev_info */
		walk_devs(top_devinfo, findcons, (char *) &consinfo);
		options_devinfo = consinfo.options;
		set_keyclick(options_devinfo);
#endif /* SAS */
		return;
	}
#ifndef SAS

	/*
	 * The following code assumes mappings for ZS minor devices:
	 *
	 * ttya/zs devinfo unit 0/Channel a --> zs minor device #0
	 * ttyb/zs devinfo unit 0/Channel b --> zs minor device #1
	 *
	 * See comments concerning zs minor dev mappings in zs_async.c
	 */

	if (prom_stdin_is_keyboard())  {
#ifdef	XPRINTF
		XPRINTF("consconfig <kbd>: Using physical keyboard\n");
#endif	XPRINTF
		goto kbddone;
	}

	if (prom_get_stdin_dev_name(devname, sizeof devname) != 0)
		goto kbddone;

	if (strcmp("zs", devname) != 0)
		goto kbddone;

	if ((unit = prom_get_stdin_unit()) != -1)  {

		zsminor = 0;
		if ((p = prom_get_stdin_subunit()) != (char *)0)
			if (*p != (char)0)
				zsminor = (*p - 'a') & 1;

		zsminor = (unit << 1) + zsminor;

#ifdef	XPRINTF
		XPRINTF("consconfig: remote kbddev zs minor dev <%d>\n",
		    zsminor);
#endif	XPRINTF

		kbddev = makedev(ZSMAJOR, zsminor);
	}

kbddone:

	if (kbddev != NODEV)
		kbdspeed = zsgetspeed(kbddev);

	/*
	 * Look for the [last] kbd/ms and matching fbtype.
	 */
	walk_devs(top_devinfo, findcons, (char *) &consinfo);
	options_devinfo = consinfo.options;
	set_keyclick(options_devinfo);

	/*
	 * Use serial keyboard and mouse if found flagged uart
	 */
	if (consinfo.kmunit != NOUNIT) {
		if (mousedev == NODEV)
			mousedev = makedev(ZSMAJOR,
				consinfo.kmunit * 2 + MOUSEMINOR);
		if (kbddev == NODEV) {
			kbddev = makedev(ZSMAJOR,
				consinfo.kmunit * 2 + KBDMINOR);
			kbdtranslatable = TR_CAN;
		}
	}
	if (kbddev == NODEV)
		panic("no keyboard found");
	/*
	 * Open the keyboard device.
	 */
	kbdvp = makespecvp(kbddev, VCHR);
	if (e = VOP_OPEN(&kbdvp, FREAD|FNOCTTY, u.u_cred)) {
		printf("keyboard open failed: error %d\n", e);
		panic("can't open keyboard");
	}
	e = FLUSHRW;			/* Flush both read and write */
	strioctl(kbdvp, I_FLUSH, (caddr_t)&e, FREAD);
	u.u_error = 0;			/* Just In Case */

	if (!ldreplace(kbdvp, kbdvp->v_stream, KBDLDISC))
		u.u_error = EINVAL;
	if (u.u_error != 0) {
		printf("consconfig: can't set line discipline: error %d\n",
		    u.u_error);
		panic("can't set up keyboard");
	}
	strioctl(kbdvp, KIOCTRANSABLE, (caddr_t)&kbdtranslatable, 0);
	if (kbdtranslatable == TR_CANNOT) {
		/*
		 * For benefit of serial port keyboard.
		 */
		strioctl(kbdvp, TCGETS, (caddr_t)&termios, FREAD);
		termios.c_cflag &= ~(CBAUD|CIBAUD);
		termios.c_cflag |= kbdspeed;
		strioctl(kbdvp, TCSETSF, (caddr_t)&termios, FREAD);
		if (u.u_error != 0) {
			printf("consconfig: TCSETSF error %d\n", u.u_error);
			u.u_error = 0;
		}
	}

	/*
	 * Open the "console keyboard" device, and link the keyboard
	 * device under it.
	 * XXX - there MUST be a better way to do this!
	 */
	conskbdvp = makespecvp(makedev(CONSKBDMAJOR, 1), VCHR);
	if (e = VOP_OPEN(&conskbdvp, FREAD|FWRITE|FNOCTTY, u.u_cred)) {
		printf("console keyboard device open failed: error %d\n", e);
		panic("can't open keyboard");
	}
	if ((fp = falloc()) == NULL)
		panic("can't get file descriptor for keyboard");
	i = u.u_r.r_val1;	/* XXX - get FD number */
	fp->f_flag = FREAD;
	fp->f_type = DTYPE_VNODE;
	fp->f_data = (caddr_t)kbdvp;
	fp->f_ops = &vnodefops;
	strioctl(conskbdvp, I_PLINK, (caddr_t)&i, FREAD);
	if (u.u_error != 0) {
		printf("I_PLINK failed: error %d\n", u.u_error);
		panic("can't link kbddev under /dev/kbd");
	}
	u.u_ofile[i] = NULL;
	closef(fp);	/* don't need this any more */

	kbddevopen = 1;

	/* try to configure mouse */
	if (mousedev != NODEV && mouseconfig(mousedev)) {
#if !defined(MUNIX) && !defined(MINIROOT)
		printf("No mouse found\n");
#endif
		mousedev = NODEV;
		u.u_error = 0;
	}

	/*
	 * Set up default frame buffer.
	 */
	if (fbdev == NODEV && (fbdev = consinfo.fbdev) == NODEV) {
#if !defined(MUNIX) && !defined(MINIROOT)
		printf("No default frame buffer found\n");
#endif
	}

	/*
	 * Open the "workstation console" device, and link the "console
	 * keyboard" device under it.
	 * XXX - there MUST be a better way to do this!
	 */
	rconsdev = makedev(WSCONSMAJOR, 0);
	rconsvp = makespecvp(rconsdev, VCHR);
	if (e = VOP_OPEN(&rconsvp, FREAD|FWRITE|FNOCTTY, u.u_cred)) {
		printf("console open failed: error %d\n", e);
		panic("can't open console");
	}
	if ((fp = falloc()) == NULL)
		panic("can't get file descriptor for console keyboard");
	i = u.u_r.r_val1;	/* XXX - get FD number */
	fp->f_flag = FREAD;
	fp->f_type = DTYPE_VNODE;
	fp->f_data = (caddr_t)conskbdvp;
	fp->f_ops = &vnodefops;
	strioctl(rconsvp, I_PLINK, (caddr_t)&i, FREAD);
	if (u.u_error != 0) {
		printf("consconfig: I_PLINK failed: error %d\n", u.u_error);
		panic("can't link /dev/kbd under workstation console");
	}
	u.u_ofile[i] = NULL;
	closef(fp);	/* don't need this any more */
	/* now we must close it to make console logins happy */
	VOP_CLOSE(rconsvp, FREAD|FWRITE, 1, u.u_cred);
	consdev = rconsdev;
	consvp = rconsvp;
#else SAS
	panic("no keyboard found");
#endif SAS
}

#ifndef SAS
static
mouseconfig(msdev)
	dev_t msdev;
{
	int e;
	int flushrw = FLUSHRW;
	struct vnode *mousevp;
	struct vnode *consmousevp;
	struct file *fp;
	int fd;

	/* Open the mouse device. */
	mousevp = makespecvp(msdev, VCHR);
	if (e = VOP_OPEN(&mousevp, FREAD|FNOCTTY, u.u_cred))
		return (e);
	strioctl(mousevp, I_FLUSH, (caddr_t) &flushrw, FREAD);
	u.u_error = e = 0;		/* Just In Case */

	if (!ldreplace(mousevp, mousevp->v_stream, MOUSELDISC))
		e = EINVAL;
	if (e != 0) {
		(void) VOP_CLOSE(mousevp, FREAD|FWRITE, 1, u.u_cred);
		return (e);
	}

	/*
	 * Open the "console mouse" device, and link the mouse device under it.
	 * XXX - there MUST be a better way to do this!
	 */

	consmousevp = makespecvp(makedev(CONSMOUSEMAJOR, 0), VCHR);
	if (e = VOP_OPEN(&consmousevp, FREAD|FWRITE|FNOCTTY, u.u_cred)) {
		(void) VOP_CLOSE(mousevp, FREAD, 1, u.u_cred);
		return (e);
	}
	if ((fp = falloc()) == NULL) {
		(void) VOP_CLOSE(consmousevp, FREAD|FWRITE, 1, u.u_cred);
		(void) VOP_CLOSE(mousevp, FREAD, 1, u.u_cred);
		return (e = EINVAL);
	}
	fd = u.u_r.r_val1;	/* XXX - get FD number */
	fp->f_flag = FREAD;
	fp->f_type = DTYPE_VNODE;
	fp->f_data = (caddr_t) mousevp;
	fp->f_ops = &vnodefops;
	strioctl(consmousevp, I_PLINK, (caddr_t) &fd, FREAD);
	if ((e = u.u_error) != 0) {
		(void) VOP_CLOSE(consmousevp, FREAD|FWRITE, 1, u.u_cred);
		(void) VOP_CLOSE(mousevp, FREAD, 1, u.u_cred);
		return (e);
	}
	u.u_ofile[fd] = NULL;

	/* don't need this any more */
	closef(fp);

	/* we're done with this, as well */
	(void) VOP_CLOSE(consmousevp, FREAD|FWRITE, 1, u.u_cred);

	VN_RELE(consmousevp);

	return (e);
}

/*
 * Look for the [last] kbd/ms and default frame buffer.
 */
findcons(dev, p)
	struct dev_info *dev;
	struct consinfo *p;
{
	static int fbid;
	dev_t finddev();

	/* options doesn't necessarily have a driver */
	if (strcmp(dev->devi_name, "options") == 0) {
		p->options = dev;
		/* assume can't also be a frame buffer or zs */
		return;
	}

	if (dev->devi_driver == NULL)
		return;

	if (strncmp(dev->devi_name, "zs", 2) == 0) {
		/*
		 * production proms use "keyboard" property.
		 * XXX pre-production proms use "flags".
		 * XXX kernel must accept either, at least until FCS.
		 */
		if (getprop(dev->devi_nodeid, "keyboard", 0) ||
		    (getprop(dev->devi_nodeid, "flags", 0) & ZS_KBDMS)) {
			p->kmunit = dev->devi_unit;
		}
		/* assume that serial ports are not frame buffers */
		return;
	}

	if (fbid == 0)
		fbid = getprop(top_devinfo->devi_nodeid, "fb", -1);

	if (fbid == dev->devi_nodeid)
		p->fbdev = finddev('c', dev);

}
/*#endif SAS*/

/* convert dev_info pointer to device number */
dev_t
finddev(type, info)
	int type;
	struct dev_info *info;
{
	int majnum = -1;
	int (*openfun)();

	openfun = info->devi_driver->devo_open;

	if (type == 'b') {
#ifdef notdef	/* no one is using this at the moment */
		struct bdevsw *swp = bdevsw;

		for (majnum = 0; majnum < nblkdev; majnum++, swp++)
			if (swp->d_open == openfun)
				break;
#endif
	} else {
		struct cdevsw *swp = cdevsw;

		for (majnum = 0; majnum < nchrdev; majnum++, swp++)
			if (swp->d_open == openfun)
				break;
	}

	if (majnum < 0)
		return (NODEV);

	return (makedev(majnum, info->devi_unit));
}

int keyclick;

set_keyclick(dev)
	struct dev_info	*dev;
{
	addr_t key_click;
	int opt_nodeid;
	static char key_click_name[] = "keyboard-click?";
	int size;

	if (dev == NULL)
		return;
	opt_nodeid = dev->devi_nodeid;
	size = getproplen(opt_nodeid, key_click_name);
	if (size <= 0) {
		printf("No \"%s\" property\n", key_click_name);
		return;
	}

	key_click = getlongprop(opt_nodeid, key_click_name);
	if (key_click) {
		if (strcmp(key_click, "true") == 0)
			keyclick = 1;
		kmem_free(key_click, (u_int)size);
	}
}

#endif SAS
/*
 * Some things, like cputype, are contained in the idprom, but are
 * needed and obtained earlier; hence they are not set (again) here.
 */
idprom()
{
	register u_char *cp, val = 0;
	register int i;
	struct idprom id;

	getidprom((char *)&id);
	cp = (u_char *)&id;
	for (i = 0; i < 16; i++)
		val ^= *cp++;
	if (val != 0)
		printf("Warning: NVRAM checksum error\n");
	if (id.id_format == 1) {
		(void) localetheraddr((struct ether_addr *)id.id_ether,
		    (struct ether_addr *)NULL);
	} else
		printf("Invalid format code in NVRAM\n");
}

/*
 * We set the cpu type from the idprom machine ID.
 * XXX CPU name should be PROM property.
 */
setcputype()
{
	struct idprom id;

	cpu = -1;

	getidprom((char *)&id);
	if (id.id_format == 1) {
		cpu = id.id_machine;
		if ((cpu & CPU_ARCH) != SUN4C_ARCH) {
			printf("Unknown machine type 0x%x in NVRAM\n", cpu);
			cpu = -1;
		}
	} else {
		printf("Invalid format type in NVRAM\n");
	}

	if (cpu == -1) {
		printf("Machine type set to Sun-4/60\n");
		cpu = CPU_SUN4C_60;
	}
}

machineid()
{
	struct idprom id;
	register int x;

	getidprom((char *)&id);
	x = id.id_machine << 24;
	x += id.id_serial;
	return (x);
}

/*
 * Initialize pseudo-devices.
 * Reads count and init routine from table in ioconf.c
 * created by 'init blah' syntax on pseudo-device line in config file.
 * Calls init routine once for each unit configured
 */

extern struct pseudo_init {
	int 	ps_count;
	int	(*ps_func)();
} pseudo_inits[];

pseudoconfig()
{
	register struct pseudo_init *ps;
	int unit;

	for (ps = pseudo_inits; ps->ps_count > 0; ps++)
		for (unit = 0; unit < ps->ps_count; unit++)
			(*ps->ps_func)(unit);
}

#ifndef SAS
/*
 * Return slot number if given physical address corresponds to
 * slave-only SBus slot, e.g. 4/60 slot 3; return -1 if OK.
 */
slaveslot(addr)
	addr_t addr;
{
	int slot;

	slot = (((int) addr) - SBUS_BASE) / SBUS_SIZE;
	return ((getprop(0, "slave-only", 8) & (1 << slot)) ? slot : -1);
}
#endif !SAS
