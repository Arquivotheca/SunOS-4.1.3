/*	@(#)scsi.h 1.1 92/07/30 SMI	*/

/* Copyright (c) 1987, 1987 by Sun Microsystems, Inc. */

/*
 * Standard SCSI control blocks and definitions.
 * These go in or out over the SCSI bus.
 */
#define CDB_GROUPID(cmd)	((cmd >> 5) & 0x7)
#define CDB_GROUP0	6	/*  6-byte cdb's */
#define CDB_GROUP1	10	/* 10-byte cdb's */
#define CDB_GROUP2	0	/* reserved */
#define CDB_GROUP3	0	/* reserved */
#define CDB_GROUP4	0	/* reserved */
#define CDB_GROUP5	0	/* reserved */
#define CDB_GROUP6	0	/* reserved */
#define CDB_GROUP7	0	/* reserved */

/*
 * Standard SCSI control blocks.
 * These go in or out over the SCSI bus.
 * The first 11 bits of the command block are the same for all
 * three defined command groups.  The first byte is an operation code
 * which consists of a command code component and a group code 
 * component.   The first 3 bits of the second byte are the unit
 * number.
 *
 * The group code determines the length of the rest of
 * the command. Group 0 commands are 6 bytes, Group 1 are 10 bytes, 
 * and Group 5 are 12 bytes.  Groups 2-4 are reserved.  
 * Groups 6 and 7 are vendor unique.
 */
#define	CDB_PAD		(10 -2)		/* allow space for 10-byte cdb */
struct	scsi_cdb {			/* scsi command description block */
	u_char cmd;			/* cmd code (byte 0) */
	u_char lun	:3;		/* lun (byte 1) */
	u_char tag	:5;		/* rest of byte 1 */
	union {				 /* bytes 2 - 12 */
		u_char	scsi[CDB_PAD];	/* max cdb size -2 */


		/* 
	 	 *	G R O U P   0   F O R M A T (6 bytes)
		 */
#define		scc_cmd		cmd
#define		scc_lun		lun
#define		g0_addr2	tag
#define		g0_addr1	sg.g0.addr1
#define		g0_addr0	sg.g0.addr0
#define		g0_count0	sg.g0.count0
#define 	g0_vu_1		sg.g0.vu_57
#define 	g0_vu_0		sg.g0.vu_56
#define 	g0_flag		sg.g0.flag
#define 	g0_link		sg.g0.link
	/*
	 * defines for SCSI tape cdb (standalone boot).
	 */
#define		t_code		tag
#define		high_count	sg.g0.addr1
#define		mid_count	sg.g0.addr0
#define		low_count	sg.g0.count0
		struct scsi_g0 {
			u_char addr1;		/* middle part of address */
			u_char addr0;		/* low part of address */
			u_char count0;		/* usually block count */
			u_char vu_57	:1;	/* vendor unique (byte 5 bit 7*/
			u_char vu_56	:1;	/* vendor unique (byte 5 bit 6*/
			u_char rsvd	:4;	/* reserved */
			u_char flag	:1;	/* interrupt when done */
			u_char link	:1;	/* another command follows */
		} g0;


		/*
		 *	G R O U P   1   F O R M A T  (10 byte)
		 */
#define		g1_reladdr	tag
#define		g1_rsvd0	sg.g1.rsvd1
#define		g1_addr3	sg.g1.addr3	/* msb */
#define		g1_addr2	sg.g1.addr2
#define		g1_addr1	sg.g1.addr1
#define		g1_addr0	sg.g1.addr0	/* lsb */
#define		g1_count1	sg.g1.count1	/* msb */
#define		g1_count0	sg.g1.count0	/* lsb */
#define 	g1_vu_1		sg.g1.vu_97
#define 	g1_vu_0		sg.g1.vu_96
#define 	g1_flag		sg.g1.flag
#define 	g1_link		sg.g1.link
		struct scsi_g1 {
			u_char addr3;		/* most sig. byte of address*/
			u_char addr2;
			u_char addr1;
			u_char addr0;
			u_char rsvd1;		/* reserved (byte 6) */
			u_char count1;		/* transfer length (msb) */
			u_char count0;		/* transfer length (lsb) */
			u_char vu_97	:1;	/* vendor unique (byte 9 bit 7*/
			u_char vu_96	:1;	/* vendor unique (byte 9 bit 6*/
			u_char rsvd0	:4;	/* reserved */
			u_char flag	:1;	/* interrupt when done */
			u_char link	:1;	/* another command follows */
		} g1;
	}sg;
};


/*
 * defines for setting fields within the various command groups
 */
#define FORMG0COUNT(cdb, cnt)	(cdb)->g0_count0  = (cnt)
#define FORMG0ADDR(cdb, addr) 	(cdb)->g0_addr2  = (addr) >> 16;\
				(cdb)->g0_addr1  = ((addr) >> 8) & 0xFF;\
				(cdb)->g0_addr0  = (addr) & 0xFF

#define FORMG1COUNT(cdb, cnt)	(cdb)->g1_count1 = ((cnt) >> 8);\
				(cdb)->g1_count0 = (cnt) & 0xFF
#define FORMG1ADDR(cdb, addr)	(cdb)->g1_addr3  = (addr) >> 24;\
				(cdb)->g1_addr2  = ((addr) >> 16) & 0xFF;\
				(cdb)->g1_addr1  = ((addr) >> 8) & 0xFF;\
				(cdb)->g1_addr0  = (addr) & 0xFF

/*
 * Definition of direction of data flow for commands.
 */
#define	SCSI_RECV_DATA_CMD(cmd)	(((cmd) == SC_READ) || \
				 ((cmd) == SC_REQUEST_SENSE) || \
				 ((cmd) == SC_INQUIRY) || \
				 ((cmd) == SC_READ_DEFECT_LIST) || \
				 ((cmd) == SC_MODE_SENSE))

#define	SCSI_SEND_DATA_CMD(cmd)	(((cmd) == SC_WRITE) || \
				 ((cmd) == SC_MODE_SELECT) || \
				 ((cmd) == SC_REASSIGN_BLOCK) || \
				 ((cmd) == SC_WRITE_FILE_MARK) || \
				 ((cmd) == SC_FORMAT))

/* Directions for dma transfers */
#define SC_NO_DATA		0
#define SC_RECV_DATA		1
#define SC_SEND_DATA		2

/* initiator's scsi device id */
/*
 * SCSI status completion block.
 */
#define SCB_STATUS_MASK		0x0e

struct	scsi_scb {		/* scsi status completion block */
	/* byte 0 */
	u_char	ext_st1	: 1;	/* extended status (next byte valid) */
	u_char	vu_06	: 1;	/* vendor unique */
	u_char	vu_05	: 1;	/* vendor unique */
	u_char	is	: 1;	/* intermediate status sent */
	u_char	busy	: 1;	/* device busy or reserved */
	u_char	cm	: 1;	/* condition met */
	u_char	chk	: 1;	/* check condition: sense data available */
	u_char	vu_00	: 1;	/* vendor unique */
	/* byte 1 */
	u_char	ext_st2	: 1;	/* extended status (next byte valid) */
	u_char	reserved: 6;	/* reserved */
	u_char	ha_er	: 1;	/* host adapter detected error */
	/* byte 2 */
	u_char	byte2;		/* third byte */
};

	/*
	 * Standard (Non Extended) SCSI Sense. Used mainly by the
  	 * Adaptec ACB 4000 which is the only controller that
	 * does not support the Extended sense format.
	 */
#define SC_SENSE_LENGTH	32
struct	scsi_sense {		/* scsi sense for error classes 0-6 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char	code	: 7;	/* error class/code */
	u_char	high_addr;	/* high byte of block addr */
	u_char	mid_addr;	/* middle byte of block addr */
	u_char	low_addr;	/* low byte of block addr */
	u_char	extra[28];	/* pad this struct so it can hold max num */
				/* of sense bytes used by any of the SCSI */
				/* controllers. */

};

/*
 * SCSI tape REQUEST SENSE parameter block.  Note,
 * this structure should have an even number of bytes to
 * eliminate any byte packing problems with our host adapters.
 */
#define SC_CLASS_EXTENDED_SENSE 0x7     /* indicates extended sense */

struct	scsi_ext_sense {	/* scsi extended sense for error class 7 */
	/* byte 0 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char	type	: 7;	/* fixed at 0x70 */
	/* byte 1 */
	u_char	seg_num;	/* segment number, applies to copy cmd only */
	/* byte 2 */
	u_char	fil_mk	: 1;	/* file mark on device */
	u_char	eom	: 1;	/* end of media */
	u_char	ili	: 1;	/* incorrect length indicator */
	u_char		: 1;	/* reserved */
	u_char	key	: 4;	/* sense key, see below */
	/* bytes 3 through 7 */
	u_char	info_1;		/* information byte 1 */
	u_char	info_2;		/* information byte 2 */
	u_char	info_3;		/* information byte 3 */
	u_char	info_4;		/* information byte 4 */
	u_char	add_len;	/* number of additional bytes */
	/* bytes 8 through 13, CCS additions */
	u_char	optional_8;	/* CCS search and copy only */
	u_char	optional_9;	/* CCS search and copy only */
	u_char	optional_10;	/* CCS search and copy only */
	u_char	optional_11;	/* CCS search and copy only */
	u_char 	error_code;	/* error class & code */
	u_char			/* reserved */
};

/*
 * Returned status particular to Adaptec 4520 only
 */
#define dev_stat_1	 optional_10;	/* device status */
#define dev_stat_2	 optional_11;
#define dev_uniq_1	 addit_sense;	/* device vendor unique status */
#define dev_uniq_2	 reserved;

/*
 * Defines for Emulex MD21 SCSI/ESDI Controller. Extended sense for Format
 * command only.
 */
#define cyl_msb		info_1
#define cyl_lsb		info_2
#define head_num	info_3
#define sect_num	info_4

/* 
 * Sense key values for extended sense.  Note,
 * values 0-0xf are standard sense key values.
 * Value 0x80+ are driver supplied.
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

#define SENSE_KEY_INFO  {\
	"no sense",			/* 0x00 */ \
	"soft error",			/* 0x01 */ \
	"not ready",			/* 0x02 */ \
	"media error",			/* 0x03 */ \
	"hardware error",		/* 0x04 */ \
	"illegal request",		/* 0x05 */ \
	"unit attention",		/* 0x06 */ \
	"write protected",		/* 0x07 */ \
	"blank check",			/* 0x08 */ \
	"vendor unique",		/* 0x09 */ \
	"copy aborted",			/* 0x0a */ \
	"aborted command",		/* 0x0b */ \
	"equal error",			/* 0x0c */ \
	"volume overflow",		/* 0x0d */ \
	"miscompare error",		/* 0x0e */ \
	"reserved",			/* 0x0f */ \
	"fatal",			/* 0x10 */ \
	"timeout",			/* 0x11 */ \
	"EOF",				/* 0x12 */ \
	"EOT",				/* 0x13 */ \
	0					   \
}


struct	scsi_inquiry_data {	/* data returned as a result of CCS inquiry */
	/* byte 0 */
	u_char	dtype;		/* device type */
	/* byte 1 */
	u_char	rmb	: 1;	/* removable media */
	u_char	dtype_qual : 7;	/* device type qualifier */
	/* byte 2 */
	u_char	iso	: 2;	/* ISO version */
	u_char	ecma	: 3;	/* ECMA version */
	u_char	ansi	: 3;	/* ANSI version */
	/* byte 3 */
	u_char	reserv1	: 4;	/* reserved */
	u_char	rdf	: 4;	/* response data format */
	/* bytes 4-7 */
	u_char	add_len;	/* additional length */
	u_char	reserv2;	/* reserved */
	u_char	reserv3;	/* reserved */
	u_char	reserv4;	/* reserved */
	/* bytes 8-35 */
	char	vid[8];		/* vendor ID */
	char	pid[16];	/* product ID */
	char	revision[4];	/* revision level */
};

/*
 * SCSI Operation codes. 
 */
#define SC_TEST_UNIT_READY	0x00
#define SC_REZERO_UNIT		0x01
#define SC_REQUEST_SENSE	0x03
#define SC_FORMAT		0x04
#define SC_REASSIGN_BLOCK	0x07		/* CCS only */
#define SC_SEEK			0x0b
#define SC_TRANSLATE		0x0f		/* ACB4000 only */
#define SC_INQUIRY		0x12		/* CCS only */
#define SC_MODE_SELECT		0x15
#define SC_MODE_SENSE		0x1a
#define SC_START		0x1b
#define SC_READ_DEFECT_LIST	0x37		/* CCS only, group 1 */
	/*
	 * Note, these two commands use identical command blocks for all
 	 * controllers except the Adaptec ACB 4000 which sets bit 1 of byte 1.
	 */
#define SC_READ			0x08
#define SC_WRITE		0x0a
#define SC_WRITE_VERIFY		0x2e            /* 10 byte write+verify */
#define SC_WRITE_FILE_MARK	0x10
#define SC_UNKNOWN		0xff		/* cmd list terminator */


/*
 * Messages that SCSI can send.
 */
#define SC_COMMAND_COMPLETE	0x00
#define SC_SYNCHRONOUS		0x01
#define SC_SAVE_DATA_PTR	0x02
#define SC_RESTORE_PTRS		0x03
#define SC_DISCONNECT		0x04
#define SC_ABORT		0x06
#define SC_MSG_REJECT		0x07
#define SC_NO_OP		0x08
#define SC_PARITY		0x09
#define SC_IDENTIFY		0x80
#define SC_DR_IDENTIFY		0xc0
#define SC_DEVICE_RESET		0x0c

#define MORE_STATUS		0x80		/* More status flag */
#define STATUS_LEN		3		/* Max status len for SCSI */

#define NLPART			NDKMAP	/* number of logical partitions (8) */

/* misc controller flags */
#define SCSI_PRESENT	0x0001		/* scsi bus is alive */
#define SCSI_ONBOARD	0x0002		/* scsi logic is onboard SCSI-3 */
#define SCSI_EN_RECON	0x0004		/* reconnect attempts are enabled */
#define SCSI_EN_DISCON	0x0008		/* disconnect attempts are enabled */
#define SCSI_FLUSH_DISQ	0x0010		/* flush disconnected tasks */
#define SCSI_FLUSHING	0x0020		/* flushing in progress */
#define SCSI_VME	0x0040		/* is a VME SCSI-3 */
#define SCSI_COBRA	0x0080		/* is a COBRA SCSI */

#define IS_ONBOARD(c)	(c->c_flags & SCSI_ONBOARD)
#define IS_VME(c)	(c->c_flags & SCSI_VME)
#define IS_SCSI3(c)	(IS_ONBOARD(c) || IS_VME(c))

/*
 * This contains the structure for accessing the scsi host adapters.  It is
 * composed of a subset of the saio structure.  For Sun systems there are
 * four possible scsi host configuration.
 *
 *	1) Scsi-2 interface, multibus
 *	2) Scsi-2 interface, VME 
 *	2) Scsi-3 interface, VME on controller card
 *	3) Scsi-3 interface, VME on board system card
 */
#define SC_NSC		2		/* number of boards */
struct host_saioreq {
	int	ctlr;			/* ctlr. number */
	int	unit;			/* unit number within controller */
	char	*devaddr;		/* pointer to mapped in device */
	char	*dmaaddr;		/* pointer to allocated dma space */
	int	ob;			/* on-board flag */
	int	dma_dir;		/* dma direction */
	int	cc;			/* character count */
	char	*ma;			/* memory address */
	int	(*doit)();		/* doit command routine */
	int	(*reset)();		/* reset command routine */
};

/*
 * SCSI timing constants.
 */
#define SCSI_RESET_DELAY	5000000	/* Reset recovery time (5 sec)*/
#define SCSI_SEL_DELAY		25000	/* Selection timeout (250 ms) */
#define SCSI_LONG_DELAY		12000000 /* Phase Change timeout (2 min) */
#define SCSI_SHORT_DELAY	1000000	/* Phase Change timeout (10 sec) */

/* 
 * Unit specific subroutines called from controllers.
 */
/*
 * Defines for getting configuration parameters out of mb_device.
 */
#define TYPE(flags)	(flags)
#define TARGET(slave)	((slave >> 3) & 0x07)
#define LUN(slave)	(slave & 0x07)
#define NUNIT		8	/* max nubmer of units per controller */

#define SCSI_DISK	0
#define SCSI_TAPE	1
#define SCSI_FLOPPY	2
#define SCSI_OPTICAL	3


/*
 * SCSI Error codes passed to device routines.
 * The codes refer to SCSI general errors, not to device
 * specific errors.  Device specific errors are discovered
 * by checking the sense data.
 * The distinction between retryable and fatal is somewhat ad hoc.
 */
#define SE_NO_ERROR	0
#define SE_RETRYABLE	1
#define SE_FATAL	2
#define SE_TIMEOUT	3		/* driver timed out */

#define	NBAD		16		/* maximum number of bad blocks
					 * really should be even value
					 */
struct	dk_bad {
	int	block;			/* Absolute location of marginal block */
	short	retries;		/* Number of times block failed
					 * (recoverable errors).
					 */
};
