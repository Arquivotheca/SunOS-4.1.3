#ident	"@(#)bdev_dsort.c 1.1 92/07/30 SMI"	/* from UCB 4.3 81/03/09 */

/*
 * Seek sort for disks.  We depend on the driver
 * which calls us using b_resid as the current cylinder number.
 *
 * The argument dp structure holds a b_actf activity chain pointer
 * on which we keep two queues, sorted in ascending cylinder order.
 * The first queue holds those requests which are positioned after
 * the current cylinder (in the first request); the second holds
 * requests which came in after their cylinder number was passed.
 * Thus we implement a one way scan, retracting after reaching the
 * end of the drive to the first request on the second queue,
 * at which time it becomes the first queue.
 *
 * A one-way scan is natural because of the way UNIX read-ahead
 * blocks are allocated.
 *
 * This implementation also allows certain page-oriented operations
 * to 'kluster' up into a single request.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <vm/page.h>
#include <sys/kmem_alloc.h>

#define	b_cylin	b_resid

/*
 * A kluster structure manages pools of buffers which are klustered together
 * with a single primary buffer. The single primary buffer is an original
 * buffer that has been modified to perform I/O for all the buffers in the
 * pool.
 *
 * The traditional action of disksort() is to sort a buffer into a disk
 * activity queue, using the b_resid field as a sort key and using the
 * av_forw tag as the forward link field. The av_back field for each buffer
 * is available for the driver to use as it sees fit.
 *
 * Diagrammatically, this is how this looks:
 *
 * queue header:
 *	front -----------> buf --------> buf --------> buf ---> 0
 *	back  -----------------------------------------^
 *
 *
 * When a certain set of conditions are met (see below), instead of sorting
 * a new buffer into this queue, we instead try and modify a buffer that is
 * currently in the queue to do the I/O operation for the new buffer
 *
 *
 * In this case, we allocate a kluster structure and modify things such
 * that things look like this:
 *
 * queue header:
 *	front -----------> buf ------> buf --------> buf ---> 0
 *	back  ------------------------/ ^ /-----------^
 *					|
 *			klust struct	|
 *			 prime buf -----/
 *			 front ------------> buf
 *			 tail ----------------^
 *
 * The kluster structure also maintains a copy of the original b_bcount
 * field for the primary buffer. This is so that when this arrangement is
 * decommissioned (either by calling klustdone() or klustbust()), that
 * the original primary buffer structure ends up looking the same as
 * if it had never been operated on by the klustering code.
 *
 * The conditions for which klustering might apply are these:
 *
 *	1) The driver wishing to use klustering calls the klustsort()
 *	function instead of the disksort() function. The klustsort()
 *	function functions as disksort did, except that it takes a third
 *	argument which is the non-inclusive integer maximum upper limit
 *	(in bytes) that a kluster operation can be increased to. The
 *	old disksort() interface is maintained by haveing it turn right
 *	around and call klustsort() with this argument as zero.
 *
 *	2) The driver uses the b_resid sort key to sort by absolute
 *	logical block. Historically, the sort key has been just the
 *	drive cylinder number for the request. This allows a number
 *	of requests for a drive to be partially sorted with respect
 *	to the drive layout, and is more or less optimal for devices
 *	where the notion of cylinder is stil meaningful (SA-450, ST-506,
 *	ESDI, and SMD devices), but is not particularly meaningful for
 *	devices which are logically addressed (SCSI and IPI).
 *
 *	3) A number or condtions for both the buffer already in the
 *	queue and the new buffer to be sorted or klustered are met.
 *	These are a fairly limiting and restrictive set of conditions.
 *
 *		+ The buffer in the queue is not the head of the queue
 *		(i.e., isn't the 'active' request).
 *
 *		+ The the b_dev fields of both buffers are the same
 *
 *		+ The buffer being added has only B_PAGEIO set of the
 *		flags B_KLUSTER, B_REMAPPED, B_PAGEIO and B_READ.
 *
 *		+ The buffer already in the queue has only B_PAGEIO set
 *		of the flage B_REMAPPED, B_PAGEIO and B_READ.
 *
 *		+ The b_un.b_addr field of both buffers is zero.
 *
 *		+ The b_bcount field of both buffers is mmu page aligned.
 *
 *		+ The logical block number for the buffer already in the
 *		queue plus the quantity btodb() of its b_bcount field
 *		equal the b_blkno field of the buffer to be added
 *		(i.e., a logical contiguous set of disk blocks are
 *		maintained)
 *
 *		+ The the b_bcount field of the buffer in the queue plus
 *		b_bcount field of the buffer to be added does not equal
 *		or exceed the maximum as passed by the driver.
 *
 *	The intent of these conditions are to ensure that all buffers are
 *	pure page-oriented operations, are write operations only, are for
 *	logically contiguous areas of the device, and do not exceed some
 *	count limitation specified by the driver, before allowing a new
 *	buffer to be klustered with a buffer already in the queue rather
 *	than being sorted into the queue.
 *
 *	If these conditions are met, a routine is called which attempts
 *	to add the new buffer to a list of buffers that are klustered
 *	with the buffer already in the queue. If this is successful, the
 *	buffer in the queue is modified to 'own' the list of pages
 *	for the new buffer, and its b_bcount field is adjusted to
 *	reflect the new size of the data area being managed for I/O.
 *
 *	The klustsort() routine returns the value 1 if the buffer that
 *	it had been passed was klustered, else 0 (in which case the
 *	buffer has just been sorted into the activity queue). The
 *	primary buffer's b_flags field has B_KLUSTER set in it to
 *	note that this is the primary buffer of a kluster of buffers.
 *
 *	4) When I/O completes for a buffer marked B_KLUSTER, the driver
 *	calls the function klustdone() (instead of iodone()). klustdone()
 *	breaks apart the list of pages from the primary buffer and restores
 *	them to their original 'owners', restores the b_bcount field for
 *	the primary kluster buffer, clears the B_KLUSTER flag in the
 *	primary buffer, and calls iodone() for all buffers that were part
 *	of this kluster. If the primary buffer had either a residual count
 *	set in b_resid, or the flag B_ERROR was set, all buffers that were
 *	part of the kluster have B_ERROR set, and b_resid set equal to their
 *	b_bcount field. klustdone() returns the integer number of buffers
 *	that had all been klustered together.
 *
 *	Optionally, if a driver wishes to retry failed I/O operations on each
 *	buffer from a kluster singly (in order to isolate the actual error
 *	more precisely), the function klustbust() is provided. The driver
 *	passes the primary buffer to klustbust(), which performs the same
 *	restoration of pages to their rightful owners and the b_bcount field
 *	back to the primary driver. It leaves the buffers linked together
 *	as a forward linked list of buffers (through the av_forw) field
 *	starting from the primary buffer. The driver can then do as it
 *	pleases with this chain.
 */

struct kluster {
	struct kluster	*klust_next;	/* next in a list of kluster structs */
	struct buf	*klust_head;	/* head of list of indirect bufs */
	struct buf	*klust_tail;	/* tail of list of indirect bufs */
	struct buf	*klust_prime;	/* primary kluster buffer */
	int		klust_pcount;	/* primary buf's original b_bcount */
};

static int kluston = 1;
static int klust_buf_flag_chk = B_REMAPPED|B_PAGEIO|B_READ;
static int nklusters;
#define	KLUSTMAXPSIZE	128
static struct kluster *klustfree, *klustbusy;

int klustsort(), klustdone();
void klustbust();
static int klustadd();

disksort(dp, bp)
struct diskhd *dp;
struct buf *bp;
{
	(void) klustsort(dp, bp, 0);
}

/*
 * Perform traditional sorting into a disk activity queue.
 *
 * If desired, instead of sorting a buffer into the queue,
 * see if it can instead have its I/O operation joined up
 * with the I/O operation of a buffer already in the queue.
 */

int
klustsort(dp, bp, maxbcount)
struct diskhd *dp;
register struct buf *bp;
int maxbcount;
{
	register struct buf *ap;

	/*
	 * If nothing on the activity queue, then
	 * we become the only thing.
	 */
	ap = dp->b_actf;
	if (ap == NULL) {
		dp->b_actf = bp;
		dp->b_actl = bp;
		bp->av_forw = NULL;
		return (0);
	}

	/*
	 * Check to see whether the requested buffer is eligible
	 * to become a candidate for klustering.
	 */

	/*
	 * If we lie after the first (currently active)
	 * request, then we must locate the second request list
	 * and add ourselves to it.
	 */
	if (bp->b_cylin < ap->b_cylin) {
		while (ap->av_forw) {
			/*
			 * Check for an ``inversion'' in the
			 * normally ascending cylinder numbers,
			 * indicating the start of the second request list.
			 */
			if (ap->av_forw->b_cylin < ap->b_cylin) {
				/*
				 * Search the second request list
				 * for the first request at a larger
				 * cylinder number.  We go before that;
				 * if there is no such request, we go at end.
				 */
				do {
					if (bp->b_cylin < ap->av_forw->b_cylin)
						goto insert;
					ap = ap->av_forw;
				} while (ap->av_forw);
				goto insert;		/* after last */
			}
			ap = ap->av_forw;
		}
		/*
		 * No inversions... we will go after the last, and
		 * be the first request in the second request list.
		 */
		goto insert;
	}
	/*
	 * Request is at/after the current request...
	 * sort in the first request list.
	 */
	while (ap->av_forw) {
		/*
		 * We want to go after the current request
		 * if there is an inversion after it (i.e. it is
		 * the end of the first request list), or if
		 * the next request is a larger cylinder than our request.
		 */
		if (ap->av_forw->b_cylin < ap->b_cylin ||
		    bp->b_cylin < ap->av_forw->b_cylin)
			goto insert;
		ap = ap->av_forw;
	}
	/*
	 * Neither a second list nor a larger
	 * request... we go at the end of the first list,
	 * which is the same as the end of the whole schebang.
	 */
insert:
	/*
	 * See if we can kluster bp with ap
	 *
	 * Note that this will probably not kluster
	 * with any device that sorts by anything other
	 * than logical block number. Historically, the
	 * b_cylin field has been used to sort via to
	 * the granularity of cylinder number. However,
	 * in order to take advantage of putting together
	 * this one-way elevator sorting and checking for
	 * the opportunity to kluster up requests at the
	 * same time, we had to make some simplifying
	 * assumptions here. Therefore, if somebody
	 * calls klustsort() directly, it is assumed
	 * that if they have gone to the effort of
	 * stating that they wish to be eligible for
	 * kluster checking (by setting the maxbcount
	 * argument to nonzero), then they must use
	 * a sort token in b_resid (b_cylin) that
	 * matches the dkblock(bp) value.
	 */

	if (kluston && maxbcount != 0 && ap != dp->b_actf &&
	    (ap->b_dev == bp->b_dev) &&
	    ((bp->b_flags & (klust_buf_flag_chk|B_KLUSTER)) == B_PAGEIO) &&
	    ((ap->b_flags & klust_buf_flag_chk) == B_PAGEIO) &&
	    (ap->b_un.b_addr == (caddr_t) 0) &&
	    (bp->b_un.b_addr == (caddr_t) 0) &&
	    (((ap->b_bcount | bp->b_bcount) & PAGEOFFSET) == 0) &&
	    (ap->b_blkno + btodb(ap->b_bcount) == bp->b_blkno) &&
	    (ap->b_bcount + bp->b_bcount < maxbcount)) {
		if (klustadd(ap, bp) != 0) {
			return (1);
		}
	}

	bp->av_forw = ap->av_forw;
	ap->av_forw = bp;
	if (ap == dp->b_actl)
		dp->b_actl = bp;
	return (0);
}

/*
 * Add a new buffer to the passed kluster buf (if possible).
 * If this is a brand new kluster being started, find a kluster
 * structure and save the original starting buffer's b_bcount
 * tag in it (for later restoration upon i/o completion).
 * If we cannot find a free kluster structure, allocate another
 * one, but don't sweat it if there isn't any memory available.
 * Also limit ourselves to the very generous overall limit of
 * 128 kluster structures.
 *
 * Returns 1 if was able kluster, else 0.
 * Called only by klustsort().
 *
 */


static int
klustadd(bp, nbp)
register struct buf *bp, *nbp;
{
	register int s;
	register struct page *ppl, *nppl;
	register struct kluster *kp;

	s = splvm();
	if ((bp->b_flags & B_KLUSTER) == 0) {
		if ((kp = klustfree) == NULL) {
			if (nklusters >= KLUSTMAXPSIZE) {
				(void) splx(s);
				return (0);
			}
			klustfree = (struct kluster *)
			    new_kmem_zalloc(sizeof (*kp), KMEM_NOSLEEP);
			if ((kp = klustfree) == NULL) {
				(void) splx(s);
				return (0);
			}
			nklusters++;
		} else {
			klustfree = kp->klust_next;
		}

		klustfree = kp->klust_next;
		kp->klust_next = klustbusy;

		kp->klust_head = nbp;
		kp->klust_prime = bp;
		kp->klust_pcount = bp->b_bcount;

		klustbusy = kp;
		bp->b_flags |= B_KLUSTER;
	} else {
		for (kp = klustbusy; kp != NULL; kp = kp->klust_next) {
			if (kp->klust_prime == bp) {
				break;
			}
		}
		if (kp == NULL) {
			(void) splx(s);
			/*
			 * This should be a panic....
			 */
			return (0);
		}
		kp->klust_tail->av_forw = nbp;
	}

	kp->klust_tail = nbp;
	nbp->av_forw = 0;
	bp->b_bcount += nbp->b_bcount;

	ppl = bp->b_pages->p_prev;
	nppl = nbp->b_pages->p_prev;

	nppl->p_next = bp->b_pages;
	bp->b_pages->p_prev = nppl;
	ppl->p_next = nbp->b_pages;
	nbp->b_pages->p_prev = ppl;

	(void) splx(s);

	/*
	 * The av_back field of the buffer we are adding to the kluster
	 * chain saves the original last page pointer for the previous buffer.
	 */

	nbp->av_back = (struct buf *) ppl;

	return (1);
}

/*
 *
 * Bust apart a klustered set of buffers and
 * decommission the active kluster structure.
 *
 * Upon return from this function the argument
 * buffer passed will be the head of a forward
 * linked list of buffers that are the real
 * buffers that constituted the kluster.
 * The linkage is through the av_forw tag.
 */

void
klustbust(bp)
register struct buf *bp;
{
	register struct page *pp;
	struct page *first_pp_prev;
	register struct kluster *kp, *kpr;
	register int s;

	if ((bp->b_flags & B_KLUSTER) == 0) {
		bp->av_forw = (struct buf *) NULL;
		return;
	}

	kpr = (struct kluster *) NULL;
	s = splvm();

	kp = klustbusy;
	while (kp != (struct kluster *) NULL) {
		if (kp->klust_prime == bp)
			break;
		kpr = kp;
		kp = kp->klust_next;
	}

	if (kp == NULL) {
		(void) splx(s);
		bp->b_flags &= ~B_KLUSTER;
		bp->av_forw = (struct buf *) NULL;
		/*
		 * This should be a logged warning..
		 */
		return;
	}

	/*
	 * Restore original buffer's b_count field
	 * and point forward link at the chain of saved
	 * buffers that made up the rest of the kluster
	 */

	bp->b_bcount = kp->klust_pcount;
	bp->av_forw = kp->klust_head;

	/*
	 * Put the kluster structure back on the free list
	 */

	if (kpr) {
		kpr->klust_next = kp->klust_next;
	} else {
		klustbusy = kp->klust_next;
	}
	kp->klust_next = klustfree;
	klustfree = kp;
	(void) splx(s);

	bp->b_flags &= ~B_KLUSTER;

	/*
	 * If the action of doing I/O caused the buffer to
	 * be mapped in, map it back out again.
	 *
	 * We don't need to worry about it having been
	 * mapped in before hand because if it had been
	 * it wouldn't have been eligible for klustering
	 * to begin with.
	 */

	bp_mapout(bp);

	/*
	 * Walk the chain and bust out the pages and restore them to
	 * their original owners. The p_prev page for any given buffer
	 * (except the last one in the chain) had been saved in the
	 * *next* buffers' av_back field. The last buffer in the chain's
	 * bp->b_pages->p_prev is obviously the bp->b_pages->p_prev
	 * for the first buffer.
	 */

	first_pp_prev = bp->b_pages->p_prev;

	while (bp) {
		if (bp->av_forw) {
			pp = (struct page *) bp->av_forw->av_back;
		} else {
			pp = first_pp_prev;
		}
		pp->p_next = bp->b_pages;
		bp->b_pages->p_prev = pp;
		bp = bp->av_forw;
	}
}

/*
 * Break apart a kluster into its original set of
 * of buffers and call iodone. Return an integer
 * count of the number of buffers passed to iodone
 */

int
klustdone(bp)
register struct buf *bp;
{
	register struct buf *nbp;
	register i, err;

	/*
	 * If this doesn't appear to be a kluster buf, call
	 * iodone() anyhow for the buffer and return a
	 * count of 1 to say that one buf was passed to
	 * iodone().
	 */

	if ((bp->b_flags & B_KLUSTER) == 0) {
		iodone(bp);
		return (1);
	}

	/*
	 * Bust out the kluster chain and
	 * 'finish' off the chain of bufs
	 * that klustbust sets up.
	 *
	 * It is considered an error if a
	 * kluster operation finishes with
	 * a non-zero residual. In any
	 * case, if an error condition is
	 * set upon the kluster buf, it
	 * is propagated to all buffers.
	 * Further, we do not count that
	 * any i/o was done, period.
	 */

	err = ((bp->b_flags & B_ERROR) || bp->b_resid);

	klustbust(bp);

	i = 0;
	while (bp) {
		nbp = bp->av_forw;
		if (err) {
			bp->b_flags |= B_ERROR;
			bp->b_resid = bp->b_bcount;
		} else {
			bp->b_resid = 0;
		}
		bp->av_forw = bp->av_back = 0;
		iodone(bp);
		bp = nbp;
		i++;
	}
	return (i);
}
