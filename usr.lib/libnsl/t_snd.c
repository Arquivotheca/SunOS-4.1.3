#ifndef lint
static	char sccsid[] = "@(#)t_snd.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_snd.c	1.3"
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
extern int putmsg();


t_snd(fd, buf, nbytes, flags)
int fd;
register char *buf;
unsigned nbytes;
int flags;
{
	struct strbuf ctlbuf, databuf;
	struct T_data_req *datareq;
	int flg = 0;
	int tmpcnt, tmp;
	char *tmpbuf;
	register struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL)
		return(-1);

	if (tiptr->ti_servtype == T_CLTS) {
		t_errno = TNOTSUPPORT;
		return(-1);
	}


	datareq = (struct T_data_req *)tiptr->ti_ctlbuf;
	if (flags&T_EXPEDITED) {
		datareq->PRIM_type = T_EXDATA_REQ;
	} else
		datareq->PRIM_type = T_DATA_REQ;


	ctlbuf.maxlen = sizeof(struct T_data_req);
	ctlbuf.len = sizeof(struct T_data_req);
	ctlbuf.buf = tiptr->ti_ctlbuf;
	tmp = nbytes;
	tmpbuf = buf;

	while (tmp) {
		if ((tmpcnt = tmp) > tiptr->ti_maxpsz) {
			datareq->MORE_flag = 1;
			tmpcnt = tiptr->ti_maxpsz;
		} else {
			if (flags&T_MORE)
				datareq->MORE_flag = 1;
			else
				datareq->MORE_flag = 0;
		}

		databuf.maxlen = tmpcnt;
		databuf.len = tmpcnt;
		databuf.buf = tmpbuf;
	
		if (putmsg(fd, &ctlbuf, &databuf, flg) < 0) {
			if (nbytes == tmp) {
				if (errno == EAGAIN)
					t_errno = TFLOW;
				else
					t_errno = TSYSERR;
				return(-1);
			} else
				return(nbytes - tmp);
		}
		tmp = tmp - tmpcnt;
		tmpbuf = tmpbuf + tmpcnt;
	}
	return(nbytes - tmp);
}
