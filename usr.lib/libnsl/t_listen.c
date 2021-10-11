#ifndef lint
static	char sccsid[] = "@(#)t_listen.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_listen.c	1.4"
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


extern int t_errno;
extern int errno;
extern struct _ti_user *_t_checkfd();
extern int getmsg();


t_listen(fd, call)
int fd;
struct t_call *call;
{
	struct strbuf ctlbuf;
	struct strbuf rcvbuf;
	int flg = 0;
	int retval;
	register union T_primitives *pptr;
	register struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL)
		return(-1);

	/*
         * check if something in look buffer
	 */
	if (tiptr->ti_lookflg) {
		t_errno = TLOOK;
		return(-1);
	}

	if (tiptr->ti_servtype == T_CLTS) {
		t_errno = TNOTSUPPORT;
		return(-1);
	}

	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = tiptr->ti_ctlbuf;
	rcvbuf.maxlen = tiptr->ti_rcvsize;
	rcvbuf.len = 0;
	rcvbuf.buf = tiptr->ti_rcvbuf;

	if ((retval = getmsg(fd, &ctlbuf, &rcvbuf, &flg)) < 0) {
		if (errno == EAGAIN)
			t_errno = TNODATA;
		else
			t_errno = TSYSERR;
		return(-1);
	}
	if (rcvbuf.len == -1) rcvbuf.len = 0;

	/*
	 * did I get entire message?
	 */
	if (retval) {
		t_errno = TSYSERR;
		errno = EIO;
		return(-1);
	}

	/*
	 * is ctl part large enough to determine type
	 */
	if (ctlbuf.len < sizeof(long)) {
		t_errno = TSYSERR;
		errno = EPROTO;
		return(-1);
	}

	pptr = (union T_primitives *)ctlbuf.buf;

	switch(pptr->type) {

		case T_CONN_IND:
			if ((ctlbuf.len < sizeof(struct T_conn_ind)) ||
			    (ctlbuf.len < (pptr->conn_ind.OPT_length
			    + pptr->conn_ind.OPT_offset))) {
				t_errno = TSYSERR;
				errno = EPROTO;
				return(-1);
			}
			if ((rcvbuf.len > (int)call->udata.maxlen) ||
			    (pptr->conn_ind.SRC_length > call->addr.maxlen) ||
			    (pptr->conn_ind.OPT_length > call->opt.maxlen)) {
				t_errno = TBUFOVFLW;
				return(-1);
			}

			memcpy(call->addr.buf, ctlbuf.buf +
				pptr->conn_ind.SRC_offset,
				(int)pptr->conn_ind.SRC_length);
			call->addr.len = pptr->conn_ind.SRC_length;
			memcpy(call->opt.buf, ctlbuf.buf +
				pptr->conn_ind.OPT_offset,
				(int)pptr->conn_ind.OPT_length);
			call->opt.len = pptr->conn_ind.OPT_length;
			memcpy(call->udata.buf, rcvbuf.buf, (int)rcvbuf.len);
			call->udata.len = rcvbuf.len;
			call->sequence = pptr->conn_ind.SEQ_number;
			return(0);

		case T_DISCON_IND:
			_t_putback(tiptr, rcvbuf.buf, rcvbuf.len, ctlbuf.buf, ctlbuf.len);
			t_errno = TLOOK;
			return(-1);

		default:
			break;
	}

	t_errno = TSYSERR;
	errno = EPROTO;
	return(-1);
}
