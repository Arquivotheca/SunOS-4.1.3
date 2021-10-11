#ifndef lint
static	char sccsid[] = "@(#)t_rcvdis.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_rcvdis.c	1.9"
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
extern int getmsg(), ioctl();


t_rcvdis(fd, discon)
int fd;
struct t_discon *discon;
{
	struct strbuf ctlbuf;
	struct strbuf databuf;
	int retval;
	int flg = 0;
	union T_primitives *pptr;
	register struct _ti_user *tiptr;
	int savemask;


	if ((tiptr = _t_checkfd(fd)) == NULL)
		return(-1);

	if (tiptr->ti_servtype == T_CLTS) {
		t_errno = TNOTSUPPORT;
		return(-1);
	}

	/*
         * is there a discon in look buffer
	 */
	savemask = sigblock(sigmask(SIGPOLL));

	if (tiptr->ti_lookflg && (*((long *)tiptr->ti_lookcbuf) == T_DISCON_IND)) {
		ctlbuf.maxlen = tiptr->ti_lookcsize;
		ctlbuf.len = tiptr->ti_lookcsize;
		ctlbuf.buf = tiptr->ti_lookcbuf;
		databuf.maxlen = tiptr->ti_lookdsize;
		databuf.len = tiptr->ti_lookdsize;
		databuf.buf = tiptr->ti_lookdbuf;
	} else {

		if ((retval = t_look(fd)) < 0) {
			sigsetmask(savemask);
			return(-1);
		}
		

		if (retval != T_DISCONNECT) {
			sigsetmask(savemask);
			t_errno = TNODIS;
			return(-1);
		}

		/*
		 * get disconnect off read queue.
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
		if (databuf.len == -1) databuf.len = 0;

		/*
		 * did I get entire message?
		 */
		if (retval) {
			sigsetmask(savemask);
			t_errno = TSYSERR;
			errno = EIO;
			return(-1);
		}
	}

	sigsetmask(savemask);
	tiptr->ti_lookflg = 0;
	
	pptr = (union T_primitives *)ctlbuf.buf;

	if ((ctlbuf.len < sizeof(struct T_discon_ind)) ||
	    (pptr->type != T_DISCON_IND)) {
		t_errno = TSYSERR;
		errno = EPROTO;
		return(-1);
	}

	if (discon != NULL) {
		if (databuf.len > (int)discon->udata.maxlen) {
			t_errno = TBUFOVFLW;
			return(-1);
		}
	
		discon->reason = pptr->discon_ind.DISCON_reason;
		memcpy(discon->udata.buf, databuf.buf, (int)databuf.len);
		discon->udata.len = databuf.len;
		discon->sequence = pptr->discon_ind.SEQ_number;
	}

	/*
	 * clear more flag
	 */
	tiptr->ti_flags &= ~MORE;

	return(0);
}
