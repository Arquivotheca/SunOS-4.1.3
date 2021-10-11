#ifndef lint
static	char		mls_sccsid[] = "@(#)golabeld.c 1.1 92/07/30 SMI; SunOS MLS";
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		golabeld()
 *
 *	Description:	Start up the golabel daemon.
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"




void
golabeld(sys_p)
	sys_info *	sys_p;
{
	if (sys_p->ether_type == ETHER_NONE)
		return;

	x_system("/usr/etc/rpc.golabeld -i");
} /* end golabeld() */
