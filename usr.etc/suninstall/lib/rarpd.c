#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)rarpd.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)rarpd.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		rarpd()
 *
 *	Description:	Start the rarp daemon for all interfaces which the
 *      		system can find, if necessary.  
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"

extern	char *		sprintf();


void
rarpd(sys_p)
	sys_info *	sys_p;
{

	if (sys_p->ether_type == ETHER_NONE)
		return;
	else
		(void) x_system("rarpd -a");

} /* end rarpd() */
