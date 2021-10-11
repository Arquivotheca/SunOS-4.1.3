#ident	"@(#)stdef.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1989, 1990, 1991 by Sun Microsystems, Inc.
 */

/*
 * Defines for SCSI tape drives.
 */


/*
 * Maximum variable length record size for a single request
 */
#define	ST_MAXRECSIZE_VARIABLE	65535

/*
 * If the requested record size exceeds ST_MAXRECSIZE_VARIABLE,
 * then the following define is used.
 */
#define	ST_MAXRECSIZE_VARIABLE_LIMIT	65534

#define	ST_MAXRECSIZE_FIXED	(63<<10)	/* maximum fixed record size */
#define	INF 1000000000

/*
 * Supported tape device types plus default type for opening.
 * Types 10 - 13, are special (ancient too) drives - *NOT SUPPORTED*
 * Types 14 - 1f, are 1/4-inch cartridge drives.
 * Types 20 - 28, are 1/2-inch cartridge or reel drives.
 * Types 28+, are rdat (vcr) drives.
 */
#define	ST_TYPE_INVALID		0x00

#define	ST_TYPE_SYSGEN1		0x10	/* Sysgen with QIC-11 only */
#define	ST_TYPE_SYSGEN		0x11	/* Sysgen with QIC-24 and QIC-11 */

#define	ST_TYPE_DEFAULT		0x12	/* Generic 1/4" or undetermined tape */
#define	ST_TYPE_EMULEX		0x14	/* Emulex MT-02 */
#define	ST_TYPE_ARCHIVE		0x15	/* Archive QIC-150 */
#define	ST_TYPE_WANGTEK		0x16	/* Wangtek QIC-150 */

#define	ST_TYPE_CDC		0x20	/* CDC - (not tested) */
#define	ST_TYPE_FUJI		0x21	/* Fujitsu - (not tested) */
#define	ST_TYPE_KENNEDY		0x22	/* Kennedy */
#define	ST_TYPE_HP		0x23	/* HP */
#define	ST_TYPE_HIC		0x26	/* Generic 1/2" Cartridge */
#define	ST_TYPE_REEL		0x27	/* Generic 1/2" Reel Tape */

#define	ST_TYPE_EXABYTE		0x28	/* Exabyte */
#define	ST_TYPE_EXB8500		0x29	/* Exabyte */


/* Defines for supported drive options */
#define	ST_VARIABLE		0x001	/* supports variable length I/O */
#define	ST_REEL			0x004	/* 1/2-inch reel tape drive */
#define	ST_BSF			0x008	/* Supports backspace file */
#define	ST_BSR			0x010	/* Supports backspace record */
#define	ST_LONG_ERASE		0x020	/* Long Erase option */
#define	ST_AUTODEN_OVERRIDE	0x040	/* Auto-Density override flag */
#define	ST_NOBUF		0x080	/* Don't use buffered mode */
#define	ST_NOPARITY		0x100	/*
					 * This device cannot generate parity,
					 * so don't check parity while talking
					 * to it.
					 */

#define	ST_KNOWS_EOD		0x200	/* knows when EOD has been reached */
#define	ST_QIC			0x202	/* QIC tape drive knows EOD */
/*
 * old defines for the options flag - not supported anymore
 * here for historical reference only.
 */
/* was ST_NO_FSR- Sysgen only	0x020	/* No forwardspace record */
/* was ST_NODISCON- Sysgen only	0x080	/* No disconnect/reconnect support */
/* was ST_NO_QIC24- Sysgen	0x100	/* No QIC-24 (for Sysgen) */
/* was ST_NO_POSITION- ignore	0x400	/* Inhibit seeks flag */

#define	NDENSITIES	4
#define	NSPEEDS		4

struct st_drivetype {
	char	*name;			/* Name, for debug */
	char	length;			/* Length of vendor id */
	char	vid[24];		/* Vendor id and model (product) id */
	char	type;			/* Drive type for driver */
	short	bsize;			/* Block size */
	int	options;		/* Drive options */
	int	max_rretries;		/* Max read retries */
	int	max_wretries;		/* Max write retries */
	u_char	densities[NDENSITIES];	/* density codes, low->hi */
	u_char	speeds[NSPEEDS];	/* speed codes, low->hi */
};

/*
 *
 * Parameter list for the MODE_SELECT and MODE_SENSE commands.
 * The parameter list contains a header, followed by zero or more
 * block descriptors, followed by vendor unique parameters, if any.
 *
 */

#define	MSIZE	(sizeof (struct seq_mode))
struct seq_mode {
	u_char	reserved1;	/* reserved, sense data length */
	u_char	reserved2;	/* reserved, medium type */
	u_char	wp	:1,	/* write protected */
		bufm	:3,	/* buffered mode */
		speed	:4;	/* speed */
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

/*
 * Private info for scsi tapes. Pointed to by the un_private pointer
 * of one of the SCSI_DEVICE chains.
 */

struct scsi_tape {
	struct scsi_device *un_sd;	/* back pointer to SCSI_DEVICE */
	struct scsi_pkt *un_rqs;	/* ptr to request sense command */
	struct	buf *un_rbufp;		/* for use in raw io */
	struct	buf *un_sbufp;		/* for use in special io */
	struct	buf *un_quef;		/* head of wait queue */
	struct	buf *un_quel;		/* tail of wait queue */
	struct	buf *un_runq;		/* head of run queue */
	struct seq_mode *un_mspl;	/* ptr to mode select info */
	struct st_drivetype *un_dp;	/* ptr to drive table entry */
	caddr_t	un_tmpbuf;		/* buf for append, autodens ops */
	daddr_t	un_blkno;		/* block # in file (512 byte blocks) */
	int	un_fileno;		/* current file number on tape */
	int	un_err_fileno;		/* file where error occurred */
	daddr_t	un_err_blkno;		/* block in file where err occurred */
	u_int	un_err_resid;		/* resid from last error */
	short	un_fmneeded;		/* filemarks to be written - HP only */
	dev_t	un_dev;			/* unix device */
	u_char	un_attached;		/* unit known && attached */
	u_char	un_density_known;	/* density is known */
	u_char	un_curdens;		/* index into density table */
	u_char	un_lastop;		/* last I/O was: read/write/ctl */
	u_char	un_eof;			/* eof states */
	u_char	un_laststate;		/* last state */
	u_char	un_state;		/* current state */
	u_char	un_status;		/* status from last sense */
	u_char	un_retry_ct;		/* retry count */
	u_char	un_read_only;		/* 1 == opened O_RDONLY */
	u_char	un_test_append;		/* check writing at end of tape */
};


/*
 * driver states..
 */

#define	ST_STATE_CLOSED			0
#define	ST_STATE_OPENING		1
#define	ST_STATE_OPEN_PENDING_IO	2
#define	ST_STATE_APPEND_TESTING		3
#define	ST_STATE_OPEN			4
#define	ST_STATE_RESOURCE_WAIT		5
#define	ST_STATE_CLOSING		6
#define	ST_STATE_SENSING		7

/*
 * operation codes
 */

#define	ST_OP_NIL	0
#define	ST_OP_CTL	1
#define	ST_OP_READ	2
#define	ST_OP_WRITE	3

/*
 * eof/eot/eom codes.
 */

#define	ST_NO_EOF		0x00
#define	ST_EOF_PENDING		0x01	/* filemark pending */
#define	ST_EOF			0x02	/* at filemark */
#define	ST_EOT_PENDING		0x03	/* logical eot pending */
#define	ST_EOT			0x04	/* at logical eot */
#define	ST_EOM			0x05	/* at physical eot */
#define	ST_WRITE_AFTER_EOM	0x06	/* flag for allowing writes after EOM */

#define	IN_EOF(un)	(un->un_eof == ST_EOF_PENDING || un->un_eof == ST_EOF)


/*
 * Error levels
 */

#define	STERR_ALL		0
#define	STERR_UNKNOWN		1
#define	STERR_INFORMATIONAL	2
#define	STERR_RECOVERED		3
#define	STERR_RETRYABLE		4
#define	STERR_FATAL		5


/*
 * stintr codes
 */

#define	COMMAND_DONE		0
#define	COMMAND_DONE_ERROR	1
#define	COMMAND_DONE_ERROR_RECOVERED	2
#define	QUE_COMMAND		3
#define	QUE_SENSE		4
#define	JUST_RETURN		5

#ifndef	RELEASE_41
/*
 * Compatibility with 'old' MTIOCGET structure
 */

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
#endif	/* !RELEASE_41 */

/*
 * Parameters
 */
	/*
	 * 60 minutes seems a reasonable amount of time
	 * to wait for tape space operations to complete.
	 *
	 */
#define	ST_SPACE_TIME		60*60	/* 60 minutes per space operation */

	/*
	 * 2 minutes seems a reasonable amount of time
	 * to wait for tape i/o operations to complete.
	 *
	 */
#define	ST_IO_TIME		2*60	/* 2 minutes per i/o */



	/*
	 * 10 seconds is what we'll wait if we get a Busy Status back
	 */
#define	ST_BSY_TIMEOUT		10*hz	/* 10 seconds Busy Waiting */

	/*
	 * Number of times we'll retry a normal operation.
	 *
	 * XXX This includes retries due to transport failure as well as
	 * XXX busy timeouts- Need to distinguish between Target and Transport
	 * XXX failure.
	 */

#define	ST_RETRY_COUNT		20

	/*
	 * Maximum number of units (determined by minor device byte)
	 */

#define	ST_MAXUNIT	8

#ifndef	SECSIZE
#define	SECSIZE	512
#endif
#ifndef	SECDIV
#define	SECDIV	9
#endif
