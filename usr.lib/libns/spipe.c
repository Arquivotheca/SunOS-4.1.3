
/*	@(#)spipe.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:spipe.c	1.1" */
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stropts.h>

#define PIPNOD	"/dev/spx"
extern int	errno;

int spipe(fds)
int *fds;
{
	struct strfdinsert fdi;
	long dummy;

	if ((fds[0] = open(PIPNOD, O_RDWR)) < 0)  {
		printf("spipe: open 0 failed, errno=%d\n",errno);
		return(-1);
	}
	if ((fds[1] = open(PIPNOD, O_RDWR)) < 0) {
		printf("spipe: open 1 failed, errno=%d\n",errno);
		return(-1);
	}
	fdi.databuf.maxlen = fdi.databuf.len = -1;
	fdi.databuf.buf = NULL;
	fdi.ctlbuf.maxlen = fdi.ctlbuf.len = sizeof(long);
	fdi.ctlbuf.buf = (caddr_t)&dummy;
	fdi.offset = 0;
	fdi.fildes = fds[1];
	fdi.flags = 0;
	if (ioctl(fds[0], I_FDINSERT, &fdi) < 0)
		return(-1);
	return(0);
}

