#ifndef lint
static	char sccsid[] = "@(#)t_unbind.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_unbind.c	1.3"
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



extern int t_errno;
extern int errno;
extern struct _ti_user *_t_checkfd();
extern int ioctl();


t_unbind(fd)
int fd;
{
	register struct _ti_user *tiptr;
	struct T_unbind_req *unbind_req;
	int savemask;

	if ((tiptr = _t_checkfd(fd)) == NULL)
		return(-1);


	savemask = sigblock(sigmask(SIGPOLL));
	if (_t_is_event(fd, tiptr)) {
		sigsetmask(savemask);
		return(-1);
	}

	unbind_req = (struct T_unbind_req *)tiptr->ti_ctlbuf;
	unbind_req->PRIM_type = T_UNBIND_REQ;

	if (!_t_do_ioctl(fd, (caddr_t)unbind_req, sizeof(struct T_unbind_req), TI_UNBIND, 0)) {
		sigsetmask(savemask);
		return(-1);
	}
	sigsetmask(savemask);

	if (ioctl(fd, I_FLUSH, FLUSHRW) < 0) {
		t_errno = TSYSERR;
		return(-1);
	}

	/*
	 * clear more data in TSDU bit
	 */
	tiptr->ti_flags &= ~MORE;

	return(0);
}
