/*	@(#)vddrv.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifdef VDDRV

/*
 * /dev/vd driver for loadable module support.  It implements ioctls
 * to load and unload loadable modules.  The types of modules supported
 * are drivers, network interfaces, and system calls.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/mman.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/map.h>

#ifdef sun386
#include <sys/aouthdr.h>
#include <sys/scnhdr.h>
#else
#include <sys/exec.h>
#endif

#include <vm/faultcode.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_vn.h>
#include <vm/page.h>

#include <machine/pte.h>
#include <machine/seg_kmem.h>
#include <machine/mmu.h>

#include <sun/vddrv.h>

/*
 * Global data.
 */
int vdisopen = 0;			/* flag to indicate /dev/vd is open */
extern struct vd_moduleops vd_nullops;	/* Initial ops for new module */
extern int vd_addrcheck();		/* Check that address is in bounds */

/*
 * Local functions
 */
static int vd_loadmodule(), vd_unloadmodule(), vd_stat(), vd_statmodule();
static int vd_getnextvaddr(), vd_freenextvaddr();
static int vd_getfilehdr(), vd_copyinmodule(), vd_entry();
static void vddrv_free(), vd_freespace();
static struct vddrv *vddrv_alloc(), *vddrv_lookup();
static struct vd_moduleops *vd_getops();

#define	vd_pagealign(n)		ptob(btopr(n))

static struct vddrv *vddrv_list = NULL;	/* List of loaded modules */
static int vdloaded = 0;		/* # of loaded modules (for kadb) */
static int vdcounter = 0;		/* For generating module ID's */
static int vdcurmod = 0;		/* current module (for kadb) */


/*ARGSUSED*/
int
vdopen(dev, flags)
	dev_t dev;
	int flags;
{
	vdisopen = 1;
	return (0);
}

/*ARGSUSED*/
int
vdclose(dev, flags)
	dev_t dev;
	int flags;
{
	vdisopen = 0;
	return (0);
}

/*ARGSUSED*/
int
vdioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	if (!suser() && cmd != VDSTAT)
		return (EPERM);

	switch (cmd) {
	case VDLOAD:		/* load a module */
		return (vd_loadmodule((struct vdioctl_load *)data));

	case VDUNLOAD:		/* unload a module */
		return (vd_unloadmodule((struct vdioctl_unload *)data));

	case VDSTAT:		/* get module status */
		return (vd_stat((struct vdioctl_stat *)data));

	case VDGETVADDR:	/* reserve kernel memory */
		return (vd_getnextvaddr((struct vdioctl_getvaddr *)data));

	case VDFREEVADDR:	/* free previously reserved memory */
		return (vd_freenextvaddr((struct vdioctl_freevaddr *)data));

	default:
		return (ENOTTY);
	}
}

/*
 * Load a loadable module.
 *
 * The user program must already have mmap'd the module image file.
 * We use its mmap'ing here to load the module.  vdi_mmapaddr is the virtual
 * address in the user program where the module is mmap'd.
 *
 */
static int
vd_loadmodule(vdi)
	register struct vdioctl_load *vdi;
{
	register struct vddrv *vdp;
	register struct vdlinkage *vdl;
	register struct vd_moduleops *ops;
	register int status;

	/*
	 * Look up vddrv entry
	 */
	if ((vdp = vddrv_lookup(vdi->vdi_id)) == NULL)
		return (EINVAL);

	/*
	 * Read the module image file header.
	 */
	status = vd_getfilehdr(vdp, vdi->vdi_mmapaddr, vdi->vdi_symtabsize);
	if (status != 0)
		goto error;

	/*
	 * Load the module into memory.  Virtual and physical space have
	 * already been reserved by the vd_getnextvaddr() ioctl.
	 */
	status = vd_copyinmodule(vdp, vdi->vdi_mmapaddr, vdi->vdi_symtabsize);
	if (status != 0)
		goto error;

	/*
	 * Call the module at its entry point so that it can do
	 * its initialization.
	 */
	status = vd_entry(VDLOAD, vdp, (caddr_t)vdi, (struct vdstat *)NULL);
	if (status != 0)
		goto error;

	vdl = vdp->vdd_vdtab;
	if (vdl == NULL || vd_addrcheck(vdp, (caddr_t)vdl) != 0) {
		status = EINVAL;
		goto error;
	}
	vdp->vdd_intransit = 0;		/* kadb can look at symbols now */

	if (vdl->vdl_magic == VD_TYPELESS)
		return (0);

	ops = vd_getops(vdl->vdl_magic);
	if (ops == NULL) {
		status = EINVAL;
		goto error;
	}
	vdp->vdd_ops = ops;

	/*
	 * Configure the new module & link into the system
	 */
	status = VDD_INSTALL(vdp, vdi);
	if (status != 0) { 		/* let module clean up */
		(void) vd_entry(VDUNLOAD, vdp, (caddr_t)vdi,
				(struct vdstat *)NULL);
		goto error;
	}

	return (0);

error:
	vd_freespace(vdp->vdd_vaddr, vdp->vdd_size); /* free memory */
	vddrv_free(vdp);			/* free vddrv entry */
	return (status);
}

/*
 * Unload a module.
 */

static int
vd_unloadmodule(vdi)
	register struct vdioctl_unload *vdi;
{
	register struct vddrv *vdp;
	register int status;

	/*
	 * Look up vddrv entry
	 */
	if ((vdp = vddrv_lookup(vdi->vdi_id)) == NULL)
		return (EINVAL);

	vdp->vdd_intransit = 1;		/* being unloaded */
	/*
	 * Unload the module.
	 */
	status = vd_entry(VDUNLOAD, vdp, (caddr_t)vdi, (struct vdstat *)NULL);
	if (status != 0) {
		vdp->vdd_intransit = 0;
		return (status);
	}

	/* unlink module from the system */
	status = VDD_REMOVE(vdp, vdi);
	if (status != 0)		/* can't unload */
		return (status);

	vd_freespace(vdp->vdd_vaddr, vdp->vdd_size); /* free memory */
	vddrv_free(vdp);		/* free vddrv slot */
	return (status);
}

/*
 * Return status of loaded modules.
 */
static int
vd_stat(vdi)
	register struct vdioctl_stat *vdi;
{
	register struct vddrv *vdp;
	register int status = 0;
	u_int bufsiz = vdi->vdi_vdstatsize;

	if (vddrv_list == NULL && vdi->vdi_id == -1) {
		vdi->vdi_vdstatsize = 0;
		return (0);
	}

	vdp = (vdi->vdi_id == -1) ? vddrv_list : vddrv_lookup(vdi->vdi_id);
	status = (vdp == NULL) ? EINVAL : vd_statmodule(vdi, vdp);

	if (status == 0) {
		vdi->vdi_vdstatsize = bufsiz - vdi->vdi_vdstatsize;
		vdp = vdp->vdd_next;
		vdi->vdi_id = (vdp == NULL) ? -1 : vdp->vdd_id;
	}
	return (status);
}

/*
 * Get status for a loaded module.
 */
static int
vd_statmodule(vdi, vdp)
	register struct vdioctl_stat *vdi;
	register struct vddrv *vdp;
{
	struct vdstat vdstat;

	if (vdi->vdi_vdstatsize < sizeof (struct vdstat))
		return (EIO);

	bzero((caddr_t)&vdstat, sizeof (struct vdstat));

	vdstat.vds_id    = vdp->vdd_id;	/* module id */
	vdstat.vds_vaddr = (u_long)vdp->vdd_vaddr; /* link address */
	vdstat.vds_size  = vdp->vdd_size;  /* module size */
	if (!vdp->vdd_intransit) {
		vdstat.vds_magic = vdp->vdd_vdtab->vdl_magic;
		(void)copystr(vdp->vdd_vdtab->vdl_name, vdstat.vds_modname,
		    (u_int)VDMAXNAMELEN, (u_int *)NULL);
		(void)VDD_STATUS(vdp, &vdstat);
		/* let module fill vdi_modinfo */
		(void)vd_entry(VDSTAT, vdp, (caddr_t)vdi, &vdstat);
	}

	if (copyout((caddr_t)&vdstat, (caddr_t)vdi->vdi_vdstat,
		    sizeof (struct vdstat)) != 0)
		return (EFAULT);		/* bad user buffer */

	vdi->vdi_vdstatsize -= sizeof (struct vdstat);
	return (0);
}

/*
 * Return the next available virtual address for a module to be loaded.
 * Physical memory is also allocated for the virtual address range.
 */
static int
vd_getnextvaddr(vdi)
	register struct vdioctl_getvaddr *vdi;
{
	register u_long a;
	register u_int size;
	register struct vddrv *vdp;
	register addr_t vaddr;
	register int s;

	size = vd_pagealign(vdi->vdi_size);
	if (size == 0)
		return (EINVAL);

	/*
	 * allocate space in kernel map
	 */
	s = splhigh();
	if ((a = rmalloc(kernelmap, (long)btoc(size))) == 0) {
		(void) splx(s);
		return (ENOMEM);
	}
	(void) splx(s);
	vaddr = kmxtob(a);
	if (segkmem_alloc(&kseg, vaddr, size, 1, SEGKMEM_LOADMOD) == 0) {
		vd_freespace(vaddr, size);
		return (ENOMEM);
	}

	/*
	 * Obtain a vddrv entry
	 */
	vdp = vddrv_alloc();
	vdp->vdd_vaddr = vaddr;
	vdp->vdd_size = size;

	vdi->vdi_id = vdp->vdd_id;
	vdi->vdi_vaddr = (u_long)vaddr;

	return (0);
}

/*
 * Free space reserved by getnextvaddr().
 */

static int
vd_freenextvaddr(vdi)
	struct vdioctl_freevaddr *vdi;
{
	register struct vddrv *vdp;

	if ((vdp = vddrv_lookup(vdi->vdi_id)) == NULL)
		return (EINVAL);
	vd_freespace(vdp->vdd_vaddr, vdp->vdd_size);
	vddrv_free(vdp);
	return (0);
}

/*
 * Free virtual and physical space for a module.
 */
static void
vd_freespace(vaddr, size)
	caddr_t vaddr;
	u_int size;
{
	register int s;

	segkmem_free(&kseg, vaddr, size); /* Free physical memory */
	s = splhigh();
	rmfree(kernelmap, (long)btoc(size), (u_long)btokmx(vaddr));
	(void) splx(s);
}

/*
 * Allocate a new struct vddrv.
 */
static struct vddrv *
vddrv_alloc()
{
	register struct vddrv *vdp;

	while (vddrv_lookup(++vdcounter) != NULL)
		vdcounter %= VD_MAXID;
	vdp = (struct vddrv *)new_kmem_zalloc(
			sizeof (struct vddrv), KMEM_SLEEP);
	vdp->vdd_id = vdcurmod = vdcounter;
	vdcounter %= VD_MAXID;
	vdp->vdd_intransit = 1;
	vdp->vdd_next = vddrv_list;
	vdp->vdd_ops = &vd_nullops;
	vddrv_list = vdp;
	vdloaded++;

	return (vdp);
}

/*
 * Free a vddrv entry.
 */
static void
vddrv_free(vdp)
	struct vddrv *vdp;
{
	register struct vddrv *v = vddrv_list, *pred = NULL;

	while (v != vdp) {
		pred = v;
		v = v->vdd_next;
		if (v == NULL)
			panic("vddrv_free");
	}

	if (pred == NULL)
		vddrv_list = v->vdd_next;
	else
		pred->vdd_next = v->vdd_next;

	kmem_free((caddr_t)v, sizeof (struct vddrv));
	vdloaded--;		/* one less loaded module */
}

/*
 * Find vddrv entry with the specified ID.
 */
static struct vddrv *
vddrv_lookup(id)
	int id;
{
	register struct vddrv *vdp;

	for (vdp = vddrv_list; vdp != NULL; vdp = vdp->vdd_next) {
		if (id == vdp->vdd_id)
			break;
	}
	return (vdp);
}

/*
 * Read the image file header to get module sizes.
 * Most of this code is straight from execve().
 */
#ifdef sun386

static int
vd_getfilehdr(vdp, mmapaddr, symtabsize)
	struct vddrv *vdp;
	caddr_t mmapaddr;
	u_int symtabsize;
{
	struct filehdr fhdr;
	struct aouthdr optaout; /* optional a.out header */
	struct scnhdr scn;	/* COFF per-section info */
	int scns = 0; 		/* must have all three sections! */
	int numsecs;		/* number of COFF sections in file */
	caddr_t hdr;		/* pointer to module headers */

	/*
	 * Copy in first page of file to get segment sizes and magic number.
	 * The a.out magic number must be ZMAGIC = demand paged RO text.
	 */
	hdr = mmapaddr;
	if (copyin(hdr, (caddr_t)&fhdr, sizeof (struct filehdr)) != 0)
		return (EFAULT);
	hdr += sizeof (struct filehdr);

	/* check and see if this is a COFF header. */
	if (!ISCOFF(fhdr.f_magic)) {
		return (EINVAL);
	}

	numsecs = fhdr.f_nscns; /* get number of section headers */

	if (fhdr.f_opthdr != sizeof (struct aouthdr))
		return (EINVAL);

	if (copyin(hdr, (caddr_t)&optaout, sizeof (struct aouthdr)) != 0)
		return (EFAULT);
	hdr += sizeof (struct aouthdr);

	/*
	 * let's get magic number and entry location from optional
	 * a.out header.
	 */
	if (optaout.magic != ZMAGIC)
		return (EINVAL);

	vdp->vdd_entry = (int (*)())optaout.entry;

	/*
	 * go through each COFF section header and get
	 * text, data, and bss size and file offset.  There
	 * had better be at least those three sections!
	 */
	vdp->vdd_textsize = sizeof (struct filehdr) +
			    sizeof (struct aouthdr) +
			    numsecs * sizeof (struct scnhdr);

	while (numsecs-- > 0) {
		if (copyin(hdr, (caddr_t)&scn, sizeof (struct scnhdr)) != 0)
			return (EFAULT);
		hdr += sizeof (struct scnhdr);

		switch ((int)scn.s_flags) {
		case STYP_TEXT:
			scns |= STYP_TEXT;
			vdp->vdd_textvaddr = (addr_t)(scn.s_vaddr &
							~PAGEOFFSET);
			vdp->vdd_textsize += scn.s_size;
			break;

		case STYP_DATA:
			scns |= STYP_DATA;
			vdp->vdd_datavaddr = (addr_t)scn.s_vaddr;
			vdp->vdd_datasize  = scn.s_size;
			break;

		case STYP_BSS:
			scns |= STYP_BSS;
			vdp->vdd_bssvaddr = (addr_t)scn.s_vaddr;
			vdp->vdd_bsssize  = vd_pagealign(scn.s_size);
			break;
		}
	}

	if (scns != (STYP_TEXT|STYP_DATA|STYP_BSS)) {
		return (EINVAL);
	}
	if (vdp->vdd_textsize == 0	||
	    vdp->vdd_datasize == 0	||
	    vdp->vdd_textvaddr + vdp->vdd_textsize != vdp->vdd_datavaddr ||
	    vd_pagealign(vdp->vdd_datavaddr + vdp->vdd_datasize) !=
			(u_int)vdp->vdd_bssvaddr) {
		return (EINVAL);
	}

	if (fhdr->f_nsyms == 0) {
		vdp->vdd_symvaddr	= 0;	/* no symbols */
		vdp->vdd_symsize	= 0;
		vdp->vdd_nsyms		= 0;
		vdp->vdd_symoffset	= 0;
	} else {
		vdp->vdd_symvaddr	= vdp->vdd_bssvaddr + vdp->vdd_bsssize;
		vdp->vdd_symsize	= vd_pagealign(symtabsize);
		vdp->vdd_nsyms		= fhdr.f_nsyms;
		vdp->vdd_symoffset	= fhdr.f_symptr;
	}
	return (0);
}

#else /* !sun386 */

static int
vd_getfilehdr(vdp, mmapaddr, symtabsize)
	struct vddrv *vdp;
	caddr_t mmapaddr;
	u_int symtabsize;
{
	struct exec exec;

	/*
	 * Copy in the module header to get segment sizes and magic number.
	 * The a.out magic number must be OMAGIC = impure text.
	 */
	if (copyin(mmapaddr, (caddr_t)&exec, sizeof (struct exec)) != 0)
		return (EFAULT);

	/*
	 * get magic number and entry location from a.out header.
	 */
	if (exec.a_magic != OMAGIC)
		return (EINVAL);

	vdp->vdd_textvaddr	= vdp->vdd_vaddr;
	vdp->vdd_textsize	= exec.a_text;
	vdp->vdd_datavaddr	= vdp->vdd_textvaddr + vdp->vdd_textsize;
	vdp->vdd_datasize	= exec.a_data;
	vdp->vdd_bssvaddr	= vdp->vdd_datavaddr + vdp->vdd_datasize;
	vdp->vdd_bsssize	= exec.a_bss;
	vdp->vdd_entry		= (int (*)())exec.a_entry;

	if (vdp->vdd_textsize == 0 || vdp->vdd_datasize == 0)
		return (EINVAL);

	if (exec.a_syms == 0) {
		vdp->vdd_symvaddr	= 0;	/* no symbols */
		vdp->vdd_symsize	= 0;
		vdp->vdd_nsyms		= 0;
		vdp->vdd_symoffset	= 0;
	} else {
		vdp->vdd_symvaddr	= vdp->vdd_bssvaddr + vdp->vdd_bsssize;
		vdp->vdd_symsize	= symtabsize;
		vdp->vdd_nsyms		= exec.a_syms;
		vdp->vdd_symoffset	= sizeof (struct exec) + exec.a_text +
				exec.a_data + exec.a_trsize + exec.a_drsize;
	}
	return (0);
}
#endif /* sun386 */

/*
 * Load the module into memory.
 */

static int
vd_copyinmodule(vdp, mmapaddr, symtabsize)
	struct vddrv *vdp;
	addr_t mmapaddr;
	u_int symtabsize;
{
	register u_int tdsize, tstart, bstart;

#ifdef sun386
	tstart = 0;
#else
	tstart = sizeof (struct exec);
#endif

	tdsize = vdp->vdd_textsize + vdp->vdd_datasize;
	bstart = tstart + tdsize;
	if (bstart + vdp->vdd_bsssize + vdp->vdd_symsize > vdp->vdd_size)
		return (EINVAL);
	/*
	 * copy text and data
	 */
	if (copyin(mmapaddr + tstart, vdp->vdd_textvaddr, tdsize) != 0)
		return (EFAULT);

	/*
	 * zero bss segment
	 */
	bzero(vdp->vdd_bssvaddr, vdp->vdd_bsssize);

	if (vdp->vdd_symsize) {
		/*
		 * Copy symbol table
		 */
		if (copyin(mmapaddr + vdp->vdd_symoffset, vdp->vdd_symvaddr,
			symtabsize) != 0)
			return (EFAULT);
	}
	return (0);		/* success */
}

/*
 * Call a module at its entry point.
 *
 * vd_loadmodule() calls this after loading the module so that the
 * module can do any initialization it needs to do.
 *
 * vd_unloadmodule() calls this before unloading the module so that
 * the module can determine if it's ok to be unloaded and to do any cleanup
 * that it needs to do.
 *
 * vd_stat() calls this so that the module can fill in any type specific
 * information into vds_modinfo.
 */
static int
vd_entry(fcn, vdp, vdi, vds)
	int fcn;
	register struct vddrv *vdp;
	caddr_t vdi;
	struct vdstat *vds;	/* only meaningful for VDSTAT */
{
	if (vdp->vdd_entry == NULL)
		return (EINVAL);
	return (*vdp->vdd_entry)(fcn, vdp, vdi, vds); /* call entry point */
}

/*
 * Return pointer to module ops for this type of module
 */
static struct vd_moduleops
*vd_getops(type)
	int type;
{
	register struct vd_modsw *sw;

	for (sw = vd_modsw; sw->vd_ops != NULL; sw++)
		if (type == sw->vd_magic)
			break;

	return (sw->vd_ops);
}
#endif /* VDDRV */
