/*      @(#)sndmes.c 1.1 92/07/30 SMI      */
 
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fumount:sndmes.c	1.8"

#include <stdio.h>
#include <sys/types.h>
#include <rfs/message.h>
#include <rfs/rfsys.h>
#include <rfs/nserve.h>
#include <rfs/rfs_misc.h>

/*
 * Send a warning message to a remote system.
 * syntax of message:	fuwarn domain.resource [time] 
 */
sndmes(sysid, time, resrc)
	sysid_t sysid;
	char *time;
	char *resrc;
{
	static char msg[200];

	strcpy(msg, "fuwarn ");
	strncat(msg, resrc, sizeof(msg) - strlen(msg));
	strncat(msg, " ", sizeof(msg) - strlen(msg));
	strncat(msg, time, sizeof(msg) - strlen(msg));
	return(rfsys(RF_SENDUMSG, sysid, msg, strlen(msg)));
}
