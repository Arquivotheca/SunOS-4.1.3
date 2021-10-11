/*	@(#)map.h 1.1 92/07/30 SMI; from UCB 4.7 81/11/08	*/

/*
 * Resource Allocation Maps.
 *
 * Associated routines manage sub-allocation of an address space using
 * an array of segment descriptors.  The first element of this array
 * is a map structure, describing the number of free elements in the array
 * and a count of the number of processes sleeping on an allocation failure.
 * Each additional structure represents a free segment of the address space.
 * The final entry has a zero in the segment size and a pointer to the name
 * of the controlled object in the address field.
 *
 * A call to rminit initializes a resource map and may also be used
 * to free some address space for the map.  Subsequent calls to rmalloc
 * and rmfree allocate and free space in the resource map.  If the resource
 * map becomes too fragmented to be described in the available space,
 * then some of the resource is discarded.  This may lead to critical
 * shortages, but is better than not checking (as the previous versions
 * of these routines did) or giving up and calling panic().  The routines
 * could use linked lists and call a memory allocator when they run
 * out of space, but that would not solve the out of space problem when
 * called at interrupt time.
 *
 * N.B.: The address 0 in the resource address space is not available
 * as it is used internally by the resource map routines.
 */

#ifndef _sys_map_h
#define _sys_map_h

struct map {
	unsigned int m_free;		/* number of free slots in map */
	unsigned int m_want;		/* # of processes sleeping on map */
};

struct mapent {
	int	m_size;		/* size of this segment of the map */
	int	m_addr;		/* resource-space addr of start of segment */
};

struct maplast {
	int	m_size;		/* size of this segment of the map */
	char	*m_nam;		/* name of resource */
/* we use m_nam when the map overflows, in warning messages */
};

#define mapstart(X)	(struct mapent *)((X)+1)
#define mapfree(X)	(X)->m_free
#define mapwant(X)	(X)->m_want
#define mapname(X)	((struct maplast *)(X))->m_nam

#ifdef KERNEL
extern struct	map *kernelmap;
extern struct	map *bufmap;
extern struct	map *heapmap;
#endif

#endif /*!_sys_map_h*/
