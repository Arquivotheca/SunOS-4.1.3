/*	@(#)xtreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Xylogics 472 multibus tape controller
 * IOPB definition.
 */

#ifndef _sundev_xtreg_h
#define _sundev_xtreg_h

struct xtiopb {
	/* Byte 1 */
	u_char	xt_swab    : 1;	/* do byte swapping */
	u_char	xt_retry   : 1;	/* enable retries */
	u_char	xt_subfunc : 6;	/* sub-function code */
	/* Byte 0 */
	u_char	xt_autoup  : 1;	/* auto update of IOPB */
	u_char	xt_reloc   : 1;	/* use relocation */
	u_char	xt_chain   : 1;	/* command chaining */
	u_char	xt_ie      : 1;	/* interrupt enable */
	u_char	xt_cmd     : 4;	/* command */
	/* Byte 3 */
	u_char	xt_errno;	/* error number */
	/* Byte 2 */	
	u_char	xt_iserr   : 1;	/* error indicator */
	u_char		   : 2;
	u_char	xt_ctype   : 3;	/* controller type */
	u_char		   : 1;
	u_char	xt_complete: 1;	/* completion code valid  */
	u_short	xt_status;	/* 5,4: various status bits */
	/* Byte 7 */
	u_char		   : 5;
	u_char	xt_unit    : 3;	/* unit number */
	/* Byte 6 */
	u_char	xt_bytebus : 1;	/* use byte transfers */
	u_char		   : 4;
	u_char	xt_throttle: 3;	/* throttle control */
	u_short	xt_cnt;		/* 9,8: requested count */
	u_short	xt_bufoff;	/* b,a: buffer offset */
	u_short	xt_bufrel;	/* d,c: buffer offset */
	u_short	xt_nxtoff;	/* f,e: next iopb offset */
	u_short	xt_acnt;	/* 11,10: actual count */
};

/* commands */
#define	XT_NOP		0x00	/* no operation */
#define	XT_WRITE	0x01	/* write */
#define	XT_READ		0x02	/* read */
#define	XT_SEEK		0x05	/* position */
#define	XT_DRESET	0x06	/* drive reset */
#define	XT_FMARK	0x07	/* write file mark / erase */
#define	XT_DSTAT	0x09	/* read drive status */
#define	XT_PARAM	0x0B	/* set drive parameters */
#define	XT_TEST		0x0C	/* self test */

/* Subfunction codes */
#define	XT_REVERSE	0x20	/* reverse tape motion */
#define	XT_REC		0	/* search for record marks */
#define	XT_FILE		1	/* search for file marks */
#define	XT_REW		2	/* rewind */
#define	XT_UNLOAD	3	/* unload */
#define	XT_ERASE	1	/* erase, when used with FMARK */
#define	XT_LODENS	0	/* low density */
#define	XT_HIDENS	1	/* high density */
#define	XT_LOW		2	/* low speed */
#define	XT_HIGH		3	/* high speed */

/* status codes */
#define	XTS_IEI		0x4000		/* interrupt on each iopb */
#define	XTS_EOT		0x80		/* end of tape */
#define	XTS_BOT		0x40		/* beginning of tape */
#define	XTS_FPT		0x20		/* write protected */
#define	XTS_REW		0x10		/* rewinding */
#define	XTS_ONL		0x08		/* on line */
#define	XTS_RDY		0x04		/* drive ready */
#define	XTS_DBY		0x02		/* data busy */
#define	XTS_FBY		0x01		/* formatter busy */
#define XTS_BITS "\1FBSY\2DBSY\3RDY\4ONL\5REW\6PROT\7BOT\10EOT"

/* error codes */
#define	XTE_NOERROR	0x00
#define XTE_TIMEOUT     0x04            /* Hardware or Software timeout */
#define	XTE_EOF		0x1E
#define	XTE_SOFTERR	0x1F		/* corrected data err */
#define	XTE_SHORTREC	0x22
#define	XTE_LONGREC	0x23
#define	XTE_BOT		0x30
#define	XTE_EOT		0x31
#define	XTE_DLATE	0x33		/* r: fifo overrun w: data late */	

#endif /*!_sundev_xtreg_h*/
