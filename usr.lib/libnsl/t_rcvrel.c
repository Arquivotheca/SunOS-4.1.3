#ifndef lint
static	char sccsid[] = "@(#)t_rcvrel.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_rcvrel.c	1.7"
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
extern int getmsg();


t_rcvrel(fd)
int fd;
{
	struct strbuf ctlbuf;
	struct strbuf databuf;
	int retval;
	int flg = 0;
	union T_primitives *pptr;
	struct _ti_user *tiptr;
	int savemask;


	if ((tiptr = _t_checkfd(fd)) == 0)
		return(-1);

	if (tiptr->ti_servtype != T_COTS_ORD) {
		t_errno = TNOTSUPPORT;
		return(-1);
	}

	savemask = sigblock(sigmask(SIGPOLL));

	if ((retval = t_look(fd)) < 0) {
		sigsetmask(savemask);
		return(-1);
	}

	if (retval == T_DISCONNECT) {
		sigsetmask(savemask);
		t_errno = TLOOK;
		return(-1);
	}

	if (tiptr->ti_lookflg && (*((long *)tiptr->ti_lookcbuf) == T_ORDREL_IND)) {
		tiptr->ti_lookflg = 0;
		sigsetmask(savemask);
		return(0);
	} else {
		if (retval != T_ORDREL) {
			sigsetmask(savemask);
			t_errno = TNOREL;
			return(-1);
		}
	}

	/*
	 * get ordrel off read queue.
	 * use ctl and rcv buffers
	 */
	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = tiptr->ti_ctlbuf;
	databuf.maxlen = tiptr->ti_rcvsize;
	databuf.len = 0;
	databuf.buf = tiptr->ti_rcvbuf;

	if ((retval = getmsg(fd, &ctlbuf, &databuf, &flg)) < 0) {
		sigsetmask(savemask);
		t_errno = TSYSERR;
		return(-1);
	}

	sigsetmask(savemask);
	/*
	 * did I get entire message?
	 */
	if (retval) {
		t_errno = TSYSERR;
		errno = EIO;
		return(-1);
	}
	pptr = (union T_primitives *)ctlbuf.buf;

	if ((ctlbuf.len < sizeof(struct T_ordrel_ind)) ||
	    (pptr->type != T_ORDREL_IND)) {
		t_errno = TSYSERR;
		errno = EPROTO;
		return(-1);
	}

	return(0);
}
