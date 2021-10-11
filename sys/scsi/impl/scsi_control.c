#ifndef lint
static  char sccsid[] = "@(#)scsi_control.c 1.1 92/07/30 SMI";
#endif	lint
/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 */


/****************************************************************
 *								*
 *	Generic Abort and Reset Routines			*
 *								*
 ****************************************************************/

#include <scsi/scsi.h>


int
scsi_abort(ap,pkt)
struct scsi_address *ap;
struct scsi_pkt *pkt;
{
	struct scsi_transport *tranp;

	tranp = (struct scsi_transport *) ap->a_cookie;

	return (*tranp->tran_abort)(ap,pkt);
}

int
scsi_reset(ap,level)
struct scsi_address *ap;
int level;
{
	struct scsi_transport *tranp;

	tranp = (struct scsi_transport *) ap->a_cookie;

	return (*tranp->tran_reset)(ap,level);
}
