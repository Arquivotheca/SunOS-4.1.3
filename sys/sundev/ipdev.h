/*
 * @(#)ipdev.h 1.1 92/07/30 Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 */
#ifndef	    _IPIPKT
#define	    _IPIPKT_

#define IPI3PKTLEN  64
/*
 * Define all the IPI-3 commands and parameters
 * (EVERYTHING HARDWARE SPECIFIC COMES HERE)
 */

/*****************************************
 * I P I - 3    P A C K E T   I N F O    
 *****************************************/


/************************  
 * P A R A M E T E R S  
 ************************/

/* 
 * NOTE:
 * Substatus parameters can have one of two parameter IDs, depending
 * on whether a controller or a facility is being referred to. The form
 * for the ID is 'xN' where x is 1 if the status refers to a controller, 
 * 2 if it refers to a facility. For example, the intervention required 
 * parameter ID is 0x12 if a command couldn't be completed because 
 * the controller needs help, or 0x22 if the facility was at fault.
 */

/*
 * Diagnostic Exception (0x12 or 0x22). An error has occured during
 * on-board diagnostics. Bit defs below.
 */
struct diag_excep_parm {
	u_char	len;
	u_char	id;
	u_char	status[2];
	u_char	reservd[2];
};

/*
 * Intervention Required (0x14 or 0x24). The controller is unable to
 * respond and intervention is required before it will be able to.
 */
struct interv_rqd_parm {
	u_char	len;
	u_char	id;
	u_char	status;
	u_char	reservd[3];
};

/*
 * Alternate Port Exception (0x15 or 0x25). The slave or facility 
 * has detected an event from an alternate port.
 */
struct altport_parm {
	u_char	len;
	u_char	id;
	u_char	status;
	u_char	reservd[2];
};

/*
 * Machine Exception (0x16 or 0x26). An unusual hardware condition
 * has been detected on a controller or facility.
 */
struct machexcp_parm {
	u_char	len;
	u_char	id;
	u_char	status[4];
	u_char	ext_status;
	u_char	reservd;
};

/*
 * Command Exception (0x17 or 0x27). Exceptions relating to command
 * processing.
 */
struct cmdexcep_parm {
	u_char len;
	u_char id;
	u_char status[4];
	u_char ext_status;
	u_char reservd;
};

/*
 * Conditional Success (0x17 or 0x27). The command succeeded, but
 * additional information is available (e.g. retries were done).
 */
struct condsucc_parm {
	short	pad;
	u_char	len;
	u_char	id;
	u_char	status[4];
	u_char	ext_status;
	u_char	reservd;
};

/* 
 * Transfer Notification Parm (0x30). Holds the read/write
 * buffer address in DVMA space.
 */
struct xnote_parm {
	short	pad;
	u_char  len;	
	u_char  id;
	caddr_t bufadr;		/* DVMA buffer address */
};	

/* 
 * Command Extent Parm (0x31). Defines the tranfer count and start address  
 * (expressed as a logical block offset from the start of the disk)
 * for the transfer.
 */
struct cmdx_parm {
	short	pad;
	u_char	len;
	u_char	id;		
	u_long	blk_cnt;		/* # of Blocks (sectors) to read */
	daddr_t lbn;		/* start address for transfer */ 
};

/* 
 * Transfer Parm (0x3c). Options for data transfer commands.
 * Currently used only by WRITE to do a "write with verify".
 */
struct xfer_parm {
	u_char len;
	u_char id;		
	u_char xferopt;	/* see bits below */
};

/*
 * Disk Modes parameter (0x51). Allows the host to change the operating
 * characteristics of the slave or facility. Bit defs for mode field are 
 * described below. 
 */
struct dkmode_parm {
	u_char len;
	u_char id;
	u_short modes; 
};

/*
 * Vendor ID parameter (0x50). Supply vendor specific info.
 * Extra fields are supplied for future expansion. This parameter
 * is read-only.
 */
struct vendor_parm {
	u_char	len;
	u_char	id;
	char	manuf[0x10];	/* Manufacturer's ID (SUNMICRO) */
	char	model[8];	/* Model # (PANTHER) */
	u_char	hdwr_rev;	/* Hardware Revision Level */
	u_char	artw_rev;	/* Artwork Revision Level */
	u_char	firm_rev;	/* Firmware Revision Level */
	u_char	resrvd;		/* Reserved */
	char	uniq_id[8];	/* Unique ID */
	u_char	switset[4];	/* Switch Settings */
	u_char	extra[8];	/* extra space */
};
/*
 * Size of DataBlocks parameter (0x51). Default is 512. 
 */
struct datbsize_parm {
	u_char	len;
	u_char	id;
	int	dblksize;
};

/*
 * Size of Physical Blocks parameter (0x52).  Always the same as DataBlocks.
 */
struct physbsize_parm {
	u_char	len;
	u_char	id;
	int	pblksize;
};

/*
 * Total Number of Disk Data Blocks parameter (0x53). 
 * The starting address field is always zero.
 */
struct numdatblks_parm {
	u_char len;
	u_char id;
	int bperpart;			/* Blocks per Partition */
	int bpercyl;			/* Blocks per Cylinder */
	int bpertrk;			/* Blocks per Track */
	int strtadr;			/* Starting Address */
}; 

/*
 * Total Number of Disk Physical Blocks parameter (0x54). 
 * The starting address field is always zero.
 */
struct numphyblks_parm {
	u_char len;
	u_char id;
	int bperpart;			/* Blocks per Partition */
	int bpercyl;			/* Blocks per Cylinder */
	int bpertrk;			/* Blocks per Track */
	int strtadr;			/* Starting Address */
}; 

/*
 * Data Block Sizes Supported parameter (0x55). Define the 
 * size (in bytes) of a data block. Default is 512.
 */
struct datbss_parm {
	u_char len;
	u_char id;
	int mindatbs;			/* Min data block size. Default = 512 */
	int maxdatbs;			/* Max data block size. Default = 2048 */
	int incsize;			/* Increment size. Default = 512 */
};

/*
 * Physical Block Sizes Supported parameter (0x56). Define the 
 * size (in bytes) of a physical block. Default is 512.
 */
struct phybss_parm {
	u_char len;
	u_char id;
	int mindatbs;			/* Min data block size. Default = 512 */
	int maxdatbs;			/* Max data block size. Default = 2048 */
	int incsize;			/* Increment size. Default = 512 */
};

/*
 * Pad with Fill Characters parameter (0x5a). The slave uses the value 
 * in this field when it needs to pad data. The default is 0xfb87. 
 */
struct pad_parm {
	u_char len;
	u_char id;
	char filler[4];			/* Fill characters */
};

/*
 * Physical Disk Configuration parameter (0x5f).
 */
struct physdk_parm {
	u_char len;
	u_char id;
	int ncyls;  			/* Number of Cylinders */
	int nheads;  			/* Number of Heads */
	int nsects;  			/* Sectors per Track */
	int nbytes;   			/* Bytes per Track */
	int sinseek;			/* Single Cylinder Seek Time (usec) */
	int avgseek;			/* Average Cylinder Seek Time (usec) */
	int maxseek;			/* Maximum Cylinder Seek Time (usec) */
	int hdswit;				/* Head Switch Time (usec) */
	int rotper;				/* Rotational period (usec) */ 
	char modno[4];			/* ASCII Model Number */
};
/*
 * Addressee Configuration parameter (0x65). Define the operating 
 * characteristics of the addressee (controller). 
 */
struct addrconf_parm {
	u_char len;
	u_char id;
	int bufrsize;    	/* size of slave buffer (256K default) */
	int cmdbufsize;     	/* Command buffer size (4352 default)!? */
	short maxbcmdpkt;      	/* Max len of cmd packet (64 bytes default) */
	short maxbrsppkt;      	/* Max len of response packet (64 bytes default) */
	u_char resrvd;         	/* Reserved */
	u_char minqued;     	/* Min # of cmds we can queue (8 default) */
	u_char maxqued;     	/* Max # of cmds we can queue (64 default) */
	u_char stksiz;       	/* Min # of cmds we can stack (68 default) */
};
/*
 * Slave Configuration (bit significant) parameter (0x66). Define the 
 * operating characteristics of the slave (facility) using bit fields.
 */
struct slavconf_bs_parm {
	u_char len;
	u_char id;
	u_char slavebits[4];	/* slave config bit fields (defines below) */
};
/*
 * Slave Configuration (Fields) parameter (0x67). Define the 
 * operating characteristics of the slave (facility) using fields.
 */
struct slavconf_fld_parm {
	u_char len;
	u_char id;
	u_char spare1[3];
	u_char nparts;		/* number of data partitions per facility (1) */
	u_char spare2[4];
	u_char maxextent;	/* max number of extents */
	u_char spare3[5];
	char fattched[8];	/* number of facitilies attached */
};
/*
 * Facilities Attached to Slave parameter (0x68). Identify the facilities
 * (disks) attached to the slave. Leave room for the maximum  possible.
 */
struct fatt_parm {      	/* no, not fat farm... */ 
	u_char len;
	u_char id;
	struct facil {
		u_char fadr;	/* Facility Address */
		u_char fclass;	/* Facility Class */
		u_char ftype;	/* Facility Type */
	} facil[8];		/* declare max possible */
};

/*
 * Request Parm parameter (0x6c). This parameter is used two ways: first, 
 * the response parameter is passed back as data (i.e. DMA'ed like a
 * normal data operation). We need to specify the length in such a case 
 * because the data comes back with no header. We always set the length
 * (via the Parm Length parameter) to be 8k to be sure that we never 
 * have a buffer overrun. The ID of the requested parameter is also needed
 * this case because a parameter sent back as data has no identifier fields.
 * The other method for obtaining a response is to ask for the response to 
 * be sent back as an IPI-3 response packet. In this case, we actually get 
 * the packet from Panther's shared memory area. The parameter ID is used
 * to tell Panther which parameter we want. 
 */

struct reqparm_parm {
	u_char len;
	u_char id;
	u_char howto;			/* specify how to respond - see bitdefs below */
	u_char whichparm;		/* ID of requested parameter */
};

/*
 * Parm Length parameter (0x6d). This is used with the Request Parm
 * parameter to specify how long a response transferred as data will 
 * be. This is always set to 8k - probably overkill, but it doesn't hurt. 
 */
struct parmlen_parm {
	u_short	pad;
	u_char	len;
	u_char	id;
	u_long	 parmlen;		/* Length of parameter in bytes */
};

/*
 * Slave Reconfiguration parameter - Bit Significant (0x6e). Define the 
 * reconfigurable attributes of the slave. Bit fields are described below.
 */
struct reconf_bs_parm {
	u_char len;
	u_char id;
	u_char conf1; 
	u_char conf2; 
	u_char conf3; 
	u_char resrvd;
};

/*
 * Slave Reconfiguration parameter - Fields (0x6f). Define the 
 * reconfigurable attributes of the slave.
 */
struct reconf_fld_parm {
	u_char len;
	u_char id;
	u_char resrv1[8];		/* Reserved */
	short maxbincmd;		/* Maximum bytes in command packet */
	short maxbinrsp;		/* Maximum bytes in response packet */
	u_char resrv2[8];
	int burstsize;			/* DMA burst length in bytes (128) */
	u_char resrv3[8];
};

/* 
 * Panther Reconfiguration parameter (0xD0). Put vendor unique stuff here.
 */

struct panreconf_parm	{
	u_char	len;
	u_char	id;
	u_char	rahead;		/* Read-Ahead enabled flag */
	u_char	burstlate;    	/* Inter-Burst latency */
	short	breqlevl;     	/* VME Bus Request level */ 
	short	vecnum;        	/* Interrupt Vector Number */
	short	shmemoff;     	/* Offset from base I/O addr for shared mem */
	char	resrv1[14];    	/* Reserved */
};

/* 
 * Common header for all IPI-3 packets. The reference number 
 * is the unique ID for a command.
 */

typedef long    Align;

union ipi3header {
        struct  {
                short   hdr_pktlen;     /* cmd packet length */
                u_short hdr_refno;      /* cmd reference number */
                u_char  hdr_opcode;     /* cmd Op Code */
                u_char  hdr_mods;               /* Common and OpCode modifiers */
                u_char  hdr_ctlr;               /* slave address (ctlr number) */
                u_char  hdr_unit;               /* facility address (unit number) */
        } hdr;
        Align   x;
};

/* 
 * Command Packet. The structure is a generic preamble followed by  
 * the parameters for this command. The parameters are broken out 
 * and reused by many different commands. The padding in many of the 
 * commands is to ensure correct longword alignment on SPARC machines.
 */

struct ipi3pkt {

	union ipi3header   pkt_hdr;
	union ipi_parms {
		/*
		 *******************
		 * CONTROL COMMANDS
		 *******************
		 */
		struct attributes {
			/* 
			 * This is a union because we only report or load 
			 * parameters one at a time.
			 */
			union {
				struct vendor_parm vendor;
				struct datbsize_parm datbs;
				struct physbsize_parm phybs;
				struct numdatblks_parm ndblks;
				struct numphyblks_parm npblks;
				struct datbss_parm dbss;
				struct phybss_parm pbss;
				struct pad_parm pad;
				struct addrconf_parm addrconf;
				struct physdk_parm physdk;
				struct slavconf_bs_parm slavconf_bit;
				struct slavconf_fld_parm slavconf_fld;
				struct fatt_parm fatt;
				struct reqparm_parm reqparm;
				struct parmlen_parm parmlen;
				struct reconf_bs_parm reconbit;
				struct reconf_fld_parm reconfld;
				struct panreconf_parm panther;
			} parm;
		} attrib;

		struct nop {
			char foo[IPI3PKTLEN - (sizeof (union ipi3header))];
		} nop;

		struct operating_mode {
			struct dkmode_parm dkmode; 
		} opermode;

		struct report_addressee_status {
			struct dkmode_parm dkstat;
		} status;

		/*
		 *******************
		 * POSITION COMMANDS
		 *******************
		 */
		struct position_control {
			struct cmdx_parm cxp;
		} posctl;

		struct report_position {
			struct cmdx_parm cxp;
		} rptpos;
		/*
		 *********************
	 	 * TRANSFER  COMMANDS
	 	 *********************
		 */
		struct format {
			struct cmdx_parm cxp;
		} format;

		struct read_write {
			struct xnote_parm xnp;
			struct cmdx_parm cxp;
		} rdwr;

		/*
		 ***********************
		 * COMBINATION COMMANDS
		 ***********************
		 */
		struct allocate_restore {
			struct cmdx_parm cxp;
		} allocrest;

		struct reallocate {
			struct cmdx_parm cxp;
		} realloc;
		/*
		 **********************
		 * DIAGNOSTIC COMMANDS
		 **********************
		 */
		
		/* XXX - these are still undefined */ 

		/* 
		struct ctlr_diag {
		} cdiag;

		struct facil_diag {
		} fdiag; 
		*/

		/*
		 * The following is for the READ DEFECT LIST and 
		 * READ ERROR LOG commands.
		 */
		struct spec_read {
			struct xnote_parm xnp;
			struct reqparm_parm reqparm;
			struct parmlen_parm plenparm;
		} rdspec;

	} p_cmd;

};

/*
 * IPI-3 response/error packet. Each controller is assigned one 
 * statically, since Panther posts only one response packet at a time.
 */
struct ipi3rsp {
	union ipi3header   res_hdr;
	u_short r_stat;
	union {
		/* 
		 * Substatus.
		 */
		struct diag_excep_parm diag_excep;
		struct interv_rqd_parm interv_rqd;
		struct altport_parm altport;
		struct machexcp_parm mach_excep;
		struct cmdexcep_parm cmd_excep;
		struct condsucc_parm cond_succ;
		/*
		 * Response parameters. The commands that currently send these
		 * are Report Addressee Status, Report Position, and Report 
		 * Attributes.
		 */
		union ipi_parms r_resp;
	} r_parm;
};


#define XFER_NOTIFY		0x30    /* transfer notification (VME addr) */
#define CMD_EXTENT		0x31    /* command extent (count + block addr ) */



/* Bitdefs for Opcode Modifier in ATTRIBUTES header */

#define	REPT_ATTRIB		0           /* Report (read) an Attribute */
#define	INIT_ATTRIB		1           /* Initialize Attributes */
#define	LOAD_ATTRIB		9           /* Load (write) an Attribute */
#define	SAVE_ATTRIB		0xa         /* Save the attribute in EEPROM */

/* ATTRIBUTES Parameters */

#define	ATTR_VENDOR		0x50    /* vendor specific information */
#define	LOG_BLK_SIZ		0x51    /* logical data block size */
#define	PHYS_BLK_SIZ		0x52    /* physical data block size */
#define	LOG_BLK_LAYOUT		0x53    /* layout of blocks on disk */
#define	PHYS_BLK_LAYOUT		0x54    /* layout of blocks on disk */
#define	LOG_BLOCK_SUP		0x55    /* logical block sizes supported */
#define	PHYS_BLOCK_SUP		0x56    /* physical block sizes supported */
#define	ATTR_FILL_PAT		0x5a
#define	ATTR_PHYSDK		0x5f
#define	ATTR_CTLR_CONF		0x65
#define	ATTR_FAC_ATTACH		0x68
#define	ATTR_REQPARM		0x6c
#define	ATTR_PARMLEN		0x6d
#define	ATTR_SLVCNF_BIT		0x6e
#define	ATTR_SLVCNF_FLD		0x6f
#define	ATTR_PANTHER		0xd0

/* Bitdefs for REQUEST PARM parameter */

#define	RESP_AS_PKT		0x10       /* Send back as a response packet */
#define	RESP_AS_DATA		0x30       /* Send back as data */

/*
 * defs for response type in major status.
 */

#define	RTYP_NRML		0x01    /* normal cmd completion */
#define	RTYP_COMBO		0x03    /* combination cmd completion */
#define	RTYP_ASYNC		0x04    /* asychronous response */

/*
 * Bitdefs for the rp_spec field in the Request Parm parameter
 */

#define	RP_PASD			0x8     /* Naked Parm as Data (yow) */
#define	RP_LEN			0x10    /* Len of data returned (used w RP_PASD) */
#define	RP_PINR			0x20    /* Send back parameter in response pkt */

/*
 * Bitdefs for the Disk Mode parameter
 */
 
#define	IREZERO			0x0     /* Set Heads to initial position */
#define	LOCKCRG			0x1     /* Lock Carriage */
#define	UNLDHEADS   		0x2     /* Unload read/write Heads */
#define	LDHEADS			0x4     /* Load read/write Heads */
#define	ULOCKCART   		0x8     /* Unlock Cartridge */
#define	LOCKCART    		0x10    /* Lock Cartridge */
#define	SPINDOWN    		0x20    /* Spin Down the disk */
#define	SPINUP			0x40    /* Spin Up the disk */
 

struct	ipidevice {
	u_long		dev_bir;	    /* Board id. reg	    */
	u_long		dev_csr;	    /* rw - CSR		    */
	u_long		dev_cmdreg;	    /* w - command register */ 
	u_long		dev_resp;	    /* r - response register */ 
	u_long		dev_vector;	    /* VME interrupt vector */
	u_long		dev_aux[3];	    /* Aux. registers	    */
	struct ipi3pkt	dev_cmd_pkt;	    /* IPI-3 command packet */
	struct ipi3rsp	dev_resp_pkt;	    /* IPI-3 response packet */
};

/* CSR Bit Definitions */

#define	CSR_RESET		(u_long)0x01    /* reset */
#define	CSR_EICRNB		(u_long)0x02    /* enable int on cmd register not busy */
#define	CSR_EIRRV		(u_long)0x04    /* enable int on response register valid */
#define	CSR_CRBUSY		(u_long)0x08    /* Command Register Busy */ 
#define	CSR_RRVLID		(u_long)0x10    /* Response Register Valid */ 
#define	CSR_RESERVE1		(u_long)0x20    /* Spare bit 1 */ 
#define	CSR_RESERVE2		(u_long)0x40    /* Spare bit 1 */ 
#define	CSR_RESERVE3		(u_long)0x80    /* Spare bit 1 */
#define	CSR_ERROR		(u_long)0x100    /* Response Register Valid */ 
#define	CSR_MRINFO		(u_long)0x200    /* Response Register Valid */ 
#define	CSR_LED0		(u_long)0x01000000	/* LED 0 */
#define	CSR_LED1		(u_long)0x02000000
#define	CSR_LED2		(u_long)0x04000000
#define	CSR_LED3		(u_long)0x08000000
#define	CSR_LED4		(u_long)0x10000000
#define	CSR_LED5		(u_long)0x20000000
#define	CSR_LED6		(u_long)0x40000000
#define	CSR_LED7		(u_long)0x80000000	/* LED 7 */

#define	BOARDD(x)	x->c_io->dev_bir
#define	STATUS(x)	x->c_io->dev_csr
#define	CMDREG(x)	x->c_io->dev_cmdreg
#define	RSPREG(x)	x->c_io->dev_resp
#define	INTVEC(x)	x->c_io->dev_vector

#define IP_NOTOPER  0x40000000
#define	IP_NOTREADY 0x10000000

#endif	    _IPIPKT_
