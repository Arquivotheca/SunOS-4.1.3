static  char sccsid[] = "@(#)srdef.h 1.1 92/07/30 Copyright Sun Micro";
/**********************************************************************
        This include file is lift from 
	blob:/usr/src/sundist/cdrom/src/sys/scsi/targets
	srdef.h 1.11 89/11/14 version.
        Please keep updating this file as needed.
***********************************************************************/

/*
 *
 * Defines for SCSI direct access devices modified for CDROM, based on sddef.h
 *
 */

#include <sys/ioctl.h>

/*
 * define for 512 bytes/sector (default to 2048 bytes/sector)
 * #define	FIVETWELVE
 */

/*
 * set drive to read 2048 bytes per sector
 */
#undef FIVETWELVE

/*
 * fixed firmware for volume control
 */
#define	FIXEDFIRMWARE

/*
 * SCSI target controller type- disks
 */

#define	CTYPE_UNKNOWN		0
#define	CTYPE_MD21		1
#define	CTYPE_ACB4000		2
#define	CTYPE_CCS		3


#ifndef SD_FORMAT
#define	DK_NOERROR		0
#define	DK_CORRECTED		1
#define	DK_RECOVERED		2
#define	DK_FATAL		3
#endif SD_FORMAT

/*
 * Define various buffer and I/O block sizes
 * notice the sector size for CDROM is 2048 bytes.
 */
#ifdef FIVETWELVE
#define	SECSIZE			512		/* Bytes/sector */
#define	SECDIV			9		/* log2 (SECSIZE) */
#else
#define	SECSIZE			2048		/* Bytes/sector */
#define	SECDIV			11		/* log2 (SECSIZE) */
#endif FIVETWELVE
#define	MAXBUFSIZE		(63 * 1024)	/* 63 KB max size of buffer */

/*
 * flags for medium removal (SCMD_DOORLOCK SCSI command)
 */
#define	SR_REMOVAL_PREVENT	1
#define	SR_REMOVAL_ALLOW	0

/*
 *
 * Additional SCSI commands for CD-ROM.
 *
 */

/*
 *
 *	Group 1 Commands
 *
 */

/*
 *
 *	Group 2 Commands
 *
 */

#define	SCMD_READ_TOC		0x43		/* optional SCSI command */
#define	SCMD_PLAYAUDIO_MSF	0x47		/* optional SCSI command */
#define	SCMD_PLAYAUDIO_TI	0x48		/* optional SCSI command */
#define	SCMD_PAUSE_RESUME	0x4B		/* optional SCSI command */
#define	SCMD_READ_SUBCHANNEL	0x42		/* optional SCSI command */
#define	SCMD_PLAYAUDIO10	0x45		/* optional SCSI command */
#define	SCMD_PLAYTRACK_REL10	0x49		/* optional SCSI command */
#define	SCMD_READ_HEADER	0x44		/* optional SCSI command */

/*
 *
 *	Group 5 Commands
 *
 */

#define	SCMD_PLAYAUDIO12	0xA5		/* optional SCSI command */
#define	SCMD_PLAYTRACK_REL12	0xA9		/* optional SCSI command */

/*
 *
 *	Group 6 Commands
 *
 */

#define	SCMD_CD_PLAYBACK_CONTROL 0xC9		/* SONY unique SCSI command */
#define	SCMD_CD_PLAYBACK_STATUS 0xC4		/* SONY unique SCSI command */

/*
 *
 * Direct Access Device Capacity Structure
 *
 */

struct scsi_capacity {
	u_long	capacity;
	u_long	lbasize;
};

/*
 *
 * Direct Access device mode sense/mode select paramters
 *
 */

#define	ERR_RECOVERY_PARMS	0x01
#define	DISCO_RECO_PARMS	0x02
#define	FORMAT_PARMS		0x03
#define	GEOMETRY_PARMS		0x04
#define	CERTIFICATION_PARMS	0x06
#define	CACHE_PARMS		0x38

/*
 * Mode select header
 */

struct ccs_modesel_head {
	u_char	reserved1;
	u_char	medium;			/* medium type */
	u_char 	reserved2;
	u_char	block_desc_length;	/* block descriptor length */
	u_char	density;		/* density code */
	u_char	number_blocks_hi;
	u_char	number_blocks_med;
	u_char	number_blocks_lo;
	u_char	reserved3;
	u_char	block_length_hi;
	u_short	block_length;
};

/*
 * Page 1 - Error Recovery Parameters
 */
struct ccs_err_recovery {
	u_char	reserved	: 2;
	u_char	page_code	: 6;	/* define page function */
	u_char	page_length;		/* length of current page */
	u_char	awre		: 1;	/* auto write realloc enabled */
	u_char	arre		: 1;	/* auto read realloc enabled */
	u_char	tb		: 1;	/* transfer block */
	u_char 	rc		: 1;	/* read continuous */
	u_char	eec		: 1;	/* enable early correction */
	u_char	per		: 1;	/* post error */
	u_char	dte		: 1;	/* disable transfer on error */
	u_char	dcr		: 1;	/* disable correction */
	u_char	retry_count;
	u_char	correction_span;
	u_char	head_offset_count;
	u_char	strobe_offset_count;
	u_char	recovery_time_limit;
};

/*
 * Page 2 - Disconnect / Reconnect Control Parameters
 */

struct ccs_disco_reco {
	u_char	reserved	: 2;
	u_char	page_code	: 6;	/* define page function */
	u_char	page_length;		/* length of current page */
	u_char	buffer_full_ratio;	/* write, how full before reconnect? */
	u_char	buffer_empty_ratio;	/* read, how full before reconnect? */

	u_short	bus_inactivity_limit;	/* how much bus time for busy */
	u_short	disconnect_time_limit;	/* min to remain disconnected */
	u_short	connect_time_limit;	/* min to remain connected */
	u_short	reserved_1;
};

/*
 * Page 4 - Rigid Disk Drive Geometry Parameters
 */
struct ccs_geometry {
	u_char	reserved	: 2;
	u_char	page_code	: 6;	/* define page function */
	u_char	page_length;		/* length of current page */
	u_char	cyl_ub;			/* number of cylinders */
	u_char	cyl_mb;
	u_char	cyl_lb;
	u_char	heads;			/* number of heads */
	u_char	precomp_cyl_ub;		/* cylinder to start precomp */
	u_char	precomp_cyl_mb;
	u_char	precomp_cyl_lb;
	u_char	current_cyl_ub;		/* cyl to start reduced current */
	u_char	current_cyl_mb;
	u_char	current_cyl_lb;
	u_short	step_rate;		/* drive step rate */
	u_char	landing_cyl_ub;		/* landing zone cylinder */
	u_char	landing_cyl_mb;
	u_char	landing_cyl_lb;
	u_char	reserved_1;
	u_char	reserved_2;
	u_char	reserved_3;
};

/*
 * Page 38 - Cache Parameters
 */
struct ccs_cache {
	u_char	reserved	: 2;
	u_char	page_code	: 6;	/* define page function */
	u_char	page_length;		/* length of current page */
	u_char	mode;			/* Cache control and size */
	u_char	threshold;		/* Prefetch threshold */
	u_char	max_prefetch;		/* Max. prefetch */
	u_char	max_multiplier;		/* Max. prefetch multiplier */
	u_char	min_prefetch;		/* Min. prefetch */
	u_char	min_multiplier;		/* Min. prefetch multiplier */
	u_char	rsvd2[8];
};

struct ccs_mode_select_parms {
	long	edl_len;
	long	reserved;
	long	bsize;
	unsigned fmt_code :8;
	unsigned ncyl	  :16;
	unsigned nhead    :8;
	u_short	rwc_cyl;
	u_short	wprc_cyl;
	u_char	ls_pos;
	u_char	sporc;
};

struct acb4000_mode_select_parms {
	long	edl_len;
	long	reserved;
	long	bsize;
	unsigned fmt_code :8;
	unsigned ncyl	:16;
	unsigned nhead	:8;
	u_short	rwc_cyl;
	u_short	wprc_cyl;
	u_char	ls_pos;
	u_char	sporc;
};

/*
 *
 * Private info for scsi disks. Pointed to by the un_private pointer
 * of one of the SCSI_DEVICE structures.
 *
 */

struct scsi_disk {
	struct scsi_device *un_sd;	/* back pointer to SCSI_DEVICE */
	struct scsi_pkt *un_rqs;	/* ptr to request sense command pkt */
	struct sr_drivetype	*un_dp;	/* drive type table */
	long	un_capacity;		/* capacity of drive */
	long	un_lbasize;		/* logical block size */
	struct	buf *un_rbufp;		/* for use in raw io */
	struct	buf *un_sbufp;		/* for use in special io */
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
	u_char	un_open0;		/* open flag #0 */
	u_char	un_open1;		/* open flag #1 */
	u_char	un_exclopen;		/* exclusive open flag */
	char	un_dkn;			/* dk number for iostats */
};

/*
 * CDROM driver states
 * note the drive is locked when opened and unlocked when closed
 */

#define	SR_STATE_NIL		0
#define	SR_STATE_CLOSED		1
#define	SR_STATE_OPENING	2
#define	SR_STATE_OPEN		3
#define	SR_STATE_SENSING	4
#define	SR_STATE_RWAIT		5
#define	SR_STATE_DETACHING	6
#define	SR_STATE_EJECTED	7
#define	SR_STATE_MODE2		8

/*
 * Error levels
 */

#define	SRERR_ALL		0
#define	SRERR_UNKNOWN		1
#define	SRERR_INFORMATIONAL	2
#define	SRERR_RECOVERED		3
#define	SRERR_RETRYABLE		4
#define	SRERR_FATAL		5

/*
 * srintr codes
 */

#define	COMMAND_DONE		0
#define	COMMAND_DONE_ERROR	1
#define	QUE_COMMAND		2
#define	QUE_SENSE		3
#define	JUST_RETURN		4

/*
 * Drive Types (and characteristics)
 */

#define	IDMAX	8+4

struct sr_drivetype {
	char	*name;		/* for debug purposes */
	char	options;	/* drive options */
	char	ctype;		/* controller type */
	char	idlen;		/* id length */
	char	id[IDMAX];	/* Vendor id + part of product id */
};

#define	SR_LINKABLE	0x1	/* drive supports linked commands.. */
#define	SR_DOLINK	0x2	/* has better performance with linked cmds */
#define	SR_NODISC	0x4	/* don't do disconnect/reconnect */

/*
 * CDROM io controls type definitions
 */
struct cdrom_msf {
	unsigned char	cdmsf_min0;	/* starting minute */
	unsigned char	cdmsf_sec0;	/* starting second */
	unsigned char	cdmsf_frame0;	/* starting frame  */
	unsigned char	cdmsf_min1;	/* ending minute   */
	unsigned char	cdmsf_sec1;	/* ending second   */
	unsigned char	cdmsf_frame1;	/* ending frame	   */
};

struct cdrom_ti {
	unsigned char	cdti_trk0;	/* starting track */
	unsigned char	cdti_ind0;	/* starting index */
	unsigned char	cdti_trk1;	/* ending track */
	unsigned char	cdti_ind1;	/* ending index */
};

struct cdrom_tochdr {
	unsigned char	cdth_trk0;	/* starting track */
	unsigned char	cdth_trk1;	/* ending track */
};

struct cdrom_tocentry {
	unsigned char	cdte_track;
	unsigned char	cdte_adr	:4;
	unsigned char	cdte_ctrl	:4;
	unsigned char	cdte_format;
	union {
		struct {
			unsigned char	minute;
			unsigned char	second;
			unsigned char	frame;
		} msf;
		int	lba;
	} cdte_addr;
	unsigned char	cdte_datamode;
};

/*
 * CDROM address format definition, for use with struct cdrom_tocentry
 */
#define	CDROM_LBA	0x01
#define	CDROM_MSF	0x02

/*
 * Bitmask for CD-ROM data track in the cdte_ctrl field
 * A track is either data or audio.
 */
#define	CDROM_DATA_TRACK	0x04

/*
 * For CDROMREADTOCENTRY, set the cdte_track to CDROM_LEADOUT to get
 * the information for the leadout track.
 */
#define	CDROM_LEADOUT	0xAA

struct cdrom_subchnl {
	unsigned char	cdsc_format;
	unsigned char	cdsc_audiostatus;
	unsigned char	cdsc_adr:	4;
	unsigned char	cdsc_ctrl:	4;
	unsigned char	cdsc_trk;
	unsigned char	cdsc_ind;
	union {
		struct {
			unsigned char	minute;
			unsigned char	second;
			unsigned char	frame;
		} msf;
		int	lba;
	} cdsc_absaddr;
	union {
		struct {
			unsigned char	minute;
			unsigned char	second;
			unsigned char	frame;
		} msf;
		int	lba;
	} cdsc_reladdr;
};

/*
 * Definition for audio status returned from Read Sub-channel
 */
#define	CDROM_AUDIO_INVALID	0x00	/* audio status not supported */
#define	CDROM_AUDIO_PLAY	0x11	/* audio play operation in progress */
#define	CDROM_AUDIO_PAUSED	0x12	/* audio play operation paused */
#define	CDROM_AUDIO_COMPLETED	0x13	/* audio play successfully completed */
#define	CDROM_AUDIO_ERROR	0x14	/* audio play stopped due to error */
#define	CDROM_AUDIO_NO_STATUS	0x15	/* no current audio status to return */

/* definition of audio volume control structure */
struct cdrom_volctrl {
	unsigned char	channel0;
	unsigned char	channel1;
	unsigned char	channel2;
	unsigned char	channel3;
};

struct cdrom_read {
	int	cdread_lba;
	caddr_t	cdread_bufaddr;
	int	cdread_buflen;
};

#ifdef FIVETWELVE
#define	CDROM_MODE1_SIZE	512
#else
#define	CDROM_MODE1_SIZE	2048
#endif FIVETWELVE
#define	CDROM_MODE2_SIZE	2336

/*
 * driver flag (used by the routine srioctl_cmd())
 */
#define	SR_USCSI_CDB_KERNEL	0x01	/* addresses used in the cdb field */
					/* of struct uscsi_scmd are kernel */
					/* addresses */
#define	SR_USCSI_BUF_KERNEL	0x02	/* addresses used in the bufaddr   */
					/* field of struct uscsi_scmd are  */
					/* kernel address */
#define	SR_MODE2	0x04	/* switch the drive to reading mode 2 data */

/*
 * CDROM io control commands
 */
#define	CDROMPAUSE	_IO(c, 10)	/* Pause Audio Operation */

#define	CDROMRESUME	_IO(c, 11)	/* Resume paused Audio Operation */

#define	CDROMPLAYMSF	_IOW(c, 12, struct cdrom_msf)	/* Play Audio MSF */
#define	CDROMPLAYTRKIND	_IOW(c, 13, struct cdrom_ti)	/*
							 * Play Audio
`							 * Track/index
							 */
#define	CDROMREADTOCHDR	\
		_IOR(c, 103, struct cdrom_tochdr)	/* Read TOC header */
#define	CDROMREADTOCENTRY	\
	_IOWR(c, 104, struct cdrom_tocentry)		/* Read a TOC entry */

#define	CDROMSTOP	_IO(c, 105)	/* Stop the cdrom drive */

#define	CDROMSTART	_IO(c, 106)	/* Start the cdrom drive */

#define	CDROMEJECT	_IO(c, 107)	/* Ejects the cdrom caddy */

#define	CDROMVOLCTRL	\
	_IOW(c, 14, struct cdrom_volctrl)	/* control output volume */

#define	CDROMSUBCHNL	\
	_IOWR(c, 108, struct cdrom_subchnl)	/* read the subchannel data */

#define	CDROMREADMODE2	\
	_IOW(c, 110, struct cdrom_read)		/* read CDROM mode 2 data */

#define	CDROMREADMODE1	\
	_IOW(c, 111, struct cdrom_read)		/* read CDROM mode 1 data */

/*
 * macros for accessing fields in the cdb block
 */
#define	cdb_cmd(cdb)	(unsigned char)(cdb[0])
#define	cdb_tag(cdb)	(cdb[1] & 0x1F)
#define	cdb0_blkno(cdb)	((((u_char)cdb[1] & 0x1F) << 16) + \
			((u_char)cdb[2] << 8) + (u_char)cdb[3])
