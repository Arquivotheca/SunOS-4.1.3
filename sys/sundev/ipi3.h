/* @(#)ipi3.h	1.1 7/30/92 19:51:58 SMI */

/*
 * IPI-3 command packets, response packets, and parameters
 */
#ifndef _ipi3_h
#define _ipi3_h

/*
 * Common header for all IPI-3 packets. 
 */
struct ipi3header {
	u_short		hdr_pktlen;	/* command packet length 
						(not including length) */
	u_short		hdr_refnum;	/* unique command reference number */
	u_char		hdr_opcode;	/* command opcode */
	u_char		hdr_mods;	/* common and opcode modifiers */
	u_char		hdr_slave;	/* slave (controller) address */
	u_char		hdr_facility;	/* facility (drive) address */
};

#define	IPI_HDRLEN	(sizeof(struct ipi3header) - sizeof(u_short))

/*
 * Response packet header.
 */
struct ipi3resp {
	u_short		hdr_pktlen;	/* response packet length 
						(not including length) */
	u_short		hdr_refnum;	/* unique command reference number */
	u_char		hdr_opcode;	/* command opcode */
	u_char		hdr_mods;	/* common and opcode modifiers */
	u_char		hdr_slave;	/* slave (controller) address */
	u_char		hdr_facility;	/* facility (drive) address */
	u_short		hdr_maj_stat;	/* major response status */
};

/*
 * Command Opcodes.
 */
#define IP_NOP		0x00	/* Nop */
#define IP_ATTRIBUTES	0x02	/* set, report, or load slave attributes */
#define IP_REPORT_STAT	0x03	/* Report Addressee Status */
#define IP_PORT_ADDR	0x04	/* Port Address */
#define IP_OPER_MODE	0x07	/* Set Operating mode of facility */

#define IP_READ		0x10	/* Read */
#define IP_WRITE	0x20	/* Write */

#define IP_FORMAT	0x28	/* Format */

#define IP_REALLOC	0x33	/* Reallocate Defect */
#define IP_ALLOC_RESTOR 0x34	/* Allocate restore */

#define IP_POS_CTL	0x41	/* Position Control */
#define IP_REPORT_POS	0x42	/* Report Position */

#define IP_READ_BUF	0x52	/* Read Buffer */
#define IP_WRITE_BUF	0x62	/* Write Buffer */

#define IP_SLAVE_DIAG	0x80	/* Perform Slave Diagnostics */
#define IP_FACIL_DIAG	0x81	/* Perform Facility Diagnostics */
#define IP_READ_DEFLIST 0x82	/* Read Defect List */
#define IP_WRITE_DEFLIST 0x83	/* Write Defect List */
#define IP_READ_ERRLOG	0x84	/* Read Error Log */

#define IP_DIAG_CTL	0x90	/* Diagnostic Control */

#define IP_ASYNC_RESP	0xff	/* opcode in asynchronous response */

/*
 * Common Opcode modifiers.
 *
 * Sequential (0x20 )and Ordered (0x30) commands are not supported.
 */
#define IP_CM_PRI	0x40	/* Priority command */
#define IP_CM_CHAIN	0x10	/* Chained command */

/*
 * Opcode modifiers for Transfer Commands
 */
#define	IP_OM_BLOCK	0x01	/* count and length are in blocks */
#define IP_OM_RAW	0x02	/* data recovery off */

/*
 * Opcode modifiers for Attributes commands.
 */
#define IP_OM_REPORT	0	/* report attributes */
#define IP_OM_INIT	1	/* initialize attributes */
#define	IP_OM_RESTORE	2	/* restore attributes */
#define	IP_OM_LOAD	9	/* load attributes */
#define	IP_OM_SAVE	0xa	/* save attributes */

/*
 * Opcode modifiers for Report Addressee Status.
 */
#define	IP_OM_PORT_QUERY 4	/* return port mask */
#define	IP_OM_STATUS	2	/* report status (vendor unique) */
#define IP_OM_CONDITION	1	/* return condition */

/*
 * Response type in major status.
 * 	Only these are supported.
 */
#define IP_RTYP(x)	((x) & 0xf0)	/* extract type */
#define IP_MAJ_CODE(x)	((x) & 0xff0f)	/* extract major type */
#define IP_RT_CCR	0x10	/* command completion response */
#define IP_RT_ASYNC	0x40	/* asychronous response */
#define IP_RT_XFER	0x50	/* transfer notification (never seen by host) */

/*
 * Flags in response major status.
 */
#define	IP_MS_CMDEXC	0x8000	/* command exception */
#define IP_MS_MCHEXC	0x4000	/* machine exception */
#define IP_MS_APEXC	0x2000	/* alternate port exception */
#define IP_MS_INTREQ	0x1000	/* intervention required */
#define IP_MS_MSG	0x0800	/* message/microcode exception */
#define	IP_MS_VENDOR	0x0400	/* vendor unique */
#define IP_MS_SUCCESS	0x0008	/* successful */
#define	IP_MS_INCOMP	0x0004	/* incomplete */
#define IP_MS_COND	0x0002	/* conditional success */
#define IP_MS_ABORT	0x0001	/* aborted */



/*
 * Parameters
 *
 * All parameters start with a one byte length followed by a one byte ID.  
 * A length of zero indicates a padding byte for alignment in the sender. 
 * On receipt, parameters may be on any boundary, which must be dealt with
 * if the machine has alignment restrictions.
 */

/*
 * Message/Microcode exception parameter (0x13 or 0x23).
 */
struct msg_ucode_parm {
	u_short		mu_flags;	/* 1-2 error flags - see below */
	u_short		reserved;	/* 3-4 reserved */
	union {				/* 5-n extended substatus */
		struct {
			u_short	mu_fru;	/* 5-6 Field-Replaceable Unit ID */
			char mu_msg[1]; /* 7-n ASCII message */
		} mu_m;
	} mu_u;
};

/*
 * Message/Microcode error flags
 */
#define	IPMU_UDAT_NA	0x8000		/* microcode data not accepted */
#define	IPMU_REQ_IML	0x4000		/* request master to IML slave */
#define	IPMU_CANT_IML	0x2000		/* slave unable to IML */
#define	IPMU_MSG	0x1000		/* message */
#define	IPMU_UC_ERR	0x0800		/* microcode execution error */
#define	IPMU_FAIL_MSG	0x0400		/* failure message */
#define	IPMU_PORT_DSBL	0x0200		/* port disable pending */
#define	IPMU_PORT_RESP	0x0100		/* port response */
#define	IPMU_FAC_STAT	0x0080		/* facility status */

/*
 * Transfer Notification Parm (0x30). 
 * Holds the read/write buffer address in DVMA space.
 * This is only used for some string controllers and drivers, notably
 * the Sun Panther VME-attached string controller.
 */
struct xnote_parm {
	caddr_t		bufadr;		/* DVMA buffer address */
};

/*
 * Command Extent Parm (0x31). Defines the tranfer count and start address
 * (expressed as a logical block offset from the start of the disk) for the
 * transfer.
 */
struct cmdx_parm {
	u_long		cx_count;	/* transfer count (bytes or blocks) */
	u_long		cx_addr;	/* data address (bytes or blocks) */
};

/*
 * Response Extent Parm (0x32).  In response this gives the residual count
 * in blocks or octets and the address of the next block or octet to be
 * transferred.
 */
struct respx_parm {
	u_long		rx_resid;	/* residual count */
	u_long		rx_addr;	/* address to resume transfer */ 
};

/*
 * Block Size Parm (0x3b).  Defines physical block size for format, etc.
 */
struct bsize_parm {
	u_long		blk_size;
};

/*
 * Transfer Parm (0x3c). Options for data transfer commands. Currently used
 * only by WRITE to do a "write with verify".
 */
struct xfer_parm {
	u_char		xferopt;	/* see bits below */
};

#define	XFER_PARM	0x3c

#define XFER_VERIFY	0x80		/* verify */
#define	XFER_VOLUME	0x40		/* transfer applies to entire volume */
#define	XFER_CERTIFY	0x20		/* certify area after format */
#define	XFER_STOP_ERR	0x10		/* stop transfer on data error */
#define	XFER_COMP_SLAVE	0x02		/* compare with slave buffer */
#define	XFER_COMP_HOST	0x01		/* comparee with master's buffer */

/*
 * Vendor ID parameter (0x50). Supply vendor specific info. Extra fields are
 * supplied for future expansion. This parameter is read-only.
 */
struct vendor_parm {
	char		manuf[16];	/* Manufacturer's ID (SUNMICRO) */
	char		model[8];	/* Model # (PANTHER) */
	u_char		rev[4];		/* revision level */
	u_char		uniq_id[8];	/* Controller Unique ID (serial #) */
	u_char		switset[4];	/* Switch Settings */
	u_char		man_data[24];	/* Manufacturer's data */
};


/*
 * Diagnostic Number parameter (for Diagnostic Control Op) (0x50).
 */
struct ipi_diag_num {
	u_short		dn_diag_num;	/* diagnostic number */
};

#define	IPDN_DUMP	1		/* dump firmware to first ready drive */

/*
 * Size of DataBlocks parameter (0x51). Default is 512.
 */
struct datbsize_parm {
	int		dblksize;
};

/*
 * Size of Physical Blocks parameter (0x52).  Always the same as DataBlocks.
 */
struct physbsize_parm {
	int		pblksize;
};

/*
 * Total Number of Disk Data Blocks parameter (0x53). The starting address
 * field is always zero.
 */
struct numdatblks_parm {
	int		bperpart;	/* Blocks per Partition */
	int		bpercyl;	/* Blocks per Cylinder */
	int		bpertrk;	/* Blocks per Track */
	int		strtadr;	/* Starting Address */
};

/*
 * Total Number of Disk Physical Blocks parameter (0x54). The starting
 * address field is always zero.
 */
struct numphyblks_parm {
	int		bperpart;	/* Blocks per Partition */
	int		bpercyl;	/* Blocks per Cylinder */
	int		bpertrk;	/* Blocks per Track */
	int		strtadr;	/* Starting Address */
};

/*
 * Data Block Sizes Supported parameter (0x55). Define the size (in bytes) of
 * a data block. Default is 512.
 */
struct datbss_parm {
	int		mindatbs;	/* Min data block size */
	int		maxdatbs;	/* Max data block size */
	int		incsize;	/* Increment size */
};

/*
 * Physical Block Sizes Supported parameter (0x56). Define the size (in
 * bytes) of a physical block. Default is 512.
 */
struct phybss_parm {
	int		mindatbs;	/* Min data block size */
	int		maxdatbs;	/* Max data block size */
	int		incsize;	/* Increment size */
};

/*
 * Pad with Fill Characters parameter (0x5a). The slave uses the value in
 * this field when it needs to pad data. The default is 0xfb87.
 */
struct pad_parm {
	char		filler[4];	/* Fill characters */
};

/*
 * Physical Disk Configuration parameter (0x5f).
 */
struct physdk_parm {
	u_int		pp_last_cyl;	/* Address of last data cylinder */
	u_int		pp_deflist_cyl;	/* defect list cylinder */
	u_short		pp_nheads;	/* heads per cylinder*/
	u_short		pp_nsects;	/* fixed sectors per revolution */
	u_int		pp_nbytes;	/* bytes per track */
	u_int		pp_sinseek;	/* single cylinder seek time (usec) */
	u_int		pp_avgseek;	/* average cylinder seek time (usec) */
	u_int		pp_maxseek;	/* maximum cylinder seek time (usec) */
	u_int		pp_rotper;	/* rotational period (usec) */
	u_int		pp_hdswit; 	/* head switch time (usec) */
	u_int		pp_wrrec;	/* write to read recovery time (usec) */
	u_char		pp_model[4];	/* model number in ASCII */
};

/*
 * Non-Standard Physical Disk Configuration parameter (0x5f).
 *	The CDC CM-3 controller uses this format.
 */
struct physdk_parm_cm3 {
	u_int		pp_last_cyl;	/* Address of last data cylinder */
	u_int		pp_deflist_cyl;	/* defect list cylinder */
	u_short		pp_nheads;	/* heads per cylinder*/
	u_int		pp_nsects;	/* fixed sectors per revolution */
	u_int		pp_nbytes;	/* bytes per track */
	u_int		pp_sinseek;	/* single cylinder seek time (usec) */
	u_int		pp_avgseek;	/* average cylinder seek time (usec) */
	u_int		pp_maxseek;	/* maximum cylinder seek time (usec) */
	u_int		pp_rotper;	/* rotational period (usec) */
	u_int		pp_hdswit; 	/* head switch time (usec) */
	u_int		pp_wrrec;	/* write to read recovery time (usec) */
	u_char		pp_model[4];	/* model number in ASCII */
};
/*
 * Addressee Configuration parameter (0x65). Define the operating
 * characteristics of the addressee (controller).
 */
struct addr_conf_parm {
	u_int		ac_bufsize;	/* size of slave buffer in bytes */
	u_int		ac_cmdbufsize;	/* Command buffer size in bytes */
	u_short		ac_max_cmdlen;	/* Max len of cmd packet */
	u_short		ac_max_resplen;	/* Max len of response packet */
	u_char		ac_acc_perm_ext; /* Max access permit extents */
	u_char		ac_min_queue;	/* minimum queued cmds per device */
	u_char		ac_max_queue;	/* maximum queued cmds per device */
	u_char		ac_stk_size;	/* size in bytes of command stack */
};

/*
 * Slave Configuration (bit significant) parameter (0x66). Define the
 * operating characteristics of the slave (facility) using bit fields.
 */
struct slavconf_bs_parm {
	u_char		slavebits[4];	/* slave config bit fields (defines
					 * below) */
};

/*
 * Slave Configuration (Fields) parameter (0x67). Define the operating
 * characteristics of the slave (facility) using fields.
 */
struct slavconf_fld_parm {
	u_char		spare1[3];
	u_char		nparts; /* number of data partitions per facility (1) */
	u_char		spare2[4];
	u_char		maxextent;	/* max number of extents */
	u_char		spare3[5];
	char		fattched[8];	/* number of facitilies attached */
};

/*
 * Facilities Attached to Slave parameter (0x68). 	
 * Identifies the facilities (disks) attached to the slave.
 * Repeats as necessary to define all facilities.
 */
struct fat_parm {
	u_char		fa_addr;	/* Facility Address */
	u_char		fa_class; 	/* Facility Class */
	u_char		fa_type;	/* Facility Type */
	u_char		reserved;	/* reserved */
};

/*
 * Request Parm parameter (0x6c). 
 *
 * Specifies parameters desired and how they are to be provided.
 *
 * The parameter can be provided as data, (i.e. DMA'ed like a normal 
 * data operation).  Panther requires a Length Parameter in this case to
 * declare the length of the buffer. 
 * 
 * The other method for obtaining parameters is to ask for them to be sent 
 * back in the IPI-3 response packet.  
 */
struct reqparm_parm {
	u_char		rp_spec;	/* specify how to respond */
	u_char		rp_parm;	/* ID of requested parameter */
};

/*
 * Bitdefs for REQUEST PARM parameter
 */
#define RESP_AS_DATA	0x80		/* Send back as data */
#define RESP_AS_PKT	0x40		/* Send back as a response packet */
#define RESP_LEN_PARM	0x20		/* provide parameter length parameter */
#define RESP_AS_NDATA	0x10		/* naked parameters as data */

/*
 * Parm Length parameter (0x6d). This is used with the Request Parm parameter
 * to specify how long a response transferred as data will be.  
 * This is usually set to 8k by the user (format) program.
 */
struct parmlen_parm {
	u_long		parmlen;	/* Length of parameter in bytes */
};

/*
 * Slave Reconfiguration parameter - Bit Significant (0x6e). 
 * Define the reconfigurable attributes of the slave.
 */
struct reconf_bs_parm {
	u_char	sr_cs_error	: 1;	/* report cond success on error retry */
	u_char	sr_cs_data	: 1;	/* report CS on data correction */
	u_char	sr_inh_ext_substat : 1;	/* inhibit extended substatus */
	u_char	sr_phys_sel_syn : 1;	/* physical selection of synonyms */
	u_char	sr_auto_realloc : 1;	/* auto realloc on excessive retrys */
	u_char	sr_seek_alg	: 1;	/* seek algorithm on */
	u_char	sr_inh_resp_succ : 1;	/* inhibit response on success */
	u_char			: 1;	/* reserved */

	u_char	sr_tnp_req 	: 1;	/* transfer notification required */
	u_char	sr_inh_slv_msg 	: 1;	/* inhibit slave messages */
	u_char	sr_inh_unant_pause : 1;	/* inhibit unanticipated pauses */
	u_char	sr_dis_err_rcvry : 1;	/* disable all error recovery */
	u_char	sr_log_unexp_cl1 : 1;	/* log unexpected class 1 events */
	u_char	sr_dis_cl1x 	: 1;	/* discard class 1 transitions */
	u_char	sr_stream_xfer	: 1;	/* data streaming data transfers */
	u_char	sr_inlk_xfer	: 1;	/* interlock data transfers */

	u_char	sr_inh_resp_npbusy : 1;	/* inhibit resp P-busy to not P-busy */
	u_char			: 7;	/* reserved */
};

/*
 * Slave Reconfiguration parameter - Fields (0x6f). Define the reconfigurable
 * attributes of the slave.
 */
struct reconf_fld_parm {
	u_char		resrv1[8];	/* Reserved */
	short		maxbincmd;	/* Maximum bytes in command packet */
	short		maxbinrsp;	/* Maximum bytes in response packet */
	u_char		resrv2[8];
	int		burstsize;	/* DMA burst length in bytes (128) */
	u_char		resrv3[8];
};

/*
 * Panther Reconfiguration parameter (0xD0). Put vendor unique stuff here.
 */
struct panreconf_parm {
	u_char		rahead;		/* Read-Ahead enabled flag */
	u_char		burstlate;	/* Inter-Burst latency */
	short		breqlevl;	/* VME Bus Request level */
	short		vecnum;		/* Interrupt Vector Number */
	short		shmemoff;	/* Offset from base I/O addr for
					 * shared mem */
	char		resrv1[14];	/* Reserved */
};


/* 
 * Entire command for read/write operation.  This makes it cheaper to
 * build since this version is so common.
 */
struct ipi_rdwr_cmd {
	struct ipi3header hdr;		/* header */
	long		pad_len_id;	/* pad, len, id */
	struct cmdx_parm cmdx;		/* extent parameter */
};

/* 
 * Read write command for host adapters that require transfer notification.
 */
struct ipi_rdwr_cmdx {
	struct ipi3header hdr;		/* header */
	long		pad_len_idx;	/* pad, len, id */
	struct xnote_parm xnote;	/* transfer notification parameter */
	long		pad_len_id;	/* pad, len, id */
	struct cmdx_parm cmdx;		/* extent parameter */
};

/*
 * Format command.
 * 	The block size parameter is not used for Panther.
 */
struct ipi_format_cmd {
	struct ipi3header fc_hdr;	/* header */
	u_short		fc_filler;	/* zeros */
	u_char		fc_bsize_len;	/* parameter length (5) */
	u_char		fc_bsize_id;	/* parameter ID */
	struct bsize_parm fc_bsize_parm; /* block size parameter */
};

/*
 * Parameter IDs.
 */

/*
 * Major status parameters for slave.
 */
#define	IPI_S_VEND	0x12	/* vendor unique */
#define	IPI_S_MESSAGE	0x13	/* message/microcode exception */
#define	IPI_S_INTREQ	0x14	/* intervention required */
#define	IPI_S_ALTPORT	0x15	/* alternate port exception */
#define	IPI_S_MCH_EXC	0x16	/* machine exception */
#define	IPI_S_CMD_EXC	0x17	/* command exception */
#define	IPI_S_ABORT	0x18	/* command aborted */
#define	IPI_S_COND_SUCC	0x19	/* conditional success */
#define	IPI_S_INCOMP	0x1a	/* incomplete */

/*
 * Major status parameters for facility.
 */
#define	IPI_F_VEND	0x22	/* vendor unique */
#define	IPI_F_MESSAGE	0x23	/* message/microcode exception */
#define	IPI_F_INTREQ	0x24	/* intervention required */
#define	IPI_F_ALTPORT	0x25	/* alternate port exception */
#define	IPI_F_MCH_EXC	0x26	/* machine exception */
#define	IPI_F_CMD_EXC	0x27	/* command exception */
#define	IPI_F_ABORT	0x28	/* command aborted */
#define	IPI_F_COND_SUCC	0x29	/* conditional success */
#define	IPI_F_INCOMP	0x2a	/* incomplete */

/*
 * Common parameters 
 */
#define	XFER_NOTIFY	0x30	/* transfer notification (VME addr) */
#define	CMD_EXTENT	0x31	/* command extent (count + block addr ) */
#define RESP_EXTENT	0x32	/* response extent (residual count and addr) */
#define	BSIZE_PARM	0x3b	/* block size */

/* ATTRIBUTES Parameters */

#define	ATTR_VENDOR	0x50	/* vendor ID */
#define	ATTR_LOG_BSIZE	0x51	/* logical data block size */
#define	ATTR_PHYS_BSIZE	0x52	/* physical data block size */
#define	ATTR_LOG_GEOM	0x53	/* layout of blocks on disk */
#define	ATTR_PHYS_GEOM	0x54	/* layout of blocks on disk */
#define	ATTR_LOG_SUP	0x55	/* logical block sizes supported */
#define	ATTR_PHYS_SUP	0x56	/* physical block sizes supported */
#define	ATTR_FILL_PAT	0x5a	/* fill pattern */
#define	ATTR_PHYSDK	0x5f	/* physical disk configuration */

#define	ATTR_ADDR_CONF	0x65	/* addresssee configuration */
#define	ATTR_FAC_ATTACH 0x68	/* facilities attached */
#define	ATTR_REQPARM	0x6c	/* request parameters */
#define	ATTR_PARMLEN	0x6d	/* length of parameter list */
#define	ATTR_SLVCNF_BIT 0x6e	/* slave configuration - bits supported */
#define	ATTR_SLVCNF_FLD 0x6f	/* slave configuration - fields supported */

#define	ATTR_PANTHER	0xd0	/* panther-unique parameter? -later */

/*
 * Read/Write Defect list Parameters
 */
#define DEFLIST_DBLOCK	0x55	/* defective datablock */
#define DEFLIST_TRACK	0x56	/* track defects list parameter */
#define DEFLIST_SECTOR	0x57	/* sector defects list parameter */

/*
 * Diagnostic Control Parameters.
 */
#define IPI_DIAG_NUM	0x50	/* diagnostic number parameter */

#endif /* _ipi3_h_ */
