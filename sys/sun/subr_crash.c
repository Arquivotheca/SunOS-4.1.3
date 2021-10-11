#ifndef lint
static	char sccsid[] = "@(#)subr_crash.c 1.1 92/07/30 SMI";
#endif lint

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vm.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/dumphdr.h>
#include <sys/bootconf.h>
#include <debug/debug.h>
#include <sun/fault.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/page.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>
#include <vm/seg_u.h>
#include <vm/anon.h>

#ifdef sun3x
#include <machine/cpu.h>
#endif
#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/seg_kmem.h>

struct dumphdr	*dumphdr;
char	*dump_bitmap;		/* bitmap */
int	dump_max = 0;		/* maximum dumpsize (pages) */
int	dump_size;		/* current dumpsize (pages) */
int	dump_allpages = 0;	/* if 1, dump everything */
int	dump_dochecksetbit = 1;	/* XXX debugging */

#define	MAX_PANIC	100	/* Maximum panic string we'll copy */

dump_addpage(i)
	u_int	i;
{
	int	chunksize = dumphdr->dump_chunksize;

	if ((dump_size < dump_max) &&
	    isclr(dump_bitmap, (i) / (chunksize))) {
		if (dump_dochecksetbit)
			dump_checksetbit((int) (i));
		dump_size += (chunksize);
		setbit(dump_bitmap, (i) / (chunksize));
	}
}

dump_checksetbit(i)
	int	i;
{
	static int	dump_checksetbit_error = 0;
	u_int	addr;

	addr = mmu_ptob(i);
	if (dump_checksetbit_machdep(addr))
		goto okay;
	dump_checksetbit_error++;
	printf("bad pfn=%d (0x%x); addr=0x%x\n",
		i, i, addr);
	if (dump_checksetbit_error == 1)	/* only trace on first error */
		tracedump();
okay:
	/* void */;
}

/*
 * Initialize the dump header; called by dumpinit() in os/init_main.c
 */
dumphdr_init()
{
	struct dumphdr	*dp;
	int		size;
	extern char	*strcpy();

	/* The "+1"'s are for the ending null bytes */
	size = sizeof (struct dumphdr) + strlen(version) + 1 +
		MAX_PANIC + 1;
	dumphdr = dp = (struct dumphdr *)new_kmem_zalloc(
				(u_int) size, KMEM_NOSLEEP);
	if (dp == NULL)
		panic("dumphdr_init: no space for dumphdr");
	dp->dump_magic = DUMP_MAGIC;
	dp->dump_version = DUMP_VERSION;
	dp->dump_pagesize = PAGESIZE;
	/*
	 * Compute the chunksize so that the bitmap will fit on a page
	 * Physmax is the highest page number (set by startup).
	 * (size * NBBY) is how many pages each byte of the bitmap
	 * represents.
	 */
	for (size = 1; size * NBBY * PAGESIZE <= physmax; size *= 2)
		/* void */;
	dp->dump_chunksize = size;
	dp->dump_bitmapsize = physmax / (size * NBBY) + 1;
	dp->dump_versionoff = sizeof (*dp);
	(void) strcpy(dump_versionstr(dp), version);
	dp->dump_panicstringoff = dp->dump_versionoff + strlen(version) + 1;
	dp->dump_headersize = dp->dump_panicstringoff + MAX_PANIC + 1;

	/* Now allocate space for bitmap */
	dump_bitmap = new_kmem_zalloc((u_int)dp->dump_bitmapsize, KMEM_NOSLEEP);
	if (dump_bitmap == NULL)
		panic("dumphdr_init: no space for bitmap");

	if (dump_max == 0 || dump_max > btop(dbtob(dumpfile.bo_size)))
		dump_max = btop(dbtob(dumpfile.bo_size));
	dump_max = (dump_max / dp->dump_chunksize) * dp->dump_chunksize;
	dump_max -= 2 + howmany(dp->dump_bitmapsize, dp->dump_pagesize);
	if (dump_max < 0) {
		printf("Warning: dump_max value inhibits crash dumps.\n");
		dump_max = 0;
	}
	dump_size = 0;

	/* Now do machine-specific initialization */
	dumphdr_machinit(dp);

	{
		/*
		 * We mark all the pages below the first page covered by
		 * a page structure.
		 * This gets us the
		 * This is about the only way to get the pages used for
		 * proc[0]'s u area.  (Page 0, and the pages after end).
		 * XXX We assume that the first memory chunk contains
		 * XXX the first page covered by a page structure.
		 * XXX We also assume the first physical page is page 0.
		 */
		struct memseg	*memseg;
		int	lomem;
		int	lophys;
		u_int	i;

		memseg = memsegs;
		lophys = 0;
		lomem = memseg->pages_base;
		for (i = lophys; i < lomem; i++)
			dump_addpage(i);
	}
}

/*
 * Dump the system.
 * Only dump "interesting" chunks of memory, preceeded by a header and a
 * bitmap.
 * The header describes the dump, and the bitmap inidicates which chunks
 * have been dumped.
 */
dumpsys()
{
	register int	 i, n;
	struct dumphdr	*dp = dumphdr;
	char *strncpy();
	int	chunksize;
	int	size;
	struct as	*as;
	struct seg	*seg, *sseg;
	struct proc	*p;
	struct memseg	*msp;
	struct page	*pp;
	u_int	pfn;
	int	error = 0;
	label_t	*saved_jb, jb;

	/*
	 * dumphdr not initialized yet...
	 */
	if (!dp)
		return;
	/*
	 * While we still have room (dump_max is positive), mark pages
	 * that belong to the following:
	 * 1. Kernel (vnode == 0 and free == 0 and gone == 0)
	 * 2. Grovel through segmap looking for pages
	 * 3. Grovel through proc array, getting users' stack
	 *    segments.
	 * We don't want to panic again if data structures are invalid,
	 * so we use nofault (can't use on_fault(); see bugid 1028320).
	 */
	dp->dump_flags = DF_VALID;
	chunksize = dp->dump_chunksize;

	/* Account for pre-allocated pages */
	printf("%5d low-memory static kernel pages\n", dump_size);
	size = dump_size;

	/* Dump kernel segkmem pages */
	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		extern struct seg_ops	segkmem_ops;

		sseg = seg = kas.a_segs;
		if (seg)
			do {
				if (seg->s_ops != &segkmem_ops)
					continue;
				dump_segkmem(seg);
			} while ((seg = seg->s_next) != sseg);

		printf("%5d additional static and sysmap kernel pages\n",
			dump_size - size);
		size = dump_size;
	} else {
		printf("failure dumping kernel static/sysmap pages: seg=0x%x\n",
			seg);
		error = 1;
	}

	/* Mark all pages in use by kernel */
	if (!setjmp(&jb)){
		for (msp = memsegs; msp; msp = msp->next) {
			for (pp = msp->pages; pp < msp->epages; pp++) {
				if (pp->p_vnode || pp->p_free || pp->p_gone)
					continue;
				pfn = page_pptonum(pp);
				dump_addpage(pfn);
			}
		}

		printf("%5d dynamic kernel data pages\n", dump_size - size);
		size = dump_size;
	} else {
		printf(
	"failure dumping kernel heap: memseg=0x%x pp=0x%x pfn=0x%x\n",
			msp, pp, pfn);
		error = 1;
	}

	/* Dump all the user structures */
	if (!setjmp(&jb)){
		for (p = allproc; p; p = p->p_nxt)
			dump_segu(p->p_segu);

		printf("%5d additional user structure pages\n",
			dump_size - size);
		size = dump_size;
		if (error == 0 && dump_size < dump_max)
			dp->dump_flags |= DF_COMPLETE;
	} else {
		printf("failure dumping user struct: proc=0x%x\n", p);
	}

	/* Dump pages in use by segmap */
	if (!setjmp(&jb)) {
		dump_segmap(segkmap);

		printf("%5d segmap kernel pages\n", dump_size - size);
		size = dump_size;
	} else {
		printf("failure dumping segkmap\n");
	}

	/* Dump kernel segvn pages */
	if (!setjmp(&jb)) {
		sseg = seg = kas.a_segs;
		if (seg)
			do {
				if (seg->s_ops != &segvn_ops)
					continue;
				dump_segvn(seg);
			} while ((seg = seg->s_next) != sseg);

		printf("%5d segvn kernel pages\n", dump_size - size);
		size = dump_size;
	} else {
		printf("failure dumping kernel segvn 0x%x\n", seg);
	}

	/* Dump current user process */
	if (!setjmp(&jb)) {
		as = u.u_procp->p_as;
		if (as && (sseg = seg = as->a_segs))
			do {
				if (seg->s_ops != &segvn_ops)
					continue;
				dump_segvn(seg);
			} while ((seg = seg->s_next) != sseg);

		printf("%5d current user process pages\n", dump_size - size);
		size = dump_size;
	} else {
		printf(
		    "failure dumping current user process: as=0x%x seg=0x%x\n",
		    as, seg);
	}

	/* Dump other user stacks */
	if (!setjmp(&jb)) {
		for (p = allproc; p; p = p->p_nxt)
			if ((as = p->p_as) &&
			    (seg = as->a_segs) &&
			    (seg = seg->s_prev) &&
			    ((seg->s_base + seg->s_size) == (addr_t)USRSTACK) &&
			    (seg->s_ops == &segvn_ops))
				dump_segvn(seg);

		printf("%5d user stack pages\n", dump_size - size);
		size = dump_size;
	} else {
		printf(
		    "failure dumping user stacks: proc=0x%x as=0x%x seg=0x%x\n",
		    p, as, seg);
	}

	/* If requested, dump everything */
	if (!setjmp(&jb)) {
		if (dump_allpages) {
			dump_allpages_machdep();

			printf("%5d \"everything else\" pages\n",
				dump_size - size);
			size = dump_size;
		}
	} else {
		printf("failure dumping \"everything else\" pages\n");
	}
	nofault = saved_jb;
	printf("%5d total pages", dump_size);

	/* Now count the bits */
	n = 0;
	for (i = 0; i < dp->dump_bitmapsize * NBBY; i++)
		if (isset(dump_bitmap, i))
			n++;
	dp->dump_nchunks = n;
	printf(" (%d chunks)\n", n);

	if (n * chunksize != dump_size)
		printf(
	"WARNING: nchunks (%d) * pages/chunk (%d) != total pages (%d) !!!\n",
		n, chunksize, dump_size);

	dp->dump_dumpsize = (1 /* for the first header */ +
			howmany(dp->dump_bitmapsize, PAGESIZE) +
			n * chunksize +
			1 /* for the last header */) *
			PAGESIZE;

	dp->dump_crashtime = time;
	(void) strncpy(dump_panicstring(dp), panicstr ? panicstr : "",
		MAX_PANIC);
	dump_panicstring(dp)[MAX_PANIC] = '\0';

	/* Now dump everything */
	dumpsys_doit();
	return;
}

/*
 * XXX Should this code be moved into the appropriate segment driver?
 * XXX Yes: add a segops_dump(seg) entry point to each driver,
 * XXX using the code below as a model.
 */
struct page *dump_aptopp();

/*
 * Dump the pages belonging to this segkmem segment.
 * Since some architectures have devices that live in OBMEM space above
 * physical memory, we must check the page frame number as well as the
 * type.
 * (Let's hope someone doesn't architect a system that allows devices
 * in OBMEM space *between* physical memory segments!  Or, if they do,
 * that it can be dumped just like memory.)
 */
dump_segkmem(seg)
	struct seg	*seg;
{
#if defined(sun3x)
	struct pte	*tpte;
#else
	struct pte	tpte;
#endif
	addr_t	addr, eaddr;
	u_int	pfn;

	for (addr = seg->s_base, eaddr = addr + seg->s_size;
	    addr < eaddr; addr += MMU_PAGESIZE) {
#ifdef sun3x
		tpte= hat_ptefind(&kas, addr);
		if (tpte-> pte_vld &&
		    (bustype(tpte-> pte_pfn) == BT_OBMEM) &&
		    ((pfn = tpte-> pte_pfn) <= physmax))
#else
#ifdef sun4m
		mmu_getpte(addr, &tpte);
		if (pte_valid(&tpte) && 
		    (bustype((int) tpte.PhysicalPageNumber) == BT_OBMEM) &&
		    ((pfn = tpte.PhysicalPageNumber) <= physmax))
#else
		mmu_getpte(addr, &tpte);
		if (pte_valid(&tpte) && (tpte.pg_type == OBMEM) &&
		    ((pfn = tpte.pg_pfnum) <= physmax))
#endif
#endif
			dump_addpage(pfn);
	}
}

/*
 * Dump the pages belonging to this segmap segment.
 */
dump_segmap(seg)
	struct seg	*seg;
{
	struct segmap_data	*smd;
	struct smap	*smp, *smp_end;
	struct page	*pp;
	u_int	pfn, off;

/* definition of MAP_PAGES comes from seg_map.c */
#define	MAP_PAGES(seg)		((seg)->s_size >> MAXBSHIFT)

	smd = (struct segmap_data *) seg->s_data;
	for (smp = smd->smd_sm, smp_end = smp + MAP_PAGES(segkmap);
	    smp < smp_end; smp++)
		if (smp->sm_refcnt)
			for (off = 0; off < MAXBSIZE; off += PAGESIZE)
				if (pp = page_find(smp->sm_vp,
						smp->sm_off + off)) {
					pfn = page_pptonum(pp);
					dump_addpage(pfn);
				}
#undef	MAP_PAGES
}

/*
 * Dump the pages belonging to this segvn segment.
 */
dump_segvn(seg)
	struct seg	*seg;
{
	struct segvn_data	*svd;
	struct page	*pp;
	struct anon	**app;
	struct vnode	*vp;
	u_int	off, pfn;
	u_int	page, npages;

	npages = seg_pages(seg);
	svd = (struct segvn_data *)seg->s_data;
	vp = svd->vp;
	off = svd->offset;
	if (svd->amp == NULL)
		app = NULL;
	else
		app = &svd->amp->anon[svd->anon_index];

	for (page = 0; page < npages; page++, off += PAGESIZE) {
		struct anon	*ap;

		if (app && (ap = *app++) && (pp = dump_aptopp(ap))) {
			/* void */
		} else
			pp = page_find(vp, off);

		if (pp) {
			pfn = page_pptonum(pp);
			dump_addpage(pfn);
		}
	}

}

/*
 * Common routine used by dump_segvn and dump_segu;
 * to deal with anonymous pages.
 */
struct page *
dump_aptopp(ap)
	struct anon	*ap;
{
	struct page	*pp = NULL;
	struct vnode	*vp;
	u_int		 off;

	if (swap_xlate_nopanic(ap, &vp, &off)) {
		pp = ap->un.an_page;
		if (!pp || (pp->p_vnode != vp) ||
		    (pp->p_offset != off) || pp->p_gone)
			pp = page_find(vp, off);
	}

	return (pp);
}

/*
 * Dump the pages belonging to this u-area segment
 */
dump_segu(seg)
	struct seguser	*seg;
{
	struct segu_segdata	*sdp = (struct segu_segdata *)segu->s_data;
	addr_t			vaddr = (addr_t)seg;
	addr_t			vbase = segu_mtos(vaddr);
	struct segu_data	*sup;
	int			slot;
	int			i;
	struct page		*pp;

	/*
	 * Next 3 sections stolen from segu_getslot, which is "private"
	 * to seg_u.c
	 */
	/*
	 * Make sure the base is in range of the segment as a whole.
	 */
	if (vaddr < segu->s_base || vaddr >= segu->s_base + segu->s_size)
		return;

	/*
	 * Figure out what slot the address lies in.
	 */
	slot = (vaddr - segu->s_base) / ptob(SEGU_PAGES);
	sup = &sdp->usd_slots[slot];

	/*
	 * Nobody has any business touching this slot if it's not currently
	 * allocated.
	 */
	if (!(sup->su_flags & SEGU_ALLOCATED))
		return;

	/*
	 * Extra check: make sure we specified base of area
	 */
	if (vbase != (segu->s_base + ptob(SEGU_PAGES) * slot))
		return;

	/*
	 * Now find all pages that are in memory and flag them.
	 */
	for (i = 0; i < SEGU_PAGES; i++) {
		register struct anon	*ap;
		u_int			pfn;

		ap = sup->su_swaddr[i];

		if (ap && (pp = dump_aptopp(ap))) {
			pfn = page_pptonum(pp);
			dump_addpage(pfn);
		}
	}
}

/*
 * Dump the system:
 * Dump the header, the bitmap, the chunks, and the header again.
 */
dumpsys_doit()
{
	register int bn;
	register int err = 0;
	struct dumphdr	*dp = dumphdr;
	register int i;
	int	totpages, npages = 0;
	register char	*bitmap = dump_bitmap;

	if (dumpvp == NULL) {
		printf("\ndump device not initialized, no dump taken\n");
		return;
	}
	bn = dumpfile.bo_size - ctod(btoc(dp->dump_dumpsize));
	printf("\ndumping to vp %x, offset %d\n", dumpvp, bn);
#ifdef sparc
	/* Must get all the kernel's windows to memory for adb */
	flush_windows();
#endif
#ifdef sun3x
	vac_flush();
#else
#ifdef	sun4m
	mmu_setctx(kctx->c_num);
#else
	mmu_setctx(kctx);
#endif	sun4m
	vac_flushall();
#endif

	/*
	 * Write the header, then the bitmap, then the chunks, then the
	 * header again.
	 * XXX Should we write the last header first, so that if
	 * XXX anything gets dumped we can use it?
	 */
	/* 1st copy of header */
	if (err = dump_kaddr(dumpvp, (caddr_t) dp, bn, ctod(1)))
		goto done;
	bn += ctod(1);

	/* bitmap */
	if (err = dump_kaddr(dumpvp, bitmap, bn,
			(int) ctod(btoc(dp->dump_bitmapsize))))
		goto done;
	bn += ctod(btoc(dp->dump_bitmapsize));

	totpages = dp->dump_nchunks * dp->dump_chunksize;
	for (i = 0; i < dp->dump_bitmapsize * NBBY; i++) {
		if (isset(bitmap, i)) {
			register int n, pg;

			for (pg = i * dp->dump_chunksize,
			    n = dp->dump_chunksize;
			    n > 0 && !err; n--, pg++) {
				err = dump_page(dumpvp, pg, bn);
				bn += ctod(1);
				npages++;
				if ((pg & 0x7) == 0) {
					register int t;
					extern int msgbufinit;

					t = msgbufinit;
					msgbufinit = 0;
					printf("\r%d pages left      ",
						totpages - npages);
					msgbufinit = t;
				}
			}
			if (err)
				goto done;
		}
	}

	/* Now dump the second copy of the header */
	if (err = dump_kaddr(dumpvp, (caddr_t) dp, bn, ctod(1)))
		goto done;
	bn += ctod(1);

done:
	printf("\r%d total pages, dump ", npages);

	switch (err) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	case 0:
		printf("succeeded\n");
		break;

	default:
		printf("failed: error %d\n", err);
		break;
	}
}
