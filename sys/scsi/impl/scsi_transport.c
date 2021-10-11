#ifndef lint
static  char sccsid[] = "@(#)scsi_transport.c 1.1 92/07/30 SMI";
#endif	lint
/*
 * Copyright (C) 1989 Sun Microsystems, Inc.
 */


/****************************************************************
 *								*
 *	Main Transport Routines					*
 *								*
 ****************************************************************/

#include <scsi/scsi.h>

int
pkt_transport(pkt)
struct scsi_pkt *pkt;
{
	struct scsi_transport *tranp;

	tranp = (struct scsi_transport *) pkt->pkt_address.a_cookie;
	return (*tranp->tran_start)((struct scsi_cmd *)pkt);
}
