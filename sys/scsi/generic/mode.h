#ident	"@(#)mode.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1989, 1990 by Sun Microsystems Inc.
 */

#ifndef	_scsi_generic_mode_h
#define	_scsi_generic_mode_h

/*
 *
 * Defines and Structures for SCSI Mode Sense/Select data - generic
 *
 */

/*
 * Structures and defines common for all device types
 */

/*
 * Mode Sense/Select Header.
 *
 * Mode Sense/Select data consists of a header, followed by zero or more
 * block descriptors, followed by zero or more mode pages.
 *
 */

struct mode_header {
	u_char length;		/* number of bytes following */
	u_char medium_type;	/* device specific */
	u_char device_specific;	/* device specfic parameters */
	u_char bdesc_length;	/* length of block descriptor(s), if any */
};

#define	MODE_HEADER_LENGTH	(sizeof (struct mode_header))

/*
 * Block Descriptor. Zero, one, or more may normally follow the mode header.
 *
 * The density code is device specific.
 *
 * The 24-bit value described by blks_{hi, mid, lo} describes the number of
 * blocks which this block descriptor applies to. A value of zero means
 * 'the rest of the blocks on the device'.
 *
 * The 24-bit value described by blksize_{hi, mid, lo} describes the blocksize
 * (in bytes) applicable for this block descriptor. For Sequential Access
 * devices, if this value is zero, the block size will be derived from
 * the transfer length in I/O operations.
 *
 */

struct block_descriptor {
	u_char density_code;	/* device specific */
	u_char blks_hi;		/* hi  */
	u_char blks_mid;	/* mid */
	u_char blks_lo;		/* low */
	u_char reserved;	/* reserved */
	u_char blksize_hi;	/* hi  */
	u_char blksize_mid;	/* mid */
	u_char blksize_lo;	/* low */
};

/*
 * Define a macro to take an address of a mode header to the address
 * of the nth (0..n) block_descriptor, or NULL if there either aren't any
 * block descriptors or the nth block descriptor doesn't exist.
 */

#define	BLOCK_DESCRIPTOR_ADDR(mhdr, bdnum) \
	((mhdr)->bdesc_length && ((unsigned)(bdnum)) < \
	((mhdr)->bdesc_length/(sizeof (struct block_descriptor)))) ? \
	((struct block_descriptor *)(((u_long)(mhdr))+MODE_HEADER_LENGTH+ \
	((bdnum) * sizeof (struct block_descriptor)))) : \
	((struct block_descriptor *) 0)

/*
 * Mode page header. Zero or more Mode Pages follow either the block
 * descriptors (if any), or the Mode Header.
 *
 * The 'ps' bit must be zero for mode select operations.
 *
 */

struct mode_page {
	u_char	ps	:1,	/* 'Parameter Saveable' bit */
			:1,	/* reserved */
		code	:6;	/* page code number */
	u_char	length;		/* length of bytes to follow */
	/*
	 * Mode Page specific data follows right after this...
	 */
};

/*
 * Define a macro to retrieve the first mode page. Could be more
 * general (for multiple mode pages).
 */

#define	MODE_PAGE_ADDR(mhdr, type)	\
	((type *)(((u_long)(mhdr))+MODE_HEADER_LENGTH+(mhdr)->bdesc_length))

/*
 * Page codes, as defined in the proposed SCSI-2 specification,
 * Revision 10c, 3/9/90.
 *
 * Page codes follow the following backwards compatible specification:
 *
 *	Code Value(s)		What
 *	----------------------------------------------------------------------
 *	0x00			Vendor Unique (does not require page format)
 *
 *	0x02, 0x09, 0x0A	pages for all Device Types
 *
 *	0x01, 0x03-0x08,	pages for specific Device Type
 *	0x0B-0x1F
 *
 *	0x20-0x3E		Vendor Unique (requires page format)
 *
 *	0x3F			Return all pages (valid for Mode Sense only)
 *
 */

/*
 * Page codes and page length values (all device types)
 */

#define	MODEPAGE_DISCO_RECO	0x02
#define	MODEPAGE_PDEVICE	0x09
#define	MODEPAGE_CTRL_MODE	0x0A

#define	MODEPAGE_ALLPAGES	0x3F

/*
 * Mode Select/Sense page structures (for all device types)
 */

/*
 * Disconnect/Reconnect Page
 */

struct mode_disco_reco {
	struct	mode_page mode_page;	/* common mode page header */
	u_char	buffer_full_ratio;	/* write, how full before reconnect? */
	u_char	buffer_empty_ratio;	/* read, how full before reconnect? */
	u_short	bus_inactivity_limit;	/* how much bus quiet time for BSY- */
	u_short	disconect_time_limit;	/* min to remain disconnected */
	u_short	connect_time_limit;	/* min to remain connected */
	u_short	max_burst_size;		/* max data burst size */
	u_char			: 6,
			dtdc	: 2;	/* data transfer disconenct control */
	u_char	reserved[3];
};

#define	DTDC_DATADONE	0x01		/*
					 * Target may not disconnect once
					 * data transfer is started until
					 * all data successfully transferred.
					 */

#define	DTDC_CMDDONE	0x03		/*
					 * Target may not disconnect once
					 * data transfer is started until
					 * command completed.
					 */

/*
 * Peripheral Device Page
 */

struct mode_pdevice {
	struct	mode_page mode_page;	/* common mode page header */
	u_short	if_ident;		/* interface identifier */
	u_char	reserved[4];		/* reserved */
	u_char	vendor_uniqe[1];	/* vendor unique data */
};

#define	PDEV_SCSI	0x0000		/* scsi interface */
#define	PDEV_SMD	0x0001		/* SMD interface */
#define	PDEV_ESDI	0x0002		/* ESDI interface */
#define	PDEV_IPI2	0x0003		/* IPI-2 interface */
#define	PDEV_IPI3	0x0004		/* IPI-3 interface */

/*
 * Control Mode Page
 */

struct mode_control {
	struct	mode_page mode_page;	/* common mode page header */
	u_char			: 7,
			rlec	: 1;	/* Report Log Exception bit */
	u_char		que_mod	: 4,	/* Queue algorithm modifier */
				: 2,
			que_err	: 1,	/* Queue error */
			qdisable: 1;	/* Queue disable */
	u_char		eeca	: 1,
				: 4,
			raenp	: 1,
			uaaenp	: 1,
			eanp	: 1;
	u_char	reserved;
	u_short	ready_aen_holdoff;
};

#define	CTRL_QMOD_RESTRICT	0x0
#define	CTRL_QMOD_UNRESTRICT	0x1


/*
 * Include known generic device specific mode definitions and structures
 */

#include <scsi/generic/dad_mode.h>

/*
 * Include implementation specific mode information
 */

#include <scsi/impl/mode.h>

#endif	/* !_scsi_generic_mode_h */
