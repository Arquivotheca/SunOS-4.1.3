#ident	"@(#)vfs_bio.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/trace.h>
#include <sys/vnode.h>
#include <sys/map.h>
#include <sys/kernel.h>

#include <vm/seg.h>
#include <vm/page.h>
#include <vm/pvn.h>

struct bstats bstats;

/*
 * Read in (if necessary) the block and return a buffer pointer.
 * Can only be used for local block devices found in bdevsw.
 */
struct buf *
bread(vp, blkno, size)
	struct vnode *vp;
	daddr_t blkno;
	int size;
{
	register struct buf *bp;
	int maj;

	bstats.n_bread++;
	if (size == 0)
		panic("bread: size 0");
	bp = getblk(vp, blkno, size);
	if (bp->b_flags & B_DONE) {
		trace2(TR_BREADHIT, vp, blkno);
		bstats.n_bread_hits++;
		return (bp);
	}
	bp->b_flags |= B_READ;
	if (bp->b_bcount > bp->b_bufsize)
		panic("bread");
	maj = major(bp->b_dev);
	if (maj >= nblkdev)
		panic("bread dev");
	(*bdevsw[maj].d_strategy)(bp);
	trace2(TR_BREADMISS, vp, blkno);
	u.u_ru.ru_inblock++;		/* pay for read */
	if (u.u_error == 0)		/* XXX - is this really needed? */
		u.u_error = biowait(bp);
	else
		(void) biowait(bp);
	return (bp);
}

/*
 * Write the buffer, waiting for completion.
 * Then release the buffer.
 */
bwrite(bp)
	register struct buf *bp;
{
	register int flag;
	int maj;

	flag = bp->b_flags;
	bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
	if ((flag & B_DELWRI) == 0)
		u.u_ru.ru_oublock++;		/* no one paid yet */
	trace2(TR_BWRITE, bp->b_vp, bp->b_blkno);
	if (bp->b_bcount > bp->b_bufsize)
		panic("bwrite");
	maj = major(bp->b_dev);
	if (maj >= nblkdev)
		panic("bwrite dev");
	(*bdevsw[maj].d_strategy)(bp);

	/*
	 * If the write was synchronous, then await i/o completion.
	 * If the write was "delayed", then we put the buffer on
	 * the q of blocks awaiting i/o completion status.
	 */
	if ((flag & B_ASYNC) == 0) {
		if (u.u_error == 0) {
			/* XXX - is this really needed? */
			u.u_error = biowait(bp);
		} else {
			(void) biowait(bp);
		}
		brelse(bp);
	} else if (flag & B_DELWRI)
		bp->b_flags |= B_DONTNEED;
}

/*
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 * This can't be done for magtape, since writes must be done
 * in the same order as requested.
 */
bdwrite(bp)
	register struct buf *bp;
{

	if ((bp->b_flags & B_DELWRI) == 0)
		u.u_ru.ru_oublock++;		/* noone paid yet */
#ifdef notdef
	/*
	 * This does not work for buffers associated with
	 * vnodes that are remote - they have no dev.
	 * Besides, we don't use bio with tapes, so rather
	 * than develop a fix, we just ifdef this out for now.
	 */
	if (bdevsw[major(bp->b_dev)].d_flags & B_TAPE)
		bawrite(bp);
	else {
		bp->b_flags |= B_DELWRI | B_DONE;
		brelse(bp);
	}
#endif
	bp->b_flags |= B_DELWRI | B_DONE;
	brelse(bp);
}

/*
 * Release the buffer, start I/O on it, but don't wait for completion.
 */
bawrite(bp)
	register struct buf *bp;
{

	bp->b_flags |= B_ASYNC;
	bwrite(bp);
}

/*
 * Release the buffer, with no I/O implied.
 * Note that the buffer remains on any hash list it was on, with possibly
 * in-progress I/O.
 * May be called at interrupt time.
 */
brelse(bp)
	register struct buf *bp;
{
	register struct buf *flist;
	register int s;

	/*
	 * If someone's waiting for the buffer, or
	 * is waiting for a buffer wake 'em up.
	 */
	if (bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	if (bfreelist[BQ_SLEEP].b_flags & B_WANTED) {
		bfreelist[BQ_SLEEP].b_flags &= ~B_WANTED;
		wakeup((caddr_t)&bfreelist[BQ_SLEEP]);
	}

	/* Make sure nobody uses incorrect data -- B_INVAL is the protocol */
	if (bp->b_flags & (B_NOCACHE | B_ERROR))
		bp->b_flags |= B_INVAL;

	/* Stick the buffer back on a free list */
	s = splbio();
	if (bp->b_bufsize <= 0) {
		/* block has no buffer ... put at front of unused buffer list */
		flist = &bfreelist[BQ_EMPTY];
		binsheadfree(bp, flist);
	} else if (bp->b_flags & (B_ERROR|B_INVAL)) {
		/* block has no info ... put at front of most free list */
		flist = &bfreelist[BQ_AGE];
		binsheadfree(bp, flist);
	} else {
		if (bp->b_flags & B_DONTNEED)
			flist = &bfreelist[BQ_AGE];
		else
			flist = &bfreelist[BQ_LRU];
		binstailfree(bp, flist);
	}
	bp->b_flags &= ~(B_WANTED|B_BUSY|B_ASYNC|B_DONTNEED|B_NOCACHE);
	(void) splx(s);
}

/*
 * Assign a buffer for the given block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 *
 * We use splx here because this routine may be called
 * on the interrupt stack during a dump, and we don't
 * want to lower the ipl back to 0.
 *
 * Note that b_flags may be set asynchronously, so they must be tested
 * at interrupt priority. However, the hash list is not affected
 * by interrupts (e.g., biodone->brelse only affects freelist, not
 * hash list) so it can be searched without locking.
 */
struct buf *
getblk(vp, blkno, size)
	struct vnode *vp;
	daddr_t blkno;
	int size;
{
	register struct buf *bp, *dp;
	register struct buf *tp;
	int s;

	if (size > MAXBSIZE)
		panic("getblk: size too big");

	/*
	 * Search the cache for the block.  If we hit, but
	 * the buffer is in use for i/o, then we wait until
	 * the i/o has completed.
	 */
	dp = BUFHASH(vp, blkno);
loop:
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if ((bp->b_blkno != blkno) || (bp->b_vp != vp))
			continue;
		s = splbio();
		if (bp->b_flags & B_INVAL) {
			(void) splx(s);
			continue;
		}
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			(void) sleep((caddr_t)bp, PRIBIO+1);
			(void) splx(s);
			goto loop;
		}
		notavail(bp);
		(void) splx(s);
		if (bp->b_bcount != size && brealloc(bp, size) == 0)
			goto loop;
		bp->b_flags |= B_CACHE;
		return (bp);
	}
	bp = getnewbuf(0);
	/* getnewbuf may sleep; need to check hash again */
	for (tp = dp->b_forw; tp != dp; tp = tp->b_forw) {
		if ((tp->b_blkno == blkno) && (tp->b_vp == vp)) {
			brelse(bp);
			goto loop;
		}
	}
	bfree(bp);
	bremhash(bp);
	bsetvp(bp, vp);
	bp->b_dev = vp->v_rdev;
	bp->b_blkno = blkno;
	bp->b_error = 0;
	bp->b_resid = 0;
	binshash(bp, dp);
	if (brealloc(bp, size) == 0)
		goto loop;
	return (bp);
}

/*
 * get an empty block,
 * not assigned to any particular device
 */
struct buf *
geteblk(size)
	int size;
{
	register struct buf *bp, *flist;

	if (size > MAXBSIZE)
		panic("geteblk: size too big");
loop:
	bp = getnewbuf(0);
	bp->b_flags |= B_INVAL;
	bfree(bp);
	bremhash(bp);
	flist = &bfreelist[BQ_AGE];
	bp->b_error = 0;
	bp->b_resid = 0;
	binshash(bp, flist);
	if (brealloc(bp, size) == 0)
		goto loop;
	return (bp);
}

/*
 * get an empty block not assigned to any particular device.
 * The block will be semi-permanently allocated: its allocation
 * will not be counted against the number of buffers allocated to
 * the buffer cache.
 */
struct buf *
getpermeblk(size)
	int size;
{
	register struct buf *bp, *flist;

	if (size > MAXBSIZE)
		panic("geteblk: size too big");
loop:
	bp = getnewbuf(1);
	bp->b_flags |= B_INVAL;
	bfree(bp);
	bremhash(bp);
	flist = &bfreelist[BQ_AGE];
	bp->b_error = 0;
	bp->b_resid = 0;
	binshash(bp, flist);
	if (brealloc(bp, size) == 0)
		goto loop;
	return (bp);
}

/* Called instead of brelse for permanently allocated buffer */
freepermeblk(bp)
	struct buf *bp;
{
	if (bp->b_flags & B_FORCE) {
		bufrelse(bp);
	} else
		panic("freepermeblk: bad buf");
}

/*
 * Allocate space associated with a buffer.
 * If can't get space, buffer is released
 */
brealloc(bp, size)
	register struct buf *bp;
	int size;
{
	daddr_t start, last;
	register struct buf *ep;
	struct buf *dp;
	int s;

	/*
	 * First need to make sure that all overlaping previous I/O
	 * is dispatched with.
	 */
	if (size == bp->b_bcount)
		return (1);
	if (size < bp->b_bcount) {
		if (bp->b_flags & B_DELWRI) {
			bwrite(bp);
			return (0);
		}
		return (allocbuf(bp, size));
	}
	bp->b_flags &= ~B_DONE;
	if (bp->b_vp == (struct vnode *)NULL)
		return (allocbuf(bp, size));

	/*
	 * Search cache for any buffers that overlap the one that we
	 * are trying to allocate. Overlapping buffers must be marked
	 * invalid, after being written out if they are dirty. (indicated
	 * by B_DELWRI).  A disk block must be mapped by at most one buffer
	 * at any point in time.  Care must be taken to avoid deadlocking
	 * when two buffer are trying to get the same set of disk blocks.
	 */
	start = bp->b_blkno;
	last = start + btodb(size) - 1;
	dp = BUFHASH(bp->b_vp, bp->b_blkno);
loop:
	for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
		/* no need to spl since if B_INVAL race, set B_INVAL anyway */
		if (ep == bp || ep->b_vp != bp->b_vp || (ep->b_flags & B_INVAL))
			continue;
		/* look for overlap */
		if (ep->b_bcount == 0 || ep->b_blkno > last ||
		    ep->b_blkno + btodb(ep->b_bcount) <= start)
			continue;
		s = splbio();
		if (ep->b_flags & B_BUSY) {
			ep->b_flags |= B_WANTED;
			(void) sleep((caddr_t)ep, PRIBIO+1);
			(void) splx(s);
			goto loop;
		}
		(void) splx(s);
		notavail(ep);
		if (ep->b_flags & B_DELWRI) {
			bwrite(ep);
			goto loop;
		}
		ep->b_flags |= B_INVAL;
		brelse(ep);
	}
	return (allocbuf(bp, size));
}

/*
 * Find a buffer which is available for use from a free list.
 * Currently, no dynamic cache expansion policy.
 *
 * The current allocation policy -- try these in order:
 *	1) Try to get an AGED buffer.
 * 	2) Dynamically allocate buffer if less than nbuf buffers in cache.
 *	3) Try to get an LRU buffer
 * 	4) Sleep for a (soon to be brelse'd) buffer.
 * If force is TRUE (a superblock request, currently), the allocated
 * buffer is removed from the quota of buffers in the cache (i.e., saying
 * that this buffer will not be returned to the cache).
 * This allows subsequent calls to getnewbuf to increase the number
 * of non-permanent buffers in the cache.
 * Extra buf headers are allocated in allocbuf if needed to shrink a buf.
 */
struct buf *
getnewbuf(force)
	int force;	/* TRUE if needed to force buf allocation */
{
	register struct buf *bp, *dp;
	int s;

loop:
	s = splbio();
	for (;;) {
		/* First, see if any garbage buffers on AGE list */
		dp = &bfreelist[BQ_AGE];
		if (dp->av_forw != dp) {
			bstats.n_ages++;
			goto found;
		}

		/* Next, try to dynamically allocate a buffer */
		if (!NOMEMWAIT() && (bufalloc < nbuf) && (bufadd() == 0)) {
			dp = &bfreelist[BQ_AGE];
			if (dp->av_forw == dp)
				panic("bufadd failed");
			goto found;
		}

		/* Then, try to steal a LRU buffer */
		dp = &bfreelist[BQ_LRU];
		if (dp->av_forw != dp) {
			bstats.n_lrus++;
			goto found;
		}

		/* Finally, sleep until someone frees something and try again */
		dp = &bfreelist[BQ_SLEEP];
		dp->b_flags |= B_WANTED;
		(void) sleep((caddr_t)dp, PRIBIO+1);
		bstats.n_sleeps++;
	}

found:
	bp = dp->av_forw;
	notavail(bp);
	(void) splx(s);
	if (bp->b_flags & B_DELWRI) {
		bp->b_flags |= B_ASYNC;
		bwrite(bp);
		goto loop;
	}
	trace2(TR_BRELSE, bp->b_vp, bp->b_blkno);
	brelvp(bp);	/* buf is free; release the vp now (brelvp can sleep) */
	bp->b_flags = B_BUSY;	/* initialize flags */
	if (force) { /* semi-permanently allocate this buffer */
		buftake(bp);
	}
	return (bp);
}

/*
 * Wait for I/O completion on the buffer; return error code.
 * If bp was for synchronous I/O, bp is invalid and associated
 * resources are freed on return.
 */
int
biowait(bp)
	register struct buf *bp;
{
	int err, s;

	s = splbio();
	while ((bp->b_flags & B_DONE) == 0) {
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t)bp, PRIBIO);
	}
	(void) splx(s);
	err = geterror(bp);
	if ((bp->b_flags & B_ASYNC) == 0) {
		if (bp->b_flags & B_PAGEIO)
			pvn_done(bp);
		else if (bp->b_flags & B_REMAPPED)
			bp_mapout(bp);
	}
	return (err);
}

/*
 * Mark I/O complete on a buffer.  If someone should be called,
 * do so.  Otherwise if async do what is needed to free up buffer
 * else wake up anyone waiting for it.
 */
int (*biodone_testpoint)();
biodone(bp)
	register struct buf *bp;
{
	if (biodone_testpoint != NULL)
		if ((*biodone_testpoint)(bp))
			return;

	if (bp->b_flags & B_DONE)
		panic("dup biodone");
	if (bp->b_flags & B_ERROR)
		bioerror(bp);
	if (bp->b_flags & B_CALL) {
		if (bp->b_iodone == NULL)
			panic("bad b_iodone");
		(*bp->b_iodone)(bp);
		return;
	}
	bp->b_flags |= B_DONE;
	if (bp->b_flags & B_ASYNC) {
		if (bp->b_flags & (B_PAGEIO|B_REMAPPED))
			swdone(bp);
		else
			brelse(bp);
	} else if (bp->b_flags & B_WANTED) {
		bp->b_flags &= ~B_WANTED;
		wakeup((caddr_t)bp);
	}
}

/*
 * Make sure all write-behind blocks associated
 * with vp (or the whole cache if vp == 0) are
 * flushed out.  (called from sync)
 */
bflush(vp)
	struct vnode *vp;
{
	register struct buf *bp;
	register struct buf *flist;
	int s;

loop:
	s = splbio();
	for (flist = bfreelist; flist < &bfreelist[BQ_EMPTY]; flist++) {
		for (bp = flist->av_forw; bp != flist; bp = bp->av_forw) {
			if ((bp->b_flags & B_DELWRI) == 0)
				continue;
			if (vp == bp->b_vp || vp == (struct vnode *)0) {
				bp->b_flags |= B_ASYNC;
				notavail(bp);
				(void) splx(s);
				bwrite(bp);
				goto loop;
			}
		}
	}
	(void) splx(s);
}

/*
 * Insure that the specified block is not incore.
 * Shouldn't have to check for overlap, as nothing changes size anymore.
 */
void
blkflush(vp, blkno)
register struct vnode *vp;
register daddr_t blkno;
{
	register struct buf *bp, *dp;
	int s;

	dp = BUFHASH(vp, blkno);
loop:
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if ((bp->b_blkno != blkno) || (bp->b_vp != vp) ||
			bp->b_flags & B_INVAL)
			continue;
		s = splbio();
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			(void) sleep((caddr_t)bp, PRIBIO+1);
			(void) splx(s);
			goto loop;
		}
		if (bp->b_flags & B_DELWRI) {
			notavail(bp);
			(void) splx(s);
			bwrite(bp);
			return;
		}
		(void) splx(s);
	}
}

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized code.
 */
geterror(bp)
	register struct buf *bp;
{
	int error = 0;

	if (bp->b_flags & B_ERROR)
		if ((error = bp->b_error)==0)
			return (EIO);
	return (error);
}

/*
 * Invalidate in core blocks belonging to closed or umounted filesystem
 *
 * This is not nicely done at all - the buffer ought to be removed from the
 * hash chains & have its dev/blkno fields clobbered, but unfortunately we
 * can't do that here, as it is quite possible that the block is still
 * being used for i/o.  Eventually, all disc drivers should be forced to
 * have a close routine, which ought ensure that the queue is empty, then
 * properly flush the queues.  Until that happy day, this suffices for
 * correctness.						... kre
 * This routine assumes that all the buffers have been written.
 */
binval(vp)
	struct vnode *vp;
{
	register struct buf *bp;
	register struct bufhd *hp;
#define	dp ((struct buf *)hp)

loop:
	for (hp = bufhash; hp < &bufhash[BUFHSZ]; hp++)
		for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
			if (bp->b_vp == vp && (bp->b_flags & B_INVAL) == 0) {
				bp->b_flags |= B_INVAL;
				brelvp(bp);
				goto loop;
			}
}

bsetvp(bp, vp)
	struct buf *bp;
	struct vnode *vp;
{

	if (bp->b_vp) {
		brelvp(bp);
	}
	VN_HOLD(vp);
	bp->b_vp = vp;
}

brelvp(bp)
	struct buf *bp;
{

	if (bp->b_vp != (struct vnode *)0) {
		VN_RELE(bp->b_vp);
		bp->b_vp = (struct vnode *)0;
	}
}

/*
 * Support for pageio buffers.
 *
 * This stuff should be generalized to provide a generalized bp
 * header facility that can be used for things other than pageio.
 */

/*
 * Variables for maintaining the free list of buf structures.
 */
static struct buf *pageio_freelist;
static int npageio_incr = 0x20;

/*
 * Pageio_out is a list of all the buffer's
 * currently checked out for pageio use.
 */
static struct bufhd pageio_out = {
	B_HEAD,
	(struct buf *)&pageio_out,
	(struct buf *)&pageio_out,
};

/*
 * Allocate and initialize a buf struct for use with pageio.
 */
struct buf *
pageio_setup(pp, len, vp, flags)
	struct page *pp;
	u_int len;
	struct vnode *vp;
	int flags;
{
	register struct buf *bp;

	if (pageio_freelist == NULL && NOMEMWAIT()) {
		/*
		 * We are pageout, we cannot risk sleeping for more
		 * memory so we return an error condition instead.
		 */
		return ((struct buf *)NULL);
	}

	bp = (struct buf *)new_kmem_fast_zalloc((caddr_t *)&pageio_freelist,
	    sizeof (*pageio_freelist), npageio_incr, KMEM_SLEEP);

	binshash(bp, (struct buf *)&pageio_out);
	bp->b_un.b_addr = 0;
	bp->b_error = 0;
	bp->b_resid = 0;
	bp->b_bcount = len;
	bp->b_bufsize = len;
	bp->b_pages = pp;
	bp->b_flags = B_PAGEIO | B_NOCACHE | B_BUSY | flags;

	VN_HOLD(vp);
	bp->b_vp = vp;

	/*
	 * Caller sets dev & blkno and can adjust
	 * b_addr for page offset and can use bp_mapin
	 * to make pages kernel addressable.
	 */
	return (bp);
}

pageio_done(bp)
	register struct buf *bp;
{

	if (bp->b_flags & B_REMAPPED)
		bp_mapout(bp);
	bremhash(bp);
	VN_RELE(bp->b_vp);
	kmem_fast_free((caddr_t *)&pageio_freelist, (caddr_t)bp);
}

/*
 * THE FOLLOWING IMPLEMENT CALLBACK-ON-DEVICE-ERRORS
 */
struct buferr {
	struct buferr	*be_next;
	dev_t		be_dev;
	caddr_t		be_arg;
	int		(*be_func)();
	long		be_errors;
} *buferr;

struct buferr_stats {
	long	allocs;
	long	frees;
	long	errors;
} buferr_stats;

/*
 * bioerror
 */
bioerror(bp)
	struct buf	*bp;
{
	int		 s;
	struct buferr	*bep;

	/*
	 * callback on device errors
	 */
	s = splbio();
	buferr_stats.errors++;
	for (bep = buferr; bep; bep = bep->be_next)
		if (bep->be_dev == bp->b_dev)
			if ((bep->be_func == NULL) ||
			    ((*(bep->be_func))(bp, bep->be_arg)))
				bep->be_errors++;
	(void) splx(s);
}

allocbioerror(dev, arg, func)
	dev_t		dev;
	caddr_t		arg;
	int		(*func)();
{
	int		s;
	struct buferr	*bep;

	/*
	 * allocate a callback struct
	 */
	bep = (struct buferr *)kmem_alloc(sizeof (struct buferr));
	bep->be_dev    = dev;
	bep->be_arg    = arg;
	bep->be_func   = func;
	bep->be_errors = 0;

	/*
	 * link onto callback-on-device-error list
	 */
	s = splbio();
	bep->be_next = buferr;
	buferr = bep;
	(void) splx(s);

	buferr_stats.allocs++;
}

freebioerror(dev, arg, func)
	dev_t		dev;
	caddr_t		arg;
	int		(*func)();
{
	int		s;
	int		ret;
	struct buferr	*bep;
	struct buferr	**bepp;

	/*
	 * find and process specified callback struct
	 *	 1 = found and removed
	 *	 0 = not found
	 *	-1 = found and not removed because be_errors != 0
	 */
	s = splbio();
	for (bepp = &buferr, bep = *bepp, ret = 0; bep; bep = *bepp) {
		if ((bep->be_dev  == dev) &&
		    (bep->be_arg  == arg) &&
		    (bep->be_func == func)) {
			if (bep->be_errors == 0) {
				ret = 1;
				*bepp = bep->be_next;
				kmem_free((caddr_t)bep, sizeof (struct buferr));
				buferr_stats.frees++;
			} else
				ret = -1;
			break;
		} else
			bepp = &bep->be_next;
	}
	(void) splx(s);
	return (ret);
}

getbioerror(dev, arg, func)
	dev_t		dev;
	caddr_t		arg;
	int		(*func)();
{
	int		s;
	int		errors;
	struct buferr	*bep;

	/*
	 * find a matching callback entry and return and clear be_errors
	 * If no matching entry is found, a -1 is returned
	 */
	s = splbio();
	for (bep = buferr, errors = -1; bep; bep = bep->be_next) {
		if ((bep->be_dev  == dev) &&
		    (bep->be_arg  == arg) &&
		    (bep->be_func == func)) {
			errors = bep->be_errors;
			bep->be_errors = 0;
			break;
		}
	}
	(void) splx(s);
	return (errors);
}
