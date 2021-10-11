/*	@(#)scsi_ctl.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988, 1989 Sun Microsystems, Inc.
 */

#ifndef	_scsi_scsi_ctl_h
#define	_scsi_scsi_ctl_h

#include	<scsi/scsi_types.h>

/*
 *	SCSI Control Information
 *
 * Defines for stating level of reset.
 */

#define	RESET_ALL	0	/* reset SCSI bus, h/a, everything */
#define	RESET_TARGET	1	/* reset SCSI target */

#ifdef	KERNEL
/*
 * Kernel function declarations
 */

/*
 * Capabilities functions
 */

extern int scsi_ifgetcap(), scsi_ifsetcap();

/*
 * Abort and Reset functions
 */

extern int scsi_abort(), scsi_reset();

#endif	KERNEL
#endif	_scsi_scsi_ctl_h
