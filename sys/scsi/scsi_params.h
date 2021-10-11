/*	@(#)scsi_params.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988, 1989 Sun Microsystems, Inc.
 */

#ifndef	_scsi_scsi_params_h
#define	_scsi_scsi_params_h

/*
 * General SCSI parameters
 *
 * What level of SCSI supported, etc.
 */


#define	NTARGETS		8	/* total # of targets per SCSI bus */
#define	NLUNS_PER_TARGET	8	/* number of luns per target */

#define	NUM_SENSE_KEYS		16	/* total number of Sense keys */

#endif	_scsi_scsi_params_h
