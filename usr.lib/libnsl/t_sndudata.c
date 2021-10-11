#ifndef lint
static	char sccsid[] = "%Z%%M% %I% %E% SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_sndudata.c	1.3"
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

extern struct _ti_user *_t_checkfd();
extern int t_errno;
extern int errno;
extern int putmsg();

t_sndudata(fd, unitdata)
int fd;
register struct t_unitdata *unitdata;
{
	register struct T_unitdata_req *udreq;
	char *buf;
	struct strbuf ctlbuf;
	int size;
	register struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL)
		return(-1);

	if (tiptr->ti_servtype != T_CLTS) {
		t_errno = TNOTSUPPORT;
		return(-1);
	}

	if ((int)unitdata->udata.len == 0)
		return(0);

	if ((int)unitdata->udata.len > tiptr->ti_maxpsz) {
		t_errno = TSYSERR;
		errno = ERANGE;
		return(-1);
	}

	buf = tiptr->ti_ctlbuf;
	udreq = (struct T_unitdata_req *)buf;
	udreq->PRIM_type = T_UNITDATA_REQ;
	udreq->DEST_length = unitdata->addr.len;
	udreq->DEST_offset = 0;
	udreq->OPT_length = unitdata->opt.len;
	udreq->OPT_offset = 0;
	size = sizeof(struct T_unitdata_req);

	if (unitdata->addr.len) {
		_t_aligned_copy(buf, unitdata->addr.len, size,
			     unitdata->addr.buf, &udreq->DEST_offset);
		size = udreq->DEST_offset + udreq->DEST_length;
	}
	if (unitdata->opt.len) {
		_t_aligned_copy(buf, unitdata->opt.len, size,
			     unitdata->opt.buf, &udreq->OPT_offset);
		size = udreq->OPT_offset + udreq->OPT_length;
	}

	if (size > tiptr->ti_ctlsize) {
		t_errno = TSYSERR;
		errno = EIO;
		return(-1);
	}
	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = size;
	ctlbuf.buf = buf;

	if (putmsg(fd, &ctlbuf, (unitdata->udata.len? &unitdata->udata: NULL), 0) < 0) {
		if (errno == EAGAIN)
			t_errno = TFLOW;
		else
			t_errno = TSYSERR;
		return(-1);
	}
	return(0);
}
