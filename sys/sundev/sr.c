#ident  "@(#)sr.c 1.1 92/07/30 Copyr 1990 Sun Micro"
#include "sr.h"
#if NSR > 0
/************************************************************************
*************************************************************************
**									*
**									*
**		SCSI CDROM driver (for CDROM, read-only)		*
**			for SunOs v4.0, 4.0.1, 4.0.3 & 4.1		*
**									*
**		Copyright (c) 1989 by Sun Microsystem, Inc.		*
**									*
*************************************************************************
*************************************************************************
*/


#define	SRDEBUG		/* Allow compiling of debug code */
#define	REL4		/* enable release 4.00 mods */


/*
** SCSI driver for SCSI disks.
*/
#ifndef REL4
#include "sr.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/dkbad.h"
#include "../h/fcntl.h"
#include "../h/file.h"
#include "../h/proc.h"

#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"

#include "../sun/dklabel.h"
#include "../sun/dkio.h"

#include "../sundev/mbvar.h"
#include "../sundev/screg.h"
#include "../sundev/sireg.h"
#include "../sundev/scsi.h"
#include "../sundev/srreg.h"

#else REL4
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
#include <sys/kmem_alloc.h>

#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/mbvar.h>
#include <sundev/screg.h>
#include <sundev/sireg.h>
#include <sundev/scsi.h>
#include <sundev/srreg.h>
#endif REL4


#define	MAX_CDROM		8
#define	SR_TIMEOUT		/* define for scsi recovery */
#define	MAX_RETRIES		4	/* retry limit */
#define	MAX_RESTORES		4	/* rezero unit limit (after retries) */
#define	MAX_FAILS		20	/* soft errors before reassign */
#define	MAX_LABEL_RETRIES	2
#define	MAX_LABEL_RESTORES	1
#define	MAX_BUSY		10
#define	EL_RETRY		2	/* Error msg threshold, retries */
#define	EL_REST			0	/* Error msg threshold, restores */
#define	EL_FAILS		10	/* Error msg threshold, soft errors */
#define	CD_SIZE			0x400000 /* default dev size = 2**21==2E6 */
					/* typical size = 560 Mb =~ 1E6 blks */


#define	LPART(dev)		(dev & (NLPART - 1))
#define	SRUNIT(dev)		((dev >> 3) & (NUNIT - 1))
#define	SRNUM(un)		(un - srunits)
#define	SRIOPBALLOC(size)	((caddr_t)rmalloc(iopbmap, (long)(size + 4)))
#define	SRIOPBFREE(ptr, size)	(rmfree(iopbmap, (long)(size +4), (u_long)ptr))


#ifdef SRDEBUG
short sr_debug = 0;		/*
				** 0 = normal operation
				** 1 = extended error info only
				** 2 = debug and error info
				** 3 = all status info
				*/
/* Handy debugging 0, 1, and 2 argument printfs */
#define	SR_PRINT_CMD(un) \
	if (sr_debug > 1) sr_print_cmd(un)
#define	DPRINTF(str) \
	if (sr_debug > 1) printf(str)
#define	DPRINTF1(str, arg1) \
	if (sr_debug > 1) printf(str, arg1)
#define	DPRINTF2(str, arg1, arg2) \
	if (sr_debug > 1) printf(str, arg1, arg2)

/* Handy extended error reporting 0, 1, and 2 argument printfs */
#define	EPRINTF(str) \
	if (sr_debug) printf(str)
#define	EPRINTF1(str, arg1) \
	if (sr_debug) printf(str, arg1)
#define	EPRINTF2(str, arg1, arg2) \
	if (sr_debug) printf(str, arg1, arg2)
#define	DEBUG_DELAY(cnt) \
	if (sr_debug)  DELAY(cnt)

#else SRDEBUG
#define	SR_PRINT_CMD(un)
#define	DPRINTF(str)
#define	DPRINTF1(str, arg2)
#define	DPRINTF2(str, arg1, arg2)
#define	EPRINTF(str)
#define	EPRINTF1(str, arg2)
#define	EPRINTF2(str, arg1, arg2)
#define	DEBUG_DELAY(cnt)
#endif SRDEBUG

extern char *strncpy();

extern struct scsi_unit srunits[];
extern struct scsi_unit_subr scsi_unit_subr[];
extern struct scsi_disk srdisk[];
extern int ncdrom;

short sr_max_retries  =	 MAX_RETRIES;
short sr_max_restores = MAX_RESTORES;
short sr_max_fails    = MAX_FAILS;
short sr_restores = EL_REST;
short sr_fails	  = EL_FAILS;
short disconenbl = 1;	/* allow disconnect / re-connect on cdrom */
			/* set to 0 to disable disconnect - reconnect */

/*
** flag locking and unlocking purposes.
** to make sure that only the very last close
** unlocks the drive
*/
struct cdrom_open_flag {
	u_char	cdopen_flag0;
	u_char	cdopen_flag1;
	u_char	cdxopen_flag;	/* exclusive open flag */
} sr_open_flag[MAX_CDROM];

static	void	sr_setup_openflag();

/*
** A string to keep track of the vendor id.
** This is needed since the scsi_disk structure does not
** has a place to put this information.
*/
struct	cdrom_inquiry {
	char	cdrom_vid[8];	/* vendor ID */
	char	cdrom_pid[16];	/* product ID */
	char	cdrom_rev[4];	/* revision level */
} sr_inq[MAX_CDROM];

#ifndef	SRDEBUG
short sr_retry	= EL_RETRY;	/*
				 * Error message threshold, retries.
				 * Make it global so manufacturing can
				 * override setting.
				 */

#else	SRDEBUG
short sr_retry	= 0;		/* For debug, set retries to 0. */

/************************************************************************
**									*
** Local Function Declarations						*
**									*
*************************************************************************/
static	int	sr_medium_removal();
static	int	sr_pause_resume(), sr_play_msf(), sr_play_trkind();
static	int	sr_read_tochdr(), sr_read_tocentry();

sr_print_buffer(y, count)
	register u_char *y;
	register int count;
{
	register int x;

	for (x = 0; x < count; x++)
		printf("%x  ", *y++);
	printf("\n");
}
#endif	SRDEBUG


/*
** Return a pointer to this unit's unit structure.
*/
srunitptr(md)
	register struct mb_device *md;
{
	return ((int)&srunits[md->md_unit]);
}


static
srtimer(dev)
	register dev_t dev;
{
	register struct scsi_disk *dsi;
	register struct scsi_unit *un;
	register struct scsi_ctlr *c;
	register u_int	unit;

	unit = SRUNIT(dev);
	un = &srunits[unit];
	dsi = &srdisk[unit];
	c = un->un_c;

	/* DPRINTF("srtimer:\n"); */
	if (dsi->un_openf >= OPENING) {
		(*un->un_c->c_ss->scs_start)(un);

#ifdef		SR_TIMEOUT
		if ((dsi->un_timeout != 0) && (--dsi->un_timeout == 0)) {
			/* Process command timeout for normal I/O operations */
			printf("sr%d:  srtimer: I/O request timeout\n", unit);
			if ((*c->c_ss->scs_deque)(c, un)) {
				/* Can't Find CDB I/O request to kill, help! */
				printf("sr%d:  srtimer: can't abort request\n",
					unit);
				(*un->un_c->c_ss->scs_reset)(un->un_c, 0);
			}
		} else if (dsi->un_timeout != 0) {
			DPRINTF2("srtimer:  running, open= %d, timeout= %d\n",
					dsi->un_openf, dsi->un_timeout);
		}
#endif		SR_TIMEOUT

	/* Process opening delay timeout */
	} else if ((dsi->un_timeout != 0) && (--dsi->un_timeout == 0)) {
		DPRINTF("srtimer:  running...\n");
		wakeup((caddr_t)dev);
	}
	timeout(srtimer, (caddr_t)dev, 30*hz);
}

/*  */
/************************************************************************
*************************************************************************
**									*
**									*
**		Autoconfiguration Routines				*
**									*
**									*
*************************************************************************
/************************************************************************/

/*
** Attach device (boot time).  The controller is there. Since this is a
** CDROM, there is no need to check the label.
*/

srattach(md)
	register struct mb_device *md;
{
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
	struct scsi_inquiry_data *sid;
	int i;

	dsi = &srdisk[md->md_unit];
	un = &srunits[md->md_unit];
	sr_open_flag[md->md_unit].cdopen_flag0 = 0;
	sr_open_flag[md->md_unit].cdopen_flag1 = 0;
	sr_open_flag[md->md_unit].cdxopen_flag = 0;

	/*
	 * link in, fill in unit struct.
	 */
	un->un_md = md;
	un->un_mc = md->md_mc;
	un->un_unit = md->md_unit;
	un->un_target = TARGET(md->md_slave);
	un->un_lun = LUN(md->md_slave);
	un->un_ss = &scsi_unit_subr[TYPE(md->md_flags)];
	un->un_present = 0;

	dsi->un_openf = OPENING;
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
	 * Allocate space for request sense/inquiry buffer in
	 * memory.  Align it to a longword boundary.
	 */
	sid = (struct scsi_inquiry_data *)rmalloc(iopbmap,
		(long)(sizeof (struct scsi_inquiry_data) +4));
	if (sid == NULL) {
		printf("sr%d:  srattach: no space for inquiry data\n",
			SRNUM(un));
		return;
	}

	while ((u_int)sid & 0x03)
		((u_int)sid)++;
	dsi->un_sense = (int)sid;

	EPRINTF2("sr%d:	 srattach: buffer= 0x%x, ", SRNUM(un), (int)sid);
	EPRINTF2("dsi= 0x%x, *scsi_unit= 0x%x\n", dsi, un);
	DPRINTF2("srattach: looking for lun %d on target %d\n",
			LUN(md->md_slave), TARGET(md->md_slave));

	/*
	 * Test for unit ready.	 The first test checks
	 * for a non-existant device.  The other tests
	 * wait for the drive to get ready.
	 */
	if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0) > 1) {
	    if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0) > 1) {
		DPRINTF("srattach:  unit offline\n");
		return;
	    }
	}
	for (i = 0; i < MAX_BUSY; i++) {
		if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0) == 0) {
			goto SRATTACH_UNIT;

		} else if (un->un_scb.chk) {
			goto SRATTACH_UNIT;

		} else if (un->un_scb.busy && !un->un_scb.is) {
			EPRINTF("srattach:  unit busy\n");
			DELAY(6000000);		/* Wait 6 Sec. */

		} else if (un->un_scb.is) {
			EPRINTF("srattach:  reservation conflict\n");
			break;
		}
	}
	printf("srattach:  unit offline\n");
	return;

SRATTACH_UNIT:
	if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0) != 0) {
		DPRINTF("srattach:  unit failed\n");
		return;
	}

	bzero((caddr_t)sid, sizeof (struct scsi_inquiry_data));
	bzero((caddr_t)&sr_inq[md->md_unit], sizeof (struct cdrom_inquiry));
	if (simple(un, SC_INQUIRY, (char *) sid - DVMA, 0,
			(int)sizeof (struct scsi_inquiry_data)) == 0) {
	/* Only CCS SCSI-1 compliant controllers support Inquiry cmd */
#ifdef		SRDEBUG
		if (sr_debug > 2)
			sr_print_buffer((u_char *)sid, 32);
#endif		SRDEBUG
		if ((bcmp(sid->vid, "TOSHIBA CD-ROM",
			sizeof (sid->vid)) == 0) ||
		    (bcmp(sid->vid, "SONY", 4) == 0)) {
/*			printf("sr0: %s found\n", sid->vid); */
			dsi->un_ctype = CTYPE_TOSHIBA;
			dsi->un_flags = SC_UNF_NO_DISCON;
			if (i = simple(un, SR_MEDIA_REMOV, 0, 0,
					SR_MEDIA_unlock)) {
			    EPRINTF1("srattach: media-unlock failed:%x\n", i);
			    return;
			}
			dsi->un_flags = 0;
		} else {
/*			printf("sr0: %s found\n", sid->vid); */
			printf("   defaulting to generic cdrom\n");
			dsi->un_ctype = CTYPE_CDROM;
			/* play it safe if unknown cdrom */
			dsi->un_flags = SC_UNF_NO_DISCON;
		}
		bcopy(sid->vid, (caddr_t)sr_inq[md->md_unit].cdrom_vid, 8);
		bcopy(sid->pid, (caddr_t)sr_inq[md->md_unit].cdrom_pid, 16);
		bcopy(sid->revision,
			(caddr_t)sr_inq[md->md_unit].cdrom_rev, 4);
	} else {
		/* non-CCS, assume Adaptec ACB-4000 */
		EPRINTF("srattach: non-CCS Adaptec found\n");
		dsi->un_ctype = CTYPE_ACB4000;
		dsi->un_flags = SC_UNF_NO_DISCON;
	}
	un->un_present = 1;			/* "it's here...(#2)" */
	dsi->un_openf = OPEN;
}



/*
** Run a command in polled mode.
** Return true if successful, false otherwise.
*/

static
simple(un, cmd, dma_addr, secno, nsect)
	register struct scsi_unit *un;
	int cmd, dma_addr, secno, nsect;
{
	register struct scsi_cdb *cdb;
	register struct scsi_ctlr *c;
	register int err;

	/*
	 * Grab and clear the command block.
	 */
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
	if (cmd == SC_INQUIRY)
		un->un_dma_count = nsect;
	else
		un->un_dma_count = nsect << SECDIV;

	/*
	 * Fire up the pup.
	 */
	if (err = (*c->c_ss->scs_cmd)(c, un, 0)) {
		if (err > 1) {
			return (2);	/* Hard failure */
		} else {
			return (1);	/* Recoverable failure */
		}
	}
	return (0);			/* No failure */
}

/*^L*/
/************************************************************************
*************************************************************************
**									*
**									*
**			Unix Entry Points				*
**									*
**									*
*************************************************************************
/************************************************************************/

/*
** This routine opens a disk.  Note that we can handle disks
** that make an appearance after boot time.
*/
/*ARGSUSED*/
sropen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	register int unit;
	register struct scsi_disk *dsi;
	register struct scsi_inquiry_data *sid;

	unit = SRUNIT(dev);
	if (unit >= ncdrom) {
		EPRINTF("sropen:  illegal unit\n");
		return (ENXIO);
	}

	un = &srunits[unit];
	dsi = &srdisk[unit];
	if (un->un_mc == 0) {
		EPRINTF("sropen:  disk not attached: no controller\n");
		return (ENXIO); }

	DPRINTF2("sropen: unit %x flag %x\n", unit, flag);
	/*
	 * If command timeouts not activated yet, switch it on.
	 */
#ifdef	SR_TIMEOUT
	if (dsi->un_timer == 0) {
		EPRINTF("sropen:  starting timer\n");
		dsi->un_timer++;
		timeout(srtimer, (caddr_t)dev, 30*hz);
	}
#endif	SR_TIMEOUT

	/*
	 * Check for special opening mode.
	 */
	lp = &dsi->un_map[LPART(dev)];
	if (un->un_present) {
		/*
		 * check for previous exclusive open
		 */
		if ((sr_open_flag[unit].cdxopen_flag) ||
		    (flag & FEXCL) && ((sr_open_flag[unit].cdopen_flag0) ||
				       (sr_open_flag[unit].cdopen_flag1))) {
			return (EBUSY);
		}
		dsi->un_openf = OPENING;
		if ((srcmd(dev, SR_START_STOP, 0, SR_START_load, 
			   (caddr_t)0, SD_SILENT, (caddr_t)0)) &&
		    (srcmd(dev, SR_START_STOP, 0, SR_START_load,
			   (caddr_t)0, SD_SILENT, (caddr_t)0))) {
			return (EIO);
		}
		(void) srcmd(dev, SR_MEDIA_REMOV, 0,
			     SR_MEDIA_lock, (caddr_t)0, 0, (caddr_t)0);
		dsi->un_openf = OPEN;
		sr_setup_openflag(&(sr_open_flag[unit]), (u_char)major(dev));

		/*
		 * set up exclusive open flags
		 */
		if (flag & FEXCL) {
			sr_open_flag[unit].cdxopen_flag = 1;
		} else {
			sr_open_flag[unit].cdxopen_flag = 0;
		}

		return (0);

		/*
		 * Didn't see it at autoconfig time?   Let's look again..
		 */
	} else {
		DPRINTF1("sropen:  opening device: lp->dkl_nblk:%x\n",
			 lp->dkl_nblk);
		lp->dkl_nblk = CD_SIZE;

		dsi->un_openf = OPENING;
		dsi->un_flags = SC_UNF_NO_DISCON;	/* no disconnects */
		if (srcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0,
			  0, (caddr_t)0) && dsi->un_openf == CLOSED) {
			EPRINTF("sropen:  not ready\n");
			dsi->un_timer = 0;
			untimeout(srtimer, (caddr_t)dev);
			return (ENXIO);
		}
		dsi->un_openf = OPENING;
		dsi->un_flags = SC_UNF_NO_DISCON;   /* no disconnects */
		if ((srcmd(dev, SR_START_STOP, 0, SR_START_load,
			   (caddr_t)0, SD_SILENT, (caddr_t)0)) &&
		    (srcmd(dev, SR_START_STOP, 0, SR_START_load,
			   (caddr_t)0, SD_SILENT, (caddr_t)0))) {
			return (EIO);
		}
		(void)srcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0, 0,
			    (caddr_t)0);
		dsi->un_openf = OPENING;
		if (srcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0, 0,
			  (caddr_t)0)) {
			EPRINTF("sropen:  not ready\n");
			dsi->un_timer = 0;
			untimeout(srtimer, (caddr_t)dev);
			return (ENXIO);
		}
		dsi->un_openf = OPENING;
		(int)sid = dsi->un_sense;	/* Inquiry data buffer */
		bzero((caddr_t)sid, sizeof (struct scsi_inquiry_data));
		bzero((caddr_t)&sr_inq[unit], sizeof (struct cdrom_inquiry));
		if (srcmd(dev, SC_INQUIRY, 0,
			  sizeof (struct scsi_inquiry_data),
			  (caddr_t)sid, 0, (caddr_t)0)) {
			/* non-CCS, assume Adaptec */
			EPRINTF("sropen:  Adaptec found\n");
			dsi->un_ctype = CTYPE_ACB4000;
			dsi->un_flags = SC_UNF_NO_DISCON;
		} else {
#ifdef	SRDEBUG
			if (sr_debug > 2) sr_print_buffer((u_char *)sid, 32);
#endif	SRDEBUG
			if (sid->dtype != 5) {
				printf("sropen - not cdrom: %d \n",
				       sid->dtype);
			}
			dsi->un_flags = 0;
			/*    printf("sr0: %s found\n", sid->vid);	*/
			if ((bcmp(sid->vid, "TOSHIBA CD-ROM",
				  sizeof (sid->vid)) == 0) ||
			    (bcmp(sid->vid, "SONY ", 4) == 0)) {
				EPRINTF("sropen:  CCS compliant found\n");
				dsi->un_ctype = CTYPE_TOSHIBA;
				un->un_present = 1;	/* "it's here...#3" */
				dsi->un_openf = OPENING;
				dsi->un_openf = OPENING;
				(void) srcmd(dev, SR_START_STOP, 0,
				     SR_START_load, (caddr_t)0, 0, (caddr_t)0);
				dsi->un_openf = OPENING;
				(void) srcmd(dev, SR_MEDIA_REMOV, 0,
				     SR_MEDIA_lock, (caddr_t)0, 0, (caddr_t)0);
				dsi->un_openf = OPEN;
				sr_setup_openflag(&(sr_open_flag[unit]),
						  (u_char)major(dev));
			} else if (sid->rdf != 0x01) {
				EPRINTF("sropen:  non-CCS found\n");
				dsi->un_ctype = CTYPE_ACB4000;
			} else {
				EPRINTF("sropen:  CCS found\n");
				/*
				** new compliant SCSI 1 default
				** dsi->un_ctype = CTYPE_CCS;
				*/
				printf("assuming CCS compliant CDROM: %s\n",
				       sid->vid);
				un->un_present = 1;	/* "it's here...#4" */
				dsi->un_ctype = CTYPE_CDROM;
				dsi->un_openf = OPENING;
				(void) srcmd(dev, SR_MEDIA_REMOV, 0,
				     SR_MEDIA_lock, (caddr_t)0, 0, (caddr_t)0);
				sr_setup_openflag(&(sr_open_flag[unit]),
				    (u_char)major(dev));
				dsi->un_openf = OPEN;
				if (!disconenbl) {
					printf(
				"sr-disconnect-reconnect disabled %x\n",
					    disconenbl);
					dsi->un_flags = SC_UNF_NO_DISCON;
				}
			}
		}
		bcopy(sid->vid, (caddr_t)sr_inq[unit].cdrom_vid, 8);
		bcopy(sid->pid, (caddr_t)sr_inq[unit].cdrom_pid, 16);
		bcopy(sid->revision, (caddr_t)sr_inq[unit].cdrom_rev,
		    4);

		/*
		 ** set up exclusive open flags
		 */
		if (flag & FEXCL) {
			sr_open_flag[unit].cdxopen_flag = 1;
		}
		return (0);
	}
}

/*
** This routine is added to unlock the drive at hsfs unmount time
*/

/*ARGSUSED*/
srclose(dev, mode)
dev_t	dev;
int	mode;
{
	register struct scsi_unit *un;
	register int unit;
	register struct scsi_disk *dsi;

	DPRINTF2("sr: last close on dev %d.%d\n", major(dev), minor(dev));

	unit = SRUNIT(dev);

	if (unit >= ncdrom) {
		EPRINTF("srclose:  illegal unit\n");
		return (ENXIO);
	}

	un = &srunits[unit];
	dsi = &srdisk[unit];
	if (un->un_mc == 0) {
		EPRINTF("srclose:  disk not attached: no controller\n");
		return (ENXIO); }


	if (sr_open_flag[unit].cdopen_flag0 == major(dev)) {
		sr_open_flag[unit].cdopen_flag0 = 0;
	} else if (sr_open_flag[unit].cdopen_flag1 == major(dev)) {
		sr_open_flag[unit].cdopen_flag1 = 0;
	} else {
		printf("srclose: sr_open_flag corrupted\n");
	}

	if (sr_open_flag[unit].cdxopen_flag) {
		sr_open_flag[unit].cdxopen_flag = 0;
	}

	dsi->un_openf = OPENING;

	if ((sr_open_flag[unit].cdopen_flag0 == 0) &&
	    (sr_open_flag[unit].cdopen_flag1 == 0)) {
		if (srcmd(dev, SR_MEDIA_REMOV, 0, SR_MEDIA_unlock,
			(caddr_t)0, 0, (caddr_t)0)) {
#if defined(MUNIX) || defined(MINIROOT)
/*
** If this is MUNIX or the miniroot then we may be using
** a magnetic disk to simulate a cdrom for installation.
** We are lenient on the error from the SR_MEDIA_REMOV
** command if we suspect a magnetic disk is being used
** (see setting of un_ctype in srattach() and sropen()).
*/
			if (dsi->un_ctype != CTYPE_CDROM)
#endif
				return (ENXIO);
		}
	}
	dsi->un_openf = CLOSED;
	return (0);
}

/*
** This routine returns the size of a logical partition.  It is called
** from the device switch at normal priority.
*/
srsize(dev)
	register dev_t dev;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	register struct scsi_disk *dsi;
	register int unit;

	unit = SRUNIT(dev);
	if (unit >= ncdrom) {
		return (-1);
	}
	un = &srunits[unit];
	DPRINTF("srsize:\n");

	if (un->un_present) {
		dsi = &srdisk[SRUNIT(dev)];
		lp = &dsi->un_map[LPART(dev)];
		return (lp->dkl_nblk = CD_SIZE); /* = 2**22 == 4 mb  */
	} else {
		EPRINTF("srsize:  unit not present\n");
		return (CD_SIZE);	/* = 2**22 == 4 mb	*/
	}
}


/*
** This routine is the focal point of internal commands to the controller.
** NOTE: this routine assumes that all operations done before the disk's
** geometry is defined.	 IT IS CALLED FROM THE BOTTOM HALF.
*/

srcmd(dev, cmd, block, count, addr, options, uscsi_cmd)
	dev_t dev;
	int cmd, block, count;
	caddr_t addr;
	int options;
	caddr_t uscsi_cmd;
{
	register struct scsi_disk *dsi;
	register struct scsi_unit *un;
	register struct buf *bp;
	int s;
	long b_flags;
	int	flag;
	struct	uscsi_cmd	*scmd;

	un = &srunits[SRUNIT(dev)];
	dsi = &srdisk[SRUNIT(dev)];
	bp = &un->un_sbuf;

	if (uscsi_cmd != (caddr_t)0) {
		scmd = (struct uscsi_cmd *)uscsi_cmd;
		flag = (scmd->uscsi_flags & USCSI_READ) ? B_READ : B_WRITE;
	} else {
		flag = B_WRITE;
	}

	/* DPRINTF("srcmd:\n"); */
	s = splr(pritospl(un->un_mc->mc_intpri));
	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t) bp, PRIBIO);
	}

	bp->b_flags = B_BUSY | flag;
	(void) splx(s);

	un->un_scmd = cmd;
	bp->b_dev = dev;
	bp->b_bcount = count;
	bp->b_blkno = block;
	(caddr_t)bp->b_un.b_addr = addr;
	dsi->un_options = options;

	un->un_scratch_addr = uscsi_cmd;

	/*
	 * Execute the I/O request.
	 */

	srstrategy(bp);
	(void) iowait(bp);

	/* s = splr(pritospl(un->un_mc->mc_intpri)); */
	bp->b_flags &= ~B_BUSY;
	b_flags = bp->b_flags;
	if (bp->b_flags & B_WANTED) {
		/* DPRINTF1("srcmd:  waking...,	 bp= 0x%x\n", bp); */
		wakeup((caddr_t)bp);
	}
	/* (void) splx(s); */
	return (b_flags & B_ERROR);
}


/*
** This routine is the high level interface to the disk.  It performs
** reads and writes on the disk using the buf as the method of communication.
** It is called from the device switch for block operations and via physio()
** for raw operations.	It is called at normal priority.
*/
srstrategy(bp)
	register struct buf *bp;
{
	register struct scsi_unit *un;
	register struct mb_device *md;
	register struct dk_map *lp;
	register u_int bn;
	register struct diskhd *dh;
	register struct scsi_disk *dsi;
	register int s;
	int unit;

	DPRINTF("srstrategy:\n");
	unit = dkunit(bp);
	if (unit >= ncdrom) {
		EPRINTF("srstrategy: invalid unit\n");
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	un = &srunits[unit];
	md = un->un_md;
	dsi = &srdisk[unit];
	lp = &dsi->un_map[LPART(bp->b_dev)];
	bn = bp->b_blkno;

	/* Check for EOM */
	if ((bp != &un->un_sbuf) && (un->un_present && (bn >= lp->dkl_nblk))) {
		printf("srstrategy:%x  invalid block addr: %x >= %x\n",
			bp, bn, lp->dkl_nblk);
		bp->b_resid = bp->b_bcount;
		iodone(bp);
		return;
	}

	s = splr(pritospl(un->un_mc->mc_intpri));
	dh = &md->md_utab;
	disksort(dh, bp);

	/*
	 * call unit start routine to queue up device, if it
	 * currently isn't queued.
	 */
	(*un->un_c->c_ss->scs_ustart)(un);

	/* call start routine to run the next SCSI command */
	(*un->un_c->c_ss->scs_start)(un);
	(void) splx(s);
}


/*
** Set up a transfer for the controller
*/
srstart(bp, un)
	register struct buf *bp;
	register struct scsi_unit *un;
{
	register struct dk_map *lp;
	register struct scsi_disk *dsi;
	register int nblk;

	DPRINTF("srstart:\n");

	dsi = &srdisk[dkunit(bp)];
	lp = &dsi->un_map[LPART(bp->b_dev)];

	/* Process internal I/O requests */
	if (bp == &un->un_sbuf) {
		DPRINTF("srstart:  internal I/O\n");
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
		DPRINTF("srstart:  read\n");
		un->un_cmd = SC_READ;
	} else {
		printf("srstart: CDROM can't write\n");
		un->un_cmd = SC_WRITE;
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
	un->un_count = nblk;
#ifndef FIVETWELVE
	un->un_blkno /= 4;  /* !! this is because each block is really 2k */
#endif FIVETWELVE
	/* printf("blkno = %x count = %x\n", un->un_blkno, un->un_count); */
	return (1);
}

/*
** Make a cdb for disk I/O.
*/
srmkcdb(un)
	struct scsi_unit *un;
{
	register struct scsi_cdb *cdb;
	register struct scsi_disk *dsi;

	struct uscsi_cmd	*uscsi_scmd;

	DPRINTF("srmkcdb:\n");

	dsi = &srdisk[un->un_unit];
	cdb = &un->un_cdb;
	bzero((caddr_t)cdb, CDB_GROUP1);
	cdb->cmd = un->un_cmd;
	cdb->lun = un->un_lun;
	dsi->un_timeout = 180;
	un->un_dma_addr = un->un_baddr;

	switch (un->un_cmd) {
	case SC_READ:
		DPRINTF("srmkcdb:  read\n");
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0ADDR(cdb, un->un_blkno);
		FORMG0COUNT(cdb, un->un_count);
		un->un_dma_count = un->un_count << SECDIV;
		break;

	case SC_REQUEST_SENSE:
		DPRINTF("srmkcdb:  request sense\n");
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0COUNT(cdb,  sizeof (struct scsi_sense));
		un->un_dma_addr = (int)dsi->un_sense - (int)DVMA;
		un->un_dma_count = sizeof (struct scsi_sense);
		bzero((caddr_t)(dsi->un_sense), sizeof (struct scsi_sense));
		return;

	case SC_SPECIAL_READ:
		DPRINTF1("srmkcdb: special read blk: 0x%x\n", un->un_blkno);
		un->un_cmd = cdb->cmd = SC_READ;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0ADDR(cdb, un->un_blkno);
		FORMG0COUNT(cdb, un->un_count >> SECDIV);
		un->un_dma_count = un->un_count;
		break;

	case SC_READ_LABEL:
		DPRINTF("srmkcdb:  read label\n");
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		un->un_cmd = cdb->cmd = SC_READ;
		FORMG0ADDR(cdb, un->un_blkno);
		FORMG0COUNT(cdb,  1);
		un->un_dma_addr = (int)un->un_sbuf.b_un.b_addr - (int)DVMA;
		un->un_dma_count = SECSIZE;
		break;

	case SC_REZERO_UNIT:
		EPRINTF("srmkcdb:  rezero unit\n");
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = 0;
		un->un_dma_count = 0;
		return;

	case SR_START_STOP:
		DPRINTF("srmkcdb:  stop eject\n");
		dsi->un_timeout = 6;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = 0;
		un->un_dma_count = 0;
		FORMG0ADDR(cdb, un->un_blkno);
		FORMG0COUNT(cdb, un->un_count);
#ifdef DEBUGCDB
		printf("START cdb g0:%x %x:%x  addr:%x %x cnt:%x ctrl %x\n",
			cdb->cmd, cdb->lun, cdb->tag,  cdb->g0_addr1,
			cdb->g0_addr0, cdb->g0_count0, cdb->g1_addr1);
#endif DEBUGCDB
		break;

	case SR_MEDIA_REMOV:
		DPRINTF("srmkcdb:  media remov\n");
		dsi->un_timeout = 6;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = 0;
		un->un_dma_count = 0;
		FORMG0ADDR(cdb, un->un_blkno);
		FORMG0COUNT(cdb, un->un_count);
#ifdef DEBUGCDB
		printf("MEDIA cdb g0:%x %x:%x  addr:%x %x cnt:%x ctrl %x\n",
			cdb->cmd, cdb->lun, cdb->tag,  cdb->g0_addr1,
			cdb->g0_addr0, cdb->g0_count0, cdb->g1_addr1);
#endif DEBUGCDB
		break;


	case SC_TEST_UNIT_READY:
		/* DPRINTF("srmkcdb:  test unit ready\n"); */
		dsi->un_timeout = 6;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = 0;
		un->un_dma_count = 0;
		break;

	case SC_INQUIRY:
	    DPRINTF("srmkcdb:  inquiry\n");
	    dsi->un_timeout = 6;
	    un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
	    FORMG0COUNT(cdb, sizeof (struct scsi_inquiry_data));
	    un->un_dma_addr = (int)dsi->un_sense - (int)DVMA;
	    un->un_dma_count = sizeof (struct scsi_inquiry_data);
	    bzero((caddr_t)(dsi->un_sense), sizeof (struct scsi_inquiry_data));
	    break;

#ifdef OLDCODE
	case SC_MODE_SELECT:
		EPRINTF("srmkcdb:  mode select\n");
		dsi->un_timeout = 6;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		FORMG0COUNT(cdb, un->un_count);
		un->un_dma_count = un->un_count;
		un->un_dma_addr = un->un_baddr;
		if (un->un_blkno & 0x80) {
			EPRINTF("srmkcdb:  savable mode select\n");
			cdb->tag = 1;		/* Save parameters */
		}
		break;
#endif OLDCODE

	case SC_MODE_SENSE:
		EPRINTF("srmkcdb:  mode sense\n");
		dsi->un_timeout = 6;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0COUNT(cdb, un->un_count);
		cdb->g0_addr1  = un->un_blkno;
		un->un_dma_count = un->un_count;
		un->un_dma_addr = un->un_baddr;
		break;

	/*
	 * case	 CDROM IOCTL COMMANDS
	 */
	default:
	    if ((uscsi_scmd = (struct uscsi_cmd *)(un->un_scratch_addr)) !=0){
		    if (uscsi_scmd->uscsi_flags & USCSI_READ) {
			    un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		    } else {
			    un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		    }

		    DPRINTF1("srmkcdb: cdb cmd is 0x%x\n",
			(u_char)uscsi_scmd->uscsi_cdb[0]);
/*		    bcopy((caddr_t)uscsi_scmd->uscsi_cdb, cdb,
			uscsi_scmd->uscsi_cdblen); */
		    sr_makecom_all((char *)uscsi_scmd->uscsi_cdb,
			cdb);
		    if (uscsi_scmd->uscsi_buflen == 0) {
			    un->un_dma_addr = 0;
			    un->un_dma_count = 0;
		    } else {
			    un->un_dma_addr = (caddr_t)uscsi_scmd->
				uscsi_bufaddr - (caddr_t)DVMA;
			    if (un->un_dma_addr & 0x3)
				printf("dma_addr %x\n", un->un_dma_addr);
			    un->un_dma_count = uscsi_scmd->uscsi_buflen;
		    }
		    un->un_cmd_len = uscsi_scmd->uscsi_cdblen;
		    break;
	    } else {
		EPRINTF("srmkcdb:  unknown command\n");
		dsi->un_timeout = 6;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		break;
	    }
	}
	dsi->un_last_cmd = un->un_cmd;
	return;
}


/*
** This routine handles controller interrupts.
** It is always called at disk interrupt priority.
*/
typedef enum srintr_error_resolution {
	real_error,		/* Hard error */
	psuedo_error,		/* What looked like an error is actually OK */
	more_processing	/* Recoverable error */
} srintr_error_resolution;

srintr_error_resolution srerror();


srintr(c, resid, error)
	register struct scsi_ctlr *c;
	u_int resid;
	int error;
{
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
	register struct buf *bp;
	register struct mb_device *md;
	int status = 0;
	int error_code = 0;
	u_char severe = DK_NOERROR;

	DPRINTF("srintr:\n");
	un = c->c_un;
	md = un->un_md;
	bp = md->md_utab.b_forw;
	if (bp == NULL) {
	    printf("srintr: bp = NULL\n");
	    return;
	}

	dsi = &srdisk[SRUNIT(bp->b_dev)];
	dsi->un_timeout = 0;		/* Disable time-outs */

	if (md->md_dk >= 0) {
		dk_busy &= ~(1 << md->md_dk);
	}

	/*
	 * Check for error or special operation states and process.
	 * Otherwise, it was a normal I/O command which was successful.
	 */
	if (dsi->un_openf < OPEN || error || resid) {
	    /*
	    ** Special processing for SCSI bus failures.
	    */
	    if (error == SE_FATAL) {
		if (dsi->un_openf == OPEN) {
			printf("sr%d:  scsi bus failure\n", SRNUM(un));
		}
		dsi->un_retries = dsi->un_restores = 0;
		dsi->un_err_severe = DK_FATAL;
		un->un_present = 0;
		dsi->un_openf = CLOSED;
		bp->b_flags |= B_ERROR;
		goto SRINTR_WRAPUP;
	    }

	    /*
	    ** Opening disk, check if command failed.  If it failed
	    ** close device.  Otherwise, it's open.
	    */
	    if (dsi->un_openf == OPENING) {
		if (error == SE_NO_ERROR) {
			dsi->un_openf = OPEN;
			goto SRINTR_SUCCESS;
		} else if (error == SE_RETRYABLE) {
			/* DPRINTF("srintr:  open failed\n"); */
			dsi->un_openf = OPEN_FAILED;
			bp->b_flags |= B_ERROR;
			goto SRINTR_WRAPUP;
		} else {
			EPRINTF("srintr:  hardware failure\n");
			dsi->un_openf = CLOSED;
			bp->b_flags |= B_ERROR;
			goto SRINTR_WRAPUP;
		    }
	    }

		/*
		** Rezero for failed command done, retry failed command
		*/
		if ((dsi->un_openf == RETRYING) &&
		    (un->un_cdb.cmd == SC_REZERO_UNIT)) {

		    if (error)
			    DPRINTF1("sr%d:  rezero failed\n", SRNUM(un));

		    DPRINTF("srintr:  rezero done\n");
		    un->un_flags &= ~SC_UNF_GET_SENSE;
		    un->un_cdb = un->un_saved_cmd.saved_cdb;
		    un->un_dma_addr = un->un_saved_cmd.saved_dma_addr;
		    un->un_dma_count = un->un_saved_cmd.saved_dma_count;
		    un->un_cmd = dsi->un_last_cmd;
		    srmkcdb(un);
		    (*c->c_ss->scs_cmd)(c, un, 1);
		    return;
		}


		/*
		 * Command failed, need to run request sense command.
		 */
		if ((dsi->un_openf == OPEN) && error) {
			srintr_sense(dsi, un, resid);
			return;
		}

		/*
		 * Request sense command done, restore failed command.
		 */
		if (dsi->un_openf == SENSING) {
			DPRINTF("srintr:  restoring sense\n");
			srintr_ran_sense(un, dsi, &resid);
		}

		/*
		 * Reassign done, restore original state
		 */
		if (dsi->un_openf == MAPPING) {
			DPRINTF("srintr:  restoring state\n");
			srintr_ran_reassign(un, dsi, &resid);
		}

		EPRINTF2("srintr:  retries= %d	restores= %d  ",
		    dsi->un_retries, dsi->un_restores);
		EPRINTF1("total= %d\n", dsi->un_total_errors);
		if ((dsi->un_openf == RETRYING) && (error == 0)) {

			EPRINTF("srintr:  ok\n\n");
			dsi->un_openf = OPEN;
			dsi->un_retries = dsi->un_restores = 0;
			dsi->un_err_severe = DK_RECOVERED;
			goto SRINTR_SUCCESS;
		}

		/*
		 * Process all other errors here
		 */
		EPRINTF2("srintr:  processing error,  error= %x	 chk= %x",
			error, un->un_scb.chk);
		EPRINTF1("  busy= %x", un->un_scb.busy);
		EPRINTF2("  resid= %d (%d)\n", resid, un->un_dma_count);
		switch (srerror(c, un, dsi, bp, resid, error)) {
		case real_error:
			/* This error is FATAL ! */
			DPRINTF("srintr:  real error\n");
			dsi->un_retries = dsi->un_restores = 0;
			bp->b_flags |= B_ERROR;
			goto SRINTR_WRAPUP;

		case psuedo_error:
			/* A psuedo-error:  soft error reported by ctlr */
			DPRINTF("srintr:  psuedo error\n");
			status = dsi->un_status;
			error_code = dsi->un_err_code;
			severe = dsi->un_err_severe;
			dsi->un_retries = dsi->un_restores = 0;
			goto SRINTR_SUCCESS;

		case more_processing:
			/* real error requiring error recovery */
			DPRINTF("stintr:  more processing\n");
			return;
		}
	}


	/*
	 * Handle successful Transfer.	Also, take care of ACB-4000
	 * seek error problem by doing single sector I/O.
	 */
SRINTR_SUCCESS:
	dsi->un_status = status;
	dsi->un_err_code = error_code;
	dsi->un_err_severe = severe;

	if (dsi->un_sec_left) {
		EPRINTF("srintr:  single sector writes\n");
		dsi->un_sec_left--;
		un->un_baddr += SECSIZE;
		un->un_blkno++;
		srmkcdb(un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) != 0)
			printf("sr%d:  single sector I/O failed\n", SRNUM(un));
	}


	/*
	 * Handle I/O request completion (both sucessful and failed).
	 */
SRINTR_WRAPUP:
	if (bp == &un->un_sbuf &&
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
** Disk error decoder/handler.
*/
static srintr_error_resolution
srerror(c, un, dsi, bp, resid, error)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
	struct buf *bp;
	u_int resid;
	int error;
{
	register struct scsi_ext_sense *ssr;

	ssr = (struct scsi_ext_sense *)dsi->un_sense;
	/* DPRINTF("srerror:\n"); */

	/*
	 * Special processing for driver command timeout errors.
	 */
	if (error == SE_TIMEOUT) {
		EPRINTF("srerror:  command timeout error\n");
		dsi->un_status = SC_TIMEOUT;
		goto SRERROR_RETRY;
	}

	/*
	 * Check for Adaptec ACB-4000 seek error problem.  If found,
	 * transfer data one sector at a time.
	 */
	if (dsi->un_ctype == CTYPE_ACB4000 && un->un_scb.chk &&
	    ssr->error_code == 15 && un->un_count > 1) {
		EPRINTF("srerror:  seek error\n");
		dsi->un_sec_left = un->un_count - 1;
		un->un_count = 1;
		srmkcdb(un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) != 0) {
			printf("sr%d:  srerror: single sector I/O failed\n",
			    SRNUM(un));
			return (real_error);
		}
		return (more_processing);
	}

	/*
	 * Check for various check condition errors.
	 */
	dsi->un_total_errors++;
	if (un->un_scb.chk) {

		switch (ssr->key) {
		case SC_RECOVERABLE_ERROR:
			sr_fix_block(c, dsi, un, resid, SE_RETRYABLE);
			dsi->un_err_severe = DK_CORRECTED;
			if (sr_retry == 0) {
				srerrmsg(un, bp, "recoverable");
			}
			return (psuedo_error);

		case SC_MEDIUM_ERROR:
			EPRINTF("srerror:  media error\n");
			/* sr_fix_block(c, dsi, un, resid, SE_HARD_ERROR); */
			goto SRERROR_RETRY;

		case SC_HARDWARE_ERROR:
			EPRINTF("srerror:  hardware error\n");
			goto SRERROR_RETRY;

/*  XXX this also occurs on an unexpected media removal with code 28	*/
		case SC_UNIT_ATTENTION:
			EPRINTF("srerror:  unit attention error\n");
			goto SRERROR_RETRY;

		case SC_NOT_READY:
			EPRINTF("srerror:  not ready\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		case SC_ILLEGAL_REQUEST:
			EPRINTF("srerror:  illegal request\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		case SC_VOLUME_OVERFLOW:
			EPRINTF("srerror:  volume overflow\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		case SC_WRITE_PROTECT:
			EPRINTF("srerror:  write protected\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		case SC_BLANK_CHECK:
			EPRINTF("srerror:  blank check\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		default:
			/*
			 * Undecoded sense key.	 Try retries and hope
			 * that will fix the problem.  Otherwise, we're
			 * dead.
			 */
			EPRINTF("srerror:  undecoded sense key error\n");
			SR_PRINT_CMD(un);
			dsi->un_err_severe = DK_FATAL;
			goto SRERROR_RETRY;
		}

	/*
	 * Process busy error.	Try retries and hope that
	 * it'll be ready soon.
	 */
	} else if (un->un_scb.busy && !un->un_scb.is) {
		EPRINTF1("sr%d:	 srerror: busy error\n", SRNUM(un));
		SR_PRINT_CMD(un);
		goto SRERROR_RETRY;

	/*
	 * Process reservation error.  Abort operation.
	 */
	} else if (un->un_scb.busy && un->un_scb.is) {
		EPRINTF1("sr%d:	 srerror: reservation conflict error\n",
			SRNUM(un));
		SR_PRINT_CMD(un);
		dsi->un_err_severe = DK_FATAL;
		return (real_error);

	/*
	 * Have left over residue data from last command.
	 * Do retries and hope this fixes it...
	 */
	} else	if (resid != 0) {
		EPRINTF1("srerror:  residue error, residue= %d\n", resid);
		SR_PRINT_CMD(un);
		goto SRERROR_RETRY;

	/*
	 * Have an unknown error.  Don't know what went wrong.
	 * Do retries and hope this fixes it...
	 */
	} else {
		EPRINTF("srerror:  unknown error\n");
		SR_PRINT_CMD(un);
		dsi->un_err_severe = DK_FATAL;
		goto SRERROR_RETRY;
	}

	/*
	 * Process command retries and rezeros here.
	 * Note, for on-line formatting, normal error
	 * recovery is inhibited.
	 */
SRERROR_RETRY:
	if (dsi->un_options & SD_NORETRY) {
		EPRINTF("srerror:  error recovery disabled\n");
		srerrmsg(un, bp, "failed, no retries");
		dsi->un_err_severe = DK_FATAL;
		return (real_error);
	}

	/*
	 * Command failed, retry it.
	 */
	if (dsi->un_retries++ < sr_max_retries) {
		if (((dsi->un_retries + dsi->un_restores) > sr_retry) ||
		    (dsi->un_restores != 0)) {
			srerrmsg(un, bp, "retry");
		}

		dsi->un_openf = RETRYING;
		srmkcdb(un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) != 0) {
			printf("sr%d:  srerror: retry failed\n", SRNUM(un));
			return (real_error);
		}
		return (more_processing);

	/*
	 * Retries exhausted, try restore
	 */
	} else if (++dsi->un_restores < sr_max_restores) {
		if (dsi->un_restores > sr_restores)
			srerrmsg(un, bp, "restore");

		/*
		 * Save away old command state.
		 */
		un->un_saved_cmd.saved_cdb = un->un_cdb;
		un->un_saved_cmd.saved_dma_addr = un->un_dma_addr;
		un->un_saved_cmd.saved_dma_count = un->un_dma_count;
		dsi->un_openf = RETRYING;
		dsi->un_retries = 0;
		un->un_cmd = SC_REZERO_UNIT;
		srmkcdb(un);
		(void) (*c->c_ss->scs_cmd)(c, un, 1);
		return (more_processing);

	/*
	 * Restores and retries exhausted, die!
	 */
	} else {
		/* complete failure */
		srerrmsg(un, bp, "failed");
		dsi->un_openf = OPEN;
		dsi->un_retries = 0;
		dsi->un_restores = 0;
		dsi->un_err_severe = DK_FATAL;
		return (real_error);
	}
}


/*
** Command failed, need to run a request sense command to determine why.
*/
static
srintr_sense(dsi, un, resid)
	register struct scsi_disk *dsi;
	register struct scsi_unit *un;
	u_int resid;
{

	/*
	 * Save away old command state.
	 */
	un->un_saved_cmd.saved_scb = un->un_scb;
	un->un_saved_cmd.saved_cdb = un->un_cdb;
	un->un_saved_cmd.saved_resid = resid;
	un->un_saved_cmd.saved_dma_addr = un->un_dma_addr;
	un->un_saved_cmd.saved_dma_count = un->un_dma_count;
	/*
	 * Note that s?start will call srmkcdb, which
	 * will notice that the flag is set and not do
	 * the copy of the cdb, doing a request sense
	 * rather than the normal command.
	 */
	DPRINTF("srintr:  getting sense\n");
	un->un_flags |= SC_UNF_GET_SENSE;
	dsi->un_openf = SENSING;
	un->un_cmd = SC_REQUEST_SENSE;
	(*un->un_c->c_ss->scs_go)(un->un_md);
}


/*
** Cleanup after running request sense command to see why the real
** command failed.
*/
static
srintr_ran_sense(un, dsi, resid_ptr)
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
	u_int *resid_ptr;
{
	register struct scsi_ext_sense *ssr;

	/*
	 * Check if request sense command failed.  This should
	 * never happen!
	 */
	un->un_flags &= ~SC_UNF_GET_SENSE;
	dsi->un_openf = OPEN;
	if (un->un_scb.chk) {
		printf("sr%d:  request sense failed\n",
		    SRNUM(un));
	}

	un->un_scb = un->un_saved_cmd.saved_scb;
	un->un_cdb = un->un_saved_cmd.saved_cdb;
	*resid_ptr = un->un_saved_cmd.saved_resid;
	un->un_dma_addr = un->un_saved_cmd.saved_dma_addr;
	un->un_dma_count = un->un_saved_cmd.saved_dma_count;
	un->un_flags &= ~SC_UNF_GET_SENSE;
	un->un_cmd = dsi->un_last_cmd;
	dsi->un_openf = OPEN;

	/*
	 * Special processing for Adaptec ACB-4000 disk controller.
	 */
	ssr = (struct scsi_ext_sense *)dsi->un_sense;
	if (dsi->un_ctype == CTYPE_ACB4000 ||  ssr->type != 0x70) {
		srintr_adaptec(dsi);
	}

	/*
	 * Log error information.
	 */
	dsi->un_err_resid = *resid_ptr;
	dsi->un_status = ssr->key;
	dsi->un_err_code = ssr->error_code;

	dsi->un_err_blkno = (ssr->info_1 << 24) | (ssr->info_2 << 16) |
			    (ssr->info_3 << 8)	| ssr->info_4;
	if (dsi->un_err_blkno == 0 ||  !(ssr->adr_val)) {
		/* No supplied block number, use original value */
		EPRINTF("srintr_ran_sense:  synthesizing block number\n");
		dsi->un_err_blkno = un->un_blkno;
	}

#ifdef	SRDEBUG
	/* dump sense info on screen */
	if (sr_debug > 1) {
		sr_error_message(un, dsi);
		printf("\n");
	}
#endif	SRDEBUG
}


/*
** Cleanup after running request sense command to see why the real
** command failed.
*/
/*ARGSUSED*/
static
sr_fix_block(c, dsi, un, resid, err_type)
	register struct scsi_ctlr *c;
	register struct scsi_disk *dsi;
	register struct scsi_unit *un;
	u_int resid;
	short err_type;
{
	register int i;

	if (err_type == SE_RETRYABLE) {
		if (dsi->un_bad_index == 0)
			goto SR_SAVE;

		/*
		 * Search bad block log for match.  If found,
		 * increment counter.  If reporting theshold passed,
		 * inform user.	 If remap threshold passed, reassign
		 * block.  Otherwise, just log it.
		 */
		for (i = 0; i < dsi->un_bad_index; i++) {

			/* Didn't find it, try next entry */
			if (dsi->un_bad[i].block != dsi->un_err_blkno)
				continue;

			/* Found it, check if we need to reassign the block */
			if ((dsi->un_bad[i].retries)++ >= sr_max_fails) {

				printf("sr%d:  block 0x%x needs mapping\n",
					SRNUM(un), dsi->un_bad[i].block);
				err_type = SE_FATAL;
			}
			goto SR_FIX_BLK;
		}
SR_SAVE:
		if (dsi->un_bad_index == NBAD) {
			EPRINTF1("sr%d:	 no storage for error logging\n",
			    SRNUM(un));
			return;
		}
		EPRINTF("si_fix_block:	logging marginal block\n");
		i = dsi->un_bad_index++;
		dsi->un_bad[i].block = dsi->un_err_blkno;
		dsi->un_bad[i].retries = 1;
	}


SR_FIX_BLK:
	/* Check if need to warn user */
	if (dsi->un_bad[i].retries > sr_fails) {
		printf("sr%d:  warning, abs. block %d has failed %d times\n",
		       SRNUM(un), dsi->un_bad[i].block,
		       dsi->un_bad[i].retries);
	}
}


/*
** Cleanup after running reassign block command.
*/
/*ARGSUSED*/
static
srintr_ran_reassign(un, dsi, resid_ptr)
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
	register u_int *resid_ptr;
{
	/*
	 * Check if reassign command failed.
	 */
	un->un_flags &= ~SC_UNF_GET_SENSE;
	dsi->un_openf = OPEN;
	if (un->un_scb.chk)
		printf("sr%d:  reassign block failed\n", SRNUM(un));

	un->un_scb = un->un_saved_cmd.saved_scb;
	un->un_cdb = un->un_saved_cmd.saved_cdb;
	*resid_ptr = un->un_saved_cmd.saved_resid;
	un->un_dma_addr = un->un_saved_cmd.saved_dma_addr;
	un->un_dma_count = un->un_saved_cmd.saved_dma_count;
	un->un_flags &= ~SC_UNF_GET_SENSE;
	un->un_cmd = dsi->un_last_cmd;
	dsi->un_openf = OPEN;
}


/*
** This routine performs raw read operations.  It is called from the
** device switch at normal priority.  It uses a per-unit buffer for the
** operation.
*/
/*ARGSUSED*/
srread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct scsi_unit *un;
	register int unit;

	DPRINTF("srread:\n");
	unit = SRUNIT(dev);
	if (unit >= ncdrom) {
		EPRINTF("srread:  invalid unit\n");
		return (ENXIO);
	}
	if ((uio->uio_fmode & FSETBLK) == 0 &&
	    (uio->uio_offset & (DEV_BSIZE - 1)) != 0) {
		EPRINTF1("srread:  block address not modulo %d\n",
			DEV_BSIZE);
		return (EINVAL);
	}
	if (uio->uio_iov->iov_len & (DEV_BSIZE - 1)) {
		EPRINTF1("srread:  block length not modulo %d\n",
			DEV_BSIZE);
		return (EINVAL);
	}
	un = &srunits[unit];
	return (physio(srstrategy, &un->un_rbuf, dev, B_READ, minphys, uio));
}

#ifdef NOTDEF

/*
** This routine performs raw write operations.	It is called from the
** device switch at normal priority.  It uses a per-unit buffer for the
** operation.
*/
srwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct scsi_unit *un;
	register int unit;

	DPRINTF("srwrite:\n");
	unit = SRUNIT(dev);
	if (unit >= ncdrom) {
		EPRINTF("srwrite:  invalid unit\n");
		return (ENXIO);
	}
	if ((uio->uio_fmode & FSETBLK) == 0 &&
	    (uio->uio_offset & (DEV_BSIZE - 1)) != 0)
		EPRINTF1("srwrite:  block address not modulo %d\n",
			DEV_BSIZE);
		return (EINVAL);
	}
	if (uio->uio_iov->iov_len & (DEV_BSIZE - 1)) {
		EPRINTF1("srwrite:  block length not modulo %d\n",
			DEV_BSIZE);
		return (EINVAL);
	}
	un = &srunits[unit];
	return (physio(srstrategy, &un->un_rbuf, dev, B_WRITE, minphys, uio));
}
#endif NOTDEF

/*
** This routine implements the ioctl calls.  It is called
** from the device switch at normal priority.
*/
/*ARGSUSED*/
srioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	register caddr_t data;
	int flag;
{
	extern char *strcpy();
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
	register struct dk_info *info;
	register struct dk_diag *diag;
	register int unit;
	short	sony;

	DPRINTF("srioctl:\n");
	unit = SRUNIT(dev);
	if (unit >= ncdrom) {
		EPRINTF("srioctl:  invalid unit\n");
		return (ENXIO);
	}
	if (!(un = &srunits[unit]) ||
	    !(dsi = &srdisk[unit])) {
		EPRINTF("srioctl: invalid open state\n");
		return (ENXIO);
	};

	sony = (bcmp((caddr_t)sr_inq[unit].cdrom_vid, "SONY", 4) == 0);

	switch (cmd) {

	/*
	** Return info concerning the controller.
	*/
	 case DKIOCINFO:
		DPRINTF("srioctl:  get info\n");
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
		DPRINTF("srioctl:  get geometry\n");
		*(struct dk_geom *)data = dsi->un_g;
		return (0);

	/*
	 * Get error status from last command.
	 */
	case DKIOCGDIAG:
		EPRINTF("srioctl:  get error status\n");
		diag = (struct dk_diag *) data;
		diag->dkd_errcmd  = dsi->un_last_cmd;
		diag->dkd_errsect = dsi->un_err_blkno;
		diag->dkd_errno = dsi->un_status;
		/* diag->dkd_errno   = dsi->un_err_code; */
		dsi->un_last_cmd   = 0;	/* Reset */
		dsi->un_err_blkno  = 0;
		dsi->un_err_code   = 0;
		dsi->un_err_severe = 0;
		return (0);

	/*
	 * Run a generic command.
	 */
	case DKIOCSCMD:
		/* DPRINTF("srioctl:  run special command\n");	*/
		return (srioctl_cmd(dev, data, 0));

	case CDROMPAUSE:
		if (sony) {
			return (sr_pause_resume(dev, (caddr_t)1));
		} else {
			return (ENOTTY);
		}

	case CDROMRESUME:
		if (sony) {
			return (sr_pause_resume(dev, (caddr_t)0));
		} else {
			return (ENOTTY);
		}

	case CDROMPLAYMSF:
		if (sony) {
			return (sr_play_msf(dev, data));
		} else {
			return (ENOTTY);
		}

	case CDROMPLAYTRKIND:
		if (sony) {
			return (sr_play_trkind(dev, data));
		} else {
			return (ENOTTY);
		}

	case CDROMREADTOCHDR:
		if (sony) {
			return (sr_read_tochdr(dev, data));
		} else {
			return (ENOTTY);
		}

	case CDROMREADTOCENTRY:
		if (sony) {
			return (sr_read_tocentry(dev, data));
		} else {
			return (ENOTTY);
		}

	case CDROMSTOP:
		if (sony) {
			return (sr_start_stop(dev, (caddr_t)0));
		} else {
			return (ENOTTY);
		}

	case CDROMSTART:
		if (sony) {
			return (sr_start_stop(dev, (caddr_t)1));
		} else {
			return (ENOTTY);
		}

	case FDKEJECT:
	case CDROMEJECT:
		if (sony) {
			return (sr_eject(dev));
		} else {
			return (ENOTTY);
		}

	case CDROMVOLCTRL:
		if (sony) {
			return (sr_volume_ctrl(dev, data));
		} else {
			return (ENOTTY);
		}

	case CDROMSUBCHNL:
		if (sony) {
			return (sr_read_subchannel(dev, data));
		} else {
			return (ENOTTY);
		}

	/*
	 * this driver does not support these ioctl command for now.
	 * will support it in the later version.
	 */

	case CDROMREADMODE2:
	case CDROMREADMODE1:
		return (EIO);

	/*
	 * Handle unknown ioctls here.
	 */
	default:
		EPRINTF("srioctl:  unknown ioctl\n");
		return (ENOTTY);
	}
}


/*
** Run a command for srioctl.
*/

/*ARGSUSED*/
static int
srioctl_cmd(dev, data, flag)
	dev_t dev;
	caddr_t data;
	int	flag;
{
/*	register struct dk_cmd *com; */
	register struct scsi_unit *un;
	register struct uscsi_cmd	*scmd;
	char	*cdb;
	int	blkno;
	int err, options;
	int s;
	int	unit;

	DPRINTF("srioctl_cmd:\n");
	unit = SRUNIT(dev);
	un = &srunits[unit];

/*	com = (struct dk_cmd *)data; */
	scmd = (struct uscsi_cmd *)data;
	cdb = scmd->uscsi_cdb;
	blkno = 0;

#ifdef	SRDEBUG
	if (sr_debug > 1) {
/*		printf("srioctl_cmd:  cmd= %x  blk= %x	cnt= %x	 ",
		    com->dkc_cmd, com->dkc_blkno, com->dkc_secnt);
		printf("buf addr= %x  buflen= 0x%x\n",
			com->dkc_bufaddr, com->dkc_buflen); */
		printf("srioctl_cmd:  cmd= %x  blk= %x	cnt= %x	 ",
			cdb_cmd(cdb), blkno, scmd->uscsi_buflen);
		printf("buf addr= %x  buflen= 0x%x\n",
			scmd->uscsi_bufaddr, scmd->uscsi_buflen);
	}
#endif	SRDEBUG
	/*
	 * Set options.
	 */
	options = 0;
	if (scmd->uscsi_flags & USCSI_SILENT) {
		options |= SD_SILENT;
#ifdef		SRDEBUG
		if (sr_debug > 0)
			options &= ~SD_SILENT;
#endif		SRDEBUG
	}
	if (scmd->uscsi_flags & USCSI_ISOLATE)
		options |= SD_NORETRY;

	/*
	 * Process the special ioctl command.
	 */
	switch (cdb_cmd(cdb)) {
	case SC_READ:
		DPRINTF("srioctl_cmd:  read\n");
		options |= SD_DVMA_RD;
		blkno = cdb0_blkno(cdb);
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = srcmd(dev, SC_SPECIAL_READ, blkno,
			    (int)scmd->uscsi_buflen,
			    (caddr_t)scmd->uscsi_bufaddr, options, (caddr_t)0);
		(void) splx(s);
		break;

#ifdef OLDCODE
	case SC_MODE_SELECT:
		EPRINTF("srioctl_cmd:  mode select\n");
		options |= SD_DVMA_WR;
		s = splr(pritospl(un->un_mc->mc_intpri));
		blkno = cdb0_blkno(cdb);
		err = srcmd(dev, SC_MODE_SELECT, blkno,
			    (int)scmd->uscsi_buflen,
			    (caddr_t)scmd->uscsi_bufaddr,
			    options, (caddr_t)0);
		(void) splx(s);
#ifdef		SRDEBUG
		if (sr_debug > 2) {
			sr_print_buffer((u_char *)scmd->uscsi_bufaddr,
					scmd->uscsi_buflen);
			printf("\n");
		}
#endif		SRDEBUG
		break;
#endif	OLDCODE

	case SC_MODE_SENSE:
		EPRINTF("srioctl_cmd:  mode sense\n");
		options |= SD_DVMA_RD;
		s = splr(pritospl(un->un_mc->mc_intpri));
		blkno = cdb0_blkno(cdb);
		err = srcmd(dev, SC_MODE_SENSE, blkno,
			    (int)scmd->uscsi_buflen,
			    (caddr_t)scmd->uscsi_bufaddr,
			    options, (caddr_t)0);
		(void) splx(s);
#ifdef		SRDEBUG
		if (sr_debug > 2) {
			sr_print_buffer((u_char *)scmd->uscsi_bufaddr,
					(int)scmd->uscsi_buflen);
			printf("\n");
		}
#endif		SRDEBUG
		break;

	default:
		EPRINTF1("srioctl_cmd: command is 0x%x\n",
		    cdb_cmd(cdb));
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = srcmd(dev, (int)cdb_cmd(cdb), 0, 0, (caddr_t)0, options,
			    (caddr_t)scmd);
		(void) splx(s);
		break;

/*	default:
		EPRINTF1("srioctl_cmd:	unknown command %x\n", com->dkc_cmd);
		return (EINVAL); */
		/* break; */
	}
	if (err) {
		EPRINTF("srioctl_cmd:  ioctl cmd failed\n");
		return (EIO);
	}
	return (0);
}

/*
** This routine locks the cdrom door and prevent medium removal.
*/

static int
sr_medium_removal(dev, flag)
dev_t dev;
int flag;
{
	struct	uscsi_cmd *com;
	char	*cdb;
	int	rtn;

	/*
	 *	So as not to worry about being swapped out, we dynamically
	 *	allocate memory for the uscsi_cmd structure
	 */

	if ((com = (struct uscsi_cmd *)kmem_alloc(sizeof (struct uscsi_cmd)))
	    == (struct uscsi_cmd *)NULL) {
		return (ENOMEM);
	}
	bzero((char *)com, sizeof (struct uscsi_cmd));

	if ((cdb = (char *)kmem_alloc(6)) == (char *)NULL) {
		return (ENOMEM);
	}

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

	rtn = srioctl_cmd(dev, (caddr_t)com, 0);

	kmem_free((caddr_t)com, sizeof (struct uscsi_cmd));
	kmem_free((caddr_t)cdb, 6);

	return (rtn);

}

/*
** This routine does a pause or resume to the cdrom player. Effects
** only audio play operation.
*/
static int
sr_pause_resume(dev, data)
dev_t	dev;
caddr_t data;
{
	int	flag;
	struct	uscsi_cmd *com;
	char	*cdb;
	int	rtn;

	/*
	 *	So as not to worry about being swapped out, we dynamically
	 *	allocate memory for the uscsi_cmd structure
	 */

	if ((com = (struct uscsi_cmd *)kmem_alloc(sizeof (struct uscsi_cmd)))
	    == (struct uscsi_cmd *)NULL) {
		return (ENOMEM);
	}
	bzero((char *)com, sizeof (struct uscsi_cmd));

	if ((cdb = (char *)kmem_alloc(10)) == (char *)NULL) {
		return (ENOMEM);
	}


	flag = (int)data;
	bzero(cdb, 10);
	cdb[0] = SCMD_PAUSE_RESUME;
	cdb[8] = (flag == 0) ? 1 : 0;
	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 10;
	com->uscsi_bufaddr = 0;
	com->uscsi_buflen = 0;
	com->uscsi_flags = DK_DIAGNOSE|DK_SILENT;

	rtn =  srioctl_cmd(dev, (caddr_t)com, 0);

	kmem_free((caddr_t)com, sizeof (struct uscsi_cmd));
	kmem_free((caddr_t)cdb, 10);

	return (rtn);
}

/*
** This routine plays audio by msf
*/
static int
sr_play_msf(dev, data)
dev_t	dev;
caddr_t data;
{
	struct	uscsi_cmd *com;
	char	*cdb;
	int	rtn;
	register struct cdrom_msf	*msf;

	/*
	 *	So as not to worry about being swapped out, we dynamically
	 *	allocate memory for the uscsi_cmd structure
	 */

	if ((com = (struct uscsi_cmd *)kmem_alloc(sizeof (struct uscsi_cmd)))
	    == (struct uscsi_cmd *)NULL) {
		return (ENOMEM);
	}
	bzero((char *)com, sizeof (struct uscsi_cmd));

	if ((cdb = (char *)kmem_alloc(10)) == (char *)NULL) {
		return (ENOMEM);
	}

	msf = (struct cdrom_msf *)data;
	bzero(cdb, 10);
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

	rtn = srioctl_cmd(dev, (caddr_t)com, 0);

	kmem_free((caddr_t)com, sizeof (struct uscsi_cmd));
	kmem_free((caddr_t)cdb, 10);

	return (rtn);
}

/*
** This routine plays audio by track/index
*/
static int
sr_play_trkind(dev, data)
dev_t	dev;
caddr_t data;
{
	struct	uscsi_cmd *com;
	char	*cdb;
	register struct cdrom_ti	*ti;
	int	err;

	/*
	 *	So as not to worry about being swapped out, we dynamically
	 *	allocate memory for the uscsi_cmd structure
	 */

	if ((com = (struct uscsi_cmd *)kmem_alloc(sizeof (struct uscsi_cmd)))
	    == (struct uscsi_cmd *)NULL) {
		return (ENOMEM);
	}
	bzero((char *)com, sizeof (struct uscsi_cmd));

	if ((cdb = (char *)kmem_alloc(10)) == (char *)NULL) {
		return (ENOMEM);
	}


	ti = (struct cdrom_ti *)data;
	bzero(cdb, 10);
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

	err = srioctl_cmd(dev, (caddr_t)com, 0);

	kmem_free((caddr_t)com, sizeof (struct uscsi_cmd));
	kmem_free((caddr_t)cdb, 10);

	return (err);
}

/*
** This routine starts the drive, stops the drive or ejects the disc
*/
static int
sr_start_stop(dev, data)
dev_t	dev;
caddr_t data;
{
	struct	uscsi_cmd *com;
	char	*cdb;
	int	rtn;

	/*
	 *	So as not to worry about being swapped out, we dynamically
	 *	allocate memory for the uscsi_cmd structure
	 */

	if ((com = (struct uscsi_cmd *)kmem_alloc(sizeof (struct uscsi_cmd)))
	    == (struct uscsi_cmd *)NULL) {
		return (ENOMEM);
	}
	bzero((char *)com, sizeof (struct uscsi_cmd));

	if ((cdb = (char *)kmem_alloc(6)) == (char *)NULL) {
		return (ENOMEM);
	}


	bzero(cdb, 6);
	cdb[0] = SR_START_STOP;
	cdb[1] = 0;	/* immediate bit is set to 0 for now */
	cdb[4] = (u_char)data;

	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 6;
	com->uscsi_bufaddr = 0;
	com->uscsi_buflen = 0;
	com->uscsi_flags = DK_DIAGNOSE|DK_SILENT;

	rtn = srioctl_cmd(dev, (caddr_t)com, 0);

	kmem_free((caddr_t)com, sizeof (struct uscsi_cmd));
	kmem_free((caddr_t)cdb, 6);

	return (rtn);
}

/*
** This routine ejects the CDROM disc
*/
/*ARGSUSED*/
static int
sr_eject(dev)
dev_t	dev;
{
	int	err;

	/* first, unlocks the eject */
	if ((err = sr_medium_removal(dev, SR_REMOVAL_ALLOW)) != 0) {
		return (err);
	}

	if ((err = srcmd(dev, SR_MEDIA_REMOV, 0, SR_MEDIA_unlock,
	    (caddr_t)0, 0, (caddr_t)0)) != 0) {
		return (err);
	}

	/* then ejects the disc */
/*	if ((err = (sr_start_stop(dev, (caddr_t)2))) == 0) {
		sr_ejected(dev);
	}
*/
	err = srcmd(dev, SR_START_STOP, 0, SR_STOP_eject, (caddr_t)0, 0,
			(caddr_t)0);
	return (err);
}

/*
** This routine control the audio output volume
*/
#ifdef NOTSKIP
#define	VolBufLng	20
static int
sr_volume_ctrl(dev, data)
dev_t	dev;
caddr_t data;
{
	struct	uscsi_cmd cblk, *com = &cblk;
	char	cdb[10];
	struct cdrom_volctrl	*vol;
	caddr_t buffer;
	int	rtn;
	caddr_t	tmp_ptr;

	DPRINTF("in sr_volume_ctrl\n");

	if ((buffer = SRIOPBALLOC(VolBufLng+4)) == (caddr_t)0) {
		return (ENOMEM);
	}
	tmp_ptr = buffer;
	buffer = (caddr_t)roundup((u_int)buffer, sizeof (int));

	vol = (struct cdrom_volctrl *)data;
	bzero(cdb, 10);
	cdb[0] = SCMD_CD_PLAYBACK_CONTROL; /* 0xc9  vendor unique command */
	cdb[7] = 0;
	cdb[8] = 18;	/* length of control buffer passed */

	/*
	** fill in the input data. Set the output channel 0, 1 to
	** output port 0, 1 respectively. Set output channel 2, 3 to mute
	** The function only adjusts the output volume for channels 0 and 1.
	*/
	bzero(buffer, VolBufLng);
	buffer[10] = 0x01;
	buffer[11] = vol->channel0;
	buffer[12] = 0x02;
	buffer[13] = vol->channel1;
	DPRINTF2("sr-volume ch0: 0x%x ch1:0x%x\n", vol->channel0,
	    vol->channel1);

	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 10;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 18;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_WRITE;

	rtn = (srioctl_cmd(dev, (caddr_t)com, 0));
	SRIOPBFREE(tmp_ptr, VolBufLng+4);

	return (rtn);
}
#endif NOTSKIP
/*
** This routine control the audio output volume
*/
static int
sr_volume_ctrl(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct	uscsi_cmd *com;
	char	*cdb;
	struct cdrom_volctrl	*vol;
	caddr_t	buffer;
	caddr_t	tmp_ptr;
	int	rtn;

	DPRINTF("in sr_volume_ctrl\n");
	/*
	 *	So as not to worry about being swapped out, we dynamically
	 *	allocate memory for the uscsi_cmd structure
	 */

	if ((com = (struct uscsi_cmd *)kmem_alloc(sizeof (struct uscsi_cmd)))
	    == (struct uscsi_cmd *)NULL) {
		return (ENOMEM);
	}
	bzero((char *)com, sizeof (struct uscsi_cmd));

	if ((cdb = (char *)kmem_alloc(6)) == (char *)NULL) {
		return (ENOMEM);
	}

	if ((buffer = SRIOPBALLOC(20)) == (caddr_t)0) {
		return (ENOMEM);
	}
	tmp_ptr = buffer;
	buffer = (caddr_t)roundup((u_int)buffer, sizeof (int));

	vol = (struct cdrom_volctrl *)data;
	bzero(cdb, 6);
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
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_WRITE;

	rtn = (srioctl_cmd(dev, (caddr_t)com, 0));
	SRIOPBFREE(tmp_ptr, 20);

	kmem_free((caddr_t)com, sizeof (struct uscsi_cmd));
	kmem_free((caddr_t)cdb, 6);

	return (rtn);
}

static int
sr_read_subchannel(dev, data)
dev_t	dev;
caddr_t	data;
{
	struct	uscsi_cmd *com;
	char	*cdb;
	caddr_t	buffer;
	int	rtn;
	struct	cdrom_subchnl	*subchnl;
	caddr_t	tmp_ptr;

	DPRINTF("in sr_read_subchannel\n");

	/*
	 *	So as not to worry about being swapped out, we dynamically
	 *	allocate memory for the uscsi_cmd structure
	 */

	if ((com = (struct uscsi_cmd *)kmem_alloc(sizeof (struct uscsi_cmd)))
	    == (struct uscsi_cmd *)NULL) {
		return (ENOMEM);
	}
	bzero((char *)com, sizeof (struct uscsi_cmd));

	if ((cdb = (char *)kmem_alloc(10)) == (char *)NULL) {
		return (ENOMEM);
	}

	if ((buffer = SRIOPBALLOC(16)) == (caddr_t)0) {
		return (ENOMEM);
	}


	tmp_ptr = buffer;
	buffer = (caddr_t)roundup((u_int)buffer, sizeof (int));

	subchnl = (struct cdrom_subchnl *)data;
	bzero(cdb, 10);
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

	rtn = (srioctl_cmd(dev, (caddr_t)com, 0));
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
	SRIOPBFREE(tmp_ptr, 16);

	kmem_free((caddr_t)com, sizeof (struct uscsi_cmd));
	kmem_free((caddr_t)cdb, 10);

	return (rtn);
}

static int
sr_read_tochdr(dev, data)
dev_t	dev;
caddr_t data;
{
	struct	uscsi_cmd *com;
	char	*cdb;
	int	rtn;
	caddr_t buffer;
	struct cdrom_tochdr	*hdr;
	caddr_t tmp_ptr;

	DPRINTF("in sr_read_tochdr.\n");

	/*
	 *	So as not to worry about being swapped out, we dynamically
	 *	allocate memory for the uscsi_cmd structure
	 */

	if ((com = (struct uscsi_cmd *)kmem_alloc(sizeof (struct uscsi_cmd)))
	    == (struct uscsi_cmd *)NULL) {
		return (ENOMEM);
	}
	bzero((char *)com, sizeof (struct uscsi_cmd));

	if ((cdb = (char *)kmem_alloc(10)) == (char *)NULL) {
		return (ENOMEM);
	}

	if ((buffer = SRIOPBALLOC(4)) == (caddr_t)0) {
		return (ENOMEM);
	}
	tmp_ptr = buffer;
	buffer = (caddr_t)roundup((u_int)buffer, sizeof (int));

	hdr = (struct cdrom_tochdr *)data;
	bzero(cdb, 10);
	cdb[0] = SCMD_READ_TOC;
	/*
	 * for now, byte 6 is set to 1, assuming all cdrom's track
	 * starts with one. When the Sony/Hitachi drive conforms to
	 * SCSI specification, this byte will set to 0.
	 */
/*	cdb[6] = 0x01; */
	/*
	 * byte 7, 8 are the allocation length. In this case, it is 4 bytes
	 */
	cdb[8] = 0x04;

	com->uscsi_cdb = cdb;
	com->uscsi_cdblen = 10;

	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 0x04;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = srioctl_cmd(dev, (caddr_t)com, 0);

	hdr->cdth_trk0 = buffer[2];
	hdr->cdth_trk1 = buffer[3];

	SRIOPBFREE(tmp_ptr, 4);
	kmem_free((caddr_t)com, sizeof (struct uscsi_cmd));
	kmem_free((caddr_t)cdb, 10);

	return (rtn);
}

/*
** This routine reads the toc of the disc and returns the information
** of a particular track. The track number is specified by the ioctl
** caller.
*/
static int
sr_read_tocentry(dev, data)
dev_t	dev;
caddr_t data;
{
	struct	uscsi_cmd *com;
	char	*cdb;
	struct cdrom_tocentry	*entry;
	caddr_t buffer, tmp_ptr;
	int	lba;
	int	rtn, rtn1;

	DPRINTF("in sr_read_tocentry.\n");

	/*
	 *	So as not to worry about being swapped out, we dynamically
	 *	allocate memory for the uscsi_cmd structure
	 */

	if ((com = (struct uscsi_cmd *)kmem_alloc(sizeof (struct uscsi_cmd)))
	    == (struct uscsi_cmd *)NULL) {
		return (ENOMEM);
	}
	bzero((char *)com, sizeof (struct uscsi_cmd));

	if ((cdb = (char *)kmem_alloc(10)) == (char *)NULL) {
		return (ENOMEM);
	}

	if ((buffer = SRIOPBALLOC(12)) == (caddr_t)0) {
		return (ENOMEM);
	}
	tmp_ptr = buffer;
	buffer = (caddr_t)roundup((u_int)buffer, sizeof (int));

	entry = (struct cdrom_tocentry *)data;
	if (!(entry->cdte_format & (CDROM_LBA | CDROM_MSF))) {
		return (EINVAL);
	}
	bzero(cdb, 10);

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
	com->uscsi_buflen = 0xC;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = (srioctl_cmd(dev, (caddr_t)com, 0));
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
		SRIOPBFREE(tmp_ptr, 12);
		kmem_free((caddr_t)com, sizeof (struct uscsi_cmd));
		kmem_free((caddr_t)cdb, 10);

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
		bzero(cdb, 10);
		cdb[0] = SCMD_READ_HEADER;
		cdb[2] = (u_char)((lba >> 24) & 0xFF);
		cdb[3] = (u_char)((lba >> 16) & 0xFF);
		cdb[4] = (u_char)((lba >> 8) & 0xFF);
		cdb[5] = (u_char)(lba & 0xFF);
		cdb[7] = 0x00;
		cdb[8] = 0x08;

		com->uscsi_buflen = 0x08;

		rtn1 = (srioctl_cmd(dev, (caddr_t)com, 0));
		if (rtn1) {
			SRIOPBFREE(tmp_ptr, 12);
			kmem_free((caddr_t)com, sizeof (struct uscsi_cmd));
			kmem_free((caddr_t)cdb, 10);
			return (rtn1);
		}
		entry->cdte_datamode = buffer[0];

	} else {
		entry->cdte_datamode = -1;
	}

	SRIOPBFREE(tmp_ptr, 12);
	kmem_free((caddr_t)com, sizeof (struct uscsi_cmd));
	kmem_free((caddr_t)cdb, 10);

	return (rtn);
}

static void
sr_setup_openflag(flag, dev_num)
struct	cdrom_open_flag	*flag;
u_char	dev_num;
{
	if ((flag->cdopen_flag0 == 0) &&
	    (flag->cdopen_flag1 == 0)) {
		flag->cdopen_flag0 = dev_num;
	} else if ((flag->cdopen_flag0 != dev_num) &&
		   (flag->cdopen_flag1 != dev_num)) {
		if (flag->cdopen_flag0 == 0) {
			flag->cdopen_flag0 = dev_num;
		} else if (flag->cdopen_flag1 == 0) {
			flag->cdopen_flag1 = dev_num;
		} else {
			printf("sropen: sr_open_flag corrupted\n");
		}
	}
}

static u_char sr_adaptec_keys[] = {
	0, 4, 4, 4,  2, 4, 4, 4,	/* 0x00-0x07 */
	4, 4, 4, 4,  4, 4, 4, 4,	/* 0x08-0x0f */
	3, 3, 3, 3,  3, 3, 3, 1,	/* 0x10-0x17 */
	1, 1, 5, 5,  1, 1, 1, 1,	/* 0x18-0x1f */
	5, 5, 5, 5,  5, 5, 5, 5,	/* 0x20-0x27 */
	6, 6, 6, 6,  6, 6, 6, 6,	/* 0x28-0x30 */
};
#define	MAX_ADAPTEC_KEYS \
	(sizeof (sr_adaptec_keys))


static char *sr_adaptec_error_str[] = {
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
	0
};
#define	MAX_ADAPTEC_ERROR_STR \
	(sizeof (sr_adaptec_error_str)/sizeof (sr_adaptec_error_str[0]))


static char *sr_emulex_error_str[] = {
	"no sense",			/* 0x00 */
	"recovered error",		/* 0x01 */
	"not ready",			/* 0x02 */
	"medium error",			/* 0x03 */
	"hardware error: not ready",	/* 0x04 */
	"illegal request",		/* 0x05 */
	"unit attention",		/* 0x06 */
	"multiple drives selected",	/* 0x07 */
	"lun failure",			/* 0x08 */
	"servo error",			/* 0x09 */
	"",				/* 0x0a */
	"aborted command",		/* 0x0b */
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
#define	MAX_EMULEX_ERROR_STR \
	(sizeof (sr_emulex_error_str)/sizeof (sr_emulex_error_str[0]))


static char *sr_key_error_str[] = SENSE_KEY_INFO;
#define	MAX_KEY_ERROR_STR \
	(sizeof (sr_key_error_str)/sizeof (sr_key_error_str[0]))


static char *sr_cmds[] = {
	"test unit ready",		/* 0x00 */
	"rezero",			/* 0x01 */
	"<bad cmd>",			/* 0x02 */
	"request sense",		/* 0x03 */
	"format",			/* 0x04 */
	"<bad cmd>",			/* 0x05 */
	"<bad cmd>",			/* 0x06 */
	"reassign",			/* 0x07 */
	"read",				/* 0x08 */
	"<bad cmd>",			/* 0x09 */
	"write",			/* 0x0a */
	"seek",				/* 0x0b */
	"<bad cmd>",			/* 0x0c */
	"<bad cmd>",			/* 0x0d */
	"<bad cmd>",			/* 0x0e */
	"<bad cmd>",			/* 0x0f */
	"<bad cmd>",			/* 0x10 */
	"<bad cmd>",			/* 0x11 */
	"inquiry",			/* 0x12 */
	"<bad cmd>",			/* 0x13 */
	"<bad cmd>",			/* 0x14 */
	"mode select",			/* 0x15 */
	"reserve",			/* 0x16 */
	"release",			/* 0x17 */
	"copy",				/* 0x18 */
	"<bad cmd>",			/* 0x19 */
	"mode sense",			/* 0x1a */
	"start/stop unit(LoEj)",	/* 0x1b */
	"<bad cmd>",			/* 0x1c */
	"send diagnostic",		/* 0x1d */
	"prevent-allow removal",	/* 0x1e */
	"<bad cmd>",			/* 0x1f */
};
#define	MAX_SR_CMDS \
	(sizeof (sr_cmds)/sizeof (sr_cmds[0]))


/*
** Translate Adaptec non-extended sense status in to
** extended sense format.  In other words, generate
** sense key.
*/
static
srintr_adaptec(dsi)
	register struct scsi_disk *dsi;
{
	register struct scsi_sense *s;
	register struct scsi_ext_sense *ssr;

	EPRINTF("srintr_adaptec\n");

	/* Reposition failed block number for extended sense. */
	(u_int)ssr = (u_int)s = dsi->un_sense;
	ssr->info_1 = 0;
	ssr->info_2 = s->high_addr;
	ssr->info_3 = s->mid_addr;
	ssr->info_4 = s->low_addr;

	/* Reposition error code for extended sense. */
	ssr->error_code = s->code;

	/* Synthesize sense key for extended sense. */
	if (s->code < MAX_ADAPTEC_KEYS) {
		ssr->key = sr_adaptec_keys[s->code];
	}
}


/*
** Return the text string associated with the sense key value.
*/
static char *
sr_print_key(key_code)
	register u_char key_code;
{
	static char *unknown_key = "unknown key";
	if ((key_code > MAX_KEY_ERROR_STR -1) ||
	    sr_key_error_str[key_code] == NULL) {

		return (unknown_key);
	}
	return (sr_key_error_str[key_code]);
}


/*
** Return the text string associated with the secondary
** error code, if availiable.
*/
static char *
sr_print_error(dsi, error_code)
	register struct scsi_disk *dsi;
	register u_char error_code;
{
	static char *unknown_error = "unknown error";
	switch (dsi->un_ctype) {
	case CTYPE_TOSHIBA:
	case CTYPE_MD21:
	case CTYPE_CCS:
		if ((MAX_EMULEX_ERROR_STR > error_code) &&
		    sr_emulex_error_str[error_code] != NULL)
			return (sr_emulex_error_str[error_code]);
		break;

	case CTYPE_ACB4000:
		if (MAX_ADAPTEC_ERROR_STR > error_code &&
		    sr_adaptec_error_str[error_code] != NULL)
			return (sr_adaptec_error_str[error_code]);
		break;
	}
	return (unknown_error);
}


/*
** Print the sense key and secondary error codes
** and dump out the sense bytes.
*/
#ifdef	SRDEBUG
static
sr_error_message(un, dsi)
	register struct scsi_unit *un;
	register struct scsi_disk *dsi;
{
	register struct scsi_ext_sense *ssr;
	register u_char	  *cp;
	register int i;
	register u_char error_code;

	/* If error messages are being suppressed, exit. */
	if (dsi->un_options & SD_SILENT)
		return;

	ssr = (struct scsi_ext_sense *)cp = (
		struct scsi_ext_sense *)dsi->un_sense;
	error_code = ssr->error_code;
	printf("sr%d error:  sense key(0x%x): %s",
		un - srunits, ssr->key, sr_print_key(ssr->key));
	if (error_code != 0) {
		printf(",  error code(0x%x): %s",
		       error_code, sr_print_error(dsi, error_code));
	}

	printf("\n	   sense = ");
	for (i = 0; i < sizeof (struct scsi_ext_sense); i++)
		printf("%x  ", *cp++);
	printf("\n");
}


static
sr_print_cmd(un)
	register struct scsi_unit *un;
{
	register int x;
	register u_char *y = (u_char *)&(un->un_cdb);

	printf("sr%d:  failed cmd =  ", SRUNIT(un->un_unit));
	for (x = 0; x < CDB_GROUP0; x++)
		printf("%x  ", *y++);
	printf("\n");
}
#endif	SRDEBUG

sr_makecom_all(cdb, scsi_cdb)
char	*cdb;
struct	scsi_cdb	*scsi_cdb;
{
	int	group;

	group = ((unsigned char)cdb_cmd(cdb)) >> 5;
	scsi_cdb->scc_cmd = cdb_cmd(cdb);
	scsi_cdb->scc_lun = 0;
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
	case 6: /* edu: only for now. eventually, we will not */
		/*      support group 6. */
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
		DPRINTF1("makecom_all: does not support group %d\n",
		    group);
	}
}

static
srerrmsg(un, bp, action)
	struct scsi_unit *un;
	struct buf *bp;
	char *action;
{
	register struct scsi_ext_sense *ssr;
	register struct scsi_disk *dsi;
	char *cmdname;
	u_char error_code;
	int blkno_flag = 1;		/* if false, blkno is meaningless */

	dsi = &srdisk[dkunit(bp)];
	ssr = (struct scsi_ext_sense *)dsi->un_sense;

	/* If error messages are being suppressed, exit. */
	if (dsi->un_options & SD_SILENT)
		return;

	/* Decode command name */
	if (un->un_cmd < MAX_SR_CMDS) {
		cmdname = sr_cmds[un->un_cmd];
	} else {
		cmdname = "unknown cmd";
	}

	/* If not a check condition error, block number is invalid */
	if (! un->un_scb.chk)
		blkno_flag = 0;


	printf("sr%d%c:	 %s %s",
		SRNUM(un), LPART(bp->b_dev) + 'a', cmdname, action);
	if (blkno_flag)
		printf(",  block %d", dsi->un_err_blkno);

	error_code = ssr->error_code;
	printf("\n	 sense key(0x%x): %s",
	       ssr->key, sr_print_key(ssr->key));
	if (error_code != 0) {
		printf(",  error code(0x%x): %s\n",
		       error_code, sr_print_error(dsi, error_code));
	} else {
		printf("\n");
	}

}
#endif	NSR > 0
