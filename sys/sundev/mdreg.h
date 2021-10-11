/*	@(#)mdreg.h 1.1 92/07/30 SMI	*/

#ifndef _sundev_mdreg_h
#define _sundev_mdreg_h

#include	<sys/ioccom.h>

/*
 * This is the current maximum number of real disks per Virtual Disk.
 */
#define MAX_MD_ROWS	4		/* max rows of disks */
#define	MAX_MD_DISKS	8		/* max real disks per virtual disk row */

/*
 * The md_struct struct describes one minor device of the Virtual Disk.   Each
 * minor device is a meta-partitions, which can be striped across several rows
 * each consisting of an array of real partitions, each real partition being
 * a part or a whole of a spindle.
 *
 * Here is the function of each field:
 *
 * md_status:		Tells what state the meta device is currently in.
 *
 *		MD_CLEAR  - Device has not bee configured or has been closed down.
 *			    Mdopen() should return 0, but no operation is allowed
 *			    except to set up the device.
 *
 *		MD_IOCOPS - No operation is currently allowed on this minor 
 *			    device except to carry out the ioctl which sets
 *			    up the configuration.
 *
 *		MD_SETUP  - The minor device has been correctly set up. This
 *			    ought to be the state before any reads and writes
 *			    are allowed.
 *
 *		MD_MIRROR - This minor device is being mirrored by another 
 *			    minor device, which must have previously been
 *			    set up.
 *
 * md_ndisks:		Total number of real partitions listed in all the
 *			rows for this meta-partition.
 *
 * md_rows:		Total number of rows for this meta-partition.
 *
 * md_mirror:		Is this meta-partition mirrored onto another
 *			meta-partition?
 *
 * For each row:
 *
 *	md_width:		Width of this row of the meta-partition	in real 
 *				partitions.
 *
 *	md_row_disks:		Width of this row of the meta-partition in real 
 *				partitions.
 *
 *	md_blocks:		Total number of blocks in this row.   This is used 
 *				in calculating the cumulative total of blocks at 
 *				each row.
 *
 *	md_cum_blocks:		The cumulative number of blocks up to this row.
 *				This is used in calculating which row the current
 *				block falls into.
 *
 *	md_start_block:		The starting block number in the real partition.
 *
 *	md_end_block:		Ending block in the real partition.
 *
 *	md_real_disks[]:	List of real partition device numbers.
 *
 */
struct md_struct	{
		int	md_status;			/* indicates what operations are
							   in progress, or allowed. */
		int	md_ndisks;			/* total disks */
		int	md_rows;			/* number of rows */
		int	md_parity_interval;		/* parity interlace. */
		dev_t	md_mirror;			/* device mirroring this one */
	struct md_row	{
		int	md_width;			/* striping width for files on
							   on this minor device. */
		int	md_row_disks;			/* number of real disks */
		int	md_blocks;			/* total blocks this row */
		int	md_data_blocks;			/* not including parity */
		int	md_cum_blocks;			/* cumulative blocks in rows
							   	including this one */
		int	md_cum_data_blocks;		/* not including parity */
		int	md_start_block;			/* starting block in real disk */
		int	md_end_block;			/* ending block in real disk */
		dev_t	md_real_disks[MAX_MD_DISKS];	/* list of real disks */
	} md_real_row[MAX_MD_ROWS];
};

/*
 * Flags per minor device which indicate current status, or which
 * say what operations are allowed.
 */
#define	MD_IOCOPS	0x1		/* Only VD_IOCGET and VD_IOCSET allowed */
#define MD_SETUP	0x2		/* device properly set up */
#define MD_CLEAR	0x4		/* device not configured */
#define MD_MIRROR	0x1000		/* this meta-device is *being* mirrored.
					   The *mirroring* device is md_mirror */
#define MD_PARITY	0x2000		/* do parity generation and checking */

#define B_CALL		0x00400000	/* call b_iodone */
#define B_RAW_FRAG	0x00800000	/* this is a fragment of a raw read/write */

#define DKC_MD		12

/*
 * At a later stage, we may decide to have extra bits in the
 * dev_t, and then we should mask them out in this macro.
 */
#define	MD_MINOR(dev)	minor(dev)

/*
 * ioctl codes.
 */
#define	MD_IOCGET	_IOR(V, 0, struct md_struct)		/* get description */
#define MD_IOCSET	_IOW(V, 1, struct md_struct)		/* set description */

/*
 * Data per unit.
 */
struct mdunit {
	struct  buf	*un_rtab;		/* for physio */
};


/*
 * Struct in which we save some essential information from the buffer
 * while passing it down to the real disk driver.
 */
struct	md_save	{
	struct md_save		*md_next;
	struct buf		*md_bp;			/* point to the real buffer */
	int			md_frags;		/* number of fragments for raw */
	int			md_frag_chars;		/* characters in fragments */
	int			md_trace_ind;
	struct buf		md_buf;			/* the temporary buffer */
};

int	mddone();

#define	MD_SAVE(mds, bp)	mds = mdget(); \
				if (mds->md_buf.b_flags & B_MD_USING) \
					panic ("B_MD_USING"); \
				mds->md_buf = *bp; \
				mds->md_buf.b_flags |= B_MD_USING; \
				mds->md_buf.b_forw = (struct buf *)0; \
				mds->md_buf.b_back = (struct buf *)0; \
				mds->md_buf.av_forw = (struct buf *)0; \
				mds->md_buf.b_chain = (struct buf *)0; \
				mds->md_frags = 0; \
				mds->md_frag_chars = 0;

#define MD_RESTORE(bp, mds)	if ((mds->md_buf.b_flags & B_MD_USING) == 0) \
					panic ("not B_MD_USING"); \
				bp->av_back = mds->md_buf.av_back; \
				bp->b_resid = mds->md_buf.b_resid; \
				bp->b_error = mds->md_buf.b_error; \
				mdfree (mds);


/*
 * Fake I/O space registers - byte accesses only
 */
struct	mddevice	{
	u_char		md_csr;
};

#define B_MD_USING	0x01000000

#endif /*!_sundev_mdreg_h*/
