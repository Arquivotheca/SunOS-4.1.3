#ifndef lint
static        char sccsid[] = "@(#)bdev_vnodeops.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include "boot/vnode.h"
#include <sys/bootconf.h>
#include <ufs/fs.h>
#include "boot/inode.h"
#include <stand/saio.h>
#include "boot/iob.h"
#include <mon/sunromvec.h>
#include "boot/conf.h"

extern struct bdevlist bdevlist[];

extern    struct  iob     iob[NFILES];

#ifdef	DUMP_DEBUG
static int dump_debug = 10;
#endif	 /* DUMP_DEBUG */


/*
 * Convert a block dev into a vnode pointer suitable for bio.
 */

struct dev_vnode {
	struct vnode dv_vnode;
	struct dev_vnode *dv_link;
} *dev_vnode_headp;

bdev_badop()
{
#ifdef	DUMP_DEBUG
	dprint(dump_debug, 10, "bdev_badop()\n");
#endif	 /* DUMP_DEBUG */

	panic("bdev_badop");
}


/*ARGSUSED*/
int
bdev_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
#ifdef	DUMP_DEBUG
	dprint(dump_debug, 6, "bdev_open(vpp 0x%x flag 0x%x cred 0x%x)\n", 
		vpp, flag, cred);
#endif	 /* DUMP_DEBUG */

	/*
	 * For the moment, all the open operations are being
	 * carried out on bdevvp() below.   Maybe we should
	 * move them up here.
	 */
	return(0);
}


int
bdev_strategy(bp)
	struct buf *bp;
{
	struct vnode *vp;
	int blkno,  size;
	struct iob *fp;
	struct saioreq *sip;

#ifdef	DUMP_DEBUG
	dprint(dump_debug, 6, "bdev_strategy(bp 0x%x)\n", bp);
#endif	 /* DUMP_DEBUG */

	vp = bp->b_vp;
	blkno = bp->b_blkno;
	size = bp->b_bcount;
	fp = (struct iob *)vp->v_data;
	sip = &(fp->i_si);

	/*
	 * Set up the parameters of the read.
	 */
	sip->si_ma = bp->b_un.b_addr;
	sip->si_bn = blkno;
	sip->si_cc = size;
	if (bp->b_flags | B_READ)	{
		if (devread(sip) < 0)
			printf ("bdev_strategy: bad device read\n");
		bp->b_flags |= B_DONE;
	} else {
#ifdef	DUMP_DEBUG
		dprint(dump_debug, 0, "bdev_strategy: sorry, no write\n");
#endif	 /* DUMP_DEBUG */
	}
	return (0);
}


struct vnodeops dev_vnode_ops = {
	bdev_open,
	bdev_badop /* bdev_close */,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop /* bdev_getattr */,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop /* bdev_inactive */,
	bdev_badop,
	bdev_strategy,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	/* bdev_dump, */
};


/*
 * Convert a block device into a special purpose vnode for bio
 */
struct vnode *
bdevvp(dev)
	dev_t dev;
{
	register struct dev_vnode *dvp;
	register struct dev_vnode *endvp;
	register struct boottab *dp;
	int lfdesc;
	register struct iob *lfile;
	int status;

#ifdef	DUMP_DEBUG
	dprint(dump_debug, 6, "bdevvp(dev 0x%x)\n", dev);
#endif	 /* DUMP_DEBUG */

	endvp = (struct dev_vnode *)0;
	for (dvp = dev_vnode_headp; dvp; dvp = dvp->dv_link) {
		if (dvp->dv_vnode.v_rdev == dev) {
			VN_HOLD(&dvp->dv_vnode);
			return (&dvp->dv_vnode);
		}
		endvp = dvp;
	}
	dvp = (struct dev_vnode *)
		kmem_alloc((u_int)sizeof (struct dev_vnode));
	bzero((caddr_t)dvp, sizeof (struct dev_vnode));
	dvp->dv_vnode.v_count = 1;
	dvp->dv_vnode.v_op = &dev_vnode_ops;
	dvp->dv_vnode.v_rdev = dev;
	if (endvp != (struct dev_vnode *)0) {
		endvp->dv_link = dvp;
	} else {
		dev_vnode_headp = dvp;
	}
	/*
	 * In addition to creating the dev-vnode, we create
	 * a file table entry and sip and leave the vnode
	 * pointing at them
	 */
	lfdesc = getiob();
        lfile = &iob[lfdesc];
	/*
	 * Find the devsw table entry for the device, 
	 * and open it.
	 */
	dp = bdevlist[major(dev)].bl_driver;
	lfile->i_boottab = dp;
	lfile->i_ino.i_dev = dev;
	status = opendev(lfdesc, lfile, 0);
	if (status == -1) {
		printf ("bdevvp: bad open\n");
		return ((struct vnode *)-1);
	} else
		/*
		 * this will only happen on v_romvec_version > 0
		 */
		if (status == -2)
			return ((struct vnode *)-1);
	/*
	 * Make the vnode point at the iob.
	 */
	dvp->dv_vnode.v_data = (caddr_t)lfile;
#ifdef	DUMP_DEBUG
	dprint(dump_debug, 6, "bdevvp: returning &dvp->dv_vnode 0x%x\n",
		&dvp->dv_vnode);
#endif	 /* DUMP_DEBUG */
	return (&dvp->dv_vnode);
}


struct vfsops bdev_vfsops = {
	bdev_badop,		/* mount */
	bdev_badop,		/* unmount */
	bdev_badop,		/* root */
	bdev_badop,		/* statfs */
	bdev_badop,		/* sync */
	bdev_badop,		/* vget */
	bdev_badop,		/* mountroot */
	bdev_badop /* bdev_swapvp */,		/* swapvp */
};
