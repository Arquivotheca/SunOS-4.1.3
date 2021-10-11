/*	@(#)vmmac.h 1.1 92/07/30 SMI; from UCB 4.7 83/07/01	*/

/*
 * Miscellanous virtual memory related conversion macros.
 */

#ifndef _sys_vmmac_h
#define _sys_vmmac_h

/*
 * Return the mmu page number for a given index
 * in the UNIX text, data, and stack segments.
 *
 * XXX - should simplify things so that we have bytes values for
 * base and length of UNIX segments instead of using [tds]size.
 * This should remove the need for these macros all together.
 */
#define	tptov(p, i)	((unsigned)(LOWPAGES + (i))

#ifdef sun386
#define dptov(p, i)     ((unsigned)(((p)->p_tsize ? \
                         (unsigned)(btop(u.u_datavaddr)) : \
                         (unsigned)(btop(u.u_textvaddr))) + (i)))
#else
#define	dptov(p, i)	((unsigned)(((p)->p_tsize ? \
    roundup((p)->p_tsize + LOWPAGES, mmu_btop(DATA_ALIGN)) : LOWPAGES) + (i)))
#endif

#define	sptov(p, i)	((unsigned)(mmu_btop(USRSTACK) - 1 - (i)))

/*
 * Turn virtual addresses into kernel map indices.
 *
 * "Sysmap" is an array of page table entries used to map virtual
 * addresses, starting at (kernel virtual address) Sysbase, to many
 * different things.  Sysmap is managed throught the resource map named
 * "kernelmap".  kmx means kernelmap index, the index (into Sysmap)
 * returned by rmalloc(kernelmap, ...).
 *
 * kmxtob expects an (integer) kernel map index and returns the virtual
 * address by the mmu page number.  btokmx expects a (caddr_t) virtual
 * address and returns the integer kernel map index.
 */
#define	kmxtob(a)	(Sysbase + ((a) << MMU_PAGESHIFT))
#define	btokmx(b)	(((caddr_t)(b) - Sysbase) >> MMU_PAGESHIFT)

/* Average new into old with aging factor time */
#define	ave(smooth, cnt, time) \
	smooth = ((time - 1) * (smooth) + (cnt)) / (time)

/* XXX - this doesn't really belong here */
#define	outofmem()	wakeup((caddr_t)&proc[2]);

#endif /*!_sys_vmmac_h*/
