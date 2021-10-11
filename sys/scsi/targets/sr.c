#ident	"@(#)sr.c 1.1 92/07/30 SMI"

/* @(#)sr.c 1.1 92/07/30 SMI */
/*
 * Copyright (c) 1988-1991 by Sun Microsystems, Inc.
 */
#include "sr.h"
#if	NSR > 0
/*
 *
 *		SCSI CDROM driver (for CDROM, read-only)
 *
 */

/*
 *
 *
 *		Includes, Declarations and Local Data
 *
 *
 */

#include <scsi/scsi.h>
#include <scsi/targets/srdef.h>
#include <scsi/impl/uscsi.h>
#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>

/*
 *
 * Local definitions, for clarity of code
 *
 */

#ifdef	OPENPROMS
#define	DRIVER		sr_ops
#define	DNAME		devp->sd_dev->devi_name
#define	DUNIT		devp->sd_dev->devi_unit
#define	CNAME		devp->sd_dev->devi_parent->devi_name
#define	CUNIT		devp->sd_dev->devi_parent->devi_unit
#else	OPENPROMS
#define	DRIVER		srdriver
#define	DNAME		devp->sd_dev->md_driver->mdr_dname
#define	DUNIT		devp->sd_dev->md_unit
#define	CNAME		devp->sd_dev->md_driver->mdr_cname
#define	CUNIT		devp->sd_dev->md_mc->mc_ctlr
#endif	OPENPROMS

#define	UPTR		((struct scsi_disk *)(devp)->sd_private)
#define	SCBP(pkt)	((struct scsi_status *)(pkt)->pkt_scbp)
#define	SCBP_C(pkt)	((*(pkt)->pkt_scbp) & STATUS_MASK)
#define	CDBP(pkt)	((union scsi_cdb *)(pkt)->pkt_cdbp)
#define	ROUTE		(&devp->sd_address)
#define	BP_PKT(bp)	((struct scsi_pkt *)bp->av_back)

#define	Tgt(devp)	(devp->sd_address.a_target)
#define	Lun(devp)	(devp->sd_address.a_lun)
#define	SRUNIT(dev)	(minor((dev))>>3)
#define	SRPART(dev)	(minor((dev))&0x7)


#define	New_state(un, s) (un)->un_last_state=(un)->un_state, (un)->un_state=(s)
#define	Restore_state(un)	\
	{ u_char tmp = (un)->un_last_state; New_state((un), tmp); }

/*
 * Parameters
 */

	/*
	 * 180 seconds is a reasonable amount of time for CDROM operations
	 */
#define	DEFAULT_SR_TIMEOUT	180

	/*
	 * 180 seconds is what we'll wait if we get a Busy Status back
	 * Note: worse case timeout with 30 second x (1 + 5 retrys) = 180
	 */

#define	SRTIMEOUT		180*hz	/* 180 seconds Busy Waiting */

	/*
	 * Number of times we'll retry a normal operation.
	 *
	 * XXX This includes retries due to transport failure
	 * XXX Need to distinguish between Target and Transport
	 * XXX failure.
	 */

#define	SR_RETRY_COUNT		1

	/*
	 * Maximum number of retries to handle unit attention key
	 */

#define	SR_UNIT_ATTENTION_RETRY	40

	/*
	 * Maximum number of units (controlled by space in minor device byte)
	 */

#define	SR_MAXUNIT		32

	/*
	 * Maximum number of retry when trying to read the capacity from
	 * the disk.
	 */

#define	MAX_RETRY	20

/*
 * Debugging macros
 */


#define	DEBUGGING	((scsi_options & SCSI_DEBUG_TGT) || srdebug > 1)
#define	DEBUGGING_ALL	((scsi_options & SCSI_DEBUG_TGT) || srdebug)
#define	DPRINTF		if (DEBUGGING) printf
#define	DPRINTF_ALL	if (DEBUGGING || srdebug > 0) printf
#define	DPRINTF_IOCTL	DPRINTF_ALL

/*
 *
 * Global Data Definitions and references
 *
 */

struct scsi_device *srunits[SR_MAXUNIT];

int nsr = SR_MAXUNIT;

extern int maxphys, dkn;

/*
 *
 * Local Static Data
 *
 */

static struct scsi_address *srsvdaddr[SR_MAXUNIT];

static int srpri = 0;
static int srdebug = 0;
static int sr_error_reporting = SRERR_RETRYABLE;

static struct sr_drivetype sr_drivetypes[] = {
/* SONY CDU-541 */
{ "Sony", 		0,		CTYPE_CCS,	4, "SONY" },
{ "Hitachi",		0,		CTYPE_CCS,	7, "HITACHI" },
};

/*
 * Generic CCS scsi drive with very pessimistic assumptions as to capabilities.
 * Well, for now, assume it has reconnect capacities.
 */
static struct sr_drivetype generic_ccs =
{ "CCS",	0,		CTYPE_CCS,	4,	"<??>"	};

#define	MAX_SRTYPES (sizeof sr_drivetypes/ (sizeof sr_drivetypes[0]))

/*
 * Configuration Data
 */

#ifdef	OPENPROMS
/*
 * Device driver ops vector
 */
int srslave(), srattach(), sropen(), srclose(), srread();
int srstrategy(), srioctl(), srsize();
extern int nulldev(), nodev();
struct dev_ops sr_ops = {
	1,
	srslave,
	srattach,
	sropen,
	srclose,
	srread,
	nodev,
	srstrategy,
	nodev,
	srsize,
	srioctl
};
#else	OPENPROMS
/*
 * srdriver is only used to attach
 */
int srslave(), srattach(), sropen(), srclose(), srread();
int srstrategy(), srdump(), srioctl(), srsize();
extern int nulldev(), nodev();
struct mb_driver srdriver = {
	nulldev, srslave, srattach, nodev, nodev, nulldev, 0, "sr", 0, 0, 0, 0
};
#endif

/*
 *
 * Local Function Declarations
 *
 */

static void clean_print(), srintr();
static void srerrmsg(), srintr_adaptec(), srdone(), sr_offline();
static void make_sr_cmd(), srstart(), srdone(), sr_error_code_print();
static int srrunout(), sr_findslave(), sr_unit_ready(), sr_medium_removal();
static	void	makecom_all();
static	int	sr_pause_resume(), sr_play_msf(), sr_play_trkind();
static	int	sr_read_tochdr(), sr_read_tocentry();
static	void	sr_not_ready();
static	void	sr_ejected();
#ifdef OLDCODE
static	int	sr_read_new_disc();
#endif OLDCODE
static	int	sr_read_mode2(), sr_read_mode1();
static	int	sr_mode_1(), sr_mode_2();
static	int	sr_handle_ua();
static	int	sr_lock_door();
static	void	sr_setup_openflags();

#ifndef	FIVETWELVE
static	int	sr_two_k();
#endif	FIVETWELVE

#ifdef OPENPROMS
static	void	sr_set_dkn();
#endif OPENPROMS

/**/
/*
 *
 *
 *		Autoconfiguration Routines
 *
 *
 */

int
srslave(devp)
struct scsi_device *devp;
{
	int r;
	/*
	 * fill in our local array
	 */

	if (DUNIT >= nsr)
		return (0);
	srunits[DUNIT] = devp;

#ifdef	OPENPROMS
	srpri = MAX(srpri, ipltospl(devp->sd_dev->devi_intr->int_pri));
#else
	srpri = MAX(srpri, pritospl(devp->sd_dev->md_intpri));
#endif
	/*
	 * Turn around and call real slave routine. If it returns -1,
	 * then try and save the address structure for a possible later open.
	 */
	r = sr_findslave (devp, 0);
	if (r < 0) {
		srsvdaddr[DUNIT] = (struct scsi_address *)
		    kmem_zalloc(sizeof (struct scsi_address));
		if (srsvdaddr[DUNIT]) {
			srunits[DUNIT] = 0;
			*srsvdaddr[DUNIT] = devp->sd_address;
		} else {
			r = 0;
		}
	}
	return (r);
}

static int
sr_findslave(devp, canwait)
register struct scsi_device *devp;
int canwait;
{
	struct scsi_pkt *rqpkt;
	struct scsi_disk *un;
	struct sr_drivetype *dp;
	auto int (*f)() = (canwait == 0)? NULL_FUNC: SLEEP_FUNC;

	/*
	 * Call the routine scsi_slave to do some of the dirty work.
	 * All scsi_slave does is do a TEST UNIT READY (and possibly
	 * a non-extended REQUEST SENSE or two), and an INQUIRY command.
	 * If the INQUIRY command succeeds, the field sd_inq in the
	 * device structure will be filled in. The sd_sense structure
	 * will also be allocated.
	 *
	 */

	switch (scsi_slave(devp, canwait)) {
	default:
	case SCSIPROBE_NOMEM:
	case SCSIPROBE_FAILURE:
	case SCSIPROBE_NORESP:
	case SCSIPROBE_NONCCS:
		if (devp->sd_inq) {
			IOPBFREE (devp->sd_inq, sizeof (struct scsi_inquiry));
			devp->sd_inq = (struct scsi_inquiry *) 0;
		}
		return (-1);
	case SCSIPROBE_EXISTS:
		/*
		 * The only type of device that we're
		 * interested in is READ ONLY DIRECT ACCESS.
		 */
		if (devp->sd_inq->inq_dtype != DTYPE_RODIRECT) {
			IOPBFREE (devp->sd_inq, sizeof (struct scsi_inquiry));
			devp->sd_inq = (struct scsi_inquiry *) 0;
			return (-1);
		}
		/*
		 * XXX: should you check to make sure that the inq_rmb
		 * XXX: is set? So that all the eject code doesn't
		 * XXX: barf?
		 */
		break;
	}

	/*
	 * allocate a request sense packet
	 */
	rqpkt = get_pktiopb(ROUTE, (caddr_t *)&devp->sd_sense,
			CDB_GROUP0, 1, SENSE_LENGTH, B_READ, f);
	if (!rqpkt) {
		return (0);
	}

	makecom_g0(rqpkt, devp, 0, SCMD_REQUEST_SENSE, 0, SENSE_LENGTH);
	rqpkt->pkt_pmon = -1;

	/*
	 * fill up some of the scsi disk information.
	 */

	un = UPTR = (struct scsi_disk *)kmem_zalloc(sizeof (struct scsi_disk));
	if (!un) {
		printf("%s%d: no memory for disk structure\n", DNAME, DUNIT);
		free_pktiopb(rqpkt, (caddr_t)devp->sd_sense, SENSE_LENGTH);
		return (0);
	}
	un->un_rbufp = (struct buf *) kmem_zalloc(sizeof (struct buf));
	un->un_sbufp = (struct buf *) kmem_zalloc(sizeof (struct buf));
	if (!un->un_rbufp || !un->un_sbufp) {
		printf("srslave: no memory for raw or special buffer\n");
		if (un->un_rbufp)
			(void) kmem_free((caddr_t)un->un_rbufp,
					sizeof (struct buf));
		(void) kmem_free((caddr_t)un, sizeof (struct scsi_disk));
		free_pktiopb(rqpkt, (caddr_t)devp->sd_sense, SENSE_LENGTH);
		return (0);
	}

	/*
	 * For sure the drive is present, and we're going to keep it.
	 * Since this is (we hope) removable media, we don't need to read the
	 * media at this point. We'll do it during the first open.
	 */

	devp->sd_present = 1;

	/*
	 * Sigh. Look through our list of supported drives to dredge
	 * up information that we haven't figured out a clever way
	 * to get inference yet.
	 */

	for (dp = &sr_drivetypes[0]; dp < &sr_drivetypes[MAX_SRTYPES]; dp++) {
		if (bcmp(devp->sd_inq->inq_vid, dp->id, dp->idlen) == 0) {
			un->un_dp = dp;
			break;
		}
	}
	if (dp == &sr_drivetypes[MAX_SRTYPES]) {
		/*
		 * assume CCS drive, don't assume anything else.
		 */
		printf("%s%d: Unrecongized Vendor '", DNAME, DUNIT);
		clean_print(&(devp->sd_inq->inq_vid[0]), 8);
		printf("', product '");
		clean_print(&(devp->sd_inq->inq_pid[0]), 16);
		printf("'");
		un->un_dp = (struct sr_drivetype *)
			kmem_zalloc(sizeof (struct sr_drivetype));
		if (!un->un_dp || !(un->un_dp->name = kmem_zalloc(8))) {
			un->un_dp = &generic_ccs;
		} else {
			bcopy(devp->sd_inq->inq_vid, un->un_dp->name, 8);
			un->un_dp->name[7] = 0;
			bcopy(devp->sd_inq->inq_vid, un->un_dp->id, IDMAX);
			un->un_dp->idlen = IDMAX;
			un->un_dp->ctype = CTYPE_CCS;
		}
	}

	un->un_capacity = -1;
	un->un_lbasize = -1;

	/*
	 * initialize open and exclusive open flags
	 */
	un->un_open0 = un->un_open1 = 0;
	un->un_exclopen = 0;

	rqpkt->pkt_comp = srintr;
	rqpkt->pkt_time = DEFAULT_SR_TIMEOUT;
	if (un->un_dp->options & SR_NODISC)
		rqpkt->pkt_flags |= FLAG_NODISCON;
	un->un_rqs = rqpkt;
	un->un_sd = devp;
	un->un_dev = DUNIT<<3;
#ifdef	OPENPROMS
	devp->sd_dev->devi_driver = &sr_ops;
#endif
	return (1);
}


/*
 * Attach disk. The controller is there. Since this is a CDROM,
 * there is no need to check the label.
 */

int
srattach(devp)
struct scsi_device *devp;
{
	register struct scsi_disk *un;

	if (!(un = UPTR)) {
		return;
	}

#ifndef FIVETWELVE
	/*
	 * Set the drive's sector size to 2048
	 */
	(void)sr_two_k(devp);
#endif	FIVETWELVE

	/*
	 * initialize the geometry and partition table.
	 */
	bzero((caddr_t) & un->un_g, sizeof (struct dk_geom));
	bzero((caddr_t)un->un_map, NDKMAP * (sizeof (struct dk_map)));

	/*
	 * It's CDROM, therefore, no label. But it is still necessary
	 * to set up various geometry information. And we are doing
	 * this here.
	 * For the number of sector per track, we put the maximum
	 * number, assuming a CDROM can hold up to 600 Mbytes of data.
	 * For the rpm, we use the minimum for the disk.
	 */
	un->un_g.dkg_ncyl = 1;
	un->un_g.dkg_acyl = 0;
	un->un_g.dkg_bcyl = 0;
	un->un_g.dkg_nhead = 1;
	un->un_g.dkg_nsect = 0;
	un->un_g.dkg_intrlv = 1;
	un->un_g.dkg_rpm = 200;
	un->un_gvalid = 1;

	/*
	 * dk instrumentation
	 */
#ifdef	OPENPROMS
	if ((un->un_dkn = (char) newdk("sr", DUNIT)) >= 0) {
		int intrlv = un->un_g.dkg_intrlv;
		if (intrlv <= 0 || intrlv >= un->un_g.dkg_nsect) {
			intrlv = 1;
		}
		dk_bps[un->un_dkn] =
			(SECSIZE * 60 * un->un_g.dkg_nsect) / intrlv;
	}
#endif	OPENPROMS	
}

static void
clean_print(p, l)
char *p;
int l;
{
	char a[2];
	register unsigned i = 0, c;
	while (i < l) {
		if ((c = *p++) < ' ' || c >= 0177)
			printf("\\%o", c&0xff);
		else {
			a[0] = c; a[1] = 0;
			printf("%s", a);
		}
		i++;
	}
}

#ifdef	OPENPROMS
static void
sr_set_dkn(devp)
struct scsi_device	*devp;
{
	register struct scsi_disk *un;

	un = UPTR;
	if ((un->un_dkn = (char) newdk("sr", DUNIT)) >= 0) {
		int intrlv = un->un_g.dkg_intrlv;
		if (intrlv <= 0 || intrlv >= un->un_g.dkg_nsect) {
			intrlv = 1;
		}
		dk_bps[un->un_dkn] =
			(SECSIZE * 60 * un->un_g.dkg_nsect) / intrlv;
	}
}
#endif OPENPROMS

/**/
/*
 *
 *
 *			Unix Entry Points
 *
 *
 */

/*ARGSUSED*/
int
sropen(dev, flag)
dev_t dev;
int flag;
{
	register struct scsi_device *devp;
	register struct scsi_disk *un;
	register int unit, s;

	DPRINTF("in sropen\n");
	if ((unit = SRUNIT(dev)) >= nsr) {
		return (ENXIO);
	}

	if (!(devp = srunits[unit])) {
		struct scsi_address *sa = srsvdaddr[unit];
		if (sa) {
			if (scsi_add_device(sa, &DRIVER, "sr", unit) == 0) {
				return (ENXIO);
			}
			devp = srunits[unit];
			if (!devp)
				panic("sropen");
			(void) kmem_free((caddr_t) sa, sizeof (*sa));
			srsvdaddr[unit] = (struct scsi_address *) 0;
		} else {
			return (ENXIO);
		}
	} else if (!devp->sd_present) {
		s = splr(srpri);
		if (sr_findslave (devp, 1) > 0) {
			printf("%s%d at %s%d target %d lun %d\n", DNAME, DUNIT,
				CNAME, CUNIT, Tgt(devp), Lun(devp));
			srattach(devp);
		} else {
			(void) splx(s);
			return (ENXIO);
		}
		(void) splx(s);
	}
	un = UPTR;

	/*
	 * check for previous exclusive open
	 */
	if ((un->un_exclopen) ||
	    ((flag & FEXCL) && ((un->un_open0 != 0) ||
				(un->un_open1 != 0)))) {
		return (EBUSY);
	}

	DPRINTF("un->un_state is %d\n", un->un_state);
	if ((un->un_state == SR_STATE_NIL) ||
	    (un->un_state == SR_STATE_CLOSED) ||
	    (un->un_state == SR_STATE_EJECTED)) {
		u_char state;

		state = un->un_last_state;
		New_state(un, SR_STATE_OPENING);
		if (sr_unit_ready(dev) == 0) {
			sr_not_ready(devp);
			un->un_state = un->un_last_state;
			un->un_last_state = state;
			return (ENXIO);
		}
		state = un->un_last_state;
		New_state(un, SR_STATE_OPENING);
		if (sr_read_capacity(devp) != 0) {
			un->un_state = un->un_last_state;
			un->un_last_state = state;
			return (ENXIO);
		}
		un->un_state = un->un_last_state;
		un->un_last_state = state;

		/*
		 * now lock the drive door, if it is the first time open
		 */
		if ((un->un_open0 == 0) &&
		    (un->un_open1 == 0)) {
			state = un->un_last_state;
			New_state(un, SR_STATE_OPENING);
			if (sr_medium_removal(dev, SR_REMOVAL_PREVENT) != 0) {
				un->un_state = un->un_last_state;
				un->un_last_state = state;
				return (ENXIO);
			}
			un->un_state = un->un_last_state;
			un->un_last_state = state;
		}
	} else if (un->un_state == SR_STATE_DETACHING) {
		u_char state;

		if (sr_unit_ready(dev) == 0) {
			return (ENXIO);
		}
		/*
		 * disk now back on line...
		 */
		New_state(un, SR_STATE_OPENING);
		printf("%s%d: disk okay\n", DNAME, DUNIT);
		if (sr_read_capacity(devp) != 0) {
			return (ENXIO);
		}
		/* now lock the drive door */
		state = un->un_last_state;
		New_state(un, SR_STATE_OPENING);
		if (sr_medium_removal(dev, SR_REMOVAL_PREVENT) != 0) {
			un->un_state = un->un_last_state;
			un->un_last_state = state;
			return (ENXIO);
		}

		un->un_state = un->un_last_state;
		un->un_last_state = state;
	}
	New_state(un, SR_STATE_OPEN);

#ifndef FIVETWELVE
	/*
	 * to be safe, set the block size to 2048 here
	 */
	(void)sr_two_k(devp);
#endif	FIVETWELVE

	/*
	 * set up open and exclusive open flags
	 */
	sr_setup_openflags(devp, un, major(dev));
	if (flag & FEXCL) {
		un->un_exclopen = 1;
	}

	return (0);
}

/*
 * sr_setup_openflags: setup un->un_open0, un->un_open1
 *
 * Look, this is a *hack* to make the driver to distinguish
 * between the close() call on the character device and the
 * the close() call on the block device. We have to assume
 * that the major numbers of the block device and character
 * device are DIFFERENT. This is the only way to let the
 * driver to unlock the drive and set the state to CLOSED
 * on the very last close() call.
 */
static void
sr_setup_openflags(devp, un, major_num)
register struct	scsi_device *devp;
register struct	scsi_disk *un;
int	major_num;
{
	/*
	 * set up open flag and exclusive open flag if necessary
	 */
	if ((un->un_open0 == 0) &&
	    (un->un_open1 == 0)) {
		un->un_open0 = major_num;
	} else if ((un->un_open0 != major_num) &&
		    (un->un_open1 != major_num)) {
		if (un->un_open0 == 0) {
			un->un_open0 = major_num;
		} else if (un->un_open1 == 0) {
			un->un_open1 = major_num;
		} else {
			printf("%s%d: sropen: open flags corrupted\n",
				DNAME, DUNIT);
		}
	}
}


int
srclose(dev)
dev_t dev;
{
	register struct scsi_device *devp;
	register struct scsi_disk *un;
	int unit;

	DPRINTF("sr: last close on dev %d.%d\n", major(dev), minor(dev));

	if ((unit = SRUNIT(dev)) >= nsr) {
		return (ENXIO);
	} else if (!(devp = srunits[unit]) || !devp->sd_present) {
		return (ENXIO);
	} else if (!(un = UPTR) || un->un_state < SR_STATE_OPEN) {
		return (ENXIO);
	}

	if (un->un_open0 == major(dev)) {
		un->un_open0 = 0;
	} else if (un->un_open1 == major(dev)) {
		un->un_open1 = 0;
	} else {
		printf("%s%d: srclose: open flags corrupted\n",
			DNAME, DUNIT);
	}

	if (un->un_exclopen) {
		un->un_exclopen = 0;
	}

	DPRINTF("un->un_state is %d\n", un->un_state);
	if ((un->un_open0 == 0) && (un->un_open1 == 0)) {
		if (un->un_state == SR_STATE_DETACHING) {
			sr_offline(devp);
		} else if (un->un_state != SR_STATE_EJECTED) {
			if (sr_medium_removal(dev, SR_REMOVAL_ALLOW) != 0) {
				return (ENXIO);
			}
#ifdef OPENPROMS
			(void) remdk((int) un->un_dkn);
#endif OPENPROMS
		}
		New_state(un, SR_STATE_CLOSED);
		/*
		 * here is where we would invalidate the whole
		 * schmeer, so that the next open would go out
		 * and make sure people hadn't switched things
		 * out from under us. I haven't put it in yet
		 * because I need to fiddle with the attach
		 * routine to not bother to print a label in
		 * cases where the disk isn't actually changed
		 */
	}
	return (0);
}

int
srsize(dev)
dev_t dev;
{
	struct scsi_device *devp;
	int unit;

	DPRINTF("srsize:\n");
	if ((unit = SRUNIT(dev)) >= nsr) {
		return (-1);
	} else if (!(devp = srunits[unit]) || !devp->sd_present) {
		return (-1);
	}
	return (UPTR->un_capacity);
}


/*
 * These routines perform raw i/o operations.
 */

srread(dev, uio)
dev_t dev;
struct uio *uio;
{
	DPRINTF("in srread:\n");
	return (srrw(dev, uio, B_READ));
}

static void
srmin(bp)
struct buf *bp;
{
	if (bp->b_bcount > maxphys)
		bp->b_bcount = maxphys;
}

static int
srrw(dev, uio, flag)
dev_t dev;
struct uio *uio;
int flag;
{
	struct scsi_device *devp;
	register int unit;


	DPRINTF("srrw\n");

	if ((unit = SRUNIT(dev)) >= nsr) {
		return (ENXIO);
	} else if (!(devp = srunits[unit]) || !devp->sd_present) {
		return (ENXIO);
	} else if ((uio->uio_fmode & FSETBLK) == 0 &&
	    (uio->uio_offset & (SECSIZE - 1)) != 0) {
		DPRINTF("srrw:  file offset not modulo %d\n", SECSIZE);
		return (EINVAL);
	} else if (uio->uio_iov->iov_len & (SECSIZE - 1)) {
		DPRINTF("srrw:  block length not modulo %d\n", SECSIZE);
		return (EINVAL);
	}
	return (physio(srstrategy, UPTR->un_rbufp, dev, flag, srmin, uio));
}



/*
 *
 * strategy routine
 *
 */

int
srstrategy(bp)
register struct buf *bp;
{
	register struct scsi_device *devp;
	register struct scsi_disk *un;
	register struct diskhd *dp;
	register s;

	DPRINTF("srstrategy\n");

	devp = srunits[SRUNIT(bp->b_dev)];
	if (!devp || !devp->sd_present || !(un = UPTR)) {
nothere:
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		iodone(bp);
		return;
	}

	if ((bp != un->un_sbufp) && (bp != un->un_rbufp) &&
	    (un->un_state == SR_STATE_MODE2)) {
		goto nothere;
	}

	if (bp != un->un_sbufp && un->un_state == SR_STATE_DETACHING) {
		if (sr_unit_ready(bp->b_dev) == 0) {
			goto nothere;
		}
		New_state(un, SR_STATE_OPEN);
		printf("%s%d: disk okay\n", DNAME, DUNIT);
		/*
		 * disk now back on line...
		 */
	}

	bp->b_flags &= ~(B_DONE|B_ERROR);
	bp->b_resid = 0;
	bp->av_forw = 0;

	DPRINTF("bp->b_bcount is %d\n", bp->b_bcount);

	dp = &un->un_utab;
	if (bp != un->un_sbufp) {
		/*
		 * this is regular read, therefore, it needs to be in
		 * mode one.
		 */
/*
		s = splr(srpri);
		while (un->un_state == SR_STATE_MODE2) {
			(void) sleep((caddr_t)un, PRIBIO);
		}
		(void) splx(s);
*/

#ifdef FIVETWELVE
		DPRINTF("dkblock(bp) is %d\n", dkblock(bp));
		if (dkblock(bp) > (un->un_capacity - 1)) {
#else
		DPRINTF("dkblock(bp) >> 2 is %d\n", dkblock(bp)>>2);
		if ((dkblock(bp) >> 2) > (un->un_capacity - 1)) {
#endif FIVETWELVE
DPRINTF("%s%d: Requested Block number is greater than disc's capacity\n",
	DNAME, DUNIT);
			bp->b_flags |= B_ERROR;
			bp->b_resid = bp->b_bcount;
			iodone(bp);
		}
		if (bp->b_bcount & (SECSIZE - 1)) {
			bp->b_resid = bp->b_bcount;
			bp->b_flags |= B_ERROR;
		} else {
			/*
			 * sort by absolute block number
			 */
			bp->b_resid = dkblock(bp);
			BP_PKT(bp) = 0;
		}
	} else {
		DPRINTF_IOCTL("srstrategy: special\n");
	}

	if ((s = (bp->b_flags & (B_DONE|B_ERROR))) == 0) {
		s = splr(srpri);
		/*
		 * We are doing it a bit non-standard. That is, the
		 * head of the b_actf chain is *not* the active command-
		 * it is just the head of the wait queue. The reason
		 * we do this is that the head of the b_actf chain is
		 * guaranteed to not be moved by disksort(), so that
		 * our actual current active command (pointed to by
		 * b_forw) and the head of the wait queue (b_actf) can
		 * have resources granted without it getting lost in
		 * the queue at some later point (where we would have
		 * to go and look for it).
		 */
		disksort(dp, bp);
#ifdef	CHECK_OVERLAP
		check_overlap(devp);
#endif
		if (dp->b_forw == NULL) { /* this device inactive? */
			srstart(devp);
		} else if (BP_PKT(dp->b_actf) == 0) {
			/*
			 * try and map this one
			 */
			make_sr_cmd(devp, dp->b_actf, NULL_FUNC);
		}
		(void) splx(s);
	} else if ((s & B_DONE) == 0) {
		iodone(bp);
	}
}
#ifdef	CHECK_OVERLAP
check_overlap(devp)
struct scsi_device *devp;
{
	struct scsi_disk *un = UPTR;
	register struct buf *bp;
}
#endif

/*
 * This routine implements the ioctl calls.  It is called
 * from the device switch at normal priority.
 */
/*
 * XX- what is 'flag' used for?
 */
/*ARGSUSED3*/
srioctl(dev, cmd, data, flag)
dev_t dev;
int cmd;
caddr_t data;
int flag;
{
	extern char *strcpy();
	register struct scsi_device *devp;
	register struct scsi_disk *un;
	struct dk_info *info;
	struct dk_conf *conf;
	struct dk_diag *diag;
	int unit, i;

	if ((unit = SRUNIT(dev)) >= nsr) {
		return (ENXIO);
	} else if (!(devp = srunits[unit]) || !devp->sd_present ||!(un=UPTR)) {
		return (ENXIO);
	} else if (un->un_state == SR_STATE_DETACHING) {
		return (ENXIO);
	}

	switch (cmd) {

	/*
	 * Return info concerning the controller.
	 */
	case DKIOCINFO:
		DPRINTF_IOCTL("srioctl:  get info\n");
		info = (struct dk_info *)data;
#ifdef	OPENPROMS
		info->dki_ctlr =
			(int) devp->sd_dev->devi_parent->devi_reg->reg_addr;
		info->dki_unit = (Tgt(devp)<<3)|Lun(devp);
#else
		info->dki_ctlr = getdevaddr(devp->sd_dev->md_mc->mc_addr);
		info->dki_unit = devp->sd_dev->md_slave;
#endif
		switch (un->un_dp->ctype) {
		case CTYPE_MD21:
			info->dki_ctype = DKC_MD21;
			break;
		case CTYPE_ACB4000:
			info->dki_ctype = DKC_ACB4000;
			break;
		default:
			info->dki_ctype = DKC_SCSI_CCS;
			break;
		}
		info->dki_flags = DKI_FMTVOL;
		return (0);

	/*
	 * Return the geometry of the specified unit.
	 */
	case DKIOCGGEOM:
		DPRINTF_IOCTL("srioctl:  get geometry\n");
		*(struct dk_geom *)data = un->un_g;
		return (0);

	/*
	 * Set the geometry of the specified unit.
	 */
	case DKIOCSGEOM:
		DPRINTF_IOCTL("srioctl:  set geometry\n");
		un->un_g = *(struct dk_geom *)data;
		return (0);

	/*
	 * Return the map for the specified logical partition.
	 * This has been made obsolete by the get all partitions
	 * command.
	 */
	case DKIOCGPART:
		DPRINTF_IOCTL("srioctl:  get partitions\n");
		bzero((caddr_t)data, sizeof (struct dk_map));
		return (0);

	/*
	 * Return configuration info
	 */
	case DKIOCGCONF:
		DPRINTF_IOCTL("srioctl:  get configuration info\n");
		conf = (struct dk_conf *)data;
		switch (un->un_dp->ctype) {
		case CTYPE_MD21:
			conf->dkc_ctype = DKC_MD21;
			break;
		case CTYPE_ACB4000:
			conf->dkc_ctype = DKC_ACB4000;
			break;
		default:
			conf->dkc_ctype = DKC_SCSI_CCS;
			break;
		}
		conf->dkc_dname[0] = 's';
		conf->dkc_dname[1] = 'r';
		conf->dkc_dname[2] = 0;
		conf->dkc_flags = DKI_FMTVOL;
#ifndef	OPENPROMS
		conf->dkc_cnum = devp->sd_dev->md_mc->mc_ctlr;
		conf->dkc_addr = getdevaddr(devp->sd_dev->md_mc->mc_addr);
		conf->dkc_space = devp->sd_dev->md_mc->mc_space;
		conf->dkc_prio = devp->sd_dev->md_mc->mc_intpri;
		if (devp->sd_dev->md_mc->mc_intr)
			conf->dkc_vec = devp->sd_dev->md_mc->mc_intr->v_vec;
		else
			conf->dkc_vec = 0;
		(void) strncpy(conf->dkc_cname,
				devp->sd_dev->md_driver->mdr_cname, DK_DEVLEN);
		conf->dkc_unit = devp->sd_dev->md_unit;
		conf->dkc_slave = devp->sd_dev->md_slave;
#else	OPENPROMS
		conf->dkc_cnum = devp->sd_dev->devi_parent->devi_unit;
		conf->dkc_addr = (int)
			devp->sd_dev->devi_parent->devi_reg->reg_addr;
		conf->dkc_space =
			devp->sd_dev->devi_parent->devi_reg->reg_bustype;
		conf->dkc_prio =
		ipltospl(devp->sd_dev->devi_parent->devi_intr->int_pri);
		(void) strcpy(conf->dkc_cname, CNAME);
		conf->dkc_cname[3] = CUNIT;
		conf->dkc_unit = unit;
		conf->dkc_slave = (Tgt(devp)<<3)|Lun(devp);
#endif	OPENPROMS
		return (0);

	/*
	 * Return the map for all logical partitions.
	 */
	case DKIOCGAPART:
		DPRINTF_IOCTL("srioctl:  get all logical partitions\n");
		for (i = 0; i < NDKMAP; i++)
			((struct dk_map *)data)[i] = un->un_map[i];
		return (0);

	/*
	 * Get error status from last command.
	 */
	case DKIOCGDIAG:
		DPRINTF_IOCTL("srioctl:  get error status\n");
		diag = (struct dk_diag *) data;
		diag->dkd_errcmd  = un->un_last_cmd;
		diag->dkd_errsect = un->un_err_blkno;
		diag->dkd_errno = un->un_status;
		diag->dkd_severe = un->un_err_severe;

		un->un_last_cmd   = 0;	/* Reset */
		un->un_err_blkno  = 0;
		un->un_err_code   = 0;
		un->un_err_severe = 0;
		return (0);


	case CDROMPAUSE:
		return (sr_pause_resume(dev, (caddr_t)1));

	case CDROMRESUME:
		return (sr_pause_resume(dev, (caddr_t)0));

	case CDROMPLAYMSF:
		return (sr_play_msf(dev, data));

	case CDROMPLAYTRKIND:
		return (sr_play_trkind(dev, data));

	case CDROMREADTOCHDR:
		return (sr_read_tochdr(dev, data));

	case CDROMREADTOCENTRY:
		return (sr_read_tocentry(dev, data));

	case CDROMSTOP:
		return (sr_start_stop(dev, (caddr_t)0));

	case CDROMSTART:
		return (sr_start_stop(dev, (caddr_t)1));

	case FDKEJECT:	/* for eject command */
	case CDROMEJECT:
		return (sr_eject(dev));

	case CDROMVOLCTRL:
		return (sr_volume_ctrl(dev, data));

	case CDROMSUBCHNL:
		return (sr_read_subchannel(dev, data));

	case CDROMREADMODE2:
		return (sr_read_mode2(dev, data));

	case CDROMREADMODE1:
		return (sr_read_mode1(dev, data));

	/*
	 * Run a generic scsi command (CDROM)
	 * user-scsi command
	 */
	case USCSICMD:
		return (srioctl_cmd(dev, data, 0));

	/*
	 * Handle unknown ioctls here.
	 */
	default:
		DPRINTF_IOCTL("srioctl:  unknown ioctl %x\n", cmd);
		return (ENOTTY);
	}
}

/*^L*/
/*
 *
 *	Local Functions
 *
 */

/*
 * Run a command for srioctl.
 */

static int
srioctl_cmd(dev, data, addr_flag)
dev_t dev;
caddr_t data;
int	addr_flag;
{
	register struct scsi_device *devp;
	register struct buf *bp, *rbp;
	register struct scsi_disk *un;
	int err = 0, flag, s;
	int err1;
	struct uscsi_cmd	*scmd;
	char	cmdblk[12], *cdb = cmdblk;
	faultcode_t fault_err = -1;
	int	blkno = 0;

	/*
	 * Checking for sanity done in srioctl
	 */

	devp = srunits[SRUNIT(dev)];
	un = UPTR;
	scmd = (struct uscsi_cmd *)data;

	if (addr_flag & SR_USCSI_CDB_KERNEL) {
		cdb = scmd->uscsi_cdb;
	} else {
		if (copyin(scmd->uscsi_cdb, (caddr_t)cdb,
			(u_int)scmd->uscsi_cdblen)) {
			return (EINVAL);
		}
	}

	if (DEBUGGING) {
		int	i;

		printf("%s%d: cdb:", DNAME, DUNIT);
		for (i=0; i!=scmd->uscsi_cdblen; i++) {
			printf(" 0x%x", (u_char)cdb[i]);
		}
		printf("\n");
	}

	switch (cdb_cmd(cdb)) {
	case SCMD_READ:
		blkno = cdb0_blkno(cdb);
/*		if (addr_flag & SR_MODE2) {
			if (scmd->uscsi_buflen & (CDROM_MODE2_SIZE - 1)) {
				return (EINVAL);
			}
		} else {
*/
		if (!(addr_flag & SR_MODE2)) {
			if (scmd->uscsi_buflen & (SECSIZE-1)) {
				return (EINVAL);
			}
		}
		/* FALLTHRU */
	default:
		flag = (scmd->uscsi_flags & USCSI_READ) ? B_READ : B_WRITE;
		break;
	}

	if (DEBUGGING_ALL) {
		printf("srioctl_cmd:  cmd= %x  blk= %x buflen= 0x%x\n",
			cdb_cmd(cdb), blkno, scmd->uscsi_buflen);
		if (flag == B_WRITE && scmd->uscsi_buflen) {
			auto u_int i, amt = min(64, (u_int)scmd->uscsi_buflen);
			char bufarray[64], *buf=bufarray;

			if (addr_flag & SR_USCSI_BUF_KERNEL) {
				buf = scmd->uscsi_bufaddr;
			} else {
				if (copyin(scmd->uscsi_bufaddr, (caddr_t)buf,
					amt)) {
					return (EFAULT);
				}
			}
			printf("user's buf:");
			for (i = 0; i < amt; i++) {
				printf(" 0x%x", buf[i]&0xff);
			}
			printf("\n");
		}
	}


	/*
	 * Get buffer resources...
	 */

	bp = UPTR->un_sbufp;
	rbp = UPTR->un_rbufp;
	s = splr(srpri);
	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t) bp, PRIBIO);
	}
	bzero((caddr_t) bp, sizeof (struct buf));
	bp->b_flags = B_BUSY | flag;
	(void) splx(s);

	/*
	 * check to see if the command is mode-2 read. If it is,
	 * the driver has to switch the drive to mode 2, fire
	 * the command and switch the drive back to mode 1.
	 * The bp->b_flags should be set to busy all the time
	 * so that other I/O request will not be serviced.
	 */
	if (addr_flag & SR_MODE2) {
		s = splr(srpri);
		while (rbp->b_flags & B_BUSY) {
			rbp->b_flags |= B_WANTED;
			(void) sleep((caddr_t) rbp, PRIBIO);
		}
		bzero((caddr_t) rbp, sizeof (struct buf));
		rbp->b_flags |= B_BUSY;
		(void) splx(s);

		New_state(un, SR_STATE_MODE2);

		err = sr_mode_2(devp);
		if (err) {
			DPRINTF("srioctl_cmd: error in switching to mode-2\n");
			New_state(un, SR_STATE_OPEN);
			goto done;
		}
	}

	/*
	 * Set options.
	 */
	UPTR->un_soptions = 0;
	if ((scmd->uscsi_flags & DK_SILENT) && !(DEBUGGING_ALL)) {
		UPTR->un_soptions |= DK_SILENT;
	}
	if (scmd->uscsi_flags & DK_ISOLATE)
		UPTR->un_soptions |= DK_ISOLATE;
	if (scmd->uscsi_flags & DK_DIAGNOSE)
		UPTR->un_soptions |= DK_DIAGNOSE;

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

	bp->b_forw = (struct buf *)cdb;
	bp->b_dev = dev;
	if ((bp->b_bcount = scmd->uscsi_buflen) > 0)
		bp->b_un.b_addr = scmd->uscsi_bufaddr;
	bp->b_blkno = blkno;
	if ((scmd->uscsi_buflen) && !(addr_flag & SR_USCSI_BUF_KERNEL)) {
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

		make_sr_cmd(devp, bp, SLEEP_FUNC);

		srstrategy(bp);

		s = splr(srpri);
		while ((bp->b_flags & B_DONE) == 0) {
			(void) sleep((caddr_t) bp, PRIBIO);
		}
		(void) splx(s);

		/*
		 * get the status block
		 */
		scmd->uscsi_status = (int)SCBP_C(BP_PKT(bp));
		DPRINTF("%s%d: status is %d\n", DNAME, DUNIT,
			scmd->uscsi_status);

		/*
		 * Release the resources.
		 *
		 */
		err = geterror(bp);
		if (fault_err == 0) {
			(void) as_fault(u.u_procp->p_as, bp->b_un.b_addr,
					(u_int)bp->b_bcount, F_SOFTUNLOCK,
					(bp->b_flags&B_READ)? S_WRITE: S_READ);
			u.u_procp->p_flag &= ~SPHYSIO;
			bp->b_flags &= ~B_PHYS;
		}
		scsi_resfree(BP_PKT(bp));
	}

	/*
	 * see if the request was for mode-2 read.
	 * If it was, the driver has to turn the read-mode back
	 * to mode-1.
	 */
	if (addr_flag & SR_MODE2) {
		err1 = sr_mode_1(devp);
		New_state(un, SR_STATE_OPEN);
		if (err1) {
			DPRINTF("srioctl_cmd: error in switching to mode-1\n");
			goto done;
		}
	}

	done:
	s = splr(srpri);
	if (bp->b_flags & B_WANTED) {
		wakeup((caddr_t)bp);
	}
	UPTR->un_soptions = 0;
	if (addr_flag & SR_MODE2) {
		if (rbp->b_flags & B_WANTED) {
			wakeup((caddr_t)rbp);
		}
		rbp->b_flags &= ~(B_BUSY|B_WANTED);
	}
	bp->b_flags &= ~(B_BUSY|B_WANTED);
	(void) splx(s);
	DPRINTF_IOCTL("returning %d from ioctl\n", err);
	return (err);
}

static void
sr_offline(devp)
register struct scsi_device *devp;
{
	register struct scsi_disk *un = UPTR;

	printf("%s%d: offline or disc ejected\n", DNAME, DUNIT);
#ifdef	OPENPROMS
	(void) remdk((int) un->un_dkn);
#endif	OPENPROMS
	free_pktiopb(un->un_rqs, (caddr_t)devp->sd_sense, SENSE_LENGTH);
	(void) kmem_free((caddr_t) un->un_rbufp,
		(unsigned) (sizeof (struct buf)));
	(void) kmem_free((caddr_t) un->un_sbufp,
		(unsigned) (sizeof (struct buf)));
	(void) kmem_free((caddr_t) un,
		(unsigned) (sizeof (struct scsi_disk)));
	devp->sd_private = (opaque_t) 0;
	IOPBFREE (devp->sd_inq, sizeof (struct scsi_inquiry));
	devp->sd_inq = 0;
	devp->sd_sense = 0;
	devp->sd_present = 0;
}

static void
sr_not_ready(devp)
register struct scsi_device *devp;
{
	register u_char	*sb;

	sb = (u_char *)devp->sd_sense;

	switch (sb[12]) {
	case 0x04:
		printf("%s%d: logical unit not ready\n", DNAME, DUNIT);
		break;
	case 0x05:
		printf("%s%d: unit does not respond to selection\n",
		DNAME, DUNIT);
		break;
	case 0x3a:
		DPRINTF("%s%d: Caddy not inserted in drive\n",
		DNAME, DUNIT);
		sr_ejected(devp);
		break;
	default:
		printf("%s%d: Unit not Ready. Additional sense code %d\n",
		DNAME, DUNIT, sb[12]);
		break;
	}

}

static void
sr_ejected(devp)
register struct scsi_device *devp;
{
	register struct scsi_disk *un = UPTR;

#ifdef	OPENPROMS
	(void) remdk((int) un->un_dkn);
#endif	OPENPROMS
	un->un_capacity = -1;
	un->un_lbasize = -1;
}

#ifdef OLDCODE
/*
 * This routine checks and see if a new disc has been inserted.
 * It performs all the necessary actions.
 * It assumes the driver is in the SR_STATE_EJECTED driver state.
 */
static int
sr_read_new_disc(dev)
dev_t	dev;
{
	register struct scsi_device *devp;
	register struct scsi_disk *un;
	u_char	state;

	devp = srunits[SRUNIT(dev)];
	un = UPTR;

	New_state(un, SR_STATE_OPENING);
	if (sr_unit_ready(dev) == 0) {
		sr_not_ready(devp);
		return (0);
	}
	state = un->un_last_state;
	New_state(un, SR_STATE_OPENING);
	if (sr_read_capacity(dev) != 0) {
		un->un_state = un->un_last_state;
		un->un_last_state = state;
		return (0);
	}
	un->un_state = un->un_last_state;
	un->un_last_state = state;

	/* now lock the drive door */
	state = un->un_last_state;
	New_state(un, SR_STATE_OPENING);
	if (sr_medium_removal(dev, SR_REMOVAL_PREVENT) != 0) {
		un->un_state = un->un_last_state;
		un->un_last_state = state;
		return (0);
	}
	un->un_state = un->un_last_state;
	un->un_last_state = state;
	New_state(un, SR_STATE_OPEN);
	printf("%s%d: disc inserted in drive\n", DNAME, DUNIT);

#ifdef	OPENPROMS
	sr_set_dkn(devp);
#endif	OPENPROMS
	return (1);
}
#endif OLDCODE

/*
 * This routine called to see whether unit is (still) there. Must not
 * be called when un->un_sbufp is in use, and must not be called with
 * an unattached disk. Soft state of disk is restored to what it was
 * upon entry- up to caller to set the correct state.
 */
static int
sr_unit_ready(dev)
dev_t dev;
{
	struct scsi_device *devp = srunits[SRUNIT(dev)];
	struct scsi_disk *un = UPTR;
	auto struct uscsi_cmd cblk, *com = &cblk;
	u_char state = un->un_last_state;
	char	cdb[6];

	New_state(un, SR_STATE_OPENING);
	com->uscsi_bufaddr = 0;
	com->uscsi_buflen = 0;
	cdb[0] = SCMD_TEST_UNIT_READY;
	cdb[1] = 0;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = 0;
	cdb[5] = 0;
	com->uscsi_flags = DK_DIAGNOSE|DK_SILENT;
	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 6;

	/*
	 * try it twice...
	 */
	if (srioctl_cmd(dev, (caddr_t)com, SR_USCSI_CDB_KERNEL) &&
	    srioctl_cmd(dev, (caddr_t)com, SR_USCSI_CDB_KERNEL)) {
		un->un_state = un->un_last_state;
		un->un_last_state = state;
		return (0);
	}
	un->un_state = un->un_last_state;
	un->un_last_state = state;
	return (1);
}
/*
 * makecom_all
 * note: for now, only bit 7 and bit 6 of the control byte in the cdb is
 *	 assigned. Those are the two vendor-unique control bits.
 */
static void
makecom_all(pkt, devp, flag, group, cdb)
struct 	scsi_pkt	*pkt;
struct	scsi_device	*devp;
int	flag;
int	group;
char	*cdb;
{
	register union	scsi_cdb	*scsi_cdb;

	pkt->pkt_address = (devp)->sd_address;
	pkt->pkt_flags = flag;
	scsi_cdb = (union scsi_cdb *)pkt->pkt_cdbp;
	scsi_cdb->scc_cmd = cdb_cmd(cdb);
	scsi_cdb->scc_lun = pkt->pkt_address.a_lun;
	switch (group) {
	case 0:
		scsi_cdb->g0_addr2 = cdb_tag(cdb);
		scsi_cdb->g0_addr1 = (u_char)cdb[2];
		scsi_cdb->g0_addr0 = (u_char)cdb[3];
		scsi_cdb->g0_count0 = (u_char)cdb[4];
		scsi_cdb->g0_vu_1 = cdb[5] & 0x80;
		scsi_cdb->g0_vu_0 = cdb[5] & 0x40;
		break;
	case 1:
	case 2:
	case 6: /*
		 *edu: only for now. eventually, we will not
		 *support group 6.
		 */
		scsi_cdb->g1_reladdr = cdb_tag(cdb);
		scsi_cdb->g1_addr3 = (u_char)cdb[2];
		scsi_cdb->g1_addr2 = (u_char)cdb[3];
		scsi_cdb->g1_addr1 = (u_char)cdb[4];
		scsi_cdb->g1_addr0 = (u_char)cdb[5];
		scsi_cdb->g1_rsvd0 = (u_char)cdb[6];
		scsi_cdb->g1_count1 = (u_char)cdb[7];
		scsi_cdb->g1_count0 = (u_char)cdb[8];
		scsi_cdb->g1_vu_1 = cdb[9] & 0x80;
		scsi_cdb->g1_vu_0 = cdb[9] & 0x40;
		break;
	default:
		DPRINTF("makecom_all: does not support group %d\n",
			group);
	}
}

/*
 * This routine locks the cdrom door and prevent medium removal.
 */

static int
sr_medium_removal(dev, flag)
dev_t dev;
int flag;
{
	auto struct uscsi_cmd cblk, *com = &cblk;
	char	cdb[6];

	com->uscsi_bufaddr = 0;
	com->uscsi_buflen = 0;
	cdb[0] = SCMD_DOORLOCK;
	cdb[1] = 0;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = flag;
	cdb[5] = 0;
	com->uscsi_flags = DK_DIAGNOSE|DK_SILENT;
	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 6;

	return (srioctl_cmd(dev, (caddr_t)com, SR_USCSI_CDB_KERNEL));
}

/*
 * This routine does a pause or resume to the cdrom player. Only affect
 * audio play operation.
 */
static int
sr_pause_resume(dev, data)
dev_t	dev;
caddr_t	data;
{
	int	flag;
	struct uscsi_cmd cblk, *com=&cblk;
	char	cdb[10];

	flag = (int)data;
	bzero((caddr_t)cdb, 10);
	cdb[0] = SCMD_PAUSE_RESUME;
	cdb[8] = (flag == 0) ? 1 : 0;
	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 10;
	com->uscsi_bufaddr = 0;
	com->uscsi_buflen = 0;
	com->uscsi_flags = DK_DIAGNOSE|DK_SILENT;

	return (srioctl_cmd(dev, (caddr_t)com, SR_USCSI_CDB_KERNEL));
}

/*
 * This routine plays audio by msf
 */
static int
sr_play_msf(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct uscsi_cmd cblk, *com=&cblk;
	char	cdb[10];
	register struct cdrom_msf	*msf;

	msf = (struct cdrom_msf *)data;
	bzero((caddr_t)cdb, 10);
	cdb[0] = SCMD_PLAYAUDIO_MSF;
	cdb[3] = msf->cdmsf_min0;
	cdb[4] = msf->cdmsf_sec0;
	cdb[5] = msf->cdmsf_frame0;
	cdb[6] = msf->cdmsf_min1;
	cdb[7] = msf->cdmsf_sec1;
	cdb[8] = msf->cdmsf_frame1;
	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 10;
	com->uscsi_bufaddr = 0;
	com->uscsi_buflen = 0;
	com->uscsi_flags = DK_DIAGNOSE|DK_SILENT;

	return (srioctl_cmd(dev, (caddr_t)com, SR_USCSI_CDB_KERNEL));
}

/*
 * This routine plays audio by track/index
 */
static int
sr_play_trkind(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct uscsi_cmd cblk, *com=&cblk;
	char	cdb[10];
	register struct cdrom_ti	*ti;

	ti = (struct cdrom_ti *)data;
	bzero((caddr_t)cdb, 10);
	cdb[0] = SCMD_PLAYAUDIO_TI;
	cdb[4] = ti->cdti_trk0;
	cdb[5] = ti->cdti_ind0;
	cdb[7] = ti->cdti_trk1;
	cdb[8] = ti->cdti_ind1;

	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 10;
	com->uscsi_bufaddr = 0;
	com->uscsi_buflen = 0;
	com->uscsi_flags = DK_DIAGNOSE|DK_SILENT;

	return (srioctl_cmd(dev, (caddr_t)com, SR_USCSI_CDB_KERNEL));
}

/*
 * This routine starts the drive, stops the drive or ejects the disc
 */
static int
sr_start_stop(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct uscsi_cmd cblk, *com=&cblk;
	char	cdb[6];

	bzero((caddr_t)cdb, 6);
	cdb[0] = SCMD_START_STOP;
	cdb[1] = 0; 	/* immediate bit is set to 0 for now */
	cdb[4] = (u_char)data;

	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 6;
	com->uscsi_bufaddr = 0;
	com->uscsi_buflen = 0;
	com->uscsi_flags = DK_DIAGNOSE|DK_SILENT;

	return (srioctl_cmd(dev, (caddr_t)com, SR_USCSI_CDB_KERNEL));
}

/*
 * This routine ejects the CDROM disc
 */
static int
sr_eject(dev)
dev_t	dev;
{
	int	err;
	register struct scsi_device 	*devp;

	devp = srunits[SRUNIT(dev)];

	/* first, unlocks the eject */
	if ((err = sr_medium_removal(dev, SR_REMOVAL_ALLOW)) != 0) {
		return (err);
	}

	/* then ejects the disc */
	if ((err = (sr_start_stop(dev, (caddr_t)2))) == 0) {
		DPRINTF("%s%d: Caddy ejected\n", DNAME, DUNIT);
		sr_ejected(devp);
	}
	return (err);
}

#ifdef FIXEDFIRMWARE
/*
 * This routine control the audio output volume
 */
static int
sr_volume_ctrl(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct	uscsi_cmd cblk, *com=&cblk;
	char	cdb[6];
	struct cdrom_volctrl	*vol;
	caddr_t	buffer;
	int	rtn;

	DPRINTF("in sr_volume_ctrl\n");

	if ((buffer = IOPBALLOC(20)) == (caddr_t)0) {
		return (ENOMEM);
	}
	vol = (struct cdrom_volctrl *)data;
	bzero((caddr_t)cdb, 6);
	cdb[0] = SCMD_MODE_SELECT;
	cdb[4] = 20;

	/*
	 * fill in the input data. Set the output channel 0, 1 to
	 * output port 0, 1 respestively. Set output channel 2, 3 to
	 * mute. The function only adjust the output volume for channel
	 * 0 and 1.
	 */
	bzero(buffer, 20);
	buffer[4] = 0xe;
	buffer[5] = 0xe;
	buffer[6] = 0x4;	/* set the immediate bit to 1 */
	buffer[12] = 0x01;
	buffer[13] = vol->channel0;
	buffer[14] = 0x02;
	buffer[15] = vol->channel1;

	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 6;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 20;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT;

	rtn = (srioctl_cmd(dev, (caddr_t)com,
			SR_USCSI_CDB_KERNEL|SR_USCSI_BUF_KERNEL));
	IOPBFREE (buffer, 20);

	return (rtn);
}
#else
/*
 * This routine control the audio output volume
 */
static int
sr_volume_ctrl(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct	uscsi_cmd cblk, *com=&cblk;
	char	cdb[10];
	struct cdrom_volctrl	*vol;
	caddr_t	buffer;
	int	rtn;

	DPRINTF("in sr_volume_ctrl\n");

	if ((buffer = IOPBALLOC(18)) == (caddr_t)0) {
		return (ENOMEM);
	}
	vol = (struct cdrom_volctrl *)data;
	bzero(cdb, 10);
	cdb[0] = 0xc9;	/* vendor unique command */
	cdb[7] = 0;
	cdb[8] = 0x12;

	/*
	 * fill in the input data. Set the output channel 0, 1 to
	 * output port 0, 1 respestively. Set output channel 2, 3 to
	 * mute. The function only adjust the output volume for channel
	 * 0 and 1.
	 */
	bzero(buffer, 18);
	buffer[10] = 0x01;
	buffer[11] = vol->channel0;
	buffer[12] = 0x02;
	buffer[13] = vol->channel1;

	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 10;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 0x12;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT;

	rtn = (srioctl_cmd(dev, (caddr_t)com,
			SR_USCSI_CDB_KERNEL|SR_USCSI_BUF_KERNEL));
	IOPBFREE (buffer, 18);

	return (rtn);
}
#endif FIXEDFIRMWARE

#ifdef OLDCODE
/*
 * read the drive's capacity and sector size
 * This routine is not used by srioctl(). Used when a new media is
 * inserted or when the device is first opened.
 */
static int
sr_read_capacity(dev)
dev_t	dev;
{
	register struct	scsi_disk *un;
	register struct	scsi_device *devp;
	struct	uscsi_cmd cblk, *com=&cblk;
	char	cdb[10];
	struct	scsi_capacity *cap;
	int	rtn;

	DPRINTF("in sr_read_capacity\n");

	devp = srunits[SRUNIT(dev)];
	un = UPTR;

	if ((cap = (struct scsi_capacity *)IOPBALLOC(sizeof
		(struct scsi_capacity))) == (struct scsi_capacity *)0) {
		return (ENOMEM);
	}
	bzero(cdb, 10);
	cdb[0] = SCMD_READ_CAPACITY;

	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 10;
	com->uscsi_bufaddr = (caddr_t)cap;
	com->uscsi_buflen = sizeof (struct scsi_capacity);
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = (srioctl_cmd(dev, (caddr_t)com,
			SR_USCSI_CDB_KERNEL|SR_USCSI_BUF_KERNEL));
	un->un_capacity = cap->capacity+1;
	un->un_lbasize = cap->lbasize;
	un->un_g.dkg_nsect = un->un_capacity;
	DPRINTF("%s%d: capacity is %d\n", DNAME, DUNIT,
		un->un_capacity);
	IOPBFREE (cap, sizeof (struct scsi_capacity));

	return (rtn);
}
#endif OLDCODE

static int
sr_read_subchannel(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct	uscsi_cmd cblk, *com=&cblk;
	char	cdb[10];
	caddr_t	buffer;
	int	rtn;
	struct	cdrom_subchnl	*subchnl;

	DPRINTF("in sr_read_subchannel\n");

	if ((buffer = IOPBALLOC(16)) == (caddr_t)0) {
		return (ENOMEM);
	}
	subchnl = (struct cdrom_subchnl *)data;
	bzero((caddr_t)cdb, 10);
	cdb[0] = SCMD_READ_SUBCHANNEL;
	cdb[1] = (subchnl->cdsc_format & CDROM_LBA) ? 0 : 0x02;
	/*
	 * set the Q bit in byte 2 to 1.
	 */
	cdb[2] = 0x40;
	/*
	 * This byte (byte 3) specifies the return data format. Proposed
	 * by Sony. To be added to SCSI-2 Rev 10b
	 * Setting it to one tells it to return time-data format
	 */
	cdb[3] = 0x01;
	cdb[8] = 0x10;

	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 10;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 0x10;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = (srioctl_cmd(dev, (caddr_t)com,
			SR_USCSI_CDB_KERNEL|SR_USCSI_BUF_KERNEL));
	subchnl->cdsc_audiostatus = buffer[1];
	subchnl->cdsc_trk = buffer[6];
	subchnl->cdsc_ind = buffer[7];
	subchnl->cdsc_adr = buffer[5] & 0xF0;
	subchnl->cdsc_ctrl = buffer[5] & 0x0F;
	if (subchnl->cdsc_format & CDROM_LBA) {
		subchnl->cdsc_absaddr.lba = ((u_char)buffer[8] << 24) +
					((u_char)buffer[9] << 16) +
					((u_char)buffer[10] << 8) +
					((u_char)buffer[11]);
		subchnl->cdsc_reladdr.lba = ((u_char)buffer[12] << 24) +
					((u_char)buffer[13] << 16) +
					((u_char)buffer[14] << 8) +
					((u_char)buffer[15]);
	} else {
		subchnl->cdsc_absaddr.msf.minute = buffer[9];
		subchnl->cdsc_absaddr.msf.second = buffer[10];
		subchnl->cdsc_absaddr.msf.frame = buffer[11];
		subchnl->cdsc_reladdr.msf.minute = buffer[13];
		subchnl->cdsc_reladdr.msf.second = buffer[14];
		subchnl->cdsc_reladdr.msf.frame = buffer[15];
	}
	IOPBFREE (buffer, 16);

	return (rtn);
}

static int
sr_read_mode2(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct	uscsi_cmd cblk, *com=&cblk;
	u_char	cdb[6];
	int	rtn;
	struct	cdrom_read	*mode2;

	DPRINTF("in sr_read_mode2\n");
	mode2 = (struct cdrom_read *)data;

#ifdef FIVETWELVE
	mode2->cdread_lba >>= 2;
#endif FIVETWELVE

	bzero((caddr_t)cdb, 6);
	cdb[0] = SCMD_READ;
	cdb[1] = (u_char)((mode2->cdread_lba >> 16) & 0XFF);
	cdb[2] = (u_char)((mode2->cdread_lba >> 8) & 0xFF);
	cdb[3] = (u_char)(mode2->cdread_lba & 0xFF);
	cdb[4] = mode2->cdread_buflen / 2336;

	com->uscsi_cdb = (caddr_t)cdb;
	com->uscsi_cdblen = 6;
	com->uscsi_bufaddr = mode2->cdread_bufaddr;
	com->uscsi_buflen = mode2->cdread_buflen;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = (srioctl_cmd(dev, (caddr_t)com, SR_USCSI_CDB_KERNEL|SR_MODE2));
	return (rtn);
}

static int
sr_read_mode1(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct	uscsi_cmd cblk, *com=&cblk;
	u_char	cdb[6];
	int	rtn;
	struct	cdrom_read	*mode1;

	DPRINTF("in sr_read_mode1\n");

	mode1 = (struct cdrom_read *)data;
	bzero((caddr_t)cdb, 6);
	cdb[0] = SCMD_READ;
	cdb[1] = (u_char)((mode1->cdread_lba >> 16) & 0XFF);
	cdb[2] = (u_char)((mode1->cdread_lba >> 8) & 0xFF);
	cdb[3] = (u_char)(mode1->cdread_lba & 0xFF);
	cdb[4] = mode1->cdread_buflen >> 11;

	com->uscsi_cdb = (caddr_t)cdb;
	com->uscsi_cdblen = 6;
	com->uscsi_bufaddr = mode1->cdread_bufaddr;
	com->uscsi_buflen = mode1->cdread_buflen;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = (srioctl_cmd(dev, (caddr_t)com, SR_USCSI_CDB_KERNEL));
	return (rtn);
}

static int
sr_read_tochdr(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct	uscsi_cmd cblk, *com=&cblk;
	char	cdb[10];
	caddr_t	buffer;
	int	rtn;
	struct cdrom_tochdr	*hdr;

	DPRINTF("in sr_read_tochdr.\n");

	if ((buffer = IOPBALLOC(4)) == (caddr_t)0) {
		return (ENOMEM);
	}
	hdr = (struct cdrom_tochdr *)data;
	bzero((caddr_t)cdb, 10);
	cdb[0] = SCMD_READ_TOC;
	cdb[6] = 0x00;
	/*
	 * byte 7, 8 are the allocation length. In this case, it is 4
	 * bytes.
	 */
	cdb[8] = 0x04;

	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 10;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 0x04;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = (srioctl_cmd(dev, (caddr_t)com,
			SR_USCSI_CDB_KERNEL|SR_USCSI_BUF_KERNEL));
	hdr->cdth_trk0 = buffer[2];
	hdr->cdth_trk1 = buffer[3];
	IOPBFREE (buffer, 4);

	return (rtn);
}

/*
 * This routine read the toc of the disc and returns the information
 * of a particular track. The track number is specified by the ioctl
 * caller.
 */
static int
sr_read_tocentry(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct	uscsi_cmd cblk, *com=&cblk;
	char	cdb[10];
	struct cdrom_tocentry	*entry;
	caddr_t	buffer;
	int	rtn, rtn1;
	int	lba;

	DPRINTF("in sr_read_tocentry.\n");

	if ((buffer = IOPBALLOC(12)) == (caddr_t)0) {
		return (ENOMEM);
	}

	entry = (struct cdrom_tocentry *)data;
	if (!(entry->cdte_format & (CDROM_LBA | CDROM_MSF))) {
		return (EINVAL);
	}
	bzero((caddr_t)cdb, 10);
	cdb[0] = SCMD_READ_TOC;
	/* set the MSF bit of byte one */
	cdb[1] = (entry->cdte_format & CDROM_LBA) ? 0 : 2;
	cdb[6] = entry->cdte_track;
	/*
	 * byte 7, 8 are the allocation length. In this case, it is 4 + 8
	 * = 12 bytes, since we only need one entry.
	 */
	cdb[8] = 0x0C;

	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 10;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 0x0C;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = (srioctl_cmd(dev, (caddr_t)com,
			SR_USCSI_CDB_KERNEL|SR_USCSI_BUF_KERNEL));
	entry->cdte_adr = (buffer[5] & 0xF0) >> 4;
	entry->cdte_ctrl = (buffer[5] & 0x0F);
	if (entry->cdte_format & CDROM_LBA) {
		entry->cdte_addr.lba = ((u_char)buffer[8] << 24) +
					((u_char)buffer[9] << 16) +
					((u_char)buffer[10] << 8) +
					((u_char)buffer[11]);
	} else {
		entry->cdte_addr.msf.minute = buffer[9];
		entry->cdte_addr.msf.second = buffer[10];
		entry->cdte_addr.msf.frame = buffer[11];
	}

	if (rtn) {
		IOPBFREE (buffer, 12);
		return (rtn);
	}

	/*
	 * Now do a readheader to determine which data mode it is in.
	 * ...If the track is a data track
	 */
	if ((entry->cdte_ctrl & CDROM_DATA_TRACK) &&
	    (entry->cdte_track != CDROM_LEADOUT)) {
		if (entry->cdte_format & CDROM_LBA) {
			lba = entry->cdte_addr.lba;
		} else {
			lba = (((entry->cdte_addr.msf.minute * 60) +
				(entry->cdte_addr.msf.second)) * 75) +
				entry->cdte_addr.msf.frame;
		}
		bzero((caddr_t)cdb, 10);
		cdb[0] = SCMD_READ_HEADER;
		cdb[2] = (u_char)((lba >> 24) & 0xFF);
		cdb[3] = (u_char)((lba >> 16) & 0xFF);
		cdb[4] = (u_char)((lba >> 8) & 0xFF);
		cdb[5] = (u_char)(lba & 0xFF);
		cdb[7] = 0x00;
		cdb[8] = 0x08;

		com->uscsi_buflen = 0x08;

		rtn1 = (srioctl_cmd(dev, (caddr_t)com,
				    SR_USCSI_CDB_KERNEL|SR_USCSI_BUF_KERNEL));
		if (rtn1) {
			IOPBFREE (buffer, 12);
			return (rtn1);
		}
		entry->cdte_datamode = buffer[0];

	} else {
		entry->cdte_datamode = -1;
	}

	IOPBFREE (buffer, 12);
	return (rtn);
}

/*
 * This routine sets the drive to reading mode-2 data tracks
 */
static int
sr_mode_2(devp)
struct scsi_device	*devp;
{
	struct	scsi_pkt	*pkt1;
	struct	scsi_pkt	*pkt2;
	caddr_t	buffer1;
	caddr_t	buffer2;
	char	cdb[6];

	DPRINTF("%s%d: switching to reading mode-2.\n", DNAME, DUNIT);
	/*
	 * first, do a mode sense of page 1 code
	 */
	bzero((caddr_t)cdb, 6);
	cdb[0] = SCMD_MODE_SENSE;
	cdb[2] = 1;
	cdb[4] = 20;
	pkt1 = get_pktiopb(ROUTE, (caddr_t *)&buffer1, 6, 1, 20, B_READ,
			NULL_FUNC);

	if (!pkt1) {
		return (ENOMEM);
	}

	makecom_all(pkt1, devp, FLAG_NOINTR, 0, cdb);
	if (scsi_poll(pkt1) || SCBP_C(pkt1) != STATUS_GOOD ||
	    (pkt1->pkt_state & STATE_XFERRED_DATA) == 0 ||
	    (pkt1->pkt_resid != 0)) {
		DPRINTF("%s%d: MODE SENSE command failed.\n",
			DNAME, DUNIT);
		free_pktiopb(pkt1, (caddr_t)buffer1, 20);
		return (EIO);
	}

	DPRINTF("%s%d: Parameter from Mode Sense:\n", DNAME, DUNIT);
	if (DEBUGGING) {
		int	i;

		for (i=0; i!=20; i++) {
			printf("0x%x ", buffer1[i]);
		}
	}

	/*
	 * then, do a mode select to set to mode-2 read
	 */
	bzero((caddr_t)cdb, 6);
	cdb[0] = SCMD_MODE_SELECT;
	cdb[4] = 20;
	pkt2 = get_pktiopb(ROUTE, (caddr_t *)&buffer2, 6, 1, 20, B_WRITE,
			NULL_FUNC);
	if (!pkt2) {
		return (ENOMEM);
	}

	/*
	 * fill in the parameter list for mode select command
	 */
	bzero(buffer2, 20);
	buffer2[3] = 0x08;
	buffer2[10] = 0x09;
	buffer2[11] = 0x20;
	buffer2[12] = 0x01;
	buffer2[13] = 0x06;
	buffer2[14] = buffer1[14] | 0x01;
	buffer2[15] = buffer1[15];

	/*
	 * fire the command
	 */
	makecom_all(pkt2, devp, FLAG_NOINTR, 0, cdb);
	if (scsi_poll(pkt2) || SCBP_C(pkt2) != STATUS_GOOD ||
	    (pkt2->pkt_state & STATE_XFERRED_DATA) == 0 ||
	    (pkt2->pkt_resid != 0)) {
		DPRINTF("%s%d: MODE SELECT command failed.\n",
			DNAME, DUNIT);
		free_pktiopb(pkt2, (caddr_t)buffer2, 20);
		return (EIO);
	}

	free_pktiopb(pkt1, (caddr_t)buffer1, 20);
	free_pktiopb(pkt2, (caddr_t)buffer2, 20);

	return (0);
}

/*
 * This routine sets the drive to reading mode-1 data tracks
 */
static int
sr_mode_1(devp)
struct scsi_device	*devp;
{
	struct	scsi_pkt	*pkt1;
	struct	scsi_pkt	*pkt2;
	caddr_t	buffer1;
	caddr_t	buffer2;
	char	cdb[6];

	DPRINTF("%s%d: switching to reading mode-1.\n", DNAME, DUNIT);
	/*
	 * first, do a mode sense of page 1 code
	 */
	bzero(cdb, 6);
	cdb[0] = SCMD_MODE_SENSE;
	/* get default page 1 values */
	cdb[2] = 0x81;
	cdb[4] = 20;
	pkt1 = get_pktiopb(ROUTE, (caddr_t *)&buffer1, 6, 1, 20, B_READ,
			NULL_FUNC);

	if (!pkt1) {
		return (ENOMEM);
	}

	makecom_all(pkt1, devp, FLAG_NOINTR, 0, cdb);
	if (scsi_poll(pkt1) || SCBP_C(pkt1) != STATUS_GOOD ||
	    (pkt1->pkt_state & STATE_XFERRED_DATA) == 0 ||
	    (pkt1->pkt_resid != 0)) {
		DPRINTF("%s%d: MODE SENSE command failed.\n",
			DNAME, DUNIT);
		free_pktiopb(pkt1, (caddr_t)buffer1, 20);
		return (EIO);
	}

	DPRINTF("%s%d: Parameter from Mode Sense:\n", DNAME, DUNIT);
	if (DEBUGGING) {
		int	i;

		for (i=0; i!=20; i++) {
			printf("0x%x ", buffer1[i]);
		}
	}

	/*
	 * then, do a mode select to set to mode-1 read
	 */
	bzero((caddr_t)cdb, 6);
	cdb[0] = SCMD_MODE_SELECT;
	cdb[4] = 20;
	pkt2 = get_pktiopb(ROUTE, (caddr_t *)&buffer2, 6, 1, 20, B_WRITE,
			NULL_FUNC);
	if (!pkt2) {
		return (ENOMEM);
	}

	/*
	 * fill in the parameter list for mode select command
	 * values obtained from default value.
	 */
	bzero(buffer2, 20);
	buffer2[3] = 0x08;

#ifdef	FIVETWELVE
	/*
	 * block length of 512 bytes - default mode 1 size
	 */
	buffer2[10] = 0x02;
#else
	/*
	 * set block length to 2048 bytes
	 */
	buffer2[10] = 0x08;
#endif	FIVETWELVE

	buffer2[11] = 0x00;
	buffer2[12] = 0x01;
	buffer2[13] = 0x06;
	buffer2[14] = buffer1[14];
	buffer2[15] = buffer1[15];

	/*
	 * fire the command
	 */
	makecom_all(pkt2, devp, FLAG_NOINTR, 0, cdb);
	if (scsi_poll(pkt2) || SCBP_C(pkt2) != STATUS_GOOD ||
	    (pkt2->pkt_state & STATE_XFERRED_DATA) == 0 ||
	    (pkt2->pkt_resid != 0)) {
		DPRINTF("%s%d: MODE SELECT command failed.\n",
			DNAME, DUNIT);
		free_pktiopb(pkt2, (caddr_t)buffer2, 20);
		return (EIO);
	}

	free_pktiopb(pkt1, (caddr_t)buffer1, 20);
	free_pktiopb(pkt2, (caddr_t)buffer2, 20);

	return (0);
}

#ifndef FIVETWELVE
/*
 * This routine sets the drive to read 2048 bytes per sector
 */
static int
sr_two_k(devp)
struct scsi_device	*devp;
{
	struct	scsi_pkt	*pkt1;
	struct	scsi_pkt	*pkt2;
	caddr_t	buffer1;
	caddr_t	buffer2;
	char	cdb[6];

	DPRINTF("%s%d: switching to reading 2048 bytes per sector.\n",
		DNAME, DUNIT);
	/*
	 * first, do a mode sense of page 1 code
	 */
	bzero((caddr_t)cdb, 6);
	cdb[0] = SCMD_MODE_SENSE;
	/* get default page 1 values */
	cdb[2] = 0x81;
	cdb[4] = 20;
	pkt1 = get_pktiopb(ROUTE, (caddr_t *)&buffer1, 6, 1, 20, B_READ,
			NULL_FUNC);

	if (!pkt1) {
		return (ENOMEM);
	}

	makecom_all(pkt1, devp, FLAG_NOINTR, 0, cdb);
	if (scsi_poll(pkt1) || SCBP_C(pkt1) != STATUS_GOOD ||
	    (pkt1->pkt_state & STATE_XFERRED_DATA) == 0 ||
	    (pkt1->pkt_resid != 0)) {
		DPRINTF("%s%d: MODE SENSE command failed.\n",
			DNAME, DUNIT);
		free_pktiopb(pkt1, (caddr_t)buffer1, 20);
		return (EIO);
	}

	DPRINTF("%s%d: Parameter from Mode Sense:\n", DNAME, DUNIT);
	if (DEBUGGING) {
		int	i;

		for (i=0; i!=20; i++) {
			printf("0x%x ", buffer1[i]);
		}
	}

	/*
	 * then, do a mode select to set to reading 2048 bytes per sector
	 */
	bzero(cdb, 6);
	cdb[0] = SCMD_MODE_SELECT;
	cdb[4] = 20;
	pkt2 = get_pktiopb(ROUTE, (caddr_t *)&buffer2, 6, 1, 20, B_WRITE,
			NULL_FUNC);
	if (!pkt2) {
		return (ENOMEM);
	}

	/*
	 * fill in the parameter list for mode select command
	 * values obtained from default value.
	 */
	bzero(buffer2, 20);
	buffer2[3] = 0x08;

	/*
	 * block length of 2048 bytes (hex 800)
	 */
	buffer2[10] = 0x08;
	buffer2[11] = 0x00;

	buffer2[12] = 0x01;
	buffer2[13] = 0x06;
	buffer2[14] = buffer1[14];
	buffer2[15] = buffer1[15];

	/*
	 * fire the command
	 */
	makecom_all(pkt2, devp, FLAG_NOINTR, 0, cdb);
	if (scsi_poll(pkt2) || SCBP_C(pkt2) != STATUS_GOOD ||
	    (pkt2->pkt_state & STATE_XFERRED_DATA) == 0 ||
	    (pkt2->pkt_resid != 0)) {
		DPRINTF("%s%d: MODE SELECT command failed.\n",
			DNAME, DUNIT);
		free_pktiopb(pkt2, (caddr_t)buffer2, 20);
		return (EIO);
	}

	free_pktiopb(pkt1, (caddr_t)buffer1, 20);
	free_pktiopb(pkt2, (caddr_t)buffer2, 20);

	return (0);
}
#endif

/*
 * This routine locks the drive door
 */
static int
sr_lock_door(devp)
struct scsi_device	*devp;
{
	struct	scsi_pkt	*pkt;
	char	cdb[6];

	DPRINTF("%s%d: locking the drive door\n",
		DNAME, DUNIT);

	bzero(cdb, 6);
	cdb[0] = SCMD_DOORLOCK;
	cdb[4] = 0x01;
	pkt = scsi_pktalloc(ROUTE, CDB_GROUP0, 1, NULL_FUNC);

	if (!pkt) {
		return (ENOMEM);
	}

	makecom_all(pkt, devp, FLAG_NOINTR, 0, cdb);
	if (scsi_poll(pkt) || SCBP_C(pkt) != STATUS_GOOD) {
		DPRINTF("%s%d: Prevent/Allow Medium Removal command failed.\n",
			DNAME, DUNIT);
		scsi_resfree(pkt);
		return (EIO);
	}

	scsi_resfree(pkt);
	return (0);
}

/*
 * This routine read the drive's capacity and logical block size
 */
static int
sr_read_capacity(devp)
struct scsi_device	*devp;
{
	struct	scsi_pkt	*pkt;
	caddr_t	buffer;
	register struct	scsi_disk	*un;

	DPRINTF("%s%d: reading the drive's capacity\n",
		DNAME, DUNIT);

#ifdef OPENPROMS
	sr_set_dkn(devp);
#endif OPENPROMS

	un = UPTR;
	pkt = get_pktiopb(ROUTE, &buffer, CDB_GROUP1, 1,
			sizeof (struct scsi_capacity), B_READ, NULL_FUNC);

	if (!pkt) {
		return (ENOMEM);
	}

	makecom_g1(pkt, devp, 0, SCMD_READ_CAPACITY, 0, 0);

	if ((scsi_poll(pkt) == 0) && (SCBP_C(pkt) == STATUS_GOOD) &&
		(pkt->pkt_state & STATE_XFERRED_DATA) &&
		(pkt->pkt_resid == 0)) {
		un->un_capacity = ((struct scsi_capacity *)buffer)->capacity+1;
		un->un_lbasize = ((struct scsi_capacity *)buffer)->lbasize;
		free_pktiopb(pkt, buffer, sizeof (struct scsi_capacity));
		return (0);
	} else {
		free_pktiopb(pkt, buffer, sizeof (struct scsi_capacity));
		return (EIO);
	}
}


/*
 *
 *		Unit start and Completion
 *
 */

static void
srstart(devp)
register struct scsi_device *devp;
{
	register struct buf *bp;
	register struct scsi_disk *un = UPTR;
	register struct diskhd *dp = &un->un_utab;

	DPRINTF("srstart\n");

	if (dp->b_forw) {
		printf("srstart: busy already\n");
		return;
	} else if ((bp = dp->b_actf) == NULL) {
		DPRINTF("%s%d: srstart idle\n", DNAME, DUNIT);
		return;
	}

	if (!BP_PKT(bp)) {
		make_sr_cmd(devp, bp, srrunout);
		if (!BP_PKT(bp)) {
			/*
			 * XXX: actually, we should see whether or not
			 * XXX:	we could fire up a SCMD_SEEK operation
			 * XXX:	here. We may not have been able to go
			 * XXX:	because DVMA had run out, not because
			 * XXX:	we were out of command packets.
			 */
			New_state(un, SR_STATE_RWAIT);
			return;
		} else {
			New_state(un, SR_STATE_OPEN);
		}
	}

	dp->b_forw = bp;
	dp->b_actf = bp->b_actf;
	bp->b_actf = 0;

	/*
	 * try and link this command with the one that
	 * is next
	 */

	if ((scsi_options & SCSI_OPTIONS_LINK) &&
		(un->un_dp->options & SR_DOLINK) &&
		dp->b_actf && bp != un->un_sbufp &&
			CDBP(BP_PKT(bp))->g0_link == 0) {
		if (BP_PKT(dp->b_actf)) {
			CDBP(BP_PKT(bp))->g0_link = 1;
		} else {
			make_sr_cmd(devp, dp->b_actf, NULL_FUNC);
			if (BP_PKT(dp->b_actf)) {
				CDBP(BP_PKT(bp))->g0_link = 1;
			}
		}
	}
	if (pkt_transport(BP_PKT(bp)) == 0) {
		printf("%s%d: transport rejected\n", DNAME, DUNIT);
		bp->b_flags |= B_ERROR;
		srdone(devp);
	} else if (dp->b_actf && !BP_PKT(dp->b_actf)) {
		make_sr_cmd(devp, dp->b_actf, NULL_FUNC);
	}
}


static int
srrunout()
{
	register i, s = splr(srpri);
	register struct scsi_device *devp;
	register struct scsi_disk *un;

	for (i = 0; i < nsr; i++) {
		devp = srunits[i];
		if (devp && devp->sd_present && (un=UPTR) && un->un_gvalid) {
			if (un->un_state == SR_STATE_RWAIT) {
				DPRINTF("%s%d: resource retry\n", DNAME,
					DUNIT);
				srstart(devp);
				if (un->un_state == SR_STATE_RWAIT) {
					(void) splx(s);
					return (0);
				}
				DPRINTF("%s%d: resource gotten\n", DNAME,
					DUNIT);
			}
		}
	}
	(void) splx(s);
	return (1);
}

static void
srdone(devp)
register struct scsi_device *devp;
{
	register struct buf *bp;
	register struct scsi_disk *un = UPTR;
	register struct diskhd *dp = &un->un_utab;

	bp = dp->b_forw;
	dp->b_forw = NULL;
	/*
	 * Start the next one before releasing resources on this one
	 */
	if (dp->b_actf) {
		DPRINTF("%s%d: srdone calling srstart\n", DNAME, DUNIT);
		srstart(devp);
	}

	if (bp != un->un_sbufp) {
		scsi_resfree(BP_PKT(bp));
		iodone(bp);
		DPRINTF("%s%d: regular done resid %d\n",
			DNAME, DUNIT, bp->b_resid);
	} else {
		DPRINTF("%s%d: special done resid %d\n",
			DNAME, DUNIT, bp->b_resid);
		bp->b_flags |= B_DONE;
		wakeup((caddr_t) bp);
	}
}

static void
make_sr_cmd(devp, bp, func)
register struct scsi_device *devp;
register struct buf *bp;
int (*func)();
{
	register struct scsi_pkt *pkt;
	register struct scsi_disk *un = UPTR;
	register daddr_t blkno = 0;
	int tval = DEFAULT_SR_TIMEOUT, count, com, flags;
	char	*cdb;

	flags = (scsi_options & SCSI_OPTIONS_DR) ? 0: FLAG_NODISCON;

	if (bp != un->un_sbufp) {
		long blkovr;

		DPRINTF("make_sr_cmd: regular\n");

		pkt = scsi_resalloc(ROUTE, CDB_GROUP0, 1, (opaque_t)bp, func);
		if ((BP_PKT(bp) = pkt) == (struct scsi_pkt *) 0) {
			return;
		}

		/* count is number of blocks */
		count = bp->b_bcount >> SECDIV;

#ifdef FIVETWELVE
		blkno = dkblock(bp);
#else
		/* shift block number. since block size is 2048 */
		blkno = dkblock(bp) >> 2;
#endif FIVETWELVE

		DPRINTF("bp->b_bcount is %d\n", bp->b_bcount);
		DPRINTF("count is %d\n", count);
		DPRINTF("blkno is %d\n", blkno);

		/*
		 * Make sure we don't run off the end of the CDROM
		 */
		if ((blkovr = blkno + count - un->un_capacity) > 0) {
			DPRINTF("%s%d: (%d, %d) overrun by %d blocks\n",
				DNAME, DUNIT, blkno, bp->b_bcount, blkovr);
			bp->b_resid = (blkovr << SECDIV);
			count -= blkovr;
		} else {
			bp->b_resid = 0;
		}

		if (bp->b_flags & B_READ) {
			DPRINTF("%s%d: read", DNAME, DUNIT);
			com = SCMD_READ;
		} else {
			DPRINTF("%s%d: write", DNAME, DUNIT);
			com = SCMD_WRITE;
		}
		DPRINTF(" blk %d amt 0x%x (%d) resid %d\n", blkno,
			count<<SECDIV, count<<SECDIV, bp->b_resid);
		makecom_g0(pkt, devp, flags, com, (int) blkno, count);
		pkt->pkt_pmon = un->un_dkn;
	} else {
		struct buf *abp = bp;
		int group;

		/*
		 * Some commands are group 1 commands, some are group 0.
		 * It is legitimate to allocate a cdb sufficient for any.
		 * Therefore, we'll ask for a GROUP 1 cdb.
		 */

		/*
		 * stored in bp->b_forw is a pointer to cdb
		 */
		cdb = (char *)bp->b_forw;
		com  = (int)cdb_cmd(cdb);

		group = ((unsigned char)com) >> 5;
		blkno = bp->b_blkno;
		count = bp->b_bcount;

		DPRINTF("%s%d: blkno is %d, count is %d ", DNAME, DUNIT,
			blkno, count);
		switch (com) {
		case SCMD_READ:
			DPRINTF_IOCTL("special %s\n", (com==SCMD_READ)?"read":
				"write");
			count = bp->b_bcount >> SECDIV;
			break;
		default:
			DPRINTF_IOCTL("scsi command 0x%x\n", com);
			if (bp->b_bcount == 0) {
				abp = (struct buf *)0;
			}
			break;
		}
		pkt = scsi_resalloc(ROUTE, CDB_GROUP1, 1,
			(opaque_t)abp, SLEEP_FUNC);
		makecom_all(pkt, devp, flags, group, cdb);
		pkt->pkt_pmon = -1;
	}
	if (un->un_dp->options & SR_NODISC)
		pkt->pkt_flags |= FLAG_NODISCON;
	pkt->pkt_comp = srintr;
	pkt->pkt_time = tval;
	pkt->pkt_private = (opaque_t) bp;
	BP_PKT(bp) = pkt;
}


/*
 *
 *		Interrupt Service Routines
 *
 */

/*
 *
 */

static
srrestart(arg)
caddr_t arg;
{
	struct scsi_device *devp = (struct scsi_device *) arg;
	struct buf *bp;
	register s = splr(srpri);
	printf("srrestart\n");
	if ((bp = UPTR->un_utab.b_forw) == 0) {
		printf("%s%d: busy restart aborted\n", DNAME, DUNIT);
	} else {
		struct scsi_pkt *pkt;
		if (UPTR->un_state == SR_STATE_SENSING) {
			pkt = UPTR->un_rqs;
		} else {
			pkt = BP_PKT(bp);
		}
		if (pkt_transport(pkt) == 0) {
			printf("%s%d: restart transport failed\n", DNAME,
				DUNIT);
			UPTR->un_state = UPTR->un_last_state;
			bp->b_resid = bp->b_bcount;
			bp->b_flags |= B_ERROR;
			srdone(devp);
		}
	}
	(void) splx(s);
}

/*
 * Command completion processing
 *
 */
static void
srintr(pkt)
struct scsi_pkt *pkt;
{
	register struct scsi_device *devp;
	register struct scsi_disk *un;
	register struct buf *bp;
	register action;

	/*
	 * Do some sanity checking
	 */

	if (!pkt) {
		printf("sdintr: null packet?\n");
		return;
	} else if (pkt->pkt_flags & FLAG_NOINTR) {
		printf("srintr:internal error - got a non-interrupting cmd\n");
		return;
	} else if ((bp = (struct buf *) pkt->pkt_private) == 0) {
		printf("sdintr: packet with no buffer pointer\n");
		return;
	}
	if (!(devp = srunits[SRUNIT(bp->b_dev)]) || !(un = UPTR)) {
		printf("srintr: mangled data structures\n");
		return;
	} else if (bp != un->un_utab.b_forw) {
		printf("srintr: completed packet's buffer not on queue\n");
		return;
	}

	DPRINTF("srintr:\n");

	if (pkt->pkt_reason != CMD_CMPLT) {
		action = sr_handle_incomplete(devp);
	/*
	 * At this point we know that the command was successfully
	 * completed. Now what?
	 */
	} else if (un->un_state == SR_STATE_SENSING) {
		/*
		 * okay. We were running a REQUEST SENSE. Find
		 * out what to do next.
		 */
		ASSERT(pkt == un->un_rqs);
		/*
		 * restore original state
		 */
		Restore_state(un);
		pkt = BP_PKT(bp);
		action = sr_handle_sense(devp);
	/*
	 * Okay, we weren't running a REQUEST SENSE. Call a routine
	 * to see if the status bits we're okay. As a side effect,
	 * clear state for this device, set non-error b_resid values, etc.
	 * If a request sense is to be run, that will happen by.
	 * sd_check_error() returning a QUE_SENSE action.
	 */
	} else {
		action = sr_check_error(devp);
	}

	switch (action) {
	case COMMAND_DONE_ERROR:
		un->un_err_severe = DK_FATAL;
		un->un_err_resid = bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		/* FALL THRU */
	case COMMAND_DONE:
		srdone(devp);
		break;
	case QUE_SENSE:
		New_state(un, SR_STATE_SENSING);
		un->un_rqs->pkt_private = (opaque_t) bp;
		bzero((caddr_t)devp->sd_sense, SENSE_LENGTH);
		if (pkt_transport(un->un_rqs) == 0) {
			panic("srintr: transport of request sense fails");
			/*NOTREACHED*/
		}
		break;
	case QUE_COMMAND:
		if (pkt_transport(BP_PKT(bp)) == 0) {
			printf("srintr: requeue of command fails\n");
			un->un_err_resid = bp->b_resid = bp->b_bcount;
			bp->b_flags |= B_ERROR;
			srdone(devp);
		}
		break;
	case JUST_RETURN:
		break;
	}
}


static int
sr_handle_incomplete(devp)
struct scsi_device *devp;
{
	static char *fail = "%s%d:\tSCSI transport failed: reason '%s': %s\n";
	static char *notresp = "%s%d: disk not responding to selection\n";
	register rval = COMMAND_DONE_ERROR;
	struct scsi_disk *un = UPTR;
	struct buf *bp = un->un_utab.b_forw;
	struct scsi_pkt *pkt = BP_PKT(bp);

	DPRINTF("%s%d: In sr_handle_incomplete\n",
		DNAME, DUNIT);
	DPRINTF("un->un_state is %d\n", un->un_state);
	if (un->un_state == SR_STATE_SENSING) {
		pkt = un->un_rqs;
		Restore_state(un);
	}

	/*
	 * If we were running a request sense,
	 * try it again if possible
	 */
	if (pkt == un->un_rqs) {
		if (un->un_retry_ct++ < SR_RETRY_COUNT) {
			rval = QUE_SENSE;
		}
	} else if (bp == un->un_sbufp &&
		(un->un_soptions & DK_ISOLATE)) {
		rval = COMMAND_DONE_ERROR;
	} else if (un->un_retry_ct++ < SR_RETRY_COUNT) {
		rval = QUE_COMMAND;
	}

	if (pkt->pkt_state == STATE_GOT_BUS && rval == COMMAND_DONE_ERROR) {
		/*
		 * Looks like someone turned off this shoebox.
		 */
		printf(notresp, DNAME, DUNIT);
		New_state(un, SR_STATE_DETACHING);
	} else if (bp != un->un_sbufp||!(un->un_soptions&DK_SILENT)) {
		printf(fail, DNAME, DUNIT, scsi_rname(pkt->pkt_reason),
			(rval == COMMAND_DONE_ERROR)?
			"giving up":"retrying command");
	}
	return (rval);
}


static int
sr_handle_sense(devp)
register struct scsi_device *devp;
{
	register struct scsi_disk *un = UPTR;
	register struct buf *bp = un->un_utab.b_forw;
	struct scsi_pkt *pkt = BP_PKT(bp), *rqpkt = un->un_rqs;
	register rval = COMMAND_DONE_ERROR;
	int severity, amt, i;
	char *p;
	static char *hex =" 0x%x";
	int	ua_retry_count;

	if (SCBP(rqpkt)->sts_busy) {
		printf ("%s%d: busy unit on request sense\n", DNAME, DUNIT);
		if (un->un_retry_ct++ < SR_RETRY_COUNT) {
			timeout (srrestart, (caddr_t)devp, SRTIMEOUT);
			rval = JUST_RETURN;
		}
		return (rval);
	}

	if (SCBP(rqpkt)->sts_chk) {
		printf ("%s%d: Check Condition on REQUEST SENSE\n", DNAME,
			DUNIT);
		return (rval);
	}
	amt = SENSE_LENGTH - rqpkt->pkt_resid;
	if ((rqpkt->pkt_state&STATE_XFERRED_DATA) == 0 || amt == 0) {
		printf("%s%d: Request Sense couldn't get sense data\n", DNAME,
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
				DNAME, DUNIT);
			return (rval);
		}
		if (devp->sd_sense->es_class != CLASS_EXTENDED_SENSE) {
			if (un->un_dp->ctype == CTYPE_ACB4000) {
				srintr_adaptec(devp);
			} else {
				p = (char *) devp->sd_sense;
				printf("%s%d: undecoded sense information:");
				for (i = 0; i < amt; i++) {
					printf(hex, *(p++)&0xff);
				}
				printf("; assuming a fatal error\n");
				return (rval);
			}
		}
	} else if (un->un_dp->ctype == CTYPE_ACB4000) {
		srintr_adaptec(devp);
	}


	un->un_status = devp->sd_sense->es_key;
	if (un->un_dp->ctype == CTYPE_MD21)
		un->un_err_code	= devp->sd_sense->emulex_ercl_ercd & 0xf;
	else
		un->un_err_code = devp->sd_sense->es_code;

	if (devp->sd_sense->es_valid) {
		un->un_err_blkno =	(devp->sd_sense->es_info_1 << 24) |
					(devp->sd_sense->es_info_2 << 16) |
					(devp->sd_sense->es_info_3 << 8)  |
					(devp->sd_sense->es_info_4);
	} else {
		un->un_err_blkno = bp->b_blkno;
	}

	if (DEBUGGING_ALL || sr_error_reporting == SRERR_ALL) {
		p = (char *) devp->sd_sense;
		printf("%s%d:sdata:", DNAME, DUNIT);
		for (i = 0; i < amt; i++) {
			printf(hex, *(p++)&0xff);
		}
		printf("\n      cmd:");
		p = pkt->pkt_cdbp;
		for (i = 0; i < CDB_SIZE; i++)
			printf(hex, *(p++)&0xff);
		printf("\n");
	}

	switch (un->un_status) {
	case KEY_NO_SENSE:
		un->un_err_severe = DK_NOERROR;
		severity = SRERR_RETRYABLE;
		rval = QUE_COMMAND;
		break;

	case KEY_RECOVERABLE_ERROR:
		if (bp != un->un_sbufp && bp->b_resid) {
			pkt->pkt_resid -= bp->b_resid;
		}
		bp->b_resid += pkt->pkt_resid;
		un->un_err_severe = DK_CORRECTED;
		un->un_retry_ct = 0;
		severity = SRERR_RECOVERED;
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
		severity = SRERR_FATAL;
		rval = COMMAND_DONE_ERROR;
		break;

	case KEY_ABORTED_COMMAND:
		severity = SRERR_RETRYABLE;
		/* Dumping? */
		rval = QUE_COMMAND;
		break;
	case KEY_UNIT_ATTENTION:
		rval = QUE_COMMAND;
		un->un_err_severe = DK_NOERROR;
		un->un_retry_ct = 0;
		severity = SRERR_INFORMATIONAL;

		ua_retry_count = 0;
		while (sr_handle_ua(devp)) {
			if (ua_retry_count++ > SR_UNIT_ATTENTION_RETRY) {
				printf("%s%d: failed to handle Unit Att.\n",
					DNAME, DUNIT);
				severity = sr_error_reporting;
				break;
			}
			/*
			 * introduce a 0.5 second delay
			 */
			DELAY(500000);
		}
		break;
	default:
		/*
		 * Undecoded sense key.  Try retries and hope
		 * that will fix the problem.  Otherwise, we're
		 * dead.
		 */
		printf("%s%d: Sense Key '%s'\n", DNAME, DUNIT,
			sense_keys[un->un_status]);
		if (un->un_retry_ct++ < SR_RETRY_COUNT) {
			un->un_err_severe = DK_RECOVERED;
			severity = SRERR_RETRYABLE;
			rval = QUE_COMMAND;
		} else {
			un->un_err_severe = DK_FATAL;
			severity = SRERR_FATAL;
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
			srerrmsg(devp, severity);
		}
	} else if (DEBUGGING_ALL || severity >= sr_error_reporting) {
		srerrmsg(devp, severity);
	}
	return (rval);
}

static int
sr_handle_ua(devp)
register struct scsi_device *devp;
{
	register u_char *sb;
	int	err;

	DPRINTF("%s%d: in sr_handle_ua.\n", DNAME, DUNIT);

	sb = (u_char *)devp->sd_sense;

	if (sb[12] == 0x29) {	/* power on or reset sense code */
		DPRINTF("%s%d: power on or reset condition detected\n",
			DNAME, DUNIT);
		if (err = sr_lock_door(devp)) {
			return (err);
		}
#ifndef FIVETWELVE
		if (err = sr_two_k(devp)) {
			return (err);
		} else {
			printf("%s%d: power on or reset condition detected\n",
				DNAME, DUNIT);
			return (0);
		}
#else
		if (err = sr_read_capacity(devp)) {
			return (err);
		}
		return (0);
#endif FIVETWELVE
	} else if (sb[12] == 0x28) {	/* medium change */
		DPRINTF("%s%d: medium might have changed condition detected\n",
			DNAME, DUNIT);
		if (err = sr_lock_door(devp)) {
			return (err);
		} else if (err = sr_read_capacity(devp)) {
			return (err);
		} else {
			DPRINTF("%s%d: Caddy has just been inserted\n",
				DNAME, DUNIT);
			return (0);
		}
	} else {
		return (0);
	}
}

static int
sr_check_error(devp)
register struct scsi_device *devp;
{
	register struct scsi_disk *un = UPTR;
	struct buf *bp = un->un_utab.b_forw;
	register struct scsi_pkt *pkt = BP_PKT(bp);
	register action;

	if (SCBP(pkt)->sts_busy) {
		if (un->un_retry_ct++ < SR_RETRY_COUNT) {
			timeout(srrestart, (caddr_t)devp, SRTIMEOUT);
			action = JUST_RETURN;
		} else {
			printf("%s%d:\tdevice busy too long\n", DNAME, DUNIT);
			if (scsi_reset(ROUTE, RESET_TARGET)) {
				action = QUE_COMMAND;
			} else if (scsi_reset(ROUTE, RESET_ALL)) {
				action = QUE_COMMAND;
			} else {
				action = COMMAND_DONE_ERROR;
			}
		}
	} else if (SCBP(pkt)->sts_chk) {
		if (pkt == un->un_rqs) {
			printf("%s%d: check condition on a Request Sense\n",
				DNAME, DUNIT);
			panic("Check Condition on a Request Sense");
		}
		DPRINTF("%s%d: check condition\n", DNAME, DUNIT);
		action = QUE_SENSE;
	} else {
		/*
		 * pkt_resid will reflect, at this point, a residual
		 * of how many bytes left to be transferred there were.
		 * If the command had a check condition, the resid
		 * will be set in sr_handle_sense().
		 */
		if (bp != un->un_sbufp && bp->b_resid) {
			pkt->pkt_resid -= bp->b_resid;
		}
		bp->b_resid += pkt->pkt_resid;
		action = COMMAND_DONE;
		un->un_err_severe = DK_NOERROR;
		un->un_retry_ct = 0;
	}
	return (action);
}
/*
 *
 *		Error Printing
 *
 *
 */

static u_char sd_adaptec_keys[] = {
	0, 4, 4, 4,  2, 4, 4, 4,	/* 0x00-0x07 */
	4, 4, 4, 4,  4, 4, 4, 4,	/* 0x08-0x0f */
	3, 3, 3, 3,  3, 3, 3, 1,	/* 0x10-0x17 */
	1, 1, 5, 5,  1, 1, 1, 1,	/* 0x18-0x1f */
	5, 5, 5, 5,  5, 5, 5, 5,	/* 0x20-0x27 */
	6, 6, 6, 6,  6, 6, 6, 6		/* 0x28-0x30 */
};
#define	MAX_ADAPTEC_KEYS (sizeof (sd_adaptec_keys))

static char *sd_cmds[] = {
	"\000test unit ready",		/* 0x00 */
	"\001rezero",			/* 0x01 */
	"\003request sense",		/* 0x03 */
	"\004format",			/* 0x04 */
	"\007reassign",			/* 0x07 */
	"\010read",			/* 0x08 */
	"\012write",			/* 0x0a */
	"\013seek",			/* 0x0b */
	"\022inquiry",			/* 0x12 */
	"\025mode select",		/* 0x15 */
	"\026reserve",			/* 0x16 */
	"\027release",			/* 0x17 */
	"\030copy",			/* 0x18 */
	"\032mode sense",		/* 0x1a */
	"\033start/stop",		/* 0x1b */
	"\036door lock",		/* 0x1e */
	"\067read defect data",		/* 0x37 */
	NULL
};


/*
 * Translate Adaptec non-extended sense status in to
 * extended sense format.  In other words, generate
 * sense key.
 */
static void
srintr_adaptec(devp)
struct scsi_device *devp;
{
	register struct scsi_sense *s;

	DPRINTF("srintr_adaptec\n");

	/* Reposition failed block number for extended sense. */
	s = (struct scsi_sense *) devp->sd_sense;
	devp->sd_sense->es_info_1 = 0;
	devp->sd_sense->es_info_2 = s->ns_lba_hi;
	devp->sd_sense->es_info_3 = s->ns_lba_mid;
	devp->sd_sense->es_info_4 = s->ns_lba_lo;

	/* Reposition error code for extended sense. */
	devp->sd_sense->es_code = s->ns_code;

	/* Synthesize sense key for extended sense. */
	if (s->ns_code < MAX_ADAPTEC_KEYS) {
		devp->sd_sense->es_key = sd_adaptec_keys[s->ns_code];
	}
}

/*
 * print the text string associated with the secondary
 * error code, if availiable.
 */

static void
sr_error_code_print(devp)
struct scsi_device *devp;
{
	register u_char code = devp->sd_sense->es_code;
	register char **vec;

	vec = (char **)NULL;

	switch (UPTR->un_dp->ctype) {
	case CTYPE_CCS:
		break;
	}

	printf("Vendor '%s' unique error code: 0x%x", UPTR->un_dp->name, code);
	if (vec == (char **)NULL) {
		return;
	}
	while (*vec != (char *) NULL) {
		if (code == *vec[0]) {
			printf(" (%s)", (char *)((int)(*vec)+1));
			return;
		} else
			vec++;
	}
}


static void
srerrmsg(devp, level)
register struct scsi_device *devp;
int level;
{
	static char *error_classes[] = {
		"All          ", "Unknown      ", "Informational",
		"Recovered    ", "Retryable    ", "Fatal        "
	};
	char *class;
	struct scsi_pkt *pkt;
	u_char *sb;

	sb = (u_char *)devp->sd_sense;
	class = error_classes[level];
	pkt = BP_PKT(UPTR->un_utab.b_forw);
	printf("%s%d:\tError for command '%s'\n", DNAME, DUNIT,
		scsi_cmd_decode(CDBP(pkt)->scc_cmd, sd_cmds));
	printf("    \tError Level: %s Block: %d\n", class, UPTR->un_err_blkno);
	printf("    \tSense Key:   %s\n", sense_keys[devp->sd_sense->es_key]);
	printf("    \tAdditional Sense key:   0x%x\n", sb[12]);

	if (devp->sd_sense->es_code) {
		printf("    \t");
		sr_error_code_print(devp);
	}
	printf("\n");

}

#endif	NSR > 0
