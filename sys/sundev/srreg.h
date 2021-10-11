/* @(#)srreg.h 1.1 92/07/30       Copyr 1989 Sun Micro */

/*
 * Defines for SCSI disk drives.
 */
#define SD_TIMEOUT		/* Enable command timeouts */
/* ffz #define SD_FORMAT  		/* Enable on-line formatting mods */
/*#define SD_REASSIGN		/* Enable auto disk repair */

#include <sys/ioctl.h>

#define FIVETWELVE
/*
 * Driver state machine codes -- Do not change order!
 * Opening states:
 *	closed -> open 
 *
 *	closed -> opening         ||   -> open
 *
 *	closed -> opening         ||   -> open
 *                opening_delay   ||
 *
 * Closing states:
 *	open   -> closed
 *
 * Open states:
 *	open   -> open (passed command)
 *
 *	open   -> sensing -> open (failed command)
 *
 *	open   -> sensing -> retrying_cmd -> open
 */
#define CLOSED			0
#define OPEN_FAILED		1
#define OPENING_DELAY		2
#define OPENING			3
#define MAPPING			4
#define SENSING			5
#define RETRYING		6
#define OPEN			7

/*
 * SCSI target controller type- disks
 */

#define CTYPE_UNKNOWN		0
#define CTYPE_MD21		1
#define CTYPE_ACB4000		2
#define CTYPE_CCS		3
#define CTYPE_TOSHIBA		4
#define CTYPE_SONY		4
#define CTYPE_PHILIPS		4
#define CTYPE_CDROM		5

#ifndef SD_FORMAT
#define DK_NOERROR		0
#define DK_CORRECTED		1
#define DK_RECOVERED		2
#define DK_FATAL		3
#endif SD_FORMAT

/*
 * Special commands for internal use only.
 */
#define SC_READ_LABEL		0x80	/* phony - for int use only */
#define SC_SPECIAL_READ		0x81	/* phony - for int use only */
#define SC_SPECIAL_WRITE	0x82	/* phony - for int use only */

/*
 * Define various buffer and I/O block sizes
 * notice the sector size for CDROM is 2048 bytes.
 */
#ifdef FIVETWELVE
#define SECSIZE			512		/* Bytes/sector */
#define SECDIV			9		/* log2 (SECSIZE) */
#else
#define SECSIZE			2048		/* Bytes/sector */
#define SECDIV			11		/* log2 (SECSIZE) */
#endif

#define MAXBUFSIZE		(63 * 1024)	/* 63 KB max size of buffer */

/*
 * Defines for SCSI disk format cdb.
 */
#define b_cylin			b_resid
#define fmt_parm_bits		g0_addr2	/* for format options */
#define fmt_interleave		g0_count0	/* for encode interleave */
#define defect_list_descrip	g1_addr3	/* list description bits */

/*
 * Bits in the special command flags field.
 */
#define SD_SILENT		0x0001	/* don't print error messages */
#define SD_DVMA_RD		0x0002	/* enable DVMA memory reads */
#define SD_DVMA_WR		0x0004	/* enable DVMA memory writes */
#define SD_NORETRY		0x0008	/* disable driver error recovery */
#define SC_SBF_SILENT		0x0001	/* don't print error messages */
#define SC_SBF_DIAGNOSE		0x0002	/* collect stats for diagnostic */
#define SC_SBF_DIR_IN		0x0004	/* need to copyin info */
#define SC_SBF_IOCTL		0x8000	/* cmd being run on behalf of ioctl */
#define SC_NOMSG		0x01	/* don't print error messages */
#define SC_DIAG			0x02	/* collect diagnostic info ??? */
#define SC_IOCTL		0x04	/* running cmd for ioctl */

/*************************************************************
 *                                                           *
 *                      Group 1 Commands                     *
 *                                                           *
 *************************************************************/

#define SCMD_DOORLOCK		0X1E		/* mandatory SCSI command */
#define SCMD_MODE_SELECT	0x15
#define SCMD_MODE_SENSE		0x1A

/*
 * flags for medium removal (SCSM_DOORLOCK SCSI command)
 */
#define SR_REMOVAL_PREVENT	1
#define	SR_REMOVAL_ALLOW	0

/*************************************************************
 *                                                           *
 *                      Group 2 Commands                     *
 *                                                           *
 *************************************************************/

#define SCMD_READ_TOC		0x43		/* optional SCSI command */
#define SCMD_PLAYAUDIO_MSF	0x47		/* optional SCSI command */
#define SCMD_PLAYAUDIO_TI	0x48		/* optional SCSI command */
#define SCMD_PAUSE_RESUME	0x4B		/* optional SCSI command */
#define SCMD_READ_SUBCHANNEL	0x42		/* optional SCSI command */
#define SCMD_PLAYAUDIO10	0x45		/* optional SCSI command */
#define SCMD_PLAYTRACK_REL10	0x49		/* optional SCSI command */
#define SCMD_READ_HEADER	0x44		/* optional SCSI command */

/*************************************************************/
/*                                                           */
/*                      Group 5 Commands                     */
/*                                                           */
/*************************************************************/

#define SCMD_PLAYAUDIO12	0xA5		/* optional SCSI command */
#define SCMD_PLAYTRACK_REL12	0xA9		/* optional SCSI command */

/*************************************************************/
/*                                                           */
/*                      Group 6 Commands                     */
/*                                                           */
/*************************************************************/

#define SCMD_CD_PLAYBACK_CONTROL 0xC9		/* SONY unique SCSI command */
#define SCMD_CD_PLAYBACK_STATUS 0xC4		/* SONY unique SCSI command */

/*
 * Error code fields for MD21 extended sense struct.
 * Not exhaustive - just those we treat specially.
 */
#define SC_ERR_READ_RECOV_RETRIES	0x17	/* ctlr fixed error w/retry */
#define SC_ERR_READ_RECOV_ECC		0x18	/* ctlr fixed error w/ECC */

/************************************************************************
 *									*
 * ACB4000 error codes.  						*
 * They are the concatenation of the class and code info.		*
 *									*
 ***********************************************************************/

/* Class 0 - drive errors */
#define SC_ERR_ACB_NO_SENSE	0x00	/* no error or sense info */
#define SC_ERR_ACB_NO_INDEX	0x01	/* no index or sector pulse */
#define SC_ERR_ACB_NO_SEEK_CMPL	0x02	/* no seek complete signal */
#define SC_ERR_ACB_WRT_FAULT	0x03	/* write fault */
#define SC_ERR_ACB_NOT_READY	0x04	/* drive not ready */
#define SC_ERR_ACB_NO_TRACK_0	0x06	/* no track 0 */

/* Class 1 - target errors */
#define SC_ERR_ACB_ID_CRC	0x10	/* ID field not found after retry */
#define SC_ERR_ACB_UNCOR_DATA	0x11	/* uncorrectable data error */
#define SC_ERR_ACB_ID_ADDR_MK	0x12	/* missing ID address mark */
#define SC_ERR_ACB_REC_NOT_FND	0x14	/* record not found */
#define SC_ERR_ACB_SEEK		0x15	/* seek error */
#define SC_ERR_ACB_DATA_CHECK	0x18	/* data check */

/* Class 2 - system-related errors */
#define SC_ERR_ACB_ECC_VERIFY	0x19	/* ECC error detected during verify */
#define SC_ERR_ACB_INTERLEAVE	0x1A	/* specified interleave too large */
#define SC_ERR_ACB_BAD_FORMAT	0x1C	/* drive not properly formatted */
#define SC_ERR_ACB_ILLEGAL_CMD	0x20	/* illegal command */
#define SC_ERR_ACB_ILLEGAL_BLK	0x21	/* illegal block address */
#define SC_ERR_ACB_VOL_OVERFLOW	0x23	/* illegal block addr after 1st blk */
#define SC_ERR_ACB_BAD_ARG	0x24	/* bad argument */
#define SC_ERR_ACB_ILLEGAL_LUN	0x25	/* invalid logical unit number */
#define SC_ERR_ACB_CART_CHANGE	0x28	/* new cartridge inserted */
#define SC_ERR_ACB_ERR_OVERFLOW	0x2C	/* too many errors */

/* Unknown psuedo error - used for convenience */
#define SC_ERR_ACB_ERR_UNKNOWN	0xFF	/* unknown psuedo error */


/*
 * defines for value of fmt_parm_bits.
 */
#define FPB_BFI			0x04		/* bytes-from-index fmt */
#define FPB_CMPLT		0x08		/* full defect list provided */
#define FPB_DATA		0x10		/* defect list data provided */

/*
 * Defines for value of defect_list_descrip.
 */
#define	DLD_MAN_DEF_LIST	0x10		/* manufacturer's defect list */
#define	DLD_GROWN_DEF_LIST	0x08		/* grown defect list */
#define	DLD_BLOCK_FORMAT	0x00		/* block format */
#define	DLD_BFI_FORMAT		0x04		/* bytes-from-index format */
#define	DLD_PS_FORMAT		0x05		/* physical sector format */


/*
 * Disk defect list - used by format command.
 */
/******************** NOTE: this should be cleaned up! ***********************/
#define ST506_NDEFECT	127     /* must fit in 1K controller buffer... */
#define ESDI_NDEFECT	ST506_NDEFECT	/* hack??? */

#define RDEF_ALL	0	/* read all defects */
#define RDEF_MANUF	1	/* read manufacturer's defects */
#define RDEF_CKLEN	2	/* check length of manufacturer's list */

struct scsi_bfi_defect {	/* defect in bytes from index format */
	unsigned cyl  : 24;
	unsigned head : 8;
	long   	 bytes_from_index;
};

struct scsi_format_params {	/* BFI format list */
	u_short reserved;
	u_short length;
	struct  scsi_bfi_defect list[ESDI_NDEFECT];
};

/*
 * Defect list returned by READ_DEFECT_LIST command.
 */
struct scsi_defect_hdr {	/* For getting defect list size */
	u_char	reserved;
	u_char	descriptor;
	u_short	length;
};

struct scsi_defect_list {	/* BFI format list */
	u_char	reserved;
	u_char	descriptor;
	u_short	length;
	struct	scsi_bfi_defect list[ESDI_NDEFECT];
};

/************************************************************************
 *									*
 * Direct Access device Reassign Block parameter			*
 *									*
 * Defect list format used by reassign block command (logical block	*
 * format).								*
 *									*
 * This defect list is limited to 1 defect, as that is the only way	*
 * we use it.								*
 *									*
 ************************************************************************/

struct scsi_reassign_blk {
	u_short	reserved;
	u_short length;		/* defect length in bytes ( defects * 4) */
	u_int 	defect;		/* Logical block address of defect */
};
/************************************************************************
 *									*
 * Direct Access Device Capacity Structure				*
 *									*
 ************************************************************************/

struct scsi_capacity {
	u_long	capacity;
	u_long	lbasize;
};

/************************************************************************
 *									*
 * Direct Access device mode sense/mode select paramters		*
 *									*
 ************************************************************************/

#define ERR_RECOVERY_PARMS	0x01
#define DISCO_RECO_PARMS	0x02
#define FORMAT_PARMS		0x03
#define GEOMETRY_PARMS		0x04
#define CERTIFICATION_PARMS	0x06
#define CACHE_PARMS		0x38

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
 * Page 3 - Direct Access Device Format Parameters
 */
struct ccs_format {
	u_char	reserved	: 2;
	u_char	page_code	: 6;	/* define page function */
	u_char	page_length;		/* length of current page */
	u_short	tracks_per_zone;	/*  Handling of Defects Fields */
	u_short	alt_sect_zone;
	u_short alt_tracks_zone;
	u_short	alt_tracks_vol;
	u_short	sect_track;		/* Track Format Field */
	u_short data_sect;		/* Sector Format Fields */
	u_short	interleave;
	u_short	track_skew;
	u_short	cylinder_skew;
	u_char	ssec		: 1;	/* Drive Type Field */
	u_char	hsec		: 1;
	u_char	rmb		: 1;
	u_char	surf		: 1;
	u_char	ins		: 1;
	u_char	reserved_1	: 3;
	u_char	reserved_2;
	u_char	reserved_3;
	u_char	reserved_4;
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
	unsigned ncyl     :16;
	unsigned nhead    :8;
	u_short	rwc_cyl;
	u_short	wprc_cyl;
	u_char	ls_pos;
	u_char	sporc;
};

/*!!!! temp stuff */
#define SR_IOC_AUDSEARCH   _IOW(o, 20,int)
#define SR_IOC_AUDPLAY   _IO(o, 21)

/* CD ROM specific commands */

#define	CDBLEN			10	/* length of audio CDB commands	 */
#define SR_CD_SUBCHANNEL	0x42	/* parallel channel for indexing */
#define SR_CD_TOC		0x43	/* Table of contents	  	 */
#define SR_CD_RD_HEADER		0x44	/* Read Header - determine data mode */
#define SR_CD_PLAYAUD_10	0x45	/* play audio 10 byte cdb by LBA */
#define SR_CD_PLAYAUD_12	0xA5	/* play audio 12 byte cdb by LBA */
#define SR_CD_PLAYAUD_MSF	0x47	/* play audio 10 byte cdb by MSF */
#define SR_CD_PLAY_TRKNDX	0x48	/* play track index 10 byte cdb	 */
#define SR_CD_AUD_PAUSE_RES	0x4B	/* audio play/resume 10 byte cdb */

#define	SR_START_STOP		0x1B	/* 8.2.18 start/stop(optional) */
#define		SR_STOP_eject	2	/* low order count bits */
#define		SR_START_load	1	/* load and lock for above */

#define	SR_MEDIA_REMOV		0x1E	/* 8.2.17 Toshiba XM-2100a p70 */
#define		SR_MEDIA_lock	1	/* lock for above */
#define		SR_MEDIA_unlock	0	/* unlock for above */


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
 * sdintr codes
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

struct sd_drivetype {
	char	*name;		/* for debug purposes */
	char	options;	/* drive options */
	char	ctype;		/* controller type */
	char	idlen;		/* id length */
	char	id[IDMAX];	/* Vendor id + part of product id */
};

#define	SD_LINKABLE	0x1	/* drive supports linked commands.. */
#define	SD_DOLINK	0x2	/* .has better performance with linked commands */
#define	SD_NODISC	0x4	/* don't do disconnect/reconnect */

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
#define CDROM_AUDIO_INVALID	0x00	/* audio status not supported */
#define CDROM_AUDIO_PLAY	0x11	/* audio play operation in progress */
#define CDROM_AUDIO_PAUSED	0x12	/* audio play operation paused */
#define CDROM_AUDIO_COMPLETED	0x13	/* audio play successfully completed */
#define CDROM_AUDIO_ERROR	0x14	/* audio play stopped due to error */
#define CDROM_AUDIO_NO_STATUS	0x15	/* no current audio status to return */
	
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
#define CDROM_LBA	0x01
#define CDROM_MSF	0x02

/*
 * Bitmask for CD-ROM data track in the cdte_ctrl field
 * A track is either data or audio.
 */
#define CDROM_DATA_TRACK	0x04

/*
 * For CDROMREADTOCENTRY, set the cdte_track to CDROM_LEADOUT to get
 * the information for the leadout track.
 */
#define CDROM_LEADOUT	0xAA

/*
 * definition for user-scsi command structure 
 */
struct uscsi_cmd {
	caddr_t	uscsi_cdb;
	int	uscsi_cdblen;
	caddr_t	uscsi_bufaddr;
	int	uscsi_buflen;
	unsigned char	uscsi_status;
	int	uscsi_flags;
};

/*
 * flags for uscsi_flags field
 */
#define	USCSI_SILENT	0x01	/* no error messages */
#define	USCSI_DIAGNOSE	0x02	/* fail if any error occurs */
#define USCSI_ISOLATE	0x04	/* isolate from normal commands */
#define USCSI_READ	0x08	/* get data from device */
#define USCSI_WRITE	0xFFF7	/* put data to device */

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
#define CDROM_MODE1_SIZE	512
#else
#define CDROM_MODE1_SIZE	2048
#endif
#define CDROM_MODE2_SIZE	2336

/*
 * driver flag (used by the routine srioctl_cmd())
 */
#define SR_USCSI_KERNEL	0x01	/* addresses used in the struct */
                                /* uscsi_scmd are kernel addresses */
#define SR_USCSI_USER	0x00	/* addresses used in the struct */
                                /* uscsi_scmd are user-land addresses */

/*
 * CDROM io control commands 
 */
#define CDROMPAUSE	_IO(c, 10)	  /* Pause Audio Operation */

#define CDROMRESUME	_IO(c, 11)	  /* Resume paused Audio Operation */

#define CDROMPLAYMSF		_IOW(c, 12, struct cdrom_msf)
                                                  /* Play Audio MSF */
#define CDROMPLAYTRKIND		_IOW(c, 13, struct cdrom_ti)
                                                  /* Play Audio Track/index*/
#define CDROMREADTOCHDR		_IOR(c, 103, struct cdrom_tochdr)
                                                 /* Read TOC header */
#define USCSICMD		_IOWR(u, 1, struct uscsi_cmd)
                                                 /* scsi command io control */
#define CDROMREADTOCENTRY	_IOWR(c, 104, struct cdrom_tocentry)
                                                 /* Read a TOC entry */
#define CDROMSTOP		_IO(c, 105)	/* Stop the cdrom drive */

#define CDROMSTART		_IO(c, 106)	/* Start the cdrom drive */

#define CDROMEJECT		_IO(c, 107)	/* Ejects the cdrom caddy */

#define CDROMVOLCTRL		_IOW(c, 14, struct cdrom_volctrl)
                                                /* control output volume */
#define CDROMSUBCHNL		_IOWR(c, 108, struct cdrom_subchnl)
                                                /* read the subchannel data */
#define CDROMREADMODE2		_IOW(c, 110, struct cdrom_read)
                                                /* read CDROM mode 2 data */
#define CDROMREADMODE1		_IOW(c, 111, struct cdrom_read)
                                                /* read CDROM mode 1 data */

/*
 * macros for accessing fields in the cdb block 
 */
#define cdb_cmd(cdb)	(unsigned char)(cdb[0])
#define cdb_tag(cdb)	(cdb[1] & 0x1F)
#define cdb0_blkno(cdb)	((((u_char)cdb[1] & 0x1F) << 16) + \
			 ((u_char)cdb[2] << 8) + (u_char)cdb[3])

#define cdb1_blkno(cdb)	(((u_char)cdb[2] << 24) + \
			 ((u_char)cdb[3] << 16) + \
			 ((u_char)cdb[4] << 8) + \
			 (u_char)cdb[5])

