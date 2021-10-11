/*	@(#)xyreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sundev_xyreg_h
#define _sundev_xyreg_h

/*
 * Xylogics 450 declarations
 */

/*
 * IOPB - in real memory so we can use bit fields
 */
struct xyiopb {
	/* Byte 1 */
	u_char		   : 1;
	u_char	xy_intrall : 1;	/* interrupt on all iopbs (450) */
	u_char	xy_intrerr : 1;	/* interrupt on error */
	u_char	xy_reserve : 1;	/* reserve dual port drive (450) */
	u_char	xy_recal   : 1;	/* recalibrate on seek errors (450) */
	u_char	xy_enabext : 1;	/* enable extensions (450) */
	u_char	xy_eccmode : 2;	/* ECC actions */
	/* Byte 0 */
	u_char	xy_autoup  : 1;	/* auto update of IOPB */
	u_char	xy_reloc   : 1;	/* use relocation */
	u_char	xy_chain   : 1;	/* command chaining */
	u_char	xy_ie      : 1;	/* interrupt enable */
	u_char	xy_cmd     : 4;	/* command */
	/* Byte 3 */
	u_char	xy_errno;	/* error number */
	/* Byte 2 */	
	u_char	xy_iserr   : 1;	/* error indicator */
	u_char		   : 2;
	u_char	xy_ctype   : 3;	/* controller type */
	u_char		   : 1;
	u_char	xy_complete: 1;	/* completion code valid (450) */
	/* Byte 5 */
	u_char	xy_drive   : 2;	/* drive type */
	u_char		   : 4;
	u_char	xy_unit    : 2;	/* unit number */
	/* Byte 4 */
	u_char	xy_bytebus : 1;	/* use byte transfers */
	u_char	xy_intrlv  : 4;	/* interleave - 1 (450) */
	u_char	xy_throttle: 3;	/* throttle control */
	u_char	xy_sector;	/* 7: sector number */
	u_char	xy_head;	/* 6: head number */
	u_short	xy_cylinder;	/* 9,8: cylinder number */
	u_short	xy_nsect;	/* b,a: sector count */
#define	xy_status xy_nsect	/* low byte is status */
	u_short	xy_bufoff;	/* d,c: buffer offset */
	u_short	xy_bufrel;	/* f,e: buffer relocation */
	u_char	xy_subfunc;	/* 11: subfunction */
	u_char	xy_bhead;	/* 10: base head (450) */
	u_short	xy_nxtoff;	/* 13,12: next iopb offset */
	u_short	xy_eccpatt;	/* 15,14: ECC pattern */
	u_short	xy_eccaddr;	/* 17,16: ECC address */
};

/*
 * Commands -- the values are shifted by a byte so they can be folded
 * with the subcommands into a single variable.
 */
#define	XY_NOP		0x000	/* nop */
#define	XY_WRITE	0x100	/* write */
#define	XY_READ		0x200	/* read */
#define	XY_WRITEHDR	0x300	/* write headers */
#define	XY_READHDR	0x400	/* read headers */
#define	XY_SEEK		0x500	/* seek */
#define	XY_RESTORE	0x600	/* drive reset */
#define	XY_FORMAT	0x700	/* format */
#define	XY_READALL	0x800	/* read all */
#define	XY_STATUS	0x900	/* drive status */
#define	XY_WRITEALL	0xa00	/* write all */
#define	XY_INIT		0xb00	/* initialize drive */
#define	XY_TEST		0xc00	/* self test */
#define	XY_BUFLOAD	0xe00	/* buffer load */
#define	XY_BUFDUMP	0xf00	/* buffer dump */

/*
 * Subcommands
 */
	/*
	 * readall
	 */
#define XY_RDHDE	0x00	/* read header, data and ecc */
#define XY_DEFLST	0x01	/* read defect list */

/*
 * xy_status bits
 */
#define	XY_ONCYL	0x80		/* true if zero */
#define	XY_READY	0x40		/* true if zero */
#define	XY_WRPOT	0x20
#define	XY_RSVRD	0x10
#define	XY_SKERR	0x08
#define	XY_FAULT	0x04

/*
 * Miscellaneous defines.
 */
#define XY_THROTTLE	4		/* 32 words/transfer */
#define	NXYDRIVE	4		/* 4 possible drive types */
#define XYUNPERC	4		/* max # of units per controller */

/*
 * Structure definition and macros used for a sector header
 */
#define XY_HDRSIZE	4		/* bytes/sector header */

struct xyhdr {
	/* Byte 1 */
	u_char	xyh_sec_hi : 2;
	u_char		   : 3;
	u_char	xyh_cyl_hi : 3;
	/* Byte 0 */
	u_char	xyh_cyl_lo;
	/* Byte 3 */
	u_char	xyh_type   : 2;
	u_char	xyh_sec_lo : 6;
	/* Byte 2 */
	u_char	xyh_head;
};

#define	XY_CYL_LO(n)	((int)(n) & 0xff)
#define	XY_CYL_HI(n)	(((int)(n) & 0x700) >> 8)
#define	XY_SEC_LO(n)	((int)(n) & 0x3f)
#define	XY_SEC_HI(n)	(((int)(n) & 0xc0) >> 6)
#define	XY_GET_SEC(h)	(((h)->xyh_sec_hi << 6) | (h)->xyh_sec_lo)
#define	XY_GET_CYL(h)	(((h)->xyh_cyl_hi << 8) | (h)->xyh_cyl_lo)

#endif /*!_sundev_xyreg_h*/
