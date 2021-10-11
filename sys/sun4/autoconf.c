#ifndef lint
static	char sccsid[] = "@(#)autoconf.c 1.1 92/07/30 SMI";
#endif lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the Mainbus
 * device tables and the memory controller monitoring. Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vfs_stat.h>
#include <sys/vnode.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
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
#include <machine/psl.h>
#include <machine/seg_kmem.h>
#include <sun/autoconf.h>
#include <sun/consdev.h>
#include <sun/fbio.h>
#include <sundev/mbvar.h>
#include <sundev/kbio.h>
#include <sundev/zsvar.h>
#include <mon/idprom.h>

#include <machine/eeprom.h>

#ifdef SAS
#include "sas_simdef.h"
#endif SAS

extern int swap_present;
/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	dkn;		/* number of iostat dk numbers assigned so far */

/*
 * This allocates the space for the per-Mainbus information.
 */
struct	mb_hd mb_hd;

/*
 * Maximum interrupt priority used by Mainbus DMA.
 * This value is determined by taking the max of the m[cd]_intpri
 * field from the mb_{cltr, device} structures which are found to
 * exist and have the MDR_BIODMA or MDR_DMA flags on in their
 * corresponding mb_driver structure.
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
 * which will probe adaptors to see what is out there.
 */
configure()
{
	extern int fpu_exists;		/* set or cleared by fpu_probe */
	extern int fpu_version;		/* ditto */
	extern int fpneedfitoX;		/* we set it if necessary */

	idprom();
	fpu_probe();
	if (!fpu_exists)
		printf("No FPU in configuration\n");
	else /* fpu exists */ {
		switch (cpu) {
		case CPU_SUN4_260:
		case CPU_SUN4_110:
			fpneedfitoX = (fpu_version == 0);
			break;
		default:
			fpneedfitoX = 0;
		}
	}
	/*
	 * Configure the Mainbus.
	 */
	mbconfig();
	splvm_val = pritospl(SPLMB);
	/*
	 * Attach pseudo-devices
	 */
	pseudoconfig();
}

/*
 * Find devices on the Mainbus.
 * Uses per-driver routine to probe for existence of the device
 * and then fills in the tables, with help from a per-driver
 * slave initialization routine.
 */
mbconfig()
{
	register struct mb_device *md;
	register struct mb_ctlr *mc;
	struct mb_driver *mdr;
	u_short *doprobe();

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
	for (mc = mbcinit; mdr = mc->mc_driver; mc++) {
		(void)mbconfctrl(mc, mdr);
	}

	/*
	 * Now look for non-mass storage peripherals.
	 */
	for (md = mbdinit; mdr = md->md_driver; md++) {
		if (md->md_alive || md->md_slave != -1)
			continue;
		(void)mbconfnonms(md, mdr); /* config non-mass storage device */
	}
}

/*
 * Configure a controller
 */
mbconfctrl(mc, mdr)
	register struct mb_ctlr *mc;
	struct mb_driver *mdr;
{
	register struct mb_device *md;
	u_short *reg;
	int err = 0;
	int stat;
	u_short *doprobe();

	if ((reg = doprobe((u_long)mc->mc_addr, (u_long)mc->mc_space,
			mdr, mdr->mdr_cname, mc->mc_ctlr, mc->mc_intpri,
			mc->mc_intr)) == 0)
		return (-1);
	if (((mdr->mdr_flags & (MDR_BIODMA | MDR_DMA)) != 0) &&
	    mc->mc_intpri > SPLMB)
		SPLMB = mc->mc_intpri;
	mc->mc_alive = 1;
	mc->mc_mh = &mb_hd;
	mc->mc_addr = (caddr_t)reg;
	if (mdr->mdr_cinfo)
		mdr->mdr_cinfo[mc->mc_ctlr] = mc;
	for (md = mbdinit; md->md_driver; md++) {
		if (md->md_driver != mdr || md->md_alive ||
		    md->md_ctlr != mc->mc_ctlr && md->md_ctlr != '?')
			continue;

		stat = mbconfdev(md, mc, mdr); /* configure the device */
		if (err == 0)
			err = stat;
	}
	return (err);
}

/*
 * Configure a mass storage device
 */
mbconfdev(md, mc, mdr)
	register struct mb_device *md;
	register struct mb_ctlr *mc;
	struct mb_driver *mdr;
{
	if ((*mdr->mdr_slave)(md, mc->mc_addr)) {
		md->md_alive = 1;
		md->md_ctlr = mc->mc_ctlr;
		md->md_hd = &mb_hd;
		md->md_addr = (caddr_t)mc->mc_addr;
		if (md->md_dk && dkn < DK_NDRIVE)
			md->md_dk = dkn++;
		else
			md->md_dk = -1;
		md->md_mc = mc;
		/* md_type comes from driver */
		if (mdr->mdr_dinfo)
			mdr->mdr_dinfo[md->md_unit] = md;
		printf("%s%d at %s%d slave %d\n",
			mdr->mdr_dname, md->md_unit,
			mdr->mdr_cname, mc->mc_ctlr, md->md_slave);
		if (mdr->mdr_attach)
			(*mdr->mdr_attach)(md);
		return (0);
	}
	return (-1);
}
/*
 * Configure a non-mass storage device
 */
mbconfnonms(md, mdr)
	register struct mb_device *md;
	register struct mb_driver *mdr;
{
	u_short *reg;

	if ((reg = doprobe((u_long)md->md_addr, (u_long)md->md_space,
		    mdr, mdr->mdr_dname, md->md_unit, md->md_intpri,
		    md->md_intr)) == 0)
		return (-1);
	if (((mdr->mdr_flags & (MDR_BIODMA | MDR_DMA)) != 0) &&
		    md->md_intpri > SPLMB)
		SPLMB = md->md_intpri;
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
}

/*
 * Make non-zero if want to be set up to handle
 * both vectored and auto-vectored interrupts
 * for the same device at the same time.
 */
int paranoid = 0;

/*
 * Probe for a device or controller at the specified addr.
 * The space argument give the page type and cpu type for the device.
 */
u_short *
doprobe(addr, space, mdr, dname, unit, br, vp)
	register u_long addr, space;
	register struct mb_driver *mdr;
	char *dname;
	int unit, br;
	register struct vec *vp;
{
	register u_short *reg = NULL;
	char *name;
	long a = 0;
	int i, extent, machine;
	u_int pageval;
	int s;

	machine = space & SP_MACHMASK;

	if (machine != SP_MACH_ALL && machine != MAKE_MACH(cpu & CPU_MACH))
		return (0);

	switch (space & SP_BUSMASK) {

	case SP_VIRTUAL:
		name = "virtual";
		reg = (u_short *)addr;
		break;

	case SP_OBMEM:
		name = "obmem";
		pageval = PGT_OBMEM | btop(addr);
		break;

	case SP_OBIO:
		name = "obio";
		pageval = PGT_OBIO | btop(addr);
		break;

	case SP_VME16D16:
		name = "vme16d16";
		pageval = PGT_VME_D16 | btop(VME16_BASE | (addr & VME16_MASK));
		break;

	case SP_VME24D16:
		name = "vme24d16";
		pageval = PGT_VME_D16 | btop(VME24_BASE | (addr & VME24_MASK));
		break;

	case SP_VME32D16:
		name = "vme32d16";
		pageval = PGT_VME_D16 | btop(addr);
		break;

	case SP_VME16D32:
		name = "vme16d32";
		pageval = PGT_VME_D32 | btop(VME16_BASE | (addr & VME16_MASK));
		break;

	case SP_VME24D32:
		name = "vme24d32";
		pageval = PGT_VME_D32 | btop(VME24_BASE | (addr & VME24_MASK));
		break;

	case SP_VME32D32:
		name = "vme32d32";
		pageval = PGT_VME_D32 | btop(addr);
		break;

	case SP_IPI:
		name = "ipi";
		reg = (u_short *)addr;	/* IPI device address */
		br = 0;			/* IPI interrupts thru host adapter */
		break;

	default:
		return (0);
	}

	if (reg == NULL) {
		int offset = addr & PGOFSET;

		extent = btoc(mdr->mdr_size + offset);
		if (extent == 0)
			extent = 1;
		s = splhigh();
		a = rmalloc(kernelmap, (long)extent);
		(void)splx(s);
		if (a == 0)
			panic("out of kernelmap for devices");
		reg = (u_short *)((int)kmxtob(a) | offset);
		segkmem_mapin(&kseg,
		    (addr_t)((int)reg&PAGEMASK), (u_int)mmu_ptob(extent),
		    PROT_READ | PROT_WRITE, pageval, 0);
	}

	i = (*mdr->mdr_probe)(reg, unit);
	if (i == 0) {
		if (a != 0) {
			s = splhigh();
			rmfree(kernelmap, (long)extent, (u_long)a);
			(void)splx(s);
		}
		return (0);
	}
	printf("%s%d at %s 0x%x ", dname, unit, name, addr);
	if (br < 0 || br >= 7) {
		printf("bad priority (%d)\n", br);
		if (a != 0) {
			s = splhigh();
			rmfree(kernelmap, (long)extent, (u_long)a);
			(void)splx(s);
		}
		return (0);
	}

	/*
	 * If br is 0, then no priority was specified in the
	 * config file and the device cannot use interrupts.
	 */
	if (br != 0) {
		/*
		 * If we are paranoid or vectored interrupts are not
		 * going to be used then set up for polling interrupts.
		 */
		if (paranoid || vp == (struct vec *)0) {
			printf("pri %d ", br);
			addintr(br, mdr, dname, unit);
		}

		/*
		 * now set up vectored interrupts if conditions are right
		 */
		if (vp != (struct vec *)0) {
			for (; vp->v_func; vp++) {
				printf("vec 0x%x ", vp->v_vec);
				if (vp->v_vec < VEC_MIN || vp->v_vec > VEC_MAX)
					panic("bad vector");
				/* following test is not completely correct */
				i = vp->v_vec - VEC_MIN;
				if (vme_vector[i].func == vp->v_func)
					panic("duplicate vector");
				vme_vector[i].func = vp->v_func;
				vme_vector[i].arg = vp->v_vptr;
			}
		}
	}
	printf("\n");
	return (reg);
}

#define	SPURIOUS	-1		/* recognized in locore.s */

int level2_spurious, level3_spurious, level4_spurious, level6_spurious;

not_serviced2()
{

	call_default_intr();
	if ((level2_spurious++ % 100) == 1)
		printf("iobus level 2 interrupt not serviced\n");
	return (SPURIOUS);
}

not_serviced3()
{

	call_default_intr();
	if ((level3_spurious++ % 100) == 1)
		printf("iobus level 3 interrupt not serviced\n");
	return (SPURIOUS);
}

not_serviced4()
{

	call_default_intr();
	if ((level4_spurious++ % 100) == 1)
		printf("iobus level 4 interrupt not serviced\n");
	return (SPURIOUS);
}

not_serviced6()
{

	call_default_intr();
	if ((level6_spurious++ % 100) == 1)
		printf("iobus level 6 interrupt not serviced\n");
	return (SPURIOUS);
}

typedef int (*func)();

/*
 * These vectors are used in locore.s to jump to device interrupt routines.
 */
func	level2_vector[NVECT] = {not_serviced2};
func	level3_vector[NVECT] = {not_serviced3};
func	level4_vector[NVECT] = {not_serviced4};
func	level6_vector[NVECT] = {not_serviced6};

/*
 * vmstat assistance.
 * These names will index into the string table generated by config
 * but in the exact order addintr sees them. This allows IOINTR to quickly
 * find the right counter to increment.
 * (We use the fact that the arrays are initialized to 0 by default).
 */

/* mapping table into av_nametab in ioconf.c. */
int level2_names[NVECT];
int level3_names[NVECT];
int level4_names[NVECT];
int level6_names[NVECT];

/* places to count interrupts */
int level2_intcnt[NVECT];
int level3_intcnt[NVECT];
int level4_intcnt[NVECT];
int level6_intcnt[NVECT];

/*
 * vmstat -i support for autovectored interrupts.
 * add the autovectored interrupt to the table of mappings of
 * interrupt counts to device names. We can't do much about
 * a device that has multiple units, so we print '?' for such.
 * Otherwise, we try to use the real unit number.
 */
void
addioname(level, vec, name, unit)
	int level;
	int vec;
	char *name;
	int unit;
{
	register int k;
	register int index = 0;
	register char **nt;
	register int *intp;
	char devunit;
	int namelen;
	extern char *av_nametab[];
	extern char **av_end;

	switch (level) {
	case 2:
		intp = level2_names;
		break;
	case 3:
		intp = level3_names;
		break;
	case 4:
		intp = level4_names;
		break;
	case 6:
		intp = level6_names;
		break;
	default:
		panic("addioname");
		/* NOTREACHED */
	}
	devunit = (char)((int)'0' + unit);
	namelen = strlen(name);
	if (intp[vec] != 0) {
		av_nametab[intp[vec] - 1][namelen] = '?';
		return;
	}
	for (nt = &av_nametab[0], k = 0; nt < av_end; nt++, k++) {
		if (strncmp(name, *nt, namelen) == 0) {
			index = k + 1;	/* 0 ==> uninitialized */
			if ((*nt)[namelen] == devunit)
				break;
		}
	}
	if (index != 0) {
		intp[vec] = index;
		av_nametab[index - 1][namelen] = devunit;
	}
}

/*
 * Arrange for a driver to be called when a particular
 * auto-vectored interrupt occurs.
 * NOTE: every device sharing a driver must be on the
 * same interrupt level for polling interrupts because
 * there is only one entry made per driver.
 */
addintr(lvl, mdr, name, unit)
	struct mb_driver *mdr;
	char *name;
	int unit;
{
	register func f;
	register func *fp;
	register int i;

	switch (lvl) {
	case 1:
		return;		/* bogus - these devices don't interrupt */
	case 2:
		fp = level2_vector;
		break;
	case 3:
		fp = level3_vector;
		break;
	case 4:
		fp = level4_vector;
		break;
	case 5:
		panic("addintr called with level 5");
		/* NOTREACHED */
	case 6:
		fp = level6_vector;
		break;
	default:
		printf(
"addintr: cannot set up polling interrupt on processor level %d\n", lvl);
		panic("addintr");
		/* NOTREACHED */
	}
	if ((f = mdr->mdr_intr) == NULL)
		return;
	for (i = 0; i < NVECT; i++) {
		if (*fp == NULL)	/* end of list found */
			break;
		if (*fp == f) {		/* already in list */
			addioname(lvl, i, name, unit);
			return;
		}
		fp++;
	}
	if (i >= NVECT)
		panic("addintr: too many devices");
	fp[0] = fp[-1];		/* move not_serviced to end */
	fp[-1] = f;		/* add f to list */
	addioname(lvl, i - 1, name, unit);
}

/*
 * This is for crazy devices that don't know when they interrupt.
 * We just call them at the end after all the sane devices have decided
 * the interrupt is not their fault.
 */
func	default_intrs[NVECT];

add_default_intr(f)
	func f;
{
	register int i;
	register func *fp;

	fp = default_intrs;
	for (i = 0; i < NVECT; i++) {
		if (*fp == NULL)	/* end of list found */
			break;
		if (*fp == f)		/* already in list */
			return;
		fp++;
	}
	if (i >= NVECT)
		panic("add_default_intr: too many devices");
	*fp = f;		/* add f to list */
}

call_default_intr()
{
	register func *fp;

	for (fp = default_intrs; *fp; fp++)
		(*fp)();
}

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
	 * No filesystem type specified, try to get get default
	 */
	if (*(swp->bo_fstype) == '\0' &&
	    (vsw = getfstype("swap", swp->bo_fstype))) {
		(void) strcpy(swp->bo_fstype, vsw->vsw_name);
		vfsp = (struct vfs *)new_kmem_alloc(sizeof (*vfsp), KMEM_SLEEP);
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
				vfsp = (struct vfs *)
				    new_kmem_alloc(sizeof (*vfsp), KMEM_SLEEP);
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

			vs = (struct vfsstats *)new_kmem_zalloc(
				sizeof (struct vfsstats), KMEM_SLEEP);
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

	swp = &swapfile;
	if (strcmp(swp->bo_name, "sim0") != 0)
		panic("bad swap device");
	swp->bo_vp = bdevvp(makedev(1, 1));
	if (VOP_OPEN(&swp->bo_vp, FREAD+FWRITE, u.u_cred))
		panic("cannot open swap device");
	swp->bo_size = SIMD_SSIZE;
	if (*swp->bo_fstype == '\0') {
		(void) strcpy(swp->bo_fstype, "spec");
	}
	printf("swap on %s fstype %s size %dK\n",
	    swp->bo_name, swp->bo_fstype,
	    dbtob(swp->bo_size) / 1024);
	swp->bo_flags |= BO_VALID;

	swap_present = 1;		/* main checks for this */
#endif SAS
}

#include "ms.h"

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
#define	NOUNIT		-1

extern	struct fileops vnodefops;

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
	char *cp;
	register struct mb_device *md;
	register struct mb_driver *mdr;
	int zsunit = NOUNIT;
	int bwunit = NOUNIT;
	int bwmajor;
	struct termios termios;
	int kbdtranslatable = TR_CANNOT;
	int kbdspeed = B9600;
	struct vnode *kbdvp;
	struct vnode *conskbdvp;
	struct vnode *mousevp;
	struct vnode *consmousevp;
	struct file *fp;
	int i;

	stop_mon_clock();	/* turn off monitor polling clock */

	/*
	 * check for console on same ascii port to allow full speed
	 * output by using the UNIX driver and avoiding the monitor.
	 */
	if (*romp->v_insource == INUARTA && *romp->v_outsink == OUTUARTA)
		rconsdev = makedev(ZSMAJOR, 0);
	else if (*romp->v_insource == INUARTB && *romp->v_outsink == OUTUARTB)
		rconsdev = makedev(ZSMAJOR, 1);
#else SAS
	rconsdev = makedev(SIMCMAJOR, 0);
#endif SAS
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
		if (e = VOP_OPEN(&rconsvp, FREAD|FWRITE|FNOCTTY, u.u_cred))
			printf("console open failed: error %d\n", e);
		/* now we must close it to make console logins happy */
		VOP_CLOSE(rconsvp, FREAD|FWRITE, 1, u.u_cred);
		consdev = rconsdev;
		consvp = rconsvp;
		return;
	}
#ifndef SAS

	if (*romp->v_insource == INUARTA)
		kbddev = makedev(ZSMAJOR, 0);
	if (*romp->v_insource == INUARTB)
		kbddev = makedev(ZSMAJOR, 1);
	if (kbddev != NODEV)
		kbdspeed = zsgetspeed(kbddev);

	/*
	 * Look for the [last] kbd/ms and matching fbtype.
	 * N.B.	We can not use fbaddr to discriminate (eg, the 70),
	 *	as it is hardwired into the proms, based on cpu type.
	 */
	for (md = mbdinit; mdr = md->md_driver; md++) {
		if (!md->md_alive || md->md_slave != -1)
			continue;
		cp = mdr->mdr_dname;
		if (strncmp(cp, "zs", 2) == 0) {
			if (md->md_flags & ZS_KBDMS)
				zsunit = md->md_unit;
		}
		console_fb(cp, md, &bwmajor, &bwunit);
	}
	/*
	 * Use serial keyboard and mouse if found flagged uart
	 */
	if (zsunit != NOUNIT) {
#if NMS > 0
		if (mousedev == NODEV)
			mousedev = makedev(ZSMAJOR, 2*zsunit+MOUSEMINOR);
#endif
		if (kbddev == NODEV) {
			kbddev = makedev(ZSMAJOR, 2*zsunit+KBDMINOR);
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

	if (mousedev != NODEV) {
		/*
		 * Open the mouse device.
		 */
		mousevp = makespecvp(mousedev, VCHR);
		if (e = VOP_OPEN(&mousevp, FREAD|FNOCTTY, u.u_cred)) {
			printf("mouse open failed: error %d\n", e);
			goto nomouse;
		}
		e = FLUSHRW;	/* Flush both read and write */
		strioctl(mousevp, I_FLUSH, (caddr_t)&e, FREAD);
		u.u_error = 0;			/* Just In Case */

		if (!ldreplace(mousevp, mousevp->v_stream, MOUSELDISC))
			u.u_error = EINVAL;
		e = u.u_error;
		if (e != 0) {
			printf(
	"consconfig: can't set mouse line discipline: error %d\n", e);
			(void) VOP_CLOSE(mousevp, FREAD+FWRITE, 1, u.u_cred);
			goto nomouse;
		}
		/*
		 * Open the "console mouse" device, and link
		 * the mouse device under it.
		 * XXX - there MUST be a better way to do this!
		 */
		consmousevp = makespecvp(makedev(CONSMOUSEMAJOR, 0), VCHR);
		e = VOP_OPEN(&consmousevp, FREAD|FWRITE|FNOCTTY, u.u_cred);
		if (e != 0) {
			printf(
			    "console mouse device open failed: error %d\n", e);
			(void) VOP_CLOSE(mousevp, FREAD, 1, u.u_cred);
			goto nomouse;
		}
		if ((fp = falloc()) == NULL) {
			printf("can't get file descriptor for mouse\n");
			(void) VOP_CLOSE(consmousevp, FREAD|FWRITE, 1,
			    u.u_cred);
			(void) VOP_CLOSE(mousevp, FREAD, 1, u.u_cred);
			goto nomouse;
		}
		i = u.u_r.r_val1;	/* XXX - get FD number */
		fp->f_flag = FREAD;
		fp->f_type = DTYPE_VNODE;
		fp->f_data = (caddr_t)mousevp;
		fp->f_ops = &vnodefops;
		strioctl(consmousevp, I_PLINK, (caddr_t)&i, FREAD);
		if (u.u_error != 0) {
			printf("I_PLINK failed: error %d\n", u.u_error);
			(void) VOP_CLOSE(consmousevp, FREAD|FWRITE, 1,
			    u.u_cred);
			(void) VOP_CLOSE(mousevp, FREAD, 1, u.u_cred);
			goto nomouse;
		}
		u.u_ofile[i] = NULL;
		closef(fp);	/* don't need this any more */
		(void)VOP_CLOSE(consmousevp, FREAD|FWRITE, 1, u.u_cred);
			/* we're done with this, as well */
		VN_RELE(consmousevp);
	}

nomouse:
	/*
	 * Set up default frame buffer.
	 */
	if (fbdev == NODEV) {
		if (bwunit != NOUNIT)
			fbdev = makedev(bwmajor, bwunit);
#if !defined(MUNIX) && !defined(MINIROOT)
		else
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
		printf("I_PLINK failed: error %d\n", u.u_error);
		panic(
	"consconfig: can't link /dev/kbd under workstation console");
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
		printf("WARNING: ID prom checksum error\n");
	if (id.id_format == 1) {
		(void) localetheraddr((struct ether_addr *)id.id_ether,
		    (struct ether_addr *)NULL);
	} else
		printf("INVALID FORMAT CODE IN ID PROM: %d\n", id.id_format);
}

char *cpu_str = 0;

/*
 * We set the cpu type and associated variables. Should there get to
 * be too many variables, they should be collected together in a
 * structure and indexed by cpu type.
 */
setcputype()
{
	struct idprom id;

	cpu = -1;
	getidprom((char *)&id);
	if (id.id_format == 1) {
		switch (id.id_machine) {
		case CPU_SUN4_470:
#ifndef SUN4_470
			panic("not configured for SUN4_470");
#endif !SUN4_470
			cpu_str = "Sun SPARCsystem 400";
			break;
		case CPU_SUN4_330:
#ifndef SUN4_330
			panic("not configured for SUN4_330");
#endif !SUN4_330
			cpu_str = "Sun SPARCsystem 300";
			break;
		case CPU_SUN4_260:
#ifndef SUN4_260
			panic("not configured for SUN4_260");
#endif !SUN4_260
			cpu_str = "Sun 4/200 Series";
			break;
		case CPU_SUN4_110:
#ifndef SUN4_110
			panic("not configured for SUN4_110");
#endif !SUN4_110
			cpu_str = "Sun 4/100 Series";
			break;
		default:
			printf("unknown machine type 0x%x in ID PROM\n",
			    id.id_machine);
			panic("unknown machine type");
			break;
		}
	} else {
		panic("invalid ID PROM format");
	}
	cpu = id.id_machine;
}

machineid()
{
	struct idprom id;
	register int x;

#ifdef SAS
	return (1);
#else
	getidprom((char *)&id);
	x = id.id_machine << 24;
	x += id.id_serial;
	return (x);
#endif SAS
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

/*
 * There should be a way to get these number from the system, instead of
 * hardcoding them.
 */
#define	BWTWOMAJOR	27		/* Sun 2 black and white driver */
#define	CGTWOMAJOR	31		/* Sun 2 color driver */
#define	CGFOURMAJOR	39		/* Sun 4 color driver */
#define	CGEIGHTMAJOR	64		/* cgeight color driver */
#define	CGSIXMAJOR	67		/* cgsix color driver */
#define	CGNINEMAJOR	68		/* cgnine color driver */
#define	VXMAJOR 	101		/* vx driver */

/*
 * This needs to be moved to <sun/fbio.h>!
 */
#define FBTYPE_SUN_VX 20

/*
 * Compare the device name as returned from the prom, the initial substring
 * of the device name from the config file, and a name template.
 */
#define	FBMATCH(prom, config, template) \
	(strcmp((prom), (template)) == 0 && \
	strncmp((config), (template), strlen((template))) == 0)

#define	FBLIST_SIZE \
			(sizeof (fb_list) / sizeof (struct fblist_strc))

/*
 * table of device names and major numbers indexed by device type defined in
 * <sun/fbio.h>
 */
static char	NO_CONSOLE[] = "!Not A Console Device!";

static struct fblist_strc {
	char	*fb_name;
	int	fb_major;
} fb_list[] = {
	{ NO_CONSOLE, NOUNIT },		/* FBTYPE_SUN1BW = 0 */
	{ NO_CONSOLE, NOUNIT },		/* FBTYPE_SUN1COLOR = 1 */
	{ "bwtwo", BWTWOMAJOR },	/* FBTYPE_SUN2BW = 2 */
	{ "cgtwo", CGTWOMAJOR },	/* FBTYPE_SUN2COLOR = 3 */
	{ NO_CONSOLE, NOUNIT },		/* FBTYPE_SUN2GP = 4 */
	{ NO_CONSOLE, NOUNIT },		/* FBTYPE_SUN5COLOR = 5 */
	{ NO_CONSOLE, NOUNIT },		/* FBTYPE_SUN3COLOR = 6 */
	{ "cgeight", CGEIGHTMAJOR },	/* FBTYPE_MEMCOLOR = 7 */
	{ "cgfour", CGFOURMAJOR },	/* FBTYPE_SUN4COLOR = 8 */
	{ NO_CONSOLE, NOUNIT },		/*  9 */
	{ NO_CONSOLE, NOUNIT },		/* 10 */
	{ NO_CONSOLE, NOUNIT },		/* 11 */
	{ "cgsix", CGSIXMAJOR },	/* FBTYPE_SUNFAST_COLOR = 12 */
	{ "cgnine", CGNINEMAJOR },	/* FBTYPE_SUNROP_COLOR = 13 */
	{ NO_CONSOLE, NOUNIT },		/* 14 */
	{ NO_CONSOLE, NOUNIT },		/* 15 */
	{ NO_CONSOLE, NOUNIT },		/* 16 */
	{ NO_CONSOLE, NOUNIT },		/* 17 */
	{ NO_CONSOLE, NOUNIT },		/* 18 */
	{ NO_CONSOLE, NOUNIT },		/* 19 */
	{ "vx",     VXMAJOR },		/* 20 
		N.B. VX is identified as FBTYPE_SUNFAST_COLOR; See code below */
	{ NO_CONSOLE, NOUNIT },		/* 21 */
};


console_fb (cp, md, bwmajor, bwunit)
	char			*cp;
	struct mb_device	*md;
	int			*bwmajor,
				*bwunit;
{
	/*
	 * Old fashion prom returns the frame buffer type as an integer as in
	 * <sun/fbio.h>.  New style prom returns the name of the frame buffer.
	 * We first try the old style. If it does not match, use the new style.
	 */
	register char *fb_type = (char *)*romp->v_fbtype;
	register unsigned int idx;
	register struct fblist_strc *tblptr = fb_list;

	idx = (unsigned int) fb_type;
	if (idx < FBLIST_SIZE) {	/* there is no small integer pointers */

/* N.B.
 * A VME Accelerated 8-bit FB is really the VX, although the boot PROMs
 * identify it as a FBTYPE_SUNFAST_COLOR, a.k.a LEGO or cgsix.  This is
 * hysterical (I mean, historical) and will be fixed.
 */
		if (idx==FBTYPE_SUNFAST_COLOR && 
		    EEPROM->ee_diag.eed_console==EED_CONS_COLOR &&
			*romp->v_fbaddr==0)		idx=FBTYPE_SUN_VX;

		tblptr += idx;
		if (strncmp (cp, tblptr->fb_name,
			strlen (tblptr->fb_name)) == 0) {
			    *bwmajor = tblptr->fb_major;
			    *bwunit = md->md_unit;
		}
		return;
	}

	/*
	 * Now that we know the fb_type is a string, we simply compare it with
	 * entries in the table one by one.  This is the new style (Forth
	 * based) boot prom.
	 */
	for (idx = 0; idx < FBLIST_SIZE; idx++, tblptr++) {
		if (FBMATCH (fb_type, cp, tblptr->fb_name)) {
			*bwmajor = tblptr->fb_major;
			*bwunit = md->md_unit;
			return;
		}
	}
}
