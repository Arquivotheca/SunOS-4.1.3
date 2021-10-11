/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)uipc_mbuf.c 1.1 92/07/30 SMI; from UCB 7.4.1.2 2/8/88
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/mbuf.h>
#include <sys/vm.h>
#include <sys/kernel.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/syslog.h>

#include <machine/pte.h>

#include <vm/hat.h>
#include <vm/seg.h>
#include <machine/seg_kmem.h>
#if defined(sun4m) && defined(IOMMU)
#include <vm/as.h>
#include <sundev/mbvar.h>
#include <sun4m/iommu.h>

extern	int cache;
#endif defined(sun4m) && defined(IOMMU)

/*
 * The way the code stands, MCLBYTES must be a power of 2
 * and type MCL_LOANED cluster mbufs, if swappable, must
 * swap this size.
 *
 */

/*
 * In contrast to the 4.3 implementation, we allocate mbmap in units
 * of bytes, converting later to units of mbclusters.  Note that the
 * map itself keeps track of _unused_ space, and that the current
 * implementation will, at most, free the most recently allocated entry.
 * These considerations imply that the value of MBMAP_ENTRIES defined
 * below is very generous.  (It's carried over unchanged from the
 * 4.3 implementation.)
 */
#define	MBMAP_ENTRIES	(btomcl(MBPOOLBYTES)/4)
struct map	mbmap[MBMAP_ENTRIES];

/*
 * One slot per mbcluster, but see commentary in mbinit
 * below. A value of -1 means "no physical memory yet".
 */
char	mclrefcnt[btomcl(MBPOOLBYTES) + 1];

/* Allocate the initial mbuf resources */
mbinit()
{
	int s;
	register char *p;

	p = mclrefcnt;
	for (s = 0; s < btomcl(MBPOOLBYTES) + 1; s++)
		*p++ = -1;

	/*
	 * We initialize the resource map for mbufs with the first
	 * PAGESIZE (our minimum allocation unit) bytes skipped,
	 * since the resource map code uses 0 as a delimiter.
	 *
	 * Note that 4.3 fixes this problem by actually allocating
	 * space for an extra PAGESIZE worth of mbuf area in locore.s,
	 * whereas we fix it here, since this file is common to all
	 * architectures and this is the only file that should
	 * really have to know how this resource map is dealt with.
	 */
	rminit(mbmap, (long)(MBPOOLBYTES - PAGESIZE), (long)PAGESIZE,
	    "mbclusters", MBMAP_ENTRIES);

	s = splimp();
	if (m_clalloc(btomcl(4096), MPG_MBUFS, M_DONTWAIT) == 0)
		goto bad;
	if (m_clalloc(btomcl(4*4096), MPG_CLUSTERS, M_DONTWAIT) == 0)
		goto bad;
	(void) splx(s);
	return;
bad:
	panic("mbinit");
}

/*
 * Allocate a pool of the requested mbuf type.
 * Must be called at splimp.
 */
/* ARGSUSED */
caddr_t
m_clalloc(ncl, how, canwait)
	register int ncl;
	int how;
	int canwait;
{
	int npg, mbx;
	register struct mbuf *m;
	register int i;
	long bytes;
	static int logged;
#if defined(sun4m) && defined(IOMMU)
	addr_t vaddr;
	union ptpe *ptpe;
	union iommu_pte *piopte;
	int mbux, map_addr;
	extern struct as kas;
#endif defined(sun4m) && defined(IOMMU)

	/*
	 * Round request size up to nearest multiple of both
	 * mbcluster size and page size.
	 */
	npg = btopr(mcltob(ncl));
	bytes = ptob(npg);
	ncl = btomcl(bytes);

	/*
	 * Slice the corresponding number of bytes out
	 * of the mbuf cluster resource map.
	 */
	mbx = rmalloc(mbmap, bytes);
	if (mbx == 0) {
		if (logged == 0) {
			logged++;
			log(LOG_ERR, "mbuf map full\n");
		}
		return (0);
	}

#if defined(sun4m) && defined(IOMMU)
	/*
	 * Slice the corresponding number of pages out
	 * of the corresponding mbutl iommu resource map.
	 */
	mbux = rmalloc(mbutlmap, (long)npg);
	if (mbux == 0) {
		rmfree(mbmap, bytes, (u_long)mbx);
		if (logged == 0) {
			logged++;
			log(LOG_ERR, "mbutl map full\n");
		}
		return (0);
	}
#endif defined(sun4m) && defined(IOMMU)

	/*
	 * Get and map physical space for the new clusters.
	 */
	m = (struct mbuf *)((int)mbutl + mbx);
	if (segkmem_alloc(&kseg, (addr_t)m, (u_int)bytes, 0,
				SEGKMEM_MBUF) == 0) {
		rmfree(mbmap, bytes, (u_long)mbx);
#if defined(sun4m) && defined(IOMMU)
		rmfree(mbutlmap, (long)npg, (u_long)mbux);
#endif defined(sun4m) && defined(IOMMU)
		return (0);
	}

#if defined(sun4m) && defined(IOMMU)
	for (i = 0; i < iommu_btop(bytes); i++) {
		vaddr = (addr_t)((int)m + (int)iommu_ptob(i));
		ptpe = hat_ptefind(&kas, vaddr);
		map_addr = (int)iommu_btop((int)vaddr - (int)mbutl +
				IOMMU_MBUTL_BASE);
		if ((piopte = iom_ptefind(map_addr, mbutlmap)) == NULL)
			panic("m_clalloc: bad iopfn");
		piopte->iopte_uint = ptpe->ptpe_int;
		if (!cache)
			piopte->iopte.cache = 0;
	}
#endif defined(sun4m) && defined(IOMMU)

	switch (how) {
	case MPG_CLUSTERS:
		for (i = 0; i < ncl; i++) {
			m->m_off = 0;
			m->m_next = mclfree;
			mclfree = m;
			mclrefcnt[mtocl(m)] = 0;
			m += MCLBYTES / sizeof (*m);
			mbstat.m_clfree++;
		}
		mbstat.m_clusters += ncl;
		break;

	case MPG_MBUFS:
		for (i = ncl * MCLBYTES / sizeof (*m); i > 0; i--) {
			m->m_off = 0;
			m->m_type = MT_DATA;
			mbstat.m_mtypes[MT_DATA]++;
			mbstat.m_mbufs++;
			(void) m_free(m);
			m++;
		}
		break;

	case MPG_SPACE:
		mbstat.m_space++;
		break;
	}
	return ((caddr_t)m);
}

#ifdef vax
/*ARGSUSED*/
m_pgfree(addr, n)
	caddr_t addr;
	int n;
{

}
#endif

/*
 * Internal routine to locate more storage for small mbufs.
 * Must be called at splimp.  The routine always returns
 * without sleeping.  Callers are themselves responsible for
 * sleeping when resources aren't available.  Returns 1
 * upon success, 0 otherwise.
 */
m_expand(canwait)
	int canwait;
{
	register struct domain *dp;
	register struct protosw *pr;
	int tries;

	tries = 0;
	for (;;) {
		if (m_clalloc(1, MPG_MBUFS, canwait))
			return (1);
		if (canwait == 0 || tries++)
			return (0);

		/* ask protocols to free space */
		for (dp = domains; dp; dp = dp->dom_next)
			for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW;
			    pr++)
				if (pr->pr_drain)
					(*pr->pr_drain)();
	}
}

/*
 * Space allocation routines.
 * These are also available as macros
 * for critical paths.
 */

/* Small mbuf allocator */
struct mbuf *
m_get(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;

	MGET(m, canwait, type);
	return (m);
}

/* Small zeroed (cleared) mbuf allocator */
struct mbuf *
m_getclr(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;

	MGET(m, canwait, type);
	if (m == 0)
		return (0);
	bzero(mtod(m, caddr_t), MLEN);
	return (m);
}

/*
 * Generic mbuf deallocator
 * frees the first mbuf in the chain
 * and returns the rest of the chain
 * (compare with m_freem())
 */
struct mbuf *
m_free(m)
	struct mbuf *m;
{
	register struct mbuf *n;

	MFREE(m, n);
	return (n);
}

/*
 * Get more mbufs; called from MGET macro if mfree list is empty.
 * Must be called at splimp.
 */
/*ARGSUSED*/
struct mbuf *
m_more(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;

	/*
	 * If we can afford to wait for storage to appear,
	 * we sleep after returning from m_expand, instead
	 * of counting on m_expand to sleep itself.
	 */
	while (m_expand(canwait) == 0) {
		if (canwait == M_WAIT) {
			mbstat.m_wait++;
			m_want++;
			(void) sleep((caddr_t)&mfree, PZERO - 1);
		} else {
			mbstat.m_drops++;
			return (NULL);
		}
	}
#define	m_more(x, y)	(panic("m_more"), (struct mbuf *)0)
	MGET(m, canwait, type);
#undef	m_more
	return (m);
}

/*
 * Generic mbuf deallocator
 * frees the entire mbuf chain
 * (compare with m_free())
 */
m_freem(m)
	register struct mbuf *m;
{
	register struct mbuf *n;
	register int s;

	if (m == NULL)
		return;
	s = splimp();
	do {
		MFREE(m, n);
	} while (m = n);
	(void) splx(s);
}

/*
 * Mbuffer utility routines.
 */

/*
 * Make a copy of an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes.  If len is M_COPYALL, copy to end of mbuf.
 * Where  possible, reference counts on pages are used instead of core to
 * core copies.  The original mbuf chain must have at least off + len bytes
 * of data.
 * Should get M_WAIT/M_DONTWAIT from caller.
 */
struct mbuf *
m_copy(m, off, len)
	register struct mbuf *m;
	int off;
	register int len;
{
	register struct mbuf *n, **np;
	struct mbuf *top;

	if (len == 0)
		return (0);
	if (off < 0 || len < 0)
		panic("m_copy");
	while (off > 0) {
		if (m == 0)
			panic("m_copy");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	np = &top;
	top = 0;
	while (len > 0) {
		if (m == 0) {
			if (len != M_COPYALL)
				panic("m_copy");
			break;
		}
		MGET(n, M_DONTWAIT, m->m_type);
		*np = n;
		if (n == 0)
			goto nospace;
		n->m_len = MIN(len, m->m_len - off);
		if (m->m_off > MMAXOFF && n->m_len > MLEN) {
			mcldup(m, n, off);
			/* Check for mcldup() fail due to kmem_alloc() fail */
			if (!M_HASCL(n))
				goto nospace;
			n->m_off += off;
		} else
			bcopy(mtod(m, caddr_t)+off, mtod(n, caddr_t),
			    (unsigned)n->m_len);
		if (len != M_COPYALL)
			len -= n->m_len;
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}
	return (top);
nospace:
	m_freem(top);
	return (0);
}

/*
 * The mbuf chain, n, is appended to the end of m.  Where
 * possible, compaction is performed.
 */
m_cat(m, n)
	register struct mbuf *m, *n;
{

	while (m->m_next)
		m = m->m_next;
	while (n) {
		if (m->m_off >= MMAXOFF ||
		    m->m_off + m->m_len + n->m_len > MMAXOFF) {
			/* just join the two chains */
			m->m_next = n;
			return;
		}
		/* splat the data from one into the other */
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		    (u_int)n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}

/*
 * The mbuf chain mp is reduced in size by |len| bytes.  If
 * len is non-negative, len bytes are shaved off the front
 * of the mbuf chain.  If len is negative, the alteration is
 * performed from back to front.  No space is reclaimed in
 * this operation, alterations are accomplished by changing
 * the m_len and m_off fields of mbufs.
 */
m_adj(mp, len)
	struct mbuf *mp;
	register int len;
{
	register struct mbuf *m;
	register int count;

	if ((m = mp) == NULL)
		return;
	if (len >= 0) {
		while (m != NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_off += len;
				break;
			}
		}
	} else {
		/*
		 * Trim from tail.  Scan the mbuf chain,
		 * calculating its length and finding the last mbuf.
		 * If the adjustment only affects this mbuf, then just
		 * adjust and return.  Otherwise, rescan and truncate
		 * after the remaining size.
		 */
		len = -len;
		count = 0;
		for (;;) {
			count += m->m_len;
			if (m->m_next == (struct mbuf *)0)
				break;
			m = m->m_next;
		}
		if (m->m_len >= len) {
			m->m_len -= len;
			return;
		}
		count -= len;
		/*
		 * Correct length for chain is "count".
		 * Find the mbuf with last data, adjust its length,
		 * and toss data from remaining mbufs on chain.
		 */
		for (m = mp; m; m = m->m_next) {
			if (m->m_len >= count) {
				m->m_len = count;
				break;
			}
			count -= m->m_len;
		}
		while (m = m->m_next)
			m->m_len = 0;
	}
}

/*
 * Rearange an mbuf chain so that len bytes are contiguous
 * and in the data area of an mbuf (so that mtod and dtom
 * will work for a structure of size len).  Returns the resulting
 * mbuf chain on success, frees it and returns null on failure.
 * If there is room, it will add up to MPULL_EXTRA bytes to the
 * contiguous region in an attempt to avoid being called next time.
 * N.B.: for m_pullup to succeed, len must be <= MLEN.
 *
 * See also ether_pullup() (in sunif/if_subr.c).  These two routines
 * should be unified, perhaps by using two length arguments, one
 * giving length that must be pulled up, and the other giving additional
 * length to pull up if possible.
 */
struct mbuf *
m_pullup(n, len)
	register struct mbuf *n;
	int len;
{
	register struct mbuf *m;
	register int count;
	int space;

	if (n->m_off + len <= MMAXOFF && n->m_next) {
		m = n;
		n = n->m_next;
		len -= m->m_len;
	} else {
		if (len > MLEN)
			goto bad;
		MGET(m, M_DONTWAIT, n->m_type);
		if (m == 0)
			goto bad;
		m->m_len = 0;
	}
	/* Calculate total room for data in this mbuf. */
	space = MMAXOFF - m->m_off;
	do {
		/*
		 * Determine amount to move from the next mbuf.  The inner
		 * MIN calculates the desired amount to move from the amount
		 * of room left in this mbuf and the overall remaining length
		 * desired.
		 */
		count = MIN(MIN(space - m->m_len, len + MPULL_EXTRA), n->m_len);
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t)+m->m_len,
			(unsigned)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		if (n->m_len)
			n->m_off += count;
		else
			n = m_free(n);
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	return (0);
}

/*
 * Copy an mbuf to the contiguous area pointed to by cp.
 * Skip <off> bytes and copy <len> bytes.
 * Returns the number of bytes not transferred.
 * The mbuf is NOT changed.
 */
int
m_cpytoc(m, off, len, cp)
	register struct mbuf *m;
	register int off, len;
	register caddr_t cp;
{
	register int ml;

	if (m == NULL || off < 0 || len < 0 || cp == NULL)
		panic("m_cpytoc");
	while (off && m)
		if (m->m_len <= off) {
			off -= m->m_len;
			m = m->m_next;
			continue;
		} else
			break;
	if (m == NULL)
		return (len);

	ml = imin(len, m->m_len - off);
	bcopy(mtod(m, caddr_t)+off, cp, (u_int)ml);
	cp += ml;
	len -= ml;
	m = m->m_next;

	while (len && m) {
		ml = imin(len, m->m_len);
		bcopy(mtod(m, caddr_t), cp, (u_int)ml);
		cp += ml;
		len -= ml;
		m = m->m_next;
	}

	return (len);
}

/*
 * Append a contiguous hunk of memory to a particular mbuf;
 * that is, it does not follow the mbuf chain.
 * XXX It should return 1 if it won't fit.  Maybe someday.
 */
int
m_cappend(cp, len, m)
	char *cp;
	register int len;
	register struct mbuf *m;
{
	bcopy(cp, mtod(m, char *) + m->m_len, (u_int)len);
	m->m_len += len;
	return (0);
}

/*
 * Tally the bytes used in an mbuf chain.
 * op: -1 == subtract; 0 == assign; 1 == add;
 */
m_tally(m, op, cc, mbcnt)
	register struct mbuf *m;
	int op, *cc, *mbcnt;
{
	struct sockbuf sockbuf;
	register struct sockbuf *sb = &sockbuf;

	bzero((caddr_t)sb, sizeof *sb);
	while (m) {
		sballoc(sb, m);
		m = m->m_next;
	}
	if (cc)
		switch (op) {
		case -1:
			*cc -= sb->sb_cc;
			break;
		case 0:
			*cc  = sb->sb_cc;
			break;
		case 1:
			*cc += sb->sb_cc;
			break;
		}
	if (mbcnt)
		switch (op) {
		case -1:
			*mbcnt -= sb->sb_mbcnt;
			break;
		case 0:
			*mbcnt  = sb->sb_mbcnt;
			break;
		case 1:
			*mbcnt += sb->sb_mbcnt;
			break;
		}
}

/*
 * Given an mbuf, allocate and attach a cluster mbuf to it.
 * Return 1 if successful, 0 otherwise.
 * NOTE: m->m_len is set to MCLBYTES if sucessful!
 * XXX should take M_WAIT/M_DONTWAIT from caller.
 */
mclget(m)
	register struct mbuf *m;
{
	int ms;
	register struct mbuf *p;

	ms = splimp();
	if (mclfree == 0)
		/* XXX need a way to reclaim */
		(void) m_clalloc(1, MPG_CLUSTERS, M_DONTWAIT);
	if (p = mclfree) {
		++mclrefcnt[mtocl(p)];
		mbstat.m_clfree--;
		mclfree = p->m_next;
		m->m_len = MCLBYTES;
		m->m_off = (int)p - (int)m;
		m->m_cltype = MCL_STATIC;
	} else
		m->m_len = 0;
	(void) splx(ms);
	return (p ? 1 : 0);
}

/*
 * Allocate a MCL_LOANED cluster mbuf, that is,
 * one whose data area is owned by someone else.
 */
struct mbuf *
mclgetx(fun, arg, addr, len, wait)
	int (*fun)(), arg, len, wait;
	caddr_t addr;
{
	register struct mbuf *m;

	MGET(m, wait, MT_DATA);
	if (m == 0)
		return (0);
	m->m_off = (int)addr - (int)m;
	m->m_len = len;
	m->m_cltype = MCL_LOANED;
	m->m_clfun = fun;
	m->m_clarg = arg;
	m->m_clswp = NULL;
	return (m);
}

/*
 * Cluster mbuf deallocator: invoked from within MFREE
 * to free the data portion of a cluster mbuf.  (The
 * mbuf itself is freed in MFREE.)
 */
mclput(m)
	register struct mbuf *m;
{

	switch (m->m_cltype) {
	case MCL_STATIC:
		m = (struct mbuf *)(mtod(m, int) & ~(MCLBYTES - 1));
		if (--mclrefcnt[mtocl(m)] == 0) {
			m->m_next = mclfree;
			mclfree = m;
			mbstat.m_clfree++;
			m_reclaim();
		}
		break;

	case MCL_LOANED:
		(*m->m_clfun)(m->m_clarg);
		break;

	default:
		panic("mclput");
	}
}


int mcl_hiwat = 32;	/* max free clusters before giving some back */
int mcl_lowat = 16;	/* give back until this many left */

#define	CLPERPAGE	(PAGESIZE/MCLBYTES)	/* Cluster per page */
	/*
	 * Since this test can be called on every mclput() when
	 * the cluster pool gets fragmented, we special case on
	 * the number of clusters in a page.  This assumes a short
	 * is two chars, and a long is four chars.  If it is not
	 * one of the special cases, PAGEFUL_ZERO will be undefined.
	 */
#if	CLPERPAGE == 1
#define	PAGEFUL_ZERO(cl)	(mclrefcnt[cl] == 0)
#endif	CLPERPAGE == 1
#if	CLPERPAGE == 2
#define	PAGEFUL_ZERO(cl)	(*(short *)(mclrefcnt+cl) == 0)
#endif	CLPERPAGE == 2
#if	CLPERPAGE == 4
#define	PAGEFUL_ZERO(cl)	(*(long *)(mclrefcnt+cl) == 0)
#endif	CLPERPAGE == 4
#if	CLPERPAGE == 8
#define	PAGEFUL_ZERO(cl)	(*(long *)(mclrefcnt+cl) == 0 && \
				 *(long *)(mclrefcnt+cl+4) == 0)
#endif CLPERPAGE == 8


/*
 * Reclaim the physical memory from unused cluster mbufs, if possible
 * and desired.
 */
m_reclaim()
{
	register int cl, freecl;		/* cluster numbers */
#if defined(sun4m) && defined(IOMMU)
	u_int dvma_addr;
	u_long map_addr;
	extern int iom_dvma_unload();
#endif defined(sun4m) && defined(IOMMU)

	if (mbstat.m_clfree < mcl_hiwat)
		return;
	/*
	 * We have more than enough free clusters, so give some of
	 * the physical memory back to the kernel.
	 * This gets complicated because segkmem only works
	 * in page size increments!
	 */

	cl = 0;
	while (mbstat.m_clfree > mcl_lowat && cl < btomcl(MBPOOLBYTES) + 1) {
		/*
		 * First find a page-ful of clusters to reclaim.
		 */
		if (PAGEFUL_ZERO(cl)) {
			/*
			 * Found such a page-ful, remove them all from
			 * the free list, and then finally give back the
			 * page of physical memory.
			 */
			for (freecl = cl; freecl < cl + CLPERPAGE; freecl++)
				m_unfree(cltom(freecl));
#if defined(sun4m) && defined(IOMMU)
			dvma_addr = (int)cltom(cl) - (int)mbutl
				+ IOMMU_MBUTL_BASE;
			(void) iom_dvma_unload(dvma_addr);
			map_addr = (u_long)(MBUTLMAP_BASE + btop(mcltob(cl))-1);
			rmfree(mbutlmap, (long)btop(PAGESIZE), map_addr);
#endif defined(sun4m) && defined(IOMMU)
			segkmem_free(&kseg, (addr_t)cltom(cl), PAGESIZE);
			rmfree(mbmap, (long)PAGESIZE, (u_long)mcltob(cl));
		}
		cl += CLPERPAGE; /* Try next page-ful */
	}
}

/*
 * remove the given cluster from the free list
 */
static m_unfree(m0)
	register struct mbuf *m0;
{
	register struct mbuf *m;

	m = mclfree;
	if (m == m0) {
		/*
		 * Special-case: delete the first one
		 */
		mclfree = m->m_next;
		mbstat.m_clfree--;
		mbstat.m_clusters--;
		mclrefcnt[mtocl(m)] = -1;
		return;
	}
	while (m) {
		if (m->m_next == m0) {
			m->m_next = m0->m_next;
			mbstat.m_clfree--;
			mbstat.m_clusters--;
			mclrefcnt[mtocl(m0)] = -1;
			return;
		}
		m = m->m_next;
	}
	/*
	 * It was not on the free list after all!
	 * Perhaps we should panic here?
	 */
}

/*
 * Deallocation routine for MCL_LOANED cluster mbufs
 * created by mcldup.
 */
static
buffree(arg)
	int arg;
{
	extern int kmem_free();

	kmem_free((caddr_t)arg, *(u_int *)arg);
}

/*
 * Generic cluster mbuf duplicator
 * which duplicates <m> into <n>.
 * If <m> is a regular cluster mbuf, mcldup simply
 * bumps the reference count and ignores <off>.
 * If <m> is a funny mbuf, mcldup allocates a chunck
 * kernel memory and makes a copy, starting at <off>.
 * XXX does not set the m_len field in <n>!
 */
mcldup(m, n, off)
	register struct mbuf *m, *n;
	int off;
{
	register struct mbuf *p;
	register caddr_t copybuf;
	int	s;

	switch (m->m_cltype) {
	case MCL_STATIC:
		p = mtod(m, struct mbuf *);
		n->m_off = (int)p - (int)n;
		n->m_cltype = MCL_STATIC;
		s = splimp();
		mclrefcnt[mtocl(p)]++;
		splx(s);
		break;
	case MCL_LOANED:
		copybuf = new_kmem_alloc(
			(u_int)(n->m_len + sizeof (int)), KMEM_NOSLEEP);
		if (!copybuf)
			break;
		* (int *) copybuf = n->m_len + sizeof (int);
		bcopy(mtod(m, caddr_t) + off, copybuf + sizeof (int),
		    (u_int)n->m_len);
		n->m_off = (int)copybuf + sizeof (int) - (int)n - off;
		n->m_cltype = MCL_LOANED;
		n->m_clfun = buffree;
		n->m_clarg = (int)copybuf;
		n->m_clswp = NULL;
		break;
	default:
		panic("mcldup");
	}
}

#ifdef	notdef
/*
 * This routine is currently unused, but may again be used someday.
 */
/*
 * Move an mbuf chain to contiguous locations.
 * Checks for possibility of page exchange to accomplish move.
 * Free chain when moved.
 */
m_movtoc(m, to, count)
	register struct mbuf *m;
	register caddr_t to;
	register int count;
{
	register struct mbuf *m0;
	register caddr_t from;
	register int i;

	while (m != NULL) {
		i = MIN(m->m_len, count);
		from = mtod(m, caddr_t);
		if (i >= MCLBYTES && m->m_cltype == MCL_LOANED && m->m_clswp &&
		    (((int)from | (int)to) & (MCLBYTES-1)) == 0 &&
		    (*m->m_clswp)(m->m_clarg, from, to)) {
			i -= MCLBYTES;
			from += MCLBYTES;
			to += MCLBYTES;
		}
		if (i > 0) {
			bcopy(from, to, (unsigned)i);
			count -= i;
			to += i;
		}
		m0 = m;
		MFREE(m0, m);
	}
	return (count);
}
#endif	notdef
