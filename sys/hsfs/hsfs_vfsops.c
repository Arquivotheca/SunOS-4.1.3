#ifndef lint
#ident "@(#)hsfs_vfsops.c 1.1 92/07/30"
#endif

/*LINTLIBRARY*/

/*
 * VFS operations for High Sierra filesystem
 * Copyright (c) 1989 by Sun Microsystem, Inc.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <ufs/inode.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include <hsfs/hsfs.h>
#include <hsfs/hsfs_spec.h>
#include <hsfs/hsfs_isospec.h>
#include <hsfs/hsfs_private.h>
#include <hsfs/hsfs_node.h>


#include <vm/pvn.h>
#include <vm/seg_map.h>
#include <machine/seg_kmem.h>

extern struct vfsops hsfs_vfsops;

struct hstable * hs_inithstbl();
static struct hsfs *hs_mounttab = NULL;


#ifndef BUILD_STATIC

#include <sun/vddrv.h>

/*
 * This is the loadable module wrapper.
 */


struct vdlsys vdldrv = {
	VDMAGIC_USER,
	"high sierra file system"
};



/*
 * Initialization routine for loadable modules.
 */
/*ARGSUSED2*/
hsfsinit(cmd, vdp, vdi, vds)
	int			 cmd;
	struct vddrv		*vdp;
	struct vdioctl_load	*vdi;
	struct vdstat 		*vds;
{
	switch (cmd) {
	case VDLOAD:
		vdp->vdd_vdtab = (struct vdlinkage *)&vdldrv;
		return (hsfs_load());
	case VDUNLOAD:
		return (hsfs_unload());
	case VDSTAT:
		return (0);
	}
	return (0);
}

static struct vfssw *hsfs_vfssw;

static
hsfs_load()
{
	struct vfssw *vs;

	for (vs = vfssw; vs < vfsNVFS; vs++) {
		if (vs->vsw_name == 0) {
			if (hsfs_vfssw == NULL)
				hsfs_vfssw = vs;
		} else {
			if (strcmp("hsfs", vs->vsw_name) == 0) {
				printf("hsfs is already loaded\n");
				return (EBUSY);
			}
		}
	}
	if (hsfs_vfssw == NULL) {
		printf("vfssw is full\n");
		return (ENOMEM);
	}
	hsfs_vfssw->vsw_name = "hsfs";
	hsfs_vfssw->vsw_ops = &hsfs_vfsops;
	return (0);
}

static
hsfs_unload()
{

	if (hsfs_vfssw == NULL) {
		printf("hsfs is not unloadable\n");
		return (EINVAL);
	}

	hsfs_vfssw->vsw_ops = NULL;
	hsfs_vfssw->vsw_name = NULL;
	hsfs_vfssw = 0;
	return (0);
}

#endif	/* BUILD_STATIC */

/*ARGSUSED*/
static int
hsfs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	struct hsfs_args args;
	int error;
	struct vnode *devvp;
	dev_t dev;
	u_short	mode;
	int use_rrip = 1;

	/* mount option must be read only, else mount will be rejected */
	if (vfsp->vfs_flag & VFS_RDONLY);
		else return (EROFS);

	if (error = copyin(data, (caddr_t)&args, sizeof (args))) {
		return (error);
	}

	if (args.norrip == 1)
		use_rrip = 0;

	if ((error = hs_getmdev(args.fspec, &dev, &mode)) != 0)
		return (error);
	/*
	 * make a special (device) vnode for the filesystem
	 */
	devvp = bdevvp(dev);

	/*
	 * If the device is a tape, return error
	 */
	if ((bdevsw[major(dev)].d_flags & B_TAPE) == B_TAPE)
		return (ENOTBLK);

	/*
	 * Mount the filesystem.
	 */
	if (error = hs_mountfs(&devvp, path, vfsp, mode, use_rrip)) {
		VN_RELE(devvp);
	}

	return (error);
}

/*ARGSUSED*/
static int
hsfs_unmount(vfsp)
	struct vfs *vfsp;
{
	struct hsfs **tspp;
	struct hsfs *fsp;

	fsp = VFS_TO_HSFS(vfsp);

	if ((fsp->hsfs_hstbl->hs_refct != 1)||(fsp->hsfs_rootvp->v_count != 1))
		return (EBUSY);

	/* release the root hsnode */
	VN_RELE(fsp->hsfs_rootvp);

	for (tspp = &hs_mounttab; *tspp != NULL; tspp = &(*tspp)->hsfs_next) {
		if (*tspp == fsp)
			break;
	}
	if (*tspp == NULL)
		panic("hsfs_unmount: vfs not mounted?!");

	*tspp = fsp->hsfs_next;

	(void) VOP_CLOSE(fsp->hsfs_devvp,
	    (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE,
	    1, u.u_cred);

	if (fsp->hsfs_devvp->v_pages != NULL) {
		/* invalidate page cache */
		/* VOP_CLOSE(devvp) should flush page cache using binval */
		/* see note on binval */
		(void) pvn_vplist_dirty(fsp->hsfs_devvp, 0, B_INVAL);
	}
	VN_RELE(fsp->hsfs_devvp);
	/* free path table space */
	if (fsp->hsfs_ptbl != NULL)
		kmem_free((caddr_t)fsp->hsfs_ptbl,
			(u_int) fsp->hsfs_vol.ptbl_len);
	/* free path table index table */
	if (fsp->hsfs_ptbl_idx != NULL)
		kmem_free((caddr_t)fsp->hsfs_ptbl_idx, (u_int)
			(fsp->hsfs_ptbl_idx_size * sizeof (struct ptable_idx)));

	/* destroy all old pages for this vfs */
	(void) hs_synchash(vfsp);

	/* free the incore path table */
	(void) hs_freehstbl(vfsp);

	kmem_free((caddr_t)fsp, sizeof (*fsp));
	return (0);
}

/*ARGSUSED*/
static int
hsfs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{

	*vpp = (VFS_TO_HSFS(vfsp))->hsfs_rootvp;
	VN_HOLD(*vpp);
	return (0);
}

/*ARGSUSED*/
static int
hsfs_statfs(vfsp, sbp)
	struct vfs *vfsp;
	struct statfs *sbp;
{
	struct hsfs *fsp;

	fsp = VFS_TO_HSFS(vfsp);
	sbp->f_bsize = vfsp->vfs_bsize;

	/* XXX - need to worry about overflow here ? */
	sbp->f_blocks =
	    (fsp->hsfs_vol.vol_size * fsp->hsfs_vol.lbn_size) / vfsp->vfs_bsize;

	sbp->f_bfree = -1;
	sbp->f_bavail = -1;
	sbp->f_files = -1;
	sbp->f_ffree = -1;
	sbp->f_fsid = vfsp->vfs_fsid;
	return (0);
}

static int
hsfs_vget(vfsp, vpp, fidp)
	struct vfs *vfsp;
	struct vnode **vpp;
	struct fid *fidp;
{
	register struct hsfid *fid;

	fid = (struct hsfid *)fidp;
	/*
	 * Look for vnode on hashlist.
	 * If found, it's now active and the refcnt was incremented.
	 */
	if ((*vpp = hs_findhash(fid->hf_ext_lbn, vfsp)) ==
	    NULL) {
		int error;
		/*
		 * Not on the hash list.  Go read the directory
		 * again and make a new vnode
		 */
		error = hs_remakenode(fid->hf_dir_lbn,
			(u_int) fid->hf_dir_off, vfsp, vpp);
		if (error != 0) return (error);
	}

	return (0);
}

static int
hsfs_badop()
{

	panic("hsfs: unimplemented vfsop called");
}


/* The rest of the VFS routines are only called by the kernel */

#ifdef KERNEL
/*ARGSUSED*/
static int
hsfs_sync()
{

	return (0);
}

/*ARGSUSED*/
static int
hsfs_mountroot(vfsp, vpp, name)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *name;
{

	return (EINVAL);
}

/*ARGSUSED*/
static int
hsfs_swapvp(vfsp, vpp, path)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *path;
{

	return (EINVAL);
}
#else

#define	hsfs_sync	hsfs_badop
#define	hsfs_mountroot	hsfs_badop
#define	hsfs_swapvp	hsfs_badop
#endif /* !KERNEL */

struct vfsops hsfs_vfsops = {
	hsfs_mount,
	hsfs_unmount,
	hsfs_root,
	hsfs_statfs,
	hsfs_sync,
	hsfs_vget,
	hsfs_mountroot,
	hsfs_swapvp,
};

/*ARGSUSED*/
static int
hs_mountfs(vpp, path, vfsp, mode, use_rrip)
	struct vnode **vpp;
	char *path;
	struct vfs *vfsp;
	u_short mode;
	int use_rrip;
{
	struct hsfs *tsp;
	struct hsfs *fsp;
	struct vattr vap;
	struct hsnode *hp;
	int error;
	struct timeval tv;
	long fsid;

	/*
	 * Check to see if this vnode is already mounted.
	 *
	 * XXX - ought to be able to check a system-wide mount list.
	 */
	for (tsp = hs_mounttab; tsp != NULL; tsp = tsp->hsfs_next) {
		if (VN_CMP(tsp->hsfs_devvp, *vpp)) {
			if (vfsp->vfs_flag & VFS_REMOUNT) {
				vfsp->vfs_flag &= ~VFS_REMOUNT;
				VN_RELE(*vpp);
				return (0);
			} else {
				return (EBUSY);
			}
		}
	}
	/*
	 * Open the target device (file).
	 */
	if (error = VOP_OPEN(vpp,
	    (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE, u.u_cred))
		return (error);

	/*
	 * Init a new hsfs structure.
	 */
	fsp = (struct hsfs *)new_kmem_alloc(sizeof (*fsp), KMEM_SLEEP);
	bzero((caddr_t)fsp, sizeof (*fsp));

	if ((error = VOP_GETATTR(*vpp, &vap, u.u_cred)) != 0)  {
		uprintf("Canot get attributes of the CD-ROM driver\n");
		goto cleanup;
	}

	/* save volume protection field for CD-ROM without UNIX extension */
	/* mode information is obtained from CD-ROM device */
	/* do it here because parsevols depends on these fields */
	fsp->hsfs_vol.vol_uid = vap.va_uid;
	fsp->hsfs_vol.vol_gid = vap.va_gid;
	/* VOP_GETATTR does not return va_mode for specfs */
	/* this is the reason mode is obtained from getmdev() */
	fsp->hsfs_vol.vol_prot = mode & 0777;

	/*
	 * Look for a Standard File Structure Volume Descriptor,
	 * of which there must be at least one.
	 * If found, check for volume size consistency.
	 *
	 * XXX - va_size may someday not be large enough to do this correctly.
	 */
	error = hs_findhsvol(fsp, *vpp, &fsp->hsfs_vol);
	if (error == EINVAL) {
		/* not in hs format, try iso 9660 format */
		error = hs_findisovol(fsp, *vpp, &fsp->hsfs_vol);
		if (error == EINVAL)
			uprintf("hsfs: Unknown CD-ROM structure format\n");
	}

	if (error) goto cleanup;

	/*
	 * We are now committed to the new filesystem.
	 *
	 * Find a unique filesystem id.
	 * Use creation date, but be prepared in case it is not unique
	 * (e.g., creation date could be 0 or -1 if conversion error).
	 *
	 * If creation date is used, this gives some chance of integrity
	 * across disk change.  That is, if a client has an fhandle, it
	 * will be valid as long as the same disk is mounted.
	 *
	 * If all else fails, use a unique value which will guarantee
	 * that client fhandles go stale if the server crashes.
	 */
	fsid = fsp->hsfs_vol.cre_date.tv_sec;
	for (tsp = hs_mounttab; tsp != NULL; tsp = tsp->hsfs_next) {
		if ((fsid == tsp->hsfs_vfs->vfs_fsid.val[0]) ||
		    (fsid == 0) || (fsid == -1)) {
			uniqtime(&tv);
			fsid = tv.tv_sec;
			break;
		}
	}

	fsp->hsfs_next = hs_mounttab;
	hs_mounttab = fsp;
	fsp->hsfs_devvp = *vpp;
	fsp->hsfs_vfs = vfsp;
	fsp->ext_impl_bits = 0L;

	vfsp->vfs_data = (caddr_t)fsp;
	vfsp->vfs_bsize = fsp->hsfs_vol.lbn_size;
	vfsp->vfs_fsid.val[0] = fsid;
	vfsp->vfs_fsid.val[1] =  11; /* MOUNT_HS; */

	/* initialize incore hsnode table */
	fsp->hsfs_hstbl = hs_inithstbl(vfsp);

/* code added to fix DMA disc problem */
#if 0
	fsp->hsfs_rootvp = hs_makenode(&fsp->hsfs_vol.root_dir,
	    fsp->hsfs_vol.root_dir.ext_lbn, 0, vfsp);
#else
	error = hs_remakenode(fsp->hsfs_vol.root_dir.ext_lbn, (u_int) 0,
				vfsp, &fsp->hsfs_rootvp);
	if (error) {
		printf("hsfs_mount: remakenode error\n");
		(void)  hs_freehstbl(vfsp);
		goto cleanup;
	}
#endif
	/* mark vnode as VROOT */
	fsp->hsfs_rootvp->v_flag |= VROOT;

	/* XXX - ignore the path table for now */
	fsp->hsfs_ptbl = NULL;
	hp = VTOH(fsp->hsfs_rootvp);
	hp->hs_ptbl_idx = NULL;

	if (use_rrip)
		hs_check_root_dirent(fsp->hsfs_rootvp, &(hp->hs_dirent));



	return (0);

cleanup:
	(void) VOP_CLOSE(*vpp, (vfsp->vfs_flag & VFS_RDONLY) ?
		FREAD : FREAD|FWRITE, 1, u.u_cred);
	if ((*vpp)->v_pages != NULL) {
		/*
		 * Invalidate page cache. VOP_CLOSE(devvp) should have
		 * flushed page cache using binval. See note on binval
		 */
		(void) pvn_vplist_dirty(*vpp, 0, B_INVAL);
	}
	kmem_free((caddr_t)fsp, sizeof (*fsp));
	return (error);
}

/*
 * hs_findhsvol()
 *
 * Locate the Standard File Structure Volume Descriptor and
 * parse it into an hs_volume structure.
 *
 * XXX - May someday want to look for Coded Character Set FSVD, too.
 */
static int
hs_findhsvol(fsp, vp, hvp)
	struct hsfs *fsp;
	struct vnode *vp;
	struct hs_volume *hvp;
{
	register int i;
	u_char *volp;
	int error;
	u_int secno;
	u_int buf[HS_SECTOR_SIZE/4];

	secno = HS_VOLDESC_SEC;		/* 1st sector of volume descriptors */
	volp = (u_char *) &buf[0];
	if (error = hs_readsector(vp, secno, volp))
		return (error);
	while (HSV_DESC_TYPE(volp) != VD_EOV) {
		for (i = 0; i < HSV_ID_STRLEN; i++)
			if (HSV_STD_ID(volp)[i] != HSV_ID_STRING[i])
				goto cantfind;
		if (HSV_STD_VER(volp) != HSV_ID_VER)
			goto cantfind;
		switch (HSV_DESC_TYPE(volp)) {
		case VD_SFS:
			/* Standard File Structure */
			fsp->hsfs_vol_type = HS_VOL_TYPE_HS;
			error = hs_parsehsvol(fsp, volp, hvp);
			return (error);

		case VD_CCFS:
			/* Coded Character File Structure */
		case VD_BOOT:
		case VD_UNSPEC:
			break;
		}
		if (error = hs_readsector(vp, ++secno, volp))
			return (error);
	}
cantfind:
	return (EINVAL);
}
/*
 * hs_parsehsvol
 *
 * Parse the Standard File Structure Volume Descriptor into
 * an hs_volume structure.  We can't just bcopy it into the
 * structure because of byte-ordering problems.
 *
 */
static int
hs_parsehsvol(fsp, volp, hvp)
	struct hsfs *fsp;
	u_char *volp;
	struct hs_volume *hvp;
{

	hvp->vol_size = HSV_VOL_SIZE(volp);
	hvp->lbn_size = HSV_BLK_SIZE(volp);
	hvp->lbn_shift = ffs((long)hvp->lbn_size) - 1;
	hvp->lbn_secshift =
		ffs((long)howmany(HS_SECTOR_SIZE, (int)hvp->lbn_size)) - 1;
	(void) hs_parse_longdate(HS_VOL_TYPE_HS,
		HSV_cre_date(volp), &hvp->cre_date);
	(void) hs_parse_longdate(HS_VOL_TYPE_HS,
		HSV_mod_date(volp), &hvp->mod_date);
	hvp->file_struct_ver = HSV_FILE_STRUCT_VER(volp);
	hvp->ptbl_len = HSV_PTBL_SIZE(volp);
	hvp->vol_set_size = (u_short) HSV_SET_SIZE(volp);
	hvp->vol_set_seq = (u_short) HSV_SET_SEQ(volp);
#ifdef vax || i386
	hvp->ptbl_lbn = HSV_PTBL_MAN_LS(volp);
#else
	hvp->ptbl_lbn = HSV_PTBL_MAN_MS(volp);
#endif

	/*
	 * Make sure that lbn_size is a power of two and otherwise valid.
	 */
	if (hvp->lbn_size & ~(1 << hvp->lbn_shift)) {
		uprintf("hsfs: %d-byte logical block size not supported\n",
		    hvp->lbn_size);
		return (EINVAL);
	}
	return (hs_parsedir(fsp, HSV_ROOT_DIR(volp), &hvp->root_dir,
			(char *) NULL, (int *)NULL));
}

/*
 * hs_findisovol()
 *
 * Locate the Primary Volume Descriptor
 * parse it into an hs_volume structure.
 *
 * XXX - Supplementary, Partition not yet done
 */
static int
hs_findisovol(fsp, vp, hvp)
	struct hsfs *fsp;
	struct vnode *vp;
	struct hs_volume *hvp;
{
	register int i;
	u_char *volp;
	int error;
	u_int secno;
	int found = 0;

	u_int buf[ISO_SECTOR_SIZE/4];

	secno = ISO_VOLDESC_SEC;	/* 1st sector of volume descriptors */
	volp = (u_char *) &buf[0];
	if (error = hs_readsector(vp, secno, volp))
		return (error);
	while ((enum iso_voldesc_type) ISO_DESC_TYPE(volp) != ISO_VD_EOV) {
		for (i = 0; i < ISO_ID_STRLEN; i++)
			if (ISO_STD_ID(volp)[i] != ISO_ID_STRING[i])
				goto cantfind;
		if (ISO_STD_VER(volp) != ISO_ID_VER)
			goto cantfind;
		switch (ISO_DESC_TYPE(volp)) {
		case ISO_VD_PVD:
			/* Standard File Structure */
			fsp->hsfs_vol_type = HS_VOL_TYPE_ISO;
			if (error = hs_parseisovol(fsp, volp, hvp))
				return (error);
			found = 1;
			break;
		case ISO_VD_SVD:
			/* Supplementary Volume Descriptor */
			break;
		case ISO_VD_BOOT:
			break;
		case ISO_VD_VPD:
			/* currently cannot handle partition */
			break;
		}
		if (error = hs_readsector(vp, ++secno, volp))
			return (error);
	}
	if (found) return (0);
cantfind:
	return (EINVAL);
}
/*
 * hs_parseisovol
 *
 * Parse the Primary Volume Descriptor into an hs_volume structure.
 *
 */
static int
hs_parseisovol(fsp, volp, hvp)
	struct hsfs *fsp;
	u_char *volp;
	struct hs_volume *hvp;
{

	hvp->vol_size = ISO_VOL_SIZE(volp);
	hvp->lbn_size = ISO_BLK_SIZE(volp);
	hvp->lbn_shift = ffs((long)hvp->lbn_size) - 1;
	hvp->lbn_secshift =
		ffs((long)howmany(ISO_SECTOR_SIZE, (int)hvp->lbn_size)) - 1;
	(void) hs_parse_longdate(HS_VOL_TYPE_ISO, ISO_cre_date(volp),
			&hvp->cre_date);
	(void) hs_parse_longdate(HS_VOL_TYPE_ISO, ISO_mod_date(volp),
			&hvp->mod_date);
	hvp->file_struct_ver = ISO_FILE_STRUCT_VER(volp);
	hvp->ptbl_len = ISO_PTBL_SIZE(volp);
	hvp->vol_set_size = (u_short) ISO_SET_SIZE(volp);
	hvp->vol_set_seq = (u_short) ISO_SET_SEQ(volp);
#ifdef vax || i386
	hvp->ptbl_lbn = ISO_PTBL_MAN_LS(volp);
#else
	hvp->ptbl_lbn = ISO_PTBL_MAN_MS(volp);
#endif

	/*
	 * Make sure that lbn_size is a power of two and otherwise valid.
	 */
	if (hvp->lbn_size & ~(1 << hvp->lbn_shift)) {
		uprintf("hsfs: %d-byte logical block size not supported\n",
		    hvp->lbn_size);
		return (EINVAL);
	}
	return (hs_parsedir(fsp, ISO_ROOT_DIR(volp), &hvp->root_dir,
			(char *) NULL, (int *)NULL));
}

/*
 * Common code for mount and umount.
 * Check that the user's argument is a reasonable
 * thing on which to mount, and return the device number if so.
 */
int
hs_getmdev(fspec, pdev, mode)
	char *fspec;
	dev_t *pdev;
	u_short *mode;
{
	register int error;
	struct vnode *vp;
	struct vattr vap;

	/*
	 * Get the device to be mounted
	 */
	error = lookupname(fspec, UIO_USERSPACE, FOLLOW_LINK,
	    (struct vnode **)0, &vp);
	if (error) {
		if (u.u_error == ENOENT) {
			printf("Cannot locate device\n");
			return (ENODEV);	/* needs translation */
		}
		return (error);
	}
	if (vp->v_type != VBLK) {
		VN_RELE(vp);
		return (ENOTBLK);
	}
	/* get protection mode */
	(void) VOP_GETATTR(vp, &vap, u.u_cred);
	*mode = vap.va_mode;

	*pdev = vp->v_rdev;
	VN_RELE(vp);
	if (major(*pdev) >= nblkdev)
		return (ENXIO);
	return (0);
}
