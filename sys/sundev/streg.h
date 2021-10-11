/* @(#)streg.h       1.1 92/07/30 Copyr 1989 Sun Micro */

/*
 * Defines for SCSI tape drives.
 */
#ifndef _sundev_streg_h
#define _sundev_streg_h

#define ST_SYSGEN   			/* Enable Sysgen controller support */
#define ST_EOF_READTHRU  		/* Enable reading thru eof */

#define MAX_ST_DEV_SIZE		65534	/* 64KB -2 for variable length I/O */
#define MAX_AUTOLOAD_DELAY	60	/* 120 Sec. Auto-load maximum delay */
#define ST_AUTODEN_LIMIT	1	/* Auto density file limit */
#define MAXSTBUF		3	/* Max outstanding buffers per unit */
#define INF			1000000000

#ifdef  lint
#define ST_SYSGEN   			/* Enable Sysgen controller support */
#define ST_TIMEOUT			/* Enable command timeouts */
#define ST_EOF_READTHRU  		/* Enable reading thru eof */
#define ST_TIMEOUT			/* Enable command timeouts */
/*#define sun2				/* Enable sun2 support */
/*#define CPU_SUN2_120		2	/*   "     "     "     */
#endif  lint

/*
 * Driver state machine codes -- Do not change order!
 * Opening states:
 *	closed -> opening         ||   -> open  (no tape change, no error)
 *		  opening_sysgen
 *
 *	closed -> opening         ||   -> open  (tape change, no error)
 *                opening_delay   ||
 *
 *	closed -> opening         ||   -> opening_sysgen  -> open  (sysgen)
 *                opening_delay   ||
 *
 *	closed -> opening         ||   -> open_failed_loading ||  -> closed
 *                opening_delay	          open_failed_tape    ||     (error)
 *			     	          open_failed
 *
 * Closing states:
 *	open   -> closing -> closed
 *
 * Open states:
 *	open   -> open (passed command)
 *
 *	open   -> sensing -> open (failed command)
 *
 *	open   -> sensing -> append_testing -> open  (failed write append)
 *
 *	open   -> sensing -> append_testing -> retrying_cmd -> sensing_retry
 *					       (failed write append)
 *
 *	open   -> sensing -> append_testing -> retrying_cmd -> open
 *
 *					       (passed write append)
 * Open states (auto-density):
 *	open   -> sensing -> density_changing -> open
 *					       (failed density change)
 *
 *	open   -> sensing -> density_changing ->
 *	          retrying_cmd -> sensing_retry -> open
 *					       (failed density change)
 *
 *	open   -> sensing -> density_changing -> retrying_cmd -> open
 *					       (passed density change)
 */
#define CLOSED			 0
#define CLOSING			 1
#define OPEN_FAILED_LOADING	 2
#define OPEN_FAILED_TAPE	 3
#define OPEN_FAILED		 4
#define OPENING_SYSGEN		 5
#define OPENING_DELAY		 6
#define OPENING			 7
#define APPEND_TESTING		 8
#define DENSITY_CHANGING	 9
#define RETRYING_CMD		10
#define SENSING_RETRY		11
#define SENSING			12
#define OPEN			13

/*
 * Eof codes.
 */
#define ST_NO_EOF		0x00
#define ST_EOF			0x01
#define ST_EOF_LOG		0x02
#define ST_EOF_PENDING		0x03
#define ST_EOT			ST_EOF

/*
 * Operation codes.
 */
#define SC_REWIND		0x01
#define SC_QIC02		0x0D
#define SC_READ_XSTATUS_CIPHER	0xe0		/* Sun-2, Cipher tape only */
#define SC_SPACE_REC		0x11
#define SC_ERASE_TAPE		0x19
#define SC_LOAD			0x1B
#define SC_READ_LOG 		0x1F
#define SC_UNLOAD		0x80		/* phony - for int use only */
#define SC_BSPACE_FILE		0x81		/* phony - for int use only */
#define SC_SPACE_FILE		0x82		/* phony - for int use only */
#define SC_RETENSION		0x83		/* phony - for int use only */
#define SC_READ_APPEND		0x84		/* phony - for int use only */
#define SC_DENSITY0		0x85		/* phony - for int use only */
#define SC_DENSITY1		0x86		/* phony - for int use only */
#define SC_DENSITY2		0x87		/* phony - for int use only */
#define SC_DENSITY3		0x88		/* phony - for int use only */

/*
 * Supported tape device types plus default type for opening.
 * Types 10 - 13, are special (ancient too) drives.
 * Types 14 - 1f, are 1/4-inch cartridge drives.
 * Types 20 - 28, are 1/2-inch cartridge or reel drives.
 * Types 28+, are rdat (vcr) drives.
 */
#define ST_TYPE_INVALID		0x00
#define ST_TYPE_SYSGEN1		0x10	/* Sysgen with QIC-11 only */
#define ST_TYPE_SYSGEN		0x11	/* Sysgen with QIC-24 and QIC-11 */

#define ST_TYPE_DEFAULT		0x12	/* Who knows ? */
#define ST_TYPE_EMULEX		0x14	/* Emulex MT-02 */
#define ST_TYPE_ARCHIVE		0x15	/* Archive QIC-150 */
#define ST_TYPE_WANGTEK		0x16	/* Wangtek QIC-150 */

#define ST_TYPE_LMS		0x20	/* LMS */
#define ST_TYPE_FUJI		0x21	/* Fujitsu */
#define ST_TYPE_KENNEDY		0x22	/* Kennedy */
#define ST_TYPE_HP		0x23	/* HP */

#define ST_TYPE_EXB8200		0x28	/* Exabyte EXB-8200 */
#define ST_TYPE_EXB8500		0x29	/* Exabyte EXB-8500 */


/* Defines for supported drive options */
#define ST_VARIABLE		0x0001	/* supports variable length I/O */
#define ST_QIC			0x0002	/* QIC tape drive */
#define ST_REEL			0x0004	/* 1/2-inch reel tape drive */
#define ST_BSF			0x0008	/* Supports backspace file */
#define ST_BSR			0x0010	/* Supports backspace record */
#define ST_NO_FSR		0x0020	/* No forwardspace record */
#define ST_LONG_ERASE		0x0040	/* Long Erase tape timeout required */
#define ST_NODISCON		0x0080	/* No disconnect/reconnect support */
#define ST_NO_QIC24		0x0100	/* No QIC-24 (for Sysgen) */
#define ST_AUTODEN_OVERRIDE	0x0200	/* Auto-Density override flag */
#define ST_NO_POSITION		0x0400	/* Inhibit seeks flag */
#define ST_ERRLOG		0x0800	/* Use error log for soft errors */
#define ST_PHYSREC		0x1000	/* bsize is physical block size */


#define ST_DENS			4	/* Max. tape densities */
struct st_drive_table {
	char	name[16];		/* Name, for debug */
	char	length;			/* Length of vendor id */
	char	vid[24];		/* Vendor id and model */
	char	type;			/* Drive type for driver */
	short	bsize;			/* Block size */
	int	options;		/* Drive options */
	int	retry_threshold;	/* Minimum I/O threshold for checking */
	int	max_rretries[ST_DENS];	/* Max read retries  */
	int	max_wretries[ST_DENS];	/* Max write retries */
	u_char	density[ST_DENS];	/* density code */
	u_char	speed[ST_DENS];		/* speed code */
};
#define ST_DRIVE_INFO {\
/* Emulex MT-02 controller for 1/4" cartridge */ \
{	"Emulex", 2, "\000\000", ST_TYPE_EMULEX, \
	512, ST_QIC, \
	1000000, 20, 20, 20, 20,	60, 60, 60, 60, \
	0x84,  0x05, 0x05, 0x05,  0, 0, 0, 0 }, \
/* Wangtek QIC-150 1/4" cartridge */ \
{	"Wangtek", 7, "WANGTEK", ST_TYPE_WANGTEK, \
	512, ST_QIC, \
	1000000, 20, 20, 20, 20,	60, 60, 60, 60, \
	0x00, 0x00, 0x00, 0x00,  0, 0, 0, 0 }, \
/* Archive QIC-150 1/4" cartridge */ \
{	"Archive", 7, "ARCHIVE", ST_TYPE_ARCHIVE, \
	512, ST_QIC, \
	1000000, 20, 20, 20, 20,	60, 60, 60, 60, \
	0x00, 0x00, 0x00, 0x00,  0, 0, 0, 0 }, \
/* HP 1/2" reel */ \
{	"HP-88780", 13, "HP      88780", ST_TYPE_HP, \
	10240, (ST_REEL | ST_VARIABLE | ST_BSF | ST_BSR | ST_ERRLOG), \
	2000000, 11, 11, 3, 3,	105, 105, 26, 26, \
	0x01, 0x02, 0x03, 0xC3,  0, 0, 0, 0 }, \
/* Exabyte 8mm 5GB cartridge */ \
{	"Exabyte EXB8500", 16, "EXABYTE EXB-8500", ST_TYPE_EXB8500, \
	1024, (ST_VARIABLE | ST_BSF | ST_BSR | ST_LONG_ERASE | ST_PHYSREC), \
	8000000, 30, 30, 30, 30,	60, 60, 60, 60, \
	0x14, 0x00, 0x00, 0x00,  0, 0, 0, 0 }, \
/* Exabyte 8mm 2.3GB cartridge */ \
{	"Exabyte EXB8200", 16, "EXABYTE EXB-8200", ST_TYPE_EXB8200, \
	1024, (ST_VARIABLE | ST_BSF | ST_BSR | ST_LONG_ERASE | ST_PHYSREC), \
	8000000, 30, 30, 30, 30,	60, 60, 60, 60, \
	0x00, 0x00, 0x00, 0x00,  0, 0, 0, 0 }, \
/* Kennedy 1/2" reel */ \
{	"Kennedy", 4, "KENNEDY", ST_TYPE_KENNEDY, \
	10240, (ST_REEL | ST_VARIABLE | ST_BSF | ST_BSR), \
	2000000, 11, 11, 3, 3,	105, 105, 26, 26, \
	0x01, 0x02, 0x03, 0x03,  0, 0, 0, 0 }, \
/* LMS 3480 1/2" cartridge */ \
{	"LMS", 3, "LMS", ST_TYPE_LMS, \
	10240, (ST_VARIABLE | ST_BSF | ST_BSR), \
	2000000, 3, 3, 3, 3,	26, 26, 26, 26, \
	0x00, 0x00, 0x00, 0x00,  0, 0, 0, 0 }, \
/* Fujitsu 3480 1/2" cartridge */ \
{	"Fujitsu", 2, "FUJITSU", ST_TYPE_FUJI, \
	10240, (ST_VARIABLE | ST_BSF | ST_BSR | ST_ERRLOG), \
	2000000, 3, 3, 3, 3,	26, 26, 26, 26, \
	0x00, 0x00, 0x00, 0x00,  0, 0, 0, 0 }, \
}

/*
 * Exceptions to the above drive table.  These drives/controllers
 * require special processing.  Note, do not change the order of
 * this table!
 */
#define IS_DEFAULT(dsi)		(dsi->un_ctype == ST_TYPE_DEFAULT)
#define IS_SYSGEN(dsi)		(dsi->un_ctype == ST_TYPE_SYSGEN)
#define IS_EMULEX(dsi)		(dsi->un_ctype == ST_TYPE_EMULEX)
#define IS_EXABYTE(dsi)		(dsi->un_ctype == ST_TYPE_EXB8200 || \
				dsi->un_ctype == ST_TYPE_EXB8500)

#define ST_DRIVE1_INFO {\
/* Sysgen controller for 1/4" cartridge (QIC-24 and QIC-11) */ \
{	"Sysgen", 0, "\000", ST_TYPE_SYSGEN, \
	512, (ST_QIC | ST_NO_FSR | ST_NODISCON), \
	1000000, 20, 20, 20, 20,	60, 60, 60, 60, \
	0x26, 0x27, 0x27, 0x27,  0, 0, 0, 0 }, \
/* Sysgen controller for 1/4" cartridge (QIC-11 only) */ \
{	"Sysgen11", 0, "\000", ST_TYPE_SYSGEN, \
	512, (ST_QIC | ST_NODISCON | ST_NO_QIC24), \
	1000000, 20, 20, 20, 20,	60, 60, 60, 60, \
	0x00, 0x00, 0x00, 0x00,  0, 0, 0, 0 }, \
/* Default values for unknown fixed-length tape drive */ \
{	"Unknown", 0, " ", ST_TYPE_DEFAULT, \
	512, (ST_QIC | ST_LONG_ERASE), \
	10000000, 30, 30, 30, 30,	60, 60, 60, 60, \
	0x00, 0x00, 0x00, 0x00,  0, 0, 0, 0 }, \
/* Default values for unknown variable-length tape drive */ \
{	"Var. Unknown", 0, " ", ST_TYPE_DEFAULT, \
	1024, (ST_VARIABLE | ST_LONG_ERASE), \
	10000000, 30, 30, 30, 30,	60, 60, 60, 60, \
	0x00, 0x00, 0x00, 0x00,  0, 0, 0, 0 }, \
}

/*
 * defines for SCSI tape CDB.
 */
#undef	t_code
#undef	high_count
#undef	mid_count
#undef	low_count
struct	scsi_cdb6 {		/* scsi command description block */
	u_char	cmd;		/* command code */
	u_char	lun	: 3;	/* logical unit number */
	u_char	t_code	: 5;	/* high part of address */
	u_char	high_count;	/* middle part of address */
	u_char	mid_count;	/* low part of address */
	u_char	low_count;	/* block count */
	u_char	vu_57	: 1;	/* vendor unique (byte 5 bit 7) */
	u_char	vu_56	: 1;	/* vendor unique (byte 5 bit 6) */
	u_char		: 4;	/* reserved */
	u_char	fr	: 1;	/* flag request (interrupt at completion) */
	u_char	link	: 1;	/* link (another command follows) */
};


/*
 * Tape error rate log (HP-88780).
 * This is needed for collecting soft error rate info.
 */
struct st_log {
	u_char	page_code;	/* Page code */
	u_char	length[2];	/* Page length */
	u_char	entry_num;	/* Current entry number */
	u_char	log_entries;	/* Number of log entries */
	u_char	rsvd1;		/* Currently displayed log */
	u_char	density;	/* Density */
	u_char	wr_hard[2];	/* Write hard errors */
	u_char	wr_soft[2];	/* Write soft errors */
	u_char	wr_data[6];	/* Data bytes written */
	u_char	rd_hard[2];	/* Read hard errors */
	u_char	rd_soft[2];	/* Read soft errors */
	u_char	rd_data[6];	/* Data bytes read */
	u_char	rsvd2;		/* For even byte alignment */
};

/*
 * Parameter list for the MODE_SELECT and MODE_SENSE commands.
 * The parameter list contains a header, followed by zero or more
 * block descriptors, followed by vendor unique parameters, if any.
 * Note, only one 8-byte block descriptor is used for 1/2-inch SCSI
 * tape devices (e.g. CDC HI/TC, HP SCSI-2, and Kennedy).
 */
#define MS_BD_LEN	8
struct st_ms_mspl {
	u_char	reserved1;	/* reserved, sense data length */
	u_char	reserved2;	/* reserved, medium type */
	u_char	wp	:1;	/* write protected */
	u_char	bufm	:3;	/* buffered mode */
	u_char	speed	:4;	/* speed */
	u_char	bd_len;		/* block length in bytes */
	u_char	density;	/* density code */
	u_char	high_nb;	/* number of logical blocks on the medium */
	u_char	mid_nb;		/* that are to be formatted with the density */
	u_char	low_nb;		/* code and block length in block descriptor */
	u_char	reserved;	/* reserved */
	u_char	high_bl;	/* block length */
	u_char	mid_bl;		/*   "      "   */
	u_char	low_bl;		/*   "      "   */
};
struct st_ms_exabyte {
	u_char	reserved1;	/* reserved, sense data length */
	u_char	reserved2;	/* reserved, medium type */
	u_char	wp	:1;	/* write protected */
	u_char	bufm	:3;	/* buffered mode */
	u_char	speed	:4;	/* speed */
	u_char	bd_len;		/* block length in bytes */
	u_char	density;	/* density code */
	u_char	high_nb;	/* number of logical blocks on the medium */
	u_char	mid_nb;		/* that are to be formatted with the density */
	u_char	low_nb;		/* code and block length in block descriptor */
	u_char	reserved;	/* reserved */
	u_char	high_bl;	/* block length */
	u_char	mid_bl;		/*   "      "   */
	u_char	low_bl;		/*   "      "   */
	u_char	optional1;	/* optional vendor unique byte */
};


/*
 * SCSI tape REQUEST SENSE parameter block.  Note,
 * this structure should have an even number of bytes to
 * eliminate any byte packing problems with our host adapters.
 */
struct	st_sense {		/* scsi tape extended sense for error class 7 */
	/* byte 0 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char		: 7;	/* fixed at binary 1110000 */
	/* byte 1 */
	u_char	seg_num;	/* segment number, applies to copy cmd only */
	/* byte 2 */
	u_char	fil_mk	: 1;	/* file mark on device */
	u_char	eom	: 1;	/* end of media */
	u_char	ili	: 1;	/* incorrect length indicator */
	u_char		: 1;	/* reserved */
	u_char	key	: 4;	/* sense key, see below */
	/* bytes 3 through 6 */
	u_char	info_0;		/* sense information byte 1 */
	u_char	info_1;		/* sense information byte 2 */
	u_char	info_2;		/* sense information byte 3 */
	u_char	info_3;		/* sense information byte 4 */
	/* bytes 7 through 13 */
	u_char	add_len;	/* number of additional bytes */
	u_char	optional_8;	/* search/copy src sense only */
	u_char	optional_9;	/* search/copy dst sense only */
	u_char	optional_10;	/* search/copy only */
	u_char	optional_11;	/* search/copy only */
	u_char	error;		/* error code */
	u_char	error1;		/* error code qualifier */
	/* bytes 14 through 27 */
	u_char	optional_14;	/* reserved */
	u_char	optional_15;	/* reserved */
	u_char	optional_16;	/* reserved */
	u_char	optional_17;	/* reserved */
	u_char	optional_18;	/* reserved */
	u_char	optional_19;	/* reserved */
	u_char	optional_20;	/* reserved */
	u_char	optional_21;	/* reserved */
	u_char	optional_22;	/* reserved */
	u_char	optional_23;	/* reserved */
	u_char	optional_24;	/* reserved */
	u_char	optional_25;	/* reserved */
	u_char	optional_26;	/* reserved */
	u_char	optional_27;	/* reserved */
};

#ifdef	ST_SYSGEN
/*
 * Sense info returned by sysgen controllers.  Note,
 * this structure should always be 16 bytes or the controller
 * will not clear it's error condition.  Also, you must
 * always follow an error condition with a request sense cmd.
 */
struct  sysgen_sense {
	/* Bytes 0 - 3  */
	u_char  disk_sense[4];		/* sense data from disk */
	/* byte 4 */
	u_char  valid4		:1;	/* some other bit set in this byte */
	u_char  no_cart		:1;	/* no cartrige, or removed */
	u_char  not_ready	:1;	/* drive not present */
	u_char  write_prot	:1;	/* write protected */
	u_char  eot		:1;	/* end of last track */
	u_char  data_err	:1;	/* unrecoverable data error */
	u_char  no_err		:1;	/* data transmitted not in error */
	u_char file_mark	:1;	/* file mark detected */
	/* byte 5 */
	u_char  valid5		:1;	/* some other bit set in this byte */
	u_char  illegal		:1;	/* illegal command */
	u_char  no_data		:1;	/* unable to find data */
	u_char  retries		:1;	/* 8 or more retries needed */
	u_char  bot		:1;	/* beginning of tape */
	u_char			:2;	/* reserved */
	u_char  reset		:1;	/* power-on or reset since last op */
	/* bytes 6 through 9 */
	short   retry_ct;		/* retry count */
	short   underruns;		/* number of underruns */
	/* bytes 10 through 15 */
	u_char   disk_xfer[3];		/* no. blks in last disk operation */
	u_char   tape_xfer[3];		/* no. blks in last tape operation */
};
#endif	ST_SYSGEN

/* structure for MTIOCGET - obsolete version */
#define	STIOCGET	_IOR(m, 2, struct st_mtget)
struct	st_mtget {
	short	mt_type;	/* type of magtape device */
	short	mt_dsreg;	/* drive status register */
	short	mt_erreg;	/* error register */
	short	mt_resid;	/* residual count */
	daddr_t	mt_fileno;	/* file number of current position */
	daddr_t	mt_blkno;	/* block number of current position */
};

/*
 * Macros for getting information from the sense data returned
 * by the various SCSI 1/2-inch tape controllers.  Note, this
 * use byte 2 of the extended sense descriptor block to mimimize
 * SCSI controller dependencies.
 */
#define ST_FILE_MARK(dsi, sense) \
	(dsi->un_ctype != 0  &&  ((struct st_sense *)sense)->fil_mk)

#define ST_END_OF_TAPE(dsi, sense) \
	(dsi->un_ctype != 0  &&  ((struct st_sense *)sense)->eom)

#define ST_LENGTH(dsi, sense) \
	(dsi->un_ctype != 0  &&  ((struct st_sense *)sense)->ili)

#endif _sundev_streg_h
