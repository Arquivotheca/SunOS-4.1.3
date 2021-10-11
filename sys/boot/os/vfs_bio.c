#ifndef lint
static        char sccsid[] = "@(#)vfs_bio.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <machine/pte.h>
#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/trace.h>
#include "boot/vnode.h"

#ifdef	DUMP_DEBUG
static int dump_debug = 20;
#endif	/* DUMP_DEBUG */

#ifdef	NFS_BOOT
#undef u
extern struct user u;
#endif	 /* NFS_BOOT */

struct b1stats {
        int     n_bread;
        int     n_bread_hits;
        int     n_breada;
        int     n_breada_hits1;
        int     n_breada_hits2;
} bstats;

char	*buffers;
struct  buf *buf;               /* the buffer pool itself */

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf *
bread(vp, blkno, size)
        struct vnode *vp;
        daddr_t blkno;
        int size;
{
        register struct buf *bp;

        bstats.n_bread++;
        if (size == 0)
                panic("bread: size 0");
#ifdef	NFS_BOOT1
	/*
	 * Assume we always have to read the block.
	 */
	bp = geteblk(size);
	bsetvp(bp, vp);
        bp->b_dev = vp->v_rdev;
        bp->b_blkno = blkno;
	bp->b_bcount = size;
        bp->b_error = 0;
        bp->b_resid = 0;
#else 
        bp = getblk(vp, blkno, size);
#endif	 /* NFS_BOOT */
        if (bp->b_flags&B_DONE) {
#if !defined(NFS_BOOT)
                trace(TR_BREADHIT, vp, blkno);
#endif	 
                bstats.n_bread_hits++;
                return (bp);
        }
        bp->b_flags |= B_READ;
        if (bp->b_bcount > bp->b_bufsize)
                panic("bread");
	bp->b_bcount = size;
        VOP_STRATEGY(bp);
#if !defined(NFS_BOOT)
        trace(TR_BREADMISS, vp, blkno);
#endif	 
        u.u_ru.ru_inblock++;            /* pay for read */
#if !defined(NFS_BOOT)
        biowait(bp);
#endif	
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


loop:
        bp = getnewbuf();
        bp->b_flags |= B_INVAL;
        bfree(bp);
        bremhash(bp);
        flist = &bfreelist[BQ_AGE];
        brelvp(bp);
        bp->b_error = 0;
        bp->b_resid = 0;
        binshash(bp, flist);
        if (brealloc(bp, size) == 0)	{
	        goto loop;
	}
        return (bp);
}

/*
 * Assign a buffer for the given block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 *
 * We use splx here because this routine may be called
 * on the interrupt stack during a dump, and we don't
 * want to lower the ipl back to 0.
 */
struct buf *
getblk(vp, blkno, size)
        struct vnode *vp;
        daddr_t blkno;
        int size;
{
        register struct buf *bp, *dp;
        int s;

        if ((unsigned)blkno >= 1 << (sizeof(int)*NBBY-DEV_BSHIFT))    /* XXX */
                blkno = 1 << ((sizeof(int)*NBBY-DEV_BSHIFT) + 1);
        /*
         * Search the cache for the block.  If we hit, but
         * the buffer is in use for i/o, then we wait until
         * the i/o has completed.
         */
        dp = BUFHASH(vp, blkno);
loop:
        for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
                if (bp->b_blkno != blkno || bp->b_vp != vp ||
                    bp->b_flags&B_INVAL)
                        continue;
                s = spl6();
                if (bp->b_flags&B_BUSY) {
#ifdef	DUMP_DEBUG
			dprint(dump_debug, 6, "getblk: busy 0x%x\n", bp);
#endif	/* DUMP_DEBUG */
                        bp->b_flags |= B_WANTED;
                        (void) sleep((caddr_t)bp, PRIBIO+1);
                        (void) splx(s);
                        goto loop;
                }
                (void) splx(s);
                notavail(bp);
                if (brealloc(bp, size) == 0)
                        goto loop;
                bp->b_flags |= B_CACHE;
                return (bp);
        }
/*
        if (major(dev) >= nblkdev)
                panic("blkdev");
*/
        bp = getnewbuf();
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
 * Release the buffer, with no I/O implied.
 */
brelse(bp)
        register struct buf *bp;
{
        register struct buf *flist;
        register s;

        /*
         * If someone's waiting for the buffer, or
         * is waiting for a buffer wake 'em up.
         */
        if (bp->b_flags&B_WANTED)	{
		wakeup((caddr_t)bp);
	}

        if (bfreelist[0].b_flags&B_WANTED) {
                bfreelist[0].b_flags &= ~B_WANTED;
                wakeup((caddr_t)bfreelist);
        }
        if (bp->b_flags & B_NOCACHE) {
                bp->b_flags |= B_INVAL;
        }
 
        /*
         * Stick the buffer back on a free list.
         */
        s = spl6();
        if (bp->b_bufsize <= 0) {
                /* block has no buffer ... put at front of unused buffer list */                flist = &bfreelist[BQ_EMPTY];
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
 * Find a buffer which is available for use.
 * Select something from a free list.
 * Preference is to AGE list, then LRU list.
 */
struct buf *
getnewbuf()
{
        register struct buf *bp, *dp;
        int s;
 

loop:
        s = spl6();      
        for (dp = &bfreelist[BQ_AGE]; dp > bfreelist; dp--)	{
	        if (dp->av_forw != dp)
                        break;
	}
                
        if (dp == bfreelist) {          /* no free blocks */
                dp->b_flags |= B_WANTED;
                (void) sleep((caddr_t)dp, PRIBIO+1);
                (void) splx(s);
                goto loop;
        }
        (void) splx(s);
        bp = dp->av_forw;
        notavail(bp);
        if (bp->b_flags & B_DELWRI) {
                bp->b_flags |= B_ASYNC;
                bwrite(bp);
                goto loop;
        }
        brelvp(bp);
#ifdef	NEVER
        trace(TR_BRELSE, bp->b_vp, bp->b_blkno);
#endif	/* NEVER */
        bp->b_flags = B_BUSY;
        return (bp);

}


brealloc(bp, size)
	struct buf *bp;
	int	size;
{
        daddr_t start, last;
        register struct buf *ep;
        struct buf *dp;
        int s;


        /*
         * First need to make sure that all overlaping previous I/O
         * is dispatched with.
         */
        if (size == bp->b_bcount)	{
	        return (1);
	}

        if (size < bp->b_bcount) {
                if (bp->b_flags & B_DELWRI) {
                        bwrite(bp);
                        return (0);
                }
                return (allocbuf(bp, size));
        }
        bp->b_flags &= ~B_DONE;
        if (bp->b_vp == (struct vnode *) 0)	{
                return (allocbuf(bp, size));
	}
 
	/*
         * Search cache for any buffers that overlap the one that we
         * are trying to allocate. Overlapping buffers must be marked
         * invalid, after being written out if they are dirty. (indicated
         * by B_DELWRI) A disk block must be mapped by at most one buffer
         * at any point in time. Care must be taken to avoid deadlocking
         * when two buffer are trying to get the same set of disk blocks.
         */
        start = bp->b_blkno;
        last = start + btodb(size) - 1;
        dp = BUFHASH(bp->b_vp, bp->b_blkno);
loop:
        for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
                if (ep == bp || ep->b_vp != bp->b_vp || (ep->b_flags&B_INVAL))
                        continue;
                /* look for overlap */
                if (ep->b_bcount == 0 || ep->b_blkno > last ||
                    ep->b_blkno + btodb(ep->b_bcount) <= start)
                        continue;
                s = spl6();
                if (ep->b_flags&B_BUSY) {
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
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized
 * code.  Actually the latter is always true because devices
 * don't yet return specific errors.
 */
geterror(bp)
        register struct buf *bp;
{
        int error = 0;

        if (bp->b_flags&B_ERROR)
                if ((error = bp->b_error)==0)
                        return (EIO);
        return (error);
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
        struct vnode *vp;

        if (bp->b_vp == (struct vnode *) 0) {
                return;
        }
        vp = bp->b_vp;          /* save vp because VN_RELE may sleep */
        bp->b_vp = (struct vnode *) 0;
        VN_RELE(vp);
}

/*
 * Following two routines stolen from init_main.c
 */

/*
 * Initialize hash links for buffers.
 */
bhinit()
{
        register int i;
        register struct bufhd *bp;
 
        for (bp = bufhash, i = 0; i < BUFHSZ; i++, bp++)
                bp->b_forw = bp->b_back = (struct buf *)bp;
}

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
binit()
{
        register struct buf *bp, *dp;
        register int i;
#ifdef NEVER
        int base, residual;
#endif NEVER

        for (dp = bfreelist; dp < &bfreelist[BQUEUES]; dp++) {
                dp->b_forw = dp->b_back = dp->av_forw = dp->av_back = dp;
                dp->b_flags = B_HEAD;
        }
#ifdef	 NFS_BOOT
	/*
	 * Initialise the buffer storage area.
	 */
	nbuf = 10;
	buf = (struct buf *)kmem_alloc((u_int)(nbuf * sizeof(struct buf)));
	buffers = kmem_alloc((u_int)(nbuf * MAXBSIZE));
        for (i = 0; i < nbuf; i++) {
                bp = &buf[i];
                bp->b_dev = NODEV;
                bp->b_bcount = 0;
                bp->b_un.b_addr = buffers + i * MAXBSIZE;
		bp->b_bufsize = MAXBSIZE;
                binshash(bp, &bfreelist[BQ_AGE]);
                bp->b_flags = B_BUSY|B_INVAL;
                brelse(bp);
        }
#else 
        base = bufpages / nbuf;
        residual = bufpages % nbuf;
        for (i = 0; i < nbuf; i++) {
                bp = &buf[i];
                bp->b_dev = NODEV;
                bp->b_bcount = 0;
                bp->b_un.b_addr = buffers + i * MAXBSIZE;
                if (i < residual)
                        bp->b_bufsize = (base + 1) * CLBYTES;
                else
                        bp->b_bufsize = base * CLBYTES;
                binshash(bp, &bfreelist[BQ_AGE]);
                bp->b_flags = B_BUSY|B_INVAL;
                brelse(bp);
        }
#endif	 /* NFS_BOOT */
}

/*
 * Stubs.
 */
bwrite(bp)
	struct buf *bp;
{
#ifdef	DUMP_DEBUG
        dprint(dump_debug, 0, "bwrite: You cannot write\n");
#endif	/* DUMP_DEBUG */
}

/*
 * This is a fake.   On a Sun2, if you have an xy card,
 * you get a level 6 interrupt as soon as you drop the
 * processor priority to 6, so we don't do that.
 */
static
spl6()
{
	return (0);
}
static
splx(s)
{
#ifdef lint
	s = s;
#endif lint
}
