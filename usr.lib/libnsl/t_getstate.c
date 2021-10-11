#ifndef lint
static	char sccsid[] = "@(#)t_getstate.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_getstate.c	1.4"
*/
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <nettli/tihdr.h>
#include <nettli/timod.h>
#include <nettli/tiuser.h>
#include <stdio.h>
#include <sys/signal.h>


extern int t_errno;
extern int errno;

t_getstate(fd)
int fd;
{
	struct T_info_ack info;
	int retlen;
	int savemask;

	if (_t_checkfd(fd) == NULL)
		return(-1);

	savemask = sigblock(sigmask(SIGPOLL));

	info.PRIM_type = T_INFO_REQ;

	if (!_t_do_ioctl(fd, (caddr_t)&info, sizeof(struct T_info_req), TI_GETINFO, &retlen)) {
		sigsetmask(savemask);
		return(-1);
	}
	sigsetmask(savemask);
		
	if (retlen != sizeof(struct T_info_ack)) {
		t_errno = TSYSERR;
		errno = EIO;
		return(-1);
	}

	switch (info.CURRENT_state) {

	case TS_UNBND:
		return(T_UNBND);
	case TS_IDLE:
		return(T_IDLE);
	case TS_WRES_CIND:
		return(T_INCON);
	case TS_WCON_CREQ:
		return(T_OUTCON);
	case TS_DATA_XFER:
		return(T_DATAXFER);
	case TS_WIND_ORDREL:
		return(T_OUTREL);
	case TS_WREQ_ORDREL:
		return(T_INREL);
	default:
		t_errno = TSTATECHNG;
		return(-1);
	}
}
