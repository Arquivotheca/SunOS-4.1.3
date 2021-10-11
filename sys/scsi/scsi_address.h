/*	@(#)scsi_address.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988, 1989 Sun Microsystems, Inc.
 */

#ifndef	_scsi_scsi_address_h
#define	_scsi_scsi_address_h

#include	<scsi/scsi_types.h>

/*
 * SCSI address definition.
 *
 *	A SCSI command is sent to a specific lun (sometimes a specific
 *	sub-lun) on a specific target on a specific SCSI bus.
 *
 *	The structure here defines this addressing scheme.
 */

struct scsi_address {
	int	a_cookie;	/* Cookie to identify which SCSI bus.. */
	u_short	a_target;	/* ..and which Target on that SCSI bus */
	u_char	a_lun;		/* ..and which Lun on that Target */
	u_char	a_sublun;	/* ..and which Sublun on that Lun */
};

#endif	_scsi_scsi_address_h
