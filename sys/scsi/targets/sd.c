#ident  "@(#)sd.c 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */
#include "sd.h"
#if	NSD > 0
/*
 *	SCSI fixed disk driver
 *
 */

/*
 *
 * Includes, Declarations and Local Data
 *
 */

#include <scsi/scsi.h>
#include <scsi/targets/sddef.h>
#include <scsi/impl/uscsi.h>
#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>

/*
 *
 * Global Data Definitions and references
 *
 */

extern int sd_error_level, sd_io_time, sd_fmt_time;
extern int sd_retry_count, sdnspecial;
extern struct sd_drivetype sdspecial[];

extern int maxphys, dkn;
static int sdmaxphys;

#ifdef	B_KLUSTER
extern int klustsort(), klustdone();
extern void klustbust();
#endif	/* B_KLUSTER */

/*
 *
 * Local Static Data
 *
 */

static struct scsi_device *sdunits[SD_MAXUNIT];
static struct scsi_address *sdsvdaddr[SD_MAXUNIT];
static int sddebug = 0;
static int sdpri = 0;
static char *diskokay = "disk okay";

/*
 * Forward reference definitions
 */
extern char *sd_cmds[];

/*
 * Configuration Data
 */

#ifdef	OPENPROMS
/*
 * Device driver ops vector
 */
int sdslave(), sdattach(), sdopen(), sdclose(), sdread(), sdwrite();
int sdstrategy(), sddump(), sdioctl(), sdsize();
struct dev_ops sd_ops = {
	1,
	sdslave,
	sdattach,
	sdopen,
	sdclose,
	sdread,
	sdwrite,
	sdstrategy,
	sddump,
	sdsize,
	sdioctl
};
#else	OPENPROMS
/*
 * sddriver is only used to attach
 */
int sdslave(), sdattach(), sdopen(), sdclose(), sdread(), sdwrite();
int sdstrategy(), sddump(), sdioctl(), sdsize();
extern int nulldev(), nodev();
struct mb_driver sddriver = {
	nulldev, sdslave, sdattach, nodev, nodev, nulldev, 0, "sd", 0, 0, 0, 0
};
#endif

/*
 *
 * Local Function Declarations
 *
 */

static void inq_fill(), sdintr(), sd_doattach();
static void sderrmsg(), sddone(), sd_offline(), sd_lock_unlock();
static void make_sd_cmd(), sdstart(), sddone();
static int sdrunout(), sd_findslave(), sd_winchester_exists();
static int sd_unit_ready();
static void sdprintf(), sdlog();
static void sd_setlink();
static int sd_testlink(), sd_maptouscsi();
#ifdef	ADAPTEC
static void sdintr_adaptec();
#endif	/* ADAPTEC */

/*
 *
 * Autoconfiguration Routines
 *
 */

int
sdslave(devp)
struct scsi_device *devp;
{
	int r, unit;

#ifdef	sun4m
	sdmaxphys = 0x3f000;
#else
	sdmaxphys = maxphys;
#endif	/* sun4m */

	unit = DUNIT;
	/*
	 * fill in our local array
	 */

	if (unit >= SD_MAXUNIT || sdunits[unit])
		return (0);
	sdunits[unit] = devp;

#ifdef	OPENPROMS
	sdpri = MAX(sdpri, ipltospl(devp->sd_dev->devi_intr->int_pri));
#else
	sdpri = MAX(sdpri, pritospl(devp->sd_dev->md_intpri));
#endif

	/*
	 * Turn around and call real slave routine. If it returns -1,
	 * then try and save the address structure for a possible later open.
	 */
	r = sd_findslave(devp, 0);
	if (r < 0) {
		sdsvdaddr[unit] = (struct scsi_address *)
		    kmem_zalloc(sizeof (struct scsi_address));
		if (sdsvdaddr[unit]) {
			sdunits[unit] = 0;
			*sdsvdaddr[unit] = devp->sd_address;
		} else {
			r = 0;
		}
	}
	return (r);
}

static int
sd_findslave(devp, canwait)
register struct scsi_device *devp;
int canwait;
{
	static char *nonccs =
		"non-CCS device found at target %d lun %d on %s%d";
	auto struct scsi_capacity cbuf;
	register struct scsi_pkt *rqpkt = (struct scsi_pkt *) 0;
	register struct scsi_disk *un = (struct scsi_disk *) 0;
	register struct sd_drivetype *dp;
	register int made_dp;

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
		if (devp->sd_inq) {
			IOPBFREE (devp->sd_inq, SUN_INQSIZE);
			devp->sd_inq = (struct scsi_inquiry *) 0;
		}
		return (-1);
	case SCSIPROBE_NONCCS:
#ifdef	ADAPTEC
		/*
		 * Okay, the INQUIRY command failed.
		 * How hard do we want to work at seeing
		 * if this is the right device, and how
		 * much can we assume about this target?
		 */
		bzero((caddr_t) devp->sd_inq, SUN_INQSIZE);
		devp->sd_inq->inq_dtype = DTYPE_DIRECT;
		devp->sd_inq->inq_rdf = RDF_LEVEL0;
		bcopy("ADAPTEC", devp->sd_inq->inq_vid, 7);
		bcopy("ACB4000", devp->sd_inq->inq_pid, 7);
		sdlog(devp, LOG_WARNING, nonccs,
		    Tgt(devp), Lun(devp), CNAME, CUNIT);
		break;
#else	/* ADAPTEC */
		sdlog(devp, LOG_ERR, nonccs,
		    Tgt(devp), Lun(devp), CNAME, CUNIT);
		devp->sd_inq->inq_dtype = DTYPE_NOTPRESENT;
		/* FALLTHROUGH */
#endif	/* ADAPTEC */
	case SCSIPROBE_EXISTS:
		switch (devp->sd_inq->inq_dtype) {
		case DTYPE_DIRECT:
			break;
		case DTYPE_NOTPRESENT:
		default:
			IOPBFREE (devp->sd_inq, SUN_INQSIZE);
			devp->sd_inq = (struct scsi_inquiry *) 0;
			return (-1);
		}
	}

	/*
	 * Allocate a request sense packet.
	 */

	rqpkt = get_pktiopb(ROUTE, (caddr_t *) &devp->sd_sense, CDB_GROUP0, 1,
		SENSE_LENGTH, B_READ, (canwait)? SLEEP_FUNC: NULL_FUNC);
	if (!rqpkt) {
		return (0);
	}
	rqpkt->pkt_pmon = -1;
	makecom_g0(rqpkt, devp, FLAG_NOPARITY,
	    SCMD_REQUEST_SENSE, 0, SENSE_LENGTH);

	/*
	 * If the device is not a removable media device, make sure that
	 * it can be started (if possible) with the START command, and
	 * attempt to read it's capacity too (if possible).
	 */

	cbuf.capacity = -1;
	cbuf.lbasize = 0;
	if (devp->sd_inq->inq_rmb == 0) {
		if (sd_winchester_exists(devp, rqpkt, &cbuf, canwait)) {
			goto error;
		}
	}

	/*
	 * The actual unit is present.
	 * The attach routine will check validity of label
	 * (and print out whether it is there).
	 * Now is the time to fill in the rest of our info..
	 */

	un = (struct scsi_disk *) kmem_zalloc(sizeof (struct scsi_disk));
	if (!(UPTR = un)) {
		goto error;
	}
	un->un_sbufp = (struct buf *) kmem_zalloc(sizeof (struct buf));
	if (un->un_sbufp == (struct buf *) NULL) {
		goto error;
	}

	/*
	 * Look through our list of special drives to see whether
	 * this drive is known to have special problems.
	 */

	made_dp = 0;
	for (dp = sdspecial; dp < &sdspecial[sdnspecial]; dp++) {
		/*
		 * It turns out that order is important for strcmp()!
		 */
		if (bcmp(devp->sd_inq->inq_vid, dp->id, strlen(dp->id)) == 0) {
			un->un_dp = dp;
			break;
		}
	}

	if (un->un_dp == 0) {
		/*
		 * Assume CCS drive, assume parity
		 */
		made_dp = 1;
		un->un_dp = (struct sd_drivetype *)
			kmem_zalloc(sizeof (struct sd_drivetype));
		if (!un->un_dp) {
			goto error;
		}
		un->un_dp->id = kmem_alloc((u_int)12);
		if (!un->un_dp->id)
			goto error;
		bcopy(devp->sd_inq->inq_vid, un->un_dp->id, 12);
		un->un_dp->ctype = CTYPE_CCS;
	}

	if ((un->un_dp->options & SD_NOPARITY) == 0)
		rqpkt->pkt_flags &= ~FLAG_NOPARITY;
	devp->sd_present = 1;
	if (cbuf.capacity != (u_long) -1) {
		/*
		 * The returned capacity is the LBA of the last
		 * addressable logical block, so the real capacity
		 * is one greater
		 */
		un->un_capacity = cbuf.capacity+1;
		un->un_lbasize = cbuf.lbasize;
	} else {
		un->un_capacity = 0;
		un->un_lbasize = SECSIZE;
	}

	rqpkt->pkt_comp = sdintr;
	rqpkt->pkt_time = sd_io_time;
	if (un->un_dp->options & SD_NODISC)
		rqpkt->pkt_flags |= FLAG_NODISCON;
	un->un_rqs = rqpkt;
	un->un_sd = devp;
	un->un_dkn = (char) -1;
#ifdef	OPENPROMS
	devp->sd_dev->devi_driver = &sd_ops;
#endif	OPENPROMS
	return (1);

error:
	if (un) {
		if (un->un_sbufp) {
			(void) kmem_free((caddr_t)un->un_sbufp,
				sizeof (struct buf));
		}
		if (made_dp) {
			if (un->un_dp->id) {
				(void) kmem_free(un->un_dp->id, (u_int) 12);
			}
			if (un->un_dp) {
				(void) kmem_free((caddr_t) un->un_dp,
					(u_int) sizeof (struct sd_drivetype));
			}
		}
		(void) kmem_free((caddr_t) un, sizeof (struct scsi_disk));
		UPTR = (struct scsi_disk *) 0;
	}
	if (rqpkt) {
		free_pktiopb(rqpkt, (caddr_t) devp->sd_sense, SENSE_LENGTH);
		devp->sd_sense = (struct scsi_extended_sense *) 0;
	}
	return (0);
}

static int
sd_winchester_exists(devp, rqpkt, cptr, canwait)
struct scsi_device *devp;
struct scsi_pkt *rqpkt;
struct scsi_capacity *cptr;
int canwait;
{
	static char *emustring = "EMULEX  MD21/S2";
	register struct scsi_pkt *pkt;
	auto caddr_t wrkbuf;
	register rval = -1;

	/*
	 * Get a work packet to play with. Get one with a buffer big
	 * enough for another INQUIRY command, and get one with
	 * a cdb big enough for the READ CAPACITY command.
	 */

	pkt = get_pktiopb(ROUTE, &wrkbuf, CDB_GROUP1, 1, SUN_INQSIZE, B_READ,
	    (canwait)? SLEEP_FUNC: NULL_FUNC);

	if (pkt == NULL) {
		return (rval);
	}

	/*
	 * Send a throwaway START UNIT command.
	 *
	 * If we fail on this, we don't care present what
	 * precisely is wrong. Fire off a throwaway REQUEST
	 * SENSE command if the failure is a CHECK_CONDITION.
	 *
	 */

	makecom_g0(pkt, devp, FLAG_NOPARITY, SCMD_START_STOP, 0, 1);
	(void) scsi_poll(pkt);
	if (SCBP(pkt)->sts_chk) {
		(void) scsi_poll (rqpkt);
	}

	/*
	 * Send another Inquiry command to the target. This is necessary
	 * for non-removable media direct access devices because their
	 * Inquiry data may not be fully qualified until they are spun up
	 * (perhaps via the START command above)
	 */

	bzero(wrkbuf, SUN_INQSIZE);
	makecom_g0(pkt, devp, FLAG_NOPARITY, SCMD_INQUIRY, 0, SUN_INQSIZE);
	if (scsi_poll(pkt) < 0)  {
		goto out;
	} else if (SCBP(pkt)->sts_chk) {
		(void) scsi_poll (rqpkt);
	} else {
		if ((pkt->pkt_state & STATE_XFERRED_DATA) &&
		    (SUN_INQSIZE - pkt->pkt_resid) >= SUN_MIN_INQLEN) {
			bcopy(wrkbuf, (caddr_t) devp->sd_inq, SUN_INQSIZE);
		}
	}

	/*
	 * It would be nice to attempt to make sure that there is a disk there,
	 * but that turns out to be very hard- We used to use the REZERO
	 * command to verify that a disk was attached, but some SCSI disks
	 * can't handle a REZERO command. We can't use a READ command
	 * because the disk may not be formatted yet. Therefore, we just
	 * have to believe a disk is there until proven otherwise, *except*
	 * (hack hack hack) for the MD21.
	 */

	if (bcmp(devp->sd_inq->inq_vid, emustring, strlen(emustring)) == 0) {
		makecom_g0(pkt, devp, FLAG_NOPARITY, SCMD_REZERO_UNIT, 0, 0);
		if (scsi_poll(pkt) || SCBP_C(pkt) != STATUS_GOOD) {
			if (SCBP(pkt)->sts_chk) {
				(void) scsi_poll (rqpkt);
			}
			goto out;
		}
	}

	/*
	 * At this point, we've 'succeeded' with this winchester
	 */

	rval = 0;

	/*
	 * Attempt to read the drive's capacity.
	 * It's okay to fail with this.
	 */

	makecom_g1(pkt, devp, FLAG_NOPARITY, SCMD_READ_CAPACITY, 0, 0);

	if (scsi_poll(pkt) >= 0 && SCBP_C(pkt) == STATUS_GOOD &&
	    (pkt->pkt_state & STATE_XFERRED_DATA) &&
	    (pkt->pkt_resid == SUN_INQSIZE-(sizeof (struct scsi_capacity)))) {
		cptr->capacity = ((struct scsi_capacity *)wrkbuf)->capacity;
		cptr->lbasize = ((struct scsi_capacity *)wrkbuf)->lbasize;
	} else if (SCBP(pkt)->sts_chk) {
		(void) scsi_poll (rqpkt);
	}
out:
	free_pktiopb(pkt, wrkbuf, SUN_INQSIZE);
	return (rval);
}

/*
 * Attach disk. The controller is there. Is the label valid?
 * This is a wrapper for sd_doattach.
 */

int
sdattach(devp)
struct scsi_device *devp;
{
	sd_doattach(devp, NULL_FUNC);

	/*
	 * If this is a removable media device,
	 * invalidate the geometry in order to
	 * force the geometry to be re-examined
	 * at open time.
	 */
	if (devp->sd_inq->inq_rmb && UPTR)
		UPTR->un_gvalid = 0;
}

static void
sd_doattach(devp, f)
register struct scsi_device *devp;
int (*f)();
{
	void sd_uselabel();
	auto struct dk_label *dkl;
	register struct scsi_pkt *pkt;
	register struct scsi_disk *un;
	int unit, fnoparity;
	auto char *label = 0, labelstring[128];

	if (!(un = UPTR))
		return;

	/*
	 * FIX ME:
	 */
	if (un->un_capacity > 0 && un->un_lbasize != SECSIZE) {
		sdlog(devp, LOG_ERR,
		    "logical block size %d not supported", un->un_lbasize);
		return;
	}
	unit = DUNIT;
	if (un->un_dp->options & SD_NOPARITY)
		fnoparity = FLAG_NOPARITY;
	else
		fnoparity = 0;

	/*
	 * Only DIRECT ACCESS devices will have Sun labels
	 */

	if (devp->sd_inq->inq_dtype == DTYPE_DIRECT) {
		int i;
		pkt = get_pktiopb(ROUTE, (caddr_t *) &dkl, CDB_GROUP0,
			1, SECSIZE, B_READ, f);
		if (pkt == NULL) {
			sdlog(devp, LOG_CRIT, "no memory for disk label");
			return;
		}
		bzero ((caddr_t) dkl,  SECSIZE);
		makecom_g0(pkt, devp, fnoparity, SCMD_READ, 0, 1);
		/*
		 * Ok, it's ready - try to read and use the label.
		 */
		un->un_gvalid = 0;
		for (i = 0; i < 3; i++) {
			if (scsi_poll(pkt) || SCBP_C(pkt) != STATUS_GOOD ||
			    (pkt->pkt_state & STATE_XFERRED_DATA) == 0 ||
			    (pkt->pkt_resid != 0)) {
				if (i > 2 && un->un_state == SD_STATE_NIL)
					sdlog(devp, LOG_ERR,
					    "unable to read label");
				if (SCBP(pkt)->sts_chk) {
					(void) scsi_poll (un->un_rqs);
				}
			} else {
				/*
				 * sd_uselabel will establish
				 * that the geometry is valid
				 */
				sd_uselabel(devp, dkl);
				break;
			}
		}
		/*
		 * XXX: Use Mode Sense to determine things like
		 * XXX; rpm, geometry, from SCSI-2 compliant
		 * XXX: peripherals
		 */
		if (un->un_g.dkg_rpm == 0)
			un->un_g.dkg_rpm = 3600;
		bcopy (dkl->dkl_asciilabel, labelstring, 128);
		label = labelstring;
		free_pktiopb(pkt, (caddr_t) dkl, SECSIZE);
	} else if (un->un_capacity < 0) {
		return;
	} else  {
		/*
		 * This is a totally bogus geometry- really need to do a mode
		 * sense.
		 */
		label = 0;
		bzero((caddr_t) &un->un_g, sizeof (struct dk_geom));
		un->un_g.dkg_pcyl = un->un_capacity >> 5;
		un->un_g.dkg_ncyl = un->un_g.dkg_pcyl - 2;
		un->un_g.dkg_acyl = 2;
		un->un_g.dkg_nhead = 1;
		un->un_g.dkg_nsect = 32;
		un->un_g.dkg_intrlv = 1;
		un->un_g.dkg_rpm = 3000;
		bzero((caddr_t)un->un_map, NDKMAP * (sizeof (struct dk_map)));
		un->un_map['c'-'a'].dkl_cylno = 0;
		un->un_map['c'-'a'].dkl_nblk = un->un_capacity;
		inq_fill(devp->sd_inq->inq_vid, 8, labelstring);
		printf("sd%d: <%s %d blocks>\n",
		    unit, labelstring, un->un_capacity);
	}

	if (un->un_state == SD_STATE_NIL) {
		New_state(un, SD_STATE_CLOSED);
		/*
		 * print out a message indicating who and what we are
		 * if the geometry is valid, print the label string,
		 * else print vendor and product info, if available
		 */
		if (un->un_gvalid && label) {
			printf("sd%d: <%s>\n", unit, label);
		} else {
			inq_fill(devp->sd_inq->inq_vid, 8, labelstring);
			inq_fill(devp->sd_inq->inq_pid, 16, &labelstring[64]);
			printf("sd%d: Vendor '%s', product '%s'",
			    unit, labelstring, &labelstring[64]);
			if (un->un_capacity > 0) {
				printf(", %d %d byte blocks\n",
				    un->un_capacity, un->un_lbasize);
			} else
				printf(", (unknown capacity)\n");
		}
	}
}

/*
 * Check the label for righteousity, and snarf yummos from validated label.
 * Marks the geometyr of the unit as being valid.
 */

static void
sd_uselabel(devp, l)
struct scsi_device *devp;
struct dk_label *l;
{
	static char *geom = "Label says %d blocks, Drive says %d blocks";
	static char *badlab = "corrupt label - %s";
	short *sp, sum, count;
	struct scsi_disk *un = UPTR;
	long capacity;

	/*
	 * Check magic number of the label
	 */
	if (l->dkl_magic != DKL_MAGIC) {
		if (UPTR->un_state == SD_STATE_NIL)
			sdlog(devp, LOG_ERR, badlab, "wrong magic number");
		return;
	}

	/*
	 * Check the checksum of the label
	 */
	sp = (short *)l;
	sum = 0;
	count = sizeof (struct dk_label) / sizeof (short);
	while (count--)  {
		sum ^= *sp++;
	}

	if (sum) {
		if (un->un_state == SD_STATE_NIL)
			sdlog(devp, LOG_ERR, badlab, "label checksum failed");
		return;
	}

	/*
	 * Fill in disk geometry from label.
	 */
	un->un_g.dkg_ncyl = l->dkl_ncyl;
	un->un_g.dkg_acyl = l->dkl_acyl;
	un->un_g.dkg_bcyl = 0;
	un->un_g.dkg_nhead = l->dkl_nhead;
	un->un_g.dkg_bhead = l->dkl_bhead;
	un->un_g.dkg_nsect = l->dkl_nsect;
	un->un_g.dkg_gap1 = l->dkl_gap1;
	un->un_g.dkg_gap2 = l->dkl_gap2;
	un->un_g.dkg_intrlv = l->dkl_intrlv;
	un->un_g.dkg_pcyl = l->dkl_pcyl;
	un->un_g.dkg_rpm = l->dkl_rpm;

	/*
	 * If labels don't have pcyl in them, make a guess at it.
	 * The right thing, of course, to do, is to do a MODE SENSE
	 * on the Rigid Disk Geometry mode page and check the
	 * information with that. Won't work for non-CCS though.
	 */

	if (un->un_g.dkg_pcyl == 0)
		un->un_g.dkg_pcyl = un->un_g.dkg_ncyl + un->un_g.dkg_acyl;

	/*
	 * Fill in partition table.
	 */

	bcopy((caddr_t) l->dkl_map, (caddr_t) un->un_map,
	    NDKMAP * sizeof (struct dk_map));

	/*
	 * dk instrumentation
	 */
#ifdef	OPENPROMS
	if ((u_char) un->un_dkn == (u_char) -1) {
		int i = newdk("sd", DUNIT);
		if (i >= 0) {
			un->un_dkn = i;
			i = un->un_g.dkg_intrlv;
			if (i <= 0 || i >= un->un_g.dkg_nsect) {
				i = 1;
			}
			dk_bps[un->un_dkn] =
			    (SECSIZE * 60 * un->un_g.dkg_nsect) / i;
		}
	}
#else	OPENPROMS
	if ((u_char) un->un_dkn == (u_char) -1 &&
	    devp->sd_dev->md_dk == 1 && dkn < DK_NDRIVE) {
		int intrlv = un->un_g.dkg_intrlv;
		if (intrlv <= 0 || intrlv >= un->un_g.dkg_nsect) {
			intrlv = 1;
		}
		devp->sd_dev->md_dk = un->un_dkn = dkn++;
		dk_bps[un->un_dkn] =
			(SECSIZE * 60 * un->un_g.dkg_nsect) / intrlv;
	} else {
		devp->sd_dev->md_dk = -1;
	}
#endif	OPENPROMS

	un->un_gvalid = 1;			/* "it's here..." */

	capacity = un->un_g.dkg_ncyl * un->un_g.dkg_nhead * un->un_g.dkg_nsect;
	if (un->un_g.dkg_acyl)
		capacity +=  (un->un_g.dkg_nhead * un->un_g.dkg_nsect);

	if (un->un_capacity > 0) {
		if (capacity > un->un_capacity &&
		    un->un_state == SD_STATE_NIL) {
			sdlog(devp, LOG_ERR, badlab, "bad geometry");
			sdlog(devp, LOG_ERR, geom, capacity, un->un_capacity);
			un->un_gvalid = 0;	/* "No it's not!.." */
		} else if (DEBUGGING_ALL) {
			sdlog(devp, LOG_INFO, geom, capacity, un->un_capacity);
		}
	} else {
		/*
		 * We have a situation where the target didn't give us a
		 * good 'read capacity' command answer, yet there appears
		 * to be a valid label. In this case, we'll
		 * fake the capacity.
		 */
		un->un_capacity = capacity;
	}
}

/*
 * Unix Entry Points
 */

int
sdopen(dev, flag)
dev_t dev;
int flag;
{
	register struct scsi_device *devp;
	register struct scsi_disk *un;
	register int unit, s;


	if ((unit = SDUNIT(dev)) >= SD_MAXUNIT) {
		return (ENXIO);
	}

	if (!(devp = sdunits[unit])) {
		struct scsi_address *sa = sdsvdaddr[unit];
		if (sa) {
			if (scsi_add_device(sa, &DRIVER, "sd", unit) == 0) {
				return (ENXIO);
			}
			devp = sdunits[unit];
			if (!devp) {
				return (ENXIO);
			}
			(void) kmem_free((caddr_t) sa, sizeof (*sa));
			sdsvdaddr[unit] = (struct scsi_address *) 0;
		} else {
			return (ENXIO);
		}
	} else if (!devp->sd_present) {
		s = splr(sdpri);
		if (sd_findslave(devp, 1) > 0) {
			printf("sd%d at %s%d target %d lun %d\n", unit,
			    CNAME, CUNIT, Tgt(devp), Lun(devp));
			sd_doattach(devp, SLEEP_FUNC);
		} else {
			(void) splx(s);
			return (ENXIO);
		}
		(void) splx(s);
	}

	un = UPTR;
	if (un->un_gvalid == 0) {
		s = splr(sdpri);
		sd_doattach(devp, SLEEP_FUNC);
		(void) splx(s);
	}

	/*
	 * no, don't put an else here: sdattach may validate the geometry....
	 */

	if (un->un_gvalid == 0 || un->un_map[SDPART(dev)].dkl_nblk <= 0) {
		/*
		 * Are we opening this for format(8)?
		 */
		if ((flag & (FNDELAY|FNBIO)) == 0 || !SD_ISCDEV(dev)) {
			return (ENXIO);
		}
	}

	if (un->un_state == SD_STATE_DETACHING) {
		if (sd_unit_ready(dev) == 0) {
			return (ENXIO);
		}
		/*
		 * disk now back on line...
		 */
		New_state(un, SD_STATE_OPENING);
		sdlog(devp, LOG_INFO, diskokay);
	} else if (un->un_state == SD_STATE_CLOSED) {
		New_state(un, SD_STATE_OPENING);
		if (sd_unit_ready(dev) == 0) {
			sd_offline(devp, 1);
			return (ENXIO);
		}
		/*
		 * If this is a removable media device, try and send
		 * a PREVENT MEDIA REMOVAL command, but don't get upset
		 * if it fails.
		 */
		if (devp->sd_inq->inq_rmb) {
			sd_lock_unlock(dev, 1);
		}
	}
	New_state(un, SD_STATE_OPEN);
	SD_SET_OMAP(un, &dev);
	return (0);
}

int
sdclose(dev)
dev_t dev;
{
	register struct scsi_device *devp;
	register struct scsi_disk *un;
	int unit;

	if ((unit = SDUNIT(dev)) >= SD_MAXUNIT) {
		return (ENXIO);
	} else if (!(devp = sdunits[unit]) || !devp->sd_present) {
		return (ENXIO);
	} else if (!(un = UPTR) || un->un_state < SD_STATE_OPEN) {
		return (ENXIO);
	}

	DPRINTF(devp, "last close on dev %d.%d", major(dev), minor(dev));

	SD_CLR_OMAP(un, &dev);
	if (un->un_omap == 0) {
		if (un->un_state == SD_STATE_DETACHING) {
			sd_offline(devp, 1);
		} else {
			New_state(un, SD_STATE_CLOSED);

			/*
			 * If this is a removable media device, try and send
			 * an ALLOW MEDIA REMOVAL command, but don't get upset
			 * if it fails.
			 *
			 * For now, if it is a removable media device,
			 * invalidate the geometry.
			 *
			 * XXX: Later investigate whether or not it
			 * XXX: would be a good idea to invalidate
			 * XXX: the geometry for all devices.
			 */

			if (devp->sd_inq->inq_rmb) {
				sd_lock_unlock(dev, 0);
				un->un_gvalid = 0;
			}
		}
	}
	return (0);
}

static void
sd_offline(devp, bechatty)
register struct scsi_device *devp;
int bechatty;
{
	register struct scsi_disk *un = UPTR;

	if (bechatty)
		sdlog(devp, LOG_WARNING, "offline");

#ifdef	OPENPROMS
	(void) remdk((int) un->un_dkn);
#endif	/* OPENPROMS */
	free_pktiopb(un->un_rqs, (caddr_t)devp->sd_sense, SENSE_LENGTH);
	(void) kmem_free((caddr_t) un->un_sbufp,
		(unsigned) (sizeof (struct buf)));
	(void) kmem_free((caddr_t) un,
		(unsigned) (sizeof (struct scsi_disk)));
	devp->sd_private = (opaque_t) 0;
	IOPBFREE (devp->sd_inq, SUN_INQSIZE);
	devp->sd_inq = (struct scsi_inquiry *) 0;
	devp->sd_sense = 0;
	devp->sd_present = 0;
}

int
sdsize(dev)
dev_t dev;
{
	struct scsi_device *devp;
	int unit;

	if ((unit = SDUNIT(dev)) >= SD_MAXUNIT) {
		return (-1);
	} else if (!(devp = sdunits[unit]) || !devp->sd_present ||
	    !UPTR->un_gvalid) {
		return (-1);
	}
	return ((int)UPTR->un_map[SDPART(dev)].dkl_nblk);
}


/*
 * These routines perform raw i/o operations.
 */

sdread(dev, uio)
dev_t dev;
struct uio *uio;
{
	return (sdrw(dev, uio, B_READ));
}

sdwrite(dev, uio)
dev_t dev;
struct uio *uio;
{
	return (sdrw(dev, uio, B_WRITE));
}


static void
sdmin(bp)
struct buf *bp;
{
	if (bp->b_bcount > sdmaxphys)
		bp->b_bcount = sdmaxphys;
}

static int
sdrw(dev, uio, flag)
dev_t dev;
struct uio *uio;
int flag;
{
	struct scsi_device *devp;
	register int unit;


	if ((unit = SDUNIT(dev)) >= SD_MAXUNIT) {
		return (ENXIO);
	} else if (!(devp = sdunits[unit]) || !devp->sd_present ||
	    !UPTR->un_gvalid) {
		return (ENXIO);
	} else if ((uio->uio_fmode & FSETBLK) == 0 &&
	    (uio->uio_offset & (DEV_BSIZE - 1))) {
		DPRINTF(devp, "file offset not modulo %d", DEV_BSIZE);
		return (EINVAL);
	} else if (uio->uio_iov->iov_len & (DEV_BSIZE - 1)) {
		DPRINTF(devp, "block length not modulo %d", DEV_BSIZE);
		return (EINVAL);
	}
	return (physio(sdstrategy, (struct buf *) 0, dev, flag, sdmin, uio));
}

/*
 * strategy routine
 */

int (*sdstrategy_tstpoint)();

int
sdstrategy(bp)
register struct buf *bp;
{
	register struct scsi_device *devp;
	register struct scsi_disk *un;
	register struct diskhd *dp;
	register s;

	if (sdstrategy_tstpoint != NULL) {
		if ((*sdstrategy_tstpoint)(bp)) {
			return;
		}
	}
	devp = sdunits[SDUNIT(bp->b_dev)];
	if (!devp || !devp->sd_present || !(un = UPTR) ||
	    un->un_state == SD_STATE_DUMPING) {
nothere:
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		iodone(bp);
		return;
	}

	if (bp != un->un_sbufp && un->un_state == SD_STATE_DETACHING) {
		if (sd_unit_ready(bp->b_dev) == 0) {
			goto nothere;
		}
		/*
		 * disk now back on line...
		 */
		New_state(un, SD_STATE_OPEN);
		sdlog(devp, LOG_INFO, diskokay);
	}

	bp->b_flags &= ~(B_DONE|B_ERROR);
	bp->av_forw = 0;

	dp = &un->un_utab;
	if (bp != un->un_sbufp) {
		if (un->un_gvalid == 0) {
			sdlog(devp, LOG_ERR, "i/o to invalid geometry");
			bp->b_flags |= B_ERROR;
		} else {
			struct dk_map *lp = &un->un_map[SDPART(bp->b_dev)];
			register daddr_t bn = dkblock(bp);

			/*
			 * Use s to note whether we are
			 * either done and/or in error.
			 */

			s = 0;
			if (bn < 0) {
				s = -1;
			} else if (bn >= lp->dkl_nblk) {
				/*
				 * if bn == lp->dkl_nblk,
				 * Not an error, resid == count
				 */
				if (bn > lp->dkl_nblk) {
					s = -1;
				} else {
					s = 1;
				}
			} else if (bp->b_bcount & (SECSIZE-1)) {
				/*
				 * This should really be:
				 *
				 * ... if (bp->b_bcount & (un->un_lbasize-1))
				 *
				 */
				s = -1;
			} else {
				/*
				 * sort by absolute block number.
				 * note that b_resid is used to store
				 * sort key and to return the transfer
				 * resid.
				 */
				bp->b_resid = bn + lp->dkl_cylno *
				    un->un_g.dkg_nhead * un->un_g.dkg_nsect;
				/*
				 * zero out av_back - this will be a signal
				 * to sdstart to go and fetch the resources
				 */
				BP_PKT(bp) = 0;
			}

			/*
			 * Check to see whether or not we are done (with
			 * or without errors), then reuse s for tmp var
			 * calculations for stats gathering.
			 */

			if (s != 0) {
				bp->b_resid = bp->b_bcount;
				if (s < 0) {
					bp->b_flags |= B_ERROR;
				} else {
					iodone(bp);
				}
			}
		}
	} else {
		/*
		 * For internal cmds, we just zero out the sort key
		 */
		bp->b_resid = 0;
	}

	if ((s = (bp->b_flags & (B_DONE|B_ERROR))) == 0) {
		s = splr(sdpri);
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
#ifdef	B_KLUSTER
		if (klustsort(dp, bp, sdmaxphys)) {
			bp = dp->b_actf;
			if (BP_PKT(bp) && (bp->b_flags & B_KLUSTER)) {
				scsi_resfree(BP_PKT(bp));
				BP_PKT(bp) = NULL;
			}
		}
#else	/* B_KLUSTER */
		disksort(dp, bp);
#endif	/* B_KLUSTER */
		if ((++dp->b_bcount) > un->un_sds.sds_hiqlen)
			un->un_sds.sds_hiqlen = dp->b_bcount;
		if (dp->b_forw == NULL) { /* this device inactive? */
			sdstart(devp);
		} else if (BP_PKT(dp->b_actf) == 0) {
			/*
			 * try and map this one
			 */
			make_sd_cmd(devp, dp->b_actf, NULL_FUNC);
		}
		(void) splx(s);
	} else if ((s & B_DONE) == 0) {
		iodone(bp);
	}
}

/*
 *	System Crash Dump routine
 */

#define	NDUMP_RETRIES	5

int
sddump(dev, addr, blkno, nblk)
dev_t dev;
caddr_t addr;
daddr_t blkno;
int nblk;
{
	struct dk_map *lp;
	struct scsi_device *devp;
	struct scsi_disk *un;
	struct scsi_pkt *pkt;
	register s, i, pflag;
	int err;

	if (SDUNIT(dev) >= SD_MAXUNIT) {
		return (ENXIO);
	} else if (!(devp = sdunits[SDUNIT(dev)]) || !devp->sd_present ||
	    !(un = UPTR) || !un->un_gvalid) {
		return (ENXIO);
	}

	lp = &un->un_map[SDPART(dev)];
	if (blkno+nblk > lp->dkl_nblk) {
		return (EINVAL);
	}

	/*
	 * The first time through, reset the specific target device.
	 * Do a REQUEST SENSE if required.
	 */

	pflag = FLAG_NOINTR|FLAG_NODISCON;
	if (un->un_dp->options & SD_NOPARITY)
		pflag |= FLAG_NOPARITY;
	un->un_rqs->pkt_flags |= pflag;

	s = splr(sdpri);

	if (un->un_state != SD_STATE_DUMPING) {

		New_state(un, SD_STATE_DUMPING);

		/*
		 * Abort any active commands for this target
		 */

		(void) scsi_abort(ROUTE, (struct scsi_pkt *) 0);

		/*
		 * Reset the bus. I'd like to not have to do this,
		 * but this is the safest thing to do...
		 */

		if (scsi_reset(ROUTE, RESET_ALL) == 0) {
			(void) splx(s);
			return (EIO);
		}
		DELAY(2*1000000);

		for (i = 0; i < NDUMP_RETRIES; i++) {
			if (scsi_poll(un->un_rqs)) {
				(void) splx(s);
				return (EIO);
			}
			if (SCBP_C(un->un_rqs) == STATUS_GOOD)
				break;
			DELAY(10000);
		}

	}
	(void) splx(s);

	blkno += (lp->dkl_cylno * un->un_g.dkg_nhead * un->un_g.dkg_nsect);

	/*
	 * It should be safe to call the allocator here without
	 * worrying about being locked for DVMA mapping because
	 * the address we're passed is already a DVMA mapping
	 */

	bzero((caddr_t) un->un_sbufp, sizeof (struct buf));
	un->un_sbufp->b_un.b_addr = addr;
	un->un_sbufp->b_bcount = nblk << DEV_BSHIFT;

	pkt = scsi_resalloc(ROUTE, CDB_GROUP1, 1,
	    (opaque_t) un->un_sbufp, NULL_FUNC);
	if (pkt == (struct scsi_pkt *) 0) {
		sdlog(devp, LOG_CRIT, "no resources for dumping");
		return (EIO);
	}

	if (blkno >= (1<<20)) {
		makecom_g1(pkt, devp, pflag, SCMD_WRITE_G1, (int) blkno, nblk);
	} else {
		makecom_g0(pkt, devp, pflag, SCMD_WRITE, (int) blkno, nblk);
	}

	s = splr(sdpri);
	for (err = EIO, i = 0; i < NDUMP_RETRIES && err == EIO; i++) {
		if (scsi_poll(pkt)) {
			if (scsi_reset(ROUTE, RESET_ALL) == 0) {
				break;
			}
		}
		switch (SCBP_C(pkt)) {
		case STATUS_GOOD:
			err = 0;
			break;
		case STATUS_BUSY:
			DELAY(5*1000000);
			break;
		case STATUS_CHECK:
			/*
			 * Hope this clears it up...
			 */
			(void) scsi_poll(un->un_rqs);
			break;
		default:
			break;
		}
	}
	(void) splx(s);
	scsi_resfree(pkt);
	return (err);
}

/*
 * This routine implements the ioctl calls.  It is called
 * from the device switch at normal priority.
 */
/*ARGSUSED3*/
sdioctl(dev, cmd, data, flag)
dev_t dev;
int cmd;
caddr_t data;
int flag;
{
	extern char *strcpy();
	register struct scsi_device *devp;
	register struct scsi_disk *un;
	struct dk_map *lp;
	struct dk_info *info;
	struct dk_conf *conf;
	struct dk_diag *diag;
	int unit, i, s, part;

	if ((unit = SDUNIT(dev)) >= SD_MAXUNIT || !(devp = sdunits[unit]) ||
	    !(devp->sd_present) || !(un = UPTR) ||
	    (un->un_state == SD_STATE_DETACHING)) {
		return (ENXIO);
	}

	part = SDPART(dev);
	lp = &un->un_map[part];

	switch (cmd) {
	case DKIOCWCHK:
	{
		int pbit = (1<<part);
		if (!suser()) {
			return (u.u_error);
		} else if (lp->dkl_nblk == 0) {
			return (ENXIO);
		}
		if (un->un_dp->options & SD_NOVERIFY) {
			return (ENOTTY);
		}
		i = (*((int *) data));
		if (i) {
			s = splr(sdpri);
			(*((int *) data)) = ((un->un_wchkmap & pbit) != 0);
			un->un_wchkmap |= pbit;

			/* VM HACK!!! */
#ifdef	VAC
			{
#ifdef	sun4
#include <machine/cpu.h>
				extern int cpu, vac, nopagereclaim;
				/* VM HACK!!! */

				if (cpu == CPU_SUN4_260 && vac) {
					nopagereclaim = 1;
				}
#endif	sun4
#ifdef	sun3
#include <machine/cpu.h>
				extern int cpu, vac, nopagereclaim;
				if (cpu == CPU_SUN3_260 && vac) {
					nopagereclaim = 1;
				}
#endif	sun4
			}
#endif	VAC
			(void) splx(s);
			printf("sd%d%c: write check enabled\n",
			    unit, 'a' + part);
		} else  {
			s = splr(sdpri);
			(*((int *) data)) = ((un->un_wchkmap & pbit) != 0);
			un->un_wchkmap &= ~pbit;
			(void) splx(s);
			printf("sd%d%c: write check disabled\n",
			    unit, 'a' + part);
		}
		return (0);
	}
	case DKIOCINFO:

	/*
	 * Return info concerning the controller.
	 */

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
#ifdef	ADAPTEC
		case CTYPE_ACB4000:
			info->dki_ctype = DKC_ACB4000;
			break;
#endif	/* ADAPTEC */
		default:
			info->dki_ctype = DKC_SCSI_CCS;
			break;
		}
		info->dki_flags = DKI_FMTVOL;
		return (0);

	case DKIOCGGEOM:
	/*
	 * Return the geometry of the specified unit.
	 */
		*(struct dk_geom *)data = un->un_g;
		return (0);

	/*
	 * Set the geometry of the specified unit.
	 */
	case DKIOCSGEOM:
		un->un_g = *(struct dk_geom *)data;
		return (0);

	case DKIOCGPART:
	/*
	 * Return the map for the specified logical partition.
	 * This has been made obsolete by the get all partitions
	 * command.
	 */

		*(struct dk_map *)data = *lp;
		return (0);
	case DKIOCSPART:
	/*
	 * Set the map for the specified logical partition.
	 * This has been made obsolete by the set all partitions
	 * command.  We raise the priority just to make sure
	 * an interrupt doesn't come in while the map is
	 * half updated.
	 */
		*lp = *(struct dk_map *)data;
		return (0);

	/*
	 * Return configuration info
	 */
	case DKIOCGCONF:
		conf = (struct dk_conf *)data;
		switch (un->un_dp->ctype) {
		case CTYPE_MD21:
			conf->dkc_ctype = DKC_MD21;
			break;
#ifdef	ADAPTEC
		case CTYPE_ACB4000:
			conf->dkc_ctype = DKC_ACB4000;
			break;
#endif	/* ADAPTEC */
		default:
			conf->dkc_ctype = DKC_MD21;
			break;
		}
		conf->dkc_dname[0] = 's';
		conf->dkc_dname[1] = 'd';
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
		for (i = 0; i < NDKMAP; i++)
			((struct dk_map *)data)[i] = un->un_map[i];
		return (0);

	/*
	 * Set the map for all logical partitions.  We raise
	 * the priority just to make sure an interrupt doesn't
	 * come in while the map is half updated.
	 */
	case DKIOCSAPART:
		s = splr(sdpri);
		for (i = 0; i < NDKMAP; i++)
			un->un_map[i] = ((struct dk_map *)data)[i];
		(void) splx(s);
		return (0);

	/*
	 * Get error status from last command.
	 */
	case DKIOCGDIAG:
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

	/*
	 * Run a generic command.
	 */
	case DKIOCSCMD:
		return (sd_maptouscsi(dev, data));

	/*
	 * Run a geneeric ucsi.h command.
	 */
	case USCSICMD:
		return (sdioctl_cmd(dev, data, 0));

	/*
	 * Handle unknown ioctls here.
	 */
	default:
		return (ENOTTY);
	}
}

static int
sd_maptouscsi(dev, data)
dev_t dev;
caddr_t data;
{
	register struct scsi_device *devp;
	struct dk_cmd *dcom;
	struct uscsi_cmd scmd, *ucom = &scmd;
	char cmdblk[CDB_SIZE], *cdb = cmdblk;
	u_short cmd;
	auto daddr_t blkno = 0;
	int count;
	int g1 = 0;

	bzero(cdb, CDB_SIZE);
	devp = sdunits[SDUNIT(dev)];
	dcom = (struct dk_cmd *)data;
	count = dcom->dkc_buflen;
	blkno = (daddr_t)dcom->dkc_blkno;
	cmd = dcom->dkc_cmd;

	switch (dcom->dkc_cmd) {
	case SCMD_READ:
	case SCMD_WRITE:
	{
		DPRINTF_IOCTL(devp, "special %s",
		    (dcom->dkc_cmd == SCMD_READ) ? "read" : "write");
		if (dcom->dkc_buflen & (SECSIZE-1))
			return (EINVAL);
		count = (count + (SECSIZE - 1)) >> SECDIV;
		if (dcom->dkc_cmd == SCMD_READ)
			ucom->uscsi_flags = USCSI_READ;
		else
			ucom->uscsi_flags = USCSI_WRITE;
		if (blkno >= (2<<20) || count > 0xff) {
			cmd |= SCMD_GROUP1;
			g1 = 1;
		}
		break;
	}
	case SCMD_MODE_SELECT:
	{
		DPRINTF_IOCTL(devp, "mode select; sp=%d",
		    (blkno & 0x80)? 1 : 0);
		if (blkno & 0x80) {
			/*
			 * The 'save parameters' bit
			 * is in the first bit of the 'tag',
			 * or bit #16 of a nominal 21 bit
			 * block address. Unfortunately, 4.0
			 * format(8) sets bit #7 to indicate
			 * that these are saveable parameters.
			 */
			blkno = 0x10000;
		} else
			blkno = 0;
		ucom->uscsi_flags = USCSI_WRITE;
		break;
	}
	case SCMD_MODE_SENSE:
	{
		DPRINTF_IOCTL(devp, "mode sense: pcf=0x%x, page 0x%x",
		    blkno>>5, blkno&0x1f);
		blkno <<= 8;
		ucom->uscsi_flags = USCSI_READ;
		break;
	}
	case SCMD_READ_DEFECT_LIST:
	{
		char descriptor;
		/*
		 * yet another compatibility deal with 4.0-
		 * we have to pick up the defect list descriptor
		 * from the user's buffer itself. Also, the
		 * Defect List Format stuff will end up in what
		 * is the most significant byte of a 32 byte
		 * block address.
		 */
		struct scsi_defect_list *sd =
		    (struct scsi_defect_list *) dcom->dkc_bufaddr;

		g1 = 1;
		u.u_error = copyin((caddr_t)&sd->descriptor, &descriptor, 1);
		if (u.u_error)
			return (u.u_error);
		DPRINTF_IOCTL(devp, "read_defect_list: descrip %x",
		    descriptor);
		blkno = descriptor & 0xff;
		blkno <<= 24;
		ucom->uscsi_flags = USCSI_READ;
		break;
	}
	case SCMD_REASSIGN_BLOCK:
	{
		/*
		 * Compatibility with 4.0. The block to be
		 * reassigned is in the block field. We
		 * have had to make a little buffer for
		 * it (in the correct format) and we will
		 * then put the block into that little buffer
		 */
		DPRINTF_IOCTL(devp, "reassign block %d, count %d (== 8?)",
		    blkno, count);
		if (devp->sd_inq->inq_rdf < RDF_CCS) {
			/* reassign block unlikely for non-CCS device */
			/* XXX SHOULDN'T WE JUST LET IT BOUNCE THE COMMAND? */
			return (EINVAL);
		} else if (dcom->dkc_buflen) {
			return (EINVAL);
		}
		ucom->uscsi_flags = USCSI_WRITE;
		/*
		 * Turn the cdb into a Group 1 command, so that the
		 * block number does not get truncated to 20 bits.
		 * This is a little wierd, since no such command
		 * exists for reassign, but we perform the
		 * inverse transformation later, so we're ok.
		 */
		cmd |= SCMD_GROUP1;
		g1 = 1;
		break;
	}
	case SCMD_FORMAT:
	{
		DPRINTF_IOCTL(devp, "format: blkno_bits 0x%x", blkno);
		if (blkno & 0x80000000) {
			unsigned int	tmp;
			/*
			 * interleave
			 */
			tmp = blkno;
			count = tmp&0xff;
			blkno = (tmp>>8)&0xff;
			/*
			 * data pattern
			 */
			blkno |= (((tmp>>16)&0xff)<<8);
			/*
			 * format parameter bits
			 */
			blkno |= (((tmp>>24)&0x1f)<<16);
		} else {
			blkno = (FPB_DATA|FPB_CMPLT|FPB_BFI)<<16;
			count = 1; /* interleave */
		}
		ucom->uscsi_flags = USCSI_WRITE;
		break;
	}
	case SCMD_TEST_UNIT_READY:
	{
		if (dcom->dkc_buflen)
			return (EINVAL);
		ucom->uscsi_flags = USCSI_WRITE;
		break;
	}
	default:
		return (EINVAL);
	}

	if (DEBUGGING_ALL) {
		sdprintf(devp, "sd_maptouscsi cmd= %x  blk= %x buflen= 0x%x",
		    dcom->dkc_cmd, dcom->dkc_blkno, dcom->dkc_buflen);
		if (ucom->uscsi_flags == USCSI_WRITE && dcom->dkc_buflen) {
			auto u_int i, amt = min(64, dcom->dkc_buflen);
			char buf [64];

			if (copyin(dcom->dkc_bufaddr, buf, amt)) {
				return (EFAULT);
			}
			printf("\tuser's buf:");
			for (i = 0; i < amt; i++) {
				printf(" 0x%x", buf[i]&0xff);
			}
			printf("\n");
		}
	}

	cmdblk[0] = (u_char)cmd;
	ucom->uscsi_cdb = cdb;
	if (g1) {
		FORMG1ADDR((union scsi_cdb *)cmdblk, blkno);
		FORMG1COUNT((union scsi_cdb *)cmdblk, count);
		ucom->uscsi_cdblen = CDB_GROUP1;
	} else {
		FORMG0ADDR((union scsi_cdb *)cmdblk, blkno);
		FORMG0COUNT((union scsi_cdb *)cmdblk, count);
		ucom->uscsi_cdblen = CDB_GROUP0;
	}
	ucom->uscsi_bufaddr = dcom->dkc_bufaddr;
	ucom->uscsi_buflen = dcom->dkc_buflen;

	if (dcom->dkc_flags & DK_SILENT) {
		ucom->uscsi_flags |= USCSI_SILENT;
	} else if (dcom->dkc_flags & DK_DIAGNOSE) {
		ucom->uscsi_flags |= USCSI_DIAGNOSE;
	} else if (dcom->dkc_flags & DK_ISOLATE) {
		ucom->uscsi_flags |= USCSI_ISOLATE;
	}

	return (sdioctl_cmd(dev, (caddr_t) ucom, SD_USCSI_CDB_KERNEL));
}

/*
 * Run a command for sdioctl.
 */

static int
sdioctl_cmd(dev, data, addr_flag)
dev_t dev;
caddr_t data;
int addr_flag;
{
	register struct scsi_device *devp;
	register struct buf *bp;
	register struct scsi_disk *un;
	int err = 0, flag, s;
	struct uscsi_cmd *scmd;
	char cmdblk[CDB_SIZE], *cdb = cmdblk;
	faultcode_t fault_err = -1;
	int blkno = 0;
	register caddr_t rb = 0;
	u_char cmd;

	devp = sdunits[SDUNIT(dev)];
	un = UPTR;
	scmd = (struct uscsi_cmd *)data;

	if (addr_flag & SD_USCSI_CDB_KERNEL) {
		cdb = scmd->uscsi_cdb;
	} else {
		if (copyin((caddr_t) scmd->uscsi_cdb, (caddr_t)cdb,
			(u_int) scmd->uscsi_cdblen)) {
			return (EFAULT);
		}
	}
	cmd = GETCMD((union scsi_cdb *)cdb);

	if (DEBUGGING) {
		int i;
		printf("%s%d: cdb:", DNAME, DUNIT);
		for (i = 0; i != scmd->uscsi_cdblen; i++) {
			printf(" 0x%x", (u_char)cdb[i]);
		}
		printf("\n");
	}

	if (scmd->uscsi_cdblen == CDB_GROUP0) {
		blkno = GETG0ADDR((union scsi_cdb *)cdb);
	} else if (scmd->uscsi_cdblen == CDB_GROUP1) {
		blkno = GETG1ADDR((union scsi_cdb *)cdb);
	} else if (scmd->uscsi_cdblen == CDB_GROUP5) {
		blkno = GETG5ADDR((union scsi_cdb *)cdb);
	} else {
		return (EINVAL);
	}
	flag = (scmd->uscsi_flags & USCSI_READ) ? B_READ : B_WRITE;


	if (DEBUGGING_ALL) {
		printf("sdioctl_cmd:  cmd= %x  blk= %x buflen= 0x%x\n",
		    cmd, blkno, scmd->uscsi_buflen);
		if (flag == B_WRITE && scmd->uscsi_buflen) {
			auto u_int i, amt = min(64, (u_int)scmd->uscsi_buflen);
			char bufarray[64], *buf = bufarray;
			if (addr_flag & SD_USCSI_BUF_KERNEL) {
				buf = scmd->uscsi_bufaddr;
			} else {
				if (copyin(scmd->uscsi_bufaddr,
				    (caddr_t) buf, amt)) {
					return (EFAULT);
				}
			}
			printf("user's buf:");
			for (i = 0; i < amt; i++) {
				printf(" 0x%x", buf[i] & 0xff);
			}
			printf("\n");
		}
	}

	/*
	 * Get buffer resources...
	 */

	bp = un->un_sbufp;
	s = splr(sdpri);
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
	un->un_soptions = 0;
	if ((scmd->uscsi_flags & USCSI_SILENT) && !(DEBUGGING_ALL)) {
		un->un_soptions |= USCSI_SILENT;
	}
	if (scmd->uscsi_flags & USCSI_ISOLATE)
		un->un_soptions |= USCSI_ISOLATE;
	if (scmd->uscsi_flags & USCSI_DIAGNOSE)
		un->un_soptions |= USCSI_DIAGNOSE;

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

	bp->b_forw = (struct buf *) cdb;
	bp->b_dev = dev;
	if ((bp->b_bcount = scmd->uscsi_buflen) > 0)
		bp->b_un.b_addr = scmd->uscsi_bufaddr;
	bp->b_blkno = blkno;

	/*
	 * XXX: I'm not sure but that if the buffer address is in kernel
	 * XXX: space, we might want to as_fault it in anyway (using
	 * XXX: kas instead of u.u_procp->p_as)
	 */

	if ((scmd->uscsi_buflen) && !(addr_flag & SD_USCSI_BUF_KERNEL)) {
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
	} else if ((cmd == SCMD_REASSIGN_BLOCK) &&
	    (addr_flag & SD_USCSI_CDB_KERNEL)) {
#define	SRBAMT	sizeof (struct scsi_reassign_blk)
		rb = IOPBALLOC(SRBAMT);
		if (rb == (caddr_t) 0) {
			err = ENOMEM;
		} else {
			struct scsi_reassign_blk *srb;
			bzero(rb, SRBAMT);
			srb = (struct scsi_reassign_blk *) rb;
			srb->reserved = 0;
			srb->length = 4;
			srb->defect = bp->b_blkno;
			/*
			 * The command is formatted as a Group 1
			 * command, although no such command exists.
			 * Transform it back into a Group 0 command.
			 */
			FORMG1ADDR((union scsi_cdb *)cdb, 0);
			((union scsi_cdb *)cdb)->scc_cmd &= ~SCMD_GROUP1;
			scmd->uscsi_cdblen = CDB_GROUP0;
			bp->b_un.b_addr = rb;
			bp->b_bcount = SRBAMT;
		}
	}

	/*
	 * If no errors, make and run the command.
	 */

	if (err == 0) {

		make_sd_cmd(devp, bp, SLEEP_FUNC);
		if (BP_PKT(bp) && u.u_error == 0) {
			sdstrategy(bp);

			s = splr(sdpri);
			while ((bp->b_flags & B_DONE) == 0) {
				(void) sleep((caddr_t) bp, PRIBIO);
			}
			(void) splx(s);
			err = geterror(bp);
		} else if (u.u_error == 0) {
			err = EFAULT;
		}

		/*
		 * get the status block
		 */

		scmd->uscsi_status = (int)SCBP_C(BP_PKT(bp));

		DPRINTF_IOCTL (devp, "sdioctl_cmd status is %0x%x",
		    scmd->uscsi_status);

		/*
		 * Release the resources.
		 */
		if (fault_err == 0) {
			(void) as_fault(u.u_procp->p_as, bp->b_un.b_addr,
			    (u_int)bp->b_bcount, F_SOFTUNLOCK,
			    (bp->b_flags&B_READ)? S_WRITE: S_READ);
			u.u_procp->p_flag &= ~SPHYSIO;
			bp->b_flags &= ~B_PHYS;
		}
		scsi_resfree(BP_PKT(bp));
		BP_PKT(bp) = NULL;
	}


	s = splr(sdpri);
	if (cmd == SCMD_REASSIGN_BLOCK && rb != (caddr_t) 0) {
		IOPBFREE (rb, SRBAMT);
	}
	if (bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= ~(B_BUSY|B_WANTED);
	un->un_soptions = 0;
	(void) splx(s);
	DPRINTF_IOCTL(devp, "returning %d from ioctl", err);
	return (err);
}

/*
 * Unit start and Completion
 */


static void
sdstart(devp)
register struct scsi_device *devp;
{
	register struct buf *bp;
	register struct scsi_disk *un = UPTR;
	register struct diskhd *dp = &un->un_utab;
	register struct scsi_pkt *pkt;

	if (dp->b_forw || (bp = dp->b_actf) == NULL) {
		return;
	}

	if (!(pkt = BP_PKT(bp))) {
		make_sd_cmd(devp, bp, sdrunout);
		if (!(pkt = BP_PKT(bp))) {
			/*
			 * XXX: actually, we should see whether or not
			 * XXX:	we could fire up a SCMD_SEEK operation
			 * XXX:	here. We may not have been able to go
			 * XXX:	because DVMA had run out, not because
			 * XXX:	we were out of command packets.
			 */
			New_state(un, SD_STATE_RWAIT);
			return;
		} else {
			New_state(un, SD_STATE_OPEN);
		}
	}

	dp->b_forw = bp;
	dp->b_actf = bp->b_actf;
	bp->b_actf = 0;

	/*
	 * Try and link this command with the one that
	 * is next, if it makes sense to do so (very rarely).
	 */
	if ((scsi_options & SCSI_OPTIONS_LINK) &&
	    (un->un_dp->options & SD_USELINKS) && dp->b_actf != NULL &&
	    bp != un->un_sbufp && un->un_wchkmap == 0 &&
	    sd_testlink(BP_PKT(bp)) == 0) {
		if (BP_PKT(dp->b_actf)) {
			sd_setlink(BP_PKT(bp));
		} else {
			make_sd_cmd(devp, dp->b_actf, NULL_FUNC);
			if (BP_PKT(dp->b_actf)) {
				sd_setlink(BP_PKT(bp));
			}
		}
	}

	/*
	 * b_resid now changes from disksort key to residue storage
	 */
	bp->b_resid = pkt->pkt_resid;
	pkt->pkt_resid = 0;

	if (pkt_transport(BP_PKT(bp)) != TRAN_ACCEPT) {
		sdlog(devp, LOG_ERR, "transport rejected");
		bp->b_flags |= B_ERROR;
		sddone(devp);
	} else if (dp->b_actf && !BP_PKT(dp->b_actf)) {
		make_sd_cmd(devp, dp->b_actf, NULL_FUNC);
	}
}


static int
sdrunout()
{
	register i, s = splr(sdpri);
	register struct scsi_device *devp;
	register struct scsi_disk *un;

	for (i = 0; i < SD_MAXUNIT; i++) {
		devp = sdunits[i];
		if (devp && devp->sd_present && (un = UPTR)) {
			if (un->un_state == SD_STATE_RWAIT) {
				sdstart(devp);
				if (un->un_state == SD_STATE_RWAIT) {
					(void) splx(s);
					return (0);
				}
			}
		}
	}
	(void) splx(s);
	return (1);
}

static void
sddone(devp)
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
		sdstart(devp);
	}

	if (bp != un->un_sbufp) {
		int flags = bp->b_flags;
		scsi_resfree(BP_PKT(bp));
		if ((DEBUGGING_ALL && bp->b_resid) || DEBUGGING) {
			sdprintf(devp, "regular done: resid %d", bp->b_resid);
		}

		/*
		 * Keep some statistics about the kinds of
		 * transfers going to/from this device.
		 */
		if ((flags & B_ERROR) == 0) {
			flags = bp->b_bcount - bp->b_resid;
			if (flags > 0 && flags < 2<<10)
				flags = SDS_512_2K;
			else if (flags < 4<<10)
				flags = SDS_2K_4K;
			else if (flags < 8<<10)
				flags = SDS_4K_8K;
			else if (flags < 16<<10)
				flags = SDS_8K_16K;
			else if (flags < 32<<10)
				flags = SDS_16K_32K;
			else if (flags < 64<<10)
				flags = SDS_32K_64K;
			else if (flags < 128<<10)
				flags = SDS_64K_128K;
			else
				flags = SDS_128K_PLUS;
			if (bp->b_flags & B_READ)
				un->un_sds.sds_rbins[flags]++;
			else
				un->un_sds.sds_wbins[flags]++;
			if (bp->b_flags & B_PAGEIO)
				un->un_sds.sds_npgio++;
			else if ((bp->b_flags & B_PHYS) == 0)
				un->un_sds.sds_nsysv++;
			flags = bp->b_flags;
		}
#ifdef	B_KLUSTER
		if (flags & B_KLUSTER) {
			if (flags & B_ERROR) {
				register struct buf *xbp;

				sdlog(devp, LOG_ERR,
				    "kluster error- retryings ops singly");

				/*
				 * Flip off the error bit
				 */

				bp->b_flags ^= B_ERROR;

				/*
				 * Bust apart the chain
				 */

				klustbust(bp);

				/*
				 * We could restore sort key for first request
				 * (other buffer's sort keys are still valid
				 * from first time through sdstrategy()), but
				 * that is an overoptimization...
				 */

				bp->b_resid = 0;

				/*
				 * re-sort each request back into the
				 * queue in order to retry them singly.
				 */

				xbp = bp;
				while (xbp) {
					bp = xbp->av_forw;
					xbp->av_forw = xbp->av_back = NULL;
					(void) klustsort(dp, xbp, 0);
					xbp = bp;
				}

				/*
				 * re-adjust max queue length by the decrement
				 * at the start of the routine above.
				 */

				sdstart(devp);

			} else {
				un->un_sds.sds_kluster++;
				dp->b_bcount -= klustdone(bp);
			}
		} else {
			dp->b_bcount--;
			iodone(bp);
		}
#else	/* B_KLUSTER */
		dp->b_bcount--;
		iodone(bp);
#endif	/* B_KLUSTER */
	} else {
		dp->b_bcount--;
		DPRINTF(devp, "special done resid %d", bp->b_resid);
		bp->b_flags |= B_DONE;
		wakeup((caddr_t) bp);
	}
}

static void
make_sd_cmd(devp, bp, func)
register struct scsi_device *devp;
register struct buf *bp;
int (*func)();
{
	auto int count, com;
	register struct scsi_pkt *pkt;
	register struct scsi_disk *un = UPTR;
	int tval = sd_io_time;
	int flags = (un->un_dp->options & SD_NOPARITY)? FLAG_NOPARITY : 0;


	if (bp != un->un_sbufp) {
		struct dk_map *lp = &un->un_map[SDPART(bp->b_dev)];
		long secnt;
		long resid;
		daddr_t blkno;

		/*
		 * Make sure we don't run off the end of a partition.
		 *
		 * Put this test here so that we can adjust b_count
		 * to accurately reflect the actual amount we are
		 * goint to transfer.
		 *
		 */

		secnt = (bp->b_bcount + (SECSIZE - 1)) >> SECDIV;
		count = MIN(secnt, lp->dkl_nblk - dkblock(bp));
		if (count != secnt) {
			/*
			 * We have an overrun
			 */
			resid = (secnt - count) << SECDIV;
			DPRINTF_ALL(devp, "overrun by %d sectors\n",
			    secnt - count);
			bp->b_bcount -= resid;
		} else {
			resid = 0;
		}

		if ((pkt = BP_PKT(bp)) == (struct scsi_pkt *) 0) {
			pkt = scsi_resalloc(ROUTE, CDB_GROUP1, 1,
			    (opaque_t)bp, func);
			if ((BP_PKT(bp) = pkt) == (struct scsi_pkt *) 0) {
				if (resid) {
					bp->b_bcount += resid;
				}
				return;
			}

			/*
			 * We can't store the resid in b_resid because
			 * disksort uses it for the sort key.  Thus we
			 * use pkt_resid temporarily.
			 */
			pkt->pkt_resid = resid;

			if (bp->b_flags & B_READ) {
				com = SCMD_READ;
			} else {
				com = SCMD_WRITE;
			}

		} else {
			if (bp->b_flags & B_READ) {
				com = SCMD_READ;
			} else if (pkt->pkt_cdbp[0] == SCMD_WRITE) {
				com = SCMD_VERIFY;
			} else {
				com = SCMD_WRITE;
			}
			bzero ((caddr_t) pkt->pkt_cdbp, (u_int) CDB_GROUP1);
		}

		/*
		 * restore b_count field if it had been changed.
		 */
		if (resid) {
			bp->b_bcount += resid;
		}

		blkno = dkblock(bp) +
		    (lp->dkl_cylno * un->un_g.dkg_nhead * un->un_g.dkg_nsect);

		/*
		 * XXX: This needs to be reworked based upon lbasize
		 */

		if (com == SCMD_VERIFY || blkno >= (2<<20) || count > 0xff) {
			com |= SCMD_GROUP1;
			makecom_g1(pkt, devp, flags, com, (int) blkno, count);
		} else {
			makecom_g0(pkt, devp, flags, com, (int) blkno, count);
		}
		pkt->pkt_pmon = un->un_dkn;
		if (DEBUGGING) {
			sdprintf(devp, "%s blk %d amt 0x%x (%d) resid %d",
			    scsi_cmd_decode((u_char) com, sd_cmds), blkno,
			    count << SECDIV, count << SECDIV, resid);
		}
	} else {
		caddr_t cdb;
		int cdblen;

		/*
		 * stored in bp->b_forw is a pointer to cdb
		 */

		cdb = (caddr_t) bp->b_forw;

		switch (GETCMD((union scsi_cdb *)cdb)) {
		case SCMD_FORMAT:
			tval = sd_fmt_time;
			break;
		}
		BP_PKT(bp) = NULL;

		switch (GETGROUP((union scsi_cdb *)cdb)) {
		case CDB_GROUPID_0:
			cdblen = CDB_GROUP0;
			break;
		case CDB_GROUPID_1:
			cdblen = CDB_GROUP1;
			break;
		case CDB_GROUPID_5:
			cdblen = CDB_GROUP5;
			break;
		default:
			sdlog(devp, LOG_ERR, "unknown group for special cmd");
			u.u_error = EINVAL;
			return;
		}

		pkt = scsi_resalloc(ROUTE, cdblen, 1,
		    (bp->b_bcount > 0)? (opaque_t) bp : (opaque_t) 0, func);
		if (pkt == (struct scsi_pkt *) NULL) {
			u.u_error = ENOMEM;
			return;
		}

		if (kcopy(cdb, (caddr_t) pkt->pkt_cdbp, (u_int)cdblen) != 0) {
			scsi_resfree(pkt);
			u.u_error = EFAULT;
			return;
		}
		pkt->pkt_pmon = (char) -1;
		pkt->pkt_flags = flags;
	}

	if ((scsi_options&SCSI_OPTIONS_DR) == 0 ||
	    (un->un_dp->options & SD_NODISC) != 0) {
		pkt->pkt_flags |= FLAG_NODISCON;
	}
	pkt->pkt_comp = sdintr;
	pkt->pkt_time = tval;
	pkt->pkt_private = (opaque_t) bp;
	BP_PKT(bp) = pkt;
}

/*
 * These two routines provide set and test for the
 * link bit in disk read or write cdb's
 */

static int
sd_testlink(pkt)
register struct scsi_pkt *pkt;
{
	switch (CDB_GROUPID(pkt->pkt_cdbp[0])) {
	case CDB_GROUPID_0:
		return ((CDBP(pkt)->g0_link == 0) ? 0: 1);
	case CDB_GROUPID_1:
		return ((CDBP(pkt)->g1_link != 0) ? 0: 1);
	default:
		return (0);
	}
}

static void
sd_setlink(pkt)
register struct scsi_pkt *pkt;
{
	switch (CDB_GROUPID(pkt->pkt_cdbp[0])) {
	case CDB_GROUPID_0:
		CDBP(pkt)->g0_link = 1;
		break;
	case CDB_GROUPID_1:
		CDBP(pkt)->g1_link = 1;
		break;
	default:
		break;
	}
	return;
}

/*
 * This routine called to see whether unit is (still) there. Must not
 * be called when un->un_sbufp is in use, and must not be called with
 * an unattached disk. Soft state of disk is restored to what it was
 * upon entry- up to caller to set the correct state.
 *
 *  Warning: bzero for non-sun4c's will crash on bootup sdopen.
 */

static int
sd_unit_ready(dev)
dev_t dev;
{
	struct scsi_device *devp = sdunits[SDUNIT(dev)];
	struct scsi_disk *un = UPTR;
	auto struct uscsi_cmd scmd, *com = &scmd;
	auto char cmdblk[CDB_GROUP0];
	register i;
	u_char state = un->un_last_state;

	New_state(un, SD_STATE_OPENING);
	com->uscsi_bufaddr = 0;
	com->uscsi_buflen = 0;
	cmdblk[0] = (char) SCMD_TEST_UNIT_READY;
	for (i = 1; i < CDB_GROUP0; i++)
		cmdblk[i] = 0;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_WRITE;
	com->uscsi_cdb = cmdblk;
	com->uscsi_cdblen = CDB_GROUP0;

	/*
	 * allow for trying it twice...
	 */

	if (sdioctl_cmd(dev, (caddr_t)com, SD_USCSI_CDB_KERNEL) &&
	    sdioctl_cmd(dev, (caddr_t)com, SD_USCSI_CDB_KERNEL)) {
		un->un_state = un->un_last_state;
		un->un_last_state = state;
		return (0);
	}

	un->un_state = un->un_last_state;
	un->un_last_state = state;
	return (1);
}

/*
 * Lock or Unlock the door for removable media devices
 */

static void
sd_lock_unlock(dev, flag)
dev_t dev;
int flag;
{
	struct scsi_device *devp = sdunits[SDUNIT(dev)];
	auto struct uscsi_cmd scmd, *com = &scmd;
	auto char cmdblk[CDB_GROUP0];
	register i;

	com->uscsi_bufaddr = 0;
	com->uscsi_buflen = 0;
	for (i = 0; i < CDB_GROUP0; i++)
		cmdblk[i] = 0;
	cmdblk[0] = SCMD_DOORLOCK;
	cmdblk[4] = flag;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_WRITE;
	com->uscsi_cdb = cmdblk;
	com->uscsi_cdblen = CDB_GROUP0;

	if (sdioctl_cmd(dev, (caddr_t)com, SD_USCSI_CDB_KERNEL)) {
		sdlog(devp, LOG_INFO, "%s media removal failed",
		    flag ? "prevent" : "allow");
	}
}

/*
 * Interrupt Service Routines
 */

static
sdrestart(arg)
caddr_t arg;
{
	struct scsi_device *devp = (struct scsi_device *) arg;
	register struct buf *bp;
	register s = splr(sdpri);

	bp = UPTR->un_utab.b_forw;
	if (bp) {
		register struct scsi_pkt *pkt;
		if (UPTR->un_state == SD_STATE_SENSING) {
			pkt = UPTR->un_rqs;
		} else {
			pkt = BP_PKT(bp);
		}
		bp->b_resid = 0;
		if (pkt_transport(pkt) != TRAN_ACCEPT) {
			sdlog(devp, LOG_ERR, "sdrestart transport failed");
			UPTR->un_state = UPTR->un_last_state;
			bp->b_resid = bp->b_bcount;
			bp->b_flags |= B_ERROR;
			sddone(devp);
		}
	}
	(void) splx(s);
}

/*
 * Command completion processing
 */

static void
sdintr(pkt)
struct scsi_pkt *pkt;
{
	register struct scsi_device *devp;
	register struct scsi_disk *un;
	register struct buf *bp;
	register action;


	bp = (struct buf *) pkt->pkt_private;
	devp = sdunits[SDUNIT(bp->b_dev)];
	un = UPTR;

	DPRINTF(devp, "sdintr");

	if (pkt->pkt_reason != CMD_CMPLT) {
		action = sd_handle_incomplete(devp);
	} else if (un->un_state == SD_STATE_SENSING) {
		/*
		 * We were running a REQUEST SENSE. Find out what to do next.
		 */
		Restore_state(un);
		pkt = BP_PKT(bp);
		action = sd_handle_sense(devp);
	} else {
		/*
		 * Okay, we weren't running a REQUEST SENSE. Call a routine
		 * to see if the status bits we're okay. As a side effect,
		 * clear state for this device, set non-error b_resid values,
		 * etc. If a request sense is to be run, that will happen by
		 * sd_check_error() returning a QUE_SENSE action.
		 */
		action = sd_check_error(devp);
	}

	/*
	 * If a WRITE command is completing, and we're s'posed to verify
	 * that it worked okay, we call make_sd_cmd() again here. make_sd_cmd()
	 * will note that we've already gotten resources and that the current
	 * command is a WRITE command that needs to be turned into a VERIFY
	 * command.
	 *
	 * If a VERIFY command fails with KEY_MISCOMPARE, sd_handle_sense
	 * will convert the VERIFY back to a WRITE (by calling make_sd_cmd()
	 * again) and retry the the WRITE (modulo retries being exhausted).
	 *
	 */

	if (action == COMMAND_DONE && bp != un->un_sbufp &&
	    (bp->b_flags & B_READ) == 0) {
		if (un->un_wchkmap & (1<<SDPART(bp->b_dev))) {
			if (pkt->pkt_cdbp[0] == SCMD_WRITE ||
			    pkt->pkt_cdbp[0] == SCMD_WRITE_G1) {
				action = QUE_COMMAND;
				make_sd_cmd(devp, bp, NULL_FUNC);
			}
		}
	}

	switch (action) {
	case COMMAND_DONE_ERROR:
error:
		un->un_err_severe = DK_FATAL;
		un->un_err_resid = bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		/* FALL THRU */
	case COMMAND_DONE:
		sddone(devp);
		break;
	case QUE_SENSE:
		New_state(un, SD_STATE_SENSING);
		un->un_rqs->pkt_private = (opaque_t) bp;
		bzero((caddr_t)devp->sd_sense, SENSE_LENGTH);
		if (pkt_transport(un->un_rqs) != TRAN_ACCEPT) {
			sdlog(devp, LOG_ERR,
			    "transport of request sense fails");
			Restore_state(un);
			goto error;
		}
		break;
	case QUE_COMMAND:
		if (pkt_transport(BP_PKT(bp)) != TRAN_ACCEPT) {
			sdlog(devp, LOG_ERR, "requeue of command fails");
			un->un_err_resid = bp->b_resid = bp->b_bcount;
			bp->b_flags |= B_ERROR;
			sddone(devp);
		}
		break;
	case JUST_RETURN:
		break;
	}
}


static int
sd_handle_incomplete(devp)
register struct scsi_device *devp;
{
	static char *fail = "SCSI transport failed: reason '%s': %s";
	static char *notresp = "disk not responding to selection";
	register rval = COMMAND_DONE_ERROR;
	register struct scsi_disk *un = UPTR;
	register struct buf *bp = un->un_utab.b_forw;
	register struct scsi_pkt *pkt = BP_PKT(bp);
	int be_chatty = (bp != un->un_sbufp || !(un->un_soptions & DK_SILENT));
	int perr = (pkt->pkt_statistics & STAT_PERR);

	if (un->un_state == SD_STATE_SENSING) {
		pkt = un->un_rqs;
		Restore_state(un);
	} else if (un->un_state == SD_STATE_DUMPING) {
		return (rval);
	}

	if (pkt->pkt_reason == CMD_TIMEOUT) {
		/*
		 * If the command timed out, we must assume that
		 * the target may still be running that command,
		 * so we should try and reset that target.
		 */
		if (scsi_reset(ROUTE, RESET_TARGET) == 0) {
			(void) scsi_reset(ROUTE, RESET_ALL);
		}

	} else if (pkt->pkt_reason == CMD_UNX_BUS_FREE) {

		/*
		 * If we had a parity error that caused the target to
		 * drop BSY*, don't be chatty about it.
		 */

		if (perr && be_chatty)
			be_chatty = 0;
	}

	/*
	 * If we were running a request sense, try it again if possible.
	 */

	if (pkt == un->un_rqs) {
		if (un->un_retry_ct++ < sd_retry_count) {
			rval = QUE_SENSE;
		}
	} else if (bp == un->un_sbufp && (un->un_soptions & DK_ISOLATE)) {
		rval = COMMAND_DONE_ERROR;
	} else if (un->un_retry_ct++ < sd_retry_count) {
		rval = QUE_COMMAND;
	}

	if (pkt->pkt_state == STATE_GOT_BUS && rval == COMMAND_DONE_ERROR) {
		/*
		 * Looks like someone turned off this shoebox.
		 */
		sdlog(devp, LOG_ERR, notresp);
		New_state(un, SD_STATE_DETACHING);
	} else if (be_chatty) {
		sdlog(devp, LOG_ERR, fail, scsi_rname(pkt->pkt_reason),
		    (rval == COMMAND_DONE_ERROR) ?
		    "giving up": "retrying command");
	}

	return (rval);
}


static int
sd_handle_sense(devp)
register struct scsi_device *devp;
{
	register struct scsi_disk *un = UPTR;
	register struct buf *bp = un->un_utab.b_forw;
	struct scsi_pkt *pkt = BP_PKT(bp), *rqpkt = un->un_rqs;
	register rval = COMMAND_DONE_ERROR;
	int severity, amt, i;
	char *p;
	static char *hex = " 0x%x";
	char *rq_err_msg;

	if (SCBP(rqpkt)->sts_busy) {
		sdlog (devp, LOG_ERR, "Busy Status on REQUEST SENSE");
		if (un->un_retry_ct++ < sd_retry_count) {
			timeout (sdrestart, (caddr_t)devp, SD_BSY_TIMEOUT);
			rval = JUST_RETURN;
		}
		return (rval);
	}

	if (SCBP_C(rqpkt)) {
		rq_err_msg = "Check Condition on REQUEST SENSE";
sense_failed:
		/*
		 * If the request sense failed, for any of a number
		 * of reasons, allow the original command to be
		 * retried.  Only log error on our last gasp.
		 */
		if (un->un_retry_ct++ < sd_retry_count) {
			un->un_err_severe = DK_NOERROR;
			rval = QUE_COMMAND;
		} else {
			if (rq_err_msg) {
				sdlog (devp, LOG_ERR, rq_err_msg);
			}
			un->un_err_severe = DK_FATAL;
			rval = COMMAND_DONE_ERROR;
		}
		return (rval);
	}

	amt = SENSE_LENGTH - rqpkt->pkt_resid;
	if ((rqpkt->pkt_state & STATE_XFERRED_DATA) == 0 || amt == 0) {
		rq_err_msg = "Request Sense couldn't get sense data";
		goto sense_failed;
	}

	/*
	 * Now, check to see whether we got enough sense data to make any
	 * sense out if it (heh-heh).
	 */

	if (amt < SUN_MIN_SENSE_LENGTH) {
		rq_err_msg = "Not enough sense information";
		goto sense_failed;
	}

	if (devp->sd_sense->es_class != CLASS_EXTENDED_SENSE) {
#ifdef	ADAPTEC
		if (un->un_dp->ctype == CTYPE_ACB4000) {
			sdintr_adaptec(devp);
		} else
#endif	/* ADAPTEC */
		{
			auto char tmp[8];
			auto char buf[128];
			p = (char *) devp->sd_sense;
			(void) strcpy(buf, "undecodable sense information:");
			for (i = 0; i < amt; i++) {
				(void) sprintf(tmp, hex, *(p++)&0xff);
				(void) strcpy(&buf[strlen(buf)], tmp);
			}
			sdlog(devp, LOG_ERR, buf);
			rq_err_msg = NULL;
			goto sense_failed;
		}
	}

	un->un_status = devp->sd_sense->es_key;
	un->un_err_code = devp->sd_sense->es_code;

	if (devp->sd_sense->es_valid) {
		un->un_err_blkno =	(devp->sd_sense->es_info_1 << 24) |
					(devp->sd_sense->es_info_2 << 16) |
					(devp->sd_sense->es_info_3 << 8)  |
					(devp->sd_sense->es_info_4);
	} else {
		/*
		 * With the valid bit not being set, we have
		 * to figure out by hand as close as possible
		 * what the real block number might have been
		 */
		un->un_err_blkno = bp->b_blkno;
		if (un->un_gvalid) {
			struct dk_map *lp = &un->un_map[SDPART(bp->b_dev)];
			un->un_err_blkno = dkblock(bp) +
			    (lp->dkl_cylno * un->un_g.dkg_nhead *
			    un->un_g.dkg_nsect);
		}
	}

	if (DEBUGGING_ALL || sd_error_level == SDERR_ALL) {
		printf("%s%d: cmd:", DNAME, DUNIT);
		p = pkt->pkt_cdbp;
		for (i = 0; i < CDB_SIZE; i++)
			printf(hex, *(p++)&0xff);
		p = (char *) devp->sd_sense;
		printf("\nsdata:");
		for (i = 0; i < amt; i++) {
			printf(hex, *(p++)&0xff);
		}
		printf("\n");
	}

	switch (un->un_status) {
	case KEY_NOT_READY:
		/*
		 * If we get a not-ready indication, wait a bit and
		 * try it again. Some drives pump this out for about
		 * 2-3 seconds after a reset.
		 */
		if (un->un_retry_ct++ < sd_retry_count) {
			un->un_err_severe = DK_NOERROR;
			severity = SDERR_RETRYABLE;
			timeout(sdrestart, (caddr_t)devp, SD_BSY_TIMEOUT);
			rval = JUST_RETURN;
		} else {
			un->un_err_severe = DK_FATAL;
			rval = COMMAND_DONE_ERROR;
			severity = SDERR_FATAL;
		}
		break;

	case KEY_NO_SENSE:
		if (un->un_retry_ct++ < sd_retry_count) {
			un->un_err_severe = DK_NOERROR;
			severity = SDERR_RETRYABLE;
			rval = QUE_COMMAND;
		} else {
			un->un_err_severe = DK_FATAL;
			rval = COMMAND_DONE_ERROR;
			severity = SDERR_FATAL;
		}
		break;

	case KEY_RECOVERABLE_ERROR:

		un->un_err_severe = DK_CORRECTED;
		severity = SDERR_RECOVERED;
		rval = COMMAND_DONE;
		break;

	case KEY_MEDIUM_ERROR:
		/*
		 * This really ought to be a fatal error, but we'll retry
		 * it upon a read, as some drives seem to report spurious
		 * media errors.
		 */
		if (bp != un->un_sbufp && (bp->b_flags & B_READ) != 0) {
			if (un->un_retry_ct++ < sd_retry_count) {
				rval = QUE_COMMAND;
				severity = SDERR_RETRYABLE;
				break;
			}
		}
		/* FALLTHROUGH */
	case KEY_HARDWARE_ERROR:
	case KEY_VOLUME_OVERFLOW:
	case KEY_WRITE_PROTECT:
	case KEY_BLANK_CHECK:
totally_bad:
		un->un_err_severe = DK_FATAL;
		severity = SDERR_FATAL;
		rval = COMMAND_DONE_ERROR;
		break;

	case KEY_ILLEGAL_REQUEST:
		/*
		 * If the command is a VERIFY command that the drive
		 * apparently doesn't support, whine to the console
		 * turn off any verification for this device, and mark
		 * the device as unable to support verification.
		 */

		if (pkt->pkt_cdbp[0] != SCMD_VERIFY) {
			goto totally_bad;
		}

		sdlog (devp, LOG_WARNING,
		    "drive does not support VERIFY command");
		sdlog (devp, LOG_WARNING, "disabling write check function");
		if (DEBUGGING_ALL) {
			sdprintf(devp, "Rel Block %d, Log Blk Count %d\n",
				bp->b_blkno, bp->b_bcount >> SECDIV);
			printf("cmd:");
			for (i = 0; i < CDB_GROUP1; i++)
				printf(" 0x%x", pkt->pkt_cdbp[i]);
			printf("\n");

		}

		un->un_wchkmap = 0;
		un->un_dp->options |= SD_NOVERIFY;
		rval = COMMAND_DONE;
		severity = SDERR_INFORMATIONAL;
		break;

	case KEY_MISCOMPARE:
		/*
		 * XXX: we'd better have just completed a VERIFY command
		 *
		 * In any case, make_sd_cmd will notice that this is
		 * a failed verify and flip the cdb over to be the
		 * write command again.
		 */
		if (un->un_retry_ct++ < sd_retry_count) {
			make_sd_cmd(devp, bp, NULL_FUNC);
			rval = QUE_COMMAND;
			severity = SDERR_RETRYABLE;
		} else {
			rval = COMMAND_DONE_ERROR;
			severity = SDERR_FATAL;
		}
		break;

	case KEY_ABORTED_COMMAND:
		severity = SDERR_RETRYABLE;
		rval = QUE_COMMAND;
		break;
	case KEY_UNIT_ATTENTION:
		rval = QUE_COMMAND;
		un->un_err_severe = DK_NOERROR;
		severity = SDERR_INFORMATIONAL;
		break;
	default:
		/*
		 * Undecoded sense key.  Try retries and hope
		 * that will fix the problem.  Otherwise, we're
		 * dead.
		 */
		sdlog (devp, LOG_ERR, "Unhandled Sense Key '%s",
		    sense_keys[un->un_status]);
		if (un->un_retry_ct++ < sd_retry_count) {
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
			sderrmsg(devp, severity);
		}
	} else if (DEBUGGING_ALL || severity >= sd_error_level) {
		sderrmsg(devp, severity);
	}
	return (rval);
}

static int
sd_check_error(devp)
register struct scsi_device *devp;
{
	register struct scsi_disk *un = UPTR;
	struct buf *bp = un->un_utab.b_forw;
	register struct scsi_pkt *pkt = BP_PKT(bp);
	register action;

	if (SCBP(pkt)->sts_busy) {
		int tval = SD_BSY_TIMEOUT;

		if (SCBP(pkt)->sts_scsi2) {
			/* Queue Full status */
			tval = 0;
		} else if (SCBP(pkt)->sts_is) {
			/*
			 * Implicit assumption here is that a device
			 * will only be reserved long enough to
			 * permit a single i/o operation to complete.
			 */
			sdlog(devp, LOG_WARNING, "reservation conflict");
			tval = sd_io_time * hz;
		}

		if (un->un_retry_ct++ < sd_retry_count) {
			if (tval)
				timeout(sdrestart, (caddr_t)devp, tval);
			action = JUST_RETURN;
		} else {
			sdlog(devp, LOG_ERR, "device busy too long");
			if (scsi_reset(ROUTE, RESET_TARGET)) {
				action = QUE_COMMAND;
			} else if (scsi_reset(ROUTE, RESET_ALL)) {
				action = QUE_COMMAND;
			} else {
				action = COMMAND_DONE_ERROR;
			}
		}
	} else if (SCBP(pkt)->sts_chk) {
		action = QUE_SENSE;
	} else {
		/*
		 * Get the low order bits of the command byte
		 */
		int com = GETCMD((union scsi_cdb *)pkt->pkt_cdbp);

		/*
		 * If the command is a read or a write, and we have
		 * a non-zero pkt_resid, that is an error. We should
		 * attempt to retry the operation if possible.
		 */
		action = COMMAND_DONE;
		if (pkt->pkt_resid && (com == SCMD_READ || com == SCMD_WRITE)) {
			if (un->un_retry_ct++ < sd_retry_count) {
				action = QUE_COMMAND;
			} else {
				/*
				 * As per 1037214, if we have exhausted retries
				 * a command with a residual is in error in
				 * this case.
				 */
				un->un_err_severe = DK_FATAL;
				action = COMMAND_DONE_ERROR;
			}
			sdlog(devp, LOG_ERR, "incomplete %s- %s",
			    (bp->b_flags & B_READ)? "read" : "write",
			    (action == QUE_COMMAND)? "retrying" :
			    "giving up");
			if (action == QUE_COMMAND)
				return (action);
		}

		/*
		 * pkt_resid will reflect, at this point, a residual
		 * of how many bytes left to be transferred there were
		 * from the actual scsi command.
		 *
		 * We only care about reporting this type of error
		 * in any but read or write commands. Since we have
		 * snagged any non-zero pkt_resids with read or writes
		 * above, all we have to do here is add pkt_resid to
		 * b_resid.
		 */

		bp->b_resid += pkt->pkt_resid;
		if (un->un_retry_ct != 0)
			un->un_err_severe = DK_RECOVERED;
		else
			un->un_err_severe = DK_NOERROR;
		un->un_retry_ct = 0;
	}
	return (action);
}

/*
 * Error Printing
 */

#ifdef	ADAPTEC
static u_char sd_adaptec_keys[] = {
	0, 4, 4, 4,  2, 4, 4, 4,	/* 0x00-0x07 */
	4, 4, 4, 4,  4, 4, 4, 4,	/* 0x08-0x0f */
	3, 3, 3, 3,  3, 3, 3, 1,	/* 0x10-0x17 */
	1, 1, 5, 5,  1, 1, 1, 1,	/* 0x18-0x1f */
	5, 5, 5, 5,  5, 5, 5, 5,	/* 0x20-0x27 */
	6, 6, 6, 6,  6, 6, 6, 6		/* 0x28-0x30 */
};
#define	MAX_ADAPTEC_KEYS (sizeof (sd_adaptec_keys))


static char *sd_adaptec_error_str[] = {
	"\000no sense",			/* 0x00 */
	"\001no index",			/* 0x01 */
	"\002no seek complete",		/* 0x02 */
	"\003write fault",		/* 0x03 */
	"\004drive not ready",		/* 0x04 */
	"\005drive not selected",	/* 0x05 */
	"\006no track 00",		/* 0x06 */
	"\007multiple drives selected",	/* 0x07 */
	"\010no address",		/* 0x08 */
	"\010no media loaded",		/* 0x09 */
	"\011end of media",		/* 0x0a */
	"\020I.D. CRC error",		/* 0x10 */
	"\021hard data error",		/* 0x11 */
	"\022no I.D. address mark",	/* 0x12 */
	"\023no data address mark",	/* 0x13 */
	"\024record not found",		/* 0x14 */
	"\025seek error",		/* 0x15 */
	"\026DMA timeout error",	/* 0x16 */
	"\027write protected",		/* 0x17 */
	"\030correctable data error",	/* 0x18 */
	"\031bad block",		/* 0x19 */
	"\032interleave error",		/* 0x1a */
	"\033data transfer failed",	/* 0x1b */
	"\034bad format",		/* 0x1c */
	"\035self test failed",		/* 0x1d */
	"\036defective track",		/* 0x1e */
	"\040invalid command",		/* 0x20 */
	"\041illegal block address",	/* 0x21 */
	"\042aborted",			/* 0x22 */
	"\043volume overflow",		/* 0x23 */
	NULL
};
#endif	/* ADAPTEC */

static char *sd_emulex_error_str[] = {
	"\001no index",			/* 0x01 */
	"\002seek incomplete",		/* 0x02 */
	"\003write fault",		/* 0x03 */
	"\004drive not ready",		/* 0x04 */
	"\005drive not selected",	/* 0x05 */
	"\006no track 0",		/* 0x06 */
	"\007multiple drives selected",	/* 0x07 */
	"\010lun failure",		/* 0x08 */
	"\011servo error",		/* 0x09 */
	"\020ID error",			/* 0x10 */
	"\021hard data error",		/* 0x11 */
	"\022no addr mark",		/* 0x12 */
	"\023no data field addr mark",	/* 0x13 */
	"\024block not found",		/* 0x14 */
	"\025seek error",		/* 0x15 */
	"\026dma timeout",		/* 0x16 */
	"\027recoverable error",	/* 0x17 */
	"\030soft data error",		/* 0x18 */
	"\031bad block",		/* 0x19 */
	"\032parameter overrun",	/* 0x1a */
	"\033synchronous xfer error",	/* 0x1b */
	"\034no primary defect list",	/* 0x1c */
	"\035compare error",		/* 0x1d */
	"\036recoverable error",	/* 0x1e */
	"\040invalid command",		/* 0x20 */
	"\041invalid block",		/* 0x21 */
	"\042illegal command",		/* 0x22 */
	"\044invalid cdb",		/* 0x24 */
	"\045invalid lun",		/* 0x25 */
	"\046invalid param list",	/* 0x26 */
	"\047write protected",		/* 0x27 */
	"\050media changed",		/* 0x28 */
	"\051reset",			/* 0x29 */
	"\052mode select changed",	/* 0x2a */
	NULL
};

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
	"\050read(10)",			/* 0x28 */
	"\052write(10)",		/* 0x2a */
	"\057verify",			/* 0x2f */
	"\067read defect data",		/* 0x37 */
	NULL
};


#ifdef	ADAPTEC
/*
 * Translate Adaptec non-extended sense status in to
 * extended sense format.  In other words, generate
 * sense key.
 */

static void
sdintr_adaptec(devp)
struct scsi_device *devp;
{
	register struct scsi_sense *s;

	/*
	 * Reposition failed block number for extended sense.
	 */
	s = (struct scsi_sense *) devp->sd_sense;
	devp->sd_sense->es_info_1 = 0;
	devp->sd_sense->es_info_2 = s->ns_lba_hi;
	devp->sd_sense->es_info_3 = s->ns_lba_mid;
	devp->sd_sense->es_info_4 = s->ns_lba_lo;

	/*
	 * Reposition error code for extended sense.
	 */
	devp->sd_sense->es_code = s->ns_code;

	/*
	 * Synthesize sense key for extended sense.
	 */
	if (s->ns_code < MAX_ADAPTEC_KEYS) {
		devp->sd_sense->es_key = sd_adaptec_keys[s->ns_code];
	}
}
#endif	/* ADAPTEC */

static void
sderrmsg(devp, level)
register struct scsi_device *devp;
int level;
{
	auto char buf[80];
	auto int lev;
	u_char com;
	register char **vec;
	register u_char code = devp->sd_sense->es_code;
	register struct scsi_disk *un = UPTR;
	register struct buf *bp = un->un_utab.b_forw;
	static char *error_classes[] = {
		"All", "Unknown", "Informational",
		"Recovered", "Retryable", "Fatal"
	};
	static char *s = "%s\n";
	static int map_to_syslog_levels[] = {
		LOG_ERR, LOG_ERR, LOG_INFO, LOG_WARNING, LOG_NOTICE, LOG_ERR
	};

	lev = map_to_syslog_levels[level];
	com = CDBP(BP_PKT(bp))->scc_cmd;

	(void) sprintf(buf, "sd%d%c:   ",
	    SDUNIT(bp->b_dev), SDPART(bp->b_dev) + 'a');
	(void) sprintf(&buf[8], "Error for command '%s'",
	    scsi_cmd_decode(com, sd_cmds));
	log(lev, s, buf);
	(void) sprintf(&buf[8], "Error Level: %s", error_classes[level]);
	log(lev, s, buf);
	com &= 0x1f;
	if (bp != un->un_sbufp || com == SCMD_READ || com == SCMD_WRITE) {
		(void) sprintf(&buf[8], "Block %d, Absolute Block: %d",
		    bp->b_blkno, un->un_err_blkno);
		log(lev, s, buf);
	}
	(void) sprintf(&buf[8], "Sense Key: %s",
	    sense_keys[devp->sd_sense->es_key]);
	log (lev, s, buf);
	if (code) {
		switch (un->un_dp->ctype) {
		default:
		case CTYPE_CCS:
			vec = (char **) NULL;
			break;
		case CTYPE_MD21:
			vec = sd_emulex_error_str;
			break;
#ifdef	ADAPTEC
		case CTYPE_ACB4000:
			vec = sd_adaptec_error_str;
			break;
#endif	/* ADAPTEC */
		}
		(void) strcpy(&buf[8], "Vendor '");
		inq_fill(devp->sd_inq->inq_vid, 8, &buf[strlen(buf)]);
		(void) sprintf(&buf[strlen(buf)],
		    "' error code: 0x%x", code);
		if (vec) {
			while (*vec != (char *) NULL) {
				if (code == *vec[0]) {
					(void) sprintf(&buf[strlen(buf)],
					    " (%s)", (char *)((int)(*vec)+1));
					break;
				} else
					vec++;
			}
		}
		log (lev, s, buf);
	}
}

/*
 * I don't use <varargs.h> 'coz I can't make it work on sparc!
 */

/*VARARGS2*/
static void
sdprintf(devp, fmt, a, b, c, d, e, f, g, h, i)
struct scsi_device *devp;
char *fmt;
int a, b, c, d, e, f, g, h, i;
{
	int tmp;
	char buf[128];

	(void) sprintf(buf, "sd%d:    ", DUNIT);
	(void) sprintf(&buf[8], fmt, a, b, c, d, e, f, g, h, i);
	tmp = strlen(buf);
	buf[tmp++] = '\n';
	buf[tmp] = 0;
	printf(buf);
}

/*VARARGS3*/
static void
sdlog(devp, level, fmt, a, b, c, d, e, f, g, h, i)
struct scsi_device *devp;
int level;
char *fmt;
int a, b, c, d, e, f, g, h, i;
{
	int tmp;
	char buf[128];

	(void) sprintf(buf, "sd%d:    ", DUNIT);
	(void) sprintf(&buf[8], fmt, a, b, c, d, e, f, g, h, i);
	tmp = strlen(buf);
	buf[tmp++] = '\n';
	buf[tmp] = 0;
	log(level, buf);
}

/*
 * Print a piece of inquiry data- cleaned up for non-printable characters
 * and stopping at the first space character after the beginning of the
 * passed string;
 */

static void
inq_fill(p, l, s)
char *p;
int l;
char *s;
{
	register unsigned i = 0, c;

	while (i++ < l) {
		if ((c = *p++) < ' ' || c >= 0177) {
			c = '*';
		} else if (i != 1 && c == ' ') {
			break;
		}
		*s++ = c;
	}
	*s++ = 0;
}
#endif	/* NSD > 0 */
