#ifndef lint
static	char sccsid[] = "@(#)t_open.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_open.c	1.5"
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
#include <fcntl.h>

extern struct _ti_user *_ti_user;
extern int t_errno;
extern int errno;
extern char *calloc();
extern char *malloc();
extern int ioctl();
extern int open();
extern int close();
extern int ulimit();


t_open(path, flags, info)
char *path;
int flags;
register struct t_info *info;
{
	int retval, fd;
	struct T_info_ack inforeq;
	register struct _ti_user *tiptr;
	int retlen;
	int savemask;


	if ((fd = open(path, flags)) < 0) {
		t_errno = TSYSERR;
		return(-1);
	}

	/*
	 * is module already pushed
	 */
	if ((retval = ioctl(fd, I_FIND, "timod")) < 0) {
		t_errno = TSYSERR;
		close(fd);
		return(-1);
	}


	if (!retval)
		if (ioctl(fd, I_PUSH, "timod") < 0) {
			t_errno = TSYSERR;
			close(fd);
			return(-1);
		}
	

	
	savemask = sigblock(sigmask(SIGPOLL));

	inforeq.PRIM_type = T_INFO_REQ;

	if (!_t_do_ioctl(fd, (caddr_t)&inforeq, sizeof(struct T_info_req), TI_GETINFO, &retlen)) {
		sigsetmask(savemask);
		close(fd);
		return(-1);
	}
	sigsetmask(savemask);
		

	if (retlen != sizeof(struct T_info_ack)) {
		t_errno = TSYSERR;
		errno = EIO;
		close(fd);
		return(-1);
	}

	if (info != NULL) {
		info->addr = inforeq.ADDR_size;
		info->options = inforeq.OPT_size;
		info->tsdu = inforeq.TSDU_size;
		info->etsdu = inforeq.ETSDU_size;
		info->connect = inforeq.CDATA_size;
		info->discon = inforeq.DDATA_size;
		info->servtype = inforeq.SERV_type;
	}

	/*
	 * if routine never been called before
	 * then allocate the ti_user structures
	 * for all file desc.
	 */
	 if (!_ti_user) 
		if ((_ti_user = (struct _ti_user *)calloc(1, (unsigned)(OPENFILES*sizeof(struct _ti_user)))) == NULL) {
			t_errno = TSYSERR;
			close(fd);
			return(-1);
		}


	/*
	 * if first time done then initialize data structure
	 * and allocate buffers
	 */
	tiptr = &_ti_user[fd];
	if (!(tiptr->ti_flags & USED)) {

		if ((_t_alloc_bufs(fd, tiptr, inforeq) < 0) ||
		    (ioctl(fd, I_FLUSH, FLUSHRW) < 0)) {
			t_close(fd);
			t_errno = TSYSERR;
			return(-1);
		}
	}
	return(fd);
}
