#ifndef lint
static	char sccsid[] = "@(#) mb.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Mainbus support routines. These are mainly wrappers for the machine
 * dependent allocation routines.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vmmac.h>
#include <sys/vmmeter.h>
#include <sys/vmparam.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mman.h>

#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/reg.h>

#include <sundev/mbvar.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/page.h>
#include <vm/seg.h>
#include <machine/seg_kmem.h>

/*
 * Do transfer on controller argument.
 * We queue for resource wait in the Mainbus code if necessary.
 * We return 1 if the transfer was started, 0 if it was not.
 * If you call this routine with the head of the queue for a
 * Mainbus, it will automatically remove the controller from the Mainbus
 * queue before it returns.  If some other controller is given
 * as argument, it will be added to the request queue if the
 * request cannot be started immediately.  This means that
 * passing a controller which is on the queue but not at the head
 * of the request queue is likely to be a disaster.
 */
int
mbgo(mc)
	register struct mb_ctlr *mc;
{
	register struct mb_hd *mh = mc->mc_mh;
	register struct buf *bp = mc->mc_tab.b_actf->b_actf;
	register int s, mc_flags;

	mc_flags= mc->mc_driver->mdr_flags;
	s = splvm();
	if ((mc_flags & MDR_XCLU) && mh->mh_users > 0 || 
			mh->mh_xclu)
		goto rwait;
	mc->mc_mbinfo = mbsetup(mh, bp, (mc_flags & MDR_MAP_MSK) | MB_CANTWAIT);
	if (mc->mc_mbinfo == 0)
		goto rwait;
	mh->mh_users++;
	if (mc->mc_driver->mdr_flags & MDR_XCLU)
		mh->mh_xclu = 1;

	if (mh->mh_actf == mc)
		mh->mh_actf = mc->mc_forw;
	(void) splx(s);

	/*
	 * If we will have to byte swap, either now or upon i/o completion,
	 * make sure the buffer is mapped into the kernel's address space.
	 * We set up the mapping now, even if we won't have to swap until
	 * completion, because the bp_mapin call might sleep.
	 */
	if (mc->mc_driver->mdr_flags & MDR_SWAB) {
		bp_mapin(bp);

		/*
		 * Write requests must be swabbed now, as well as at
		 * completion time.
		 */
		if ((bp->b_flags & B_READ) == 0)
			swab(bp->b_un.b_addr, bp->b_un.b_addr,
			    (int)bp->b_bcount);
	}

	(*mc->mc_driver->mdr_go)(mc);
	return (1);

rwait:
	if (mh->mh_actf != mc) {
		mc->mc_forw = NULL;
		if (mh->mh_actf == NULL)
			mh->mh_actf = mc;
		else
			mh->mh_actl->mc_forw = mc;
		mh->mh_actl = mc;
	}
	(void) splx(s);
	return (0);
}

/*
 * Do a transfer on device argument.  This is used when a controller
 * can have multiple devices active at once.
 * We queue for resource wait in the Mainbus code if necessary.
 * We return 1 if the transfer was started, 0 if it was not.
 * If you call this routine with the head of the queue for its
 * controller, it will automatically remove the device from the
 * queue before it returns.  If some other device is given as argument,
 * it (and its controller if not already present) will be added to the
 * request queue if the request cannot be started immediately.  This means
 * that passing a device that is on the queue but not at the head is
 * likely to be a disaster.
 */
int
mbugo(md)
	register struct mb_device *md;
{
	register struct mb_hd *mh = md->md_hd;
	register struct buf *bp = md->md_utab.b_forw;
	register struct mb_ctlr *mc = md->md_mc;
	struct mb_ctlr *nmc;
	register int s, md_flags;

	md_flags= md->md_driver->mdr_flags;
	s = splvm();
	if ((md_flags & MDR_XCLU) && mh->mh_users || mh->mh_xclu)
		goto rwait;
	md->md_mbinfo = mbsetup(mh, bp, (md_flags & MDR_MAP_MSK) | MB_CANTWAIT);
	if (md->md_mbinfo == 0)
		goto rwait;
	mh->mh_users++;
	if (md->md_driver->mdr_flags & MDR_XCLU)
		mh->mh_xclu = 1;
	if (mc->mc_tab.b_forw == (struct buf *)md)
		mc->mc_tab.b_forw = (struct buf *)md->md_forw;
	(void) splx(s);

	/*
	 * If we will have to byte swap, either now or upon i/o completion,
	 * make sure the buffer is mapped into the kernel's address space.
	 * We set up the mapping now, even if we won't have to swap until
	 * completion, because the bp_mapin call might sleep.
	 */
	if (mc->mc_driver->mdr_flags & MDR_SWAB) {
		bp_mapin(bp);

		/*
		 * Write requests must be swabbed now, as well as at
		 * completion time.
		 */
		if ((bp->b_flags & B_READ) == 0)
			swab(bp->b_un.b_addr, bp->b_un.b_addr,
			    (int)bp->b_bcount);
	}

	(*md->md_driver->mdr_go)(md);
	return (1);

rwait:
	if (mc->mc_tab.b_forw != (struct buf *)md) {
		md->md_forw = NULL;
		if (mc->mc_tab.b_forw == NULL)
			mc->mc_tab.b_forw = (struct buf *)md;
		else
			((struct mb_device *)mc->mc_tab.b_back)->md_forw = md;
		mc->mc_tab.b_back = (struct buf *)md;
		/*
		 * If controller isn't in queue, must add it too.
		 */
		for (nmc = mh->mh_actf; nmc != NULL; nmc = nmc->mc_forw)
			if (nmc == mc)
				break;
		if (nmc == NULL) {
			mc->mc_forw = NULL;
			mc->mc_tab.b_actf = NULL;
			if (mh->mh_actf == NULL)
				mh->mh_actf = mc;
			else
				mh->mh_actl->mc_forw = mc;
			mh->mh_actl = mc;
		}
	}
	(void) splx(s);
	return (0);
}

void
mbdone(mc)
	register struct mb_ctlr *mc;
{
	register struct mb_hd *mh = mc->mc_mh;
	register struct buf *bp = mc->mc_tab.b_actf->b_actf;

	if (mc->mc_driver->mdr_flags & MDR_XCLU)
		mh->mh_xclu = 0;
	mh->mh_users--;
	mbrelse(mh, &mc->mc_mbinfo);
	if (mc->mc_driver->mdr_flags & MDR_SWAB) {
		/* We assume already mapped in in mbgo. */
		swab(bp->b_un.b_addr, bp->b_un.b_addr, (int)bp->b_bcount);
	}
	(*mc->mc_driver->mdr_done)(mc);
}

void
mbudone(md)
	register struct mb_device *md;
{
	register struct mb_hd *mh = md->md_hd;
	register struct buf *bp = md->md_utab.b_forw;

	if (md->md_driver->mdr_flags & MDR_XCLU)
		mh->mh_xclu = 0;
	mh->mh_users--;
	mbrelse(mh, &md->md_mbinfo);
	if (md->md_driver->mdr_flags & MDR_SWAB) {
		/* We assume already mapped in in mbugo. */
		swab(bp->b_un.b_addr, bp->b_un.b_addr, (int)bp->b_bcount);
	}
	(*md->md_driver->mdr_done)(md);
}

/*
 * This is a wrapper for mbugo, since each controller may have
 * multiple devices on the queue.  Only returns 1 if all devices
 * on the controller fit into the newly freed Mainbus space.
 */
int
mbuwrap(mc)
	register struct mb_ctlr *mc;
{

	while (mc->mc_tab.b_forw &&
	    mbugo((struct mb_device *)mc->mc_tab.b_forw))
		;
	if (mc->mc_tab.b_forw)
		return (0);
	else {
		/*
		 * Since all devices are dequeued,
		 * remove controller from queue too.
		 */
		mc->mc_mh->mh_actf = mc->mc_forw;
		return (1);
	}
}

int dopresetup = 1;

/*
 * Allocate DVMA resources and save them with the bp request.
 * This is used by the xy driver to allocate DVMA resources
 * when the request is queued, to avoid the overhead of doing
 * the allocation in the interrupt routine.
 */
void
mbpresetup(mh, bp, flags)
	struct mb_hd *mh;
	register struct buf *bp;
	int flags;
{

	if (!dopresetup)
		return;
	if (bp->b_mbinfo) {
		printf("mbpresetup: hasinfo %x?\n", bp->b_mbinfo);
		return;
	}
	bp->b_mbinfo = mbsetup(mh, bp, flags);
}

/*
 * Non buffer setup interface... set up a buffer and call mbsetup.
 */
int
mballoc(mh, addr, bcnt, flags)
	struct mb_hd *mh;
	caddr_t addr;
	int bcnt, flags;
{
	struct buf mbbuf;

	bzero((caddr_t)&mbbuf, sizeof (mbbuf));
	mbbuf.b_un.b_addr = addr;
	mbbuf.b_flags = B_BUSY;
	mbbuf.b_bcount = bcnt;
	return (mbsetup(mh, &mbbuf, flags));
}

/*
 * mb_nbmapalloc now in <sun/mb_machdep.c>
 */

/*
 * swab now in sun/mb_machdep.c
 */

/*
 * Run the wait queue for requestors that use the mb structures 
 * to queue for resources. Return DVMA_RUNOUT if there was no space
 * to satisfy the request.
 */
static int
run_mhq()
{
	register struct mb_hd *mh = &mb_hd;
	register int s;

	s = splvm();
	while (mh->mh_actf && (mh->mh_actf->mc_tab.b_actf == NULL ?
	    mbuwrap(mh->mh_actf) : mbgo(mh->mh_actf)))
		;
	(void) splx(s);
	return (mh->mh_actf ? DVMA_RUNOUT : 0);
}

/*
 * This routine is now a wrapper for mb_mapalloc() to setup mainbus
 * transfers. It is called by drivers that use the mb structures.
 */
int
mbsetup(mh, bp, flags)
	register struct mb_hd *mh;
	register struct buf *bp;
	int flags;
{
	register int s;
	/*
	 * If DVMA resources are already allocated for
	 * this buffer then return them now and forget
	 * that we preallocated them.
	 */
	if (bp->b_mbinfo) {
		s = bp->b_mbinfo;
		bp->b_mbinfo = 0;
		return (s);
	}
	return (mb_mapalloc(mh->mh_map, bp, flags, run_mhq, (caddr_t)NULL));
}

/*
 * This has been made a wrapper for mb_mapfree() to isolate the release 
 * of DVMA space from the use of the mb structs.
 */
void
mbrelse(mh, amr)
	register struct mb_hd *mh;
	int *amr;
{
	mb_mapfree(mh->mh_map, amr);
}

