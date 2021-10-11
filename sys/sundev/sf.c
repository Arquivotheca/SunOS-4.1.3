/*      @(#)sf.c 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include "sf.h"
#if NSF > 0

#define SFDEBUG 1		/* Allow compiling of debug code */
#define REL4			/* enable release 4.00 mods */

/*
 * SCSI driver for SCSI disks.
 */
#ifndef REL4
#include "sf.h"
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
#include "../sundev/sfreg.h"

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
#include <sundev/sfreg.h>
#endif REL4

#if defined(REL4)  &&  defined(SF_FORMAT)
#include <vm/faultcode.h>
#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h> 

#endif defined(REL4)  &&  defined(SF_FORMAT)


#define SFDTYPE(flags)	((flags & 0xff00) >> 8)
/* #define MAX_RETRIES 		 4	/* retry limit */
#define MAX_RETRIES 		 1	/* retry limit */
/* #define MAX_RESTORES 		 4	/* rezero unit limit (after retries) */
#define MAX_RESTORES 		 1	/* rezero unit limit (after retries) */
#define MAX_FAILS 		20 	/* soft errors before reassign */
#define MAX_LABEL_RETRIES  	 1
#define MAX_LABEL_RESTORES 	 1
#define MAX_BUSY		10
#define EL_RETRY		 1	/* Error msg threshold, retries */
#define EL_REST			 0	/* Error msg threshold, restores */
#define EL_FAILS 		10 	/* Error msg threshold, soft errors */


#define LPART(dev)		(dev & (NLPART -1))
#define SFUNIT(dev)		((dev >> 3) & (NUNIT -1))
#define SFNUM(un)		(un - sfunits)


#ifdef SFDEBUG
short sf_debug = 0;		/*
				 * 0 = normal operation
				 * 1 = extended error info only
				 * 2 = debug and error info
				 * 3 = all status info
				 */
/* Handy debugging 0, 1, and 2 argument printfs */
#define SF_PRINT_CMD(un) \
	if (sf_debug > 1) sf_print_cmd(un)
#define DPRINTF(str) \
	if (sf_debug > 1) printf(str)
#define DPRINTF1(str, arg1) \
	if (sf_debug > 1) printf(str,arg1)
#define DPRINTF2(str, arg1, arg2) \
	if (sf_debug > 1) printf(str,arg1,arg2)

/* Handy extended error reporting 0, 1, and 2 argument printfs */
#define EPRINTF(str) \
	if (sf_debug) printf(str)
#define EPRINTF1(str, arg1) \
	if (sf_debug) printf(str,arg1)
#define EPRINTF2(str, arg1, arg2) \
	if (sf_debug) printf(str,arg1,arg2)
#define DEBUG_DELAY(cnt) \
	if (sf_debug)  DELAY(cnt)

#else SFDEBUG
#define SF_PRINT_CMD(un)
#define DPRINTF(str)
#define DPRINTF1(str, arg2)
#define DPRINTF2(str, arg1, arg2)
#define EPRINTF(str)
#define EPRINTF1(str, arg2)
#define EPRINTF2(str, arg1, arg2)
#define DEBUG_DELAY(cnt)
#endif SFDEBUG

extern char *strncpy();

extern struct scsi_unit sfunits[];
extern struct scsi_unit_subr scsi_unit_subr[];
extern struct scsi_floppy sfdisk[];
extern int nsfdisk;
extern int scsi_debug;
short sf_retry = 1;	/*
/* short sf_retry = EL_RETRY;	/*
				 * Error message threshold, retries.
				 * Make it global so manufacturing can
				 * override setting.
				 */

#ifdef	SFDEBUG
sf_print_buffer(y, count)
	register u_char *y;
	register int count;
{
	register int x;

	for (x = 0; x < count; x++)
		printf("%x  ", *y++);
	printf("\n");
}
#endif	SFDEBUG


/*
 * Return a pointer to this unit's unit structure.
 */
sfunitptr(md)
	register struct mb_device *md;
{
	return ((int)&sfunits[md->md_unit]);
}


static
sftimer(dev)
	register dev_t dev;
{
	register struct scsi_floppy *dsi;
	register struct scsi_unit *un;
	register struct scsi_ctlr *c;
	register u_int	unit;

	unit = SFUNIT(dev);
	un = &sfunits[unit];
	dsi = &sfdisk[unit];
	c = un->un_c;

/* 	DPRINTF("sftimer:\n"); */
	if (dsi->un_openf >= OPENING) {
#ifdef		SF_TIMEOUT
		if ((dsi->un_timeout != 0)  &&  (--dsi->un_timeout == 0)) {

			/* Process command timeout for normal I/O operations */
			printf("sf%d:  sftimer: I/O request timeout\n", unit);
			if ((*c->c_ss->scs_deque)(c, un)) {
				/* Can't Find CDB I/O request to kill, help! */
				printf("sf%d:  sftimer: can't abort request\n",
					unit);
				(*un->un_c->c_ss->scs_reset)(un->un_c, 0);
			}
		} else
			if (dsi->un_timeout != 0) {
/* 			DPRINTF2("sftimer:  running, open= %d, timeout= %d\n",
				 dsi->un_openf, dsi->un_timeout); */
			}
#endif		SF_TIMEOUT

	/* Process opening delay timeout */
	} else if ((dsi->un_timeout != 0)  &&  (--dsi->un_timeout == 0)) {
		DPRINTF("sftimer:  running...\n");
		wakeup((caddr_t)dev);
	}
	timeout(sftimer, (caddr_t)dev, 30*hz);
}


/*
 * Attach device (boot time).
 */
sfattach(md)
	register struct mb_device *md;
{
	register struct scsi_unit *un;
	register struct scsi_floppy *dsi;
	struct scsi_inquiry_data *sid;
	int i;

	dsi = &sfdisk[md->md_unit];
	un = &sfunits[md->md_unit];

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
	dsi->sf_spt = 0;
	dsi->sf_mdb = 0;
	dsi->sf_nhead = 0;
	dsi->sf_nblk = 0;
	dsi->sf_rate = 0xfa;
	switch (SFDTYPE(md->md_flags)) {
	default:
	case 0:
		dsi->sf_mdb = 0x1e;
		break;
	case 1:
		dsi->sf_mdb = 0x16;
		break;
	}
	
	/*
	 * Set default disk geometry and partition table.
	 * This is necessary so we can later open the device
	 * if we don't find it at probe time.
	 */
	bzero((caddr_t)&(dsi->un_g), sizeof (struct dk_geom));
	bzero((caddr_t)dsi->un_map, sizeof (struct dk_allmap));
	dsi->un_g.dkg_ncyl  = 78;
	dsi->un_g.dkg_acyl  = 0;
	dsi->un_g.dkg_pcyl  = 80;
	dsi->un_g.dkg_nhead = 2;
	dsi->un_g.dkg_nsect = 9;
	dsi->un_cyl_size = 9;

	/*
	 * Allocate space for request sense/inquiry buffer in
	 * Multibus memory.  Align it to a longword boundary.
	 */
	sid = (struct scsi_inquiry_data *)rmalloc(iopbmap,
		(long)(sizeof (struct scsi_inquiry_data) + 4));
	if (sid == NULL) {
		printf("sf%d:  sfattach: no space for inquiry data\n",
		       SFNUM(un));
		return;
	}

	while ((u_int)sid & 0x03)
		((u_int)sid)++;
	dsi->un_sense = (int)sid;

#ifdef notdef
	EPRINTF2("sf%d:  sfattach: buffer= 0x%x, ", SFNUM(un), (int)sid);
	EPRINTF2("dsi= 0x%x, un= 0x%x\n", dsi, un);
	DPRINTF2("sfattach: looking for lun %d on target %d\n",
		 LUN(md->md_slave), TARGET(md->md_slave));

#endif
	/*
	 * Test for unit ready.  The first test checks
	 * for a non-existant device.  The other tests
	 * wait for the drive to get ready.
	 */
	if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0) > 1) {
		DPRINTF("sfattach:  unit offline\n");
		return;
	}
	for (i = 0; i < MAX_BUSY; i++) {
		if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0) == 0) {
			goto SFATTACH_UNIT;

		} else if (un->un_scb.chk) {
			goto SFATTACH_UNIT;

		} else if (un->un_scb.busy  &&  !un->un_scb.is) {
			EPRINTF("sfattach:  unit busy\n");
			DELAY(4000000);		/* Wait 4 Sec. */

		} else if (un->un_scb.is) {
			EPRINTF("sfattach:  reservation conflict\n");
			break;
		}
	}
	DPRINTF("sfattach:  unit offline\n");
	return;

SFATTACH_UNIT:
	if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0) != 0) {
		DPRINTF("sfattach:  unit failed\n");
		return;
	}

	bzero((caddr_t)sid, (u_int)sizeof (struct scsi_inquiry_data));
	if (simple(un, SC_INQUIRY, (char *) sid - DVMA, 0,
			(int)sizeof (struct scsi_inquiry_data)) == 0) {
		/* Only CCS controllers support Inquiry command */
#ifdef		SFDEBUG
/* 		if (sf_debug > 2)
			sf_print_buffer((u_char *)sid, 32); */
#endif		SFDEBUG
		if (bcmp(sid->vid, "NCR HC F", 8) == 0) {
 			EPRINTF("sfattach:  NCR floppy controller found\n");
 			dsi->un_ctype = CTYPE_NCRFLOP;
 			dsi->un_flags = 0;	/* dis/reconnect turn on */
		} else {
			EPRINTF("sfattach:  CCS found\n");
			dsi->un_ctype = CTYPE_CCS;
			dsi->un_flags = 0;
		} 
        } else {
                EPRINTF("sfattach:  unknow conntroller type\n");
		dsi->un_ctype = CTYPE_UNKNOWN;
		return;
	}

}


/*
 * Read the label from the disk.
 * Return true if read was ok, false otherwise.
 */
static
getlabel(un, l, dev)
	register struct scsi_unit *un;
	register struct fk_label *l;
	dev_t dev;
{
	register int s;
	register struct scsi_floppy *sfp;
	int err;
	struct ms_med_parms {
		struct ccs_modesel_head_fl header;
		struct ccs_modesel_medium page5;
	} *ms_med_parms, *oms_med_parms;

 	
	sfp = &sfdisk[un->un_unit];
	/*
	 * SET DISK MEDIUM PARAMETERS (PAGE 5)
	 */
	ms_med_parms = (struct ms_med_parms *)rmalloc(iopbmap,
		(long)(sizeof (struct ms_med_parms) + 4));
	oms_med_parms = ms_med_parms;
	while ((u_int)ms_med_parms & 0x03)
		((u_int)ms_med_parms)++;
	bzero((char *)ms_med_parms, sizeof (struct ms_med_parms));
 	/* First get them */
	if (sfcmd(dev, SC_MODE_SENSE, 0x5, 
		     sizeof (struct ms_med_parms), 
		     (caddr_t)ms_med_parms, 0)) {
		EPRINTF("Can not get characteristics\n");
		return (ENXIO);
	}

               /* set page5 stuff */
	sfp->sf_rate = 0xfa;
	sfp->sf_spt = 9;
	ms_med_parms->header.medium = sfp->sf_mdb;
	ms_med_parms->page5.xfer_rate = sfp->sf_rate;
	ms_med_parms->page5.data_bytes = 512;
	ms_med_parms->page5.nhead = 2;
	ms_med_parms->page5.sec_trk = 9;
 	ms_med_parms->page5.ncyl = 0x50;
 	ms_med_parms->page5.ssn = 1;
	s = splr(pritospl(un->un_mc->mc_intpri));
	if (sfcmd(dev, SC_MODE_SELECT, 0x5, 
		     sizeof (struct ms_med_parms), 
		     (caddr_t)ms_med_parms, 0)) {
		EPRINTF("Can not set characteristics\n");
		rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
		  (u_long)oms_med_parms);
		(void) splx(s);
		return (ENXIO);
	}
	err = sfcmd(dev, SC_READ_LABEL, 1, 1, (caddr_t)l, 0);
	if (err == 0) {
		rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
				  (u_long)oms_med_parms);
		(void) splx(s);
		return (1);
	}
/* the stupid floppy could be high density so switch and try again */
	if (sfp->sf_mdb == 0x1e) {
		sfp->sf_rate = 0x1f4;
		sfp->sf_spt = 18;
	} else {
		sfp->sf_mdb = 0x1a;
		sfp->sf_rate = 0x1f4;
		sfp->sf_spt = 15;
	}
	ms_med_parms->page5.xfer_rate = sfp->sf_rate;
	ms_med_parms->page5.sec_trk = sfp->sf_spt;
	if (sfcmd(dev, SC_MODE_SELECT, 0x5, 
		     sizeof (struct ms_med_parms), 
		     (caddr_t)ms_med_parms, 0)) {
		EPRINTF("Can not set characteristics\n");
		sfp->sf_rate = 0xfa;
		rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
		  (u_long)oms_med_parms);
		(void) splx(s);
		return (ENXIO);
	}
	err = sfcmd(dev, SC_READ_LABEL, 1, 1, (caddr_t)l, 0);
	if (err == 0) {
		rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
				  (u_long)oms_med_parms);
		(void) splx(s);
		return (1);
	}
	(void) sfcmd(dev, SC_REZERO_UNIT, 0, 0, (caddr_t)0, 0);
	sfp->sf_rate = 0xfa;
	sfp->sf_spt = 9;
	if (sfp->sf_mdb == 0x1a)
		sfp->sf_mdb = 16;
	rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
		  (u_long)oms_med_parms);
	(void) splx(s);
	return (0);
}

static
uselabel(un, l, dev)
	register struct scsi_unit *un;
	register struct fk_label *l;
	dev_t dev;
	{       
	register struct scsi_floppy *sfp;
	struct ms_med_parms {
		struct ccs_modesel_head_fl header;
		struct ccs_modesel_medium page5;
	} *ms_med_parms, *oms_med_parms;
	

	un->un_present = 1;			/* "it's here..." */
	sfp = &sfdisk[un->un_unit];
/* set default geometry */
	sfp->sf_spt = 9;
	sfp->sf_nhead = 2;
	sfp->sf_nblk = 720 * 2;
	sfp->sf_xrate = 0xfa;
	sfp->sf_bsec = 512;
	sfp->sf_step = 1;
	sfp->un_g.dkg_ncyl = sfp->sf_nblk;
	sfp->un_g.dkg_acyl = 0;
	sfp->un_g.dkg_bcyl = 0;
	sfp->un_g.dkg_nhead = sfp->sf_nhead;
	sfp->un_g.dkg_bhead = 0;
	sfp->un_g.dkg_nsect = sfp->sf_spt;
	sfp->un_g.dkg_gap1 = 0;
	sfp->un_g.dkg_gap2 = 0;
	sfp->un_g.dkg_intrlv =0;
	sfp->un_g.dkg_pcyl = 0;
        /*
         * Old labels don't have pcyl in them, so we make a guess at it.
         */
	if (sfp->un_g.dkg_pcyl == 0)
                sfp->un_g.dkg_pcyl = sfp->un_g.dkg_ncyl + sfp->un_g.dkg_acyl;
	sfp->un_cyl_size = sfp->un_g.dkg_nhead * sfp->un_g.dkg_nsect;

	/*
	 * Fill in partition table.
	 */
        /* default to three partitions for now */
	sfp->un_map[0].dkl_cylno = 0;
	sfp->un_map[0].dkl_nblk = 8;
	sfp->un_map[2].dkl_cylno = 0;
	sfp->un_map[2].dkl_nblk = sfp->sf_nblk;
	sfp->un_map[6].dkl_cylno = 1;
	sfp->un_map[6].dkl_nblk = sfp->sf_nblk - 8;
	sfp = &sfdisk[un->un_unit];
	/*
	 * Check magic number of the label
	 */
	if ((l->fkl_magich != FKL_MAGIC) || (l->fkl_magicl != FKL_MAGIC) ) {
    		return (0);
	}

	switch (l->fkl_type) {
	case DSHSPT:
	    if (sfp->sf_rate == 0xfa) {
		    sfp->sf_spt = 9;
		    sfp->sf_nblk = 1440;
		    sfp->sf_xrate = 0xfa;
	    } else {
		    sfp->sf_spt = 18;
		    sfp->sf_nblk = 2880;
		    sfp->sf_xrate = 0x1f4;
	    }
	    sfp->sf_nhead = 2;
	    sfp->sf_bsec = 512;
	    sfp->sf_step = 1;
	    break;

	case SS8SPT:
	    sfp->sf_spt = 8;
	    sfp->sf_nhead = 1;
	    sfp->sf_nblk = 640;
	    sfp->sf_xrate = 0xfa;
	    sfp->sf_bsec = 512;
	    sfp->sf_step = 1;
	    break;

	case DS8SPT:
	    sfp->sf_spt = 8;
	    sfp->sf_nhead = 2;
	    sfp->sf_nblk = 640 * 2;
	    sfp->sf_xrate = 0xfa;
	    sfp->sf_bsec = 512;
	    sfp->sf_step = 1;
	    break;

	case SS9SPT:
	    sfp->sf_spt = 9;
	    sfp->sf_nhead = 1;
	    sfp->sf_nblk = 720;
	    sfp->sf_xrate = 0xfa;
	    sfp->sf_bsec = 512;
	    sfp->sf_step = 1;
	    break;

	case DS9SPT:
	    sfp->sf_spt = 9;
	    sfp->sf_nhead = 2;
	    sfp->sf_nblk = 720 * 2;
	    sfp->sf_xrate = 0xfa;
	    sfp->sf_bsec = 512;
	    sfp->sf_step = 1;
	    break;

	default:
	    sfp->sf_spt = 9;
	    sfp->sf_nhead = 2;
	    sfp->sf_nblk = 720 * 2;
	    sfp->sf_xrate = 0xfa;
	    sfp->sf_bsec = 512;
	    sfp->sf_step = 1;
	    EPRINTF2("sf%d: unknown label %x\n", un->un_unit, sfp->sf_mdb);
	    break;

	}
        /*
	 * Fill in disk geometry from label.
	 */
	un->un_present = 1;			/* "it's here..." */
	sfp->un_g.dkg_ncyl = sfp->sf_nblk;
	sfp->un_g.dkg_acyl = 0;
	sfp->un_g.dkg_bcyl = 0;
	sfp->un_g.dkg_nhead = sfp->sf_nhead;
	sfp->un_g.dkg_bhead = 0;
	sfp->un_g.dkg_nsect = sfp->sf_spt;
	sfp->un_g.dkg_gap1 = 0;
	sfp->un_g.dkg_gap2 = 0;
	sfp->un_g.dkg_intrlv =0;
	sfp->un_g.dkg_pcyl = 0;
        /*
         * Old labels don't have pcyl in them, so we make a guess at it.
         */
	if (sfp->un_g.dkg_pcyl == 0)
                sfp->un_g.dkg_pcyl = sfp->un_g.dkg_ncyl + sfp->un_g.dkg_acyl;
	sfp->un_cyl_size = sfp->un_g.dkg_nhead * sfp->un_g.dkg_nsect;

	/*
	 * Fill in partition table.
	 */
        /* default to three partitions for now */
	sfp->un_map[0].dkl_cylno = 0;
	sfp->un_map[0].dkl_nblk = 8;
	sfp->un_map[2].dkl_cylno = 0;
	sfp->un_map[2].dkl_nblk = sfp->sf_nblk;
	sfp->un_map[6].dkl_cylno = 1;
	sfp->un_map[6].dkl_nblk = sfp->sf_nblk - 8;
	/*
	 * SET DISK MEDIUM PARAMETERS (PAGE 5)
	 */
	ms_med_parms = (struct ms_med_parms *)rmalloc(iopbmap,
		(long)(sizeof (struct ms_med_parms) + 4));
	oms_med_parms = ms_med_parms;
	while ((u_int)ms_med_parms & 0x03)
		((u_int)ms_med_parms)++;
	bzero((char *)ms_med_parms, sizeof (struct ms_med_parms));
/* First get them */
	if (sfcmd(dev, SC_MODE_SENSE, 0x5, 
		     sizeof (struct ms_med_parms), 
		     (caddr_t)ms_med_parms, 0)) {
		EPRINTF("Can not get characteristics\n");
		return (ENXIO);
	}



               /* set page5 stuff */
	ms_med_parms->header.medium = sfp->sf_mdb;
	ms_med_parms->page5.xfer_rate = sfp->sf_rate;
	ms_med_parms->page5.nhead = sfp->sf_nhead;
	ms_med_parms->page5.sec_trk = sfp->sf_spt;
	ms_med_parms->page5.data_bytes = 512;
	ms_med_parms->page5.ncyl = 0x50;

	if (sfcmd(dev, SC_MODE_SELECT, 0x5, 
		     sizeof (struct ms_med_parms), 
		     (caddr_t)ms_med_parms, 0)) {
		EPRINTF("Can not set characteristics\n");
		rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
		  (u_long)oms_med_parms);
		return (ENXIO);
	}
	rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
		  (u_long)oms_med_parms);
	return (0);
}

/*
 * Run a command in polled mode.
 * Return true if successful, false otherwise.
 */
static
simple(un, cmd, dma_addr, secno, nsect)
	register struct scsi_unit *un;
	register int cmd, secno, nsect, dma_addr;
	{       
	register struct scsi_cdb *cdb;
	register struct scsi_ctlr *c;
	register int err;

	/*
	 * Grab and clear the command block.
	 */
	c = un->un_c;
	cdb = &un->un_cdb;
	bzero((caddr_t)cdb, CDB_GROUP0);

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


/*
 * This routine opens a disk.  Note that we can handle disks
 * that make an appearance after boot time.
 */
/*ARGSUSED*/
sfopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct scsi_unit *un;
	register struct fk_label *l;
	register struct dk_map *lp;
	register int unit;
	register struct scsi_floppy *dsi;
	register struct scsi_inquiry_data *sid;
	struct fk_label *old_l;

	unit = SFUNIT(dev);
	if (unit >= nsfdisk) {
		EPRINTF("sfopen:  illegal unit\n");
		return (ENXIO);
	}

	un = &sfunits[unit];
	dsi = &sfdisk[unit];
	if (un->un_mc == 0) {
		EPRINTF("sfopen:  disk not atached\n");
		return (ENXIO);
	}

	/*
	 * If command timeouts not activated yet, switch it on.
	 */
#ifdef	SF_TIMEOUT
	if (dsi->un_timer == 0) {
		EPRINTF("sfopen:  starting timer\n");
		dsi->un_timer++;
		timeout(sftimer, (caddr_t)dev, 30*hz);
	}
#endif	SF_TIMEOUT

	/*
	 * Check for special opening mode.
	 */
		EPRINTF("sfopen:  opening device\n");
		lp = &dsi->un_map[LPART(dev)];
		lp->dkl_nblk = 1536;

		dsi->un_openf = OPENING;
		dsi->un_flags = SC_UNF_NO_DISCON;	/* no disconnects */
		if (sfcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0, 0)  &&
		    dsi->un_openf == CLOSED) {
			EPRINTF("sfopen:  not ready\n");
			dsi->un_timer = 0;
			untimeout(sftimer, (caddr_t)dev);
			return (ENXIO);
		}

		dsi->un_openf = OPENING;
		(void) sfcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0, 0);
		dsi->un_openf = OPENING;
		if (sfcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0, 0)) {
			EPRINTF("sfopen:  not ready\n");
			dsi->un_timer = 0;
			untimeout(sftimer, (caddr_t)dev);
			return (ENXIO);
		}
		dsi->un_openf = OPENING;
		sid = (struct scsi_inquiry_data *)dsi->un_sense;
		bzero((caddr_t)sid, (u_int)sizeof (struct scsi_inquiry_data));
		if (sfcmd(dev, SC_INQUIRY, 0, 
			sizeof (struct scsi_inquiry_data), (caddr_t)sid, 0)) {
			EPRINTF("sfopen:  unknown controller type\n");
			dsi->un_ctype = CTYPE_UNKNOWN;
			return (ENXIO);
		} else {
#ifdef			SFDEBUG
/* 			if (sf_debug > 2)
				sf_print_buffer((u_char *)sid, 32); */
#endif			SFDEBUG
			dsi->un_flags = 0;
			if (bcmp(sid->vid, "NCR HC F", 8) == 0) {
/* 				EPRINTF("sfopen: NCR floppy controller found\n"); */
				dsi->un_ctype = CTYPE_NCRFLOP;
			} else {
				EPRINTF("sfopen:  CCS found\n");
				dsi->un_ctype = CTYPE_CCS;
			}
		}


		/* Allocate space for label on longword boundary */
		old_l =	l =
			(struct fk_label *)rmalloc(iopbmap, (long)(SECSIZE +4));
		if (l == NULL) {
			printf("sf%d:  no space for disk label\n", SFNUM(un));
			return (ENXIO);
		}
		while ((u_int)l & 0x03)
			((u_int)l)++;

		DPRINTF("sfopen:  reading label\n");
		dsi->un_openf = OPEN;
		dsi->un_restores = 1/* MAX_RESTORES */;
		if (getlabel(un, l, dev)) {
			if (uselabel(un, l, dev)) {
			rmfree(iopbmap, (long)(SECSIZE + 4), (u_long)old_l);
			if (flag & O_NDELAY) {
				EPRINTF("sfopen:  suppressing label error\n");
				return (0);
			}
			return (0);
		}
		rmfree(iopbmap, (long)(SECSIZE + 4), (u_long)old_l);
		return (0);
	}
	return (0);
}

/*
 * Dummy close routine
 */

/*ARGSUSED*/
sfclose(dev,flag)
	dev_t dev;
	int flag;
{
	return (0);
}

/*
 * This routine returns the size of a logical partition.  It is called
 * from the device switch at normal priority.
 */
sfsize(dev)
	register dev_t dev;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	register struct scsi_floppy *dsi;

	if (SFUNIT(dev) >= nsfdisk) {
		EPRINTF("sfsize:  illegal unit\n");
		return (-1);
	}
	un = &sfunits[SFUNIT(dev)];
	/* DPRINTF("sfsize:\n"); */

	if (un->un_present) {
	/* 	DPRINTF("sfsize:  getting info\n"); */
		dsi = &sfdisk[SFUNIT(dev)];
		lp = &dsi->un_map[LPART(dev)];
		return (lp->dkl_nblk);
	} else {
/* 		EPRINTF("sfsize:  unit not present\n"); */
		return (-1);
	}
}


/*
 * This routine is the focal point of internal commands to the controller.
 * NOTE: this routine assumes that all operations done before the disk's
 * geometry is defined.
 */
sfcmd(dev, cmd, block, count, addr, options)
	dev_t dev;
	u_int cmd, block, count;
	caddr_t addr;
	int options;
{
	register struct scsi_floppy *dsi;
	register struct scsi_unit *un;
	register struct buf *bp;
	int s;
	long b_flags;

	un = &sfunits[SFUNIT(dev)];
	dsi = &sfdisk[SFUNIT(dev)];
	bp = &un->un_sbuf;

	/* DPRINTF("sfcmd:\n"); */
	/* s = splr(pritospl(un->un_mc->mc_intpri)); */
	s = splr(pritospl(3));
	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANTED;
		/* DPRINTF1("sfcmd:  sleeping...,  bp= 0x%x\n", bp); */
		(void) sleep((caddr_t) bp, PRIBIO);
	}

	bp->b_flags = B_BUSY | B_READ;
	(void) splx(s);
	/* DPRINTF1("sfcmd:  waking...,  bp= 0x%x\n", bp); */
/* 	DPRINTF1("sfcmd: waking up with addr=0x%x\n", addr); */

	un->un_scmd = cmd;
	bp->b_dev = dev;
	bp->b_bcount = count;
	bp->b_blkno = block;
	bp->b_un.b_addr = addr;
	dsi->un_options = options;

#ifdef  SF_FORMAT
	if (options & (SD_DVMA_RD | SD_DVMA_WR)) {
/*   		DPRINTF2("sfcmd:  addr= 0x%x size= 0x%x\n", addr, count); */
		bp->b_flags |= B_PHYS;
		bp->b_proc = u.u_procp;
		u.u_procp->p_flag |= SPHYSIO;
		/*
		 * Fault lock the address range of the buffer.
		 */
		if (as_fault(u.u_procp->p_as, bp->b_un.b_addr,
		   	 (u_int)count, F_SOFTLOCK,
			 (options & SD_DVMA_WR) ? S_WRITE : S_READ))  {

			EPRINTF("sfcmd:  as_fault error1\n");
			b_flags = bp->b_flags = B_ERROR;
			goto SFCMD_EXIT;
		}
	}
#endif  SF_FORMAT
	/*
	 * Execute the I/O request.
	 */
	sfstrategy(bp);
	(void) iowait(bp);

#ifdef  SF_FORMAT
	/*
	 * Release memory and DVMA resources.
	 */
	if (options & (SD_DVMA_RD | SD_DVMA_WR)) {
		if (as_fault(u.u_procp->p_as, bp->b_un.b_addr, 
			 (u_int)bp->b_bcount, F_SOFTUNLOCK,
			 (options & SD_DVMA_WR) ? S_WRITE : S_READ)) {

			EPRINTF("sfcmd:  as_fault error2\n");
			b_flags = bp->b_flags = B_ERROR;
			goto SFCMD_EXIT;
		}
		s = splr(pritospl(3));
		/* s = splr(pritospl(un->un_mc->mc_intpri));*/
		u.u_procp->p_flag &= ~SPHYSIO;
		bp->b_flags &= ~(B_BUSY | B_PHYS);
		(void) splx(s);
	}

SFCMD_EXIT:
#endif  SF_FORMAT

	bp->b_flags &= ~B_BUSY;
	b_flags = bp->b_flags;
	if (bp->b_flags & B_WANTED) {
		/* DPRINTF1("sfcmd:  waking...,  bp= 0x%x\n", bp); */
		wakeup((caddr_t)bp);
	}
	return (b_flags & B_ERROR);
}


/*
 * This routine is the high level interface to the disk.  It performs
 * reads and writes on the disk using the buf as the method of communication.
 * It is called from the device switch for block operations and via physio()
 * for raw operations.  It is called at normal priority.
 */
sfstrategy(bp)
	register struct buf *bp;
{
	register struct scsi_unit *un;
	register struct mb_device *md;
	register struct dk_map *lp;
	register u_int bn, cyl;
	register struct diskhd *dh;
	register struct scsi_floppy *dsi;
	register int s;
	int unit;

	/* DPRINTF("sfstrategy:\n"); */
	unit = dkunit(bp);
	if (unit >= nsfdisk) {
		EPRINTF("sfstrategy: invalid unit\n");
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	un = &sfunits[unit];
	md = un->un_md;
	dsi = &sfdisk[unit];
	lp = &dsi->un_map[LPART(bp->b_dev)];/* which partition */
	bn = bp->b_blkno;

	if (bp != &un->un_sbuf)  {
		if ((un->un_present == 0)  ||  (lp->dkl_nblk == 0)) {
			EPRINTF("sfstrategy:  unit not present\n");
			EPRINTF2("sfstrategy:  bn= 0x%x  nblk= 0x%x\n",
				bn, lp->dkl_nblk);
			bp->b_flags |= B_ERROR;
			iodone(bp);
			return;
		}
		/* Check for EOM */
		if (un->un_present  &&  (bn >= lp->dkl_nblk)) {
			EPRINTF("sfstrategy:  invalid block addr\n");
			bp->b_resid = bp->b_bcount;
			iodone(bp);
			return;
		}
	}

	if ((bn >= dsi->un_cyl_start)  &&  (bn < dsi->un_cyl_end)  &&
	    ( (int)lp == dsi->un_lp1)) {
				/* DPRINTF("WIN  "); */
		bp->b_cylin = dsi->un_cylin_last;

	} else {
 				/* DPRINTF("LOSE\n"); */
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

	/* call unit start routine to queue up device, if it
	 * currently isn't queued.  Does not actually start command
	 */
	(*un->un_c->c_ss->scs_ustart)(un);

	/* call start routine to run the next SCSI command */
        /* Now we start the command */
	(*un->un_c->c_ss->scs_start)(un);
	(void) splx(s);
}


/*
 * Set up a transfer for the controller
 */
sfstart(bp, un)
	register struct buf *bp;
	register struct scsi_unit *un;
{
	register struct dk_map *lp;
	register struct scsi_floppy *dsi;
	register int nblk;

/* 	DPRINTF("sfstart:\n"); */
		
	dsi = &sfdisk[dkunit(bp)];
	lp = &dsi->un_map[LPART(bp->b_dev)];

	/* Process internal I/O requests */
	if (bp == &un->un_sbuf) {
/* 		DPRINTF("sfstart:  internal I/O\n"); */
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
				/* DPRINTF("sfstart:  read\n"); */
		un->un_cmd = SC_READ;
	} else {
				/* DPRINTF("sfstart:  write\n"); */
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
	return (1);
}


/*
 * Make a cdb for disk I/O.
 */
sfmkcdb(un)
	struct scsi_unit *un;
{
	register struct scsi_cdb *cdb;
	register struct scsi_floppy *dsi;

/* 	DPRINTF("sfmkcdb:\n"); */

	dsi = &sfdisk[un->un_unit];
	cdb = &un->un_cdb;
	bzero((caddr_t)cdb, CDB_GROUP0);
	cdb->cmd = un->un_cmd;
	cdb->lun = un->un_lun;

	switch (un->un_cmd) {
	case SC_READ:
/* 		DPRINTF("sfmkcdb:  read\n"); */
		dsi->un_timeout = 4;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0ADDR(cdb, un->un_blkno);
		FORMG0COUNT(cdb, un->un_count);
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count << SECDIV;
		break;

	case SC_WRITE:
/* 		DPRINTF("sfmkcdb:  write\n"); */
		dsi->un_timeout = 4;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		FORMG0ADDR(cdb, un->un_blkno);
		FORMG0COUNT(cdb, un->un_count);
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count << SECDIV;
		break;

	case SC_WRITE_VERIFY:
/* 		DPRINTF("sfmkcdb:  write verify\n"); */
		dsi->un_timeout = 4;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		FORMG1ADDR(cdb, un->un_blkno);
		FORMG1COUNT(cdb, un->un_count);
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count << SECDIV;
		break;

	case SC_REQUEST_SENSE:
		DPRINTF("sfmkcdb:  request sense\n");
		dsi->un_timeout = 4;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0COUNT(cdb,  sizeof (struct scsi_sense));
		un->un_dma_addr = (int)dsi->un_sense - (int)DVMA;
		un->un_dma_count = sizeof (struct scsi_sense);
		bzero((caddr_t)(dsi->un_sense), sizeof (struct scsi_sense));
		return;

	case SC_SPECIAL_READ:
/* 		DPRINTF1("sfmkcdb:  special read blkno= 0x%x\n", un->un_blkno); */
		dsi->un_timeout = 4;
		un->un_cmd = cdb->cmd = SC_READ;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0ADDR(cdb, un->un_blkno);
		FORMG0COUNT(cdb, un->un_count >> SECDIV);
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count;
		break;

	case SC_SPECIAL_WRITE:
/* 		DPRINTF1("sfmkcdb:  special write blkno= 0x%x\n", un->un_blkno); */
		dsi->un_timeout = 4;
		un->un_cmd = cdb->cmd = SC_WRITE;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		FORMG0ADDR(cdb, un->un_blkno);
		FORMG0COUNT(cdb, un->un_count >> SECDIV);
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count;
		break;

	case SC_READ_LABEL:
		DPRINTF("sfmkcdb:  read label\n");
		dsi->un_timeout = 4;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		un->un_cmd = cdb->cmd = SC_READ;
		FORMG0ADDR(cdb, un->un_blkno);
		FORMG0COUNT(cdb,  1);
		un->un_dma_addr = (int)un->un_sbuf.b_un.b_addr - (int)DVMA;
		un->un_dma_count = SECSIZE;
		break;

	case SC_REZERO_UNIT:
		EPRINTF("sfmkcdb:  rezero unit\n");
		dsi->un_timeout = 4;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = 0;
		un->un_dma_count = 0;
		return;

	case SC_TEST_UNIT_READY:
		/* DPRINTF("sfmkcdb:  test unit ready\n"); */
		dsi->un_timeout = 2;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		un->un_dma_addr = 0;
		un->un_dma_count = 0;
		break;

	case SC_INQUIRY:
		DPRINTF("sfmkcdb:  inquiry\n");
		dsi->un_timeout = 2;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0COUNT(cdb, sizeof (struct scsi_inquiry_data));
		un->un_dma_addr = (int)dsi->un_sense - (int)DVMA;
		un->un_dma_count = sizeof (struct scsi_inquiry_data);
		bzero((caddr_t)(dsi->un_sense), sizeof (struct scsi_inquiry_data));
		break;

	case SC_FORMAT:
		EPRINTF("sfmkcdb:  format\n");
		DPRINTF1("sfmkcdb:  count= 0x%d\n", un->un_count);
		dsi->un_timeout = 60;
		un->un_dma_addr = un->un_baddr;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
/* 		cdb->fmt_parm_bits =  FPB_BFI; XXXXX ???*/
		cdb->fmt_parm_bits =  0;
		un->un_dma_count = un->un_count;
		cdb->fmt_interleave = 1;
		break;

	case SC_MODE_SELECT:
		EPRINTF("sfmkcdb:  mode select\n");
		dsi->un_timeout = 2;
		cdb->tag |= 0x10;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		FORMG0COUNT(cdb, un->un_count);
		un->un_dma_count = un->un_count;
		un->un_dma_addr = (int)un->un_sbuf.b_un.b_addr - (int)DVMA;
/* 		un->un_dma_addr = un->un_baddr; */
		break;

	case SC_MODE_SENSE:
/* 		EPRINTF("sfmkcdb:  mode sense\n"); */
		dsi->un_timeout = 2;
		un->un_flags |= (SC_UNF_RECV_DATA | dsi->un_flags);
		FORMG0COUNT(cdb, un->un_count);
		cdb->g0_addr1  = un->un_blkno;
		un->un_dma_count = un->un_count;
		un->un_dma_addr = (int)un->un_sbuf.b_un.b_addr - (int)DVMA;
/* 		un->un_dma_addr = un->un_baddr; */
		break;

	case SC_READ_DEFECT_LIST:
		EPRINTF("sfmkcdb:  read defect list- NOT SUPPORTED\n");
		dsi->un_timeout = 4;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		break;

	case SC_REASSIGN_BLOCK:
		EPRINTF1("sfmkcdb:  reassign block 0x%x NOT SUPPORTED\n", 
			    un->un_blkno);
		dsi->un_timeout = 4;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		break;
	default:
		EPRINTF("sfmkcdb:  unknown command\n");
		dsi->un_timeout = 4;
		un->un_flags = (un->un_flags & ~SC_UNF_RECV_DATA) |
				dsi->un_flags;
		break;
	}
	dsi->un_last_cmd = un->un_cmd;
	return;
}


/*
 * This routine handles controller interrupts.
 * It is always called at disk interrupt priority.
 */
typedef enum sfintr_error_resolution {	
	real_error, 		/* Hard error */
	psuedo_error, 		/* What looked like an error is actually OK */
	more_processing 	/* Recoverable error */
} sfintr_error_resolution;
sfintr_error_resolution sferror();

sfintr(c, resid, error)
	register struct scsi_ctlr *c;
	u_int resid;
	int error;
{
	register struct scsi_unit *un;
	register struct scsi_floppy *dsi;
	register struct buf *bp;
	register struct mb_device *md;
	int status = 0;
	int error_code = 0;
	u_char severe = DK_NOERROR;

	/* DPRINTF("sfintr:\n"); */
	un = c->c_un;
	md = un->un_md;
	bp = md->md_utab.b_forw;
	dsi = &sfdisk[SFUNIT(bp->b_dev)];
	dsi->un_timeout = 0;		/* Disable time-outs */

/* XXXX What it this? */
	if (md->md_dk >= 0) {
		dk_busy &= ~(1 << md->md_dk);
	}

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
				printf("sf%d:  scsi bus failure\n", SFNUM(un));
			}
			dsi->un_retries = dsi->un_restores = 0;
			dsi->un_err_severe = DK_FATAL;
			un->un_present = 0;
			dsi->un_openf = CLOSED;
			bp->b_flags |= B_ERROR;
			goto SFINTR_WRAPUP;
		}

		/*
		 * Opening disk, check if command failed.  If it failed
		 * close device.  Otherwise, it's open.
		 */
		if (dsi->un_openf == OPENING) {
			if (error == SE_NO_ERROR) {
				dsi->un_openf = OPEN;
				goto SFINTR_SUCCESS;
			} else if (error == SE_RETRYABLE) {
				DPRINTF("sfintr:  open failed\n");
				dsi->un_openf = OPEN_FAILED;
				bp->b_flags |= B_ERROR;
				goto SFINTR_WRAPUP;
			} else {
				EPRINTF("sfintr:  hardware failure\n");
				dsi->un_openf = CLOSED;
				bp->b_flags |= B_ERROR;
				goto SFINTR_WRAPUP;
			}
		}

		/*
		 * Rezero for failed command done, retry failed command
		 */
		if ((dsi->un_openf == RETRYING)  &&
		    (un->un_cdb.cmd == SC_REZERO_UNIT)) {

			if (error)
				printf("sf%d:  rezero failed\n", SFNUM(un));

			DPRINTF("sfintr:  rezero done\n");
			un->un_flags &= ~SC_UNF_GET_SENSE;
			un->un_cdb = un->un_saved_cmd.saved_cdb;
			un->un_dma_addr = un->un_saved_cmd.saved_dma_addr;
			un->un_dma_count = un->un_saved_cmd.saved_dma_count;
			un->un_cmd = dsi->un_last_cmd;
			sfmkcdb(un);
			(*c->c_ss->scs_cmd)(c, un, 1);
			return;
		}


		/*
		 * Command failed, need to run request sense command.
		 */
		if ((dsi->un_openf == OPEN)  &&  error) {
			sfintr_sense(dsi, un, resid);
			return;
		}

		/*
		 * Request sense command done, restore failed command.
		 */
		if (dsi->un_openf == SENSING) {
			DPRINTF("sfintr:  restoring sense\n");
			sfintr_ran_sense(un, dsi, &resid);
		}

		/*
		 * Reassign done, restore original state
		 */
		if (dsi->un_openf == MAPPING) {
			DPRINTF("sfintr:  restoring state\n");
			sfintr_ran_reassign(un, dsi, &resid);
		}

		EPRINTF2("sfintr:  retries= %d  restores= %d  ",
			 dsi->un_retries, dsi->un_restores);
		EPRINTF1("total= %d\n", dsi->un_total_errors);
		if ((dsi->un_openf == RETRYING)  &&  (error == 0)) {

			EPRINTF("sfintr:  ok\n\n");
			dsi->un_openf = OPEN;
			dsi->un_retries = dsi->un_restores = 0;
			dsi->un_err_severe = DK_RECOVERED;
			goto SFINTR_SUCCESS;
		}

		/*
		 * Process all other errors here
		 */
#ifdef notdef
		EPRINTF2("sfintr:  processing error,  error= %x  chk= %x",
			error, un->un_scb.chk);
		EPRINTF1("  busy= %x", un->un_scb.busy);
#endif
		EPRINTF2("  resid= %d (%d)\n", resid, un->un_dma_count);
		switch (sferror(c, un, dsi, bp, resid, error)) {
		case real_error:
			/* This error is FATAL ! */
/* 			DPRINTF("sfintr:  real error\n"); */
			SF_PRINT_CMD(un);
			dsi->un_retries = dsi->un_restores = 0;
			bp->b_flags |= B_ERROR;
			goto SFINTR_WRAPUP;

		case psuedo_error:
			/* A psuedo-error:  soft error reported by ctlr */
			DPRINTF("sfintr:  psuedo error\n");
			status = dsi->un_status;
			error_code = dsi->un_err_code;
			severe = dsi->un_err_severe;
			dsi->un_retries = dsi->un_restores = 0;
			goto SFINTR_SUCCESS;

		case more_processing:
			/* real error requiring error recovery */
/*			DPRINTF("stintr:  more processing\n"); */
			return;
		}
	}


	/*
	 * Handle successful Transfer.  Also, take care of 
	 * seek error problem by doing single sector I/O.
	 */
SFINTR_SUCCESS:
	dsi->un_status = status;
	dsi->un_err_code = error_code;
	dsi->un_err_severe = severe;

	if (dsi->un_sec_left) {
		EPRINTF("sfintr:  single sector writes\n");
		dsi->un_sec_left--;
		un->un_baddr += SECSIZE;
		un->un_blkno++;
		sfmkcdb(un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) != 0)
			printf("sf%d:  single sector I/O failed\n", SFNUM(un));
	}


	/*
	 * Handle I/O request completion (both sucessful and failed).
	 */
SFINTR_WRAPUP:
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
static sfintr_error_resolution
sferror(c, un, dsi, bp, resid, error)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct scsi_floppy *dsi;
	struct buf *bp;
	u_int resid;
	int error;
{
	register struct scsi_ext_sense *ssf;

	ssf = (struct scsi_ext_sense *)dsi->un_sense;
	/* DPRINTF("sferror:\n"); */

	/*
	 * Special processing for driver command timeout errors.
	 */
	if (error == SE_TIMEOUT) {
		EPRINTF("sferror:  command timeout error\n");
		dsi->un_status = SC_TIMEOUT;
		goto SFERROR_RETRY;
	}

	/*
	 * Check for various check condition errors.
	 */
	dsi->un_total_errors++;
	if (un->un_scb.chk) {

		switch (ssf->key) {
		case SC_RECOVERABLE_ERROR:
			dsi->un_err_severe = DK_CORRECTED;
			if (sf_retry == 0) {
				sferrmsg(un, bp, "recoverable");
			}
			if (dsi->un_options & SD_NORETRY) {
				dsi->un_err_severe = DK_CORRECTED;
				return (real_error);
			}
			return (psuedo_error);

		case SC_MEDIUM_ERROR:
			EPRINTF("sferror:  media error\n");
			goto SFERROR_RETRY;

		case SC_HARDWARE_ERROR:
			EPRINTF("sferror:  hardware error\n");
			goto SFERROR_RETRY;

		case SC_UNIT_ATTENTION:
			EPRINTF("sferror:  unit attention error\n");
			goto SFERROR_RETRY;

		case SC_NOT_READY:
			EPRINTF("sferror:  not ready\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		case SC_ILLEGAL_REQUEST:
			EPRINTF("sferror:  illegal request\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		case SC_VOLUME_OVERFLOW:
			EPRINTF("sferror:  volume overflow\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		case SC_WRITE_PROTECT:
			EPRINTF("sferror:  write protected\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		case SC_BLANK_CHECK:
			EPRINTF("sferror:  blank check\n");
			dsi->un_err_severe = DK_FATAL;
			return (real_error);

		default:
			/*
			 * Undecoded sense key.  Try retries and hope
			 * that will fix the problem.  Otherwise, we're
			 * dead.
			 */
			EPRINTF("sferror:  undecoded sense key error\n");
			SF_PRINT_CMD(un);
			dsi->un_err_severe = DK_FATAL;
			goto SFERROR_RETRY;
		}

	/*
	 * Process busy error.  Try retries and hope that
	 * it'll be ready soon.
	 */
	} else if (un->un_scb.busy  &&  !un->un_scb.is) {
		EPRINTF1("sf%d:  sferror: busy error\n", SFNUM(un));
		SF_PRINT_CMD(un);
		goto SFERROR_RETRY;

	/*
	 * Process reservation error.  Abort operation.
	 */
	} else if (un->un_scb.busy  &&  un->un_scb.is) {
		EPRINTF1("sf%d:  sferror: reservation conflict error\n",
			SFNUM(un));
		SF_PRINT_CMD(un);
		dsi->un_err_severe = DK_FATAL;
		return (real_error);

	/*
	 * Have left over residue data from last command.
	 * Do retries and hope this fixes it...
	 */
	} else  if (resid != 0) {
		EPRINTF1("sferror:  residue error, residue= %d\n", resid);
		SF_PRINT_CMD(un);
		goto SFERROR_RETRY;

	/*
	 * Have an unknown error.  Don't know what went wrong.
	 * Do retries and hope this fixes it...
	 */
	} else {
		EPRINTF("sferror:  unknown error\n");
		SF_PRINT_CMD(un);
		dsi->un_err_severe = DK_FATAL;
		goto SFERROR_RETRY;
	}

	/*
	 * Process command retries and rezeros here.
	 * Note, for on-line formatting, normal error
	 * recovery is inhibited.
	 */
SFERROR_RETRY:
	if (dsi->un_options & SD_NORETRY) {
		EPRINTF("sferror:  error recovery disabled\n");
		sferrmsg(un, bp, "failed, no retries");
		dsi->un_err_severe = DK_FATAL;
		return (real_error);
	}

	/*
	 * Command failed, retry it.
	 */
	if (dsi->un_retries++ < MAX_RETRIES) {
		if (((dsi->un_retries + dsi->un_restores) > sf_retry) ||
	    	    (dsi->un_restores != 0)) {
			sferrmsg(un, bp, "retry");
		}

		dsi->un_openf = RETRYING;
		sfmkcdb(un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) != 0) {
			printf("sf%d:  sferror: retry failed\n", SFNUM(un));
			return (real_error);
		}
		return (more_processing);

	/*
	 * Retries exhausted, try restore
	 */
	} else if (++dsi->un_restores < MAX_RESTORES) {
		if (dsi->un_restores != EL_REST)
			sferrmsg(un, bp, "restore");

		/*
		 * Save away old command state.
		 */
		un->un_saved_cmd.saved_cdb = un->un_cdb;
		un->un_saved_cmd.saved_dma_addr = un->un_dma_addr;
		un->un_saved_cmd.saved_dma_count = un->un_dma_count;
		dsi->un_openf = RETRYING;
		dsi->un_retries = 0;
		un->un_cmd = SC_REZERO_UNIT;
		sfmkcdb(un);
		(void) (*c->c_ss->scs_cmd)(c, un, 1);
		return (more_processing);

	/*
	 * Restores and retries exhausted, die!
	 */
	} else {
		/* complete failure */
		sferrmsg(un, bp, "failed");
		dsi->un_openf = OPEN;
		dsi->un_retries = 0;
		dsi->un_restores = 0;
		dsi->un_err_severe = DK_FATAL;
		return (real_error);
	}
}


/*
 * Command failed, need to run a request sense command to determine why.
 */
static
sfintr_sense(dsi, un, resid)
	register struct scsi_floppy *dsi;
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
	 * Note that s?start will call sfmkcdb, which
	 * will notice that the flag is set and not do
	 * the copy of the cdb, doing a request sense
	 * rather than the normal command.
	 */
/* 	DPRINTF("sfintr:  getting sense\n"); */
	un->un_flags |= SC_UNF_GET_SENSE;
	dsi->un_openf = SENSING;
	un->un_cmd = SC_REQUEST_SENSE;
	(*un->un_c->c_ss->scs_go)(un->un_md);
}


/*
 * Cleanup after running request sense command to see why the real
 * command failed.
 */
static
sfintr_ran_sense(un, dsi, resid_ptr)
	register struct scsi_unit *un;
	register struct scsi_floppy *dsi;
	u_int *resid_ptr;
{
	register struct scsi_ext_sense *ssf;

	/*
	 * Check if request sense command failed.  This should
	 * never happen!
	 */
	un->un_flags &= ~SC_UNF_GET_SENSE;
	dsi->un_openf = OPEN;
	if (un->un_scb.chk) {
		printf("sf%d:  request sense failed\n",
		       SFNUM(un));
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
	 * Log error information.
	 */
	ssf = (struct scsi_ext_sense *)dsi->un_sense;
	dsi->un_err_resid = *resid_ptr;
	dsi->un_status = ssf->key;
	dsi->un_err_code = ssf->error_code;

	dsi->un_err_blkno = (ssf->info_1 << 24) | (ssf->info_2 << 16) |
			    (ssf->info_3 << 8)  | ssf->info_4;
	if (dsi->un_err_blkno == 0  ||  !(ssf->adr_val)) {
		/* No supplied block number, use original value */
		EPRINTF("sfintr_ran_sense:  synthesizing block number\n");
		dsi->un_err_blkno = un->un_blkno;
	}

#ifdef	SFDEBUG
	/* dump sense info on screen */
	if (sf_debug > 1) {
		sf_error_message(un, dsi);
		printf("\n");
	}
#endif	SFDEBUG
}


/*
 * Cleanup after running reassign block command.
 */
static
sfintr_ran_reassign(un, dsi, resid_ptr)
	register struct scsi_unit *un;
	register struct scsi_floppy *dsi;
	register u_int *resid_ptr;
{
	/*
	 * Check if reassign command failed.
	 */
	un->un_flags &= ~SC_UNF_GET_SENSE;
	dsi->un_openf = OPEN;
	if (un->un_scb.chk)
		printf("sf%d:  reassign block failed\n", SFNUM(un));

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
 * This routine performs raw read operations.  It is called from the
 * device switch at normal priority.  It uses a per-unit buffer for the
 * operation.
 */
sfread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct scsi_unit *un;
	register int unit;

/* 	DPRINTF("sfread:\n"); */
	unit = SFUNIT(dev);
	if (unit >= nsfdisk) {
		EPRINTF("sfread:  invalid unit\n");
		return (ENXIO);
	}
	/*
	 * The following tests assume a block size which is power of 2.
	 * This allows a bit mask operation to be used instead of a
	 * divide operation thus saving considerable time.
	 */
	if (uio->uio_offset & (DEV_BSIZE -1)) {
		EPRINTF1("sfread:  block address not modulo %d\n",
			DEV_BSIZE);
		return (EINVAL);
	}
	if (uio->uio_iov->iov_len & (DEV_BSIZE -1)) {
		tracedump();
		EPRINTF2("sfread:  block length not modulo %d %d\n",
			DEV_BSIZE, uio->uio_iov->iov_len);
		return (EINVAL);
	}
	un = &sfunits[unit];
	return (physio(sfstrategy, &un->un_rbuf, dev, B_READ, minphys, uio));
}


/*
 * This routine performs raw write operations.  It is called from the
 * device switch at normal priority.  It uses a per-unit buffer for the
 * operation.
 */
sfwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct scsi_unit *un;
	register int unit;

	/* DPRINTF("sfwrite:\n"); */
	unit = SFUNIT(dev);
	if (unit >= nsfdisk) {
		EPRINTF("sfwrite:  invalid unit\n");
		return (ENXIO);
	}
	/*
	 * The following tests assume a block size which is power of 2.
	 * This allows a bit mask operation to be used instead of a
	 * divide operation thus saving considerable time.
	 */
	if (uio->uio_offset & (DEV_BSIZE -1)) {
		EPRINTF1("sfwrite:  block address not modulo %d\n",
			DEV_BSIZE);
		return (EINVAL);
	}
	if (uio->uio_iov->iov_len & (DEV_BSIZE -1)) {
		EPRINTF1("sfwrite:  block length not modulo %d\n",
			DEV_BSIZE);
		return (EINVAL);
	}
	un = &sfunits[unit];
	return (physio(sfstrategy, &un->un_rbuf, dev, B_WRITE, minphys, uio));
}


/*
 * This routine implements the ioctl calls.  It is called
 * from the device switch at normal priority.
 */
/*ARGSUSED*/
sfioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	register caddr_t data;
	int flag;
{
	register struct scsi_unit *un;
	register struct scsi_floppy *dsi;
	register struct dk_map *lp;
	register struct dk_info *info;
#ifdef SF_FORMAT
	register struct dk_conf *conf;
#endif SF_FORMAT
	register struct dk_diag *diag;
	register struct fdk_char *fchar;
	register int unit;
	register int i;
	int s;
	struct ms_med_parms {
		struct ccs_modesel_head_fl header;
		struct ccs_modesel_medium page5;
	} *ms_med_parms, *oms_med_parms;

/*  	DPRINTF("sfioctl:\n"); */
	unit = SFUNIT(dev);
	if (unit >= nsfdisk) {
		EPRINTF("sfioctl:  invalid unit\n");
		return (ENXIO);
	}
	un = &sfunits[unit];
	dsi = &sfdisk[unit];
	lp = &dsi->un_map[LPART(dev)];
	switch (cmd) {

	/*
	 * Return info concerning the controller.
	 */
	case DKIOCINFO:
		DPRINTF("sfioctl:  get info\n");
		info = (struct dk_info *)data;
		info->dki_ctlr = getdevaddr(un->un_mc->mc_addr);
		info->dki_unit = un->un_md->md_slave;

		switch (dsi->un_ctype) {
		case CTYPE_NCRFLOP:
			info->dki_ctype = DKC_NCRFLOPPY;
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
		DPRINTF("sfioctl:  get geometry\n");
		*(struct dk_geom *)data = dsi->un_g;
		return (0);

	/*
	 * Set the geometry of the specified unit.
	 */
	case DKIOCSGEOM:
		EPRINTF("sfioctl:  set geometry\n");
		dsi->un_g = *(struct dk_geom *)data;
		return (0);

	/*
	 * Return the map for the specified logical partition.
	 * This has been made obsolete by the get all partitions
	 * command.
	 */
	case DKIOCGPART:
 		DPRINTF("sfioctl:  get partition\n");
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
		EPRINTF("sfioctl:  set partitions\n");
		*lp = *(struct dk_map *)data;
		return (0);

	/*
	 * Return configuration info
	 */
	case DKIOCGCONF:
#ifdef  	SF_FORMAT
		DPRINTF("sfioctl:  get configuration info\n");
		conf = (struct dk_conf *)data;
		switch (dsi->un_ctype) {
		case CTYPE_NCRFLOP:
			conf->dkc_ctype = DKC_NCRFLOPPY;
			break;
		default:
			conf->dkc_ctype = DKC_SCSI_CCS;
			break;
		}
		(void) strncpy(conf->dkc_dname, "sf", DK_DEVLEN);
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
#endif  	SF_FORMAT
		return (0);

	/*
	 * Return the map for all logical partitions.
	 */
	case DKIOCGAPART:
		DPRINTF("sfioctl:  get all logical partitions\n");
		for (i = 0; i < NLPART; i++)
			((struct dk_map *)data)[i] = dsi->un_map[i];
		return (0);

	/*
	 * Set the map for all logical partitions.  We raise
	 * the priority just to make sure an interrupt doesn't
	 * come in while the map is half updated.
	 */
	case DKIOCSAPART:
		EPRINTF("sfioctl:  set all logical partitions\n");
		s = splr(pritospl(un->un_mc->mc_intpri));
		for (i = 0; i < NLPART; i++)
			dsi->un_map[i] = ((struct dk_map *)data)[i];
		(void) splx(s);
		return (0);

	/*
	 * Get error status from last command.
	 */
	case DKIOCGDIAG:
		EPRINTF("sfioctl:  get error status\n");
		diag = (struct dk_diag *) data;
		diag->dkd_errcmd  = dsi->un_last_cmd;
		diag->dkd_errsect = dsi->un_err_blkno;
		diag->dkd_errno = dsi->un_status;
		/* diag->dkd_errno   = dsi->un_err_code; */
#ifdef		SF_FORMAT
		diag->dkd_severe = dsi->un_err_severe;
#endif		SF_FORMAT

		dsi->un_last_cmd   = 0;	/* Reset */
		dsi->un_err_blkno  = 0;
		dsi->un_err_code   = 0;
		dsi->un_err_severe = 0;
		return (0);

	/*
	 * Run a generic command.
	 */
	case FDKIOCSCMD:
	case DKIOCSCMD:
#ifdef  SF_FORMAT
/*  		DPRINTF("sfioctl:  run special command\n"); */
		return (sfioctl_cmd(dev, un, dsi, data));
#endif  SF_FORMAT

	case FDKIOGCHAR:
		DPRINTF("sfioctl: get characteristics\n");
		fchar = (struct fdk_char *)data;
/* request page 5 stuff */
		ms_med_parms = (struct ms_med_parms *)rmalloc(iopbmap,
				(long)(sizeof (struct ms_med_parms) + 4));
		oms_med_parms = ms_med_parms;
		while ((u_int)ms_med_parms & 0x03)
			((u_int)ms_med_parms)++;
		bzero((char *)ms_med_parms, sizeof (struct ms_med_parms));
		s = splr(pritospl(un->un_mc->mc_intpri));
		if (sfcmd(dev, SC_MODE_SENSE, 0x5, 
			     sizeof (struct ms_med_parms), 
			     (caddr_t)ms_med_parms, 0)) {
			EPRINTF("Can not get characteristics\n");
			(void) splx(s);
			return (ENXIO);
		}
		(void) splx(s);

		fchar->medium = ms_med_parms->header.medium;
		fchar->transfer_rate = ms_med_parms->page5.xfer_rate;
		fchar->ncyl = ms_med_parms->page5.ncyl;
		fchar->nhead = ms_med_parms->page5.nhead;
		fchar->sec_size = ms_med_parms->page5.data_bytes;
		fchar->secptrack = ms_med_parms->page5.sec_trk ;
		rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
			  (u_long)oms_med_parms);
		return (0);

	case FDKIOSCHAR:
		DPRINTF("sfioctl: set characteristics\n");
		fchar = (struct fdk_char *)data;
		ms_med_parms = (struct ms_med_parms *)rmalloc(iopbmap,
		(long)(sizeof (struct ms_med_parms) + 4));
		oms_med_parms = ms_med_parms;
		while ((u_int)ms_med_parms & 0x03)
			((u_int)ms_med_parms)++;
		bzero((char *)ms_med_parms, sizeof (struct ms_med_parms));
		s = splr(pritospl(un->un_mc->mc_intpri));
		if (sfcmd(dev, SC_MODE_SENSE, 0x5, 
			     sizeof (struct ms_med_parms), 
			     (caddr_t)ms_med_parms, 0)) {
			EPRINTF("Can not get characteristics\n");
			rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
				  (u_long)oms_med_parms);
		
			(void) splx(s);
			return(ENXIO);
		}
				/* stuff set by user */
		ms_med_parms->header.medium = fchar->medium;
		ms_med_parms->page5.xfer_rate = fchar->transfer_rate;
		ms_med_parms->page5.nhead = fchar->nhead;
		ms_med_parms->page5.sec_trk = fchar->secptrack;
		ms_med_parms->page5.data_bytes = fchar->sec_size;
		ms_med_parms->page5.ncyl = fchar->ncyl;
		if (sfcmd(dev, SC_MODE_SELECT, 0x5, 
			     sizeof (struct ms_med_parms), 
			     (caddr_t)ms_med_parms, 0)) {
			EPRINTF("Can not set characteristics\n");
			rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
			  (u_long)oms_med_parms);
			(void) splx(s);
			return (ENXIO);
		}
		(void) splx(s);
		rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
			  (u_long)oms_med_parms);
		return (0);

	/*
	 * Handle unknown ioctls here.
	 */
	default:
		EPRINTF("sfioctl:  unknown ioctl\n");
		return (ENOTTY);
	}
}


/*
 * Run a command for sfioctl.
 */
#ifdef  SF_FORMAT
/*ARGSUSED*/
static
sfioctl_cmd(dev, un, dsi, data)
	dev_t dev;
	register struct scsi_unit *un;
	struct scsi_floppy *dsi;
	caddr_t data;
{
	register struct dk_cmd *com;
	register struct fdk_state *flstate;
	int err, options;
	int s;

/* 	DPRINTF("sfioctl_cmd:\n"); */

	com = (struct dk_cmd *)&((struct fdk_cmd *)data)->dcmd;
	flstate = (struct fdk_state *)&((struct fdk_cmd *)data)->fstate;
#ifdef  SFDEBUG
#ifdef notdef
	if (sf_debug > 1) {
		printf("sfioctl_cmd:  cmd= %x  blk= %x  cnt= %x  ",
				com->dkc_cmd, com->dkc_blkno, com->dkc_secnt);
		printf("buf addr= %x  buflen= 0x%x\n",
				com->dkc_bufaddr, com->dkc_buflen);
	}
#endif
#endif  SFDEBUG
	/*
	 * Set options.
	 */
	options = 0;
	if (com->dkc_flags & DK_SILENT) {
		options |= SD_SILENT;
#ifdef		SFDEBUG
		if (sf_debug > 0)
			options &= ~SD_SILENT;
#endif		SFDEBUG
	}
	if (com->dkc_flags & DK_ISOLATE)
		options |= SD_NORETRY;

	/*
	 * Process the special ioctl command.
	 */
	switch (com->dkc_cmd) {
	case FKREAD:
/*  		DPRINTF("sfioctl_cmd:  read\n"); */
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sf_check_char(flstate, un, dev);
 		/* make sure we have not changed */
		options |= SD_DVMA_RD;
		err = sfcmd(dev, SC_SPECIAL_READ, (u_int)com->dkc_blkno,
			    (u_int)com->dkc_buflen,
			    (caddr_t)com->dkc_bufaddr, options);
		(void) splx(s);
		break;

	case FKWRITE:
/* 		DPRINTF("sfioctl_cmd:  write\n"); */
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sf_check_char(flstate, un, dev);
 		/* make sure we have not changed */
		options |= SD_DVMA_WR;
		err = sfcmd(dev, SC_SPECIAL_WRITE, (u_int)com->dkc_blkno,
			    (u_int)com->dkc_buflen,
			    (caddr_t)com->dkc_bufaddr, options);
		(void) splx(s);
		break;

	case FKSEEK:
		DPRINTF("sfioctl_cmd:  seek\n");
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sfcmd(dev, SC_SEEK, (u_int)com->dkc_blkno,
			    (u_int)com->dkc_buflen,
			    (caddr_t)com->dkc_bufaddr, options);
		(void) splx(s);
		break;

	case FKREZERO:
		DPRINTF("sfioctl_cmd:  rezero\n");
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sfcmd(dev, SC_REZERO_UNIT, (u_int)com->dkc_blkno,
			    (u_int)com->dkc_buflen,
			    (caddr_t)com->dkc_bufaddr, options);
		(void) splx(s);
		break;

	case SC_MODE_SELECT:
		EPRINTF("sfioctl_cmd:  mode select\n");
		options |= SD_DVMA_WR;
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sfcmd(dev, SC_MODE_SELECT, (u_int)com->dkc_blkno,
			    (u_int)com->dkc_buflen, (caddr_t)com->dkc_bufaddr,
			    options);
		(void) splx(s);
#ifdef		SFDEBUG
		if (sf_debug > 2) {
			sf_print_buffer((u_char *)com->dkc_bufaddr,
					(int)com->dkc_buflen);
			printf("\n");
		}
#endif		SFDEBUG
		break;

	case SC_MODE_SENSE:
		EPRINTF("sfioctl_cmd:  mode sense\n");
		options |= SD_DVMA_RD;
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sfcmd(dev, SC_MODE_SENSE, (u_int)com->dkc_blkno,
			    (u_int)com->dkc_buflen, (caddr_t)com->dkc_bufaddr,
			    options);
		(void) splx(s);
#ifdef		SFDEBUG
		if (sf_debug > 2) {
			sf_print_buffer((u_char *)com->dkc_bufaddr,
					(int)com->dkc_buflen);
			printf("\n");
		}
#endif		SFDEBUG
		break;

        case FKFORMAT_TRACK:
 		EPRINTF("sfioctl_cmd:  format track\n");
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sf_check_char(flstate, un, dev);
		err = sfcmd(dev, SC_REZERO_UNIT, (u_int)com->dkc_blkno,
			    (u_int)com->dkc_buflen,
			    (caddr_t)com->dkc_bufaddr, options);
		err = sfcmd(dev, SC_FORMAT_TRACK, (u_int)com->dkc_blkno,
			    (u_int)com->dkc_buflen, (caddr_t)com->dkc_bufaddr,
			    options);
		(void) splx(s);
		break;

	case FKFORMAT_UNIT:
/* 		EPRINTF("sfioctl_cmd:  format\n"); */
		s = splr(pritospl(un->un_mc->mc_intpri));
		err = sf_check_char(flstate, un, dev);
		err = sfcmd(dev, SC_FORMAT, (u_int)com->dkc_blkno,
			    (u_int)com->dkc_buflen, (caddr_t)com->dkc_bufaddr,
			    options);
		(void) splx(s);
		break;

	default:
		EPRINTF1("sfioctl_cmd:  unknown command %x\n", com->dkc_cmd);
		return (EINVAL);
		/* break; */
	}
	if (err) {
/* 		EPRINTF("sfioctl_cmd:  ioctl cmd failed\n"); */
		return (EIO);
	}
	return (0);
}

/* see if any of the characteristics of the floppy have changed */
sf_check_char(fchar, un, dev)
struct fdk_state *fchar;
register struct scsi_unit *un;
dev_t dev;
{       
	register struct scsi_floppy *sfp;
	struct ms_med_parms {
		struct ccs_modesel_head_fl header;
		struct ccs_modesel_medium page5;
	} *ms_med_parms, *oms_med_parms;
	int change = 0;
	
	sfp = &sfdisk[un->un_unit];
	if (fchar->fkc_bsec || fchar->fkc_strack || 
	       fchar->fkc_step || fchar->fkc_rate) {
		ms_med_parms = (struct ms_med_parms *)rmalloc(iopbmap,
		(long)(sizeof (struct ms_med_parms) + 4));
		oms_med_parms = ms_med_parms;
		while ((u_int)ms_med_parms & 0x03)
			((u_int)ms_med_parms)++;
		bzero((char *)ms_med_parms, sizeof (struct ms_med_parms));
				/* stuff set by user */
		if (fchar->fkc_rate != sfp->sf_xrate) {
			sfp->sf_xrate = fchar->fkc_rate;
			change = 1;
		}
		if (fchar->fkc_strack != sfp->sf_spt) {
			sfp->sf_spt = fchar->fkc_strack;
			change = 1;
		}
		if (fchar->fkc_bsec != sfp->sf_bsec) {
			sfp->sf_bsec = fchar->fkc_bsec;
			change = 1;
		}
		if (fchar->fkc_step != sfp->sf_step) {
			sfp->sf_step = fchar->fkc_step;
			change = 1;
		}
		if (change) {
			if (sfcmd(dev, SC_MODE_SENSE, 0x5, 
			     sizeof (struct ms_med_parms), 
			     (caddr_t)ms_med_parms, 0)) {
			EPRINTF("Can not get characteristics\n");
			rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
				  (u_long)oms_med_parms);
			return(ENXIO);
		}
		ms_med_parms->page5.xfer_rate = sfp->sf_xrate;
		ms_med_parms->page5.sec_trk = sfp->sf_spt;
		ms_med_parms->page5.data_bytes = sfp->sf_bsec;
		ms_med_parms->page5.step = sfp->sf_step;
#ifdef notdef
		if (sfcmd(dev, SC_MODE_SELECT, 0x5, 
			     sizeof (struct ms_med_parms), 
			     (caddr_t)ms_med_parms, 0)) {
			EPRINTF("Can not set characteristics\n");
			rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
			  (u_long)oms_med_parms);
			return (ENXIO);
		}
#endif
		}
		rmfree(iopbmap, (long)(sizeof (struct ms_med_parms) + 4), 
			  (u_long)oms_med_parms);
		return (0);
	}
	return (0);
}
#endif  SF_FORMAT


static char *sf_ncrflop_error_str[] = {
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
#define MAX_NCRFLOP_ERROR_STR \
	(sizeof(sf_ncrflop_error_str)/sizeof(sf_ncrflop_error_str[0]))


static char *sf_key_error_str[] = {
	"no sense",			/* 0x00 */
	"soft error",			/* 0x01 */
	"not ready",			/* 0x02 */
	"medium error",			/* 0x03 */
	"hardware error",		/* 0x04 */
	"illegal request",		/* 0x05 */
	"unit attention",		/* 0x06 */
	"write protected",		/* 0x07 */
	"blank check",			/* 0x08 */
	"vendor unique",		/* 0x09 */
	"copy aborted",			/* 0x0a */
	"aborted command",		/* 0x0b */
	"equal error",			/* 0x0c */
	"volume overflow",		/* 0x0d */
	"miscompare error",		/* 0x0e */
	"reserved",			/* 0x0f */
	0
};
#define MAX_KEY_ERROR_STR \
	(sizeof(sf_key_error_str)/sizeof(sf_key_error_str[0]))


static char *sf_cmds[] = {
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
	"start/stop",			/* 0x1b */
	"<bad cmd>",			/* 0x1c */
};
#define MAX_SF_CMDS \
	(sizeof(sf_cmds)/sizeof(sf_cmds[0]))


/*
 * Return the text string associated with the sense key value.
 */
static char *
sf_print_key(key_code)
	register u_char key_code;
{
	static char *unknown_key = "unknown key";
	if ((key_code > MAX_KEY_ERROR_STR -1)  ||
	     sf_key_error_str[key_code] == NULL) {

		return (unknown_key);
	}
	return (sf_key_error_str[key_code]);
}


/*
 * Return the text string associated with the secondary
 * error code, if availiable.
 */
static char *
sf_print_error(dsi, error_code)
	register struct scsi_floppy *dsi;
	register u_char error_code;
{
	static char *unknown_error = "unknown error";
	switch (dsi->un_ctype) {
	case CTYPE_NCRFLOP:
	case CTYPE_CCS:
		if ((MAX_NCRFLOP_ERROR_STR > error_code)  &&
		    sf_ncrflop_error_str[error_code] != NULL)
			return (sf_ncrflop_error_str[error_code]);
		break;

	}
	return (unknown_error);
}


/*
 * Print the sense key and secondary error codes
 * and dump out the sense bytes.
 */
#ifdef	SFDEBUG
static
sf_error_message(un, dsi)
	register struct scsi_unit *un;
	register struct scsi_floppy *dsi;
{
	register struct scsi_ext_sense *ssf;
	register u_char   *cp;
	register int i;
	register u_char error_code;

	/* If error messages are being suppressed, exit. */
	if (dsi->un_options & SD_SILENT)
		return;

	ssf = (struct scsi_ext_sense *)cp = (
		struct scsi_ext_sense *)dsi->un_sense;
	error_code = ssf->error_code;
	printf("sf%d error:  sense key(0x%x): %s",
		un - sfunits, ssf->key, sf_print_key(ssf->key));
	if (error_code != 0) {
		printf(",  error code(0x%x): %s",
		       error_code, sf_print_error(dsi, error_code));
	}

	printf("\n           sense = ");
	for (i=0; i < sizeof (struct scsi_ext_sense); i++)
		printf("%x  ", *cp++);
	printf("\n");
}


static
sf_print_cmd(un)
	register struct scsi_unit *un;
{
	register int x;
	register u_char *y = (u_char *)&(un->un_cdb);

	printf("sf%d:  failed cmd =  ", SFUNIT(un->un_unit));
	for (x = 0; x < CDB_GROUP0; x++)
		printf("%x  ", *y++);
	printf("\n");
}
#endif	SFDEBUG


static
sferrmsg(un, bp, action)
	struct scsi_unit *un;
	struct buf *bp;
	char *action;
{
	register struct scsi_ext_sense *ssf;
	register struct scsi_floppy *dsi;
	char *cmdname;
	u_char error_code;
	int blkno_flag = 1;		/* if false, blkno is meaningless */

	dsi = &sfdisk[dkunit(bp)];
	ssf = (struct scsi_ext_sense *)dsi->un_sense;

	/* If error messages are being suppressed, exit. */
	if ((dsi->un_options & SD_SILENT) || (sf_debug == 0))
		return;

	/* Decode command name */
	if (un->un_cmd < MAX_SF_CMDS) {
		cmdname = sf_cmds[un->un_cmd];
	} else {
		cmdname = "unknown cmd";
	}

	/* If not a check condition error, block number is invalid */
	if (! un->un_scb.chk)
		blkno_flag = 0;


	printf("sf%d%c:  %s %s",
		SFNUM(un), LPART(bp->b_dev) + 'a', cmdname, action);
	if (blkno_flag)
		printf(",  block %d", dsi->un_err_blkno);

	error_code = ssf->error_code;
	printf("\n       sense key(0x%x): %s",
	       ssf->key, sf_print_key(ssf->key));
	if (error_code != 0) {
		printf(",  error code(0x%x): %s\n",
		       error_code, sf_print_error(dsi, error_code));
	} else {
		printf("\n");
	}

}
#endif	NSF > 0
