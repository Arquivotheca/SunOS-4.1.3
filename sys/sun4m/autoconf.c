#ident "@(#)autoconf.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1987-1991 by Sun Microsystems, Inc.
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
#include <machine/module.h>

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

#ifdef IOMMU
struct map *vme32map;
struct map *vme24map;
struct map *altsbusmap;
struct map *sbusmap;
struct map *bigsbusmap;
struct map *mbutlmap;

/* this is here to keep non-OBP driver happay */
struct mb_hd mb_hd;
#endif
struct map *dvmamap;

struct dev_opslist *dev_opslist = 0;
extern struct dev_opslist av_opstab[];
struct dev_info *top_devinfo;
struct dev_info *options_devinfo;

#ifdef	VME
struct dev_info *vme_devinfo;	/* To store vme dev_info pointer */
int	found_vme = 0;
struct vme_spaces {
	char	       *name;	/*  */
	unsigned	cookie;
	unsigned	space;
	unsigned	base;
} vme_spaces[] = {
	{ "vme16d16", SP_VME16D16, 0xC, 0xFFFF0000 },
	{ "vme24d16", SP_VME24D16, 0xC, 0xFF000000 },
	{ "vme32d16", SP_VME32D16, 0xC, 0x00000000 },
	{ "vme16d32", SP_VME16D32, 0xD, 0xFFFF0000 },
	{ "vme24d32", SP_VME24D32, 0xD, 0xFF000000 },
	{ "vme32d32", SP_VME32D32, 0xD, 0x00000000 },
	{ NULL },
};
void add_ipi_drive();
#endif	VME

int	svimap[] = { 0, 0, 1, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 0 };
#define	LPL2SPL(n)	((n)&15)
#define	LPL2SVL(n)	svimap[LPL2SPL(n)]


static char *out_of_range =
    "Warning: Out of range %s specification in device node <%s>\n";

void
apply_range_to_reg(name, nrange, rangep, nreg, regp)
char *name;
int nrange;
struct dev_rng *rangep;
int nreg;
struct dev_reg *regp;
{
	register int i;
	register struct dev_rng *rp;

	if (nrange>0)
		while (nreg-->0) {
			for (i = 0, rp = rangep; i < nrange; ++i, ++rp)
				if (regp->reg_bustype == (int)rp->rng_cbustype)
					break;
			if (i == nrange)  {
				printf(out_of_range, "register", name);
				continue;
			}
			regp->reg_addr += (u_int)rp->rng_offset;
			regp->reg_bustype = (int)rp->rng_bustype;
			++regp;
		}
}

void
apply_range_to_range(name, nr, pp, nc, cp)
char *name;
int nr;			/* Number of Parent ranges */
struct dev_rng *pp;	/* Parent ranges */
int nc;			/* Number of child ranges */
struct dev_rng *cp;	/* Child ranges */
{
	register int i;
	register struct dev_rng *rp;

	if (nr>0)
		while (nc-->0) {
			for (i = 0, rp = pp; i < nr; ++i, ++rp)
				if (cp->rng_bustype == rp->rng_cbustype)
					break;
			if (i == nr)  {
				printf(out_of_range, "range", name);
				continue;
			}
			cp->rng_offset += rp->rng_offset;
			cp->rng_bustype = rp->rng_bustype;
			++cp;
		}
}

/*
 * This routine fills in the common information for a given dev_info struct.
 * Using the nodeid that is already initialized, it pulls the following
 * information out of the PROMS:
 *	name, reg, range, intr
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
	static int curzs = 0;

	nodeid = dev_info->devi_nodeid;
	dev_info->devi_name = (char *)getlongprop(nodeid, "name");

#ifdef	SUN4M_690
	/*
	 * XXX: Since we're using the generic attach_devs function for
	 * XXX: children of the "SUNW,pn" node, and it does not follow the
	 * XXX: "obio" model, we need to escape at this point...
	 * XXX: (The reg property in the "ipi3sc" node does not describe
	 * XXX: byte addressable space in it's parent's space.)
	 */

	if (strcmp(dev_info->devi_name, "ipi3sc") == 0)
		return;
#endif	SUN4M_690

	/*
	 * We try to match stdin/stdin (which is the "second" zs,
	 * if console/kbd) while attaching the first zs device.
	 * We need the unit number initialized.
	 */
	if (strcmp("zs", dev_info->devi_name) == 0)
		dev_info->devi_unit = curzs++;

	dev_info->devi_nintr = getproplen(nodeid, "intr") /
		sizeof (struct dev_intr);
	if (dev_info->devi_nintr > 0)
		dev_info->devi_intr =
			(struct dev_intr *)getlongprop(nodeid, "intr");

	dev_info->devi_nreg = getproplen(nodeid, "reg") /
		sizeof (struct dev_reg);
	if (dev_info->devi_nreg > 0) {
		dev_info->devi_reg =
			(struct dev_reg *)getlongprop(nodeid, "reg");
		/*
		 * The "reg" property is relative to the
		 * range specified by the node's parent.
		 * Convert the relative data from the prom into
		 * absolute data by combining with the absolute
		 * range information stored in the parent.
		 */
		if ((dev_info->devi_parent) &&
		    (dev_info->devi_parent->devi_nrng > 0) &&
		    (dev_info->devi_parent->devi_rng))
			apply_range_to_reg(dev_info->devi_name,
				    dev_info->devi_parent->devi_nrng,
				    dev_info->devi_parent->devi_rng,
				    dev_info->devi_nreg,
				    dev_info->devi_reg);
	}
	dev_info->devi_nrng = getproplen(nodeid, "ranges") /
		sizeof (struct dev_rng);
	if (dev_info->devi_nrng > 0) {
		dev_info->devi_rng =
			(struct dev_rng *)getlongprop(nodeid, "ranges");

		/*
		 * The "ranges" property is relative to the
		 * range specified by the node's parent.
		 * Convert the relative data from the prom into
		 * absolute data by combining with the absolute
		 * range information stored in the parent.
		 */
		if ((dev_info->devi_parent) &&
		    (dev_info->devi_parent->devi_nrng > 0) &&
		    (dev_info->devi_parent->devi_rng))
			apply_range_to_range(dev_info->devi_name,
				    dev_info->devi_parent->devi_nrng,
				    dev_info->devi_parent->devi_rng,
				    dev_info->devi_nrng,
				    dev_info->devi_rng);
	} else if (dev_info->devi_parent) {
		/* inherit range from parent */
		dev_info->devi_nrng = dev_info->devi_parent->devi_nrng;
		dev_info->devi_rng = dev_info->devi_parent->devi_rng;
	} else {
		/* root node. default is "nothing". */
		dev_info->devi_nrng = 0;
		dev_info->devi_rng = (struct dev_rng *)0;
	}
}

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

	for (ops = dev_opslist; ops->devl_ops != NULL; ops = ops->devl_next)
		if (ops->devl_ops->devo_identify(name))
			return (ops->devl_ops);
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

#ifdef	VME
/* Check if device already a child of vme */
/*ARGSUSED*/
static int
vme_check_dupdev(dev_info, name, unit)
	struct dev_info *dev_info;
	char *name;
	int unit;
{
	struct dev_info *dev;

#ifdef	DEBUG_IPI
	printf("vme_check_dupdev: looking for dup of %s, unit#0x%X\n",
		name, unit);
#endif

	/*
	 * %%% HACK for building the ipi related dev_info
	 * nodes.  We are throwing out everything we dont need.
	 * The IPI related devinfo nodes are created via the pseudo
	 * (sbus style) drivers included in this file.
	 */
	if ((strcmp(name, "isc") == 0) ||
	    (strcmp(name, "is") == 0) ||
	    (strcmp(name, "idc") == 0) ||
	    (strcmp(name, "id") == 0))  {
#ifdef	DEBUG_IPI
	printf("vme_check_dupdev: THROWING OUT %s, unit#0x%X\n", 
		name, unit);
#endif
		return (1);
	}

	for (dev = dev_info->devi_slaves; dev != NULL; dev = dev->devi_next) {
		if (strcmp(dev->devi_name, name) == 0) {
			if (dev->devi_unit == unit) {
#ifdef	DEBUG_IPI
	printf("vme_check_dupdev: FOUND dup of %s, unit#0x%X\n",
		name, unit);
#endif
				return (1);	/* found a dup */
			}
		}
	}
#ifdef	DEBUG_IPI
	printf("vme_check_dupdev: found NO dup of %s, unit#0x%X\n",
		name, unit);
#endif
	return (0);
}
#endif	VME


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

#define	DEFAULT_SPLVM	(ipltospl(9))
/*
 * Maximum interrupt priority used by Mainbus DMA.
 * This value is determined as each driver that uses DMA calls adddma
 * with the hardware interrupt priority level that they use.  Adddma
 * increases splvm_val as necessary, and computes SPLMB based on it.
 *
 * XXX SPLMB is for backward compatibility only.
 * XXX Drivers should call splvm() instead of splr(pritospl(SPLMB)).
 */
int SPLMB = spltopri(DEFAULT_SPLVM);	/* reasonable default */

/*
 * We use splvmval to implement splvm().
 */
int splvm_val = DEFAULT_SPLVM;		/* reasonable default */

extern struct machinfo mach_info;
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
	 * Configure devinfo-style devices.
	 */
	sbus_config();

	/*
	 * Configure VME (mainbus-style) devices.
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


#ifndef	NOPROM
sbus_config()
{

	extern dnode_t prom_nextnode();

	/*
	 * chain together the opslist that we're built with
	 */
	init_opslist();

	/*
	 * build the dev_info tree, attaching devices along the way
	 */
	top_devinfo = (struct dev_info *) kmem_zalloc(sizeof (*top_devinfo));
	top_devinfo->devi_nodeid = (int)prom_nextnode((dnode_t)0);
	fill_devinfo(top_devinfo);
	attach_devs(top_devinfo);
}
#else	NOPROM
/*
 * SAS doesn't not simulate the open prom, and only has a
 * simulated console and simulated disk.  The attach and probe
 * routines of these simulated devices don't do much, so there
 * is probably not much to do here.  May have to add more here
 * though, so that data structures get initialized?
 * console:
 * 	simcprobe: does nothing.
 *	simcattach: does a little bit of initialization.
 *	simdprobe: doesn't exist.
 *	simdattach: does nothing.
 */
sbus_config()
{
#ifdef SAS
	simcattach();
	/* probably will have to do more for SAS */
#endif
}
#endif	NOPROM

int
obio_identify(name)
	char *name;
{
	if (strcmp(name, "obio"))
		return 0;
	return 1;
}

int
obio_attach(devi)
	struct dev_info *devi;
{
	attach_devs(devi);
	return (0);
}

struct dev_ops obio_ops = {
	1,
	obio_identify,
	obio_attach,
};

int
iommu_identify(name)
	char *name;
{
	if (strcmp(name, "iommu"))
		return 0;
	return 1;
}

int
iommu_attach(devi)
	struct dev_info *devi;
{
	attach_devs(devi);
	return (0);
}

struct dev_ops iommu_ops = {
	1,
	iommu_identify,
	iommu_attach,
};

int
sbus_identify(name)
	char *name;
{
	if (strcmp(name, "sbus"))
		return 0;
	return 1;
}

int
sbus_attach(devi)
	struct dev_info *devi;
{
	attach_devs(devi);
	return (0);
}

struct dev_ops sbus_ops = {
	1,
	sbus_identify,
	sbus_attach,
};

#ifdef	VME
int
vme_identify(name)
	char *name;
{
	if (strcmp(name, "vme"))
		return (0);
	return (1);
}

int
vme_attach(devi)
	struct dev_info *devi;
{
	attach_devs(devi);
	return (0);
}

struct dev_ops vme_ops = {
	1,
	vme_identify,
	vme_attach,
};
#endif	VME

#ifdef	SUN4M_690
/*
 * Pseudo device drivers for Self-identifying VME/IPI
 */
int
pn_identify(name)
	char *name;
{
	if (strcmp(name, "SUNW,pn") == 0)
		return (1);
	return (0);
}

int
pn_attach(devi)
	register struct dev_info *devi;
{
	/*
	 * XXX: Stop report_dev from printing all 5 sets of regs and intrs.
	 */

	register int nregs = devi->devi_nreg;
	register int nintr = devi->devi_nintr;

	devi->devi_nreg = (nregs > 0 ? 1 : 0);
	devi->devi_nintr = (nintr > 0 ? 1 : 0);
	report_dev(devi);
	devi->devi_nreg = nregs;
	devi->devi_nintr = nintr;

	attach_devs(devi);
	return (0);
}

struct dev_ops pn_ops = {
	1,
	pn_identify,
	pn_attach,
};

int
idpseudo_identify(name)
	char *name;
{
	if (strcmp(name, "id") == 0)
		return (1);
	return (0);
}

/*ARGSUSED*/
int
idpseudo_attach(devi)
	register struct dev_info *devi;
{
#ifdef	IPI_DEBUG
	register struct dev_info *pdp = devi->devi_parent;

	printf("%s%d at %s%d", devi->devi_name, devi->devi_unit,
	    pdp->devi_name, pdp->devi_unit);
#endif	IPI_DEBUG

	return (0);
}

struct dev_ops idpseudo_ops = {
	1,
	idpseudo_identify,
	idpseudo_attach,
};

int
ipi3sc_identify(name)
	char *name;
{
#ifdef	IPI_DEBUG
	prom_printf("ipi3sc_identify: name %s\n", name);
#endif	IPI_DEBUG

	if (strcmp(name, "ipi3sc") == 0)
		return (1);
#ifndef	NO_IDC_COMPATIBILITY
	/*
	 * XXX: For compatability with pre-FCS PROMS, also identify "idc".
	 */

	if (strcmp(name, "idc") == 0)
		return (1);
#endif	!NO_IDC_COMPATIBILITY
	return (0);
}

/*
 * XXX: Fill in the unit number, properly, and create "id" children.
 * XXX: This is a terrible hack, that augments the mbconfig stuff,
 * XXX: basically, so we can ignore it, later.  Assumption is 8 drives
 * XXX: per slave controller.
 */

int
ipi3sc_attach(devi)
	struct dev_info *devi;
{
	register struct dev_reg *rp;
	register struct dev_info *dp;
	int n, length;

#ifdef	IPI_DEBUG
	prom_printf("ipi3sc_attach...\n");
#endif	IPI_DEBUG

	/*
	 * For device "ipi3sc", the channel number is expressed in the
	 * "bustype" field of the device's reg property (which does not
	 * refer to byte addressable space).
	 */

	length = getproplen(devi->devi_nodeid, "reg");
	if (length == -1)
		length = 0;
	ASSERT(length != 0);
	rp = (struct dev_reg *)getlongprop(devi->devi_nodeid, "reg");

	n = devi->devi_unit = rp->reg_bustype;

#ifndef	NO_IDC_COMPATIBILITY
	if (strcmp("ipi3sc", devi->devi_name) != 0)  {		/* For idc */
		/*
		 * For device "idc", * the channel number is computed from the
		 * address field.
		 */
		n = devi->devi_unit = (int)(((int)rp->reg_addr & 0x1c00) >> 10);
	}
#endif	NO_IDC_COMPATIBILITY

	(void)kmem_free((caddr_t)rp, (u_int)length);
	dp = devi->devi_parent;

#ifndef	NO_IDC_COMPATIBILITY
	if (strcmp("ipi3sc", devi->devi_name) == 0)
#endif	NO_IDC_COMPATIBILITY
	printf("%s channel #%d at %s%d\n", devi->devi_name, n,
	    dp->devi_name, dp->devi_unit);
#ifndef	NO_IDC_COMPATIBILITY
	else
		report_dev(devi);
#endif	NO_IDC_COMPATIBILITY
	return (0);
}

struct dev_ops ipisc_ops = {
	1,
	ipi3sc_identify,
	ipi3sc_attach,
};
#endif SUN4M_690

/* For VME/Old style drivers */
static struct dev_ops pseudo_ops = { 0 };

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
	register int curnode;
#ifdef TRACK_RANGES_IN_DEV_INFO
	struct dev_reg *nrp;
	int nrng, mrng, i;
	int npr = 0;
	struct dev_reg *prp = 0; /* parent's range info */
	struct dev_reg *rp = 0;
#endif
	extern dnode_t prom_nextnode(), prom_childnode();

	curnode = (int)prom_childnode((dnode_t)dev_info->devi_nodeid);
	if (curnode == 0)
		return;
	for (; curnode != 0; curnode = (int)prom_nextnode((dnode_t)curnode)) {
		/*
		 * If it's already on the list, skip it.
		 */
		if (check_dupdev(dev_info, curnode) != 0)
			continue;
		/*
		 * Allocate it, fill it in and add it to the list.
		 */
		dev = (struct dev_info *)kmem_zalloc(sizeof (*dev));
		append_dev(dev_info, dev);
		dev->devi_nodeid = curnode;
		dev->devi_parent = dev_info;
		fill_devinfo(dev);
		dev->devi_driver = match_driver(dev->devi_name);
#ifdef	VME
	/* Save away the dev_info pointer for vme */
	if (!found_vme) {
		if (strcmp(dev->devi_name, "vme") == 0) {
			vme_devinfo = dev;
			found_vme = 1;
		}
	}
#endif	VME
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
#if	!defined(VDDRV) && !defined(MUNIX) && !defined(MINIROOT)
			if (remove_dev(dev_info, dev))
				panic("attach_devs remove_dev 1");
			(void) kmem_free((caddr_t) dev, sizeof (*dev));
#endif	VDDRV && MUNIX && MINIROOT
			continue;
		}
		if ((*dev->devi_driver->devo_attach)(dev) < 0) {
#if	defined(VDDRV) || defined(MUNIX) || defined(MINIROOT)
			dev->devi_driver = NULL;
#else	VDDRV || MUNIX || MINIROOT
			if (remove_dev(dev_info, dev))
				panic("attach_devs remove_dev 2");
			(void) kmem_free((caddr_t) dev, sizeof (*dev));
#endif	VDDRV || MUNIX || MINIROOT
		}
	}
}

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

/*
 * Given a physical address, a bus type, and a size, return a virtual
 * address that maps the registers.
 * Returns NULL on any error.
 * NOTE: for Sun-4M, the allowable values for "space" are 0..15
 * specifying the upper four bits of the 36-bit physical address,
 * or SPO_VIRTUAL. Other values will result in an error.
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
	int offset = (int)addr & PGOFSET;
	long a = 0;
	int extent;

	if (space == SPO_VIRTUAL)
		return addr;
	else if (space < 16)
		pageval = btop(addr) + (btop(space<<28)<<4);
	else
		return 0;

	extent = btoc(size + offset);
	if (extent == 0)
		extent = 1;
	s = splhigh();
	a = rmalloc(kernelmap, (long)extent);
	(void)splx(s);
	if (a == 0)
		panic("out of kernelmap for devices");
	reg = (addr_t)((int)kmxtob(a) | offset);
	segkmem_mapin(&kseg, (addr_t)((int)reg&PAGEMASK),
		      (u_int)mmu_ptob(extent), PROT_READ | PROT_WRITE,
		      pageval, 0);
	return (reg);
}

/*
 * Supposedly, this routine is the only place that enumerates details
 * of the encoding of the "bustype" part of the physical address.
 * This routine depends on details of the physical address map, and
 * will probably be somewhat different for different machines.
 *
 * Future implementations of Sun-4M machines may do this differently.
 *
 * Sun-4M treats space and addr as the upper and lower 32 bits
 * of a 64-bit physical address; SP0_VIRTUAL is special-cased
 * as noting that this is a 'virtual' connection.
 */
char ukbuf[] = "space #";

decode_address(space, addr)
unsigned space, addr;
{
	char *name;
#ifdef	VME
	struct vme_spaces *vp;
#endif	VME
	if (space == SPO_VIRTUAL) /* weird name */
		name = "virtual";
	else {
		ukbuf[6] = "0123456789ABCDEF"[space & 15];
		name = ukbuf;
		switch(cpu) {
#if	defined(SUN4M_690) || defined(SUN4M_50)
		case CPU_SUN4M_690:
		case CPU_SUN4M_50:
			switch(space) {
			case 0x0:
				name = "obmem";
				break;

			case 0xE:
				name = "SBus slot #";
				name[10] = "0123456789abcdef"[addr>>28];
				addr &= 0x0FFFFFFF;
				break;

			case 0xF:
/* XXX - magic numbers are ugly. */
				if (addr < 0xF1000000)
					name = "sys";
				else {
					name = "obio";
					addr -= 0xF1000000;
				}
				break;
			}
#ifdef	VME
			for (vp = vme_spaces; vp->name; ++vp)
				if ((vp->space == space) &&
				    (vp->base <= addr)) {
					name = vp->name;
					addr -= vp->base;
					break;
				}
#endif	VME
			break;
#endif	(SUN4M_690 || SUN4M_50)
		default:
			break;
		}
	}
	printf(" %s 0x%x", name, addr);
}

/*
 * Compute the address of an I/O device within standard address
 * ranges and return the result.  This is used by DKIOCINFO
 * ioctl to get the best guess possible for the actual address
 * the card is at.
 */
getdevaddr(addr)
	caddr_t addr;
{
	int off = (int)addr & MMU_PAGEOFFSET;
	struct pte pte;
	int paddr, pageno, space;
#ifdef	VME
	struct vme_spaces *vp;
#endif	VME

	mmu_getkpte(addr, &pte);
	pageno = pte.PhysicalPageNumber;
	paddr = mmu_ptob(pageno);
	space = pageno >> 20;

	switch(cpu) {
#if defined(SUN4M_690) || defined(SUN4M_50)
	case CPU_SUN4M_690:
	case CPU_SUN4M_50:
		if ((pageno >= 0xFF1000) && (pageno < 0xFF2000)) {
			paddr -= 0xF1000000;
			break;
		}
#ifdef	VME
		for (vp = vme_spaces; vp->name; ++vp)
			if ((vp->space == space) &&
			    (vp->base <= paddr)) {
				paddr -= vp->base;
				break;
			}
#endif	VME
		break;
#endif
	default:
		break;
	}

	return (paddr + off);
}

/*
 * figure out what type of bus a page frame number points at.
 * NOTE: returns a BT_ macro, not the top four bits. most
 * things called "bustype" are actually just the top four
 * bits of the pte, which are part of the physical address
 * space as defined in the archetecture and which change between
 * various Sun-4m models.
 */
bustype(pageno)
	int	pageno;
{
	unsigned	space = pageno >> (32 - MMU_PAGESHIFT);	/* %%% */

	switch(cpu) {
#if defined(SUN4M_690)
	case CPU_SUN4M_690:
		switch(space) {
		case 0x0:
			return BT_OBMEM;
		case 0x9:
			return BT_VIDEO;
		case 0xA: case 0xB: case 0xC: case 0xD:
			return BT_VME; /* do we need more detail? */
		case 0xE:
			return BT_SBUS;
		case 0xF:
			return BT_OBIO;
		}

		break;
#endif
#if defined(SUN4M_50)
	case CPU_SUN4M_50:	/* campus 2 */
		switch(space) {
		case 0x0:
			return BT_OBMEM;
		case 0x9:
			return BT_VIDEO;
		case 0xE:
			return BT_SBUS;
		case 0xF:
			return BT_OBIO;
		}
		break;
#endif
	default:
		break;
	}
	return BT_UNKNOWN;
}

/*
 * Say that a device is here
 */

void
report_dev(dev)
	register struct dev_info *dev;
{
	int i, pri;
	register int nregs = dev->devi_nreg;
	register int nintr = dev->devi_nintr;

	/* char *ty; */ /* Not used anywhere */

	printf("%s%d", dev->devi_name, dev->devi_unit);
	for (i = 0; i < nregs; i++) {
		if (i == 0)
			printf(" at");
		else
			printf(" and");
		decode_address((unsigned) dev->devi_reg[i].reg_bustype,
			       (unsigned) dev->devi_reg[i].reg_addr);
	}
	for (i = 0; i < nintr; i++) {
		if (i == 0)
			printf(" ");
		else
			printf(", ");
		/* ty = ""; */ /* Don't know why this is useful */
		pri = LPL2SPL(dev->devi_intr[i].int_pri);
		printf("pri %d", pri);
		switch (dev->devi_intr[i].int_pri & ~15) {
		case INTLEVEL_SOFT:
			printf(" (soft)");
			break;
		case INTLEVEL_ONBOARD:
			printf(" (onboard)");
			break;
		case INTLEVEL_SBUS:
			printf(" (sbus level %d)", LPL2SVL(dev->devi_intr[i].int_pri));
			break;
#ifdef	VME
		case INTLEVEL_VME:
			printf(" (vme level %d)", LPL2SVL(dev->devi_intr[i].int_pri));
			break;
#endif	VME
		}
		if (dev->devi_intr[i].int_vec)
			printf(" vec 0x%x", dev->devi_intr[i].int_vec);
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

/*
 * This routine returns the length of the property value for the named
 * property.
 */
int
getproplen(id, name)
	int id;
	char *name;
{
	return prom_getproplen((dnode_t) id, (caddr_t) name);
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
		(void) prom_getprop((dnode_t)id, name, (caddr_t) &value);
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
	if (value == NULL) {
#ifdef	MULTIPROCESSOR
		mpprom_eprintf("getlongprop: unable to allocate %d bytes\n", size);
#endif	MULTIPROCESSOR
		panic("getlongprop: kmem_zalloc failed");
	}
	(void)prom_getprop((dnode_t) id, name, value);
	return (value);
}

/*
 * Spurious interrupt messages
 *
 * Probably better if the scanning code calls
 * not_serviced manually if nothing in the list
 * mananged to handle the interrupt, so we
 * don't call it by mistake when servicing
 * soft interrupts (which MUST call everything
 * in the entire list).
 */

#define	SPURIOUS	-1		/* recognized in locore.s */
int
not_serviced(counter, level, bus)
	int *counter, level;
	char *bus;
{
	if ((*counter)++ % 100 == 1)
		printf("Level %d %sinterrupt not serviced\n", level, bus);
	return (SPURIOUS);
}

char	busname_ovec[] = "onboard ";
char	busname_svec[] = "SBus ";
char	busname_vvec[] = "VME ";
char	busname_vec[] = "";

#define	OVECTOR(n)		\
int	olvl/**/n/**/_spurious;	\
struct autovec olvl/**/n[NVECT]

#define	SVECTOR(n)		\
int	slvl/**/n/**/_spurious;	\
struct autovec slvl/**/n[NVECT]

#define	VVECTOR(n)		\
int	vlvl/**/n/**/_spurious;	\
struct autovec vlvl/**/n[NVECT]

#define	XVECTOR(n)		\
struct autovec xlvl/**/n[NVECT]

#define	VECTOR(n)		\
int	level/**/n/**/_spurious;	\
struct autovec level/**/n[NVECT]

typedef int (*func)();

extern int	softint();
extern int	process_aflt();
extern int	hardlevel10();

/*
 * These structures are used in locore.s to jump to device interrupt routines.
 * They also provide vmstat assistance.
 * They will index into the string table generated by autoconfig
 * but in the exact order addintr sees them. This allows IOINTR to quickly
 * find the right counter to increment.
 * (We use the fact that the arrays are initialized to 0 by default).
 */

/*
 * Initial interrupt vector information.
 * Each of these macros defines both the "spurious-int" counter and
 * the list of autovec structures that will be used by locore.s
 * to distribute interrupts to the interrupt requestors.
 * Each list is terminated by a null.
 * Lists are scanned only as needed: hard ints
 * stop scanning when the int is claimed; soft ints
 * scan the entire list. If nobody on the list claims the
 * interrupt, then a spurious interrupt is reported.
 *
 * These should all be initialized to zero, except for the
 * few interrupts that we have handlers for built into the
 * kernel that are not installed by calling "addintr".
 * I would like to eventually get everything going through
 * the "addintr" path.

 * It might be a good idea to remove VECTORs that are not
 * actually processed by locore.s
 */

				/* onboard interrupts */
/* levels 1..13: lock acquired before calling service */
OVECTOR(1) = {{0}};
OVECTOR(2) = {{0}};
OVECTOR(3) = {{0}};
OVECTOR(4) = {{0}};		/* scsi */
OVECTOR(5) = {{0}};
OVECTOR(6) = {{0}};		/* ethernet */
OVECTOR(7) = {{0}};
OVECTOR(8) = {{0}};		/* video */
OVECTOR(9) = {{0}};		/* module int */
OVECTOR(10) = {{0}};		/* system timer */
OVECTOR(11) = {{0}};
OVECTOR(12) = {{0}};		/* serial ports */
OVECTOR(13) = {{0}};
/* levels 14..15: lock not entered */
OVECTOR(14) = {{0}};		/* processor timer */
OVECTOR(15) = {{0}};		/* async errors */

				/* SBus Interrupts */
/* all levels: lock held before calling service routines */
SVECTOR(1) = {{0}};
SVECTOR(2) = {{0}};
SVECTOR(3) = {{0}};
SVECTOR(4) = {{0}};
SVECTOR(5) = {{0}};
SVECTOR(6) = {{0}};
SVECTOR(7) = {{0}};

				/* VME Interrupts */
/* all levels: lock held before calling service routines */
VVECTOR(1) = {{0}};
VVECTOR(2) = {{0}};
VVECTOR(3) = {{0}};
VVECTOR(4) = {{0}};
VVECTOR(5) = {{0}};
VVECTOR(6) = {{0}};
VVECTOR(7) = {{0}};

				/* software vectored interrupts */
/* levels 1..9: lock held before calling service routines */
XVECTOR(1) = {{softint},{0}};	/* time-scheduled tasks */
XVECTOR(2) = {{0}};
XVECTOR(3) = {{0}};
XVECTOR(4) = {{0}};
XVECTOR(5) = {{0}};
XVECTOR(6) = {{0}};
XVECTOR(7) = {{0}};
XVECTOR(8) = {{0}};
XVECTOR(9) = {{0}};
/* levels 10..13: forwarding is responsibility of service routine */
XVECTOR(10) = {{0}};
XVECTOR(11) = {{0}};
XVECTOR(12) = {{process_aflt},{0}};	/* process async. fault */
XVECTOR(13) = {{0}};
XVECTOR(14) = {{0}};
XVECTOR(15) = {{0}};
				/* otherwise unclaimed sparc interrupts */
/* levels 1..13: lock held before calling service routine */
VECTOR(1) = {{0}};
VECTOR(2) = {{0}};
VECTOR(3) = {{0}};
VECTOR(4) = {{0}};
VECTOR(5) = {{0}};
VECTOR(6) = {{0}};
VECTOR(7) = {{0}};
VECTOR(8) = {{0}};
VECTOR(9) = {{0}};
VECTOR(10) = {{0}};
VECTOR(11) = {{0}};
VECTOR(12) = {{0}};
VECTOR(13) = {{0}};
/* levels 14..15: lock not entered. */
VECTOR(14) = {{0}};
VECTOR(15) = {{0}};

/*
 * indirection table, to save us some large switch statements
 * NOTE: This must agree with "INTLEVEL_foo" constants in
 *	<sun/autoconf.h>
 */
struct autovec *vectorlist[] = {
/*
 * otherwise unidentified interrupts at SPARC levels 1..15
 */
	0,	level1,	level2,	level3,	level4,	level5,	level6,	level7,
	level8,	level9,	level10,level11,level12,level13,level14,level15,
/*
 * interrupts identified as "soft"
 */
	0,	xlvl1,	xlvl2,	xlvl3,	xlvl4,	xlvl5,	xlvl6,	xlvl7,
	xlvl8,	xlvl9,	xlvl10,	xlvl11,	xlvl12,	xlvl13,	xlvl14,	xlvl15,
/*
 * interrupts identified as "onboard"
 */
	0,	olvl1,	olvl2,	olvl3,	olvl4,	olvl5,	olvl6,	olvl7,
	olvl8,	olvl9,	olvl10,	olvl11,	olvl12,	olvl13,	olvl14,	olvl15,
/*
 * interrupts identified as "sbus" -- note that sbus has only level 1
 * through 7; these are mapped to the proper sparc levels via the
 * boot prom forth word "???".
 */
	0,	0,	slvl1,	slvl2,	0,	slvl3,	0,	slvl4,
	0,	slvl5,	0,	slvl6,	0,	slvl7,	0,	0,
/*
 * interrupts identified as "vme" -- note that vme has only level 1
 * through 7; these are mapped to the proper sparc levels via the
 * boot prom forth word "???".
 */
	0,	0,	vlvl1,	vlvl2,	0,	vlvl3,	0,	vlvl4,
	0,	vlvl5,	0,	vlvl6,	0,	vlvl7,	0,	0,
};
#define	MAXAUTOVEC	(sizeof vectorlist / sizeof vectorlist[0])

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
	if (levelp->av_name == NULL)
		levelp->av_name = name;
	else if (strcmp(levelp->av_name, name)) {
		printf("addioname: can't change %s to %s!\n",
		       levelp->av_name, name);
		panic("addioname");
	}
	levelp->av_devcnt++;
}

/*
 * Arrange for a driver to be called when a particular
 * auto-vectored interrupt occurs.
 * NOTE: if a device can generate interrupts on more than
 * one level, or if a driver services devices that interrupt
 * on more than one level, then the driver should install
 * itself on each of those levels.

 * devices may also install themselves on the VME, Internal, or SBUS
 * vector lists using inerrupt level offsets, see <sun/autoconf.h>.

 * On Hard-ints, order of evaluation of the chains is:
 *   if onboard int at this level active in SIPR,
 *     scan "onboard" chain; if nobody claims,
 *     remember bus-type is onboard.
 *   if sbus int at this level active in SIPR,
 *     scan "sbus" chain; if nobody claims,
 *     remember bus-type is sbus.
 *   if vme int at this level active in SIPR,
 *     scan "vme" chain; if nobody claims,
 *     remember bus-type is vme.
 *   scan "unspecified" chain; if nobody claims,
 *     report spurious interrupt and remembered bus-type if any.
 * Scanning terminates with the first driver that claims it has
 * serviced the interrupt.
 *
 * On Soft-ints, order of evaulation of the chains is:
 *   scan the "soft" chain
 *   scan the "unspecified" chain
 * Scanning continues until all service routines have been called.
 * If nobody claims the interrupt, report a spurious soft interrupt.
 */

addintr(lvl, xxintr, name, unit)
	func xxintr;
	char *name;
	int unit;
{
	register func f;
	register struct autovec *levelp;
	register int i;

	if ((f = xxintr) == NULL)
		return;
	if (lvl >= MAXAUTOVEC)
		panic("addintr: vector number out of range");
	levelp = vectorlist[lvl];
	if (levelp == (struct autovec *)0)
		panic("addintr: specified vector can not be polled");

	/* If devcnt == -1, settrap() claimed this trap */
	if (levelp->av_devcnt == -1) {
		/* XXX - addintr() should return success/failure */
		panic("addintr: interrupt trap vector busy\n");
	}

	for (i = 0; i < NVECT; i++) {
		if (levelp->av_vector == NULL) { /* found an open slot */
			bzero((caddr_t) levelp, sizeof (*levelp));
			levelp->av_vector = f;		/* add f to list */
			addioname(levelp, name, unit);
			return;
		}
		if (levelp->av_vector == f) {	/* already in list */
			addioname(levelp, name, unit);
			return;
		}
		levelp++;
	}
	panic("addintr: too many devices");
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
 * For sun4m machines interrupt level assignments are handled differently.
 * (see ../sun/autoconf.h for details).  As such the interrupt level passed
 * to settrap "lvl" is INTLEVEL_XXX+n.  Where n is the normal level (1..15)
 * and XXX represent one of (SOFT, ONBOARD, SBUS, VME). This logical
 * interrupt number gets translated into the real SPARC level to select
 * which interrupt vector will be modified.
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
	extern		sys_trap();
	extern int	nwindows;		/* number of register windows */

	if (lvl >= MAXAUTOVEC)
		panic("settrap: vector number out of range\n");
	lvl = LPL2SPL(lvl);

	if ((lvl == 10) ||	/* reserved for system clock */
	    (lvl == 12) ||	/* reserved for serial ports */
	    (lvl == 14) ||	/* reserved for monitor clock */
	    (lvl == 15)) {	/* reserved for internal sys communication */
		printf("settrap: cannot set trap for processor level %d\n", lvl);
		return(0);
	}
	levelp = vectorlist[lvl];

	if (!levelp) {
		printf("settrap: cannot set trap for processor level %d\n", lvl);
		return(0);
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
		vec.instr[1] = SETHIL3 | SETHI_HI((int)sys_trap);
		vec.instr[2] = JMPL3 | JMP_LO((int)sys_trap);
		vec.instr[3] = MOVL4 | T_INTERRUPT | lvl;
	} else {
		vec.instr[0] = MOVPSRL0;
		vec.instr[1] = SETHIL3 | SETHI_HI((int)xxintr);
		vec.instr[2] = JMPL3 | JMP_LO((int)xxintr);
		vec.instr[3] = MOVL6 | (nwindows - 1);
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

	spl = ipltospl(LPL2SPL(level));
	if (spl > splvm_val) {
		splvm_val = spl;
		SPLMB = spltopri(spl);
/*
 * note: splr(pritospl(SPLMB)) might be one level
 * higher than splvm(). This is important if we
 * get an adddma for SPARC Level-9, where the splr
 * would lock out the clock at Level-10 as well.
 */
	}
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

	if ((f = xxintr) == NULL)
		return (0);

	if (lvl >= MAXAUTOVEC)
		panic("remintr: vector number out of range");
	levelp = vectorlist[lvl];
	if (levelp == (struct autovec *)0)
		panic("remintr: specified vector can not be polled");

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
			printf("error %d getting size of swapfile %s\n",
			    error, swp->bo_name);
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
	/*
	 * Previously was making the call, makespecvp(makedev(1, 1), VBLK), instead of 
	 * making the call, bdevvp(makedev(1, 1)).  Calling bdevvp is what the non-SAS
	 * call to VFS_SWAPVP (see above) essentially does, so it is the right thing for
	 * SAS to do simulate the real world as close as possible, given the way the sim0
	 * device is currently implemented.  Calling makespecvp would result in it first
	 * creating a shadow snode (and vnode contained inside), then creating the real
	 * vnode via calling bdevvp, and having the s_bdevvp field of the shadow snode
	 * point to this real vnode.  The problem with this approach is that when the
	 * shaddow vnode is passed to spec_getpage to perform a swapin, spec_getpage is
	 * expecting the real vnode, and panics when VTOS(vp)->b_devvp != vp.
	 */
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
	swap_present = 1;               /* main checks for this */
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

	rconsdev = makedev(ZSMAJOR, zsminor);

#else SAS
	rconsdev = makedev(SIMCMAJOR, 0);
#endif SAS

namedone:

	if (rconsdev) {
		/*
		 * Console is a CPU serial port.
		 */
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

	if (prom_stdin_is_keyboard())
		goto kbddone;

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

	if (!ldreplace(kbdvp, kbdvp->v_stream, KBDLDISC)) {
		u.u_error = EINVAL;
	}

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
	struct dev_info *fbdevi;
	dev_t finddev();
	static dev_t fbdevno = NODEV;
	extern char *prom_stdoutpath();
	extern struct dev_info *path_to_devi();

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

/*
 * It is possible for "path_to_devi" to return to us
 * a reference to a node that has no driver.  If this
 * happens, finddev() will return NODEV, and things
 * will at least continue to function.
 */
	if ((fbdevno == NODEV) && (prom_stdout_is_framebuffer()))
		if ((fbdevi = path_to_devi(prom_stdoutpath())) != 0)
			fbdevno = finddev('c', fbdevi);
	if (fbdevno != NODEV)
		p->fbdev = fbdevno;
}
#endif SAS

/* convert dev_info pointer to device number */
dev_t
finddev(type, info)
	int type;
	struct dev_info *info;
{
	int majnum = -1;
	struct dev_ops *driver;
	int (*openfun)();

	if (info == (struct dev_info *)0)
		return NODEV;

	driver = info->devi_driver;
	if (driver == (struct dev_ops *)0)
		return NODEV;

	openfun = driver->devo_open;
	if (openfun == (int (*)())0)
		return NODEV;

	if (type == 'b') {
#ifdef notdef	/* no one is using this at the moment */
		struct bdevsw *swp = bdevsw;

		for (majnum = 0; majnum < nblkdev; majnum++, swp++)
			if (swp->d_open == openfun)
				return (makedev(majnum, info->devi_unit));
#endif
	} else {
		struct cdevsw *swp = cdevsw;

		for (majnum = 0; majnum < nchrdev; majnum++, swp++)
			if (swp->d_open == openfun)
				return (makedev(majnum, info->devi_unit));
	}

	return (NODEV);
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
		printf("Warning: NVRAM checksum error [%x]\n", val);
	if (id.id_format == 1) {
		(void) localetheraddr((struct ether_addr *)id.id_ether,
		    (struct ether_addr *)NULL);
	} else
		printf("Invalid format code (%d) in NVRAM\n", id.id_format);
}

#ifdef NOPROM
char *cpu_str = 0;
char *mod_str = 0;
extern int module_type;

/*
 * We set the cpu type from the idprom machine ID.
 * XXX CPU name should be PROM property.
 */
setcputype()
{
	struct idprom id;

	cpu = -1;		/* not possible value for id.id_machine */
	getidprom((char *)&id);

	if (id.id_format == 1)
		cpu = id.id_machine;
	else
		printf("Invalid format type (%d) in NVRAM\n", id.id_format);

/*
 * This is just for pretty printing for now.
 */
	switch (module_type) {
	      case ROSS604:
		mod_str = "ross604";
		break;
	      case ROSS605:
		mod_str = "ross605";
		break;
	      case ROSS605_D:
		mod_str = "ross605d";
		break;
	      case VIKING:
		mod_str = "viking";
		break;
	      case VIKING_M:
		mod_str = "viking+mxcc";
		break;
	      case SSPARC:
		mod_str = "ssparc";
		break;
	      default:
		mod_str = "unknown";
		break;
	}

	switch(cpu) {
	default:
	case -1:
		printf("machine type 0x%x in NVRAM\n", cpu);
		panic("No known machine types configured in!\n");

	case CPU_SUN4M_690:
		cpu_str = "sun-4m/690";
		break;

	case CPU_SUN4M_50: /* campus2 */
		cpu_str = "sun-4m/50";
		break;

	}
}
#else NOPROM
setcputype()
{
	struct idprom id;
	cpu = -1;		/* not possible value for id.id_machine */
	getidprom((char *)&id);

	if (id.id_format == 1)
		cpu = id.id_machine;
	else
		printf("Invalid format type (%d) in NVRAM\n", id.id_format);

	switch(cpu) {
	default:
	case -1:
		printf("machine type 0x%x in NVRAM\n", cpu);
		panic("No known machine types configured in!\n");

	case CPU_SUN4M_690:
	case CPU_SUN4M_50: /* campus2 */
		break;
	}
	mach_info.sys_type = cpu;
}
#endif NOPROM

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

extern unsigned	sbus_numslots;
extern unsigned	sbus_basepage[];
extern unsigned	sbus_slotsize[];

/*
 * a word about the INRANGE macro:
 *	If addr < size, then the difference becomes a huge unsigned
 *	number, which will compare true with the range if and only
 *	if the size is big enough to wrap around across the end of
 *	the number system.
 *
 *	If addr > size, then we see if it is within the limit, which
 *	is one *less* than the size -- this way, a size of zero becomes
 *	a limit of 0xFFFFFFFF, corresponding to 4gig of space.
 *
 *	Some simpler ways of doing this barf when the range ends at zero,
 *	or when the size is 4gig, or when the range wraps across zero, or
 *	when the range wraps across 0x80000000. Think carefully before you
 *	change this macro.
 *
 *	With all that work, maybe the macro should be used elsewhere!
 */
#define	UNS(x)	((unsigned)(x))
#define	INRANGE(addr, base, size)	\
	(UNS(UNS(addr) - UNS(base)) <= UNS(UNS(size)-1))
/*
 * INPAGE uses INRANGE to decide if an address falls inside
 * an area described by a page number and a size in bytes.
 * Note that page information above PA31 are lost, so space
 * comparisons (if any) must be done elsewhere.
 */
#define	INPAGE(addr, page, bytes)	\
	INRANGE(addr, mmu_ptob(page), bytes)

/*
 * INSBUSSLOT uses the INPAGE macro and the sbus layout information
 * from sbus_basepage and sbus_slotsize to decide if a physical
 * address is in a specific sbus slot. Since the physical address
 * is given only as a 32-bit number, the remainder of the PA must
 * be checked elsewhere.
 */
#define	INSBUSSLOT(addr, slot)		\
	INPAGE(addr, sbus_basepage[slot], sbus_slotsize[slot])

/*
 * sbusslot returns the slot number for a given
 * physical address. Since only the low 32 bits
 * of the slot are given, that's all we check.
 */
int
sbusslot(addr)
	unsigned	addr;
{
	int slot;

	for (slot=0; slot<sbus_numslots; ++slot)
		if (INSBUSSLOT(addr, slot))
			return slot;
	return -1;
}

/*
 * Return slot number if given physical address corresponds to
 * slave-only SBus slot, e.g. 4/60 slot 3; return -1 if OK.
 *
 * NOTE: this is only the low 32 bits of the physaddr, so the
 * assumption is made that (a) the address is somewhere in
 * sbus space, and (b) the low 32 bits of the physical address
 * of two sbus slots do not overlap.
 */

int
slaveslot(addr)
	addr_t	addr;
{
	int slot;
	unsigned slaves;

	slaves = getprop(0, "slave-only", 0);
	if (slaves) {
		slot = sbusslot((u_int)addr);
		if (slot != -1)
			return (slaves & (1 << slot)) ? slot : -1;
	}
	return -1;
}


/*
 * Find devices on the Mainbus.
 *
 * Note: This is no longer called to find SBus devices.
 *
 * Uses per-driver routine to probe for existence of the device
 * and then fills in the tables, with help from a per-driver
 * slave initialization routine.
 */
mbconfig()
{
	register struct mb_device       *md;
	register struct mb_ctlr	 *mc;
	struct mb_driver		*mdr;
 
	/*
	 * Grab some memory to record the Mainbus address space in use,
	 * so we can be sure not to place two devices at the same address.
	 * If we run out of kernelmap space, we could reuse the mapped
	 * pages if we did all probes first to determine the target
	 * locations and sizes, and then remucked with the kernelmap to
	 * share spaces, then did all the attaches.
	 *
	 * We could use just 1/8 of this (we only want a 1 bit flag) but
	 * we are going to give it back anyway, and that would make the
	 * code here bigger (which we can't give back), so ...
	 */
 
	/*
	 * Check each Mainbus mass storage controller.
	 * See if it is really there, and if it is record it and
	 * then go looking for slaves.
	 */
	for (mc = mbcinit; mdr = mc->mc_driver; ++mc)
		(void)mbconfctrl(mc, mdr);
	
	
	/*
	 * Now look for non-mass storage peripherals.
	 */

	for (md = mbdinit; mdr = md->md_driver; ++md)
	{
		if (md->md_alive || md->md_slave != -1)
			continue;
		(void)mbconfnonms(md, mdr);     /* config non-mass storage device */
	}
}       /* end of mbconfig */

#ifdef	VME
/* The routine to fill in the dev_info struct for VME devices
 * If configuring a controller the mb_device pointer will be
 * NULL, and vice-versa.
 * Common routine for mbconfctrl and mbconfnonms.
 * Using names passed from driver structures now, this will change.
 */

static void
vme_fill_devinfo(mc, md, mdr, dev)
	register struct mb_ctlr *mc;
	register struct mb_device *md;
	struct mb_driver	*mdr;
	struct dev_info		*dev;
{
	u_int	space;

	/*
	 * Allocate space for reg and intr structures
	 * Dont have the luxury of calling getlongprop()
	 */
	dev->devi_reg = (struct dev_reg *)kmem_zalloc(sizeof(*dev->devi_reg));
	dev->devi_intr = (struct dev_intr *)kmem_zalloc(sizeof(*dev->devi_intr));
	/*  %%% following may not always be true, but for now... */
	dev->devi_nreg = 1;
	dev->devi_nintr = 1;
	/* Having a non-NULL devi_driver entry is necessary for boot */
	dev->devi_driver = &pseudo_ops;

	if(mc) {	/* configuring a controller */
		dev->devi_name = mdr->mdr_cname;
		dev->devi_reg->reg_addr = mc->mc_addr;
		dev->devi_reg->reg_size = mdr->mdr_size;
		space = mc->mc_space;
		dev->devi_unit = mc->mc_ctlr;
		dev->devi_intr->int_pri = mc->mc_intpri;
		if (mc->mc_intr) 	/* %%% for IPI, idc doesnt intr */
			dev->devi_intr->int_vec = mc->mc_intr->v_vec;
	} else if (md) { 	/* configuring a device */
		dev->devi_name = mdr->mdr_dname;
		dev->devi_reg->reg_addr = md->md_addr;
		dev->devi_reg->reg_size = mdr->mdr_size;
		space = md->md_space;
		dev->devi_unit = md->md_unit;
		dev->devi_intr->int_pri = md->md_intpri;
		if (md->md_intr) 
			dev->devi_intr->int_vec = md->md_intr->v_vec;
	} else { 	/* should never happen */
		printf("vme_fill_devinfo: specified no controller/device\n");
	}

	/* To get bustype from the space field */
	switch (space & SP_BUSMASK) {

	case SP_VME16D16:
	case SP_VME24D16:
	case SP_VME32D16:
		dev->devi_reg->reg_bustype = 0xC;
		break;

	case SP_VME16D32:
	case SP_VME24D32:
	case SP_VME32D32:
		dev->devi_reg->reg_bustype = 0xD;
		break;
	
	default:	/* This is where IPI falls */
		dev->devi_reg->reg_bustype = 0x0;
	}

} /* End of vme_fill_devinfo() */
#endif	VME
/*
 * Configure a controller
 */
 
mbconfctrl(mc, mdr)
	register struct mb_ctlr *mc;
	struct mb_driver	*mdr;
{
	struct mb_device	*md;
	u_short		 *reg;
	int		     err = 0;
	int		     stat;
	u_short		 *doprobe();
	struct dev_info *dev = NULL;
 
	if ((reg = doprobe((u_long)mc->mc_addr, (u_long)mc->mc_space,
			   mdr, mdr->mdr_cname, mc->mc_ctlr, mc->mc_intpri,
			   mc->mc_intr)) == 0)
		return (-1);

#ifdef	VME

	/* Check if already have controller listed in dev_info tree */
	if (!vme_check_dupdev(vme_devinfo, mdr->mdr_cname, mc->mc_ctlr)) {

		/*
		 * Do the work of adding device to dev_info tree
		 */
		dev = (struct dev_info *)kmem_zalloc(sizeof (*dev));
		append_dev(vme_devinfo, dev);

		/* no nodeid to add, since not working with the PROM */

		dev->devi_parent = vme_devinfo;
		vme_fill_devinfo(mc, (struct mb_device *) NULL, mdr, dev);

		/* no devops structure since not an sbus/fcode driver */
	}
	
	if ((mdr->mdr_flags & (MDR_BIODMA | MDR_DMA)) != 0)
		adddma(mc->mc_intpri);

	mc->mc_alive = 1;
	mc->mc_mh = &mb_hd;
	mc->mc_addr = (caddr_t)reg;
	if (mdr->mdr_cinfo)
		mdr->mdr_cinfo[mc->mc_ctlr] = mc;
	for (md = mbdinit; md->md_driver; ++md) {
	/* 
	 * Dont check for alive here, so can run disks thru the
	 * fill_devinfo code in mbconfdev
	 */
		if (md->md_driver != mdr || md->md_ctlr != mc->mc_ctlr &&
		    md->md_ctlr != '?')
			continue;
		stat = mbconfdev(md, mc, mdr, dev);  /* configure the device */
		if (err == 0)
			err = stat;
	}
	return (err);
#else	VME
	return (-1);
#endif	VME

}       /* end of mbconfctrl */
 
#ifdef	VME
add_ipi_drives(md, mdr)
register struct mb_device *md;
struct mb_driver *mdr;
{
	struct dev_info *pd, *id;

	if (!found_vme)
		return (0);
#ifdef	lint
	mdr = mdr;
#endif	lint
#ifdef	IPI_DEBUG
	printf("Node %s unit %d at %s%d\n", mdr->mdr_dname, md->md_unit,
		mdr->mdr_cname, md->md_ctlr);
#endif	IPI_DEBUG
	for (pd = vme_devinfo->devi_slaves; pd != NULL; pd = pd->devi_next) {
		if (pn_identify(pd->devi_name)) {
			for (id = pd->devi_slaves; id != NULL;
			     id = id->devi_next) {
				if ((ipi3sc_identify(id->devi_name)) &&
				    (md->md_ctlr == id->devi_unit))
					(void) add_ipi_drive(id, md->md_unit);
			}
		}
	}
	return (0);
}
 
void
add_ipi_drive(devi, unit)
	struct dev_info *devi;
	short unit;
{
	register struct dev_info *dp, *ndp;

#ifdef	IPI_DEBUG
	printf("Parent Node %s%d\n", devi->devi_name, devi->devi_unit);
#endif	IPI_DEBUG
	dp = (struct dev_info *)kmem_zalloc(sizeof (struct dev_info));
	dp->devi_name = "id";
	dp->devi_unit = unit;
	dp->devi_nodeid = -1;
	dp->devi_driver = match_driver(dp->devi_name);
	dp->devi_parent = devi;

	if (devi->devi_slaves == NULL) {
		devi->devi_slaves = dp;
	} else {	
		for (ndp = devi->devi_slaves; ndp != NULL;
		     ndp = ndp->devi_next)
			if (ndp->devi_next == NULL) {
				ndp->devi_next = dp;
				break;
			}
	}
}
#endif	VME

/*
 * Configure a mass storage device.
 */
 

/*
 * Added "ctrl_dev" as argument passwd to this routine in order
 * to add this device as a child to it controller in dev_info tree.
 */
mbconfdev(md, mc, mdr, ctrl_dev)
register struct mb_device *md;
register struct mb_ctlr *mc;
struct mb_driver *mdr;
struct dev_info *ctrl_dev;
{
	struct dev_info *dev_dev;

	if (((*mdr->mdr_slave)(md, mc->mc_addr)) && !(md->md_alive)) {
		md->md_alive = 1;
		md->md_ctlr = mc->mc_ctlr;
		md->md_hd = &mb_hd;
		md->md_addr = (caddr_t)mc->mc_addr;
		if (md->md_dk && dkn < DK_NDRIVE)
			md->md_dk = dkn++;
		else md->md_dk = -1;
		md->md_mc = mc;
		/* md_type comes from driver */
		if (mdr->mdr_dinfo)
			mdr->mdr_dinfo[md->md_unit] = md;
		printf("%s%d at %s%d slave %d\n",
			mdr->mdr_dname, md->md_unit, mdr->mdr_cname, mc->mc_ctlr, md->md_slave);

	/*
	 * If we were passed a ctrl_dev, add its children to the tree.
	 */
	if (ctrl_dev) {

#ifdef	VME
	/* 
	 * Check if already have device listed in dev_info tree
	 */
		if (!vme_check_dupdev(ctrl_dev, mdr->mdr_dname, md->md_unit)) {

	/*
	 * Add the device to dev_info tree as a child of its controller.
	 */
		dev_dev = (struct dev_info *)kmem_zalloc(sizeof (*dev_dev));
		append_dev(ctrl_dev, dev_dev);
	
	/* no nodeid */
	
		dev_dev->devi_parent = ctrl_dev;
		vme_fill_devinfo(( struct mb_ctlr *) NULL, md, mdr, dev_dev);

	/* no devops structure */
		}
#endif	VME
	}
		if (mdr->mdr_attach)
			(*mdr->mdr_attach)(md);
		return (0);

	}
	return (-1);
}       /* end of mbconfdev */
 
 
/*
 * Configure a non-mass storage device
 */
mbconfnonms(md, mdr)
	register struct mb_device *md;
	register struct mb_driver *mdr;
{
	u_short *reg;
	struct dev_info *dev;

	if ((reg = doprobe((u_long)md->md_addr, (u_long)md->md_space,
			   mdr, mdr->mdr_dname, md->md_unit, md->md_intpri,
			   md->md_intr)) == 0)
		return (-1);

#ifdef	VME
	/* Check if already have controller listed in dev_info tree */
	if (vme_check_dupdev(vme_devinfo, mdr->mdr_dname, md->md_unit) == 0) {
 
		/*
		 * Do the work of adding device to VME dev_info tree
		 */
		dev = (struct dev_info *)kmem_zalloc(sizeof (*dev));
		append_dev(vme_devinfo, dev);
 
		/* no nodeid to add, since not working with the PROM */
 
		dev->devi_parent = vme_devinfo;
		vme_fill_devinfo(( struct mb_ctlr *) NULL, md, mdr, dev);

		/* no devops structure since not an sbus/fcode driver */
        }

	if ((mdr->mdr_flags & (MDR_BIODMA | MDR_DMA)) != 0)
		adddma(md->md_intpri);

	md->md_hd = &mb_hd;
	md->md_alive = 1;
	md->md_addr = (caddr_t)reg;
	md->md_dk = -1;
	/* md_type comes from driver */
	if (mdr->mdr_dinfo)
		mdr->mdr_dinfo[md->md_unit] = md;
	if (mdr->mdr_attach)
		(*mdr->mdr_attach)(md);
	return (0);
#else	VME
	return (-1);
#endif	VME
}				/* end of mbconfnonms */
 
 
/*
 * Probe for a device or controller at the specified addr.
 * The space argument give the page type and cpu type for the device.
 *
 * This code is now only for VME devices.
 * *****We can no longer probe for SBus devices.*****
 */
u_short *doprobe(addr, space, mdr, dname, unit, br, vp)
u_long			addr;
u_long			space;
struct mb_driver	*mdr;
char			*dname;
int			unit;
int			br;
register struct vec	*vp;
{
	register u_short *reg = NULL;
	char    *name;
	unsigned	base = 0;
	int     i;
	int	is_ipi = 1;
#ifdef	VME
/*
 * %%% this routine "knows" about the space and base
 * addresses of all the VME mapping areas. Ugh.
 */ 
	switch (space & SP_BUSMASK) {
 
	case SP_VME16D16:
		name = "vme16d16";
		space = 0xC;
		base = 0xFFFF0000;
		break;
 
	case SP_VME16D32:
		name = "vme16d32";
		space = 0xD;
		base = 0xFFFF0000;
		break;

	case SP_VME24D16:
		name = "vme24d16";
		space = 0xC;
		base = 0xFF000000;
		break;
 
	case SP_VME24D32:
		name = "vme24d32";
		space = 0xD;
		base = 0xFF000000;
		break;
 
	case SP_VME32D16:
		name = "vme32d16";
		space = 0xC;
		break;
 
	case SP_VME32D32:
		name = "vme32d32";
		space = 0xD;
		break;

	case SP_IPI:
		name = "ipi";
		reg = (u_short *)addr;	/* IPI device address */
		br = 0;			/* IPI interrupts thru host adaptor */
		break;
 
	default:
		return (0);
	}

 
	if (reg == NULL) {
	reg = (u_short *)map_regs((addr_t)(base + addr), (u_int) mdr->mdr_size, (int) space);
	is_ipi = 0;
	}
	i = (*mdr->mdr_probe)(reg, unit);
	if (i == 0) {
		if (!is_ipi) {
			unmap_regs((addr_t) reg, (u_int) mdr->mdr_size);
		}
		return (0);
	}	
	printf("%s%d at %s 0x%x ", dname, unit, name, addr);
	if (br < 0 || br >= 7)
	{
		printf("bad priority (%d)\n", br);
		unmap_regs((addr_t) reg, (u_int) mdr->mdr_size);
		return (0);
	}

 
	/*
	 * If br is 0, then no priority was specified in the
	 * config file and the device cannot use interrupts.
	 */
	if (br != 0)
	{
		/*
		 * now set up VME vectored interrupts if conditions are right
		 */
		if (vp != (struct vec *)0)
		{  
			for (; vp->v_func; vp++)
			{
				void vme_vecinstall();
 
				printf("vec 0x%x ", vp->v_vec);
				vme_vecinstall(vp);
			}
		}
	}
	printf("\n");
	return (reg);
#else	VME
	return (0);
#endif	VME
}

#ifdef	VME
void vme_vecinstall(vp)
struct vec	*vp;
{
	int	vec_num;

	if (vp->v_vec < VEC_MIN || vp->v_vec > VEC_MAX)
		panic("vme_vecinstall: bad vector");

	vec_num = vp->v_vec - VEC_MIN;
	if (vme_vector[vec_num].func != 0)
		panic("vme_vecinstall: vector in use");
	vme_vector[vec_num].func = vp->v_func;
	vme_vector[vec_num].arg = vp->v_vptr;
}	/* end of vme_vecinstall */
#endif	VME
