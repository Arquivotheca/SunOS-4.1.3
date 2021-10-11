#ifndef lint
static	char sccsid[] = "@(#)t_sync.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_sync.c	1.4"
*/
#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <nettli/tihdr.h>
#include <nettli/timod.h>
#include <nettli/tiuser.h>
#include <sys/signal.h>


extern struct _ti_user *_ti_user;
extern struct _ti_user _null_ti;
extern int t_errno;
extern int errno;
extern struct _ti_user *_t_checkfd();
extern void free();
extern char *calloc();
extern int ioctl();
extern int ulimit();


t_sync(fd)
int fd;
{
	int retval;
	struct T_info_ack info;
	register struct _ti_user *tiptr;
	int retlen;
	int savemask;

	/*
         * if needed allocate the ti_user structures
	 * for all file desc.
	 */
	 if (!_ti_user) 
		if ((_ti_user = (struct _ti_user *)calloc(1, (unsigned)(OPENFILES*sizeof(struct _ti_user)))) == NULL) {
			t_errno = TSYSERR;
			return(-1);
		}

	savemask = sigblock(sigmask(SIGPOLL));

	info.PRIM_type = T_INFO_REQ;


	if ((retval = ioctl(fd, I_FIND, "timod")) < 0) {
		sigsetmask(savemask);
		t_errno = TSYSERR;
		return(-1);
	}

	if (!retval) {
		sigsetmask(savemask);
		t_errno = TBADF;
		return(-1);
	}
	if (!_t_do_ioctl(fd, (caddr_t)&info, sizeof(struct T_info_req), TI_GETINFO, &retlen) < 0) {
		sigsetmask(savemask);
		return(-1);
	}
	sigsetmask(savemask);
			
	if (retlen != sizeof(struct T_info_ack)) {
		errno = EIO;
		t_errno = TSYSERR;
		return(-1);
	}

	/* 
	 * Range of file desc. is OK, the ioctl above was successful!
	 */

	if ((tiptr = _t_checkfd(fd)) == NULL) {

		tiptr = &_ti_user[fd];

		if (_t_alloc_bufs(fd, tiptr, info) < 0) {
			*tiptr = _null_ti;
			t_errno = TSYSERR;
			return(-1);
		}

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
		return(T_INREL);
	case TS_WREQ_ORDREL:
		return(T_OUTREL);
	default:
		t_errno = TSTATECHNG;
		return(-1);
	}
}
