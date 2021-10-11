#ifndef lint
static	char sccsid[] = "@(#)md.c 1.1 92/07/30 SMI";
#endif

#include "md.h"
#if NMD > 0

/*
 * Md - is the meta-disk driver.   It sits below the UFS file system
 * but above the 'real' disk drivers,  xy,  sd etc.
 *
 * To the UFS software, md looks like a normal driver, since it has
 * the normal kinds of entries in the bdevsw and cdevsw arrays.  So
 * UFS accesses md in the usual ways,  in particular,  the strategy
 * routine, mdstrategy(), gets called by fbiwrite(), ufs_getapage(),
 * and ufs_write().
 *
 * Md maintains an array of minor devices (meta-partitions).   Each
 * meta partition stands for a matrix of real partitions, in rows
 * which are not necessarily of equal length.   Md maintains a table,
 * with one entry for each meta-partition,  which lists the rows and
 * columns of actual partitions, and the job of the strategy routine
 * is to translate from the meta-partition device and block numbers
 * known to UFS into the actual partitions' device and block numbers.
 *
 * See below, in mdstrategy(), ndreal(),  and mddone() for details of
 * this translation.
 */

/*
 * Debugging and other defines.
 */
#define	MD_DEBUG	1
#define	MD_STATS	0

/*
 * Driver for Virtual Disk.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/dkbad.h>
#include <sys/file.h>
#include <sys/trace.h>
#include <sun/dkio.h>

#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>

#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>

#include <sundev/mbvar.h>
#include <sundev/mdreg.h>

/*
 * Local variables.
 */
struct md_save	*md_free_list = (struct md_save *)0;
struct buf	*md_raw_free_list = (struct buf *)0;
int		md_alloc_size = 16;
int		md_in_use = 0;
int		md_raw_in_use = 0;
caddr_t		md_parity_block = (caddr_t)0;
struct buf	*md_parity_buf = (struct buf *)0;

#if MD_STATS > 0
#define	MAX_TRACE	2048
struct {
	dev_t		tr_dev[2];
	int		tr_flags;
	int		tr_blk[2];
	struct buf	*tr_bufp[2];
	struct md_save	*tr_save;
	long		tr_secs;
	long		tr_usecs[2];
} md_trace[MAX_TRACE];
int	md_trace_index = 0;
#endif MD_STATS

#if MD_STATS > 0
/*
 * Store the maximum number of local structures that we generate.
 */
int		max_md_raw_in_use = 0;
int		max_md_in_use = 0;
#endif

int	mdprobe(),	mdslave(),	mdattach(),	mdgo(),	mddone(),	mdpoll();

struct mb_driver	mdcdriver = {
	mdprobe,	mdslave,	mdattach,	mdgo,	mddone,	mdpoll,
	sizeof (struct mddevice), "md", 0, "mdc", 0, MDR_BIODMA | MDR_PSEUDO,
};

/*
 * Bufs used by physio().
 */
struct mdunit		md_units[NMD];

/*
 * See the description in mdreg.h for the function of each field of the
 * md_struct structure shown here.
 */
struct md_struct	md_conf[NMD];


/*
 * Determine existence of a device.
 *
 * Called by:
 *
 */
mdprobe(reg, ctlr)
	caddr_t		reg;
	int		ctlr;
{
	printf("mdprobe(reg 0x%x, ctlr 0x%x)\n", reg, ctlr);

return (sizeof (struct mddevice));
}

/*
 * See if a slave unit exists.
 *
 * Called by:
 *
 */
mdslave(md, reg)
	register struct mb_device	*md;
	caddr_t				reg;
{
	printf("mdslave(md 0x%x, reg 0x%x)\n", md, reg);

return (1);
}

/*
 * This routine is used to attach a drive to the system.
 *
 * Called by:
 *
 */
mdattach(md)
	register struct mb_device	*md;
{
	printf("mdattach(md 0x%x)\n", md);
}


mdvalid(mdstruct)
	struct md_struct	*mdstruct;
{
	int			row;
	struct md_row		*mdrow;
	int			i;
	dev_t			real_dev;
	int			size;


	/*
	 * Make sure that we have a somewhat valid Virtual Disk description
	 * record for this minor device of the Virtual Disk.
	 */
	if (mdstruct->md_mirror) {
		printf("mdvalid: md_mirror 0x%x\n", mdstruct->md_mirror);
	}
	for (row = 0; row < mdstruct->md_rows; row++) {
		mdrow = &mdstruct->md_real_row[row];
		if ((mdrow->md_row_disks < 0) ||
		    (mdrow->md_row_disks > MAX_MD_DISKS)) {
			printf("mdvalid: illegal entry md_ndisks 0x%x MAX_MD_DISKS 0x%x\n",
				mdrow->md_row_disks, MAX_MD_DISKS);
			return (ENXIO);
		}
		if (mdrow->md_end_block & 0xf) {
			printf("mdvalid: not a complete block 0x%x\n",
				mdrow->md_end_block);
		}
		for (i = 0; i < mdrow->md_row_disks; i++) {
			real_dev = mdrow->md_real_disks[i];
			/*
			 * Must check the major device for validity.  The
			 * underlying block device driver will check the
			 * minor device as if it were a regular open.
			 */
			if (major(real_dev) >= nblkdev) {
				printf("mdvalid: bad real disk 0x%x\n",
					real_dev);
				return (ENXIO);
			}
			size = (*bdevsw[major(real_dev)].d_psize)(real_dev);
			if (size < mdrow->md_end_block) {
				printf("mdvalid: dev 0x%x size 0x%x end_block 0x%x\n",
				real_dev, size, mdrow->md_end_block);
				return (ENXIO);
			}
		}
		if ((mdrow->md_width <= 0) ||
		    (mdrow->md_width > mdrow->md_row_disks)) {
			printf("mdvalid: illegal md_width 0x%x md_ndisks 0x%x\n",
				mdrow->md_width, mdrow->md_row_disks);
		return (ENXIO);
		}
	}
	/*
	 * Make sure that this Unit of the Virtual Disk was really configured.
	 * If not, then all we allow is an ioctl.
	 */
	if ((mdstruct->md_ndisks <= 0) || (mdstruct->md_rows <= 0)) {
		printf("mdvalid:  no entry: may only open for ioctl\n");
		mdstruct->md_status |= MD_IOCOPS;
		return (ENXIO);
	}
	mdstruct->md_status &= ~MD_IOCOPS;
	return (0);
}

/*
 * This routine opens one minor device of the Virtual Disk System.   The
 * particular minor device of the Virtual Disk which is being opened is
 * described by one entry in the md_struct structure.
 *
 * Called by:
 *
 */
mdopen(dev, flag)
	dev_t		dev;
	int		flag;
{
	struct md_struct	*mdstruct;
	struct md_row		*mdrow;
	dev_t			real_dev;
	int			row;
	int			i;
	int			err;

	trace2(TR_MD_OPEN, dev, flag);
	printf("mdopen(dev 0x%x, flag 0x%x)\n", dev, flag);

	/*
	 * Make sure that the minor device is a valid part of the Virtual
	 * Disk subsystem.
	 */
	if (MD_MINOR(dev) >= NMD) {
		printf("mdopen: dev 0x%x bad minor device 0x%x\n",
			dev, MD_MINOR(dev));
		return (ENXIO);
	}
	if (md_units[MD_MINOR(dev)].un_rtab == (struct buf *)0) {
		md_units[MD_MINOR(dev)].un_rtab =
			(struct buf *)new_kmem_alloc(
					sizeof (struct buf), KMEM_SLEEP);
		bzero(md_units[MD_MINOR(dev)].un_rtab, sizeof (struct buf));
	}
	mdstruct = &md_conf[MD_MINOR(dev)];
	if ((err = mdvalid(mdstruct)) != 0) {
		printf("mdopen: dev 0x%x bad\n", dev);
	}
	/*
	 * This device is only to be opened for ioctls.
	 */
	if (mdstruct->md_status & MD_IOCOPS) return (0);
	/*
	 * This device has not been properly set up.
	 */
	if ((mdstruct->md_status & MD_SETUP) == 0) {
		printf("mdopen: bad open\n");
		return (ENXIO);
	}
	if (mdstruct->md_parity_interval != 0)
		printf("mdopen: md_parity_interval %d\n",
			mdstruct->md_parity_interval);
	/*
	 * Make sure that each real disk listed in the md_struct entry for
	 * this unit is plausible, and also make sure that we can open each
	 * underlying real disk
	 *
	 */
	for (row = 0; row < mdstruct->md_rows; row++) {
		mdrow = &mdstruct->md_real_row[row];
		for (i = 0; i < mdrow->md_row_disks; i++) {
			real_dev = mdrow->md_real_disks[i];
			/*
			 * Must check the major device for validity.  The
			 * underlying block device driver will check the
			 * minor device as if it were a regular open.
			 */
			if (major(real_dev) >= nblkdev) {
				printf("mdopen: row minor 0x%x bad real disk 0x%x\n",
					i, real_dev);
				return (ENXIO);
			}
			err = (*bdevsw[major(real_dev)].d_open)(real_dev, flag);
			if (err) {
				printf("\tmopen: row minor 0x%x real_dev 0x%x bad open 0x%x\n",
					i, real_dev, err);
				return (err);
			}
			printf("mdopen: opened 0x%x\n", real_dev);
		}
	}
	return (err);
}

/*
 * This routine returns the size of a logical partition.  It is called
 * from the device switch at normal priority.
 *
 * Called by:
 *
 */
int
mdsize(dev)
	dev_t		dev;
{
	struct md_struct	*mdstruct;
	struct md_row		*mdrow;
	int			i;
	int			row;
	int			total_size = 0;
	int			size = 0;
	dev_t			real_dev;

	printf("mdsize(dev 0x%x)\n", dev);


	/*
	 * Make sure that the minor device is a valid part of the Virtual
	 * Disk subsystem.
	 */
	if (MD_MINOR(dev) >= NMD) {
		printf("mdsize: dev 0x%x bad minor device 0x%x\n",
			dev, MD_MINOR(dev));
		return (ENXIO);
	}
	mdstruct = &md_conf[MD_MINOR(dev)];

	for (row = 0; row < mdstruct->md_rows; row++) {
		mdrow = &mdstruct->md_real_row[row];
		for (i = 0; i < mdrow->md_row_disks; i++) {
			real_dev = mdrow->md_real_disks[i];
			/*
			 * Must check the major device for validity.  The
			 * underlying block device driver will check the
			 * minor device as if it were a regular open.
			 */
			if (major(real_dev) >= nblkdev) {
				printf("mdsize: dev 0x%x bad real disk 0x%x\n",
					dev, real_dev);
				return (ENXIO);
			}
			printf("mdsize: dev 0x%x real disk 0x%x",
				dev, real_dev);
			size = (*bdevsw[major(real_dev)].d_psize)(real_dev);
			printf(" size 0x%x\n", size);
			total_size += size;
		}
	}
	trace2(TR_MD_SIZE, dev, total_size);
	printf("mdsize: total_size 0x%x\n", total_size);
	return (total_size);
}

/*
 * Create and return the address of a buffer structure which we
 * can use to hold a fragment of a longer raw read/write/
 */
struct buf	*
md_raw_get()
{
	struct buf	*mdr;

	/*
	 * This will panic higher up if I can't get one, so don't
	 * test for NULL returns here.
	 */
	mdr = (struct buf *)new_kmem_fast_alloc((caddr_t *)&md_raw_free_list,
	    sizeof (*mdr), md_alloc_size, KMEM_SLEEP);
	bzero((caddr_t)mdr, sizeof (*mdr));
	md_raw_in_use++;
#if MD_STATS > 0
	if (md_raw_in_use > max_md_raw_in_use) {
		max_md_raw_in_use = md_raw_in_use;
#if MD_DEBUG > 0
		printf("md_raw_get: max_md_raw_in_use 0x%x\n",
			max_md_raw_in_use);
#endif
	}
#endif
	return (mdr);
}

/*
 * Reclaim a buffer structure.
 */
md_raw_free(mdr)
	struct buf	*mdr;
{
	kmem_fast_free((caddr_t *)&md_raw_free_list, (caddr_t)mdr);
	md_raw_in_use--;
	return;
}

/*
 * Create and return the address of a save structure in which we
 * save the details of the current transfer.
 */
struct md_save	*
mdget()
{
	struct md_save	*mds;

	/*
	 * This will panic higher up if I can't get one, so don't
	 * test for NULL returns here.
	 */
	mds = (struct md_save *)new_kmem_fast_zalloc((caddr_t *)&md_free_list,
	    sizeof (*mds), md_alloc_size, KMEM_SLEEP);
	md_in_use++;
#if MD_STATS > 0
	if (md_in_use > max_md_in_use) {
		max_md_in_use = md_in_use;
#if MD_DEBUG > 0
		printf("md_get: max_md_in_use 0x%x\n",
			max_md_in_use);
#endif
	}
#endif
	return (mds);
}

mdfree(mds)
	struct md_save	*mds;
{
	kmem_fast_free((caddr_t *)&md_free_list, (caddr_t)mds);
	md_in_use--;
	return;
}

mdsavedump(mds)
	struct md_save	*mds;
{
	printf("    mdsavedump: mds 0x%x  md_bp 0x%x  md_frags 0x%x  md_frag_chars 0x%x\n",
		mds,
		mds->md_bp,
		mds->md_frags,
		mds->md_frag_chars);
	mddumpbuf("mdsavedump", &(mds->md_buf));

}

mddumpbuf(str, bp)
	char		*str;
	struct buf	*bp;
{
	printf("    mddumpbuf(%s): bp 0x%x  av_back 0x%x  b_flags 0x%x  b_bcount 0x%x  b_error 0x%x  b_dev 0x%x  b_blkno 0x%x  b_iodone 0x%x b_addr 0x%x\n",
		str,
		bp,
		bp->av_back,
		bp->b_flags,
		bp->b_bcount,
		bp->b_error,
		bp->b_dev,
		bp->b_blkno,
		bp->b_iodone,
		bp->b_un.b_addr);
}

/*
 * This routine is the high level interface to the Virtual Disk.  It
 * performs reads and writes on the disk using the buf as the method
 * of communication.    It is called from the device switch for block
 * operations and via physio() for raw operations.  It is called at
 * normal priority.
 *
 * We save the original buffer header so that it is still in the
 * buffer cache in case the kernel comes looking for it and create
 * a new buffer header in *mdsave which is a duplicate of the old.
 *
 * We also save a pointer back to the original buffer header so that
 * on completion we can go and make it look as though it went through
 * the driver before sending it to iodone().
 *
 * Called by:
 *	ufs_getapage();
 *	ufs_writelbn()
 */
mdstrategy(bp)
	register struct buf	*bp;
{
	struct md_struct	*mdstruct;
	struct md_save		*mdsave;
	int			s;
	daddr_t			fragment;
	long			total_count, this_count;
	daddr_t			this_blkno;
	caddr_t			this_address;
	struct buf		*mdraw;
	dev_t			dev,
				mirror_dev = 0;
	int			primary = 1;


	dev = bp->b_dev;
	/*
	 * Make sure that the minor device is a valid part of the Virtual
	 * Disk subsystem.
	 */
	if (MD_MINOR(dev) >= NMD) {
		printf("mdstrategy: dev 0x%x bad minor device 0x%x\n",
			dev, MD_MINOR(dev));
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	mdstruct = &md_conf[MD_MINOR(dev)];
	if ((mdstruct->md_status & MD_SETUP) == 0) {
		printf("mdstrategy: dev 0x%x not set up\n", dev);
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	while (mdstruct != (struct md_struct *)0) {
		s = spl6();
		/*
		 * Save essential information.
		 */
		MD_SAVE(mdsave, bp);
		if (primary) {
			bp->av_back = (struct buf *)mdsave;
			mdsave->md_bp = bp;
		} else {
			mdsave->md_bp = (struct buf *)0;
			mdsave->md_buf.b_dev = mirror_dev;
		}
		(void) splx(s);

#if MD_STATS > 0
		mdsave->md_trace_ind = md_trace_index;
		md_trace[md_trace_index].tr_flags = bp->b_flags;
		md_trace[md_trace_index].tr_save = mdsave;
		md_trace[md_trace_index].tr_bufp[0] = bp;
		md_trace[md_trace_index].tr_bufp[1] = &(mdsave->md_buf);
		md_trace[md_trace_index].tr_dev[0] = bp->b_dev;
		md_trace[md_trace_index].tr_blk[0] = bp->b_blkno;
		md_trace[md_trace_index].tr_secs = time.tv_sec;
		md_trace[md_trace_index].tr_usecs[0] = time.tv_usec;
		md_trace_index =
		    (md_trace_index == (MAX_TRACE-1)) ? 0 : md_trace_index + 1;
#endif MD_STATS

		/*
		 * Make sure that completion comes back to us, so we can clean up.
		 */
		mdsave->md_buf.b_iodone = mddone;
		mdsave->md_buf.b_flags |= B_CALL;

		/*
		 * Remember where it's saved.
		 */
		mdsave->md_buf.av_back = (struct buf *)mdsave;

		/*
		 * Handle very long read/write operations correctly.  The catch
		 * is that we can be called from physio() which hands us a
		 * buffer asking for a read or write which is up to 63kb in
		 * length.   It's OK to send this to the driver in one piece
		 * on a single unstriped partition, but we have to break it up
		 * into file system block-sized reads and  writes since we are
		 * striping.   the read/write might look like this:
		 *
		 *	_________________________________________________
		 *	:   |   |	|	|	|	|     | :
		 *	:   |<--------------------------------------->| :
		 *	:___|___|_______|_______|_______|_______|_____|_:
		 *	      n     8k      8k      8k      8k    m
		 *
		 * The method is to do one read/write of (n) bytes, several of
		 * 8k, and then one of (m) bytes.   The essential is not to have
		 * a single read/write which spans file system blocks, so all
		 * we have to do is to break up the original request into single
		 * buffer reads/writes, and pass them down to mdreal() which
		 * will figure out to which real partition and block number to
		 * distribute them.    Unfortunately, this is a little slower
		 * than passing a single huge read/write down.
		 */
		fragment = bp->b_blkno & 0xf;
		if ((bp->b_bcount > ((0x10 - fragment) * DEV_BSIZE))) {
			for (total_count = bp->b_bcount,
			    this_count = (0x10 - fragment) * DEV_BSIZE,
			    this_blkno = bp->b_blkno,
			    this_address = bp->b_un.b_addr;
			    total_count > 0; ) {
				s = spl6();
				/*
				 * Send down a buffer for the current fragment
				 * of the raw read/write.
				 */
				mdraw = md_raw_get();
				(void) splx(s);
				*mdraw = *bp;
				mdraw->b_flags = bp->b_flags | B_RAW_FRAG | B_CALL;
				mdraw->av_back = (struct buf *)mdsave;
				mdraw->b_bcount = this_count;
				if (primary)
					mdraw->b_dev = bp->b_dev;
				else
					mdraw->b_dev = mirror_dev;
				mdraw->b_un.b_addr = this_address;
				mdraw->b_blkno = this_blkno;
				mdraw->b_iodone = mddone;
				mdraw->b_pages = (struct page *)0;
				mdraw->b_chain = (struct buf *)0;
				mdsave->md_frags++;
				mdreal (mdraw);
				/*
				 * Update the parameters for the next fragment.
				 */
				total_count = total_count - this_count;
				this_address = this_address + this_count;
				this_blkno = this_blkno + (this_count / DEV_BSIZE);
				this_count = min(total_count, 0x10 * DEV_BSIZE);
			}
			goto mdstrategyout;
		}
		/*
		 * Call the real driver.
		 */
		mdreal (&(mdsave->md_buf));
mdstrategyout:
		if ((mdstruct->md_mirror) && ((bp->b_flags & B_READ) == 0)) {
			mirror_dev = mdstruct->md_mirror;
			mdstruct = &md_conf[MD_MINOR(mirror_dev)];
		}

		else
			mdstruct = (struct md_struct *)0;
		primary = 0;
	}
	return;
}

mdread_parity(dev, sector)
	dev_t		dev;
	daddr_t		sector;
{
	printf("mdread_parity(dev 0x%x sector 0x%x)\n",
		dev, sector);

	if (md_parity_block == (caddr_t)0) {
		md_parity_block = new_kmem_alloc(0x10 * DEV_BSIZE, KMEM_SLEEP);
		printf("mdread_parity: md_parity_block 0x%x\n", md_parity_block);
	}

	if (md_parity_buf == (struct buf *)0) {
		md_parity_buf = (struct buf *)new_kmem_alloc(
					sizeof (struct buf), KMEM_SLEEP);
		printf("mdread_parity: md_parity_buf 0x%x\n", md_parity_buf);
		md_parity_buf->b_forw = (struct buf *)0;
		md_parity_buf->b_back = (struct buf *)0;
		md_parity_buf->av_forw = (struct buf *)0;
		md_parity_buf->av_back = (struct buf *)0;
		md_parity_buf->b_chain = (struct buf *)0;
		md_parity_buf->b_bcount = 0x10 * DEV_BSIZE;
		md_parity_buf->b_bufsize = 0x10 * DEV_BSIZE;
		md_parity_buf->b_error = 0;
		md_parity_buf->b_iodone = (int)0;
		md_parity_buf->b_vp = (struct vnode *)0;
		md_parity_buf->b_pages = (struct page *)0;
		md_parity_buf->b_chain = (struct buf *)0;
		md_parity_buf->b_mbinfo = 0;
	}
	md_parity_buf->b_dev = dev;
	md_parity_buf->b_blkno = sector;
	md_parity_buf->b_un.b_addr = md_parity_block;
	md_parity_buf->b_resid = 0;
	(*bdevsw[major(md_parity_buf->b_dev)].d_strategy)(md_parity_buf);
}

/*
 * Accept the duplicate buffer header which initially contains the
 * same information (dev and block number) passed down from the UFS
 * layer.
 *
 * Go to the md_conf[] table to find out the correct real partition
 * dev and block number for this buffer.   Calculate which real disk
 * to give the request to, and invoke its strategy routine.
 */
mdreal(bp)
	register struct buf	*bp;
{
	struct md_struct	*mdstruct;
	struct md_row		*mdrow;
	dev_t			real_dev,
				parity_device;
	dev_t			dev;
	int			minor_device;
	daddr_t			sector,
				real_sector,
				fragment,
				min_block,
				blk_in_row;
	int			row_index;

	trace4(TR_MD_REAL, bp, bp->b_blkno, bp->b_dev, bp->b_flags);

	dev = bp->b_dev;
	/*
	 * Make sure that the minor device is a valid part of the Virtual
	 * Disk subsystem.
	 */
	if (MD_MINOR(dev) >= NMD) {
		printf("mdreal: dev 0x%x bad minor device 0x%x\n",
			dev, MD_MINOR(dev));
		printf("mdreal: bp 0x%x b_flags 0x%x av_back 0x%x\n",
		    bp, bp->b_flags, bp-> av_back);
		panic ("mdreal: bad real device");
	}
	mdstruct = &md_conf[MD_MINOR(dev)];

	/*
	 * Fake calculation to check what is below.
	 */
	mdrow = &mdstruct->md_real_row[0];

	/*
	 * Do a real calculation to derive the minor device of the
	 * Virtual Disk, which in turn will let us derive the
	 * device/minor of the underlying real device.
	 */
	if (mdstruct->md_parity_interval == 0) {
		for (row_index = 0; row_index < mdstruct->md_rows; row_index++) {
			mdrow = &mdstruct->md_real_row[row_index];
			if (bp->b_blkno < mdrow->md_cum_blocks) break;
		}
		min_block = mdrow->md_cum_blocks - mdrow->md_blocks;
		if ((bp->b_blkno < min_block) || (bp->b_blkno > mdrow->md_cum_blocks))
			printf("mdreal: block 0x%x max 0x%x min 0x%x\n",
			    bp->b_blkno,
			    mdrow->md_cum_blocks,
			    min_block);
		blk_in_row = bp->b_blkno - min_block;
		fragment = blk_in_row & 0xf;
		sector = blk_in_row >> 4;
		if (bp->b_bcount > ((0x10 - fragment) * DEV_BSIZE)) {
			mddumpbuf("mdreal(fragment)", bp);
			printf("mdreal: should panic count 0x%x fragment 0x%x",
				bp->b_bcount, fragment);
		}
		minor_device = sector % mdrow->md_width;
		real_sector = sector / mdrow->md_width;
		bp->b_blkno = (real_sector << 4) + fragment;
		real_dev = mdrow->md_real_disks[minor_device];
		bp->b_dev = real_dev;
	} else {
		for (row_index = 0; row_index < mdstruct->md_rows; row_index++) {
			mdrow = &mdstruct->md_real_row[row_index];
			if (bp->b_blkno < mdrow->md_cum_data_blocks) break;
		}
		min_block = mdrow->md_cum_data_blocks - mdrow->md_data_blocks;
		if ((bp->b_blkno < min_block) || (bp->b_blkno > mdrow->md_cum_data_blocks))
			printf("mdreal: block 0x%x max 0x%x min 0x%x\n",
			    bp->b_blkno,
			    mdrow->md_cum_data_blocks,
			    min_block);
		blk_in_row = bp->b_blkno - min_block;
		fragment = blk_in_row & 0xf;
		sector = blk_in_row >> 4;
		if (bp->b_bcount > ((0x10 - fragment) * DEV_BSIZE)) {
			mddumpbuf("mdreal(fragment)", bp);
			printf("mdreal: should panic count 0x%x fragment 0x%x",
				bp->b_bcount, fragment);
		}
		minor_device = sector % (mdrow->md_width - 1);
		real_sector = sector / (mdrow->md_width - 1);
		parity_device = real_sector % mdrow->md_width;
		mdread_parity (mdrow->md_real_disks[parity_device], real_sector << 4);
		if (minor_device >= parity_device) {
			minor_device = minor_device + 1;
		}
		bp->b_blkno = (real_sector << 4) + fragment;
		real_dev = mdrow->md_real_disks[minor_device];
		bp->b_dev = real_dev;
	}

	trace4(TR_MD_REAL, bp, bp->b_blkno, bp->b_dev, bp->b_flags);

#if MD_STATS > 0
	md_trace[bp->av_back->md_trace_ind].tr_dev[1] = bp->b_dev;
	md_trace[bp->av_back->md_trace_ind].tr_blk[1] = bp->b_blkno;
#endif MD_STATS
	(*bdevsw[major(bp->b_dev)].d_strategy)(bp);
}

/*
 * This routine performs raw read operations.  It is called from the
 * device switch at normal priority.  It uses a per-unit buffer for
 * the operation.
 *
 * The main catch is that the *uio struct which is passed to us may
 * specify a read which spans two buffers, which would be contiguous
 * on a single partition,  but not on a striped partition.
 *
 * Called by:
 *
 */
mdread(dev, uio)
	dev_t		dev;
	struct uio	*uio;
{
	int			unit;
	struct mdunit		*un;
	struct buf		*bp;
	int			length;
	int			error;

	trace2(TR_MDREAD, dev, uio);

	length = uio->uio_iov->iov_len;
	if ((unit = MD_MINOR(dev)) >= NMD) {
		printf("mdread: bad unit 0x%x\n", unit);
		return (ENXIO);
	}
	un = &md_units[unit];
	bp = un->un_rtab;
	error = physio(mdstrategy, bp, dev, B_READ, minphys, uio);
	if (error) {
		printf("mdread: error 0x%x\n", error);
	}
	if (uio->uio_resid != 0) {
		printf("mdread: uio_resid 0x%x length 0x%x b_bcount 0x%x b_resid 0x%x\n",
			uio->uio_resid, length, bp->b_bcount, bp->b_resid);
	}
	return (error);
}

/*
 * This routine performs raw write operations.  It is called from the
 * device switch at normal priority.  It uses a per-unit buffer for
 * the operation.
 *
 * The main catch is that the *uio struct which is passed to us may
 * specify a write which spans two buffers, which would be contiguous
 * on a single partition,  but not on a striped partition.
 * Called by:
 *
 */
mdwrite(dev, uio)
	dev_t		dev;
	struct uio	*uio;
{
	int			unit;
	struct mdunit		*un;
	struct buf		*bp;
	int			length;
	int			error;

	trace2(TR_MDWRITE, dev, uio);

	length = uio->uio_iov->iov_len;
	if ((unit = MD_MINOR(dev)) >= NMD) {
		printf("mdwrite: bad unit 0x%x\n", unit);
		return (ENXIO);
	}
	un = &md_units[unit];
	bp = un->un_rtab;
	error = physio(mdstrategy, bp, dev, B_WRITE, minphys, uio);
	if (error) {
		printf("mdwrite: error 0x%x\n", error);
	}
	if (uio->uio_resid != 0) {
		printf("mdwrite: uio_resid 0x%x length 0x%x b_bcount 0x%x b_resid 0x%x\n",
			uio->uio_resid, length, bp->b_bcount, bp->b_resid);
	}
	return (error);
}

/*
 * This routine finishes a buf-oriented operation.  It is called from
 * the underlying driver level.   Buffer are sent here by biodone().
 *
 * We use the backwards pointer in the buffer header to find the md_save
 * structure.   If this is a single buffer derived from a single buffer,
 * we make the original buffer look as though is has been completed,
 * and pass it back to biodone().
 *
 * If the original buffer was a long read/write and had to be split
 * into several buffers, then we count them off as they arrive, and
 * when all are in, we proceed as above.
 *
 * Called by:
 *	biodone()
 *
 */
mddone(bp)
	struct buf		*bp;
{
	struct md_save		*mdsave;
	int			s;

	/*
	 * We should never see the B_DONE flag, since biodone ought
	 * to call us without setting it.
	 */
	if (bp->b_flags & B_DONE) {
		panic ("mddone: B_DONE\n");
	}
	/*
	 * First, locate the MD save structure that holds the information
	 * for this buf.
	 */
	mdsave = (struct md_save *)bp->av_back;
	if (&(mdsave->md_buf) != bp) {
		/*
		 * Such a buffer can be completely illegal, or it can be one
		 * of several fragments of a long raw read/write.
		 */
		if (bp->b_flags & B_RAW_FRAG) {
			/*
			 * If this buffer is one fragment of a long raw read/write
			 * then get rid of it, and if it is the final fragment
			 * signal completion.
			 */
			if (mdsave->md_frags <= 0) {
				printf("mddone: md_frags 0\n");
				panic ("mddone: md_frags");
			}
			mdsave->md_frags =  mdsave->md_frags - 1;
			mdsave->md_frag_chars = mdsave->md_frag_chars + bp->b_bcount;
			if ((mdsave->md_frag_chars < 0) ||
			    (mdsave->md_frag_chars > mdsave->md_buf.b_bcount)) {
				printf("mddone: md_frag_chars 0x%x b_bcount 0x%x\n",
					mdsave->md_frag_chars, mdsave->md_buf.b_bcount);
				panic ("mddone: md_frag_chars");
			}
			s = spl6();
			md_raw_free (bp);
			if (mdsave->md_frags == 0) {
				if (mdsave->md_frag_chars != mdsave->md_buf.b_bcount) {
					printf("mddone: md_frag_chars 0x%x b_bcount 0x%x\n",
						mdsave->md_frag_chars,
						mdsave->md_buf.b_bcount);
					panic ("mddone: md_frag_chars");
				}
				if (mdsave->md_bp != (struct buf *)0) {
					bp = mdsave->md_bp;
					bp->b_resid =
					    bp->b_bcount - mdsave->md_frag_chars;
					bp->b_flags &= ~B_CALL;
					trace4(TR_MD_DONE, bp, bp->b_blkno,
						bp->b_dev, bp->b_flags);
					iodone(bp);
				}

				mdfree (mdsave);
				(void) splx(s);
				return;
			}
			(void) splx(s);
			return;
		} else {
			mddumpbuf("mddone(2)", bp);
			mdsavedump(mdsave);
			panic ("mddone: bad bp");
		}
	}


	/*
	 * Now, restore the saved information, so that to the rest of the
	 * kernel, it looks as thought nothing has changed.
	 */
#if MD_STATS > 0
	md_trace[mdsave->md_trace_ind].tr_usecs[1] = time.tv_usec;
#endif MD_STATS
	s = spl6();
	if (mdsave->md_bp != (struct buf *)0) {
		bp = mdsave->md_bp;
		MD_RESTORE(bp, mdsave);
		/*
		 * Make sure biodone() does not try to call back twice.
		 */
		bp->b_flags &= ~B_CALL;
		iodone(bp);
	} else {
		mdfree (mdsave);
	}
	trace4(TR_MD_DONE, bp, bp->b_blkno, bp->b_dev, bp->b_flags);
	(void) splx(s);
}

/*
 * This routine implements the ioctl calls for the Virtual Disk System.
 *  It is called from the device switch at normal priority.
 *
 * Called by:
 *
 */
/* ARGSUSED */
mdioctl(dev, cmd, data, flag)
	dev_t		dev;
	int		cmd, flag;
	caddr_t		data;
{
	struct md_struct	*mdstruct;
	struct md_struct	*md_data;
	struct dk_info		*info;
	int			status;

	printf("mdioctl(dev 0x%x, cmd 0x%x, data 0x%x, flag 0x%x)\n",
	    dev, cmd, data, flag);
	trace4(TR_MD_IOCTL, dev, cmd, data, flag);
	/*
	 * Make sure that the minor device is a valid part of the Virtual
	 * Disk subsystem.
	 */
	if (MD_MINOR(dev) >= NMD) {
		printf("mdioctl: dev 0x%x bad minor device 0x%x\n",
			dev, MD_MINOR(dev));
		return (ENXIO);
	}
	mdstruct = &md_conf[MD_MINOR(dev)];

	switch (cmd) {

	/*
	 * Return info concerning the controller.
	 */
	case	DKIOCINFO:
		printf("mdioctl: get info\n");
		info = (struct dk_info *)data;
		info->dki_ctlr = 0;
		info->dki_unit = 0;
		info->dki_ctype = DKC_MD;
		info->dki_flags = 0;
		return (0);

	case	MD_IOCGET:	/* anyone can read the Virtual Disk table */
		printf("mdioctl: get Virtual Disk table\n");
		md_data = (struct md_struct *)data;
		*md_data = *mdstruct;
		return (0);

	case	MD_IOCSET:	/* only superuser can set Virtual Disk table */
		printf("mdioctl: set Virtual Disk table\n");
		if (!suser()) {
			printf("mdioctl: dev 0x%x not superuser\n",
				dev);
			return (EPERM);
		}
		md_data = (struct md_struct *)data;
		if (md_data->md_status == MD_CLEAR) {
			printf("mdioctl: clearing device\n");
			mdstruct->md_status &= ~MD_SETUP;
			mdstruct->md_status |= MD_IOCOPS;
			return (0);
		}
		if ((status = mdvalid (md_data)) != 0) {
			printf("mdioctl: invalid ioctl\n");
			mdstruct->md_status &= ~MD_SETUP;
			return (status);
		}
		*mdstruct = *md_data;
		mdstruct->md_status |= MD_SETUP;
		return (0);

	default:
		printf("mdioctl: dev 0x%x invalid cmd 0x%x\n",
			dev, cmd);
		return (EINVAL);
	}
}

/*
 * This routine dumps memory to the disk.  It assumes that the memory has
 * already been mapped into mainbus space.  It is called at disk interrupt
 * priority when the system is in trouble.
 *
 * Called by:
 *
 */
mddump()
{
}

/*
 * This routine translates a buf oriented command down to a level
 * where it can actually be passed to the underlying driver.
 *
 * Called by:
 *
 */
mdgo(md)
	register struct mb_device	*md;
{
	printf("mdgo: md 0x%x\n", md);
}

/*
 * This routine polls all the underlying drivers to see if one is
 * interrupting.    It is called whenever a non-vectored interrupt
 * of the correct priority is received.
 *
 * Called by:
 *
 */
mdpoll()
{
	printf("mdpoll()\n");
	return (0);
}

#endif NMD
