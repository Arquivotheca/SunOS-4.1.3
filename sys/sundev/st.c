#include "st.h"
#if NST > 0

#ident	"@(#)st.c 1.1 92/07/30 SMI"

/*#define STDEBUG 			/* Allow compiling of debug code */

/*
 * Generic Driver for 1/2-inch reel, 1/2-inch cartridge,
 * and 1/4-inch cartridge embedded SCSI tape drives.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/map.h>
#include <sys/vm.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/syslog.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <sundev/mbvar.h>
#include <sundev/screg.h>
#include <sundev/sireg.h>
#include <sundev/scsi.h>
#include <sundev/streg.h>

extern struct scsi_unit stunits[];
extern struct scsi_unit_subr scsi_unit_subr[];
extern struct scsi_tape stape[];
extern struct st_drive_table st_drivetab[];
extern int cpu, nstape, st_drivetab_size;

struct st_drive_table st_drivetab1[] = ST_DRIVE1_INFO;
static char *iopb_error = "st%d:  no iopb space for buffer\n";
static char *wrong_media =
  "st%d:  wrong tape for writing, use DC6150 tape (or equivalent)\n";
static char *tape_worn1 = "st%d:  warning, the tape may be wearing out or\n";
static char *tape_worn2 = "      the head may need cleaning or\n";
static char *tape_worn3 = "      the drive may have a problem.\n";
static char *wprotect_error = "st%d:  write protected\n";
static char *size_error = "st%d:  %s: not modulo %d block size\n";
static char *position_error = "st%d:  %s positioning error\n";
static char *rewinding_msg = "st%d:  warning, rewinding tape\n";
static char *format_error = "st%d:  format change failed\n";
static char *format_msg = "st%d:  warning, using alternate tape format\n";
static char *retry_msg = "st%d:  %s retries= %u (%u.%u%%), file= %u, block= %u\n";
static char *hard_error = "st%d:  %s failed\n";
static char *sense_msg1 = "st%d error:  sense key(0x%x): %s\n";
static char *sense_msg2 = "st%d error:  sense key(0x%x): %s, error code(0x%x): %s\n";

#define LONG			(sizeof(u_long))
#define LONG_ALIGN(arg)	  	(((u_int)arg + LONG -1) & ~(LONG -1))

#define SHORT_TIMEOUT 		  4 	/* Short timeout (2*minutes) */
#define NORMAL_TIMEOUT 		  6 	/* Read/Write timeout (2*minutes) */
#define REWIND_TIMEOUT 		 12 	/* Rewind timeout (2*minutes) */
#define SPACE_TIMEOUT 		120 	/* Space file timeout (2*minutes) */
#define ERASE_TIMEOUT 		120 	/* Erase timeout (2*minutes) */
#define LONG_ERASE_TIMEOUT 	360	/* 3 hr. erase timeout (2*minutes) */

#define SIGNALS	(sigmask(SIGHUP)  | sigmask(SIGKILL) | sigmask(SIGQUIT) | \
		 sigmask(SIGSTOP) | sigmask(SIGCONT) | sigmask(SIGINT)  | \
		 sigmask(SIGALRM) | sigmask(SIGTSTP) | sigmask(SIGTTIN) | \
		 sigmask(SIGTTOU) | sigmask(SIGTERM))
/*#define	PRIST	((PZERO+1) | PCATCH)	/* For interruptible sleep */
#define	PRIST	PRIBIO			/* For non-interruptible sleep */
int st_sleep = PRIBIO;			/* For non-interruptible sleep */
/*int st_sleep = PRIST;			/* For interruptible sleep */
int st_debug = 0;			/* 0 = normal operation
					 * 1 = extended error info
					 * 2 = adds debug info
					 * 3 = all status info
					 */
#ifdef STDEBUG
short st_retry = 0;		/* Enable special soft error reporting */
/* Handy debugging 0, 1, and 2 argument printfs */
#define DPRINTF(str) \
	if (st_debug > 1) printf(str)
#define DPRINTF1(str, arg1) \
	if (st_debug > 1) printf(str,arg1)
#define DPRINTF2(str, arg1, arg2) \
	if (st_debug > 1) printf(str,arg1,arg2)

/* Handy extended error reporting 0, 1, and 2 argument printfs */
#define EPRINTF(str) \
	if (st_debug) printf(str)
#define EPRINTF1(str, arg1) \
	if (st_debug) printf(str,arg1)
#define EPRINTF2(str, arg1, arg2) \
	if (st_debug) printf(str,arg1,arg2)

#else STDEBUG
short st_retry = 1;		/* Disable special soft error reporting */
#define DPRINTF(str)
#define DPRINTF1(str, arg2)
#define DPRINTF2(str, arg1, arg2)
#define EPRINTF(str)
#define EPRINTF1(str, arg2)
#define EPRINTF2(str, arg1, arg2)
#endif STDEBUG

/*
 * NOTES:
 *    This device driver supports both fixed length block I/O or
 * variable length block I/O.  Fixed length I/O is common for
 * 1/4-inch tape drives.  Variable length is common for 1/2-inch
 * reel (Pertec) tape drives and various other types of tape
 * drives.
 *
 *   To add a new tape device one has sereral choices.  One
 * can rely on the built-in adaptive code to take care of
 * things.  The default tape drive id will be reported for
 * this class of tape drive.
 *
 *   The next method of adding a tape drive is to use adb
 * and enter the pertinent drive information in the st_drive_table
 * structure of st.o.  This solution allows customers to add
 * tape devices which can support more than one tape density.
 * There is one spare entry provided, named SPARE in the table,
 * to allow for this contingency.
 *
 *   The final solution for adding a new tape device rquires
 * access to the source of st.c and streg.h.  First add a
 * new entry in the streg.h file table, st_drive_table.
 * Then modify the following routines in st.c to provide
 * complete support for this device:  st_get_retries,
 * st_get_error_code, and st_print_error.  Also, a new
 * error message table may need to be optionally added
 * (like st_emulex_error_str).
 *
 *   Command timeouts don't work 100% of the time.  Si raises
 * it's interrupt priority so high that timeouts are blocked.
 * Unfortunately, this is usually triggered by spurious phase changes
 * and it doesn't appear to ever lower it.  Under these conditions,
 * the whole system will hang.
 *
 *    Generic tape device operation assumes the device is capable
 * of fixed-length I/O.  A mode sense is done to obtain the device's
 * physical record size.  If the record size is zero, the default
 * physical record size (512 bytes/record) will be used.  This
 * will be checked at mode select time.  Also, only one density/format
 * is supported, density 0.
 *
 *    Be careful when modifying the following variables as these
 * are used throughout this driver:  dsi->un_lastiow, dsi->next_block,
 * dsi->un_last_block, dsi->un_openf, dsi->un_ctype, dsi->un_eof,
 * dsi->un_last_bcount, dsi->un_last_count, dsi->un_offset,
 * and dsi->un_iovlen.
 */

/*
 * Return a pointer to this unit's unit structure.
 */
stunitptr(md)
	register struct mb_device *md;
{
	return ((int) &stunits[md->md_unit]);
}


/*
 * Deadman timer for tape commands.  Also, used for generating timing delays
 * while waiting for tape to load.
 */
static
sttimer(dev)
	register dev_t dev;
{
	register struct scsi_tape *dsi;
	register struct scsi_unit *un;
	register struct scsi_ctlr *c;
	register u_int	unit;

	unit = MTUNIT(dev);
	un = &stunits[unit];
	dsi = &stape[unit];
	c = un->un_c;

	if (dsi->un_openf >= OPENING) {
		(*c->c_ss->scs_start)(un);
		if ((dsi->un_timeout > 0)  &&  (--dsi->un_timeout == 0)) {

			/* Process command timeout for normal I/O operations */
			log(LOG_CRIT, "st%d:  I/O request timeout\n", unit);
			dsi->un_timeout = 4;
			if ((*c->c_ss->scs_deque)(c, un)) {
				/* Can't recover, reset! */
				dsi->un_timeout = 0;
				dsi->un_openf = CLOSED;
				(*c->c_ss->scs_reset)(c, 1);
			}
		} else {
			EPRINTF1("st%d: sttimer running ", unit);
			EPRINTF2("open= %d,  timeout= %d\n",
				 dsi->un_openf, dsi->un_timeout);
		}
		timeout(sttimer, (caddr_t)dev, 30*hz);
	} else {
		/* Process tape opening delay timeout */
		DPRINTF1("st%d: sttimer running...\n", unit);
		wakeup((caddr_t)dev);
	}
}


/*
 * Attach device (boot time).  Initialize data structures and
 * allocate inquiry/mode_select/mode_sense/sense buffer.
 */
stattach(md)
	register struct mb_device *md;
{
	register struct scsi_unit *un;
	register struct scsi_tape *dsi;
	/*DPRINTF("stattach:\n");*/

	un = &stunits[md->md_unit];
	dsi = &stape[md->md_unit];
	un->un_md = md;
	un->un_mc = md->md_mc;
	un->un_unit = md->md_unit;
	un->un_target = TARGET(md->md_slave);
	un->un_lun = LUN(md->md_slave);
	un->un_ss = &scsi_unit_subr[TYPE(md->md_flags)];
	un->un_blkno = un->un_count = 0;
	un->un_present = 0;

	dsi->un_bufcnt = 0;			/* No I/O request in que */
	dsi->un_iovlen = 0;			/* Guarantee block size check */
	dsi->un_offset = 0;
	dsi->un_lastior = dsi->un_lastiow = 0;
	dsi->un_next_block = 0;			/* Assume file #0, Blk #0 */
	dsi->un_last_block = INF;
	dsi->un_blocks = 0;
	dsi->un_records = 0;
	dsi->un_fileno = 0;
	dsi->un_eof = ST_NO_EOF;

	dsi->un_timeout = 0;			/* Disable timeouts */
	dsi->un_reset_occurred = 2;		/* Force init */
	dsi->un_density = 0;
	dsi->un_openf = CLOSED;
	dsi->un_ctype = ST_TYPE_INVALID;	/* Set drive defaults */
	dsi->un_flags = 0;			/* Special un->un_flags copy */
	dsi->un_last_count = 1;			/* Must not match un_count */
	dsi->un_last_bcount = 0;
	dsi->un_mspl = NULL;
	dsi->un_msplsize = 0;
	dsi->un_dtab = (int *)&st_drivetab1[2];	/* Set default device */
}

/*
 * Log error stats.  Called by stinit and stopen.
 */
static
stinit_error(dsi, status)
	register struct scsi_tape *dsi;
	u_char status;
{
	dsi->un_err_resid = 0;
	dsi->un_err_fileno = dsi->un_fileno;
	dsi->un_err_blkno = dsi->un_next_block;
	dsi->un_status = status;
	dsi->un_reset_occurred = 2;
	dsi->un_openf = CLOSED;
}


/*
 * Initialize tape device (called by stopen).
 * Check if device on-line and identify who it is.
 */
/*ARGSUSED*/
static
stinit(dev, io_flag)
	dev_t	dev;
	int	io_flag;
{
	register struct scsi_tape *dsi;
	register struct scsi_unit *un;
	register struct scsi_inquiry_data *sid;
	register struct st_drive_table *dp;
	struct st_sense *ssd;
	register int i;
	int unit, error = 0;
	long buf_size;
	u_char flags;

	unit = MTUNIT(dev);
	un = &stunits[unit];
	dsi = &stape[unit];
	flags = dsi->un_flags;			/* save current settings */
	/*DPRINTF2("stinit:  un= 0x%x, un->un_c= 0x%x\n", un, un->un_c);*/

	/*
	 * If first time for drive or after it had a SE_FATAL error
	 * to a full initialization pass to check it out.  Otherwise,
	 * if we already know who it is, use the short check.
	 */
	if (dsi->un_ctype == ST_TYPE_INVALID  ||
	    dsi->un_ctype == ST_TYPE_DEFAULT  ||
	    un->un_present == 0) {
		buf_size = DEV_BSIZE + LONG;
		dsi->un_ctype = ST_TYPE_DEFAULT;	/* Set drive defaults */
	} else {
		buf_size = sizeof (struct scsi_inquiry_data) + LONG;
	}

	/*
	 * Allocate buffer for mode_select/inquiry/request_sense data
	 * in iopb memory and align to long word if not done already.
	 */
	if ((int)dsi->un_mspl == NULL) {
		sid = (struct scsi_inquiry_data *) rmalloc(iopbmap, buf_size);
		if (sid == NULL) {
			log(LOG_CRIT, iopb_error, MTUNIT(un->un_unit));
			return (ENXIO);
		}
		dsi->un_msplsize = buf_size;
		dsi->un_rmspl = (int *)sid;
		sid = (struct scsi_inquiry_data *) LONG_ALIGN(sid);
		dsi->un_mspl = (int *)sid;
		EPRINTF2("st%d:  stinit: buffer= 0x%x", MTUNIT(un->un_unit),sid);
		EPRINTF2(", dsi= 0x%x, un= 0x%x\n", dsi, un);
	}
	sid = (struct scsi_inquiry_data *) dsi->un_mspl;
	ssd = (struct st_sense *) dsi->un_mspl;
#ifdef 	lint
	sid = sid; ssd = ssd;
#endif	lint

	/*
	 * Check if the tape drive is ready.  If it's not ready then
	 * either the tape has been changed, or it's offline, or maybe
	 * it's not there (offline).  Note, this also catches power-on reset
	 * and tape change conditions as they both generate unit attention
	 * check condition status.  If the drive is auto-loading, we'll
	 * get a busy indication until it's done.  In any case, it
	 * probably would be a good idea to set the reset occurred
	 * indicator.
	 */
#ifdef	ST_SYSGEN
	if (IS_SYSGEN(dsi)) {
		/*
		 * If we're in read mode, you'd better want to read.
		 * Otherwise, we're going to have to see if you've
		 * changed the tape.  If we have to check, this'll
		 * probably rewind the tape too.
		 */
		if (dsi->un_lastior  &&  ((io_flag & FWRITE) == 0)  &&
		    (dsi->un_fileno != 0)) {
			EPRINTF("stinit:  Sysgen in read mode\n");
			goto STINIT_EXIT;
		}
		if (dsi->un_lastior  &&  (dsi->un_fileno != 0)  &&
		    (dsi->un_next_block != 0)) {
			log(LOG_ERR, rewinding_msg, unit);
		}

		dsi->un_flags = SC_UNF_NO_DISCON;	/* no disconnects */
		dsi->un_openf = OPENING;
		if (stcmd(dev, SC_REQUEST_SENSE, 0)  &&
		    dsi->un_openf != OPENING_SYSGEN) {
			EPRINTF("stinit:  Sysgen restart\n");
			dsi->un_ctype = ST_TYPE_DEFAULT;	/* Force identify */
			dsi->un_reset_occurred = 2;
			dsi->un_fileno = 0;
			dsi->un_next_block = 0;
		} else {
			EPRINTF("stinit:  Sysgen ready\n");
			goto STINIT_EXIT;
		}
	}
#endif	ST_SYSGEN

	dsi->un_flags = SC_UNF_NO_DISCON;	/* no disconnects */
	dsi->un_openf = OPENING;		/* special error handling */
	if (stcmd(dev, SC_TEST_UNIT_READY, 0)) {

		/* Major SCSI device failure, stop! */
		DPRINTF("stinit:  check condition\n");
		if (dsi->un_openf == CLOSED) {
			EPRINTF1("st%d:  hardware failure, check drive\n", unit);
			error = ENXIO;
			goto STINIT_ERROR;
		}
		/*
		 * Last chance -- If this is the first command after
		 * a SCSI reset, we'll always get a unit attention error
		 * the first time.  Giving it one more chance neatly
		 * hides it.  By the way retrying this command also
		 * provides the necessary delay if you just inserted
		 * the tape and tried to access it.  Don't change the
		 * number of test unit ready retries!
		 *
		 * Note, if sysgen support is enabled, we'll have to do
		 * a request sense after the first test unit ready or
		 * the dumb sysgen won't ever clear its check condition.
		 */
#ifdef		ST_SYSGEN
		if (IS_SYSGEN(dsi)  ||  IS_DEFAULT(dsi)) {
			dsi->un_openf = OPENING;
			if (stcmd(dev, SC_REQUEST_SENSE, 0)  &&
			    dsi->un_openf == OPENING_SYSGEN) {
				EPRINTF("stinit:  Sysgen tape drive found\n\n");
				goto STINIT_EXIT;
			}
		}
#endif		ST_SYSGEN

		/*
		 * Auto-loading tape drives need a few seconds
		 * after tape insertion to figure out that a tape
		 * has been inserted.
		 */
		dsi->un_openf = OPENING_DELAY;
		timeout(sttimer, (caddr_t)dev, 3*hz);
		(void) sleep((caddr_t) dev, PRIBIO);
		untimeout(sttimer, (caddr_t)dev);
		dsi->un_reset_occurred = 2;

		/*
		 * Do 2 test unit readys to survive the case of the tape
		 * drive finished auto-loading between the first test unit
		 * ready and here.  We have to do it twice to get both the
		 * unit attention and the drive not ready errors.  If this
		 * fails, then either we're not finished auto-loading or
		 * the drive really is offline.
		 */
		dsi->un_openf = OPENING;
		if (stcmd(dev, SC_TEST_UNIT_READY, 0) == 0) {
			dsi->un_openf = OPENING;
			(void) stcmd(dev, SC_TEST_UNIT_READY, 0);
		}

		if (dsi->un_openf == OPEN_FAILED_LOADING) {
			/*
			 * This takes care of auto-load busy status until the
			 * tape drive is ready.  Each loop sleeps for 2 Sec.
			 * to reduce CPU loading as this process can take
			 * quite a while.
			 */
			i = MAX_AUTOLOAD_DELAY;
			dsi->un_openf = OPENING;
			while((stcmd(dev, SC_TEST_UNIT_READY, 0))  &&
			      (i-- > 0))  {
				dsi->un_openf = OPENING_DELAY;
				timeout(sttimer, (caddr_t)dev, 2*hz);
				/* Allow user to control-C out of this sleep */
				if (sleep((caddr_t)dev, PRIST)) {
					EPRINTF("stopen:  sleep interrupted\n");
					error = EINTR;
					goto STINIT_ERROR;
				}
				untimeout(sttimer, (caddr_t)dev);
			}
		}
		/* If tape isn't loaded by now, quit! */
		if (dsi->un_openf == OPEN_FAILED_LOADING  ||
		    dsi->un_openf == OPEN_FAILED_TAPE  ||
		    dsi->un_openf == CLOSED) {
			error = EIO;
			goto STINIT_ERROR;
		}

		/*
		 * If we had a check condition on the initial test unit
		 * ready we may have had a media change error.  Do a
		 * mode sense to check.  Note, if this is the first
		 * time or a Sysgen controller, don't do it!
		 * Note, this will set a read-only bit via
		 * stintr_opening routine which can be checked in stopen.
		 */
		DPRINTF("stinit:  after check condition\n");
		if (dsi->un_ctype != ST_TYPE_DEFAULT  &&
		    dsi->un_ctype != ST_TYPE_SYSGEN) {
			dsi->un_openf = OPENING;
			(void) stcmd(dev, SC_MODE_SENSE, 0);
		}
	}
	/*
	 * If this is the first time that this device has been opened,
	 * we'll need to figure out what type of tape drive it is.  After
	 * that we'll always remember it.  Also, start up the deadman
	 * timer.
	 */
	if (dsi->un_ctype != ST_TYPE_DEFAULT) {
#ifdef		ST_SYSGEN
		if (IS_SYSGEN(dsi))
			dsi->un_reset_occurred = 2;
#endif		ST_SYSGEN
		DPRINTF("stinit:  quick exit\n");
		un->un_present = 1;
		dsi->un_openf = OPEN;		/* normal error handling */
		dsi->un_flags = flags;		/* restore saved flags */
		dsi->un_timeout = 0;
		timeout(sttimer, (caddr_t)dev, 30*hz);
		return (0);
	}

	DPRINTF("stinit:  doing inquiry\n");
	dsi->un_openf = OPENING;
	if (stcmd(dev, SC_INQUIRY, 0)) {
#ifdef		ST_SYSGEN
		/*
		 * Check for Sysgen tape controller.  This is done by
		 * doing a mode sense followed by a request sense after
		 * it fails.  If the error code is 0x20, then we're
		 * talking to a sysgen.  Otherwise, it's a non-CCS tape
		 * controller.  Also, make sure there is a tape loaded
		 * in the tape drive, or else !
		 */
		dsi->un_openf = OPENING;
		if (stcmd(dev, SC_REQUEST_SENSE, 0) == 0) {
			if (dsi->un_openf == OPENING_SYSGEN) {
#ifdef				sun2
				/*
				* Check for a cipher tape drive if we're on
				* a Sun-2/120 system.  A cipher tape driver
				* can be recognized by issuing the vendor
				* unique read extended status command.  Note,
				* that afterwards that a SCSI bus reset will
				* be required to reset the QIC-02 bus.  This
				* is necessary because it does not support the
				* QIC-24 format (it erases the tape instead).
				*
				* Do a cipher read status command to see
				* if it's an old QIC-11 drive.  If command
				* works, we'll have to reset the bus to
				* continue.  And of course, we have to
				* do a test unit ready to get rid of reset
				* status.
				*/
				if ((cpu == CPU_SUN2_120)  &&
				    (stcmd(dev, SC_READ_XSTATUS_CIPHER, 0) == 0)) {
					EPRINTF("stinit:  Sun2 Cipher drive found\n");
					(*un->un_c->c_ss->scs_reset)(un->un_c, 0);
					dsi->un_openf = OPENING;
					(void) stcmd(dev, SC_TEST_UNIT_READY, 0);
					dsi->un_openf = OPENING;
					(void) stcmd(dev, SC_REQUEST_SENSE, 0);
					dp = &st_drivetab1[1];
					goto STINIT_SET_DRIVE;
				} else if (cpu == CPU_SUN2_120) {
					(void) stcmd(dev, SC_REQUEST_SENSE, 0);
				}
#endif				sun2
				EPRINTF("stinit:  Sysgen tape controller found\n\n");
				dp = &st_drivetab1[0];
				goto STINIT_SET_DRIVE;

			} else if (dsi->un_openf == OPEN_FAILED_TAPE) {
				EPRINTF1("st%d:  no tape loaded\n", unit);
				dsi->un_options = 0;
				error = EIO;
				goto STINIT_ERROR;
			}
		}
#endif		ST_SYSGEN
		/* Handle non-CCS tape drive types */
		/*DPRINTF1("st%d:  unknown SCSI device found\n", unit);*/
		dsi->un_ctype = ST_TYPE_INVALID;
		error = ENXIO;
		goto STINIT_ERROR;
	}
	/*
	 * Determine type of tape controller (see streg.h).
	 */
	DPRINTF("stinit: checking inquiry data\n");
	for (dp = st_drivetab; dp < &st_drivetab[st_drivetab_size]; dp++) {
		if (bcmp(sid->vid, dp->vid, dp->length) == 0) {
			/* Drive found, set stats */
			goto STINIT_SET_DRIVE;
		}
	}
	/*
	 * Don't know anything about this tape drive.  Use at your own risk !!
	 * Note, if block size is zero, we'll assumed variable-length
	 * block size.
	 */
	if (un->un_present == 0) {
		EPRINTF1("stinit: vid= <%s>\n", sid->vid); 
		EPRINTF2("        dtype= %d, rdf= %d\n", sid->dtype, sid->rdf);
#ifdef		STDEBUG
		if (st_debug > 2)
			st_print_sid_buffer(sid, 32);
#endif		STDEBUG
		if (sid->dtype != 1) {
			EPRINTF("stinit: not a tape\n"); 
			dsi->un_ctype = ST_TYPE_INVALID;
			error = ENXIO;
			goto STINIT_ERROR;
		}
		log(LOG_WARNING, "st%d:  warning, unknown tape drive found\n",
		       unit);
	}
	dsi->un_openf = OPENING;
	(void) stcmd(dev, SC_MODE_SENSE, 0);
	if (dsi->un_dev_bsize != 0)
		dp = &st_drivetab1[2];	/* Fixed-length I/O */
	else
		dp = &st_drivetab1[3];	/* Variable-length I/O */
	/* goto STINIT_SET_DRIVE; */

STINIT_SET_DRIVE:
	/* Store tape drive characteristics. */
	EPRINTF1("stinit:  %s drive found\n\n", dp->name);
	dsi->un_dtab = (int *)dp;
	dsi->un_ctype = dp->type;
	dsi->un_dev_bsize = dp->bsize;
	dsi->un_options = dp->options;

	/* If zero, this could panic kernel if tape repositioned */
	if (dsi->un_dev_bsize == 0)
		dsi->un_dev_bsize = DEV_BSIZE;
	dsi->un_phys_bsize = dsi->un_dev_bsize;

#ifdef	STDEBUG
	if (st_debug > 2) {
		printf("  bsize= 0x%x type= 0x%x\n", dp->bsize, dp->type);
		printf("  density= 0x%x, 0x%x, 0x%x, 0x%x\n",
			dp->density[0], dp->density[1],
			dp->density[2], dp->density[3]);
		printf("  speed= 0x%x, 0x%x, 0x%x, 0x%x\n",
			dp->speed[0], dp->speed[1],
			dp->speed[2], dp->speed[3]);
	}
#endif	STDEBUG

	/*
	 * Do mode sense to check for write protected tape,
	 * except for Sysgen, which doesn't do mode sense's.
	 */
	if (! IS_SYSGEN(dsi)) {
		dsi->un_openf = OPENING;
		(void) stcmd(dev, SC_MODE_SENSE, 0);
	}

STINIT_EXIT:
	/* Check if disconnects disabled */
	if (dsi->un_options & ST_NODISCON) {
		/*DPRINTF("stinit:  disconnects disabled\n");*/
		flags = SC_UNF_NO_DISCON;
	} else {
		/*DPRINTF("stinit:  disconnects enabled\n");*/
		flags = 0;
	}

	/*
	 * Tape is ready to go.  Set mode to OPEN to signify the
	 * fact.  Also, start the free-running deadman timer.
	 * Return successful status to stopen.
	 */
	un->un_present = 1;
	dsi->un_openf = OPEN;
	dsi->un_flags = flags;			/* restore saved flags */
	dsi->un_timeout = 0;
	timeout(sttimer, (caddr_t)dev, 30*hz);
	return (0);


STINIT_ERROR:
	dsi->un_flags = flags;		/* restore saved flags */
	stinit_error(dsi, SC_NOT_READY);
	if ((int)dsi->un_mspl) {
		rmfree(iopbmap, (long)dsi->un_msplsize, (u_long)dsi->un_rmspl);
		dsi->un_mspl = NULL;
		dsi->un_msplsize = 0;
	}
	return (error);
}

/*
 * Main entry point for opening tape driver.  Checks for device
 * unused and present.  Also, sets recording format/density.
 */
stopen(dev, flag)
	register dev_t dev;
	int flag;
{
	register struct scsi_unit *un;
	register u_int unit;
	register int i, s;
	register int status;
	register struct scsi_tape *dsi;

	unit = MTUNIT(dev);
	if (unit >= nstape) {
		EPRINTF1("st%d:  stopen: invalid unit\n", unit);
		return (ENXIO);
	}
	un = &stunits[unit];
	dsi = &stape[unit];

	if (un->un_mc == 0) {
		EPRINTF1("st%d:  stopen: invalid unit\n", unit);
		return (ENXIO);
	}

	/*
	 * Can't open a drive that's already open.  If closing
	 * or closed, go ahead with open.  If open, wait for a
	 * while and hope it'll free up.  If not, return busy.
	 * Note, raise interrupt priority high enough to prevent
	 * I/O requestor from being swapped while we're checking.
	 */
	s = splr(pritospl(un->un_mc->mc_intpri));
	if (dsi->un_openf > CLOSING) {
		(void) splx(s);
		for(i = 0; i < MAX_AUTOLOAD_DELAY *2; i++)  {
			EPRINTF("stopen:  waiting...\n");
			/* Allow user to control-C out of this sleep */
			if (sleep((caddr_t) &lbolt, PRIST)) {
				EPRINTF("stopen:  sleep interrupted\n");
				return (EINTR);
			}
			s = splr(pritospl(un->un_mc->mc_intpri));
			if (dsi->un_openf <= CLOSING)
				goto STOPEN_READY;
			(void) splx(s);
		}
		if (dsi->un_openf == OPENING  && dsi->un_timeout == 0)
			dsi->un_timeout = 4;	/* Insurance */
		(void) splx(s);
		EPRINTF1("stopen:  open state= %d\n", dsi->un_openf);
		return (EBUSY);
	}

STOPEN_READY:
	dsi->un_openf = OPENING;
	(void) splx(s);

	/*
	 * Open tape drive. Also, make sure it's online.  Note, this
	 * will also set stintr state machine to open state.
	 */
	if ((status=stinit(dev, flag)) != 0)
		return (status);
	/* dsi->un_openf = OPEN; */

	/*
	 * While we're opening the tape, check for write protect.  If
	 * it is and we're opening the tape for write operations, you're
	 * dead.  Note, this error will also force the tape drive to be
	 * reinitialized via stinit.  Also, log stats for 'mt status'.
	 */
	if (flag & FWRITE) {
		if (dsi->un_read_only > 1) {
			/* Check if write protected or wrong media error */
			if (dsi->un_read_only > 2) {
				log(LOG_ERR, wrong_media, unit);
				stinit_error( dsi, SC_MEDIA);
			} else {
				log(LOG_ERR, wprotect_error, unit);
				stinit_error(dsi, SC_WRITE_PROTECT);
			}
			return (EACCES);
		}
		dsi->un_read_only = 0;

		/* If opening in write-only mode, force eof write */
		if (flag == FWRITE)
			dsi->un_lastiow = 3;

	/*
	 * If opening for read only, set the write protect flag if
	 * it's not already set.
	 */
	} else if ((flag & FWRITE) == 0  &&  dsi->un_read_only == 0)
		dsi->un_read_only = 1;

	/*
	 * If at BOT, use whatever the user selects.  Otherwise,
	 * use what worked last time.
	 */
	if (dsi->un_fileno == 0  &&  dsi->un_next_block == 0)
		dsi->un_options &= ~ST_AUTODEN_OVERRIDE;

	/*
	 * If a device reset occurred, the mode select information
	 * should be given to the tape device again just to be safe.
	 */
	if (dsi->un_reset_occurred) {
		EPRINTF("stopen:  resetting tape device\n");
		if (dsi->un_reset_occurred > 1) {
			dsi->un_fileno = 0;		/* suppress rewinding msg */
			dsi->un_next_block = 0;
			dsi->un_offset = 0;
			if (! (IS_SYSGEN(dsi)  ||  IS_EMULEX(dsi)))
				(void) stcmd(dev, SC_RESERVE, 0);
		}
		dsi->un_density = -1;			/* force mode select */
		dsi->un_options &= ~ST_AUTODEN_OVERRIDE;
		dsi->un_eof = ST_NO_EOF;
		dsi->un_reset_occurred = 0;
	}

#ifdef	sun2
	/*
	 * For Sysgen on Sun 2, if it doesn't support QIC-24 don't do mode
	 * selects.  Note, if this is attempted for this combo the tape will
	 * be erased.)
	 */
	if (dsi->un_options & ST_NO_QIC24) {
		EPRINTF("stopen:  using default density\n");
		dsi->un_density = SC_DENSITY0;
		return (0);
	}
#endif	sun2

	switch (minor(dev) & MT_DENSITY_MASK) {
	case MT_DENSITY1:
		/* Check for need to convert density */
		if (dsi->un_density == SC_DENSITY0  ||
		    (dsi->un_options & ST_AUTODEN_OVERRIDE)) {
			break;
		}
		if (dsi->un_fileno != 0  ||  dsi->un_next_block != 0)
			log(LOG_ERR, rewinding_msg, unit);

		/* Special case -- mode select does not abort open */
		if ((status=stcmd(dev, SC_REWIND, 0)) == EINTR  ||
		    (status=stcmd(dev, SC_DENSITY0, 0))) {
			log(LOG_WARNING, format_error, unit);
			if (status == EINTR) {
				dsi->un_ctype = ST_TYPE_INVALID;
				dsi->un_openf = CLOSED;
				return (status);
			}
		}
		dsi->un_density = SC_DENSITY0;
		break;

	case MT_DENSITY2:
		/* Check for need to convert density */
		if (dsi->un_density == SC_DENSITY1)
			break;

		/* Must convert to specified density */
		if (dsi->un_options & ST_AUTODEN_OVERRIDE){
			log(LOG_WARNING, format_msg, unit);
			break;
		}
		if (dsi->un_fileno != 0  ||  dsi->un_next_block != 0)
			log(LOG_ERR, rewinding_msg, unit);

		if ((status=stcmd(dev, SC_REWIND, 0)) == EINTR  ||
		    (status=stcmd(dev, SC_DENSITY1, 0))) {
			log(LOG_WARNING, format_error, unit);
			dsi->un_ctype = ST_TYPE_INVALID;
			dsi->un_openf = CLOSED;
			return (status);
		}
		dsi->un_density = SC_DENSITY1;
		break;

	case MT_DENSITY3:
		/* Check for need to convert density */
		if (dsi->un_density == SC_DENSITY2)
			break;

		/* Must convert to specified density */
		if (dsi->un_options & ST_AUTODEN_OVERRIDE){
			log(LOG_WARNING, format_msg, unit);
			break;
		}
		if (dsi->un_fileno != 0  ||  dsi->un_next_block != 0)
			log(LOG_ERR, rewinding_msg, unit);

		if ((status=stcmd(dev, SC_REWIND, 0)) == EINTR  ||
		    (status=stcmd(dev, SC_DENSITY2, 0))) {
			log(LOG_WARNING, format_error, unit);
			dsi->un_ctype = ST_TYPE_INVALID;
			dsi->un_openf = CLOSED;
			return (status);
		}
		dsi->un_density = SC_DENSITY2;
		break;

	case MT_DENSITY4:
		/* Check for need to convert density */
		if (dsi->un_density == SC_DENSITY3)
			break;

		/* Must convert to specified density */
		if (dsi->un_options & ST_AUTODEN_OVERRIDE){
			log(LOG_WARNING, format_msg, unit);
			break;
		}
		if (dsi->un_fileno != 0  ||  dsi->un_next_block != 0)
			log(LOG_ERR, rewinding_msg, unit);

		if ((status=stcmd(dev, SC_REWIND, 0)) == EINTR  ||
		    (status=stcmd(dev, SC_DENSITY3, 0))) {
			log(LOG_WARNING, format_error, unit);
			dsi->un_ctype = ST_TYPE_INVALID;
			dsi->un_openf = CLOSED;
			return (status);
		}
		dsi->un_density = SC_DENSITY3;
		break;
	}
	return (0);
}

/*
 * Write 1 or 2 filemarks, depending on type of device.
 * If lastiow = 3 and norewind, write 1 eof.
 * If lastiow = 3 and rewind, write 1 or 2 eofs.
 * If lastiow = 1 and no rewind, don't write an eof.
 * If lastiow = 1 and rewind, write 0 or 1 eofs.
 */
static
stwrite_eof(dev, dsi)
	register dev_t dev;
	register struct scsi_tape *dsi;
{
	int status, state;
	/*
	 * If the last I/O to the tape drive is complete,  write a
	 * filemark before closing.  Note, for 1/2-inch reel tape
	 * two filemarks will be written for Pertec compatibility.
	 * Note, if opened with write and no data is written,
	 * we'll always write a filemark.
	 */

	/* Write eom and rewind */
	if (dsi->un_lastiow == 3) {
		/* Write eom and rewind */
		if (dsi->un_options & ST_REEL) {
			EPRINTF("stwrite_eof: 2 eoms\n");
			status = stcmd(dev, SC_WRITE_FILE_MARK, 2);
		} else {
			EPRINTF("stwrite_eof: 1 eoms\n");
			status = stcmd(dev, SC_WRITE_FILE_MARK, 1);
		}
		dsi->un_lastiow = 0;	/* Done writing */

	/* Write eof and continue */
	} else if (dsi->un_lastiow == 2) {
		if (dsi->un_options & ST_REEL) {
			EPRINTF("stwrite_eof: 2 eoms\n");
			status = stcmd(dev, SC_WRITE_FILE_MARK, 2);
			if (status == 0) {
				EPRINTF("stwrite_eof:  backspace filemark\n");
				status = stcmd(dev, SC_BSPACE_FILE, 1);
				dsi->un_offset = 0;
				dsi->un_next_block = 0;
				dsi->un_last_block = 0;
			}
			dsi->un_lastiow = 0;	/* Done writing */
		} else {
			EPRINTF("stwrite_eof: 1 eof\n");
			status = stcmd(dev, SC_WRITE_FILE_MARK, 1);
			dsi->un_lastiow = 1;	/* Have been writing */
		}

	/* Write rest of eom and rewind */
	} else if (dsi->un_lastiow == 1  &&  (dsi->un_options & ST_REEL)) {
		EPRINTF("stwrite_eof: 1 eom\n");
		status = stcmd(dev, SC_WRITE_FILE_MARK, 1);
		dsi->un_lastiow = 0;	/* Done writing */
	}
	/* Handle filemark write delay for Exabyte */
	if (IS_EXABYTE(dsi)) {
		untimeout(sttimer, (caddr_t)dev);
		state = dsi->un_openf;

		dsi->un_openf = OPENING_DELAY;
		timeout(sttimer, (caddr_t)dev, 5*hz);
		(void) sleep((caddr_t) dev, PRIBIO);
		untimeout(sttimer, (caddr_t)dev);

		dsi->un_openf = state;
		timeout(sttimer, (caddr_t)dev, 30*hz);
	}
	return (status);
}

/*ARGSUSED*/
stclose(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct scsi_unit *un;
	register struct scsi_tape *dsi;
	register struct st_drive_table *dp;
	u_long blocks;
	int status, resid;
	un = &stunits[MTUNIT(dev)];
	dsi = &stape[MTUNIT(dev)];
	dp = (struct st_drive_table *)dsi->un_dtab;

#ifdef 	lint
	flag = flag;
#endif	lint

	/*
	 * If we're closed already, something majorly went wrong.
	 */
	if (dsi->un_openf == CLOSED) {
		untimeout(sttimer, (caddr_t)dev);
		return;
	}
	/*
	 * Manufacturing patch to allow printing of all soft errors.
	 */
	if (st_retry != 0  &&  dsi->un_lastiow)
		dsi->un_retry_limit = dp->max_wretries[dsi->un_density-SC_DENSITY0];
	else if (st_retry != 0)
		dsi->un_retry_limit = dp->max_rretries[dsi->un_density-SC_DENSITY0];
	else if (st_retry == 0)
		dsi->un_retry_limit = 0;

	/*
	 * If we've been reading or writing and we're above the threshold,
	 * check the soft error rates to verify that the tape is ok.
	 * The threshold is used to delay checking until the sample size is
	 * valid.  (It also helps compensate for the high wear area at BOT
	 * too.)
	 *
	 * XXX Note, if you back up the tape, the soft error rate is not
	 * necessarily correct.  I suspect one should treat a backspace
	 * as a rewind and reset the soft error counters.
	 */
	if ((dsi->un_options & ST_VARIABLE) == 0 &&
	    dsi->un_dev_bsize != 512)
		/* fixed-length devices whose physical block size is not 512 */
		blocks = (dsi->un_blocks * dsi->un_dev_bsize) >> 9;
	else
		blocks = dsi->un_blocks;

	if ((dsi->un_lastior  ||  dsi->un_lastiow)  &&
	    blocks > (dp->retry_threshold >> 9)) {
		register int soft_errors;

		/*
		 * Get read/write retries count from the drive.	Have yet to
		 * see two vendors do this the same way.  Note for HP,
		 * conditionally reset the error long (because they won't
		 * do it on a rewind like everybody else).
		 */
		if (! IS_SYSGEN(dsi)) {
			status = dsi->un_status;
			resid = dsi->un_err_resid;
			if (dsi->un_options & ST_ERRLOG)
				(void) stcmd(dev, SC_READ_LOG, minor(dev) & MT_NOREWIND);
			else
				(void) stcmd(dev, SC_REQUEST_SENSE, 0);
			dsi->un_status = status;
			dsi->un_err_resid = resid;
		}
		/*
		 * Compensation for fixed block length devices used in
		 * variable block length mode (e.g. Exaybte).
		 *
		 * XXX:  ST_PHYSREC presumes ST_VARIABLE set too.
		 */
		/*
		 * paranoid check for zero divide
		 * (theoretically this can't happen
		 */
		if (dsi->un_blocks == 0)
			dsi->un_blocks++;

		if (dsi->un_options & ST_PHYSREC) {
			soft_errors = (dsi->un_phys_bsize + 511) / 512;

			soft_errors = (1000 * dsi->un_retry_ct * soft_errors) /
					dsi->un_blocks;
		} else if (dsi->un_options & ST_VARIABLE) {
			if (dsi->un_records == 0)
				dsi->un_records++;
			soft_errors = (1000 * dsi->un_retry_ct) /
			    dsi->un_records;
		} else {
			soft_errors = (1000 * dsi->un_retry_ct) / dsi->un_blocks;
		}

		/*
		 * If soft errors beyond limits, warn user of possible problems.
		 * Suppress wear-out message if manuf. mode enabled.
		 */
		if (soft_errors > dsi->un_retry_limit) {
			if (dsi->un_retry_limit != 0) {
				log(LOG_ERR, tape_worn1, un-stunits);
				log(LOG_ERR, tape_worn2);
				log(LOG_ERR, tape_worn3);
			}
			if (dsi->un_lastiow == 0) {
				log(LOG_ERR, retry_msg, un-stunits, "read",
				    dsi->un_retry_ct, soft_errors /10, soft_errors %10,
				    dsi->un_fileno, dsi->un_next_block);
			} else {
				log(LOG_ERR, retry_msg, un-stunits, "write",
				    dsi->un_retry_ct, soft_errors /10, soft_errors %10,
				    dsi->un_fileno, dsi->un_next_block);
			}
		}
	}

	/*
	 * Write filemarks as needed.
	 */
	if (dsi->un_lastiow) {
		if ((minor(dev) & MT_NOREWIND)  &&  dsi->un_lastiow == 3)
			dsi->un_lastiow--;
		(void) stwrite_eof(dev, dsi);
	}

	/*
	 * If rewind on close, set closing driver state and allow
	 * stintr to close the driver down.  Otherwise, now
	 * close the driver down.
	 */
	EPRINTF2("stclose: file= %u,  block= %u\n",
		 dsi->un_fileno, dsi->un_next_block);
	if ((minor(dev) & MT_NOREWIND) == 0) {
		DPRINTF("stclose:  rewinding...\n");
		untimeout(sttimer, (caddr_t)dev);
		dsi->un_options &= ~ST_NO_POSITION;
		dsi->un_lastiow = 0;
		dsi->un_openf = CLOSING;
		(void) stcmd(dev, SC_REWIND, -1);
	} else {
		DPRINTF("stclose:  no rewind...\n");
		untimeout(sttimer, (caddr_t)dev);
		dsi->un_options |= ST_AUTODEN_OVERRIDE;
		dsi->un_openf = CLOSED;
		if (dsi->un_eof == ST_EOF_PENDING  ||
		    dsi->un_eof == ST_EOF_LOG  ||
		    dsi->un_eof == ST_EOF) {
			dsi->un_eof = ST_NO_EOF;
			dsi->un_next_block = 0;
			dsi->un_last_block = INF;
			dsi->un_fileno++;
			dsi->un_lastior = 0;
			dsi->un_offset = 0;
		}
	}
}


/*
 * Run a command initiated internally by the driver.
 * Returns 0 for no error.  Otherwise, returns error code of error.
 */
stcmd(dev, cmd, count)
	dev_t dev;
	u_int cmd;
	int count;
{
	register struct buf *bp;
	register int s;
	register int error;
	register struct scsi_unit *un;
	register struct scsi_tape *dsi;
	register u_int unit;
#ifdef DISABLE
	register struct proc *p = u.u_procp;
	int  saved_signals;
#endif

	unit = MTUNIT(dev);
	un = &stunits[unit];
	dsi = &stape[unit];
	bp = &un->un_sbuf;
#ifdef 	lint
	dsi = dsi;
#endif	lint

	/*
	 * If this ever happens, we can't run any more
	 * commands until we're opened.
	 */
	if (dsi->un_openf == CLOSED) {
		EPRINTF("stcmd:  device closed\n");
		return (EIO);
	}

#ifdef DISABLE
	/* Block ALL non-essential signals for the duration. */
	(void) splhigh();
	saved_signals = p->p_sigmask;
	p->p_sigmask |= ~SIGNALS;
	(void) spl0();
#endif

	s = splr(pritospl(un->un_mc->mc_intpri));
	while (bp->b_flags & B_BUSY) {
		/*
		 * Special test because B_BUSY never gets cleared in
		 * the non-waiting rewind case.
		 */
		if ((bp->b_bcount == -1)  &&  (bp->b_flags & B_DONE))
			break;
		bp->b_flags |= B_WANTED;
		/*DPRINTF1("stcmd:  sleeping...,  bp= 0x%x\n", bp);*/
		/* Allow user to control-C out of this sleep */
		if (sleep((caddr_t)bp, PRIST)) {
#ifdef DISABLE
			(void) splhigh();
			p->p_sigmask = saved_signals;	/* Restore signal mask */
			(void) splx(s);
#endif

			EPRINTF("stcmd:  sleep interrupted\n");
			return (EINTR);
		}
	}

	/*DPRINTF1("stcmd:  waking...,  bp= 0x%x\n", bp);*/
	bp->b_flags = B_BUSY | B_READ | B_WANTED;
	(void) splx(s);

	bp->b_dev = dev;
	bp->b_bcount = count;
	un->un_scmd = cmd;
	ststrategy(bp);
	if (dsi->un_openf <= OPENING)
		timeout(sttimer, (caddr_t)dev, 30*hz);

	/* In case of rewind on close, don't wait.  */
	if (cmd == SC_REWIND  &&  count == -1) {
#ifdef DISABLE
		(void) splhigh();
		p->p_sigmask = saved_signals;	/* Restore signal mask */
#endif
		untimeout(sttimer, (caddr_t)dev);
		return (0);
	}

	s = splr(pritospl(un->un_mc->mc_intpri));
	while ((bp->b_flags & B_DONE) == 0) {
		/*
		 * Allow user to control-C out of this sleep.
		 * Note, this will force a rewind on the next open
		 * because we've now lost our position on the tape.
		 */
		/*DPRINTF1("stcmd:  still sleeping..., bp= 0x%x\n", bp);*/
		if (sleep((caddr_t)bp, st_sleep)  &&  dsi->un_openf == OPEN) {
			(void) splx(s);

			log(LOG_ERR, "st%d:  tape synchronization lost\n",
				 unit);
			dsi->un_reset_occurred = 1;
			dsi->un_next_block++;
			dsi->un_status = SC_INTERRUPTED;
			bp->b_flags |= B_ERROR;
			bp->b_error = EINTR;
			break;
		}
	}
#ifdef DISABLE
	(void) splhigh();
	p->p_sigmask = saved_signals;	/* Restore signal mask */
	(void) splx(s);
#endif

	if (dsi->un_openf <= OPENING)
		untimeout(sttimer, (caddr_t)dev);

	/*DPRINTF1("stcmd:  waking...,  bp= 0x%x\n", bp);*/
	error = geterror(bp);
	if (bp->b_flags & B_WANTED)
		wakeup((caddr_t) bp);
	bp->b_flags &= B_ERROR;			/* Clears B_BUSY */
	return (error);
}

/*
 * Run a command either from the user or from the driver.
 */
ststrategy(bp)
	register struct buf *bp;
{
	register struct scsi_unit *un;
	register struct mb_device *md;
	register u_int unit;
	register int s;
	register struct buf *ap;
	register struct scsi_tape *dsi;
	unit = MTUNIT(bp->b_dev);

#ifdef	STDEBUG
	if (unit >= nstape) {
		EPRINTF1("st%d:  ststrategy: invalid unit\n", unit);
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
#endif	STDEBUG

	un = &stunits[unit];
	md = un->un_md;
	dsi = &stape[unit];
	/*DPRINTF1("ststrategy:  bp= 0x%x\n", bp);*/
	/*DPRINTF2("             un= 0x%x, un->un_c= 0x%x\n", un, un->un_c);*/

	if (dsi->un_openf != OPEN  &&  bp != &un->un_sbuf) {
		EPRINTF("ststrategy:  device not open\n");
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	s = splr(pritospl(un->un_mc->mc_intpri));
	while (dsi->un_bufcnt >= MAXSTBUF) {
		/*DPRINTF1("ststrategy:  sleeping on bp= 0x%x\n", bp);*/
		/* Allow user to control-C out of this sleep */
		if (sleep((caddr_t)&dsi->un_bufcnt, PRIST)) {
			(void) splx(s);
			EPRINTF("ststrategy:  sleep interrupted\n");
			bp->b_flags |= B_ERROR;
			bp->b_error = EINTR;
			iodone(bp);
			return;
		}
	}
	dsi->un_bufcnt++;

	/*
	 * Put the block at the end of the queue.  Should probably have
	 * a pointer to the end of the queue, but the queue can't get too
	 * long, so the added code complexity probably isn't worth it.
	 */
	/*DPRINTF1("ststrategy:  queing bp= 0x%x\n", bp);*/
	ap = (struct buf *)(&md->md_utab);
	while (ap->b_actf != NULL)
		ap = ap->b_actf;
	ap->b_actf = bp;
	bp->b_actf = NULL;

	/*
	 * Call unit start routine to queue up device, if it currently isn't
	 * queued.  Then call start routine to run the next SCSI command.
	 */
	(*un->un_c->c_ss->scs_ustart)(un);
	(*un->un_c->c_ss->scs_start)(un);
	(void) splx(s);
}

/*
 * Start the operation.  Called by the host adapter to start the command.
 */
/*ARGSUSED*/
ststart(bp, un)
	register struct buf *bp;
	register struct scsi_unit *un;
{
	register struct scsi_tape *dsi;

	dsi = &stape[MTUNIT(bp->b_dev)];
	/*DPRINTF("ststart:\n");*/

	/*
	 * Process raw I/O tape commands.
	 */
	if (bp == &un->un_rbuf) {
		/*DPRINTF("ststart:  raw I/O\n");*/
		if (bp->b_flags & B_READ) {
			/*
			 * Can read past EOF.  Note, if at EOF, we'll
			 * return 0 bytes.  After that, we'll start reading
			 * on the next file.
			 */
			if (dsi->un_eof != ST_NO_EOF) {
				if (dsi->un_eof == ST_EOF) {
#ifdef					ST_EOF_READTHRU
					EPRINTF("ststart: clearing eof\n");
					dsi->un_eof = ST_NO_EOF;
					dsi->un_next_block = 0;
					dsi->un_last_block = INF;
					dsi->un_fileno++;
					dsi->un_lastior = 0;
					dsi->un_options |= ST_NO_POSITION;
					goto ST_START_RD;
#endif 					ST_EOF_READTHRU
				}
				EPRINTF("ststart:  eof\n");
				dsi->un_eof = ST_EOF;
				dsi->un_status = SC_EOF;
				dsi->un_err_blkno = dsi->un_next_block;
				dsi->un_err_fileno = dsi->un_fileno;
				dsi->un_err_resid = bp->b_resid = bp->b_bcount;
				if (dsi->un_bufcnt-- >= MAXSTBUF)
					wakeup((caddr_t) &dsi->un_bufcnt);
				return (0);
			}
ST_START_RD:
			un->un_cmd = SC_READ;
		} else {
			if (dsi->un_eof != ST_NO_EOF) {
				EPRINTF("ststart:  eot\n");
				dsi->un_status = SC_EOT;
				dsi->un_err_fileno = dsi->un_fileno;
				dsi->un_err_blkno = dsi->un_next_block;
				dsi->un_err_resid = bp->b_resid = bp->b_bcount;
				if (dsi->un_bufcnt-- >= MAXSTBUF)
					wakeup((caddr_t) &dsi->un_bufcnt);
				return (0);
			}
			un->un_cmd = SC_WRITE;
		}
		un->un_count = bp->b_bcount;
		un->un_flags = SC_UNF_DVMA;
		bp->b_resid = 0;
		return (1);

	/*
	 * Process internal I/O tape commands using our internal buffer.
	 */
	} else if (bp == &un->un_sbuf) {
		/*DPRINTF("ststart:  internal I/O\n");*/
		un->un_cmd = un->un_scmd;
		un->un_count = bp->b_bcount;
		un->un_flags = 0;
		bp->b_resid = 0;
		return (1);

	/*
	 * Process block I/O tape commands.
	 */
	} else {
		DPRINTF("ststart:  block mode I/O\n");
		if (bp->b_flags & B_READ) {
			/*
			 * Can't read past EOF.  Note, if at EOF,
			 * we'll return 0 bytes.
			 */
			if (dsi->un_eof != ST_NO_EOF) {
				EPRINTF("ststart:  eof\n");
				if (dsi->un_eof == ST_EOF_PENDING)
					dsi->un_eof = ST_EOF_LOG;
				dsi->un_status = SC_EOF;
				dsi->un_err_fileno = dsi->un_fileno;
				dsi->un_err_blkno = dsi->un_next_block;
				dsi->un_err_resid = bp->b_resid = bp->b_bcount;
				if (dsi->un_bufcnt-- >= MAXSTBUF)
					wakeup((caddr_t) &dsi->un_bufcnt);
				return (0);
			}
			un->un_cmd = SC_READ;

		/* Must be block mode write operation */
		} else {
			if (dsi->un_eof != ST_NO_EOF) {
				EPRINTF("ststart:  eot\n");
				dsi->un_status = SC_EOT;
				dsi->un_err_fileno = dsi->un_fileno;
				dsi->un_err_blkno = dsi->un_next_block;
				dsi->un_err_resid = bp->b_resid = bp->b_bcount;
				if (dsi->un_bufcnt-- >= MAXSTBUF)
					wakeup((caddr_t) &dsi->un_bufcnt);
				return (0);
			}
			un->un_cmd = SC_WRITE;
		}
		un->un_count = bp->b_bcount;
		un->un_flags = SC_UNF_DVMA;
		bp->b_resid = 0;
		return (1);
	}
}

/*
 * Make a command description block.  Called by the host adapter driver.
 */
stmkcdb(un)
	register struct scsi_unit *un;
{
	register struct scsi_cdb6 *cdb;
	register struct scsi_tape *dsi;
	register struct st_drive_table *dp;
	register struct st_sense *ssd;
	register struct st_ms_mspl *em;
	register struct st_ms_exabyte *ems;
	register struct st_log *elog;
	u_int count, density, speed;

	dsi = &stape[un->un_unit];
	dp = (struct st_drive_table *)dsi->un_dtab;
	cdb = (struct scsi_cdb6 *)&un->un_cdb;
	bzero((caddr_t)cdb, sizeof (struct scsi_cdb6));
	cdb->cmd = un->un_cmd;
	cdb->lun = un->un_lun;
#ifdef 	lint
	ssd = NULL; ssd = ssd;
#endif	lint

	switch (un->un_cmd) {
	case SC_READ:
		DPRINTF1("st%d:  stmkcdb: read\n", un-stunits);
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		un->un_dma_addr = un->un_baddr;
		dsi->un_timeout = NORMAL_TIMEOUT;
		if (dsi->un_options & ST_VARIABLE) {
			/* cdb->t_code = 0;	/* Variable-length */
			count = un->un_dma_count = un->un_count;
			cdb->mid_count = (un->un_count >> 8) & 0xff;
			cdb->low_count = count & 0xff;
		} else {
			cdb->t_code = 1;	/* Fixed-length mode */
			if (un->un_count != dsi->un_last_count) {
				count = un->un_count / dsi->un_dev_bsize;
			} else {
				count = dsi->un_last_bcount;
			}
			dsi->un_last_count = un->un_dma_count = un->un_count;
			dsi->un_last_bcount = count;
			cdb->mid_count = (count >> 8) & 0xff;
			cdb->low_count = count & 0xff;
		}
		break;

	case SC_WRITE:
		DPRINTF1("st%d:  stmkcdb: write\n", un-stunits);
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_baddr;
		dsi->un_timeout = NORMAL_TIMEOUT;
		if (dsi->un_options & ST_VARIABLE) {
			/* cdb->t_code = 0;	/* Variable-length */
			count = un->un_dma_count = un->un_count;
			cdb->mid_count = (un->un_count >> 8) & 0xff;
			cdb->low_count = count & 0xff;
		} else {
			cdb->t_code = 1;	/* Fixed-length mode */
			if (un->un_count != dsi->un_last_count) {
				count = un->un_count / dsi->un_dev_bsize;
			} else {
				count = dsi->un_last_bcount;
			}
			dsi->un_last_count = un->un_dma_count = un->un_count;
			dsi->un_last_bcount = count;
			cdb->mid_count = (count >> 8) & 0xff;
			cdb->low_count = count & 0xff;
		}
		break;


	case SC_WRITE_FILE_MARK:
	case SC_LOAD:
		EPRINTF1("st%d:  stmkcdb: write eof\n", un-stunits);
		cdb->low_count = un->un_count;	/* Limit is 255 */
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_timeout = NORMAL_TIMEOUT;
		break;

	case SC_BSPACE_FILE:
		un->un_count = - un->un_count;
		/* Fall through to sc_space_file... */
#ifdef		STDEBUG
		EPRINTF1("st%d:  stmkcdb: backspace file\n", un-stunits);
		cdb->t_code = 1;	/* Space files, not records */
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		cdb->cmd = SC_SPACE_REC;
		cdb->high_count = (un->un_count >> 16) & 0xff;
		cdb->mid_count  = (un->un_count >>  8) & 0xff;
		cdb->low_count  = un->un_count & 0xff;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_timeout = SPACE_TIMEOUT;
		break;
#endif		STDEBUG

	case SC_SPACE_FILE:
		EPRINTF1("st%d:  stmkcdb: space file\n", un-stunits);
		cdb->t_code = 1;	/* Space files, not records */
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		cdb->cmd = SC_SPACE_REC;
		cdb->high_count = (un->un_count >> 16) & 0xff;
		cdb->mid_count  = (un->un_count >>  8) & 0xff;
		cdb->low_count  = un->un_count & 0xff;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_timeout = SPACE_TIMEOUT;
		break;

	case SC_SPACE_REC:
		DPRINTF1("st%d:  stmkcdb: space rec\n", un-stunits);
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		cdb->cmd = SC_SPACE_REC;
		cdb->high_count = (un->un_count >> 16) & 0xff;
		cdb->mid_count  = (un->un_count >>  8) & 0xff;
		cdb->low_count  = un->un_count & 0xff;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_timeout = NORMAL_TIMEOUT;
#ifdef		ST_SYSGEN
		if (IS_SYSGEN(dsi)) {
			/*
			 * Even though Sysgen says it can do space recs,
			 * it really can't.  Use reads to do the job.
			 */
			cdb->cmd = SC_READ;
			un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
			un->un_dma_addr = (int)dsi->un_mspl - (int)DVMA;
			cdb->t_code = 1;
			un->un_dma_count = DEV_BSIZE;
			un->un_count = cdb->low_count = 1;
		}
#endif		ST_SYSGEN
		break;

	case SC_REQUEST_SENSE:
		EPRINTF1("st%d:  stmkcdb: request sense\n", un-stunits);
		dsi->un_timeout = SHORT_TIMEOUT;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		un->un_dma_addr = (int)dsi->un_mspl - (int)DVMA;
		un->un_dma_count = cdb->low_count =
				sizeof (struct st_sense);
		bzero((caddr_t)(dsi->un_mspl), sizeof (struct st_sense));
#ifdef		ST_SYSGEN
		if (IS_SYSGEN(dsi)  ||  dsi->un_openf == OPENING) {
			un->un_dma_count = cdb->low_count =
				sizeof (struct sysgen_sense);
		}
#endif		ST_SYSGEN
		return;
		/* break; */

	case SC_READ_APPEND:
		/* Special read command for write append. */
		EPRINTF1("st%d:  stmkcdb: read append\n", un-stunits);
		cdb->cmd = SC_READ;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		un->un_dma_addr = (int)un->un_sbuf.b_un.b_addr - (int)DVMA;
		un->un_dma_count = dsi->un_dev_bsize;
		dsi->un_timeout = NORMAL_TIMEOUT;
		if (dsi->un_options & ST_VARIABLE) {
			/* cdb->t_code = 0; */
			cdb->mid_count = (dsi->un_dev_bsize >> 8) & 0xff;
			cdb->low_count = dsi->un_dev_bsize & 0xff;
		} else {
			cdb->t_code = 1;
			cdb->mid_count = 0;
			cdb->low_count = 1;
		}
		return;
		/* break; */

	/* case SC_RETENSION: */
	case SC_REWIND:
		EPRINTF1("st%d:  stmkcdb: rewind\n", un-stunits);
		if (dsi->un_reten_rewind  &&  (dsi->un_options & ST_QIC)) {

			/* If QIC tape, allow retensioning */
			cdb->cmd = SC_LOAD;
			cdb->low_count = 3;
#ifdef			ST_SYSGEN
			if (IS_SYSGEN(dsi)) {
				cdb->cmd = SC_REWIND;
				cdb->low_count = 0;
				cdb->vu_57 = 1;
			}
#endif			ST_SYSGEN
		}
		dsi->un_reten_rewind = 0;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_timeout = REWIND_TIMEOUT;
		break;

	case SC_UNLOAD:
		EPRINTF1("st%d:  stmkcdb: unload\n", un-stunits);
		cdb->cmd = SC_LOAD;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_timeout = REWIND_TIMEOUT;
		break;

	case SC_INQUIRY:
		EPRINTF1("st%d:  stmkcdb: inquiry\n", un-stunits);
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		un->un_dma_addr = (int)dsi->un_mspl - (int)DVMA;
		un->un_dma_count = cdb->low_count =
				sizeof (struct scsi_inquiry_data);
		dsi->un_timeout = SHORT_TIMEOUT;
		bzero((caddr_t)(dsi->un_mspl), sizeof (struct scsi_inquiry_data));
		break;

	case SC_ERASE_TAPE:
		EPRINTF1("st%d:  stmkcdb: erase tape\n", un-stunits);
		cdb->t_code = un->un_count;	/* Erase type */
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_timeout = ERASE_TIMEOUT;
		if (dsi->un_options & ST_LONG_ERASE)
			dsi->un_timeout = LONG_ERASE_TIMEOUT;	/* 3 hours */
		break;

	case SC_READ_LOG:
		if (un->un_count) {
			EPRINTF1("st%d:  stmkcdb: read log\n", un-stunits);
			cdb->t_code = 1;	/* Don't reset log */
		} else {
			EPRINTF1("st%d:  stmkcdb: read log and reset\n",
				 un-stunits);
			cdb->t_code = 0;	/* Reset log */
		}
		cdb->high_count = 0x12;	/* Tape log */
		dsi->un_timeout = SHORT_TIMEOUT;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		elog = (struct st_log *)dsi->un_mspl;
		bzero((caddr_t)elog, sizeof (struct st_log));
		un->un_dma_addr = (int)dsi->un_mspl - (int)DVMA;
		un->un_dma_count = cdb->low_count =
				sizeof (struct st_log);
		break;

	case SC_MODE_SENSE:
		EPRINTF1("st%d:  stmkcdb: mode sense\n", un-stunits);
		dsi->un_timeout = SHORT_TIMEOUT;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		em = (struct st_ms_mspl *)dsi->un_mspl;
		bzero((caddr_t)em, sizeof (struct st_ms_mspl));
		un->un_dma_addr = (int)dsi->un_mspl - (int)DVMA;
		un->un_dma_count = cdb->low_count =
				sizeof (struct st_ms_mspl);
		break;

	case SC_DENSITY0:
		EPRINTF1("st%d:  density0 selected\n", un-stunits);
		density = dp->density[0];
		speed = dp->speed[0];
		goto MODE;			 /* goto SC_MODE_SELECT... */
		/* break; */

	case SC_DENSITY1:
		EPRINTF1("st%d:  density1 selected\n", un-stunits);
		density = dp->density[1];
		speed = dp->speed[1];
		goto MODE;			 /* goto SC_MODE_SELECT... */
		/* break; */

	case SC_DENSITY2:
		EPRINTF1("st%d:  density2 selected\n", un-stunits);
		density = dp->density[2];
		speed = dp->speed[2];
		goto MODE;			 /* goto SC_MODE_SELECT... */
		/* break; */

	case SC_DENSITY3:
		EPRINTF1("st%d:  density3 selected\n", un-stunits);
		density = dp->density[3];
		speed = dp->speed[3];
		goto MODE;			 /* goto SC_MODE_SELECT... */
		/* break; */

	case SC_MODE_SELECT:
MODE:
		dsi->un_timeout = SHORT_TIMEOUT;
		un->un_cmd = cdb->cmd = SC_MODE_SELECT;
		em = (struct st_ms_mspl *)dsi->un_mspl;
		bzero((caddr_t)em, sizeof (struct scsi_inquiry_data));
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = (int)dsi->un_mspl - (int)DVMA;
		un->un_dma_count = cdb->low_count =
				sizeof (struct st_ms_mspl);
		em->bufm = 1;
		em->bd_len = MS_BD_LEN;
		em->density = density;
		em->speed  =  speed;
		if (dsi->un_options & ST_VARIABLE) {
			EPRINTF("stmkcdb:  Variable length I/O selected\n");
		} else {
			EPRINTF("stmkcdb:  Fixed length I/O selected\n");
			em->mid_bl =  (dsi->un_dev_bsize >> 8) & 0xff;
			em->low_bl =  dsi->un_dev_bsize & 0xff;
#ifdef			ST_SYSGEN
			if (IS_SYSGEN(dsi)) {
				un->un_flags = dsi->un_flags |
					(un->un_flags & ~SC_UNF_RECV_DATA);
				cdb->cmd = SC_QIC02;
				cdb->high_count = density;
				cdb->low_count = 0;
				un->un_dma_count = 0;
			}
#endif			ST_SYSGEN
		}
		/* For MT-02, disable soft error reporting */
		if (IS_EMULEX(dsi)) {
			ems = (struct st_ms_exabyte *)dsi->un_mspl;
			ems->optional1 = 0x01;
			un->un_dma_count++;
			cdb->low_count++;

		/* For Exabyte, enable even byte disconnect */
		} else if (IS_EXABYTE(dsi)) {
			ems = (struct st_ms_exabyte *)dsi->un_mspl;
			/* ems->optional1 = 0x04; */
			ems->optional1 = 0x0C;
			un->un_dma_count++;
			cdb->low_count++;
		}
		break;


	case SC_TEST_UNIT_READY:
#ifdef		STDEBUG
		DPRINTF1("st%d:  stmkcdb: test unit\n", un-stunits);
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_timeout = SHORT_TIMEOUT;
		break;
#endif		STDEBUG

	case SC_RESERVE:
#ifdef		STDEBUG
		EPRINTF1("st%d:  stmkcdb: reserve\n", un-stunits);
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_timeout = SHORT_TIMEOUT;
		break;
#endif		STDEBUG

	case SC_RELEASE:
		EPRINTF1("st%d:  stmkcdb: release\n", un-stunits);
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_timeout = SHORT_TIMEOUT;
		break;

#ifdef		sun2
	case SC_READ_XSTATUS_CIPHER:
		DPRINTF1("st%d:  stmkcdb: read cipher status\n", un-stunits);
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
			dsi->un_flags;
		cdb->cmd = un->un_cmd = SC_QIC02;
		cdb->high_count = SC_READ_XSTATUS_CIPHER;
		un->un_dma_addr = un->un_dma_count = 0;
		break;
#endif		sun2

	default:
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		EPRINTF2("st%d:  stmkcdb: invalid command %x\n", un-stunits,
			 un->un_cmd);
		break;

	}
	/* Save last command for stintr error recovery */
	dsi->un_last_cmd = un->un_cmd;
}


typedef enum stintr_error_resolution {
	real_error, 		/* We found a genuine error */
	psuedo_error, 		/* What looked like an error is actually OK */
	more_processing 	/* We need to run another command */
} stintr_error_resolution;

stintr_error_resolution stintr_handle_error(),
	stintr_ran_append(), stintr_append_error(),
	stintr_file_mark_detected(), stintr_eot(),
	stintr_change(), stintr_ran_change();

stintr(c, resid, error)
	register struct scsi_ctlr *c;
	register u_int error;
	int resid;
{
	register struct scsi_unit *un;
	register struct buf *bp;
	register struct scsi_tape *dsi;
	register struct st_sense *ssd;
	register u_char status = 0;

	/*DPRINTF("stintr:\n");*/
	un = c->c_un;
	bp = un->un_md->md_utab.b_forw;
	if (bp == NULL  ||  bp->b_flags & B_DONE) {
		EPRINTF("stintr: bp = NULL\n");
		return;
	}
	dsi = &stape[MTUNIT(bp->b_dev)];

#ifdef	STDEBUG
	/*DPRINTF2("stintr:  c= 0x%x, un= 0x%x, ", c, un);*/
	/*DPRINTF1("bp= 0x%x\n", bp);*/
	if (st_debug > 2)
		st_print_cmd_status(error, un, dsi);
#endif	STDEBUG

	if (error  ||  (dsi->un_openf < OPEN)  ||  resid)  {
		/*
		 * Special processing for driver command timeout errors.
		 * Also, log location tape died at.
		 */
		if (error == SE_TIMEOUT) {
			dsi->un_openf = CLOSED;
			dsi->un_status = SC_TIMEOUT;
			dsi->un_err_resid = resid;
			dsi->un_err_fileno = dsi->un_fileno;
			dsi->un_err_blkno = dsi->un_next_block;
			dsi->un_reset_occurred = 1;
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			goto STINTR_WRAPUP;
		}
		/*
		 * Special processing for scsi bus failures.  Note,
		 * this error is implies a SCSI bus handshake failure.
		 * SCSI may now be dead too.
		 */
		if (error == SE_FATAL) {
			dsi->un_openf = CLOSED;
			dsi->un_status = SC_FATAL;
			dsi->un_reset_occurred = 1;
			dsi->un_err_resid = resid;
			dsi->un_err_fileno = dsi->un_fileno;
			dsi->un_err_blkno = dsi->un_next_block;
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			goto STINTR_WRAPUP;
		}

		/*
		 * Check if we're closing the tape device.
		 */
		if (dsi->un_openf <= CLOSING) {
#ifdef 			STDEBUG
			if (dsi->un_openf == CLOSED) {
				EPRINTF("stintr:  warning, already closed\n");
			}
#endif 			STDEBUG
			dsi->un_openf = CLOSED;
			untimeout(sttimer, (caddr_t)(bp->b_dev));
			goto STINTR_SUCCESS;
		}
		/*
		 * Special processing for opening the tape.
		 */
		ssd = (struct st_sense *)dsi->un_mspl;
		if (dsi->un_openf <= OPENING) {
			stintr_opening(un, dsi, bp, ssd);
			goto STINTR_WRAPUP;
		}
		DPRINTF2("stintr:  dma count= %u   blk count= %u  ",
			 un->un_dma_count, un->un_count);
		DPRINTF2("error= %d   resid= %d\n", error, resid);

		/*
		 * Processing for failed command -- do a request sense.
		 */
		if (un->un_scb.chk  &&
		    (dsi->un_openf == OPEN  ||
		     dsi->un_openf == RETRYING_CMD)) {
			stintr_sense(dsi, un, resid);
			return;
		}
		/*
		 * Finished running request sense, verify that it
		 * worked and restore state variable.  Actual error
		 * processing will be done by stintr_handle_error.
		 */
		if (dsi->un_openf == SENSING  ||
		    dsi->un_openf == SENSING_RETRY) {
			stintr_ran_sense(dsi, un, &resid);
		}
		/*
		 * Finished with density change.  Retry command.
		 */
		if (dsi->un_openf == DENSITY_CHANGING) {
			if (stintr_ran_change(dsi, un, &resid) == real_error) {
				EPRINTF("stintr:  density change failed\n");
				bp->b_flags |= B_ERROR;
				goto STINTR_WRAPUP;
			}
			return;
		}
		/*
		 * Special processing for retried commands.
		 */
		if (dsi->un_openf == RETRYING_CMD) {
			un->un_flags &= ~SC_UNF_GET_SENSE; 
			if (un->un_scb.chk  ||  un->un_scb.busy) {
				EPRINTF("stintr:  retried cmd failed\n");
				dsi->un_err_resid = resid;
				dsi->un_status = ssd->key;
				bp->b_flags |= B_ERROR;
				dsi->un_openf = OPEN;
				goto STINTR_WRAPUP;
			} else {
				/*DPRINTF("stintr:  retried cmd worked\n");*/
				dsi->un_err_resid = resid;
				dsi->un_status = 0;
				dsi->un_openf = OPEN;
				goto STINTR_SUCCESS;
			}
		}
		/*
		 * Process all other errors here
		 */
		switch (stintr_handle_error(c, un, bp, dsi, &resid, error)) {
		case real_error:
			/* This error is FATAL ! */
			/*DPRINTF("stintr:  real error handling\n");*/
			bp->b_flags |= B_ERROR; goto STINTR_WRAPUP;

		case psuedo_error:
			/* This error really isn't an error. */
			/*DPRINTF("stintr:  psuedo error handling\n");*/
			status = dsi->un_status; goto STINTR_SUCCESS;

		case more_processing:
			/*DPRINTF("stintr:  more processing\n");*/
			return;
		}
	} else {

STINTR_SUCCESS:
		/*
		 * Process all successful commands with driver in
		 * OPEN state here.
		 */
		switch (un->un_cmd) {
		case SC_READ:
			dsi->un_status = status;
			dsi->un_lastior = 2;
			if (dsi->un_options & ST_VARIABLE) {
				dsi->un_next_block++;
				dsi->un_records++;
				dsi->un_blocks += un->un_count >> 9; /* /512 */
			} else {
				dsi->un_records++;
				dsi->un_next_block += dsi->un_last_bcount;
				dsi->un_blocks += dsi->un_last_bcount;
			}
			break;

		case SC_WRITE:
			dsi->un_status = status;
			dsi->un_lastiow = 3;
			if (dsi->un_options & ST_VARIABLE) {
				dsi->un_next_block++;
				dsi->un_records++;
				dsi->un_blocks += un->un_count >> 9; /* /512 */
			} else {
				dsi->un_records++;
				dsi->un_next_block += dsi->un_last_bcount;
				dsi->un_blocks += dsi->un_last_bcount;
			}
			break;

		case SC_WRITE_FILE_MARK:
			dsi->un_eof = ST_NO_EOF;
			dsi->un_lastiow = 1;
			dsi->un_offset = 0;
			dsi->un_next_block = 0;
			dsi->un_last_block = 0;		/* no reads allowed */
			dsi->un_fileno += un->un_count;
			break;

		case SC_BSPACE_FILE:
			/*DPRINTF("stintr: bspace file\n");*/
			dsi->un_next_block = INF;
			dsi->un_offset = INF;
			goto ST_SPACE_FILE;

		case SC_SPACE_FILE:
			/*DPRINTF("stintr: space file\n");*/
			dsi->un_next_block = 0;
			dsi->un_offset = 0;
ST_SPACE_FILE:
			dsi->un_lastior = dsi->un_lastiow = 0;
			dsi->un_last_block = INF;
			dsi->un_eof = ST_NO_EOF;
			dsi->un_fileno += un->un_count;
			break;

		case SC_SPACE_REC:
			/*DPRINTF("stintr: space rec\n");*/
			dsi->un_lastior = 1;
			dsi->un_next_block += un->un_count;
			break;

		case SC_REWIND:
			/*DPRINTF("stintr: rewind\n");*/
			dsi->un_lastior = dsi->un_lastiow = 0;
			dsi->un_offset = 0;
			dsi->un_next_block = 0;
			dsi->un_last_block = INF;
			dsi->un_blocks = 0;
			dsi->un_records = 0;
			dsi->un_fileno = 0;
			dsi->un_eof = ST_NO_EOF;
			break;

		case SC_MODE_SELECT:
		case SC_ERASE_TAPE:
		case SC_LOAD:
		case SC_UNLOAD:
		case SC_QIC02:
			dsi->un_lastior = dsi->un_lastiow = 0;
			dsi->un_offset = 0;
			dsi->un_next_block = 0;
			dsi->un_last_block = INF;
			dsi->un_blocks = 0;
			dsi->un_records = 0;
			dsi->un_fileno = 0;
			dsi->un_eof = ST_NO_EOF;
			dsi->un_retry_ct = 0;
			dsi->un_underruns = 0;
			dsi->un_options &=
				~(ST_AUTODEN_OVERRIDE | ST_NO_POSITION);
			break;

		case SC_REQUEST_SENSE:
		case SC_READ_LOG:
			dsi->un_retry_ct = st_get_retries(dsi, error);
			break;

		case SC_RESERVE:
		case SC_RELEASE:
		case SC_INQUIRY:
		case SC_MODE_SENSE:
#ifdef		sun2
		case SC_READ_XSTATUS_CIPHER:
#endif		sun2
			break;

		default:
			/* If you get here, there's a bug in the driver */
			EPRINTF2("st%d:  stintr: invalid command %x\n",
			        un-stunits, un->un_cmd);
			break;
		}

STINTR_WRAPUP:
		/*
		 * Wrap up command processing for a command that has run
		 * to completion (successfully or otherwise).
		 */
		dsi->un_timeout = 0;		/* Disable time-outs */
		if (bp == &un->un_sbuf  &&
		    ((un->un_flags & SC_UNF_DVMA) == 0)) {
			(*c->c_ss->scs_done)(un->un_md);
		} else {
			mbudone(un->un_md);
			un->un_flags &= ~SC_UNF_DVMA;
		}
		if (dsi->un_bufcnt-- >= MAXSTBUF)
			wakeup((caddr_t) &dsi->un_bufcnt);
	}
}


/*
 * Handle a possible tape error.
 */
static stintr_error_resolution
stintr_handle_error(c, un, bp, dsi, resid, error)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct buf *bp;
	register struct scsi_tape *dsi;
	int *resid;
	u_int error;
{
	register struct st_sense *ssd = (struct st_sense *)dsi->un_mspl;
	register struct st_drive_table *dp;

	dp = (struct st_drive_table *)dsi->un_dtab;

	/* Log error info for "mt status" (ioctl) error reporting */
	dsi->un_err_resid = *resid;
	dsi->un_err_fileno = dsi->un_fileno;
	dsi->un_err_blkno = dsi->un_next_block;
	dsi->un_status = ssd->key;
	dsi->un_retry_ct = st_get_retries(dsi, error);

	if (un->un_scb.chk) {
		/*
		 * If you get a length error and resid != 0, you read a smaller
		 * record than you thought it was.  No  problem.  Otherwise,
		 * we've lost part of a record which is an EINVAL error.
		 */
		if (ST_LENGTH(dsi, ssd)) {
			EPRINTF("stintr_handle_error:  length error\n");
			dsi->un_options |=  ST_NO_POSITION;
			bp->b_resid = *resid;
			if (*resid == 0  ||  dsi->un_last_cmd != SC_READ) {
				dsi->un_status = SC_LENGTH;
				dsi->un_err_resid = bp->b_resid = bp->b_bcount;
				bp->b_error = EINVAL;
				return (real_error);
			}
			/* Something else failed too. */
			if (! ssd->key)
				return (psuedo_error);
		}
		if (ST_FILE_MARK(dsi, ssd)) {
			return (stintr_file_mark_detected(un, bp, dsi, *resid));
		}


		/*
		 * Special processing for tape write append.
		 */
		if (dsi->un_openf == APPEND_TESTING) {
			DPRINTF("stintr_handle_error:  checking write append test\n");
			return (stintr_ran_append(dsi, un, resid));

		/*
		 * Check for tape write append error.  Note, only available
		 * for QIC tapes and writing or writing filemarks.
		 */
		} else if (ssd->key == SC_ILLEGAL_REQUEST  &&
		    (dsi->un_options & ST_QIC)  &&
		    (dsi->un_last_cmd == SC_WRITE  ||
		     dsi->un_last_cmd == SC_WRITE_FILE_MARK)) {
			EPRINTF("stintr_handle_error:  write append error\n");
			return (stintr_append_error(dsi, un, *resid));

		/*
		 * Special processing for auto-density (format) during
		 * first read at BOT.  Triggered by read, write, space
		 * file, and space record which fails with blank check
		 * error.
		 */
		} else if (((dsi->un_last_cmd == SC_READ)  ||
		    (dsi->un_last_cmd == SC_SPACE_REC)  ||
		    (dsi->un_last_cmd == SC_SPACE_FILE))  &&
		    (ssd->key == SC_BLANK_CHECK)  &&
		    (dp->density[0] != dp->density[1])  &&
		    (dsi->un_fileno < ST_AUTODEN_LIMIT) &&
		    (dsi->un_next_block == 0)) {
			EPRINTF("stintr_handle_error:  changing density\n");
			return (stintr_change(dsi, un, *resid));

		/*
		 * Check for EOT.  Note, two cases are checked for.
		 */
		} else if (ssd->key == SC_BLANK_CHECK) {
			EPRINTF("stintr_handle_error:  blank check\n");
			return (stintr_eot(bp, dsi, *resid));
		} else if (ST_END_OF_TAPE(dsi, ssd)) {
			EPRINTF("stintr_handle_error:  eot\n");
			dsi->un_status = SC_EOT;
			return (stintr_eot(bp, dsi, *resid));

		} else if (ssd->key == SC_NOT_READY) {
			EPRINTF("stintr_handle_error:  no tape\n");
			dsi->un_reset_occurred = 2;
			return (real_error);

		} else if (ssd->key == SC_WRITE_PROTECT  &&
			   (dsi->un_last_cmd == SC_WRITE  ||
			    dsi->un_last_cmd == SC_WRITE_FILE_MARK)) {
			if (dsi->un_ctype == ST_TYPE_ARCHIVE  ||
			    dsi->un_ctype == ST_TYPE_WANGTEK) {
				log(LOG_ERR, wrong_media, un-stunits);
				dsi->un_status = SC_MEDIA;
				dsi->un_read_only = 3;
			} else {
				EPRINTF("stintr_handle_error:  write protected\n");
				dsi->un_read_only = 2;
			}
			bp->b_error = EACCES;
			return (real_error);

		/*
		 * A block had to be read/written more than once but was
		 * successfully read/written.  Just bump stats and consider
		 * the operation a success.
		 */
		} else if (ssd->key == SC_RECOVERABLE_ERROR) {
			EPRINTF("stintr_handle_error:  soft error\n");
			return (psuedo_error);

		} else if (ssd->key == SC_UNIT_ATTENTION) {
			EPRINTF("stintr_handle_error:  reset\n");
			dsi->un_reset_occurred = 2;
			return (real_error);

		} else if (ssd->key == SC_ILLEGAL_REQUEST) {
#ifdef			ST_SYSGEN
			/*
			 * Sysgen will fail to rewind if it's in the middle of
			 * a file.  Also, space file has the same problem.
			 */
			if (IS_SYSGEN(dsi)  &&
			    (dsi->un_last_cmd == SC_REWIND  ||
			     dsi->un_last_cmd == SC_SPACE_FILE)) {
				return (real_error);
			}
#endif			ST_SYSGEN
			/*
			 * Mode selects, reserve, and release can fail on
			 * foreign tape drives.  Report error, but don't
			 * print error message in this case.
			 */
			if (dsi->un_last_cmd == SC_MODE_SELECT  ||
			    dsi->un_last_cmd == SC_RESERVE      ||
			    dsi->un_last_cmd == SC_RELEASE)
				return (real_error);

			st_error_message(c, dsi, LOG_CRIT);
			return (real_error);

		/*
		 * Have an sense key error we aren't decoding.
		 */
		} else {
			st_error_message(c, dsi, LOG_CRIT);
			return (real_error);
		}


	/*
	 * Check for reservation conflict error.
	 */
	} else if (un->un_scb.is  &&  un->un_scb.busy) {
		EPRINTF("stintr_handle_error:  reservation conflict\n");
		dsi->un_status = SC_RESERVATION;
		bp->b_error = EPERM;
		return (real_error);

	/*
	 * Check for busy error.  Note, this must follow reservation
	 * conflict test.
	 */
	} else if (un->un_scb.busy) {
		EPRINTF("stintr_handle_error:  busy\n");
		dsi->un_status = SC_BUSY;
		return (real_error);


	/*
	 * Check for host adapter error.
	 */
	} else  if (error != 0) {
		EPRINTF1("stintr:  host adapter error, error= %d\n", error);
		dsi->un_status = SC_FATAL;
		dsi->un_err_resid = bp->b_resid = *resid;
		st_error_message(c, dsi, LOG_ERR);
		return (real_error);

	/*
	 * Have left over residue data from last command.  Note, for
	 * Request sense and mode select, this is allowable as there
	 * are variations here from vendor to vendor.
	 */
	} else  if (*resid != 0) {
		if (un->un_cmd == SC_REQUEST_SENSE  ||
		    dsi->un_last_cmd == SC_MODE_SELECT)
			return (psuedo_error);

		EPRINTF1("stintr:  residue error, residue= %d\n", *resid);
		dsi->un_status = SC_RESIDUE;
		dsi->un_err_resid = bp->b_resid = *resid;
#ifdef		STDEBUG
		st_error_message(c, dsi, LOG_ERR);
#endif		STDEBUG
		return (real_error);

	/*
	 * Have an unknown error.  Don't know what went wrong.
	 */
	} else {
		EPRINTF("stintr_handle_error:  unknown error\n");
		st_error_message(c, dsi, LOG_CRIT);
		return (real_error);
	}
}


/*
 * Special processing for opening and closing the tape drive.
 * Note, no request sense is performed until the device is open.
 */
static
stintr_opening(un, dsi, bp, ssd)
	register struct scsi_unit *un;
	register struct scsi_tape *dsi;
	register struct buf *bp;
	register struct st_sense *ssd;
{
	register struct sysgen_sense *sgs;
	register struct scsi_cdb6 *cdb;
	register struct st_ms_mspl *smsd;
	cdb = (struct scsi_cdb6 *)&un->un_cdb;
	smsd = (struct st_ms_mspl *)ssd;
	sgs = (struct sysgen_sense *)ssd;

	/* Reservation conflict error (no hope) */
	if (un->un_scb.is  &&  un->un_scb.busy) {
		EPRINTF("stintr_opening:  reservation error\n");
		dsi->un_status = SC_RESERVATION;
		bp->b_error = EPERM;
		dsi->un_openf = OPEN_FAILED;
		bp->b_flags |= B_ERROR;
		return;

#ifdef	ST_SYSGEN
	/*
	 * Since the sysgen will always return an illegal command
	 * error if the tape is not rewound, we have to handle this
	 * special case here during opening.
	 */
	} else if ((IS_SYSGEN(dsi)  ||  IS_DEFAULT(dsi))  &&
	    (cdb->cmd == SC_REQUEST_SENSE)  &&  sgs->illegal) {
		EPRINTF("stintr_opening:  sysgen found\n");
		dsi->un_openf = OPENING_SYSGEN;
		bp->b_flags |= B_ERROR;
		return;

	} else if (IS_SYSGEN(dsi)  &&  (cdb->cmd == SC_REQUEST_SENSE)) {
		dsi->un_openf = OPENING_SYSGEN;
		dsi->un_read_only = 0;
		if (sgs->write_prot) {
			EPRINTF("stintr_opening:  sysgen read only\n");
			dsi->un_read_only = 2;
		}
		if (sgs->no_cart) {
			EPRINTF("stintr_opening:  no tape\n");
			dsi->un_openf = OPEN_FAILED_TAPE;
			bp->b_flags |= B_ERROR;
			return;
		}
#endif	ST_SYSGEN

	/* Check condition error (maybe recoverable) */
	} else if (un->un_scb.chk) {
		EPRINTF("stintr_opening:  check condition\n");
		dsi->un_openf = OPEN_FAILED_TAPE;
		bp->b_flags |= B_ERROR;
		return;

	/* Busy error (recoverable) */
	} else if (un->un_scb.busy) {
		EPRINTF("stintr_opening:  busy\n");
		dsi->un_openf = OPEN_FAILED_LOADING;
		bp->b_flags |= B_ERROR;
		return;
	}

	/*
	 * If we were closing the tape then opened it before it was done,
	 * we've screwed up handling the rewind command.  This is fixed here.
	 */
	if (cdb->cmd == SC_REWIND) {
		DPRINTF("stintr_opening:  rewind done\n");
		dsi->un_lastior = dsi->un_lastiow = 0;
		dsi->un_offset = 0;
		dsi->un_next_block = 0;
		dsi->un_last_block = INF;
		dsi->un_fileno = 0;
		dsi->un_eof = ST_NO_EOF;
	}
	/*
	 * If running mode_sense, check if tape is write protected. Note, using
	 * mode sense during normal operation will cause a fault condition.
	 */
	if (cdb->cmd == SC_MODE_SENSE) {
		DPRINTF1("stintr_opening:  density= 0x%x\n", smsd->density);
		dsi->un_read_only = 0;
		if (smsd->wp) {
			DPRINTF("stintr_opening:  CCS read only\n");
			dsi->un_read_only = 2;
		}
		if (IS_DEFAULT(dsi)) {
			dsi->un_dev_bsize = (smsd->high_bl <<16) +
			    (smsd->mid_bl <<8) + (smsd->low_bl);
			DPRINTF1("stintr_opening:  block size= %d\n",
			dsi->un_dev_bsize);
		}
	}
#ifdef	ST_SYSGEN
	/*
	 * Since a request sense is issued after an inquiry cmd, this
	 * will cause a fault condition.  Thus the first byte of status
	 * should be 0x20 if it's a sysgen controller; otherwise it's
	 * some other drive.  Also, verify that tape is loaded in drive
	 * and check if it's write protected.
	 */
	if ((cdb->cmd == SC_REQUEST_SENSE)  &&  (sgs->disk_sense[0] == 0x20)) {
		dsi->un_openf = OPENING_SYSGEN;
		dsi->un_read_only = 0;
		if (sgs->write_prot) {
			DPRINTF("stintr_opening:  sysgen read only\n");
			dsi->un_read_only = 2;
		}
		if (sgs->no_cart) {
			dsi->un_openf = OPEN_FAILED_TAPE;
		}
	}
#endif	ST_SYSGEN
}


/*
 * Command failed, need to run a request sense command to determine why.
 */
static
stintr_sense(dsi, un, resid)
	register struct scsi_tape *dsi;
	register struct scsi_unit *un;
	register int resid;
{
	/*
	 * Note that ststart will call stmkcdb, which will notice
	 * that the flag is set and not do the copy of the cdb,
	 * doing a request sense rather than the normal command.
	 */
	stsave_cmd(un, resid);
	if (dsi->un_openf == RETRYING_CMD)
		dsi->un_openf = SENSING_RETRY;
	else
		dsi->un_openf = SENSING;

	un->un_cmd = SC_REQUEST_SENSE;
	(*un->un_c->c_ss->scs_go)(un->un_md); 
}

/*
 * Cleanup after running request sense command to see why the real
 * command failed.
 */
static
stintr_ran_sense(dsi, un, resid_ptr)
	register struct scsi_tape *dsi;
	register struct scsi_unit *un;
	register int *resid_ptr;
{
	/* Restore to previous state */
	if (dsi->un_openf == SENSING_RETRY)
		dsi->un_openf = RETRYING_CMD;
	else
		dsi->un_openf = OPEN;

	/*
	 * Check if request sense command failed.  This should never happen!
	 */
	if (un->un_scb.chk  ||  un->un_scb.busy) {
		dsi->un_status = SC_FATAL;
		EPRINTF("stintr_ran_sense:  request sense failed\n");
	}

	/* Restore failed command which caused request sense to be run. */
	strestore_cmd(dsi, un, resid_ptr);

#ifdef	ST_SYSGEN
	/* Special processing for sysgen tape controller. */
	if (IS_SYSGEN(dsi))
		stintr_sysgen(dsi, un);
#endif	ST_SYSGEN
}

#ifdef	ST_SYSGEN
/*
 * Translate Sysgen QIC-02 status in SCSI status that the
 * error handler can understand.  In other words, map
 * QIC-02 sense status into SCSI sense key info.
 */
static
stintr_sysgen(dsi, un)
	register struct scsi_tape *dsi;
	register struct scsi_unit *un;
{
	register struct sysgen_sense *sgs;
	register struct st_sense *ssd;

	sgs = (struct sysgen_sense *) dsi->un_mspl;
	ssd = (struct st_sense *) dsi->un_mspl;
	sgs->disk_sense[2] = 0;		/* sense key = no sense */

	if (sgs->bot  &&  (dsi->un_fileno != 0)  &&
	    (dsi->un_next_block != 0)) {
		log(LOG_ERR, "st%d:  warning, tape rewound\n", un-stunits);
		dsi->un_reset_occurred = 2;
		ssd->key = SC_NOT_READY;
	}
	/* Handle the following errors to guarantee detection. */
	if (sgs->file_mark)
		ssd->fil_mk = 1;
	if (sgs->eot)
		ssd->eom = 1;
	if (sgs->write_prot) {
		dsi->un_read_only = 2;
		ssd->key = SC_WRITE_PROTECT;
	}

	if (sgs->disk_sense[0] == 0x20  ||  sgs->illegal) {
		EPRINTF("stintr_sysgen:  illegal request\n");
		ssd->key = SC_ILLEGAL_REQUEST;

	} else  if (sgs->not_ready  ||  sgs->reset  ||  sgs->no_cart) {
		EPRINTF("stintr_sysgen:  not ready\n");
		ssd->key = SC_NOT_READY;

	} else  if (sgs->no_data) {
		EPRINTF("stintr_sysgen:  no data\n");
		ssd->key = SC_BLANK_CHECK;

	} else  if (sgs->write_prot) {
		EPRINTF("stintr_sysgen:  write protected\n");
		dsi->un_read_only = 2;
		ssd->key = SC_WRITE_PROTECT;

	} else  if (sgs->data_err) {
		EPRINTF("stintr_sysgen:  hardware error\n");
		ssd->key = SC_HARDWARE_ERROR;

	} else  if (sgs->retries) {
		EPRINTF("stintr_sysgen:  soft error\n");
		ssd->key = SC_RECOVERABLE_ERROR;
	}
}
#endif	ST_SYSGEN


/*
 * Change tape density/format setting to other one if read fails at BOT.
 */
static stintr_error_resolution
stintr_change(dsi, un, resid)
	register struct scsi_tape *dsi;
	register struct scsi_unit *un;
	int resid;
{
	EPRINTF("stintr_change:\n");
	/*
	 * Note that ststart will call stmkcdb, which will notice
	 * that the flag is set and not do the copy of the cdb,
	 * doing a request sense rather than the normal command.
	 */
	stsave_cmd(un, resid);
	dsi->un_openf = DENSITY_CHANGING;
	un->un_flags &= ~SC_UNF_DVMA;
	un->un_cmd = SC_REWIND;
	dsi->un_timeout = 12;
	(*un->un_c->c_ss->scs_go)(un->un_md); 
	return (more_processing);
}


/*
 * Check results after changing tape density/format.  Try running read
 * command again.
 * FIXME: really should extend this to 4 densities from 2.
 */
static stintr_error_resolution
stintr_ran_change(dsi, un, resid_ptr)
	register struct scsi_tape *dsi;
	register struct scsi_unit *un;
	register int *resid_ptr;
{
	/*
	 * Check if any command failed.  If it did, it's all over.
	 */
	/*DPRINTF("stintr_ran_change:\n");*/
	if (un->un_scb.chk  ||  un->un_scb.busy  ||  un->un_scb.is) {
		EPRINTF("stintr_ran_change:  command failed\n");
		/* FIXME: should we or should we not give up on error ??? */
		dsi->un_openf = OPEN;
		if (un->un_cmd == SC_READ)
			un->un_flags |= SC_UNF_DVMA;
		strestore_cmd(dsi, un, resid_ptr);
		return (real_error);
	}
	/*
	 * If tape is rewound, first change density, and then retry
	 * failed command.  Note, only done for first 2 densities.
	 */
	switch (un->un_cmd) {
	case SC_REWIND:
		dsi->un_lastior = 0;
		dsi->un_lastiow = 0;
		dsi->un_offset = 0;
		dsi->un_next_block = 0;
		dsi->un_last_block = INF;
		dsi->un_fileno = 0;
		dsi->un_eof = ST_NO_EOF;
		dsi->un_err_resid = 0;
		dsi->un_status = 0;
		dsi->un_options &= ~ST_AUTODEN_OVERRIDE;
		dsi->un_timeout = 4;
		if (dsi->un_density == SC_DENSITY0) {
			EPRINTF("stintr_ran_change: trying density2\n");
			un->un_cmd = SC_DENSITY1;
		} else {
			EPRINTF("stintr_ran_change: trying density1\n");
			un->un_cmd = SC_DENSITY0;
		}
		break;

	/* case SC_DENSITY0: */
	/* case SC_DENSITY1: */
	case SC_MODE_SELECT:
		if (dsi->un_density == SC_DENSITY0)
			dsi->un_density = SC_DENSITY1;
		else
			dsi->un_density = SC_DENSITY0;

		strestore_cmd(dsi, un, resid_ptr);
		dsi->un_options |= ST_AUTODEN_OVERRIDE;
		dsi->un_openf = RETRYING_CMD;
		if (un->un_cmd == SC_READ) {
			EPRINTF("stintr_ran_change: read\n");
			dsi->un_timeout = 8;
			un->un_flags |= SC_UNF_DVMA;
		} else {
			EPRINTF("stintr_ran_change: space file\n");
			dsi->un_timeout = 60;
		}
		break;

	}
	(*un->un_c->c_ss->scs_go)(un->un_md); 
	return (more_processing);

}


/*
 * A filemark was detected.  Normally, no error is returned; but a
 * non-zero residue transfer count is returned to signal the error.
 */
static stintr_error_resolution
stintr_file_mark_detected(un, bp, dsi, resid)
	register struct scsi_unit *un;
	register struct buf *bp;
	register struct scsi_tape *dsi;
	register int resid;
{
	switch (dsi->un_last_cmd) {
	case SC_READ:
		EPRINTF("stintr_file_mark: read\n");
		if (dsi->un_options & ST_VARIABLE) {
			--dsi->un_next_block;
		} else if (resid > 0) {
			dsi->un_next_block -=  (resid / dsi->un_dev_bsize);
		}
		dsi->un_last_block = dsi->un_next_block;
		break;

	case SC_SPACE_REC:
		EPRINTF("stintr_file_mark: space rec\n");
		dsi->un_last_block = dsi->un_next_block;
		dsi->un_err_resid = resid = bp->b_bcount;
		dsi->un_status = SC_EOF;
		bp->b_resid = resid;
		dsi->un_eof = ST_EOF_LOG;
		return (real_error);
		/* break; */

	case SC_SPACE_FILE:
		EPRINTF("stintr_file_mark: space file\n");
		break;

	default:
		EPRINTF2("st%d:  EOF on cmd 0x%x\n",
			un-stunits, un->un_cmd);
		break;
	}

	/*
	 * If EOF, don't fail this time.  Return what data we could
	 * get and set flag to inhibit further reads.
	 */
	bp->b_resid = resid;
	if (resid == un->un_count) {
		EPRINTF("stintr_file_mark: eof\n");
		dsi->un_eof = ST_EOF;
		dsi->un_status = SC_EOF;

	} else if (resid > 0) {
		EPRINTF("stintr_file_mark: log eof\n");
		dsi->un_eof = ST_EOF_LOG;
	} else {
		EPRINTF("stintr_file_mark: pending eof\n");
		dsi->un_eof = ST_EOF_PENDING;
	}
	return (psuedo_error);
}

/*
 * End of tape.  Note, due to buffered mode I/O, there's
 * no telling where you really died from eot.  We trust
 * that the tape drive takes care of these little details.
 * Normally no error condition is returned; however, a
 * non-zero residue is returned to indicate eot for writes.
 * No further writing will be allowed until the tape is changed.
 */
static stintr_error_resolution
stintr_eot(bp, dsi, resid)
	register struct buf *bp;
	register struct scsi_tape *dsi;
	register int resid;
{
	/*
	 * Set residual dma count.  Note, for Sysgen's
	 * this is terminal.
	 */
	if (IS_SYSGEN(dsi))
	    	bp->b_resid = bp->b_bcount;
	else
	    	bp->b_resid = resid;

	/* EOT stops file positioning operations */
	if ((dsi->un_last_cmd == SC_SPACE_REC)  ||
	    (dsi->un_last_cmd == SC_SPACE_FILE)) {
		EPRINTF("stintr_eot:  space\n");
		bp->b_resid = bp->b_bcount;
		return (real_error);
	}

	/* Ignore EOT for reads */
	if (dsi->un_last_cmd == SC_READ) {
		EPRINTF("stintr_eot:  read\n");
		if (dsi->un_options & ST_VARIABLE) {
			--dsi->un_next_block;
		} else if (resid > 0) {
			dsi->un_next_block -=  (resid / dsi->un_dev_bsize);
		}
		dsi->un_last_block = dsi->un_next_block;
		return (psuedo_error);
	}
	/*
	 * For writes which hit EOT, stop all further writes until a file
	 * mark written.  Note, partial transfers are counted as complete
	 * transfer failure.
	 */
	if (dsi->un_last_cmd == SC_WRITE) {
		EPRINTF("stintr_eot:  write\n");

		/* If no resid, override default resid */
		dsi->un_lastiow = 3;	/* Force filemark write */
		dsi->un_eof = ST_EOT;	/* no more writes */
	}
	return (psuedo_error);
}

/*
 * Handle an append error.  QIC tapes don't allow you to overwrite data
 * due to the way data is recorded on the tape.  Also, they return a
 * false indication of this error if the heads are positioned after
 * the last file.  The controller depends on blank tape not filemarks
 * to know when to let you write.  We'd like to start writing after the
 * last filemark.  
 *
 * We handle this problem by doing a read and checking
 * to see what happens.  If it fails, retry the write as it should work
 * now.  If it works, we're trying to illegally overwrite data.
 * Appending files on QIC tapes is such fun!
 */
static stintr_error_resolution
stintr_append_error(dsi, un, resid)
	register struct scsi_tape *dsi;
	register struct scsi_unit *un;
	int resid;
{
	register caddr_t *sbuf;
	/*
	 * Allocate a scratch buffer for the read if buffer size is not equal
	 * to DEV_BSIZE.  Otherwise, use internal buffer.  We appropriate the
	 * sbuf for this.
	 */
	if (dsi->un_dev_bsize == dsi->un_msplsize) {
		sbuf = (caddr_t *)dsi->un_mspl;
	} else {
		sbuf =(caddr_t *) rmalloc(iopbmap,
				 (long)(dsi->un_dev_bsize + LONG));
		if ((int)sbuf == NULL) {
			log(LOG_CRIT, iopb_error, MTUNIT(un->un_unit));
			return (real_error);
		}
	}

	/* Save the original address for rmfree and word align buffer. */
	un->un_scratch_addr = (caddr_t)sbuf;
	sbuf = (caddr_t *) LONG_ALIGN(sbuf);
	un->un_sbuf.b_un.b_addr = (caddr_t)sbuf;

	/*
	 * s?start will call s?mkcdb, which will notice
	 * that we are using a special read command and
	 * not do the copy of the cdb.
	 */
	stsave_cmd(un, resid);
	dsi->un_openf = APPEND_TESTING;
	un->un_cmd = SC_READ_APPEND;
	(*un->un_c->c_ss->scs_go)(un->un_md);
	return (more_processing);
}

/*
 * Cleanup after running a read command on behalf of append error evaluation.
 */
static stintr_error_resolution
stintr_ran_append(dsi, un, resid_ptr)
	register struct scsi_tape *dsi;
	register struct scsi_unit *un;
	int *resid_ptr;
{
	if (dsi->un_dev_bsize != dsi->un_msplsize) {
		rmfree(iopbmap, (long)(dsi->un_dev_bsize + LONG),
			 (u_long)(un->un_scratch_addr));
	}
	/*
	 * Read failed, thus we are at EOM, so retry write command.
	 */
	if (un->un_scb.chk) {
		DPRINTF("stintr_ran_append:  retrying write append\n");
		strestore_cmd(dsi, un, resid_ptr);
		dsi->un_openf = RETRYING_CMD;
		(*un->un_c->c_ss->scs_go)(un->un_md);
		return (more_processing);

	/*
	 * Read worked, which means we are not at EOM.  Thus,
	 * the controller legitimately reported the append error.
	 */
	} else {
		DPRINTF("stintr_ran_append:  cannot write append\n");
		strestore_cmd(dsi, un, resid_ptr);
		dsi->un_openf = OPEN;
		return (real_error);
	}
}

static
st_print_cmd(un)
	register struct scsi_unit *un;
{
	register int x;
	register u_char *y = (u_char *)&(un->un_cdb);
#ifdef 	lint
	un = un;
#endif	lint

	printf("st%d:  failed cmd =", un-stunits);
	for (x = 0; x < sizeof (struct scsi_cdb6); x++)
		printf("  %x", *y++);
	printf("\n");
}

#ifdef	STDEBUG
st_print_sid_buffer(sid, count)
	register struct scsi_inquiry_data *sid;
	register int count;
{
	register int x;
	register u_char *y = (u_char *)sid;

	EPRINTF("sid buffer:");
	for (x = 0; x < count; x++)
		printf(" %x", *y++);
	printf("\n\n");
}


st_print_cmd_status(error, un, dsi)
	register u_int error;
	register struct scsi_unit *un;
	register struct scsi_tape *dsi;
{
	register int x;
	register u_char *y = (u_char *)&(un->un_cdb);
	register struct scsi_ctlr *c = un->un_c;

	/* Driver open, no error */
	if (error == 0  &&  (dsi->un_openf == OPEN)) {
		EPRINTF("stcmd worked:");

	/* Driver open, error */
	} else if (error  &&  (dsi->un_openf == OPEN)) {
		EPRINTF("stcmd failed:");

	/* Driver not open, error */
	} else if (error  &&  (dsi->un_openf != OPEN)) {
		EPRINTF("stcmd failed (no sense):");

	/* Dump Sense data for failed cmd */
	} else if (error != 0  &&  (dsi->un_openf == SENSING)) {
		st_error_message(c, dsi, NULL);
		return;
	}
	for (x = 0; x < sizeof (struct scsi_cdb6); x++)
		printf(" %x", *y++);
	printf("\n");
}
#endif	STDEBUG


/*
 * Restore old command state after running request sense command.
 */
stsave_cmd(un, resid)
	register struct scsi_unit *un;
	register int resid;
{
	/* Save away old command state. */
	un->un_saved_cmd.saved_cmd = un->un_cmd;
	un->un_saved_cmd.saved_scb = un->un_scb;
	un->un_saved_cmd.saved_cdb = un->un_cdb;
	un->un_saved_cmd.saved_resid = resid;
	un->un_saved_cmd.saved_dma_addr = un->un_dma_addr;
	un->un_saved_cmd.saved_dma_count = un->un_dma_count;
	un->un_flags |= SC_UNF_GET_SENSE;
}


/*
 * Restore old command state after running request sense command.
 */
strestore_cmd(dsi, un, resid_ptr)
	register struct scsi_tape *dsi;
	register struct scsi_unit *un;
	register int *resid_ptr;
{
	un->un_cmd = un->un_saved_cmd.saved_cmd;
	un->un_scb = un->un_saved_cmd.saved_scb;
	un->un_cdb = un->un_saved_cmd.saved_cdb;
	*resid_ptr = un->un_saved_cmd.saved_resid;
	un->un_dma_addr = un->un_saved_cmd.saved_dma_addr;
	un->un_dma_count = un->un_saved_cmd.saved_dma_count;
	un->un_flags &= ~SC_UNF_GET_SENSE;
	dsi->un_last_cmd = un->un_cmd;
}


static char *st_key_error_str[] = SENSE_KEY_INFO;
#define MAX_KEY_ERROR_STR \
	(sizeof(st_key_error_str)/sizeof(st_key_error_str[0]))


static char *st_hp_error_str[] = {
	"",				/* 0x00 */
	"",				/* 0x01 */
	"",				/* 0x02 */
	"write fault",			/* 0x03 */
	"drive not ready",		/* 0x04 */
	"drive not selected",		/* 0x05 */
	"",				/* 0x06 */
	"",				/* 0x07 */
	"logical unit fault",		/* 0x08 */
	"",				/* 0x09 */
	"error log overflow",		/* 0x0a */
	"time-out error",		/* 0x0b */
	"",				/* 0x0c */
	"soft write error",		/* 0x0d */
	"soft interface error",		/* 0x0e */
	"",				/* 0x0f */
	"",				/* 0x10 */
	"hard read error",		/* 0x11 */
	"",				/* 0x12 */
	"space command error",		/* 0x13 */
	"no record found",		/* 0x14 */
	"locate error",			/* 0x15 */
	"",				/* 0x16 */
	"soft read error",		/* 0x17 */
	"soft write error",		/* 0x18 */
	"",				/* 0x19 */
	"parameter overrun",		/* 0x1a */
	"synch. xfer error",		/* 0x1b */
	"",				/* 0x1c */
	"verify error",			/* 0x1d */
	"",				/* 0x1e */
	"hard write error",		/* 0x1f */
	"invalid command",		/* 0x20 */
	"invalid block address",	/* 0x21 */
	"",				/* 0x22 */
	"space cmd error",		/* 0x23 */
	"",				/* 0x24 */
	"invalid cdb lun",		/* 0x25 */
	"invalid cdb field",		/* 0x26 */
	"write protected",		/* 0x27 */
	"media changed",		/* 0x28 */
	"drive reset",			/* 0x29 */
	"mode select changed",		/* 0x2a */
	"block length error",		/* 0x2b */
	"cmd sequence error",		/* 0x2c */
	"overwrite error",		/* 0x2d */
	"blank tape error",		/* 0x2e */
	"",				/* 0x2f */
	"unknown tape format",		/* 0x30 */
	"format failed",		/* 0x31 */
	"",				/* 0x32 */
	"tape length error",		/* 0x33 */
	"invalid cdb",			/* 0x34 */
	"undetected ecc error",		/* 0x35 */
	"no gap found",			/* 0x36 */
	"miscorrected error",		/* 0x37 */
	"block sequence error",		/* 0x38 */
	"tape not ready",		/* 0x39 */
	"no tape installed",		/* 0x3a */
	"tape position error",		/* 0x3b */
	0
};
#define MAX_HP_ERROR_STR \
	(sizeof(st_hp_error_str)/sizeof(st_hp_error_str[0]))


static char *st_emulex_error_str[] = {
	"",				/* 0x00 */
	"",				/* 0x01 */
	"",				/* 0x02 */
	"",				/* 0x03 */
	"drive not ready",		/* 0x04 */
	"",				/* 0x05 */
	"",				/* 0x06 */
	"",				/* 0x07 */
	"",				/* 0x08 */
	"no tape",			/* 0x09 */
	"tape too short",		/* 0x0a */
	"drive timeout",		/* 0x0b */
	"",				/* 0x0c */
	"",				/* 0x0d */
	"",				/* 0x0e */
	"",				/* 0x0f */
	"",				/* 0x10 */
	"hard data error",		/* 0x11 */
	"",				/* 0x12 */
	"",				/* 0x13 */
	"block not found",		/* 0x14 */
	"",				/* 0x15 */
	"dma timeout",			/* 0x16 */
	"write protected",		/* 0x17 */
	"soft data error",		/* 0x18 */
	"bad block",			/* 0x19 */
	"",				/* 0x1a */
	"",				/* 0x1b */
	"filemark detected",		/* 0x1c */
	"compare error",		/* 0x1d */
	"",				/* 0x1e */
	"",				/* 0x1f */
	"invalid command",		/* 0x20 */
	"",				/* 0x21 */
	"",				/* 0x22 */
	"",				/* 0x23 */
	"",				/* 0x24 */
	"",				/* 0x25 */
	"",				/* 0x26 */
	"",				/* 0x27 */
	"",				/* 0x28 */
	"",				/* 0x29 */
	"",				/* 0x2a */
	"",				/* 0x2b */
	"",				/* 0x2c */
	"",				/* 0x2d */
	"",				/* 0x2e */
	"",				/* 0x2f */
	"unit attention",		/* 0x30 */
	"command timeout",		/* 0x31 */
	"",				/* 0x32 */
	"append error",			/* 0x33 */
	"end-of-media",			/* 0x34 */
	0
};
#define MAX_EMULEX_ERROR_STR \
	(sizeof(st_emulex_error_str)/sizeof(st_emulex_error_str[0]))

/* 
 * scsi command decode table.
 * FIXME: should merge with sd and form composite table.
 */
static char *st_cmds[] = {
	"\010read",			/* 0x08 */
	"\012write",			/* 0x0a */
	"\020filemark write",		/* 0x10 */
	"\201backspace filemark",	/* 0x81 */
	"\202forwardspace filemark",	/* 0x82 */
	"\021space record",		/* 0x11 */
	"\001rewind",			/* 0x01 */
	"\202retension",		/* 0x83 */
	"\031erase tape",		/* 0x19 */
	"\003request sense",		/* 0x03 */
	"\037read log",			/* 0x1f */
	"\200unload",			/* 0x80 */
	"\033load",			/* 0x1b */
	"\026reserve",			/* 0x16 */
	"\027release",			/* 0x17 */
	"\036door lock",		/* 0x1e */
	"\203read append",		/* 0x84 */
	"\025mode select",		/* 0x15 */
	"\032mode sense",		/* 0x1a */
	"\022inquiry",			/* 0x12 */
	"\204read append",		/* 0x84 */
	"\000test unit ready",		/* 0x00 */
	0
};

/*
 * Return the secondary error code, if available.
 */
static u_char
st_get_error_code(dsi, ssd)
	register struct scsi_tape *dsi;
	register struct st_sense *ssd;
{
	switch (dsi->un_ctype) {
	case ST_TYPE_EMULEX:
		return (ssd->optional_8);

	case ST_TYPE_HP:
	case ST_TYPE_LMS:
	case ST_TYPE_FUJI:
	case ST_TYPE_KENNEDY:
		return (ssd->error);
	}
	return (0);		/* No secondary error code */
}


/*
 * Return the text string associated with the sense key value.
 */
static char *
st_print_key(key_code)
	register u_char key_code;
{
	static char *unknown_key = "unknown key";
	if ((key_code > MAX_KEY_ERROR_STR -1)  ||
	    st_key_error_str[key_code] == NULL) {
		return (unknown_key);
	}
	return (st_key_error_str[key_code]);
}

/*
 * Return the text string associated with the secondary error code,
 * if availiable.
 */
static char *
st_print_error(dsi, error_code)
	register struct scsi_tape *dsi;
	register u_char error_code;
{
	static char *unknown_error = " ";
	switch (dsi->un_ctype) {
	case ST_TYPE_HP:
	case ST_TYPE_KENNEDY:
	case ST_TYPE_LMS:
	case ST_TYPE_FUJI:
		if (MAX_HP_ERROR_STR > error_code  && 
	    	st_hp_error_str[error_code] != NULL)
			return (st_hp_error_str[error_code]);
		break;

	case ST_TYPE_EMULEX:
		if ((MAX_EMULEX_ERROR_STR > error_code)  && 
	    	st_emulex_error_str[error_code] != NULL)
			return (st_emulex_error_str[error_code]);
		break;
	}
	return (unknown_error);
}


/*
 * Print the sense key and secondary error codes and dump out the sense bytes.
 */
static
st_error_message(c, dsi, log_error)
	register struct scsi_ctlr *c;
	register struct scsi_tape *dsi;
	int log_error;
{
	register struct st_sense *ssd;
	register u_char *cp;
	register int i;
	register u_char error_code;
	struct scsi_unit *un = c->c_un;
        char *cmdname, **cmdtbl;
	int unknown_cmd = 1;

	/* In debug mode, dump all cdb bytes too. */
	if (st_debug)
		st_print_cmd(un);

	/* Decode command name */
	if (log_error) {
		cmdtbl = st_cmds;
		cmdname = "<unknown cmd>";
		while (*cmdtbl != (char *) NULL) {
			if ((u_char)un->un_cmd == (u_char) *cmdtbl[0]) {
				cmdname = (char *)((int)(*cmdtbl)+1);
				unknown_cmd = 0;
				break;
			}
			cmdtbl++;
		}
		log(log_error, hard_error, un-stunits, cmdname);
		if (unknown_cmd)
			st_print_cmd(un);
	}

	ssd = (struct st_sense *)cp = (struct st_sense *)dsi->un_mspl;
	error_code = st_get_error_code(dsi, ssd);
	if (error_code) {
		log(LOG_ERR, sense_msg2, un-stunits,
			ssd->key, st_print_key(ssd->key),
			error_code, st_print_error(dsi, error_code));
	} else {
		log(LOG_ERR, sense_msg1, un-stunits,
			 ssd->key, st_print_key(ssd->key));
	}
	if (st_debug) {
		printf("    sense =");
		for (i = 0; i < sizeof(struct st_sense); i++)
			printf(" %x", *cp++);
		printf("\n\n");
	}
}

/*
 * Return the retry count.  This tends to be very vendor unique.
 */
static int
st_get_retries(dsi, error)
	register struct scsi_tape *dsi;
	u_int error;
{
	register struct sysgen_sense *sgs;
	register struct st_sense *ssd;
	register struct st_log *elog;
	ssd = (struct st_sense *) dsi->un_mspl;

	/*
	 * These devices maintain their own soft error logging
	 * statistics.  For them, we'll just use their data.
	 * Otherwise, we'll just count recoverable errors.
	 */
	switch (dsi->un_ctype) {

	case ST_TYPE_WANGTEK:
		return ((ssd->optional_10 <<8) + ssd->optional_11);

	case ST_TYPE_ARCHIVE:
		return ((ssd->error <<8) + ssd->error1);

	case ST_TYPE_EMULEX:
		return ((ssd->optional_9 <<8) + ssd->optional_10);

	case ST_TYPE_HP:
	case ST_TYPE_LMS:
	case ST_TYPE_FUJI:
		/* Only valid if read log command worked. */
		if (dsi->un_last_cmd != SC_READ_LOG  &&  error)
			break;

		elog = (struct st_log *) dsi->un_mspl;
		if (dsi->un_lastiow)
			return ((elog->wr_soft[0] <<8) + elog->wr_soft[1]);
		else
			return ((elog->rd_soft[0] <<8) + elog->rd_soft[1]);

	case ST_TYPE_SYSGEN:
		sgs = (struct sysgen_sense *) dsi->un_mspl;
		return (sgs->retry_ct);

	case ST_TYPE_EXB8200:
	case ST_TYPE_EXB8500:
		return ((ssd->optional_16 <<16) + (ssd->optional_17 <<8) +
			 ssd->optional_18);
	}

	/*
	 * The default case is to count the number of times
	 * a recoverable error sense key is posted.
	 */
	if (dsi->un_status == SC_RECOVERABLE_ERROR) 
		return (dsi->un_retry_ct +1);
	else
		return (dsi->un_retry_ct);
}

/*
 * Perform max. record blocking.  If greater than 64KB -1, block into
 * 64 KB -2 requests or DMA engine will fail.  Otherwise, let it through
 * unmodified.
 */
void
stminphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > MAX_ST_DEV_SIZE +1)
		bp->b_bcount = MAX_ST_DEV_SIZE;
}


stread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register struct scsi_unit *un;
	register int resid;
	register u_int unit;
	register struct scsi_tape *dsi;

	/*DPRINTF("stread:\n");*/
	unit = MTUNIT(dev);
	if (unit >= nstape) {
		EPRINTF2("st%d:  stread: invalid unit %d\n", unit, unit);
		return (ENXIO);
	}
	un = &stunits[unit];
	dsi = &stape[unit];
#ifdef 	lint
	resid = 0; resid = resid;
#endif	lint

	if (dsi->un_options & ST_VARIABLE) {
#ifdef		ST_AUTOPOSITION
		/*
		 * Position the tape to the desired record.  Note,
		 * no repositioning will take place if the request
		 * count is not the same as the device block size.
		 * The device block size is the request size of block 0.
		 */
		if (uio->uio_offset != dsi->un_offset) {

			if ((dsi->un_options & ST_NO_POSITION) == 0  &&
#if			    ST_AUTOPOSITION < 1
			    uio->uio_iov->iov_len == BLKDEV_IOSIZE  &&
#endif			    ST_AUTOPOSITION < 1
			    uio->uio_iov->iov_len == dsi->un_dev_bsize) {

				un->un_blkno = uio->uio_offset / dsi->un_dev_bsize;
				EPRINTF2("stread:  want blk= %u, at blk= %u\n",
					 un->un_blkno, dsi->un_next_block);

			   	if (stposition_block(dev, dsi, un->un_blkno)) {
					dsi->un_options |= ST_NO_POSITION;
					EPRINTF("stread:  warning, tape not repositioned\n");
					DPRINTF1("stread:  block size= %d\n",
						 dsi->un_dev_bsize);
					return (EIO);
				} else {
					dsi->un_dev_bsize = uio->uio_iov->iov_len;
					DPRINTF("stread:  tape repositioned\n");
					DPRINTF1("stread:  block size= %d\n",
						 dsi->un_dev_bsize);
				}
			}
		} else if (uio->uio_offset == 0) {
			dsi->un_dev_bsize = uio->uio_iov->iov_len;
			EPRINTF1("stread:  block size= %d\n",
				 dsi->un_dev_bsize);
		}
		dsi->un_offset = uio->uio_offset + uio->uio_iov->iov_len;
#endif		ST_AUTOPOSITION

		resid = uio->uio_resid;
		return (physio(ststrategy, &un->un_rbuf, dev, B_READ,
			       stminphys, uio));

	/* HANDLE FIXED LENGTH BLOCK CASE */
	} else {
		/*
		 * If read data is not a multiple of our block size,
		 * we won't let you read.
		 */
		if (dsi->un_iovlen != uio->uio_iov->iov_len) {
			DPRINTF("stread:  checking block size\n");
			dsi->un_iovlen = uio->uio_iov->iov_len;
			if (uio->uio_iov->iov_len % dsi->un_dev_bsize) {
				log(LOG_ERR, size_error, unit,
					"stread", dsi->un_dev_bsize);
				return (EINVAL);
			}
		}
#ifdef		ST_AUTOPOSITION
		/*
		 * Position the tape to the desired record.
		 */
		if (uio->uio_offset != dsi->un_offset) {
			un->un_blkno = uio->uio_offset / dsi->un_dev_bsize;

#if			ST_AUTOPOSITION < 1
			if ((dsi->un_options & ST_NO_POSITION) == 0  &&
			    uio->uio_iov->iov_len == BLKDEV_IOSIZE) {

				EPRINTF2("stread:  repositioning, want blk= %u, at blk= %u\n",
					 (int)un->un_blkno, dsi->un_next_block);
				if (stposition_block(dev, dsi, un->un_blkno)) {
					dsi->un_options |= ST_NO_POSITION;
					EPRINTF("stread:  repositioning failure\n");
					return (EIO);
				}
			}
#else			ST_AUTOPOSITION < 1
			if ((dsi->un_options & ST_NO_POSITION) == 0) {
				EPRINTF2("stread:  repositioning, want blk= %u, at blk= %u\n",
					 (int)un->un_blkno, dsi->un_next_block);
				if (stposition_block(dev, dsi, un->un_blkno)) {
					dsi->un_options |= ST_NO_POSITION;
					EPRINTF("stread:  repositioning failure\n");
					return (EIO);
				}
			}
#endif			ST_AUTOPOSITION < 1
		}
		dsi->un_offset = uio->uio_offset + uio->uio_iov->iov_len;
#endif		ST_AUTOPOSITION

		resid = uio->uio_resid;
		return (physio(ststrategy, &un->un_rbuf, dev, B_READ,
			   minphys, uio));
	}
}


stwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register struct scsi_unit *un;
	register struct scsi_tape *dsi;
	register u_int unit;

	/*DPRINTF("stwrite:\n");*/
	unit = MTUNIT(dev);
	if (unit >= nstape) {
		EPRINTF2("st%d:  stwrite: invalid unit %d\n", unit, unit);
		return (ENXIO);
	}
	un = &stunits[unit];
	dsi = &stape[unit];

	/*
	 * If the tape was opened read-only, don't allow writes.
	 */
	if (dsi->un_read_only) {
		log(LOG_ERR, wprotect_error,unit);
		dsi->un_err_resid = uio->uio_iov->iov_len;
		dsi->un_err_fileno = dsi->un_fileno;
		dsi->un_err_blkno = dsi->un_next_block;
		dsi->un_status = SC_WRITE_PROTECT;
		return (EACCES);
	}

	if (dsi->un_options & ST_VARIABLE) {
#ifdef		ST_AUTOPOSITION
		/*
		 * Check tape position. Only allow repositioning to the
		 * beginning of a file.  Do not allow positioning within
		 * a file.  Note, if this is a QIC tape you'll die later
		 * since overwriting is prohibited.
		 */
		if (uio->uio_offset != dsi->un_offset) {

			if (uio->uio_offset != 0  ||
#if			    ST_AUTOPOSITION < 1
			    uio->uio_iov->iov_len != BLKDEV_IOSIZE  ||
#endif			    ST_AUTOPOSITION < 1
			    stposition_block(dev, dsi, 0)) {
				EPRINTF("stwrite:  warning, tape not repositioned\n");
				EPRINTF2("stwrite:  want blk= %u, at blk=%u\n",
					 (uio->uio_offset / dsi->un_dev_bsize),
					 dsi->un_next_block);
			} else {
				DPRINTF("stwrite:  tape repositioned\n");
				dsi->un_dev_bsize = uio->uio_iov->iov_len;
			}
		} else {
			dsi->un_dev_bsize = uio->uio_iov->iov_len;
		}
		dsi->un_offset = uio->uio_offset + uio->uio_iov->iov_len;
#endif		ST_AUTOPOSITION

		return (physio(ststrategy, &un->un_rbuf, dev, B_WRITE,
			       stminphys, uio));

	/* HANDLE FIXED LENGTH BLOCK CASE */
	} else {
		/*
		 * If write data is not a multiple of our block size,
		 * we won't let you write.
		 */
		if (dsi->un_iovlen != uio->uio_iov->iov_len) {
			DPRINTF("stwrite:  checking block size\n");
			if (uio->uio_iov->iov_len % dsi->un_dev_bsize) {
				log(LOG_ERR, size_error, unit,
					"stwrite", dsi->un_dev_bsize);
				return (EINVAL);
			}
			dsi->un_iovlen = uio->uio_iov->iov_len;
		}
#ifdef		ST_AUTOPOSITION
		/*
		 * Check tape position. Only allow repositioning to the
		 * beginning of a file.  Do not allow positioning within
		 * a file.  Note, if this is a QIC tape you'll die later
		 * since overwriting is prohibited.
		 */
		if (uio->uio_offset != dsi->un_offset) {

			if (uio->uio_offset != 0  ||
#if			    ST_AUTOPOSITION < 1
			    uio->uio_iov->iov_len != BLKDEV_IOSIZE  ||
#endif			    ST_AUTOPOSITION < 1
			    stposition_block(dev, dsi, 0)) {
				EPRINTF("stwrite:  warning, tape not repositioned\n");
				EPRINTF2("stwrite:  want blk= %u, at blk= %u\n",
					 (uio->uio_offset / dsi->un_dev_bsize),
					 dsi->un_next_block);
			}
		}
		dsi->un_offset = uio->uio_offset + uio->uio_iov->iov_len;
#endif		ST_AUTOPOSITION

		return (physio(ststrategy, &un->un_rbuf, dev, B_WRITE,
			       minphys, uio));
	}
}


/*
 * Position_eom - position the tape to the end of recorded media.
 * Returns 0 (success) or error code (failure).
 */
/*ARGSUSED*/
static
stposition_eom(dev, dsi, fileno)
	register dev_t dev;
	register struct scsi_tape *dsi;
	register int fileno;
{
	register int i, err;
	register u_int unit;
	unit = MTUNIT(dev);
#ifdef	lint
	unit = unit;
#endif	lint

	for (i = 0; i < fileno; i++) {
		EPRINTF("stposition_eom:  space 1 file\n");
		if (err=stcmd(dev, SC_SPACE_FILE, 1)) {
			if (dsi->un_status == SC_BLANK_CHECK)
				break;

			EPRINTF("stposition_eom:  failed\n");
			dsi->un_err_resid = fileno - i;
			return (err);	/* Failed */
		}
		/*
		 * If not QIC, check for the end of recorded data.
		 */
		if ((dsi->un_options & ST_QIC) == 0) {
			EPRINTF("stposition_eom:  space rec\n");
			if (err=stcmd(dev, SC_SPACE_REC, 1)) {
				EPRINTF1("stposition_eom:  space rec status = 0x%x\n",
				 	dsi->un_status);
				dsi->un_status = SC_BLANK_CHECK;
				break;
			}
		}
	}
	dsi->un_err_resid = fileno - i;
	if (dsi->un_options & ST_REEL) {
		EPRINTF("stposition_eom:  backspace filemark\n");
		if (err=stcmd(dev, SC_BSPACE_FILE, 1)) {
			EPRINTF("stposition_eom:  ... failed\n");
			return (err);	/* Failed */
		} else {	
			/* Fix up position info. */
			dsi->un_fileno++;
			dsi->un_err_resid--;
		}
	}	
	EPRINTF("stposition_eom:  positioned at eom\n");
	dsi->un_next_block = 0;
	dsi->un_eof = ST_NO_EOF;
	return (0);
}


/*
 * Position_file - position the tape to the specified absolute file number.
 * Returns 0 (success) or error code (failure).
 */
static
stposition_file(dev, dsi, fileno)
	register dev_t dev;
	register struct scsi_tape *dsi;
	register int fileno;
{
	register int i, err;
	register u_int unit;
	unit = MTUNIT(dev);
	/*
	 * If the file number is negative or zero, just rewind the tape
	 * and assume he wants file 0.  Note, for Sysgen, we'll try this
	 * twice since it fails first time if you're within a file.
	 */
	if (fileno <= 0) {
		EPRINTF("stposition_file:  back spacing to BOT...\n");
		if ((err=stcmd(dev, SC_REWIND, 0))  &&
		    err != EINTR  &&
		    (err=stcmd(dev, SC_REWIND, 0)))
			goto ST_FILE_ERROR;
		dsi->un_err_resid = - fileno;
		if (fileno != 0)
			dsi->un_status = SC_BOT;

	/*
	 * If the file number is less than our current file
	 * position, then we'll have to backspace some files.
	 * Note, for dumb QIC drives, we'll have to rewind
	 * and space forward since they can't backspace files.
	 */
	} else  if (fileno < dsi->un_fileno)	{
		EPRINTF("stposition_file:  back spacing...\n");
		if (dsi->un_options & ST_BSF) {
			i = dsi->un_fileno - fileno +1;
			if ((err=stcmd(dev, SC_BSPACE_FILE, i))  ||
		    	    err == EINTR  ||
			    (err=stcmd(dev, SC_SPACE_FILE, 1)))
				goto ST_FILE_ERROR;
		} else {
			if ((err=stcmd(dev, SC_REWIND, 0))  ||
		    	    err == EINTR  ||
			    (err=stcmd(dev, SC_SPACE_FILE, fileno)))
				goto ST_FILE_ERROR_RETRY;
		}

	/*
	 * If the file number is greater than the current file
	 * position, then we'll have to forward space some files.
	 */
	} else  if (fileno > dsi->un_fileno) {
		EPRINTF("stposition_file:  forward spacing...\n");
		i = fileno - dsi->un_fileno;
		if ((dsi->un_options & ST_QIC)  &&
		    dsi->un_fileno == 0  &&  i > 1) {
			/* Have to retry for Sysgens */
			if (err=stcmd(dev, SC_SPACE_FILE, 1))
				goto ST_FILE_ERROR_RETRY;
			i = i -1;
		}
		if (err=stcmd(dev, SC_SPACE_FILE, i))
			goto ST_FILE_ERROR_RETRY;
	/*
	 * If the file number is the same as the current file
	 * position, then we'll just sit here.
	 */
	 } else if ((fileno == dsi->un_fileno)  &&  (dsi->un_next_block != 0)) {
		EPRINTF("stposition_file:  begining of file...\n");
		if (dsi->un_options & ST_BSF) {
			if ((err=stcmd(dev, SC_BSPACE_FILE, 1))  ||
		    	    err == EINTR  ||
			    (err=stcmd(dev, SC_SPACE_FILE, 1)))
				goto ST_FILE_ERROR;
		} else {
			if ((err=stcmd(dev, SC_REWIND, 0))  ||
		    	    err == EINTR  ||
			    (err=stcmd(dev, SC_SPACE_FILE, fileno)))
				goto ST_FILE_ERROR_RETRY;
		}
	}
	dsi->un_next_block = 0;
	return (0);

	/*
	 * If the forward space fails, try again, but space 1 file at a
	 * time to catch location of failure.  Note, for 1/2" reel
	 * tape, we'll have to backspace 1 filemark since the tape
	 * ends with 2 filemarks. 
	 *
	 * Suppress printing extraneous error messages on failed FSFs if
	 * we are at EOM.  This allows the user to use FSF to get to the
	 * EOM by doing mt fsf <bignum>.  We will however, return a
	 * error on exit.
	 */
ST_FILE_ERROR_RETRY:
	/* Suppress original error, if we're successful */
	if (err != EINTR  &&  (err=stcmd(dev, SC_REWIND, 0) == 0)) {
		EPRINTF("stposition_file:  retrying space file...\n");
		if ((err=stposition_eom(dev, dsi, fileno)) == 0) {
				dsi->un_next_block = 0;
				return (0);
		}
	}

ST_FILE_ERROR:
	log(LOG_ERR, position_error, unit, "file");
	return (err);
}

/*
 * Position_tape - position the tape at the requested block from
 * the begining of the current tape file.  Returns 0 (success) or
 * error code (failure).
 *	 After positioning the tape, dsi->un_next_block reflects the
 * new position.
 *	 Note, this routine does not support writing; only reading.
 * For writes, you should pad with null blocks and only fail on
 * backward motion.
 */
/*ARGSUSED*/
static
stposition_block(dev, dsi, block)
	register dev_t dev;
	register struct scsi_tape *dsi;
	register int block;
{
	register int delta;
	int fileno, err=0;
	u_int unit;

	unit = MTUNIT(dev);
	fileno = dsi->un_fileno;
	dsi->un_err_resid = 0;

	/*
	 * If block < 0, position to block 0 and return error as there
	 * are no blocks < 0. If block = 0, position to block 0.
	 */
	if (block <= 0) {
		EPRINTF("stposition_block:  beginning of file...\n");
		if (dsi->un_next_block)
			err=stposition_file(dev, dsi, fileno);
		dsi->un_err_resid = - block;

		/* If block < 0, return error (eof or bot) */
		if (err == 0  &&  block != 0) {
			dsi->un_err_fileno = dsi->un_fileno;
			dsi->un_err_blkno = dsi->un_next_block;
			if (fileno)
				dsi->un_status = SC_EOF;
			else
				dsi->un_status = SC_BOT;
			return (EIO);
		}
		/* If block = 0, we're done */
		return (err);
	}

	/*
	 * If record is behind us, space backward to it.
	 */
	delta = block - dsi->un_next_block;
	if (delta < 0) {
		EPRINTF1("stposition_block:  space %d blocks\n",
			 delta);
		if (dsi->un_options & ST_BSR) {
			if (err=stcmd(dev, SC_SPACE_REC, delta))
				goto ST_BLOCK_ERROR_RETRY;
		} else if ((dsi->un_options & ST_NO_FSR) == 0) {
			if ((err=stposition_file(dev, dsi, dsi->un_fileno))  ||
		    	    err == EINTR  ||
			    (err=stcmd(dev, SC_SPACE_REC, block)))
				goto ST_BLOCK_ERROR_RETRY;
		} else {
			/* Handle Sysgen here (no FSR) */
			goto ST_BLOCK_ERROR_RETRY;
		}

	/*
	 * If record is ahead of us, space forward to it.
	 */
	} else  if (delta > 0) {
		EPRINTF1("stposition_block:  space %d blocks\n",
			 delta);
		if ((dsi->un_options & ST_NO_FSR) == 0) {
			if (err=stcmd(dev, SC_SPACE_REC, delta))
				goto ST_BLOCK_ERROR_RETRY;
		} else {
			/* Handle Sysgen here (no FSR) */
			goto ST_BLOCK_ERROR_RETRY;
		}
	}

	/* Handle delta = 0 case here */
	/*DPRINTF("stposition_block:  position ok\n");*/
	return (err);


	/*
	 * If the forward space record fails, try again, but space
	 *  1 record at a time to catch location of failure.  Note,
	 *
	 * Suppress printing error messages on failed FSRs if we are
	 * at EOF.  This allows the user to use FSF to get to the
	 * EOF by doing mt fsr <bignum>.
	 */
ST_BLOCK_ERROR_RETRY:
	EPRINTF("stposition_block:  retrying space record...\n");
	if (dsi->un_eof == ST_EOF_PENDING  || dsi->un_eof == ST_EOF_LOG  ||
	    dsi->un_eof == ST_EOF) {
		EPRINTF("stposition_block: logging eof\n");
		dsi->un_eof = ST_NO_EOF;
		dsi->un_fileno++;
		dsi->un_next_block = 0;
		dsi->un_last_block = INF;
		dsi->un_lastior = 0;
		dsi->un_offset = 0;
	}
	if (err=stposition_file(dev, dsi, fileno))
		goto ST_BLOCK_ERROR;
	if (block != 0) {
		for (delta = 0; delta < block; delta++) {
			DPRINTF("stposition_block:  space 1 block\n");
			if (err=stcmd(dev, SC_SPACE_REC, 1)) {
				dsi->un_err_resid = block - delta;
#ifdef				ST_SYSGEN
				if (IS_SYSGEN(dsi)  &&
				    (dsi->un_status == SC_EOF)) {
					dsi->un_err_resid--;
					dsi->un_err_blkno = dsi->un_next_block;
				}
#endif				ST_SYSGEN
				if (dsi->un_status == SC_EOF) {
					EPRINTF("stposition_block: eof\n");
					break;
				}
				goto ST_BLOCK_ERROR;
			}
		}
		return (0);
	} else {
		dsi->un_err_resid = block;
		return (0);
	}

ST_BLOCK_ERROR:
	log(LOG_ERR, position_error, unit, "block");
	return (err);
}


/*ARGSUSED*/
stioctl(dev, cmd, data, flag)
	dev_t dev;
	register u_int cmd;
	register caddr_t data;
	int flag;
{
	register struct mtop *mtop;
	register int callcount, fcount;
	register struct mtget *mtget;
	register u_int unit;
	register struct scsi_tape *dsi;
	register int err= 0;
	u_char reten_rewind = 0;
	static u_int ops[] = {
		SC_WRITE_FILE_MARK,	/* write tape mark */
		SC_SPACE_FILE,		/* forward space filemark */
		SC_BSPACE_FILE,		/* backspace filemark */
		SC_SPACE_REC,		/* forward space record */
		SC_SPACE_REC,		/* backspace record */
		SC_REWIND,		/* rewind tape */
		SC_UNLOAD,		/* unload */
		SC_REQUEST_SENSE,	/* get status */
		SC_REWIND,		/* retension  */
		SC_ERASE_TAPE,		/* erase entire tape */
		SC_SPACE_FILE,		/* position to EOM */
		SC_BSPACE_FILE,		/* backspace filemark then forward space
					 * filemark */
	};

	unit = MTUNIT(dev);
	if (unit >= nstape) {
		EPRINTF1("st%d:  stioctl: invalid unit\n", unit);
		return (ENXIO);
	}
	dsi = &stape[unit];
	switch (cmd) {
	case MTIOCTOP:			/* Tape operation */
		mtop = (struct mtop *) data;
		switch (mtop->mt_op) {

		case MTNBSF:
			dsi->un_status = 0;
			fcount = dsi->un_fileno - mtop->mt_count;
			if (dsi->un_eof == ST_EOF_PENDING  ||
			    dsi->un_eof == ST_EOF_LOG  ||
			    dsi->un_eof == ST_EOF) {
				EPRINTF("stioctl:  logging eof\n");
				dsi->un_eof = ST_NO_EOF;
				dsi->un_fileno++;
				dsi->un_next_block = 0;
				dsi->un_last_block = INF;
				dsi->un_lastior = 0;
				dsi->un_offset = 0;
			}
			DPRINTF2("MTNBSF:  file= %u, delta= %d\n",
				dsi->un_fileno, fcount);
			if (dsi->un_lastiow) {
				(void) stwrite_eof(dev, dsi);
				dsi->un_lastiow = 0;
			}
			dsi->un_options &= ~ST_NO_POSITION;
			if (err=stposition_file(dev, dsi, fcount))
				return (err);
			return (0);

		case MTFSF:
			if (dsi->un_lastiow) {
				EPRINTF("stioctl:  can't fsf after writing\n");
				dsi->un_err_resid = mtop->mt_count;
				dsi->un_err_fileno = dsi->un_fileno;
				dsi->un_err_blkno = dsi->un_next_block;
				dsi->un_status = SC_ILLEGAL_REQUEST;
				return (EIO);
			}
			dsi->un_status = 0;
			fcount = dsi->un_fileno + mtop->mt_count;
			if (dsi->un_eof == ST_EOF_PENDING  ||
			    dsi->un_eof == ST_EOF_LOG  ||
			    dsi->un_eof == ST_EOF) {
				EPRINTF("stioctl:  logging eof\n");
				dsi->un_eof = ST_NO_EOF;
				dsi->un_fileno++;
				dsi->un_next_block = 0;
				dsi->un_last_block = INF;
				dsi->un_lastior = 0;
				dsi->un_offset = 0;
			}
			DPRINTF2("MTFSF:  file= %u, delta= %d\n",
				dsi->un_fileno, fcount);
			dsi->un_options &= ~ST_NO_POSITION;
			if (err=stposition_file(dev, dsi, fcount))
				return (err);
			return (0);

		case MTBSR:
			dsi->un_status = 0;
			fcount = dsi->un_next_block - mtop->mt_count;
			if (err=stposition_block(dev, dsi, fcount))
				return (err);
			return (0);

		case MTFSR:
			if (dsi->un_lastiow) {
				EPRINTF("stioctl:  can't fsr after writing\n");
				dsi->un_err_resid = mtop->mt_count;
				dsi->un_err_fileno = dsi->un_fileno;
				dsi->un_err_blkno = dsi->un_next_block;
				dsi->un_status = SC_ILLEGAL_REQUEST;
				return (EIO);
			}
			dsi->un_status = 0;
			fcount = dsi->un_next_block + mtop->mt_count;
			if (err=stposition_block(dev, dsi, fcount))
				return (err);
			return (0);

		case MTRETEN:
			/* Only for cartridge tape */
			if ((dsi->un_options & ST_QIC) == 0) {
				dsi->un_err_resid = mtop->mt_count;
				dsi->un_err_fileno = dsi->un_fileno;
				dsi->un_err_blkno = dsi->un_next_block;
				dsi->un_status = SC_ILLEGAL_REQUEST;
				return (ENOTTY);
			}
			reten_rewind = 1;
			/* Fall through into MTREW... */

		case MTREW:
			dsi->un_status = 0;
			dsi->un_err_resid = 0;
			if (mtop->mt_count > 1)
				dsi->un_err_resid = mtop->mt_count -1;
			if (dsi->un_lastiow) {
				(void) stwrite_eof(dev, dsi);
				dsi->un_lastiow = 0;
			}
#ifdef			ST_SYSGEN
			/*
			 * If Sysgen and in the middle of a file, rewind
			 * twice since the first will fail.
			 */
			if (IS_SYSGEN(dsi)  &&  dsi->un_next_block != 0)
				err = stcmd(dev, SC_REWIND, 0);
#endif			ST_SYSGEN
			if (err == EINTR  ||  (err=stcmd(dev, SC_REWIND, 0)))
				return (err);
			dsi->un_options &= ~ST_NO_POSITION;
			dsi->un_reten_rewind = reten_rewind;
			err = stcmd(dev, SC_REWIND, 0);
			return (err);

		case MTWEOF:
			if (dsi->un_read_only) {
				log(LOG_ERR, wprotect_error,unit);
				dsi->un_err_resid = mtop->mt_count;
				dsi->un_err_fileno = dsi->un_fileno;
				dsi->un_err_blkno = dsi->un_next_block;
				dsi->un_status = SC_WRITE_PROTECT;
				return (EACCES);
			}
			dsi->un_status = 0;
			dsi->un_err_resid = 0;
#ifdef			ST_SYSGEN
			if (IS_SYSGEN(dsi)) {
				callcount = 1;
				fcount = mtop->mt_count;
			} else {
				callcount = mtop->mt_count;
				fcount = 1;
			}
#else			ST_SYSGEN
			callcount = 1;
			fcount = mtop->mt_count;
#endif			ST_SYSGEN
			break;

		case MTBSF:
			/*
			 * This ioctl only works for those devices
			 * which can bacspace filemarks (e.g. 1/2" tape).
			 */
			if ((dsi->un_options & ST_BSF) == 0) {
				dsi->un_err_resid = mtop->mt_count;
				dsi->un_err_fileno = dsi->un_fileno;
				dsi->un_err_blkno = dsi->un_next_block;
				dsi->un_status = SC_ILLEGAL_REQUEST;
				return (ENOTTY);
			}
			dsi->un_status = 0;
			callcount = 1;
			fcount = mtop->mt_count;
			break;

		case MTERASE:
			DPRINTF("stioctl:  erase\n");
			if (dsi->un_read_only) {
				log(LOG_ERR, wprotect_error,unit);
				dsi->un_offset = 0;
				dsi->un_err_resid = 0;
				dsi->un_err_fileno = dsi->un_fileno;
				dsi->un_err_blkno = dsi->un_next_block;
				dsi->un_status = SC_WRITE_PROTECT;
				return (EACCES);
			}
			dsi->un_status = 0;
			dsi->un_err_resid = 0;
			if (mtop->mt_count > 1)
				dsi->un_err_resid = mtop->mt_count -1;
#ifdef			ST_SYSGEN
			/* If in the middle of a file, doit twice. */
			if (IS_SYSGEN(dsi)  &&  dsi->un_next_block != 0)
				(void) stcmd(dev, SC_REWIND, 0);
#endif			ST_SYSGEN
			if ((err=stcmd(dev, SC_REWIND, 0)) == EINTR)
				return (err);
			callcount = 1;
			fcount = 1;
			break;

		case MTOFFL:
			DPRINTF("stioctl:  offline\n");
			dsi->un_status = 0;
			dsi->un_err_resid = 0;
			if (mtop->mt_count > 1)
				dsi->un_err_resid = mtop->mt_count -1;
			if (dsi->un_lastiow) {
				(void) stwrite_eof(dev, dsi);
				dsi->un_lastiow = 0;
			}
			dsi->un_read_only = 0;	/* for broken QIC-150's */
			dsi->un_options &= ~ST_NO_POSITION;
#ifdef			ST_SYSGEN
			/* If Sysgen, do it twice as first one will fail. */
			if (IS_SYSGEN(dsi)) {
				(void) stcmd(dev, SC_REWIND, 0);
				(void) stcmd(dev, SC_REWIND, 0);
				return (0);
			}
#endif			ST_SYSGEN
			(void) stcmd(dev, SC_REWIND, 0);
			(void) stcmd(dev, SC_UNLOAD, 0);
			(void) stcmd(dev, SC_RELEASE, 0);	/* Do last! */
			return (0);

		case MTEOM:
			EPRINTF("stioctl:  eom\n");
			dsi->un_status = 0;
			if (dsi->un_lastiow == 0)
				err=stposition_eom(dev, dsi, INF);

			dsi->un_options &= ~ST_NO_POSITION;
			dsi->un_err_resid = 0;	/* hide files not skipped */
			if (err)
				return (err);
			dsi->un_status = 0;	/* hide error code too */
			return (0);

		case MTNOP:
			dsi->un_status = 0;
			return (0);

		default:
			EPRINTF("stioctl:  illegal command\n");
			dsi->un_err_resid = mtop->mt_count;
			dsi->un_err_fileno = dsi->un_fileno;
			dsi->un_err_blkno = dsi->un_next_block;
			dsi->un_status = SC_ILLEGAL_REQUEST;
			return (ENOTTY);
		}


		if (callcount <= 0  ||  fcount <= 0) {
			EPRINTF("stioctl:  invalid parameters\n");
			return (EINVAL);
		}
		while (--callcount >= 0) {
			if ((err=stcmd(dev, ops[mtop->mt_op], fcount))) {
				EPRINTF("stioctl:  command failed\n");
				return (err);
			}
		}
		/* For HP, force rewind and reset soft error accounting. */
		if (mtop->mt_op == MTERASE  &&  (dsi->un_options & ST_REEL)) {
			(void) stcmd(dev, SC_REWIND, 0);

			if (dsi->un_options & ST_ERRLOG)
				(void) stcmd(dev, SC_READ_LOG, 0);
		}
		return (0);


	case MTIOCGET:				/* Get tape status */
		DPRINTF("stioctl:  get tape status\n");
		mtget = (struct mtget *) data;
		mtget->mt_erreg = dsi->un_status;
		mtget->mt_dsreg =  dsi->un_retry_ct;

		/* If no error, report current file position. */
		if (dsi->un_status == 0) {
			mtget->mt_resid = 0;
			mtget->mt_fileno = dsi->un_fileno;
			mtget->mt_blkno =  dsi->un_next_block;
		} else {
			mtget->mt_resid =  dsi->un_err_resid;
			mtget->mt_fileno = dsi->un_err_fileno;
			mtget->mt_blkno =  dsi->un_err_blkno;
		}
		dsi->un_status = 0;		/* Reset status */
		mtget->mt_type = dsi->un_ctype;
		mtget->mt_flags = MTF_SCSI | MTF_ASF;
		if ((mtget->mt_type >= MT_ISCDC) &&
		    (mtget->mt_type <= MT_ISHP)) {	/* 1/2" reels */
			mtget->mt_flags |= MTF_REEL;
			mtget->mt_bf = 20;
		}
		else {					/* 1/4" cartridges */
			switch (mtget->mt_type) {

			/* older 1/4" cartridge tapes */
			case MT_ISSYSGEN11:
			case MT_ISSYSGEN:
			case MT_ISAR:
				mtget->mt_bf = 126;
				break;

			/* current 1/4" cartridge tapes */
			default:
				mtget->mt_bf = 40;
				break;
			}
		}
#ifdef		sun2
		if (dsi->un_options & ST_NO_QIC24)
			mtget->mt_type--;
#endif		sun2
		return (0);

	default:
		EPRINTF("stioctl:  illegal command\n");
		dsi->un_err_resid = 1;
		dsi->un_err_fileno = dsi->un_fileno;
		dsi->un_err_blkno = dsi->un_next_block;
		dsi->un_status = SC_ILLEGAL_REQUEST;
		return (ENOTTY);
	}
}
#endif	NST > 0
