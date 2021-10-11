#ident	"@(#)scsi_resource.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */
#ifndef	_scsi_scsi_resource_h
#define	_scsi_scsi_resource_h

#include	<scsi/scsi_types.h>

/*
 * SCSI Resource Function Declaratoins
 */

/*
 * Defines for stating preferences in resource allocation
 */

#define	NULL_FUNC	((int (*)())0)
#define	SLEEP_FUNC	((int (*)())1)

#ifdef	KERNEL
/*
 * Kernel function declarations
 */

extern struct scsi_pkt *scsi_resalloc();
extern struct scsi_pkt *scsi_pktalloc();
extern struct scsi_pkt *scsi_dmaget();
extern void scsi_resfree();
extern void scsi_dmafree();

/*
 * Preliminary version of the SCSA specification
 * mentioned a routine called scsi_pktfree, which
 * turned out to be semantically equivialent to
 * scsi_resfree.
 */

#define	scsi_pktfree	scsi_resfree

#endif	KERNEL
#endif	_scsi_scsi_resource_h
