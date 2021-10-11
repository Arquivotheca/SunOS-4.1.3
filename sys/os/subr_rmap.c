/*	@(#)subr_rmap.c 1.1 92/07/30 SMI; from UCB 4.9 83/05/18	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/debug.h>

/*
 * Resource map handling routines.
 *
 * A resource map is an array of structures each
 * of which describes a segment of the address space of an available
 * resource.  The segments are described by their base address and
 * length, and sorted in address order.  Each resource map has a fixed
 * maximum number of segments allowed.  Resources are allocated
 * by taking part or all of one of the segments of the map.
 *
 * Returning of resources will require another segment if
 * the returned resources are not adjacent in the address
 * space to an existing segment.  If the return of a segment
 * would require a slot which is not available, then one of
 * the resource map segments is discarded after a warning is printed.
 * Returning of resources may also cause the map to collapse
 * by coalescing two existing segments and the returned space
 * into a single segment.  In this case the resource map is
 * made smaller by copying together to fill the resultant gap.
 *
 * N.B.: the current implementation uses a dense array and does
 * not admit the value ``0'' as a legal address, since that is used
 * as a delimiter.
 */

/*
 * Initialize map mp to have (mapsize-2) segments
 * and to be called ``name'', which we print if
 * the slots become so fragmented that we lose space.
 * The map itself is initialized with size elements free
 * starting at addr.
 */
rminit(mp, size, addr, name, mapsize)
	register struct map *mp;
	long size;
	u_long addr;
	char *name;
	int mapsize;
{
	register struct mapent *ep = mapstart(mp);
	register struct maplast *lp = (struct maplast *)(mp + mapsize - 1);

	ASSERT(sizeof (struct map) == sizeof (struct mapent) &&
		sizeof (struct map) == sizeof (struct maplast));

	if (mapsize < 2) {
		printf("rminit: map %x, mapsize %d too small\n",
			mp, mapsize);
		panic("rminit");
	}

	/*
	 * One of the mapsize slots is taken by the map structure; the
	 * first free segment has size 0.  The final segment in the
	 * array has size 0 and acts as a delimiter, with addr pointing
	 * to the name of the map.
	 * We insure that we never use segments past the end of
	 * the array by maintaining a free segment count in m_free.
	 * Instead, when excess segments occur we discard some resources.
	 */
	lp->m_nam = name;
	lp->m_size = 0;
	if (mapsize == 2) {
		mapfree(mp) = 0;
	} else {
		mapfree(mp) = mapsize - 3;
		/*
		 * Simulate a rmfree(), but with the option to
		 * call with size 0 and addr 0 when we just want
		 * to initialize without freeing.
		 */
		ep->m_size = size;
		ep->m_addr = addr;
		ep[1].m_size = 0;
		if (size == 0)
			mapfree(mp)++;
	}
}

/*
 * Allocate 'size' units from the given
 * map. Return the base of the allocated space.
 * In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 *
 * Algorithm is first-fit.
 */
u_long
rmalloc(mp, size)
	register struct map *mp;
	long size;
{
	register struct mapent *ep = mapstart(mp);
	register u_long addr;
	register struct mapent *bp;

	if (size <= 0)
		panic("rmalloc");
	/*
	 * Search for a piece of the resource map which has enough
	 * free space to accomodate the request.
	 */
	for (bp = ep; bp->m_size != 0; bp++) {
		if (bp->m_size >= size) {
			/*
			 * Allocate from the map.
			 * If there is no space left of the piece
			 * we allocated from, move the rest of
			 * the pieces to the left and increment the free
			 * segment count.
			 */
			addr = bp->m_addr;
			bp->m_addr += size;
			bp->m_size -= size;
			if (bp->m_size == 0) {
				do {
					bp[0] = bp[1];
					bp++;
				} while (bp->m_size != 0);
				mapfree(mp)++;
			}
			return (addr);
		}
	}
	return (0);
}

/*
 * Free the previously allocated space at addr
 * of size units into the specified map.
 * Sort addr into map and combine on
 * one or both ends if possible.
 */
rmfree(mp, size, addr)
	struct map *mp;
	long size;
	u_long addr;
{
	struct mapent *firstbp;
	register struct mapent *bp, *lp;
	register int t;

	/*
	 * Address must be non-zero and size must be
	 * positive, or the protocol has broken down.
	 */
	if (addr == 0 || size <= 0)
		goto badrmfree;
	/*
	 * Locate the piece of the map which starts after the
	 * returned space (or the end of the map).
	 */
retry:
	firstbp = bp = mapstart(mp);
	while (bp->m_addr <= addr && bp->m_size != 0)
		bp++;
	/*
	 * If the piece on the left abuts us,
	 * then we should combine with it.
	 */
	lp = bp - 1;
	if (bp != firstbp && lp->m_addr + lp->m_size >= addr) {
		/*
		 * Check no overlap (internal error).
		 */
		if (lp->m_addr + lp->m_size > addr)
			goto badrmfree;
		/*
		 * Add into piece on the left by increasing its size.
		 */
		lp->m_size += size;
		/*
		 * If the combined piece abuts the piece on
		 * the right now, compress it in also,
		 * by shifting the remaining pieces of the map over.
		 * Also, increment free segment count.
		 */
		if (bp->m_size != 0 && addr + size >= bp->m_addr) {
			if (addr + size > bp->m_addr)
				goto badrmfree;
			lp->m_size += bp->m_size;
			while (bp->m_size != 0) {
				bp[0] = bp[1];
				bp++;
			}
			mapfree(mp)++;
		}
		goto done;
	}
	/*
	 * Don't abut on the left, check for abutting on
	 * the right.
	 */
	if (bp->m_size != 0 && addr + size >= bp->m_addr) {
		if (addr + size > bp->m_addr)
			goto badrmfree;
		bp->m_addr -= size;
		bp->m_size += size;
		goto done;
	}
	/*
	 * Don't abut at all.  Check for map overflow.
	 * Discard the smaller of the last/next-to-last entries.
	 * Then retry the rmfree operation.
	 */
	if (mapfree(mp) == 0) {
		/* locate final entry */
		for (firstbp = bp; firstbp->m_size != 0; firstbp++)
			continue;

		/* point to smaller of the last two segments */
		bp = firstbp - 1;
		if (bp->m_size > (bp - 1)->m_size)
			bp--;
		printf("%s: rmap overflow, lost [%d, %d)\n", mapname(firstbp),
		    bp->m_addr, bp->m_addr + bp->m_size);

		/* destroy one entry, compressing down; inc free count */
		bp[0] = bp[1];
		bp[1].m_size = 0;
		mapfree(mp)++;
		goto retry;
	}
	/*
	 * Make a new entry and push the remaining ones up
	 */
	do {
		t = bp->m_addr;
		bp->m_addr = addr;
		addr = t;
		t = bp->m_size;
		bp->m_size = size;
		size = t;
		bp++;
	} while (size != 0);
	mapfree(mp)--;		/* one less free segment remaining */
done:
	/* if anyone sleeping on rmalloc failure, wake 'em up */
	if (mapwant(mp)) {
		mapwant(mp) = 0;
		wakeup((caddr_t)mp);
	}
	return;
badrmfree:
	panic("bad rmfree");
}

/*
 * Allocate 'size' units from the given map, starting at address 'addr'.
 * Return 'addr' if successful, 0 if not.
 * This may cause the creation or destruction of a resource map segment.
 *
 * This routine will return failure status if there is not enough room
 * for a required additional map segment.
 */
rmget(mp, size, addr)
	register struct map *mp;
	long size;
	u_long addr;
{
	register struct mapent *ep = mapstart(mp);
	register struct mapent *bp, *bp2;

	if (size <= 0)
		panic("rmget");
	/*
	 * Look for a map segment containing the requested address.
	 * If none found, return failure.
	 */
	for (bp = ep; bp->m_size != 0; bp++) {
		if (bp->m_addr <= addr && bp->m_addr + bp->m_size > addr)
			break;
	}
	if (bp->m_size == 0)
		return (0);

	/*
	 * If segment is too small, return failure.
	 * If big enough, allocate the block, compressing or expanding
	 * the map as necessary.
	 */
	if (bp->m_addr + bp->m_size < addr + size)
		return (0);
	if (bp->m_addr == addr) {
		if (bp->m_size == size) {
			/*
			 * Allocate entire segment and compress map
			 * Increment free segment map
			 */
			bp2 = bp;
			while (bp2->m_size != 0) {
				bp2[0] = bp2[1];
				bp2++;
			}
			mapfree(mp)++;
		} else {
			/*
			 * Allocate first part of segment
			 */
			bp->m_addr += size;
			bp->m_size -= size;
		}
	} else if (bp->m_addr + bp->m_size == addr + size) {
		/*
		 * Allocate last part of segment
		 */
		bp->m_size -= size;
	} else {
		/*
		 * Allocate from middle of segment, but only
		 * if table can be expanded.
		 */
		if (mapfree(mp) == 0)
			return (0);
		for (bp2 = bp; bp2->m_size != 0; bp2++)
			continue;

		/*
		 * Since final m_addr is also m_nam,
		 * set terminating m_size without destroying m_nam
		 */
		bp2[1].m_size = 0;
		bp2--;

		while (bp2 != bp) {
			bp2[1] = bp2[0];
			bp2--;
		}
		mapfree(mp)--;

		bp[1].m_addr = addr + size;
		bp[1].m_size = bp->m_addr + bp->m_size - (addr + size);
		bp->m_size = addr - bp->m_addr;
	}
	return (addr);
}

int
rm_avail(map)
	struct map *map;
{
	register struct mapent *m = mapstart(map);
	register int avail = 0;

	while (m->m_size != 0) {
		avail = MAX(avail, m->m_size);
		m++;
	}
	return (avail);
}
