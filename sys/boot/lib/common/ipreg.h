/*      @(#)ipreg.h 1.1 92/07/30 SMI; from UCB X.X XX/XX/XX       */
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * I/O space registers for Interphase disk controllers
 * (Note byte swapping)
 */
struct ipdevice {
	u_char	ip_r1, ip_r0;
	u_char	ip_r3, ip_r2;
};

/* bits written to R0 */
#define IP_GO		0x01
#define IP_CLRINT	0x02
/* bits read from R0 */
#define IP_BUSY		0x01
#define IP_COMPLETE	0x02

/*
 * Format of IOPB in Multibus memory
 * Brain damage due to byte-swapping
 * iopb0 for 2180, 2880
 * iopb1 for 2181
 */
struct iopb0 {
	u_char	i0_status;	/* 01 - status of command */
	u_char	i0_cmd;		/* 00 - command code */
	u_char	i0_unit_cylhi;	/* 03 - unit(4 bits) & hi-order cyl(4 bits) */
	u_char	i0_error;	/* 02 - error code */
	u_char	i0_sector;	/* 05 - sector number */
	u_char	i0_cylinder;	/* 04 - low-order cylinder */
	u_char	i0_buf_xmb;	/* 07 - high-order buffer address */
	u_char	i0_secnt;	/* 06 - sector count */
	u_char	i0_buf_lsb;	/* 09 - low-order buffer address */
	u_char	i0_buf_msb;	/* 08 - mid-order buffer address */
	u_char	i0_ioaddr;	/* 0b - controller I/O address */
	u_char	i0_head;	/* 0a - head number */
	u_char	i0_nxt_xmb;	/* 0d - high-order next IOPB addr */
	u_char	i0_burstlen;	/* 0c - DMA burst length */
	u_char	i0_nxt_lsb;	/* 0f - low-order next IOPB addr */
	u_char	i0_nxt_msb;	/* 0e - mid-order next IOPB addr */
	u_char	i0_seg_lsb;	/* 11 - 8086 segment low-order (0 for us) */
	u_char	i0_seg_msb;	/* 10 - 8086 segment high-order (0 for us) */
};

struct iopb1 {
	u_char	i1_cmdopt;	/* 01 - command options */
	u_char	i1_cmd;		/* 00 - command code */
	u_char	i1_error;	/* 03 - error code */
	u_char	i1_status;	/* 02 - status of command */
	u_char	i1_head;	/* 05 - head number */
	u_char	i1_unit;	/* 04 - unit number */
	u_char	i1_cyl_lsb;	/* 07 - cyl low byte */
	u_char	i1_cyl_msb;	/* 06 - cyl hi byte */
	u_char	i1_sect_lsb;	/* 09 - sector low byte */
	u_char	i1_sect_msb;	/* 08 - sector hi byte */
	u_char	i1_secnt_lsb;	/* 0b - # sectors low byte */
	u_char	i1_secnt_msb;	/* 0a - # sectors hi byte */
	u_char	i1_buf_xmb;	/* 0d - high-order buffer address */
	u_char	i1_burstlen;	/* 0c - DMA burst length */
	u_char	i1_buf_lsb;	/* 0f - low-order buffer address */
	u_char	i1_buf_msb;	/* 0e - mid-order buffer address */
	u_char	i1_ioadd_lsb;	/* 11 - I/O address low byte */
	u_char	i1_ioadd_msb;	/* 10 - I/O address hi byte */
	u_char	i1_seg_lsb;	/* 13 - 8086 segment low-order (0 for us) */
	u_char	i1_seg_msb;	/* 12 - 8086 segment high-order (0 for us) */
	u_char	i1_nxt_xmb;	/* 15 - high-order next IOPB addr */
	u_char	i1_unused;	/* 14 - reserved */
	u_char	i1_nxt_lsb;	/* 17 - low-order next IOPB addr */
	u_char	i1_nxt_msb;	/* 16 - mid-order next IOPB addr */
};
#ifdef lint
#define IPIOPBSZ (sizeof (struct iopb1))
#else
#define IPIOPBSZ (MAX(sizeof (struct iopb0), sizeof (struct iopb1)))
#endif

#define	IP_BUS		0x20	/* 0 => 8 bit bus, 0x20 => 16 bit bus */
#define IP1_CMDOPT	0x11	/* 16 bit bus for 2181 */
#define IP_REL		0	/* absolute addressing */
#define IP0_BURSTLEN	16	/* # bus cycles at a time */
/*
 * The blood of countless hours of exasperation lies in the following constant.
 * Do not lightly muck with it!
 */
#define IP1_BURSTLEN	0x40	/* # bus cycles at a time */

/* Command codes for i_cmd */
#define	IP_READ		0x81
#define	IP_WRITE	0x82
#define	IP_VERIFY	0x83
#define	IP_FORMAT	0x84
#define	IP_MAP		0x85
#define	IP_SWITCH	0x86		/* read switches */
#define	IP_INIT		0x87		/* not in 2180 */
#define	IP_RESTORE	0x89
#define	IP_SEEK		0x8A
#define	IP_ZERO		0x8B
#define	IP_SPINDWN	0x8C		/* ANSI only */
#define	IP_RESET	0x8F		/* SMD only */

/* Status codes for i_status */
#define	IP_OK		0x80
#define	IP_DBUSY	0x81
#define	IP_ERROR	0x82

/*
 * Unit Initialization Block
 * Not used for SMD-2180
 */
struct uib {
	u_char	uib_nsect;	/* 1 - # sectors per track */
	u_char	uib_nhead;	/* 0 - # heads per cyl */
	u_short	uib_secsize;	/* 2,3 - sector size */
	u_char	uib_gap2;	/* 5 - size of gap 2 */
	u_char	uib_gap1;	/* 4 - size of gap 1 */
	u_char	uib_retry;	/* 7 - # of retries */
	u_char	uib_intrlv;	/* 6 - interleave factor */
	u_char	uib_reseek;	/* 9 - # of restores */
	u_char	uib_ecc;	/* 8 - ECC enable */
	u_char	uib_incrh;	/* 11 - incr by head enable */
	u_char	uib_movebad;	/* 10 - move bad data enable */
	/* Following fields for 2181 only */
	u_char	uib_istatus;	/* 13 - interrupt on status change */	
	u_char	uib_dualport;	/* 12 - dual port drive */
	u_char	uib_reserve1;	/* 15 - reserved */
	u_char	uib_skew;	/* 14 - spiral skew factor */
	u_short	uib_reserve2;	/* 16,17 - reserved */
};
