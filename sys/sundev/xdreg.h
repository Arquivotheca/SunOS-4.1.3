/*	@(#)xdreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sundev_xdreg_h
#define	_sundev_xdreg_h

/*
 * Xylogics 7053 declarations
 */

/*
 * IOPB - in real memory so we can use bit fields
 */
struct xdiopb {
		/* BYTE 0 */
	u_char	xd_iserr	:1;	/* error indicator */
	u_char	xd_complete	:1;	/* completion code valid */
	u_char	xd_chain	:1;	/* command chaining */
	u_char	xd_sgm		:1;	/* scatter/gather mode */
	u_char	xd_cmd		:4;	/* command */
		/* BYTE 1 */
	u_char	xd_errno;		/* error number */
		/* BYTE 2 */
	u_char	xd_dstat;		/* drive status */
		/* BYTE 3 */
	u_char	xd_resvd;		/* reserved */
		/* BYTE 4 */
	u_char	xd_subfunc;		/* subfunction code */
		/* BYTE 5 */
	u_char	xd_fixd		:1;	/* fixed/removable media */
	u_char	xd_hdp		:1;	/* hold dual-port drive */
	u_char	xd_psel		:1;	/* priority select (dual-port) */
	u_char	xd_bht		:1;	/* black hole transfer */
	u_char			:1;
	u_char	xd_unit		:3;	/* unit number */
		/* BYTE 6 */
	u_char	xd_llength	:5;	/* link length (scatter/gather) */
#define	xd_drparam xd_llength		/* used for set drive params */
#define	xd_interleave xd_llength	/* used for set format params */
	u_char	xd_intpri	:3;	/* interrupt priority */
		/* BYTE 7 */
	u_char	xd_intvec;		/* interrupt vector */
	u_short	xd_nsect;		/* 8, 9: sector count */
	u_short	xd_cylinder;		/* a, b: cylinder number */
#define	xd_throttle xd_cylinder		/* used for write ctlr params */
	u_char	xd_head;		/* c: head number */
	u_char	xd_sector;		/* d: sector number */
	u_char	xd_bufmod;		/* e: buffer address modifier */
#define	xd_hsect xd_bufmod		/* used for read drive status */
#define	xd_ctype xd_bufmod		/* used for read ctlr params */
		/* BYTE F */
	u_char	xd_prio		:1;	/* high priority iopb */
	u_char			:1;
	u_char	xd_nxtmod 	:6;	/* next iopb addr modifier */
	u_int	xd_bufaddr;		/* 10-13: buffer address */
#define	xd_promrev xd_bufaddr		/* used for read ctlr params */
	struct xdiopb *xd_nxtaddr;	/* 14-17: next iopb address */
	u_short	xd_cksum;		/* 18, 19: iopb checksum */
	u_short	xd_eccpat;		/* 1a, 1b: ECC pattern */
	u_short	xd_eccaddr;		/* 1c, 1d: ECC address */
};

/*
 * Commands -- the values are shifted by a byte so they can be folded
 * with the subcommands into a single variable.
 */
#define	XD_NOP		0x000	/* nop */
#define	XD_WRITE	0x100	/* write */
#define	XD_READ		0x200	/* read */
#define	XD_SEEK		0x300	/* seek */
#define	XD_RESTORE	0x400	/* drive reset */
#define	XD_WPAR		0x500	/* write params */
#define	XD_RPAR		0x600	/* read params */
#define	XD_WEXT		0x700	/* extended write */
#define	XD_REXT		0x800	/* extended read */
#define	XD_DIAG		0x900	/* diagnostics */
#define	XD_ABORT	0xa00	/* abort */

/*
 * Subcommands
 */
	/*
	 * seek
	 */
#define	XD_RCA		0x00	/* report current address */
#define	XD_SRCA		0x01	/* seek and report current address */
#define	XD_SSRCI	0x02	/* start seek and report completion */
	/*
	 * read and write parameters
	 */
#define	XD_CTLR		0x00	/* controller parameters */
#define	XD_DRIVE	0x80	/* drive parameters */
#define	XD_FORMAT	0x81	/* format parameters */
#define	XD_DSTAT	0xa0	/* drive status (read only) */
	/*
	 * extended read and write
	 */
#define	XD_THEAD	0x80	/* track headers */
#define	XD_FORMVER	0x81	/* format (write), verify (read) */
#define	XD_HDE		0x82	/* header, data and ecc */
#define	XD_DEFECT	0xa0	/* defect map */
#define	XD_EXTDEF	0xa1	/* extended defect map */

/*
 * Compound Compounds (for clarity of code)
 */

#define	XD_VERIFY	(XD_REXT|XD_FORMVER)

/*
 * drive status bits
 */
#define	XD_READY	0x01
#define	XD_ONCYL	0x02
#define	XD_SKERR	0x04
#define	XD_DFAULT	0x08
#define	XD_WPROT	0x10
#define	XD_DPBUSY	0x80

/*
 * Miscellaneous defines.
 */
#define	XD_THROTTLE	32		/* 32 words/transfer */
#define	XDUNPERC	4		/* max # of units per controller */

/*
 * Structure definition and macros used for a sector header.
 */
#define	XD_HDRSIZE	4		/* bytes/sector header */

struct xdhdr {
	/* Byte 0 */
	u_char	xdh_cyl_lo;
	/* Byte 1 */
	u_char	xdh_cyl_hi;
	/* Byte 2 */
	u_char	xdh_head;
	/* Byte 3 */
	u_char	xdh_sec;
};

#define	XD_CYL_LO(n)	((int)(n) & 0xff)
#define	XD_CYL_HI(n)	(((int)(n) & 0xff00) >> 8)
#define	XD_GET_CYL(h)	(((h)->xdh_cyl_hi << 8) | (h)->xdh_cyl_lo)


/*
 * Bits that are to be used for setting controller parameters
 *
 * iopb bytes 0x8 and 0x9 are mushed together here as one short.
 *
 */

	/* iopb byte 0x8 */
#define	XD_AUD	0x8000	/*
			 * auto update mode. If false, update still
			 * occurs on error
			 */
#define	XD_LWRD	0x4000	/* 32bit xfer mode if on */
#define	XD_DACF	0x2000	/* disable VMEbus ACFAIL* recognition */
#define	XD_ICS	0x1000	/* IOPB checksum done by 7053 */
#define	XD_EDT	0x0800	/* Enable DMA timeout */
#define	XD_NPRM	0x0400	/* Non-privileged register access mode */
/*		0x0200	   Reserved */
/*		0x0100	   Reserved */
	/* iopb byte 0x9 */
#define	XD_TDTM	0x00c0	/* Mask for throttle dead time value - 2 bits */
#define	xd_tdtv(x)	(((x)&0x3)<<6)
/*		0x0020	   Reserved */
#define	XD_ROR	0x0010	/* enable VMEbus Release-On-Request */
/*		0x0008	   Reserved */
/*		0x0004	   Reserved */
/*		0x0002	   Reserved */
#define	XD_DRA	0x0001	/* Disable Read-Ahead on 7053 */


	/* iopb byte 0xa */
#define	XD_OVS	0x80	/* Enable overlapped seeks */
#define	XD_ESWO	0x40	/*
			 * (undocumented feature) enable elevator seeks
			 * and write optimization
			 */
#define	XD_NOQD	0x20	/* if set disables queue delays */
#define	XD_ASR	0x10	/* Automatic Seek Retry */
/*		0x08	   Reserved */
#define	XD_RBC	0x04	/* Retry Before Correction */
#define	XD_ECC2	0x02	/* ECC Mode 2 */
#define	XD_ECC1	0x01	/* ECC Mode 1 */
#define	XD_ECC0	0x00	/* ECC Mode 0 */

#endif /*!_sundev_xdreg_h*/
