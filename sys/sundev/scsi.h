/* @(#)scsi.h	1.1 92/07/30 Copyr 1989 Sun Micro */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */
#ifndef _sundev_scsi_h
#define _sundev_scsi_h

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
struct	scsi_cdb {			/* scsi command description block */
	u_char cmd;			/* cmd code (byte 0) */
	u_char lun	:3;		/* lun (byte 1) */
	u_char tag	:5;		/* rest of byte 1 */
	union {				 /* bytes 2 - 12 */
		u_char	scsi[22];	/* allow space for 24-byte cdb */


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


		/*
		 *	G R O U P   5   F O R M A T  (12 byte)
		 */
#define		scc5_reladdr	tag
#define		scc5_addr3	sg.g5.addr3	/* msb */
#define		scc5_addr2	sg.g5,addr2
#define		scc5_addr1	sg.g5.addr1
#define		scc5_addr0	sg.g5.addr0	/* lsb */
#define		scc5_count1	sg.g5.count1	/* msb */
#define		scc5_count0	sg.g5.count0	/* lsb */
#define		scc5_vu_1	sg.g5.v1
#define		scc5_vu_0	sg.g5.v0
#define		scc5_flag	sg.g5.flag
		struct scsi_g5 {
			u_char addr3;	/* most sig. byte of address*/
			u_char addr2;
			u_char addr1;
			u_char addr0;
			u_char rsvd3;		/* reserved */
			u_char rsvd2;		/* reserved */
			u_char rsvd1;		/* reserved */
			u_char count1;		/* transfer length (msb) */
			u_char count0;		/* transfer length (lsb) */
			u_char vu_117	:1;	/* vendor unique (byte 11 bit 7*/
			u_char vu_116	:1;	/* vendor unique (byte 11 bit 6*/
			u_char rsvd0	:4;	/* reserved */
			u_char flag	:1;	/* interrupt when done */
			u_char link	:1;	/* another command follows */
		} g5;
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

#define FORMG5COUNT(cdb, cnt)	(cdb)->g5_count1 = ((cnt) >> 8);\
				(cdb)->g5_count0 = (cnt) & 0xFF
#define FORMG5ADDR(cdb, addr)	(cdb)->g5_addr3  = (addr) >> 24;\
				(cdb)->g5_addr2  = ((addr) >> 16) & 0xFF;\
				(cdb)->g5_addr1  = ((addr) >> 8) & 0xFF;\
				(cdb)->g5_addr0  = (addr) & 0xFF


/*
 * Definition of direction of data flow for commands.
 */
#define	SCSI_RECV_DATA_CMD(cmd)	(((cmd) == SC_READ) || \
				 ((cmd) == SC_EREAD) || \
				 ((cmd) == SC_REQUEST_SENSE) || \
				 ((cmd) == SC_INQUIRY) || \
				 ((cmd) == SC_READ_DEFECT_LIST) || \
				 ((cmd) == SC_MODE_SENSE) || \
				 ((cmd) == SC_READ_BUFFER))

#define	SCSI_SEND_DATA_CMD(cmd)	(((cmd) == SC_WRITE) || \
				 ((cmd) == SC_EWRITE) || \
				 ((cmd) == SC_MODE_SELECT) || \
				 ((cmd) == SC_REASSIGN_BLOCK) || \
				 ((cmd) == SC_WRITE_FILE_MARK) || \
				 ((cmd) == SC_FORMAT))

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
struct	scsi_sense {		/* scsi sense for error classes 0-6 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char	code	: 7;	/* error class/code */
	u_char	high_addr;	/* high byte of block addr */
	u_char	mid_addr;	/* middle byte of block addr */
	u_char	low_addr;	/* low byte of block addr */
	u_char	extra[14];	/* pad this struct so it can hold max num */
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
	"length error",			/* 0x14 */ \
	"BOT",				/* 0x15 */ \
	"wrong tape media",		/* 0x16 */ \
	"dma residue",			/* 0x17 */ \
	"busy",				/* 0x18 */ \
	"reservation",			/* 0x19 */ \
	"user interrupted",		/* 0x1a */ \
	0					   \
}


struct	scsi_inquiry_data {	/* data returned as a result of CCS inquiry */
	/* byte 0 */
	u_char	pqualifier :3;	/* peripheral qualifier */
	u_char	dtype	:5;	/* device type */
	/* byte 1 */
	u_char	rmb	: 1;	/* removable media */
	u_char	dtype_qual : 7;	/* device type qualifier */
	/* byte 2 */
	u_char	iso	: 2;	/* ISO version */
	u_char	ecma	: 3;	/* ECMA version */
	u_char	ansi	: 3;	/* ANSI version */
	/* byte 3 */
	u_char	aenc	: 1;	/* asynch event notification */
	u_char	reserv1	: 3;	/* reserved */
	u_char	rdf	: 4;	/* response data format */
	/* bytes 4-7 */
	u_char	add_len;	/* additional length */
	u_char	reserv2;	/* reserved */
	u_char	reserv3;	/* reserved */
	u_char	reladr	: 1;	/* supports relative addressing */
	u_char	wbus32	: 1;	/* supports 32-bit wide scsi */
	u_char	wbus16	: 1;	/* supports 16-bit wide scsi */
	u_char	sync	: 1;	/* supports synchronous */
	u_char	link	: 1;	/* supports linked cmds */
	u_char	cache	: 1;	/* supports cacheing (optional) */
	u_char	cmdque	: 1;	/* supports cmd queueing */
	u_char	sftre	: 1;	/* supports soft reset */
	/* bytes 8-55 */
	char	vid[8];		/* vendor ID */
	char	pid[16];	/* product ID */
	char	revision[4];	/* revision level */
	char	ucode_rev[8];	/* microcode revision level (optional) */
	char	serialno[12];	/* drive serial number (optional) */
};

/*
 * SCSI Operation codes. 
 */
#define SC_TEST_UNIT_READY	0x00
#define SC_REZERO_UNIT		0x01
#define SC_REQUEST_SENSE	0x03
#define SC_FORMAT		0x04
#define SC_FORMAT_TRACK		0x06
#define SC_REASSIGN_BLOCK	0x07		/* CCS only */
#define SC_SEEK			0x0b
#define SC_TRANSLATE		0x0f		/* ACB4000 only */
#define SC_INQUIRY		0x12		/* CCS only */
#define SC_MODE_SELECT		0x15
#define SC_RESERVE		0x16
#define SC_RELEASE		0x17
#define SC_MODE_SENSE		0x1a
#define SC_START		0x1b
#define SC_READ_DEFECT_LIST	0x37		/* CCS only, group 1 */
#define SC_READ_BUFFER          0x3c            /* CCS only, group 1 */
	/*
	 * Note, these two commands use identical command blocks for all
 	 * controllers except the Adaptec ACB 4000 which sets bit 1 of byte 1.
	 */
#define SC_READ			0x08
#define SC_WRITE		0x0a
#define SC_EREAD		0x28		/* 10 byte read */
#define SC_EWRITE		0x2a		/* 10 byte write */
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
#define NTARGETS		8	/* number of targets */
#define NLUNS			8	/* number of luns per target */

/*
 * SCSI device unit block.  Warning, must be even-byte aligned.
 */
struct scsi_unit {
	char	un_target;		/* scsi bus address of controller */
	char	un_lun;			/* logical unit number of device */
	char	un_present;		/* unit is present */
	u_char	un_scmd;		/* special command */
	u_short	un_unit;		/* real unit number of device */
	u_short	un_flags;		/* misc flags relating to cur xfer */
	struct	scsi_unit_subr *un_ss;	/* scsi device subroutines */
	struct	scsi_ctlr *un_c;	/* scsi ctlr */
	struct	mb_device *un_md;	/* mb device */
	struct	mb_ctlr *un_mc;		/* mb controller */
	struct	buf un_sbuf;		/* fake buffer for special commands */
	struct	buf un_rbuf;		/* buffer for raw i/o */
	struct	scsi_cdb un_cdb;	/* command description block */
	struct	scsi_scb un_scb;	/* status completion block */
	/* current transfer: */
	int	un_baddr;		/* virtual buffer address */
	int	un_blkno;		/* current block */
	int	un_count;		/* num sectors to xfer (for cdb) */
	short	un_cmd;			/* command (for cdb) */
	int	un_dma_addr;		/* dma address */
	u_short	un_dma_count;		/* byte count expected */
	u_char	un_wantint;		/* expecting interrupt */
	u_char	un_cmd_len;		/* cdb length (special cdb's)
	/* the following save current dma information in case of disconnect */
	int	un_dma_curaddr;		/* current addr to start dma to/from */
	u_short	un_dma_curcnt;		/* current dma count */
	u_short	un_dma_curdir;		/* direction of dma transfer */
	u_short	un_dma_curbcr;		/* bcr, saved at head of intr routine */
	/* the following is a copy of current unit command info */
	struct un_saved_cmd_info {
		short	saved_cmd; 		/* saved cmd */
		struct	scsi_cdb saved_cdb; 	/* saved cdb */
		struct	scsi_scb saved_scb; 	/* saved scb */
		int	saved_resid;		/* saved amt untransferred */
		int	saved_dma_addr;		
		u_short	saved_dma_count;
	} un_saved_cmd;
	caddr_t	un_scratch_addr;	/* scratch buffer */
	struct	scsi_unit *un_forw;	/* next unit on device queue */
};

/* 
 * bits in the scsi unit flags field
 */
#define SC_UNF_DVMA		0x0001	/* set if cur xfer requires dvma */
#define SC_UNF_SPECIAL_DVMA	0x0002	/* set if xfer requires special dvma */
#define SC_UNF_PREEMPT		0x0004	/* cur xfer was preempted by recon */
#define SC_UNF_DMA_ACTIVE	0x0008	/* DMA active on this unit */
#define SC_UNF_RECV_DATA	0x0010	/* direction of data xfer is recieve */
#define SC_UNF_GET_SENSE	0x0020	/* run get sense cmd for failed cmd */
#define SC_UNF_DMA_INITIALIZED	0x0040	/* initialized DMA hardware */
#define SC_UNF_NO_DISCON	0x0080  /* disable disconnects */
#define SC_UNF_WORD_XFER	0x0100  /* Unaligned word transfer */

#define	NPHASE		8		/* phase history */
struct sc_phases {
	short phase;
	short arg;
	int target;
	int lun;
};

struct scsi_ctlr {
	int	c_flags;		/* misc state flags */
	int	c_reg;			/* controller registers in I/O space */
	int	c_intpri;		/* controller interrupt priority */
	struct 	scsi_unit *c_un;	/* scsi unit using the bus */
	struct	scsi_unit *c_drainun;	/* scsi unit being drained */
	struct	scsi_ctlr_subr *c_ss;	/* scsi device subroutines */
	struct	udc_table *c_udct;	/* scsi dma info for Sun3/50 */
	struct	buf c_tab;		/* queue of pending devices */
	struct  buf c_disqtab;		/* disconnect queue of devices */
	struct	buf *c_flush;		/* ptr to last element to flush */
	char    *c_name;                /* host adaptor name */
	short	c_recon_target;		/* reconnecting target */
	short	c_last_phase;		/* last SCSI bus phase */
	short	c_phase_index;		/* next entry in c_phases table */
	long	c_kment;		/* ptr to reserved kernel page for */
					/* draining dma left-over odd bytes */
	struct	sc_phases c_phases[NPHASE];
};

/* misc controller flags */
#define SCSI_PRESENT	0x0001		/* scsi bus is alive */
#define SCSI_ONBOARD	0x0002		/* scsi logic is onboard SCSI-3 */
/*#define SCSI_EN_RECON	0x0004		/* RESERVED, UNUSED */
#define SCSI_EN_DISCON	0x0008		/* disconnect attempts are enabled */
#define SCSI_FLUSH_DISQ	0x0010		/* flush disconnected tasks */
#define SCSI_FLUSHING	0x0020		/* flushing in progress */
/*#define SCSI_VME	0x0040		/* RESERVED, UNUSED */
#define SCSI_COBRA	0x0080		/* scsi logic is 4/110 */

#define IS_ONBOARD(c)	(c->c_flags & SCSI_ONBOARD)
#define IS_COBRA(c)	(c->c_flags & SCSI_COBRA)
#define IS_VME(c)	((c->c_flags & SCSI_ONBOARD) == 0)

/* 
 * Unit specific subroutines called from controllers.
 */
struct	scsi_unit_subr {
	int	(*ss_attach)();
	int	(*ss_start)();
	int	(*ss_mkcdb)();
	int	(*ss_intr)();
	int	(*ss_unit_ptr)();
	char	*ss_devname;
};

/* 
 * Controller specific subroutines called from units.
 */
struct	scsi_ctlr_subr {
	int	(*scs_ustart)();
	int	(*scs_start)();
	int	(*scs_done)();
	int	(*scs_cmd)();
	int	(*scs_getstat)();
	int	(*scs_cmd_wait)();
	int	(*scs_off)();
	int	(*scs_reset)();
	int	(*scs_dmacount)();
	int	(*scs_go)();
	int	(*scs_deque)();
};

/*
 * Defines for getting configuration parameters out of mb_device.
 */
#define TYPE(flags)	(flags & 0xFF)
#define TARGET(slave)	((slave >> 3) & 07)
#define LUN(slave)	(slave & 07)
#define NUNIT		8		/* max units per controller */
#define SCUNIT(dev)     (minor(dev) & 0x07)

#define SCSI_DISK	0
#define SCSI_TAPE	1
#define SCSI_FLOPPY	2
#define SCSI_OPTICAL	3

/*
 * scsi_ctlr and mb_device interlock flags
 */
#define C_INACTIVE		0x0	/* SCSI controller inactive */ 
#define C_QUEUED		0x1	/* SCSI controller queued */
#define C_ACTIVE		0x2	/* SCSI controller active */
#define C_CURRENT		0x4     /* current SCSI command */
#define C_FLUSHING		0x8     /* SCSI controller timeout flush */
#define MD_INACTIVE		0x0	/* mb_device inactive */
#define MD_QUEUED		0x1	/* mb_device queued */
#define MD_PREEMPT		0x2	/* mb_device preempted */
#define MD_IN_PROGRESS		0x4	/* mb_device active */
#define MD_NODVMA		0x8	/* mb_device waiting for resources */


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


/*
 * SCSI disk device driver structure.  Warning, must be even-byte aligned.
 */
struct scsi_disk {
	struct	dk_map un_map[NLPART];	/* logical partitions */
	struct	dk_geom un_g;		/* disk geometry */
	struct	dk_bad un_bad[NBAD];	/* bad block table (for reassigns) */
	short	un_bad_index;		/* next entry in un_bad table */
	u_int	un_cyl_size;		/* blocks (sectors) per cyl */
	daddr_t	un_cyl_start;		/* first block of cyl */
	daddr_t	un_cyl_end;		/* last  block of cyl */
	u_int	un_cylin_last;		/* last cyl accessed */
	int	un_lp1;			/* last partiton accessed */
	int	un_lp2;			/* last partiton accessed */
	int	un_last_cyl_size;	/* last cylinder size accessed */
	u_int	un_sense;		/* ptr to request sense buffer */
	u_int	un_timeout;		/* timeout time (in minutes.) */
	u_int	un_total_errors;	/* total errors to date */
	int	un_err_resid;		/* resid from last error */
	int	un_err_blkno;		/* disk block where error occurred */
	short	un_retries;		/* retry count */
	short	un_restores;		/* restore count */
	u_short	un_sec_left;		/* sector count for single sectors */
	u_short	un_timer;		/* timer is on */
	u_short	un_status;		/* sense key from last error */
	u_short	un_flags;		/* special global unit flags,      */
					/*   disables disconnect/reconnect */
	u_char	un_err_severe;		/* error severity */
	u_char	un_err_code;		/* vendor unique error code  */
	u_char	un_last_cmd;		/* last scsi command issued */
	u_char	un_openf;		/* disk open state */
	u_char	un_ctype;		/* disk controller type */
	u_char	un_options;		/* driver options, bit encoded */
					/*   disables disconnect/reconnect */
	/*u_char;			/* reserved  */
};


/*
 * SCSI tape device driver structure.  Warning, must be even-byte aligned.
 */
struct scsi_tape {
	int	*un_mspl;		/* aligned internal buffer */
	int	*un_rmspl;		/* unaligned internal buffer */
	int	un_msplsize;		/* size of internal buffer */
	int	*un_dtab;		/* ptr to drive table list */
	int	un_bufcnt;		/* number of buffers in use */
	int	un_timeout;		/* timeout time (in minutes.) */
	int	un_offset;		/* byte offset into file */
	int	un_iovlen;		/* last transfer request size (bytes) */
	int	un_next_block;		/* next record on tape */
	int	un_last_block;		/* last record of file, if known */
	int	un_fileno;		/* current file number on tape */
	int	un_dev_bsize;		/* device unit block size (bytes) */
	int	un_phys_bsize;		/* device physical block size (bytes) */
	u_long	un_blocks;		/* total 512-byte blocks transferred */
	u_long	un_records;		/* total records transferred */
	int	un_err_fileno;		/* file where error occurred */
	int	un_err_blkno;		/* block in file where error occurred */
	int	un_err_resid;		/* resid from last error */
	u_int	un_options;		/* driver options, bit options */
					/*   0 = variable block-length */
					/*   1 = supports BSF */
					/*   2 = supports BSR */
	u_short	un_flags;		/* special global unit flags,      */
					/*   disables disconnect/reconnect */
	short	un_status;		/* status from last sense */
	u_long	un_retry_ct;		/* retry count */
	short	un_retry_limit;		/* retry limit */
	short	un_underruns;		/* number of underruns */
	u_short	un_last_count;		/* last num blocks xfered */
	u_short	un_last_bcount;		/* last num blocks xfered (for cdb) */
	char	un_openf;		/* tape open state */
	u_char	un_ctype;		/* controller type */
	u_char	un_read_only;		/* tape is read only */
	u_char	un_lastiow;		/* last I/O was write */
	u_char	un_lastior;		/* last I/O was read */
	u_char	un_reten_rewind;	/* retension on next rewind */
	u_char	un_density;		/* current tape density or qic format */
	u_char	un_reset_occurred;	/* reset occured since last command */
	u_char	un_last_cmd;		/* last scsi command issued */
	u_char	un_eof;			/* eof = 0, not found */
					/*     = 1, eof hit */
					/*     = 2, eof pending */
/*	u_char;				/* for even-byte alignment */
};

/*
 * SCSI floppy driver structure.  Warning, must be even-byte aligned.
 */
struct scsi_floppy {
	u_char	sf_flags;		/* various flags */
	u_char	sf_mdb;			/* media descriptor byte */
	u_short	sf_rate;		/* controller specific rate */
	int	sf_nhead;		/* number of heads */
	int	sf_nblk;		/* number of blocks on the disk */
	int	sf_ncyl;		/* number of tracks */
/* below is the stateful information about the floppy */
	int	sf_spt;			/* sectors per track */
	int	sf_xrate;		/* transfer rate */
	int	sf_bsec;		/* bytes per sector */
	int	sf_step;		/* step rate */
/* above is the stateful information about the floppy */
	struct	dk_map un_map[NLPART];	/* logical partitions */
	struct	dk_geom un_g;		/* disk geometry */
	u_int	un_cyl_size;		/* blocks (sectors) per cyl */
	daddr_t	un_cyl_start;		/* first block of cyl */
	daddr_t	un_cyl_end;		/* last  block of cyl */
	u_short	un_cylin_last;		/* last cyl accessed */
	int	un_lp1;			/* last partiton accessed */
	int	un_lp2;			/* last partiton accessed */
	int	un_last_cyl_size;	/* last cylinder size accessed */
	u_int	un_sense;		/* ptr to request sense buffer */
	u_int	un_timeout;		/* timeout time (in minutes.) */
	u_int	un_total_errors;	/* total errors to date */
	u_int	un_err_resid;		/* resid from last error */
	u_int	un_err_blkno;		/* disk block where error occurred */
	u_short	un_flags;		/* special global unit flags,      */
					/*   disables disconnect/reconnect */
	u_short	un_retries;		/* retry count */
	u_short	un_restores;		/* restore count */
	u_short	un_sec_left;		/* sector count for single sectors */
	u_short	un_timer;		/* timer is on */
	u_short	un_status;		/* sense key from last error */
	char	un_openf;		/* disk open state */
	u_char	un_err_severe;		/* error severity */
	u_char	un_err_code;		/* vendor unique error code  */
	u_char	un_last_cmd;		/* last scsi command issued */
	u_char	un_ctype;		/* disk controller type */
	u_char	un_options;		/* driver options, bit encoded */
};

#endif _sundev_scsi_h
