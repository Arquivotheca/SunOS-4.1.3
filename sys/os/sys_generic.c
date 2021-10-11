#ident "@(#)sys_generic.c 1.1 92/07/30 SMI"	/* from UCB 5.42 83/06/24 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/stat.h>

#if defined(ASYNCHIO) && defined(LWP)
#include <lwp/lwperror.h>
#include <machlwp/stackdep.h>
#include <sys/asynch.h>
#include <sys/vnode.h>
static int nasynchio = 0;
#endif /* ASYNCHIO && LWP */


/*
 * Read system call.
 */
read()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		unsigned count;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov;

	aiov.iov_base = (caddr_t)uap->cbuf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	rwuio(&auio, UIO_READ);
}

readv()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		int	iovcnt;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov[16];		/* XXX */

	if (uap->iovcnt <= 0 || uap->iovcnt > sizeof (aiov)/sizeof (aiov[0])) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	u.u_error = copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	    (unsigned)(uap->iovcnt * sizeof (struct iovec)));
	if (u.u_error)
		return;
	rwuio(&auio, UIO_READ);
}

/*
 * Write system call
 */
write()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		int	count;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov;

	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = uap->cbuf;
	aiov.iov_len = uap->count;
	rwuio(&auio, UIO_WRITE);
}

writev()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		int	iovcnt;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov[16];		/* XXX */

	if (uap->iovcnt <= 0 || uap->iovcnt > sizeof (aiov)/sizeof (aiov[0])) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	u.u_error = copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	    (unsigned)(uap->iovcnt * sizeof (struct iovec)));
	if (u.u_error)
		return;
	rwuio(&auio, UIO_WRITE);
}

#if defined(ASYNCHIO) && defined(LWP)

#define	AIO_DONE 1

void adoit();
void arw();
void arwuio();
void cancelaio();
void del_aiodone();
void mark_unwanted();
void return_stk();
extern label_t *sleepqsave;

/* used to copy arguments onto thread stack */
struct auiotemp {
	struct uio auio;
	struct iovec aiov;
	struct file af;
};

aioread()
{
	arw(UIO_READ);
}

aiowrite()
{
	arw(UIO_WRITE);
}

int __LwpRunCnt;
int maxasynchio = 0;
int maxunreaped = 0;
int perproc_maxunreaped = 0;

/*
 * allocate a thread stack and initialize it to contain
 * an environment for adoit.
 */
void
arw(rw)
	enum uio_rw rw;
{
	register struct a {
		int	fildes;
		char	*bufp;
		u_int	bufsz;
		off_t	offset;
		int	whence;
		aio_result_t *resultp;
	} *uap = (struct a *)u.u_ap;
	struct auiotemp *at;
	register struct file *fp;
	struct proc *p = u.u_procp;
	register struct file *newfp;
	caddr_t sp;
	int s;
	extern stkalign_t *lwp_datastk();
	extern int perproc_maxasynchio;

	GETF(fp, uap->fildes);
	if ((fp->f_flag & (rw == UIO_READ ? FREAD : FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}
	s = splclock();
	if ((__LwpRunCnt >= maxasynchio) ||
	    (p->p_threadcnt >= perproc_maxasynchio) ||
	    (nasynchio >= maxunreaped) ||
	    (p->p_aio_count >= perproc_maxunreaped)) {
		splx(s);
		u.u_error = EAGAIN;
		return;
	}
	sp = (caddr_t)lwp_datastk((caddr_t)NULL,
	    sizeof (struct auiotemp), (caddr_t *)&at);
	splx (s);
	newfp = &(at->af);
	bcopy((caddr_t)fp, (caddr_t)newfp, sizeof (struct file));
	u.u_error = lseek1(newfp, uap->offset, uap->whence);
	if (u.u_error) {
		return_stk(sp);
		return;
	}

	/*
	 * The calls to fuword allow us to check whether the parameters are
	 * legal. This needs to be done before the lwp is forked.
	 */
	if (uap->bufp == (caddr_t)NULL ||
	    (fubyte(&uap->bufp[uap->bufsz - 1]) == -1)) {
		return_stk(sp);
		u.u_error = EFAULT;
		return;
	}
	at->aiov.iov_base = (caddr_t)uap->bufp;
	at->aiov.iov_len = uap->bufsz;
	at->auio.uio_iov = &(at->aiov);
	at->auio.uio_iovcnt = 1;
	if (fuword((caddr_t)uap->resultp) == -1) {
		return_stk(sp);
		u.u_error = EFAULT;
		return;
	}
	arwuio(at, rw, sp, uap->resultp, uap->fildes);
	if (u.u_error) {
		return_stk(sp);
		return;
	}
}

/*
 * do the uio for asynch IO.
 * This is equivalent to rwuio, but we also create a thread
 * to do the work asynchronously.
 */
void
arwuio(at, rw, sp, res, fd)
	struct auiotemp *at;
	enum uio_rw rw;
	caddr_t sp;
	aio_result_t *res;
	int fd;
{
	register struct uio *uio;
	register struct iovec *iov;
	register struct file *fp;
	register aiodone_t *ares;
	struct proc *p = u.u_procp;
	int i, count;
	int s;
	thread_t tid;
	aiodone_t *aprev = (aiodone_t *)NULL;
	extern void adoit();
	aiodone_t *aiodone_alloc();

	uio = &at->auio;
	uio->uio_resid = 0;
	uio->uio_segflg = UIO_USERSPACE;
	iov = uio->uio_iov;
	fp = &at->af;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		uio->uio_resid += iov->iov_len;
		if (uio->uio_resid < 0) {
			u.u_error = EINVAL;
			return;
		}
		iov++;
	}
	count = uio->uio_resid;
	uio->uio_offset = fp->f_offset;
	uio->uio_fmode = fp->f_flag;
	s = splclock();
	ares = aiodone_alloc();
	ares->aiod_fd = fd;
	ares->aiod_result = res;
	ares->aiod_next = (aiodone_t *)0;
	ares->aiod_state = AIO_INPROGRESS;
	if (p->p_aio_forw) {
		aprev = p->p_aio_back;
		p->p_aio_back->aiod_next = ares;
	} else
		p->p_aio_forw = ares;
	p->p_aio_back = ares;
	if (lwp_create(&tid, adoit, MINPRIO, LWPSUSPEND, sp,
	    5, fp, rw, count, uio, ares) < 0) {
		u.u_error = EAGAIN;
		del_aiodone(aprev, ares);
		(void) splx(s);
		return;
	}
	p->p_threadcnt++;	/* number of threads in this process */
	ares->aiod_thread = tid;
	/*
	 * u.u_procp could change so we give it special context
	 * before resuming it
	 */
	unixset(tid);
	(void) splx(s);
	(void) lwp_resume(tid);
}

/*
 * This is the thread that actually does the work of asynchronous IO.
 * We have copied the arguments to the thread on its stack so there
 * is no reliance on the invoker's stack.
 * The u-area this thread operates in is a dummy, but u.u_procp
 * is set on context switch, and the thread may safely read u values like
 * u.u_error. We currently do no accounting.
 */
void
adoit(fp, rw, count, uio, ares)
	struct file *fp;
	enum uio_rw rw;
	int count;
	register struct uio *uio;
	aiodone_t *ares;
{
	int err;
	int size;
	caddr_t addr;
	register struct proc *p = u.u_procp;
	int s;

	s = splclock();
	ares->aiod_state = AIO_INPROGRESS;
	nasynchio++;	/* total asynch io's in the system */
	(void) splx(s);

	if (setjmp(sleepqsave))
		err = EINTR;
	else
		err = (*fp->f_ops->fo_rw)(fp, rw, uio);

	s = splclock();
	addr = (caddr_t)(ares->aiod_result);
	size = count - uio->uio_resid;
	(void)suword(addr, size);	/* aio_return */
	(void)suword(addr + sizeof (int), err);	/* aio_errno */
	ares->aiod_state = AIO_DONE;
	p->p_aio_count++;
	wakeup((caddr_t)&p->p_aio_count);
	p->p_threadcnt--;
	psignal(p, SIGIO);
	(void) splx(s);

	/*
	 * When the thread returns, it returns to lwpkill for cleanup
	 */
}

/*
 * Cancel a pending asynchronous IO.
 * We don't sleep if we can't do it.
 */
aiocancel()
{
	register struct a {
		aio_result_t *resultp;
	} *uap = (struct a *)u.u_ap;
	register aiodone_t *ares;
	register struct aio_result_t *res;
	struct proc *p = u.u_procp;
	int s;

	if (fuword((caddr_t)uap->resultp) == -1) {
		u.u_error = EFAULT;
		return;
	}
	res = uap->resultp;
	s = splclock();
	ares = p->p_aio_forw;
	while (ares && (ares->aiod_result != res)) {
		ares = ares->aiod_next;
	}
	if (ares == (aiodone_t *)0) {
		u.u_error = EINVAL;
		(void) splx(s);
		return;
	}

	/*
	 * If in progress, then state won't change
	 * because we are at clock. If we are done
	 * then just return.
	 */
	if (ares->aiod_state == AIO_INPROGRESS)
		mark_unwanted(ares->aiod_thread);
	else u.u_error = EACCES;
	(void)splx(s);
}

/*
 * Wait for any asynchronous IO to complete.
 */
aiowait()
{
	register struct a {
		struct timeval *tv;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
	register aiodone_t *ares = NULL;
	register aiodone_t *prev = NULL;
	struct timeval atv;
	int unawait();
	label_t lqsave;
	int s;

	if (uap->tv) {
		if ((u.u_error = copyin((caddr_t)uap->tv, (caddr_t)&atv,
			sizeof (atv))))
			return;
		if (itimerfix(&atv)) {
			u.u_error = EINVAL;
			return;
		}
		/* atv is relative amount of time to wait from current time */
		s = splclock();
		timevaladd(&atv, &time);
		(void) splx(s);
	}

	/*
	 * Exit if timer expires or asynch i/o completes; threadcnt is set to 1
	 * fork
	 */
	s = splclock();
	while (p->p_aio_count == 0 && nasynchio != 0) {
		if (uap->tv && (time.tv_sec > atv.tv_sec ||
		    time.tv_sec == atv.tv_sec && time.tv_usec >= atv.tv_usec)) {
			/* time expired */
			goto out;
		}
		if (uap->tv) {
			lqsave = u.u_qsave;
			if (setjmp(&u.u_qsave)) {
				untimeout(unawait, (caddr_t)u.u_procp);
				u.u_error = EINTR;
				goto out;
			}
			timeout(unawait, (caddr_t)u.u_procp, hzto(&atv));
		}
		(void) sleep((caddr_t)&p->p_aio_count, PZERO);
		if (uap->tv) {
			u.u_qsave = lqsave;
			untimeout(unawait, (caddr_t)u.u_procp);
		}

	}
	ares = p->p_aio_forw;
	while ((ares != (aiodone_t *)NULL) && ares->aiod_state != AIO_DONE) {
		prev = ares;
		ares = ares->aiod_next;
	}
	if (ares == (aiodone_t *)NULL) {
		goto out;
	}
	u.u_r.r_val1 = (int)ares->aiod_result;
	p->p_aio_count--;
	nasynchio--;
	del_aiodone(prev, ares);
	(void) splx(s);
	return;
out:
	if (p->p_aio_count == 0) {
		if (p->p_aio_forw == NULL) {
			u.u_error = EINVAL;	/* No outstanding asynch. I/O */
			u.u_r.r_val1 = -1;	/* Ugly, but specified */
		} else
			u.u_r.r_val1 = 0;

		(void) splx(s);
		return;
	}
	(void) splx(s);
}

unawait(p)
	register struct proc *p;
{
	register int s = splhigh();


	switch (p->p_stat) {

	case SSLEEP:
		setrun(p);
		break;

	case SSTOP:
		unsleep(p);
		break;
	}
	(void) splx(s);
}


void
astop(fd, res)
	int fd;
	struct aio_result_t *res;
{
	aiodone_t *ares = (aiodone_t *)NULL;
	aiodone_t *prev = (aiodone_t *)NULL;
	struct proc *p = u.u_procp;
	int pri;
	extern int __Nrunnable;

	for (;;) {
		pri = splclock(); /* hold splclock since we won't be blocking */
		if ((ares = p->p_aio_forw) == (aiodone_t *)0) {
			if (p->p_aio_count != 0)
				panic("astop: bad p_aio_count");
			else {
				(void) splx(pri);
				return;
			}
		}
		if ((res == (struct aio_result_t *)ALL_AIO) &&
		    (fd != ares->aiod_fd) && (fd != ALL_AIO)) {
			while (fd != ares->aiod_fd) {
				prev = ares;
				if ((ares = ares->aiod_next) ==
				    (aiodone_t *)NULL) {
					(void) splx(pri);
					return;
				}
			}
		}
		if (ares->aiod_state == AIO_INPROGRESS)
			cancelaio(ares);

		/*
		 * The decrementing of aio_count and nasynchio is also
		 * done in await
		 */
		p->p_aio_count--;
		--nasynchio;
		del_aiodone(prev, ares);
		(void) splx(pri);
	}
}

static aiodone_t *aiodone_freelst;
static int aiodone_freelst_cnt;
aiodone_t *
aiodone_alloc()
{
	aiodone_t *aio;
	if ((aio = aiodone_freelst) == NULL) {
	    aio  = (aiodone_t *)new_kmem_alloc(sizeof (aiodone_t),
		KMEM_SLEEP);
	} else {
		aiodone_freelst = aiodone_freelst->aiod_next;
		aiodone_freelst_cnt--;
		aio->aiod_next = 0;
	}
	return (aio);
}

void
aiodone_free(aio)
	aiodone_t *aio;
{
	if (aiodone_freelst_cnt < maxasynchio) {
		aio->aiod_next = aiodone_freelst;
		aiodone_freelst = aio;
		aiodone_freelst_cnt++;
	} else
		kmem_free((caddr_t)aio, (u_int)sizeof (aiodone_t));
}

/*
 * It is assumed that the next 2 routines are called
 * at splclock
 */
void
cancelaio(ares)
	aiodone_t *ares;
{
	struct proc *p = u.u_procp;

	/*
	 * Thread is marked the first time; if sleeping
	 * at interruptable pri. then cancelled else it
	 * was a uninterruptable pri. and will finish.
	 */
	(void) mark_unwanted(ares->aiod_thread);
	while (ares->aiod_state == AIO_INPROGRESS)
		(void)sleep((caddr_t)&p->p_aio_count, PZERO - 1);
}

void
del_aiodone(prev, ares)
	register aiodone_t *prev;
	register aiodone_t *ares;
{
	struct proc *p = u.u_procp;
	void aiodone_free();

	if (prev == (aiodone_t *)NULL)  /* first element */
		p->p_aio_forw = ares->aiod_next;
	else
		prev->aiod_next = ares->aiod_next;
	if (ares->aiod_next == (aiodone_t *)NULL) /* last element */
		p->p_aio_back = prev;
	aiodone_free(ares);
}

#endif /* ASYNCHIO && LWP */

rwuio(uio, rw)
	register struct uio *uio;
	enum uio_rw rw;
{
	struct a {
		int	fdes;
	};
	register struct file *fp;
	register struct iovec *iov;
	int i, count;

	GETF(fp, ((struct a *)u.u_ap)->fdes);
	if ((fp->f_flag & (rw == UIO_READ ? FREAD : FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}
	uio->uio_resid = 0;
	uio->uio_segflg = UIO_USERSPACE;
	iov = uio->uio_iov;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		uio->uio_resid += iov->iov_len;
		if (uio->uio_resid < 0) {
			u.u_error = EINVAL;
			return;
		}
		iov++;
	}
	count = uio->uio_resid;
	uio->uio_offset = fp->f_offset;
	uio->uio_fmode = fp->f_flag;
	if (setjmp(&u.u_qsave)) {
		if (uio->uio_resid == count) {
			if ((u.u_sigintr & sigmask(u.u_procp->p_cursig)) != 0)
				u.u_error = EINTR;
			else
				u.u_eosys = RESTARTSYS;
		}
	} else
		u.u_error = (*fp->f_ops->fo_rw)(fp, rw, uio);
	u.u_r.r_val1 = count - uio->uio_resid;
	/*
	 * Check if the underlying code has switched us to block mode
	 */
	if ((uio->uio_fmode & FSETBLK) && !(fp->f_flag & FSETBLK)) {
		fp->f_flag |= FSETBLK;
		fp->f_offset = btodb(fp->f_offset);
	}
	/* update file offset only if there is no error */
	if (u.u_error == 0) {
		fp->f_offset += (fp->f_flag & FSETBLK) ?
		    howmany(u.u_r.r_val1, DEV_BSIZE) : u.u_r.r_val1;
	}
	u.u_ioch += (unsigned)u.u_r.r_val1;
}

/*
 * Ioctl system call
 */
ioctl()
{
	register struct file *fp;
	struct a {
		int	fdes;
		int	cmd;
		caddr_t	cmarg;
	} *uap;
	register int com;
	register u_int size;
	int data[howmany(_IOCPARM_MASK/2, sizeof (int))];
	register caddr_t iocparm;

	uap = (struct a *)u.u_ap;
	GETF(fp, uap->fdes);
	if ((fp->f_flag & (FREAD|FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}
	com = uap->cmd;

#if defined(vax) && defined(COMPAT)
	/*
	 * Map old style ioctl's into new for the
	 * sake of backwards compatibility (sigh).
	 */
	if ((com&~0xffff) == 0) {
		com = mapioctl(com);
		if (com == 0) {
			u.u_error = EINVAL;
			return;
		}
	}
#endif
	if (com == FIOCLEX) {
		u.u_pofile[uap->fdes] |= UF_EXCLOSE;
		return;
	}
	if (com == FIONCLEX) {
		u.u_pofile[uap->fdes] &= ~UF_EXCLOSE;
		return;
	}

	/*
	 * Interpret high order word to find
	 * amount of data to be copied to/from the
	 * user's address space.
	 */
	size = (com &~ (_IOC_INOUT|_IOC_VOID)) >> 16;
	if (size <= sizeof (data))
		/*
		 * If size is less than IOCPARM_MASK/2, use data[]
		 * to avoid overheads from kmem_alloc and kmem_free.
		 */
		iocparm = (caddr_t)data;
	else if (size <= _IOCPARM_MASK)
		/*
		 * Get space from kmem_alloc if parameter size is more
		 * than IOCPARM_MASK / 2 to avoid kernel stack overflow.
		 */
		iocparm = new_kmem_alloc(size, KMEM_SLEEP);
	else {
		/*
		 * Size > IOCPARM_MASK, not possible to detect with
		 * the current ioctl macros.
		 */
		u.u_error = EINVAL;
		return;
	}
	if (com&_IOC_IN) {
		if (size == sizeof (int) && uap->cmarg == NULL)
			*(int *)iocparm = 0;
		else if (size != 0) {
			u.u_error = copyin(uap->cmarg, iocparm, size);
			if (u.u_error)
				return;
		} else
			*(caddr_t *)iocparm = uap->cmarg;
	} else if ((com&_IOC_OUT) && size)
		/*
		 * Zero the buffer on the stack so the user
		 * always gets back something deterministic.
		 */
		bzero(iocparm, size);
	else if (com&_IOC_VOID)
		*(caddr_t *)iocparm = uap->cmarg;

	if (setjmp(&u.u_qsave)) {
		u.u_error = EINTR;
		goto done;
	} else
		u.u_error = (*fp->f_ops->fo_ioctl)(fp, com, iocparm);
	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (u.u_error == 0) {
		/*
		 * XXX Actually, there should not be any connection
		 * between the "ioctl"-settable per-object FIONBIO
		 * and FASYNC flags and any per-file-descriptor flags,
		 * so we shouldn't have to do this.
		 * Unfortunately, 4.2BSD has such a connection, so we
		 * must support that.  Thus, we must set or clear the
		 * FNDELAY or FASYNC flag on the file descriptor.
		 * In order to do this right, it should really hit
		 * every file descriptor that refers to the same
		 * object, but that's too expensive.
		 */
		switch (com) {

		case FIONBIO:
			if (*(int *)iocparm)
				fp->f_flag |= FNDELAY;
			else
				fp->f_flag &= ~FNDELAY;
			break;

		case FIOASYNC:
			if (*(int *)iocparm)
				fp->f_flag |= FASYNC;
			else
				fp->f_flag &= ~FASYNC;
			break;
		}
		if ((com&_IOC_OUT) && size)
			u.u_error = copyout(iocparm, uap->cmarg, size);
	}
done:
	if (iocparm != (caddr_t)data)
		kmem_free(iocparm, size);
}

int	unselect();
int	nselcoll;

/*
 * Select system call.
 */
select()
{
	register struct uap  {
		int	nd;
		fd_set	*in, *ou, *ex;
		struct	timeval *tv;
	} *uap = (struct uap *)u.u_ap;
	fd_set ibits[3], obits[3];
	struct timeval atv;
	int s, ncoll, ni;
	label_t lqsave;
	int rwe_flag = 0;

	bzero((caddr_t)ibits, sizeof (ibits));
	bzero((caddr_t)obits, sizeof (obits));
	if (uap->nd < 0 || uap->nd > NOFILE)
		uap->nd = NOFILE;	/* forgiving, if slightly wrong */
	ni = howmany(uap->nd, NFDBITS);

#define	getbits(name, x) \
	if (uap->name) { \
		rwe_flag |= (1<<x); \
		u.u_error = copyin((caddr_t)uap->name, (caddr_t)&ibits[x], \
		    (unsigned)(ni * sizeof (fd_mask))); \
		if (u.u_error) \
			goto done; \
	}
	getbits(in, 0);
	getbits(ou, 1);
	getbits(ex, 2);
#undef	getbits

	if (uap->tv) {
		u.u_error = copyin((caddr_t)uap->tv, (caddr_t)&atv,
			sizeof (atv));
		if (u.u_error)
			goto done;
		if (itimerfix(&atv)) {
			u.u_error = EINVAL;
			goto done;
		}
		s = splclock(); timevaladd(&atv, &time); (void) splx(s);
	}
retry:
	ncoll = nselcoll;
	u.u_procp->p_flag |= SSEL;
	u.u_r.r_val1 = selscan(ibits, obits, uap->nd, rwe_flag);
	if (u.u_error || u.u_r.r_val1)
		goto done;
	s = splhigh();
	/* this should be timercmp(&time, &atv, >=) */
	if (uap->tv && (time.tv_sec > atv.tv_sec ||
	    time.tv_sec == atv.tv_sec && time.tv_usec >= atv.tv_usec)) {
		(void) splx(s);
		goto done;
	}
	if ((u.u_procp->p_flag & SSEL) == 0 || nselcoll != ncoll) {
		u.u_procp->p_flag &= ~SSEL;
		(void) splx(s);
		goto retry;
	}
	u.u_procp->p_flag &= ~SSEL;
	if (uap->tv) {
		lqsave = u.u_qsave;
		if (setjmp(&u.u_qsave)) {
			untimeout(unselect, (caddr_t)u.u_procp);
			u.u_error = EINTR;
			(void) splx(s);
			goto done;
		}
		timeout(unselect, (caddr_t)u.u_procp, hzto(&atv));
	}
	(void) sleep((caddr_t)&selwait, PZERO+1);
	if (uap->tv) {
		u.u_qsave = lqsave;
		untimeout(unselect, (caddr_t)u.u_procp);
	}
	(void) splx(s);
	goto retry;
done:
#define	putbits(name, x) \
	if (uap->name) { \
		int error = copyout((caddr_t)&obits[x], (caddr_t)uap->name, \
		    (unsigned)(ni * sizeof (fd_mask))); \
		if (error) \
			u.u_error = error; \
	}
	if (u.u_error == 0) {
		putbits(in, 0);
		putbits(ou, 1);
		putbits(ex, 2);
#undef putbits
	}
}

unselect(p)
	register struct proc *p;
{
	register int s = splhigh();

	switch (p->p_stat) {

	case SSLEEP:
		setrun(p);
		break;

	case SSTOP:
		unsleep(p);
		break;
	}
	(void) splx(s);
}

selscan(ibits, obits, nfd, rwe_flag)
	register fd_set *ibits;
	fd_set *obits;
	int nfd;
	int rwe_flag;
{
	register fd_mask bits;
	register int which, j, i;
	struct file *fp;
	int n = 0;
	static int which_flag[3] = { FREAD, FWRITE, 0 };

	for (which = 0; which < 3; which++) {
		if (!(rwe_flag & (1 << which)))
			continue;
		for (i = 0; i < nfd; i += NFDBITS) {
			bits = ibits[which].fds_bits[i/NFDBITS];
			while (bits && (j = ffs(bits)) && i + --j < nfd) {
				register int fd;

				bits &= ~(1 << j);
				fd = i + j;
				fp = (struct file *)getf(fd);
				if (fp == NULL) {
					/* u.u_error has been set. */
					break;
				}
				if ((*fp->f_ops->fo_select)(fp,
				    which_flag[which])) {
					FD_SET(fd, &obits[which]);
					n++;
				}
			}
		}
	}
	return (n);
}

/*ARGSUSED*/
seltrue(dev, flag)
	dev_t dev;
	int flag;
{

	return (1);
}

selwakeup(p, coll)
	register struct proc *p;
	int coll;
{

	if (coll) {
		nselcoll++;
		wakeup((caddr_t)&selwait);
	}
	if (p) {
		int s = splhigh();
		if (p->p_wchan == (caddr_t)&selwait) {
			if (p->p_stat == SSLEEP)
				setrun(p);
			else
				unsleep(p);
		} else if (p->p_flag & SSEL)
			p->p_flag &= ~SSEL;
		(void) splx(s);
	}
}
