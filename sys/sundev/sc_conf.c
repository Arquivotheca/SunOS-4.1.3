#ifndef lint
static	char sccsid[] = "@(#)sc_conf.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */
#define REL4			/* Enable Release 4.0 mods */

#include "sc.h"
#include "si.h"
#include "se.h"
#ifdef REL4
#include "sm.h" 		/* Sun 3/80 and Sun 4/330 support */
#include "sw.h" 		/* Sun 4/110 support */
#include "wds.h"		/* Sun 386i  support */
#endif REL4

#if ((NSC > 0) || (NSI > 0) || (NSE > 0) || (NSW > 0) || (NWDS > 0) || (NSM >0))

#ifndef REL4
#include "../h/types.h"
#include "../h/buf.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../sundev/screg.h"
#include "../sundev/sireg.h"
#include "../sundev/scsi.h"

#else REL4
#include <sys/types.h>
#include <sys/buf.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <sundev/screg.h>
#include <sundev/sireg.h>
#include <sundev/scsi.h>
#endif REL4

/* generic scsi debug flag */
int scsi_debug = 0;

/* scsi disconnect/reconnect and synchronous support global enable flag */
int scsi_disre_enable = 2;		/* =0, disconnect disabled */
					/* =1, disconnect enabled */
					/* =2, + synchronous  */

/* scsi initiator host id.  Note, not used by sc host adapter. */
/* FIXME: really should be put in eeprom like 4c does. */
#define	HOST_ID		7
int scsi_host_id = 1 << HOST_ID;	/* id 7 = 0x80 */

/* scsi reset recovery time */
int scsi_reset_delay = 1 *1000000;	/* 1-2 Sec. delay (1 us units) */

/* 
 * scsi CDB op code to command length decode table.
 */
u_char sc_cdb_size[] = {
	CDB_GROUP0,		/* Group 0, 6 byte cdb */
	CDB_GROUP1,		/* Group 1, 10 byte cdb */
	CDB_GROUP2,		/* Group 2, ? byte cdb */
	CDB_GROUP3,		/* Group 3, ? byte cdb */
	CDB_GROUP4,		/* Group 4, ? byte cdb */
	CDB_GROUP5,		/* Group 5, ? byte cdb */
	CDB_GROUP6,		/* Group 6, ? byte cdb */
	CDB_GROUP7		/* Group 7, ? byte cdb */
};

/*
 * If host adaptors exist, declare structures for them here.
 */
#if NSC > 0
int nsc = NSC;
struct scsi_ctlr scctlrs[NSC];
struct mb_ctlr *scinfo[NSC];
#endif NSC

#if NSI > 0
int nsi = NSI;
struct scsi_ctlr sictlrs[NSI];
struct mb_ctlr *siinfo[NSI];
#endif NSI

#if NSE > 0
int nse = NSE;
struct scsi_ctlr se_ctlrs[NSE];
struct mb_ctlr *se_info[NSE];
#endif NSE

#if NSW > 0
int nsw = NSW;
struct scsi_ctlr swctlrs[NSW];
struct mb_ctlr *swinfo[NSW];
#endif NSW

#if NWDS > 0
int nwds = NWDS;
struct scsi_ctlr wdsctlrs[NWDS];
struct mb_ctlr *wdsinfo[NWDS];
#endif NWDS

#if NSM > 0
int nsm = NSM;
struct scsi_ctlr smctlrs[NSM];
struct mb_ctlr *sminfo[NSM];
#endif NSM

/*
 * declare structures for top-level SCSI drivers, if present.
 * (e.g. disks, tapes, and floppy disks) 
 */
#include "sd.h"
#include "st.h"
#include "sr.h"

#if NSD > 0
int sd_repair = 3;	/* =0, auto disk repair disabled
			/* =1, rewrite soft errors (before reassign)
			/* =2, reassign soft errors
			/* =4, reassign hard errors
			/* =3, rewrite/reassign soft errors only
			/* =7, all of the above
			 */
int nsdisk = NSD;
struct scsi_unit sdunits[NSD];
struct scsi_disk sdisk[NSD];
#ifdef	DKIOCWCHK
/* XXX: Supports 8 partitions/unit max. */
u_char sdwchkmap[NSD] = { 0 };	/* write check enable map. */
#endif	DKIOCWCHK

#endif NSD

#if NST > 0
#ifdef REL4
#include <sundev/streg.h>
#else REL4
#include "../sundev/streg.h"
#endif REL4
struct scsi_unit stunits[NST];
struct scsi_tape stape[NST];
struct st_drive_table st_drivetab[] = ST_DRIVE_INFO;
int st_drivetab_size = sizeof(st_drivetab)/sizeof(st_drivetab[0]);
int nstape = NST;
#endif NST

#if NSR > 0
struct scsi_unit srunits[NSR];
struct scsi_disk srdisk[NSR];
int ncdrom = NSR;
#endif NSR

struct	mb_device *sdinfo[NSD + NST + NSR];


/*
 * Device specific subroutines. Indexed by "flag" from the configuration
 * file.  Flag is is in mc_flag (e.g. Disk is 0, tape is 1).
 */
#if NSD > 0
int	sdattach(), sdstart(), sdmkcdb(), sdintr(), sdunitptr();
#endif NSD > 0

#if NST > 0
int	stattach(), ststart(), stmkcdb(), stintr(), stunitptr();
#endif NST > 0

#if NSR > 0
int	srattach(), srstart(), srmkcdb(), srintr(), srunitptr();
#endif NSR > 0

struct	scsi_unit_subr scsi_unit_subr[] = {
#if NSD > 0
	{ sdattach, sdstart, sdmkcdb, sdintr, sdunitptr, "sd", },
#else
	{ (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0,
	(char *)0},
#endif NSD > 0

#if NST > 0
	{ stattach, ststart, stmkcdb, stintr, stunitptr, "st", },
#else
	{ (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0,
	(char *)0},
#endif NST > 0

#if NSR > 0
	{ srattach, srstart, srmkcdb, srintr, srunitptr, "sr", },
#else
	{ (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0,
	(char *)0},
#endif NSR > 0
};

/*
 * Number of SCSI device driver types.
 */
int scsi_ntype = sizeof scsi_unit_subr / sizeof (struct scsi_unit_subr);

#endif ((NSC > 0) || (NSI > 0) || (NSE > 0) || (NSW > 0) || (NWDS > 0) || (NSM >0))
