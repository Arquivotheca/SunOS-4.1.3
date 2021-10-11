#ident  "@(#)spec_vnodeops.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/mman.h>
#include <sys/debug.h>
#include <sys/unistd.h>
#include <sys/termios.h>
#include <sys/vmmeter.h>
#include <specfs/snode.h>

#include <krpc/lockmgr.h>

#include <vm/hat.h>
#include <vm/page.h>
#include <vm/as.h>
#include <vm/pvn.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_dev.h>
#include <vm/seg_vn.h>
#include <vm/swap.h>

static	int spec_open();
static	int spec_close();
static	int spec_rdwr();
static	int spec_ioctl();
static	int spec_select();
static	int spec_getattr();
static	int spec_inactive();
static	int spec_noop();
static	int spec_getpage();
static	int spec_putpage();
static	int spec_map();
static	int spec_dump();
static  int spec_cmp();
/*
 * Used directly in fifo_vnodeops
 */
int spec_setattr();
int spec_access();
int spec_link();
int spec_lockctl();
int spec_fsync();
int spec_fid();
int spec_realvp();
int spec_cntl();

struct vnodeops spec_vnodeops = {
	spec_open,
	spec_close,
	spec_rdwr,
	spec_ioctl,
	spec_select,
	spec_getattr,
	spec_setattr,
	spec_access,
	spec_noop,	/* lookup */
	spec_noop,	/* create */
	spec_noop,	/* remove */
	spec_link,
	spec_noop,	/* rename */
	spec_noop,	/* mkdir */
	spec_noop,	/* rmdir */
	spec_noop,	/* readdir */
	spec_noop,	/* symlink */
	spec_noop,	/* readlink */
	spec_fsync,
	spec_inactive,
	spec_lockctl,
	spec_fid,
	spec_getpage,
	spec_putpage,
	spec_map,
	spec_dump,
	spec_cmp,
	spec_realvp,
	spec_cntl,
};

/*
 * open a special file (device)
 * Some weird stuff here having to do with clone and indirect devices:
 * When a file lookup operation happens (e.g. ufs_lookup) and the vnode has
 * type VDEV specvp() is used to return a spec vnode instead.  Then when
 * the VOP_OPEN routine is called, we get control here.  When we do the
 * device open routine there are several possible strange results:
 * 1) An indirect device will return the error EAGAIN on open and return
 *    a new dev number.  We have to make that into a spec vnode and call
 *    open on it again.
 * 2) The clone device driver will return the error EEXIST and return a
 *    new dev number.  As above, we build a new vnode and call open again,
 *    explicitly asking the open routine to do a clone open.
 * 3) A clone device will return a new dev number on open but no error.
 *    In this case we just make a new spec vnode out of the new dev number
 *    and return that.
 * The last two cases differ in that the decision to clone arises outside
 * of the target device in 2) and from within in 3).
 *
 * TODO: extend case 2) to apply to all character devices, not just streams
 * devices.
 */
#define	MAX_S_SIZE \
		((1 << sizeof (off_t) * NBBY - DEV_BSHIFT - 1) - 1)
/*ARGSUSED*/
static int
spec_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	register struct snode *sp;
	dev_t dev;
	dev_t newdev;
	int sflag = 0;
	register int error;

	sp = VTOS(*vpp);

	/*
	 * Do open protocol for special type.
	 */
	dev = sp->s_dev;

	switch ((*vpp)->v_type) {

	case VCHR:
		newdev = dev;
		error = 0;
		for (;;) {
			register struct vnode *nvp;

			dev = newdev;
			if ((u_int)major(dev) >= nchrdev)
				return (ENXIO);

			while (isclosing(dev, (*vpp)->v_type))
				(void) sleep((caddr_t)sp, PSLEP);

			if (cdevsw[major(dev)].d_str) {
				/*
				 * Open the stream.  Stropen handles
				 * the mechanics of cloning itself.
				 * In particular, it builds a fresh
				 * vnode for the cloned instance and
				 * does streams-specific cross-linking.
				 */
				error = stropen(vpp, flag, sflag);
				sp = VTOS(*vpp);
				break;
			} else
				error = (*cdevsw[major(dev)].d_open)(dev,
					flag, &newdev);

			/*
			 * If this is an indirect device or a forced clone,
			 * we need to do the open again.  In both cases,
			 * we insist that newdev differ from dev, to help
			 * avoid infinite regress.
			 */
			if (newdev == dev ||
			    (error != 0 && error != EAGAIN && error != EEXIST))
				break;

			/*
			 * Allocate new snode with new device.  Release old
			 * snode. Set vpp to point to new one.  This snode will
			 * go away when the last reference to it goes away.
			 * Warning: if you stat this, and try to match it with
			 * a name in the filesystem you will fail, unless you
			 * had previously put names in that match.
			 */
			nvp = makespecvp(newdev, VCHR);
			sp = VTOS(nvp);
			VN_RELE(*vpp);
			*vpp = nvp;

			/* If we've completed a clone open, we're done. */
			if (error == 0)
				break;
			else
				sflag = error == EEXIST ? CLONEOPEN : 0;
		}
		break;

	case VFIFO:
		printf("spec_open: got a VFIFO???\n");
		/* fall through to... */

	case VSOCK:
		error = EOPNOTSUPP;
		break;

	case VBLK:
		/*
		 * The block device sizing was already done in specvp().
		 * However, we still need to verify that we can open the
		 * block device here (since specvp was called as part of a
		 * "lookup", not an "open", and e.g. "stat"ing a block special
		 * file with an illegal major device number should be legal).
		 *
		 * With loadable drivers, removable media devices, or
		 * metadevices, the block device sizing might need to be
		 * done again, as the open will likely find a changed size
		 * to the device.
		 *
		 * If the special file for a device is opened before the
		 * driver is loaded, or a lookup on /dev is done, there
		 * might be an snode around with s_size == 0. In this case,
		 * we need to resize the device.
		 *
		 * Another way of putting it is that the XXsize function
		 * reports the current size, if any, of a device, and doesn't
		 * imply any other action that the driver will take, while
		 * the open might imply sizing.
		 */

		if ((u_int)major(dev) >= nblkdev)
			error = ENXIO;
		else
			error = (*bdevsw[major(dev)].d_open)(dev, flag);
		if (error == 0) {
			struct snode *sptmp;
			int (*size)() = bdevsw[major(dev)].d_psize;

			sptmp = VTOS(bdevvp(dev));
			if (size != NULL) {
				int rsize = (*size)(dev);
				if (rsize == -1)
					sptmp->s_size = 0;
				else
					sptmp->s_size =
					    dbtob(MIN(rsize, MAX_S_SIZE));
			}
			VN_RELE(STOV(sptmp));
		}
		break;
	default:
		panic("spec_open: type not VCHR or VBLK");
		break;
	}
	if (error == 0)
		sp->s_count++;		/* one more open reference */
	return (error);
}

/*ARGSUSED*/
static int
spec_close(vp, flag, count, cred)
	struct vnode *vp;
	int flag;
	int count;
	struct ucred *cred;
{
	register struct snode *sp;
	dev_t dev;

	if (count > 1)
		return (0);

	/*
	 * setjmp in case close is interrupted
	 */
	if (setjmp(&u.u_qsave)) {
		sp = VTOS(vp);	/* recompute - I don't trust setjmp/longjmp */
		sp->s_flag &= ~SCLOSING;
		wakeup((caddr_t)sp);
		return (EINTR);
	}

	sp = VTOS(vp);
	sp->s_count--;		/* one fewer open reference */

	/*
	 * Only call the close routine when the last open
	 * reference through any [s, v]node goes away.
	 */
	if (stillopen(sp->s_dev, vp->v_type))
		return (0);

	dev = sp->s_dev;

	switch (vp->v_type) {

	case VCHR:
		/*
		 * Mark this device as closing, so that opens will wait until
		 * the close finishes.  Since the close may block, this
		 * prevents an open from getting in while the close is blocked,
		 * and then getting surprised when the close finishes and
		 * potentially clears out the driver's state.
		 *
		 * XXX - really should be done on all devices, but for now we
		 * only do it on streams (as that's the one case where the
		 * close blocks before the close routine is called, and thus
		 * the one case where the close routine really can't protect
		 * itself).
		 */
		/*
		 * If it's a stream, call stream close routine.
		 */
		if (cdevsw[major(dev)].d_str) {
			sp->s_flag |= SCLOSING;
			strclose(vp, flag);
			sp->s_flag &= ~SCLOSING;
			wakeup((caddr_t)sp);
		} else
			(void) (*cdevsw[major(dev)].d_close)(dev, flag);
		break;

	case VBLK:
		/*
		 * On last close of a block device, we flush back
		 * and invalidate any in core buffers to help make
		 * the spec vnode inactive ASAP if it is not currently
		 * held by someone else for something (e.g., swapping).
		 */
		bflush(sp->s_bdevvp);
		binval(sp->s_bdevvp);
		(void) (*bdevsw[major(dev)].d_close)(dev, flag);
		break;

	case VFIFO:
		printf("spec_close: got a VFIFO???\n");
		break;
	}

	return (0);
}

/*
 * read or write a spec vnode
 */
/*ARGSUSED*/
static int
spec_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	register struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	register struct snode *sp;
	register addr_t base;
	register u_int off;
	struct vnode *blkvp;
	dev_t dev;
	register int n, on;
	u_int flags;
	u_int bdevsize;
	int pagecreate;
	int error;
	extern int mem_no;

	sp = VTOS(vp);
	dev = (dev_t)sp->s_dev;
	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("spec_rdwr");
	if (rw == UIO_READ && uiop->uio_resid == 0)
		return (0);
	n = uiop->uio_resid;
	/*
	 * If this I/O will carry us over the 2GB threshold,
	 * switch automatically to block mode if possible.
	 *
	 * XXX We switch if the I/O leaves us exactly at 2GB,
	 * which is arguably wrong, but the old code didn't
	 * allow such I/O's anyway, so there is no compatibility
	 * problem.
	 */
	if (vp->v_type == VCHR && (uiop->uio_fmode & FSETBLK) == 0 &&
	    mem_no != major(dev) && vp->v_stream == NULL &&
	    uiop->uio_offset >= 0 && uiop->uio_offset + n < 0 &&
	    uiop->uio_offset % DEV_BSIZE == 0) {
		uiop->uio_fmode |= FSETBLK;
		uiop->uio_offset = btodb(uiop->uio_offset);
	}
	if (uiop->uio_fmode & FSETBLK) {
		if (n % DEV_BSIZE != 0)
			return (EINVAL);
		n = btodb(n);
	}
	if ((uiop->uio_offset < 0 ||
	    (n != 0 && uiop->uio_offset + n - 1 < 0)) &&
	    !(vp->v_type == VCHR &&
	    (mem_no == major(dev) || vp->v_stream != NULL))) {
		return (EINVAL);
	}

	if (rw == UIO_READ)
		smark(sp, SACC);

	if (vp->v_type == VCHR) {
		if (rw == UIO_READ) {
			if (cdevsw[major(dev)].d_str) {
				int saverr = u.u_error;

				u.u_error = 0;
				strread(vp, uiop);
				error = u.u_error;
				u.u_error = saverr;
			} else
				error = (*cdevsw[major(dev)].d_read)(dev, uiop);
		} else {
			smark(sp, SUPD|SCHG);
			if (cdevsw[major(dev)].d_str) {
				int saverr = u.u_error;

				u.u_error = 0;
				strwrite(vp, uiop);
				error = u.u_error;
				u.u_error = saverr;
			} else
				error = (*cdevsw[major(dev)].d_write)(dev,
						uiop);
		}
		return (error);
	}

	if (vp->v_type != VBLK)
		return (EOPNOTSUPP);

	if (uiop->uio_resid == 0)
		return (0);

	error = 0;
	blkvp = sp->s_bdevvp;
	bdevsize = sp->s_size;
	do {
		int diff;

		off = uiop->uio_offset & MAXBMASK;
		on = uiop->uio_offset & MAXBOFFSET;
		n = MIN(MAXBSIZE - on, uiop->uio_resid);
		pagecreate = 0;
		diff = bdevsize - uiop->uio_offset;

		if (diff <= 0)
			break;
		if (diff < n)
			n = diff;

		base = segmap_getmap(segkmap, blkvp, off);

		/*
		 * Check to see if we can skip reading in the page
		 * and just allocate the memory.  We can do this
		 * if we are going to rewrite the entire mapping
		 * or if we are going to write to end of the device
		 * from the beginning of the mapping.
		 */
		if (rw == UIO_WRITE && (n == MAXBSIZE ||
		    (on == 0 && (off + n) == bdevsize))) {
			SNLOCK(sp);
			segmap_pagecreate(segkmap, base + on, (u_int)n, 0);
			SNUNLOCK(sp);
			pagecreate = 1;
		}

		error = uiomove(base + on, n, rw, uiop);

		if (pagecreate && uiop->uio_offset <
		    roundup(off + on + n, PAGESIZE)) {
			/*
			 * We created pages w/o initializing them completely,
			 * thus we need to zero the part that wasn't set up.
			 * This can happen if we write to the end of the device
			 * or if we had some sort of error during the uiomove.
			 */
			int nzero, nmoved;

			nmoved = uiop->uio_offset - (off + on);
			ASSERT(nmoved >= 0 && nmoved <= n);
			nzero = roundup(on + n, PAGESIZE) - nmoved;
			ASSERT(nzero > 0 && on + nmoved + nzero <= MAXBSIZE);
			(void) kzero(base + on + nmoved, (u_int)nzero);
		}

		if (error == 0) {
			flags = 0;
			if (rw == UIO_WRITE) {
				/*
				 * Force write back for synchronous write cases.
				 */
				if (ioflag & IO_SYNC) {
					flags = SM_WRITE;
				} else if (n + on == MAXBSIZE ||
				    IS_SWAPVP(vp)) {
					/*
					 * Have written a whole block.
					 * Start an asynchronous write and
					 * mark the buffer to indicate that
					 * it won't be needed again soon.
					 * Push swap files here, since it
					 * won't happen anywhere else.
					 */
					flags = SM_WRITE | SM_ASYNC |
					    SM_DONTNEED;
				}
				smark(sp, SUPD|SCHG);
			} else if (rw == UIO_READ) {
				/*
				 * If read a whole block, won't need this
				 * buffer again soon.  Don't mark it with
				 * SM_FREE, as that can lead to a deadlock
				 * if the block corresponds to a u-page.
				 * (The keep count never drops to zero, so
				 * waiting for "i/o to complete" never
				 * terminates; this points out a flaw in
				 * our locking strategy.)
				 */
				if (n + on == MAXBSIZE)
					flags = SM_DONTNEED;
			}
			error = segmap_release(segkmap, base, flags);
		} else {
			(void) segmap_release(segkmap, base, 0);
		}

	} while (error == 0 && uiop->uio_resid > 0 && n != 0);

	return (error);
}

/*ARGSUSED*/
static int
spec_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
	register struct snode *sp;

	sp = VTOS(vp);
	if (vp->v_type != VCHR)
		panic("spec_ioctl");
	if (cdevsw[major(sp->s_dev)].d_str) {
		int saverr = u.u_error;
		int error;

		u.u_error = 0;
		strioctl(vp, com, data, flag);
		error = u.u_error;
		u.u_error = saverr;
		return (error);
	}
	return ((*cdevsw[major(sp->s_dev)].d_ioctl)
	    (sp->s_dev, com, data, flag));
}

/*ARGSUSED*/
static int
spec_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{
	register struct snode *sp;

	sp = VTOS(vp);
	if (vp->v_type != VCHR)
		panic("spec_select");
	if (cdevsw[major(sp->s_dev)].d_str)
		return (strselect(vp, which));
	else
		return ((*cdevsw[major(sp->s_dev)].d_select)(sp->s_dev, which));
}

static int
spec_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	struct snode *sp;
	int error;

	sp = VTOS(vp);
	/* must sunsave() first to prevent a race when spec_fsync() sleeps */
	sunsave(sp);

	if (sp->s_realvp && (sp->s_bdevvp == NULL || !IS_SWAPVP(sp->s_bdevvp)))
		(void) spec_fsync(vp, cred);
	if (vp->v_type == VBLK && vp->v_pages != NULL) {
		/*
		 * Device is no longer referenced by anyone.
		 * Destroy all the old pages (which BTW don't
		 * count against the vnode reference count) so
		 * we can, for instance, change floppy disks.
		 */
		error = spec_putpage(sp->s_bdevvp, 0, 0, B_INVAL,
		    (struct ucred *)0);
	} else {
		error = 0;
	}

	/* now free the realvp (no longer done by sunsave()) */
	if (sp->s_realvp) {
		VN_RELE(sp->s_realvp);
		sp->s_realvp = NULL;
		if (sp->s_bdevvp)
			VN_RELE(sp->s_bdevvp);
	}

	kmem_free((caddr_t)sp, sizeof (*sp));
	return (error);
}

static int
spec_getattr(vp, vap, cred)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	int error;
	register struct snode *sp;
	register struct vnode *realvp;

	sp = VTOS(vp);
	if ((realvp = sp->s_realvp) == NULL) {
		/*
		 * No real vnode behind this one.
		 * Set the device size from snode.
		 * Set times to the present.
		 * Set blocksize based on type in the unreal vnode.
		 */
		bzero((caddr_t)vap, sizeof (*vap));
		vap->va_size = sp->s_size;
		vap->va_rdev = sp->s_dev;
		vap->va_type = vp->v_type;
		vap->va_nodeid = ++fake_vno;
	} else {
		extern int dump_no;
		error = VOP_GETATTR(realvp, vap, cred);
		if (error != 0)
			return (error);
		/* if this is the dump file, copy the size, too */
		/* XXX there should be a more general way of doing this */
		if (vp->v_type == VCHR && dump_no == major(sp->s_dev))
			vap->va_size = sp->s_size;
	}
	/* set current times from snode, even if older than vnode */
	vap->va_atime = sp->s_atime;
	vap->va_mtime = sp->s_mtime;
	vap->va_ctime = sp->s_ctime;

	/* set device-dependent blocksizes */
	switch (vap->va_type) {
	case VBLK:
		vap->va_blocksize = MAXBSIZE;		/* was BLKDEV_IOSIZE */
		break;

	case VCHR:
		vap->va_blocksize = MAXBSIZE;
		break;
	}
	return (0);
}

int
spec_setattr(vp, vap, cred)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	register struct snode *sp;
	register struct vnode *realvp;
	int error;
	register int chtime = 0;

	sp = VTOS(vp);
	if ((realvp = sp->s_realvp) == NULL)
		error = 0;			/* no real vnode to update */
	else
		error = VOP_SETATTR(realvp, vap, cred);
	if (error == 0) {
		/* if times were changed, update snode */
		if (vap->va_mtime.tv_sec != -1) {

			/*
			 * If SysV-compatible option to set access and
			 * modified times if root, owner, or write access,
			 * need to read back the new times in order to
			 * keep the snode times in sync.  If VOP_GETATTR()
			 * fails, use current client time as an approximation.
			 *
			 * XXX - va_mtime.tv_usec == -1 flags this.
			 */
			if (vap->va_mtime.tv_usec == -1) {
				struct vattr vtmp;

				if ((realvp == NULL) ||
				    VOP_GETATTR(realvp, &vtmp, cred) != 0) {
					/* if error, simulate server time */
					sp->s_mtime = time;
					sp->s_atime = time;
					sp->s_ctime = time;
				} else {
					sp->s_mtime = vtmp.va_mtime;
					sp->s_atime = vtmp.va_atime;
					sp->s_ctime = vtmp.va_ctime;
				}
				goto no_chtime;
			}

			sp->s_mtime = vap->va_mtime;
			chtime++;
		}
		if (vap->va_atime.tv_sec != -1) {
			sp->s_atime = vap->va_atime;
			chtime++;
		}
		if (chtime)
			sp->s_ctime = time;
	}
no_chtime:
	return (error);
}

int
spec_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	register struct vnode *realvp;

	if ((realvp = VTOS(vp)->s_realvp) != NULL)
		return (VOP_ACCESS(realvp, mode, cred));
	else
		return (0);	/* allow all access */
}

int
spec_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{
	register struct vnode *realvp;

	if ((realvp = VTOS(vp)->s_realvp) != NULL)
		return (VOP_LINK(realvp, tdvp, tnm, cred));
	else
		return (ENOENT); /* can't link to something non-existent */
}

/*
 * In order to sync out the snode times without multi-client problems,
 * make sure the times written out are never earlier than the times
 * already set in the vnode.
 */
int
spec_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	register int error = 0;
	register struct snode *sp;
	register struct vnode *realvp;
	struct vattr *vap;
	struct vattr *vatmp;
	int err;

	sp = VTOS(vp);
	/*
	 * If times didn't change on a non-block
	 * special file, don't flush anything.
	 */
	if ((sp->s_flag & (SACC|SUPD|SCHG)) == 0 && vp->v_type != VBLK)
		return (0);
	sp->s_flag &= ~(SACC|SUPD|SCHG);

	/*
	 * If the vnode represents a block device and it is a "shadow"
	 * vnode, then flush all pages associated with the "common" vnode.
	 */
	if (vp->v_type == VBLK && sp->s_bdevvp != vp &&
	    sp->s_bdevvp->v_pages != NULL)
		error = spec_putpage(sp->s_bdevvp, 0, 0, 0,
		    (struct ucred *)0);

	/*
	 * If no real vnode to update, don't flush anything
	 */
	if ((realvp = sp->s_realvp) == NULL)
		return (error);

	vatmp = (struct vattr *)new_kmem_alloc(sizeof (*vatmp), KMEM_SLEEP);
	err = VOP_GETATTR(realvp, vatmp, cred);
	if (err == 0) {
		vap = (struct vattr *)new_kmem_alloc(sizeof (*vap), KMEM_SLEEP);
		vattr_null(vap);
		vap->va_atime = timercmp(&vatmp->va_atime, &sp->s_atime, >) ?
		    vatmp->va_atime : sp->s_atime;
		vap->va_mtime = timercmp(&vatmp->va_mtime, &sp->s_mtime, >) ?
		    vatmp->va_mtime : sp->s_mtime;
		VOP_SETATTR(realvp, vap, cred);
		kmem_free((caddr_t)vap, sizeof (*vap));
	}
	kmem_free((caddr_t)vatmp, sizeof (*vatmp));
	(void) VOP_FSYNC(realvp, cred);
	return (error);
}

static int
spec_dump(vp, addr, bn, count)
	struct vnode *vp;
	caddr_t addr;
	int bn;
	int count;
{

	return ((*bdevsw[major(vp->v_rdev)].d_dump)
	    (vp->v_rdev, addr, bn, count));
}

static int
spec_noop()
{

	return (EINVAL);
}

/*
 * Record-locking requests are passed back to the real vnode handler.
 */
int
spec_lockctl(vp, ld, cmd, cred, clid)
	struct vnode *vp;
	struct flock *ld;
	int cmd;
	struct ucred *cred;
	int clid;
{
	register struct vnode *realvp;

	if ((realvp = VTOS(vp)->s_realvp) != NULL)
		return (VOP_LOCKCTL(realvp, ld, cmd, cred, clid));
	else
		return (EINVAL);	/* can't lock this, it doesn't exist */
}

int
spec_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	register struct vnode *realvp;

	if ((realvp = VTOS(vp)->s_realvp) != NULL)
		return (VOP_FID(realvp, fidpp));
	else
		return (EINVAL);	/* you lose */
}

/*
 * klustsize should be a multiple of PAGESIZE and <= MAXPHYS.
 */
#define	KLUSTSIZE	(56 * 1024)
int klustsize = KLUSTSIZE;
int spec_ra = 1;
int spec_lostpage;	/* number of times we lost original page */

/*
 * Called from pvn_getpages or spec_getpage to get a particular page.
 * When we are called the snode is already locked.
 */
/*ARGSUSED*/
static int
spec_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cred)
	register struct vnode *vp;
	u_int off, *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct ucred *cred;
{
	register struct snode *sp;
	struct buf *bp, *bp2;
	struct page *pp, *pp2, **ppp, *pagefound;
	u_int io_off, io_len;
	u_int blksz, blkoff;
	int dora, err;
	u_int xlen;

	sp = VTOS(vp);

reread:
	err = 0;
	bp = NULL;
	bp2 = NULL;

	if (spec_ra && sp->s_nextr == off)
		dora = 1;
	else
		dora = 0;

	/*
	 * We SNLOCK here to try and allow more concurrent access
	 * to the snode.  We release the lock as soon as we know
	 * we won't be allocating more pages for the vnode.
	 * NB: It's possible that the snode was already locked by
	 * this process (e.g. we were called through pvn_getpages),
	 * thus we are assuming that SNLOCK is recursive.
	 */
	SNLOCK(sp);
again:
	if ((pagefound = page_find(vp, off)) == NULL) {
		/*
		 * Need to really do disk IO to get the page.
		 */
		blkoff = (off / klustsize) * klustsize;
		if (blkoff + klustsize <= sp->s_size)
			blksz = klustsize;
		else
			blksz = sp->s_size - blkoff;

		pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len,
		    blkoff, blksz, 0);
		/*
		 * Somebody has entered the page before us, so
		 * just use it.
		 */
		if (pp == NULL)
			goto again;

		if (!dora)
			SNUNLOCK(sp);

		if (pl != NULL) {
			register int sz;

			if (plsz >= io_len) {
				/*
				 * Everything fits, set up to load
				 * up and hold all the pages.
				 */
				pp2 = pp;
				sz = io_len;
			} else {
				/*
				 * Set up to load plsz worth
				 * starting at the needed page.
				 */
				for (pp2 = pp; pp2->p_offset != off;
				    pp2 = pp2->p_next) {
					ASSERT(pp2->p_next->p_offset !=
					    pp->p_offset);
				}
				sz = plsz;
			}

			ppp = pl;
			do {
				PAGE_HOLD(pp2);
				*ppp++ = pp2;
				pp2 = pp2->p_next;
				sz -= PAGESIZE;
			} while (sz > 0);
			*ppp = NULL;		/* terminate list */
		}

		bp = pageio_setup(pp, io_len, vp, pl == NULL ?
		    (B_ASYNC | B_READ) : B_READ);

		bp->b_dev = vp->v_rdev;
		bp->b_blkno = btodb(io_off);

		/*
		 * Zero part of page which we are not
		 * going to be reading from disk now.
		 */
		xlen = io_len & PAGEOFFSET;
		if (xlen != 0)
			pagezero(pp->p_prev, xlen, PAGESIZE - xlen);

		(*bdevsw[major(vp->v_rdev)].d_strategy)(bp);

		sp->s_nextr = io_off + io_len;
		u.u_ru.ru_majflt++;
		if (seg == segkmap)
			u.u_ru.ru_inblock++;	/* count as `read' operation */
		cnt.v_pgin++;
		cnt.v_pgpgin += btopr(io_len);
	} else if (!dora)
		SNUNLOCK(sp);

	if (dora) {
		u_int off2;
		addr_t addr2;

		off2 = ((off / klustsize) + 1) * klustsize;
		addr2 = addr + (off2 - off);

		/*
		 * If addr is now in a different seg or we are past
		 * EOF then don't bother trying with read-ahead.
		 */
		if (addr2 >= seg->s_base + seg->s_size || off2 >= sp->s_size) {
			pp2 = NULL;
		} else {
			if (off2 + klustsize <= sp->s_size)
				blksz = klustsize;
			else
				blksz = sp->s_size - off2;

			pp2 = pvn_kluster(vp, off2, seg, addr2, &io_off,
			    &io_len, off2, blksz, 1);
		}

		SNUNLOCK(sp);

		if (pp2 != NULL) {
			bp2 = pageio_setup(pp2, io_len, vp, B_READ | B_ASYNC);

			bp2->b_dev = vp->v_rdev;
			bp2->b_blkno = btodb(io_off);

			/*
			 * Zero part of page which we are not
			 * going to be reading from disk now.
			 */
			xlen = io_len & PAGEOFFSET;
			if (xlen != 0)
				pagezero(pp2->p_prev, xlen, PAGESIZE - xlen);

			(*bdevsw[major(vp->v_rdev)].d_strategy)(bp2);

			/*
			 * Should we bill read ahead to extra faults?
			 */
			u.u_ru.ru_majflt++;
			if (seg == segkmap)
				u.u_ru.ru_inblock++;	/* count as `read' */
			cnt.v_pgin++;
			cnt.v_pgpgin += btopr(io_len);
		}
	}

	if (bp != NULL && pl != NULL) {
		err = biowait(bp);
		pageio_done(bp);
	} else if (pagefound != NULL) {
		register int s;

		/*
		 * We need to be careful here because if the page was
		 * previously on the free list, we might have already
		 * lost it at interrupt level.
		 */
		s = splvm();
		if (pagefound->p_vnode == vp && pagefound->p_offset == off) {
			/*
			 * If the page is still intransit or if
			 * it is on the free list call page_lookup
			 * to try and wait for / reclaim the page.
			 */
			if (pagefound->p_intrans || pagefound->p_free)
				pagefound = page_lookup(vp, off);
		}
		if (pagefound == NULL || pagefound->p_offset != off ||
		    pagefound->p_vnode != vp || pagefound->p_gone) {
			(void) splx(s);
			spec_lostpage++;
			goto reread;
		}
		if (pl != NULL) {
			PAGE_HOLD(pagefound);
			pl[0] = pagefound;
			pl[1] = NULL;
			u.u_ru.ru_minflt++;
			sp->s_nextr = off + PAGESIZE;
		}
		(void) splx(s);
	}

	if (err && pl != NULL) {
		for (ppp = pl; *ppp != NULL; *ppp++ = NULL)
			PAGE_RELE(*ppp);
	}

	return (err);
}

/*
 * Return all the pages from [off..off+len) in block device
 */
static int
spec_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
	struct vnode *vp;
	u_int off, len;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct ucred *cred;
{
	struct snode *sp = VTOS(vp);
	int err;

	if (vp->v_type != VBLK || sp->s_bdevvp != vp)
		panic("spec_getpage");

	if (off + len - PAGEOFFSET > sp->s_size)
		return (EFAULT);	/* beyond EOF */

	if (protp != NULL)
		*protp = PROT_ALL;

	if (len <= PAGESIZE)
		err = spec_getapage(vp, off, protp, pl, plsz, seg, addr,
		    rw, cred);
	else {
		SNLOCK(sp);
		err = pvn_getpages(spec_getapage, vp, off, len, protp, pl,
		    plsz, seg, addr, rw, cred);
		SNUNLOCK(sp);
	}

	return (err);
}

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED}
 */
static int
spec_wrtblk(vp, pp, off, len, flags)
	struct vnode *vp;
	struct page *pp;
	u_int off, len;
	int flags;
{
	struct buf *bp;
	int err;

	bp = pageio_setup(pp, len, vp, B_WRITE | flags);
	if (bp == NULL) {
		pvn_fail(pp, B_WRITE | flags);
		return (ENOMEM);
	}

	bp->b_dev = vp->v_rdev;
	bp->b_blkno = btodb(off);

	(*bdevsw[major(vp->v_rdev)].d_strategy)(bp);
	u.u_ru.ru_oublock++;

	/*
	 * If async, assume that pvn_done will
	 * handle the pages when IO is done
	 */
	if ((flags & B_ASYNC) != 0)
		return (0);

	err = biowait(bp);
	pageio_done(bp);

	return (err);
}

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_DIRTY B_FREE, B_DONTNEED}
 * If len == 0, do from off to EOF.
 *
 * The normal cases should be len == 0 & off == 0 (entire vp list),
 * len == MAXBSIZE (from segmap_release actions), and len == PAGESIZE
 * (from pageout).
 */
/*ARGSUSED*/
static int
spec_putpage(vp, off, len, flags, cred)
	register struct vnode *vp;
	u_int off;
	u_int len;
	int flags;
	struct ucred *cred;
{
	register struct snode *sp;
	register struct page *pp;
	struct page *dirty, *io_list;
	register u_int io_off, io_len;
	int vpcount;
	int err = 0;

	sp = VTOS(vp);
	if (vp->v_pages == NULL || off >= sp->s_size)
		return (0);

	if (vp->v_type != VBLK || sp->s_bdevvp != vp)
		panic("spec_putpage");

	vpcount = vp->v_count;
	VN_HOLD(vp);

again:
	if (len == 0) {
		/*
		 * We refuse to act on behalf of the pageout daemon to push
		 * out a page to a snode which is currently locked.
		 */
		if ((sp->s_flag & SLOCKED) && u.u_procp == &proc[2]) {
			err = EWOULDBLOCK;		/* XXX */
			goto out;
		}

		/*
		 * Search the entire vp list for pages >= off.
		 * We lock the snode here to prevent us from having
		 * multiple instances of pvn_vplist_dirty working
		 * on the same vnode active at the same time.
		 */
		SNLOCK(sp);
		dirty = pvn_vplist_dirty(vp, off, flags);
		SNUNLOCK(sp);
	} else {
		/*
		 * Do a range from [off...off + len) via page_find.
		 * We set limits so that we kluster to klustsize boundaries.
		 */
		if (off >= sp->s_size) {
			dirty = NULL;
		} else {
			u_int fsize, eoff, offlo, offhi;

			fsize = (sp->s_size + PAGEOFFSET) & PAGEMASK;
			eoff = MIN(off + len, fsize);
			offlo = (off / klustsize) * klustsize;
			offhi = roundup(eoff, klustsize);
			dirty = pvn_range_dirty(vp, off, eoff, offlo, offhi,
			    flags);
		}
	}

	/*
	 * Now pp will have the list of kept dirty pages marked for
	 * write back.  It will also, handle invalidation and freeing
	 * of pages that are not dirty.  All the pages on the list
	 * returned need to still be dealt with here.
	 */

	/*
	 * Destroy read ahead value (since we are really going to write)
	 */
	if (dirty != NULL)
		sp->s_nextr = 0;

	/*
	 * Handle all the dirty pages not yet dealt with.
	 */
	while ((pp = dirty) != NULL) {
		/*
		 * Pull off a contiguous chunk
		 */
		page_sub(&dirty, pp);
		io_list = pp;
		io_off = pp->p_offset;
		io_len = PAGESIZE;
		while (dirty != NULL && dirty->p_offset == io_off + io_len) {
			pp = dirty;
			page_sub(&dirty, pp);
			page_sortadd(&io_list, pp);
			io_len += PAGESIZE;
			if (io_len >= klustsize - PAGEOFFSET)
				break;
		}
		/*
		 * Check for page length rounding problems
		 */
		if (io_off + io_len > sp->s_size) {
			ASSERT((io_off + io_len) - sp->s_size < PAGESIZE);
			io_len = sp->s_size - io_off;
		}
		err = spec_wrtblk(vp, io_list, io_off, io_len, flags);
		if (err)
			break;
	}

	if (err != 0) {
		if (dirty != NULL)
			pvn_fail(dirty, B_WRITE | flags);
	} else if (off == 0 && (len == 0 || len >= sp->s_size)) {
		/*
		 * If doing "synchronous invalidation", make
		 * sure that all the pages are actually gone.
		 */
		if ((flags & (B_INVAL | B_ASYNC)) == B_INVAL &&
		    (vp->v_pages != NULL))
			goto again;
	}

out:
	/*
	 * Instead of using VN_RELE here we are careful to only call
	 * the inactive routine if the vnode reference count is now zero,
	 * but it wasn't zero coming into putpage.  This is to prevent
	 * recursively calling the inactive routine on a vnode that
	 * is already considered in the `inactive' state.
	 * XXX - inactive is a relative term here (sigh).
	 */
	if (--vp->v_count == 0 && vpcount > 0)
		(void) spec_inactive(vp, cred);
	return (err);
}

/*
 * This routine is called through the cdevsw[] table to handle
 * traditional mmap'able devices that support a d_mmap function.
 */
/*ARGSUSED*/
int
spec_segmap(dev, off, as, addrp, len, prot, maxprot, flags, cred)
	dev_t dev;
	u_int off;
	struct as *as;
	addr_t *addrp;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct ucred *cred;
{
	struct segdev_crargs dev_a;
	int (*mapfunc)();
	register int i;

	if ((mapfunc = cdevsw[major(dev)].d_mmap) == NULL)
		return (ENODEV);

	/*
	 * Character devices that support the d_mmap
	 * interface can only be mmap'ed shared.
	 */
	if ((flags & MAP_TYPE) != MAP_SHARED)
		return (EINVAL);

	/*
	 * Check to insure that the entire range is
	 * legal and we are not trying to map in
	 * more than the device will let us.
	 */
	for (i = 0; i < len; i += PAGESIZE) {
		if ((*mapfunc)(dev, off + i, maxprot) == -1)
			return (ENXIO);
	}

	if ((flags & MAP_FIXED) == 0) {
		/*
		 * Pick an address w/o worrying about
		 * any vac alignment contraints.
		 */
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL)
			return (ENOMEM);
	} else {
		/*
		 * User specified address -
		 * Blow away any previous mappings.
		 */
		(void) as_unmap(as, *addrp, len);
	}

	dev_a.mapfunc = mapfunc;
	dev_a.dev = dev;
	dev_a.offset = off;
	dev_a.prot = prot;
	dev_a.maxprot = maxprot;

	return (as_map(as, *addrp, len, segdev_create, (caddr_t)&dev_a));
}

static int
spec_map(vp, off, as, addrp, len, prot, maxprot, flags, cred)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t *addrp;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct ucred *cred;
{

	if (vp->v_type == VCHR) {
		int (*segmap)();
		dev_t dev = vp->v_rdev;

		/*
		 * Character device, let the device driver
		 * pick the appropriate segment driver.
		 */
		segmap = cdevsw[major(dev)].d_segmap;
		if (segmap == NULL) {
			if (cdevsw[major(dev)].d_mmap == NULL)
				return (ENODEV);

			/*
			 * For cdevsw[] entries that specify a d_mmap
			 * function but don't have a d_segmap function,
			 * we default to spec_segmap for compatibility.
			 */
			segmap = spec_segmap;
		}

		return ((*segmap)(dev, off, as, addrp, len, prot, maxprot,
		    flags, cred));
	} else if (vp->v_type == VBLK) {
		struct segvn_crargs vn_a;

		/*
		 * Block device, use the underlying bdevvp name for pages.
		 */
		if ((int)off < 0 || (int)(off + len) < 0)
			return (EINVAL);

		if ((flags & MAP_FIXED) == 0) {
			map_addr(addrp, len, (off_t)off, 1);
			if (*addrp == NULL)
				return (ENOMEM);
		} else {
			/*
			 * User specified address -
			 * Blow away any previous mappings.
			 */
			(void) as_unmap(as, *addrp, len);
		}

		ASSERT(VTOS(vp)->s_bdevvp != NULL);

		vn_a.vp = VTOS(vp)->s_bdevvp;
		vn_a.offset = off;
		vn_a.type = flags & MAP_TYPE;
		vn_a.prot = prot;
		vn_a.maxprot = maxprot;
		vn_a.cred = cred;
		vn_a.amp = NULL;

		return (as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a));
	} else {
		return (ENODEV);
	}
}

static int
spec_cmp(vp1, vp2)
	struct vnode *vp1, *vp2;
{

	return (vp1 == vp2);
}

int
spec_realvp(vp, vpp)
	struct vnode *vp;
	struct vnode **vpp;
{
	extern struct vnodeops spec_vnodeops;
	extern struct vnodeops fifo_vnodeops;
	struct vnode *rvp;

	if (vp &&
	    (vp->v_op == &spec_vnodeops || vp->v_op == &fifo_vnodeops)) {
		vp = VTOS(vp)->s_realvp;
	}
	if (vp && VOP_REALVP(vp, &rvp) == 0) {
		vp = rvp;
	}
	*vpp = vp;
	return (0);
}

spec_cntl(vp, cmd, idata, odata, iflg, oflg)
	struct vnode *vp;
	int cmd, iflg, oflg;
	caddr_t idata, odata;
{
	struct vnode *realvp;
	int error;

	switch (cmd) {
	/*
	 * ask the dev for this one
	 */
	case _PC_MAX_INPUT:
		if (vp->v_type == VCHR && vp->v_stream) {
			ASSERT(odata && oflg == CNTL_INT32);
			return (VOP_IOCTL(vp, TIOCISIZE, odata, 0, 0));
		} else if ((realvp = other_specvp(vp)) &&
		    realvp->v_type == VCHR && realvp->v_stream) {
			ASSERT(odata && oflg == CNTL_INT32);
			vp->v_stream = realvp->v_stream;
			return (VOP_IOCTL(vp, TIOCISIZE, odata, 0, 0));
		} else {
			/*
			 * This is for posix conformance.  Max input will
			 * always be at least 1 char.  Used to return EINVAL
			 */
			*odata = 1;
			return(0);
		}

	/*
	 * ask the supporting fs for everything else
	 */
	default:
		if (error = VOP_REALVP(vp, &realvp))
			return (error);
		return (VOP_CNTL(realvp, cmd, idata, odata, iflg, oflg));
	}
	/*NOTREACHED*/
}
