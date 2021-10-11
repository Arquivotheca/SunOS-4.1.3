#include "sf.h"
#if	NSF > 0

/************************************************************************
 ************************************************************************
 *									*
 *									*
 *		SCSI floppy disk driver					*
 *									*
 *									*
 ************************************************************************
 ************************************************************************/
/*
 * Copyright (C) 1989 Sun Microsystems, Inc.
 */
#ident	"@(#)sf.c 1.1 92/07/30 SMI"

/************************************************************************
 ************************************************************************
 *									*
 *									*
 *		Includes, Declarations and Local Data			*
 *									*
 *									*
 ************************************************************************
 ************************************************************************/

#include <scsi/scsi.h>
#include <scsi/targets/sfdef.h>
#include <sun/vddrv.h>
#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>

/************************************************************************
 *									*
 * Local definitions, for clarity of code				*
 *									*
 ************************************************************************/

#ifdef	OPENPROMS
#define	DNAME		devp->sd_dev->devi_name
#define	DUNIT		devp->sd_dev->devi_unit
#define	CNAME		devp->sd_dev->devi_parent->devi_name
#define	CUNIT		devp->sd_dev->devi_parent->devi_unit
#else	OPENPROMS
#define	DNAME		devp->sd_dev->md_driver->mdr_dname
#define	DUNIT		devp->sd_dev->md_unit
#define	CNAME		devp->sd_dev->md_driver->mdr_cname
#define	CUNIT		devp->sd_dev->md_ctlr->mc_ctlr
#endif	OPENPROMS

#define	UPTR		((struct scsi_floppy *)(devp)->sd_private)
#define	SCBP(pkt)	((struct scsi_status *)(pkt)->pkt_scbp)
#define	SCBP_C(pkt)	((*(pkt)->pkt_scbp) & STATUS_MASK)
#define	CDBP(pkt)	((union scsi_cdb *)(pkt)->pkt_cdbp)
#define	ROUTE		(&devp->sd_address)
#define	BP_PKT(bp)	((struct scsi_pkt *)bp->av_back)

#define	Tgt(devp)	(devp->sd_address.a_target)
#define	Lun(devp)	(devp->sd_address.a_lun)
#define	SFUNIT(dev)	(minor((dev))>>3)
#define	SFPART(dev)	(minor((dev))&0x7)

/*
 * Parameters
 */

	/*
	 * 120 seconds is a *very* reasonable amount of time for most floppy
	 * operations.
	 */
#define	DEFAULT_SF_TIMEOUT	120

	/*
	 * 5 seconds is what we'll wait if we get a Busy Status back
	 */

#define	SFTIMEOUT		5*hz	/* 2 seconds Busy Waiting */

	/*
	 * Number of times we'll retry a normal operation.
	 *
	 * XXX This includes retries due to transport failure
	 * XXX Need to distinguish between Target and Transport
	 * XXX failure.
	 */

#define	SF_RETRY_COUNT		30

	/*
	 * maximum number of supported units
	 */

#define	SF_MAXUNIT		2

/*
 * Debugging macros
 */


#define	DEBUGGING	((scsi_options & SCSI_DEBUG_TGT) || sfdebug > 1)
#define	DEBUGGING_ALL	((scsi_options & SCSI_DEBUG_TGT) || sfdebug)
#define	DPRINTF		if(DEBUGGING) printf
#define	DPRINTF_ALL	if (DEBUGGING || sfdebug > 0) printf
#define	DPRINTF_IOCTL	DPRINTF_ALL

/************************************************************************
 *									*
 * Gloabal Data Definitions						*
 *									*
 ************************************************************************/

struct scsi_device *sfunits[SF_MAXUNIT];
extern int nsf = SF_MAXUNIT;

/************************************************************************
 *									*
 * Local Static Data							*
 *									*
 ************************************************************************/

static int sfpri = 0;
static int sfdebug = 0;
static int sf_error_reporting = SDERR_RETRYABLE;

static struct sf_drivetype sf_drivetypes[] = {
/* Emulex MD21 */
{	"SMS",		0,		CTYPE_SMS,	3,	"SMS" },
/* Connor */
{	"NCR",		0,		CTYPE_NCR,	3,	"NCR" },
};

#define	MAX_SFTYPES (sizeof sf_drivetypes/ (sizeof sf_drivetypes[0]))

static u_char media_types[] = {
	/*
	 * first one must be ff (replaced with whatever the drive starts with)
	 */
	0xff, 0x1e, 
};

/*
 * Configuration Data
 */

#ifdef	OPENPROMS
/*
 * Device driver ops vector
 */
int sfslave(), sfopen(), sfclose(), sfread(), sfwrite();
int sfstrategy(), sfioctl(), sfsize();
extern int nulldev(), nodev();
struct dev_ops sf_ops = {
	1,
	sfslave,
	nulldev,
	sfopen,
	sfclose,
	sfread,
	sfwrite,
	sfstrategy,
	nodev,
	sfsize,
	sfioctl
};

	
#else	OPENPROMS
XXX * BARF * XXX
#endif

/************************************************************************
 *									*
 * Local Function Declarations						*
 *									*
 ************************************************************************/

static void sfintr(), sferrmsg(), sfdone(), make_sf_cmd(), sfstart(), sfdone();
static int sfrunout(), sf_findslave(), sf_unit_ready();

/**/
#if	defined(VDDRV) && defined(LOADABLE)
/***********************************************************************
 ************************************************************************
 *									*
 *									*
 *		Loadable Module support					*
 *									*
 *									*
 ************************************************************************
 ************************************************************************/
#ifndef	OPENPROMS
XXXXXXXXXXXXXXXXXXXXXXXXXX BLETCH XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
#endif

/*
 * Loadable module support:
 *	for a loadable driver
 *	modload modstat & modunload
 */
int nsf = NSF;
struct scsi_device *sfunits[NSF];

/*
 * awk.
 */

static int targetid = 6;
static int sf_can_load(), sf_can_unload();

extern int seltrue();
static struct bdevsw sf_bdevsw = {
	sfopen, sfclose, sfstrategy, nodev, sfsize, 0
};

static struct cdevsw sf_cdevsw = {
	sfopen, sfclose, sfread, sfwrite, sfioctl, nulldev, seltrue, 0, 0
};

static struct vdldrv sfdrv = {
	VDMAGIC_DRV,
	"SCSI Floppy Driver",
	&sf_ops,
	&sf_bdevsw,
	&sf_cdevsw,
	9,
	33
};

sfinit(fcn, vdp, vdi, vds)
int			fcn;
register struct vddrv	*vdp;
caddr_t			vdi;
struct vdstat		*vds;
{
	extern struct scsi_device *sd_root;
	struct scsi_device *devp;
	int status = 0;

	switch (fcn) {

	case VDLOAD:
		status = sf_can_load();
		if (status == 0) {
			vdp->vdd_vdtab = (struct vdlinkage *)&sfdrv;
		}
		break;

	case VDUNLOAD:
		status = sf_can_unload();
		break;

	case VDSTAT:
		break;
 
	default:
		printf("sfinit: unknown function 0x%x\n", fcn);
		status = EINVAL;
	}
 
	return status;
}

static int
sf_can_load()
{
	register scsi_device *devp, *newdevp;
	register i, s;

	if (sfpri == 0)
		sfpri = pritospl(SPLMB);

	s = splr(sfpri);
	for  (i = 0; i < nsf; i++) {
		if (sfunits[i]) {
			(void) splx(s);
			return (EBUSY);
		}
	}
	if (devp = sd_root) {
		for (;;) {
			if (devp->sd_address.a_target == targetid) {
				(void) splx(s);
				return (EBUSY);
			}
			if (devp->sd_next) {
				devp = devp->sd_next;
			} else
				break;
		}
	}
	(void) splx(s);

	newdevp =
		(struct scsi_device *) kmem_zalloc((unsigned) sizeof (*devp));
	if (!newdevp)
		return (ENOMEM);
	newdevp->sd_address = devp->sd_address;
	newdevp->sd_address.a_target = targetid;
	newdevp->sd_address.a_lun = 0;
	devp->sd_next = newdevp;
}
#endif	defined(VDDRV) && defined(LOADABLE)

/************************************************************************
 ************************************************************************
 *									*
 *									*
 *		Autoconfiguration Routines				*
 *									*
 *									*
 ************************************************************************
 ************************************************************************/

int
sfslave(devp)
struct scsi_device *devp;
{
	/*
	 * fill in our local array
	 */

	if (DUNIT >= nsf)
		return (0);
	sfunits[DUNIT] = devp;

#ifdef	OPENPROMS
	sfpri = MAX(sfpri,ipltospl(devp->sd_dev->devi_intr->int_pri));
#else
	XXX BARF XXX sfpri = MAX(sfpri,pritospl(devp->sd_dev->md_inr));
#endif

	/*
	 * Turn around and call real slave routine..
	 */
	return (sf_findslave(devp,0));
}

static int
sf_findslave(devp,canwait)
register struct scsi_device *devp;
int canwait;
{
	static char *badtyp1 =
		"%s%d:\tdevice is a '%s' device\n\texpected a '%s' device\n";
	static char *badtyp2 = "\tat target %d lun %d on %s%d\n";
	static char *nonccs =
"%s%d: non-CCS device found at target %d lun %d on %s%d- not supported\n";
	struct scsi_pkt *pkt, *rqpkt;
	struct scsi_floppy *un;
	struct scsi_capacity *cap;
	struct sf_drivetype *dp;
	long capacity, lbasize;
	auto int (*f)() = (canwait == 0)? NULL_FUNC: SLEEP_FUNC;

	/*
	 * Call the routine scsi_slave to do some of the dirty work.
	 * All scsi_slave does is do a TEST UNIT READY (and possibly
	 * a non-extended REQUEST SENSE or two), and an INQUIRY command.
	 * If the INQUIRY command succeeds, the field sd_inq in the
	 * device structure will be filled in.
	 *
	 * XXX NEED TO CHANGE scsi_slave TO JUST DO REQUEST_SENSE
	 * XXX AND INQUIRY- THAT WAY WE CAN DO THE TEST_UNIT_READY
	 * XXX HERE AND POSSIBLY SEND A START UNIT COMMAND IF IT
	 * XXX ISN'T READY..
	 */


	switch (scsi_slave(devp,canwait)) {
	default:
	case SCSIPROBE_NOMEM:
	case SCSIPROBE_FAILURE:
	case SCSIPROBE_NORESP:
		return (0);
	case SCSIPROBE_NONCCS:
		printf(nonccs,DNAME,DUNIT,Tgt(devp),Lun(devp),CNAME,CUNIT);
		return (0);
	case SCSIPROBE_EXISTS:
		if(devp->sd_inq->inq_dtype != DTYPE_DIRECT) {
			printf(badtyp1,DNAME,DUNIT,
				scsi_dname((int)devp->sd_inq->inq_dtype),
				scsi_dname(DTYPE_DIRECT));
			printf(badtyp2,Tgt(devp),Lun(devp),CNAME,CUNIT);
			return (0);
		}
		break;
	}


	/*
	 * Sigh. Look through our list of supported drives to dredge
	 * up information that we haven't figured out a clever way
	 * to get inference yet.
	 */

	/*
	 * The actual unit is present.
	 * Now is the time to fill in the rest of our info..
	 */

	un = UPTR = (struct scsi_floppy *)
		kmem_zalloc(sizeof(struct scsi_floppy));
	if (!un) {
		printf("%s%d: no memory for disk structure\n",DNAME,DUNIT);
		return (0);
	}

	for (dp = &sf_drivetypes[0]; dp < &sf_drivetypes[MAX_SFTYPES]; dp++) {
		if (bcmp(devp->sd_inq->inq_vid,dp->vid,dp->vidlen) == 0) {
			un->un_dp = dp;
			break;
		}
	}

	if (dp == &sf_drivetypes[MAX_SFTYPES]) {
		printf("%s%d: unknown drive not supported\n",DNAME,DUNIT);
		(void) kmem_free((caddr_t) un,
			(unsigned) sizeof (struct scsi_floppy));
		return 0;
	}

	/*
	 * now allocate a request sense packet
	 */
	rqpkt = scsi_pktiopb(ROUTE,(caddr_t *)&devp->sd_sense,CDB_GROUP0,
		1,SENSE_LENGTH,B_READ,f);
	if (!rqpkt) {
		(void) kmem_free((caddr_t) un,
			(unsigned) sizeof (struct scsi_floppy));
		return (0);
	}
	makecom_g0(rqpkt,devp,0,SCMD_REQUEST_SENSE,0,SENSE_LENGTH);
	rqpkt->pkt_pmon = -1;

	un->un_mode = (caddr_t) rmalloc(iopbmap, (long) MSIZE);
	un->un_rbufp = (struct buf *) kmem_zalloc(sizeof (struct buf));
	un->un_sbufp = (struct buf *) kmem_zalloc(sizeof (struct buf));
	if (!un->un_rbufp || !un->un_sbufp || !un->un_mode) {
		printf("sfslave: no memory for data structures\n");
		if (un->un_sbufp)
			(void) kmem_free((caddr_t)un->un_rbufp,sizeof (struct buf));
		if (un->un_rbufp)
			(void) kmem_free((caddr_t)un->un_rbufp,sizeof (struct buf));
		if (un->un_mode)
			(void) rmfree(iopbmap,
				(long) MSIZE, (u_long) un->un_mode);
		(void) kmem_free((caddr_t) un,sizeof (struct scsi_floppy));
		(void) rmfree(iopbmap,
			(long) SENSE_LENGTH, (u_long) devp->sd_sense);
		scsi_resfree(rqpkt);
		return 0;
	}
	devp->sd_present = 1;
	un->un_capacity = -1;		/* XXX */
	un->un_lbasize = SECSIZE;	/* XXX */
	rqpkt->pkt_comp = sfintr;
	rqpkt->pkt_time = DEFAULT_SF_TIMEOUT;
	un->un_rqs = rqpkt;
	un->un_sd = devp;
	un->un_dev = DUNIT<<3;
#ifdef	OPENPROMS
	devp->sd_dev->devi_driver = &sf_ops;
#endif
	un->un_g.dkg_ncyl = 78;
	un->un_g.dkg_acyl = 0;
	un->un_g.dkg_pcyl = 80;
	un->un_g.dkg_nhead = 2;
	un->un_g.dkg_nsect = 9;

	return (1);
}
/**/
/************************************************************************
 ************************************************************************
 *									*
 *									*
 *			Unix Entry Points				*
 *									*
 *									*
 ************************************************************************
 ************************************************************************/

int
sfopen(dev, flag)
dev_t dev;
int flag;
{
	register struct scsi_device *devp;
	register struct scsi_floppy *un;
	register int unit, s;


	if((unit = SFUNIT(dev)) >= nsf) {
		return ENXIO;
	} else if (!(devp = sfunits[unit])) {
		return ENXIO;
	} else if (!devp->sd_present) {
		s = splr(sfpri);
		if (sf_findslave(devp,1)) {
			printf("%s%d at %s%d target %d lun %d\n",DNAME,DUNIT,
				CNAME,CUNIT,Tgt(devp),Lun(devp));
		} else {
			(void) splx(s);
			return ENXIO;
		}
		(void) splx(s);
	}

	un = UPTR;
	if (un->un_state >= SF_STATE_OPEN)
		return (0);

	un->un_last_state = un->un_state;
	un->un_state = SF_STATE_OPENING;

	/*
	 * sf_unit_ready just determines whether a diskette is inserted
	 *
	 */

	if (sf_unit_ready(dev) == 0) {
		un->un_state = un->un_last_state;
		un->un_last_state = SF_STATE_OPENING;
		return (ENXIO);
	}

	/*
	 * sf_get_density will run through all known density modes
	 * to try and read the device. If that fails, it must be
	 * either an unformatted disk, or must be a density we cannot
	 * hack. In either case, we will allow only a format command
	 * of this device at this point to open the device.
	 */

	if (sf_get_density(dev) == 0) {
		/*
		 * Are we opening this for format(8) or fdformat?
		 */
		if ((flag & (FNDELAY|FNBIO)) == 0 ||
				cdevsw[major(dev)].d_open != sfopen) {
			un->un_last_state = un->un_state;
			un->un_state = SF_STATE_CLOSED;
			return ENXIO;
		}

		/*
		 * XXX:	put in a block here- opening for formatting is
		 * XXX:	exclusive use.
		 */

		if (sf_set_density(dev) == 0) {
			un->un_last_state = un->un_state;
			un->un_state = SF_STATE_CLOSED;
			return EIO;
		}
	}
	un->un_last_state = un->un_state;
	un->un_state = SF_STATE_OPEN;
	un->un_open |= (1<<SFPART(dev));
	return (0);
}

int
sfclose(dev)
dev_t dev;
{
	register struct scsi_device *devp;
	register struct scsi_floppy *un;
	int unit;

#ifdef	NOT
	printf("sf: last close on dev %x\n",dev);
#endif

	if((unit = SFUNIT(dev)) >= nsf) {
		return ENXIO;
	} else if (!(devp = sfunits[unit]) || !devp->sd_present) {
		return ENXIO;
	} else if (!(un = UPTR) || un->un_state < SF_STATE_OPEN) {
		return ENXIO;
	}

	un->un_open &= ~(1<<SFPART(dev));
	if (un->un_open == 0) {
		un->un_state = SF_STATE_CLOSED;
	}
	return (0);
}

int
sfsize(dev)
dev_t dev;
{
	int unit = SFUNIT(dev);
	struct scsi_device *devp;

	if (unit >= nsf || !(devp = sfunits[unit]) || !devp->sd_present)
		return (-1);
	else
		return ((2*(1<<20))>>9);/*XXX*/
}


/*
 * These routines perform raw i/o operations.
 */

sfread(dev, uio)
dev_t dev;
struct uio *uio;
{
	return sfrw(dev,uio,B_READ);
}

sfwrite(dev, uio)
dev_t dev;
struct uio *uio;
{
	return sfrw(dev,uio,B_WRITE);
}

static int
sfrw(dev, uio, flag)
dev_t dev;
struct uio *uio;
int flag;
{
	struct scsi_device *devp;
	register int unit;


	DPRINTF("sfrw:\n");

	if ((unit = SFUNIT(dev)) >= nsf) {
		return ENXIO;
	} else if (!(devp = sfunits[unit]) || !devp->sd_present ||
	    !UPTR->un_gvalid) {
		return ENXIO;
	} else if ((uio->uio_fmode & FSETBLK) == 0 &&
	    uio->uio_offset % DEV_BSIZE != 0) {
		DPRINTF("sfrw:  file offset not modulo %d\n",DEV_BSIZE);
		return (EINVAL);
	} else if (uio->uio_iov->iov_len % DEV_BSIZE != 0) {
		DPRINTF("sfrw:  block length not modulo %d\n",DEV_BSIZE);
		return (EINVAL);
	}
	return physio(sfstrategy, UPTR->un_rbufp, dev, flag, minphys, uio);
}



/*
 *
 * strategy routine
 *
 */

int
sfstrategy(bp)
register struct buf *bp;
{
	register struct scsi_device *devp;
	register struct scsi_floppy *un;
	register struct diskhd *dp;
	register s;

	DPRINTF("sfstrategy\n");

	devp = sfunits[SFUNIT(bp->b_dev)];
	if (!devp || !devp->sd_present || !(un = UPTR)) {
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		iodone(bp);
		return;
	}

	bp->b_flags &= ~(B_DONE|B_ERROR);
	bp->b_resid = 0;
	bp->av_forw = 0;

	dp = &un->un_utab;
	if (bp != un->un_sbufp) {
#ifdef	NOT
		int part = SFPART(bp->b_dev);
		struct dk_map *lp = &un->un_map[part];
		register daddr_t bn = dkblock(bp);
		if (bn == lp->dkl_nblk) {
			/*
			 * Not an error, resid == count
			 */
			bp->b_resid = bp->b_bcount;
			iodone(bp);
		} else if (bn > lp->dkl_nblk) { 
			bp->b_flags |= B_ERROR;
		/*
		 * This should really be:
		 *
		 * ... if (bp->b_bcount & (un->un_lbasize-1)) ...
		 *
		 * but only if I put in support to handle SECSIZE
		 * size requests on disks that have a 1024 byte
		 * lbasize.
		 */
		} else if (bp->b_bcount & (SECSIZE-1)) {
			bp->b_resid = bp->b_bcount;
			bp->b_flags |= B_ERROR;
			/* if b_error == 0 ==> u.u_error == EIO */
		} else {
			/*
			 * sort by absolute block number.
			 */
			bp->b_resid = bn + lp->dkl_cylno *
				un->un_g.dkg_nhead *
				un->un_g.dkg_nsect;
			/*
			 * zero out av_back - this will be a signal
 			 * to sdstart to go and fetch the resources
			 */
			BP_PKT(bp) = 0;
		}
#else	NOT
		BP_PKT(bp) = 0;
		bp->b_resid = dkblock(bp);
#endif
	}

	if ((s = (bp->b_flags & (B_DONE|B_ERROR))) == 0) {
		s = splr(sfpri);
		disksort(dp,bp);
		if (dp->b_forw == NULL) /* this device inactive? */
			sfstart(devp);
		(void) splx(s);
	} else if((s & B_DONE) == 0) {
		iodone(bp);
	}
}

/*
 * This routine implements the ioctl calls.  It is called
 * from the device switch at normal priority.
 */
/*
 * XX- what is 'flag' used for?
 */
/*ARGSUSED3*/
sfioctl(dev, cmd, data, flag)
dev_t dev;
int cmd;
caddr_t data;
int flag;
{
	return (ENOTTY);
}

static int
sfioctl_cmd(dev,data)
dev_t dev;
caddr_t data;
{
	register struct scsi_device *devp;
	register struct buf *bp;
	int err = 0, flag, s;
	struct dk_cmd *com;
	faultcode_t fault_err = -1;

	/*
	 * Checking for sanity done in sfioctl
	 */
	devp = sfunits[SFUNIT(dev)];
	com = (struct dk_cmd *)data;

	switch (com->dkc_cmd) {
	case SCMD_READ:
		if (com->dkc_buflen & (SECSIZE-1))
			return EINVAL;
		/*FALLTHRU*/
	case SCMD_MODE_SENSE:
		flag = B_READ;
		break;

	case SCMD_WRITE:
		if (com->dkc_buflen & (SECSIZE-1))
			return EINVAL;
		/*FALLTHRU*/
	case SCMD_MODE_SELECT:
	case SCMD_TEST_UNIT_READY:
	case SCMD_REZERO_UNIT:
		flag = B_WRITE;
		break;
	default:
		return EINVAL;
	}

	if (DEBUGGING_ALL) {
		printf("sfioctl_cmd:  cmd= %x  blk= %x buflen= 0x%x\n",
			com->dkc_cmd, com->dkc_blkno, com->dkc_buflen);
		if (flag == B_WRITE && com->dkc_buflen) {
			auto u_int i, amt = min(64,com->dkc_buflen);
			char buf [64];

			if (copyin(com->dkc_bufaddr,buf,amt)) {
				return EFAULT;
			}
			printf("user's buf:");
			for (i = 0; i < amt; i++) {
				printf(" 0x%x",buf[i]&0xff);
			}
			printf("\n");
		}
	}


	/*
	 * Get buffer resources...
	 */

	bp = UPTR->un_sbufp;
	s = splr(sfpri);
	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t) bp, PRIBIO);
	}
	bzero((caddr_t) bp, sizeof (struct buf));
	bp->b_flags = B_BUSY | flag;
	(void) splx(s);

	/*
	 * Set options.
	 */
	UPTR->un_soptions = 0;
	if ((com->dkc_flags & DK_SILENT) && !(DEBUGGING_ALL)) {
		UPTR->un_soptions |= DK_SILENT;
	}
	if (com->dkc_flags & DK_ISOLATE)
		UPTR->un_soptions |= DK_ISOLATE;

	/*
	 * NB:
	 * I don't know why, but changing the order of some of these things
	 * below causes panics in kmem_free later on when ioctl(sys_generic.c)
	 * attempt to kmem_free something it shouldn't.
	 */


	/*
	 * Fill in the buffer with information we need
	 *
	 */

	bp->b_forw = (struct buf *) com;
	bp->b_dev = dev;
	if ((bp->b_bcount = com->dkc_buflen) > 0)
		bp->b_un.b_addr = com->dkc_bufaddr;
	bp->b_blkno = com->dkc_blkno;
		
	if (com->dkc_buflen & (1<<31)) {
		/*
		 * mapped in kernel address...
		 */
		com->dkc_buflen ^= (1<<31);
		bp->b_bcount = com->dkc_buflen;
		bp->b_un.b_addr = com->dkc_bufaddr;
	} else if (com->dkc_buflen) {
		bp->b_flags |= B_PHYS;
		bp->b_proc = u.u_procp;
		u.u_procp->p_flag |= SPHYSIO;
		/*
		 * Fault lock the address range of the buffer.
		 */
		fault_err = as_fault(u.u_procp->p_as, bp->b_un.b_addr,
		   		(u_int)bp->b_bcount, F_SOFTLOCK,
				(bp->b_flags & B_READ) ? S_WRITE : S_READ);
		if (fault_err != 0) {
			if (FC_CODE(fault_err) == FC_OBJERR)
				err = FC_ERRNO(fault_err);
			else
				err = EFAULT;
		} else if (buscheck(bp) < 0) {
			err = EFAULT;
		}
	}
	/*
	 * If no errors, make and run the command.
	 */
	if (err == 0) {

		make_sf_cmd(devp,bp,SLEEP_FUNC);

		sfstrategy(bp);

		s = splr(sfpri);
		while((bp->b_flags & B_DONE) == 0) {
			sleep((caddr_t) bp, PRIBIO);
		}
		(void) splx(s);

		/*
		 * Release the resources.
		 *
		 */
		err = geterror(bp);
		if (fault_err == 0) {
			(void) as_fault(u.u_procp->p_as,bp->b_un.b_addr,
					(u_int)bp->b_bcount, F_SOFTUNLOCK,
					(bp->b_flags&B_READ)? S_WRITE: S_READ);
			u.u_procp->p_flag &= ~SPHYSIO;
			bp->b_flags &= ~B_PHYS;
		}
		scsi_resfree(BP_PKT(bp));
	}


	s = splr(sfpri);
	if (bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	UPTR->un_soptions = 0;
	bp->b_flags &= ~(B_BUSY|B_WANTED);
	(void) splx(s);
	DPRINTF_IOCTL("returning %d from ioctl\n",err);
	return (err);

}

/************************************************************************
 ************************************************************************
 *									*
 *									*
 *		Unit start and Completion				*
 *									*
 *									*
 ************************************************************************
 ************************************************************************/


static void
sfstart(devp)
register struct scsi_device *devp;
{
	register struct buf *bp;
	register struct scsi_floppy *un = UPTR;
	register struct diskhd *dp = &un->un_utab;


	if (dp->b_forw) {
		printf("sfstart: busy already\n");
		return;
	} else if((bp = dp->b_actf) == NULL) {
		DPRINTF("%s%d: sfstart idle\n",DNAME,DUNIT);
		return;
	}

	if (!BP_PKT(bp)) {
		make_sf_cmd(devp,bp,sfrunout);
		if (!BP_PKT(bp)) {
			un->un_last_state = un->un_state;
			un->un_state = SF_STATE_RWAIT;
			return;
		} else {
			un->un_last_state = un->un_state;
			un->un_state = SF_STATE_OPEN;
		}
	}

	dp->b_forw = bp;
	dp->b_actf = bp->b_actf;
	bp->b_actf = 0;

	if (pkt_transport(BP_PKT(bp)) == 0) {
		printf("%s%d: transport rejected\n",DNAME,DUNIT);
		bp->b_flags |= B_ERROR;
		sfdone(devp);
	}
}


static int
sfrunout()
{
	register i, s = splr(sfpri);
	register struct scsi_device *devp;
	register struct scsi_floppy *un;

	for (i = 0; i < nsf; i++) {
		devp = sfunits[i];
		if (devp && devp->sd_present && un->un_state == SF_STATE_RWAIT) {
			DPRINTF("%s%d: resource retry\n",DNAME,DUNIT);
			sfstart(devp);
			if (un->un_state == SF_STATE_RWAIT) {
				(void) splx(s);
				return (0);
			}
			DPRINTF("%s%d: resource gotten\n",DNAME,DUNIT);
		}
	}
	(void) splx(s);
	return (1);
}

static void
sfdone(devp)
register struct scsi_device *devp;
{
	register struct buf *bp;
	register struct scsi_floppy *un = UPTR;
	register struct diskhd *dp = &un->un_utab;

	bp = dp->b_forw;
	dp->b_forw = NULL;

	/*
	 * Start the next one before releasing resources on this one
	 */
	if (dp->b_actf) {
		DPRINTF("%s%d: sfdone calling sdstart\n",DNAME,DUNIT);
		sfstart(devp);
	}

	if (bp != un->un_sbufp) {
		scsi_resfree(BP_PKT(bp));
		iodone(bp);
		DPRINTF("regular done\n");
	} else {
		DPRINTF("special done\n");
		bp->b_flags |= B_DONE;
		wakeup((caddr_t) bp);
	}
}

static void
make_sf_cmd(devp,bp,func)
register struct scsi_device *devp;
register struct buf *bp;
int (*func)();
{
	register struct scsi_pkt *pkt;
	register struct scsi_floppy *un = UPTR;
	register daddr_t blkno = 0;
	int tval = DEFAULT_SF_TIMEOUT, count, com, flags;

	flags = (scsi_options & SCSI_OPTIONS_DR) ? 0: FLAG_NODISCON;

	DPRINTF("make_sf_cmd: ");

	if (bp != un->un_sbufp) {
		struct dk_map *lp = &un->un_map[SFPART(bp->b_dev)];

		pkt = scsi_resalloc(ROUTE,CDB_GROUP0,1,(opaque_t)bp,func);
		if ((BP_PKT(bp) = pkt) == (struct scsi_pkt *) 0) {
			return;
		}
		count = bp->b_bcount >> SECDIV;
		blkno = dkblock(bp) + (lp->dkl_cylno *
				un->un_g.dkg_nhead * un->un_g.dkg_nsect);

		if (bp->b_flags & B_READ) {
			DPRINTF("read");
			com = SCMD_READ;
		} else {
			DPRINTF("write");
			com = SCMD_WRITE;
		}
		DPRINTF(" %d amt 0x%x\n",blkno,bp->b_bcount);
		makecom_g0(pkt, devp, flags, com, (int) blkno, count);
	} else {
		struct buf *abp = bp;
		int g1 = 0;
		/*
		 * Some commands are group 1 commands, some are group 0.
		 * It is legitimate to allocate a cdb sufficient for any.
		 * Therefore, we'll ask for a GROUP 1 cdb.
		 */

		/*
		 * stored in bp->b_forw is a pointer to the dk_cmd
		 */

		com  = ((struct dk_cmd *) bp->b_forw)->dkc_cmd;
		blkno = bp->b_blkno;
		count = bp->b_bcount;

		switch(com) {
		case SCMD_WRITE:
		case SCMD_READ:
			DPRINTF_IOCTL("special %s\n",(com==SCMD_READ)?"read":
				"write");
			count = bp->b_bcount >> SECDIV;
			break;
		case SCMD_MODE_SELECT:
			DPRINTF_IOCTL("mode select\n");
			break;
		case SCMD_MODE_SENSE:
			DPRINTF_IOCTL("mode sense: pcf = 0x%x, page %x\n",
				blkno>>5,blkno&0x1f);
			blkno <<= 8;
			break;
		case SCMD_FORMAT:
			DPRINTF_IOCTL("format\n");
			if (bp->b_blkno & 0x80000000) {
				/*
				 * interleave
				 */
				count = bp->b_blkno&0xff;
				blkno = (bp->b_blkno>>8)&0xff;
				/*
				 * data pattern
				 */
				blkno |= (((bp->b_blkno>>16)&0xff)<<8);
				/*
				 * format parameter bits
				 */
				blkno |= (((bp->b_blkno>>24)&0x1f)<<16);
			} else {
				blkno = (FPB_DATA|FPB_CMPLT|FPB_BFI)<<16;
				count = 1;/* interleave */
			}
			tval = 30*60;
			break;
		case SCMD_TEST_UNIT_READY:
		case SCMD_REZERO_UNIT:
			DPRINTF_IOCTL("%s\n",(com == SCMD_TEST_UNIT_READY)?
				"tur":"rezero");
			/*
			 * no data transfer...
			 */
			abp = (struct buf *) 0;
			break;
		default:
			panic("unknown special command in make_sf_cmd");
			/*NOTREACHED*/
		}
		pkt = scsi_resalloc(ROUTE,CDB_GROUP1,1,
			(opaque_t)abp,SLEEP_FUNC);
		if (g1) {
			makecom_g1(pkt, devp, flags, com, (int) blkno, count);
		} else {
			makecom_g0(pkt, devp, flags, com, (int) blkno, count);
		}
	}
	pkt->pkt_comp = sfintr;
	pkt->pkt_time = tval;
	pkt->pkt_private = (opaque_t) bp;
	pkt->pkt_pmon = -1;
	BP_PKT(bp) = pkt;
}

/*
 * This routine called to see whether unit is (still) there. Must not
 * be called when un->un_sbufp is in use, and must not be called with
 * an unattached disk. Soft state of disk is restored to what it was
 * upon entry- up to caller to set the correct state.
 *
 */

static int
sf_unit_ready(dev)
dev_t dev;
{
	struct scsi_device *devp = sfunits[SFUNIT(dev)];
	struct scsi_floppy *un = UPTR;
	auto struct dk_cmd cblk, *com = &cblk;
	u_char state;

	un->un_last_state = un->un_state;
	un->un_state = SF_STATE_OPENING;
	com->dkc_cmd = SCMD_REZERO_UNIT;
	com->dkc_flags = DK_SILENT;
	com->dkc_blkno = 0;
	com->dkc_secnt = 0;
	com->dkc_bufaddr = 0;
	com->dkc_buflen = 0;

	if (sfioctl_cmd(dev,com)) {
		state = un->un_state;
		un->un_last_state = un->un_state;
		un->un_state = state;
		return (0);
	}
	state = un->un_state;
	un->un_last_state = un->un_state;
	un->un_state = state;
	return (1);
}

static int
sf_get_density(dev)
dev_t dev;
{
	struct scsi_device *devp = sfunits[SFUNIT(dev)];
	struct scsi_floppy *un = UPTR;
	struct ccs_modesel_head *hp = (struct ccs_modesel_head *) un->un_mode;
	auto struct dk_cmd cblk, *com = &cblk;
	caddr_t dblk;
	register int i, dens, rval = -1;
	u_char state;

	un->un_last_state = un->un_state;
	un->un_state = SF_STATE_OPENING;
	dblk = kmem_alloc((unsigned)SECSIZE);
	if (dblk == 0) {
		printf("%s%d: unable to alloc space\n",DNAME,DUNIT);
		return (rval);
	}

	for (dens = 0; media_types[dens] != 0; dens++) {
		if (sf_get_mode(dev,com) == 0) {
			break;
		}
		if (dens == 0)
			media_types[dens] = hp->medium;
		hp->medium = media_types[dens];
		DPRINTF_IOCTL("%s%d: trying to set density %x\n",DNAME,DUNIT,
			hp->medium);
		if (sf_set_shortmode(dev,com)) {
			DPRINTF_IOCTL("%s%d: mode select fails; key %x\n",
				DNAME,DUNIT,un->un_status);
			continue;
		}
		com->dkc_cmd = SCMD_READ;
		com->dkc_flags = DK_SILENT;
		com->dkc_blkno = 0;
		com->dkc_secnt = 1;
		com->dkc_bufaddr = dblk;
		com->dkc_buflen = SECSIZE | (1<<31);
		if (sfioctl_cmd(dev,com) == 0) {
			rval = 0;
			DPRINTF_IOCTL("%s%d: test read succeeds; dens=%x\n",
				DNAME,DUNIT,media_types[dens]&0xff);
			break;			
		}
		DPRINTF_IOCTL("%s%d: test read fails; key %x\n",DNAME,DUNIT,
			un->un_status);
	}

	(void) kmem_free (dblk, (unsigned) SECSIZE);
	state = un->un_state;
	un->un_last_state = un->un_state;
	un->un_state = state;
	return (rval);
}

static int
sf_get_mode(dev,com)
dev_t dev;
struct dk_cmd *com;
{
	struct scsi_device *devp = sfunits[SFUNIT(dev)];
	struct scsi_floppy *un = UPTR;

	com->dkc_cmd = SCMD_MODE_SENSE;
	com->dkc_flags = DK_SILENT;
	com->dkc_blkno = 0x5;
	com->dkc_secnt = 0;
	com->dkc_bufaddr = un->un_mode;
	com->dkc_buflen = MSIZE | (1<<31);/* to avoid an as_fault */
	bzero(un->un_mode,MSIZE);
	if (sfioctl_cmd(dev,com)) {
		return (0);
	}
	if (DEBUGGING_ALL) {
		register i;
		char *p = un->un_mode;
		printf("mode sense data:\n");
		for (i = 0; i < MSIZE; i++) {
			printf(" 0x%x",p[i]&0xff);
			if (((i+1)&0xf) == 0)
				printf("\n");
		}
		printf("\n");
	}
	return (1);
}

static int
sf_set_shortmode(dev,com)
dev_t dev;
struct dk_cmd *com;
{
	struct scsi_device *devp = sfunits[SFUNIT(dev)];
	struct scsi_floppy *un = UPTR;
	struct ccs_modesel_head *hp = (struct ccs_modesel_head *) un->un_mode;

	hp->block_desc_length = 0;
	com->dkc_cmd = SCMD_MODE_SELECT;
	com->dkc_flags = DK_SILENT;
	com->dkc_blkno = 0;
	com->dkc_secnt = 0;
	com->dkc_bufaddr = un->un_mode;
	com->dkc_buflen =  4 | (1<<31);
	if (sfioctl_cmd(dev,com)) {
		return (0);
	}
	return (1);
}

static int
sf_set_density(dev)
dev_t dev;
{
	return (0);
}

/************************************************************************
 ************************************************************************
 *																		*
 *																		*
 *				Interrupt Service Routines								*
 *																		*
 *																		*
 ************************************************************************
 ************************************************************************/

static
sfrestart(arg)
caddr_t arg;
{
		struct scsi_device *devp = (struct scsi_device *) arg;
		struct buf *bp;
		register s = splr(sfpri);
		printf("sfrestart\n");
		if ((bp = UPTR->un_utab.b_forw) == 0) {
				printf("%s%d: busy restart aborted\n",DNAME,DUNIT);
		} else {
				struct scsi_pkt *pkt;
				if (UPTR->un_state == SF_STATE_SENSING) {
						pkt = UPTR->un_rqs;
				} else {
						pkt = BP_PKT(bp);
				}
				if (pkt_transport(pkt) == 0) {
						printf("%s%d: restart transport failed\n",DNAME,DUNIT);
						UPTR->un_state = UPTR->un_last_state;
						bp->b_resid = bp->b_bcount;
						bp->b_flags |= B_ERROR;
						sfdone(devp);
				}
		}
		(void) splx(s);
}

/*
 * Command completion processing
 *
 */
static void
sfintr(pkt)
struct scsi_pkt *pkt;
{
		register struct scsi_device *devp;
		register struct scsi_floppy *un;
		register struct buf *bp;
		register action;

		if (pkt->pkt_flags & FLAG_NOINTR) {
				printf("sfintr:internal error - got a non-interrupting cmd\n");
				return;
		}

		DPRINTF("sfintr:\n");
		bp = (struct buf *) pkt->pkt_private;
		ASSERT(bp != NULL);
		devp = sfunits[SFUNIT(bp->b_dev)];
		un = UPTR;
		if(bp != un->un_utab.b_forw) {
				printf("sfintr: buf not on queue?");
				panic("sfintr1");
		}

		if (pkt->pkt_reason != CMD_CMPLT) {
				action = sf_handle_incomplete(devp);
		/*
		 * At this point we know that the command was successfully
		 * completed. Now what?
		 */
		} else if (un->un_state == SF_STATE_SENSING) {
				/*
				 * okay. We were running a REQUEST SENSE. Find
				 * out what to do next.
				 */
				ASSERT(pkt == un->un_rqs);
				un->un_state = un->un_last_state;
				un->un_last_state = un->un_state;
				pkt = BP_PKT(bp);
				action = sf_handle_sense(devp);
		/*
		 * Okay, we weren't running a REQUEST SENSE. Call a routine
		 * to see if the status bits we're okay. As a side effect,
		 * clear state for this device, set non-error b_resid values, etc.
		 * If a request sense is to be run, that will happen.
		 */
		} else {
				action = sf_check_error(devp);
		}

		switch(action) {
		case COMMAND_DONE_ERROR:
				un->un_err_severe = DK_FATAL;
				un->un_err_resid = bp->b_resid = bp->b_bcount;
				bp->b_flags |= B_ERROR;
				/* FALL THRU */
		case COMMAND_DONE:
				sfdone(devp);
				break;
		case QUE_SENSE:
				un->un_last_state = un->un_last_state;
				un->un_state = SF_STATE_SENSING;
				un->un_rqs->pkt_private = (opaque_t) bp;
				bzero((caddr_t)devp->sd_sense,SENSE_LENGTH);
				if (pkt_transport(un->un_rqs) == 0) {
						panic("sfintr: transport of request sense fails");
						/*NOTREACHED*/
				}
				break;
		case QUE_COMMAND:
				if (pkt_transport(BP_PKT(bp)) == 0) {
						printf("sfintr: requeue of command fails\n");
						un->un_err_resid = bp->b_resid = bp->b_bcount;		
						bp->b_flags |= B_ERROR;
						sfdone(devp);
				}
				break;
		case JUST_RETURN:
				break;
		}
}


static int
sf_handle_incomplete(devp)
struct scsi_device *devp;
{
		static char *notresp = "%s%d: disk not responding to selection\n";
		register rval = COMMAND_DONE_ERROR;
		struct scsi_floppy *un = UPTR;
		struct buf *bp = un->un_utab.b_forw;
		struct scsi_pkt *pkt = (un->un_state == SF_STATE_SENSING)?
				un->un_rqs : BP_PKT(bp);

		if (pkt == un->un_rqs) {
				un->un_state = un->un_last_state;
				un->un_last_state = SF_STATE_SENSING;
				if (un->un_retry_ct++ < SF_RETRY_COUNT)
						rval = QUE_SENSE;
		} else if (bp == un->un_sbufp && (un->un_soptions & DK_ISOLATE)) {
				rval = COMMAND_DONE_ERROR;
		} else if (un->un_retry_ct++ < SF_RETRY_COUNT) {
				rval = QUE_COMMAND;
		}

		if (pkt->pkt_state == STATE_GOT_BUS && rval == COMMAND_DONE_ERROR) {
				/*
				 * Looks like someone turned off this shoebox.
				 */
				printf(notresp,DNAME,DUNIT);
				un->un_last_state = un->un_state;
				un->un_state = SF_STATE_DETACHING;
		} else if (bp != un->un_sbufp || (un->un_soptions & DK_SILENT) == 0) {
				printf("%s%d: command transport failed: reason '%s': %s\n",
						DNAME,DUNIT,scsi_rname(pkt->pkt_reason),
						(rval == COMMAND_DONE_ERROR)?"giving up":"retrying");
		}
		return rval;
}


static int
sf_handle_sense(devp)
register struct scsi_device *devp;
{
		register struct scsi_floppy *un = UPTR;
		register struct buf *bp = un->un_utab.b_forw;
		struct scsi_pkt *pkt = BP_PKT(bp), *rqpkt = un->un_rqs;
		register rval = COMMAND_DONE_ERROR;
		int severity, amt, i;
		char *p;
		static char *hex =" 0x%x";

		if (SCBP(rqpkt)->sts_busy) {
				printf ("%s%d: busy unit on request sense\n",DNAME,DUNIT);
				if (un->un_retry_ct++ < SF_RETRY_COUNT) {
						timeout (sfrestart, (caddr_t)devp, SFTIMEOUT);
						rval = JUST_RETURN;
				}
				return(rval);
		}

		if (SCBP(rqpkt)->sts_chk) {
				printf ("%s%d: Check Condition on REQUEST SENSE\n",DNAME,DUNIT);
				return (rval);
		}
		amt = SENSE_LENGTH - rqpkt->pkt_resid;
		if ((rqpkt->pkt_state&STATE_XFERRED_DATA) == 0 || amt == 0) {
				printf("%s%d: Request Sense couldn't get sense data\n",DNAME,
						DUNIT);
				return (rval);
		}

		/*
		 * Now, check to see whether we got enough sense data to make any
		 * sense out if it (heh-heh).
		 */

		if (amt < sizeof (struct scsi_extended_sense)) {
				if (amt < sizeof (struct scsi_sense)) {
						printf("%s%d: couldn't get enough sense information\n",
								DNAME,DUNIT);
						return (rval);
				}
				if (devp->sd_sense->es_class != CLASS_EXTENDED_SENSE) {
						p = (char *) devp->sd_sense;
						printf("%s%d: undecoded sense information:");
						for (i = 0; i < amt; i++) {
								printf(hex,*(p++)&0xff);
						}
						printf("; assuming a fatal error\n");
						return (rval);
				}
		}


		un->un_status = devp->sd_sense->es_key;
		un->un_err_code = devp->sd_sense->es_code;

		if (devp->sd_sense->es_valid) {
				un->un_err_blkno =		(devp->sd_sense->es_info_1 << 24) |
										(devp->sd_sense->es_info_2 << 16) |
										(devp->sd_sense->es_info_3 << 8)  |
										(devp->sd_sense->es_info_4);
		} else {
				un->un_err_blkno = bp->b_blkno;
		}

		if (DEBUGGING_ALL || sf_error_reporting == SDERR_ALL) {
				p = (char *) devp->sd_sense;
				printf("%s%d:sdata:",DNAME,DUNIT);
				for (i = 0; i < amt; i++) {
						printf(hex,*(p++)&0xff);
				}
				printf("\n      cmd:");
				p = pkt->pkt_cdbp;
				for (i = 0; i < CDB_SIZE; i++)
						printf(hex,*(p++)&0xff);
				printf("\n");
		}

		switch (un->un_status) {
		case KEY_NO_SENSE:
				un->un_err_severe = DK_NOERROR;
				severity = SDERR_RETRYABLE;
				rval = SDERR_INFORMATIONAL;
				break;

		case KEY_RECOVERABLE_ERROR:

				bp->b_resid = pkt->pkt_resid;
				un->un_err_severe = DK_CORRECTED;
				un->un_retry_ct = 0;
				severity = SDERR_RECOVERED;
				rval = COMMAND_DONE;
				break;

		case KEY_NOT_READY:
		case KEY_MEDIUM_ERROR:
		case KEY_HARDWARE_ERROR:
		case KEY_ILLEGAL_REQUEST:
		case KEY_VOLUME_OVERFLOW:
		case KEY_WRITE_PROTECT:
		case KEY_BLANK_CHECK:
				un->un_err_severe = DK_FATAL;
				severity = SDERR_FATAL;
				rval = COMMAND_DONE_ERROR;
				break;


		case KEY_ABORTED_COMMAND:
				severity = SDERR_RETRYABLE;
				rval = QUE_COMMAND;
				break;

		case KEY_UNIT_ATTENTION:
		{
				bp->b_resid = pkt->pkt_resid;
				un->un_err_severe = DK_NOERROR;
				un->un_retry_ct = 0;
				severity = SDERR_INFORMATIONAL;
				rval = QUE_COMMAND;
				break;
		}
		default:
				/*
				 * Undecoded sense key.  Try retries and hope
				 * that will fix the problem.  Otherwise, we're
				 * dead.
				 */
				printf("%s%d: Sense Key '%s'\n",DNAME,DUNIT,
						sense_keys[un->un_status]);
				if (un->un_retry_ct++ < SF_RETRY_COUNT) {
						un->un_err_severe = DK_RECOVERED;
						severity = SDERR_RETRYABLE;
						rval = QUE_COMMAND;
				} else {
						un->un_err_severe = DK_FATAL;
						severity = SDERR_FATAL;
						rval = COMMAND_DONE_ERROR;
				}
		}

		if (rval == QUE_COMMAND && bp == un->un_sbufp &&
						(un->un_soptions & DK_DIAGNOSE) == 0) {
				un->un_err_severe = DK_FATAL;
				rval = COMMAND_DONE_ERROR;
		}

		if (bp == un->un_sbufp) {
				if ((un->un_soptions & DK_SILENT) == 0) {
						sferrmsg(devp,severity);
				}
		} else if (DEBUGGING_ALL || severity >= sf_error_reporting) {
				sferrmsg(devp,severity);
		}
		return (rval);
}

static int
sf_check_error(devp)
register struct scsi_device *devp;
{
		register struct scsi_floppy *un = UPTR;
		struct buf *bp = un->un_utab.b_forw;
		register struct scsi_pkt *pkt = BP_PKT(bp);
		register action;

		if (SCBP(pkt)->sts_busy) {
				printf("%s%d: unit busy\n",DNAME,DUNIT);
				if (un->un_retry_ct++ < SF_RETRY_COUNT) {
						timeout(sfrestart,(caddr_t)devp,SFTIMEOUT);
						action = JUST_RETURN;
				} else {
						printf("%s%d: device busy too long\n",DNAME,DUNIT);
						action = COMMAND_DONE_ERROR;
				}
		} else if (SCBP(pkt)->sts_chk) {
				DPRINTF("%s%d: check condition\n",DNAME,DUNIT);
				action = QUE_SENSE;
		} else {
				/*
				 * pkt_resid will reflect, at this point, a residual
				 * of how many bytes left to be transferred there were
				 *
				 */
				bp->b_resid = pkt->pkt_resid;
				action = COMMAND_DONE;
				un->un_err_severe = DK_NOERROR;
				un->un_retry_ct = 0;
		}
		return (action);
}
/************************************************************************
 ************************************************************************
 *																		*
 *																		*
 *				Error Printing												*
 *																		*
 *																		*
 ************************************************************************
 ************************************************************************/

static char *sf_cmds[] = {
		"\000test unit ready",				/* 0x00 */
		"\001rezero",						/* 0x01 */
		"\003request sense",				/* 0x03 */
		"\004format",						/* 0x04 */
		"\007reassign",						/* 0x07 */
		"\010read",						/* 0x08 */
		"\012write",						/* 0x0a */
		"\013seek",						/* 0x0b */
		"\022inquiry",						/* 0x12 */
		"\025mode select",				/* 0x15 */
		"\026reserve",						/* 0x16 */
		"\027release",						/* 0x17 */
		"\030copy",						/* 0x18 */
		"\032mode sense",				/* 0x1a */
		"\033start/stop",				/* 0x1b */
		"\036door lock",				/* 0x1e */
		"\067read defect data",				/* 0x37 */
		NULL
};


static void
sferrmsg(devp, level)
register struct scsi_device *devp;
int level;
{
		static char *error_classes[] = {
				"All          ", "Unknown      ", "Informational",
				"Recovered    ", "Retryable    ", "Fatal        "
		};
		char *class;
		struct scsi_pkt *pkt;

		class = error_classes[level];
		pkt = BP_PKT(UPTR->un_utab.b_forw);
		printf("%s%d:\tError for command '%s'\n",DNAME,DUNIT,
				scsi_cmd_decode(CDBP(pkt)->scc_cmd,sf_cmds));
		printf("    \tError Level: %s Block: %d\n",class,UPTR->un_err_blkno);
		printf("    \tSense Key:   %s\n",sense_keys[devp->sd_sense->es_key]);

		if (devp->sd_sense->es_code) {
				printf("    \tVendor Unique Error Code: %x",
						devp->sd_sense->es_code);
		}
		printf("\n");
}
#endif		NSF > 0
