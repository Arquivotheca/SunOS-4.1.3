/*	@(#)str_syscalls.c 1.1 92/07/30 SMI; from S5R3 os/sys2.c 10.19	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/poll.h>
#include <sys/kernel.h>
#include <sys/debug.h>

/*
 * getmsg system call
 */
getmsg()
{
	msgio(FREAD);
}

/*
 * putmsg system call
 */
putmsg()
{
	msgio(FWRITE);
}

/*
 * common code for getmsg and putmsg calls:
 * check permissions, copy in args, preliminary setup,
 * and switch to appropriate stream routine
 */
msgio(mode)
register mode;
{
	register struct file *fp;
	register struct a {
		int		fdes;
		struct strbuf	*ctl;
		struct strbuf	*data;
		union {
			int		flags;
			int		*flagsp;
		} flags_un;
	} *uap;
	struct strbuf msgctl, msgdata;
	register struct vnode *vp;

	uap = (struct a *)u.u_ap;
	GETF(fp, uap->fdes);
	if (!(fp->f_flag&mode)) {
		u.u_error = EBADF;
		return;
	}
	if (fp->f_type != DTYPE_VNODE) {
		u.u_error = ENOSTR;
		return;
	}
	vp = (struct vnode *)fp->f_data;
	if ((vp->v_type != VCHR) || (vp->v_stream == NULL)) {
		u.u_error = ENOSTR;
		return;
	}
	if (uap->ctl) {
		u.u_error = copyin((caddr_t)uap->ctl, (caddr_t)&msgctl,
		    sizeof (struct strbuf));
		if (u.u_error)
			return;
	}
	if (uap->data) {
		u.u_error = copyin((caddr_t)uap->data, (caddr_t)&msgdata,
		    sizeof (struct strbuf));
		if (u.u_error)
			return;
	}
	if (mode == FREAD) {
		if (!uap->ctl)
			msgctl.maxlen = -1;
		if (!uap->data)
			msgdata.maxlen = -1;
		u.u_r.r_val1 = strgetmsg(vp, &msgctl, &msgdata,
		    (caddr_t)uap->flags_un.flagsp, fp->f_flag);
		if (u.u_error)
			return;
		if (uap->ctl) {
			u.u_error = copyout((caddr_t)&msgctl,
			    (caddr_t)uap->ctl, sizeof (struct strbuf));
			if (u.u_error)
				return;
		}
		if (uap->data) {
			u.u_error = copyout((caddr_t)&msgdata,
			    (caddr_t)uap->data, sizeof (struct strbuf));
			if (u.u_error)
				return;
		}
		return;
	}
	/*
	 * FWRITE case
	 */
	if (!uap->ctl)
		msgctl.len = -1;
	if (!uap->data)
		msgdata.len = -1;
	strputmsg(vp, &msgctl, &msgdata, uap->flags_un.flags, fp->f_flag);
}

/*
 * Poll system call
 */
poll()
{
	register struct uap  {
		struct	pollfd *fdp;
		unsigned long nfds;
		long	timo;
	} *uap = (struct uap *)u.u_ap;
	struct pollfd pollfd[NPOLLFILE];
	register struct pollfd *pfd;
	caddr_t fdp;
	register int fdcnt = 0;
	register int i, j, s;
	struct timeval atv;
	register struct file *fp;
	int polltime();
	struct strevent *timeproc;
	int size;
	int mark;
	register struct vnode *vp;
	char newu_error;

	if (uap->nfds > NOFILE) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->timo > 0) {
		/*
		 * Convert milliseconds to wait to an
		 * absolute time in timeval format.
		 */
		atv.tv_sec = uap->timo / 1000;
		atv.tv_usec = (uap->timo % 1000) * 1000;
		if (itimerfix(&atv)) {
			u.u_error = EINVAL;
			return;
		}
		s = splclock();
		timevaladd(&atv, &time);
		(void) splx(s);
	}

	/*
	 * retry scan of fds until an event is found or until the
	 * timeout is reached.
	 */
retry:

	/*
	 * Polling the fds is a relatively long process.  Set up the SPOLL
	 * flag so that we can see if something happened
	 * to an fd after we checked it but before we go to sleep.
	 */
	u.u_procp->p_flag |= SPOLL;

	/*
	 * Check fd's for specified events.
	 * Read in pollfd records in blocks of NPOLLFILE.  Test each fd in the
	 * block and store the result of the test in the event field of the
	 * in-core record.  After a block of fds is finished, write the result
	 * out to the user.  Note that if no event is found, the whole
	 * procedure will be repeated after awakenening from the sleep
	 * (subject to timeout).
	 */

	mark = uap->nfds;
	size = 0;
	fdp = (caddr_t)uap->fdp;
	for (i = 0, pfd = pollfd; i < uap->nfds; i++, pfd++) {
		j = i % NPOLLFILE;
		/*
		 * If we have looped back around to the base of pollfd,
		 * write out the results of the strpoll calls kept in pollfd
		 * to the user fdp.  Read in the next batch of fds to check.
		 */
		if (!j) {
			if (i > 0) {
				ASSERT(size == NPOLLFILE *
				    sizeof (struct pollfd));
				u.u_error = copyout((caddr_t)pollfd, fdp,
						(u_int)size);
				if (u.u_error)
					return;
				fdp += size;
			}
			size = MIN(uap->nfds - i, NPOLLFILE) *
			    sizeof (struct pollfd);
			u.u_error =
			    copyin((caddr_t)fdp, (caddr_t)pollfd,
						(u_int)size);
			if (u.u_error)
				return;
			pfd = pollfd;
		}

		if (pfd->fd < 0)
			pfd->revents = 0;
		else if ((fp = getf(pfd->fd)) == NULL){
			u.u_error = 0;
			pfd->revents = POLLNVAL;
		}
		else if ((fp->f_type != DTYPE_VNODE) ||
		    !(vp = (struct vnode *)fp->f_data) ||
		    (vp->v_type != VCHR) ||
		    !vp->v_stream) {
			pfd->revents = pollsel(fp, pfd->events);
			if (u.u_error) {
				if (fdcnt == 0)
					mark = i;
				goto pollout;
			}
		} else {
			pfd->revents = strpoll(vp->v_stream,
			    pfd->events, fdcnt);
			if (u.u_error) {
				if (fdcnt == 0)
					mark = i;
				goto pollout;
			}
		}
		if (pfd->revents && fdcnt++ == 0)
			mark = i;
	}

	/*
	 * Poll of fds completed.
	 * Copy out the last batch of events.  If the poll was successful,
	 * return fdcnt to user.
	 */
	u.u_r.r_val1 = fdcnt;
	u.u_error = copyout((caddr_t)pollfd, fdp, (u_int)size);
	if (u.u_error)
		return;
	if (fdcnt)
		goto pollout;

	/*
	 * If you get here, the poll of fds was unsuccessful.
	 * First make sure your timeout hasn't been reached.
	 * If not then sleep and wait until some fd becomes
	 * readable, writeable, or gets an exception.
	 */
	if (uap->timo == 0)
		goto pollout;
	s = splclock();
	/* this should be timercmp(&time, &atv, >=) */
	if (uap->timo > 0 && (time.tv_sec > atv.tv_sec ||
	    time.tv_sec == atv.tv_sec && time.tv_usec >= atv.tv_usec)) {
		(void) splx(s);
		goto pollout;
	}

	/*
	 * If anything has happened on an fd since it was checked, it will
	 * have turned off SPOLL.  Check this and rescan if so.
	 */
	if (!(u.u_procp->p_flag & SPOLL)) {
		(void) splx(s);
		goto retry;
	}
	u.u_procp->p_flag &= ~SPOLL;

	if (!(timeproc = sealloc(SE_SLEEP))) {
		(void) splx(s);
		u.u_error = EAGAIN;
		goto pollout;
	}
	timeproc->se_procp = u.u_procp;
	if (uap->timo > 0)
		timeout(polltime, (caddr_t)timeproc, hzto(&atv));

	/*
	 * The sleep will be awakened either by this poll's timeout (which
	 * will have nulled timeproc), or by the strwakepoll function called
	 * from a stream head.
	 */
	if (sleep((caddr_t)&pollwait, (PZERO+1)|PCATCH)) {
		if (uap->timo > 0)
			untimeout(polltime, (caddr_t)timeproc);
		(void) splx(s);
		u.u_error = EINTR;
		sefree(timeproc);
		goto pollout;
	}
	(void) splx(s);
	if (uap->timo > 0)
		untimeout(polltime, (caddr_t)timeproc);

	/*
	 * If timeproc is not NULL, you know you were woken because
	 * write queue emptied, read queue got data, or an exception
	 * condition occurred.  If so go back up and poll fds again.
	 * Otherwise, you've timed out so you will fall thru and return.
	 */
	if (timeproc->se_procp) {
		sefree(timeproc);
		goto retry;
	}
	sefree(timeproc);

pollout:

	/*
	 * Poll general cleanup code. Go back to all of the streams
	 * before the mark and reset the wakeup mechanisms that were
	 * set up during the poll.
	 */
	u.u_procp->p_flag &= ~SPOLL;
	fdp = (caddr_t)uap->fdp;
	for (i = 0, pfd = pollfd; i < mark; i++, pfd++) {
		j = i % NPOLLFILE;
		/*
		 * Read in next block of pollfds.  If the total number of
		 * pollfds is less than NPOLLFILE, don't bother because the
		 * pollfds of interest are still in the pollfd[] array.
		 */
		if (!j && (uap->nfds > NPOLLFILE)) {
			size = MIN(uap->nfds - i, NPOLLFILE) *
			    sizeof (struct pollfd);
			newu_error =
			    copyin((caddr_t)fdp, (caddr_t)pollfd,
				(u_int)size);
			if (newu_error) {
				u.u_error = newu_error;
				return;
			}
			fdp += size;
			pfd = pollfd;
		}

		if (pfd->fd < 0)
			continue;
		if ((fp = getf(pfd->fd)) == NULL)
			continue;
		if (fp->f_type != DTYPE_VNODE)
			continue;
		vp = (struct vnode *)fp->f_data;
		if (vp->v_type != VCHR || vp->v_stream == NULL)
			continue;
		pollreset(vp->v_stream);
	}
}

/*
 * Removes all event cells that refer to the current process in the
 * given stream's poll list.
 */
pollreset(stp)
register struct stdata *stp;
{
	register struct strevent *psep, *sep, *tmp;

	sep = stp->sd_pollist;
	psep = NULL;
	while (sep) {
		tmp = sep->se_next;
		if (sep->se_procp == u.u_procp) {
			if (psep)
				psep->se_next = tmp;
			else
				stp->sd_pollist = tmp;
			sefree(sep);
		}
		sep = tmp;
	}
	/*
	 * Recalculate pollflags
	 */
	stp->sd_pollflags = 0;
	for (sep = stp->sd_pollist; sep; sep = sep->se_next)
		stp->sd_pollflags |= sep->se_events;
}

/*
 * This function is placed in the callout table to time out a process
 * waiting on poll.  If the poll completes, this function is removed
 * from the table.  Its argument is a pointer to a variable which holds
 * the process table pointer for the process to be awakened.  This
 * variable is nulled to indicate that polltime ran.
 */
polltime(timeproc)
struct strevent *timeproc;
{
	register struct proc *p = timeproc->se_procp;

	if (p->p_wchan == (caddr_t)&pollwait) {
		setrun(p);
		timeproc->se_procp = NULL;
	}
}

int
pollsel(fp, events)
	struct file *fp;
	int events;
{
	int revents = 0;

	if ((events & POLLIN) &&
	    (*fp->f_ops->fo_select)(fp, FREAD)) {
		revents |= POLLIN;
	}
	if ((events & POLLOUT) &&
	    (*fp->f_ops->fo_select)(fp, FWRITE)) {
		revents |= POLLOUT;
	}
	if ((events & POLLPRI) &&
	    (*fp->f_ops->fo_select)(fp, 0)) {
		revents |= POLLPRI;
	}
	return (revents);
}
