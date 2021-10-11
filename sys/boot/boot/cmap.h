/*	@(#)cmap.h 1.1 92/07/30 SMI; from UCB 4.5 81/03/09	*/

/*
 * core map entry
 */
struct cmap
{
unsigned int 	c_next:13,	/* index of next free list entry */
		c_prev:13,	/* index of previous free list entry */
		c_type:2,	/* type CSYS or CTEXT or CSTACK or CDATA */
		c_lock:1,	/* locked for raw i/o or pagein */
		c_want:1,	/* wanted */
		c_free:1,	/* on the free list */
		c_gone:1,	/* associated page has been released */
		c_page:16,	/* virtual page number in segment */
		c_hlink:13,	/* hash link for <vp,blkno> */
		c_intrans:1,	/* intransit bit */
		c_blkno:22,	/* disk block this is a copy of */
		c_ndx:10;	/* index of owner proc or text */
struct vnode	*c_vp;		/* vnode to which c_blkno refers */
};

#define	CMHEAD	0

/*
 * Shared text pages are not totally abandoned when a process
 * exits, but are remembered while in the free list hashed by <vp,blkno>
 * off the cmhash structure so that they can be reattached
 * if another instance of the program runs again soon.
 */
#define	CMHSIZ	128		/* SHOULD BE DYNAMIC */
#define	CMHASH(bn)	((bn)&(CMHSIZ-1))

#ifdef	KERNEL
struct	cmap *cmap;
struct	cmap *ecmap;
int	ncmap;
struct	cmap *mfind();
int	firstfree, maxfree;
int	ecmx;			/* cmap index of ecmap */
short	cmhash[CMHSIZ];
#endif

/* bits defined in c_type */

#define	CSYS		0		/* none of below */
#define	CTEXT		1		/* belongs to shared text segment */
#define	CDATA		2		/* belongs to data segment */
#define	CSTACK		3		/* belongs to stack segment */

#define	pgtocm(x)	((((x)-firstfree) / CLSIZE) + 1)
#define	cmtopg(x)	((((x)-1) * CLSIZE) + firstfree)
