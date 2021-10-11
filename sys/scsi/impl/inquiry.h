#ident	"@(#)inquiry.h	1.1"
/* @(#)inquiry.h 1.1 92/07/30 SMI */
/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 */

#ifndef	_scsi_impl_inquiry_h
#define	_scsi_impl_inquiry_h

/*
 * Implementation inquiry data that is not within
 * the scope of any released SCSI standard.
 */

/*
 * Minimum inquiry data length (includes up through RDF field)
 */

#define	SUN_MIN_INQLEN	4

/*
 * Inquiry data size definition
 */
#define	SUN_INQSIZE	(sizeof (struct scsi_inquiry))

#endif	_scsi_impl_inquiry_h
