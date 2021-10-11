/*
 * @(#)uscsi.h 1.1 92/07/30 Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *
 * Defines for user SCSI commands					*
 *
 */

#ifndef _scsi_impl_uscsi_h
#define	_scsi_impl_uscsi_h

/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 * definition for user-scsi command structure
 */
struct uscsi_cmd {
	caddr_t	uscsi_cdb;
	int	uscsi_cdblen;
	caddr_t	uscsi_bufaddr;
	int	uscsi_buflen;
	unsigned char	uscsi_status;
	int	uscsi_flags;
};

/*
 * flags for uscsi_flags field
 */
#define	USCSI_SILENT	0x01	/* no error messages */
#define	USCSI_DIAGNOSE	0x02	/* fail if any error occurs */
#define	USCSI_ISOLATE	0x04	/* isolate from normal commands */
#define	USCSI_READ	0x08	/* get data from device */
#define	USCSI_WRITE	0xFFF7	/* use to zero the READ bit in uscsi_flags */

/*
 * User SCSI io control command
 */
#define	USCSICMD	_IOWR(u, 1, struct uscsi_cmd) /* user scsi command */

/*
 * user scsi status bit masks
 */

#define	USCSI_STATUS_GOOD			0x00
#define	USCSI_STATUS_CHECK			0x02
#define	USCSI_STATUS_MET			0x04
#define	USCSI_STATUS_BUSY			0x08
#define	USCSI_STATUS_INTERMEDIATE		0x10
#define	USCSI_STATUS_RESERVATION_CONFLICT	\
	(USCSI_STATUS_INTERMEDIATE | USCSI_STATUS_BUSY)

#endif _uscsi_impl_uscsi_h
