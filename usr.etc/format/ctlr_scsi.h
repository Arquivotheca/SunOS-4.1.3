#ifndef lint
#ident	"@(#)ctlr_scsi.h	1.1	92/07/30 SMI"
#endif	lint

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#ifndef	__CTLR_SCSI__
#define	__CTLR_SCSI__



/*
 * Rounded parameter, as returned in Extended Sense information
 */
#define	ROUNDED_PARAMETER	0x37

/*
 * Mode sense/select page header information
 */
struct scsi_ms_header {
	struct mode_header	mode_header;
	struct block_descriptor	block_descriptor;
};

/*
 * Mode Sense Page Control
 */
#define	MODE_SENSE_PC_CURRENT		(0 << 6)
#define	MODE_SENSE_PC_CHANGEABLE	(1 << 6)
#define	MODE_SENSE_PC_DEFAULT		(2 << 6)
#define	MODE_SENSE_PC_SAVED		(3 << 6)

/*
 * Mode Select options
 */
#define	MODE_SELECT_SP			0x01
#define	MODE_SELECT_PF			0x10


/*
 * Request sense extensions:  offsets into es_add_len[]
 * in struct scsi_extended_sense.
 */
#define	ADD_SENSE_CODE		4
#define	ADD_SENSE_QUAL_CODE	5

#define	MIN_REQUEST_SENSE_LEN	18


/*
 * Convert a three-byte triplet into an int
 */
#define	TRIPLET(u, m, l)	((int) ((((u))&0xff<<16) + \
				(((m)&0xff)<<8) + (l&0xff)))

/*
 * Define the amount of slop we can tolerate on a SCSI-2 mode sense.
 * Usually we try to deal with just the common subset between the
 * the SCSI-2 structure and the CCS structure.  The length of the
 * data returned can vary between targets, so being tolerant gives
 * gives us a higher chance of success.
 */
#define	PAGE1_SLOP		5
#define	PAGE2_SLOP		6
#define	PAGE3_SLOP		3
#define	PAGE4_SLOP		8
#define	PAGE8_SLOP		8
#define	PAGE38_SLOP		8

/*
 * Minimum lengths of a particular SCSI-2 mode sense page that
 * we can deal with.  We must reject anything less than this.
 */
#define	MIN_PAGE1_LEN		(sizeof (struct mode_err_recov)-PAGE1_SLOP)
#define	MIN_PAGE2_LEN		(sizeof (struct mode_disco_reco)-PAGE2_SLOP)
#define	MIN_PAGE3_LEN		(sizeof (struct mode_format)-PAGE3_SLOP)
#define	MIN_PAGE4_LEN		(sizeof (struct mode_geometry)-PAGE4_SLOP)
#define	MIN_PAGE8_LEN		(sizeof (struct mode_cache)-PAGE8_SLOP)
#define	MIN_PAGE38_LEN		(sizeof (struct mode_cache_ccs)-PAGE38_SLOP)

/*
 * Macro to extract the length of a mode sense page
 * as returned by a target.
 */
#define	MODESENSE_PAGE_LEN(p)	(((int) ((struct mode_page *)p)->length) + \
					sizeof (struct mode_page))

/*
 * Request this number of bytes for all mode senses.  Since the
 * data returned is self-defining, we can accept anywhere from
 * the minimum for a particular page, up to this maximum.
 * Whatever the drive gives us, we return to the drive, delta'ed
 * by whatever we want to change.
 */
#define	MAX_MODE_SENSE_SIZE		255


/*
 * Format parameter to dump()
 */
#define	HEX_ONLY	0	/* dump data in hex notation only */
#define	HEX_ASCII	1	/* dump in both hex and ascii formats */

/*
 * List of strings with arbitrary matching values
 */
typedef struct slist {
	char	*str;
	int	value;
} slist_t;


#define	SC_ERR_UNKNOWN		0xff

/*
 * List of possible errors.
 */
struct dk_err {
	u_char errno;		/* error number */
	u_char errlevel;	/* error level (corrected, fatal, etc) */
	u_char errtype;		/* error type (media vs nonmedia) */
	char *errmsg;		/* error message */ 
};


/* 
 * Sense key values for extended sense.  Note,
 * values 0-0xf are standard sense key values.
 * Value 0x10+ are driver supplied.
 */
#define SC_NO_SENSE		0x00
#define SC_RECOVERABLE_ERROR	0x01
#define SC_NOT_READY		0x02
#define SC_MEDIUM_ERROR		0x03	/* parity error */
#define SC_HARDWARE_ERROR	0x04
#define SC_ILLEGAL_REQUEST	0x05
#define SC_UNIT_ATTENTION	0x06
#define SC_WRITE_PROTECT	0x07
#define SC_BLANK_CHECK		0x08
#define SC_VENDOR_UNIQUE	0x09
#define SC_COPY_ABORTED		0x0A
#define SC_ABORTED_COMMAND	0x0B
#define SC_EQUAL		0x0C
#define SC_VOLUME_OVERFLOW	0x0D
#define SC_MISCOMPARE		0x0E
#define SC_RESERVED		0x0F
#define SC_FATAL		0x10	/* driver, scsi handshake failure */
#define SC_TIMEOUT		0x11	/* driver, command timeout */
#define SC_EOF			0x12	/* driver, eof hit */
#define SC_EOT			0x13	/* driver, eot hit */
#define SC_LENGTH		0x14	/* driver, length error */
#define SC_BOT			0x15	/* driver, bot hit */
#define SC_MEDIA		0x16	/* driver, wrong media error */
#define SC_RESIDUE		0x17	/* driver, dma residue error */
#define SC_BUSY			0x18	/* driver, device busy error */
#define SC_RESERVATION		0x19	/* driver, reservation error */
#define SC_INTERRUPTED		0x1a	/* driver, user interrupted */


/*
 * Functions declarations
 */
int	scsi_rdwr();
int	scsi_ck_format();
int	scsi_format();
int	scsi_ms_page1();
int	scsi_ms_page2();
int	scsi_ms_page3();
int	scsi_ms_page4();
int	scsi_ms_page8();
int	scsi_ms_page38();
int	scsi_ex_man();
int	scsi_ex_cur();
int	scsi_ex_grown();
int	scsi_read_defect_data();
int	scsi_repair();
void	scsi_convert_list_to_new();
int	scsi_cmd();
int	scsi_uscsi_cmd();
int	uscsi_mode_sense();
int	uscsi_mode_select();
int	uscsi_request_sense();
int	uscsi_inquiry();
void	scsi_dump_mode_sense_pages();
void	scsi_printerr();
char	*scsi_find_command_name();
int	scsi_supported_page();
int	apply_chg_list();
char	*find_string();
void	dump();
int	scsi_maptodkcmd();
int	scsi_dkcmd();
void	scsi_dk_printerr();
struct	dk_err *scsi_dk_finderr();

#endif	!__CTLR_SCSI__
