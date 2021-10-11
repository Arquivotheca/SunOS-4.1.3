/* @(#)sfdef.h       1.1 92/07/30 Copyr 1987 Sun Micro */

/************************************************************************
 *									*
 * Defines for SCSI direct access devices				*
 * Specifically for SCSI floppies.					*
 *									*
 ***********************************************************************/

#include	<scsi/targets/sddef.h>
/*
 * Compile options
 */

/*
 * SCSI disk controller specific stuff.
 */
#define CTYPE_NCR	4
#define CTYPE_SMS	5

/************************************************************************
 *									*
 * Floppy Direct Access device mode sense/mode select paramters		*
 *									*
 ************************************************************************/

#define	FLOPPY_PARMS	0x5

/*
 * Mode select header
 */

/*
 * Page 5 - Floppy parameters
 */

struct ccs_modesel_flexible {
	u_char page_code;
	u_char page_length;	/* length of page */
	u_char xfer_rate[2];	/* transfer rate */
	u_char nhead;		/* number of heads */
	u_char sec_trk;		/* sectors per track */
	u_char data_bytes[2];	/* date bytes per sector */
	u_char ncyl[2];
	u_char start_cyl[2];
	u_char start_cyl1[2];
	u_char step[2];		/* drive step rate */
	u_char  pulse;
	u_char  head_settle[2];
	u_char	motor_on_delay;
	u_char	motor_off_delay;
	u_char	true_ready	:1,
		ssn		:1,
		mo		:1,
		/*reserved*/	:5;
	u_char  step_pulse;
	u_char  write_precomp;
	u_char  hload_delay;
	u_char  huload_delay;
	u_char	pins;		/* how pins on the interface are used */
	u_char  pins1;
	u_char  reserved[4];
};

#define SS8SPT		0xFE	/* single sided 8 sectors per track */
#define DS8SPT		0xFF	/* double sided 8 sectors per track */
#define SS9SPT		0xFC	/* single sided 9 sectors per track */
#define DS9SPT		0xFD	/* double sided 9 sectors per track */
#define DSHSPT		0xF9	/* High density */

#define	MSIZE	(sizeof (struct ccs_modesel_head) +\
			sizeof (struct ccs_modesel_flexible))
#define	MOFF	(sizeof (struct ccs_modesel_head) - 1)

#define	MSTOC(x,a)	\
	(a)[0] = ((((short)(x))>>8)&0xff), (a)[1] = (((short)(x))&0xff)

struct sf_mode {
	struct ccs_modesel_head h;
	struct ccs_modesel_flexible s;
};

/*
 * sf specific SCSI commands
 */
#define SCMD_INIT_CHARACTERISTICS 0x0c	/* initialize drive characteristics */

/************************************************************************
 *									*
 * Private info for scsi floppies. Pointed to by the un_private pointer	*
 * of one of the SCSI_DEVICE structures.				*
 *									*
 ************************************************************************/

struct scsi_floppy {
	struct scsi_device *un_sd;	/* back pointer to SCSI_DEVICE */
	struct scsi_pkt *un_rqs;	/* ptr to request sense command pkt */
	struct sf_drivetype	*un_dp;	/* drive type table */
	long	un_capacity;		/* capacity of drive */
	long	un_lbasize;		/* logical block size */
	struct	buf *un_rbufp;		/* for use in raw io */
	struct	buf *un_sbufp;		/* for use in special io */
	caddr_t	un_mode;		/* ptr to mode sense/select info */
	struct	dk_map un_map[NDKMAP];	/* logical partitions */
	struct	dk_geom un_g;		/* disk geometry */
	struct	diskhd	un_utab;	/* for queuing */
	u_int	un_err_resid;		/* resid from last error */
	u_int	un_err_blkno;		/* disk block where error occurred */
	dev_t	un_dev;			/* unix device */
	u_char	un_last_cmd;		/* last cmd (DKIOCGDIAG only) */
	u_char	un_soptions;		/* 'special' command options */
	u_char	un_err_severe;		/* error severity */
	u_char	un_status;		/* sense key from last error */
	u_char	un_err_code;		/* vendor unique error code  */
	u_char	un_retry_ct;		/* retry count */
	u_char	un_gvalid;		/* geometry is valid */
	u_char	un_state;		/* current state */
	u_char	un_last_state;		/* last state */
	u_char	un_open;		/* bit pattern of open partitions */
};

/*
 * Disk driver states
 */

#define	SF_STATE_NIL		0
#define	SF_STATE_CLOSED		1
#define	SF_STATE_OPEN		2
#define	SF_STATE_OPENING	3
#define	SF_STATE_SENSING	4
#define	SF_STATE_RWAIT		5
#define	SF_STATE_DETACHING	6

/*
 * Drive Types (and characteristics)
 */

struct sf_drivetype {
	char	*name;		/* for debug purposes */
	char	options;	/* drive options */
	char	ctype;		/* controller type */
	char	vidlen;		/* Vendor id length */
	char	vid[8];		/* Vendor id */
};


