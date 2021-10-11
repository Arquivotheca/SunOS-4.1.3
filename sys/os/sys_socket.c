/*	@(#)sys_socket.c 1.1 92/07/30 SMI; from UCB 7.1 6/5/86	*/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/stat.h>

#include <net/if.h>
#include <net/route.h>

int	soo_rw(), soo_ioctl(), soo_select(), soo_close();
struct	fileops socketops =
	    { soo_rw, soo_ioctl, soo_select, soo_close };

soo_rw(fp, rw, uio)
	struct file *fp;
	enum uio_rw rw;
	struct uio *uio;
{
	int soreceive(), sosend();

	return (
	    (*(rw==UIO_READ?soreceive:sosend))
		((struct socket *)fp->f_data, 0, uio, 0, 0));
}

soo_ioctl(fp, cmd, data)
	struct file *fp;
	int cmd;
	register caddr_t data;
{
	register struct socket *so = (struct socket *)fp->f_data;

	switch (cmd) {

	case FIONBIO:
		if (*(int *)data)
			so->so_state |= SS_NBIO;
		else
			so->so_state &= ~SS_NBIO;
		return (0);

	case FIOASYNC:
		if (*(int *)data)
			so->so_state |= SS_ASYNC;
		else
			so->so_state &= ~SS_ASYNC;
		return (0);

	case FIONREAD:
		*(int *)data = so->so_rcv.sb_cc;
		return (0);

	case FIOSETOWN:
	case SIOCSPGRP:
		so->so_pgrp = *(int *)data;
		return (0);

	case FIOGETOWN:
	case SIOCGPGRP:
		*(int *)data = so->so_pgrp;
		return (0);

	case SIOCATMARK:
		*(int *)data = (so->so_state&SS_RCVATMARK) != 0;
		return (0);
	}
	/*
	 * Interface/routing/protocol specific ioctls:
	 * interface and routing ioctls should have a
	 * different entry since a socket's unnecessary
	 */
#define	cmdbyte(x)	(((x) >> 8) & 0xff)
	if (cmdbyte(cmd) == 'i')
		return (ifioctl(so, cmd, data));
	if (cmdbyte(cmd) == 'r')
		return (rtioctl(cmd, data));
	return ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
	    (struct mbuf *)cmd, (struct mbuf *)data, (struct mbuf *)0));
}

soo_select(fp, which)
	struct file *fp;
	int which;
{
	register struct socket *so = (struct socket *)fp->f_data;
	register int s = splnet();

	switch (which) {

	case FREAD:
		if (soreadable(so)) {
			(void) splx(s);
			return (1);
		}
		sbselqueue(&so->so_rcv);
		break;

	case FWRITE:
		if (sowriteable(so)) {
			(void) splx(s);
			return (1);
		}
		sbselqueue(&so->so_snd);
		break;

	case 0:
		if (so->so_oobmark ||
		    (so->so_state & SS_RCVATMARK)) {
			(void) splx(s);
			return (1);
		}
		sbselqueue(&so->so_rcv);
		break;
	}
	(void) splx(s);
	return (0);
}

soo_stat(so, ub)
	register struct socket *so;
	register struct stat *ub;
{
	/*
	 * Fill in the stat structure with reasonable default values and then
	 * call the protocol to give it a chance to override.
	 */
	bzero((caddr_t)ub, sizeof (*ub));
	ub->st_blksize = so->so_snd.sb_hiwat;
	ub->st_dev = NODEV;
	ub->st_ino = (ino_t)so;
	ub->st_mode = S_IFSOCK|0666;
	ub->st_nlink = 1;
	ub->st_uid = u.u_uid;
	ub->st_gid = u.u_gid;
	ub->st_size = so->so_rcv.sb_cc;
	ub->st_atime = time.tv_sec;
	ub->st_mtime = time.tv_sec;
	ub->st_ctime = time.tv_sec;
	return ((*so->so_proto->pr_usrreq)(so, PRU_SENSE,
	    (struct mbuf *)ub, (struct mbuf *)0,
	    (struct mbuf *)0));
}

soo_close(fp)
	struct file *fp;
{
	int error = 0;

	if (fp->f_count > 1)
		return (error);
	if (fp->f_data)
		error = soclose((struct socket *)fp->f_data);
	fp->f_data = 0;
	return (error);
}
