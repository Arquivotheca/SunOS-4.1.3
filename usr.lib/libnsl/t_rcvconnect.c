#ifndef lint
static	char sccsid[] = "@(#)t_rcvconnect.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_rcvconnect.c	1.2"
*/
#include <nettli/tiuser.h>
#include <sys/param.h>

extern rcv_conn_con();


t_rcvconnect(fd, call)
int fd;
struct t_call *call;
{

	if (_t_checkfd(fd) == NULL)
		return(-1);

	return(_rcv_conn_con(fd, call));
}
