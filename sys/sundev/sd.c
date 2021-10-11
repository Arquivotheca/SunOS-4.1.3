#ident	"@(#)sd.c 1.1 92/07/30 Copyr 1990 Sun Micro"

#include "sd.h"
#if NSD > 0

/*#define SDDEBUG 		/* Allow compiling of debug code */

/*
 * SCSI driver for SCSI disks.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/dkbad.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/syslog.h>

#include <machine/psl.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/mbvar.h>
#include <sundev/screg.h>
#include <sundev/sireg.h>
#include <sundev/scsi.h>
#include <sundev/sdreg.h>

#if defined(SD_FORMAT)
#include <vm/faultcode.h>
#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h> 
#endif defined(SD_FORMAT)

/* Driver error recovery and error message error thresholds */
#define EL_FAILS 		 6 	/* Soft msg threshold */
#define MAX_FAILS 		10 	/* Soft errors before reassign */

#define EL_RETRY		 2	/* Retry msg threshold */
#define EL_REST			 0	/* Restore msg threshold */
#define MAX_RETRIES 		 4	/* Cmd retry limit */
#define MAX_RESTORES 		 3	/* Rezero unit limit (after retries) */
#define MAX_LABEL_RETRIES  	 2	/* Cmd retry limit for label */
#define MAX_LABEL_RESTORES 	 1	/* Restore limit for label */
#define MAX_SPINUP		30	/* Spin up delay limit */
#define MAX_BUSY	      1000	/* Busy status retry limit */

#define SHORT_TIMEOUT 		  2 	/* Short timeout (2 * minutes) */
#define NORMAL_TIMEOUT 		  4 	/* Read/Write timeout (2 * minutes) */
#define FORMAT_TIMEOUT 		150 	/* Format timeout (2 * minutes) */

#define LPART(dev)		(dev & (NLPART -1))
#define SDUNIT(dev)		(minor(dev) >> 3)
#define SDNUM(un)		(un-sdunits)
#define LONG			(sizeof(u_long))
#define LONG_ALIGN(arg)	  	(((u_int)arg + LONG -1) & ~(LONG -1))
#define CDB6_LIMIT	  	(-1 << 21)

/* un_present definitions */
#define ONLINE		2
#define MEDIA_CHANGED	1
#define OFFLINE		0


#ifdef SDDEBUG
short sd_debug = 1;		/*
				 * 0 = normal operation
				 * 1 = extended error info only
				 * 2 = debug and error info
				 * 3 = all status info
				 */
/* Handy debugging 0, 1, and 2 argument printfs */
#define SD_PRINT_CMD(un) \
	if (sd_debug > 1) sd_print_cmd(un)
#define DPRINTF(str) \
	if (sd_debug > 1) printf(str)
#define DPRINTF1(str, arg1) \
	if (sd_debug > 1) printf(str,arg1)
#define DPRINTF2(str, arg1, arg2) \
	if (sd_debug > 1) printf(str,arg1,arg2)

/* Handy extended error reporting 0, 1, and 2 argument printfs */
#define EPRINTF(str) \
	if (sd_debug) printf(str)
#define EPRINTF1(str, arg1) \
	if (sd_debug) printf(str,arg1)
#define EPRINTF2(str, arg1, arg2) \
	if (sd_debug) printf(str,arg1,arg2)
#define DEBUG_DELAY(cnt) \
	if (sd_debug)  DELAY(cnt)

#else SDDEBUG
#define SD_PRINT_CMD(un)
#define DPRINTF(str)
#define DPRINTF1(str, arg2)
#define DPRINTF2(str, arg1, arg2)
#define EPRINTF(str)
#define EPRINTF1(str, arg2)
#define EPRINTF2(str, arg1, arg2)
#define DEBUG_DELAY(cnt)
#endif SDDEBUG

extern char *strncpy();

extern struct scsi_unit sdunits[];
extern struct scsi_unit_subr scsi_unit_subr[];
extern struct scsi_disk sdisk[];
extern int nsdisk, sd_repair;
#ifdef	DKIOCWCHK
extern u_char sdwchkmap[];
#endif	DKIOCWCHK

short sd_restores = EL_REST;
short sd_max_retries  =  MAX_RETRIES; 
short sd_max_restores = MAX_RESTORES;
short sd_max_busy  =  MAX_BUSY; 

short sd_fails    = EL_FAILS;
short sd_min_fails    = EL_FAILS;
short sd_max_fails    = MAX_FAILS;

static char *iopb_error = "sd%d:  no iopb space for buffer\n";
static char *overflow_error= "sd%d:  Excessive soft errors!  Check drive.\n";
static char *sector_error = "sd%d:  single sector I/O failed\n";
static char *reassign_error = "sd%d:  reassigning %s block %u\n";
static char *clear_msg = "sd%d:  unable to zero remapped block\n";
static char *sense_msg1 = "sd%d%c error:  sense key(0x%x): %s\n";
static char *sense_msg2 = "sd%d%c error:  sense key(0x%x): %s, error code(0x%x): %s\n";
static char *disk_error1 = "sd%d%c:  %s %s\n";
static char *disk_error2 = "sd%d%c:  %s %s, block %u (%u relative)\n";
static char *hard_error  = "sd%d%c:  hard error, block %u (%u relative)\n";

#ifndef	SDDEBUG
short sd_retry  = EL_RETRY;	/*
				 * Error message threshold, retries.
				 * Make it global so manufacturing can
				 * override setting.
				 */

#else	SDDEBUG
short sd_retry  = 0;		/* For debug, set retries to 0. */

sd_print_buffer(y, count)
	register u_char *y;
	register int count;
{
	register int x;

	for (x = 0; x < count; x++)
		printf(" %x", *y++);
	printf("\n");
}
#endif	SDDEBUG


/*
 * Return a pointer to this unit's unit structure.
 */
sdunitptr(md)
	register struct mb_device *md;
{
	return ((int)&sdunits[md->md_unit]);
}


static
sdtimer(dev)
	register dev_t dev;
{
	register struct scsi_disk *dsi;
	register struct scsi_unit *un;
	register struct scsi_ctlr *c;
	register u_int	unit;

	unit = SDUNIT(dev);
	un = &sdunits[unit];
	dsi = &sdisk[unit];
	c = un->un_c;
#ifdef	lint
	c = c;
#endif	lint

	/* DPRINTF("sdtimer:\n"); */
	if (dsi->un_openf >= OPENING) {
		(*c->c_ss->scs_start)(un);
		if ((dsi->un_timeout != 0)  &&  (--dsi->un_timeout == 0)) {

			/* Process command timeout for normal I/O operations */
			log(LOG_CRIT,"sd%d:  I/O request timeout\n", unit);
			dsi->un_timeout = 4;
			if ((*c->c_ss->scs_deque)(c, un)) {
				/* Can't recover, reset! */
				dsi->un_timeout = 0;
				(*c->c_ss->scs_reset)(c, 1);
			}
		} else if (dsi->un_timeout != 0) {
			DPRINTF1("sd%d: sttimer running ", unit);
			DPRINTF2("open= %d,  timeout= %d\n",
				 dsi->un_openf, dsi->un_timeout);
		}

	/* Process opening delay timeout */
	} else if ((dsi->un_timeout != 0)  &&  (--dsi->un_timeout == 0)) {
		DPRINTF1("sd%d: sttimer running...\n", unit);
		wakeup((caddr_t)dev);
	}
	timeout(sdtimer, (caddr_t)dev, 30*hz);
}


/*
 * Attach device (boot time).
 */
sdattach(md)
	register struct mb_device *md;
{
	register struct scsi_unit *un;
	register struct dk_label *l;
	register struct scsi_disk *dsi;
	struct scsi_inquiry_data *sid, *sid_orig;
	int i;

	dsi = &sdisk[md->md_unit];
	un = &sdunits[md->md_unit];

	/*
	 * link in, fill in unit struct.
	 */
	un->un_md = md;
	un->un_mc = md->md_mc;
	un->un_unit = md->md_unit;
	un->un_target = TARGET(md->md_slave);
	un->un_lun = LUN(md->md_slave);
	un->un_ss = &scsi_unit_subr[TYPE(md->md_flags)];
	un->un_present = OFFLINE;

	dsi->un_openf = CLOSED;
	dsi->un_sense = NULL;
	dsi->un_flags = 0;
	dsi->un_timeout = 0;
	dsi->un_timer = 0;
	dsi->un_bad_index = 0;
	dsi->un_status = 0;
	dsi->un_retries = 0;
	dsi->un_restores = 0;
	dsi->un_sec_left = 0;
	dsi->un_total_errors = 0;
	dsi->un_err_resid = 0;
	dsi->un_err_blkno = 0;
	dsi->un_cyl_start = 0;
	dsi->un_cyl_end = 0;
	dsi->un_cylin_last = 0;
	dsi->un_lp1 = NULL;
	dsi->un_lp2 = NULL;
	dsi->un_ctype = CTYPE_UNKNOWN;

	/*
	 * Set default disk geometry and partition table.
	 * This is necessary so we can later open the device
	 * if we don't find it at probe time.
	 */
	bzero((caddr_t)&(dsi->un_g), sizeof (struct dk_geom));
	bzero((caddr_t)dsi->un_map, sizeof (struct dk_allmap));
	dsi->un_g.dkg_ncyl  = 1;
	dsi->un_g.dkg_acyl  = 0;
	dsi->un_g.dkg_pcyl  = 1;
	dsi->un_g.dkg_nhead = 1;
	dsi->un_g.dkg_nsect = 1;
	dsi->un_cyl_size = 1;

	/*
	 * Test for unit ready.  The first test checks for a non-existant
	 * device.  The other tests wait for the drive to get ready.
	 */
	if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0) > 1) {
		DPRINTF("sdattach:  unit offline\n");
		return;
        }

	/*
	 * Temporarily allocate space for label/request sense/inquiry buffer.
	 * Align it to a longword boundary.
	 */
	sid = sid_orig = (struct scsi_inquiry_data *)rmalloc(iopbmap,
			 (long)(SECSIZE + LONG));
	if (sid) {
		sid = (struct scsi_inquiry_data *) LONG_ALIGN(sid);
		dsi->un_sense = (int)sid;
	} else {
		log(LOG_CRIT, iopb_error, SDNUM(un));
		return;
	}

	(void) simple(un, SC_START, 0, 0, 1);	/* Spin-up disk */
	for (i = 0; i < MAX_SPINUP; i++) {
		if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0) == 0) {
			goto SDATTACH_UNIT;

		/* Possible device fault, maybe recoverable */
		} else if (un->un_scb.chk) {
#ifdef			SDDEBUG
			if (sd_debug > 2) {
				(void) simple(un, SC_REQUEST_SENSE,
					(char *)sid - DVMA, 0,
					sizeof (struct scsi_sense));
				EPRINTF("  sid = ");
				sd_print_buffer((u_char *)sid,
					sizeof(struct scsi_sense));
			}
#endif			SDDEBUG
			goto SDATTACH_UNIT;

		/* Reservation conflict, quit */
		} else if (un->un_scb.is  &&  un->un_scb.busy) {
			EPRINTF("sdattach:  reservation conflict\n");
			break;

		/* Busy, wait for final status. */
		} else if (un->un_scb.busy) {
			EPRINTF("sdattach:  unit busy\n");
			DELAY(2000000);		/* Wait 2 Sec. */
		}
	}
	DPRINTF("sdattach:  unit offline\n");
	return;

SDATTACH_UNIT:
	if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0) != 0) {
		DPRINTF("sdattach:  unit failed\n");
		return;
	}

	/*
	 * Only CCS controllers support Inquiry command.
	 * Note, dtype should be 0 for random access disks.
	 * Also, rdf should be 1 for CCS compatible device. 
	 */
	bzero((caddr_t)sid, (u_int)sizeof (struct scsi_inquiry_data));
	if (simple(un, SC_INQUIRY, (char *) sid - DVMA, 0,
			(int)sizeof (struct scsi_inquiry_data)) == 0) {
#ifdef		SDDEBUG
		if (sd_debug > 2) {
			printf("sdopen: vid= <%s>\n", sid->vid);
			printf("        dtype= %d, rdf= %d\n",
			       sid->dtype, sid->rdf);
			printf("  sid =");
			sd_print_buffer((u_char *)sid, 32);
		}
#endif		SDDEBUG
		dsi->un_flags = 0;
		if (bcmp(sid->vid, "EMULEX", 6) == 0) {
			EPRINTF("sdattach:  Emulex found\n");
			dsi->un_ctype = CTYPE_MD21;
		} else if (sid->dtype == 0) {
			EPRINTF("sdattach:  fixed disk found\n");
			dsi->un_ctype = CTYPE_CCS;
		} else if (sid->dtype == 4  ||  sid->dtype == 8) {
			EPRINTF("sdattach:  disk found\n");
			dsi->un_ctype = CTYPE_CCS;
		} else {
			EPRINTF("sdattach:  no disk found\n");
			return;
		}
		if (sid->rmb) {
			EPRINTF("sdattach:  removable disk\n");
			dsi->un_options |= SD_REMOVABLE;
		} else {
			dsi->un_options &= ~SD_REMOVABLE;
		}
#ifdef		SDDEBUG
		if (sid->rdf == 0) {
			EPRINTF("sdattach:  non-CCS disk\n");
		}
#endif		SDDEBUG
	} else {
		/* non-CCS, assume Adaptec ACB-4000 */
		EPRINTF("sdattach:  Adaptec found\n");
		dsi->un_ctype = CTYPE_ACB4000;
		dsi->un_flags = SC_UNF_NO_DISCON;
	}

	/*
	 * Ok, it's ready - try to read and use the label.
	 * Note uselabel also marks the unit as present.
	 */
	l = (struct dk_label *) (dsi->un_sense);
	l->dkl_magic = 0;
	if (getlabel(un, l)) {
		(void) uselabel(un, l);
	}
	rmfree(iopbmap, (long)(SECSIZE + LONG), (u_long)sid_orig);
	dsi->un_sense = NULL;

	/* Permanently allocate request sense/inquiry buffer and align it. */
	sid = (struct scsi_inquiry_data *)rmalloc(iopbmap,
			 (long)(SECSIZE + LONG));
	if (sid) {
		sid = (struct scsi_inquiry_data *) LONG_ALIGN(sid);
		dsi->un_sense = (int)sid;
	} else {
		un->un_present = OFFLINE;
		dsi->un_openf = CLOSED;
		log(LOG_CRIT, iopb_error, SDNUM(un));
	}
	DPRINTF2("sd%d:  sdattach: buffer= 0x%x, ", SDNUM(un), (int)sid);
	DPRINTF2("dsi= 0x%x, un= 0x%x\n", dsi, un);
	return;

}


/*
 * Read the label from the disk.
 * Return true if read was ok, false otherwise.
 */
static
getlabel(un, l)
	register struct scsi_unit *un;
	register struct dk_label *l;
{
	register int retries, restores;

	for (restores = 0; restores < MAX_LABEL_RESTORES; ++restores) {
		for (retries = 0; retries < MAX_LABEL_RETRIES; retries++) {
			if (simple(un, SC_READ, (char *) l - DVMA, 0, 1) == 0) {
				return (1);
			}
		}
		(void) simple(un, SC_REZERO_UNIT, 0, 0, 0);
	}
	return (0);
}


/*
 * Check the label for righteousity.  Returns 0 for OK, 1 for failure.
 * Snarf yummos from validated label.  Responsible for autoconfig
 * message, interestingly enough.  Also marks the unit as present.
 */
static
uselabel(un, l)
	register struct scsi_unit *un;
	register struct dk_label *l;
{
	register struct scsi_disk *dsi;
	register int intrlv;
	register short *sp;
	register short sum;
	register short count;

	/*
	 * Check magic number of the label
	 */
	if (l->dkl_magic != DKL_MAGIC) {
		return (1);
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
		EPRINTF1("sd%d:  corrupt label\n", SDNUM(un));
		return (1);
	}
	printf("sd%d:  <%s>\n", SDNUM(un), l->dkl_asciilabel);

	/*
	 * Fill in disk geometry from label.
	 */
	dsi = &sdisk[un->un_unit];
	dsi->un_g.dkg_ncyl = l->dkl_ncyl;
	dsi->un_g.dkg_acyl = l->dkl_acyl;
	dsi->un_g.dkg_bcyl = 0;
	dsi->un_g.dkg_nhead = l->dkl_nhead;
	dsi->un_g.dkg_bhead = l->dkl_bhead;
	dsi->un_g.dkg_nsect = l->dkl_nsect;
	dsi->un_g.dkg_gap1 = l->dkl_gap1;
	dsi->un_g.dkg_gap2 = l->dkl_gap2;
	dsi->un_g.dkg_intrlv = l->dkl_intrlv;
	dsi->un_g.dkg_pcyl = l->dkl_pcyl;
        /*
         * Old labels don't have pcyl in them, so we make a guess at it.
         */
	if (dsi->un_g.dkg_pcyl == 0) {
		EPRINTF1("sd%d:  old label\n", SDNUM(un));
                dsi->un_g.dkg_pcyl = dsi->un_g.dkg_ncyl + dsi->un_g.dkg_acyl;
	}
	dsi->un_cyl_size = dsi->un_g.dkg_nhead * dsi->un_g.dkg_nsect;

	/*
	 * Fill in partition table.
	 */
	for (count = 0; count < NLPART; count++)
		dsi->un_map[count] = l->dkl_map[count];
	/*
	 * Diddle stats if neccessary.
	 */
	if (un->un_md->md_dk >= 0) {
		intrlv = dsi->un_g.dkg_intrlv;
		if (intrlv <= 0  ||  intrlv >= dsi->un_g.dkg_nsect) {
			intrlv = 1;
		}
		dk_bps[un->un_md->md_dk] =
			(SECSIZE * 60 * dsi->un_g.dkg_nsect) / intrlv;
	}
	un->un_present = ONLINE;		/* "it's here..." */
	dsi->un_openf = OPEN_PROBE;
	return (0);
}


/*
 * Run a command in polled mode.
 * Return true if successful, false otherwise.
 */
static
simple(un, cmd, dma_addr, secno, nsect)
	register struct scsi_unit *un;
	int cmd, dma_addr, secno, nsect;
{
	register struct scsi_disk *dsi = &sdisk[un->un_unit];
	register struct scsi_cdb *cdb;
	register struct scsi_ctlr *c;
	register int err;

	/*
	 * Grab and clear the command block.
	 */
	dsi->un_openf = OPENING;
	c = un->un_c;
	cdb = &un->un_cdb;
	bzero((caddr_t)cdb, CDB_GROUP1);

	/*
	 * Plug in command block values.
	 */
	cdb->cmd = cmd;
	if (SCSI_RECV_DATA_CMD(cmd))
		un->un_flags |= SC_UNF_RECV_DATA;
	else
		un->un_flags &= ~SC_UNF_RECV_DATA;

	c->c_un = un;
	cdb->lun = un->un_lun;
	FORMG0ADDR(cdb, secno);
	FORMG0COUNT(cdb, nsect);
	un->un_dma_addr = dma_addr;
	if (cmd == SC_START)
		un->un_dma_count = 0;
	else if (cmd == SC_INQUIRY  ||  cmd == SC_REQUEST_SENSE)
		un->un_dma_count = nsect;
	else
		un->un_dma_count = nsect << SECDIV;

	/*
	 * Run the command and check the return status for errors.
	 * Note, err = 1, recoverable failure;  >1, hard failure.
	 */
	if (err = (*c->c_ss->scs_cmd)(c, un, 0)) {
		if (err > 1)
			err = 2;	/* Hard failure */
	}
	dsi->un_openf = CLOSED;
	return (err);			/* No failure */
}


/*
 * This routine opens a disk.  Note that we can handle disks
 * that make an appearance after boot time.
 */
/*ARGSUSED*/
sdopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct scsi_unit *un;
	register struct dk_label *l, *l_orig = NULL;
	register struct dk_map *lp;
	register int unit = SDUNIT(dev);
	register struct scsi_disk *dsi;
	register struct scsi_inquiry_data *sid = NULL;
	int i, s, error = 0;

	if (unit >= nsdisk) {
		DPRINTF("sdopen:  illegal unit\n");
		return (ENXIO);
	}
	un = &sdunits[unit];
	dsi = &sdisk[unit];
	if (un->un_mc == 0) {
		DPRINTF("sdopen:  disk not attached\n");
		return (ENXIO);
	}

	/*
	 * If command timeouts not activated yet, switch it on.
	 */
	if (dsi->un_timer == 0) {
		DPRINTF("sdopen:  starting timer\n");
		dsi->un_timer++;
		timeout(sdtimer, (caddr_t)dev, 30*hz);
	}

	/*
	 * If already found on autoconfig probe, use it.
	 */
	if (dsi->un_openf == OPEN_PROBE) {
		dsi->un_openf = OPEN;
		return (0);
	}
	/*
	 * If normal open, check if drive/media changed.
	 */
	if (un->un_present == ONLINE) {
		EPRINTF("sdopen:  checking device\n");
		if (sdcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0, 0) == 0)
			return (0);
	}
	/*
	 * If open in progress, wait for completion to prevent
	 * race conditions during open.
	 */
	if (dsi->un_openf == OPENING) {
		for(i = 0; i < MAX_SPINUP; i++)  {
			DPRINTF1("sd%d: sdopen:  waiting...\n", unit);
			(void) sleep((caddr_t) &lbolt, PRIBIO);
			if (dsi->un_openf != OPENING)
				break;
		}
		if (dsi->un_openf == OPEN  &&  un->un_present == ONLINE) {
			DPRINTF1("sd%d: sdopen:  open delay ok\n", unit);
			return (0);
		} else if (dsi->un_openf == OPENING) {
			EPRINTF1("sd%d: sdopen:  open wait timeout\n", unit);
			return (EBUSY);
		} else if (dsi->un_openf == CLOSED) {
			EPRINTF1("sd%d: sdopen:  prior open failed\n", unit);
			return (ENXIO);
		}
	}
	/*
	 * Didn't see it at autoconfig time?   Let's look again..
	 * Note, raise priority until we've figure out whether
	 * to enable or disable disconnect/reconnect to prevent
	 * collisions with multiple luns of same target.  Otherwise
	 * serious target failures tend to occur (some targets
	 * hang).
	 */
	DPRINTF1("sd%d: sdopen:  opening device\n", unit);
	s = splr(pritospl(un->un_mc->mc_intpri));
	dsi->un_openf = OPENING;

	lp = &dsi->un_map[LPART(dev)];
	lp->dkl_nblk = 2;
	if ((error=sdcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0, 0))  &&
	    error == ENXIO) {
		EPRINTF1("sd%d: sdopen:  not online\n", unit);
		error = ENXIO;
		goto SDOPEN_EXIT;
	}
	error = 0;

	/* Spin-up disk */
	(void) sdcmd(dev, SC_START, 0, 1, (caddr_t)0, 0);

	if (sdcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0, 0)  &&
	    sdcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0, 0)) {
		EPRINTF1("sd%d: sdopen:  not ready\n", unit);
		error = ENXIO;
		goto SDOPEN_EXIT;
	}
	/*
	 * If request sense/inquiry buffer not already allocated,
	 * allocate it and align it to a longword boundary.
	 */
	sid = (struct scsi_inquiry_data *) dsi->un_sense;
	if ((int) sid == NULL) {
		sid = (struct scsi_inquiry_data *)rmalloc(iopbmap,
			(long)(sizeof (struct scsi_inquiry_data) + LONG));
		if (sid) {
			sid = (struct scsi_inquiry_data *) LONG_ALIGN(sid);
			dsi->un_sense = (int)sid;
		} else {
			log(LOG_CRIT, iopb_error, SDNUM(un));
			error = ENXIO;
			goto SDOPEN_EXIT;
		}
	}

	/*
	 * Only CCS controllers support Inquiry command.
	 * Note, dtype should be 0 for random access disks.
	 * Also, rdf should be 1 for CCS compatible device. 
	 */
	bzero((caddr_t)sid, (u_int)sizeof (struct scsi_inquiry_data));
	if (sdcmd(dev, SC_INQUIRY, 0,
	    sizeof (struct scsi_inquiry_data), (caddr_t)sid, 0)) {
		/* non-CCS, assume Adaptec */
		EPRINTF("sdopen:  Adaptec found\n");
		dsi->un_ctype = CTYPE_ACB4000;
		dsi->un_flags = SC_UNF_NO_DISCON;
	} else {
#ifdef		SDDEBUG
		if (sd_debug > 2) {
			printf("sdopen: vid= <%s>\n", sid->vid);
			printf("        dtype= %d, rdf= %d\n",
			       sid->dtype, sid->rdf);
			printf("  sid =");
			sd_print_buffer((u_char *)sid, 32);
		}
#endif		SDDEBUG
		dsi->un_flags = 0;
		if (bcmp(sid->vid, "EMULEX", 6) == 0) {
			EPRINTF("sdopen:  Emulex found\n");
			dsi->un_ctype = CTYPE_MD21;
		} else if (sid->dtype == 0) {
			EPRINTF("sdopen:  fixed disk found\n");
			dsi->un_ctype = CTYPE_CCS;
		} else if (sid->dtype == 4  ||  sid->dtype == 8) {
			EPRINTF("sdattach:  disk found\n");
			dsi->un_ctype = CTYPE_CCS;
		} else {
			EPRINTF("sdopen:  no disk found\n");
			error = ENXIO;
			goto SDOPEN_EXIT;
		}
		if (sid->rmb) {
			EPRINTF("sdopen:  removable disk\n");
			dsi->un_options |= SD_REMOVABLE;
		} else {
			dsi->un_options &= ~SD_REMOVABLE;
		}
#ifdef		SDDEBUG
		if (sid->rdf == 0) {
			EPRINTF("sdattach:  non-CCS disk\n");
		}
#endif		SDDEBUG
	}

	/* Initialize error logging */
	dsi->un_bad_index = 0;
	dsi->un_total_errors = dsi->un_retries = 0;
	dsi->un_restores = sd_max_restores;

	/* Temporarily allocate volume label buffer and align it. */
	l = l_orig = (struct dk_label *)rmalloc(iopbmap,
			 (long)(SECSIZE + LONG));
	if (l) {
		l = (struct dk_label *) LONG_ALIGN(l);
	} else {
		log(LOG_CRIT, iopb_error, SDNUM(un));
		error = ENXIO;
		goto SDOPEN_EXIT;
	}

	DPRINTF("sdopen:  reading label\n");
	dsi->un_openf = OPEN;
	if (sdcmd(dev, SC_READ_LABEL, 0, 1, (caddr_t)l, 0)) {
		if (flag & O_NDELAY) {
			DPRINTF("sdopen:  suppressing read label error\n");
		} else {
			DPRINTF1("sd%d:  read label error\n", SDNUM(un));
			error = EIO;
		}
		goto SDOPEN_EXIT;
	}
	dsi->un_restores = 0;

	if (uselabel(un, l)) {
		if ((flag & O_NDELAY) == 0) {
			DPRINTF("sdopen:  label error\n");
			error = EIO;
			goto SDOPEN_EXIT;
		}
	}

SDOPEN_EXIT:
	if (error) {
		if (dsi->un_timer) {		/* disable timer */
			dsi->un_timer = 0;
			untimeout(sdtimer, (caddr_t)dev);
		}
		un->un_present = OFFLINE;
		dsi->un_openf = CLOSED;
	} else {
		dsi->un_openf = OPEN;
	}
	if (l_orig != NULL)			/* release label buffer */
		rmfree(iopbmap, (long)(SECSIZE + LONG), (u_long)l_orig);
	(void) splx(s);
	return (error);
}

/*
 * stub close routine for 4c compatibility.
 */
/*ARGSUSED*/
sdclose(dev, flag)
	dev_t dev;
	int flag;
{
	return (0);
}

/*
 * This routine returns the size of a logical partition.  It is called
 * from the device switch at normal priority.
 */
sdsize(dev)
	register dev_t dev;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	register struct scsi_disk *dsi;
	register int unit;

	/* DPRINTF("sdsize:\n"); */
	unit = SDUNIT(dev);
	if (unit >= nsdisk) {
		DPRINTF("sdsize:  illegal unit\n");
		return (-1);
	}

	un = &sdunits[unit];
	if (un->un_present == ONLINE) {
		DPRINTF("sdsize:  getting info\n");
		dsi = &sdisk[SDUNIT(dev)];
		lp = &dsi->un_map[LPART(dev)];
		return (lp->dkl_nblk);
	} else {
		/*DPRINTF("sdsize:  unit not present\n");*/
		return (-1);
	}
}


/*
 * This routine is the focal point of internal commands to the controller.
 * NOTE: this routine assumes that all operations done before the disk's
 * geometry is defined.
 */
sdcmd(dev, cmd, block, count, addr, options)
	dev_t dev;
	int cmd, block, count;
	caddr_t addr;
	int options;
{
	register struct scsi_disk *dsi;
	register struct mb_ctlr *mc;
	register struct scsi_unit *un;
	register struct buf *bp;
	int s, old_options, b_error;

	un = &sdunits[SDUNIT(dev)];
	dsi = &sdisk[SDUNIT(dev)];
	bp = &un->un_sbuf;
#ifdef	lint
	mc = (struct mb_ctlr *) 0; mc = mc;
	dsi= (struct scsi_disk *) 0; dsi = dsi;
	old_options = 0; old_options = old_options;
#endif	lint

	/*DPRINTF("sdcmd:\n");*/
	s = splr(pritospl(un->un_mc->mc_intpri));
	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANTED;
		/* DPRINTF1("sdcmd:  sleeping...,  bp= 0x%x\n", bp); */
		(void) sleep((caddr_t) bp, PRIBIO);
	}

	bp->b_flags = B_BUSY | B_READ;
	(void) splx(s);
	/* DPRINTF1("sdcmd:  waking...,  bp= 0x%x\n", bp); */
	/* DPRINTF1("sdcmd: waking up with addr=0x%x\n", addr); */

	un->un_scmd = cmd;
	bp->b_dev = dev;
	bp->b_bcount = count;
	bp->b_blkno = block;
	bp->b_un.b_addr = addr;
	old_options = dsi->un_options;
	dsi->un_options |= options;

#ifdef  SD_FORMAT
	if (options & (SD_DVMA_RD | SD_DVMA_WR)) {

		DPRINTF2("sdcmd:  addr= 0x%x size= 0x%x\n", addr, count);
		mc = un->un_mc;
		bp->b_flags |= B_PHYS;
		bp->b_proc = u.u_procp;
		u.u_procp->p_flag |= SPHYSIO;
		/*
		 * Fault lock the address range of the buffer.
		 */
		if (as_fault(u.u_procp->p_as, bp->b_un.b_addr,
		   	 (u_int)count, F_SOFTLOCK,
			 (options & SD_DVMA_WR) ? S_WRITE : S_READ))  {

			EPRINTF("sdcmd:  as_fault error1\n");
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			goto SDCMD_EXIT;
		}
	}
#endif  SD_FORMAT
	/*
	 * Execute the I/O request.
	 */
	sdstrategy(bp);
	(void) iowait(bp);

#ifdef  SD_FORMAT
	/*
	 * Release memory and DVMA resources.
	 */
	if (options & (SD_DVMA_RD | SD_DVMA_WR)) {
		if (as_fault(u.u_procp->p_as, bp->b_un.b_addr, 
			 (u_int)bp->b_bcount, F_SOFTUNLOCK,
			 (options & SD_DVMA_WR) ? S_WRITE : S_READ)) {

			EPRINTF("sdcmd:  as_fault error2\n");
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			goto SDCMD_EXIT;
		}
		s = splr(pritospl(un->un_mc->mc_intpri));
		u.u_procp->p_flag &= ~SPHYSIO;
		bp->b_flags &= ~(B_BUSY | B_PHYS);
		(void) splx(s);
	}

SDCMD_EXIT:
#endif  SD_FORMAT

	/* s = splr(pritospl(un->un_mc->mc_intpri)); */
	dsi->un_options = old_options;
	b_error = geterror(bp);
	bp->b_flags &= ~B_BUSY;
	if (bp->b_flags & B_WANTED) {
		/* DPRINTF1("sdcmd:  waking...,  bp= 0x%x\n", bp); */
		wakeup((caddr_t)bp);
	}
	/* (void) splx(s); */
	return (b_error);
}


/*
 * This routine is the high level interface to the disk.  It performs
 * reads and writes on the disk using the buf as the method of communication.
 * It is called from the device switch for block operations and via physio()
 * for raw operations.  It is called at normal priority.
 */

int (*sdstrategy_tstpoint)();

sdstrategy(bp)
	register struct buf *bp;
{
	register struct scsi_unit *un;
	register struct mb_device *md;
	register struct dk_map *lp;
	register u_int bn, cyl;
	register struct diskhd *dh;
	register struct scsi_disk *dsi;
	register int s;
	int unit = dkunit(bp);

	if (sdstrategy_tstpoint != NULL) {
		if ((*sdstrategy_tstpoint)(bp)) {
			return;
		}
	}


	/* DPRINTF("sdstrategy:\n"); */
	if (unit >= nsdisk) {
		EPRINTF("sdstrategy: invalid unit\n");
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	un = &sdunits[unit];
	md = un->un_md;
	dsi = &sdisk[unit];
	lp = &dsi->un_map[LPART(bp->b_dev)];
	bn = bp->b_blkno;

	if (bp != &un->un_sbuf)  {
		if ((un->un_present < ONLINE)  ||  (lp->dkl_nblk == 0)) {
			DPRINTF("sdstrategy:  unit not present\n");
			DPRINTF2("sdstrategy:  bn= 0x%x  nblk= 0x%x\n",
				bn, lp->dkl_nblk);
			bp->b_flags |= B_ERROR;
			iodone(bp);
			return;
		}
		/* If EOM, set residue but don't report error */
		if (un->un_present == ONLINE  &&  (bn >= lp->dkl_nblk)) {
			EPRINTF1("sdstrategy:  invalid block, %u\n", bn);
			bp->b_resid = bp->b_bcount;
			iodone(bp);
			return;
		}
	}

	if ((bn >= dsi->un_cyl_start)  &&  (bn < dsi->un_cyl_end)  &&
	    ( (int)lp == dsi->un_lp1)) {
		/*DPRINTF("WIN  ");*/
		bp->b_cylin = dsi->un_cylin_last;

	} else {
		/*DPRINTF("LOSE\n");*/
		dsi->un_lp1 = (int)lp;
		cyl = bn / dsi->un_cyl_size;

		dsi->un_cyl_start = cyl * dsi->un_cyl_size;
		dsi->un_cyl_end   = dsi->un_cyl_start + dsi->un_cyl_size;

		cyl += lp->dkl_cylno + dsi->un_g.dkg_bcyl;
		bp->b_cylin = dsi->un_cylin_last = cyl;
	}
	s = splr(pritospl(un->un_mc->mc_intpri));
	dh = &md->md_utab;
	disksort(dh, bp);

	/*
	 * Call unit start routine to queue up device, if it
	 * currently isn't queued.
	 */
	(*un->un_c->c_ss->scs_ustart)(un);
	(*un->un_c->c_ss->scs_start)(un);
	(void) splx(s);
}


/*
 * Set up a transfer for the controller
 */
sdstart(bp, un)
	register struct buf *bp;
	register struct scsi_unit *un;
{
	register struct dk_map *lp;
	register struct scsi_disk *dsi;
	register int nblk;

	/* DPRINTF("sdstart:\n"); */

	dsi = &sdisk[dkunit(bp)];
	lp = &dsi->un_map[LPART(bp->b_dev)];

	/* Process internal I/O requests */
	if (bp == &un->un_sbuf) {
		DPRINTF("sdstart:  internal I/O\n");
		un->un_cmd = un->un_scmd;
		un->un_count = bp->b_bcount;
		un->un_blkno = bp->b_blkno;
		un->un_flags = 0;
		if (dsi->un_options & (SD_DVMA_RD | SD_DVMA_WR))
			un->un_flags = SC_UNF_SPECIAL_DVMA;
		return (1);
	}

	/* Process file system I/O requests */
	if (bp->b_flags & B_READ) {
		DPRINTF("sdstart:  read\n");
		un->un_cmd = SC_READ;
	} else {
#ifdef		DKIOCWCHK
		register u_char i;
		if ((i = sdwchkmap[dkunit(bp)]) &&
				(i & (1<<LPART(bp->b_dev)))) {
			DPRINTF("sdstart:  writev\n");
			un->un_cmd = SC_WRITE_VERIFY;
		} else {
			DPRINTF("sdstart:  write\n");
			un->un_cmd = SC_WRITE;
		}
#else		DKIOCWCHK
		DPRINTF("sdstart:  write\n");
		un->un_cmd = SC_WRITE;
#endif		DKIOCWCHK
	}

	/* Compute absolute block location */
	if ((int)lp == dsi->un_lp2) {
		un->un_blkno = bp->b_blkno + dsi->un_last_cyl_size;
	} else {
		dsi->un_lp2 = (int)lp;
		dsi->un_last_cyl_size = lp->dkl_cylno * dsi->un_cyl_size;
		un->un_blkno = bp->b_blkno + dsi->un_last_cyl_size;
	}

	/* Finish processing read/write request */
	nblk = (bp->b_bcount + (SECSIZE - 1)) >> SECDIV;
	un->un_count = MIN(nblk, (lp->dkl_nblk - bp->b_blkno));
	un->un_flags = SC_UNF_DVMA;
	return (1);
}


/*
 * Make a cdb for disk I/O.
 */
sdmkcdb(un)
	struct scsi_unit *un;
{
	register struct scsi_cdb *cdb;
	register struct scsi_disk *dsi;
	register struct scsi_reassign_blk *srb;
	register struct scsi_defect_list *sd;
	register short cmd;
	/*DPRINTF("sdmkcdb:\n");*/

	dsi = &sdisk[un->un_unit];
	cdb = &un->un_cdb;
	bzero((caddr_t)cdb, CDB_GROUP1);
	cmd = cdb->cmd = un->un_cmd;
	cdb->lun = un->un_lun;

	switch (un->un_cmd) {
	case SC_READ:
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count << SECDIV;
		if (un->un_blkno & CDB6_LIMIT) {
			DPRINTF1("sd%d: sdmkcdb:  eread\n", un-sdunits);
			cdb->cmd = SC_EREAD;
			FORMG1ADDR(cdb, un->un_blkno);
			FORMG1COUNT(cdb, un->un_count);
		} else {
			DPRINTF1("sd%d: sdmkcdb:  read\n", un-sdunits);
			cdb->cmd = SC_READ;
			FORMG0ADDR(cdb, un->un_blkno);
			FORMG0COUNT(cdb, un->un_count);
		}
		break;

	case SC_WRITE:
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count << SECDIV;
		if (un->un_blkno & CDB6_LIMIT) {
			DPRINTF1("sd%d: sdmkcdb:  ewrite\n", un-sdunits);
			cdb->cmd = SC_EWRITE;
			FORMG1ADDR(cdb, un->un_blkno);
			FORMG1COUNT(cdb, un->un_count);
		} else {
			DPRINTF1("sd%d: sdmkcdb:  write\n", un-sdunits);
			cdb->cmd = SC_WRITE;
			FORMG0ADDR(cdb, un->un_blkno);
			FORMG0COUNT(cdb, un->un_count);
		}
		break;

	case SC_WRITE_VERIFY:
		DPRINTF1("sd%d: sdmkcdb:  write verify\n", un-sdunits);
		cdb->cmd = un->un_cmd;
		cdb->lun = un->un_lun;
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		FORMG1ADDR(cdb, un->un_blkno);
		FORMG1COUNT(cdb, un->un_count);
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count << SECDIV;
		break;

	case SC_REQUEST_SENSE:
		EPRINTF1("sd%d: sdmkcdb:  request sense\n", un-sdunits);
		dsi->un_timeout = SHORT_TIMEOUT;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0COUNT(cdb,  sizeof (struct scsi_sense));
		un->un_dma_addr = (int)dsi->un_sense - (int)DVMA;
		un->un_dma_count = sizeof (struct scsi_sense);
		bzero((caddr_t)(dsi->un_sense), sizeof (struct scsi_sense));
		return;

	case SC_SPECIAL_READ:
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count;
		un->un_cmd = SC_READ;
		if (un->un_blkno & CDB6_LIMIT) {
			DPRINTF1("sdmkcdb:  special eread blk= 0x%x\n",
				 un->un_blkno);
			cdb->cmd = SC_EREAD;
			FORMG1ADDR(cdb, un->un_blkno);
			FORMG1COUNT(cdb, un->un_count >> SECDIV);
		} else {
			DPRINTF1("sdmkcdb:  special read blk= 0x%x\n",
				 un->un_blkno);
			cdb->cmd = SC_READ;
			FORMG0ADDR(cdb, un->un_blkno);
			FORMG0COUNT(cdb, un->un_count >> SECDIV);
		}
		break;

	case SC_SPECIAL_WRITE:
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count;
		un->un_cmd = SC_WRITE;
		if (un->un_blkno & CDB6_LIMIT) {
			DPRINTF1("sdmkcdb:  special ewrite blk= 0x%x\n",
				 un->un_blkno);
			cdb->cmd = SC_EWRITE;
			FORMG1ADDR(cdb, un->un_blkno);
			FORMG1COUNT(cdb, un->un_count >> SECDIV);
		} else {
			DPRINTF1("sdmkcdb:  special write blk= 0x%x\n",
				 un->un_blkno);
			cdb->cmd = SC_WRITE;
			FORMG0ADDR(cdb, un->un_blkno);
			FORMG0COUNT(cdb, un->un_count >> SECDIV);
		}
		break;

	case SC_READ_LABEL:
		DPRINTF1("sd%d: sdmkcdb:  read label\n", un-sdunits);
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		un->un_dma_addr = (int)un->un_sbuf.b_un.b_addr - (int)DVMA;
		un->un_dma_count = SECSIZE;
		un->un_cmd = SC_READ;
		if (un->un_blkno & CDB6_LIMIT) {
			DPRINTF1("sd%d: sdmkcdb:  eread label\n", un-sdunits);
			cdb->cmd = SC_EREAD;
			FORMG1ADDR(cdb, un->un_blkno);
			FORMG1COUNT(cdb, un->un_count);
		} else {
			DPRINTF1("sd%d: sdmkcdb:  read label\n", un-sdunits);
			cdb->cmd = SC_READ;
			FORMG0ADDR(cdb, un->un_blkno);
			FORMG0COUNT(cdb, un->un_count);
		}
		break;

	case SC_ZERO_WRITE:
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = (int)dsi->un_sense - (int)DVMA;
		un->un_dma_count = SECSIZE;
		un->un_cmd = SC_WRITE;
		bzero((caddr_t)(dsi->un_sense), SECSIZE);
		if (un->un_blkno & CDB6_LIMIT) {
			EPRINTF1("sd%d: sdmkcdb:  zero ewrite\n", un-sdunits);
			cdb->cmd = SC_EWRITE;
			FORMG1ADDR(cdb, un->un_blkno);
			FORMG1COUNT(cdb, un->un_count);
		} else {
			EPRINTF1("sd%d: sdmkcdb:  zero write\n", un-sdunits);
			cdb->cmd = SC_WRITE;
			FORMG0ADDR(cdb, un->un_blkno);
			FORMG0COUNT(cdb, un->un_count);
		}
		break;

	case SC_INQUIRY:
		EPRINTF1("sd%d: sdmkcdb:  inquiry\n", un-sdunits);
		dsi->un_timeout = SHORT_TIMEOUT;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0COUNT(cdb, sizeof (struct scsi_inquiry_data));
		un->un_dma_addr = (int)dsi->un_sense - (int)DVMA;
		un->un_dma_count = sizeof (struct scsi_inquiry_data);
		bzero((caddr_t)(dsi->un_sense),
			 sizeof (struct scsi_inquiry_data));
		break;

	case SC_REZERO_UNIT:
		EPRINTF1("sd%d: sdmkcdb:  rezero unit\n", un-sdunits);
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_count = un->un_dma_addr = 0;
		return;

	case SC_REASSIGN_BLOCK:
		EPRINTF2("sd%d: sdmkcdb:  reassign blk %u\n", un-sdunits, un->un_blkno);
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = (int)dsi->un_sense - (int)DVMA;
		un->un_dma_count = sizeof (struct scsi_reassign_blk);
		srb = (struct scsi_reassign_blk *)dsi->un_sense;
		srb->reserved = 0;
		srb->length = 4;		/* Only 1 defect */
		srb->defect = un->un_blkno;
		return;

	case SC_FORMAT:
		EPRINTF1("sd%d: sdmkcdb:  format\n", un-sdunits);
		dsi->un_timeout = FORMAT_TIMEOUT;
		un->un_dma_addr = un->un_baddr;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		cdb->fmt_parm_bits = FPB_DATA | FPB_CMPLT | FPB_BFI;
		un->un_dma_count = un->un_count;
		cdb->fmt_interleave = 1;
		break;

	case SC_READ_DEFECT_LIST:
		EPRINTF1("sd%d: sdmkcdb:  read defect list\n", un-sdunits);
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		un->un_dma_count = un->un_count;
		un->un_dma_addr = un->un_baddr;
		FORMG1COUNT(cdb, un->un_count);

		sd = (struct scsi_defect_list *) un->un_sbuf.b_un.b_addr;
		cdb->defect_list_descrip = sd->descriptor;
		break;

	case SC_MODE_SELECT:
		EPRINTF1("sd%d: sdmkcdb:  mode select\n", un-sdunits);
		dsi->un_timeout = SHORT_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		FORMG0COUNT(cdb, un->un_count);
		un->un_dma_count = un->un_count;
		un->un_dma_addr = un->un_baddr;
		if (un->un_blkno & 0x80) {
			EPRINTF("sdmkcdb:  savable mode select\n");
			cdb->tag = 1;		/* Save parameters */
		}
		break;

	case SC_MODE_SENSE:
		EPRINTF1("sd%d: sdmkcdb:  mode sense\n", un-sdunits);
		dsi->un_timeout = SHORT_TIMEOUT;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0COUNT(cdb, un->un_count);
		cdb->g0_addr1  = un->un_blkno;
		un->un_dma_count = un->un_count;
		un->un_dma_addr = un->un_baddr;
		break;

	case SC_TEST_UNIT_READY:
#ifdef		SDDEBUG
		DPRINTF1("sd%d: sdmkcdb:  test unit ready\n", un-sdunits);
		dsi->un_timeout = SHORT_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_options |= SD_NORETRY;
		break;
#endif		SDDEBUG

	case SC_START:
		DPRINTF1("sd%d: sdmkcdb:  start\n", un-sdunits);
		dsi->un_timeout = NORMAL_TIMEOUT;
		FORMG0COUNT(cdb, un->un_count);
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_dma_count = 0;
		dsi->un_options |= SD_NORETRY;
		break;

	case SC_RESERVE:
	case SC_RELEASE:
		EPRINTF1("sd%d: sdmkcdb:  reserve/release\n", un-sdunits);
		dsi->un_timeout = SHORT_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = un->un_dma_count = 0;
		break;

	case SC_TRANSLATE:
		EPRINTF1("sd%d: sdmkcdb:  translate\n", un-sdunits);
		dsi->un_timeout = SHORT_TIMEOUT;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0ADDR(cdb, un->un_blkno);
		FORMG0COUNT(cdb, 0);
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count;
		break;

	default:
		EPRINTF1("sd%d: sdmkcdb:  unknown command\n", un-sdunits);
		dsi->un_timeout = NORMAL_TIMEOUT;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		break;
	}
	dsi->un_last_cmd = cmd;
	return;
}


/*
 * This routine handles controller interrupts.
 * It is always called at disk interrupt priority.
 */
typedef enum sdintr_error_resolution {	
	real_error, 		/* Hard error */
	psuedo_error, 		/* What looked like an error is actually OK */
	more_processing 	/* Recoverable error */
} sdintr_error_resolution;
sdintr_error_resolution sderror(), sd_fix_block(), sdintr_ran_reassign();

sdintr(c, resid, error)
	register struct scsi_ctlr *c;
	int resid;
	int error;
{
	register struct scsi_unit *un = c->c_un;
	register struct scsi_disk *dsi;
	register struct buf *bp;
	register struct mb_device *md;
	int status = 0;
	int error_code = 0;
	u_char severe = DK_NOERROR;

	/* DPRINTF("sdintr:\n"); */
	md = un->un_md;
	bp = md->md_utab.b_forw;
	if (bp == NULL) {
		EPRINTF("sdintr: bp = NULL\n");
		return;
	}

	dsi = &sdisk[SDUNIT(bp->b_dev)];
	dsi->un_timeout = 0;		/* Disable time-outs */
#ifdef	lint
	un = un;
#endif	lint

	if (md->md_dk >= 0)
		dk_busy &= ~(1 << md->md_dk);

	/*
	 * Check for error or special operation states and process.
	 * Otherwise, it was a normal I/O command which was successful.
	 */
	if (dsi->un_openf < OPEN  ||  error  ||  resid ) {

		/*
		 * Special processing for SCSI bus failures.
		 */
		if (error == SE_FATAL) {
			if (dsi->un_openf == OPEN) {
				log(LOG_ERR, "sd%d:  scsi bus failure\n",
					SDNUM(un));
			}
			dsi->un_retries = dsi->un_restores = 0;
			dsi->un_err_severe = DK_FATAL;
			un->un_present = OFFLINE;
			dsi->un_openf = CLOSED;
			bp->b_error = ENXIO;
			bp->b_flags |= B_ERROR;
			goto SDINTR_WRAPUP;
		}

		/*
		 * Opening disk, check if command failed.  If it failed
		 * close device.  Otherwise, it's open.
		 */
		if (dsi->un_openf == OPENING) {
			if (error == SE_NO_ERROR) {
				/*DPRINTF("sdintr:  open ok\n");*/
				goto SDINTR_SUCCESS;
			} else if (error == SE_RETRYABLE) {
				DPRINTF("sdintr:  open failed\n");
				bp->b_error = EIO;
			} else {
				DPRINTF("sdintr:  hardware failure\n");
				bp->b_error = ENXIO;
			}
			un->un_present = OFFLINE;
			bp->b_flags |= B_ERROR;
			goto SDINTR_WRAPUP;
		}

		/*
		 * Rezero for failed command done, retry failed command
		 */
		if ((dsi->un_openf == RETRYING)  &&
		    (un->un_cdb.cmd == SC_REZERO_UNIT)) {

			if (error)
				log(LOG_ERR, "sd%d:  rezero failed\n", SDNUM(un));

			DPRINTF("sdintr:  rezero done\n");
			sdrestore_cmd(dsi, un, &resid);
			sdmkcdb(un);
			(*c->c_ss->scs_cmd)(c, un, 1);
			return;
		}


		/*
		 * Command failed, need to run request sense command.
		 */
		if ((dsi->un_openf == OPEN)  &&  error) {
			sdintr_sense(dsi, un, resid);
			return;
		}

		/*
		 * Request sense command done, restore failed command.
		 */
		if (dsi->un_openf == SENSING) {
			DPRINTF("sdintr:  restoring sense\n");
			sdintr_ran_sense(un, dsi, &resid);
		}

		EPRINTF2("sdintr:  retries= %d  restores= %d  ",
			 dsi->un_retries, dsi->un_restores);
		EPRINTF1("state= %d  ", dsi->un_openf);
		EPRINTF1("total= %d\n", dsi->un_total_errors);
		if ((dsi->un_openf == RETRYING)  &&  (error == 0)) {
			EPRINTF("sdintr:  ok\n\n");
			dsi->un_openf = OPEN;
			dsi->un_retries = dsi->un_restores = 0;
			dsi->un_err_severe = DK_RECOVERED;
			goto SDINTR_SUCCESS;
		}

		/*
		 * Process all other errors here
		 */
		EPRINTF2("sdintr:  processing error,  error= %x  chk= %x",
			error, un->un_scb.chk);
		EPRINTF1("  busy= %x", un->un_scb.busy);
		EPRINTF2("  resid= %d (%d)\n", resid, un->un_dma_count);
		switch (sderror(c, un, dsi, bp, &resid, error)) {
		case real_error:
			/* This error is FATAL ! */
			DPRINTF("sdintr:  real error\n");
			dsi->un_retries = dsi->un_restores = 0;
			bp->b_flags |= B_ERROR;
			goto SDINTR_WRAPUP;

		case psuedo_error:
			/* A psuedo-error:  soft error reported by ctlr */
			DPRINTF("sdintr:  psuedo error\n");
			status = dsi->un_status;
			error_code = dsi->un_err_code;
			severe = dsi->un_err_severe;
			dsi->un_retries = dsi->un_restores = 0;
			goto SDINTR_SUCCESS;

		case more_processing:
			/* real error requiring error recovery */
			DPRINTF("stintr:  more processing\n");
			return;
		}
	}


	/*
	 * Handle successful Transfer.  Also, take care of ACB-4000
	 * seek error problem by doing single sector I/O.
	 */
SDINTR_SUCCESS:
	dsi->un_status = status;
	dsi->un_err_code = error_code;
	dsi->un_err_severe = severe;

	if (dsi->un_sec_left) {
		EPRINTF("sdintr:  single sector writes\n");
		dsi->un_sec_left--;
		un->un_baddr += SECSIZE;
		un->un_blkno++;
		sdmkcdb(un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) != 0)
			log(LOG_ERR, sector_error, SDNUM(un));
	}


	/*
	 * Handle I/O request completion (both sucessful and failed).
	 */
SDINTR_WRAPUP:
	if (bp == &un->un_sbuf  &&
	    ((un->un_flags & SC_UNF_DVMA) == 0)) {
		bp->b_resid = 0;
		if (un->un_flags & SC_UNF_SPECIAL_DVMA)
			mbudone(un->un_md);
		else
			(*c->c_ss->scs_done)(un->un_md);

	} else {
		bp->b_resid = bp->b_bcount - (un->un_count << SECDIV);
		mbudone(un->un_md);
		un->un_flags &= ~SC_UNF_DVMA;
	}
}


/*
 * Disk error decoder/handler.
 */
static sdintr_error_resolution
sderror(c, un, dsi, bp, resid, error)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
	struct buf *bp;
	register int *resid;
	int error;
{
	register struct scsi_ext_sense *ssd =
		 (struct scsi_ext_sense *)dsi->un_sense;
	/* DPRINTF("sderror:\n"); */

	/*
	 * Special processing for driver command timeout errors.
	 */
	if (error == SE_TIMEOUT) {
		EPRINTF("sderror:  command timeout error\n");
		dsi->un_status = SC_TIMEOUT;
		dsi->un_err_code = 0;
		goto SDERROR_RETRY;
	}
	/*
	 * Special processing for reassign block operations
	 */
	if (dsi->un_openf == MAPPING  ||  dsi->un_openf == MAPPING_BAD  ||
	    dsi->un_openf == REWRITING) {
		return (sdintr_ran_reassign(c, un, dsi, bp, resid));
	}

	/*
	 * Check for Adaptec ACB-4000 seek error problem.  If found,
	 * transfer data one sector at a time.
	 */
	if (dsi->un_ctype == CTYPE_ACB4000  &&  un->un_scb.chk  &&
	    ssd->error_code == 15  &&  un->un_count > 1) {
		EPRINTF("sderror:  seek error\n");
		dsi->un_sec_left = un->un_count - 1;
		un->un_count = 1;
		sdmkcdb(un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) != 0) {
			log(LOG_ERR, sector_error, SDNUM(un));
			return (real_error);
		}
		return (more_processing);
	}

	/*
	 * Check for various check condition errors.
	 */
	dsi->un_total_errors++;
	if (un->un_scb.chk) {

		switch (ssd->key) {
		case SC_RECOVERABLE_ERROR:
			dsi->un_err_severe = DK_CORRECTED;
			if (sd_fix_block(c, dsi, un, *resid,
				    SD_RECOVERABLE) == more_processing) {
					return (more_processing);
			}
			if (sd_retry == 0)
				sderrmsg(un, bp, "recoverable", LOG_ERR, NULL);

			if (dsi->un_options & SD_DIAGNOSE)
				return (real_error);

			return (psuedo_error);

		case SC_MEDIUM_ERROR:
			EPRINTF("sderror:  media error\n");
			goto SDERROR_RETRY;

		case SC_HARDWARE_ERROR:
			EPRINTF("sderror:  hardware error\n");
			goto SDERROR_RETRY;

		case SC_UNIT_ATTENTION:
			/*
			 * If fixed, must be power glitch.
			 * If removable, the disk has been changed.
			 */
			if (dsi->un_options & SD_REMOVABLE) {
				EPRINTF("sderror:  media changed\n");
				un->un_present = MEDIA_CHANGED;
				dsi->un_openf = CLOSED;
				dsi->un_err_severe = DK_FATAL;
				bp->b_error = ENXIO;
				return (real_error);
			}
			EPRINTF("sderror:  unit attention error\n");
			goto SDERROR_RETRY;

		case SC_NOT_READY:
			/*
			 * If fixed, must be power glitch.
			 * If removable, the disk has been removed.
			 */
			if (dsi->un_options & SD_REMOVABLE) {
				EPRINTF("sderror:  offline\n");
				un->un_present = MEDIA_CHANGED;
				dsi->un_openf = CLOSED;
				dsi->un_err_severe = DK_FATAL;
				bp->b_error = ENXIO;
				return (real_error);
			}
			EPRINTF("sderror:  not ready\n");
			goto SDERROR_RETRY;

		case SC_ILLEGAL_REQUEST:
			EPRINTF("sderror:  illegal request\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		case SC_VOLUME_OVERFLOW:
			EPRINTF("sderror:  volume overflow\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		case SC_WRITE_PROTECT:
			EPRINTF("sderror:  write protected\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		case SC_BLANK_CHECK:
			EPRINTF("sderror:  blank check\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		default:
			/*
			 * Undecoded sense key.  Try retries and hope
			 * that will fix the problem.  Otherwise, we're
			 * dead.
			 */
			DPRINTF1("sderror: sense key error (0x%x)\n",
				ssd->key);
			SD_PRINT_CMD(un);
			dsi->un_err_severe = DK_FATAL;
			goto SDERROR_RETRY;
		}

	/*
	 * Process reservation error.  Abort operation.
	 */
	} else if (un->un_scb.busy  &&  un->un_scb.is) {
		EPRINTF1("sd%d:  sderror: reservation conflict error\n",
			SDNUM(un));
		dsi->un_status = SC_RESERVATION;
		dsi->un_err_code = 0;
		bp->b_error = EPERM;
		dsi->un_err_severe = DK_FATAL;
		return (real_error);

	/*
	 * Process busy error.  Try retries and hope that
	 * it'll be ready soon.  Note, this test must follow
	 * the reservation conflict error test.
	 */
	} else if (un->un_scb.busy) {
		EPRINTF1("sd%d:  busy\n", SDNUM(un));
		dsi->un_retries = 0;
		if (++dsi->un_restores > sd_max_busy) {
			dsi->un_retries = sd_max_retries;
			dsi->un_restores = sd_max_restores;
		}
		dsi->un_status = SC_BUSY;
		dsi->un_err_code = 0;
		goto SDERROR_RETRY;

	/*
	 * Have left over residue data from last command.
	 * If retries enabled, try a couple of times and see
	 * if it's fixable.  Otherwise, return residue.
	 */
	} else  if (*resid != 0) {
		EPRINTF2("sderror:  residue error %d(%d)\n",
			 *resid, un->un_dma_count);
		dsi->un_err_resid = bp->b_resid = *resid;
		dsi->un_status = SC_RESIDUE;
		dsi->un_err_code = 0;
		SD_PRINT_CMD(un);
		goto SDERROR_RETRY;

	/*
	 * Have an unknown error.  Don't know what went wrong.
	 * Do retries and hope this fixes it...
	 */
	} else {
		EPRINTF("sderror:  unknown error\n");
		SD_PRINT_CMD(un);
		dsi->un_status = SC_FATAL;
		dsi->un_err_code = 0;
		dsi->un_err_severe = DK_FATAL;
		goto SDERROR_RETRY;
	}

	/*
	 * Process command retries and rezeros here.
	 * Note, for on-line formatting, normal error
	 * recovery is inhibited.
	 */
SDERROR_RETRY:
	if (dsi->un_options & SD_NORETRY) {
		EPRINTF("sderror:  error recovery disabled\n");
		/* If no error pending, invent one. */
		if (! dsi->un_status) {
			dsi->un_status = SC_FATAL;
			dsi->un_err_code = 0;
		}
		dsi->un_options &= ~SD_NORETRY;
		sderrmsg(un, bp, "failed, no retries", LOG_ERR, NULL);
		if (dsi->un_status == SC_RESIDUE)
			return (psuedo_error);

		dsi->un_err_severe = DK_FATAL;
		return (real_error);
	}

	/*
	 * Command failed, retry it.
	 */
	if (dsi->un_retries++ < sd_max_retries) {
		if (dsi->un_retries > sd_retry  ||
		    (dsi->un_restores > 0  &&  !un->un_scb.busy)) {
			sderrmsg(un, bp, "retry", LOG_ERR, NULL);
		}
		dsi->un_openf = RETRYING;
		sdmkcdb(un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) != 0) {
			log(LOG_ERR, "sd%d:  retry failed\n", SDNUM(un));
			return (real_error);
		}
		return (more_processing);

	/*
	 * Retries exhausted, try restore
	 */
	} else if (++dsi->un_restores < sd_max_restores) {
		if (dsi->un_restores > sd_restores)
			sderrmsg(un, bp, "restore", LOG_ERR, NULL);

		sdsave_cmd(un, *resid);
		dsi->un_openf = RETRYING;
		dsi->un_retries = 0;
		un->un_cmd = SC_REZERO_UNIT;
		sdmkcdb(un);
		(void) (*c->c_ss->scs_cmd)(c, un, 1);
		return (more_processing);

	/*
	 * Restores and retries exhausted, die!
	 */
	} else {
		/* complete failure */
		sderrmsg(un, bp, "failed", LOG_ERR, LOG_ALERT);
		dsi->un_openf = OPEN;
		dsi->un_retries = 0;
		dsi->un_restores = 0;
		if (ssd->key == SC_MEDIUM_ERROR  &&  (sd_repair & 0x04)  &&
		    sd_fix_block(c, dsi, un, *resid, SD_REMAP)
		      == more_processing) {
				return (more_processing);
		}
		if (dsi->un_status == SC_RESIDUE)
			return (psuedo_error);

		dsi->un_err_severe = DK_FATAL;
		return (real_error);
	}
}


/*
 * Command failed, need to run a request sense command to determine why.
 */
static
sdintr_sense(dsi, un, resid)
	register struct scsi_disk *dsi;
	register struct scsi_unit *un;
	int resid;
{
	/*
	 * Note that s?start will call sdmkcdb, which
	 * will notice that the flag is set and not do
	 * the copy of the cdb, doing a request sense
	 * rather than the normal command.
	 */
	DPRINTF("sdintr:  getting sense\n");
	sdsave_cmd(un, resid);
	dsi->un_openf = SENSING;
	un->un_cmd = SC_REQUEST_SENSE;
	(*un->un_c->c_ss->scs_go)(un->un_md);
}


/*
 * Cleanup after running request sense command to see why the real
 * command failed.
 */
static
sdintr_ran_sense(un, dsi, resid)
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
	int *resid;
{
	register struct scsi_ext_sense *ssd =
			 (struct scsi_ext_sense *)dsi->un_sense;
#ifdef	lint
	resid = 0;
#endif	lint

	/* Special processing for Adaptec ACB-4000 disk controller. */
	if (dsi->un_ctype == CTYPE_ACB4000  ||  ssd->type != 0x70)
		sdintr_adaptec(dsi);
	dsi->un_status = ssd->key;
	dsi->un_err_code = ssd->error_code;

	/*
	 * Check if request sense command failed.  This should never happen!
	 */
	if (un->un_scb.chk) {
		/* If no error already pending, invent one. */
		if (! dsi->un_status) {
			dsi->un_status = SC_FATAL;
			dsi->un_err_code = 0;
		}
		log(LOG_ERR, "sd%d:  request sense failed\n", SDNUM(un));
	}

	/* Log error information. */
	sdrestore_cmd(dsi, un, resid);
	dsi->un_err_resid = *resid;
	dsi->un_openf = OPEN;

	dsi->un_err_blkno = (ssd->info_1 << 24) | (ssd->info_2 << 16) |
			    (ssd->info_3 << 8)  | ssd->info_4;
	if (dsi->un_err_blkno == 0  ||  !(ssd->adr_val)) {
		/* No supplied block number, use original value */
		EPRINTF("sdintr_ran_sense:  synthesizing block number\n");
		dsi->un_err_blkno = un->un_blkno;
	}

#ifdef	SDDEBUG
	/* dump sense info on screen */
	if (sd_debug > 1)
		sd_error_message(un, dsi);
#endif	SDDEBUG
}


/*
 * Cleanup after running request sense command to see why the real
 * command failed.
 */
/*ARGSUSED*/
static sdintr_error_resolution
sd_fix_block(c, dsi, un, resid, err_type)
	register struct scsi_ctlr *c;
	register struct scsi_disk *dsi;
	register struct scsi_unit *un;
	int resid;
	short err_type;
{
	register int i;
	register int block = dsi->un_err_blkno;
	short flag = err_type;

#ifdef	lint
	c = c; dsi = dsi;
#endif	lint

	/*
	 * If last command wasn't a read or write, don't perform
	 * auto-repair function.  Also, if format override in
	 * effect, inhibit auto-repair.
	 */
	if ((un->un_cmd != SC_READ  &&  un->un_cmd != SC_WRITE  &&
	     un->un_cmd != SC_WRITE_VERIFY)  ||
	    dsi->un_options & SD_DIAGNOSE)
			return (psuedo_error);

	if (err_type == SD_RECOVERABLE) {
		if (dsi->un_bad_index == 0)
			goto SD_SAVE;
		/*
		 * Search bad block log for match.  If found,
		 * increment counter.  If reporting theshold passed,
		 * inform user.  If remap threshold passed, reassign
		 * block.  Otherwise, just log it.
		 */
		for (i = 0; i < dsi->un_bad_index; i++) {

			/* Didn't find it, try next entry */
			if (dsi->un_bad[i].block != dsi->un_err_blkno)
				continue;

			/* Found it, warn user of number of retries */
			if (++dsi->un_bad[i].retries >= sd_fails) {
				log(LOG_ERR, "sd%d:  warning, block %u has failed %d times\n",
					SDNUM(un), block,
					dsi->un_bad[i].retries);
			}
			/* Found it, check if we need to rewrite block */
			if (dsi->un_bad[i].retries >= sd_min_fails  &&
			    (sd_repair & 0x01)) {
				log(LOG_ERR, "sd%d:  rewriting block %u\n",
					SDNUM(un), block);
				flag = SD_REWRITE;
			}
			/* Found it, check if we need to reassign block */
			if (dsi->un_bad[i].retries >= sd_max_fails  &&
			    (sd_repair & 0x02)) {
				dsi->un_bad[i].retries = 0;
				flag = SD_REMAP;
			}
			goto SD_FIX_BLK;
		}
		/*
		 * Table overflow, looks serious. Disable auto-repair.
		 * as the disk is failing.
		 */
SD_SAVE:
		if (dsi->un_bad_index == NBAD) {
			log(LOG_ALERT, overflow_error, SDNUM(un));
			sd_repair = 0;
			return (psuedo_error);

		/* Block not in table, add it */
		} else {
			EPRINTF("si_fix_block:  logging marginal block\n");
			i = dsi->un_bad_index++;
			dsi->un_bad[i].block = dsi->un_err_blkno;
			dsi->un_bad[i].retries = 1;
			return (psuedo_error);
		}
	}

SD_FIX_BLK:
	/* If rewrite threshold reached, rewrite block */
	if (flag == SD_REWRITE) {
		EPRINTF("si_fix_block:  rewriting block\n");
		sdsave_cmd(un, resid);
		dsi->un_openf = REWRITING;
		un->un_cmd = SC_WRITE;
		sdmkcdb(un);
		(void) (*c->c_ss->scs_cmd)(c, un, 1);
		return (more_processing);

	/* if hard failure, reassign bad block */
	} else if (flag == SD_REMAP  &&  dsi->un_ctype != CTYPE_ACB4000) {
		sdsave_cmd(un, resid);
		if (err_type == SD_REMAP) {
			dsi->un_openf = MAPPING_BAD;
			log(LOG_ERR, reassign_error, SDNUM(un), "defective", block);
		} else {
			dsi->un_openf = MAPPING;
			log(LOG_ERR, reassign_error, SDNUM(un), "marginal", block);
		}
		un->un_cmd = SC_REASSIGN_BLOCK;
		un->un_scratch_addr = (caddr_t) un->un_blkno;
		un->un_blkno = block;
		sdmkcdb(un);
		(void) (*c->c_ss->scs_cmd)(c, un, 1);
		return (more_processing);
	}
	return (psuedo_error);
}


/*
 * Cleanup after running reassign block command.
 */
/*ARGSUSED*/
static sdintr_error_resolution
sdintr_ran_reassign(c, un, dsi, bp, resid)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
	struct buf *bp;
	int *resid;
{
	DPRINTF("sdintr_ran_reassign:\n");
	if (dsi->un_openf == MAPPING_BAD) {
		/* If reassign fails, get the reason why */
		if (un->un_cmd == SC_REASSIGN_BLOCK  &&  un->un_scb.chk) { 
			EPRINTF("sd_ran_reassign:  reassign failed\n");
			un->un_cmd = SC_REQUEST_SENSE;
			sdmkcdb(un);
			(*un->un_c->c_ss->scs_go)(un->un_md);
			return (more_processing);

		/* After getting reassign status, display it and quit */
		} else if (un->un_cmd == SC_REQUEST_SENSE) {
			EPRINTF("sd_ran_reassign:  request sense\n");
			un->un_cmd = SC_REASSIGN_BLOCK;
			sderrmsg(un, bp, "failed", LOG_ALERT, NULL);

		/* If reassign ok, zero out the block */
		} else if (un->un_cmd == SC_REASSIGN_BLOCK) {
			EPRINTF("sd_ran_reassign:  zeroing block\n");
			sdrestore_cmd(dsi, un, resid);
			sdsave_cmd(un, *resid);
			un->un_cmd = SC_ZERO_WRITE;
			sdmkcdb(un);
			(void) (*c->c_ss->scs_cmd)(c, un, 1);
			return (more_processing);

		/* If zero block failed, notify user. */
		} else if (un->un_cmd == SC_WRITE  &&  un->un_scb.chk) { 
			log(LOG_ERR, clear_msg, SDNUM(un));
		}
		dsi->un_retries = dsi->un_restores = 0;
		sdrestore_cmd(dsi, un, resid);
		dsi->un_openf = OPEN;
		return (real_error);

	} else if (dsi->un_openf == MAPPING) {
		/* If reassign fails, get the reason why. */
		if (un->un_cmd == SC_REASSIGN_BLOCK  &&  un->un_scb.chk) { 
			EPRINTF("sd_ran_reassign:  reassign failed\n");
			un->un_cmd = SC_REQUEST_SENSE;
			sdmkcdb(un);
			(*un->un_c->c_ss->scs_go)(un->un_md);
			return (more_processing);

		/* After getting reassign status, display it. */
		} else if (un->un_cmd == SC_REQUEST_SENSE) {
			EPRINTF("sd_ran_reassign:  request sense\n");
			un->un_cmd = SC_REASSIGN_BLOCK;
			sderrmsg(un, bp, "failed", LOG_ALERT, NULL);
		}
		/* After reassign, write back data. */
		if (un->un_cmd == SC_REASSIGN_BLOCK  &&  un->un_scb.chk == 0) { 
			EPRINTF("sd_ran_reassign:  write back\n");
			sdrestore_cmd(dsi, un, resid);
			sdsave_cmd(un, *resid);
			un->un_blkno = (int) un->un_scratch_addr;
			un->un_cmd = SC_WRITE;
			sdmkcdb(un);
			dsi->un_openf = REWRITING;
			(void) (*c->c_ss->scs_cmd)(c, un, 1);
			return (more_processing);
			/* continued below */
		}

	} else if (dsi->un_openf == REWRITING) {
		/* If write back failed, notify user. */
		if (un->un_cmd == SC_WRITE  &&  un->un_scb.chk) { 
			EPRINTF("sd_ran_reassign:  rewrite failed\n");
			un->un_cmd = SC_REQUEST_SENSE;
			sdmkcdb(un);
			(*un->un_c->c_ss->scs_go)(un->un_md);
			return (more_processing);

		/* After getting rewrite status, display it. */
		} else if (un->un_cmd == SC_REQUEST_SENSE) {
			EPRINTF("sd_ran_reassign:  request sense\n");
			un->un_cmd = SC_WRITE;
			sderrmsg(un, bp, "back failed", LOG_ALERT, NULL);
		}
	}
	sdrestore_cmd(dsi, un, resid);
	dsi->un_openf = OPEN;
	return (psuedo_error);
}


/*
 * Save the current command and state away.
 */
/*ARGSUSED*/
sdsave_cmd(un, resid)
	register struct scsi_unit *un;
	int resid;
{
	un->un_saved_cmd.saved_scb = un->un_scb;
	un->un_saved_cmd.saved_cdb = un->un_cdb;
	un->un_saved_cmd.saved_resid = resid;
	un->un_saved_cmd.saved_dma_addr = un->un_dma_addr;
	un->un_saved_cmd.saved_dma_count = un->un_dma_count;
	un->un_flags |= SC_UNF_GET_SENSE;
}


/*
 * Restore the saved command and state.
 */
/*ARGSUSED*/
sdrestore_cmd(dsi, un, resid_ptr)
	register struct scsi_disk *dsi;
	register struct scsi_unit *un;
	int *resid_ptr;
{
	un->un_scb = un->un_saved_cmd.saved_scb;
	un->un_cdb = un->un_saved_cmd.saved_cdb;
	*resid_ptr = un->un_saved_cmd.saved_resid;
	un->un_dma_addr = un->un_saved_cmd.saved_dma_addr;
	un->un_dma_count = un->un_saved_cmd.saved_dma_count;
	un->un_flags &= ~SC_UNF_GET_SENSE;
	un->un_cmd = dsi->un_last_cmd;
}


/*
 * This routine performs raw read operations.  It is called from the
 * device switch at normal priority.  It uses a per-unit buffer for the
 * operation.
 */
/*ARGSUSED*/
sdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = SDUNIT(dev);

	/*DPRINTF("sdread:\n");*/
	if (unit >= nsdisk) {
		EPRINTF("sdread:  invalid unit\n");
		return (ENXIO);
	}
	/*
	 * The following tests assume a block size which is power of 2.
	 * This allows a bit mask operation to be used instead of a
	 * divide operation thus saving considerable time.
	 */
	if ((uio->uio_fmode & FSETBLK) == 0 &&
	    (uio->uio_offset & (DEV_BSIZE - 1))) {
		EPRINTF1("sdread:  block address not modulo %d\n", DEV_BSIZE);
		return (EINVAL);
	}
	if (uio->uio_iov->iov_len & (DEV_BSIZE - 1)) {
		EPRINTF1("sdread:  block length not modulo %d\n", DEV_BSIZE);
		return (EINVAL);
	}
	return (physio(sdstrategy, (struct buf *)NULL, dev, B_READ, minphys, uio));
}


/*
 * This routine performs raw write operations.  It is called from the
 * device switch at normal priority.  It uses a per-unit buffer for the
 * operation.
 */
sdwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = SDUNIT(dev);

	/*DPRINTF("sdwrite:\n");*/
	if (unit >= nsdisk) {
		EPRINTF("sdwrite:  invalid unit\n");
		return (ENXIO);
	}
	if ((uio->uio_fmode & FSETBLK) == 0 &&
	    (uio->uio_offset & (DEV_BSIZE - 1))) {
		EPRINTF1("sdwrite:  block address not modulo %d\n", DEV_BSIZE);
		return (EINVAL);
	}
	if (uio->uio_iov->iov_len & (DEV_BSIZE - 1)) {
		EPRINTF1("sdwrite:  block length not modulo %d\n", DEV_BSIZE);
		return (EINVAL);
	}
	return (physio(sdstrategy, (struct buf *)NULL, dev, B_WRITE, minphys, uio));
}


/*
 * This routine implements the ioctl calls.  It is called
 * from the device switch at normal priority.
 */
sdioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	register caddr_t data;
	int flag;
{
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
	register struct dk_map *lp;
	register struct dk_info *info;
	register struct dk_conf *conf;
	register struct dk_diag *diag;
	register struct dk_loghdr  *loghdr;
	register struct dk_log  *elog;
	register int unit = SDUNIT(dev);
	int i, j, s;

#ifdef 	lint
	conf = (struct dk_conf *) 0;
	lp = (struct dk_map *) 0;
	info = (struct dk_info *) 0;
	loghdr = (struct dk_loghdr  *) 0;
	elog = (struct dk_log  *) 0;
	conf = conf; lp = lp; info = info; flag = flag; j = 0; j = j;
	elog = elog; loghdr = loghdr;
#endif	lint

	/* DPRINTF("sdioctl:\n"); */
	if (unit >= nsdisk) {
		EPRINTF("sdioctl:  invalid unit\n");
		return (ENXIO);
	}
	un = &sdunits[unit];
	dsi = &sdisk[unit];
	lp = &dsi->un_map[LPART(dev)];
	switch (cmd) {

#ifdef	DKIOCGLOG
	/*
	 * Return error log info.
	 */
	case DKIOCGLOG:
		EPRINTF("sdioctl:  get log info\n");
		loghdr = (struct dk_loghdr *)data;
		loghdr->dkl_entries = j = dsi->un_bad_index;

		/* If no space or no bad blocks, we're done */
		if (loghdr->dkl_max_size == 0  ||  dsi->un_bad_index == 0)
			return (0);
		/*
 		 * Copy error info across.
		 */
		if (loghdr->dkl_entries > loghdr->dkl_max_size)
			j = loghdr->dkl_max_size;
		elog = (struct dk_log *)loghdr->dkl_logbfr;
		for (i = 0; i < j; i++) {
			elog->block = dsi->un_bad[i].block;
			elog->count = dsi->un_bad[i].retries;
			elog->type = DKL_SOFT;

			/* FIXME: need real error codes */
			elog->err1 = elog->err2 = 0;
			elog++;
		}
		return (0);
#endif	DKIOCGLOG

#ifdef	DKIOCWCHK
	/*
	 * Enable/Disable write verification.
	 */
	case DKIOCWCHK:
		/*
		 * The only thing that I am certain this works
		 * on is the ACB4000.  MD-21's can't do this.
		 * Embedded SCSI disks need to be checked.
		 *
		 */
		if (dsi->un_ctype != CTYPE_ACB4000)
			return (ENOTTY);

		if(!lp || lp->dkl_nblk == 0)
			return (ENXIO);
		s = splr(pritospl(un->un_mc->mc_intpri));

		if ((sdwchkmap[unit] & (1<<LPART(dev))) != 0)
			i = 1;
		else
			i = 0;

		if (*(int *)data)
			sdwchkmap[unit] |= (1<<LPART(dev));
		else
			sdwchkmap[unit] &= ~(1<<LPART(dev));
		(void) splr(s);
		DPRINTF1("sdioctl: write check %s\n",
			 ((*(int *) data)?"enabled": "disabled"));
		*(int *) data = i;
		return (0);
#endif		DKIOCWCHK

	/*
	 * Return info concerning the controller.
	 */
	case DKIOCINFO:
		DPRINTF("sdioctl:  get info\n");
		info = (struct dk_info *)data;
		info->dki_ctlr = getdevaddr(un->un_mc->mc_addr);
		info->dki_unit = un->un_md->md_slave;

		switch (dsi->un_ctype) {
		case CTYPE_MD21:
			info->dki_ctype = DKC_MD21;
			break;
		case CTYPE_ACB4000:
			info->dki_ctype = DKC_ACB4000;
			break;
		default:
			info->dki_ctype = DKC_MD21;
			break;
		}
		info->dki_flags = DKI_FMTVOL;
		return (0);

	/*
	 * Return the geometry of the specified unit.
	 */
	case DKIOCGGEOM:
		DPRINTF("sdioctl:  get geometry\n");
		*(struct dk_geom *)data = dsi->un_g;
		return (0);

	/*
	 * Set the geometry of the specified unit.
	 */
	case DKIOCSGEOM:
		EPRINTF("sdioctl:  set geometry\n");
		dsi->un_g = *(struct dk_geom *)data;
		return (0);

	/*
	 * Return the map for the specified logical partition.
	 * This has been made obsolete by the get all partitions
	 * command.
	 */
	case DKIOCGPART:
		DPRINTF("sdioctl:  get partitions\n");
		*(struct dk_map *)data = *lp;
		return (0);

	/*
	 * Set the map for the specified logical partition.
	 * This has been made obsolete by the set all partitions
	 * command.  We raise the priority just to make sure
	 * an interrupt doesn't come in while the map is
	 * half updated.
	 */
	case DKIOCSPART:
		EPRINTF("sdioctl:  set partitions\n");
		*lp = *(struct dk_map *)data;
		return (0);

	/*
	 * Return configuration info
	 */
	case DKIOCGCONF:
#ifdef  	SD_FORMAT
		DPRINTF("sdioctl:  get configuration info\n");
		conf = (struct dk_conf *)data;
		switch (dsi->un_ctype) {
		case CTYPE_MD21:
			conf->dkc_ctype = DKC_MD21;
			break;
		case CTYPE_ACB4000:
			conf->dkc_ctype = DKC_ACB4000;
			break;
		default:
			conf->dkc_ctype = DKC_MD21;
			break;
		}
		(void) strncpy(conf->dkc_dname, "sd", DK_DEVLEN);
		conf->dkc_flags = DKI_FMTVOL;
		conf->dkc_cnum = un->un_mc->mc_ctlr;
		conf->dkc_addr = getdevaddr(un->un_mc->mc_addr);
		conf->dkc_space = un->un_mc->mc_space;
		conf->dkc_prio = un->un_mc->mc_intpri;
		if (un->un_mc->mc_intr)
			conf->dkc_vec = un->un_mc->mc_intr->v_vec;
		else
			conf->dkc_vec = 0;
		(void) strncpy(conf->dkc_cname, un->un_c->c_name, DK_DEVLEN);
		conf->dkc_unit = un->un_md->md_unit;
		conf->dkc_slave = un->un_md->md_slave;
#endif  	SD_FORMAT
		return (0);

	/*
	 * Return the map for all logical partitions.
	 */
	case DKIOCGAPART:
		DPRINTF("sdioctl:  get all logical partitions\n");
		for (i = 0; i < NLPART; i++)
			((struct dk_map *)data)[i] = dsi->un_map[i];
		return (0);

	/*
	 * Set the map for all logical partitions.  We raise
	 * the priority just to make sure an interrupt doesn't
	 * come in while the map is half updated.
	 */
	case DKIOCSAPART:
		EPRINTF("sdioctl:  set all logical partitions\n");
		s = splr(pritospl(un->un_mc->mc_intpri));
		for (i = 0; i < NLPART; i++)
			dsi->un_map[i] = ((struct dk_map *)data)[i];
		(void) splx(s);
		return (0);

	/*
	 * Get error status from last command.
	 */
	case DKIOCGDIAG:
		EPRINTF("sdioctl:  get error status\n");
		diag = (struct dk_diag *) data;
		diag->dkd_errcmd  = dsi->un_last_cmd;
		diag->dkd_errsect = dsi->un_err_blkno;
		diag->dkd_errno = dsi->un_status;
		/* diag->dkd_errno = dsi->un_err_code; */
#ifdef		SD_FORMAT
		diag->dkd_severe = dsi->un_err_severe;
#endif		SD_FORMAT

		dsi->un_last_cmd   = 0;	/* Reset */
		dsi->un_err_blkno  = 0;
		dsi->un_err_code   = 0;
		dsi->un_err_severe = 0;
		return (0);

	/*
	 * Run a generic command.
	 */
	case DKIOCSCMD:
#ifdef		SD_FORMAT
		/* DPRINTF("sdioctl:  run special command\n"); */
		return (sdioctl_cmd(dev, un, dsi, data));
#endif		SD_FORMAT

	/*
	 * Handle unknown ioctls here.
	 */
	default:
		EPRINTF("sdioctl:  unknown ioctl\n");
		return (ENOTTY);
	}
}


/*
 * Run a command for sdioctl.
 */
#ifdef  SD_FORMAT
static
sdioctl_cmd(dev, un, dsi, data)
	dev_t dev;
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
	caddr_t data;
{
	register struct dk_cmd *com;
	int s, err;
	int options = SD_IOCTL;

	/* DPRINTF("sdioctl_cmd:\n"); */

	com = (struct dk_cmd *)data;
#ifdef  SDDEBUG
	if (sd_debug > 1) {
		printf("sdioctl_cmd:  cmd= %x  blk= %x  cnt= %x\n",
				com->dkc_cmd, com->dkc_blkno, com->dkc_secnt);
		printf("              buf addr= %x  buflen= 0x%x\n",
				com->dkc_bufaddr, com->dkc_buflen);
	}
#endif  SDDEBUG
	/*
	 * Set options for no driver messages, no retries, and
	 * no auto-repair.
	 */
	if (com->dkc_flags & DK_SILENT) {
		options |= SD_SILENT;
#ifdef		SDDEBUG
		if (sd_debug > 0)
			options &= ~SD_SILENT;
#endif		SDDEBUG
	}
	if (com->dkc_flags & DK_ISOLATE)
		options |= SD_NORETRY;

	if (com->dkc_flags & DK_DIAGNOSE)
		options |= SD_DIAGNOSE;

	/*
	 * Process the special ioctl command.
	 */
	switch (com->dkc_cmd) {
	case SC_READ:
		DPRINTF("sdioctl_cmd:  read\n");
		options |= SD_DVMA_RD;
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sdcmd(dev, SC_SPECIAL_READ, (int)com->dkc_blkno,
			    (int)com->dkc_buflen,
			    (caddr_t)com->dkc_bufaddr, options);
		(void) splx(s);
		break;

	case SC_WRITE:
		DPRINTF("sdioctl_cmd:  write\n");
		options |= SD_DVMA_WR;
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sdcmd(dev, SC_SPECIAL_WRITE, (int)com->dkc_blkno,
			    (int)com->dkc_buflen,
			    (caddr_t)com->dkc_bufaddr, options);
		(void) splx(s);
		break;

	case SC_MODE_SELECT:
		EPRINTF("sdioctl_cmd:  mode select\n");
		options |= SD_DVMA_WR;
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sdcmd(dev, SC_MODE_SELECT, (int)com->dkc_blkno,
			    (int)com->dkc_buflen, (caddr_t)com->dkc_bufaddr,
			    options);
		(void) splx(s);
#ifdef		SDDEBUG
		if (sd_debug > 2) {
			printf("  bufaddr =");
			sd_print_buffer((u_char *)com->dkc_bufaddr,
					(int)com->dkc_buflen);
		}
#endif		SDDEBUG
		break;

	case SC_MODE_SENSE:
		EPRINTF("sdioctl_cmd:  mode sense\n");
		options |= SD_DVMA_RD;
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sdcmd(dev, SC_MODE_SENSE, (int)com->dkc_blkno,
			    (int)com->dkc_buflen, (caddr_t)com->dkc_bufaddr,
			    options);
		(void) splx(s);
#ifdef		SDDEBUG
		if (sd_debug > 2) {
			printf("  bufaddr =");
			sd_print_buffer((u_char *)com->dkc_bufaddr,
					(int)com->dkc_buflen);
		}
#endif		SDDEBUG
		break;

	case SC_FORMAT:
		EPRINTF("sdioctl_cmd:  format\n");
		options |= SD_DVMA_WR;
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sdcmd(dev, SC_FORMAT, (int)com->dkc_blkno,
			    (int)com->dkc_buflen, (caddr_t)com->dkc_bufaddr,
			    options);
		(void) splx(s);
		break;

	case SC_READ_DEFECT_LIST:
		EPRINTF("sdioctl_cmd:  read defect list\n");
		options |= SD_DVMA_RD;
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sdcmd(dev, SC_READ_DEFECT_LIST, 0,
			    (int)com->dkc_buflen, (caddr_t)com->dkc_bufaddr,
			    options);
		(void) splx(s);
		break;

	case SC_REASSIGN_BLOCK:
		EPRINTF("sdioctl_cmd:  reassign block\n");
		options |= SD_NORETRY;
		if (dsi->un_ctype == CTYPE_ACB4000) {
			EPRINTF("sdioctl_cmd:  cannot reassign block\n");
			return (EIO);
		}
		err = sdcmd(dev, (int)com->dkc_cmd, (int)com->dkc_blkno,
			    (int)0, (caddr_t)0, options);
		break;

	case SC_INQUIRY:
		EPRINTF("sdioctl_cmd:  inquiry\n");
		options |= SD_DVMA_RD;
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sdcmd(dev, SC_INQUIRY, (int)com->dkc_blkno,
			    (int)com->dkc_buflen, (caddr_t)com->dkc_bufaddr,
			    options);
		(void) splx(s);
		bcopy((caddr_t)dsi->un_sense,(caddr_t)com->dkc_bufaddr,
		      sizeof (struct scsi_inquiry_data));
#ifdef		SDDEBUG
		if (sd_debug > 2) {
			printf("  bufaddr =");
			sd_print_buffer((u_char *)com->dkc_bufaddr,
					(int)com->dkc_buflen);
		}
#endif		SDDEBUG
		break;

	case SC_RESERVE:
	case SC_RELEASE:
		EPRINTF("sdioctl_cmd:  reserve/release\n");
		options |= SD_NORETRY;
		if (dsi->un_ctype == CTYPE_ACB4000)
			return (0);

		err = sdcmd(dev, (int)com->dkc_cmd, (int)com->dkc_blkno,
			    (int)0, (caddr_t)0, options);
		break;

	case SC_TRANSLATE:
		EPRINTF("sdioctl_cmd:  translate\n");
		options |= SD_DVMA_RD;
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sdcmd(dev, SC_TRANSLATE, (int)com->dkc_blkno,
			(int)com->dkc_buflen,
			(caddr_t)com->dkc_bufaddr, options);
		(void) splx(s);
		break;

	default:
		EPRINTF1("sdioctl_cmd:  unknown command %x\n", com->dkc_cmd);
		return (EINVAL);
		/* break; */
	}
	if (err) {
		EPRINTF("sdioctl_cmd:  ioctl cmd failed\n");
		return (EIO);
	}
	return (0);
}
#endif  SD_FORMAT


 /*
  * This routine dumps memory to the disk.  It assumes that the memory has
  * already been mapped into mainbus space.  It is called at disk interrupt
  * priority when the system is in trouble.
  */
sddump(dev, addr, blkno, nblk)
	register dev_t dev;
	register caddr_t addr;
	/* register daddr_t blkno, nblk; */
	int blkno, nblk;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	static int first_time = 1;
	register struct scsi_disk *dsi;

#ifdef 	lint
	first_time = 0;
#endif	lint

	un = &sdunits[SDUNIT(dev)];
	dsi = &sdisk[SDUNIT(dev)];
	lp = &dsi->un_map[LPART(dev)];
	if (blkno >= lp->dkl_nblk  ||  (blkno + nblk) > lp->dkl_nblk) {
		EPRINTF("sddump:  no place to dump\n");
		return (EINVAL);
	}

	blkno += lp->dkl_cylno * dsi->un_cyl_size;
	if (first_time) {
		/*
		 * After scsi bus reset, it is necessary to
		 * handle a unit attention error.  If the device
		 * fails after that, we assume it's dead and
		 * abort dump.
		 */
		(*un->un_c->c_ss->scs_reset)(un->un_c, 0);
		if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0)  &&
		    simple(un, SC_TEST_UNIT_READY, 0, 0, 0)) {
			EPRINTF("sddump:  dump device offline\n");
			return (ENXIO);
		}
		first_time = 0;
	}
	/*
	 * If write fails, try again.  After two failures,
	 * report failure which kills dump.
	 */
	addr = addr - (int)DVMA;
	if (simple(un, SC_WRITE, (int)addr, blkno, nblk)  &&
	    simple(un, SC_WRITE, (int)addr, blkno, nblk)) {
		EPRINTF("sddump:  write failed\n");
		return (EIO);
	}
	return (0);
}


static u_char sd_adaptec_keys[] = {
	0, 4, 4, 4,  2, 4, 4, 4,	/* 0x00-0x07 */
	4, 4, 4, 4,  4, 4, 4, 4,	/* 0x08-0x0f */
	3, 3, 3, 3,  3, 3, 3, 1,	/* 0x10-0x17 */
	1, 1, 5, 5,  1, 1, 1, 1,	/* 0x18-0x1f */
	5, 5, 5, 5,  5, 5, 5, 5,	/* 0x20-0x27 */
	6, 6, 6, 6,  6, 6, 6, 6,	/* 0x28-0x30 */
};
#define MAX_ADAPTEC_KEYS \
	(sizeof(sd_adaptec_keys))


static char *sd_adaptec_error_str[] = {
	"no sense",			/* 0x00 */
	"no index",			/* 0x01 */
	"no seek complete",		/* 0x02 */
	"write fault",			/* 0x03 */
	"drive not ready",		/* 0x04 */
	"drive not selected",		/* 0x05 */
	"no track 00",			/* 0x06 */
	"multiple drives selected",	/* 0x07 */
	"no address",			/* 0x08 */
	"no media loaded",		/* 0x09 */
	"end of media",			/* 0x0a */
	"",				/* 0x0b */
	"",				/* 0x0c */
	"",				/* 0x0d */
	"",				/* 0x0e */
	"",				/* 0x0f */
	"I.D. CRC error",		/* 0x10 */
	"hard data error",		/* 0x11 */
	"no I.D. address mark",		/* 0x12 */
	"no data address mark",		/* 0x13 */
	"record not found",		/* 0x14 */
	"seek error",			/* 0x15 */
	"DMA timeout error",		/* 0x16 */
	"write protected",		/* 0x17 */
	"correctable data error",	/* 0x18 */
	"bad block",			/* 0x19 */
	"interleave error",		/* 0x1a */
	"data transfer failed",		/* 0x1b */
	"bad format",			/* 0x1c */
	"self test failed",		/* 0x1d */
	"defective track",		/* 0x1e */
	"",				/* 0x1f */
	"invalid command",		/* 0x20 */
	"illegal block address",	/* 0x21 */
	"aborted",			/* 0x22 */
	"volume overflow",		/* 0x23 */
	"bad argument",			/* 0x24 */
	0
};
#define MAX_ADAPTEC_ERROR_STR \
	(sizeof(sd_adaptec_error_str)/sizeof(sd_adaptec_error_str[0]))


static char *sd_emulex_error_str[] = {
	"",				/* 0x00 */
	"no index",			/* 0x01 */
	"seek incomplete",		/* 0x02 */
	"write fault",			/* 0x03 */
	"drive not ready",		/* 0x04 */
	"drive not selected",		/* 0x05 */
	"no track 0",			/* 0x06 */
	"multiple drives selected",	/* 0x07 */
	"lun failure",			/* 0x08 */
	"servo error",			/* 0x09 */
	"",				/* 0x0a */
	"",				/* 0x0b */
	"",				/* 0x0c */
	"",				/* 0x0d */
	"",				/* 0x0e */
	"",				/* 0x0f */
	"ID error",			/* 0x10 */
	"hard data error",		/* 0x11 */
	"no addr mark",			/* 0x12 */
	"no data field addr mark",	/* 0x13 */
	"block not found",		/* 0x14 */
	"seek error",			/* 0x15 */
	"dma timeout",			/* 0x16 */
	"recoverable error",		/* 0x17 */
	"soft data error",		/* 0x18 */
	"bad block",			/* 0x19 */
	"parameter overrun",		/* 0x1a */
	"synchronous xfer error",	/* 0x1b */
	"no primary defect list",	/* 0x1c */
	"compare error",		/* 0x1d */
	"recoverable error",		/* 0x1e */
	"",				/* 0x1f */
	"invalid command",		/* 0x20 */
	"invalid block",		/* 0x21 */
	"illegal command",		/* 0x22 */
	"",				/* 0x23 */
	"invalid cdb",			/* 0x24 */
	"invalid lun",			/* 0x25 */
	"invalid param list",		/* 0x26 */
	"write protected",		/* 0x27 */
	"media changed",		/* 0x28 */
	"reset",			/* 0x29 */
	"mode select changed",		/* 0x2a */
	0
};
#define MAX_EMULEX_ERROR_STR \
	(sizeof(sd_emulex_error_str)/sizeof(sd_emulex_error_str[0]))


static char *sd_key_error_str[] = SENSE_KEY_INFO;
#define MAX_KEY_ERROR_STR \
	(sizeof(sd_key_error_str)/sizeof(sd_key_error_str[0]))

/* 
 * scsi command decode table.
 */
char *sd_cmds[] = {
	"\010read",			/* 0x08 */
	"\012write",			/* 0x0a */
	"\003request sense",		/* 0x03 */
	"\001rezero",			/* 0x01 */
	"\000test unit ready",		/* 0x00 */
	"\004format",			/* 0x04 */
	"\007reassign",			/* 0x07 */
	"\022inquiry",			/* 0x12 */
	"\032mode sense",		/* 0x1a */
	"\025mode select",		/* 0x15 */
	"\067read defect data",		/* 0x37 */
	"\026reserve",			/* 0x16 */
	"\027release",			/* 0x17 */
	"\033start/stop",		/* 0x1b */
	"\013seek",			/* 0x0b */
	"\036door lock",		/* 0x1e */
	"\200read",			/* 0x80 */
	"\201read",			/* 0x81 */
	"\202write",			/* 0x82 */
	"\202zero block",		/* 0x83 */
	0				/* terminates table */
};



/*
 * Translate Adaptec non-extended sense status in to
 * extended sense format.  In other words, generate
 * sense key.
 */
static
sdintr_adaptec(dsi)
	register struct scsi_disk *dsi;
{
	register struct scsi_sense *s = (struct scsi_sense *)dsi->un_sense;
	register struct scsi_ext_sense *ssd;
#ifdef	lint
	s = 0;
#endif	lint

	EPRINTF("sdintr_adaptec:\n");
	/* Reposition failed block number for extended sense. */
	ssd = (struct scsi_ext_sense *)dsi->un_sense;
	ssd->info_1 = 0;
	ssd->info_2 = s->high_addr;
	ssd->info_3 = s->mid_addr;
	ssd->info_4 = s->low_addr;

	/* Reposition error code for extended sense. */
	ssd->error_code = s->code;

	/* Synthesize sense key for extended sense. */
	if (s->code < MAX_ADAPTEC_KEYS)
		ssd->key = sd_adaptec_keys[s->code];
}


/*
 * Return the text string associated with the sense key value.
 */
static char *
sd_print_key(key_code)
	register u_char key_code;
{
	static char *unknown_key = "unknown key";
	if ((key_code > MAX_KEY_ERROR_STR -1)  ||
	     sd_key_error_str[key_code] == NULL) {

		return (unknown_key);
	}
	return (sd_key_error_str[key_code]);
}


/*
 * Return the text string associated with the secondary
 * error code, if availiable.
 */
static char *
sd_print_error(dsi, error_code)
	register struct scsi_disk *dsi;
	register u_char error_code;
{
	static char *unknown_error = " ";
	switch (dsi->un_ctype) {
	case CTYPE_MD21:
	case CTYPE_CCS:
		if ((MAX_EMULEX_ERROR_STR > error_code)  &&
		    sd_emulex_error_str[error_code] != NULL)
			return (sd_emulex_error_str[error_code]);
		break;

	case CTYPE_ACB4000:
		if (MAX_ADAPTEC_ERROR_STR > error_code  &&
		    sd_adaptec_error_str[error_code] != NULL)
			return (sd_adaptec_error_str[error_code]);
		break;
	}
	return (unknown_error);
}


/*
 * Print the sense key and secondary error codes
 * and dump out the sense bytes.
 */
#ifdef	SDDEBUG
static
sd_error_message(un, dsi)
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
{
	register struct scsi_ext_sense *ssd;
	register u_char   *cp = (u_char *)dsi->un_sense;
	register int i;

	/* If error messages are being suppressed, exit. */
	if (dsi->un_options & SD_SILENT)
		return;

	if (dsi->un_err_code) {
		printf(sense_msg2, un-sdunits,
			 dsi->un_status, sd_print_key(dsi->un_status),
		   	 dsi->un_err_code,
			 sd_print_error(dsi, dsi->un_err_code));
	} else {
		printf(sense_msg1, un-sdunits,
			dsi->un_status, sd_print_key(dsi->un_status));
	}

	printf("           sense =");
	for (i = 0; i < sizeof (struct scsi_ext_sense); i++)
		printf(" %x", *cp++);
	printf("\n");
}


static
sd_print_cmd(un)
	register struct scsi_unit *un;
{
	register int x;
	register u_char *y = (u_char *)&(un->un_cdb);
#ifdef 	lint
	un = un; x = 0; y = NULL;
#endif	lint

	printf("sd%d:  failed cmd =", SDUNIT(un->un_unit));
	for (x = 0; x < CDB_GROUP1; x++)
		printf(" %x", *y++);
	printf("\n");
}
#endif	SDDEBUG


static
sderrmsg(un, bp, action, log_priority, log_error)
	struct scsi_unit *un;
	struct buf *bp;
	char *action;
	int log_priority, log_error;
{
	register struct dk_map *lp;
	register struct scsi_disk *dsi;
	char partition = NULL;
	char **cmdtbl = sd_cmds;
	char *cmdname = "<unknown cmd>";
	u_int blkno = 0;

	dsi = &sdisk[dkunit(bp)];
#ifdef	lint
	log_priority = log_priority;
#endif	lint

	/* If error messages are being suppressed, exit. */
	if (dsi->un_options & SD_SILENT)
		return;

	/*
	 * If normal I/O, get relative block and partition info.
	 * Note, if error not a drive error (check condition),
	 * then the block number is invalid.
	 */
	if ((dsi->un_options & SD_IOCTL) == 0)  {
		partition = LPART(bp->b_dev) + 'a';
		lp = &dsi->un_map[LPART(bp->b_dev)];
		blkno = dsi->un_err_blkno;
		blkno -= lp->dkl_cylno * dsi->un_cyl_size;
	}

	/* Decode command name */
	while (*cmdtbl != (char *) NULL) {
		if ((u_char)un->un_cmd == (u_char) *cmdtbl[0]) {
			cmdname = (char *)((int)(*cmdtbl)+1);
			break;
		}
		cmdtbl++;
	}

	/* Report command and location of failure. */
	if (un->un_scb.chk  &&  dsi->un_err_blkno != 0) {
		log(log_priority, disk_error2, SDNUM(un), partition,
			 cmdname, action, dsi->un_err_blkno, blkno);
	} else {
		log(log_priority, disk_error1, SDNUM(un), partition,
			 cmdname, action);
	}

	/* Report error code (e.g. sense key and error code). */
	if (un->un_scb.chk  &&  dsi->un_err_code) {
		log(log_priority, sense_msg2, un-sdunits, partition,
			 dsi->un_status, sd_print_key(dsi->un_status),
		   	 dsi->un_err_code,
			 sd_print_error(dsi, dsi->un_err_code));
	} else if (un->un_scb.chk  &&  dsi->un_err_code == 0) {
		log(log_priority, sense_msg1, un-sdunits, partition,
			dsi->un_status, sd_print_key(dsi->un_status));
	}

	/* Special hard error reporting. */
	if (log_error  &&  un->un_scb.chk  &&  dsi->un_err_blkno != 0) {
		log(log_error, hard_error, SDNUM(un), partition,
			 dsi->un_err_blkno, blkno);
	}
}
#endif	NSD > 0
