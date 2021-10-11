#ident	"@(#)transport.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
 */

#ifndef	_scsi_impl_transport_h
#define	_scsi_impl_transport_h

#include	<scsi/scsi_types.h>

/*
 * Attribution:	Greg Slaughter
 */

/*
 * SCSI transport structures
 *
 *	As each Host Adapter makes itself known to the system,
 *	it will create and register with the library the structure
 *	described below. This is so that the library knows how to route
 *	packets, resource control requests, and capability requests
 *	for any particular host adapter. The 'a_cookie' field of a
 *	scsi_address structure made known to a Target driver will
 *	point to one of these transport structures.
 *
 * The functional interfaces defined below follow, and are expected
 * to follow, the Sun SCSI specification. They are the implementation.
 *
 */


struct scsi_transport {
	int	tran_spl;			/* 'splx' interrupt mask */
	int	(*tran_start)();		/* transport start */
	int	(*tran_reset)();		/* transport reset */
	int	(*tran_abort)();		/* transport abort */
	int	(*tran_getcap)();		/* capability retrieval */
	int	(*tran_setcap)();		/* capability establishment */
	struct scsi_pkt	*(*tran_pktalloc)();	/* packet allocation */
	struct scsi_pkt *(*tran_dmaget)();	/* dma allocation */
	void	(*tran_pktfree)();		/* packet deallocation */
	void	(*tran_dmafree)();		/* dma deallocation */
};

/*
 * This implementation provides some 'standard' allocation and
 * deallocation functions. Host Adapters may provide their own
 * functions, but if they use the standard functions, they
 * must use all of them.
 */

#ifdef	KERNEL
extern struct scsi_pkt *scsi_std_pktalloc();
extern struct scsi_pkt *scsi_std_dmaget();
extern void scsi_std_pktfree();
extern void scsi_std_dmafree();
extern void scsi_rinit();
extern int scsi_cookie();
#endif	KERNEL

#endif	_scsi_impl_transport_h
