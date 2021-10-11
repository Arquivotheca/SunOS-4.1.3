#ifndef lint
static	char sccsid[] = "@(#)t_optmgmt.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_optmgmt.c	1.3"
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


t_optmgmt(fd, req, ret)
int fd;
struct t_optmgmt *req;
struct t_optmgmt *ret;
{
	int size;
	register char *buf;
	register struct T_optmgmt_req *optreq;
	register struct _ti_user *tiptr;
	int savemask;


	if ((tiptr = _t_checkfd(fd)) == NULL)
		return(-1);

	savemask = sigblock(sigmask(SIGPOLL));

	buf = tiptr->ti_ctlbuf;
	optreq = (struct T_optmgmt_req *)buf;
	optreq->PRIM_type = T_OPTMGMT_REQ;
	optreq->OPT_length = req->opt.len;
	optreq->OPT_offset = 0;
	optreq->MGMT_flags = req->flags;
	size = sizeof(struct T_optmgmt_req);

	if (req->opt.len) {
		_t_aligned_copy(buf, req->opt.len, size,
			     req->opt.buf, &optreq->OPT_offset);
		size = optreq->OPT_offset + optreq->OPT_length;
	}

	if (!_t_do_ioctl(fd, buf, size, TI_OPTMGMT, 0)) {
		sigsetmask(savemask);
		return(-1);
	}
	sigsetmask(savemask);


	if (optreq->OPT_length > ret->opt.maxlen) {
		t_errno = TBUFOVFLW;
		return(-1);
	}

	memcpy(ret->opt.buf, (char *) (buf + optreq->OPT_offset),
	       (int)optreq->OPT_length);
	ret->opt.len = optreq->OPT_length;
	ret->flags = optreq->MGMT_flags;

	return(0);
}

