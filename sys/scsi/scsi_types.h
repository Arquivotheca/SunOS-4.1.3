#ident "@(#)scsi_types.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */

#ifndef	_scsi_scsi_types_h
#define	_scsi_scsi_types_h

/*
 * Types for SCSI subsystems.
 *
 * This file picks up specific as well as generic type
 * defines, and also serves as a wrapper for many common
 * includes.
 */

#include	<sys/types.h>
typedef	char *	opaque_t;


/*
 * Each implementation will have it's own specific set
 * of types it wishes to define.
 *
 */


/*
 * Generally useful files to include
 */

#include	<scsi/scsi_params.h>
#include	<scsi/scsi_address.h>
#include	<scsi/scsi_ctl.h>
#include	<scsi/scsi_pkt.h>
#include	<scsi/scsi_resource.h>

#include	<scsi/generic/commands.h>
#include	<scsi/generic/status.h>
#include	<scsi/generic/message.h>
#include	<scsi/generic/mode.h>


/*
 * Sun SCSI type definitions
 */

#include	<scsi/impl/types.h>


#endif	/* !_scsi_scsi_types_h */
