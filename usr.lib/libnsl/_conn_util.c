#ifndef lint
static	char sccsid[] = "@(#)_conn_util.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/_conn_util.c	1.6"
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
extern int t_errno;
extern int errno;
extern int getmsg(), putmsg();

/*
 * Snd_conn_req - send connect request message to 
 * transport provider
 */
_snd_conn_req(fd, call)
int fd;
register struct t_call *call;
{
	register struct T_conn_req *creq;
	struct strbuf ctlbuf;
	char *buf;
	int size;
	register struct _ti_user *tiptr;
	int savemask;
	

	tiptr = &_ti_user[fd];

	if (tiptr->ti_servtype == T_CLTS) {
		t_errno = TNOTSUPPORT;
		return(-1);
	}

	savemask = sigblock(sigmask(SIGPOLL));
	if (_t_is_event(fd, tiptr)) {
		sigsetmask(savemask);
		return(-1);
	}


	buf = tiptr->ti_ctlbuf;
	creq = (struct T_conn_req *)buf;
	creq->PRIM_type = T_CONN_REQ;
	creq->DEST_length = call->addr.len;
	creq->DEST_offset = 0;
	creq->OPT_length = call->opt.len;
	creq->OPT_offset = 0;
	size = sizeof(struct T_conn_req);

	if (call->addr.len) {
		_t_aligned_copy(buf, call->addr.len, size,
			     call->addr.buf, &creq->DEST_offset);
		size = creq->DEST_offset + creq->DEST_length;
	}
	if (call->opt.len) {
		_t_aligned_copy(buf, call->opt.len, size,
			     call->opt.buf, &creq->OPT_offset);
		size = creq->OPT_offset + creq->OPT_length;
	}

	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = size;
	ctlbuf.buf = buf;

	if (putmsg(fd, &ctlbuf, (call->udata.len? &call->udata: NULL), 0) < 0) {
		t_errno = TSYSERR;
		sigsetmask(savemask);
		return(-1);
	}

	if (!_t_is_ok(fd, tiptr, T_CONN_REQ)) {
		sigsetmask(savemask);
		return(-1);
	}

	sigsetmask(savemask);
	return(0);
}



/*
 * Rcv_conn_con - get connection confirmation off
 * of read queue
 */
_rcv_conn_con(fd, call)
int fd;
register struct t_call *call;
{
	struct strbuf ctlbuf;
	struct strbuf rcvbuf;
	int flg = 0;
	register union T_primitives *pptr;
	int retval;
	register struct _ti_user *tiptr;


	tiptr = &_ti_user[fd];

	if (tiptr->ti_servtype == T_CLTS) {
		t_errno = TNOTSUPPORT;
		return(-1);
	}

	/*
         * see if there is something in look buffer
	 */
	if (tiptr->ti_lookflg) {
		t_errno = TLOOK;
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
	 * did we get entire message 
	 */
	if (retval) {
		t_errno = TSYSERR;
		errno = EIO;
		return(-1);
	}

	/*
	 * is cntl part large enough to determine message type?
	 */
	if (ctlbuf.len < sizeof(long)) {
		t_errno = TSYSERR;
		errno = EPROTO;
		return(-1);
	}

	pptr = (union T_primitives *)ctlbuf.buf;

	switch(pptr->type) {

		case T_CONN_CON:

			if ((ctlbuf.len < sizeof(struct T_conn_con)) ||
			    (ctlbuf.len < (pptr->conn_con.OPT_length +
			     pptr->conn_con.OPT_offset))) {
				t_errno = TSYSERR;
				errno = EPROTO;
				return(-1);
			}

			if (call != NULL) {
				if ((rcvbuf.len > (int)call->udata.maxlen) ||
				    (pptr->conn_con.RES_length > call->addr.maxlen) ||
				    (pptr->conn_con.OPT_length > call->opt.maxlen)) {
					t_errno = TBUFOVFLW;
					return(-1);
				}
				memcpy(call->addr.buf, ctlbuf.buf +
					pptr->conn_con.RES_offset,
					(int)pptr->conn_con.RES_length);
				call->addr.len = pptr->conn_con.RES_length;
				memcpy(call->opt.buf, ctlbuf.buf +
					pptr->conn_con.OPT_offset,
					(int)pptr->conn_con.OPT_length);
				call->opt.len = pptr->conn_con.OPT_length;
				memcpy(call->udata.buf, rcvbuf.buf, (int)rcvbuf.len);
				call->udata.len = rcvbuf.len;
				/*
				 * since a confirmation seq number
				 * is -1 by default
				 */
				call->sequence = -1;
			}

			return(0);

		case T_DISCON_IND:

			/*
			 * if disconnect indication then put it in look buf
			 */
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
