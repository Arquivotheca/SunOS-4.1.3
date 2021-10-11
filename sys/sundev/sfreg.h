/* @(#)sfreg.h       1.1 92/07/30 Copyr 1988 Sun Micro */

#ifndef _sundev_sfreg_h
#define _sundev_sfreg_h

#include <sundev/sdreg.h>

/*
 * Defines for SCSI disk drives.
 */
#define SF_TIMEOUT		/* Enable command timeouts */
#define SF_FORMAT  		/* Enable on-line formatting mods */

/*
 * SCSI disk controller specific stuff.
 */
#define CTYPE_NCRFLOP		4

/*
 * Mode Select Page Codes
 */
struct ccs_modesel_head_fl {
	u_char	reserved1;
	u_char	medium;			/* medium type */
	u_char 	reserved2;
	u_char 	reserved3;
}; 

struct ccs_modesel_medium {
	u_char page_code;
	u_char page_length;	/* length of page */
	u_short xfer_rate;	/* transfer rate */
	u_char nhead;		/* number of heads */
	u_char sec_trk;		/* sectors per track */
	u_short data_bytes;	/* date bytes per sector */
	u_short ncyl;
	u_short start_cyl;
	u_short start_cyl1;
	u_short step;		/* drive step rate */
	u_char  pulse;
	u_char  head_settle;
	u_char  head_settle1;
	u_char	motor_on_delay;
	u_char	motor_off_delay;
	u_char	true_ready	:1;	/* actually a bit field */
	u_char	ssn		:1;	/* actually a bit field */
	u_char	mo		:1;	/* actually a bit field */
	u_char	dummy		:5;	/* actually a bit field */
	u_char  step_pulse;
	u_char  write_precomp;
	u_char  hload_delay;
	u_char  huload_delay;
	u_char	pins;		/* how pins on the interface are used */
	u_char  pins1;
	u_char  reserved1;
	u_char  reserved2;
	u_char  reserved3;
	u_char  reserved4;
};
/* 
 * Vendor specific stuff
  */
struct ccs_modesel_vendor {
	u_char  reserved;
};

#define SS8SPT		0xFE	/* single sided 8 sectors per track */
#define DS8SPT		0xFF	/* double sided 8 sectors per track */
#define SS9SPT		0xFC	/* single sided 9 sectors per track */
#define DS9SPT		0xFD	/* double sided 9 sectors per track */
#define DSHSPT		0xF9	/* High density */

/*
 * sf specific SCSI commands (KSS should go in scsi.h someday)
 */
#define SC_INIT_CHARACTERISTICS 0x0c	/* initialize drive characteristics */

#endif /*!_sundev_sfreg_h*/
