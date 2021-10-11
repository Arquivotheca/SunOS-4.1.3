/*
 * @(#)smreg.h 1.1 92/07/30 Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 *	Emulex ESP SCSI (low-level) driver header file
 *
 *	5/90	JK	Simplified reg defs & added bus analyzer
 *	7/89	KSAM	Merge into 4.1-beta release
 *	6/89	KSAM	Merge into 4.1-alpha10 release
 *	4/89	KSAM	Merge into 4.0.3-FCS release
 *	2/89	KSAM	Merge into 4.0.3-beta2 release
 *	12/88	KSAM	Merge into 4.0.3-beta release
 *	5/88	KSAM	Add in supports for Hydra, Campus, and P4-ESP
 *	2/88	KSAM	initial written for Stingray
 *
 */
#if (defined sun3x) || (defined sun4)

/* must be byte accesses in Campus */
/*
 * Emules ESP (Enhanced SCSM Controller) Registers.
 */
/* read of ESP's registers yields the following: */
struct esp_read_reg {	/* for P4 card, it is always 32 bit access */
	u_char	xcnt_lo,	/* transfer counter (low byte)*/
		pad1[3];

	u_char	xcnt_hi,	/* transfer counter (high byte)*/
		pad2[3];

	u_char	fifo_data,	/* fifo data buffer */
		pad3[3];

	u_char	cmd,		/* command register */
		pad4[3];

	u_char	stat,		/* status register */
		pad5[3];

	u_char	intr,		/* interrupt status register */
		pad6[3];

	u_char	step,		/* sequence step register */
		pad7[3];

	u_char	fifo_flag,	/* fifo flag register */
		pad8[3];

	u_char	conf,		/* configuration register, */
		pad9[3];

	u_char	reserved1,	/* reserved */
		pad10[3];

	u_char	reserved2,	/* reserved */
		pad11[3];

	u_char	conf2,		/* ESP-II configuration register */
		pad12[3];
};

/* write of ESP's registers yields the following: */
struct esp_write_reg {	/* for P4 card, it is always 32 bit access */
	u_char	xcnt_lo,	/* transfer counter (low byte)*/
		pad1[3];

	u_char	xcnt_hi,	/* transfer counter (high byte)*/
		pad2[3];

	u_char	fifo_data,	/* fifo data buffer */
		pad3[3];

	u_char	cmd,		/* command register */
		pad4[3];

	u_char	busid,		/* bus_id register */
		pad5[3];

	u_char	timeout,	/* timeout limit register */
		pad6[3];

	u_char	sync_period,	/* sync period register */
		pad7[3];

	u_char	sync_offset,	/* sync offset register */
		pad8[3];

	u_char	conf,		/* configuration register, */
		pad9[3];

	u_char	clock_conv,	/* clock conversion register */
		pad10[3];

	u_char	test,		/* test register */
		pad11[3];

	u_char	conf2,		/* ESP-II configuration register */
		pad12[3];
};

/* DMA GATE-ARRAY registers */
struct udc_table {
	u_long	scsi_ctlstat;	/* 32 bit for HYDRA */
	u_long	scsi_dmaaddr;	/* 32 bit dor hydra */
};

#define STINGRAY 		1		/* boot ID_PROM type */
#define HYDRA 			2
#define CAMPUS 			3

#define STINGRAY_BUSID 		0		/* bit 31-28 in DVMA status reg */
#define CAMPUS_BUSID		1
#define HYDRA_BUSID		2
#define SM_SIZE			0x2000		/* ESP and DVMA space */	

#define P4DVMA_SCSI_BASE	0xff600000
#define STINGRAY_SCSI_BASE	0xfa000000
#define STINGRAY_DMA_OFFSET	0x1000		/* SCSI_BASE + OFFSET */

#define CAMPUS_SCSI_BASE	0xf8800000
#define CAMPUS_DMA_OFFSET	0x400000	/* SCSI_BASE - OFFSET */

#define HYDRA_SCSI_BASE		0x66000000
#define HYDRA_DMA_OFFSET	0x1000		/* SCSI_BASE + OFFSET */

#define P4ESP_SCSI_BASE		0xff200200
#define P4ESP_DMA_OFFSET	0x103		/* SCSI_BASE + OFFSET */

/* bits in the ESP's initiator command register */
#define CMD_DMA		0x80	/* =1= enable DMA */

/* Several common Initiator command */
#define	CMD_NOP		0x0	/* misc command */	/* NO INT */
#define	CMD_FLUSH	0x1				/* NO INT */
#define	CMD_RESET_ESP	0x2				/* NO INT */
#define	CMD_RESET_SCSI	0x3				/* NO INT */
#define	CMD_RESEL_SEQ	0x40	/* disconnect command */	
#define	CMD_SEL_NOATN	0x41
#define	CMD_SEL_ATN	0x42
#define	CMD_SEL_STOP 	0x43
#define	CMD_EN_RESEL	0x44				/* NO INT */
#define	CMD_DIS_RESEL	0x45
#define	CMD_TRAN_INFO	0x10	/* initiator command */
#define	CMD_COMP_SEQ	0x11
#define	CMD_MSG_ACPT	0x12
#define	CMD_TRAN_PAD	0x18
#define	CMD_SET_ATN	0x1a				/* NO INT */


/* bits in the ESP's status register (read only) 	*/
#define ESP_STAT_RES	0x80	/* reserved */
#define ESP_STAT_GERR	0x40	/* gross error */
#define ESP_STAT_PERR	0x20	/* parity error */
#define ESP_STAT_XZERO	0x10	/* transfer counter zero */
#define ESP_STAT_XCMP	0x8	/* transfer completed */
#define ESP_STAT_MSG	0x4	/* scsi phase bit: MSG */
#define ESP_STAT_CD	0x2	/* scsi phase bit: CD */
#define ESP_STAT_IO	0x1	/* scsi phase bit: IO */
#define STAT_ERR_MASK	(ESP_STAT_GERR | ESP_STAT_PERR)
#define STAT_RES_MASK	~(ESP_STAT_RES)

/* settings of status to reflect different information transfer phases */
#define ESP_PHASE_MASK		(ESP_STAT_MSG | ESP_STAT_CD | ESP_STAT_IO)
#define ESP_PHASE_DATA		(ESP_PHASE_DATA_IN | ESP_PHASE_DATA_OUT)
#define ESP_PHASE_DATA_OUT	0
#define ESP_PHASE_DATA_IN	(ESP_STAT_IO)
#define ESP_PHASE_COMMAND	(ESP_STAT_CD)
#define ESP_PHASE_STATUS	(ESP_STAT_CD | ESP_STAT_IO)
#define ESP_PHASE_MSG_OUT	(ESP_STAT_MSG | ESP_STAT_CD)
#define ESP_PHASE_MSG_IN	(ESP_STAT_MSG | ESP_STAT_CD | ESP_STAT_IO)

#define ESP_PHASE_SMURIOUS	0x80	/* for driver use only */
#define ESP_PHASE_ARBITRATION	0x81	/* for driver use only */
#define ESP_PHASE_IDENTIFY	0x82	/* for driver use only */
#define ESP_PHASE_SAVE_PTR	0x83	/* for driver use only */
#define ESP_PHASE_RESTORE_PTR	0x84	/* for driver use only */
#define ESP_PHASE_DISCONNECT	0x85	/* for driver use only */
#define ESP_PHASE_CMD_CPLT	0x86	/* for driver use only */

/* FIXME: NEED A WAY TO TELL DATA OUT FROM BUS FREE */
#define PHASE_STRING_DATA {\
	"DATA OUT/BUS FREE", \
	"DATA IN", \
	"COMMAND", \
	"STATUS", \
	"", \
	"", \
	"MSG OUT", \
	"MSG IN", \
	"BUS FREE", \
};


/* bits in the ESP's interrupt status register */
#define ESP_INT_RESET	0x80	/* SCSI reset detected */
#define ESP_INT_ILLCMD	0x40	/* illegal cmd */
#define ESP_INT_DISCON	0x20	/* disconnect */
#define ESP_INT_BUS	0x10	/* bus service */
#define ESP_INT_FCMP	0x8	/* function completed */
#define ESP_INT_RESEL	0x4	/* reselected */
#define ESP_INT_SELATN	0x2	/* selected with ATN */
#define ESP_INT_SEL	0x1	/* selected without ATN */
#define ESP_INT_MASK	0xfc	/* all execpt least 2 bits */

#define INT_OK_MASK	(ESP_INT_BUS | ESP_INT_FCMP)
#define INT_RESEL_OK	(ESP_INT_RESEL | ESP_INT_FCMP)
#define INT_RESEL_OK1	(ESP_INT_RESEL | ESP_INT_BUS)
#define INT_DISCON_OK	(ESP_INT_DISCON | ESP_INT_FCMP)
#define INT_DISCON_OK1	(ESP_INT_DISCON | ESP_INT_BUS)
#define INT_ILL_BUS		(ESP_INT_ILLCMD | ESP_INT_BUS)
#define INT_ILL_FCMP		(ESP_INT_ILLCMD | ESP_INT_FCMP)
#define INT_ILL_DISCON		(ESP_INT_ILLCMD | ESP_INT_DISCON)
#define INT_ILL_DISCON_OK	(ESP_INT_ILLCMD | INT_DISCON_OK)
#define INT_ILL_DISCON_OK1	(ESP_INT_ILLCMD | INT_DISCON_OK1)
#define INT_ILL_RESEL		(ESP_INT_ILLCMD | ESP_INT_RESEL)
#define INT_ILL_RESEL_OK	(ESP_INT_ILLCMD | INT_RESEL_OK)
#define INT_ILL_RESEL_OK1	(ESP_INT_ILLCMD | INT_RESEL_OK1)

/* bits in the ESP's step sequnce register */
#define ESP_SEQ_STEP		0x7	/* last 3 bits */

/* bits in the ESP's configuration register */
#define ESP_CONF_SLOWMODE	0x80	/* slow cable mode */
#define ESP_CONF_DISRINT	0x40	/* disable reset int */
#define ESP_CONF_PARTEST	0x20	/* parity test mode */
#define ESP_CONF_PAREN		0x10	/* enable parity */
#define ESP_CONF_CHIPTEST	0x8	/* chip test mode */
#define ESP_CONF_BUSID		0x7	/* last 3 bits to be host id 7 */
#define DEF_ESP_HOSTID		0x7	/* higher priority */

/* bits in the DVMA status register */
#define DVMA_INTPEND	0x1	/* (r) interrupt pending, clear when ESP stops INT */
#define DVMA_ERRPEND	0x2	/* (r) error pending on memory exception */
#define DVMA_PACKCNT	0xc	/* (r) number of bytes in reg_pack */
#define DVMA_INTEN	0x10	/* (r/w) interrupt enable */
#define DVMA_FLUSH	0x20	/* (w) =1= clears PACKCNT and ERRPEND */
#define DVMA_DRAIN	0x40	/* (r/w) =1= pushes PACKCNT bytes to memory */
#define DVMA_RESET	0x80	/* (r/w) =1= reset ESP */
#define DVMA_WRITE	0x100	/* (r/w) DVMA direction, 1 to SCSI, 0 from SCSI */
#define DVMA_ENDVMA	0x200	/* (r/w) =1= responds to ESP's dma requests */
#define DVMA_REQPEND	0x400	/* (r) =1= active, NO reset and flush allowed */
#define DVMA_BYTEADR	0x1800	/* (r) next byte to be accessed by ESP */
#define DVMA_DIAG	0x3fff8000	/* (r/w) resevered for diagnostics */
#define DVMA_BUSID	0xf0000000	/* (r) device ID (bit 31-28) */
					/* STINGRAY= 0, CAMPUS= 1, HYDRA= 2 */
#define DVMA_CHK_MASK	(DVMA_ERRPEND | DVMA_REQPEND)
#define DVMA_INT_MASK	(DVMA_INTPEND | DVMA_ERRPEND)

/*
 * Misc defines 
 */
/* directions for dma transfers */
#define SM_RECV_DATA		0
#define SM_SEND_DATA		1
#define SM_NO_DATA		2

struct scsi_sm_reg {
	union {
		struct esp_read_reg	read;	/* scsi bus ctlr, read reg */
		struct esp_write_reg	write;	/* scsi bus ctlr, write reg */
	} esp;
};

/* Shorthand, to make the code look a bit cleaner. */
#define esp_rreg	esp.read	/* ESP read regs. */
#define ESP_RD		smr->esp.read	/* smr points to SCSM ha regs */

#define esp_wreg	esp.write	/* ESP write regs. */
#define ESP_WR		smr->esp.write	/* smr points to SCSM ha regs */

#define	SET_ESP_COUNT(smr, val)	ESP_WR.xcnt_lo = val & 0xff;\
				ESP_WR.xcnt_hi = ((val >> 8) & 0xff)
#define	GET_ESP_COUNT(smr) 	((int)ESP_RD.xcnt_hi << 8) & 0xff | \
					 ((int)ESP_RD.xcnt_lo & 0xff)
struct sm_snap {
	u_char		cur_state;	/* ESP software state */
	u_char		cur_err;	/* error status */
	u_char		cur_retry;	/* retry counter */
	u_char		cur_target;	/* current target */
	u_char		esp_step;	/* ESP step sequence reg */
	u_char		esp_stat;	/* ESP status reg */
	u_char		esp_intr;	/* ESP interrupt reg */
	u_char		sub_state;	/* ESP internal sub_state */
	u_char		scsi_status;	/* target status */
	u_char		scsi_message;	/* target message */
	u_char		num_dis_target;	/* number of discon-targets */
	u_char		sync_known;	/* synchronous check */
};

/*
 * sm_snap sub_state bit-encoded definitions.
 * accumlated state=path (target_accstate)
 */
#define TAR_START       0x1
#define TAR_SELECT      0x2
#define TAR_DATA        0x4
#define TAR_STATUS      0x8
#define TAR_MSGIN       0x10
#define TAR_DISCON      0x20
#define TAR_RESELECT    0x40
#define TAR_JOBDONE     0x80

#define TAR_SAVE_MSG    0x100
#define TAR_EXT_MSG     0x200
#define TAR_COMP_MSG    0x400
#define TAR_DIS_MSG     0x800

#define TAR_COMMAND     0x1000
#define TAR_MSGOUT      0x2000
#define TAR_CHK_COND    0x4000
#define TAR_BUSY        0x8000


/*
 * sm_snap cur_state definitions.
 */
#define STATE_FREE		0
#define STATE_RESET		1
#define STATE_DVMA_STUCK	2  /* DVMA req_pend stuck, send reset manually */
#define STATE_SPEC_RESTORE	3
#define STATE_SPEC_SAVEPTR	4

#define STATE_SEL_REQ		0x10
#define STATE_COMP_REQ		0x11
#define STATE_DATA_REQ		0x12
#define STATE_MSGOUT_REQ	0x13
#define STATE_STAT_REQ		0x14
#define STATE_ATN_REQ		0x15
#define STATE_DSRSEL_REQ	0x16
#define STATE_ENRSEL_REQ	0x17
#define STATE_CMD_DONE		0x18
#define STATE_MSGIN_REQ		0x19
#define STATE_DATA_DONE		0x1a
#define STATE_DSRSEL_DONE	0x1b
#define STATE_SYNCHK_REQ	0x1c
#define STATE_SYNC_DONE		0x1d
#define STATE_RESEL_ACPT	0x1e
#define STATE_RESEL_DONE	0x1f

#define STATE_MSG_CMPT		0x20
#define STATE_MSG_EXTEND	0x21
#define STATE_MSG_SAVE_PTR	0x22
#define STATE_MSG_RESTORE_PTR	0x23
#define STATE_MSG_DISCON	0x24
#define STATE_MSG_ERROR		0x25
#define STATE_MSG_ABORT		0x26
#define STATE_MSG_REJECT	0x27
#define STATE_MSG_NOP		0x28
#define STATE_MSG_PARITY	0x29
#define STATE_MSG_IDENT		0x2a
#define STATE_MSG_RESET		0x2b
#define STATE_MSG_SYNC		0x2c
#define STATE_MSG_LINKCMD	0x2d
#define STATE_MSG_UNKNOWN	0x2e
#define STATE_MSGOUT_DONE	0x2f
#define STATE_DATA_IN		0x30
#define STATE_DATA_OUT		0x31
#define STATE_RESELECT		0x32
#define STATE_BAD_RESELECT	0x33
#define STATE_ARBITRATE		0x34
#define STATE_COMMAND		0x35
#define STATE_STATUS		0x36
#define STATE_LAST		STATE_STATUS	/* Must be last */

#define STATE_STRING_DATA {\
	"Free",			/* 0x00 */ \
	"Reset",		/* 0x01 */ \
	"DVMA stuck",		/* 0x02 */ \
	"Spec restore",		/* 0x03 */ \
	"Spec save ptr",	/* 0x04 */ \
	"",			/* 0x05 */ \
	"",			/* 0x06 */ \
	"",			/* 0x07 */ \
	"",			/* 0x08 */ \
	"",			/* 0x09 */ \
	"",			/* 0x0a */ \
	"",			/* 0x0b */ \
	"",			/* 0x0c */ \
	"",			/* 0x0d */ \
	"",			/* 0x0e */ \
	"",			/* 0x0f */ \
	"Sel req",		/* 0x10 */ \
	"Comp req",		/* 0x11 */ \
	"Data req",		/* 0x12 */ \
	"Msgout req",		/* 0x13 */ \
	"Stat req",		/* 0x14 */ \
	"Atn req",		/* 0x15 */ \
	"Dsrsel req",		/* 0x16 */ \
	"Enrsel req",		/* 0x17 */ \
	"Cmd done",		/* 0x18 */ \
	"Msgin req",		/* 0x19 */ \
	"Data done",		/* 0x1a */ \
	"Dsrel done",		/* 0x1b */ \
	"Synch check",		/* 0x1c */ \
	"Synch done",		/* 0x1d */ \
	"Resel acpt",		/* 0x1e */ \
	"Resel done",		/* 0x1f */ \
	"Cmd complete MSG",	/* 0x20 */ \
	"Extended MSG",		/* 0x21 */ \
	"Save ptr MSG",		/* 0x22 */ \
	"Restore ptr MSG",	/* 0x23 */ \
	"Disconnect MSG",	/* 0x24 */ \
	"Error MSG",		/* 0x25 */ \
	"Abort MSG",		/* 0x26 */ \
	"Reject MSG",		/* 0x27 */ \
	"Noop MSG",		/* 0x28 */ \
	"Parity MSG",		/* 0x29 */ \
	"Identify MSG",		/* 0x2a */ \
	"Reset MSG",		/* 0x2b */ \
	"Synchronous MSG",	/* 0x2c */ \
	"",			/* 0x2d */ \
	"Unknown MSG",		/* 0x2e */ \
	"Msgout done",		/* 0x2f */ \
	"DATA IN",		/* 0x30 */ \
	"DATA OUT",		/* 0x31 */ \
	"Reconnect",		/* 0x32 */ \
	"Bad Reconnect",	/* 0x33 */ \
	"Arbitrate",		/* 0x34 */ \
	"COMMAND",		/* 0x35 */ \
	"STATUS",		/* 0x36 */ \
};

/*
 * sm_snap cur_err definitions.
 */
/* driver interface errors: (for reference) */
/*#define SE_NO_ERROR	0x00	/* no error */
/*#define SE_RETRYABLE	0x01	/* retryable */
/*#define SE_FATAL	0x02	/* fatal, no retries */

/* fatal errors: (reset internal ESP and DVMA only) */
#define	SE_SELTOUT	0x03	/* selection timeout */

/* retryable errors: */
#define	SE_RSELTOUT	0x04	/* reselection timeout */
#define	SE_RESET	0x05	/* reset */
#define	SE_TOUTERR	0x06	/* time out error */
#define	SE_LOST_BUSY	0x07	/* lost busy */
#define	SE_ILLCMD	0x08	/* illegal command detected */
#define	SE_PARITY	0x09	/* Parity error */
#define	SE_FIFOVER	0x0a	/* ESP fifo overflow */
#define	SE_CMDOVER	0x0b	/* ESP command fifo overflow */ 

/* retryable errors: (reset ESP, DVMA, and SCSI bus) */
#define SE_MEMERR	0x0c	/* memory exception error during DMA */
#define	SE_DIRERR	0x0d	/* WRONG DMA direction */
#define	SE_DVMAERR	0x0e	/* drain bit stuck */
#define	SE_RECONNECT	0x0f	/* failed to reconnect job */	
#define	SE_MSGERR	0x10	/* ESP has un-expected message */
#define	SE_PHASERR	0x11	/* ESP has un-expected phase */

/* fatal errors: (panic system) */
#define	SE_REQPEND	0x12	/* DMA stuck busy even after a long wait */

/* NON-error: (ignore and clear cur_err) */
#define	SE_SPURINT	0x13	/* spurious int */
#define	SE_LAST_ERROR	SE_SPURINT	/* Last error */

/*all other others: (ignore and leave alone cur_err) */

#define ERR_STRING_DATA {\
	"No",				/* 0x00 */ \
	"Retryable",			/* 0x01 */ \
	"Fatal",			/* 0x02 */ \
	"Selection timeout",		/* 0x03 */ \
	"Reselection timeout",		/* 0x04 */ \
	"Reset",			/* 0x05 */ \
	"Timeout",			/* 0x06 */ \
	"Lost Busy",			/* 0x07 */ \
	"Illegal cmd",			/* 0x08 */ \
	"Parity",			/* 0x09 */ \
	"Fifo overflow",		/* 0x0a */ \
	"Cmd fifo overflow",		/* 0x0b */ \
	"Memory",			/* 0x0c */ \
	"DMA direction",		/* 0x0d */ \
	"DMA drain bit",		/* 0x0e */ \
	"Reconnect",			/* 0x0f */ \
	"Unexpected msg",		/* 0x10 */ \
	"Unexpected phase",		/* 0x11 */ \
	"DMA stuck",			/* 0x12 */ \
	"Spurious int",			/* 0x13 */ \
};


#ifdef SUN4_330                 
/* stingray is 24 MHz clock (41.667 ns clock period) */
#define CLK_FREQ    	24
#define DEF_CLK_CONV    5	/* 200/41.667ns (input clock rate) */
#define DEF_TIMEOUT     0x93    /* 250ms*24*1000hz/8192*4 */
#define MIN_SYNC_PERIOD 0x35    /* 53(d) = 208.333/4 ns = 5*41.667/4 */
#define MAX_SYNC_PERIOD 0x16d   /* 364.6(d)= 1458.333/4 ns = 35*41.667/4 */
#define DEF_SYNC_PERIOD 0x35    
#else				
/* hydra and Campus is 20Mhz clock (50ns clock period) */
#define CLK_FREQ    	20
#define DEF_CLK_CONV    4	/* 200/50ns (input clock rate) */
#define DEF_TIMEOUT     0x98    /* 250ms*20*1000hz/8192*4 */
#define MIN_SYNC_PERIOD 0x3f    /* 63(d)= 250/4 = 5*50ns/4 */
#define MAX_SYNC_PERIOD 0x1b6   /* 438(d)= 1750/4 = 35*50ns/4 */
#define DEF_SYNC_PERIOD 0x3f   
#endif SUN4_330

#define CLOCK_PERIOD	(1000/CLK_FREQ)		/*41.667 nsec for 4/330 */
#define MIN_SYNC_CYL	5
#define MAX_SYNC_CYL	35
#define DEF_SYNC_OFFSET	15	/* Max # of allowable outstanding REQ */

#define KEEP_LBYTE	0x000000ff
#define KEEP_LWORD	0x0000ffff
#define MASK_LBYTE	0x0000ff00
#define MASK_LWORD	0xffff0000
#define KEEP_24BITS	0x00ffffff

#define NO_DISCON_MSG	0x0080
#define DISCON_MSG	0x00c0
#define KEEP_3BITS	0x7	/* sync_offset, bus_id */
#define MAXID(x)        (x & KEEP_3BITS)
#define KEEP_4BITS	0xf	/* sync_period */
#define KEEP_5BITS	0x1f	/* fifo_flag */
#define ESP_TPATTERN	0x55aa	/* data test pattern */
#define DVMA_TPATTERN	0x3355aacc	/* data test pattern */
#define LOOP_1USEC	10	/* 1msec / 10 inst per 5 cyl (1/25Mhz) */
#define LOOP_1MSEC	250	/* 1msec / 10 inst per 5 cyl (1/25Mhz) */
#define LOOP_2MSEC	500	/* 2msec / 20 inst per 5 cyl (1/25Mhz) */
#define LOOP_4MSEC	1000	/* 2msec / 20 inst per 5 cyl (1/25Mhz) */
#define LOOP_8MSEC	4000	/* 2msec / 20 inst per 5 cyl (1/25Mhz) */
#define LOOP_100MSEC	25000	/* 100msec / 20 inst per 5 cyl (1/25Mhz) */
#define LOOP_1SEC	250000	/* 1 sec */
#define LOOP_2SEC	500000
#define LOOP_4SEC	1000000	
#define LOOP_8SEC	2000000
#define LOOP_10SEC	2500000
#define LOOP_CMDTOUT	LOOP_10SEC  /* for worst-case Quantum 105's recal */
#define MAX_FIFO_FLAG	15
#define SC_EXT_LENGTH	3
#define SC_EXT_TOTALEN	5
#define ZERO		0
#define ONE		1
#define TWO		2
#define THREE		3
#define FOUR		4
#define SC_ACTIVE	0x1
#define SC_RECONNECT	0x2
#define SCSI_BUSY_MASK	0x1e	/* scsi bus bit 4=res conf, 3= busy, 1= chk */
#define MAX_RETRY	30

/* reset condition */
#define RESET_ALL_SCSI	0x80
#define RESET_INT_ONLY	0x81
#define RESET_EXT_ONLY	0x82

/* some defines that should be in scsi.h, but rather is in here for simpilitcy  */
#define SC_EXTENDED_MESSAGE   	SC_SYNCHRONOUS
#define SC_LINK_CMD_CPLT       	0x0a
#define SC_FLAG_LINK_CMD_CPLT   0x0b
#define SCSI_DEVID_MASK       	0x0300          /* DMA gate array type */
#define SCSI_ESP   		0x0400          /* NCR 53C90 or Emulex 100 */
#define SCSI_ESP2	   	0x0800          /* NCR 53C90A or Emulex 100A */
#define SCSI_POLLING  		0x1000          
#define SC_SYNC_CHKING        	0x2000          

/* add here for future deletes */
#define SMNUM(sm)	(sm - smctlrs)

#define INTERREG	0x0ffe6000	/* virtual addr for OBIO_int reg */
#define ENABLE_P4INT	0x10		/* bit 4 */
/* for debug trace */
#define MARK_PROBE	0x11
#define MARK_SLAVE	0x22
#define MARK_ATTACH	0x33

#define MARK_USTART	0x11
#define MARK_START	0x22
#define MARK_GO		0x33
#define MARK_CMD	0x44
#define MARK_SETUP	0x55
#define MARK_CLEANUP	0x66
#define MARK_INTR	0x77
#define MARK_DONE	0x88
#define MARK_IDLE	0x99
#define MARK_DISCON	0xaa
#define MARK_RECON	0xbb
#define MARK_OFF	0xcc
#define MARK_DEQUEUE	0xdd
#define MARK_RESET	0xee

#endif (defined sun3x) || (defined sun4)
/* end of smreg.h */
