#ifndef lint
static	char sccsid[] = "@(#)t_look.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_look.c	1.2"
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
extern int ioctl();


t_look(fd)
int fd;
{
	struct strpeek strpeek;
	int retval;
	union T_primitives *pptr;
	register struct _ti_user *tiptr;
	long type;

	if ((tiptr = _t_checkfd(fd)) == NULL)
		return(-1);



	strpeek.ctlbuf.maxlen = sizeof(long);
	strpeek.ctlbuf.len = 0;
	strpeek.ctlbuf.buf = tiptr->ti_ctlbuf;
	strpeek.databuf.maxlen = 0;
	strpeek.databuf.len = 0;
	strpeek.databuf.buf = NULL;
	strpeek.flags = 0;

	if ((retval = ioctl(fd, I_PEEK, &strpeek)) < 0)
		return(T_ERROR);

	/*
	 * if something there and cnt part also there
	 */
	if (tiptr->ti_lookflg || (retval && (strpeek.ctlbuf.len >= sizeof(long)))) {
		pptr = (union T_primitives *)strpeek.ctlbuf.buf;
		if (tiptr->ti_lookflg) {
			if (((type = *((long *)tiptr->ti_lookcbuf)) != T_DISCON_IND) &&
		    	    (retval && (pptr->type == T_DISCON_IND))) {
				type = pptr->type;
				tiptr->ti_lookflg = 0;
			}
		} else
			type = pptr->type;

		switch(type) {

		case T_CONN_IND:
			return(T_LISTEN);

		case T_CONN_CON:
			return(T_CONNECT);

		case T_DISCON_IND:
			return(T_DISCONNECT);

		case T_DATA_IND:
		case T_UNITDATA_IND:
			return(T_DATA);

		case T_EXDATA_IND:
			return(T_EXDATA);

		case T_UDERROR_IND:
			return(T_UDERR);

		case T_ORDREL_IND:
			return(T_ORDREL);

		default:
			t_errno = TSYSERR;
			errno = EPROTO;
			return(-1);
		}
	}

	/*
	 * if something there put no control part
	 * it must be data
	 */
	if (retval && (strpeek.ctlbuf.len <= 0))
		return(T_DATA);

	/*
	 * if msg there and control
	 * part not large enough to determine type?
	 * it must be illegal TLI message
	 */
	if (retval && (strpeek.ctlbuf.len > 0)) {
		t_errno = TSYSERR;
		errno = EPROTO;
		return(-1);
	}
	return(0);
}
