#ident	"@@(#)st.c 1.1 92/07/30 SMI"
#include "st.h"
#if	NST > 0
/*
 *
 *	SCSI Tape Driver
 *
 */

/*
 * Copyright (C) 1988, 1989, 1990, 1991 by Sun Microsystems, Inc.
 */


/*
 * the order of these defines is important
 */

#include <scsi/scsi.h>
#include <sys/file.h>
#include <sys/mtio.h>

#include <scsi/targets/stdef.h>

#ifndef	MT_DENSITY
#define	MT_DENSITY(dev)	((minor(dev) & MT_DENSITY_MASK) >> 3)
#endif

#ifndef STAT_BUS_RESET
#define	STAT_BUS_RESET	0x8	/* Command experienced a bus reset */
#define	STAT_DEV_RESET	0x10	/* Command experienced a device reset */
#define	STAT_ABORTED	0x20	/* Command was aborted */
#define	STAT_TIMEOUT	0x40	/* Command experienced a timeout */
#endif


#ifdef	OPENPROMS
#define	DRIVER		st_ops
#define	DNAME		devp->sd_dev->devi_name
#define	DUNIT		devp->sd_dev->devi_unit
#define	CNAME		devp->sd_dev->devi_parent->devi_name
#define	CUNIT		devp->sd_dev->devi_parent->devi_unit
#else	OPENPROMS
#define	DRIVER		stdriver
#define	DNAME		devp->sd_dev->md_driver->mdr_dname
#define	DUNIT		devp->sd_dev->md_unit
#define	CNAME		devp->sd_dev->md_driver->mdr_cname
#define	CUNIT		devp->sd_dev->md_mc->mc_ctlr
#endif	OPENPROMS

#define	UPTR		((struct scsi_tape *)(devp)->sd_private)
#define	SCBP(pkt)	((struct scsi_status *)(pkt)->pkt_scbp)
#define	CDBP(pkt)	((union scsi_cdb *)(pkt)->pkt_cdbp)
#define	ROUTE		(&devp->sd_address)
#define	BP_PKT(bp)	((struct scsi_pkt *)bp->av_back)

#define	Tgt(devp)	(devp->sd_address.a_target)
#define	Lun(devp)	(devp->sd_address.a_lun)

#define	IS_CLOSING(un)	((un)->un_state == ST_STATE_CLOSING || \
	((un)->un_state == ST_STATE_SENSING && \
		(un)->un_laststate == ST_STATE_CLOSING))

#define	ASYNC_CMD	0
#define	SYNC_CMD	1

/*
 * Macros for internal coding of count for SPACE command:
 *
 * Isfmk is 1 when spacing filemarks; 0 when spacing records:
 * bit 24 set indicates a space filemark command.
 * Fmk sets the filemark bit (24) and changes a backspace
 * count into a positive number with the sign bit set.
 * Blk changes a backspace count into a positive number with
 * the sign bit set.
 * space_cnt converts backwards counts to negative numbers.
 */

#define	Isfmk(x)	(((int)x & (1<<24)) != 0)
#define	Fmk(x)		((1<<24)|(((int)x & (1<<31)) ? ((-(int)x) | (1<<31)):\
			(int)x))
#define	Blk(x)		(((int)x & (1<<31))? ((-(int)x) | (1<<31)): (int)x)
#define	space_cnt(x)	((((int)x)&(1<<31))? (-(((int)x)&((1<<24)-1))):\
			((int)x)&((1<<24)-1))

/*
 * Debugging macros
 */


#define	DEBUGGING	((scsi_options & SCSI_DEBUG_TGT) || stdebug > 1)
#define	DEBUGGING_ALL	((scsi_options & SCSI_DEBUG_TGT) || stdebug)
#define	DPRINTF		if (DEBUGGING) stprintf
#define	DPRINTF_IOCTL	if (DEBUGGING || stdebug > 0) stprintf
#define	DPRINTF_ALL	DPRINTF_IOCTL

/*
 * Debugging turned on via conditional compilation switch -DST_DEBUG
 */


#ifdef  ST_DEBUG
int	st_debug = 0;
#define	ST_DEBUG_CMDS   0x01
#endif  ST_DEBUG

/*
 *
 * Global Data Definitions
 *
 */

extern struct st_drivetype st_drivetypes[];
extern int st_ndrivetypes, st_retry_count, st_io_time, st_space_time;
extern st_error_level;

/*
 *
 * Local Static Data
 *
 */

static int stdebug = 0;
static int stpri = 0;
struct scsi_device *stunits[ST_MAXUNIT];
struct scsi_address *stsvdaddr[ST_MAXUNIT];
static char *wrongtape =
	"%s%d: wrong tape for writing- use DC6150 tape (or equivalent)\n";


/*
 * Configuration Data
 */

#ifdef	OPENPROMS
/*
 * Device driver ops vector
 */
int stslave(), stattach(), stopen(), stclose(), stread(), stwrite();
int ststrategy(), stioctl();
extern int nulldev(), nodev();
struct dev_ops st_ops = {
	1,
	stslave,
	stattach,
	stopen,
	stclose,
	stread,
	stwrite,
	ststrategy,
	nodev,
	nulldev,
	stioctl
};
#else	OPENPROMS
/*
 * stdriver is used to call slave, attach routines from scsi_make_device
 */
int stslave(), stattach(), stopen(), stclose(), stread(), stwrite();
int ststrategy(), stioctl();
extern int nulldev(), nodev();
struct mb_driver stdriver = {
	nulldev, stslave, stattach, nodev, nodev, nulldev, 0, "st", 0, 0, 0, 0
};
#endif	OPENPROMS

/*
 *
 * Local Function Declarations
 *
 */

static void ststart(), stinit(), make_st_cmd(), stintr();
static void st_set_state(), sterrmsg(), stdone(), st_test_append();
static int strunout(), st_determine_generic();
static int stcmd();
static void stprintf();

/*
 *
 *	Autoconfiguration Routines
 *
 */

/*
 * stslave is called by scsi_make_device at boot time.
 * Returns 1 on success; -1 on failure; 0 if kernel resources unavailable.
 */

int
stslave(devp)
struct scsi_device *devp;
{
	int r;
	/*
	 * fill in our local array
	 */

	if (DUNIT >= ST_MAXUNIT)
		return (0);

	/* Save pointer to scsi_device struct allocated by scsi_make_device */
	stunits[DUNIT] = devp;

	/*
	 * Make sure priority is the maximum of all host adapters with
	 * tape devices so that spls properly block out other devices.
	 */
#ifdef	OPENPROMS
	stpri = MAX(stpri, ipltospl(devp->sd_dev->devi_intr->int_pri));
#else
	stpri = MAX(stpri, pritospl(devp->sd_dev->md_intpri));
#endif	OPENPROMS

	r = st_findslave(devp, 0);
	if (r < 0) {
		/*
		 * Save the address so we can scsi_add_device later
		 * (in stopen) if necessary.
		 */
		stsvdaddr[DUNIT] = (struct scsi_address *)
		    kmem_zalloc(sizeof (struct scsi_address));
		if (stsvdaddr[DUNIT]) {
			stunits[DUNIT] = 0;
			*stsvdaddr[DUNIT] = devp->sd_address;
		} else {
			r = 0;
		}
	}
	return (r);
}

/*
 * st_findslave is normally called by stslave, but may be called by stopen.
 * Returns 1 on success; -1 on failure; 0 if kernel resources unavailable.
 * Allocate buffers and initialize data structures.
 */

static int
st_findslave(devp, canwait)
register struct scsi_device *devp;
int canwait;
{
	struct scsi_pkt *rqpkt;
	struct scsi_tape *un;
	auto int (*f)() = (canwait == 0)? NULL_FUNC: SLEEP_FUNC;
	int rval = -1;

	/*
	 * Call the routine scsi_slave to do some of the dirty work.
	 * All scsi_slave does is do a TEST UNIT READY (and possibly
	 * a non-extended REQUEST SENSE or two), and an INQUIRY command.
	 * If the INQUIRY command succeeds, the field sd_inq in the
	 * device structure will be filled in.
	 *
	 */

	switch (scsi_slave(devp, canwait)) {
	default:
	case SCSIPROBE_NONCCS:
		/*
		 * If it returns as NON-CCS- we may have a tape drive
		 * that is so busy it cannot return INQUIRY data, hence
		 * we'll hang on to the driver node so that a later
		 * MUNIX-style autoconfiguration will spot it and attempt
		 * to open the drive again...
		 */
		rval = 0;
		/* FALLTHROUGH */
	case SCSIPROBE_NOMEM:
	case SCSIPROBE_NORESP:
	case SCSIPROBE_FAILURE:
		if (devp->sd_inq) {
			IOPBFREE (devp->sd_inq, SUN_INQSIZE);
			devp->sd_inq = (struct scsi_inquiry *) 0;
		}
		return (rval);
	case SCSIPROBE_EXISTS:
		if (devp->sd_inq->inq_dtype != DTYPE_SEQUENTIAL) {
			IOPBFREE (devp->sd_inq, SUN_INQSIZE);
			devp->sd_inq = (struct scsi_inquiry *) 0;
			return (rval);
		}
		break;
	}

	rval = 0;
	/* Allocate request sense buffer. */
	rqpkt = get_pktiopb(ROUTE, (caddr_t *)&devp->sd_sense, CDB_GROUP0,
			1, SENSE_LENGTH, B_READ, f);
	if (!rqpkt) {
		return (rval);
	}
	makecom_g0(rqpkt, devp, FLAG_NOPARITY,
	    SCMD_REQUEST_SENSE, 0, SENSE_LENGTH);
	rqpkt->pkt_pmon = -1;


	/*
	 * The actual unit is present.
	 * Now is the time to fill in the rest of our info..
	 */

	un = UPTR =
		(struct scsi_tape *) kmem_zalloc(sizeof (struct scsi_tape));
	if (!un) {
		printf("%s%d: no memory for tape structure\n", DNAME, DUNIT);
		free_pktiopb(rqpkt, (caddr_t)devp->sd_sense, SENSE_LENGTH);
		return (rval);
	}
	un->un_rbufp = (struct buf *) kmem_zalloc(sizeof (struct buf));
	un->un_sbufp = (struct buf *) kmem_zalloc(sizeof (struct buf));
	/* IOPBALLOC guarantees longword aligned allocation from iopbmap */
	un->un_mspl = (struct seq_mode *) IOPBALLOC(MSIZE);
	if (!un->un_rbufp || !un->un_sbufp || !un->un_mspl) {
		printf("%s%d: no memory for raw/special buffer\n",
			DNAME, DUNIT);
		if (un->un_rbufp)
			(void) kmem_free((caddr_t)un->un_rbufp,
				sizeof (struct buf));
		if (un->un_sbufp)
			(void) kmem_free((caddr_t)un->un_rbufp,
				sizeof (struct buf));
		if (un->un_mspl) {
			IOPBFREE(un->un_mspl, MSIZE);
		}
		(void) kmem_free((caddr_t) un, sizeof (struct scsi_tape));
		free_pktiopb(rqpkt, (caddr_t)devp->sd_sense, SENSE_LENGTH);
		return (rval);
	} else {
		rval = 1;
	}

	devp->sd_present = 1;
	un->un_fileno = -1;
	rqpkt->pkt_time = st_io_time;
	rqpkt->pkt_comp = stintr;
	un->un_rqs = rqpkt;
	un->un_sd = devp;
	un->un_dev = DUNIT;
#ifdef	OPENPROMS
	devp->sd_dev->devi_driver = &st_ops;
#else
	devp->sd_dev->md_dk = (short) -1;
#endif	OPENPROMS
	return (rval);
}

/*
 * Attach tape. The controller is there.
 * Called by scsi_make_device.
 * Attach device (boot time or load time, if loadable module).
 * Also called by stopen.
 */

int
stattach(devp)
struct scsi_device *devp;
{
	struct st_drivetype *dp;
	struct scsi_tape *un = UPTR;
	auto int found = 0;

	/*
	 * Determine type of tape controller.  Type is determined by
	 * checking the resulta of the earlier inquiry command and
	 * comparing vendor ids with strings in a table declared in stdef.h.
	 */

	for (dp = st_drivetypes; dp < &st_drivetypes[st_ndrivetypes]; dp++) {
		if (dp->length == 0)
			continue;
		if (bcmp(devp->sd_inq->inq_vid,  dp->vid, dp->length) == 0) {
			found = 1;
			break;
		}
	}

	if (!found) {
		found = (sizeof (struct st_drivetype)) + 44;
		dp = (struct st_drivetype *) kmem_zalloc((unsigned) found);
		if (!dp) {
			return;
		}
		dp->name = (caddr_t) ((u_long) dp + sizeof (*dp));
		/*
		 * Make up a name
		 */
		bcopy("Vendor '", dp->name, 8);
		bcopy((caddr_t) devp->sd_inq->inq_vid, &dp->name[8], 8);
		bcopy("' Product '", &dp->name[16], 11);
		bcopy((caddr_t) devp->sd_inq->inq_pid, &dp->name[27], 16);
		dp->name[43] = '\'';
		/*
		 * 'clean' vendor and product strings of non-printing chars
		 */
		for (found = 0; found < 43; found++) {
			if (dp->name[found] < 040 || dp->name[found] > 0176)
				dp->name[found] = '.';
		}
	}

	/* Store tape drive characteristics. */
	un->un_dp = dp;
	un->un_status = 0;
	un->un_attached = 1;
	if ((dp->options & ST_NOPARITY) == 0)
		un->un_rqs->pkt_flags &= ~FLAG_NOPARITY;
	printf("%s%d: <%s>\n", DNAME, DUNIT, dp->name);
}

/*
 *
 *
 * Regular Unix Entry points
 *
 *
 */

int
stopen(dev, flag)
dev_t dev;
int flag;
{
	int s, unit, err;
	u_int resid;
	register struct scsi_device *devp;
	register struct scsi_tape *un;

	/*
	 * validate that we are addressing a sensible unit
	 */
	unit = MTUNIT(dev);

	if (DEBUGGING_ALL) {
		printf("st%d:\tstopen dev 0x%x flag 0x%x\n", unit, dev, flag);
	}

	if (unit >= ST_MAXUNIT) {
		return (ENXIO);
	}
	if (!(devp = stunits[unit])) {
		struct scsi_address *sa = stsvdaddr[unit];
		if (sa) {
			if (scsi_add_device(sa, &DRIVER, "st", unit) == 0) {
				return (ENXIO);
			}
			/* stslave will have stuffed stunits */
			devp = stunits[unit];
			(void) kmem_free((caddr_t) sa, sizeof (*sa));
			stsvdaddr[unit] = (struct scsi_address *) 0;
		} else {
			return (ENXIO);
		}
	}

	if (!devp->sd_present || (!(un = UPTR))) {
		/*
		 *  If findslave succeeds, we'll get a valid UPTR...
		 */
		s = splr(stpri);
		if (st_findslave(devp, 1) <= 0) {
			(void) splx(s);
			return (ENXIO);
		}
		(void) splx(s);
		un = UPTR;
#ifdef	OPENPROMS
		printf("%s%d at %s%d target %d lun %d\n", DNAME, DUNIT,
				CNAME, CUNIT, Tgt(devp), Lun(devp));
#else	OPENPROMS
		printf("%s%d at %s%d slave %d\n", DNAME, DUNIT,
				CNAME, CUNIT, Tgt(devp)<<3|Lun(devp));
#endif	OPENPROMS
	}

	if (!un->un_attached) {
		s = splr(stpri);
		stattach(devp);
		(void) splx(s);
		if (!un->un_attached)
			return (ENXIO);
	}

	/*
	 * Check for the case of the tape in the middle of closing.
	 * This isn't simply a check of the current state, because
	 * we could be in state of sensing with the previous state
	 * that of closing.
	 */

	s = splr(stpri);
	if (IS_CLOSING(un)) {
		while (IS_CLOSING(un)) {
			if (sleep((caddr_t) &lbolt, (PZERO+1)|PCATCH)) {
				(void) splx(s);
				return (EINTR);
			}
		}
	} else if (un->un_state != ST_STATE_CLOSED) {
		(void) splx(s);
		return (EBUSY);
	}
	(void) splx(s);

	/*
	 * record current dev
	 */

	un->un_dev = dev;


	/*
	 * See whether this is a generic device that we haven't figured
	 * anything out about yet.
	 */

	if (un->un_dp->type == ST_TYPE_INVALID) {
		if (st_determine_generic(devp))
			return (EIO);
	}

	/*
	 * Clean up after any errors left by 'last' close.
	 * This also handles the case of the initial open.
	 */

	un->un_laststate = un->un_state;
	un->un_state = ST_STATE_OPENING;

	resid = un->un_err_resid;	/* save resid for status reporting */
	err = stcmd(devp, SCMD_TEST_UNIT_READY, 0, SYNC_CMD);
	un->un_err_resid = resid;	/* restore resid */

	if (err != 0) {
		if (err == EINTR) {
			un->un_laststate = un->un_state;
			un->un_state = ST_STATE_CLOSED;
			return (err);
		}
		/*
		 * Make sure the tape is ready
		 */
		un->un_fileno = -1;
		if (un->un_status != KEY_UNIT_ATTENTION) {
			un->un_laststate = un->un_state;
			un->un_state = ST_STATE_CLOSED;
			return (EIO);
		}
	}

	resid = un->un_err_resid;	/* save resid for status reporting */
	if (un->un_fileno < 0 && st_loadtape(devp)) {
		un->un_err_resid = resid;	/* restore resid */
		un->un_laststate = un->un_state;
		un->un_state = ST_STATE_CLOSED;
		return (EIO);
	}

	/*
	 * do a mode sense to pick up state of current write-protect,
	 */
	(void) stcmd(devp, SCMD_MODE_SENSE, MSIZE, SYNC_CMD);

	/*
	 * If we are opening the tape for writing, check
	 * to make sure that the tape can be written.
	 */

	if (flag & FWRITE) {
		err = 0;
		if (un->un_mspl->wp)  {
			un->un_status = KEY_WRITE_PROTECT;
			un->un_laststate = un->un_state;
			un->un_state = ST_STATE_CLOSED;
			/* restore resid for status reporting */
			un->un_err_resid = resid;
			return (EACCES);
		} else {
			un->un_read_only = 0;
		}
	} else {
		un->un_read_only = 1;
	}

	/*
	 * If we're opening the tape write-only, we need to
	 * write 2 filemarks on the HP 1/2 inch drive, to
	 * create a null file.
	 */
	if (flag == FWRITE && un->un_dp->options & ST_REEL)
		un->un_fmneeded = 2;
	else if (flag == FWRITE)
		un->un_fmneeded = 1;
	else
		un->un_fmneeded = 0;

	/*
	 * Now this is sort of stupid, but we need to check
	 * whether or not, independent of doing any I/O,
	 * whether or not we can set the requested density
	 * unless the device being opened is the 'generic'
	 * device open (the 'lowest' density) device.
	 *
	 * If we can, fine. If not, we have to bounce the
	 * open with EIO. We use the B_WRITE flag here
	 * becuase in this case we are not searching for
	 * the right density- we are taking the user
	 * specified density and seeing whether or not
	 * setting that density (for writing) would
	 * work.
	 *
	 * Later on this might get done again.
	 */

	if ((un->un_fileno < 0 || (un->un_fileno == 0 && un->un_blkno == 0)) &&
	    MT_DENSITY(dev) != 0) {
		if (st_determine_density(devp, B_WRITE)) {
			/* restore resid for status reporting */
			un->un_err_resid = resid;
			un->un_status = KEY_ILLEGAL_REQUEST;
			un->un_laststate = un->un_state;
			un->un_state = ST_STATE_CLOSED;
			return (EIO);
		}
		/*
		 * Destroy the knowledge that we have 'determined'
		 * density so that a later read at BOT comes along
		 * does the right density determination.
		 */
		un->un_density_known = 0;
	}

	/* restore resid for status reporting */
	un->un_err_resid = resid;

	/*
	 * Okay, the tape is loaded and either at BOT or somewhere past.
	 * Mark the state such that any I/O or tape space operations
	 * will get/set the right density, etc..
	 */

	un->un_laststate = un->un_state;
	un->un_state = ST_STATE_OPEN_PENDING_IO;

	/*
	 *  Set test append flag if writing.
	 *  First write must check that tape is positioned correctly.
	 */
	un->un_test_append = (flag & FWRITE);

	return (0);
}


int
stclose(dev, flag)
dev_t dev;
int flag;
{
	int err = 0;
	register struct scsi_device *devp;
	register struct scsi_tape *un;
	int unit, norew, count;

	unit = MTUNIT(dev);

	if (DEBUGGING_ALL) {
		printf("st%d:\tstclose dev 0x%x flag %d\n", unit, dev, flag);
	}

	/*
	 * validate arguments
	 */
	if ((unit = MTUNIT(dev)) >= ST_MAXUNIT) {
		return (ENXIO);
	}
	if (!(devp = stunits[unit]) || !(un = UPTR)) {
		return (ENXIO);
	}

	/*
	 * a close causes a silent span to the next file if we've hit
	 * an EOF (but not yet read across it).
	 */

	if (un->un_eof == ST_EOF) {
		if (un->un_fileno >= 0) {
			un->un_fileno++;
			un->un_blkno = 0;
		}
		un->un_eof = ST_NO_EOF;
	}

	/*
	 * set state to indicate that we are in process of closing
	 */

	un->un_laststate = un->un_state;
	un->un_state = ST_STATE_CLOSING;

	/*
	 * rewinding?
	 */

	norew = (minor(dev) & MT_NOREWIND);
	/*
	 * For performance reasons (HP 88780), the driver should
	 * postpone writing the second tape mark until just before a file
	 * positioning ioctl is issued (e.g., rewind).  This means that
	 * the user must not manually rewind the tape because the tape will
	 * be missing the second tape mark which marks EOM.
	 * However, this small performance improvement is not worth the risk.
	 */

	/*
	 * We need to back up over the filemark we inadvertently popped
	 * over doing a read in between the two filemarks that constitute
	 * logical eot for 1/2" tapes. Note that ST_EOT_PENDING is only
	 * set while reading.
	 *
	 * If we happen to be at physical eot (ST_EOM) (writing case),
	 * the writing of filemark(s) will clear the ST_EOM state, which
	 * we don't want, so we save this state and restore it later.
	 */

	if (un->un_eof == ST_EOT_PENDING) {
		if (norew) {
			if (stcmd(devp, SCMD_SPACE, Fmk((-1)), SYNC_CMD)) {
				err = EIO;
			} else {
				un->un_blkno = 0;
				un->un_eof = ST_EOT;
			}
		} else {
			un->un_eof = ST_NO_EOF;
		}
	} else if (((flag & FWRITE) &&
	    (un->un_lastop == ST_OP_WRITE || un->un_fmneeded > 0)) ||
	    ((flag == FWRITE) && (un->un_lastop == ST_OP_NIL))) {
		/* save ST_EOM state */
		int was_at_eom = (un->un_eof == ST_EOM)? 1: 0;
		/*
		 * Do we need to write a file mark?
		 *
		 * Note that we will write a filemark if we had opened
		 * the tape write only and no data was written, thus
		 * creating a null file.
		 *
		 * For HP (1/2 inch tape), un_fmneeded tells us how many
		 * filemarks need to be written out.  If the user
		 * already wrote one, we only have to write one more.
		 * If they wrote two, we don't have to write any.
		 */

		count = (un->un_dp->options & ST_REEL) ? un->un_fmneeded : 1;
		if (count > 0) {
			if (stcmd(devp, SCMD_WRITE_FILE_MARK,
			    count, SYNC_CMD)) {
				err = EIO;
			}
			if ((un->un_dp->options & ST_REEL) && norew) {
				if (stcmd(devp, SCMD_SPACE, Fmk((-1)),
				    SYNC_CMD)) {
					err = EIO;
				}
				un->un_eof = ST_NO_EOF;
				/* fix up block number */
				un->un_blkno = 0;
			}
		}

		/*
		 * If we aren't going to be rewinding, and we were at
		 * physical eot, restore the state that indicates we
		 * are at physical eot. Once you have reached physical
		 * eot, and you close the tape, the only thing you can
		 * do on the next open is to rewind. Access to trailer
		 * records is only allowed without closing the device.
		 */
		if (norew == 0 && was_at_eom)
			un->un_eof = ST_EOM;
	}

	/*
	 * Do we need to rewind? Can we rewind?
	 */

	if (norew == 0 && un->un_fileno >= 0 && err == 0) {
		/*
		 * We'd like to rewind with the
		 * 'immediate' bit set, but this
		 * causes problems on some drives
		 * where subsequent opens get a
		 * 'NOT READY' error condition
		 * back while the tape is rewinding,
		 * which is impossible to distinguish
		 * from the condition of 'no tape loaded'.
		 *
		 * Also, for some targets, if you disconnect
		 * with the 'immediate' bit set, you don't
		 * actually return right away, i.e., the
		 * target ignores your request for immediate
		 * return.
		 *
		 * Instead, we'll fire off an async rewind
		 * command. We'll mark the device as closed,
		 * and any subsequent open will stall on
		 * the first TEST_UNIT_READY until the rewind
		 * completes.
		 *
		 */
		(void) stcmd(devp, SCMD_REWIND, 0, ASYNC_CMD);
	}

	/*
	 * clear up state
	 */

	un->un_laststate = un->un_state;
	un->un_state = ST_STATE_CLOSED;
	un->un_lastop = ST_OP_NIL;

	/*
	 * any kind of error on closing causes all state to be tossed
	 */

	if (err && un->un_status != KEY_ILLEGAL_REQUEST) {
		un->un_density_known = 0;
		/* stintr already set un_fileno to -1 */
	}

	return (err);
}


/*
 * These routines perform raw i/o operations.
 */

int
stread(dev, uio)
dev_t dev;
struct uio *uio;
{
	return (strw(dev, uio, B_READ));
}

int
stwrite(dev, uio)
dev_t dev;
struct uio *uio;
{
	return (strw(dev, uio, B_WRITE));
}

/*
 * Perform max. record blocking.  For variable-length devices:
 * if greater than 64KB -1, block into 64 KB -2 requests; otherwise,
 * let it through unmodified.
 * For fixed-length record devices: 63K is max (default minphys).
 */
static void
stminphys(bp)
struct buf *bp;
{
	struct scsi_device *devp = stunits[MTUNIT(bp->b_dev)];

	if (UPTR->un_dp->options & ST_VARIABLE) {
		if (bp->b_bcount > ST_MAXRECSIZE_VARIABLE)
			bp->b_bcount = ST_MAXRECSIZE_VARIABLE_LIMIT;
	} else {
		if (bp->b_bcount > ST_MAXRECSIZE_FIXED)
			bp->b_bcount = ST_MAXRECSIZE_FIXED;
	}

}

static int
strw(dev, uio, flag)
dev_t dev;
struct uio *uio;
int flag;
{
	struct scsi_device *devp;
	struct scsi_tape *un;
	register int rval, len = uio->uio_iov->iov_len;

	if (MTUNIT(dev) >= ST_MAXUNIT) {
		return (ENXIO);
	}

	devp = stunits[MTUNIT(dev)];
	un = UPTR;

	if ((un->un_dp->options & ST_VARIABLE) == 0) {
		if (uio->uio_iov->iov_len & (un->un_dp->bsize-1)) {
			printf("%s%d:  %s: not modulo %d block size\n",
			    DNAME, DUNIT, (flag == B_WRITE) ? "write": "read",
			    un->un_dp->bsize);
			return (EINVAL);
		}
	}

	rval = physio(ststrategy, un->un_rbufp, dev, flag, stminphys, uio);
	/*
	 * if we hit logical EOT during this xfer and there is not a
	 * full residue, then set un_eof back  to ST_EOM to make sure that
	 * the user will see at least one zero write
	 * after this short write
	 */
	if (un->un_eof == ST_WRITE_AFTER_EOM &&
	    uio->uio_resid != len)
		un->un_eof = ST_EOM;
	return (rval);
}


int
ststrategy(bp)
register struct buf *bp;
{
	register s;
	register struct scsi_device *devp;
	register struct scsi_tape *un;
	int unit = MTUNIT(bp->b_dev);

	/*
	 * validate arguments
	 */

	devp = stunits[unit];
	if (!devp || !(un = UPTR)) {
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		bp->b_error = ENXIO;
		iodone(bp);
		return;
	}

	if (bp != un->un_sbufp) {
		char reading = bp->b_flags & B_READ;
		int wasopening = 0;

		/*
		 * Check for legal operations
		 */
		if (un->un_fileno < 0) {
			DPRINTF(devp, "strategy with un->un_fileno < 0");
			goto b_done_err;
		}


		/*
		 * Process this first. If we were reading, and we're pending
		 * logical eot, that means we've bumped one file mark too far.
		 */

		if (un->un_eof == ST_EOT_PENDING) {
			if (stcmd(devp, SCMD_SPACE, Fmk((-1)), SYNC_CMD)) {
				un->un_fileno = -1;
				un->un_density_known = 0;
				goto b_done_err;
			}
			un->un_blkno = 0; /* fix up block number.. */
			un->un_eof = ST_EOT;
		}

		/*
		 * If we are in the process of opening, we may have to
		 * determine/set the correct density. We also may have
		 * to do a test_append (if QIC) to see whether we are
		 * in a position to append to the end of the tape.
		 *
		 * If we're already at logical eot, we transition
		 * to ST_NO_EOF. If we're at physical eot, we punt
		 * to the switch statement below to handle.
		 */

		if ((un->un_state == ST_STATE_OPEN_PENDING_IO) ||
		    (un->un_test_append && (un->un_dp->options & ST_QIC))) {

			if (un->un_state == ST_STATE_OPEN_PENDING_IO) {
				if (st_determine_density(devp, (int) reading)) {
					goto b_done_err;
				}
			}

			DPRINTF(devp,
			    "pending_io@fileno %d rw %d qic %d eof %d",
			    un->un_fileno, (int) reading,
			    (un->un_dp->options & ST_QIC)? 1: 0,
			    un->un_eof);

			if (!reading && un->un_eof != ST_EOM) {
				if (un->un_eof == ST_EOT) {
					un->un_eof = ST_NO_EOF;
				} else if (un->un_fileno > 0 &&
				    (un->un_dp->options & ST_QIC)) {
					/*
					 * st_test_append() will do it all
					 */
					st_test_append(bp);
					return;
				}
			}
			if (un->un_state == ST_STATE_OPEN_PENDING_IO)
				wasopening = 1;
			un->un_laststate = un->un_state;
			un->un_state = ST_STATE_OPEN;
		}


		/*
		 * Process rest of END OF FILE and END OF TAPE conditions
		 */

		DPRINTF(devp, "un_eof=%x, wasopening=%x",
		    un->un_eof, wasopening);

		switch (un->un_eof) {
		case ST_EOM:
			/*
			 * This allows writes to proceed past physical
			 * eot. We'll *really* be in trouble if the
			 * user continues blindly writing data too
			 * much past this point (unwind the tape).
			 * Physical eot really means 'early warning
			 * eot' in this context.
			 *
			 * Every other write from now on will succeed
			 * (if sufficient  tape left).
			 * This write will return with resid == count
			 * but the next one should be successful
			 *
			 * Note that we only transition to logical EOT
			 * if the last state wasn't the OPENING state.
			 * We explicitly prohibit running up to physical
			 * eot, closing the device, and then re-opening
			 * to proceed. Trailer records may only be gotten
			 * at by keeping the tape open after hitting eot.
			 *
			 * Also note that ST_EOM cannot be set by reading-
			 * this can only be set during writing. Reading
			 * up to the end of the tape gets a blank check
			 * or a double-filemark indication (ST_EOT_PENDING),
			 * and we prohibit reading after that point.
			 *
			 */
			DPRINTF(devp, "EOM");
			if (wasopening == 0)
				/*
				 * this allows strw() to reset it back to
				 * ST_EOM to make sure that the application
				 * will see a zero write
				 */
				un->un_eof = ST_WRITE_AFTER_EOM;
			un->un_status = SUN_KEY_EOT;
			goto b_done;

		case ST_WRITE_AFTER_EOM:
		case ST_EOT:
			DPRINTF(devp, "EOT");
			un->un_status = SUN_KEY_EOT;
			if (reading) {
				goto b_done;
			}
			un->un_eof = ST_NO_EOF;
			break;

		case ST_EOF_PENDING:
			DPRINTF(devp, "EOF PENDING");
		case ST_EOF:
			DPRINTF(devp, "EOF");
			un->un_status = SUN_KEY_EOF;
			un->un_eof = ST_NO_EOF;
			un->un_fileno += 1;
			un->un_blkno = 0;
			if (reading) {
				DPRINTF(devp, "now file %d (read)",
				    un->un_fileno);
				goto b_done;
			}
			DPRINTF(devp, "now file %d (write)", un->un_fileno);
			break;
		default:
			un->un_status = 0;
			break;
		}
	}

	bp->b_flags &= ~(B_DONE|B_ERROR);
	bp->av_forw = 0;
	bp->b_resid = 0;

	if (bp != un->un_sbufp) {
		BP_PKT(bp) = 0;
	}
	s = splr(stpri);
	if (un->un_quef) {
		un->un_quel->av_forw = bp;
	} else
		un->un_quef = bp;
	un->un_quel = bp;
	if (un->un_runq == NULL)
		ststart(devp);
	(void) splx(s);
	return;

b_done_err:
	bp->b_flags |= B_ERROR;
	bp->b_error = EIO;
b_done:
	un->un_err_resid = bp->b_resid = bp->b_bcount;
	iodone(bp);
}


/*
 * this routine spaces forward over filemarks
 * XXX space_fmks and find_eom should be merged
 */
static int
space_fmks(dev, count)
dev_t dev;
int count;
{
	register struct scsi_device *devp;
	register struct scsi_tape *un;
	int rval = 0;

	if ((devp = stunits[MTUNIT(dev)]) == 0) {
		return (ENXIO); /* can't happen */
	}
	un = UPTR;

	/*
	 * the risk with doing only one space operation is that we
	 * may accidentily jump in old data
	 */
	if (un->un_dp->options & ST_KNOWS_EOD) {
		if (stcmd(devp, SCMD_SPACE, Fmk(count), SYNC_CMD)) {
			rval = EIO;
		}
	} else {
		while (count > 0) {
			if (stcmd(devp, SCMD_SPACE, Fmk(1), SYNC_CMD)) {
				rval = EIO;
				break;
			}
			count -= 1;
			/*
			 * read a block to see if we have reached
			 * end of medium (double filemark for reel or
			 * medium error for others)
			 */
			if (count > 0) {
				if (stcmd(devp, SCMD_SPACE, Blk(1),
				    SYNC_CMD)) {
					rval = EIO;
					break;
				}
				if (un->un_eof >= ST_EOF_PENDING) {
					un->un_status = SUN_KEY_EOT;
					rval = EIO;
					break;
				}
			}
		}
		un->un_err_resid = count;
	}
	return (rval);
}




/*
 * this routine spaces to to EOM
 * it keeps track of the current filenumber and returns the
 * filenumber after the last successful space operation
 */
#define	MAX_SKIP	0x1000 /* somewhat arbitrary */

static int
find_eom(dev)
dev_t dev;
{
	register struct scsi_device *devp;
	register struct scsi_tape *un;
	int count, savefile = -1;


	if ((devp = stunits[MTUNIT(dev)]) == 0)
		return (savefile); /* this can't happen */
	un = UPTR;
	savefile = un->un_fileno;

	/*
	 * see if the drive is smart enough to do the skips in
	 * one operation; 1/2" use two filemarks
	 */
	if (un->un_dp->options & ST_KNOWS_EOD)
		count = MAX_SKIP;
	else
		count = 1;

	while (stcmd(devp, SCMD_SPACE, Fmk(count), SYNC_CMD) == 0) {
		savefile = un->un_fileno;
		/*
		 * If we're not EOM smart,  space a record
		 * to see whether we're now in the slot between
		 * the two sequential filemarks that logical
		 * EOM consists of (REEL) or hit nowhere land
		 * (8mm).
		 */
		if ((un->un_dp->options & ST_KNOWS_EOD) == 0) {
			if (stcmd(devp, SCMD_SPACE, Blk((1)), SYNC_CMD))
				break;
			else if (un->un_eof >= ST_EOF_PENDING) {
				un->un_status = KEY_BLANK_CHECK;
				un->un_fileno++;
				un->un_blkno = 0;
				break;
			}
		}
		else
			savefile = un->un_fileno;
	}

	if (un->un_dp->options & ST_KNOWS_EOD) {
		savefile = un->un_fileno;
	}
	DPRINTF_IOCTL(devp, "find_eom: %x\n", savefile);
	return (savefile);
}


/*
 * this routine is frequently used in ioctls below;
 * it determines whether we know the density and if not will
 * determine it
 * if we have written the tape before, one or more filemarks are written
 *
 * depending on the stepflag, the head is repositioned to where it was before
 * the filemarks were written in order not to confuse step counts
 */
#define	STEPBACK    0
#define	NO_STEPBACK 1

static int
check_density_or_wfm(dev, wfm, mode, stepflag)
dev_t dev;
int wfm, mode, stepflag;
{
	register struct scsi_device *devp;
	register struct scsi_tape *un;


	if ((devp = stunits[MTUNIT(dev)]) == 0)
		return (ENXIO);
	un = UPTR;


	/*
	 * If we don't yet know the density of the tape we have inserted,
	 * we have to either unconditionally set it (if we're 'writing'),
	 * or we have to determine it. As side effects, check for any
	 * write-protect errors, and for the need to put out any file-marks
	 * before positioning a tape.
	 *
	 * If we are going to be spacing forward, and we haven't determined
	 * the tape density yet, we have to do so now...
	 */
	if (un->un_state == ST_STATE_OPEN_PENDING_IO) {
		if (st_determine_density(devp, mode)) {
			return (EIO);
		}
		/*
		 * Presumably we are at BOT. If we attempt to write, it will
		 * either work okay, or bomb. We don't do a st_test_append
		 * unless we're past BOT.
		 */
		un->un_laststate = un->un_state;
		un->un_state = ST_STATE_OPEN;
	} else if (un->un_fmneeded > 0 ||
			(un->un_lastop == ST_OP_WRITE && wfm)) {
		int blkno = un->un_blkno;
		int fileno = un->un_fileno;

		/*
		 * We need to write one or two filemarks.
		 * In the case of the HP, we need to
		 * position the head between the two
		 * marks.
		 */
		if (un->un_fmneeded > 0) {
			wfm = un->un_fmneeded;
			un->un_fmneeded = 0;
		}
		if (st_write_fm(dev, wfm)) {
			un->un_fileno = -1;
			un->un_density_known = 0;
			return (EIO);
		}
		if (stepflag == STEPBACK) {
			if (stcmd(devp, SCMD_SPACE, Fmk((-wfm)), SYNC_CMD)) {
				return (EIO);
			}
			un->un_blkno = blkno;
			un->un_fileno = fileno;
		}
	}
	/*
	 * Whatever we do at this point clears the state of the eof flag.
	 */

	un->un_eof = ST_NO_EOF;


	/*
	 * If writing, let's check that we're positioned correctly
	 * at the end of tape before issuing the next write.
	 */
	if (!un->un_read_only) {
		un->un_test_append = 1;
	}
	return (0);
}


/*
 * This routine implements the ioctl calls.  It is called
 * from the device switch at normal priority.
 */
/*ARGSUSED*/
stioctl(dev, cmd, data, flag)
dev_t dev;
register int cmd;
register caddr_t data;
int flag;
{
	register struct scsi_device *devp;
	register struct scsi_tape *un;
	struct mtop *mtop;
	int savefile, tmp, rval;

	if ((devp = stunits[MTUNIT(dev)]) == 0)
		return (ENXIO);
	un = UPTR;

	/*
	 * first and foremost, handle any ST_EOT_PENDING cases.
	 * That is, if a logical eot is pending notice, notice it.
	 */


	mtop = (struct mtop *) data;
	DPRINTF_IOCTL(devp, "stioctl: mt_op=%x", mtop->mt_op);
	DPRINTF_IOCTL(devp, "         fileno=%x, blkno=%x, un_eof=%x\n",
	    un->un_fileno, un->un_blkno, un->un_eof);

	if (un->un_eof == ST_EOT_PENDING) {
		int resid = un->un_err_resid;
		int status = un->un_status;
		int lastop = un->un_lastop;

		if (stcmd(devp, SCMD_SPACE, Fmk((-1)), SYNC_CMD)) {
			return (EIO);
		}
		un->un_lastop = lastop;	/* restore last operation */
		if (status == SUN_KEY_EOF)
			un->un_status = SUN_KEY_EOT;
		else
			un->un_status = status;
		un->un_err_resid  = resid;
		un->un_err_blkno = un->un_blkno = 0; /* fix up block number */
		un->un_eof = ST_EOT;	/* now we're at logical eot */
	}

	/*
	 * now, handle the rest of the situations
	 */

	if (cmd == MTIOCGET) {
		/* Get tape status */
		struct mtget *mtget = (struct mtget *) data;
		mtget->mt_erreg = un->un_status;
		mtget->mt_resid = un->un_err_resid;
		mtget->mt_dsreg = un->un_retry_ct;
		mtget->mt_fileno = un->un_err_fileno;
		mtget->mt_blkno = un->un_err_blkno;
		mtget->mt_type = un->un_dp->type;
		mtget->mt_flags = MTF_SCSI | MTF_ASF;
		if (un->un_dp->options & ST_REEL) {
			mtget->mt_flags |= MTF_REEL;
			mtget->mt_bf = 20;
		} else {				/* 1/4" cartridges */
			switch (mtget->mt_type) {
			/* Emulex cartridge tape */
			case MT_ISMT02:
				mtget->mt_bf = 40;
				break;
			default:
				mtget->mt_bf = 126;
				break;
			}
		}
		un->un_status = 0;		/* Reset status */
		un->un_err_resid = 0;
		return (0);
	}

	if (cmd != MTIOCTOP) {
		return (ENOTTY);
	}



	rval = 0;
	un->un_status = 0;

	switch (mtop->mt_op) {
	case MTERASE:
		/*
		 * MTERASE rewinds the tape, erase it completely, and returns
		 * to the beginning of the tape
		 */
		if (un->un_dp->options & ST_REEL)
			un->un_fmneeded = 2;

		if (un->un_mspl->wp || un->un_read_only) {
			un->un_status = KEY_WRITE_PROTECT;
			un->un_err_resid = mtop->mt_count;
			un->un_err_fileno = un->un_fileno;
			un->un_err_blkno = un->un_blkno;
			return (EACCES);
		}
		if (check_density_or_wfm(dev, 1, B_WRITE, NO_STEPBACK) ||
		    stcmd(devp, SCMD_REWIND, 0, SYNC_CMD) ||
		    stcmd(devp, SCMD_ERASE, 0, SYNC_CMD)) {
			un->un_fileno = -1;
			rval = EIO;
		} else {
			/* QIC and helical scan rewind after erase */
			if (un->un_dp->options & ST_REEL)
				(void) stcmd(devp, SCMD_REWIND, 0, ASYNC_CMD);
		}
		break;

	case MTWEOF:
		/*
		 * write an end-of-file record
		 */
		if (un->un_mspl->wp || un->un_read_only) {
			un->un_status = KEY_WRITE_PROTECT;
			un->un_err_resid = mtop->mt_count;
			un->un_err_fileno = un->un_fileno;
			un->un_err_blkno = un->un_blkno;
			return (EACCES);
		}
		if (mtop->mt_count == 0)
			return (0);

		un->un_eof = ST_NO_EOF;

		if (!un->un_read_only) {
			un->un_test_append = 1;
		}

		if (un->un_state == ST_STATE_OPEN_PENDING_IO) {
			if (st_determine_density(devp, B_WRITE))
				return (EIO);
		}

		if (st_write_fm(dev, (int) mtop->mt_count)) {
			/*
			 * Failure due to something other than illegal
			 * request results in loss of state (stintr).
			 */
			rval = EIO;
		} else if (un->un_dp->options & ST_REEL) {
			/*
			 * Check if user has written all the
			 * filemarks we need.  If so,
			 * back up over the last one.
			 */
#ifdef NOTNEEDED
			if (un->un_fmneeded <= 0) {
				if (stcmd(devp, SCMD_SPACE, Fmk((-1)),
				    SYNC_CMD)) {
					rval = EIO;
				}
			}
#endif
		}
		break;

	case MTRETEN:
		/*
		 * retension the tape
		 * only for cartridge tape
		 */
		if ((un->un_dp->options & ST_QIC) == 0)
		    return (ENOTTY);

		if (check_density_or_wfm(dev, 1, 0, NO_STEPBACK) ||
		    stcmd(devp, SCMD_LOAD, 3, SYNC_CMD)) {
			un->un_fileno = -1;
			rval = EIO;
		}
		break;

	case MTREW:
		/*
		 * rewind  the tape
		 */
		if (check_density_or_wfm(dev, 1, 0, NO_STEPBACK)) {
			return (EIO);
		}
		(void) stcmd(devp, SCMD_REWIND, 0, SYNC_CMD);
		break;

	case MTOFFL:
		/*
		 * rewinds, and, if appropriate, takes the device offline by
		 * unloading the tape
		 */
		if (check_density_or_wfm(dev, 1, 0, NO_STEPBACK)) {
			return (EIO);
		}
		(void) stcmd(devp, SCMD_REWIND, 0, SYNC_CMD);
		(void) stcmd(devp, SCMD_LOAD, 0, SYNC_CMD);
		un->un_eof = ST_NO_EOF;
		break;

	case MTNOP:
		un->un_status = 0;		/* Reset status */
		un->un_err_resid = 0;
		break;

	case MTEOM:
		/*
		 * positions the tape at a location just after the last file
		 * written on the tape. For cartridge and 8 mm, this after
		 * the last file mark; for reel, this is inbetween the two
		 * last 2 file marks
		 */
		if (un->un_eof >= ST_EOT) {
			/*
			 * If the command wants to move to logical end
			 * of media, and we're already there, we're done.
			 * If we were at logical eot, we reset the state
			 * to be *not* at logical eot.
			 *
			 * If we're at physical or logical eot, we prohibit
			 * forward space operations (unconditionally).
			 */
			return (0);
		}

		if (check_density_or_wfm(dev, 1, B_READ, NO_STEPBACK)) {
			return (EIO);
		}

		/*
		 * find_eom() returns the last fileno we knew about;
		 */
		savefile = find_eom(dev);

		if (un->un_status != KEY_BLANK_CHECK) {
			un->un_fileno = -1;
			rval = EIO;
		} else {
			/*
			 * For 1/2" reel tapes assume logical EOT marked
			 * by two file marks or we don't care that we may
			 * be extending the last file on the tape.
			 */
			if (un->un_dp->options & ST_REEL) {
				if (stcmd(devp, SCMD_SPACE, Fmk((-1)),
				    SYNC_CMD)) {
					un->un_fileno = -1;
					rval = EIO;
					break;
				}
				/*
				 * Fix up the block number.
				 */
				un->un_blkno = 0;
				un->un_err_blkno = 0;
			}
			un->un_err_resid = 0;
			un->un_fileno = savefile;
			un->un_eof = ST_EOT;
		}
		un->un_status = 0;
		break;

	case MTFSF:
		/*
		 * forward space over filemark
		 *
		 * For ASF we allow a count of 0 on fsf which means
		 * we just want to go to beginning of current file.
		 * Equivalent to "nbsf(0)" or "bsf(1) + fsf".
		 */
		if ((un->un_eof >= ST_EOT) && (mtop->mt_count > 0)) {
			/* we're at EOM */
			un->un_err_resid = mtop->mt_count; /* XXX added XXX */
			un->un_status = KEY_BLANK_CHECK;
			return (EIO);
		}

		/*
		 * physical tape position may not be what we've been
		 * telling the user; adjust the request accordingly
		 */
		if (mtop->mt_count && IN_EOF(un)) {
			un->un_fileno++;
			un->un_blkno = 0;
			/*
			 * For positive direction case, we're now covered.
			 * For zero or negative direction, we're covered
			 * (almost)
			 */
			mtop->mt_count--;
		}

		if (check_density_or_wfm(dev, 1, B_READ, STEPBACK)) {
			return (EIO);
		}


		/*
		 * Forward space file marks.
		 * We leave ourselves at block zero
		 * of the target file number.
		 */
		if (mtop->mt_count < 0) {
			mtop->mt_count = -mtop->mt_count;
			mtop->mt_op = MTNBSF;
			goto bspace;
		}
fspace:
		if ((tmp = mtop->mt_count) == 0) {
			if (un->un_blkno == 0) {
				un->un_err_resid = 0;
				un->un_err_fileno = un->un_fileno;
				un->un_err_blkno = un->un_blkno;
				break;
			} else if (un->un_fileno == 0) {
				rval = stcmd(devp, SCMD_REWIND, 0, SYNC_CMD);
			} else if (un->un_dp->options & ST_BSF) {
				rval = (stcmd(devp, SCMD_SPACE, Fmk((-1)),
				    SYNC_CMD) ||
				    stcmd(devp, SCMD_SPACE, Fmk(1), SYNC_CMD));
			} else {
				tmp = un->un_fileno;
				rval = (stcmd(devp, SCMD_REWIND, 0, SYNC_CMD) ||
				    stcmd(devp, SCMD_SPACE, Fmk(tmp),
				    SYNC_CMD));
			}
			if (rval) {
				un->un_fileno = -1;
				rval = EIO;
			}
		} else {
			rval = space_fmks(dev, tmp);
		}
		break;


	case MTFSR:
		/*
		 * forward space to inter-record gap
		 *
		 */
		if (mtop->mt_count == 0) {
			return (0);
		}
		if ((un->un_eof >= ST_EOT) && (mtop->mt_count > 0)) {
			/* we're at EOM */
			un->un_err_resid = mtop->mt_count;
			un->un_status = KEY_BLANK_CHECK;
			return (EIO);
		}

		/*
		 * physical tape position may not be what we've been
		 * telling the user; adjust the position accordingly
		 */
		if (mtop->mt_count && IN_EOF(un)) {
			int blkno = un->un_blkno;
			int fileno = un->un_fileno;
			int lastop = un->un_lastop;
			if (stcmd(devp, SCMD_SPACE, Fmk((-1)), SYNC_CMD) == -1)
				return (EIO);
			un->un_blkno = blkno;
			un->un_fileno = fileno;
			un->un_lastop = lastop;
		}

		if (check_density_or_wfm(dev, 1, B_READ, STEPBACK)) {
			return (EIO);
		}

space_records:
		tmp = un->un_blkno + mtop->mt_count;
		if (tmp == un->un_blkno) {
			un->un_err_resid = 0;
			un->un_err_fileno = un->un_fileno;
			un->un_err_blkno = un->un_blkno;
			break;
		} else if (un->un_blkno < tmp ||
		    (un->un_dp->options & ST_BSR)) {
			/*
			 * If we're spacing forward, or the device can
			 * backspace records, we can just use the SPACE
			 * command.
			 */
			tmp = tmp - un->un_blkno;
			if (stcmd(devp, SCMD_SPACE, Blk(tmp), SYNC_CMD)) {
				rval = EIO;
			} else if (un->un_eof >= ST_EOF_PENDING) {
				/*
				 * check if we hit BOT/EOT
				 */
				if (tmp < 0 && un->un_eof == ST_EOM) {
					un->un_status = SUN_KEY_BOT;
					un->un_eof = ST_NO_EOF;
				} else if (tmp < 0 && un->un_eof ==
				    ST_EOF_PENDING) {
					int residue = un->un_err_resid;
					/*
					 * we skipped over a filemark
					 * and need to go forward again
					 */
					if (stcmd(devp, SCMD_SPACE, Fmk(1),
					    SYNC_CMD)) {
						rval = EIO;
					}
					un->un_err_resid = residue;
				}
				if (rval == 0)
					rval = EIO;
			}
		} else {
			/*
			 * else we rewind, space forward across filemarks to
			 * the desired file, and then space records to the
			 * desired block.
			 */

			int t = un->un_fileno;	/* save current file */

			if (tmp < 0) {
				/*
				 * Wups - we're backing up over a filemark
				 */
				if (un->un_blkno != 0 &&
				    (stcmd(devp, SCMD_REWIND, 0, SYNC_CMD) ||
				    stcmd(devp, SCMD_SPACE, Fmk(t), SYNC_CMD)))
					un->un_fileno = -1;
				un->un_err_resid = -tmp;
				if (un->un_fileno == 0 && un->un_blkno == 0) {
					un->un_status = SUN_KEY_BOT;
					un->un_eof = ST_NO_EOF;
				} else if (un->un_fileno > 0) {
					un->un_status = SUN_KEY_EOF;
					un->un_eof = ST_NO_EOF;
				}
				un->un_err_fileno = un->un_fileno;
				un->un_err_blkno = un->un_blkno;
				rval = EIO;
			} else if (stcmd(devp, SCMD_REWIND, 0, SYNC_CMD) ||
				    stcmd(devp, SCMD_SPACE, Fmk(t), SYNC_CMD) ||
				    stcmd(devp, SCMD_SPACE, Blk(tmp), SYNC_CMD))
			{
				un->un_fileno = -1;
				rval = EIO;
			}
		}
		break;


	case MTBSF:
		/*
		 * backward space of file filemark (1/2" and 8mm)
		 * tape position will end on the beginning of tape side
		 * of the desired file mark
		 */
		if ((un->un_dp->options & ST_BSF) == 0) {
			return (ENOTTY);
		}

		/*
		 * If a negative count (which implies a forward space op)
		 * is specified, and we're at logical or physical eot,
		 * bounce the request.
		 */

		if (un->un_eof >= ST_EOT && mtop->mt_count < 0) {
			un->un_err_resid = mtop->mt_count;
			un->un_status = SUN_KEY_EOT;
			return (EIO);
		}
		/*
		 * physical tape position may not be what we've been
		 * telling the user; adjust the request accordingly
		 */
		if (mtop->mt_count && IN_EOF(un)) {
			un->un_fileno++;
			un->un_blkno = 0;
			if (mtop->mt_count < 0) {
				mtop->mt_count--;
			} else if (mtop->mt_count == 0) {
				/*
				 * For all cases here, this means move
				 * to block zero of the current (physical)
				 * file.
				 */
				mtop->mt_count = 1;
				mtop->mt_op = MTNBSF;
				goto mtnbsf;
			} else if (mtop->mt_count > 0) {
				mtop->mt_count++;
			}
		}

		if (check_density_or_wfm(dev, 1, 0, STEPBACK)) {
			return (EIO);
		}

		if (mtop->mt_count <= 0) {
			mtop->mt_op = MTFSF;
			mtop->mt_count = -mtop->mt_count;
			goto fspace;
		}

bspace:
	{
		/*
		 * Backspace files (MTNBSF):
		 *
		 *	For tapes that can backspace, backspace
		 *	count+1 filemarks and then run forward over
		 *	a filemark
		 *
		 *	For tapes that can't backspace,
		 *		calculate desired filenumber
		 *		(un->un_fileno - count), rewind,
		 *		and then space forward this amount
		 *
		 * Backspace filemarks (MTBSF)
		 *
		 *	For tapes that can backspace, backspace count
		 *	filemarks
		 *
		 *	For tapes that can't backspace, calculate
		 *	desired filenumber (un->un_fileno - count),
		 *	add 1, rewind, space forward this amount,
		 *	and mark state as ST_EOF_PENDING appropriately.
		 */
		int skip_cnt, end_at_eof;

		if (mtop->mt_op == MTBSF)
			end_at_eof = 1;
		else
			end_at_eof = 0;

		DPRINTF_IOCTL(devp,
		    "bspace: mt_op=%x, count=%x, fileno=%x, blkno=%x\n",
		    mtop->mt_op, mtop->mt_count, un->un_fileno, un->un_blkno);

		/*
		 * Handle the simple case of BOT
		 * playing a role in these cmds.
		 * We do this by calculating the
		 * ending file number. If the ending
		 * file is < BOT, rewind and set an
		 * error and mark resid appropriately.
		 * If we're backspacing a file (not a
		 * filemark) and the target file is
		 * the first file on the tape, just
		 * rewind.
		 */

		tmp = un->un_fileno - mtop->mt_count;
		if ((end_at_eof && tmp < 0) || (end_at_eof == 0 && tmp <= 0)) {
			if (stcmd (devp, SCMD_REWIND, 0, SYNC_CMD)) {
				rval = EIO;
			}
			if (tmp < 0) {
				rval = EIO;
				un->un_err_resid = -tmp;
				un->un_status = SUN_KEY_BOT;
			}
			break;
		}

		if (un->un_dp->options & ST_BSF) {
			skip_cnt = 1 - end_at_eof;
			/*
			 * If we are going to end up at the beginning
			 * of the file, we have to space one extra file
			 * first, and then space forward later.
			 */
			tmp = -(mtop->mt_count + skip_cnt);
			DPRINTF_IOCTL(devp, "skip_cnt=%x, tmp=%x\n",
				skip_cnt, tmp);
			if (stcmd(devp, SCMD_SPACE, Fmk(tmp), SYNC_CMD)) {
				rval = EIO;
			}
		} else {
			if (stcmd(devp, SCMD_REWIND, 0, SYNC_CMD)) {
				rval = EIO;
			} else {
				skip_cnt = tmp + end_at_eof;
			}
		}

		/*
		 * If we have to space forward, do so...
		 */
		DPRINTF_IOCTL(devp, "space forward skip_cnt=%x, rval=%x\n",
		    skip_cnt, rval);
		if (rval == 0 && skip_cnt) {
			if (stcmd(devp, SCMD_SPACE, Fmk(skip_cnt), SYNC_CMD)) {
				rval = EIO;
			} else if (end_at_eof) {
				/*
				 * If we had to space forward, and we're
				 * not a tape that can backspace, mark state
				 * as if we'd just seen a filemark during a
				 * a read.
				 */
				if ((un->un_dp->options & ST_BSF) == 0) {
					un->un_eof = ST_EOF_PENDING;
					un->un_fileno -= 1;
					un->un_blkno = INF;
				}
			}
		}

		if (rval)
			un->un_fileno = -1;
		break;
	}

	case MTNBSF:
		/*
		 * backward space file to beginning of file
		 *
		 * If a negative count (which implies a forward space op)
		 * is specified, and we're at logical or physical eot,
		 * bounce the request.
		 */

		if (un->un_eof >= ST_EOT && mtop->mt_count < 0) {
			un->un_err_resid = mtop->mt_count;
			un->un_status = SUN_KEY_EOT;
			return (EIO);
		}
		/*
		 * physical tape position may not be what we've been
		 * telling the user; adjust the request accordingly
		 */
		if (mtop->mt_count && IN_EOF(un)) {
			un->un_fileno++;
			un->un_blkno = 0;
			if (mtop->mt_count < 0) {
				mtop->mt_count--;
			} else if (mtop->mt_count == 0) {
				/*
				 * For all cases here, this means move
				 * to block zero of the current (physical)
				 * file.
				 */
				mtop->mt_count = 1;
			} else if (mtop->mt_count > 0) {
				mtop->mt_count++;
			}
		}

		if (check_density_or_wfm(dev, 1, 0, STEPBACK)) {
			return (EIO);
		}

mtnbsf:
		if (mtop->mt_count <= 0) {
			mtop->mt_op = MTFSF;
			mtop->mt_count = -mtop->mt_count;
			goto fspace;
		}
		goto bspace;

	case MTBSR:
		/*
		 * backward space into inter-record gap
		 *
		 * If a negative count (which implies a forward space op)
		 * is specified, and we're at logical or physical eot,
		 * bounce the request.
		 */
		if (un->un_eof >= ST_EOT && mtop->mt_count < 0) {
			un->un_err_resid = mtop->mt_count;
			un->un_status = SUN_KEY_EOT;
			return (EIO);
		}

		if (mtop->mt_count == 0) {
			return (0);
		}

		/*
		 * physical tape position may not be what we've been
		 * telling the user; adjust the position accordingly.
		 * bsr can not skip filemarks and continue to skip records
		 * therefore if we are logically before the filemark but
		 * physically at the EOT side of the filemark, we need to step
		 * back; this allows fsr N where N > number of blocks in file
		 * followed by bsr 1 to position at the beginning of  last block
		 */
		if (IN_EOF(un)) {
			int blkno = un->un_blkno;
			int fileno = un->un_fileno;
			int lastop = un->un_lastop;
			if (stcmd(devp, SCMD_SPACE, Fmk((-1)), SYNC_CMD) == -1)
				return (EIO);
			un->un_blkno = blkno;
			un->un_fileno = fileno;
			un->un_lastop = lastop;
		}

		un->un_eof = ST_NO_EOF;

		if (check_density_or_wfm(dev, 1, 0, STEPBACK)) {
			return (EIO);
		}

		mtop->mt_count = -mtop->mt_count;
		goto space_records;

	default:
		rval = ENOTTY;
	}

	DPRINTF_IOCTL(devp,
	    "stioctl: fileno=%x, blkno=%x, un_eof=%x\n", un->un_fileno,
	    un->un_blkno, un->un_eof);

	if (un->un_fileno < 0)
		un->un_density_known = 0;
	return (rval);
}




static int
st_write_fm(dev, wfm)
dev_t dev;
int wfm;
{
	register struct scsi_device *devp;
	register struct scsi_tape *un;
	int i;

	if ((devp = stunits[MTUNIT(dev)]) == 0)
		return (ENXIO);
	un = UPTR;

	/*
	 * write one filemark at the time after EOT
	 */
	if (un->un_eof >= ST_EOT) {
		for (i = 0; i < wfm; i++) {
			if (stcmd(devp, SCMD_WRITE_FILE_MARK, 1, SYNC_CMD)) {
				return (EIO);
			}
		}
	} else if (stcmd(devp, SCMD_WRITE_FILE_MARK, wfm, SYNC_CMD)) {
				return (EIO);
	}
	return (0);
}


/*
 * Command start && done functions
 */

static void
ststart(devp)
register struct scsi_device *devp;
{
	register struct buf *bp;
	register struct scsi_tape *un = UPTR;


	if (un->un_runq || (bp = un->un_quef) == NULL) {
		return;
	}

	if (!BP_PKT(bp)) {
		make_st_cmd(devp, bp, strunout);
		if (!BP_PKT(bp)) {
			un->un_state = ST_STATE_RESOURCE_WAIT;
			return;
		}
		un->un_state = ST_STATE_OPEN;
	}

	un->un_runq = bp;
	un->un_quef = bp->b_actf;
	bp->b_actf = 0;

	if (pkt_transport(BP_PKT(bp)) != TRAN_ACCEPT) {
		stprintf(devp, "transport rejected");
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		stdone(devp);
	}
}


static int
strunout()
{
	register i, rval, s;
	register struct scsi_device *devp;
	register struct scsi_tape *un;

	rval = 1;
	s = splr(stpri);
	for (i = 0; i < ST_MAXUNIT; i++) {
		devp = stunits[i];
		if (devp && devp->sd_present && (un = UPTR) &&
		    un->un_attached) {
			if (un->un_state == ST_STATE_RESOURCE_WAIT) {
				ststart(devp);
				if (un->un_state == ST_STATE_RESOURCE_WAIT) {
					rval = 0;
					break;
				}
			}
		}
	}
	(void) splx(s);
	return (rval);
}

static void
stdone(devp)
register struct scsi_device *devp;
{
	register struct buf *bp;
	register struct scsi_tape *un = UPTR;

	bp = un->un_runq;
	un->un_runq = NULL;
	/*
	 * Start the next one before releasing resources on this one
	 */
	if (un->un_quef) {
		ststart(devp);
	}

	if (bp != un->un_sbufp) {
		scsi_resfree(BP_PKT(bp));
		iodone(bp);
	} else {
		bp->b_flags |= B_DONE;
		if (bp->b_flags & B_ASYNC) {
			int com;
			scsi_resfree (BP_PKT(bp));
			com = (int) bp->b_forw;
			if (com == SCMD_READ || com == SCMD_WRITE) {
				bp->b_un.b_addr = (caddr_t) 0;
			}
			bp->b_flags &= ~(B_BUSY|B_ASYNC);
			if (bp->b_flags & B_WANTED) {
				wakeup((caddr_t) bp);
			}
		} else {
			wakeup((caddr_t) un);
		}
	}
}

/*
 * Utility functions
 */

static int
st_determine_generic(devp)
struct scsi_device *devp;
{
	u_char dens;
	int bsize;
	struct scsi_tape *un = UPTR;
	static char *cart = "0.25 inch cartridge";
	char *sizestr, local[8+3];

	if (stcmd(devp, SCMD_MODE_SENSE, MSIZE, SYNC_CMD))
		return (-1);

	bsize = (un->un_mspl->high_bl << 16)	|
		(un->un_mspl->mid_bl << 8)	|
		(un->un_mspl->low_bl);

	if (bsize == 0) {
		un->un_dp->options |= ST_VARIABLE;
		un->un_dp->bsize = 10 << 10;
	} else if (bsize > ST_MAXRECSIZE_FIXED) {
		/*
		 * record size of this device too big.
		 * try and convert it to variable record length.
		 *
		 */
		un->un_dp->options |= ST_VARIABLE;
		if (st_modeselect(devp)) {
			stprintf(devp, "Fixed Record Size %d is too large",
			    bsize);
			stprintf(devp, "Cannot switch to variable record size");
			un->un_dp->options &= ~ST_VARIABLE;
			return (-1);
		}
		un->un_mspl->high_bl = un->un_mspl->mid_bl =
		    un->un_mspl->low_bl = 0;
	} else {
		un->un_dp->bsize = (u_short) bsize;
	}


	switch (dens = un->un_mspl->density) {
	default:
	case 0x0:
		/*
		 * default density, cannot determine any other
		 * information.
		 */
		sizestr = "Unknown type- assuming 0.25 inch cartridge";
		un->un_dp->type = ST_TYPE_DEFAULT;
		un->un_dp->options |= (ST_AUTODEN_OVERRIDE|ST_QIC);
		break;
	case 0x1:
	case 0x2:
	case 0x3:
	case 0x6:
		/*
		 * 1/2" reel
		 */
		sizestr = "0.50 inch reel";
		un->un_dp->type = ST_TYPE_REEL;
		un->un_dp->options |= ST_REEL;
		un->un_dp->densities[0] = 0x1;
		un->un_dp->densities[1] = 0x2;
		un->un_dp->densities[2] = 0x6;
		un->un_dp->densities[3] = 0x3;
		break;
	case 0x4:
	case 0x5:
	case 0x7:
	case 0x0b:

		/*
		 * Quarter inch.
		 */
		sizestr = cart;
		un->un_dp->type = ST_TYPE_DEFAULT;
		un->un_dp->options |= ST_QIC;

		un->un_dp->densities[1] = 0x4;
		un->un_dp->densities[2] = 0x5;
		un->un_dp->densities[3] = 0x7;
		un->un_dp->densities[0] = 0x0b;
		break;

	case 0x0f:
	case 0x10:
	case 0x11:
	case 0x12:
		/*
		 * QIC-120, QIC-150, QIC-320, QIC-600
		 */
		sizestr = cart;
		un->un_dp->type = ST_TYPE_DEFAULT;
		un->un_dp->options |= ST_QIC;
		un->un_dp->densities[0] = 0x0f;
		un->un_dp->densities[1] = 0x10;
		un->un_dp->densities[2] = 0x11;
		un->un_dp->densities[3] = 0x12;
		break;

	case 0x09:
	case 0x0a:
	case 0x0c:
	case 0x0d:
		/*
		 * 1/2" cartridge tapes. Include HI-TC.
		 */
		sizestr = cart;
		sizestr[2] = '5';
		sizestr[3] = '0';
		un->un_dp->type = ST_TYPE_HIC;
		un->un_dp->densities[0] = 0x09;
		un->un_dp->densities[1] = 0x0a;
		un->un_dp->densities[2] = 0x0c;
		un->un_dp->densities[3] = 0x0d;
		break;

	case 0x13:
	case 0x14:
		/*
		 * Helical Scan (Exabyte) devices
		 */
		if (dens == 0x13)
			sizestr = "3.81mm helical scan cartridge";
		else
			sizestr = "8mm helical scan cartridge";
		un->un_dp->type = ST_TYPE_EXABYTE;
		un->un_dp->options |= ST_AUTODEN_OVERRIDE;
		break;
	}

	/*
	 * Assume LONG ERASE and NO BUFFERED MODE
	 */

	un->un_dp->options |= (ST_LONG_ERASE|ST_NOBUF);

	/*
	 * set up large read and write retry counts
	 */

	un->un_dp->max_rretries = un->un_dp->max_wretries = 1000;

	/*
	 * make up name
	 */

	local[0] = '<';
	bcopy(devp->sd_inq->inq_vid, &local[1], 8);
	local[9] = '>';
	local[10] = 0;

	/*
	 * If this is a 0.50 inch reel tape, and
	 * it is *not* variable mode, try and
	 * set it to variable record length
	 * mode.
	 */
	if ((un->un_dp->options & (ST_VARIABLE|ST_REEL)) == ST_REEL) {
		un->un_dp->options |= ST_VARIABLE;
		if (st_modeselect(devp)) {
			un->un_dp->options &= ~ST_VARIABLE;
		} else {
			un->un_dp->bsize = 10 << 10;
			un->un_mspl->high_bl = un->un_mspl->mid_bl =
			    un->un_mspl->low_bl = 0;
		}
	}

	/*
	 * Write to console about type of device found
	 */

	stprintf(devp, "Generic Drive, Vendor=%s\n\t%s", local, sizestr);
	if (un->un_dp->options & ST_VARIABLE) {
		printf("\tVariable record length I/O\n");
	} else {
		printf("\tFixed record length (%d byte blocks) I/O\n",
			un->un_dp->bsize);
	}
	return (0);
}

static int
st_determine_density(devp, rw)
struct scsi_device *devp;
int rw;
{
	struct scsi_tape *un = UPTR;

	/*
	 * If we're past BOT, density is determined already.
	 */
	if (un->un_fileno > 0) {
		/*
		 * XXX:	put in a bitch message about attempting to
		 * XXX:	change density past BOT.
		 */
		return (0);
	}

	/*
	 * If we're going to be writing, we set the density
	 */
	if (rw == B_WRITE) {
		/* un_curdens is used as an index into densities table */
		un->un_curdens = MT_DENSITY(un->un_dev);
		if (st_set_density(devp)) {
			return (-1);
		}
		return (0);
	}
	/*
	 * If density is known already,
	 * we don't have to get it again.(?)
	 */
	if (un->un_density_known)
		return (0);
	if (st_get_density(devp)) {
		return (-1);
	}
	return (0);
}


/*
 * Try to determine density. We do this by attempting to read the
 * first record off the tape, cycling through the available density
 * codes as we go.
 */

static int
st_get_density(devp)
struct scsi_device *devp;
{
	register struct scsi_tape *un = UPTR;
	int succes = 0, rval = -1, i;
	u_int size;
	u_char dens, olddens;

	if (un->un_dp->options & ST_AUTODEN_OVERRIDE) {
		un->un_density_known = 1;
		return (0);
	}

	/*
	 * This will only work on variable record length tapes
	 * if and only if all variable record length tapes autodensity
	 * select.
	 */
	size = (unsigned) un->un_dp->bsize;
	un->un_tmpbuf = (caddr_t) kmem_alloc(size);

	/*
	 * Start at the specified density
	 */

	dens = olddens = un->un_curdens = MT_DENSITY(un->un_dev);

	for (i = 0; i < NDENSITIES; i++, ((un->un_curdens == NDENSITIES-1) ?
					un->un_curdens = 0 :
					un->un_curdens += 1)) {
		/*
		 * If we've done this density before,
		 * don't bother to do it again.
		 */
		dens = un->un_dp->densities[un->un_curdens];
		if (i > 0 && dens == olddens)
			continue;
		olddens = dens;
		DPRINTF(devp, "trying density 0x%x", dens);
		if (st_set_density(devp))
			continue;

		succes = (stcmd(devp, SCMD_READ, (int) size, SYNC_CMD) == 0);
		if (stcmd(devp, SCMD_REWIND, 0, SYNC_CMD)) {
			break;
		}
		if (succes) {
			stinit(un);
			rval = 0;
			un->un_density_known = 1;
			break;
		}
	}
	(void) kmem_free(un->un_tmpbuf, size);
	un->un_tmpbuf = 0;
	return (rval);
}

static int
st_set_density(devp)
struct scsi_device *devp;
{
	struct scsi_tape *un = UPTR;
	if ((un->un_dp->options & ST_AUTODEN_OVERRIDE) == 0) {
		un->un_mspl->density = un->un_dp->densities[un->un_curdens];
		if (st_modeselect(devp)) {
			return (-1);
		}
	}
	un->un_density_known = 1;
	return (0);
}

static int
st_loadtape(devp)
struct scsi_device *devp;
{
	/*
	 * 'LOAD' the tape to BOT
	 */
	if (stcmd(devp, SCMD_LOAD, 1, SYNC_CMD)) {
		/*
		 * Don't bounce the loadtape function here- some devices
		 * don't support the load command. For them, if the load
		 * tape command fails with an ILLEGAL REQUEST sense key,
		 * try a rewind instead.
		 */
		if (UPTR->un_status == KEY_ILLEGAL_REQUEST) {
			if (stcmd(devp, SCMD_REWIND, 0, SYNC_CMD)) {
				return (-1);
			}
		} else
			return (-1);
	}

	/*
	 * run a MODE SENSE to get the write protect status, then run
	 * a MODESELECT operation in order to set any modes that might
	 * be appropriate for this device (like VARIABLE, etc..)
	 */

	if (stcmd(devp, SCMD_MODE_SENSE, MSIZE, SYNC_CMD) ||
	    st_modeselect(devp))
		return (-1);
	stinit(UPTR);
	UPTR->un_density_known = 0;
	return (0);
}


/*
 * Note: QIC devices aren't so smart.  If you try to append
 * after EOM, the write can fail because the device doesn't know
 * it's at EOM.  In that case, issue a read.  The read should fail
 * because there's no data, but the device knows it's at EOM,
 * so a subsequent write should succeed.  To further confuse matters,
 * the target returns the same error if the tape is positioned
 * such that a write would overwrite existing data.  That's why
 * we have to do the append test.  A read in the middle of
 * recorded data would succeed, thus indicating we're attempting
 * something illegal.
 */

static void
st_test_append(bp)
struct buf *bp;
{
	struct scsi_device *devp = stunits[MTUNIT(bp->b_dev)];
	struct scsi_tape *un = UPTR;
	int status;

	DPRINTF(devp, "testing append to file %d", un->un_fileno);

	un->un_laststate = un->un_state;
	un->un_state = ST_STATE_APPEND_TESTING;
	un->un_test_append = 0;

	/*
	 * first, map in the buffer, because we're doing a double write --
	 * first into the kernel, then onto the tape.
	 */

	bp_mapin(bp);

	/*
	 * get a copy of the data....
	 */

	un->un_tmpbuf = (caddr_t) kmem_alloc((unsigned) bp->b_bcount);
	bcopy(bp->b_un.b_addr, un->un_tmpbuf, (u_int) bp->b_bcount);

	/*
	 * attempt the write..
	 */

	if (stcmd(devp, SCMD_WRITE, (int) bp->b_bcount, SYNC_CMD) == 0) {
success:
		DPRINTF(devp, "append write succeeded");
		bp->b_resid = un->un_sbufp->b_resid;
		/*
		 * Note: iodone will do a bp_mapout()
		 */
		iodone(bp);
		un->un_laststate = un->un_state;
		un->un_state = ST_STATE_OPEN;
		(void) kmem_free (un->un_tmpbuf, (unsigned)bp->b_bcount);
		un->un_tmpbuf = 0;
		return;
	}

	/*
	 * The append failed. Do a short read. If that fails,  we are at EOM
	 * so we can retry the write command. If that succeeds, than we're
	 * all screwed up (the controller reported a real error).
	 *
	 * XXX:	should the dummy read be > SECSIZE? should it be the device's
	 * XXX:	block size?
	 *
	 */

	status = un->un_status;
	un->un_status = 0;
	(void) stcmd(devp, SCMD_READ, SECSIZE, SYNC_CMD);
	if (un->un_status == KEY_BLANK_CHECK) {
		DPRINTF(devp, "append at EOM");
		/*
		 * Okay- the read failed. We should actually have confused
		 * the controller enough to allow writing. In any case, the
		 * i/o is on its own from here on out.
		 */
		un->un_laststate = un->un_state;
		un->un_state = ST_STATE_OPEN;
		bcopy(bp->b_un.b_addr, un->un_tmpbuf, (u_int) bp->b_bcount);
		if (stcmd(devp, SCMD_WRITE, (int) bp->b_bcount, SYNC_CMD) == 0)
			goto success;
	}

	DPRINTF(devp, "append write failed- not at EOM");
	bp->b_resid = bp->b_bcount;
	bp->b_flags |= B_ERROR;
	bp->b_error = EIO;

	/*
	 * backspace one record to get back to where we were
	 */
	if (stcmd(devp, SCMD_SPACE, Blk(-1), SYNC_CMD))
		un->un_fileno = -1;

	un->un_err_resid = bp->b_resid;
	un->un_status = status;

	/*
	 * Note: iodone will do a bp_mapout()
	 */
	iodone(bp);
	un->un_laststate = un->un_state;
	un->un_state = ST_STATE_OPEN_PENDING_IO;
	(void) kmem_free (un->un_tmpbuf, (unsigned)bp->b_bcount);
	un->un_tmpbuf = 0;
}

/*
 * Special command handler
 */

/*
 * common stcmd code. The fourth parameter states
 * whether the caller wishes to await the results
 */

static int
stcmd(devp, com, count, wait)
register struct scsi_device *devp;
int com, count, wait;
{
	struct scsi_tape *un = UPTR;
	register struct buf *bp;
	register s, error;

#ifdef  ST_DEBUG
	if (st_debug & ST_DEBUG_CMDS)
		st_debug_cmds(devp, com, count, wait);
#endif

	bp = un->un_sbufp;
	s = splr(stpri);
	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANTED;
		if (un->un_state == ST_STATE_OPENING) {
			if (sleep((caddr_t) bp, (PZERO+1)|PCATCH)) {
				bp->b_flags &= ~B_WANTED;
				(void) splx(s);
				return (EINTR);
			}
		} else {
			(void) sleep((caddr_t) bp, PRIBIO);
		}
	}
	if (!wait)
		bp->b_flags = B_BUSY|B_ASYNC;
	else
		bp->b_flags = B_BUSY;
	(void) splx(s);
	if (com == SCMD_READ || com == SCMD_WRITE) {
		bp->b_un.b_addr = un->un_tmpbuf;
		if (com == SCMD_READ)
			bp->b_flags |= B_READ;
	}
	bp->b_error = 0;
	bp->b_dev = un->un_dev;
	bp->b_bcount = count;
	bp->b_resid = 0;
	bp->b_forw = (struct buf *) com;
	make_st_cmd(devp, bp, SLEEP_FUNC);
	ststrategy(bp);
	if (!wait) {
		return (0);
	}
	s = splr(stpri);
	while ((bp->b_flags & B_DONE) == 0) {
		if (un->un_state == ST_STATE_OPENING) {
			if (sleep((caddr_t) un, (PZERO+1)|PCATCH)) {
				if (bp->b_flags & B_DONE)
					break;
				bp->b_flags |= B_ASYNC;
				(void) splx(s);
				return (EINTR);
			}
		} else
			(void) sleep((caddr_t) un, PRIBIO);
	}
	error = geterror(bp);
	scsi_resfree(BP_PKT(bp));
	(void) splx(s);
	if (com == SCMD_READ || com == SCMD_WRITE) {
		bp->b_un.b_addr = (caddr_t) 0;
	}
	bp->b_flags &= ~B_BUSY;
	if (bp->b_flags & B_WANTED) {
		wakeup((caddr_t) bp);
	}
	return (error);
}

/*
 * We assume someone else has set the density code
 */

static int
st_modeselect(devp)
struct scsi_device *devp;
{
	register struct scsi_tape *un = UPTR;

	un->un_mspl->reserved1 = un->un_mspl->reserved2 = 0;
	un->un_mspl->wp = 0;
	if (un->un_dp->options & ST_NOBUF)
		un->un_mspl->bufm = 0;
	else
		un->un_mspl->bufm = 1;
	un->un_mspl->bd_len = 8;
	un->un_mspl->high_nb = un->un_mspl->mid_nb = un->un_mspl->low_nb = 0;
	if ((un->un_dp->options & ST_VARIABLE) == 0) {
		un->un_mspl->high_bl = 0;
		un->un_mspl->mid_bl = (un->un_dp->bsize>>8) & 0xff;
		un->un_mspl->low_bl = (un->un_dp->bsize) & 0xff;
	} else {
		un->un_mspl->high_bl = un->un_mspl->mid_bl =
			un->un_mspl->low_bl = 0;
	}
	if (stcmd(devp, SCMD_MODE_SELECT, MSIZE, SYNC_CMD) ||
			stcmd(devp, SCMD_MODE_SENSE, MSIZE, SYNC_CMD)) {
		if (un->un_state >= ST_STATE_OPEN) {
			stprintf(devp, "unable to set tape mode");
			un->un_fileno = -1;
			return (ENXIO);
		}
		return (-1);
	}
	return (0);
}

static void
stinit(un)
struct scsi_tape *un;
{
	un->un_blkno = 0;
	un->un_fileno = 0;
	un->un_lastop = ST_OP_NIL;
	un->un_eof = ST_NO_EOF;
	if (st_error_level != STERR_ALL) {
		if (DEBUGGING)
			st_error_level = STERR_ALL;
		else
			st_error_level = STERR_RETRYABLE;
	}
}

static void
make_st_cmd(devp, bp, func)
register struct scsi_device *devp;
register struct buf *bp;
int (*func)();
{
	register struct scsi_pkt *pkt;
	register struct scsi_tape *un = UPTR;
	register count, com, flags, tval = st_io_time;
	char fixbit = (un->un_dp->options & ST_VARIABLE) ? 0: 1;

	flags = (scsi_options & SCSI_OPTIONS_DR) ? 0: FLAG_NODISCON;
	if (un->un_dp->options & ST_NOPARITY)
		flags |= FLAG_NOPARITY;

	/*
	 * fixbit is for setting the Fixed Mode and Suppress Incorrect
	 * Length Indicator bits on read/write commands, for setting
	 * the Long bit on erase commands, and for setting the Code
	 * Field bits on space commands.
	 */
/*
 * XXX why do we set lastop here?
 */

	if (bp != un->un_sbufp) {		/* regular raw I/O */
		pkt = scsi_resalloc(ROUTE, CDB_GROUP0, 1, (opaque_t)bp, func);
		if ((BP_PKT(bp) = pkt) == (struct scsi_pkt *) 0) {
			return;
		}
		if (un->un_dp->options & ST_VARIABLE) {
			count = bp->b_bcount;
		} else
			count = bp->b_bcount >> SECDIV;
		if (bp->b_flags & B_READ) {
			com = SCMD_READ;
			un->un_lastop = ST_OP_READ;
		} else {
			com = SCMD_WRITE;
			un->un_lastop = ST_OP_WRITE;
			if (un->un_dp->options & ST_REEL) {
				un->un_fmneeded = 2;
			}
		}
		DPRINTF(devp, "%s %d amt 0x%x", (com == SCMD_WRITE) ?
			"write": "read", un->un_blkno, bp->b_bcount);
	} else {				/* special I/O */
		char saved_lastop = un->un_lastop;
		register struct buf *allocbp = 0;

		un->un_lastop = ST_OP_CTL;	/* usual */

		com = (int) bp->b_forw;
		count = bp->b_bcount;

		switch (com) {
		case SCMD_READ:
			DPRINTF_IOCTL(devp, "special read %d", count);
			if (un->un_dp->options & ST_VARIABLE) {
				fixbit = 2;	/* suppress SILI */
			} else
				count >>= SECDIV;
			bp->b_flags |= B_READ;
			allocbp = bp;
			un->un_lastop = ST_OP_READ;
			break;
		case SCMD_WRITE:
			DPRINTF_IOCTL(devp, "special write %d", count);
			if ((un->un_dp->options & ST_VARIABLE) == 0)
				count >>= SECDIV;
			allocbp = bp;
			un->un_lastop = ST_OP_WRITE;
			if (un->un_dp->options & ST_REEL) {
				un->un_fmneeded = 2;
			}
			break;
		case SCMD_WRITE_FILE_MARK:
			DPRINTF_IOCTL(devp, "write %d file marks", count);
			fixbit = 0;
			break;

		case SCMD_REWIND:
			fixbit = 0;
			count = 0;
			tval = st_space_time;
			DPRINTF_IOCTL(devp, "rewind");
			break;

		case SCMD_SPACE:
			fixbit = Isfmk(count);
			count = space_cnt(count);
			DPRINTF_IOCTL(devp, "space %d %s from file %d blk %d",
				count, (fixbit) ? "filemarks" : "records",
				un->un_fileno, un->un_blkno);
			tval = st_space_time;
			break;

		case SCMD_LOAD:
			DPRINTF_IOCTL(devp, "%s tape", (count) ?
				"load":"unload");
			if ((flags&FLAG_NODISCON) == 0)
				fixbit = 0;
			tval = st_space_time;
			break;

		case SCMD_ERASE:
			DPRINTF_IOCTL(devp, "erase tape");
			count = 0;
			/*
			 * We support long erase only
			 */
			fixbit = 1;
			if (un->un_dp->options & ST_LONG_ERASE) {
				tval = 3 * st_space_time;
			} else {
				tval = st_space_time;
			}
			break;

		case SCMD_MODE_SENSE:
			DPRINTF_IOCTL(devp, "mode sense");
			bp->b_flags |= B_READ;
			allocbp = bp;
			fixbit = 0;
			bp->b_un.b_addr = (caddr_t) (un->un_mspl);
			un->un_lastop = saved_lastop;
			break;

		case SCMD_MODE_SELECT:

			DPRINTF_IOCTL(devp, "mode select");
			allocbp = bp;
			fixbit = 0;
			bp->b_un.b_addr = (caddr_t) (un->un_mspl);
			un->un_lastop = saved_lastop;
			break;

		case SCMD_TEST_UNIT_READY:
			DPRINTF_IOCTL(devp, "test unit ready");
			fixbit = 0;
			un->un_lastop = saved_lastop;
			break;
		}
		pkt = scsi_resalloc(ROUTE, CDB_GROUP0, 1,
			(opaque_t)allocbp, func);
		if (pkt == (struct scsi_pkt *) 0)
			return;

	}
	makecom_g0_s(pkt, devp, flags, com, count, fixbit);
	pkt->pkt_time = tval;
	pkt->pkt_comp = stintr;
	pkt->pkt_private = (opaque_t) bp;
	BP_PKT(bp) = pkt;
	pkt->pkt_pmon = -1;	/* no performance measurement */
}

/*
 *
 */
static
strestart(arg)
caddr_t arg;
{
	struct scsi_device *devp = (struct scsi_device *) arg;
	struct buf *bp;
	register s = splr(stpri);
	if ((bp = UPTR->un_runq) == 0) {
		stprintf(devp, "busy restart aborted");
	} else if (pkt_transport(BP_PKT(bp)) != TRAN_ACCEPT) {
		stprintf(devp, "restart transport rejected");
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		stdone(devp);
	}
	(void) splx(s);
}

/*
 * Command completion processing
 *
 */

static void
stintr(pkt)
struct scsi_pkt *pkt;
{
	register struct scsi_device *devp;
	register struct scsi_tape *un;
	register struct buf *bp;
	register action;

	bp = (struct buf *) pkt->pkt_private;
	devp = stunits[MTUNIT(bp->b_dev)];
	un = UPTR;

	DPRINTF(devp, "stintr");

	if (pkt->pkt_reason != CMD_CMPLT) {
		action = st_handle_incomplete(devp);
	/*
	 * At this point we know that the command was successfully
	 * completed. Now what?
	 */
	} else if (un->un_state == ST_STATE_SENSING) {
		/*
		 * okay. We were running a REQUEST SENSE. Find
		 * out what to do next.
		 */
		action = st_handle_sense(devp);
		/*
		 * set pkt back to original packet in case we will have
		 * to requeue it
		 */
		pkt = BP_PKT(bp);
	} else {
		/*
		 * Okay, we weren't running a REQUEST SENSE. Call a routine
		 * to see if the status bits we're okay. As a side effect,
		 * clear state for this device, set non-error b_resid values,
		 * etc.
		 * If a request sense is to be run, that will happen.
		 */
		action = st_check_error(devp);
	}

	/*
	 * Restore old state if we were sensing.
	 *
	 */
	if (un->un_state == ST_STATE_SENSING && action != QUE_SENSE)
		un->un_state = un->un_laststate;


	switch (action) {
	case COMMAND_DONE_ERROR:
		if (un->un_eof < ST_EOT_PENDING &&
		    un->un_state >= ST_STATE_OPEN) {
			/*
			 * all errors set state of the tape to 'unknown'
			 * unless we're at EOT or are doing append testing.
			 * If sense key was illegal request, preserve state.
			 */
			if (un->un_status != KEY_ILLEGAL_REQUEST)
				un->un_fileno = -1;
		}
		un->un_err_resid = bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		stdone(devp);
		break;
	case COMMAND_DONE_ERROR_RECOVERED:
		un->un_err_resid = bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		st_set_state(devp);
		stdone(devp);
		break;
	case COMMAND_DONE:
		st_set_state(devp);
		stdone(devp);
		break;
	case QUE_SENSE:
		if (un->un_state != ST_STATE_SENSING) {
			un->un_laststate = un->un_state;
			un->un_state = ST_STATE_SENSING;
		}
		un->un_rqs->pkt_private = (opaque_t) bp;
		bzero((caddr_t)devp->sd_sense, SENSE_LENGTH);
		if (pkt_transport(un->un_rqs) != TRAN_ACCEPT) {
			panic("stintr: transport of request sense fails");
			/*NOTREACHED*/
		}
		break;
	case QUE_COMMAND:
		if (pkt_transport(pkt) != TRAN_ACCEPT) {
			stprintf(devp, "requeue of command fails");
			un->un_fileno = -1;
			un->un_err_resid = bp->b_resid = bp->b_bcount;
			bp->b_flags |= B_ERROR;
			stdone(devp);
		}
		break;
	case JUST_RETURN:
		break;
	}
}

static int
st_handle_incomplete(devp)
struct scsi_device *devp;
{
	static char *fail = "SCSI transport failed: reason '%s': %s";
	register rval = COMMAND_DONE_ERROR;
	struct scsi_tape *un = UPTR;
	struct buf *bp = un->un_runq;
	struct scsi_pkt *pkt = (un->un_state == ST_STATE_SENSING) ?
			un->un_rqs : BP_PKT(bp);
	int result;

	switch (pkt->pkt_reason) {
				/* cleanup already done: */
	case CMD_RESET:		/* SCSI bus reset destroyed command */
	case CMD_ABORTED:	/* Command tran aborted on request */
		stprintf(devp, "transport completed with %s",
			scsi_rname(pkt->pkt_reason));
		break;

				/* the following may need cleanup: */
	case CMD_DMA_DERR:	/* dma direction error occurred */
	case CMD_UNX_BUS_FREE:	/* Unexpected Bus Free Phase occurred */
	case CMD_TIMEOUT:	/* Command timed out */
	case CMD_TRAN_ERR:	/* unspecified transport error */
	case CMD_NOMSGOUT:	/* Targ refused to go to Msg Out phase */
	case CMD_CMD_OVR:	/* Command Overrun */
	case CMD_DATA_OVR:	/* Data Overrun */

				/* can't happen: */
	case CMD_INCOMPLETE:	/* tran stopped with not normal state */
	case CMD_STS_OVR:	/* Status Overrun */
	case CMD_BADMSG:	/* Message not Command Complete */
	case CMD_XID_FAIL:	/* Extended Identify message rejected */
	case CMD_IDE_FAIL:	/* Initiator Detected Err msg rejected */
	case CMD_ABORT_FAIL:	/* Abort message rejected */
	case CMD_REJECT_FAIL:	/* Reject message rejected */
	case CMD_NOP_FAIL:	/* No Operation message rejected */
	case CMD_PER_FAIL:	/* Msg Parity Error message rejected */
	case CMD_BDR_FAIL:	/* Bus Device Reset message rejected */
	case CMD_ID_FAIL:	/* Identify message rejected */
		stprintf(devp, "transport completed with %s",
			scsi_rname(pkt->pkt_reason));
		goto reset_target;

	default:
		stprintf(devp, "transport completed for unknown reason");
reset_target:
		if ((pkt->pkt_state & STATE_GOT_BUS) &&
		    ((pkt->pkt_statistics & (STAT_BUS_RESET | STAT_DEV_RESET |
			STAT_ABORTED)) == 0)) {
			stprintf(devp, "attempting a device reset");
			result = scsi_reset(ROUTE, RESET_TARGET);
			/*
			 * if target reset fails, then pull the chain
			 */
			if (result == 0) {
				stprintf(devp, "attempting a bus reset");
				result = scsi_reset(ROUTE, RESET_ALL);
			}
			if (result == 0) {
				/* no hope left to recover */
				stprintf(devp, "recovery by resets failed");
				return (rval);
			}
		}
		break;
	}

	if (un->un_state == ST_STATE_SENSING) {
		if (un->un_retry_ct++ < st_retry_count)
			rval = QUE_SENSE;
	} else if (un->un_retry_ct++ < st_retry_count) {
		if (bp == un->un_sbufp) {
			switch ((int)bp->b_forw) {
			case SCMD_MODE_SENSE:
			case SCMD_MODE_SELECT:
			case SCMD_REWIND:
			case SCMD_LOAD:
			case SCMD_TEST_UNIT_READY:
				/*
				 * These commands can be rerun with impunity
				 */
				rval = QUE_COMMAND;
				break;
			default:
				break;
			}
		}
	}

	if (un->un_state >= ST_STATE_OPEN) {
		stprintf(devp, fail, scsi_rname(pkt->pkt_reason),
			(rval == COMMAND_DONE_ERROR)?
			"giving up":"retrying command");
	}
	return (rval);
}


static int
st_handle_sense(devp)
register struct scsi_device *devp;
{
	register struct scsi_tape *un = UPTR;
	register struct buf *bp = un->un_runq;
	struct scsi_pkt *pkt = BP_PKT(bp), *rqpkt = un->un_rqs;
	register rval = COMMAND_DONE_ERROR, resid;
	int severity, amt;

	if (SCBP(rqpkt)->sts_busy) {
		stprintf (devp, "busy unit on request sense");
		if (un->un_retry_ct++ < st_retry_count) {
			timeout (strestart, (caddr_t)devp, ST_BSY_TIMEOUT);
			rval = JUST_RETURN;
		}
		return (rval);
	} else if (SCBP(rqpkt)->sts_chk) {
		stprintf (devp, "Check Condition on REQUEST SENSE");
		return (rval);
	}

	/* was there enough data?  */
	amt = SENSE_LENGTH - rqpkt->pkt_resid;
	if ((rqpkt->pkt_state&STATE_XFERRED_DATA) == 0 ||
	    (amt < SUN_MIN_SENSE_LENGTH)) {
		stprintf(devp, "REQUEST SENSE couldn't get sense data");
		return (rval);
	}

	/*
	 * If the drive is an MT-02, reposition the
	 * secondary error code into the proper place.
	 */
	/*
	 * XXX MT-02 is non-CCS tape, so secondary error code
	 * is in byte 8.  However, in SCSI-2, tape has CCS definition
	 * so it's in byte 12.
	 */
	if (un->un_dp->type == ST_TYPE_EMULEX)
		devp->sd_sense->es_code = devp->sd_sense->es_add_info[0];

	if (bp != un->un_sbufp) {
		if (devp->sd_sense->es_valid) {
			resid = (devp->sd_sense->es_info_1 << 24) |
				(devp->sd_sense->es_info_2 << 16) |
				(devp->sd_sense->es_info_3 << 8)  |
				(devp->sd_sense->es_info_4);
			if ((un->un_dp->options & ST_VARIABLE) == 0)
				resid *= un->un_dp->bsize;

		} else {
			resid = pkt->pkt_resid;
		}
		/*
		 * The problem is, what should we believe?
		 */
		if (resid && (pkt->pkt_resid == 0))
			pkt->pkt_resid = resid;
	} else {
		/*
		 * If the command is SCMD_SPACE, we need to get the
		 * residual as returned in the sense data, to adjust
		 * our idea of current tape position correctly
		 */
		if ((CDBP(pkt)->scc_cmd == SCMD_SPACE ||
		    CDBP(pkt)->scc_cmd == SCMD_WRITE_FILE_MARK) &&
		    (devp->sd_sense->es_valid)) {
			resid = (devp->sd_sense->es_info_1 << 24) |
			    (devp->sd_sense->es_info_2 << 16) |
			    (devp->sd_sense->es_info_3 << 8)  |
			    (devp->sd_sense->es_info_4);
			bp->b_resid = resid;
		} else {
			/*
			 * If the special command is SCMD_READ,
			 * the correct resid will be set later.
			 */
			resid = bp->b_bcount;
		}
	}

	if (DEBUGGING || st_error_level == STERR_ALL) {
		register char *p = (char *) devp->sd_sense;
		register i;
		printf("%s%d: sense data:", DNAME, DUNIT);
		for (i = 0; i < amt; i++) {
			printf(" 0x%x", *(p++)&0xff);
		}
		printf("\n      count 0x%x resid 0x%x pktresid 0x%x\n",
			bp->b_bcount, resid, pkt->pkt_resid);
	}

	switch (un->un_status = devp->sd_sense->es_key) {
	case KEY_NO_SENSE:
		severity = STERR_INFORMATIONAL;
		goto common;

	case KEY_RECOVERABLE_ERROR:
		severity = STERR_RECOVERED;
	common:
		/*
		 * XXX only want reads to be stopped by filemarks.
		 * Don't want them to be stopped by EOT.  EOT matters
		 * only on write.
		 */
		if (devp->sd_sense->es_filmk && !devp->sd_sense->es_eom) {
			rval = COMMAND_DONE;
		} else if (devp->sd_sense->es_eom) {
			rval = COMMAND_DONE;
		} else if (devp->sd_sense->es_ili) {
			/*
			 * Fun with variable length record devices:
			 * for specifying larger blocks sizes than the
			 * actual physical record size.
			 */
			if ((un->un_dp->options & ST_VARIABLE) && resid > 0) {
				/*
				 * XXX! Ugly
				 */
				pkt->pkt_resid = resid;
				rval = COMMAND_DONE;
			} else if ((un->un_dp->options & ST_VARIABLE) &&
			    resid < 0) {
				rval = COMMAND_DONE_ERROR_RECOVERED;
				bp->b_error = EINVAL;
			} else {
				severity = STERR_FATAL;
				rval = COMMAND_DONE_ERROR;
				bp->b_error = EINVAL;
			}
		} else {
			/*
			 * we hope and pray for this just being
			 * something we can ignore (ie. a
			 * truly recoverable soft error)
			 */
			rval = COMMAND_DONE;
		}
		if (devp->sd_sense->es_filmk) {
			DPRINTF_ALL(devp, "filemark");
			un->un_status = SUN_KEY_EOF;
			un->un_eof = ST_EOF_PENDING;
		}
		if (devp->sd_sense->es_eom) {
			DPRINTF_ALL(devp, "eom");
			un->un_status = SUN_KEY_EOT;
			un->un_eof = ST_EOM;
		}
		break;

	case KEY_ILLEGAL_REQUEST:

		if (un->un_laststate >= ST_STATE_OPEN)
			severity = STERR_FATAL;
		else
			severity = STERR_INFORMATIONAL;
		rval = COMMAND_DONE_ERROR;
		break;

	case KEY_VOLUME_OVERFLOW:
		un->un_eof = ST_EOM;
		/* FALL THROUGH */
	case KEY_MEDIUM_ERROR:
	case KEY_HARDWARE_ERROR:

		severity = STERR_FATAL;
		rval = COMMAND_DONE_ERROR;
		break;

	case KEY_BLANK_CHECK:
		severity = STERR_INFORMATIONAL;

		/*
		 * In case Blank Check and a variable length record
		 * ever happen simultaneously
		 */
		if (devp->sd_sense->es_ili) {
			DPRINTF(devp, "Blank Check and variable length\n");
			if ((un->un_dp->options & ST_VARIABLE) && resid > 0) {
				pkt->pkt_resid = resid;
			}
		}

		/*
		 * if not a special request and some data was xferred then it
		 * it is not an error yet
		 */
		if (bp != un->un_sbufp &&
		    (pkt->pkt_state & STATE_XFERRED_DATA)) {
			rval = COMMAND_DONE;
		} else {
			rval = COMMAND_DONE_ERROR_RECOVERED;
		}

		if (un->un_laststate >= ST_STATE_OPEN){
			DPRINTF(devp, "blank check");
			un->un_eof = ST_EOM;
		}
		if ((CDBP(pkt)->scc_cmd == SCMD_SPACE) &&
		    (un->un_dp->options & ST_KNOWS_EOD) &&
		    (severity = STERR_INFORMATIONAL)) {
			/*
			 * we were doing a fast forward by skipping multiple fmk
			 * at the time
			 */
			bp->b_flags |= B_ERROR;
			severity = STERR_RECOVERED;
			rval	 = COMMAND_DONE;
		}
		break;

	case KEY_WRITE_PROTECT:
		if (st_wrongtapetype(un)) {
			un->un_status = SUN_KEY_WRONGMEDIA;
			printf(wrongtape, DNAME, DUNIT);
			severity = STERR_UNKNOWN;
		} else
			severity = STERR_FATAL;
		rval = COMMAND_DONE_ERROR;
		bp->b_error = EACCES;
		break;

	case KEY_UNIT_ATTENTION:
		if (un->un_laststate <= ST_STATE_OPENING &&
				un->un_lastop == ST_OP_CTL) {
			severity = STERR_INFORMATIONAL;
			rval = COMMAND_DONE_ERROR;
		} else {
			severity = STERR_FATAL;
			rval = COMMAND_DONE_ERROR;
		}
		un->un_fileno = -1;
		break;

	case KEY_NOT_READY:
		/*
		 * XXX FIX ME XXX
		 */
		severity = STERR_FATAL;
		rval = COMMAND_DONE_ERROR;
		break;

	case KEY_ABORTED_COMMAND:

		if (devp->sd_sense->es_eom) {
			DPRINTF_ALL(devp, "eot on aborted command");
			un->un_eof = ST_EOM;
			severity = STERR_INFORMATIONAL;
			rval = COMMAND_DONE;
		} else {
			/*
			 * Probably a parity error...
			 */
			goto canretry;
		}
		break;
	default:
		if (devp->sd_sense->es_ili) {
			/*
			 * Variable length records again:
			 * in case sense key is something weird
			 */
			DPRINTF(devp,
			    "Undecoded Sense Key '%s' and variable length\n",
				sense_keys[un->un_status]);
			if ((un->un_dp->options & ST_VARIABLE) && resid > 0) {
				/*
				 * XXX! Ugly
				 */
				pkt->pkt_resid = resid;
				rval = COMMAND_DONE;
			} else {
				/*
				 * don't care if resid < 0 & variable mode
				 */
				severity = STERR_FATAL;
				rval = COMMAND_DONE_ERROR;
				bp->b_error = EINVAL;
			}
		} else {
			/*
			 * Undecoded sense key.  Try retries and hope
			 * that will fix the problem.  Otherwise, we're
			 * dead.
			 */
			stprintf(devp, "Unhandled Sense Key '%s'",
				sense_keys[un->un_status]);
		canretry:
			if (un->un_retry_ct++ < st_retry_count) {
				severity = STERR_RETRYABLE;
				rval = QUE_COMMAND;
			} else {
				severity = STERR_FATAL;
				rval = COMMAND_DONE_ERROR;
			}
		}
	}

	if (DEBUGGING_ALL ||
		(un->un_laststate > ST_STATE_OPENING &&
			severity >= st_error_level)) {
		sterrmsg(devp, severity);
	}
	return (rval);
}

static int
st_wrongtapetype(un)
struct scsi_tape *un;
{

	/*
	 * Hack to handle  600A, 600XTD, 6150 && 660 vs. 300XL tapes...
	 */

	if (un->un_dp && (un->un_dp->options & ST_QIC) && un->un_mspl) {
		switch (un->un_dp->type) {
		case ST_TYPE_WANGTEK:
		case ST_TYPE_ARCHIVE:
			/*
			 * If this really worked, we could go off of
			 * the density codes set in the modesense
			 * page. For this drive, 0x10 == QIC-120,
			 * 0xf == QIC-150, and 0x5 should be for
			 * both QIC-24 and, maybe, QIC-11. However,
			 * the h/w doesn't do what the manual says
			 * that it should, so we'll key off of
			 * getting a WRITE PROTECT error AND wp *not*
			 * set in the mode sense information.
			 */
			/*
			 * XXX  but we already know that status is
			 * write protect, so don't check it again.
			 */

			if (un->un_status == KEY_WRITE_PROTECT &&
						un->un_mspl->wp == 0) {
				return (1);
			}
			break;
		default:
			break;
		}
	}
	return (0);
}

static int
st_check_error(devp)
register struct scsi_device *devp;
{
	register struct scsi_tape *un = UPTR;
	struct buf *bp = un->un_runq;
	register struct scsi_pkt *pkt = BP_PKT(bp);
	register action;

	if (SCBP(pkt)->sts_busy) {
		DPRINTF_ALL(devp, "unit busy");
		if (un->un_retry_ct++ < st_retry_count) {
			timeout(strestart, (caddr_t)devp, ST_BSY_TIMEOUT);
			action = JUST_RETURN;
		} else {
			stprintf(devp, "unit busy too long");
			if (scsi_reset (ROUTE, RESET_TARGET) == 0) {
				(void) scsi_reset (ROUTE, RESET_ALL);
			}
			action = COMMAND_DONE_ERROR;
		}
	} else if (SCBP(pkt)->sts_chk) {
		action = QUE_SENSE;
	} else {
		action = COMMAND_DONE;
	}
	return (action);
}

static void
st_calc_bnum(un, bp)
struct scsi_tape *un;
struct buf *bp;
{
	if (un->un_dp->options & ST_VARIABLE)
		un->un_blkno += ((bp->b_bcount - bp->b_resid  == 0)? 0: 1);
	else
		un->un_blkno += ((bp->b_bcount - bp->b_resid) >> SECDIV);
}

static void
st_set_state(devp)
struct scsi_device *devp;
{
	struct scsi_tape *un = UPTR;
	struct buf *bp = un->un_runq;
	struct scsi_pkt *sp = BP_PKT(bp);


	DPRINTF(devp, "st_set_state: un_eof = %x, fmneeded=%x\n",
	    un->un_eof, un->un_fmneeded);

	if (bp != un->un_sbufp) {
		if (DEBUGGING_ALL && sp->pkt_resid) {
			stprintf(devp, "pkt_resid %d bcount %d",
			    sp->pkt_resid, bp->b_bcount);
		}
		bp->b_resid = sp->pkt_resid;
		st_calc_bnum(un, bp);
		if (bp->b_flags & B_READ) {
			un->un_lastop = ST_OP_READ;
		} else {
			un->un_lastop = ST_OP_WRITE;
			if (un->un_dp->options & ST_REEL) {
				un->un_fmneeded = 2;
			}
		}
	} else {
		char saved_lastop = un->un_lastop;

		un->un_lastop = ST_OP_CTL;

		switch ((int)bp->b_forw) {
		case SCMD_WRITE:
			bp->b_resid = sp->pkt_resid;
			un->un_lastop = ST_OP_WRITE;
			st_calc_bnum(un, bp);
			if (un->un_dp->options & ST_REEL) {
				un->un_fmneeded = 2;
			}
			break;
		case SCMD_READ:
			bp->b_resid = sp->pkt_resid;
			un->un_lastop = ST_OP_READ;
			st_calc_bnum(un, bp);
			break;
		case SCMD_WRITE_FILE_MARK:
			un->un_eof = ST_NO_EOF;
			un->un_fileno += (bp->b_bcount - bp->b_resid);
			un->un_blkno = 0;
			if (un->un_dp->options & ST_REEL) {
				un->un_fmneeded -=
					(bp->b_bcount - bp->b_resid);
				if (un->un_fmneeded < 0)
					un->un_fmneeded = 0;
			} else
				un->un_fmneeded = 0;

			break;
		case SCMD_REWIND:
			un->un_eof = ST_NO_EOF;
			un->un_fileno = 0;
			un->un_blkno = 0;
			break;

		case SCMD_SPACE:
		{
			int space_fmk, count;

			count = space_cnt(bp->b_bcount);
			space_fmk = ((bp->b_bcount) & (1<<24))? 1: 0;


			if (count >= 0) {
				if (space_fmk) {
					un->un_eof = ST_NO_EOF;
					un->un_fileno += (count - bp->b_resid);
					un->un_blkno = 0;
				} else {
					un->un_blkno += count - bp->b_resid;
				}
			} else if (count < 0) {
				if (space_fmk) {
					un->un_fileno -=
					    ((-count) - bp->b_resid);
					if (un->un_fileno < 0) {
						un->un_fileno = 0;
						un->un_blkno = 0;
					} else
						un->un_blkno = INF;
				} else {
					if (un->un_eof >= ST_EOF_PENDING) {
					/*
					 * we stepped back into
					 * a previous file; we are not
					 * making an effort to pretend that
					 * we are still in the current file
					 * ie. logical == physical position
					 * and leave it to stioctl to correct
					 */
						if (un->un_fileno > 0) {
							un->un_fileno--;
							un->un_blkno = INF;
						} else {
							un->un_blkno = 0;
						}
					} else
						un->un_blkno -=
						    (-count) - bp->b_resid;
				}
			}
			DPRINTF_IOCTL(devp, "aft_space rs %d fil %d blk %d",
				bp->b_resid, un->un_fileno, un->un_blkno);
			break;
		}
		case SCMD_LOAD:
			if (bp->b_bcount&0x1) {
				un->un_fileno = 0;
			} else {
				un->un_fileno = -1;
			}
			un->un_density_known = 0;
			un->un_eof = ST_NO_EOF;
			un->un_blkno = 0;
			break;
		case SCMD_ERASE:
			un->un_eof = ST_NO_EOF;
			un->un_blkno = 0;
			un->un_fileno = 0;
			break;
		case SCMD_MODE_SELECT:
		case SCMD_MODE_SENSE:
		case SCMD_TEST_UNIT_READY:
			un->un_lastop = saved_lastop;
			break;
		default:
			printf("%s%d: unknown cmd causes loss of state\n",
				DNAME, DUNIT);
			un->un_fileno = -1;
			break;
		}
	}

	un->un_err_resid = bp->b_resid;
	un->un_err_fileno = un->un_fileno;
	un->un_err_blkno = un->un_blkno;
	un->un_retry_ct = 0;

	/*
	 * If we've seen a filemark via the last read operation
	 * advance the file counter, but mark things such that
	 * the next read operation gets a zero count. We have
	 * to put this here to handle the case of sitting right
	 * at the end of a tape file having seen the file mark,
	 * but the tape is closed and then re-opened without
	 * any further i/o. That is, the position information
	 * must be updated before a close.
	 */

	if (un->un_lastop == ST_OP_READ && un->un_eof == ST_EOF_PENDING) {
		/*
		 * If we're a 1/2" tape, and we get a filemark
		 * right on block 0, *AND* we were not in the
		 * first file on the tape, and we've hit logical EOM.
		 * We'll mark the state so that later we do the
		 * right thing (in stclose(), ststrategy() or
		 * stioctl()).
		 *
		 */
		if ((un->un_dp->options & ST_REEL) &&
		    un->un_blkno == 0 && un->un_fileno > 0) {
			un->un_eof = ST_EOT_PENDING;
			DPRINTF(devp, "eot pending");
			un->un_fileno++;
			un->un_blkno = 0;
		} else {
			/*
			 * If the read of the filemark was a side effect
			 * of reading some blocks (i.e., data was actually
			 * read), then the EOF mark is pending and the
			 * bump into the next file awaits the next read
			 * operation (which will return a zero count), or
			 * a close or a space operation, else the bump
			 * into the next file occurs now.
			 */
			DPRINTF(devp, "resid=%x, bcount=%x\n",
				bp->b_resid, bp->b_bcount);
			if (bp->b_resid != bp->b_bcount) {
				un->un_eof = ST_EOF;
			} else {
				un->un_eof = ST_NO_EOF;
				un->un_fileno++;
				un->un_blkno = 0;
			}
			DPRINTF(devp, "eof of file %d, un_eof=%d",
				un->un_fileno, un->un_eof);
		}
	}
}

/*
 * Error printing routines
 *
 * There used to be bunch of strings matching
 * with Vendor Unique Error codes. I've removed
 * them. Customers aren't interested, and the
 * SunOS driver is not a diagnostic engine. If
 * a non-zero secondary error code exists, it
 * will be printed in hex to the console, and
 * the appropriate vendor manual will then be
 * consulted.
 */


static char *st_cmds[] = {
	"\000test unit ready",		/* 0x00 */
	"\001rewind",			/* 0x01 */
	"\003request sense",		/* 0x03 */
	"\010read",			/* 0x08 */
	"\012write",			/* 0x0a */
	"\020write file mark",		/* 0x10 */
	"\021space",			/* 0x11 */
	"\022inquiry",			/* 0x12 */
	"\025mode select",		/* 0x15 */
	"\031erase tape",		/* 0x19 */
	"\032mode sense",		/* 0x1a */
	"\033load tape",		/* 0x1b */
	NULL
};

static void
sterrmsg(devp, level)
register struct scsi_device *devp;
int level;
{
	static char *error_classes[] = {
		"All", "Unknown", "Informational",
		"Recovered", "Retryable", "Fatal"
	};
	char *class;
	struct scsi_tape *un = UPTR;
	struct scsi_pkt *pkt;

	class = error_classes[level];
	pkt = BP_PKT(un->un_runq);
	stprintf(devp, "Error for command '%s', Error Level: '%s'",
		scsi_cmd_decode(CDBP(pkt)->scc_cmd, st_cmds), class);
	printf("    \tBlock: %d", un->un_err_blkno);
	if (un->un_fileno >= 0)
		printf("\tFile Number: %d", un->un_fileno);
	printf("\n    \tSense Key: %s\n",
		sense_keys[devp->sd_sense->es_key]);
	/*
	 * Print the secondary error code, if any
	 */
	if (devp->sd_sense->es_code) {
		register char *vname = un->un_dp->name;
		auto char local[8+3];
		if (vname == NULL) {
			bcopy(devp->sd_inq->inq_vid, &local[1], 8);
			local[0] = '<';
			local[9] = '>';
			local[10] = 0;
			vname = local;
		}
		printf("    \tVendor (%s) Unique Error Code: 0x%x\n",
			vname, devp->sd_sense->es_code);
	}
	if (devp->sd_sense->es_filmk)
		printf("\tFile Mark Detected\n");
	if (devp->sd_sense->es_eom)
		printf("\tEnd-of-Media Detected\n");
	if (devp->sd_sense->es_ili)
		printf("\tIncorrect Length Indicator Set\n");
}

/*
 * I don't use <varargs.h> 'coz I can't make it work on sparc!
 */

/*VARARGS2*/
static void
stprintf(devp, fmt, a, b, c, d, e, f, g, h, i)
struct scsi_device *devp;
char *fmt;
int a, b, c, d, e, f, g, h, i;
{
	printf("%s%d:\t", DNAME, DUNIT);
	printf(fmt, a, b, c, d, e, f, g, h, i);
	printf("\n");
}


/*
 * Conditionally enabled debugging
 */
#ifdef	ST_DEBUG

int
st_debug_cmds(devp, com, count, wait)
struct scsi_device *devp;
int com, count, wait;
{
	printf("st: cmd=%s", scsi_cmd_decode(com, st_cmds));
	printf("  count=0x%x (%d)", count, count);
	printf("  %ssync\n", wait == ASYNC_CMD ? "a" : "");
}

#endif	ST_DEBUG

#endif	NST > 0
