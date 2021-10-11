#ifndef lint
static	char sccsid[] = "@(#)t_rcvuderr.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_rcvuderr.c	1.3"
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


t_rcvuderr(fd, uderr)
int fd;
struct t_uderr *uderr;
{
	struct strbuf ctlbuf, rcvbuf;
	int flg;
	int retval;
	register union T_primitives *pptr;
	register struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL)
		return(-1);


	if (tiptr->ti_servtype != T_CLTS) {
		t_errno = TNOTSUPPORT;
		return(-1);
	}
	/* 
         * is there an error indication in look buffer
	 */
	if (tiptr->ti_lookflg) {
		ctlbuf.maxlen = tiptr->ti_lookcsize;
		ctlbuf.len = tiptr->ti_lookcsize;
		ctlbuf.buf = tiptr->ti_lookcbuf;
		rcvbuf.maxlen = 0;
		rcvbuf.len = 0;
		rcvbuf.buf = NULL;
	} else {
		if ((retval = t_look(fd)) < 0)
			return(-1);
		if (retval != T_UDERR) {
			t_errno = TNOUDERR;
			return(-1);
		}
	
		ctlbuf.maxlen = tiptr->ti_ctlsize;
		ctlbuf.len = 0;
		ctlbuf.buf = tiptr->ti_ctlbuf;
		rcvbuf.maxlen = 0;
		rcvbuf.len = 0;
		rcvbuf.buf = NULL;

		if ((retval = getmsg(fd, &ctlbuf, &rcvbuf, &flg)) < 0) {
			t_errno = TSYSERR;
			return(-1);
		}
		/*
		 * did I get entire message?
		 */
		if (retval) {
			t_errno = TSYSERR;
			errno = EIO;
			return(-1);
		}

	}

	tiptr->ti_lookflg = 0;

	pptr = (union T_primitives *)ctlbuf.buf;

	if ((ctlbuf.len < sizeof(struct T_uderror_ind)) ||
	    (pptr->type != T_UDERROR_IND)) {
		t_errno = TSYSERR;
		errno = EPROTO;
		return(-1);
	}

	if (uderr) {
		if ((uderr->addr.maxlen < pptr->uderror_ind.DEST_length) ||
		    (uderr->opt.maxlen < pptr->uderror_ind.OPT_length)) {
			t_errno = TBUFOVFLW;
			return(-1);
		}
	
		uderr->error = pptr->uderror_ind.ERROR_type;
		memcpy(uderr->addr.buf, ctlbuf.buf +
		       pptr->uderror_ind.DEST_offset,
		       (int)pptr->uderror_ind.DEST_length);
		memcpy(uderr->opt.buf, ctlbuf.buf +
		       pptr->uderror_ind.OPT_offset,
		       (int)pptr->uderror_ind.OPT_length);
	}
	return(0);
}
