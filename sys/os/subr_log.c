/*	@(#)subr_log.c 1.1 92/07/30 SMI; from UCB 7.1 6/5/86	*/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Error log buffer for kernel printf's.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/msgbuf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/errno.h>

#define	LOG_RDPRI	(PZERO + 1)

#define	LOG_NBIO	0x02
#define	LOG_ASYNC	0x04
#define	LOG_RDWAIT	0x08

struct logsoftc {
	int	sc_state;		/* see above for possibilities */
	struct	proc *sc_selp;		/* process waiting on select call */
	int	sc_pgrp;		/* process group for async I/O */
} logsoftc;

int	log_open;			/* also used in log() */

/*ARGSUSED*/
logopen(dev, flag)
	dev_t dev;
{

	if (log_open)
		return (EBUSY);
	log_open = 1;
	logsoftc.sc_selp = 0;
	logsoftc.sc_pgrp = u.u_procp->p_pgrp;
	/*
	 * Potential race here with putchar() but since putchar should be
	 * called by autoconf, msg_magic should be initialized by the time
	 * we get here.
	 */
	if (msgbuf.msg_magic != MSG_MAGIC) {
		register int i;

		msgbuf.msg_magic = MSG_MAGIC;
		msgbuf.msg_bufx = msgbuf.msg_bufr = 0;
		for (i=0; i < MSG_BSIZE; i++)
			msgbuf.msg_bufc[i] = 0;
	}
	return (0);
}

/*ARGSUSED*/
logclose(dev, flag)
	dev_t dev;
{
	log_open = 0;
	logsoftc.sc_state = 0;
	logsoftc.sc_selp = 0;
	logsoftc.sc_pgrp = 0;
	return (0);
}

/*ARGSUSED*/
logread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register long l;
	register int s;
	int error = 0;

	s = splhigh();
	while (msgbuf.msg_bufr == msgbuf.msg_bufx) {
		if (uio->uio_fmode & FNONBIO) {
			(void) splx(s);
			return (EAGAIN);
		}
		if (logsoftc.sc_state & LOG_NBIO) {
			(void) splx(s);
			return (EWOULDBLOCK);
		}
		if (uio->uio_fmode & FNBIO) {
			(void) splx(s);
			return (0);
		}
		logsoftc.sc_state |= LOG_RDWAIT;
		(void) sleep((caddr_t)&msgbuf, LOG_RDPRI);
	}
	(void) splx(s);
	logsoftc.sc_state &= ~LOG_RDWAIT;

	while (uio->uio_resid > 0) {
		l = msgbuf.msg_bufx - msgbuf.msg_bufr;
		if (l < 0)
			l = MSG_BSIZE - msgbuf.msg_bufr;
		l = MIN(l, uio->uio_resid);
		if (l == 0)
			break;
		error = uiomove((caddr_t)&msgbuf.msg_bufc[msgbuf.msg_bufr],
			(int)l, UIO_READ, uio);
		if (error)
			break;
		msgbuf.msg_bufr += l;
		if (msgbuf.msg_bufr < 0 || msgbuf.msg_bufr >= MSG_BSIZE)
			msgbuf.msg_bufr = 0;
	}
	return (error);
}

/*ARGSUSED*/
logselect(dev, rw)
	dev_t dev;
	int rw;
{
	int s = splhigh();

	switch (rw) {

	case FREAD:
		if (msgbuf.msg_bufr != msgbuf.msg_bufx) {
			(void) splx(s);
			return (1);
		}
		logsoftc.sc_selp = u.u_procp;
		break;
	}
	(void) splx(s);
	return (0);
}

logwakeup()
{

	if (!log_open)
		return;
	if (logsoftc.sc_selp) {
		selwakeup(logsoftc.sc_selp, 0);
		logsoftc.sc_selp = 0;
	}
	if (logsoftc.sc_state & LOG_ASYNC)
		gsignal(logsoftc.sc_pgrp, SIGIO);
	if (logsoftc.sc_state & LOG_RDWAIT) {
		wakeup((caddr_t)&msgbuf);
		logsoftc.sc_state &= ~LOG_RDWAIT;
	}
}

/*ARGSUSED*/
logioctl(com, data, flag)
	caddr_t data;
{
	long l;
	int s;

	switch (com) {

	/* return number of characters immediately available */
	case FIONREAD:
		s = splhigh();
		l = msgbuf.msg_bufx - msgbuf.msg_bufr;
		(void) splx(s);
		if (l < 0)
			l += MSG_BSIZE;
		*(off_t *)data = l;
		break;

	case FIONBIO:
		if (*(int *)data)
			logsoftc.sc_state |= LOG_NBIO;
		else
			logsoftc.sc_state &= ~LOG_NBIO;
		break;

	case FIOASYNC:
		if (*(int *)data)
			logsoftc.sc_state |= LOG_ASYNC;
		else
			logsoftc.sc_state &= ~LOG_ASYNC;
		break;

	case FIOSETOWN:
		logsoftc.sc_pgrp = -(*(int *)data);
		break;

	case TIOCSPGRP:
		logsoftc.sc_pgrp = *(int *)data;
		break;

	case FIOGETOWN:
		*(int *)data = -logsoftc.sc_pgrp;
		break;

	case TIOCGPGRP:
		*(int *)data = logsoftc.sc_pgrp;
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}
