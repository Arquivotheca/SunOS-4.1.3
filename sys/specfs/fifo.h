/*	@(#)fifo.h 1.1 92/07/30 SMI;	*/

#ifndef _specfs_fifo_h
#define	_specfs_fifo_h

#ifdef KERNEL
/*
 *	Configuration Parameters
 *
 * These parameters are tuned by editing the system configuration file.
 * The following lines establish the default values.
 */
#ifndef FIFOCNT
#define	FIFOCNT 10	/* number of simultaneously open fifos */
#endif

/*
 * The following parameters are assumed not to require tuning.
 */
#define	FIFOBUF	4096	/* max # bytes stored in a fifo */
#define	FIFOMAX ~0	/* largest size of a single write to a fifo */
#define	FIFOBSZ 4096	/* number of data bytes in each fifo data buffer */
#define	FIFOMNB (FIFOBUF*(FIFOCNT+1))	/* # bytes allowed for all fifos */

/*
 * NOTE: When FIFOBUF == FIFOBSZ, a single buffer is used for each fifo.
 *	Multiple, linked buffers will be used if FIFOBUF > FIFOBSZ.
 *	In this case, FIFOBUF should be a multiple of FIFOBSZ and,
 *	in order to minimize unnecessary fragmentation, FIFOBSZ should
 *	probably be set to a power of 2 minus 4 (e.g., 4092).
 *
 *	Note, also, that this decision is made at compile-time so
 *	the run-time modification of fifoinfo parameters is dangerous.
 */
#if (FIFOBUF > FIFOBSZ)
struct fifo_bufhdr {
	struct fifo_bufhdr *fb_next;	/* ptr to next buffer */
	char fb_data[1];
};
#define	FIFO_BUFHDR_SIZE (sizeof (struct fifo_bufhdr *))
#define	FIFO_BUFFER_SIZE (fifoinfo.fifobsz + FIFO_BUFHDR_SIZE)

#else /*(FIFOBUF == FIFOBSZ)*/
struct fifo_bufhdr {
	union {
		struct fifo_bufhdr *fu_next;
		char fu_data[1];	/* must be at first byte in buffer */
	} fb_u;
};
#define	fb_next fb_u.fu_next
#define	fb_data fb_u.fu_data
#define	FIFO_BUFFER_SIZE (fifoinfo.fifobsz)
#endif

/*
 *	Fifo information structure.
 */
struct fifoinfo {
	int	fifobuf,	/* max # bytes stored in a fifo */
		fifomax,	/* largest size of a single write to a fifo */
		fifobsz,	/* # of data bytes in each fifo data buffer */
		fifomnb;	/* max # bytes reserved for all fifos */
};

int fifo_alloc;			/* total number of bytes reserved for fifos */
struct fifoinfo	fifoinfo;	/* fifo parameters */

#endif KERNEL
#endif /*!_specfs_fifo_h*/
