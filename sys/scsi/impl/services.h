#ident	"@(#)services.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
 */

#ifndef	_scsi_impl_services_h
#define	_scsi_impl_services_h
/*
 * Implementation services not classified by type
 */

#ifdef	KERNEL
extern	int	scsi_poll();
extern	void	scsi_pollintr();

extern struct scsi_pkt *get_pktiopb();
extern void free_pktiopb();

extern char *scsi_dname(), *scsi_rname(), *scsi_cmd_decode(), *scsi_mname();
extern char *sprintf();

extern char *state_bits, *sense_keys[NUM_SENSE_KEYS + NUM_IMPL_SENSE_KEYS];
/*
 * Common Capability Strings Array
 */
extern char *scsi_capstrings[];

#define	SCSI_CAP_DMA_MAX	0
#define	SCSI_CAP_MSG_OUT	1
#define	SCSI_CAP_DISCONNECT	2
#define	SCSI_CAP_SYNCHRONOUS	3
#define	SCSI_CAP_WIDE_XFER	4
#define	SCSI_CAP_PARITY		5
#define	SCSI_CAP_INITIATOR_ID	6
#define	SCSI_CAP_UNTAGGED_QING	7
#define	SCSI_CAP_TAGGED_QING	8

#endif
#endif	_scsi_impl_services_h
