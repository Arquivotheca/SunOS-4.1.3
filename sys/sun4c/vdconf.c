/*	@(#)vdconf.c 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifdef VDDRV

#include <syscall.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/map.h>
#include <sys/vm.h>
#include <sys/conf.h>
#include <sys/socket.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <machine/pte.h>
#include <machine/psl.h>

#include <sundev/mbvar.h>

#include <net/if.h>
#include <net/af.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>

#include <sun/autoconf.h>
#include <sun/vddrv.h>

/*
 * Define vd_moduleops for each supported module type
 */

/*
 * Null operations; used for uninitialized and "user" modules.
 */
int vd_null();

struct vd_moduleops vd_nullops = {
	vd_null,	vd_null,	vd_null
};

/*
 * Device drivers
 */
int vd_statdrv(), vd_installdrv(), vd_removedrv();

struct vd_moduleops vd_driverops = {
	vd_statdrv,	vd_installdrv,	vd_removedrv
};

/*
 * Network interface drivers
 */
int vd_installnet(), vd_removenet();

struct vd_moduleops vd_netops = {
	vd_null,	vd_installnet,	vd_removenet
};

/*
 * System calls
 */

int vd_statsys(), vd_installsys(), vd_removesys();

struct vd_moduleops vd_syscallops = {
	vd_statsys,	vd_installsys,	vd_removesys
};

extern int nosys(), errsys(), nodev(), nulldev(), seltrue();
extern char *strcpy();

struct bdevsw *vd_getbdevsw();
struct cdevsw *vd_getcdevsw();
struct sysent *vd_getsysent();
struct vdconf *vd_getvdconf();
int vd_linkdrv(), vd_configdrv(), vd_unconfigdrv();
int vd_checkdrvlinkage(), vd_unlinknet();
int vd_userconfig();
void vd_userunconfig();
#ifdef UFS
int has_ufsmounted();
#endif 

/*
 * Null operation; return 0.
 */
/*ARGSUSED*/
/*VARARGS1*/
int
vd_null(vdp, vdi)
	struct vddrv *vdp;
	caddr_t vdi;
{
	return 0;
}

/*
 * Driver status info
 */
int
vd_statdrv(vdp, vds)
	struct vddrv *vdp;
	struct vdstat *vds;
{
	vds->vds_modinfo[0] = vdp->vdd_vdtab->vdldrv_blockmajor;
	vds->vds_modinfo[1] = vdp->vdd_vdtab->vdldrv_charmajor;
	return 0;
}

/*
 * Install a new driver
 */

int
vd_installdrv(vdp, vdi)
	struct vddrv *vdp;
	struct vdioctl_load *vdi;
{
	register struct vdlinkage *vdl = vdp->vdd_vdtab;
	register int status;

	/*
	 * Get user config info
	 */
	if (vdi->vdi_userconf != NULL) {
		status = vd_userconfig(vdp, vdi);
		if (status != 0)
			return status;
	}
	status = vd_configdrv(vdp, vdl->vdldrv_dev_ops);
	if (status != 0)
		return status;
	status = vd_linkdrv(vdp);
	if (status != 0) {
		(void) vd_unconfigdrv(vdp, vdl->vdldrv_dev_ops);
	}
	return status;
}

int
vd_removedrv(vdp, vdi)
	struct vddrv *vdp;
	struct vdioctl_unload *vdi;
{
	register struct vdlinkage *vdl = vdp->vdd_vdtab;
	register int status;

	status = vd_unlinkdrv(vdp, vdi);
	if (status == 0) {
		status = vd_unconfigdrv(vdp, vdl->vdldrv_dev_ops);
	}
	return status;
}

/*
 * Link a driver into the system tables.
 */
int
vd_linkdrv(vdp)
	struct vddrv *vdp;
{
	register struct vdlinkage *vdl = vdp->vdd_vdtab;
	register struct bdevsw *bdp = NULL;
	register struct cdevsw *cdp = NULL;
	register int s;

	if (vdl->vdldrv_bdevsw) { 	/* find a bdevsw entry */
		bdp = vd_getbdevsw(vdl->vdldrv_blockmajor);
		if (bdp == NULL)
			return EINVAL;
	}

	if (vdl->vdldrv_cdevsw) {	/* find a cdevsw entry */
		cdp = vd_getcdevsw(vdl->vdldrv_charmajor);
		if (cdp == NULL)
			return EINVAL;
	}

	/*
	 * Everything looks go.
	 */
	s = spl7();
	if (bdp != NULL) {
		*bdp = *vdl->vdldrv_bdevsw; /* setup bdevsw */
		vdl->vdldrv_bdevsw = bdp;
		vdl->vdldrv_blockmajor = bdp - bdevsw; /* set block major */
	}

	if (cdp != NULL) {
		*cdp = *vdl->vdldrv_cdevsw; /* setup cdevsw */
		vdl->vdldrv_cdevsw = cdp;
		vdl->vdldrv_charmajor = cdp - cdevsw; /* set char major */
	}

	(void) splx(s);
	return 0;
}

/*
 * Unlink a driver from from the system.
 */

static struct bdevsw free_bdevsw_slot = {
	vd_unuseddev, vd_unuseddev,	/* d_open, d_close */
	vd_unuseddev, vd_unuseddev,	/* d_strategy, d_dump */
	vd_unuseddev, 0			/* d_psize, d_flags */
};

static struct cdevsw free_cdevsw_slot = {
	vd_unuseddev, vd_unuseddev,	/* d_open, d_close */
	vd_unuseddev, vd_unuseddev,	/* d_read, d_write */
	vd_unuseddev, vd_unuseddev,	/* d_ioctl, d_reset */
	vd_unuseddev, vd_unuseddev,	/* d_select, d_mmap */
	0				/* d_str */
};

/*ARGSUSED*/
int
vd_unlinkdrv(vdp, vdi)
	struct vddrv *vdp;
	struct vdioctl_unload *vdi;
{
	register struct vdlinkage *vdl = vdp->vdd_vdtab;
	register struct bdevsw *bdp = vdl->vdldrv_bdevsw;
	register struct cdevsw *cdp = vdl->vdldrv_cdevsw;
	register int s;

#ifdef UFS
	/*
	 * Make sure there aren't filesystems mounted on this device.
	 */

	if (bdp != NULL && has_ufsmounted(vdl->vdldrv_blockmajor))
		return EBUSY;
#endif /* UFS */

	s = spl7();
	if (bdp != NULL)
		*bdp = free_bdevsw_slot;
	if (cdp != NULL)
		*cdp = free_cdevsw_slot;
	(void) splx(s);
	return 0;
}

#ifdef UFS
/*
 * Check whether any file systems are mounted on the specified block device.
 */

#include <ufs/mount.h>

int
has_ufsmounted(blockmajor)
	register int blockmajor;
{
	register struct mount *mp;

	MLOCK();
	for (mp = mounttab; mp != NULL; mp = mp->m_nxt) {
		if (major(mp->m_dev) == blockmajor)
			break;
	}
	MUNLOCK();
	return (mp != NULL);
}
#endif /* UFS */

/*
 * System call status info
 */
int
vd_statsys(vdp, vds)
	struct vddrv *vdp;
	struct vdstat *vds;
{
	vds->vds_modinfo[0] = vdp->vdd_vdtab->vdlsys_num;
	return 0;
}

/*
 * Link a system call into the system by setting the proper sysent entry.
 */

int
vd_installsys(vdp, vdi)
	struct vddrv *vdp;
	struct vdioctl_load *vdi;
{
	register struct sysent *sysp;
	register struct vdlinkage *vdl = vdp->vdd_vdtab;
	register struct vdconf *vdc;
	register int status;

	status = vd_addrcheck(vdp, (caddr_t) vdl->vdlsys_sysent);
	if (status != 0)
		return status;

	/*
	 * Get user config info
	 */
	if (vdi->vdi_userconf != NULL) {
		vdc = vd_getvdconf(vdi->vdi_userconf);
		if (vdc == NULL)
			return EFAULT;
		if (vdc->vdc_type != VDCSYSCALLNUM)
			return EINVAL;
		vdl->vdlsys_num = vdc->vdc_data;
	}

	sysp = vd_getsysent(vdl->vdlsys_num); /* get ptr to sysent entry */
	if (sysp == NULL)
		return EINVAL;		/* invalid system call number */

	sysp->sy_narg = vdl->vdlsys_sysent->sy_narg;
	sysp->sy_call = vdl->vdlsys_sysent->sy_call;
	vdl->vdlsys_sysent = sysp;
	vdl->vdlsys_num    = sysp - sysent;

	return 0;
}

/*
 * Unlink a system call from the system.
 */

/*ARGSUSED*/
int
vd_removesys(vdp, vdi)
	struct vddrv *vdp;
	struct vdioctl_unload *vdi;
{
	vdp->vdd_vdtab->vdlsys_sysent->sy_call = nosys;
	vdp->vdd_vdtab->vdlsys_sysent->sy_narg = 0;
	return 0;
}

/*
 * Configure a network interface
 */

/*ARGSUSED*/
int 
vd_installnet(vdp, vdi)
	struct vddrv *vdp;
	struct vdioctl_load *vdi;
{
	register struct vdlinkage *vdl = vdp->vdd_vdtab;

	return vd_configdrv(vdp, vdl->vdlnet_dev_ops);
}

/*
 * Remove a network interface
 */

/*ARGSUSED*/
int
vd_removenet(vdp, vdi)
	struct vddrv *vdp;
	struct vdioctl_unload *vdi;
{
	register struct vdlinkage *vdl = vdp->vdd_vdtab;
	register int status;

	status = vd_unlinknet(vdp);
	if (status == 0) {
		status = vd_unconfigdrv(vdp, vdl->vdlnet_dev_ops);
	}
	return status;

}

/*
 * Unlink a network interface from the system.
 */

int
vd_unlinknet(vdp)
	struct vddrv *vdp;
{
	register struct ifnet *ifp;
	register struct ifnet *ifp_prev;
	register struct ifaddr *ifa;

	struct inpcb *inp, *head;
	struct in_ifaddr *ia, *iaprev;
	char ifname[IFNAMSIZ];
	char *s2;
	int s;

	extern struct inpcb udb;
	extern struct inpcb tcb;

	/*
	 * Get the interface name and make it into a name-unit array such
	 * as "ub0".
	 */
	s2 = strcpy(ifname, vdp->vdd_vdtab->vdlnet_dname);
	*s2++ = '0';		/* we only support unit 0 for now */
	*s2 = '\0';		/* terminate the string */

	/*
	 * Get the ifnet structure from the interface name-unit
	 */
	if ((ifp = ifunit(ifname, strlen(ifname))) == NULL) {
		printf("vd_unlinknet: can't find %s\n", ifname);
		return EINVAL;
	}

	if (ifp->if_flags & IFF_UP)
		return EBUSY;

	s = spl7();

	/*
	 * Flush inpcbs.  Too bad we have to play all these games here.
	 * But the system wasn't designed to have drivers unloaded!!!
	 */
	head = &udb;			/* start with udp inpcb list */

again:
	for (inp = head->inp_next; inp != head; inp = inp->inp_next) {
		if (inp->inp_route.ro_rt && inp->inp_route.ro_rt->rt_ifp == ifp)
			in_losing(inp);	/* flush routing info */
	}
	if (head == &udb) {
		head = &tcb;		/* flush tcp inpcb list */
		goto again;
	}

	/*
	 * Flush the routing tables!!!
	 */
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		rtinit(&ifa->ifa_addr, &ifa->ifa_addr, SIOCDELRT, RTF_HOST);
		rtinit(&ifa->ifa_addr, &ifa->ifa_addr, SIOCDELRT, 0);
	}

	/*
	 * Remove in_ifaddr entry (if any)
	 * ZZZ -- have to figure out how to m_free the in_ifaddr entry!!!!
	 */
	for (ia = in_ifaddr, iaprev = NULL; ia != NULL; ia = ia->ia_next) {
		if (ia->ia_ifp == ifp) {
			if (iaprev == NULL)
				in_ifaddr = ia->ia_next;
			else
				iaprev->ia_next = ia->ia_next;

			break;
		}
		iaprev = ia;
	}

	/*
	 * Now unlink our ifnet structure
	 */
	if (ifp == ifnet) {
		ifnet = ifp->if_next;
	} else {
		ifp_prev = ifnet;

		while (ifp_prev && ifp_prev->if_next != ifp)
			ifp_prev = ifp_prev->if_next;

		if (ifp_prev == NULL)
			panic("vd_unlinknet");

		ifp_prev->if_next = ifp->if_next;
	}
	(void) splx(s);
	return 0;
}

#define empty(slot) ((slot) == nodev || (slot) == nulldev || \
			(slot) == seltrue || (slot) == 0)

/*
 * Find a free bdevsw entry or check if the specified one if free.
 */
struct bdevsw *
vd_getbdevsw(major_dev)
	register int major_dev;
{
	register struct bdevsw *bdp;

	if (major_dev < 0 || major_dev >= nblkdev)
		return NULL;

	if (major_dev != 0) {
		bdp = &bdevsw[major_dev];
		/*
		 * If a major number is specified by the user and
		 * the bdevsw entry is not in use or it's one reserved
		 * for loadable drivers, then it's ok to use this one.
		 */
		return (bdp->d_open == vd_unuseddev ||
			(empty(bdp->d_open)	&& empty(bdp->d_close) &&
			 empty(bdp->d_strategy)	&& empty(bdp->d_dump)))
			? bdp : NULL;
	}

	for (bdp = bdevsw; bdp != &bdevsw[nblkdev]; bdp++) {
		if (bdp->d_open == vd_unuseddev)
			return bdp;		/* found an unused one */
	}
	return NULL;
}

/*
 * Find a free cdevsw entry or check if the specified one is free.
 */
struct cdevsw *
vd_getcdevsw(major_dev)
	register int major_dev;
{
	register struct cdevsw *cdp;

	if (major_dev < 0 || major_dev >= nchrdev)
		return NULL;

	if (major_dev != 0) {
		cdp = &cdevsw[major_dev];
		/*
		 * If a major number is specified by the user and
		 * the cdevsw entry is not in use or it's one reserved
		 * for loadable drivers, then it's ok to use this one.
		 */
		return (cdp->d_open == vd_unuseddev ||
			(empty(cdp->d_open)	&& empty(cdp->d_close) &&
			 empty(cdp->d_read)	&& empty(cdp->d_write) &&
			 empty(cdp->d_ioctl)	&& empty(cdp->d_reset) &&
			 empty(cdp->d_select)	&& empty(cdp->d_mmap)))
			? cdp : NULL;
	}

	for (cdp = cdevsw; cdp != &cdevsw[nchrdev]; cdp++) {
		if (cdp->d_open == vd_unuseddev)
			return cdp;		/* found an unused one */
	}
	return NULL;
}

/*
 * Find a free sysent entry or check if the specified one is free.
 */
struct sysent *
vd_getsysent(sysnum)
	register int sysnum;
{
	register int i;

	extern int nsysent;
	extern int firstvdsys;
	extern int lastvdsys;

	if (sysnum < 0 || sysnum >= nsysent)
		return NULL;

	if (sysnum != 0)
		return (sysent[sysnum].sy_call == nosys) ?
			&sysent[sysnum] : NULL;

	/*
	 * Find the next free system call number
	 */
	for (i = firstvdsys; i <= lastvdsys; i++) {
		if (sysent[i].sy_call == nosys)
			return &sysent[i];
	}
	return NULL;
}

/*
 * Routine address placed in devsw entries which are usable for
 * a loadable driver.
 */
int
vd_unuseddev()
{
	return ENODEV;
}

/*
 * Routine to verify that an address is valid.
 */

int
vd_addrcheck(vdp, addr)
	register struct vddrv *vdp;
	register caddr_t addr;
{
	return (addr <  vdp->vdd_vaddr ||
	    addr >= vdp->vdd_vaddr + vdp->vdd_size) ? EINVAL : 0;
}

/*
 * Configure a new driver.
 *
 * Failure to successfully configure a controller or a device is not considered
 * an error.  However, if no devices or controllers come online then an
 * error is returned.
 */
int
vd_configdrv(vdp, ops)
	struct vddrv *vdp;
	struct dev_ops *ops;
{
	register int status = 0;

	if (vd_checkdrvlinkage(vdp, ops) < 0)
		return EINVAL;	/* bad linkage structure */

	/*
	 * Configure the driver
	 * 1. Add the driver name to the av_nametab array (how?!) XXX
	 * 2. Add the ops vector to the ops list (do we really need to??) XXX
	 * 3. If not a pseudo-device, call add_drv to attach any
	 *    devices to this driver.  If no devices attach, return ENXIO
	 */

	/* XXX add the driver name to the av_nametab array ??? */

	status = add_ops(ops);
	if (status != 0)
		return (status);

	if (vdp->vdd_vdtab->vdl_magic != VDMAGIC_PSEUDO &&
	    add_drv(ops) <= 0) {
		(void) rem_ops(ops);
		return ENXIO; /* no devices came up*/
	}

	return 0;	/* success */
}

/*
 * Unconfigure a driver.
 */

int
vd_unconfigdrv(vdp, ops)
	struct vddrv *vdp;
	struct dev_ops *ops;
{
	register int s;
	void rem_drv();

	s = splx(pritospl(SPLMB));

	if (vdp->vdd_userconf) {
		vdp->vdd_userconf = 0;
		vd_userunconfig(vdp);
	}

	if (vdp->vdd_vdtab->vdl_magic != VDMAGIC_PSEUDO)
		rem_drv(ops);

	(void) rem_ops(ops);

	(void) splx(s);
	return 0;	/* success */
}

/*
 * Make sure that the driver linkage structure looks ok.
 * This is mostly for debugging to help the driver developer.
 */

int
vd_checkdrvlinkage(vdp, ops)
	register struct vddrv *vdp;
	struct dev_ops *ops;
{
	if (vd_addrcheck(vdp, (caddr_t) ops) != 0)
		return EINVAL;

	return 0;
}

struct vdconf *
vd_getvdconf(vdcp)
	struct vdconf *vdcp;
{
	static struct vdconf vdc;

	return (copyin((caddr_t)vdcp, (caddr_t)&vdc, sizeof vdc) == 0) ?
		 &vdc : NULL;
}

/*
 * Get user specified configuration information and put it in
 * the vdlinkage structure.
 *
 * The user configuration is specified in an array of vdconf structures.
 * Each entry has a type and a pointer to the structure appropriate for
 * that type.
 */
int
vd_userconfig(vdp, vdi)
	register struct vddrv *vdp;
	struct vdioctl_load *vdi;
{
	register struct vdconf *uvdc, *vdc;

	uvdc = vdi->vdi_userconf;
	vdc = vd_getvdconf(uvdc);	/* get ptr to user config info */

	/*
	 * Make a pass through the user configuration info.  On this pass
	 * we count up the controllers and devices and set the block
	 * and character major numbers.
	 */
	while (vdc != NULL && vdc->vdc_type != VDCEND) {
		switch (vdc->vdc_type) {
		case VDCBLOCKMAJOR:
			vdp->vdd_vdtab->vdldrv_blockmajor = vdc->vdc_data;
			break;
		case VDCCHARMAJOR:
			vdp->vdd_vdtab->vdldrv_charmajor = vdc->vdc_data;
			break;
		default:
			return EINVAL;
		}
		vdc = vd_getvdconf(++uvdc);
	}

	if (vdc == NULL)
		return EFAULT;

	vdp->vdd_userconf = 1; /* indicate we're using user conf info */
	return 0;
}

/*
 * Free user specified configuration information.
 */
/*ARGSUSED*/
void
vd_userunconfig(vdp)
	struct vddrv *vdp;
{

	/* nothing to do on a sun4c! */
}
#endif /* VDDRV */
