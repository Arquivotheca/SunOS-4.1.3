#ifndef lint
static	char sccsid[] = "@(#)win_syscall.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * SunWindows kernel equivalents of selected user process level calls,
 * i.e, dup and read.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <specfs/snode.h>

/*
 * Dup file descriptor for device opened by user for kernel use.
 */
int
kern_dupfd(fd, fpp)
	int	fd;
	struct	file **fpp;
{
	register struct file *fp;

	*fpp = NULL;
	fp = getf(fd);
	if (fp == 0) {
#ifdef WINDEVDEBUG
		printf("getf fp == 0\n");
#endif
		return (EINVAL);
	}
	if (fp->f_type != DTYPE_VNODE)
		return (EINVAL);	/* not a vnode */
	*fpp = fp;
	fp->f_count++;
	return (0);
}

int
kern_read(vp, buf, len_ptr)
	struct	vnode *vp;
	caddr_t	buf;
	int	*len_ptr;
{
	int error;

	error = strkread(vp->v_stream, buf, len_ptr);
	/*
	 * If any data was read, mark the vnode as accessed.
	 */
	if (*len_ptr != 0)
		smark(VTOS(vp), SACC);
	return (error);
}
