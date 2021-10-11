#ifndef lint
static        char sccsid[] = "@(#)kern_subr.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include <sys/uio.h>
#include <mon/sunromvec.h>

#ifdef	DUMP_DEBUG
static int dump_debug = 20;
#endif	/* DUMP_DEBUG */

uiomove(cp, n, rw, uio)
	register caddr_t cp;
	register int n;
	enum uio_rw rw;
	register struct uio *uio;
{
	register struct iovec *iov;
	u_int cnt;
	int error = 0;


	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;
		switch (uio->uio_seg) {

		case 0:
		case 2:
#ifdef	DUMP_DEBUG
	dprint(dump_debug, 0,
		"uiomove: copyin/out not supported uio_seg 0x%x\n",
		uio->uio_seg);
#endif	/* DUMP_DEBUG */
			error = -1;
			break;



		case 1:
			if (rw == UIO_READ)
				bcopy((caddr_t)cp, iov->iov_base, cnt);
			else
				bcopy(iov->iov_base, (caddr_t)cp, cnt);
			break;
		}
		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return (error);
}

