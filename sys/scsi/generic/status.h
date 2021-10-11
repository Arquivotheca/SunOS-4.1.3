#ident	"@(#)status.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */

#ifndef	_scsi_generic_status_h
#define	_scsi_generic_status_h
/*
 * SCSI status completion block
 *
 * Compliant with SCSI-2, Rev 10b
 * August 22, 1989
 *
 * The SCSI standard specifies one byte of status.
 *
 */

/*
 * The definition of of the Status block as a bitfield
 */
 
struct scsi_status {
	u_char	sts_resvd	: 1,	/* reserved */
		sts_vu7		: 1,	/* vendor unique */
		sts_vu6		: 1,	/* vendor unique */
#define	sts_scsi2	sts_vu6		/* SCSI-2 modifier bit */
		sts_is		: 1,	/* intermediate status sent */
		sts_busy	: 1,	/* device busy or reserved */
		sts_cm		: 1,	/* condition met */
		sts_chk		: 1,	/* check condition */
		sts_vu0		: 1;	/* vendor unique */
};

/*
 * Bit Mask definitions, for use accessing the status as a byte.
 */

#define	STATUS_MASK			0x3E
#define	STATUS_GOOD			0x00
#define	STATUS_CHECK			0x02
#define	STATUS_MET			0x04
#define	STATUS_BUSY			0x08
#define	STATUS_INTERMEDIATE		0x10
#define	STATUS_SCSI2			0x20

#define	STATUS_INTERMEDIATE_MET		(STATUS_INTERMEDIATE|STATUS_MET)
#define	STATUS_RESERVATION_CONFLICT	(STATUS_INTERMEDIATE|STATUS_BUSY)
#define	STATUS_TERMINATED		(STATUS_SCSI2|STATUS_CHECK)
#define	STATUS_QFULL			(STATUS_SCSI2|STATUS_BUSY)

/*
 * Some deviations from the one byte of Status are known. Each
 * implementation will define them specifically
 */

#include	<scsi/impl/status.h>

#endif	/* !_scsi_generic_status_h */
