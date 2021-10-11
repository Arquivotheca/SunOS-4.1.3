#ifndef lint
static	char sccsid[] = "@(#)str_buf.c 1.1 92/07/30 SMI";
/*
 * from S5R3.1 io/stream.c 10.14.1.1,
 * with lots of stuff added from io/stream.c 1.23
 */
#endif

/*LINTLIBRARY*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/strstat.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/vmmac.h>

#include <machine/pte.h>

#include <vm/hat.h>
#include <vm/seg.h>
#include <machine/seg_kmem.h>

/*
 * queue scheduling control variables
 */
char	qrunflag;		/* set iff queues are enabled */
char	queueflag;		/* set iff inside queuerun() */

dblk_t	*dbfreelist;		/* data block freelist */
mblk_t	*mbfreelist;		/* message block freelist */

struct	stdata *stream_free;	/* list of available streams */
struct	stdata *allstream;	/* list of active streams */
struct	queue *queue_free;	/* list of available queue pairs */
struct	queue *qhead;		/* head of queues to run */
struct	queue *qtail;		/* last queue */

struct strevent	*sefreelist;	/* stream event cell freelist */

struct	strstat strst;		/* Streams statistics structure */
alcdat	*strbufstat;		/* Streams buffer allocation statistics */
unsigned int nstrbufsz;		/* number of entries in strbufsizes, */
				/*   including the terminating 0 entry */

char	strbcwait;		/* kmem_free schedules queues when set */
char	strbcflag;		/* bufcall functions ready to go */
struct	bclist strbcalls;	/* pending bufcall events */


/*
 * Mblk and dblk allocation.
 *
 * The SVR4 code places no bounds on the number of outstanding mblks and
 * dblks.  When an mblk or dblk is freed, it is simply placed on a free list;
 * there's no attempt to give space back to the system.  Stated differently,
 * the code uses a kmem_fast_alloc/kmem_fast_free-like strategy.
 *
 * For now, we simply use kmem_fast_{alloc,free}, but we should consider other
 * strategies that don't tie down memory for this specific purpose and/or that
 * place bounds on the amount of memory dedicated to this purpose.
 */

mblk_t	*xmballoc();
dblk_t	*xdballoc();

#ifdef	notdef
/* SVR4 code */
/*
 * free macros for data and message blocks
 */
#define	DBFREE(dp) { \
	(dp)->db_freep = dbfreelist; \
	dbfreelist = (dp); \
	strst.dblock.use--; \
}
#define	MBFREE(mp) { \
	(mp)->b_next = mbfreelist; \
	mbfreelist = (mp); \
	strst.mblock.use--; \
}

/*
 * These two macros must be protected by splstr()-splx().
 * They are ugly, but improve performance and isolate the details of
 * allocation in one place.  If we can't allocate off the freelist,
 * have to call a function to get more space.
 */
#define	MBALLOC(mp, slpflg)	if (mbfreelist) { \
					mp = mbfreelist; \
					mbfreelist = mp->b_next; \
					mp->b_next = NULL; \
					BUMPUP(strst.mblock); \
				} \
				else \
					mp = xmballoc(slpflg);
#define	DBALLOC(dp, slpflg)	if (dbfreelist) { \
					dp = dbfreelist; \
					dbfreelist = dp->db_freep; \
					dp->db_freep = NULL; \
					BUMPUP(strst.dblock); \
				} \
				else \
					dp = xdballoc(slpflg);
#else	notdef
#define	MBFREE(mp) { \
	kmem_fast_free(&(caddr_t)mbfreelist, (caddr_t)mp); \
	strst.mblock.use--; \
}
#define	DBFREE(dp) { \
	kmem_fast_free(&(caddr_t)dbfreelist, (caddr_t)dp); \
	strst.dblock.use--; \
}

#define	MBALLOC(mp, slpflg)	mp = xmballoc(slpflg);
#define	DBALLOC(dp, slpflg)	dp = xdballoc(slpflg);
#endif	notdef

/*
 * Allocate a message and data block.
 * Tries to get a block large enough to hold 'size' bytes.
 * A message block and data block are allocated together, initialized,
 * and a pointer to the message block returned.  Data blocks are
 * always allocated in association with a message block, but there
 * may be several message blocks per data block (see dupb).
 * If no message blocks or data blocks (of the required size)
 * are available, NULL is returned.
 *
 * 'pri' is no longer used, but is retained for compatibility.
 */
/* ARGSUSED */
mblk_t *
allocb(size, pri)
	register int size;
	int pri;
{
	register dblk_t *databp;
	register mblk_t *bp;
	register int s;
	register int tmpsiz;
	register u_char *buf = NULL;
	register alcdat *bufstat;

	/*
	 * if size >= 0, get buffer.
	 *
	 * if size < 0, don't bother with buffer, just get data
	 * and message block.
	 *
	 * if size > 0, but <= DB_CACHESIZE, don't allocate buffer -- use
	 * cache at end of data block instead.
	 *
	 * Set bufstat to point to the slot in strbufstat corresponding to the
	 * size allocated.  Note that we assume that the number of distinct
	 * slots is small enough that linear search is appropriate, and that
	 * the slots are ordered by increasing size.
	 */
	tmpsiz = size;
	if (tmpsiz >= 0) {
		if (tmpsiz > DB_CACHESIZE) {
			register int i = 0;

			buf = (u_char *)
				new_kmem_alloc((u_int)tmpsiz, KMEM_NOSLEEP);
			bufstat = strbufstat + 2;
			while (i < nstrbufsz && tmpsiz > strbufsizes[i++])
				bufstat++;
			if (buf == NULL) {
				bufstat->fail++;
				return (NULL);
			}
		} else {
			/*
			 * DB_CACHESIZE must be >= 4 bytes.  Old code may
			 * rely on getting a min buffer size of four bytes
			 * even though requesting less (e.g. zero).
			 */
			tmpsiz = DB_CACHESIZE;
			bufstat = strbufstat + 1;
		}
	} else
		bufstat = strbufstat;

	s = splstr();

	/*
	 * The data buffer is in hand, try for a data block.
	 */
	DBALLOC(databp, SE_NOSLP);
	if (databp == NULL) {
		if (tmpsiz > DB_CACHESIZE)
			kmem_free((caddr_t)buf, (u_int)tmpsiz);
		(void) splx(s);
		bufstat->fail++;
		return (NULL);
	}

	/*
	 * and for a message block.
	 */
	MBALLOC(bp, SE_NOSLP);
	if (bp == NULL) {
		DBFREE(databp);
		if (tmpsiz > DB_CACHESIZE)
			kmem_free((caddr_t)buf, (u_int)tmpsiz);
		(void) splx(s);
		bufstat->fail++;
		return (NULL);
	}

	(void) splx(s);

	if (tmpsiz > 0 && tmpsiz <= DB_CACHESIZE)
		buf = databp->db_cache;

	/*
	 * initialize message block and
	 * data block descriptors
	 */
	bp->b_next = NULL;
	bp->b_prev = NULL;
	bp->b_cont = NULL;
	bp->b_datap = databp;
	databp->db_type = M_DATA;
	/*
	 * set reference count to 1 (first use)
	 */
	databp->db_ref = 1;

	if (tmpsiz > 0) {
		databp->db_base = buf;
		databp->db_size = tmpsiz;
		databp->db_lim = buf + tmpsiz;
		bp->b_rptr = buf;
		bp->b_wptr = buf;
	} else
		databp->db_size = -1;

	/*
	 * Gather allocation statistics.
	 */
	BUMPUP(*bufstat);

	return (bp);
}

/*
 * Allocate a data block.
 *
 * XXX:	Need a more intelligent allocation policy.  For now, punt and simply
 *	use kmem_fast_alloc, never giving anything back.
 */
/* ARGSUSED */
dblk_t *
xdballoc(slpflag)
	int slpflag;
{
#define	DBLK_INCR	32		/* XXX */
	register dblk_t *dp;
	register int s = splstr();

	dp = (dblk_t *)new_kmem_fast_alloc(&(caddr_t)dbfreelist, sizeof *dp,
		DBLK_INCR, KMEM_NOSLEEP);
	if (dp) {
		dp->db_freep = NULL;
		BUMPUP(strst.dblock);
	} else {
		/*
		 * This should occur only when kmem_alloc's pool of available
		 * memory is totally exhausted.  If we never allocated at
		 * interrupt level, this wouldn't be a problem...
		 *
		 * XXX:	There's the potential for heap big trouble here.  We
		 *	need what we just failed to allocate in order to print
		 *	this message on the console.
		 */
		strst.dblock.fail++;
		log(LOG_ERR, "xdballoc: out of dblks\n");
	}

	(void) splx(s);
	return (dp);
}

/*
 * allocate a message block
 *
 * XXX:	Need a more intelligent allocation policy.  For now, punt and simply
 *	use kmem_fast_alloc, never giving anything back.
 */
/* ARGSUSED */
mblk_t *
xmballoc(slpflag)
	int slpflag;
{
#define	MBLK_INCR	32		/* XXX */
	register mblk_t *mp;
	register int s = splstr();

	mp = (mblk_t *)new_kmem_fast_alloc(&(caddr_t)mbfreelist, sizeof *mp,
		MBLK_INCR, KMEM_NOSLEEP);
	if (mp) {
		mp->b_next = NULL;
		BUMPUP(strst.mblock);
	} else {
		/*
		 * This should occur only when kmem_alloc's pool of available
		 * memory is totally exhausted.  If we never allocated at
		 * interrupt level, this wouldn't be a problem...
		 *
		 * XXX:	There's the potential for heap big trouble here.  We
		 *	need what we just failed to allocate in order to print
		 *	this message on the console.
		 */
		strst.mblock.fail++;
		log(LOG_ERR, "xmballoc: out of mblks\n");
	}

	(void) splx(s);
	return (mp);
}

/*
 * Allocate a class zero data block and attach an externally-supplied
 * data buffer pointed to by base to it.
 */
/* ARGSUSED */
mblk_t *
esballoc(base, size, pri, fr_rtn)
	unsigned char *base;
	int size;
	int pri;
	frtn_t *fr_rtn;
{
	register mblk_t *mp;

	if (!base || !fr_rtn)
		return (NULL);
	if (!(mp = allocb(-1, pri)))
		return (NULL);

	mp->b_datap->db_frtnp = fr_rtn;
	mp->b_datap->db_base = base;
	mp->b_datap->db_lim = base + size;
	mp->b_datap->db_size = size;
	mp->b_rptr = base;
	mp->b_wptr = base;
	return (mp);
}

/*
 * Call function 'func' with 'arg' when a class zero block can
 * be allocated with priority 'pri'.
 */
/* ARGSUSED */
int
esbbcall(pri, func, arg)
	int pri;
	int (*func)();
	long arg;
{
	register int s;
	struct strevent *sep;

	/*
	 * allocate new stream event to add to linked list
	 */
	if (!(sep = sealloc(SE_NOSLP))) {
		log(LOG_ERR, "esbbcall: could not allocate stream event\n");
		return (0);
	}

	sep->se_next = NULL;
	sep->se_func = func;
	sep->se_arg = arg;
	sep->se_size = -1;

	s = splstr();
	/*
	 * add newly allocated stream event to existing
	 * linked list of events.
	 */
	if (strbcalls.bc_head == NULL) {
		strbcalls.bc_head = strbcalls.bc_tail = sep;
	} else {
		strbcalls.bc_tail->se_next = sep;
		strbcalls.bc_tail = sep;
	}
	(void) splx(s);

#ifdef	notdef
	/*
	 * XXX:	This appears in bufcall; why not here?  Probably because any
	 *	amount of memory should satisfy the request, so that having
	 *	kmem_alloc goose the STREAMS scheduler won't do any particular
	 *	good.
	 */
	strbcwait = 1;	/* so kmem_free() will know to call setqsched() */
#endif	notdef
	return ((int)sep);
}

/*
 * test if block of given size can be allocated with a request of
 * the given priority.
 * 'pri' is no longer used, but is retained for compatibility.
 */
/* ARGSUSED */
int
testb(size, pri)
	register int size;
	uint pri;
{
	return (size <= kmem_avail());
}


/*
 * Call function 'func' with argument 'arg' when there is a reasonably
 * good chance that a block of size 'size' can be allocated.
 * 'pri' is no longer used, but is retained for compatibility.
 */
/* ARGSUSED */
int
bufcall(size, pri, func, arg)
	uint size;
	int pri;
	int (*func)();
	long arg;
{
	register int s;
	struct strevent *sep;

	ASSERT(size >= 0);

	if (!(sep = sealloc(SE_NOSLP))) {
		log(LOG_ERR, "bufcall: could not allocate stream event\n");
		return (0);
	}
	sep->se_next = NULL;
	sep->se_func = func;
	sep->se_arg = arg;
	sep->se_size = size;

	s = splstr();
	/*
	 * add newly allocated stream event to existing
	 * linked list of events.
	 */
	if (strbcalls.bc_head == NULL) {
		strbcalls.bc_head = strbcalls.bc_tail = sep;
	} else {
		strbcalls.bc_tail->se_next = sep;
		strbcalls.bc_tail = sep;
	}
	(void) splx(s);

	strbcwait = 1;	/* so kmem_free() will know to call setqsched() */
	return ((int)sep);
}

/*
 * Cancel a bufcall request.
 */
void
unbufcall(id)
	register int id;
{
	register int s;
	register struct strevent *sep;
	register struct strevent *psep;

	s = splstr();
	psep = NULL;
	for (sep = strbcalls.bc_head; sep; sep = sep->se_next) {
		if (id == (int)sep)
			break;
		psep = sep;
	}
	if (sep) {
		if (psep)
			psep->se_next = sep->se_next;
		else
			strbcalls.bc_head = sep->se_next;
		if (sep == strbcalls.bc_tail)
			strbcalls.bc_tail = psep;
		sefree(sep);
	}
	(void) splx(s);
}

/*
 * Free a message block and decrement the reference count on its
 * data block. If reference count == 0 also return the data block.
 */
freeb(bp)
	register mblk_t *bp;
{
	register int s;
	register struct datab *dbp;

	ASSERT(bp);

	s = splstr();
	dbp = bp->b_datap;
	ASSERT(dbp->db_ref > 0);
	if (--dbp->db_ref == 0) {
		register alcdat *bufstat;

		if (dbp->db_frtnp == NULL) {
			if (dbp->db_size > DB_CACHESIZE) {
				register int	i = 0;

				/*
				 * Find the allocation statistics slot
				 * associated with the buffer.
				 */
				bufstat = strbufstat + 2;
				while (i < nstrbufsz &&
					    dbp->db_size > strbufsizes[i++])
					bufstat++;
				kmem_free((caddr_t)dbp->db_base, dbp->db_size);
			} else
				bufstat = strbufstat + 1;
		} else {
			/*
			 * no data buffer (extended STREAMS buffer)
			 * if there is a value for free_func
			 * then
			 *	call free_func (passing any arguments),
			 *	set db_base and db_lim to NULL
			 */
			if (dbp->db_frtnp->free_func) {
				(*dbp->db_frtnp->free_func) (
					dbp->db_frtnp->free_arg);
				dbp->db_base = NULL;
				dbp->db_lim = NULL;
				dbp->db_size = -1;
			bufstat = strbufstat;
			}
		}
		DBFREE(dbp);
		bufstat->use--;
	}
	bp->b_datap = NULL;
	MBFREE(bp);
	(void) splx(s);
	return;
}


/*
 * Free all message blocks in a message using freeb().
 * The message may be NULL.
 */

freemsg(bp)
	register mblk_t *bp;
{
	register mblk_t *tp;
	register int s;

	s = splstr();
	while (bp) {
		tp = bp->b_cont;
		freeb(bp);
		bp = tp;
	}
	(void) splx(s);
}


/*
 * Duplicate a message block
 *
 * Allocate a message block and assign proper
 * values to it (read and write pointers)
 * and link it to existing data block.
 * Increment reference count of data block.
 */

mblk_t *
dupb(bp)
	register mblk_t *bp;
{
	register s;
	register mblk_t *nbp;

	ASSERT(bp);

	s = splstr();
	MBALLOC(nbp, SE_NOSLP);
	if (nbp == NULL) {
		(void) splx(s);
		return (NULL);
	}
	(nbp->b_datap = bp->b_datap)->db_ref++;
	(void) splx(s);

	nbp->b_next = NULL;
	nbp->b_prev = NULL;
	nbp->b_cont = NULL;
	nbp->b_rptr = bp->b_rptr;
	nbp->b_wptr = bp->b_wptr;
	return (nbp);
}


/*
 * Duplicate a message block by block (uses dupb), returning
 * a pointer to the duplicate message.
 * Returns a non-NULL value only if the entire message
 * was dup'd.
 */

mblk_t *
dupmsg(bp)
	register mblk_t *bp;
{
	register mblk_t *head, *nbp;
	register int s;

	if (!bp || !(nbp = head = dupb(bp)))
		return (NULL);

	s = splstr();
	while (bp->b_cont) {
		if (!(nbp->b_cont = dupb(bp->b_cont))) {
			freemsg(head);
			(void) splx(s);
			return (NULL);
		}
		nbp = nbp->b_cont;
		bp = bp->b_cont;
	}
	(void) splx(s);
	return (head);
}



/*
 * Copy data from message block to newly allocated message block and
 * data block.  The copy is rounded out to full word boundaries so that
 * the (usually) more efficient word copy can be done.
 * Returns new message block pointer, or  NULL if error.
 */

mblk_t *
copyb(bp)
	register mblk_t *bp;
{
	register mblk_t *nbp;
	register dblk_t *dp, *ndp;
	caddr_t base;

	ASSERT(bp);
	ASSERT(bp->b_wptr >= bp->b_rptr);

	dp = bp->b_datap;
	if (!(nbp = allocb(dp->db_lim - dp->db_base, BPRI_MED)))
		return (NULL);
	ndp = nbp->b_datap;
	ndp->db_type = dp->db_type;
	nbp->b_rptr = ndp->db_base + (bp->b_rptr - dp->db_base);
	nbp->b_wptr = ndp->db_base + (bp->b_wptr - dp->db_base);
	base = straln(nbp->b_rptr);
	strbcpy(straln(bp->b_rptr), base,
		straln(nbp->b_wptr + (sizeof (int) - 1)) - base);
	return (nbp);
}


/*
 * Copy data from message to newly allocated message using new
 * data blocks.  Returns a pointer to the new message, or NULL if error.
 */
mblk_t *
copymsg(bp)
	register mblk_t *bp;
{
	register mblk_t *head, *nbp;
	register int s;

	if (!bp || !(nbp = head = copyb(bp)))
		return (NULL);

	s = splstr();
	while (bp->b_cont) {
		if (!(nbp->b_cont = copyb(bp->b_cont))) {
			freemsg(head);
			(void) splx(s);
			return (NULL);
		}
		nbp = nbp->b_cont;
		bp = bp->b_cont;
	}
	(void) splx(s);
	return (head);
}


/*
 * link a message block to tail of message
 */

linkb(mp, bp)
	register mblk_t *mp;
	register mblk_t *bp;
{
	register int s;

	ASSERT(mp && bp);

	s = splstr();
	for (; mp->b_cont; mp = mp->b_cont)
		continue;
	mp->b_cont = bp;
	(void) splx(s);
}


/*
 * unlink a message block from head of message
 * return pointer to new message.
 * NULL if message becomes empty.
 */

mblk_t *
unlinkb(bp)
	register mblk_t *bp;
{
	register mblk_t *bp1;
	register int s;

	ASSERT(bp);

	s = splstr();
	bp1 = bp->b_cont;
	bp->b_cont = NULL;
	(void) splx(s);
	return (bp1);
}


/*
 * remove a message block "bp" from message "mp"
 *
 * Return pointer to new message or NULL if no message remains.
 * Return -1 if bp is not found in message.
 */
mblk_t *
rmvb(mp, bp)
	register mblk_t *mp;
	register mblk_t *bp;
{
	register mblk_t *tmp;
	register mblk_t *lastp = NULL;
	register int s;

	s = splstr();
	for (tmp = mp; tmp; tmp = tmp->b_cont) {
		if (tmp == bp) {
			if (lastp)
				lastp->b_cont = tmp->b_cont;
			else
				mp = tmp->b_cont;
			tmp->b_cont = NULL;
			(void) splx(s);
			return (mp);
		}
		lastp = tmp;
	}
	(void) splx(s);
	return ((mblk_t *)-1);
}



/*
 * macro to check pointer alignment
 * (true if alignment is sufficient for worst case)
 */

#define	str_aligned(X)	(((uint)(X) & (sizeof (int) - 1)) == 0)

/*
 * Concatenate and align first len bytes of common
 * message type.  Len == -1, means concat everything.
 * Returns 1 on success, 0 on failure
 * After the pullup, mp points to the pulled up data.
 * This is convenient but messy to implement.
 */
int
pullupmsg(mp, len)
	mblk_t *mp;
	register int len;
{
	register mblk_t *bp;
	register mblk_t *new_bp;
	register n;
	mblk_t *tmp;
	int s;

	ASSERT(mp != NULL);

	/*
	 * Quick checks for success or failure:
	 */
	if (len == -1) {
		if (mp->b_cont == NULL && str_aligned(mp->b_rptr))
			return (1);
		len = xmsgsize(mp);
	} else {
		ASSERT(mp->b_wptr >= mp->b_rptr);
		if (mp->b_wptr - mp->b_rptr >= len && str_aligned(mp->b_rptr))
			return (1);
		if (xmsgsize(mp) < len)
			return (0);
	}

	/*
	 * Allocate a new mblk header.  It is used later to interchange
	 * mp and new_bp.
	 */
	s = splstr();
	MBALLOC(tmp, SE_NOSLP);
	if (tmp == NULL) {
		(void) splx(s);
		return (0);
	}
	(void) splx(s);

	/*
	 * Allocate the new mblk.  We might be able to use the existing
	 * mblk, but we don't want to modify it in case its shared.
	 * The new dblk takes on the type of the old dblk
	 */
	if ((new_bp = allocb(len, BPRI_MED)) == NULL) {
		s = splstr();
		MBFREE(tmp);
		(void) splx(s);
		return (0);
	}
	new_bp->b_datap->db_type = mp->b_datap->db_type;

	/*
	 * Scan mblks and copy over data into the new mblk.
	 * Two ways to fall out: exact count match: while (len)
	 * Bp points to the next mblk containing data or is null.
	 * Inexact match: if (bp->b_rptr != ...)  In this case,
	 * bp points to an mblk that still has data in it.
	 */
	bp = mp;
	while (len && bp) {
		mblk_t *b_cont;

		ASSERT(bp->b_wptr >= bp->b_rptr);
		n = MIN(bp->b_wptr - bp->b_rptr, len);
		bcopy((caddr_t)bp->b_rptr, (caddr_t)new_bp->b_wptr, (u_int)n);
		new_bp->b_wptr += n;
		bp->b_rptr += n;
		len -= n;
		if (bp->b_rptr != bp->b_wptr)
			break;
		b_cont = bp->b_cont;
		if (bp != mp)	/* don't free the head mblk */
			freeb(bp);
		bp = b_cont;
	}

	/*
	 * At this point:  new_bp points to a dblk that
	 * contains the pulled up data.  The head mblk, mp, is
	 * preserved and may or may not have data in it.  The
	 * intermediate mblks are freed, and bp points to the
	 * last mblk that was pulled-up or is null.
	 *
	 * Now the tricky bit.  After this, mp points to the new dblk
	 * and tmp points to the old dblk.  New_bp points nowhere
	 */
	*tmp = *mp;
	*mp = *new_bp;
	new_bp->b_datap = NULL;

	/*
	 * If the head mblk (now tmp) still has data in it, link it to mp
	 * otherwise link the remaining mblks to mp and free the
	 * old head mblk.
	 */
	if (tmp->b_rptr != tmp->b_wptr)
		mp->b_cont = tmp;
	else {
		mp->b_cont = bp;
		freeb(tmp);
	}

	/*
	 * Free new_bp
	 */
	s = splstr();
	MBFREE(new_bp);
	(void) splx(s);

	return (1);
}

/*
 * Trim bytes from message
 *  len > 0, trim from head
 *  len < 0, trim from tail
 * Returns 1 on success, 0 on failure.
 */
int
adjmsg(mp, len)
	mblk_t *mp;
	register int len;
{
	register mblk_t *bp;
	register n;
	int fromhead;

	ASSERT(mp != NULL);

	fromhead = 1;
	if (len < 0) {
		fromhead = 0;
		len = -len;
	}
	if (xmsgsize(mp) < len)
		return (0);

	if (fromhead) {
		bp = mp;
		while (len && bp) {
			ASSERT(bp->b_wptr >= bp->b_rptr);
			n = MIN(bp->b_wptr - bp->b_rptr, len);
			bp->b_rptr += n;
			len -= n;
			bp = bp->b_cont;
		}
	} else {
		register mblk_t *save_bp;
		register unsigned char type;

		type = mp->b_datap->db_type;
		while (len) {
			/*
			 * Find last nonempty block of the proper type,
			 * recording it in save_bp.
			 */
			save_bp = NULL;
			bp = mp;
			while (bp && bp->b_datap->db_type == type) {
				ASSERT(bp->b_wptr >= bp->b_rptr);
				if (bp->b_wptr - bp->b_rptr > 0)
					save_bp = bp;
				bp = bp->b_cont;
			}
			if (!save_bp)
				break;
			/*
			 * Trim it as required.
			 */
			n = MIN(save_bp->b_wptr - save_bp->b_rptr, len);
			save_bp->b_wptr -= n;

			len -= n;
		}
	}
	return (1);
}

/*
 * Return size of message of block type (bp->b_datap->db_type)
 */
int
xmsgsize(bp)
	register mblk_t *bp;
{
	register unsigned char type;
	register count = 0;
	register int s;

	type = bp->b_datap->db_type;

	s = splstr();
	for (; bp; bp = bp->b_cont) {
		if (type != bp->b_datap->db_type)
			break;
		ASSERT(bp->b_wptr >= bp->b_rptr);
		count += bp->b_wptr - bp->b_rptr;
	}
	(void) splx(s);
	return (count);
}


/*
 * get number of data bytes in message
 */
int
msgdsize(bp)
	register mblk_t *bp;
{
	register int count = 0;
	register int s;

	s = splstr();
	for (; bp; bp = bp->b_cont)
		if (bp->b_datap->db_type == M_DATA) {
			ASSERT(bp->b_wptr >= bp->b_rptr);
			count += bp->b_wptr - bp->b_rptr;
		}
	(void) splx(s);
	return (count);
}


/*
 * Get a message off head of queue
 *
 * If queue has no buffers then mark queue
 * with QWANTR. (queue wants to be read by
 * someone when data becomes available)
 *
 * If there is something to take off then do so.
 * If queue falls below hi water mark turn off QFULL
 * flag.  Decrement weighted count of queue.
 * Also turn off QWANTR because queue is being read.
 *
 * If queue count is below the lo water mark and QWANTW
 * is set, enable the closest backq which has a service
 * procedure and turn off the QWANTW flag.
 *
 * getq could be built on top of rmvq, but isn't because
 * of performance considerations.
 */
mblk_t *
getq(q)
	register queue_t *q;
{
	register mblk_t *bp;
	register mblk_t *tmp;
	register int s;

	ASSERT(q);

	s = splstr();
	if (!(bp = q->q_first))
		q->q_flag |= QWANTR;
	else {
		if (!(q->q_first = bp->b_next))
			q->q_last = NULL;
		else
			q->q_first->b_prev = NULL;
		for (tmp = bp; tmp; tmp = tmp->b_cont)
			q->q_count -= (tmp->b_wptr - tmp->b_rptr);
		if (q->q_count < q->q_hiwat)
			q->q_flag &= ~QFULL;
		q->q_flag &= ~QWANTR;
		bp->b_next = bp->b_prev = NULL;
	}

	if (q->q_count <= q->q_lowat && (q->q_flag & QWANTW)) {
		q->q_flag &= ~QWANTW;
		/* find nearest back queue with service proc */
		for (q = backq(q); q && !q->q_qinfo->qi_srvp; q = backq(q))
			continue;
		if (q)
			qenable(q);
	}
	(void) splx(s);
	return (bp);
}


/*
 * Remove a message from a queue.  The queue count and other
 * flow control parameters are adjusted and the back queue
 * enabled if necessary.
 */
rmvq(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register int s;
	register mblk_t *tmp;

	ASSERT(q);
	ASSERT(mp);

	s = splstr();

	if (mp->b_prev)
		mp->b_prev->b_next = mp->b_next;
	else
		q->q_first = mp->b_next;

	if (mp->b_next)
		mp->b_next->b_prev = mp->b_prev;
	else
		q->q_last = mp->b_prev;

	mp->b_next = mp->b_prev = NULL;

	for (tmp = mp; tmp; tmp = tmp->b_cont)
		q->q_count -= (tmp->b_wptr - tmp->b_rptr);

	if (q->q_count < q->q_hiwat)
		q->q_flag &= ~QFULL;

	if (q->q_count <= q->q_lowat && (q->q_flag & QWANTW)) {
		q->q_flag &= ~QWANTW;
		/* find nearest back queue with service proc */
		for (q = backq(q); q && !q->q_qinfo->qi_srvp; q = backq(q))
			continue;
		if (q)
			qenable(q);
	}
	(void) splx(s);
}

/*
 * Empty a queue.
 * If flag is set, remove all messages.  Otherwise, remove
 * only non-control messages.  If queue falls below its low
 * water mark, and QWANTW was set before the flush, enable
 * the nearest upstream service procedure.
 */

flushq(q, flag)
	register queue_t *q;
	int flag;
{
	register mblk_t *mp, *nmp;
	register int s;
	int wantw;

	ASSERT(q);

	s = splstr();

	wantw = q->q_flag & QWANTW;

	mp = q->q_first;
	q->q_first = NULL;
	q->q_last = NULL;
	q->q_count = 0;
	q->q_flag &= ~(QFULL|QWANTW);
	while (mp) {
		nmp = mp->b_next;
		if (!flag && !datamsg(mp->b_datap->db_type))
			putq(q, mp);
		else
			freemsg(mp);
		mp = nmp;
	}
	if ((q->q_count <= q->q_lowat) && wantw) {
		/* find nearest back queue with service proc */
		for (q = backq(q); q && !q->q_qinfo->qi_srvp; q = backq(q))
			continue;
		if (q)
			qenable(q);
	} else
		q->q_flag |= wantw;

	(void) splx(s);
}


/*
 * Return the space in the next queueing queue.
 * Analogous to canput() in that it looks for a service procedure.
 */
int
qhiwat(q)
	queue_t *q;
{
	if (!q)
		return (0);
	while (q->q_next && !q->q_qinfo->qi_srvp)
		q = q->q_next;
	return (q->q_hiwat);
}


/*
 * Return the amount of space left in the next queueing queue.
 * Analogous to canput() in that it looks for a service procedure.
 */
int
qspace(q)
	queue_t *q;
{
	if (!q)
		return (0);
	while (q->q_next && !q->q_qinfo->qi_srvp)
		q = q->q_next;
	if (q->q_flag & QFULL)
		return (0);
	return (q->q_hiwat - q->q_count);
}


/*
 * Return 1 if the queue is not full.  If the queue is full, return
 * 0 (may not put message) and set QWANTW flag (caller wants to write
 * to the queue).
 */
int
canput(q)
	register queue_t *q;
{
	register int s;

	if (!q)
		return (0);

	s = splstr();
	while (q->q_next && !q->q_qinfo->qi_srvp)
		q = q->q_next;
	if (q->q_flag & QFULL) {
		q->q_flag |= QWANTW;
		(void) splx(s);
		return (0);
	}
	(void) splx(s);
	return (1);
}


/*
 * Return 1 if the queue is not full past the extra amount allowed for
 * kernel printfs.
 * If the queue is full past that point, return 0.
 */
int
canputextra(q)
	register queue_t *q;
{
	register int s;

	if (!q)
		return (0);

	s = splstr();
	while (q->q_next && !q->q_qinfo->qi_srvp)
		q = q->q_next;
	if (q->q_count >= q->q_hiwat + 200) {
		(void) splx(s);
		return (0);
	}
	(void) splx(s);
	return (1);
}


/*
 * Put a message on a queue.
 *
 * Messages are enqueued on a priority basis.  The priority classes
 * are HIGH PRIORITY (type >= QPCTL) and NORMAL (type < QPCTL).
 *
 * Add data block sizes to queue count.
 * If queue hits high water mark then set QFULL flag.
 *
 * If QNOENB is not set (putq is allowed to enable the queue),
 * enable the queue only if the message is PRIORITY,
 * or the QWANTR flag is set (indicating that the service procedure
 * is ready to read the queue.  This implies that a service
 * procedure must NEVER put a priority message back on its own
 * queue, as this would result in an infinite loop (!).
 */

putq(q, bp)
	register queue_t *q;
	register mblk_t *bp;
{
	register int s;
	register mblk_t *tmp;
	register int mcls = (int)queclass(bp);

	ASSERT(q && bp);

	s = splstr();

	/*
	 * If queue is empty or queue class of message is less than
	 * that of the last one on the queue, tack on to the end.
	 */
	if (!q->q_first || (mcls <= (int)queclass(q->q_last))) {
		if (q->q_first) {
			q->q_last->b_next = bp;
			bp->b_prev = q->q_last;
		} else {
			q->q_first = bp;
			bp->b_prev = NULL;
		}
		bp->b_next = NULL;
		q->q_last = bp;
	} else {
		register mblk_t *nbp = q->q_first;

		while ((int)queclass(nbp) >= mcls)
			nbp = nbp->b_next;
		bp->b_next = nbp;
		bp->b_prev = nbp->b_prev;
		if (nbp->b_prev)
			nbp->b_prev->b_next = bp;
		else
			q->q_first = bp;
		nbp->b_prev = bp;
	}

	for (tmp = bp; tmp; tmp = tmp->b_cont)
		q->q_count += (tmp->b_wptr - tmp->b_rptr);
	if (q->q_count >= q->q_hiwat)
		q->q_flag |= QFULL;

	if (mcls > QNORM || (canenable(q) && (q->q_flag & QWANTR)))
		qenable(q);

	(void) splx(s);
}


/*
 * Put stuff back at beginning of Q according to priority order.
 * See comment on putq above for details.
 */
putbq(q, bp)
	register queue_t *q;
	register mblk_t *bp;
{
	register int s;
	register mblk_t *tmp;
	register int mcls = (int)queclass(bp);

	ASSERT(q && bp);
	ASSERT(bp->b_next == NULL);

	s = splstr();

	/*
	 * If queue is empty or queue class of message >= that of the
	 * first message, place on the front of the queue.
	 */
	if (!q->q_first || (mcls >= (int)queclass(q->q_first))) {
		bp->b_next = q->q_first;
		bp->b_prev = NULL;
		if (q->q_first)
			q->q_first->b_prev = bp;
		else
			q->q_last = bp;
		q->q_first = bp;
	} else {
		register mblk_t *nbp = q->q_first;

		while ((nbp->b_next) && ((int)queclass(nbp->b_next) > mcls))
			nbp = nbp->b_next;

		if (bp->b_next = nbp->b_next)
			nbp->b_next->b_prev = bp;
		else
			q->q_last = bp;
		nbp->b_next = bp;
		bp->b_prev = nbp;
	}

	for (tmp = bp; tmp; tmp = tmp->b_cont)
		q->q_count += (tmp->b_wptr - tmp->b_rptr);
	if (q->q_count >= q->q_hiwat)
		q->q_flag |= QFULL;

	if (mcls > QNORM || (canenable(q) && (q->q_flag & QWANTR)))
		qenable(q);

	(void) splx(s);
}


/*
 * Insert a message before an existing message on the queue.  If the
 * existing message is NULL, the new messages is placed on the end of
 * the queue.  The queue class of the new message is ignored.
 * All flow control parameters are updated.
 */
insq(q, emp, mp)
	register queue_t *q;
	register mblk_t *emp;
	register mblk_t *mp;
{
	register mblk_t *tmp;
	register int s;

	s = splstr();
	if (mp->b_next = emp) {
		if (mp->b_prev = emp->b_prev)
			emp->b_prev->b_next = mp;
		else
			q->q_first = mp;
		emp->b_prev = mp;
	} else {
		if (mp->b_prev = q->q_last)
			q->q_last->b_next = mp;
		else
			q->q_first = mp;
		q->q_last = mp;
	}

	for (tmp = mp; tmp; tmp = tmp->b_cont)
		q->q_count += (tmp->b_wptr - tmp->b_rptr);

	if (q->q_count >= q->q_hiwat)
		q->q_flag |= QFULL;

	if (canenable(q) && (q->q_flag & QWANTR))
		qenable(q);

	(void) splx(s);
}


/*
 * Create and put a control message on queue.
 */
int
putctl(q, type)
	queue_t *q;
{
	register mblk_t *bp;

	if (datamsg(type) || !(bp = allocb(0, BPRI_HI)))
		return (0);
	bp->b_datap->db_type = type;
	(*q->q_qinfo->qi_putp)(q, bp);
	return (1);
}


/*
 * Control message with a single-byte parameter
 */
int
putctl1(q, type, param)
	queue_t *q;
{
	register mblk_t *bp;

	if (datamsg(type) ||!(bp = allocb(1, BPRI_HI)))
		return (0);
	bp->b_datap->db_type = type;
	*bp->b_wptr++ = param;
	(*q->q_qinfo->qi_putp)(q, bp);
	return (1);
}


/*
 * Init routine run from main at boot time.
 */
strinit()
{
	extern int	stream_init;	/* XXX: should be in include file */
	extern int	queue_init;	/* XXX: should be in include file */
	register int	i;
	register int	size;

	/*
	 * Set up initial stream event cell free list.  sealloc()
	 * may allocate new cells for the free list if the initial list is
	 * exhausted (processor dependent).
	 *
	 * N.B.: The SVR4 code relies entirely on sealloc's ability to
	 * dynamically grab more memory.  We don't adopt it here because the
	 * routine grabs overly large increments.  Ultimately, the routine
	 * should be made more reasonable in its resource demands.
	 */
	sefreelist = NULL;
	for (i = 0; i < nstrevent; i++) {
		strevent[i].se_next = sefreelist;
		sefreelist = &strevent[i];
	}

	/*
	 * Grab space for stdata, queue, dblk, and mblk structures.  The
	 * allocations here are for initial amounts that should be adequate
	 * for "light" use.  If any of these amounts turn out to be
	 * inadequate, the corresponding allocations routine will (potentially
	 * repeatedly) ask for incremental storage to satisfy demand.
	 *
	 * Note that none of the storage so obtained is ever returned.
	 *
	 * The technique we use is to force an initial allocation that
	 * implicitly allocates the amount of space we desire, and then
	 * free what we just got, leaving all the entires on the free list for
	 * the data structure at hand.
	 *
	 * XXX:	Either move the two defines below to param.c or (preferably)
	 *	get rid of them altogether.  The same comment applies to
	 *	stream_init and queue_init as well.
	 */
#define	DBLK_INIT	64	/* XXX */
#define	MBLK_INIT	64	/* XXX */
	kmem_fast_free(&(caddr_t)stream_free, kmem_fast_alloc(
		&(caddr_t)stream_free, sizeof (struct stdata), stream_init));
	kmem_fast_free(&(caddr_t)queue_free, kmem_fast_alloc(
		&(caddr_t)queue_free, 2 * sizeof (queue_t), queue_init));
	kmem_fast_free(&(caddr_t)dbfreelist, kmem_fast_alloc(
		&(caddr_t)dbfreelist, sizeof (dblk_t), DBLK_INIT));
	kmem_fast_free(&(caddr_t)mbfreelist, kmem_fast_alloc(
		&(caddr_t)mbfreelist, sizeof (mblk_t), MBLK_INIT));

	/*
	 * Allocate an array of alcdat structures for keeping track of buffer
	 * allocation statistics.  This array has two entries at the beginning
	 * for external and within-dblk buffers, then has one element for each
	 * entry in the strbufsizes array defined in conf.c, and finally has
	 * an element for allocations larger than the maximum value mentioned
	 * in strbufsizes.
	 *
	 * Note that this code assumes that the entries in strbufsizes are
	 * monotonically increasing except for a terminal entry of zero.
	 */
	nstrbufsz = 0;
	size = 0;
	while (size < strbufsizes[nstrbufsz++])
		size = strbufsizes[nstrbufsz - 1];
	strbufstat = (alcdat *)new_kmem_alloc(
			(2 + nstrbufsz) * sizeof *strbufstat, KMEM_SLEEP);
	if (strbufstat == NULL)
		panic("strinit: no space for strbufstat");
	bzero((caddr_t)strbufstat, (2 + nstrbufsz) * sizeof *strbufstat);
}



/*
 * Allocate a stream head.  The caller is responsible for initializing
 * all fields in the stream head.
 */
struct stdata *
allocstr()
{
	extern int	stream_incr;
	register int s;
	register struct stdata *stp;

	s = splstr();
	stp = (struct stdata *)new_kmem_fast_alloc(&(caddr_t)stream_free,
		sizeof (struct stdata), stream_incr, KMEM_NOSLEEP);
	if (stp) {
		BUMPUP(strst.stream);
		if ((stp->sd_next = allstream) != NULL) {
			/*
			 * The list of active streams is not empty; link the
			 * previous head of that list back to the new head of
			 * the list, namely this stream.
			 */
			allstream->sd_prev = stp;
		}
		allstream = stp;
		stp->sd_prev = NULL;
	} else {
		/*
		 * This should occur only when kmem_alloc's pool
		 * of available memory is totally exhausted.  If
		 * this actually happens, we have worse problems
		 * than the one we complain about here...
		 */
		strst.stream.fail++;
		log(LOG_ERR, "allocstr: out of streams\n");
	}
	(void) splx(s);
	return (stp);
}

void
freestr(stp)
	register struct stdata *stp;
{
	register int	s = splstr();

	if (stp->sd_prev == NULL) {
		/*
		 * This stream is at the head of the list of active streams.
		 * Make its successor in that list the new head; if there is
		 * such a successor, make its predecessor NULL since it is now
		 * the new head.
		 */
		if ((allstream = stp->sd_next) != NULL)
			stp->sd_next->sd_prev = NULL;
	} else {
		/*
		 * This stream is not at the head of the list of active
		 * streams.  If it has a successor, make its predecessor be its
		 * successor's predecessor.  In any case, make its sucessor be
		 * its predecessor's successor.
		 */
		if (stp->sd_next != NULL)
			stp->sd_next->sd_prev = stp->sd_prev;
		stp->sd_prev->sd_next = stp->sd_next;
	}
	kmem_fast_free(&(caddr_t)stream_free, (caddr_t)stp);
	strst.stream.use--;

	(void) splx(s);
}




/*
 * allocate a pair of queues
 */

queue_t *
allocq()
{
	extern int	queue_incr;
	static queue_t	zeroR = {
		NULL, NULL, NULL, NULL, NULL, NULL, 0, QUSE|QREADR, 0, 0, 0, 0
	};
	static queue_t	zeroW = {
		NULL, NULL, NULL, NULL, NULL, NULL, 0, QUSE, 0, 0, 0, 0
	};
	register int s;
	register queue_t *qp;

	s = splstr();
	qp = (queue_t *) new_kmem_fast_alloc(&(caddr_t)queue_free,
		2 * sizeof (queue_t), queue_incr, KMEM_NOSLEEP);
	if (qp) {
		*qp = zeroR;
		*WR(qp) = zeroW;
		BUMPUP(strst.queue);
	} else {
		/*
		 * This should occur only when kmem_alloc's pool
		 * of available memory is totally exhausted.  If
		 * this actually happens, we have worse problems
		 * than the one we complain about here...
		 */
		strst.queue.fail++;
		log(LOG_ERR, "allocq: out of queues\n");
	}
	(void) splx(s);
	return (qp);
}




/*
 * return the queue upstream from this one
 */

queue_t *
backq(q)
	register queue_t *q;
{
	ASSERT(q);

	q = OTHERQ(q);
	if (q->q_next) {
		q = q->q_next;
		return (OTHERQ(q));
	}
	return (NULL);
}



/*
 * Send a block back up the queue in reverse from this
 * one (e.g. to respond to ioctls)
 */

qreply(q, bp)
	register queue_t *q;
	mblk_t *bp;
{
	ASSERT(q && bp);

	q = OTHERQ(q);
	ASSERT(q->q_next);
	(*q->q_next->q_qinfo->qi_putp)(q->q_next, bp);
}



/*
 * Streams Queue Scheduling
 *
 * Queues are enabled through qenable() when they have messages to
 * process.  They are serviced by queuerun(), which runs each enabled
 * queue's service procedure.  The call to queuerun() is processor
 * dependent - the general principle is that it be run whenever a queue
 * is enabled but before returning to user level.  For system calls,
 * the function runqueues() is called if their action causes a queue
 * to be enabled.  For device interrupts, queuerun() should be
 * called before returning from the last level of interrupt.  Beyond
 * this, no timing assumptions should be made about queue scheduling.
 */

/*
 * Enable a queue: put it on list of those whose service procedures are
 * ready to run and set up the scheduling mechanism.
 */
qenable(q)
	register queue_t *q;
{
	register int s;

	ASSERT(q);

	if (!q->q_qinfo->qi_srvp)
		return;

	s = splstr();
	/*
	 * Do not place on run queue if already enabled.
	 */
	if (q->q_flag & QENAB) {
		(void) splx(s);
		return;
	}

	/*
	 * mark queue enabled and place on run list
	 */
	q->q_flag |= QENAB;

	if (!qhead)
		qhead = q;
	else
		qtail->q_link = q;
	qtail = q;
	q->q_link = NULL;

	/*
	 * set up scheduling mechanism
	 */
	setqsched();
	(void) splx(s);
}


/*
 * Run the service procedures of each enabled queue
 *	-- must not be reentered
 *
 * Called by service mechanism (processor dependent) if there
 * are queues to run.  The mechanism is reset.
 */
queuerun()
{
	register queue_t *q;
	register int s;
	register int count;
	register struct strevent *sep;
	register struct strevent **sepp;

	s = splstr();
	do {
		if (strbcflag) {
			strbcwait = strbcflag = 0;

			/*
			 * Get estimate of available memory from kmem_avail().
			 * Awake all bufcall functions waiting for memory
			 * whose request could be satisfied by 'count' memory
			 * and let 'em fight for it.
			 *
			 * The loop below handles list deletions by keeping a
			 * pointer to the link field to the current request
			 * and overwriting it when removing an entry.  The
			 * list tail pointer is recalculated from scratch
			 * while traversing the list by keeping track of the
			 * latest surviving request.
			 */
			count = kmem_avail();
			for (sepp = &strbcalls.bc_head; sep = *sepp; ) {
				if (sep->se_size <= count) {
					/*
					 * Can satisfy this request: do it,
					 * and then unlink the request from
					 * the list and free it.
					 */
					(*sep->se_func)(sep->se_arg);
					*sepp = sep->se_next;
					sefree(sep);
				} else {
					/*
					 * Since we can't satisfy the request
					 * this time around, we must reset
					 * strbcwait to arrange to try again
					 * later.
					 */
					strbcwait = 1;
					/*
					 * Advance to next entry; update tail.
					 */
					strbcalls.bc_tail = sep;
					sepp = &sep->se_next;
				}
			}
			if (strbcalls.bc_head == NULL)
				strbcalls.bc_tail = NULL;
		}

		while (q = qhead) {
			if (!(qhead = q->q_link))
				qtail = NULL;
			q->q_flag &= ~QENAB;
			if (q->q_qinfo->qi_srvp) {
				(void) spl1();
				(*q->q_qinfo->qi_srvp)(q);
				(void) splstr();
			}
		}
	} while (strbcflag);

	qrunflag = 0;
	(void) splx(s);
}

/*
 * Function to kick off queue scheduling for those system calls
 * that cause queues to be enabled (read, recv, write, send, ioctl).
 */

runqueues()
{
	register int s;

	if (!qrunflag)
		return;		/* nothing to do */
	if (queueflag)
		return;		/* actively being run */
#ifdef	MULTIPROCESSOR
	if (!klock_knock())
		return;
#endif	MULTIPROCESSOR
	s = splhigh();
	if (qrunflag && !queueflag) { /* be very sure. */
		queueflag = 1;
		(void) splx(s);		/* yes, run it at low priority */
		queuerun();
		queueflag = 0;
	} else {
		(void) splx(s);
	}
}



/*
 * find module
 *
 * return index into fmodsw
 * or -1 if not found
 */
int
findmod(name)
	register char *name;
{
	register int i, j;

	for (i = 0; i < fmodcnt; i++)
		for (j = 0; j < FMNAMESZ + 1; j++) {
			if (fmodsw[i].f_name[j] != name[j])
				break;
			if (name[j] == '\0')
				return (i);
		}
	return (-1);
}



/*
 * return number of messages on queue
 */
int
qsize(qp)
	register queue_t *qp;
{
	register count = 0;
	register mblk_t *mp;

	ASSERT(qp);

	for (mp = qp->q_first; mp; mp = mp->b_next)
		count++;

	return (count);
}


/*
 * allocate a stream event cell
 */
struct strevent *
sealloc(slpflag)
	int slpflag;
{
	register s;
	register struct strevent *sep;
	int i;
	static sepgcnt = 0;
	u_long a;

retry:
	s = splstr();
	if (sefreelist) {
		sep = sefreelist;
		sefreelist = sep->se_next;
		(void) splx(s);
		sep->se_procp = NULL;
		sep->se_events = 0;
		sep->se_next = NULL;
		return (sep);
	}

	if (sepgcnt >= maxsepgcnt)
		goto fail3;
	sepgcnt++;

	/*
	 * Allocate a new page of memory, overlay an event cell array on
	 * it and add it to the linked list of free cells.
	 */
	a = rmalloc(kernelmap, 1L);
	if (a == 0)
		goto fail2;
	sep = (struct strevent *)kmxtob(a);
	if (segkmem_alloc(&kseg, (addr_t)sep, CLBYTES,
	    (slpflag == SE_NOSLP)? 0 : 1, SEGKMEM_STREAMS) == 0)
		goto fail1;

	(void) splx(s);
	bzero((caddr_t)sep, CLBYTES);
	for (i = 0; i < (CLBYTES / sizeof (struct strevent)); i++, sep++) {
		s = splstr();
		sep->se_next = sefreelist;
		sefreelist = sep;
		(void) splx(s);
	}
	goto retry;

fail1:
	rmfree(kernelmap, 1L, a);
fail2:
	sepgcnt--;
fail3:
	(void) splx(s);
	return (NULL);
}


/*
 * free a stream event cell
 */
sefree(sep)
	struct strevent *sep;
{
	register s;

	s = splstr();
	sep->se_next = sefreelist;
	sefreelist = sep;
	(void) splx(s);
}
