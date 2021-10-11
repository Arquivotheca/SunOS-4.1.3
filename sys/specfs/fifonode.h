/*	@(#)fifonode.h 1.1 92/07/30 SMI;	*/

#ifndef _specfs_fifonode_h
#define	_specfs_fifonode_h

#ifdef KERNEL

/* fifonodes start with an snode so that 'spec_xxx()' routines work */
struct fifonode {
	struct snode	fn_snode;	/* must be first */
	struct fifo_bufhdr *fn_buf;	/* ptr to first buffer */
	struct fifo_bufhdr *fn_bufend;	/* ptr to last buffer */
	struct proc	*fn_rsel;	/* ptr to read selector */
	struct proc	*fn_wsel;	/* ptr to write selector */
	struct proc	*fn_xsel;	/* ptr to exception selector */
	u_long		fn_size;	/* number of bytes in fifo */
	short		fn_wcnt;	/* number of waiting readers */
	short		fn_rcnt;	/* number of waiting writers */
	short		fn_wptr;	/* write offset */
	short		fn_rptr;	/* read offset */
	short		fn_flag;	/* (see below) */
	short		fn_nbuf;	/* number of buffers allocated */
};

#define	fn_vnode	fn_snode.s_vnode

/* bits in fn_flag in fifonode */
#define	FIFO_RBLK	0x0001	/* blocked readers */
#define	FIFO_WBLK	0x0002	/* blocked writers */
#define	FIFO_RCOLL	0x0004	/* more than one read selector */
#define	FIFO_WCOLL	0x0008	/* more than one write selector */
#define	FIFO_XCOLL	0x0010	/* more than one exception selector */

/*
 * Convert between fifonode, snode, and vnode pointers
 */
#define	VTOF(VP)	((struct fifonode *)(VP)->v_data)
#define	FTOV(FP)	(&(FP)->fn_vnode)
#define	FTOS(FP)	(&(FP)->fn_snode)


/* define fifonode handling routines */
#define	FIFOMARK(fp, x)	smark(FTOS(fp), x)
#define	FIFOLOCK(fp)	SNLOCK(FTOS(fp))
#define	FIFOUNLOCK(fp)	SNUNLOCK(FTOS(fp))

#endif KERNEL
#endif /*!_specfs_fifonode_h*/
