#ifndef lint
static	char sccsid[] = "@(#)_dummy.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/_dummy.c	1.2"
*/

#include <sys/errno.h>
#include <nettli/tiuser.h>

extern int t_errno;
extern int errno;

_dummy()
{
	t_errno = TSYSERR;
	errno = ENXIO;
	return(-1);
}
