/*	@(#) ufs_machdep.c 1.1 92/07/30 SMI; from UCB 4.1 83/06/14	*/

#include <sys/param.h>
#include <sys/map.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/kernel.h>

#include <machine/pte.h>
#include <machine/seg_kmem.h>

#include <vm/seg.h>
#include <vm/hat.h>
#include <vm/page.h>

int bufnhead = 0;	/* count of total buffer headers in the system */
int bufalloc = 0;	/* number of MAXBSIZE buffers allocated */
int buftrace = 0;	/* enable tracing of buf alloc info */
struct buf *bufchain = (struct buf *)0;	/* chain of all buffers in the system */
caddr_t buffree = (caddr_t)0;	/* cookie for fast alloc of buf headers */

/*
 * Machine dependent handling of the buffer cache.
 */

/*
 * Expand or contract the actual memory allocated to a buffer.
 * If no memory is available, release buffer and take error exit.
 * A really fancy scheme would keep buffers sorted by size, but the
 * complexity doesn't seem warranted.
 */
allocbuf(tp, size)
	register struct buf *tp;
	int size;
{
	register struct buf *bp, *ep;
	int sizealloc, take;
	int force;

	sizealloc = roundup(size, PAGESIZE);
	/*
	 * Buffer size does not change
	 */
	if (sizealloc == tp->b_bufsize)
		goto out;
	/*
	 * Buffer size is shrinking.
	 * Place excess space in a buffer header taken from the
	 * BQ_EMPTY buffer list and placed on the "most free" list.
	 * If no extra buffer headers are available, leave the
	 * extra space in the present buffer.
	 * If splitting a request, try to allocate an empty header first.
	 * We allow twice as many buf headers as bufs.
	 */
	if (sizealloc < tp->b_bufsize) {
		ep = bfreelist[BQ_EMPTY].av_forw;
		if (ep == &bfreelist[BQ_EMPTY]) {
			/*
			 * Get a new empty buffer if number of
			 * bufs is not unreasonable.
			 */
			if ((bufnhead > (2 * nbuf)) || (emptybuf() == -1))
				goto out;
			ep = bfreelist[BQ_EMPTY].av_forw;
		}
		notavail(ep);
		pagemove(tp->b_un.b_addr + sizealloc, ep->b_un.b_addr,
		    (int)tp->b_bufsize - sizealloc);
		ep->b_bufsize = tp->b_bufsize - sizealloc;
		tp->b_bufsize = sizealloc;
		ep->b_flags |= B_INVAL;
		ep->b_bcount = 0;
		brelse(ep);
		goto out;
	}
	/*
	 * More buffer space is needed. Get it out of buffers on
	 * the "most free" list, placing the empty headers on the
	 * BQ_EMPTY buffer header list.
	 */
	force = ((tp->b_flags & B_FORCE) == B_FORCE);
	while (tp->b_bufsize < sizealloc) {
		take = sizealloc - tp->b_bufsize;
		bp = getnewbuf(force);
		if (take >= bp->b_bufsize)
			take = bp->b_bufsize;
		pagemove(&bp->b_un.b_addr[bp->b_bufsize - take],
		    &tp->b_un.b_addr[tp->b_bufsize], take);
		tp->b_bufsize += take;
		bp->b_bufsize = bp->b_bufsize - take;
		if (bp->b_bcount > bp->b_bufsize)
			bp->b_bcount = bp->b_bufsize;
		if (bp->b_bufsize <= 0) {
			bremhash(bp);
			binshash(bp, &bfreelist[BQ_EMPTY]);
			bp->b_dev = (dev_t)NODEV;
			bp->b_error = 0;
			bp->b_flags |= B_INVAL;
		}
		brelse(bp);
	}
out:
	tp->b_bcount = size;
	return (1);
}

/*
 * Release space associated with a buffer.
 */
bfree(bp)
	struct buf *bp;
{

	bp->b_bcount = 0;
}

/*
 * Dynamically add another buf and get physical memory for it.
 * Once added, the buf remains part of the buffer cache.
 * MAXBSIZE is a given at 8k on all systems. If filesystems are 4k then
 * only half of MAXBSIZE is needed.
 * Thus, we allocate at most nbuf number of MAXBSIZE buffers, but also
 * allocate at most another nbuf empty buffer headers in which to
 * split (via pagemove) the MAXBSIZE buffers if 4k requests come along.
 * The empty headers are allocated dynamically in allocbuf.
 * We refrain from renaming nbuf to something like maxbsznbuf since some drivers
 * use nbuf to initialize the number of private buffers.
 */

/* XXX move to pte.h */
extern char Bufbase[];

int
bufadd()
{
	u_long a;
	addr_t va;
	register struct buf *bp;
	register int oldps;
	long npages = (long)btopr(MAXBSIZE);
	u_int bufsize = (u_int)ptob(npages);
	static int count = 0;

	oldps = splhigh();
	a = rmalloc(bufmap, npages);	/* allocate space in bufmap */
	(void)splx(oldps);
	if (a == 0)
		return (-1);
	va = (addr_t) (Bufbase + (a << MMU_PAGESHIFT));
	if (segkmem_alloc(&bufheapseg, va, bufsize, 1,
				SEGKMEM_UFSBUF) == 0) /* allocate phys. mem */
		panic("can't get mem for buffers");
	bzero((caddr_t)va, bufsize);
	bp = (struct buf *)new_kmem_fast_alloc(
			&buffree, sizeof (struct buf), nbuf, KMEM_SLEEP);
	bzero((caddr_t)bp, sizeof (struct buf));
	bp->b_dev = NODEV;
	bp->b_bcount = 0;
	bp->b_un.b_addr = (caddr_t)va;
	bp->b_bufsize = bufsize;
	bp->b_flags = B_BUSY|B_INVAL;
	oldps = splclock();
	binshash(bp, &bfreelist[BQ_AGE]);
	bp->b_chain = bufchain;
	bufchain = bp;
	bufalloc++;
	bufnhead++;
	(void) splx(oldps);
	brelse(bp);
	count++;
	if (buftrace) {
		printf("growing <%d>pages of <%d>bytes alloc<%d>\n",
			npages, bufsize, bufalloc);
		printf("total %d bufs(%d), <%d>bytes mem\n",
			count, nbuf, (nbuf * bufsize));
	}
	return (0);
}

/*
 * Get an empty buffer with no physical memory behind it.
 * We get the virtual space to ensure
 * that pagemove can shrink or coalesce into this space.
 */
int
emptybuf()
{
	u_long a;
	register struct buf *bp;
	register int oldps;

	oldps = splhigh();
	a = rmalloc(bufmap, (long)btopr(MAXBSIZE));
	(void)splx(oldps);
	if (a == 0)
		return (-1);
	bp = (struct buf *)new_kmem_fast_alloc(
			&buffree, sizeof (struct buf), nbuf, KMEM_SLEEP);
	bzero((caddr_t)bp, sizeof (struct buf));
	bp->b_dev = NODEV;
	bp->b_bcount = 0;
	bp->b_un.b_addr = (caddr_t) (Bufbase + (a << MMU_PAGESHIFT));
	bp->b_bufsize = 0;	/* force BQ_EMPTY */
	bp->b_flags = 0;
	oldps = splclock();
	binshash(bp, &bfreelist[BQ_AGE]);
	bp->b_chain = bufchain;
	bufchain = bp;
	bufnhead++;
	(void) splx(oldps);
	brelse(bp);
	return (0);
}

/*
 * Take a buffer "semi-permanently" out of the buffer cache allocation.
 * The buffer is still in the cache, but the size of the cache is not
 * charged by this buffer.
 */
buftake(bp)
	struct buf *bp;
{
	bufalloc--;
	bufnhead--;
	bp->b_flags |= B_FORCE;
	if (buftrace)
		printf("buftake: %d\n", bufalloc);
}

/*
 * Release a buffer that was semi-permanently allocated.
 * Note that bufalloc remains the same; we are not putting this guy back.
 * Return the memory and resource map behind the buf,
 * and return its header to the free list.
 * A buffer always has MAXBSIZE amount of resource map.
 * allocbuf will never grow a buffer to bigger than MAXBSIZE.
 * A buffer may have shrunk though, but it will still have the full
 * MAXBSIZE resource map behind it.
 */
bufrelse(bp)
	struct buf *bp;
{
	addr_t va;
	u_int bufsize;
	register struct buf *b;
	int s;

	if ((bp->b_flags & B_FORCE) == 0)
		panic("freeing cached buf?");
	bufsize = bp->b_bufsize;	/* actual physical memory in use */
	va = (addr_t)(bp->b_un.b_addr);
	if (bufsize != 0) {
		segkmem_free(&bufheapseg, va, bufsize); /* Free physical memory */
	}

	/* Free resouce map entries */
	s = splhigh();
	rmfree(bufmap, (long)btopr(MAXBSIZE),
	       (u_long)((va - Bufbase) >> MMU_PAGESHIFT));

	/*
	 * Free buffer header:
	 * 	remove from hash list, remove from buf chain, free buf itself
	 */
	(void)splclock();
	bremhash(bp);
	if (bp == bufchain) {
		bufchain = bp->b_chain;
	} else {
		for (b = bufchain; b != (struct buf *)0; b = b->b_chain) {
			if (b->b_chain == bp) {
				b->b_chain = bp->b_chain;
				break;
			}
		}
		if (b == (struct buf *)0)
			panic("buffree: bad chain");
	}
	kmem_fast_free(&buffree, (caddr_t)bp);
	(void) splx(s);
	if (buftrace)
		printf("bufrelse: %d\n", bufalloc);
}
