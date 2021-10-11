/*	@(#)tmreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * IOPB Definitions for Computer Products Corp. TapeMaster
 */

#ifndef _sundev_tmreg_h
#define _sundev_tmreg_h

struct	addr86 {
	u_short	a_offset;
	u_short	a_base;
};

typedef	struct addr86 ptr86_t;
typedef	unsigned bit;

/*
 * The wire-wrapped configured System Configuration Pointer:
 * This is the same for all controllers since it is used only
 * to initialize the controller.
 */
#define	TM_SCPADDR	0x1106		/* History! */

/*
 * System Configuration Pointer
 * At a jumpered address (low nibble 6)
 */
struct	tmscp {
	u_char	tmscb_busx, tmscb_bus;	/* 8/16 bit bus flag */
	ptr86_t	tmscb_ptr;		/* pointer to configuration block */
};

/* Definitions for tmscb_bus */
#define	TMSCB_BUS8	0	/* 8 bit bus */
#define	TMSCB_BUS16	1	/* 16 bit bus */

/*
 * System Configuration Block
 * Statically located between controller resets
 */
struct	tmscb {
	u_char	tmscb_03x, tmscb_03;	/* constant 0x03 */
	ptr86_t	tmccb_ptr;		/* pointer to channel control block */
};
#define	TMSCB_CONS	0x03	/* random constant for SCB */

/*
 * Channel Control Block
 * Statically located between controller resets
 */
struct	tmccb {
	u_char tmccb_gate, tmccb_ccw;	/* interrupt control */
	ptr86_t	tmtpb_ptr;		/* pointer to tape parm block */
};
/* definitions for ccb_gate */
#define	TMG_OPEN   0x00		/* open - ctlr available */ 
#define	TMG_CLOSED 0xFF		/* closed - ctlr active or alloc */
/* definitions for ccb_ccw */
#define	TMC_NORMAL 0x11		/* normal command */
#define	TMC_CLRINT 0x09		/* clear active interrupt */

/*
 * Tape Status Structure - one word
 */
struct	tmstat {
	bit	tms_entered:1;	/* tpb entered */
	bit	tms_compl : 1;	/* tpb complete */
	bit	tms_retry : 1;	/* tpb was retried */
	bit	tms_error : 5;	/* error code */
	/* byte */
	bit 	tms_eof   : 1;	/* filemark */
	bit	tms_online: 1;	/* on line */
	bit	tms_load  : 1;	/* at load point */
	bit	tms_eot   : 1;	/* end of tape */
	bit	tms_ready : 1;	/* ready */
	bit	tms_fbusy : 1;	/* fmt busy */
	bit	tms_prot  : 1;	/* wrt protected */
	bit 		  : 1;	/* unused */
	/* byte */
};
#define	TMS_BITS "\2PROT\3BUSY\4RDY\5EOT\6BOT\7ONL\10EOF"

/* Tape error codes (tms_error) */
#define	E_NOERROR 0	/* normal completion */
#define	E_EOT	  0x09	/* end of file on read */
#define	E_BADTAPE 0x0A	/* bad spot on tape */
#define	E_OVERRUN 0x0B	/* Bus over/under run */
#define	E_PARITY  0x0D	/* Read parity error */
#define	E_SHORTREC 0x0F	/* short record on read; error on write */
#define	E_EOF	  0x15	/* end of file on read */

/*
 * Tape Parameter Block
 * Dynamically located via CCB
 */
struct	tpb {
	short	tm_cmd;			/* command word(byte) */
	short	tm_cmd2;		/* zero command word */
	struct {
		bit	tmc_width : 1;	/* bus width */
		bit              : 2;	/* unused */
		bit	tmc_cont  : 1;	/* continuous movement */
		bit	tmc_speed : 1;	/* slow or stream */
		bit	tmc_rev   : 1;	/* reverse */
		bit		 : 1;	/* unused */
		bit	tmc_bank  : 1;	/* bank select */
		/* byte */
		bit	tmc_lock  : 1;	/* bus lock */
		bit	tmc_link  : 1;	/* tpb link */
		bit	tmc_intr  : 1;	/* want interrupt */
		bit	tmc_mail  : 1;	/* mailbox intr */
		bit	tmc_tape  : 2;	/* tape select */
		bit		 : 2;	/* unused */
		/* byte */
	}	tm_ctl;			/* control word */
	u_short	tm_count;		/* return count */
	u_short	tm_bsize;		/* buffer size */
	u_short	tm_rcount;		/* real size / overrun */
	ptr86_t tm_baddr;		/* buffer address */
	struct	tmstat tm_stat;		/* tape status */
	ptr86_t	tm_intrlink;		/* intr/link addr */
};

/*
 * Interesting tape commands (tm_cmd)
 */
#define	TM_CONFIG	0x00	/* Configure controller */
#define	TM_REWIND	0x04	/* Rewind (overlapped) */
#define	TM_NOP		0x20	/* NOP - for clearing intrs */
#define	TM_STATUS	0x28	/* Drive Status */
#define	TM_READ		0x2C	/* Read to MB memory */
#define	TM_WRITE	0x30	/* Write to MB memory */
#define	TM_REWINDX	0x34	/* Rewind (non-overlapped) */
#define	TM_UNLOAD	0x38	/* Unload or go offline */
#define	TM_WEOF		0x40	/* Write file mark (EOF) */ 
#define	TM_SEARCH	0x44	/* search multiple filemarks */
#define	TM_SPACE	0x48	/* move over tape record */
#define	TM_ERASE	0x4C	/* Erase fixed length */
#define	TM_SPACEF	0x70	/* move over tape record till EOF */
#define	TM_OPTION	0x78	/* TapeMaster A only: set options */

#define	TMOP_DOSWAB	0x04	/* tm_rcount to enable swab with TM_OPTION */
#define	TMOP_ISSWAB	0x200	/* swabbing is enabled */

/*
 * Tape directions (tmc_rev)
 */
#define	TM_DIRBIT	1
#define	TM_FORWARD	0
#define	TM_REVERSE	1

/* 
 * Definition for Multibus I/O space registers
 * These are byte swapped relative to documentation
 */
struct	tmdevice {
	u_char	tmdev_reset;	/* Write resets */
	u_char	tmdev_attn;	/* Write wakes up ctlr */
};

/*
 * Data which must be present for each controller
 * in Multibus memory.
 */
struct	tm_mbinfo {
	struct tmscb tmb_scb;	/* System Conf Block */
	struct tmccb tmb_ccb;	/* Channel Control Block */
	struct tpb tmb_tpb;	/* Tape Parameter Block */
};

#define	b_repcnt  b_bcount
#define	b_command b_resid

#endif /*!_sundev_tmreg_h*/
