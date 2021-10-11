#ident	"@(#)sddef.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1989, 1990 by Sun Microsytems, Inc.
 */
#ifndef	_scsi_targets_sddef_h
#define	_scsi_targets_sddef_h

/*
 *
 * Defines for SCSI direct access devices
 *
 */

/*
 * Compile options
 */

#define	ADAPTEC			/* compile in support for the ACB 4000 */

/*
 * Manifest defines
 */

#define	SECSIZE		DEV_BSIZE	/* Bytes/sector */
#define	SECDIV		DEV_BSHIFT	/* log2 (SECSIZE) */


/*
 *
 * Local definitions, for clarity of code
 *
 */

#ifdef	OPENPROMS
#define	DRIVER		sd_ops
#define	DNAME		devp->sd_dev->devi_name
#define	DUNIT		devp->sd_dev->devi_unit
#define	CNAME		devp->sd_dev->devi_parent->devi_name
#define	CUNIT		devp->sd_dev->devi_parent->devi_unit
#else	OPENPROMS
#define	DRIVER		sddriver
#define	DNAME		devp->sd_dev->md_driver->mdr_dname
#define	DUNIT		devp->sd_dev->md_unit
#define	CNAME		devp->sd_dev->md_driver->mdr_cname
#define	CUNIT		devp->sd_dev->md_mc->mc_ctlr
#endif	OPENPROMS

#define	UPTR		((struct scsi_disk *)(devp)->sd_private)
#define	ROUTE		(&devp->sd_address)

#define	SCBP(pkt)	((struct scsi_status *)(pkt)->pkt_scbp)
#define	SCBP_C(pkt)	((*(pkt)->pkt_scbp) & STATUS_MASK)
#define	CDBP(pkt)	((union scsi_cdb *)(pkt)->pkt_cdbp)
#define	BP_PKT(bp)	((struct scsi_pkt *)bp->av_back)

#define	Tgt(devp)	(devp->sd_address.a_target)
#define	Lun(devp)	(devp->sd_address.a_lun)

#define	New_state(un, s)	\
	(un)->un_last_state=(un)->un_state,  (un)->un_state=(s)
#define	Restore_state(un)	\
	{ u_char tmp = (un)->un_last_state; New_state((un), tmp); }

/*
 * Debugging macros
 */


#define	DEBUGGING	((scsi_options & SCSI_DEBUG_TGT) || sddebug > 1)
#define	DEBUGGING_ALL	((scsi_options & SCSI_DEBUG_TGT) || sddebug)
#define	DPRINTF		if (DEBUGGING) sdprintf
#define	DPRINTF_ALL	if (DEBUGGING || sddebug > 0) sdprintf
#define	DPRINTF_IOCTL	DPRINTF_ALL


/*
 * Macros for marking a partition open or closed
 * Arguments are a pointer to a unit structure and
 * a pointer to its dev_t.
 *
 * Assumptions:
 *
 *	+ There are less than 65536 partitions
 *	+ There is only one bdevsw and one cdevsw entry for the sd driver
 */

#define	SDUNIT(dev)	(minor((dev))>>3)
#define	SDPART(dev)	(minor((dev))&0x7)

#define	SD_ISCDEV(dev)	\
	(major((dev)) < nchrdev && cdevsw[major((dev))].d_open == sdopen)

#define	SD_ISBDEV(dev)	\
	(major((dev)) < nblkdev && bdevsw[major((dev))].d_open == sdopen)

#define	SD_SET_OMAP(un, dp)	\
	if (SD_ISCDEV(*(dp))) { \
		(un)->un_omap |= ((1<<SDPART(*(dp)))<<16); \
	} else { \
		(un)->un_omap |= (1<<SDPART(*(dp))); \
	}

#define	SD_CLR_OMAP(un, dp)	\
	if (SD_ISCDEV(*(dp))) { \
		(un)->un_omap &= ~((1<<SDPART(*(dp)))<<16); \
	} else { \
		(un)->un_omap &= ~(1<<SDPART(*(dp))); \
	}

#ifdef	ADAPTEC
/*
 * Support for ADAPTEC ACB4000 controller
 *
 * ACB4000 error codes.
 *
 * They are the concatenation of the class and code info.
 *
 */

/* Class 0 - drive errors */
#define	SC_ERR_ACB_NO_SENSE	0x00	/* no error or sense info */
#define	SC_ERR_ACB_NO_INDEX	0x01	/* no index or sector pulse */
#define	SC_ERR_ACB_NO_SEEK_CMPL	0x02	/* no seek complete signal */
#define	SC_ERR_ACB_WRT_FAULT	0x03	/* write fault */
#define	SC_ERR_ACB_NOT_READY	0x04	/* drive not ready */
#define	SC_ERR_ACB_NO_TRACK_0	0x06	/* no track 0 */

/* Class 1 - target errors */
#define	SC_ERR_ACB_ID_CRC	0x10	/* ID field not found after retry */
#define	SC_ERR_ACB_UNCOR_DATA	0x11	/* uncorrectable data error */
#define	SC_ERR_ACB_ID_ADDR_MK	0x12	/* missing ID address mark */
#define	SC_ERR_ACB_REC_NOT_FND	0x14	/* record not found */
#define	SC_ERR_ACB_SEEK		0x15	/* seek error */
#define	SC_ERR_ACB_DATA_CHECK	0x18	/* data check */

/* Class 2 - system-related errors */
#define	SC_ERR_ACB_ECC_VERIFY	0x19	/* ECC error detected during verify */
#define	SC_ERR_ACB_INTERLEAVE	0x1A	/* specified interleave too large */
#define	SC_ERR_ACB_BAD_FORMAT	0x1C	/* drive not properly formatted */
#define	SC_ERR_ACB_ILLEGAL_CMD	0x20	/* illegal command */
#define	SC_ERR_ACB_ILLEGAL_BLK	0x21	/* illegal block address */
#define	SC_ERR_ACB_VOL_OVERFLOW	0x23	/* illegal block addr after 1st blk */
#define	SC_ERR_ACB_BAD_ARG	0x24	/* bad argument */
#define	SC_ERR_ACB_ILLEGAL_LUN	0x25	/* invalid logical unit number */
#define	SC_ERR_ACB_CART_CHANGE	0x28	/* new cartridge inserted */
#define	SC_ERR_ACB_ERR_OVERFLOW	0x2C	/* too many errors */

/* Unknown psuedo error - used for convenience */
#define	SC_ERR_ACB_ERR_UNKNOWN	0xFF	/* unknown psuedo error */


struct acb4000_mode_select_parms {
	long	edl_len;
	long	reserved;
	long	bsize;
	unsigned fmt_code :8;
	unsigned ncyl	:16;
	unsigned nhead	:8;
	u_short	rwc_cyl;
	u_short	wprc_cyl;
	u_char	ls_pos;
	u_char	sporc;
};


#endif	/* ADAPTEC */


/*
 * Format defines and parameters
 *
 * XXX: This is all a mess, and should be in <scsi/impl/commands.h>
 *
 */

#define	fmt_parm_bits		g0_addr2	/* for format options */
#define	fmt_interleave		g0_count0	/* for encode interleave */
#define	defect_list_descrip	g1_addr3	/* list description bits */

/*
 * defines for value of fmt_parm_bits.
 */

#define	FPB_BFI			0x04	/* bytes-from-index fmt */
#define	FPB_CMPLT		0x08	/* full defect list provided */
#define	FPB_DATA		0x10	/* defect list data provided */

/*
 * Defines for value of defect_list_descrip.
 */

#define	DLD_MAN_DEF_LIST	0x10	/* manufacturer's defect list */
#define	DLD_GROWN_DEF_LIST	0x08	/* grown defect list */
#define	DLD_BLOCK_FORMAT	0x00	/* block format */
#define	DLD_BFI_FORMAT		0x04	/* bytes-from-index format */
#define	DLD_PS_FORMAT		0x05	/* physical sector format */


/*
 * Disk defect list - used by format command.
 */

#define	RDEF_ALL	0	/* read all defects */
#define	RDEF_MANUF	1	/* read manufacturer's defects */
#define	RDEF_CKLEN	2	/* check length of manufacturer's list */


#define	ST506_NDEFECT	127	/* must fit in 1K controller buffer... */
#define	ESDI_NDEFECT	ST506_NDEFECT	/* hack??? */

struct scsi_bfi_defect {	/* defect in bytes from index format */
	unsigned cyl  : 24;
	unsigned head : 8;
	long	bytes_from_index;
};

struct scsi_format_params {	/* BFI format list */
	u_short reserved;
	u_short length;
	struct  scsi_bfi_defect list[ESDI_NDEFECT];
};

/*
 * Defect list returned by READ_DEFECT_LIST command.
 */
struct scsi_defect_hdr {	/* For getting defect list size */
	u_char	reserved;
	u_char	descriptor;
	u_short	length;
};

struct scsi_defect_list {	/* BFI format list */
	u_char	reserved;
	u_char	descriptor;
	u_short	length;
	struct	scsi_bfi_defect list[ESDI_NDEFECT];
};

/*
 *
 * Direct Access device Reassign Block parameter
 *
 * Defect list format used by reassign block command (logical block format).
 *
 * This defect list is limited to 1 defect, as that is the only way we use it.
 *
 */

struct scsi_reassign_blk {
	u_short	reserved;
	u_short length;		/* defect length in bytes (defects * 4) */
	u_int 	defect;		/* Logical block address of defect */
};
/*
 *
 * Direct Access Device Capacity Structure
 *
 */

struct scsi_capacity {
	u_long	capacity;
	u_long	lbasize;
};

/*
 * Transfer statistics structure
 */

#define	NSDSBINS	8
#define	SDS_512_2K	0
#define	SDS_2K_4K	1
#define	SDS_4K_8K	2
#define	SDS_8K_16K	3
#define	SDS_16K_32K	4
#define	SDS_32K_64K	5
#define	SDS_64K_128K	6
#define	SDS_128K_PLUS	7

struct sdstat {
	/*
	 * Keep track of hi-water mark length of disk queue
	 */
	int	sds_hiqlen;

	/*
	 * How many requests turned into kluster ops
	 */
	int	sds_kluster;

	/*
	 * A transfer is either B_PAGEIO, to/from some
	 * kernel virtual address, or is some B_PHYS transfer.
	 */
	int	sds_npgio;
	int	sds_nsysv;
	/*
	 * We bin sort reads and writes by size.
	 */
	int	sds_wbins[NSDSBINS];
	int	sds_rbins[NSDSBINS];

};

/*
 * driver flags (used by the routine sdioctl_cmd())
 */

#define	SD_USCSI_CDB_KERNEL	0x01    /*
					 * addresses used in the cdb field
					 * of struct uscsi_scmd are kernel
					 * addresses
					 */

#define	SD_USCSI_BUF_KERNEL	0x02    /*
					 * addresses used in the bufaddr
					 * field of struct uscsi_scmd are
					 * kernel address
					 */
/*
 * Private info for scsi disks.
 *
 * Pointed to by the un_private pointer
 * of one of the SCSI_DEVICE structures.
 */

struct scsi_disk {
	struct scsi_device *un_sd;	/* back pointer to SCSI_DEVICE */
	struct scsi_pkt *un_rqs;	/* ptr to request sense command pkt */
	struct sd_drivetype *un_dp;	/* drive type table */
	long	un_capacity;		/* capacity of drive */
	long	un_lbasize;		/* logical block size */
	struct	buf *un_sbufp;		/* for use in special io */
	struct	dk_map un_map[NDKMAP];	/* logical partitions */
	struct	dk_geom un_g;		/* disk geometry */
	struct	diskhd	un_utab;	/* for queuing */
	struct	sdstat	un_sds;		/* sd statistics structure */
	u_long	un_omap;		/* open partition map, block && char */
	u_int	un_err_resid;		/* resid from last error */
	u_int	un_err_blkno;		/* disk block where error occurred */
	u_char	un_last_cmd;		/* last cmd (DKIOCGDIAG only) */
	u_char	un_soptions;		/* 'special' command options */
	u_char	un_err_severe;		/* error severity */
	u_char	un_status;		/* sense key from last error */
	u_char	un_err_code;		/* vendor unique error code  */
	u_char	un_retry_ct;		/* retry count */
	u_char	un_gvalid;		/* geometry is valid */
	u_char	un_state;		/* current state */
	u_char	un_last_state;		/* last state */
	u_char	un_wchkmap;		/*
					 * write check map - change size if
					 * # of partitions change...
					 */
	char	un_dkn;			/* dk number for iostats */
};

/*
 * Disk driver states
 */

#define	SD_STATE_NIL		0
#define	SD_STATE_CLOSED		1
#define	SD_STATE_OPENING	2
#define	SD_STATE_OPEN		3
#define	SD_STATE_SENSING	4
#define	SD_STATE_RWAIT		5
#define	SD_STATE_DETACHING	6
#define	SD_STATE_DUMPING	7

/*
 * Error levels
 */

#define	SDERR_ALL		0
#define	SDERR_UNKNOWN		1
#define	SDERR_INFORMATIONAL	2
#define	SDERR_RECOVERED		3
#define	SDERR_RETRYABLE		4
#define	SDERR_FATAL		5

/*
 * Parameters
 */

	/*
	 * 35 seconds is a *very* reasonable amount of time for most disk
	 * operations.
	 */

#define	SD_IO_TIME	35

	/*
	 * 2 hours is an excessively reasonable amount of time for format
	 * operations.
	 */

#define	SD_FMT_TIME	120*60

	/*
	 * 5 seconds is what we'll wait if we get a Busy Status back
	 */

#define	SD_BSY_TIMEOUT		5*hz	/* 5 seconds Busy Waiting */

	/*
	 * Number of times we'll retry a normal operation.
	 *
	 * This includes retries due to transport failure
	 * (need to distinguish between Target and Transport failure)
	 */

#define	SD_RETRY_COUNT		30


	/*
	 * Maximum number of units we can support
	 * (controlled by room in minor device byte)
	 */
#define	SD_MAXUNIT		32


/*
 * sdintr codes
 */

#define	COMMAND_DONE		0
#define	COMMAND_DONE_ERROR	1
#define	QUE_COMMAND		2
#define	QUE_SENSE		3
#define	JUST_RETURN		4

/*
 * Drive Types (and characteristics)
 */

struct sd_drivetype {
	char	ctype;		/* controller type */
	char	options;	/* drive options */
	char	*id;		/* Vendor id + part of Product id */
};

/*
 * Controller type - partially historical.
 * Note that while CTYPE_CCS is defined,
 * the DKIOCINFO ioctl returns DKC_MD21
 * if CTYPE_CCS is defined. This will
 * happen until format(8) is fixed to
 * distinguish between a MD21 SCSI/ESDI
 * controller and a true CCS device.
 *
 */

#define	CTYPE_ACB4000		0
#define	CTYPE_MD21		1
#define	CTYPE_CCS		2

/*
 * Options
 */

#define	SD_NODISC	0x1	/*
				 * Target has difficulty with
				 * disconnect/reconnect
				 */

#define	SD_USELINKS	0x2	/*
				 * Target supports linked commands and
				 * has a performance advantage in using
				 * them.
				 */

#define	SD_NOVERIFY	0x4	/*
				 * Target does not support the VERIFY
				 * command (for DKIOCWCHK operations).
				 * This can be found out the hard way
				 * if the target bounces a VERIFY command
				 * with an ILLEGAL REQUEST check condition.
				 */

#define	SD_NOPARITY	0x8	/*
				 * Target does not generate parity, so request
				 * transport to *not* check parity for this
				 * target.
				 */

#define	SD_MULTICMD	0x10	/*
				 * Target supports SCSI-2 multiple commands.
				 */

#endif	/* _scsi_targets_sddef_h */
