/*
 * @(#)smreg.h 1.1 92/07/30 Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 *	Emulex ESP SCSI (low-level) driver header file
 *
 *	11/88	KSAM	Add in supports for Billie installation
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
	u_char	xcnt_lo;	/* transfer counter (low byte)*/
	u_char	pad1;
	u_char	pad2;
	u_char	pad3;
	u_char	xcnt_hi;	/* transfer counter (high byte)*/
	u_char	pad5;
	u_char	pad6;
	u_char	pad7;
	u_char	fifo_data;	/* fifo data buffer */
	u_char	pad9;
	u_char	pad10;
	u_char	pad11;
	u_char	cmd;		/* command register */
	u_char	pad12;
	u_char	pad13;
	u_char	pad14;
	u_char	stat;		/* status register */
	u_char	pad16;
	u_char	pad17;
	u_char	pad18;
	u_char	intr;		/* interrupt status register */
	u_char	pad19;
	u_char	pad20;
	u_char	pad21;
	u_char	step;		/* sequence step register */
	u_char	pad22;
	u_char	pad23;
	u_char	pad24;
	u_char	fifo_flag;	/* fifo flag register */
	u_char	pad25;
	u_char	pad26;
	u_char	pad27;
	u_char	conf;		/* configuration register, */
	u_char	pad28;
	u_char	pad29;
	u_char	pad30;
	u_char	reserved1;	/* reservered */
	u_char	pad31;
	u_char	pad32;
	u_char	pad33;
	u_char	reserved2;	/* reservered */
	u_char	pad34;
	u_char	pad35;
	u_char	pad36;
	u_char	conf2;		/* ESP-II configuration register */
};

/* write of ESP's registers yields the following: */
struct esp_write_reg {	/* for P4 card, it is always 32 bit access */
	u_char	xcnt_lo;	/* transfer counter (low byte)*/
	u_char	pad1;
	u_char	pad2;
	u_char	pad3;
	u_char	xcnt_hi;	/* transfer counter (high byte)*/
	u_char	pad5;
	u_char	pad6;
	u_char	pad7;
	u_char	fifo_data;	/* fifo data buffer */
	u_char	pad9;
	u_char	pad10;
	u_char	pad11;
	u_char	cmd;		/* command register */
	u_char	pad13;
	u_char	pad14;
	u_char	pad15;
	u_char	busid;		/* bus_id register */
	u_char	pad16;
	u_char	pad17;
	u_char	pad18;
	u_char	timeout;	/* timeout limit register */
	u_char	pad19;
	u_char	pad20;
	u_char	pad21;
	u_char	sync_period;	/* sync period register */
	u_char	pad22;
	u_char	pad23;
	u_char	pad24;
	u_char	sync_offset;	/* sync offset register */
	u_char	pad25;
	u_char	pad26;
	u_char	pad27;
	u_char	conf;		/* configuration register, */
	u_char	pad28;
	u_char	pad29;
	u_char	pad30;
	u_char	clock_conv;	/* clock convertion register */
	u_char	pad31;
	u_char	pad32;
	u_char	pad33;
	u_char	test;		/* test register */
	u_char	pad34;
	u_char	pad35;
	u_char	pad36;
	u_char	conf2;		/* ESP-II configuration register */
	u_char	pad38;
	u_char	pad39;
	u_char	pad40;
};

/* DMA GATE-ARRAY registers */
struct udc_table {
	u_long	ctl_stat;	/* 32 bit for HYDRA */
	u_long	dma_addr;	/* 32 bit dor hydra */
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

/* Several common Initiator cammand */
#define	CMD_NOP		0x0	/* misc cammand */	/* NO INT */
#define	CMD_FLUSH	0x1				/* NO INT */
#define	CMD_RESET_ESP	0x2				/* NO INT */
#define	CMD_RESET_SCSI	0x3				/* NO INT */
#define	CMD_RESEL_SEQ	0x40	/* disconnect cammand */	
#define	CMD_SEL_NOATN	0x41
#define	CMD_SEL_ATN	0x42
#define	CMD_SEL_STOP 	0x43
#define	CMD_EN_RESEL	0x44				/* NO INT */
#define	CMD_DIS_RESEL	0x45
#define	CMD_TRAN_INFO	0x10	/* initiator cmd */
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
#define DVMA_ERRPEND	0x2	/* (r) error pendinf on meory exception */
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
/* arbitrate retry count */
#define SM_NUM_RETRIES		20

/* scsi timer values, units as specified to the right  */
#define SM_ARBITRATION_DELAY	3	/*  1 us */
#define SM_BUS_CLEAR_DELAY	1	/*  1 us */
#define SM_BUS_SETTLE_DELAY	1	/*  1 us */
#define SM_UDC_WAIT		1	/*  1 us */
#define SM_ARB_WAIT		5	/* 10 us  */
#define SM_RESET_DELAY		4000000	/*  1 us  ( 4  Sec.) */
#define SM_LONG_WAIT		3000000	/* 10 us  (30  Sec.) */
#define SM_WAIT_COUNT		400000	/* 10 us  ( 4  Sec.) */
#define SM_SHORT_WAIT		25000	/* 10 us  (.25 Sec.) */
#define SM_PHASE_WAIT		30	/* 10 us */

/* directions for dma transfers */
#define SM_RECV_DATA		0
#define SM_SEND_DATA		1
#define SM_NO_DATA		2
#define MAX_SCSI_TARGET		8

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
	u_char		cur_cmd;	/* previous cmd out */
	u_char		esp_step;	/* ESP step sequence reg */
	u_char		esp_stat;	/* ESP status reg */
	u_char		esp_intr;	/* ESP interrupt reg */
	u_char		chk_ptr;	/* debug check point */
	u_char		scsi_status;	/* target status */
	u_char		scsi_message;	/* target message */
	u_char		num_dis_target;	/* number of discon-targets */
	u_char		sync_chk;	/* synchronous check */
	u_char		sync_offset[MAX_SCSI_TARGET];
	u_long		sync_period[MAX_SCSI_TARGET];	
};

#define STATE_FREE		0
#define STATE_RESET		1
#define STATE_DVMA_STUCK	2  /* DVMA req_pend stuck, send reset manually */
					
#define STATE_SEL_REQ		0x10
#define STATE_COMP_REQ		0x11
#define STATE_DATA_REQ		0x12
#define STATE_MSGOUT_REQ	0x13
#define STATE_STAT_REQ		0x14
#define STATE_ATN_REQ		0x15
#define STATE_DSRSEL_REQ	0x16
#define STATE_ENRSEL_REQ	0x17

#define STATE_SEL_DONE		0x18
#define STATE_DATA_DONE		0x1a
#define STATE_DSRSEL_DONE	0x1d
#define STATE_RESEL_ACPT	0x1e
#define STATE_RESEL_DONE	0x1f

#define STATE_MSGIN_REQ		0x1b
#define STATE_MSG_CMPT		0x20
#define STATE_MSG_EXTEND	0x21
#define STATE_MSG_SAVEPTR	0x22
#define STATE_MSG_RESTORE	0x23
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
#define STATE_MSG_DONE		0x2f

/* sm.c (esp driver's error defines */
#define	SE_FIFOVER	0x1	/* ESP status error, fifo overflown */
#define	SE_CMDOVER	0x2	/* ESP status error, command overflown */ 
#define	SE_PARITY	0x3	/* Parity error */
#define	SE_MSGERR	0x4	/* ESP has un-expected message */
#define	SE_PHASERR	0x5	/* ESP has un-expected phase */
#define	SE_RESET	0x6	/* reset */
#define SE_MEMERR	0x7	/* memory exception error during DMA */
#define	SE_REQPEND	0x8	/* DMA stuck busy even after a long wait */
#define	SE_DIRERR	0x9	/* WRONG DMA direction, could trash mem-data */
#define	SE_SELTOUT	0x10	/* selection timeout */
#define	SE_RECONNECT	0x11	/* failed to reconnect job */	
#define	SE_ILLCMD	0x12	/* illegal command detected */
#define	SE_DVMAERR	0x13	/* drain bit stuck */
#define	SE_BUSTATERR	0x14	/* bus status err, i.e, busy or chk_con */
#define	SE_SPURINT	0x15	/* smurous int */
#define	SE_TOUTERR	0x16	/* DMA timed out error */
#define	SE_STEPERR	0x17	/* ESP command sequence step error */
#define	SE_EXHAUST	0x18	/* retry exhausted */

#ifdef SUN4_330                 /* stingray is 24 MHz clock */
#define DEF_CLK_CONV    5
#define DEF_SYNC_CONV   10      /* 40 nsec/4 */
#define DEF_TIMEOUT     0x93
#define DEF_SYNC_PERIOD 0x34    /* 52= 5 (min cycle) * 1/24Mhz (cycle)/4nsec */
#define MAX_SYNC_PERIOD 0x16c   /* 364= 35 (min cycle)*1/24Mhz (cycle)/4nsec */
#else				/* hydra and Campus is 20Mhz clock */
#define DEF_CLK_CONV    4
#define DEF_SYNC_CONV   10      /* 40 nsec/4 */
#define DEF_TIMEOUT     0x98    /* 250ms*20*1000hz/8192*4 */
#define DEF_SYNC_PERIOD 0x3f    /* 64= 5 (min cycle)* 1/20Mhz (cycle)/ 4nsec */
#define MAX_SYNC_PERIOD 0x1b6   /* 438= 35 (min cycle)*1/24Mhz (cycle)/4nsec */
#endif SUN4_330

#define DEF_SYNC_OFFSET	15	/* Max # of allowable outstanding REQ */
#define SYNC_MODIFIER	4	/* 4 nanosecond */

#define KEEP_LBYTE	0x000000ff
#define KEEP_LWORD	0x0000ffff
#define MASK_LBYTE	0x0000ff00
#define MASK_LWORD	0xffff0000

#define NO_DISCON_MSG	0x0080
#define DISCON_MSG	0x00c0
#define KEEP_3BITS	0x7	/* sync_offset, bus_id */
#define KEEP_4BITS	0xf	/* sync_period */
#define KEEP_5BITS	0x1f	/* fifo_flag */
#define ESP_TPATTERN	0x55aa	/* data test pattern */
#define DVMA_TPATTERN	0x3355aacc	/* data test pattern */
#define LOOP_1USEC	10	/* 1msec / 10 inst per 5 cyl (1/25Mhz) */
#define LOOP_1MSEC	250	/* 1msec / 10 inst per 5 cyl (1/25Mhz) */
#define LOOP_2MSEC	500	/* 2msec / 20 inst per 5 cyl (1/25Mhz) */
#define LOOP_100MSEC	25000	/* 100msec / 20 inst per 5 cyl (1/25Mhz) */
#define LOOP_1SEC	250000	/* 1sec */
#define LOOP_2SEC	500000	/* 2sec */
#define LOOP_4SEC	1000000	/* 4sec */
#define LOOP_6SEC	2000000	/* 6sec */
#define LOOP_8SEC	4000000	/* 8sec */
#define LOOP_CMDTOUT	0x10000000	/* 1 minutes */
#define MAX_FIFO_FLAG	15
#define SC_EXT_LENGTH	3
#define SC_EXT_TOTALEN	5
#define ZERO		0
#define SC_ACTIVE	0x1
#define SC_RECONNECT	0x2
#define SCSI_BUSY_MASK	0x1e	/* scsi bus bit 4=res conf, 3= busy, 1= chk */
#define STEP_SEL_OK	0x4	/* four sub steps */
#define MAX_RETRY	10

/* reset condition */
#define RESET_ALL_SCSI	0
#define RESET_INT_ONLY	1
#define RESET_EXT_ONLY	2

/* some defines that should be in scsi.h, but rather is in here for simpilitcy  */
#define SC_EXTENDED_MESSAGE   	SC_SYNCHRONOUS
#define SC_LINK_CMD_CPLT       	0x0a
#define SC_FLAG_LINK_CMD_CPLT   0x0b
#define SCSI_DEVID_MASK       	0x0300          /* DMA gate array type */
#define SCSI_ESP    		0x0400          /* ESP or ESP 2 */
#define SCSI_ESP2     		0x0800          /* ESP or ESP 2 */
#define SCSI_POLLING  		0x1000          
#define SC_SYNC_CHKING        	0x2000          

#ifdef notdef
/* scsi.h has all these defines, but it is duplicated here just for reference only */
#define SC_INQUIRY		0x12		/* CCS only */
#define SC_MODE_SELECT		0x15
#define SC_MODE_SENSE		0x1a
#define SC_READ_DEFECT_LIST	0x37		/* CCS only, group 1 */
#define SC_WRITE_VERIFY		0x2e            /* 10 byte write+verify */
#define SC_WRITE_FILE_MARK	0x10

#define SC_EXTENDED_MESSAGE	0x01
#define SC_ABORT		0x06
#define SC_MSG_REJECT		0x07
#define SC_NO_OP		0x08
#define SC_PARITY		0x09
#define SC_LINK_CMD_CPLT	0x0a
#define	SC_FLAG_LINK_CMD_CPLT	0x0b
#define SC_DEVICE_RESET		0x0c
#endif notdef

/* add here for future deletes */
#define SMNUM(sm)	(sm - smctlrs)
#define SMUNIT(dev)     (minor(dev) & 0x03)

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
