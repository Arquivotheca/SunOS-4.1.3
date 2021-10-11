#ifndef lint
static	char sccsid[] = "@(#)t_rcv.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_rcv.c	1.6"
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
extern int ioctl(), getmsg();


t_rcv(fd, buf, nbytes, flags)
int fd;
register char *buf;
unsigned nbytes;
int *flags;
{
	struct strbuf ctlbuf, rcvbuf;
	int retval, flg = 0;
	int msglen;
	register union T_primitives *pptr;
	register struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL)
		return(-1);

	if (tiptr->ti_servtype == T_CLTS) {
		t_errno = TNOTSUPPORT;
		return(-1);
	}
	
	/*
         * Check in lookbuf for stuff
	 */
	if (tiptr->ti_lookflg) {
		/*
		 * Beware - this is right!
		 *	If something in lookbuf then check
		 *	read queue to see if there is something there.
		 *	If there is something there and there is not a
		 * 	discon in lookbuf, then it must be a discon.
		 *      If so, fall through to get it off of queue.
		 *	I fall through to make sure it is a discon,
		 * 	instead of making check here.
		 *
		 *	If nothing in read queue then just return TLOOK.
		 */
		if ((retval = ioctl(fd, I_NREAD, &msglen)) < 0) {
			t_errno = TSYSERR;
			return(-1);
		}
		if (retval) {
			if (*((long *)tiptr->ti_lookcbuf) == T_DISCON_IND) {
				t_errno = TLOOK;
				return(-1);
			}
		} else {
			t_errno = TLOOK;
			return(-1);
		}
	}

	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = tiptr->ti_ctlbuf;
	rcvbuf.maxlen = nbytes;
	rcvbuf.len = 0;
	rcvbuf.buf = buf;
	*flags = 0;

	/*
	 * data goes right in user buffer
	 */
	if ((retval = getmsg(fd, &ctlbuf, &rcvbuf, &flg)) < 0) {
		if (errno == EAGAIN)
			t_errno = TNODATA;
		else
			t_errno = TSYSERR;
		return(-1);
	}
	if (rcvbuf.len == -1) rcvbuf.len = 0;


	if (ctlbuf.len > 0) {
		if (ctlbuf.len < sizeof(long)) {
			t_errno = TSYSERR;
			errno = EPROTO;
			return(-1);
		}

		pptr = (union T_primitives *)ctlbuf.buf;

		switch(pptr->type) {

			case T_EXDATA_IND:
				*flags |= T_EXPEDITED;
				/* flow thru */
			case T_DATA_IND:
				if ((ctlbuf.len < sizeof(struct T_data_ind)) ||
				    (tiptr->ti_lookflg)) {
					t_errno = TSYSERR;
					errno = EPROTO;
					return(-1);
				}
	
				if ((pptr->data_ind.MORE_flag) || retval)
					*flags |= T_MORE;
				if ((pptr->data_ind.MORE_flag) && retval)
					tiptr->ti_flags |= MORE;
				return(rcvbuf.len);
	
			case T_ORDREL_IND:
				if (tiptr->ti_lookflg) {
					t_errno = TSYSERR;
					errno = EPROTO;
					return(-1);
				}
				/* flow thru */

			case T_DISCON_IND:
				_t_putback(tiptr, rcvbuf.buf, rcvbuf.len, ctlbuf.buf, ctlbuf.len);
				if (retval&MOREDATA) {
					ctlbuf.maxlen = 0;
					ctlbuf.len = 0;
					ctlbuf.buf = tiptr->ti_ctlbuf;
					rcvbuf.maxlen = tiptr->ti_rcvsize - rcvbuf.len;
					rcvbuf.len = 0;
					rcvbuf.buf = tiptr->ti_lookdbuf+tiptr->ti_lookdsize;
					*flags = 0;

					if ((retval = getmsg(fd, &ctlbuf, &rcvbuf, &flg)) < 0) {
						t_errno = TSYSERR;
						return(-1);
					}
					if (rcvbuf.len == -1) rcvbuf.len = 0;
					if (retval) {
						t_errno = TSYSERR;
						errno = EPROTO;
						tiptr->ti_lookflg = 0;
						return(-1);
					}
					tiptr->ti_lookdsize += rcvbuf.len;
				}
					
				t_errno = TLOOK;
				return(-1);
		
			default:
				break;
		}
	
		t_errno = TSYSERR;
		errno = EPROTO;
		return(-1);
	} else {
		if (!retval && (tiptr->ti_flags&MORE)) {
			*flags |= T_MORE;
			tiptr->ti_flags &= ~MORE;
		}
		if (retval)
			*flags |= T_MORE;
		return(rcvbuf.len);
	}
}
