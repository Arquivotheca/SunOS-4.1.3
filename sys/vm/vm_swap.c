/*	@(#)vm_swap.c 1.1 92/07/30 SMI */

#ident	"$SunId: @(#)vm_swap.c 1.2 91/02/19 SMI [RMTC] $"

/*
 * Copyright (c) 1988, 1989 by Sun Microsystems, Inc.
 */

/*
 * Virtual swap device
 *
 * The virtual swap device consists of the logical concatenation of one
 * or more physical swap areas.  It provides a logical array of anon
 * slots, each of which corresponds to a page of swap space.
 *
 * Each physical swap area has an associated anon array representing
 * its physical storage.  These anon arrays are logically concatenated
 * sequentially to form the overall swap device anon array.  Thus, the
 * offset of a given entry within this logical array is computed as the
 * sum of the sizes of each area preceding the entry plus the offset
 * within the area containing the entry.
 *
 * The anon array entries for unused swap slots within an area are
 * linked together into a free list.  Allocation proceeds by finding a
 * suitable area (attempting to balance use among all the areas) and
 * then returning the first free entry within the area.  Thus, there's
 * no linear relation between offset within the swap device and the
 * address (within its segment(s)) of the page that the slot backs;
 * instead, it's an arbitrary one-to-one mapping.
 *
 * Associated with each swap area is a swapinfo structure.  These
 * structures are linked into a linear list that determines the
 * ordering of swap areas in the logical swap device.  Each contains a
 * pointer to the corresponding anon array, the area's size, and its
 * associated vnode.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/bootconf.h>
#include <sys/trace.h>

#include <vm/hat.h>
#include <vm/anon.h>
#include <vm/page.h>
#include <vm/swap.h>

/* these includes are used for the "fake" swap support of /dev/drum */
#include <sun/mem.h>
#include <specfs/snode.h>

static struct swapinfo *silast;
struct swapinfo *swapinfo;

/*
 * To balance the load among multiple swap areas, we don't allow
 * more than swap_maxcontig allocations to be satisfied from a
 * single swap area before moving on to the next swap area.  This
 * effectively "interleaves" allocations among the many swap areas.
 */
int	swap_maxcontig = 1024 * 1024 / PAGESIZE;	/* 1MB of pages */

extern	int klustsize;		/* from spec_vnodeops.c */
int     swap_order = 1;         /* see swap_alloc,free */

#define	MINIROOTSIZE	14000   /* ~7 Meg */

/*
 * Initialize a new swapinfo structure.
 */
static int
swapinfo_init(vp, npages, skip)
	struct vnode *vp;
	register u_int npages;
	u_int skip;
{
	register struct anon *ap, *ap2;
	register struct swapinfo **sipp, *nsip;

	for (sipp = &swapinfo; nsip = *sipp; sipp = &nsip->si_next)
		if (nsip->si_vp == vp)
			return (EBUSY);		/* swap device already in use */

	nsip = (struct swapinfo *)new_kmem_zalloc(
			sizeof (struct swapinfo), KMEM_SLEEP);
	nsip->si_vp = vp;
	nsip->si_size = ptob(npages);
	/*
	 * Don't indirect through NULL if called with npages < skip (too tacky)
	 */
	if (npages < skip)
		npages = skip;
	nsip->si_anon = (struct anon *)new_kmem_zalloc(
		npages * sizeof (struct anon), KMEM_SLEEP);
	nsip->si_eanon = &nsip->si_anon[npages - 1];
#ifdef RECORD_USAGE
	/*
	 *  Monitoring of swap space usage is enabled,  so malloc
	 *  a parallel array to hold the PID responsible for
	 *  causing the anon page to be created.
	 */
	nsip->si_pid = (short *)
		new_kmem_zalloc(npages * sizeof (short), KMEM_SLEEP);
#endif RECORD_USAGE
	npages -= skip;

	/*
	 * ap2 now points to the first usable slot in the swap area.
	 * Set up free list links so that the head of the list is at
	 * the front of the usable portion of the array.
	 */
	ap = nsip->si_eanon;
	ap2 = nsip->si_anon + skip;
	while (--ap >= ap2)
		ap->un.an_next = ap + 1;
	if (npages == 0) 			/* if size was <= skip */
		nsip->si_free = NULL;
	else
		nsip->si_free = ap + 1;
	anoninfo.ani_free += npages;
	anoninfo.ani_max += npages;

	*sipp = nsip;
	if (silast == NULL)		/* first swap device */
		silast = nsip;
	return (0);
}

/*
 * Initialize a swap vnode.
 */
int
swap_init(vp)
	struct vnode *vp;
{
	struct vattr vattr;
	u_int skip;
	int err;
        
	err = VOP_GETATTR(vp, &vattr, u.u_cred);	/* XXX - u.u_cred? */
	if (err) {
		printf("swap_init: getattr failed, errno %d\n", err);
		return (err);
	}

	/*
	 * To prevent swap I/O requests from crossing the boundary
	 * between swap areas, we erect a "fence" between areas by
	 * not allowing the first page of each swap area to be used.
	 * (This also prevents us from scribbling on the disk label
	 * if the swap partition is the first partition on the disk.)
	 * This may not be strictly necessary, since swap_blksize also
	 * prevents requests from crossing the boundary.
	 *
	 * If swapping on the root filesystem, don't put swap blocks that
	 * correspond to the miniroot filesystem on the swap free list.
	 */
	if (rootvp == vp)
		skip = btoc(roundup(dbtob(MINIROOTSIZE), klustsize));
	else
		skip = 1;

	err = swapinfo_init(vp, (u_int)btop(vattr.va_size), skip);

	if (!err)
		vp->v_flag |= VISSWAP;
	return (err);
}

/*
 * This routine is used to fake npages worth of swap space.
 * These pages will have no backing and cannot be paged out any where.
 */
swap_cons(npages)
	u_int npages;
{

	if (swapinfo_init((struct vnode *)NULL, npages, 0) != 0)
		panic("swap_cons");
}

/*
 * Points to the location (close to) the last block handed to
 * swap_free.  The theory is that if you free one in this area,
 * you'll probably free more, so use the hint as a starting point.
 * hint is reset on each free to the block that preceeds the one
 * freed (or the block freed, if we can't find the block before it).
 * It is also reset if it points at block that is allocated.
 *
 * XXX - swap_free and swap_alloc both manipulate hint; the free
 * lists are now protected with splswap(). Don't call into these routines
 * from higher level interrupts!
 */
static struct {
        struct anon     *ap;    /* pointer to the last freed */
        struct swapinfo *sip;   /* swap list for which hint is valid */
} hint;

int     swap_hit;               /* hint helped */
int     swap_miss;              /* hint was no good */


/*
 * Allocate a single page from the virtual swap device.
 */
struct anon *
swap_alloc()
{
	struct swapinfo *sip = silast;
	struct anon *ap;

	do {
		ap = sip->si_free;
		if (ap) {
                        /*
                         * can't condition this on swap_order since some
                         * idiot might turn it on and off.  It's not cool
                         * to have the hint point at an allocated block.
                         */
                        if (hint.sip == sip && hint.ap == ap)
                                hint.sip = NULL;
			sip->si_free = ap->un.an_next;
			if (++sip->si_allocs >= swap_maxcontig) {
				sip->si_allocs = 0;
				if (sip == silast) {
					silast = sip->si_next;
					if (silast == NULL)
						silast = swapinfo;
				}
			} else {
				silast = sip;
			}
#			ifdef	TRACE
			{
				struct vnode *vp;
				u_int off;

				swap_xlate(ap, &vp, &off);
				trace3(TR_MP_SWAP, vp, off, ap);
			}
#			endif	TRACE
#ifdef RECORD_USAGE
			if (u.u_procp) {
			/*  swap monitoring is on - record the current PID */
			sip->si_pid[ap - sip->si_anon] = u.u_procp->p_pid;
			}
#endif RECORD_USAGE
			return (ap);
		}
		/*
		 * No more free anon slots here.
		 */
		sip->si_allocs = 0;
		sip = sip->si_next;
		if (sip == NULL)
			sip = swapinfo;
	} while (sip != silast);
	return ((struct anon *)NULL);
}

/*
 * Free a swap page.
 * List is maintained in sorted order.  Worst case is a linear search on the
 * list; we maintain a hint to mitigate this.
 *
 * Pointing the hint at the most recently free'd anon struct makes it
 * really fast to free anon pages in ascending order.
 *
 * Pointing the hint at the anon struct that is just *before* this makes
 * it really fast to free anon pages in descending order, at nearly zero
 * cost.
 *
 * This alogrithm points the hint at the anon struct that points to
 * the one most recently free'd. When freeing a block of anon structs
 * presented in ascending order, the hint advances one block behind
 * the blocks as they are free'd. When freeing a block of anon structs
 * precented in descending order -- which happens if a large hunk of
 * memory is allocated in reverse order then free'd in forward order,
 * common enough to be a problem -- the hint remains pointing at the
 * anon struct that ends up pointing at each of the free'd blocks
 * in order. This is worth an example.
 *
 * Assume anons #2 and #9 are free, the hint points to anon #2, and
 * #2's "next" pointer goes to #9. Now, we present a set of swap_free
 * requests for blocks #8 through #3, in descending order. This results
 * in a series of hits on the hint, which just keeps pointing at #2.
 * The previous algorithm would have set the hint to each block as
 * it came in, resulting in worst-case behavior as the list had to
 * be scanned from the front.
 */
void
swap_free(ap)
	struct anon *ap;
{
	register struct swapinfo *sip = silast;
        register struct anon *tap, **tapp;
	register struct anon *tap_hint;

	/*
	 * Find the swap area containing ap and then put
	 * ap at the head of that area's free list.
	 */
	do {
		if (sip->si_anon <= ap && ap <= sip->si_eanon) {
/*
			ap->un.an_next = sip->si_free;
			sip->si_free = ap;
*/
                        /*
                         * old unordered way
                         */
                        if (!swap_order) {
                                ap->un.an_next = sip->si_free;
                                sip->si_free = ap;
#ifdef RECORD_USAGE
                                /*  Swap monitoring is on - undo the PID */
                                sip->si_pid[ap - sip->si_anon] = 0;
#endif RECORD_USAGE
                                return;
                        }
                        /*
                         * Do it in order; use hint if possible
                         */
			tap = hint.ap;
                        if (hint.sip == sip && tap < ap) {
				/*
				 * The anon we are freeing
				 * follows the hint tap somewhere.
				 * save the hint and advance
				 * to the next free anon.
				 */
				tapp = &tap->un.an_next;
				tap_hint = tap;
				tap = tap->un.an_next;
                                swap_hit++;
                        } else {
				/*
				 * Wrong swapinfo, or
				 * the anon being free'd
				 * preceeds the hint.
				 * must start scanning
				 * from the front of the
				 * list. The best hint we
				 * can seed with is the
				 * anon we are freeing.
				 */
                                tapp = &sip->si_free;
                                tap = sip->si_free;
				tap_hint = ap;
                                swap_miss++;
                        }
			/*
			 * advance tap until it is greater
			 * than the incoming anon.
			 */
                        while (tap && tap < ap) {
                            tapp = &tap->un.an_next;
			    tap_hint = tap;
                            tap = tap->un.an_next;
                        }
                        *tapp = ap;
                        ap->un.an_next = tap;
#ifdef RECORD_USAGE
			/*  Swap monitoring is on - undo the PID */
			sip->si_pid[ap - sip->si_anon] = 0;
#endif RECORD_USAGE
                        hint.sip = sip;
                        hint.ap = tap_hint;
			return;
		}
		sip = sip->si_next;
		if (sip == NULL)
			sip = swapinfo;
	} while (sip != silast);
	panic("swap_free");
	/* NOTREACHED */
}

/*
 * Return the <vnode, offset> pair
 * corresponding to the given anon struct.
 */
void
swap_xlate(ap, vpp, offsetp)
	struct anon *ap;
	struct vnode **vpp;
	u_int *offsetp;
{
	register struct swapinfo *sip = silast;

	do {
		if (sip->si_anon <= ap && ap <= sip->si_eanon) {
			*offsetp = ptob(ap - sip->si_anon);
			*vpp = sip->si_vp;
			return;
		}
		sip = sip->si_next;
		if (sip == NULL)
			sip = swapinfo;
	} while (sip != silast);
	panic("swap_xlate");
	/* NOTREACHED */
}

/*
 * Like swap_xlate, but return a status instead of panic'ing.
 * Used by dump routines when we know we may be corrupted.
 */
swap_xlate_nopanic(ap, vpp, offsetp)
	struct anon *ap;
	struct vnode **vpp;
	u_int *offsetp;
{
	register struct swapinfo *sip = swapinfo;

	do {
		if (sip->si_anon <= ap && ap <= sip->si_eanon) {
			*offsetp = (ap - sip->si_anon) << PAGESHIFT;
			*vpp = sip->si_vp;
			return (1);
		}
	} while (sip = sip->si_next);

	/* Couldn't find it; return failure */
	return (0);
}

/*
 * Return the anon struct corresponding for the given
 * <vnode, offset> if it is part of the virtual swap device.
 */
struct anon *
swap_anon(vp, offset)
	struct vnode *vp;
	u_int offset;
{
	register struct swapinfo *sip = silast;

	if (vp && sip) {
		do {
			if (vp == sip->si_vp && offset < sip->si_size)
				return (sip->si_anon + (offset >> PAGESHIFT));
			sip = sip->si_next;
			if (sip == NULL)
				sip = swapinfo;
		} while (sip != silast);
	}
	/*
	 * Note - we don't return the anon structure for
	 * fake'd anon slots which have no real vp.
	 */
	return ((struct anon *)NULL);
}

/*
 * swread and swwrite implement the /dev/drum device, an indirect,
 * user visible, device to allow reading of the (virtual) swap device.
 */

/*ARGSUSED*/
swread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (sw_rdwr(uio, UIO_READ));
}

/*ARGSUSED*/
swwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (sw_rdwr(uio, UIO_WRITE));
}

/*
 * Handle all the work of reading "fake" swap pages that are in memory.
 */
static int
fake_sw_rdwr(uio, rw, cred)
	register struct uio *uio;
	enum uio_rw rw;
	struct ucred *cred;
{
	struct page *pp;
	struct vnode *memvp;
	int nbytes;
	u_int off;
	int err;
	extern int mem_no;

	nbytes = uio->uio_resid;
	off = uio->uio_offset;
	memvp = makespecvp(makedev(mem_no, M_MEM), VCHR);

	do {
		/*
		 * Find the page corresponding to the "fake" name
		 * and then read the corresponding page from /dev/mem.
		 */
		pp = page_find((struct vnode *)NULL, (u_int)(off & PAGEMASK));
		if (pp == NULL) {
			err = EIO;
			break;
		}
		uio->uio_offset = ptob(page_pptonum(pp)) + (off & PAGEOFFSET);

		if ((off & PAGEOFFSET) == 0)
			uio->uio_resid = MIN(PAGESIZE, nbytes);
		else
			uio->uio_resid = min(ptob(btopr(off)) - off,
			    (u_int)nbytes);
		nbytes -= uio->uio_resid;
		off += uio->uio_resid;
		err = VOP_RDWR(memvp, uio, rw, 0, cred);
	} while (err == 0 && nbytes > 0 && uio->uio_resid == 0);

	VN_RELE(memvp);
	return (err);
}

/*
 * Common routine used to break up reads and writes to the
 * (virtual) swap device to the underlying vnode(s).  This is
 * used to implement the user visable /dev/drum interface.
 */
static int
sw_rdwr(uio, rw)
	register struct uio *uio;
	enum uio_rw rw;
{
	register struct swapinfo *sip = swapinfo;
	int nbytes = uio->uio_resid;
	u_int off = 0;
	int err = 0;

	do {
		if (uio->uio_offset >= off &&
		    uio->uio_offset < off + sip->si_size)
			break;
		off += sip->si_size;
	} while (sip = sip->si_next);

	if (sip) {
		uio->uio_offset -= off;
		do {
			uio->uio_resid = MIN(sip->si_size - uio->uio_offset,
			    nbytes);
			nbytes -= uio->uio_resid;
			if (sip->si_vp)
				err = VOP_RDWR(sip->si_vp, uio, rw, 0,
				    u.u_cred);
			else
				err = fake_sw_rdwr(uio, rw, u.u_cred);
			uio->uio_offset = 0;
		} while (err == 0 && nbytes > 0 && uio->uio_resid == 0 &&
		    (sip = sip->si_next));
		uio->uio_resid = nbytes + uio->uio_resid;
	}

	return (err);
}

/*
 * System call swapon(name) enables swapping on device name,
 * Return EBUSY if already swapping on this device.
 */
swapon()
{
	register struct a {
		char	*name;
	} *uap = (struct a *)u.u_ap;
	struct vnode *vp;

	if (!suser())
		return;
	uap = (struct a *)u.u_ap;
	if (u.u_error = lookupname(uap->name, UIOSEG_USER, FOLLOW_LINK,
	    (struct vnode **)NULL, &vp))
		return;

	switch (vp->v_type) {
	case VBLK: {
		struct vnode *nvp;

		nvp = bdevvp(vp->v_rdev);
		VN_RELE(vp);
		vp = nvp;
		/*
		 * Call the partition's open routine, to give it a chance to
		 * check itself for consistency (e.g., for scrambled disk
		 * labels).  (The open isn't otherwise required.)
		 */
		if (u.u_error = VOP_OPEN(&vp, FREAD|FWRITE, u.u_cred))
			goto out;
		break;
	}

	case VREG:
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
			u.u_error = EROFS;
			goto out;
		}
		if (u.u_error = VOP_ACCESS(vp, VREAD|VWRITE, u.u_cred))
			goto out;
		if (u.u_error = VOP_OPEN(&vp, FREAD|FWRITE, u.u_cred))
			goto out;
		break;

	case VDIR:
		u.u_error = EISDIR;
		goto out;

	case VCHR:
	case VSOCK:
	default:
		u.u_error = EOPNOTSUPP;
		goto out;
	}
	u.u_error = swap_init(vp);
out:
	if (u.u_error) {
		VN_RELE(vp);
	}
}
