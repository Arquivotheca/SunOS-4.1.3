/* @(#)buf.h 1.1 92/07/30 SMI; from UCB 4.22 83/07/01 */

#ifndef _sys_buf_h
#define	_sys_buf_h

/*
 * The header for buffers in the buffer pool and otherwise used
 * to describe a block i/o request is given here.  The routines
 * which manipulate these things are given in vfs_bio.c.
 *
 * Buffers are used for control (superblocks, cylinder groups,
 * indirect blocks, inodes), and pages.
 *
 * Each control cache buffer in the pool is usually doubly linked into 2 lists:
 * hashed into a chain by <vp, blkno> so it can be located in the cache,
 * and (usually) on (one of several) queues.  These lists are circular and
 * doubly linked for easy removal.
 *
 * There are currently three queues for control cache buffers:
 * 	one for buffers containing ``useful'' information (the cache)
 *	one for buffers containing ``non-useful'' information
 *	one for empty buffers
 *
 * Buffers are kept in lru order.  When not on one of these queues,
 * the buffers are ``checked out'' to drivers which use the available list
 * pointers to keep track of them in their i/o active queues.
 */

/*
 * Bufhd structures used at the head of the hashed buffer queues.
 * We only need three words for these, so this abbreviated
 * definition saves some space.
 */
struct bufhd {
	long	b_flags;		/* see defines below */
	struct	buf *b_forw, *b_back;	/* fwd/bkwd pointer in chain */
};

/*
 * Diskhd structures used at the head of the disk unit queues.
 * We only need a few elements for these, so this abbreviated
 * definition saves some space.
 */
struct diskhd {
	long b_flags;			/* not used, needed for consistency */
	struct	buf *b_forw, *b_back;	/* queue of unit queues */
	struct	buf *av_forw, *av_back;	/* queue of bufs for this unit */
	long	b_bcount;		/* active flag */
};

struct buf {
	long	b_flags;		/* too much goes here to describe */
	struct	buf *b_forw, *b_back;	/* hash chain (2 way street) */
	struct	buf *av_forw, *av_back;	/* position on free list if not BUSY */
#define	b_actf	av_forw			/* alternate names for driver queue */
#define	b_actl	av_back			/*    head - isn't history wonderful */
	long	b_bcount;		/* transfer count */
	long	b_bufsize;		/* size of allocated buffer */
#define	b_active b_bcount		/* driver queue head: drive active */
	short	b_error;		/* returned after I/O */
	dev_t	b_dev;			/* major+minor device name */
	union {
	    caddr_t b_addr;		/* low order core address */
	    int	*b_words;		/* words for clearing */
	    struct fs *b_fs;		/* superblocks */
	    struct csum *b_cs;		/* superblock summary information */
	    struct cg *b_cg;		/* cylinder group block */
	    struct dinode *b_dino;	/* ilist */
	    daddr_t *b_daddr;		/* indirect block */
	} b_un;
	daddr_t	b_blkno;		/* block # on device */
	long	b_resid;		/* bytes not transferred after error */
#define	b_errcnt b_resid		/* while i/o in progress: # retries */
	struct  proc *b_proc;		/* proc doing physical or swap I/O */
	int	(*b_iodone)();		/* function called by iodone */
	struct	vnode *b_vp;		/* vnode associated with block */
	struct	page *b_pages;		/* page list for PAGEIO */
	struct buf *b_chain;		/* chain together all buffers here */
	int	b_mbinfo;		/* mb resources from mbpresetup() */
};

struct bstats {
	int	n_bread;		/* total bread requests */
	int	n_bread_hits;		/* hits in cache (no strategy needed) */
	int	n_ages;			/* times an aged buf was allocated */
	int	n_lrus;			/* times an lru buf was allocated */
	int	n_sleeps;		/* times had to sleep for a buf */
};

#define	BQUEUES		4	/* number of free buffer queues */

#define	BQ_SLEEP	0	/* address to sleep on if no bufs available */
#define	BQ_LRU		1	/* lru, useful buffers */
#define	BQ_AGE		2	/* rubbish */
#define	BQ_EMPTY	3	/* buffer headers with no memory */

#ifdef	KERNEL
#define	BUFHSZ	63
#define	RND	(MAXBSIZE/DEV_BSIZE)
#define	BUFHASH(dvp, dblkno)	\
	((struct buf *)&bufhash[((u_int)(dvp)+(((int)(dblkno))/RND)) % BUFHSZ])

int	nbuf;			/* min number of buffer headers to allocate */
int	bufalloc;		/* number of allocated buffers */
struct	bufhd bufhash[BUFHSZ];	/* heads of hash lists */
struct	buf bfreelist[BQUEUES];	/* heads of available lists */
struct	buf *bufchain;		/* head of chain of all buffers in the system */
struct	buf *bclnlist;		/* head of cleaned page list */

struct	buf *getblk();
struct	buf *geteblk();
struct	buf *getpermeblk();
struct	buf *getnewbuf();
struct	buf *bread();
struct	buf *pageio_setup();
struct	vnode *bdevvp();

void	minphys();
void	blkflush();
#endif

/*
 * These flags are kept in b_flags.
 */
#define	B_WRITE		0x00000000	/* non-read pseudo-flag */
#define	B_READ		0x00000001	/* read when I/O occurs */
#define	B_DONE		0x00000002	/* transaction finished */
#define	B_ERROR		0x00000004	/* transaction aborted */
#define	B_BUSY		0x00000008	/* not on av_forw/back list */
#define	B_PHYS		0x00000010	/* physical IO */
#define	B_PAGEIO	0x00000020	/* do I/O to pages on bp->p_pages */
#define	B_WANTED	0x00000040	/* issue wakeup when BUSY goes off */
#define	B_DONTNEED	0x00000080	/* after write, need not be cached */
#define	B_ASYNC		0x00000100	/* don't wait for I/O completion */
#define	B_DELWRI	0x00000200	/* write at exit of avail list */
#define	B_TAPE		0x00000400	/* this is a magtape (no bdwrite) */
#define	B_REMAPPED	0x00001000	/* buffer is kernel addressable */
#define	B_FREE		0x00002000	/* free page when done */
#define	B_PGIN		0x00004000	/* pagein op, so swap() can count it */
#define	B_CACHE		0x00008000	/* did bread find us in the cache ? */
#define	B_INVAL		0x00010000	/* does not contain valid info  */
#define	B_FORCE		0x00020000	/* semi-permanent removal from cache */
#define	B_HEAD		0x00040000	/* a buffer header, not a buffer */
#define	B_NOCACHE	0x00080000	/* don't cache block when released */
#define	B_BAD		0x00100000	/* bad block revectoring in progress */
#define	B_CALL		0x00400000	/* call b_iodone */
#define	B_NODELAY	0x00800000	/* Don't stall the write for clust */
#define	B_KLUSTER	0x01000000	/* this buffer is a kluster header */

/*
 * Insq/Remq for the buffer hash lists.
 */
#define	bremhash(bp) { \
	(bp)->b_back->b_forw = (bp)->b_forw; \
	(bp)->b_forw->b_back = (bp)->b_back; \
}
#define	binshash(bp, dp) { \
	(bp)->b_forw = (dp)->b_forw; \
	(bp)->b_back = (dp); \
	(dp)->b_forw->b_back = (bp); \
	(dp)->b_forw = (bp); \
}

/*
 * Insq/Remq for the buffer free lists.
 */
#define	bremfree(bp) { \
	(bp)->av_back->av_forw = (bp)->av_forw; \
	(bp)->av_forw->av_back = (bp)->av_back; \
}
#define	binsheadfree(bp, dp) { \
	(dp)->av_forw->av_back = (bp); \
	(bp)->av_forw = (dp)->av_forw; \
	(dp)->av_forw = (bp); \
	(bp)->av_back = (dp); \
}
#define	binstailfree(bp, dp) { \
	(dp)->av_back->av_forw = (bp); \
	(bp)->av_back = (dp)->av_back; \
	(dp)->av_back = (bp); \
	(bp)->av_forw = (dp); \
}

/*
 * Take a buffer off the free list it's on and
 * mark it as being use (B_BUSY) by a device.
 */
#define	notavail(bp) { \
	int x = spl6(); \
	bremfree(bp); \
	(bp)->b_flags |= B_BUSY; \
	(void) splx(x); \
}

#define	iodone	biodone
#define	iowait	biowait

/*
 * Zero out a buffer's data portion.
 */
#define	clrbuf(bp) { \
	blkclr(bp->b_un.b_addr, (u_int)bp->b_bcount); \
	bp->b_resid = 0; \
}

#endif /*!_sys_buf_h*/
