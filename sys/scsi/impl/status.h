/*	@(#)status.h 1.1 92/07/30 SMI	*/

#ifndef	_scsi_impl_status_h
#define	_scsi_impl_status_h


/*
 * Copyright (c) 1988, 1989 Sun Microsystems, Inc.
 */


/*
 * Implementation specific SCSI status definitions
 *
 */

/*
 * The size of a status block (much more than is really needed...)
 */

#define	STATUS_SIZE	3

/*
 * This structure is in violation of the SCSI spec, but someone
 * has claimed to need it.
 */


struct	scsi_funky_scb {		/* funky scsi status  block */
	u_char	sts_resvd	: 1,	/* reserved */
#define	sts_ext_st1	sts_resvd	/* extended status (next byte valid) */
		sts_vu7		: 1,	/* vendor unique */
		sts_vu6		: 1,	/* vendor unique */
		sts_is		: 1,	/* intermediate status sent */
		sts_busy	: 1,	/* device busy or reserved */
		sts_cm		: 1,	/* condition met */
		sts_chk		: 1,	/* check condition */
		sts_vu0		: 1;	/* vendor unique */
	u_char	sts_ext_st2	: 1,	/* extended status (next byte valid) */
				: 6,	/* reserved */
		sts_ha_err	: 1;	/* host adapter detected error */
	u_char	sts_byte2;		/* third byte */
};



#endif	_scsi_impl_status_h
