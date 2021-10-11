/* @(#)streg.h       1.1 92/07/30 Copyr 1987 Sun Micro */

/*
 * Defines for SCSI tape drives.
 */
#define ST_SYSGEN   			/* Enable Sysgen controller support */
#define MAX_ST_DEV_SIZE		65535	/* 64KB limit for variable length I/O */
#define MAX_AUTOLOAD_DELAY	60	/* 120 Sec. Auto-load maximum delay */
#define ST_AUTODEN_LIMIT	1	/* Auto density file limit */

#ifdef  lint
#define ST_SYSGEN   			/* Enable Sysgen controller support */
#define ST_AUTOPOSITION  		/* Enable seek support */
#define ST_TIMEOUT			/* Enable command timeouts */
/*#define sun2				/* Enable sun2 support */
/*#define CPU_SUN2_120		2	/*   "     "     "     */
#endif  lint

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
#define SENSING			11
#define OPEN			12

/*
 * Eof codes.
 */
#define ST_NO_EOF		0x00
#define ST_EOF			0x01
#define ST_EOF_PENDING		0x02
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
#define SC_UNLOAD		0x80		/* phony - for int use only */
#define SC_BSPACE_FILE		0x81		/* phony - for int use only */
#define SC_SPACE_FILE		0x82		/* phony - for int use only */
#define SC_RETENSION		0x83		/* phony - for int use only */
#define SC_QIC11		0x84		/* phony - for int use only */
#define SC_QIC24		0x85		/* phony - for int use only */
#define SC_READ_APPEND		0x86		/* phony - for int use only */

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
#define ST_TYPE_ADAPTEC		0x13	/* Adaptec */

#define ST_TYPE_EMULEX		0x14	/* Emulex MT-02 */
#define ST_TYPE_ARCHIVE		0x15	/* Archive QIC-150 */
#define ST_TYPE_WANGTEK		0x16	/* Wangtek QIC-150 */
#define ST_TYPE_ADSI		0x17	/* ADSI */

#define ST_TYPE_CDC		0x20	/* CDC */
#define ST_TYPE_FUJI		0x21	/* Fujitsu */
#define ST_TYPE_KENNEDY		0x22	/* Kennedy */
#define ST_TYPE_HP		0x23	/* HP */

#define ST_TYPE_EXABYTE		0x28	/* Exabyte */


/*
 * Misc defines specific to sysgen controllers
 */
#define ST_SYSGEN_QIC11		0x26
#define ST_SYSGEN_QIC24		0x27
#define ST_SYSGEN_SENSE_LEN	16
#define ST_SYSGEN_SENSE_BYTES	4

/*
 * Misc defines specific to emulex controllers
 */
#define ST_EMULEX_QIC11		0x84
#define ST_EMULEX_QIC24		0x00
#define ST_EMULEX_SENSE_LEN	12

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
	/* bytes 14 through 15 */
	u_char	optional_14;	/* reserved */
	u_char	optional_15;	/* reserved */
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
