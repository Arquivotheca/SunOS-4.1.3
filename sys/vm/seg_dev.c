/*	@(#)seg_dev.c 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1988, 1989 by Sun Microsystems, Inc.
 */

/*
 * VM - segment of a mapped device.
 *
 * This segment driver is used when mapping character special devices.
 */

#include <machine/pte.h>

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/systm.h>
#include <sys/errno.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_dev.h>
#include <vm/pvn.h>
#include <vm/vpage.h>

#define	vpgtob(n)	((n) * sizeof (struct vpage))	/* For brevity */

/*
 * Private seg op routines.
 */
static	int segdev_dup(/* seg, newsegp */);
static	int segdev_unmap(/* seg, addr, len */);
static	int segdev_free(/* seg */);
static	faultcode_t segdev_fault(/* seg, addr, len, type, rw */);
static	faultcode_t segdev_faulta(/* seg, addr */);
static	int segdev_hatsync(/* seg, addr, ref, mod, flags */);
static	int segdev_setprot(/* seg, addr, size, len */);
static	int segdev_checkprot(/* seg, addr, size, len */);
static	int segdev_badop();
static	int segdev_incore(/* seg, addr, size, vec */);
static	int segdev_ctlops(/* seg, addr, size, [flags] */);

struct	seg_ops segdev_ops = {
	segdev_dup,
	segdev_unmap,
	segdev_free,
	segdev_fault,
	segdev_faulta,
	segdev_hatsync,
	segdev_setprot,
	segdev_checkprot,
	segdev_badop,		/* kluster */
	(u_int (*)()) NULL,	/* swapout */
	segdev_ctlops,		/* sync */
	segdev_incore,
	segdev_ctlops,		/* lockop */
	segdev_ctlops,		/* advise */
};

/*
 * Create a device segment.
 */
int
segdev_create(seg, argsp)
	struct seg *seg;
	caddr_t argsp;
{
	register struct segdev_data *sdp;
	register struct segdev_crargs *a = (struct segdev_crargs *)argsp;

	sdp = (struct segdev_data *)
		new_kmem_alloc(sizeof (struct segdev_data), KMEM_SLEEP);
	sdp->mapfunc = a->mapfunc;
	sdp->dev = a->dev;
	sdp->offset = a->offset;
	sdp->prot = a->prot;
	sdp->maxprot = a->maxprot;
	sdp->pageprot = 0;
	sdp->vpage = NULL;

	seg->s_ops = &segdev_ops;
	seg->s_data = (char *)sdp;

	return (0);
}

/*
 * Duplicate seg and return new segment in newsegp.
 */
static int
segdev_dup(seg, newseg)
	struct seg *seg, *newseg;
{
	register struct segdev_data *sdp = (struct segdev_data *)seg->s_data;
	register struct segdev_data *newsdp;
	struct segdev_crargs a;

	a.mapfunc = sdp->mapfunc;
	a.dev = sdp->dev;
	a.offset = sdp->offset;
	a.prot = sdp->prot;
	a.maxprot = sdp->maxprot;

	(void) segdev_create(newseg, (caddr_t)&a);
	newsdp = (struct segdev_data *)newseg->s_data;
	newsdp->pageprot = sdp->pageprot;
	if (sdp->vpage != NULL) {
		register u_int nbytes = vpgtob(seg_pages(seg));
		newsdp->vpage = (struct vpage *)
					new_kmem_alloc(nbytes, KMEM_SLEEP);
		bcopy((caddr_t)sdp->vpage, (caddr_t)newsdp->vpage, nbytes);
	}
	return (0);
}

/*
 * Split a segment at addr for length len.
 */
/*ARGSUSED*/
static int
segdev_unmap(seg, addr, len)
	register struct seg *seg;
	register addr_t addr;
	u_int len;
{
	register struct segdev_data *sdp = (struct segdev_data *)seg->s_data;
	register struct segdev_data *nsdp;
	register struct seg *nseg;
	register u_int npages, spages, tpages;
	addr_t nbase;
	u_int nsize, hpages;

	/*
	 * Check for bad sizes
	 */
	if (addr < seg->s_base || addr + len > seg->s_base + seg->s_size ||
	    (len & PAGEOFFSET) || ((u_int)addr & PAGEOFFSET))
		panic("segdev_unmap");

	/*
	 * Unload any hardware translations in the range to be taken out.
	 */
	hat_unload(seg, addr, len);

	/*
	 * Check for entire segment
	 */
	if (addr == seg->s_base && len == seg->s_size) {
		seg_free(seg);
		return (0);
	}

	/*
	 * Check for beginning of segment
	 */
	spages = seg_pages(seg);
	npages = btop(len);
	if (addr == seg->s_base) {
		if (sdp->vpage != NULL) {
			sdp->vpage = (struct vpage *)new_kmem_resize(
				(caddr_t)sdp->vpage, vpgtob(npages),
				vpgtob(spages - npages), vpgtob(spages),
				KMEM_SLEEP);
		}
		sdp->offset += len;

		seg->s_base += len;
		seg->s_size -= len;
		return (0);
	}

	/*
	 * Check for end of segment
	 */
	if (addr + len == seg->s_base + seg->s_size) {
		tpages = spages - npages;
		if (sdp->vpage != NULL)
			sdp->vpage = (struct vpage *)
				new_kmem_resize((caddr_t)sdp->vpage, (u_int)0,
				vpgtob(tpages), vpgtob(spages), KMEM_SLEEP);
		seg->s_size -= len;
		return (0);
	}

	/*
	 * The section to go is in the middle of the segment,
	 * have to make it into two segments.  nseg is made for
	 * the high end while seg is cut down at the low end.
	 */
	nbase = addr + len;				/* new seg base */
	nsize = (seg->s_base + seg->s_size) - nbase;	/* new seg size */
	seg->s_size = addr - seg->s_base;		/* shrink old seg */
	nseg = seg_alloc(seg->s_as, nbase, nsize);
	if (nseg == NULL)
		panic("segdev_unmap seg_alloc");

	nseg->s_ops = seg->s_ops;
	nsdp = (struct segdev_data *)
		new_kmem_alloc(sizeof (struct segdev_data), KMEM_SLEEP);
	nseg->s_data = (char *)nsdp;
	nsdp->pageprot = sdp->pageprot;
	nsdp->prot = sdp->prot;
	nsdp->maxprot = sdp->maxprot;
	nsdp->mapfunc = sdp->mapfunc;
	nsdp->offset = sdp->offset + nseg->s_base - seg->s_base;

	if (sdp->vpage == NULL)
		nsdp->vpage = NULL;
	else {
		tpages = btop(nseg->s_base - seg->s_base);
		hpages = btop(addr - seg->s_base);

		nsdp->vpage = (struct vpage *)
			new_kmem_alloc(vpgtob(spages - tpages), KMEM_SLEEP);
		bcopy((caddr_t)&sdp->vpage[tpages], (caddr_t)nsdp->vpage,
			vpgtob(spages - tpages));
		sdp->vpage = (struct vpage *)
			new_kmem_resize((caddr_t)sdp->vpage, (u_int)0,
				vpgtob(hpages), vpgtob(spages), KMEM_SLEEP);
	}

	/*
	 * Now we do something so that all the translations which used
	 * to be associated with seg but are now associated with nseg.
	 */
	hat_newseg(seg, nseg->s_base, nseg->s_size, nseg);

	return (0);
}

/*
 * Free a segment.
 */
static
segdev_free(seg)
	struct seg *seg;
{
	register struct segdev_data *sdp = (struct segdev_data *)seg->s_data;
	register u_int nbytes = vpgtob(seg_pages(seg));

	if (sdp->vpage != NULL)
		kmem_free((caddr_t)sdp->vpage, nbytes);
	kmem_free((caddr_t)sdp, sizeof (*sdp));
}

/*
 * Handle a fault on a device segment.
 */
static faultcode_t
segdev_fault(seg, addr, len, type, rw)
	register struct seg *seg;
	addr_t addr;
	u_int len;
	enum fault_type type;
	enum seg_rw rw;
{
	register struct segdev_data *sdp = (struct segdev_data *)seg->s_data;
	register addr_t adr;
	register u_int prot, protchk;
	int pf;
	struct vpage *vpage;

	if (type == F_PROT) {
		/*
		 * Since the seg_dev driver does not implement copy-on-write,
		 * this means that a valid translation is already loaded,
		 * but we got an fault trying to access the device.
		 * Return an error here to prevent going in an endless
		 * loop reloading the same translation...
		 */
		return (FC_PROT);
	}

	if (type != F_SOFTUNLOCK) {
		if (sdp->pageprot == 0) {
			switch (rw) {
			case S_READ:
				protchk = PROT_READ;
				break;
			case S_WRITE:
				protchk = PROT_WRITE;
				break;
			case S_EXEC:
				protchk = PROT_EXEC;
				break;
			case S_OTHER:
			default:
				protchk = PROT_READ | PROT_WRITE | PROT_EXEC;
				break;
			}
			prot = sdp->prot;
			if ((prot & protchk) == 0)
				return (FC_PROT);
			vpage = NULL;
		} else {
			vpage = &sdp->vpage[seg_page(seg, addr)];
		}
	}

	for (adr = addr; adr < addr + len; adr += PAGESIZE) {
		if (type == F_SOFTUNLOCK) {
			hat_unlock(seg, adr);
			continue;
		}

		if (vpage != NULL) {
			switch (rw) {
			case S_READ:
				protchk = PROT_READ;
				break;
			case S_WRITE:
				protchk = PROT_WRITE;
				break;
			case S_EXEC:
				protchk = PROT_EXEC;
				break;
			case S_OTHER:
			default:
				protchk = PROT_READ | PROT_WRITE | PROT_EXEC;
				break;
			}
			prot = vpage->vp_prot;
			vpage++;
			if ((prot & protchk) == 0)
				return (FC_PROT);
		}

		pf = (*sdp->mapfunc)(sdp->dev,
		    sdp->offset + (adr - seg->s_base), prot);
		if (pf == -1)
			return (FC_MAKE_ERR(EFAULT));

		hat_devload(seg, adr, pf, prot, type == F_SOFTLOCK);
	}

	return (0);
}

/*
 * Asynchronous page fault.  We simply do nothing since this
 * entry point is not supposed to load up the translation.
 */
/*ARGSUSED*/
static faultcode_t
segdev_faulta(seg, addr)
	struct seg *seg;
	addr_t addr;
{

	return (0);
}

/*ARGSUSED*/
static
segdev_hatsync(seg, addr, ref, mod, flags)
	struct seg *seg;
	addr_t addr;
	u_int ref, mod;
	u_int flags;
{

	/* cannot use ref and mod bits on devices, so ignore 'em */
}

static int
segdev_setprot(seg, addr, len, prot)
	register struct seg *seg;
	register addr_t addr;
	register u_int len, prot;
{
	register struct segdev_data *sdp = (struct segdev_data *)seg->s_data;
	register struct vpage *vp, *evp;

	if ((sdp->maxprot & prot) != prot)
		return (-1);				/* violated maxprot */

	if (addr == seg->s_base && len == seg->s_size && sdp->pageprot == 0) {
		if (sdp->prot == prot)
			return (0);			/* all done */
		sdp->prot = prot;
	} else {
		sdp->pageprot = 1;
		if (sdp->vpage == NULL) {
			/*
			 * First time through setting per page permissions,
			 * initialize all the vpage structures to prot
			 */
			sdp->vpage = (struct vpage *)new_kmem_zalloc(
					vpgtob(seg_pages(seg)), KMEM_SLEEP);
			evp = &sdp->vpage[seg_pages(seg)];
			for (vp = sdp->vpage; vp < evp; vp++)
				vp->vp_prot = sdp->prot;
		}
		/*
		 * Now go change the needed vpages protections.
		 */
		evp = &sdp->vpage[seg_page(seg, addr + len)];
		for (vp = &sdp->vpage[seg_page(seg, addr)]; vp < evp; vp++)
			vp->vp_prot = prot;
	}

	if (prot == 0)
		hat_unload(seg, addr, len);
	else
		hat_chgprot(seg, addr, len, prot);
	return (0);
}

static int
segdev_checkprot(seg, addr, len, prot)
	register struct seg *seg;
	register addr_t addr;
	register u_int len, prot;
{
	struct segdev_data *sdp = (struct segdev_data *)seg->s_data;
	register struct vpage *vp, *evp;

	/*
	 * If segment protection can be used, simply check against them
	 */
	if (sdp->pageprot == 0)
		return (((sdp->prot & prot) != prot) ? -1 : 0);

	/*
	 * Have to check down to the vpage level
	 */
	evp = &sdp->vpage[seg_page(seg, addr + len)];
	for (vp = &sdp->vpage[seg_page(seg, addr)]; vp < evp; vp++)
		if ((vp->vp_prot & prot) != prot)
			return (-1);

	return (0);
}

static
segdev_badop()
{

	panic("segdev_badop");
	/*NOTREACHED*/
}

/*
 * segdev pages are not in the cache, and thus can't really be controlled.
 * syncs, locks, and advice are simply always successful.
 */
/*ARGSUSED*/
static int
segdev_ctlops(seg, addr, len, flags)
	struct seg *seg;
	addr_t addr;
	u_int len, flags;
{

	return (0);
}

/*
 * segdev pages are always "in core".
 */
/*ARGSUSED*/
static int
segdev_incore(seg, addr, len, vec)
	struct seg *seg;
	addr_t addr;
	register u_int len;
	register char *vec;
{
	u_int v = 0;

	for (len = (len + PAGEOFFSET) & PAGEMASK; len; len -= PAGESIZE,
	    v += PAGESIZE)
		*vec++ = 1;
	return (v);
}
