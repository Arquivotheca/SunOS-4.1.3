#ident	"@(#)scsi_capabilities.c 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */

/*
 *
 * Generic Capabilities Routines
 *
 */

#include <scsi/scsi.h>


int
scsi_ifgetcap(ap, cap, whom)
struct scsi_address *ap;
char *cap;
{
	register struct scsi_transport *tranp;

	tranp = (struct scsi_transport *) ap->a_cookie;

	return (*tranp->tran_getcap)(ap, cap, whom);
}

int
scsi_ifsetcap(ap, cap, value, whom)
struct scsi_address *ap;
char *cap;
int value, whom;
{
	register struct scsi_transport *tranp;

	tranp = (struct scsi_transport *) ap->a_cookie;

	return (*tranp->tran_setcap)(ap, cap, value, whom);
}
