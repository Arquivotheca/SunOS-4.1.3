#ifndef lint
static	char sccsid[] = "@(#)rd.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * RAM pseudo-device to let data live in real memory. In its present form
 * it is meant to hold a root file system. When first accessed, it will
 * prompt at the console for the name of a local block device. It will
 * then copy a 4.2 file system from that device into a dynamically
 * allocated chunk of memory sized to fit that file system.
 */

#include "rd.h"

#if NRD > 0

#include <sys/param.h>		/* Includes <sys/types.h> */
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/kmem_alloc.h>
#include <ufs/fs.h>
#include <sundev/mbvar.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/ioccom.h>
#include <sys/mtio.h>
#include <sys/vnode.h>
#include <vm/page.h>
#ifdef sun4c	/* and anyone else doing programmed IO of boot devices? */
#include <machine/clock.h>
#endif
#include <sun/dklabel.h>
#ifdef sun3	/* XXX sun3e HACK */
#include <machine/cpu.h>
#endif sun3

#ifdef RAMDISK_PRELOADED
#include <mon/sunromvec.h>
#ifndef MUNIXFS_SIZE
/*
 * HARDCODE (yecch!) numbers from sundist/Makefile
 * XXX ought to have a included config file in both Makefiles
 */
#if defined(sun4) || defined(sun4c) || defined(sun4m)
#define	MUNIXFS_SIZE 4096
#elif defined(sun3) || defined(sun3x)
#define	MUNIXFS_SIZE 3072
#endif
#endif /* not MUNIXFS_SIZE */
int	ramdiskispreloaded = 0;
/*
 * space for filesys, aligned at word, as well as force into data segment.
 * also init'd with its size, so stamprd.c can check it.
 */
int	ramdisk0[(DEV_BSIZE * MUNIXFS_SIZE) / sizeof (int)] =
	    { ((int)(DEV_BSIZE * MUNIXFS_SIZE)), 0, 2 };
#endif RAMDISK_PRELOADED

extern char *strncpy();

/* UNITIALIZED matches value of unitialized data structure */
typedef enum { UNINITIALIZED = 0, INITIALIZED, FAILED } rdstate_t;

typedef struct	{
	caddr_t	data;
	u_int	size;
	rdstate_t	state;
} ramdisk_t;

ramdisk_t	rd[NRD];

off_t rd_offset = 0;		/* offset in disk blocks */
extern int loadramdiskfile;

#ifndef OPENPROMS
/* This structure exists because swapgeneric.c:getchardev() wants to see it. */
struct	mb_driver rddriver = {
	0,		/* probe:	see if a driver is really there */
	0,		/* slave:	see if a slave is there */
	0,		/* attach:	setup driver for a slave */
	0,		/* go:		routine to start transfer */
	0,		/* done:	routine to finish transfer */
	0,		/* poll:	polling interrupt routine */
	0,		/* size:	mount of memory space needed */
	"rd",		/* dname:	name of a device */
	0,		/* dinfo:	backpointers to mbdinit structs */
	"rd",		/* cname:	name of a controller */
	0,		/* cinfo:	backpointers to mbcinit structs */
	MDR_PSEUDO,	/* flags:	Mainbus usage flags */
	0,		/* link:	interrupt routine linked list */
};
#else	!OPENPROMS
/*
 * This is a pseudo-device that needs to be initialized properly,
 * and only at the proper time. Unfortunately, at the time of this
 * writing, we don't yet have a clever way to do this worked out,
 * so the hacks that are present in ../sun/swapgeneric.c will
 * have to suffice.
 *
 */
#endif	!OPENPROMS

#define	TAPEBLOCK(x)	((daddr_t)(((x)*(BLKDEV_IOSIZE/DEV_BSIZE))))
static int fnumber = -1;

ramdisk_t *
ramdisk(dev)
	dev_t	dev;
{
	register ramdisk_t *rdp;
	dev_t	cdev, bdev;	/* get ram disk data from here */
	char	devname[128];	/* name gets copied back to this char array */
	extern u_int max_page_get, total_pages;
	u_int	save_max_page_get;
	struct	cdevsw	*dp;
	int	e;
#ifdef sun4c	/* and anyone else doing programmed IO of boot devices? */
	int	savedmonclock;
#endif

	rdp = &rd[minor(dev)];

	if (rdp < &rd[NRD] && rdp->state != UNINITIALIZED)
		return (rdp->state == INITIALIZED ? rdp : (ramdisk_t *)0);
	else if (rdp >= &rd[NRD])
		return ((ramdisk_t *)0);

#ifdef RAMDISK_PRELOADED
	if ((minor(dev) == 0) && ramdiskispreloaded) {
		if (rdp->state == INITIALIZED)
			return (rdp);

		rdp->data = (caddr_t)ramdisk0;
		rdp->size = DEV_BSIZE * MUNIXFS_SIZE;
		rdp->state = INITIALIZED;
		printf("rd0: using preloaded munixfs\n");
#ifdef OPENPROMS
		/* XXX OPENBOOT warning! */
		/* XXX: devname is limitted to 2 char name */
		(void) prom_get_boot_dev_name(rdp->data, 3);
		rdp->data[2] = prom_get_boot_dev_unit() + '0';
		rdp->data[3] = prom_get_boot_dev_part() + 'a';
		rdp->data[4] = (char)0;
#else	OPENPROMS
		rdp->data[0] = (*romp->v_bootparam)->bp_dev[0];
		rdp->data[1] = (*romp->v_bootparam)->bp_dev[1];
		rdp->data[2] = '0' + (*romp->v_bootparam)->bp_unit;
/* XXX NOTE: do this - OR - just say NO to PRE-loaded non OPENPROM machines */
/* XXX #ifndef OPENPROM (kinda - Campus PROMS too) *XXX*/
/* XXX rdp->data[2] = INSCRUTABLE_TRANSLATION(rdp->data[2]); *XXX*/
/* XXX #endif *XXX*/
		rdp->data[3] = 'a' + (*romp->v_bootparam)->bp_part;
		rdp->data[4] = '\0';
#endif	OPENPROMS
		return (rdp);
	}
#endif RAMDISK_PRELOADED

retry:
	/* initialize the ramdisk from something */

	(void)bzero(devname, sizeof (devname));
	getchardev("Initialize ram disk from", devname, &fnumber, &cdev, &bdev);

#ifdef sun4c	/* and anyone else doing programmed IO of boot devices? */
	/*
	 * as to why this HACK is here:  the problem is that the proms
	 * use level 14 interrupts to poll the keyboard every 10 ms, and lock
	 * out the floppy - which always gets overruns and we can't load the
	 * ramdisk.  Choices: (a) fix proms to use keyboard interrupts instead
	 * [-too much code to do so quickly], (b) put DMA on floppy chip
	 * [-WAY too late for that], (c) add a bunch of code to fd.c to do
	 * reads by wait with spl15 and knowing that its at MUNIX and loading
	 * ramdisk [-will hang system, too much code to do so quickly], or
	 * (d) bump the monitor's clock down in either rd.c or fd.c.
	 * since the problem could be generic (for similar PIO devices) -AND-
	 * rd.c/swapgeneric.c are the dumping place for HACKS, it goes here.
	 */
	/* if "fd" or other similar semi-fast PIO device... */
	if (devname[0] == 'f' && devname[1] == 'd') {
		/* slow down the monitor clock to 2 seconds */
		savedmonclock = COUNTER->limit14;
		COUNTER->limit14 = (2 * 1000000) << CTR_USEC_SHIFT;
	} else
#endif
	/*
	 * if cdrom, it can't understand partitions, so
	 * we teach the ramdisk drive how to read the disk label.
	 * we also have to strip out the partition number.
	 * we don't optimize for sd, as we want to test this!
	 */
	if (devname[0] == 's' && ((devname[1] == 'd') || (devname[1] == 'r'))) {
		struct vnode *vp;
		dev_t	part;
		struct dk_label dkl;	/* is 512 bytes (better be!) */

		part = minor(bdev) & 0x7;	/* 8 partitions! */
		bdev = bdev & ~(0x7);		/* force to partition "a" */
		if (bdevsw[major(bdev)].d_open(bdev, FREAD, u.u_cred) != 0) {
			printf("rd: can't open sr bdev of: %s\n", devname);
			goto outnull;
		}
		vp = bdevvp(bdev);
		if (rd_readidev(vp, (daddr_t)0, (caddr_t)&dkl, 512)) {
			printf("rd: can't read sr label on: %s\n", devname);
			goto outnull;
		}
		/* now set rd_offset (in DEV_BSIZE units) */
		rd_offset = btodb(loadramdiskfile) +
		    (dkl.dkl_map[part].dkl_cylno *
		    (dkl.dkl_nsect * dkl.dkl_nhead));
		printf("rd: rd_offset of \"%s\" is %d\n", devname, rd_offset);
		goto opened;
	}

	/* fnumber != -1 return from getchardev() means TAPE (& tape ONLY!) */
	if ((fnumber > 0) && (major(cdev) != major(-1))) {
		struct mtop	op;

#ifdef sun3	/* XXX sun3e HACK */
		if ((cpu == CPU_SUN3_E) && (devname[0] == 's') &&
		    (devname[1] =='t')) {
			cdev |= MT_DENSITY2;	/* force to QIC-24 */
			bdev |= MT_DENSITY2;
		}
#endif sun3
		dp = &cdevsw[major(cdev)];
		if ((*dp->d_open)(cdev, FREAD, u.u_cred) != 0)
			goto outnull;
		op.mt_op = MTFSF;
		op.mt_count = fnumber;
		if ((*dp->d_ioctl)(cdev, MTIOCTOP, &op, 0) != 0)
			goto outnull;
	} else {
		if (bdevsw[major(bdev)].d_open(bdev, FREAD, u.u_cred) != 0)
			goto outnull;
	}
opened:

	/* read the 4.2 file system in a semi-portable fashion */

	/* ignore the usual allocation limits */
	save_max_page_get = max_page_get; /* save arbitrary limits */
	max_page_get = total_pages;
	if ((e = rd_readdata(bdev, rdp)) != 0) {
		struct vnode *vp;

		(void)printf(
	"ramdisk: error %d, try another tape file # or floppy disk\n", e);

		(*dp->d_close)(bdev);
		vp = bdevvp(bdev);
		binval(vp);
		goto retry;
	} else
		rdp->state = INITIALIZED;

	max_page_get = save_max_page_get;	/* restore arbitrary limits */

	if ((fnumber > 0) && (major(cdev) != major(-1)))
		(*dp->d_close)(cdev);
	else {
		if (bdevsw[major(bdev)].d_close(bdev, FREAD, u.u_cred) != 0)
			rdp->state = FAILED;
	}

	/*
	 * whew: we got here ok, now lets stuff the 1st block of the ramdisk
	 * (if unit 0) with where loading from (as Unix devname).
	 * useful for munixfs extract of miniroot
	 */
	if (rdp == &rd[0])
		(void)strncpy(rdp->data, devname, sizeof (devname));

outnull:	/* if rdp->state != INITIALIZED, we'll return null */
#ifdef sun4c	/* and anyone else doing programmed IO of boot devices? */
	if (devname[0] == 'f' && devname[1] == 'd') {
		/* restore the monitor clock */
		COUNTER->limit14 = savedmonclock;
	}
#endif

	return (rdp->state == INITIALIZED ? rdp : (ramdisk_t *)0);
}

/*
 * rd_readidev - read from the initializing device
 */
rd_readidev(vp, bno, base, len)
	struct vnode	*vp;
	daddr_t	bno;
	caddr_t	base;
	int	len;
{
	struct buf *tp;
	int	e;

	bno += rd_offset;

	tp = bread(vp, bno, len);
	if ((e = geterror(tp)) == 0)
		bcopy(tp->b_un.b_addr, base, (u_int)len);
	brelse(tp);

	return (e);
}

rd_readdata(dev, rdp)
	dev_t	dev;
	register ramdisk_t *rdp;
{
	register caddr_t data;
	register int maxblock, chunk, i;
	int bsize, nblocks;
	int	e;
	struct vnode *vp;

	vp = bdevvp(dev);

	rdp->size = (u_int)(BBSIZE + SBSIZE);
	if ((data = new_kmem_alloc(rdp->size, KMEM_NOSLEEP)) == 0)
		return (1);

	/*
	 * Egregious hack for tapes here. xt driver attempts
	 * to position you. This doesn't work for a variety of reasons.
	 * The st driver ignores you. The ar tape device pisses on you.
	 * So, we will hack these reads to always be the right blkno
	 * for tape devices. We will also assume 8k records. Sorry. (mjacob).
	 */
	if (fnumber < 0) {
		if ((e = rd_readidev(vp, BBLOCK, data, BBSIZE)) != 0)
			return (e);
		if ((e = rd_readidev(vp, SBLOCK, data+BBSIZE, SBSIZE)) != 0)
			return (e);
	} else {
		if ((e = rd_readidev(vp, TAPEBLOCK(0), data, BBSIZE)) != 0)
			return (e);
		if ((e = rd_readidev(vp, TAPEBLOCK(1), data+BBSIZE, SBSIZE))
				!= 0)
			return (e);
	}

	if (((struct fs *)(data+dbtob(SBLOCK)))->fs_magic != FS_MAGIC) {
		(void)printf("rd: bad magic number\n");
		return (1);
	}

	bsize = ((struct fs *)(data+dbtob(SBLOCK)))->fs_bsize;

	/* calculate number of fs blocks */
	nblocks = ((struct fs *)(data+dbtob(SBLOCK)))->fs_size
		* ((struct fs *)(data+dbtob(SBLOCK)))->fs_fsize
		/ bsize;

	(void)printf("rd: reading %d, %d byte blocks: ", nblocks, bsize);

	/*
	 * allocate space for the rest of the ram disk and transfer
	 * the data read so far into it.
	 */
	rdp->size = (u_int)(nblocks * bsize);
#ifdef RAMDISK_PRELOADED
	/* just use existing space if big enuff */
	if ((rdp == &rd[0]) && (ramdisk0[0] >= rdp->size)) {
		rdp->data = (caddr_t)ramdisk0;
		printf("rd0: using preloaded munixfs space\n");
	} else
#endif RAMDISK_PRELOADED
	if ((rdp->data = new_kmem_alloc(rdp->size, KMEM_NOSLEEP)) == 0) {
		kmem_free(data, BBSIZE + SBSIZE);
		return (1);
	}

	bcopy(data, rdp->data, BBSIZE + SBSIZE);
	kmem_free(data, BBSIZE + SBSIZE);
	data = rdp->data;

	/* read in the rest of the file system a fs block at a time */
	maxblock = NSPB((struct fs *)(data+dbtob(SBLOCK))) * nblocks;
	chunk = btodb(bsize);
	if (fnumber < 0) {
		for (i = SBLOCK + btodb(SBSIZE); i < maxblock; i += chunk) {
			if ((e = rd_readidev(vp, (daddr_t)i, data+dbtob(i),
					bsize)) != 0)
				return (e);
			(void)printf(".");
		}
	} else {
		int j = 2;
		bsize = BBSIZE;	/* force it to first read's size */
		chunk = btodb(bsize);
		for (i = SBLOCK + btodb(SBSIZE); i < maxblock; i += chunk, j++){
			if ((e = rd_readidev(vp, TAPEBLOCK(j), data+dbtob(i),
					bsize)) != 0)
				return (e);
			(void)printf(".");
		}
	}
	(void)printf("done\n");
	return (0);	/* OK */
}

/*ARGSUSED*/
rdopen(dev, wrtflag)
	dev_t	dev;
	int	wrtflag;
{
	if (!ramdisk(dev))
		return (ENXIO);
	return (minor(dev) >= NRD ? ENXIO : 0);
}

rdsize(dev)
	dev_t	dev;
{
	register ramdisk_t	*rdp;

	if ((rdp = ramdisk(dev)) == (ramdisk_t *)0)
		return (-1);
	else
		return (btodb(rdp->size));
}

#include <sys/uio.h>

rdread(dev, uio)
	dev_t	dev;
	register struct uio *uio;
{
	register ramdisk_t *rdp;

	if ((rdp = ramdisk(dev)) == (ramdisk_t *)0 ||
	    (unsigned) uio->uio_offset > rdp->size)
		return (EINVAL);

	return (uiomove(rdp->data + uio->uio_offset,
		(int)(MIN(uio->uio_resid, rdp->size - uio->uio_offset)),
		UIO_READ, uio));
}

rdwrite(dev, uio)
	dev_t dev;
	register struct uio *uio;
{
	register ramdisk_t *rdp;

	if ((rdp = ramdisk(dev)) == (ramdisk_t *)0 ||
	    ((unsigned) uio->uio_offset > rdp->size))
		return (EINVAL);

	return (uiomove(rdp->data + uio->uio_offset,
		(int)(MIN(uio->uio_resid, rdp->size - uio->uio_offset)),
		UIO_WRITE, uio));
}

rdstrategy(bp)
	register struct buf *bp;
{
	register ramdisk_t *rdp;
	register long	offset = dbtob(bp->b_blkno);

	if ((rdp = ramdisk(bp->b_dev)) == (ramdisk_t *)0 ||
	    (u_long) offset > rdp->size) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
	} else {
		caddr_t	raddr;
		unsigned	nbytes;

		raddr = rdp->data + offset;
		nbytes = MIN(bp->b_bcount, rdp->size - offset);

		if (bp->b_flags & B_PAGEIO)
			bp_mapin(bp);

		if (bp->b_flags & B_READ)
			bcopy(raddr, bp->b_un.b_addr, nbytes);
		else
			bcopy(bp->b_un.b_addr, raddr, nbytes);
		bp->b_resid = bp->b_bcount - nbytes;
	}
	iodone(bp);
}
#endif
