/*	@(#)spec_vfsops.c 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/bootconf.h>
#include <specfs/snode.h>
#include <sys/reboot.h>
#include <vm/swap.h>
#include <sun/dklabel.h>

static	int spec_sync();
static	int spec_badop();
static	int spec_root();
static	int spec_mountroot();
static	int spec_swapvp();

struct vfsops spec_vfsops = {
	spec_badop,		/* mount */
	spec_badop,		/* unmount */
	spec_root,
	spec_badop,		/* statfs */
	spec_sync,
	spec_badop,		/* vget */
	spec_mountroot,
	spec_swapvp,
};

static int
spec_badop()
{

	panic("spec_badop");
}

/*
 * Run though all the snodes and force write back
 * of all dirty pages on the block devices.
 */
/*ARGSUSED*/
static int
spec_sync(vfsp)
	struct vfs *vfsp;
{
	static int spec_lock;
	register struct snode **spp, *sp;
	register struct vnode *vp;

	if (spec_lock)
		return (0);

	spec_lock++;
	for (spp = stable; spp < &stable[STABLESIZE]; spp++) {
		for (sp = *spp; sp != (struct snode *)NULL; sp = sp->s_next) {
			vp = STOV(sp);
			/*
			 * Don't bother sync'ing a vp if it
			 * is part of virtual swap device.
			 */
			if (IS_SWAPVP(vp))
				continue;
			if (vp->v_type == VBLK && vp->v_pages)
				(void) VOP_PUTPAGE(vp, 0, 0, B_ASYNC,
				    (struct ucred *)0);
		}
	}
	spec_lock = 0;
	return (0);
}

/*ARGSUSED*/
static int
spec_root(vfsp, vpp, name)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *name;
{

	return (EINVAL);
}

/*ARGSUSED*/
static int
spec_mountroot(vfsp, vpp, name)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *name;
{

	return (EINVAL);
}

/*ARGSUSED*/
static int
spec_swapvp(vfsp, vpp, name)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *name;
{
	extern char *strcpy();
	extern dev_t getblockdev();
	extern struct vnodeops spec_vnodeops;
	char *cp;
	dev_t dev;

	if ((*name == '\0') || (boothowto & RB_ASKNAME)){
		/*
		 * No swap name specified, use root dev partition "b"
		 * if it is a block device, otherwise fail.
		 * XXX - should look through device list or something here
		 * if root is not local.
		 */
		if (rootvp->v_op == &spec_vnodeops &&
		    (boothowto & RB_ASKNAME) == 0) {
			dev = makedev(major(rootvp->v_rdev),
				(minor(rootvp->v_rdev) & ~(NDKMAP - 1)) | 1);
			/*
			 * kernel strcpy (unlike libc strcpy) returns a
			 * pointer to the null byte of the destination string.
			 */
			cp = strcpy(name, rootfs.bo_name);
			*(cp - 1) = 'b';	/* change last char to 'b' */
		} else {
retry:
			if (!(dev = getblockdev("swap", name))) {
				return (ENODEV);
			}
			/*
			 * Check for swap on root device
			 */
			if (rootvp->v_op == &spec_vnodeops &&
			    dev == rootvp->v_rdev) {
				char resp[128];

				printf("Swapping on root device, ok? ");
				gets(resp);
				if (*resp != 'y' && *resp != 'Y') {
					goto retry;
				}
			}
		}
	} else if (!(dev = getblockdev("swap", name))) {
		return (ENODEV);
	}
	*vpp = bdevvp(dev);
	return (0);
}
