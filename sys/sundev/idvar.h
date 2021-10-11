/* @(#)idvar.h	1.1 7/30/92 */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _idvar_h
#define _idvar_h

#include <sun/dkio.h>

/*
 * Define MULT_MAJOR if code for multiple major numbers should be included.
 * With an 8-bit minor number and 8 sections per disk, only 32 disks can
 * be supported per major device number.  If MULT_MAJ is defined, the
 * driver supports multiple major numbers so that more disks can be supported.
 * All this can go away if the minor number field size is increased.
 */
#ifndef MULT_MAJOR
#define MULT_MAJOR	1	/* handle multiple major devices */
#endif

#ifdef	MULT_MAJOR
#define NMAJOR		256	/* maximum number of major devices */ 
#define NDEV_PER_MAJOR	256	/* number of devices per major device */
#define NO_MAJOR	(NMAJOR-1)	/* illegal value for translation */

#define ID_CMINOR(dev)	(cmajor_lookup[major(dev)]*NDEV_PER_MAJOR + minor(dev))
#define ID_BMINOR(dev)	(bmajor_lookup[major(dev)]*NDEV_PER_MAJOR + minor(dev))
#define ID_BLKDEV(dev)	(makedev(cmajor_trans[major(dev)], minor(dev)))

#else	/* no MULT_MAJOR */

#define ID_CMINOR(dev)	minor(dev)
#define ID_BMINOR(dev)	minor(dev)
#define ID_BLKDEV(dev)  (dev)

#endif	/* MULT_MAJOR */

#define ID_NLPART	NDKMAP		/* # of logical partitions */
#define	ID_CUNIT(dev)	ID_UNIT(ID_CMINOR(dev))
#define	ID_BUNIT(dev)	ID_UNIT(ID_BMINOR(dev))
#define ID_UNIT(dev)	((unsigned)(dev)/ID_NLPART) /* dev already mapped */
#define ID_LPART(dev)	((unsigned)(dev)%ID_NLPART)
#define NOLPART		(-1)		/* used for non-partition commands */
#define ID_SECSIZE	512		/* sector size assumed by dump */
#define ID_POLL_TIMEOUT	(30000000)	/* poll timeout in microseconds */
#define ID_MAXBUFSIZE	(63 * 1024)	/* Maximum ioctl command buffer size */

/*
 * Macro to convert a unit number into IPI channel/slave/facility address.
 */
#define ID_CHAN(unit)	((unit) >> 7)
#define ID_SLAVE(unit)	((unit) >> 4 & 7)
#define ID_FAC(unit)	((unit) & 0xf)
#define ID_MAKE_UNIT(c,s,f) ((c) << 7 | (s) << 4 | (f))
#define ID_CSF(unit) \
		IPI_MAKE_ADDR(ID_CHAN(unit), ID_SLAVE(unit), ID_FAC(unit))

/*
 * Convert the unit number to the IPI address of it's controller.
 */
#define ID_CTLR_CSF(unit) \
		IPI_MAKE_ADDR(ID_CHAN(unit), ID_SLAVE(unit), IPI_NO_ADDR)

/*
 * Unit number printable in hex.  Looks like device name format.
 */
#define ID_CSF_PRINT(unit) \
		(ID_CHAN(unit) << 8 | ID_SLAVE(unit) << 4 | ID_FAC(unit))


/*
 * Although IPI-2 only allows 8 drives per channel, some controllers may have
 * multiple IPI-2 ports.
 */
#define IDC_NUNIT	16		/* max # of units per controller */

/*
 * Recovery control structure.
 */

struct id_rec_state {
	char		rec_state;	/* recovery states */
	char		rec_substate;	/* recovery substates */
	short		rec_data;	/* data for current recovery state */
	long		rec_history;	/* recovery history: a bit per state */
	long		rec_flags;	/* flags for recovery requests */
};

/*
 * Recovery states.
 * 	These definitions must be consistent with the id_next_state table
 *	initialization in id.c.
 * 	The corresponding bit in c_rec_history is set after each state.
 *	The maximum is 32 states because of c_rec_history size.
 */
#define IDR_NORMAL	0	/* not in recovery mode */
#define IDR_TEST_BOARD	1	/* testing channel board */
#define IDR_RST_BOARD	2	/* resetting channel board */
#define IDR_TEST_CHAN	3	/* testing channel */
#define IDR_RST_CHAN	4	/* resetting channel */
#define IDR_TEST_CTLR	5	/* testing controller */
#define IDR_RST_CTLR	6	/* resetting controler */
#define IDR_GET_XFR	7	/* get transfer settings */
#define IDR_SET_XFR	8	/* set transfer settings */
#define IDR_STAT_CTLR	9	/* report status of controller */
#define IDR_ID_CTLR	10	/* get vendor ID of controller */
#define IDR_ATTR_CTLR	11	/* get attributes of controller */
#define IDR_SET_XFR1	12	/* set transfer settings */
#define IDR_ABORT	13	/* abort commands */
#define IDR_RETRY	14	/* re-issuing commands */
#define IDR_REC_FAIL	15	/* recovery failed */

#define	IDR_RST_CTLR1	16	/* reset controller */
#define IDR_GET_XFR1	17	/* get transfer settings */
#define IDR_SET_XFR2	18	/* set transfer settings */
#define IDR_STAT_CTLR1	19	/* report addressee status */
#define IDR_ID_CTLR1	20	/* get vendor ID */
#define IDR_ATTR_CTLR1	21	/* get attributes */
#define IDR_SET_XFR3	22	/* set transfer settings */

#define IDR_STAT_UNIT	23	/* stat device */
#define IDR_ATTR_UNIT	24	/* get device attributes */
#define IDR_READ_LABEL	25	/* read label */
#define IDR_UNIT_NORMAL	26	/* unit recovery successfully complete */
#define IDR_UNIT_FAILED	27	/* unit recovery failed */

#define IDR_SATTR_CTLR1	28	/* set controller attributes - recovery */
#define IDR_SATTR_CTLR2	29	/* set controller attributes - initialization */
#define IDR_IDLE_CTLR	30	/* wait for controller to go idle */
#define IDR_DUMP_CTLR	31	/* dump controller firmware to disk */

/* 
 * Controller structure
 */
struct id_ctlr {
	struct	id_unit	*c_unit[IDC_NUNIT];	/* unit pointers */
/* 40 */
	u_int		c_flags;	/* state information */
	int		c_ipi_addr;	/* channel/slave/facility address */
	short		c_cmd_q;	/* number of outstanding commands */
	short		c_max_q;	/* limit on outstanding commands */
	int		c_cmd_q_len;	/* length of outstanding commands */
/* 50 */
	int		c_max_q_len;	/* command buffer size (bytes) */
	short		c_rw_cmd_len;	/* length of single read/write cmd */
	u_short		c_ctlr;		/* controller number (as in idc%d) */
	u_short		c_max_cmdlen;	/* maximum command packet length */
	u_short		c_max_resplen;	/* maximum response packet length */
	u_char		c_idle_time;	/* seconds since a command completion */
	u_char		c_max_parms;	/* maximum args for request parameter */
	u_char		c_ctype;	/* Controller type (see dkio.h) */
	u_char		c_intpri;	/* interrupt priority */
/* 60 */
	char		c_xfer_mode[2];	/* buffer for get transfer mode */
	u_short		c_fac_flags;	/* bit per facility attached */
	struct ipiq	*c_retry_q;	/* requests waiting retry */
	struct id_rec_state c_recover;	/* recovery state */
/* 74 */
	struct reconf_bs_parm c_reconf_bs_parm;	/* slave reconfiguration parm */
};

/*
 * Flags for controller. 
 */
#define ID_C_PRESENT	1	/* initialized */
#define ID_NO_BLK_SIZE	2	/* doesn't support blk size parm for format */
#define ID_SEEK_ALG	4	/* controller does head scheduling */
#define ID_NO_RECONF	8	/* slave reconfiguration attr not supported */
#define	ID_CTLR_VERIFY	0x10	/* controller can do write verification */
#define	ID_CTLR_WAIT	0x20	/* a unit is waiting for controller queue lim */
	/*	0xffffff00	bits 31-8 reserved. flags from ipi_error.h */

/*
 * Unit structure.
 */
struct id_unit {
	struct id_ctlr	*un_ctlr;	/* controller */
	int		un_ipi_addr;	/* IPI channel/slave/facility address */
	u_int		un_flags;	/* state information */
	short		un_cmd_q;	/* number of outstanding commands */
	u_short		un_unit;	/* unit number (as in id%x) */

	u_int		un_phys_bsize;	/* physical block size */
	u_int		un_log_bsize;	/* logical block size */
	u_int		un_log_bshift;	/* shift amount for block size divide */
	u_int		un_first_block;	/* starting block number */

	struct ipiq	*un_wait_q;	/* partly setup request */
	short		un_dk;		/* index for iostats (-1 if no stats) */
	u_char		un_idle_time;	/* seconds since a command completion */
	u_char		un_wchkmap;	/* write check flags per partition */
	struct dk_label	*un_lp;	/* label (allocated only during read) */
	struct id_rec_state un_recover;	/* recovery state */
	struct diskhd	un_bufs;	/* queued buffers */
	struct dk_diag	un_diag;	/* diagnostic information */
	struct dk_geom	un_g;		/* disk geometry info */
	/*
	 * Logical partition information.
	 */
	struct un_lpart {
		struct dk_map un_map;	/* first cylinder and block count */
		u_int	un_blkno;	/* first block for partition */
	} un_lpart[NDKMAP];
};

/*
 * Flags for unit.
 */
#define ID_UN_PRESENT	1	/* device map and geometry initialized */
#define	ID_ATTACHED	2	/* attached */
#define ID_LABEL_VALID	4	/* label read and valid */
#define ID_FORMATTED	8	/* drive appears to have been formatted */
#define	ID_LOCKED	0x10	/* locked for exclusive use by ioctl command */
#define ID_WANTED	0x20	/* ioctl command waiting for exclusive use */
	/*	0xffffff00	bits 31-8 reserved. flags from ipi_error.h */

/*
 * Modes for id_send_cmd().
 */
#define ID_SYNCH	0	/* wait for I/O to complete before returning */
#define ID_ASYNCH	1	/* do I/O asynchronously */
#define ID_ASYNCHWAIT	2	/* do I/O asynchronously, sleep until done */

/*
 * Flags in ipiq (see ipi_driver.h)
 */
#define	IP_DIAGNOSE	IP_DD_FLAG0	/* conditional success is an error */
#define	IP_SILENT	IP_DD_FLAG1	/* suppress messages about errors */
#define	IP_WAKEUP	IP_DD_FLAG2	/* wakeup on q after completion */
#define	IP_ABS_BLOCK	IP_DD_FLAG3	/* for ioctl rdwr - no block mapping */
#define IP_BYTE_EXT	IP_DD_FLAG4	/* extent is in bytes, not blocks */
#define	IP_WCHK		IP_DD_FLAG5	/* read after write to verify this op */
#define	IP_WCHK_READ	IP_DD_FLAG6	/* this op was read to check write */

/*
 * Fields used in ipiq (see ipi_driver.h)
 */
#define	q_errblk	q_dev_data[0]	/* block containing error */
#define	q_related(q)	((struct ipiq *)((q)->q_dev_data[1]))	/* "other" q */

/*
 * Error codes.
 * 
 * The error code has three parts, the media/nonmedia flag, the DK severity
 * (from sun/dkio.h) and the id-driver error code.
 */
#define IDE_CODE(media, sev, err) ((media) << 15 | (sev) << 8 | (err))
#define IDE_MEDIA(code)		((code) & (1<<15))
#define IDE_SEV(code)		(((code) >> 8) & 0xf)
#define IDE_ERRNO(code)		((code) & 0xff)

#define IDE_NOERROR	0
#define IDE_FATAL	IDE_CODE(0, DK_FATAL,	   1)	/* unspecified bad */
#define IDE_CORR	IDE_CODE(1, DK_CORRECTED,  2)	/* corrected data err */
#define IDE_UNCORR	IDE_CODE(1, DK_FATAL,      3)	/* hard data error */
#define IDE_DATA_RETRIED IDE_CODE(1, DK_RECOVERED, 4)	/* media retried OK */
#define IDE_RETRIED	IDE_CODE(0, DK_RECOVERED,  5)	/* retried OK */

#endif	/* !_idvar_h */
