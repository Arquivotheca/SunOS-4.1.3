/*	@(#)str_io.c 1.1 92/07/30 SMI; from S5R3.1 os/streamio.c 10.17.3.4	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
 */

/*
 * Streams I/O interface.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/ttold.h>
#include <sys/filio.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strstat.h>
#include <sys/poll.h>
#include <sys/debug.h>
#include <sys/syslog.h>
#include <sys/session.h>
#include <specfs/snode.h>

#define	HZ		hz

/*
 * id value used to distinguish between different ioctl messages
 */
static long ioc_id;


/*
 *  Qinit structure and Module_info structures
 *	for stream head read and write queues
 */
static int	strrput(/*queue_t *q, mblk_t *bp*/);
static int	strwsrv(/*queue_t *q*/);

static struct module_info strm_info = {
	0,
	"strrhead",
	0,
	INFPSZ,
	STRHIGH,
	STRLOW
};

static struct module_info stwm_info = {
	0,
	"strwhead",
	0,
	0,
	0,
	0
};

struct	qinit strdata = {
	strrput,
	NULL,
	NULL,
	NULL,
	NULL,
	&strm_info,
	NULL
};

struct	qinit stwdata = {
	NULL,
	strwsrv,
	NULL,
	NULL,
	NULL,
	&stwm_info,
	NULL
};

static void	assign_ctty(/*struct vnode *vp,
    struct stdata *stp, int force*/);
static void	strdismantle(/*struct stdata *stp, int flag*/);
void		strclean(/*struct vnode *vp*/);
static void	strsendsig(/*struct strevent *siglist, int event*/);
static void	strwakepoll(/*struct stdata *stp, int events*/);
static void	dopush(/*struct stdata *stp, char *mname, struct vnode *vp,
    int waitflag, int oflag*/);
static void	initialize_ldtab(/*void*/);
static queue_t	*getcompatqueue(/*struct stdata *stp*/);
static char	*getmname(/*queue_t *q*/);
static void	strdoioctl(/*struct stdata *stp, struct strioctl *strioc,
    int copyflg, int flag*/);
static int	qattach(/*struct streamtab *qinfo, queue_t *qp, dev_t dev,
    int oflag, int sflag*/);
static int	qdetach(/*queue_t *qp, int clmode, int flag*/);
static int	strtime(/*struct stdata *stp*/);
static int	str2time(/*struct stdata *stp*/);
static int	str3time(/*struct stdata *stp*/);
static int	putiocd(/*mblk_t *bp, caddr_t arg, int copymode*/);
static int	getiocd(/*mblk_t *bp, caddr_t arg, int copymode*/);
static struct linkblk *alloclink(/*queue_t *qup, queue_t *qdown, int index*/);
static int	linkcycle(/*queue_t *qup, queue_t *qdown*/);
static int	munlinkone(/*struct stdata *stp, struct file *fpdown,
    struct linkblk *linkblkp, int cflag*/);
static void	munlinkall(/*struct stdata *stp, int cflag*/);
void	setq(/*queue_t *rq, struct qinit *rinit,
    struct qinit *winit*/);
mblk_t		*strmakemsg(/*struct strbuf *mctl, int count,
    struct uio *uio, int wroff, long flag, int nowait*/);
int		strwaitbuf(/*int size, int pri, int ncalls*/);
void	strunbcall(/*int size, int pri, struct proc *p*/);
static int	strwaitq(/*struct stdata *stp, int flag, int fmode*/);
static int	strvtime(/*struct stdata *stp*/);

/*
 * Open a stream device.  Sflag may be 0 or CLONEOPEN.
 * Return zero on success, or errno value on failure.
 * The first argument is a pointer to a vnode pointer
 * because clone opens require a new vnode to replace
 * the original one.
 */
int
stropen(vpp, oflag, sflag)
	struct vnode **vpp;
	int oflag, sflag;
{
	register struct vnode *vp = *vpp;
	register struct stdata *stp;
	register queue_t *qp;
	register int unit;
	int newctty;
	char **modlp;
	char *name;
	int sverrno;
	register int s;

	ASSERT(sflag == 0 || sflag == CLONEOPEN);

	/*
	 * Clone opens will result either in failure or in a fresh
	 * device instance with no superstructure of pushed modules.
	 * Since the vnode we have in hand doesn't match the one that
	 * will result from cloning, it's inappropriate to go through
	 * the code below, which assumes that the original vnode will
	 * remain valid.  The primary consequence is that this code
	 * can't handle an extension to stream semantics that allows
	 * modules to remain pushed after the last close.
	 */
	if (sflag == CLONEOPEN)
		goto forcedclone;

	if (!(vp->v_stream)) {
		/*
		 * This vnode isn't streaming, but another vnode
		 * may refer to same device, so look for it in snode
		 * table to avoid building 2 streams to 1 device.
		 */
		register struct vnode *othervp;

		if ((othervp = other_specvp(vp)) != NULL)
			vp->v_stream = othervp->v_stream;
	}

	/*
	 * If the stream already exists, wait for any open in progress
	 * to complete then call the open function of each module and
	 * driver in the stream.  Otherwise create the stream.
	 */
retry:
	if (stp = vp->v_stream) {
		int error;
		int strhup_set = 0;

		/*
		 * Waiting for stream to be created to device
		 * due to another open, or for it to be dismantled
		 * due to a close in progress.
		 */
		if (stp->sd_flag & (STWOPEN|STRCLOSE)) {
			if (sleep((caddr_t)stp, STOPRI|PCATCH))
				return (EINTR);
			/*
			 * We can't assume anything about the stream;
			 * the open could have failed or cloned to a
			 * new instance.
			 */
			goto retry;
		}
		if (stp->sd_flag & STRERR)
			return (EIO);

		/*
		 * XXX - Allow opens of a stream marked as STRHUP
		 * to go through if and only if it is no longer a
		 * controlling TTY for some process.  (e.g., this
		 * will occur when a user spawns off background
		 * processes and hangs up a dial-in line)
		 */
		if (stp->sd_flag & STRHUP) {
			if (stp->sd_pgrp == 0)
				strhup_set = 1;
			else
				return (EIO);
		}
		s = splstr();
		stp->sd_flag |= STWOPEN;
		(void) splx(s);

		/*
		 * Open all modules and devices down stream to notify
		 * that another user is streaming.
		 * For modules, set the last argument to MODOPEN and
		 * do not pass any open flags.
		 * Note that this code makes no provision for cloned
		 * opens.  (This means that we can't currently handle
		 * modules that remain pushed after the last close.)
		 */
		qp = stp->sd_wrq;
		error = 0;
		while (qp = qp->q_next) {
			if (qp->q_flag & QREADR)
				break;
			unit = (*RD(qp)->q_qinfo->qi_qopen)(RD(qp), vp->v_rdev,
					(qp->q_next ? 0 : oflag),
					(qp->q_next ? MODOPEN : sflag));
			if (unit == OPENFAIL) {
				error = u.u_error ? u.u_error : ENXIO;
				break;
			}

			/*
			 * If the driver asked to have this made the
			 * controlling TTY, do so if we're really supposed to.
			 */
			if (!(oflag & FNOCTTY) && (unit & NEWCTTY))
				assign_ctty(vp, stp, 0);
		}
		/*
		 * XXX - If all modules and devices on a stream with
		 * STRHUP set were successfully opened, clear it, as
		 * the stream is no longer in that state.
		 */
		if (strhup_set) {
			if (error == 0) {
				s = splstr();
				stp->sd_flag &= ~STRHUP;
				(void) splx(s);
			} else {
				/*
				 * If an error occurred and this is the "last"
				 * open reference, dismantle the stream
				 * after clearing the STWOPEN flag.
				 */
				if (!stillopen(VTOS(vp)->s_dev, vp->v_type)) {
					s = splstr();
					stp->sd_flag &= ~STWOPEN;
					(void) splx(s);
					strclose(vp, oflag);
					return (error);
				}
			}
		}
		s = splstr();
		stp->sd_flag &= ~STWOPEN;
		(void) splx(s);
		wakeup((caddr_t)stp);
		return (error);
	}

forcedclone:
	/*
	 * Not already streaming so create a stream to driver
	 */

	if (!(qp = allocq()))
		return (ENOSR);

	if (!(stp = allocstr())) {
		freeq(qp);
		return (ENOSR);
	}

	/*
	 * Initialize stream head.  The initialization can't be
	 * complete because we still don't have the right vnode
	 * to cross-link to.
	 */
	s = splstr();
	stp->sd_wrq = WR(qp);
	stp->sd_flag = STWOPEN;
	stp->sd_siglist = NULL;
	stp->sd_pollist = NULL;
	stp->sd_sigflags = 0;
	stp->sd_pollflags = 0;
	(void) splx(s);
	stp->sd_pgrp = 0;
	stp->sd_sigio = 0;
	stp->sd_vmin = -1;	/* vmin processing initially disabled */
	stp->sd_vtime = 0;
	stp->sd_error = 0;
	stp->sd_wroff = 0;
	stp->sd_iocwait = 0;
	stp->sd_iocblk = NULL;
	stp->sd_pushcnt = 0;
	stp->sd_selr = NULL;
	stp->sd_selw = NULL;
	stp->sd_sele = NULL;
	setq(qp, &strdata, &stwdata);
	qp->q_ptr = WR(qp)->q_ptr = (caddr_t)stp;
	stp->sd_strtab = cdevsw[major(vp->v_rdev)].d_str;

	/*
	 * XXX - The `v_stream' field may be valid for CLONEOPENS and must
	 * not be unconditionally reset before the device open actually
	 * completes.  This, however, may result in a race leading to the
	 * creation of more than one stream to the device if it's open
	 * routine sleeps.  The "other_specvp()" routine cannot prevent
	 * this since it will either find a vnode with a NULL `v_stream'
	 * field if the normal open has not completed, or fail if CLONEOPEN
	 * has not completed (since the new vnode hasn't been created).
	 *
	 * The correct way of solving this is to block all "stropen()" calls
	 * to the device which hasn't completed the CLONEOPEN.  This requires
	 * processes to sleep on a data structure that uniquely represents
	 * the device.  Since no such structure exists, the fix is to set
	 * `v_stream' only if it isn't valid.
	 */
	if (!(vp->v_stream))
		vp->v_stream = stp;

	/*
	 * Open driver and create stream to it (via qattach).  Device opens
	 * may sleep, but must set PCATCH if they do so that signals will
	 * not cause a longjump.  Failure to do this may result in the queues
	 * and stream head not being freed.
	 *
	 * Note that the device's open routine has no access to the
	 * associated vnode, since it doesn't necessarily yet exist.
	 * (If we're cloning, the original vnode is inappropriate.)
	 */
	unit = qattach(stp->sd_strtab, qp, vp->v_rdev, oflag, sflag);

	if (unit == OPENFAIL)
		goto openfail;

	/*
	 * If the module requested that this be made a controlling
	 * TTY, note that fact and clear out the special flag.
	 */
	if (unit & NEWCTTY) {
		newctty = 1;
		unit &= ~NEWCTTY;
	} else
		newctty = 0;

	/*
	 * Get a vnode for the device instance we've just opened
	 * and cross-link it with the stream head.  A cloned instance
	 * requires a fresh vnode.
	 */
	if (sflag == CLONEOPEN) {
		register dev_t newdev = makedev(major(vp->v_rdev), unit);
		register struct vnode *nvp = makespecvp(newdev, vp->v_type);

		VN_RELE(vp);
		*vpp = vp = nvp;
	}
	stp->sd_vnode = vp;
	vp->v_stream = stp;

	/*
	 * If the driver asked to have this made the controlling TTY,
	 * do so if we're really supposed to.
	 */
	if (newctty && !(oflag & FNOCTTY))
		assign_ctty(vp, stp, 0);

	/*
	 * If this driver's streamtab specifies a list of modules to
	 * automatically push, do so.
	 */
	modlp = cdevsw[major(vp->v_rdev)].d_str->st_modlist;
	if (modlp != NULL) {
		while ((name = *modlp++) != NULL) {
			dopush(stp, name, vp, 0, oflag);
			if ((sverrno = u.u_error) != 0) {
				s = splstr();
				stp->sd_flag |= (STRHUP | STRCLOSE);
				stp->sd_flag &= ~STWOPEN;
				(void) splx(s);
				/*
				 * Processes waiting for this stream
				 * will be woken up only after it is
				 * dismantled.
				 */
				strdismantle(stp, oflag);
				u.u_error = sverrno;
				return (u.u_error);
			}
		}
	}

	/*
	 * Wake up others that are waiting for stream to be created.
	 */
	s = splstr();
	stp->sd_flag &= ~STWOPEN;
	(void) splx(s);
	wakeup((caddr_t)stp);
	return (0);

openfail:
	s = splstr();
	stp->sd_flag |= STRHUP;
	stp->sd_flag &= ~STWOPEN;
	(void) splx(s);
	vp->v_stream = NULL;
	wakeup((caddr_t)stp);
	stp->sd_vnode = NULL;
	freestr(stp);
	freeq(qp);
	return (u.u_error ? u.u_error : ENXIO);
}

/*
 * Assign the stream referred to by "stp", whose controlling vnode is
 * "vp", as the controlling TTY for this process, if the device or module
 * so requested (by returning a value with NEWCTTY set).
 */
static void
assign_ctty(vp, stp, force)
	register struct vnode *vp;
	register struct stdata *stp;
{
	register struct proc *p;
	struct sess *isactty();
	register struct sess *sp;

	/*
	 * All processes must satisfy the following criteria before
	 * getting a ctty:
	 *	1) The process must be a session leader.
	 *	2) The session must not have a ctty.
	 *	3) The tty can't be a ctty (i.e., in a session) already.
	 *
	 * We make sure that they will by handing out sessions if they
	 * do a setpgrp(0, 0) or an ioctl TIOCNOTTY and by making every
	 * child of init a session leader.
	 */
	if ((p = u.u_procp)->p_pid == 0)
		return;
	/*
	 * We have a problem as follows:  A pre 4.4BSD program may assume
	 * (more or less legitimately) that its pgrp is 0 and that an open
	 * of a tty will give it a ctty.  The hack here is that we have
	 * an unsetsid() system call that puts you back in session 0.
	 * If you are in sess 0 (pgrp == 0) then I'll call setsid()
	 * for you and give you a ctty.
	 */
	if (p->p_pgrp == 0 && !isactty(vp)) {
		(void) setsid(SESS_NEW);
	}
	if (p->p_pid != p->p_sessp->s_sid || p->p_sessp->s_vp)
		return;
	/*
	 * XXX - this kludge is to allow the tty to be ripped away from someone
	 * else.  force comes from TIOCSCTTY which getty calls to get the
	 * ctty.
	 */
	if (sp = isactty(vp)) {
		if (!force)
			return;
		SESS_VN_RELE(sp);
	}
	VN_HOLD(vp);
	stp->sd_pgrp = p->p_pgrp;
	p->p_sessp->s_ttyp = &stp->sd_pgrp;
	p->p_sessp->s_vp = vp;
	p->p_sessp->s_ttyd = vp->v_rdev;
}


/*
 * Close a stream.
 * This is called from closef() on the last close of an open stream.
 * Strclean() will already have removed the siglist and pollist
 * information, so all that remains is to remove all multiplexor links
 * for the stream, pop all the modules (and the driver), and free the
 * stream structure.
 */
strclose(vp, flag)
	struct vnode *vp;
{
	register struct stdata *stp;
	register struct vnode *othervp;

	ASSERT(vp->v_stream);

	stp = vp->v_stream;
	/*
	 * XXX - Do not dismantle the stream if STWOPEN is set
	 * since this implies that some process is waiting in
	 * a driver open routine (e.g., for carrier).  The stream
	 * will eventually be dismantled by that process.
	 */
	if (stp->sd_flag & STWOPEN)
		return;

	if ((othervp = other_specvp(vp)) != NULL) {
		/*
		 * Another vnode refers to this device.  If "vp" currently
		 * "owns" this stream, make that other vnode "own" it, since
		 * "vp" presumably wants nothing more to do with it.
		 */
		if (stp->sd_vnode == vp)
			stp->sd_vnode = othervp;
		vp->v_stream = NULL;
		wakeup((caddr_t)stp);
	} else {
		/*
		 * This is the last vnode that refers to this stream.
		 * Shut it down by ripping out all transient links under it and
		 * dismantling it.
		 * And don't forget to take the ctty away from the session.
		 */
		register int s;

		s = splstr();
		stp->sd_flag |= STRCLOSE;
		(void) splx(s);
		munlinkall(stp, 1);
		strdismantle(stp, flag);
		if (vp == u.u_procp->p_sessp->s_vp)
			SESS_VN_RELE(u.u_procp->p_sessp);
	}
}

/*
 * Dismantle a stream.
 * This is called by "stropen" if an auto-push fails, or by "strclose".
 * Pop all the modules (and the driver), and free the stream structure.
 */
static void
strdismantle(stp, flag)
	register struct stdata *stp;
{
	register s;
	register queue_t *qp;
	register mblk_t *mp, *tmp;
	int strtime();
	register struct vnode *vp;

	ASSERT(stp->sd_flag & STRCLOSE);

	qp = stp->sd_wrq;
	while (qp->q_next) {
		if (!(flag & (FNDELAY | FNBIO | FNONBIO))) {
			s = splstr();
			stp->sd_flag |= STRTIME | WSLEEP;
			timeout(strtime, (caddr_t)stp, STRTIMOUT*HZ);
			/*
			 * Sleep until awakened by strwsrv() or strtime();
			 * i.e., until something is removed from the
			 * queue below the stream head or the timeout
			 * expires.
			 */
			while ((stp->sd_flag & STRTIME) &&
			    qp->q_next->q_count) {
				stp->sd_flag |= WSLEEP;
				/* ensure strwsrv gets enabled */
				qp->q_next->q_flag |= QWANTW;
				if (sleep((caddr_t)qp, STIPRI | PCATCH))
					break;
			}
			untimeout(strtime, (caddr_t)stp);
			stp->sd_flag &= ~(STRTIME | WSLEEP);
			(void) splx(s);
		}
		qdetach(RD(qp->q_next), 1, flag);
	}
	flushq(qp, FLUSHALL);
	qp = RD(qp);
	s = splstr();
	mp = qp->q_first;
	qp->q_first = qp->q_last = NULL;
	qp->q_count = 0;

	if (stp->sd_iocblk) {
		freemsg(stp->sd_iocblk);
		stp->sd_iocblk = NULL;
	}
	vp = stp->sd_vnode;
	ASSERT(vp->v_stream == stp);
	stp->sd_vnode = NULL;

	/* free stream head queue pair */
	freeq(qp);
	(void) splx(s);

	while (mp) {
		tmp = mp->b_next;
		if (mp->b_datap->db_type == M_PASSFP)
			closef(((struct strrecvfd *)mp->b_rptr)->f.fp);
		freemsg(mp);
		mp = tmp;
	}
	s = splstr();
	vp->v_stream = NULL;
	stp->sd_flag &= ~STRCLOSE;
	(void) splx(s);
	wakeup((caddr_t)stp);
	freestr(stp);
}

/*
 * Clean up after a process when it closes a stream.  This is called
 * from closef for all closes, whereas strclose is called only for the
 * last close on a stream.  The pollist and siglist are scanned for entries
 * for the current process, and these are removed.
 */
void
strclean(vp)
	struct vnode *vp;
{
	register struct strevent *sep, *psep, *tsep;
	struct stdata *stp;

	stp = vp->v_stream;
	psep = NULL;
	sep = stp->sd_siglist;
	while (sep) {
		if (sep->se_procp == u.u_procp) {
			tsep = sep->se_next;
			if (psep)
				psep->se_next = tsep;
			else
				stp->sd_siglist = tsep;
			sefree(sep);
			sep = tsep;
		} else {
			psep = sep;
			sep = sep->se_next;
		}
	}

	psep = NULL;
	sep = stp->sd_pollist;
	while (sep) {
		if (sep->se_procp == u.u_procp) {
			tsep = sep->se_next;
			if (psep)
				psep->se_next = tsep;
			else
				stp->sd_pollist = tsep;
			sefree(sep);
			sep = tsep;
		} else {
			psep = sep;
			sep = sep->se_next;
		}
	}
}

/*
 * Read a stream according to the mode flags in sd_flag:
 *
 * (default mode)		- Byte stream, msg boundaries are ignored
 * RMSGDIS (msg discard)	- Read on msg boundaries and throw away
 *				  any data remaining in msg
 * RMSGNODIS (msg non-discard)	- Read on msg boundaries and put back
 *				  any remaining data on head of read queue
 *
 * Consume readable messages on the front of the queue until uio->uio_resid
 * is satisfied, the readable messages are exhausted and:
 *	VMIN is non-zero and VMIN characters are consumed or, if VTIME is
 *	non-zero, VTIME tenths of a second have elapsed since the last
 *	character arrived;
 *	VMIN is zero and, if VTIME is non-zero, VTIME tenths of a second have
 *	expired since the read was done;
 * or a boundary is reached in a message mode.  If no data was read, the
 * VMIN and VTIME criterion has not been satisfied, and the stream was not
 * opened with the NDELAY flag, block until data arrives.
 * Otherwise return the data read and update the count.
 *
 * In default mode a 0 length message signifies end-of-file and terminates
 * a read in progress.  The 0 length message is removed from the queue
 * only if it is the only message read (no data is read).
 *
 * Attempts to read an M_PROTO or M_PCPROTO message result in an
 * EBADMSG error return.
 */
strread(vp, uio)
	struct vnode *vp;
	struct uio *uio;
{
	register s;
	register struct stdata *stp;
	register mblk_t *bp, *nbp;
	int n;
	u_int nreturned;
	int is_controlling_tty;
	int check_background;

	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if (stp->sd_flag & (STRERR|STPLEX)) {
		u.u_error = ((stp->sd_flag & STPLEX) ? EINVAL : stp->sd_error);
		return;
	}

	nreturned = 0;
	stp->sd_flag &= ~TIMEDOUT;
	is_controlling_tty = (vp == u.u_procp->p_sessp->s_vp);
	check_background = is_controlling_tty;

	/* loop terminates when uio->uio_resid == 0 */
	for (;;) {
		/*
		 * Loop until we're allowed to read and we have something to
		 * read.
		 */
		for (;;) {
			register int waitflags;

			if (check_background) {
				if (u.u_error = tty_rd_wait(stp))
					return;
				check_background = 0;
			}
			s = splstr();
			if (bp = getq(RD(stp->sd_wrq))) {
				(void) splx(s);
				break;
			}
			if (stp->sd_flag & TIMEDOUT) {
				/*
				 * Timer has expired.  Don't wait for any
				 * more data, just go with what we've got.
				 */
				(void) splx(s);
				goto out;
			}
			if (stp->sd_vmin >= 0) {
				/*
				 * Vmin/vtime processing is in effect.
				 */
				if (stp->sd_vmin > 0) {
					/*
					 * Waiting for some minimum number of
					 * characters.
					 */
					stp->sd_deficit =
					    stp->sd_vmin - nreturned;
					if (stp->sd_deficit <= 0) {
						/*
						 * VMIN or more bytes returned;
						 * read is satisfied.
						 */
						(void) splx(s);
						goto out;
					}
					/*
					 * If both vmin and vtime are active,
					 * make sure that characters present
					 * when the read was issued start the
					 * timer.
					 */
					else if (nreturned != 0 &&
					    stp->sd_vtime != 0 &&
					    !(stp->sd_flag & CLKRUNNING)) {
						stp->sd_flag |= CLKRUNNING;
						timeout(strvtime, (caddr_t)stp,
						    (int)stp->sd_vtime*(HZ/10));
					}
				} else {
					if (stp->sd_vtime == 0 ||
					    nreturned != 0) {
						/*
						 * Timeout of zero expires
						 * immediately; return, no
						 * matter how much data was
						 * returned.  If any data has
						 * been read, return.
						 */
						(void) splx(s);
						goto out;
					}
					if (!(stp->sd_flag & CLKRUNNING)) {
						stp->sd_flag |= CLKRUNNING;
						timeout(strvtime, (caddr_t)stp,
						    (int)stp->sd_vtime*(HZ/10));
					}
					stp->sd_deficit = 0;
				}
			} else {
				/*
				 * Vmin/vtime processing inactive.
				 * Firewall: make sure sd_deficit has a
				 * reasonable value, although its value in
				 * this case shouldn't matter.
				 */
				stp->sd_deficit = 0;
			}
			if (stp->sd_flag & STRHUP) {
				/*
				 * Carrier (or other connection) went away;
				 * don't wait for more data, there's none
				 * coming.
				 */
				(void) splx(s);
				goto out;
			}
			/*
			 * Wait for something to show up in the read queue.
			 * If interrupted and we haven't yet transferred any
			 * data, allow the call to restart, otherwise arrange
			 * to return successfully with what we have.
			 */
			waitflags = (nreturned != 0) ?
			    (READWAIT|NOINTR) : (READWAIT|INTRRESTART);
			if (strwaitq(stp, waitflags, uio->uio_fmode)) {
				/*
				 * Old-style S5 no-delay mode caused 0 to be
				 * returned if no data, not -1 with "errno" set
				 * to EAGAIN, on terminals.  Support for POSIX
				 * added as well.
				 */
				if ((uio->uio_fmode & FNONBIO) == 0 &&
				    (uio->uio_fmode & FNBIO) &&
				    (stp->sd_flag & OLDNDELAY) &&
				    (u.u_error == EAGAIN))
					u.u_error = 0;
				(void) splx(s);
				goto out;
			}
			(void) splx(s);
			check_background = is_controlling_tty;
		}

		if (qready())
			runqueues();

		switch (bp->b_datap->db_type) {

		case M_DATA:
			/*
			 * Strip off leading zero-length message
			 * blocks, making sure that at least one
			 * block remains.
			 */
			while (bp->b_cont && (bp->b_rptr >= bp->b_wptr)) {
				nbp = bp;
				bp = bp->b_cont;
				freeb(nbp);
			}

			if ((bp->b_wptr - bp->b_rptr) == 0) {
				/*
				 * If vmin is in effect, we cannot return
				 * until the entire vmin amount has been
				 * satisfied, so an empty message is
				 * equivalent to no message at all.
				 */
				if (stp->sd_vmin > 0) {
					freemsg(bp);
					continue;
				}
				/*
				 * if already read data put zero
				 * length message back on queue else
				 * free msg and return 0.
				 */
				if (nreturned != 0)
					putbq(RD(stp->sd_wrq), bp);
				else
					freemsg(bp);
				goto out;
			}

			while (bp && uio->uio_resid) {
				if (n = MIN(uio->uio_resid,
				    bp->b_wptr - bp->b_rptr)) {
					u.u_error = uiomove((caddr_t)
						bp->b_rptr, n, UIO_READ, uio);
					nreturned += n;
				}

				if (u.u_error) {
					freemsg(bp);
					goto out;
				}

				bp->b_rptr += n;
				while (bp && (bp->b_rptr >= bp->b_wptr)) {
					nbp = bp;
					bp = bp->b_cont;
					freeb(nbp);
				}
			}

			/*
			 * The data may have been the leftover of a PCPROTO, so
			 * if none are left reset the STRPRI flag just in case.
			 */
			if (bp) {
				/*
				 * Have remaining data in message.
				 * Free msg if in discard mode.
				 */
				if (stp->sd_flag & RMSGDIS) {
					freemsg(bp);
					s = splstr();
					stp->sd_flag &= ~STRPRI;
					(void) splx(s);
				} else
					putbq(RD(stp->sd_wrq), bp);

			} else {
				s = splstr();
				stp->sd_flag &= ~STRPRI;
				(void) splx(s);
			}

			/*
			 * Check for signal messages at the front of the read
			 * queue and generate the signal(s) if appropriate.
			 * The only signal that can be on queue is M_SIG at
			 * this point.
			 */
			while (((bp = RD(stp->sd_wrq)->q_first)) &&
			    (bp->b_datap->db_type == M_SIG)) {
				bp = getq(RD(stp->sd_wrq));
				switch (*bp->b_rptr) {
				case SIGPOLL:
					if (stp->sd_sigflags & S_MSG)
						strsendsig(stp->sd_siglist,
						    S_MSG);
					break;

				default:
					if (stp->sd_pgrp)
						gsignal(stp->sd_pgrp,
						    (int)*bp->b_rptr);
					break;
				}
				freemsg(bp);
				if (qready())
					runqueues();
			}

			if ((uio->uio_resid == 0) ||
			    (stp->sd_flag & (RMSGDIS|RMSGNODIS)))
				goto out;
			continue;

		case M_PROTO:
		case M_PCPROTO:
		case M_PASSFP:
			/*
			 * Only data messages are readable.
			 * Any others generate an error.
			 */
			u.u_error= EBADMSG;
			putbq(RD(stp->sd_wrq), bp);
			goto out;

		default:
			/*
			 * Garbage on stream head read queue
			 */
			ASSERT(0);
			freemsg(bp);
			break;
		}
	}

out:
	/*
	 * The read is complete; shut off any timers that were running.
	 */
	if (stp->sd_flag & CLKRUNNING) {
		untimeout(strvtime, (caddr_t)stp);
		stp->sd_flag &= ~CLKRUNNING;
	}
}



#if	defined(sun)
/*
 * Kludge to do reads on streams from within the kernel; used by the
 * window system.  Never blocks; reads are always treated as no-delay
 * reads.  Never lowers interrupt priority; this routine is run from
 * a "timeout" routine.
 */
int
strkread(stp, bufptr, len_ptr)
	register struct stdata *stp;
	caddr_t bufptr;
	int *len_ptr;
{
	register mblk_t *bp, *nbp;
	register int n;
	char rflg;
	register int len;

	if (stp->sd_flag&(STRERR|STPLEX))
		return ((stp->sd_flag&STPLEX) ? EINVAL : stp->sd_error);

	/* loop terminates when len == 0 or when no more data */
	len = *len_ptr;
	rflg = 0;
	for (;;) {
		if (!(bp = getq(RD(stp->sd_wrq))))
			goto done;

		switch (bp->b_datap->db_type) {

		case M_DATA:
			/*
			 * Strip off leading zero-length message
			 * blocks, making sure that at least one
			 * block remains.
			 */
			while (bp->b_cont && (bp->b_rptr >= bp->b_wptr)) {
				nbp = bp;
				bp = bp->b_cont;
				freeb(nbp);
			}

			if ((bp->b_wptr - bp->b_rptr) == 0) {
				/*
				 * if already read data put zero
				 * length message back on queue else
				 * free msg and return 0.
				 */
				if (rflg)
					putbq(RD(stp->sd_wrq), bp);
				else
					freemsg(bp);
				goto done;
			}

			rflg = 1;
			while (bp && len) {
				if (n = MIN(len, bp->b_wptr - bp->b_rptr))
					bcopy((caddr_t)bp->b_rptr, bufptr,
					    (u_int)n);

				bp->b_rptr += n;
				bufptr += n;
				len -= n;
				while (bp && (bp->b_rptr >= bp->b_wptr)) {
					nbp = bp;
					bp = bp->b_cont;
					freeb(nbp);
				}
			}

			/*
			 * The data may have been the leftover of a PCPROTO, so
			 * if none are left reset the STRPRI flag just in case.
			 */
			if (bp) {
				/*
				 * Have remaining data in message.
				 */
				putbq(RD(stp->sd_wrq), bp);
			} else
				stp->sd_flag &= ~STRPRI;

			if (len == 0)
				goto done;
			continue;

		case M_PROTO:
		case M_PCPROTO:
		case M_PASSFP:
			/*
			 * Only data messages are readable.
			 * Any others generate an error.
			 */
			putbq(RD(stp->sd_wrq), bp);
			return (EBADMSG);

		default:
			/*
			 * Garbage on stream head read queue
			 */
			ASSERT(0);
			freemsg(bp);
			break;
		}
	}

done:
	if ((*len_ptr -= len) == 0)
		return (EWOULDBLOCK);	/* nothing read */
	return (0);
}
#endif



/*
 * Stream read put procedure.  Called from downstream driver/module
 * with messages for the stream head.  Data, protocol, and in-stream
 * signal messages are placed on the queue, others are handled directly.
 */
static int
strrput(q, bp)
	register queue_t *q;
	register mblk_t *bp;
{
	register struct stdata *stp;
	register struct iocblk *iocbp;
	struct stroptions *sop;
	register int datalen;
	register int s;

	stp = (struct stdata *)q->q_ptr;

	ASSERT(!(stp->sd_flag & STPLEX));

	switch (bp->b_datap->db_type) {

	case M_DATA:
	case M_PROTO:
	case M_PCPROTO:
	case M_PASSFP:
		s = splstr();
		if (bp->b_datap->db_type == M_PCPROTO) {
			/*
			 * Only one priority protocol message is allowed at the
			 * stream head at a time.
			 */
			if (stp->sd_flag & STRPRI) {
				freemsg(bp);
				(void) splx(s);
				return;
			}
			stp->sd_flag |= STRPRI;
			if (stp->sd_sele) {
				selwakeup(stp->sd_sele,
				    (stp->sd_flag & ECOLL) != 0);
				stp->sd_sele = 0;
				stp->sd_flag &= ~ECOLL;
			}
			if (stp->sd_sigflags & S_HIPRI)
				strsendsig(stp->sd_siglist, S_HIPRI);
			if (stp->sd_pollflags & POLLPRI)
				strwakepoll(stp, POLLPRI);
		} else if (!q->q_first) {
			if (stp->sd_selr) {
				selwakeup(stp->sd_selr,
				    (stp->sd_flag & RCOLL) != 0);
				stp->sd_selr = 0;
				stp->sd_flag &= ~RCOLL;
			}
			if (stp->sd_sigflags & S_INPUT)
				strsendsig(stp->sd_siglist, S_INPUT);
			if (stp->sd_pollflags & POLLIN)
				strwakepoll(stp, POLLIN);
		}

		if (stp->sd_flag & RSLEEP) {
			/*
			 * Cancel any pending VTIME timeouts, and start a
			 * new one, if VMIN is non-zero.
			 */
			if (stp->sd_vmin > 0 && stp->sd_vtime != 0) {

				if (stp->sd_flag & CLKRUNNING)
					untimeout(strvtime, (caddr_t)stp);
				stp->sd_flag |= CLKRUNNING;
				stp->sd_flag &= ~TIMEDOUT;
				timeout(strvtime, (caddr_t)stp,
				    (int) stp->sd_vtime*(HZ/10));
			}
			/*
			 * Avoid extra work by awakening processes sleeping in
			 * read/getmsg only if the arrival of this message
			 * makes it possible that they can proceed.  Criteria
			 * are any of the following:
			 * - Not a data message.
			 * - Zero-length (semantics are tricky enough that we
			 *   don't want to pre-empt them here).
			 * - Vmin/vtime processing not active.
			 * - The message contains enough data to satisfy the
			 *   outstanding vmin deficit.
			 */
			if (bp->b_datap->db_type != M_DATA ||
			    (datalen = msgdsize(bp)) == 0 ||
			    stp->sd_vmin < 0 ||
			    (stp->sd_deficit -= datalen) <= 0) {
				char oldpri;

				stp->sd_flag &= ~RSLEEP;
				oldpri = curpri;
				curpri = STIPRI; /* no preemption */
				wakeup((caddr_t)q);
				curpri = oldpri;
			}
		}

		/*
		 * Notify with SIGIO if 4.2-style asynchronous I/O requested.
		 */
		if (stp->sd_flag & ASYNC)
			str_sigio(stp);

		putq(q, bp);
		(void) splx(s);
		return;


	case M_ERROR:
		/*
		 * An error has occurred downstream; the errno is in the first
		 * byte of the message.
		 */
		if (*bp->b_rptr != 0) {
			s = splstr();
			stp->sd_flag |= STRERR;
			stp->sd_error = *bp->b_rptr;
			(void) splx(s);
			wakeup((caddr_t)q);	/* the readers */
			wakeup((caddr_t)WR(q));	/* the writers */
			wakeup((caddr_t)stp);	/* the ioctllers */

			strwakepoll(stp, POLLERR);

			bp->b_datap->db_type = M_FLUSH;
			*bp->b_rptr = FLUSHRW;
			qreply(q, bp);
			return;
		}
		freemsg(bp);
		return;

	case M_HANGUP:
		freemsg(bp);
		s = splstr();
		stp->sd_error = ENXIO;
		stp->sd_flag |= STRHUP;
		(void) splx(s);

		/*
		 * send signal if controlling tty
		 */
		if (stp->sd_pgrp)
			gsignal(stp->sd_pgrp, SIGHUP);

		wakeup((caddr_t)q);	/* the readers */
		wakeup((caddr_t)WR(q));	/* the writers */
		wakeup((caddr_t)stp);	/* the ioctllers */

		/*
		 * wake up read, write, and exception pollers and
		 * reset wakeup mechanism.
		 */
		strwakepoll(stp, POLLHUP);
		return;

	case M_UNHANGUP:
		freemsg(bp);
		s = splstr();
		stp->sd_error = 0;
		stp->sd_flag &= ~STRHUP;
		(void) splx(s);
		return;

	case M_SIG:
		/*
		 * Someone downstream wants to post a signal.  The
		 * signal to post is contained in the first byte of the
		 * message.  If the message would go on the front of
		 * the queue, send a signal to the process group
		 * (if not SIGPOLL) or to the siglist processes
		 * (SIGPOLL).  If something is already on the queue,
		 * just enqueue the message.
		 */
		if (q->q_first) {
			putq(q, bp);
			return;
		}
		/* flow through */

	case M_PCSIG:
		/*
		 * Don't enqueue, just post the signal.
		 */
		switch (*bp->b_rptr) {
		case SIGPOLL:
			if (stp->sd_sigflags & S_MSG)
				strsendsig(stp->sd_siglist, S_MSG);
			break;

		default:
			if (stp->sd_pgrp)
				gsignal(stp->sd_pgrp, (int)*bp->b_rptr);
			break;
		}
		freemsg(bp);
		return;

	case M_FLUSH:
		/*
		 * Flush queues.  The indication of which queues to flush
		 * is in the first byte of the message.  If the read queue
		 * is specified, then flush it.
		 */
		if (*bp->b_rptr & FLUSHR) {
			mblk_t *mp, *tmp;

			s = splstr();
			mp = q->q_first;
			q->q_first = q->q_last = NULL;
			q->q_count = 0;
			q->q_flag &= ~(QFULL | QWANTW);
			(void) splx(s);
			while (mp) {
				tmp = mp->b_next;
				if (mp->b_datap->db_type == M_PASSFP)
					closef(((struct strrecvfd *)
						mp->b_rptr)->f.fp);
				freemsg(mp);
				mp = tmp;
			}
		}
		if (*bp->b_rptr & FLUSHW) {
			*bp->b_rptr &= ~FLUSHR;
			qreply(q, bp);
			return;
		}
		freemsg(bp);
		return;

	case M_IOCACK:
	case M_IOCNAK:
		iocbp = (struct iocblk *)bp->b_rptr;
		/*
		 * if not waiting for ACK or NAK then just free msg
		 * if already have ACK or NAK for user then just free msg
		 * if incorrect id sequence number then just free msg
		 */
		s = splstr();
		if (!(stp->sd_flag & IOCWAIT) || stp->sd_iocblk ||
		    (stp->sd_iocid != iocbp->ioc_id)) {
			freemsg(bp);
			(void) splx(s);
			return;
		}

		/*
		 * assign ACK or NAK to user and wake up
		 */
		stp->sd_iocblk = bp;
		(void) splx(s);
		wakeup((caddr_t)stp);
		return;

	case M_SETOPTS:
		/*
		 * Set stream head options (read option, write offset,
		 * min/max packet size, high/low water marks for the read
		 * side only, and/or min/time values).
		 *
		 * N.B.: The stream head and ldterm both cooperate in treating
		 * so_vmin as a signed value, even though it's declared as a
		 * ushort.  Tty semantics force normal values to fit in a
		 * u_char, so the cast below should cause no significance
		 * problems there.  The value (short) -1 is used as an out of
		 * band value to disable vmin processing altogether.
		 */

		ASSERT((bp->b_wptr - bp->b_rptr) == sizeof (struct stroptions));
		sop = (struct stroptions *)bp->b_rptr;
		s = splstr();
		if (sop->so_flags & SO_READOPT) {
			switch (sop->so_readopt) {
			case RNORM:
				stp->sd_flag &= ~(RMSGDIS | RMSGNODIS);
				break;
			case RMSGD:
				stp->sd_flag =
				    (stp->sd_flag & ~RMSGNODIS) | RMSGDIS;
				break;
			case RMSGN:
				stp->sd_flag =
				    (stp->sd_flag & ~RMSGDIS) | RMSGNODIS;
				break;
			}
		}

		if (sop->so_flags & SO_WROFF)
			stp->sd_wroff = sop->so_wroff;
		if (sop->so_flags & SO_MINPSZ)
			q->q_minpsz = sop->so_minpsz;
		if (sop->so_flags & SO_MAXPSZ)
			q->q_maxpsz = sop->so_maxpsz;
		if (sop->so_flags & SO_HIWAT)
			q->q_hiwat = sop->so_hiwat;
		if (sop->so_flags & SO_LOWAT)
			q->q_lowat = sop->so_lowat;
		if (sop->so_flags & SO_VMIN)
			stp->sd_vmin = (short) sop->so_vmin;
		if (sop->so_flags & SO_VTIME)
			stp->sd_vtime = sop->so_vtime;
		if (sop->so_flags & SO_NDELON)
			stp->sd_flag |= OLDNDELAY;
		if (sop->so_flags & SO_NDELOFF)
			stp->sd_flag &= ~OLDNDELAY;
		if (sop->so_flags & SO_TOSTOP) {
			if (sop->so_tostop)
				stp->sd_flag |= STRTOSTOP;
			else
				stp->sd_flag &= ~STRTOSTOP;
		}

		freemsg(bp);

		if ((q->q_count <= q->q_lowat) && (q->q_flag & QWANTW)) {
			q->q_flag &= ~QWANTW;
			for (q=backq(q); q && !q->q_qinfo->qi_srvp; q=backq(q))
				;
			if (q)
				qenable(q);
		}

		(void) splx(s);
		return;

	/*
	 * The following set of cases deal with situations where two stream
	 * heads are connected to each other (twisted streams).  These messages
	 * should never originate at a driver or module.
	 */
	case M_BREAK:
	case M_CTL:
	case M_DELAY:
	case M_START:
	case M_STOP:
		freemsg(bp);
		return;

	case M_IOCTL:
		/*
		 * Always NAK this condition
		 * (makes no sense)
		 */
		bp->b_datap->db_type = M_IOCNAK;
		qreply(q, bp);
		return;

	default:
		ASSERT(0);
		freemsg(bp);
		return;
	}
}


/*
 * Send SIGPOLL signal to all processes registered on the given signal
 * list that want a signal for the specified event.
 */
static void
strsendsig(siglist, event)
	register struct strevent *siglist;
	register event;
{
	struct strevent *sep;

	for (sep = siglist; sep; sep = sep->se_next) {
		if (sep->se_events & event)
			psignal(sep->se_procp, SIGPOLL);
	}
}

/*
 * Send a SIGIO to the pid/pgrp that wants it.
 *
 * N.B.  This does not follow the old semantics in that a process
 * that has asked for sigio on its ctty and then changes from fg to bg,
 * it will still be signaled.  Previously, the process that became
 * the fg process would get signaled, like it or not.
 *
 * XXX - This should be merged in with the sys5 way of doing ASYNC IO.
 */
str_sigio(stp)
	struct stdata *stp;
{
	register sigio = stp->sd_sigio;

	/*
	 * If they were assuming that the tty pgrp gets sigio, match that.
	 * If no one wants the signal, drop it.
	 */
	if (!sigio && !(sigio = -stp->sd_pgrp))
		return;
	if (sigio < 0)
		gsignal(-sigio, SIGIO);
	else {
		struct proc *p;

		if (p = pfind(sigio))
			psignal(p, SIGIO);
	}
}

/*
 * Wake up all processes sleeping on a poll for the given events
 * on this stream.  POLLERR and POLLHUP cause a wakeup of all processes
 * regardless of the events they were looking for.  Remove all of
 * the event cells matching the given events from the pollist.
 */
static void
strwakepoll(stp, events)
	struct stdata *stp;
{
	register struct strevent *sep, *psep;
	register s;

	stp->sd_pollflags &= ~events;
	sep = stp->sd_pollist;
	psep = NULL;
	while (sep) {
		if ((sep->se_events & events) ||
		    (events & (POLLHUP|POLLERR|POLLNVAL))) {
			s = splstr();
			if (sep->se_procp->p_wchan == (caddr_t)&pollwait)
				unselect(sep->se_procp);
			else
				sep->se_procp->p_flag &= ~SPOLL;
			(void) splx(s);
			if (psep){
				psep->se_next = sep->se_next;
				sefree(sep);
				sep = psep->se_next;
			} else  {
				stp->sd_pollist = sep->se_next;
				sefree(sep);
				sep = stp->sd_pollist;
			}
		} else {
			psep = sep;
			sep = sep->se_next;
		}
	}
}


/*
 * Write attempts to break the read request into messages conforming
 * with the minimum and maximum packet sizes set downstream.
 *
 * Write will always attempt to get the largest buffer it can to satisfy the
 * message size. If it can not, then it will try up to 2 classes down to try
 * to satisfy the write. Write will not block if downstream queue is full and
 * O_NDELAY is set, otherwise it will block waiting for the queue to get room.
 *
 * A write of zero bytes gets packaged into a zero length message and sent
 * downstream like any other message.
 *
 * If buffers of the requested sizes are not available, the write will
 * sleep until the buffers become available.
 *
 * Write (if specified) will supply a write offset in a message if it
 * makes sense. This can be specified by downstream modules as part of
 * a M_SETOPTS message. Write will not supply the write offset if it
 * cannot supply any data in a buffer. In other words, write will never
 * send down an empty packet due to a write offset.
 */
strwrite(vp, uio)
	struct vnode *vp;
	struct uio *uio;
{
	int tempmode;
	register s;
	register struct stdata *stp;
	mblk_t *mp;
	short rmin, rmax;
	char waitflag;
	int is_controlling_tty;
	int check_background;
	int written;

	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if (stp->sd_flag & (STRERR|STRHUP|STPLEX)) {
		if (stp->sd_flag & STPLEX)
			u.u_error = EINVAL;
		else if (stp->sd_flag & STRHUP)
			u.u_error = EIO;	/* posix */
		else
			u.u_error = stp->sd_error;
		return;
	}

	/*
	 * Check the min/max packet size constraints.  If min packet size
	 * is non-zero, the write cannot be split into multiple messages
	 * and still guarantee the size constraints.
	 */
	rmin = stp->sd_wrq->q_next->q_minpsz;
	rmax = stp->sd_wrq->q_next->q_maxpsz;
	ASSERT(rmax);
	if (rmax == 0)
		return;
	if (rmax == INFPSZ)
		rmax = strmsgsz;
	else
		rmax = MIN(strmsgsz, rmax);

	if (rmin > 0 && (uio->uio_resid < rmin || uio->uio_resid > rmax)) {
		u.u_error = ERANGE;
		return;
	}

	/*
	 * Do until count satisfied or error.
	 */
	waitflag = WRITEWAIT|INTRRESTART;

	/*
	 * Old-style S5 no-delay mode didn't apply on writes to terminals.
	 */
	if (stp->sd_flag & OLDNDELAY)
		tempmode = uio->uio_fmode & ~FNBIO;
	else
		tempmode = uio->uio_fmode;

	is_controlling_tty = (vp == u.u_procp->p_sessp->s_vp);
	check_background = is_controlling_tty;
	written = 0;

	do {
		for (;;) {
			if (check_background) {
				if (u.u_error = tty_wr_wait(stp, 1))
					return;
				check_background = 0;
			}
			s = splstr();
			if (canput(stp->sd_wrq->q_next)) {
				(void) splx(s);
				break;
			}
			if (strwaitq(stp, waitflag, tempmode)) {
				/*
				 * Don't error on POSIX- or BSD-style
				 * non-blocking I/O if anything has been
				 * written.
				 */
				if (written &&
				    (uio->uio_fmode & (FNONBIO|FNDELAY)) &&
				    (u.u_error == EAGAIN ||
						u.u_error == EWOULDBLOCK))
					u.u_error = 0;
				(void) splx(s);
				return;
			}
			(void) splx(s);
			check_background = is_controlling_tty;
		}

		/*
		 * Determine the size of the next message to be
		 * packaged.  May have to break write into several
		 * messages based on max packet size.
		 */
		if (!(mp = strmakemsg((struct strbuf *)NULL,
		    MIN(uio->uio_resid, rmax), uio,
		    (int)stp->sd_wroff, (long)0, 0)))
			return;

		/*
		 * Put block downstream
		 */
		(*stp->sd_wrq->q_next->q_qinfo->qi_putp)
		    (stp->sd_wrq->q_next, mp);
		written = 1;
		waitflag |= NOINTR;
		if (qready())
			runqueues();

	} while (uio->uio_resid);
}



/*
 * Check the uppermost write queue on the stream associated with vp for
 * space for a kernel message (from uprintf/tprintf).  Allow some space
 * over the normal hiwater mark so we don't lose messages due to normal
 * flow control, but don't let the device (tty) run amok.
 * The sleep is interruptable.
 */
int
strcheckoutq(vp, wait)
	struct vnode *vp;
	int wait;
{
	register struct stdata *stp;
	int s;
	int serror;

	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if ((stp->sd_flag & (STRERR|STRHUP|STPLEX|STWOPEN|STRCLOSE)) ||
	    (stp->sd_wrq == (queue_t *)0))
		return (0);

	s = splstr();
	if (!canputextra(stp->sd_wrq->q_next)) {
		if (!wait) {
			(void) splx(s);
			return (0);
		}
		serror = u.u_error;
		while (!canput(stp->sd_wrq->q_next)) {
			if (strwaitq(stp, WRITEWAIT|NOINTR, FWRITE)) {
				(void) splx(s);
				u.u_error = serror;
				return (0);
			}
		}
	}
	(void) splx(s);
	return (1);
}

/*
 * Variant of "strwrite" for printing kernel messages to a tty.
 * Arguments are a pointer to the vnode to write to, the address
 * of the characters to print, and the number of characters to print.
 * Flow control is ignored; it is assumed that if any special flow-control
 * checking is to be done, it's already been done by "strcheckoutq".
 */
int
stroutput(vp, base, count)
	struct vnode *vp;
	char *base;
	int count;
{
	register struct stdata *stp;
	register queue_t *qnext;
	mblk_t *mp;
	struct uio uio;
	struct iovec iov;
	short rmin, rmax;
	int serror;

	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if (stp->sd_flag & (STRERR|STRHUP|STPLEX|STWOPEN|STRCLOSE))
		return (0);

	/*
	 * Check the min/max packet size constraints.  If min packet size
	 * is non-zero, the write cannot be split into multiple messages
	 * and still guarantee the size constraints.
	 */
	qnext = stp->sd_wrq->q_next;
	rmin = qnext->q_minpsz;
	rmax = qnext->q_maxpsz;
	ASSERT(rmax);
	if (rmax == 0)
		return (0);
	if (rmax == INFPSZ)
		rmax = strmsgsz;
	else
		rmax = MIN(strmsgsz, rmax);

	if ((rmin > 0) && ((count < rmin) || (count > rmax)))
		return (0);

	/*
	 * do until count satisfied or error
	 */
	iov.iov_base = base;
	iov.iov_len = count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_fmode = 0;
	uio.uio_resid = iov.iov_len;
	serror = u.u_error;
	do {
		/*
		 * Determine the size of the next message to be
		 * packaged.  May have to break write into several
		 * messages based on max packet size.
		 */
		if (!(mp = strmakemsg((struct strbuf *)NULL,
		    MIN(uio.uio_resid, rmax), &uio,
		    (int)stp->sd_wroff, (long)0, 1))) {
			u.u_error = serror;
			return (0);
		}

		/*
		 * Put block downstream
		 */
		(*qnext->q_qinfo->qi_putp)(qnext, mp);
	} while (uio.uio_resid);
	u.u_error = serror;
	return (1);
}



/*
 * Stream head write service routine.
 * Its job is to wake up any sleeping writers when a queue
 * downstream needs data (part of the flow control in putq and getq).
 * It also must wake anyone sleeping on a poll().
 * For stream head right below mux module, it must also invoke put procedure
 * of next downstream module
 */
static int
strwsrv(q)
	register queue_t *q;
{
	register struct stdata *stp;
	register int s;

	stp = (struct stdata *)q->q_ptr;

	s = splstr();
	ASSERT(!(stp->sd_flag & STPLEX));

	if (stp->sd_selw) {
		selwakeup(stp->sd_selw, (stp->sd_flag & WCOLL) != 0);
		stp->sd_selw = 0;
		stp->sd_flag &= ~WCOLL;
	}

	if (stp->sd_flag & WSLEEP) {
		char oldpri;

		stp->sd_flag &= ~WSLEEP;
		oldpri = curpri;
		curpri = STOPRI;
		wakeup((caddr_t)q);
		curpri = oldpri;
	}

	if (stp->sd_flag & ASYNC)
		str_sigio(stp);
	if (stp->sd_sigflags & S_OUTPUT)
		strsendsig(stp->sd_siglist, S_OUTPUT);
	if (stp->sd_pollflags & POLLOUT)
		strwakepoll(stp, POLLOUT);
	(void) splx(s);
}


/*
 * Table to map module names to line discipline numbers.
 */
typedef	struct {
	char	*ld_name;		/* name of line discipline module */
	struct streamtab *ld_tab;	/* streamtab entry for that module */
} ldinfo;

static ldinfo ldtab[] = {
	{ "ldterm" },	/* old line discipline */
	{ "bk" },	/* Berknet line discipline */
	{ "ldterm" },	/* new line discipline */
	{ "tab" },	/* Hitachi tablet discipline */
	{ "ntab" },	/* gtco tablet discipline */
	{ "ms" },	/* mouse discipline */
	{ "kb" },	/* keyboard discipline */
	{ "slip" },	/* Serial Line IP discipline */
};

#define	NLDISC	(sizeof ldtab / sizeof ldtab[0])

/*
 * Module name of compatibility module.
 * We assume that this guy sits on top of all line discipline modules,
 * since it doesn't make much sense to have TIOCSETD and TIOCGETD
 * without the rest of the V7/4.2BSD "ioctl"s.
 */

static char ttcompatname[] = "ttcompat";
static struct qinit *ttcompat_qinit;

static int ldtab_initialized = 0;

/*
 * ioctl for streams
 */
/*ARGSUSED*/
strioctl(vp, cmd, data, flag)
	struct vnode *vp;
	caddr_t data;
	int flag;
{
	register struct stdata *stp;
	struct strioctl strioc;
	int arg = *(int *)data;
	int size;
	int strict_posix = 0;
	register int s;

	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if (stp->sd_flag & (STRERR|STPLEX)) {
		u.u_error = ((stp->sd_flag & STPLEX) ? EINVAL : stp->sd_error);
		return;
	}
	switch (cmd) {

	/*
	 * Backwards compatibility hacks.
	 */
	case TIOCGETD: {
		register queue_t *q;
		register int i;

		if (!ldtab_initialized)
			initialize_ldtab();
		if ((q = getcompatqueue(stp)) == NULL ||
		    (q = q->q_next) == NULL)
			goto regular;	/* do it as a regular "ioctl" */
		/*
		 * We found a module below the compatibility module.
		 * Now see if it corresponds to a line discipline.
		 * We start the search at the first entry, so that
		 * we always seem to be in the new line discipline
		 * if we're using the regular TTY module.
		 */
		for (i = 1; i < NLDISC; i++) {
			if (ldtab[i].ld_tab != NULL &&
			    q->q_qinfo == ldtab[i].ld_tab->st_wrinit)
				goto found_getd;
		}
		goto regular;	/* do it as a regular "ioctl" */

	    found_getd:
		*(int *)data = i;
		break;
	    }

	case TIOCSETD:
		if (ldreplace(vp, stp, *(int *)data))
			break;
		/*
		 * Try doing it as a regular "ioctl".
		 */

	default:
	regular:
		/*
		 * Set up strioc buffer and call strdoioctl() to do
		 * the work.
		 */
		strioc.ic_cmd = cmd;

		/*
		 * Turn the timeout OFF; TCSETSW waits for output
		 * to drain, and if the user hits ^S and goes out
		 * to lunch, this could take a while to finish...
		 */
		strioc.ic_timout = INFTIM;

		/*
		 * Interpret high order word to find
		 * amount of data to be copied to/from the
		 * user's address space.
		 */
		size = (cmd &~ (_IOC_INOUT|_IOC_VOID)) >> 16;
		if (cmd&_IOC_IN) {
			/*
			 * Argument is a pointer to a variable that is read
			 * by the system; it may also be modified.
			 * "data" is a pointer to a copy of that variable;
			 * if the call modifies that variable, whatever we
			 * stuff back into what "data" points to will be
			 * copied back there.
			 */
			strioc.ic_len = size;
		} else if (cmd&_IOC_OUT) {
			/*
			 * Argument is a pointer to a variable that is
			 * modified, but not read, by the system.
			 * "data" is a pointer to a buffer for that variable;
			 * whatever we stuff back into what "data" points to
			 * will be copied back there.
			 */
			strioc.ic_len = 0;
		} else if (cmd&_IOC_VOID) {
			/*
			 * Argument is a cookie interpreted by the module;
			 * it had better not be a pointer, as the module does
			 * not run in process context!
			 * The cookie is assumed to be an "int"; "data"
			 * points to a copy of it.
			 */
			strioc.ic_len = sizeof (int);
		} else {
			/*
			 * Argument is an old-style "ioctl" command, and "data"
			 * is not valid.
			 */
			strioc.ic_len = 0;
		}

		strioc.ic_dp = (char *)data;
		strdoioctl(stp, &strioc, K_TO_K_BOTH, flag);
		return;

	/*
	 * Calls that apply to all sorts of stream objects.
	 */
	case FIONBIO:
		if (*(int *)data)
			stp->sd_flag |= NBIO;
		else
			stp->sd_flag &= ~NBIO;
		return;

	case FIOASYNC:
		if (*(int *)data)
			stp->sd_flag |= ASYNC;
		else
			stp->sd_flag &= ~ASYNC;
		return;

	case FIONREAD: {
		/*
		 * return total number of bytes of data in all messages
		 * in read queue
		 */
		register mblk_t *mp;
		int count = 0;

		for (mp = RD(stp->sd_wrq)->q_first; mp; mp = mp->b_next)
			count += msgdsize(mp);
		*(int *)data = count;
		return;
	    }

	case FIOSETOWN:
		stp->sd_sigio = *(int*)data;
		break;

	case TIOCSETPGRP:
		strict_posix = 1;
		/* fall through */

	case TIOCSPGRP: {
		/*
		 * POSIX checks:
		 *	range
		 *	calling proc must have an active ctty,
		 *	and this stream must be it,
		 *	and this tty must be still accessible by the session
		 *	and arg must be a pgrp in the process' session.
		 * SunOS checks:
		 *	and tty must be open for reading.
		 */
		struct proc *p;
		struct sess *sp = u.u_procp->p_sessp;

		if (arg < 0 || arg >= MAXPID) {
			u.u_error = EINVAL;
			return;
		}
		if (vp == sp->s_vp) {
			if (u.u_error = tty_wr_wait(stp, 0)) {
				return;
			}
		} else {
			u.u_error = ENOTTY;
			return;
		}
		p = pgfind(arg);
		if (strict_posix) {
			if (!p || p->p_sessp != sp) {
				u.u_error = EPERM;
				    return;
			}
		} else {
			/*
			 * Should insist that p exists and is in this session.
			 * Instead we insist that if arg is an existing pgrp
			 * it is in our session.  This goes away when the
			 * shells work properly.  The shell problem is that
			 * the parent or the child may try to give the tty to
			 * a pgrp that doesn't exist yet but will exist soon.
			 */
			if (p && p->p_sessp != sp) {
				u.u_error = EPERM;
				    return;
			}
		}
		if (u.u_uid && (flag & FREAD) == 0) {
			u.u_error = EPERM;
			return;
		}
		stp->sd_pgrp = arg;
		break;
	    }

	case TIOCSCTTY:
		assign_ctty(vp, stp, suser() && arg == 1);
		break;

	case FIOGETOWN:
		*(int *)data = stp->sd_sigio ?  stp->sd_sigio : -stp->sd_pgrp;
		break;

	case TIOCGETPGRP: {
		register struct sess *sp;

		sp = u.u_procp->p_sessp;
		if (sp == NULL || sp->s_vp != vp) {
			u.u_error = ENOTTY;
			return;
		}
		/* fall through to TIOCGPRP */
	    }

	case TIOCGPGRP:
		*(int *)data = stp->sd_pgrp;
		break;

	case I_STR: {
		/*
		 * Stream ioctl.  Read in an strioctl buffer from the user
		 * along with any data specified and send it downstream.
		 * Strdoioctl will wait allow only one ioctl message at
		 * a time, and waits for the acknowledgement.
		 */
		register struct strioctl *striocp;

		striocp = (struct strioctl *)data;
		if (striocp->ic_timout < -1) {
			u.u_error = EINVAL;
			return;
		}

		strdoioctl(stp, striocp, U_TO_K, flag);
		return;
	    }

	case I_NREAD: {
		/*
		 * return number of bytes of data in first message
		 * in queue in "arg" and return the number of messages
		 * in queue in return value
		 */
		mblk_t *bp;

		if (bp = RD(stp->sd_wrq)->q_first)
			*(int *)data = msgdsize(bp);
		u.u_rval1 = qsize(RD(stp->sd_wrq));
		return;
	    }

	case I_FIND: {
		/*
		 * get module name
		 */
		char *mname;
		queue_t *q;
		int i;

		mname = (char *)data;
		/*
		 * find module in fmodsw
		 */
		if ((i = findmod(mname)) < 0) {
			u.u_error = EINVAL;
			return;
		}
		u.u_rval1 = 0;

		/* look downstream to see if module is there */
		for (q = stp->sd_wrq->q_next;
		    q && (fmodsw[i].f_str->st_wrinit != q->q_qinfo);
		    q = q->q_next)
			;

		u.u_rval1 = (q ? 1 : 0);
		return;
	    }

	case I_PUSH:
		/*
		 * Push a module below the stream head.
		 */
		dopush(stp, (char *)data, vp, 1, 0);
		return;

	case I_POP:
		/*
		 * Pop the module that's below the stream head (if there
		 * is one).
		 */
		if (stp->sd_flag & STRHUP) {
			u.u_error = ENXIO;
			return;
		}
		if (stp->sd_wrq->q_next->q_next &&
		    !(stp->sd_wrq->q_next->q_next->q_flag & QREADR)) {
			qdetach(RD(stp->sd_wrq->q_next), 1, 0);
			stp->sd_pushcnt--;
			return;
		}
		u.u_error = EINVAL;
		return;

	case I_LOOK: {
		/*
		 * Get name of first module downstream.
		 * If no module, return error.
		 */
		register char *mname;

		if ((mname = getmname(stp->sd_wrq->q_next)) != NULL) {
			bcopy((caddr_t) mname, data, FMNAMESZ + 1);
			return;
		}
		u.u_error = EINVAL;
		return;
	    }


	case I_LINK:
	case I_PLINK: {
		/*
		 * link a multiplexer
		 */
		struct file *fpdown;
		struct linkblk *linkblkp, *alloclink();
		struct stdata *stpdown;
		queue_t *qup;
		queue_t *rq;

		/*
		 * Test for invalid upper stream
		 */
		if (stp->sd_flag & STRHUP) {
			u.u_error = ENXIO;
			return;
		}
		if (!stp->sd_strtab->st_muxwinit) {
			u.u_error = EINVAL;
			return;
		}

		fpdown = getf(arg);
		if (u.u_error)
			return;

		/*
		 * Test for invalid lower stream.
		 * If the potential lower stream isn't a stream,
		 * or if it's the same as the upper stream,
		 * or if it's already linked below something,
		 *    has had a hangup or an error, or is waiting
		 *    for an "ioctl" response,
		 * disallow the link.
		 */
		if (!(stpdown = ((struct vnode *)(fpdown->f_data))->v_stream) ||
		    (stpdown == stp) ||
		    (stpdown->sd_flag & (STPLEX|STRHUP|STRERR|IOCWAIT))) {
			u.u_error = EINVAL;
			return;
		}

		/*
		 * If this is not a permanent link and you're
		 * going to introduce a cycle, disallow the link.
		 * (If it's a permanent link, the notion of a cycle
		 * doesn't apply, since a permanent link only
		 * involves one stream.)
		 */
		if (cmd == I_LINK) {
			qup = getendq(stp->sd_wrq);
			    if (linkcycle(qup, stpdown->sd_wrq)) {
				u.u_error = EINVAL;
				return;
			}
		} else
			qup = NULL;

		if (!(linkblkp = alloclink(qup,
		    stpdown->sd_wrq, fpdown - &file[0]))) {
			u.u_error = EAGAIN;
			return;
		}
		strioc.ic_cmd = cmd;
		strioc.ic_timout = 0;
		strioc.ic_len = sizeof (struct linkblk);
		strioc.ic_dp = (char *) linkblkp;

		/* Set up queues for link */
		rq = RD(stpdown->sd_wrq);
		setq(rq, stp->sd_strtab->st_muxrinit,
			stp->sd_strtab->st_muxwinit);
		rq->q_ptr = WR(rq)->q_ptr = NULL;
		rq->q_flag |= QWANTR;
		WR(rq)->q_flag |= QWANTR;

		strdoioctl(stp, &strioc, K_TO_K, flag);

		if (u.u_error) {
			linkblkp->l_qbot = NULL;
			setq(rq, &strdata, &stwdata);
			rq->q_ptr = WR(rq)->q_ptr = (caddr_t)stpdown;
			return;
		}
		s = splstr();
		stpdown->sd_flag |= STPLEX;
		(void) splx(s);
		fpdown->f_count++;
		/*
		 * Wake up any other processes that may have been
		 * waiting on the lower stream.  These will all
		 * error out.
		 */
		wakeup((caddr_t)rq);
		wakeup((caddr_t)WR(rq));
		wakeup((caddr_t)stpdown);
		u.u_rval1 = fpdown - &file[0];
		return;
	    }


	case I_UNLINK: {
		/*
		 * Unlink a multiplexer.
		 * If arg is -1, unlink all links for which this is the
		 * controlling stream.  Otherwise, arg is an index number
		 * for a link to be removed.
		 */
		struct file *fpdown;
		struct linkblk *linkblkp;
		struct stdata *stpdown;

		if (stp->sd_flag & STRHUP) {
			u.u_error = ENXIO;
			return;
		}
		if (arg == -1) {
			munlinkall(stp, 0);
		} else {
			fpdown = &file[arg];
			if ((fpdown < file) ||
			    (fpdown >= fileNFILE) ||
			    fpdown->f_type != DTYPE_VNODE ||
			    !fpdown->f_data ||
			    !(stpdown = ((struct vnode *)
					(fpdown->f_data))->v_stream) ||
			    !(stpdown->sd_flag & STPLEX) ||
			    !(linkblkp = findlinks(getendq(stp->sd_wrq),
					stpdown->sd_wrq, arg))) {
				/* invalid user supplied index number */
				u.u_error = EINVAL;
				return;
			}
			(void) munlinkone(stp, fpdown, linkblkp, 0);
		}
		return;
	    }

	case I_PUNLINK: {
		/*
		 * Unlink a multiplexer; the link being broken is a
		 * persistent link.  The argument is a file descriptor
		 * referring to the stream being unlinked.  This is not,
		 * however, the file descriptor that held the lower
		 * stream open, so it's not the one we close; that one
		 * is the one referred to by "linkblkp->l_index".
		 */
		struct file *fpdown;
		struct linkblk *linkblkp;
		struct stdata *stpdown;

		if (stp->sd_flag & STRHUP) {
			u.u_error = ENXIO;
			return;
		}
		fpdown = getf(arg);
		if (u.u_error)
			return;
		if (fpdown->f_type != DTYPE_VNODE ||
		    !fpdown->f_data ||
		    !(stpdown = ((struct vnode *)(fpdown->f_data))->v_stream) ||
		    !(stpdown->sd_flag & STPLEX) ||
		    !(linkblkp = findlinks((queue_t *)NULL,
			stpdown->sd_wrq, 0))) {
			/* invalid user supplied file descriptor */
			u.u_error = EINVAL;
			return;
		}
		(void) munlinkone(stp, &file[linkblkp->l_index],
		    linkblkp, 0);
		return;
	    }

	case I_FLUSH:
		/*
		 * send a flush message downstream
		 * flush message can indicate
		 * FLUSHR - flush read queue
		 * FLUSHW - flush write queue
		 * FLUSHRW - flush read/write queue
		 */
		if (stp->sd_flag & STRHUP) {
			u.u_error = EINVAL;
			return;
		}
		if (arg & ~FLUSHRW) {
			u.u_error = EINVAL;
			return;
		}
		while (!putctl1(stp->sd_wrq->q_next, M_FLUSH, arg)) {
			if (strwaitbuf(1, BPRI_HI, 1))
				return;
		}

		if (qready())
			runqueues();
		return;

	case I_SRDOPT:
		/*
		 * Set read options
		 *
		 * RNORM - default stream mode
		 * RMSGN - message no discard
		 * RMSGD - message discard
		 */
		s = splstr();
		switch (arg) {
		case RNORM:
			stp->sd_flag &= ~(RMSGDIS | RMSGNODIS);
			break;
		case RMSGD:
			stp->sd_flag = (stp->sd_flag & ~RMSGNODIS) | RMSGDIS;
			break;
		case RMSGN:
			stp->sd_flag = (stp->sd_flag & ~RMSGDIS) | RMSGNODIS;
			break;
		default:
			u.u_error = EINVAL;
			break;
		}
		(void) splx(s);
		return;

	case I_GRDOPT: {
		/*
		 * Get read option and return the value
		 * to spot pointed to by arg
		 */

		*(int *)data = ((stp->sd_flag & RMSGDIS ? RMSGD :
		    (stp->sd_flag & RMSGNODIS ? RMSGN : RNORM)));
		return;
	    }

	case I_SETSIG: {
		/*
		 * Register the calling proc to receive the SIGPOLL
		 * signal based on the events given in arg.  If
		 * arg is zero, remove the proc from register list.
		 */
		struct strevent *sep, *psep;

		psep = NULL;

		s = splstr();
		for (sep = stp->sd_siglist;
		    sep && (sep->se_procp != u.u_procp);
		    psep = sep, sep = sep->se_next)
			;

		if (arg) {
			if (arg & ~(S_INPUT|S_HIPRI|S_OUTPUT|S_MSG)) {
				(void) splx(s);
				u.u_error = EINVAL;
				return;
			}

			/*
			 * If proc not already registered, add it to list
			 */
			if (!sep) {
				if (!(sep = sealloc(SE_SLEEP))) {
					(void) splx(s);
					u.u_error = EAGAIN;
					return;
				}
				if (psep)
					psep->se_next = sep;
				else
					stp->sd_siglist = sep;
				sep->se_procp = u.u_procp;
			}
			/*
			 * set events
			 */
			sep->se_events = arg;
			stp->sd_sigflags |= arg;
			(void) splx(s);
			return;
		}
		/*
		 * Remove proc from register list
		 */
		if (sep) {
			if (psep)
				psep->se_next = sep->se_next;
			else
				stp->sd_siglist = sep->se_next;
			sefree(sep);
			/*
			 * recalculate OR of sig events
			 */
			stp->sd_sigflags = 0;
			for (sep = stp->sd_siglist; sep; sep = sep->se_next)
				stp->sd_sigflags |= sep->se_events;
			(void) splx(s);
			return;
		}
		(void) splx(s);
		u.u_error = EINVAL;
		return;
	    }

	case I_GETSIG: {
		/*
		 * Return (in arg) the current registration of events
		 * for which the calling proc is to be signaled.
		 */
		struct strevent *sep;

		s = splstr();
		for (sep = stp->sd_siglist; sep; sep = sep->se_next)
			if (sep->se_procp == u.u_procp) {
				*(int *)data = sep->se_events;
				(void) splx(s);
				return;
			};
		(void) splx(s);
		u.u_error = EINVAL;
		return;
	    }

	case I_PEEK: {
		mblk_t *bp;
		register struct strpeek *strpeekp = (struct strpeek *)data;
		struct uio uio;
		struct iovec iov;
		int n;

		if (!(bp = RD(stp->sd_wrq)->q_first) ||
		    ((strpeekp->flags & RS_HIPRI) && (queclass(bp) == QNORM))){
			u.u_rval1 = 0;
			return;
		}

		if (bp->b_datap->db_type == M_PASSFP) {
			u.u_error = EBADMSG;
			return;
		}

		if (bp->b_datap->db_type == M_PCPROTO)
			strpeekp->flags = RS_HIPRI;
		else
			strpeekp->flags = 0;


		/*
		 * First process PROTO blocks, if any
		 */
		iov.iov_base = strpeekp->ctlbuf.buf;
		iov.iov_len = strpeekp->ctlbuf.maxlen;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = 0;
		uio.uio_segflg = UIO_USERSPACE;
		uio.uio_fmode = 0;
		uio.uio_resid = iov.iov_len;
		while (bp && bp->b_datap->db_type != M_DATA &&
		    uio.uio_resid >= 0) {
			if (n = MIN(uio.uio_resid, bp->b_wptr - bp->b_rptr))
				u.u_error = uiomove((caddr_t)bp->b_rptr,
						n, UIO_READ, &uio);
			if (u.u_error)
				return;
			bp = bp->b_cont;
		}
		strpeekp->ctlbuf.len = strpeekp->ctlbuf.maxlen - uio.uio_resid;

		/*
		 * Now process DATA blocks, if any
		 */
		iov.iov_base = strpeekp->databuf.buf;
		iov.iov_len = strpeekp->databuf.maxlen;
		uio.uio_iovcnt = 1;
		uio.uio_resid = iov.iov_len;
		while (bp && uio.uio_resid) {
			if (n = MIN(uio.uio_resid, bp->b_wptr - bp->b_rptr))
				u.u_error = uiomove((caddr_t)bp->b_rptr,
						n, UIO_READ, &uio);
			if (u.u_error)
				return;
			bp = bp->b_cont;
		}


		strpeekp->databuf.len =
		    strpeekp->databuf.maxlen - uio.uio_resid;
		u.u_rval1 = 1;
		return;
	    }

	case I_FDINSERT: {
		register struct strfdinsert *strfdinsertp;
		struct file *resftp;
		struct stdata *resstp;
		queue_t *q;
		mblk_t *mp;
		register msgsize;
		short rmin, rmax;
		struct uio uio;
		struct iovec iov;

		if (stp->sd_flag & (STRERR|STRHUP|STPLEX)) {
			u.u_error = ((stp->sd_flag&STPLEX) ?
			    EINVAL : stp->sd_error);
			return;
		}
		strfdinsertp = (struct strfdinsert *)data;
		if (strfdinsertp->offset < 0 ||
		    (strfdinsertp->offset % sizeof (queue_t *)) != 0) {
			u.u_error = EINVAL;
			return;
		}
		if (!(resftp = getf(strfdinsertp->fildes)) ||
		    !(resstp = ((struct vnode *)(resftp->f_data))->v_stream)) {
			u.u_error = EINVAL;
			return;
		}

		if (resstp->sd_flag & (STRERR|STRHUP|STPLEX)) {
			u.u_error = ((resstp->sd_flag&STPLEX) ?
			    EINVAL : resstp->sd_error);
			return;
		}

		/* get read queue of stream terminus */
		for (q = resstp->sd_wrq->q_next; q->q_next; q = q->q_next)
			;
		q = RD(q);

		if (strfdinsertp->ctlbuf.len <
		    (strfdinsertp->offset + sizeof (queue_t *))) {
			u.u_error = EINVAL;
			return;
		}

		/*
		 * Check for legal flag value
		 */
		if (strfdinsertp->flags & ~RS_HIPRI) {
			u.u_error = EINVAL;
			return;
		}

		/*
		 * make sure ctl and data sizes together fall within
		 * the limits of the max and min receive packet sizes
		 * and do not exceed system limit.  A negative data length
		 * means that no data part is to be sent.
		 */
		rmin = stp->sd_wrq->q_next->q_minpsz;
		rmax = stp->sd_wrq->q_next->q_maxpsz;
		if (rmax == INFPSZ)
			rmax = strmsgsz;
		else
			rmax = MIN(rmax, strmsgsz);
		msgsize = strfdinsertp->databuf.len;
		if (msgsize < 0)
			msgsize = 0;
		if ((msgsize < rmin) || (msgsize > rmax) ||
		    (strfdinsertp->ctlbuf.len > strctlsz)) {
			u.u_error = ERANGE;
			return;
		}

		s = splstr();
		while (!(strfdinsertp->flags & RS_HIPRI) &&
		    !canput(stp->sd_wrq->q_next))
			if (strwaitq(stp, WRITEWAIT, flag)) {
				(void) splx(s);
				return;
			}
		(void) splx(s);

		iov.iov_base = strfdinsertp->databuf.buf;
		iov.iov_len = strfdinsertp->databuf.len;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = 0;
		uio.uio_segflg = UIO_USERSPACE;
		uio.uio_fmode = 0;
		uio.uio_resid = iov.iov_len;
		if (!(mp = strmakemsg(&strfdinsertp->ctlbuf,
		    strfdinsertp->databuf.len, &uio,
		    (int)stp->sd_wroff, strfdinsertp->flags, 0)))
			return;

		/*
		 * place pointer to queue 'offset' bytes from the
		 * start of the control portion of the message
		 */

		*((queue_t **)(mp->b_rptr + strfdinsertp->offset)) = q;

		/*
		 * Put message downstream
		 */
		(*stp->sd_wrq->q_next->q_qinfo->qi_putp)
		    (stp->sd_wrq->q_next, mp);
		if (qready())
			runqueues();
		return;
	    }

	case I_SENDFD: {
		register queue_t *qp;
		register mblk_t *mp;
		register struct strrecvfd *srf;
		struct file *fp;

		if (stp->sd_flag & STRHUP) {
			u.u_error = ENXIO;
			return;
		}
		for (qp = stp->sd_wrq; qp->q_next; qp = qp->q_next)
			;
		if (qp->q_qinfo != &strdata) {
			u.u_error = EINVAL;
			return;
		}
		if (!(fp = getf(arg)))
			return;
		if ((qp->q_flag & QFULL) ||
		    !(mp = allocb((int)sizeof (struct strrecvfd), BPRI_MED))) {
			u.u_error = EAGAIN;
			return;
		}
		srf = (struct strrecvfd *)mp->b_rptr;
		mp->b_wptr += sizeof (struct strrecvfd);
		mp->b_datap->db_type = M_PASSFP;
		srf->f.fp = fp;
		srf->uid = u.u_uid;
		srf->gid = u.u_gid;
		fp->f_count++;
		strrput(qp, mp);
		return;
	    }

	case I_RECVFD: {
		register mblk_t *mp;
		register struct strrecvfd *srf;
		register i;

		if (stp->sd_flag & (STRERR|STPLEX)) {
			u.u_error = ((stp->sd_flag & STPLEX) ?
			    EINVAL :stp->sd_error);
			return;
		}
		s = splstr();
		while (!(mp = getq(RD(stp->sd_wrq)))) {
			if (stp->sd_flag&STRHUP) {
				(void) splx(s);
				u.u_error = ENXIO;
				return;
			}
			if (strwaitq(stp, READWAIT, flag)) {
				(void) splx(s);
				return;
			}
		}
		if (mp->b_datap->db_type != M_PASSFP) {
			putbq(RD(stp->sd_wrq), mp);
			(void) splx(s);
			u.u_error = EBADMSG;
			return;
		}
		(void) splx(s);
		srf = (struct strrecvfd *)mp->b_rptr;
		if ((i = ufalloc(0)) < 0) {
			putbq(RD(stp->sd_wrq), mp);
			return;
		}
		u.u_ofile[i] = srf->f.fp;
		srf->f.fd = i;
		bcopy((caddr_t)srf, data, sizeof (struct strrecvfd));
#ifdef	notdef
		/* XXX - can ioctl fail to copyout? */
		if (copyout((caddr_t)srf, (caddr_t)arg,
		    sizeof (struct strrecvfd))) {
			u.u_error = EFAULT;
			srf->f.fp = u.u_ofile[i];
			putbq(RD(stp->sd_wrq), mp);
			u.u_ofile[i] = NULL;
			return;
		}
#endif
		freemsg(mp);
		u.u_rval1 = 0;	/* reset value set by ufalloc() */
		return;
	    } /* case I_RECVFD */
	} /* switch */
}

/*
 * Replace one streams module that implements an old-style line discipline with
 * another; line disciplines must appear below a compatibility module.
 * Return 0 if not currently a line discipline, 1 otherwise; if "otherwise",
 * "u.u_error" is set on failure.  (If return is 0, TIOCSETD should be sent
 * downstream in case somebody there wants to respond to it.)
 */
int
ldreplace(vp, stp, newldisc)
	register struct vnode *vp;
	register struct stdata *stp;
	int newldisc;
{
	register queue_t *q;
	register int i;
	struct streamtab *current_streamtab, *new_streamtab;
	register int status;
	register int s;

	if (!ldtab_initialized)
		initialize_ldtab();

	for (;;) {
		if (vp == u.u_procp->p_sessp->s_vp &&
		    (u.u_error = tty_wr_wait(stp, 0)))
				return (1);

		/*
		 * If nobody else is doing opens, pushes, etc., grab exclusive
		 * use of the stream to prevent anyone else from doing opens,
		 * pushes, etc.  Otherwise, wait until they're done, and check
		 * foreground/background access again.
		 */
		if (!(stp->sd_flag & STWOPEN))
			break;	/* everybody else is done */
		if (sleep((caddr_t)stp, STOPRI|PCATCH)) {
			u.u_error = EINTR;
			return (1);
		}
		if (stp->sd_flag & (STRERR|STRHUP|STPLEX)) {
			u.u_error = ((stp->sd_flag&STPLEX) ?
			    EINVAL : stp->sd_error);
			return (1);
		}
	}
	s = splstr();
	stp->sd_flag |= STWOPEN;
	(void) splx(s);

	if ((q = getcompatqueue(stp)) == NULL ||
	    q->q_next == NULL) {
		status = 0;	/* stream not set up properly */
		goto done;
	}

	/*
	 * We found a module below the compatibility module.
	 * Now see if it corresponds to a line discipline.
	 * We start the search at the first entry, so that
	 * we always seem to be in the new line discipline
	 * if we're using the regular TTY module.
	 */
	for (i = 1; i < NLDISC; i++) {
		if (ldtab[i].ld_tab != NULL &&
		    q->q_next->q_qinfo == ldtab[i].ld_tab->st_wrinit)
			goto found_setd;
	}

	status = 0;	/* module isn't a line discipline */
	goto done;

found_setd:
	current_streamtab = ldtab[i].ld_tab;
	if ((i = newldisc) < 0 || i > NLDISC) {
		u.u_error = EINVAL;	/* not a valid new line discipline */
		status = 1;
		goto done;
	}
	if ((new_streamtab = ldtab[i].ld_tab) == NULL) {
		u.u_error = EINVAL;	/* not a valid new line discipline */
		status = 1;
		goto done;
	}
	if (new_streamtab == current_streamtab) {
		status = 1;	/* nothing to do */
		goto done;
	}

	/*
	 * Pop off old module.
	 */
	if (q->q_next->q_next == NULL ||
	    (q->q_next->q_next->q_flag & QREADR)) {
		u.u_error = EINVAL;
		goto done;
	}
	qdetach(RD(q->q_next), 1, 0);
	u.u_error = 0;		/* ignore errors */

	/*
	 * Push new module and call its open routine via qattach.
	 */
	i = qattach(new_streamtab, RD(q), vp->v_rdev, 0, MODOPEN);
	if (i == OPENFAIL) {
		if (!u.u_error)
			u.u_error = ENXIO;
		(void) qattach(current_streamtab, RD(q), vp->v_rdev,
		    0, MODOPEN);
	}
	status = 1;

done:
	s = splstr();
	stp->sd_flag &= ~STWOPEN;
	(void) splx(s);
	wakeup((caddr_t)stp);
	return (status);
}

static void
dopush(stp, mname, vp, waitflag, oflag)
	register struct stdata *stp;
	char *mname;
	register struct vnode *vp;
	int waitflag;
	int oflag;		/* passed in from stropen for autopushes */
{
	register int unit;
	register int s;
	int i;
	queue_t *q;

	if (stp->sd_flag & STRHUP) {
		u.u_error = ENXIO;
		return;
	}
	if (stp->sd_pushcnt >= nstrpush) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * Look up module in fmodsw.
	 */
	if ((i = findmod(mname)) < 0) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * If "waitflag" is set, wait until we have exclusive use of
	 * the stream; i.e., wait until everybody else doing opens, pushes,
	 * etc. is done, and then either quit if there is an error or
	 * set STWOPEN to lock everybody else out.
	 * If "waitflag" is not set, it is presumed that we already have
	 * exclusive use of the stream.
	 */
	if (waitflag) {
		while (stp->sd_flag & (STWOPEN|STRCLOSE)) {
			if (sleep((caddr_t)stp, STOPRI|PCATCH)) {
				u.u_error = EINTR;
				return;
			}
			if (stp->sd_flag & (STRERR|STRHUP|STPLEX)) {
				u.u_error = ((stp->sd_flag&STPLEX) ?
				    EINVAL : stp->sd_error);
				return;
			}
		}
		s = splstr();
		stp->sd_flag |= STWOPEN;
		(void) splx(s);
	}

	/*
	 * Push new module and call its open routine via qattach.
	 */
	unit = qattach(fmodsw[i].f_str, RD(stp->sd_wrq), vp->v_rdev, 0,
	    MODOPEN);
	if (unit == OPENFAIL) {
		if (!u.u_error)
			u.u_error = ENXIO;
	} else {
		/*
		 * If controlling tty established, mark the
		 * process group and tty vnode pointer.
		 */
		if ((unit & NEWCTTY) && !(oflag & FNOCTTY))
			assign_ctty(vp, stp, 0);
		stp->sd_pushcnt++;
	}

	/*
	 * If flow control is on, don't break it; enable the
	 * first back queue with a service procedure.
	 */
	if (RD(stp->sd_wrq)->q_flag & QWANTW) {
		for (q = backq(RD(stp->sd_wrq->q_next));
		    q && !q->q_qinfo->qi_srvp; q = backq(q))
			;
		if (q)
			qenable(q);
	}

	s = splstr();
	stp->sd_flag &= ~STWOPEN;
	(void) splx(s);
	wakeup((caddr_t)stp);
}

static void
initialize_ldtab()
{
	register int i;
	register ldinfo *ldtabp;

	for (ldtabp = &ldtab[0]; ldtabp < &ldtab[NLDISC]; ldtabp++) {
		for (i = 0; i < fmodcnt; i++) {
			if (strncmp(ldtabp->ld_name, fmodsw[i].f_name,
			    FMNAMESZ + 1) == 0) {
				ldtabp->ld_tab = fmodsw[i].f_str;
				break;
			}
		}
	}

	for (i = 0; i < fmodcnt; i++) {
		if (strcmp(ttcompatname, fmodsw[i].f_name) == 0) {
			ttcompat_qinit = fmodsw[i].f_str->st_wrinit;
			break;
		}
	}

	ldtab_initialized++;
}

/*
 * Search for a queue belonging to the compatibility module, if there is
 * one.
 */
static queue_t *
getcompatqueue(stp)
	register struct stdata *stp;
{
	register queue_t *q;

	q = stp->sd_wrq->q_next;
	do {
		if (ttcompat_qinit == q->q_qinfo)
			break;
	} while ((q = q->q_next) != NULL);

	return (q);
}

/*
 * Get the name of the module to which a particular queue belongs.
 */
static char *
getmname(q)
	register queue_t *q;
{
	register int i;

	for (i = 0; i < fmodcnt; i++) {
		if (fmodsw[i].f_str &&
			fmodsw[i].f_str->st_wrinit == q->q_qinfo)
			return (fmodsw[i].f_name);
	}
	return (NULL);
}

/*
 * Send an ioctl message downstream and wait for acknowledgement
 */
static void
strdoioctl(stp, strioc, copyflg, flag)
	register struct stdata *stp;
	register struct strioctl *strioc;
	int copyflg;
	int flag;
{
	register mblk_t *bp;
	register s;
	register struct iocblk *iocbp;
	extern str2time(), str3time();

	if ((strioc->ic_len < 0) || (strioc->ic_len > strmsgsz)) {
		u.u_error = EINVAL;
		return;
	}

retry:
	/*
	 * If the ioctl is a tty ioctl that involves modification,
	 * hang if in the background.
	 * If it's a TIOCSTI, do extra permission checks up here.
	 * We can't do this check on TIOCSSIZE/TIOCSWINSZ; "shelltool"
	 * sets the size on the slave side, not the controller side (which
	 * is what it should do), and it's not in the same process as
	 * the terminal so it would hang if we did the check.  In the
	 * old system, the check was in effect suppressed for pseudo-
	 * ttys, but then the old TIOCSSIZE only worked on pseudo-ttys.
	 */
	switch (strioc->ic_cmd) {

	case TIOCSTI:
		if (u.u_uid) {
			if ((flag & FREAD) == 0) {
				u.u_error = EPERM;
				return;
			}
			if (u.u_procp->p_sessp->s_vp != stp->sd_vnode) {
				u.u_error = EACCES;
				return;
			}
		}

	case TIOCSETP:
	case TIOCSETN:
	case TIOCFLUSH:
	case TIOCSETC:
	case TIOCSLTC:
	case TIOCLBIS:
	case TIOCLBIC:
	case TIOCLSET:
	case TIOCSETX:
	case TCSETA:
	case TCSETAF:
	case TCSETAW:
	case TCSETS:
	case TCSETSW:
	case TCSETSF:
	case TCFLSH:
	case TCSBRK:
	case TCXONC:
		if (stp->sd_vnode == u.u_procp->p_sessp->s_vp &&
		    (u.u_error = tty_wr_wait(stp, 0))) {
			return;
		}
		break;
	}

	/*
	 * Allocate the M_IOCTL message after passing the background and
	 * permission tests; that way a longjmp out of the sleep in
	 * tty_wr_wait (to issue a SIGTTOU) won't cause a storage leak.
	 */
	if (!(bp = allocb((int)sizeof (struct iocblk), BPRI_HI))) {
		if (strwaitbuf((int)sizeof (struct iocblk), BPRI_HI, 1))
			return;
		/*
		 * Might have gone into the background; revalidate.
		 */
		goto retry;
	}

	if (stp->sd_flag & STRHUP) {
		u.u_error = ENXIO;
		freemsg(bp);
		return;
	}

	iocbp = (struct iocblk *)bp->b_wptr;
	iocbp->ioc_count = strioc->ic_len;
	iocbp->ioc_cmd = strioc->ic_cmd;
	iocbp->ioc_uid = u.u_uid;
	iocbp->ioc_gid = u.u_gid;
	iocbp->ioc_error = 0;
	iocbp->ioc_rval = 0;
	bp->b_datap->db_type = M_IOCTL;
	bp->b_wptr += sizeof (struct iocblk);

	/*
	 * If there is data to copy into ioctl block, do so.
	 */
	if (iocbp->ioc_count && !putiocd(bp, strioc->ic_dp, copyflg)) {
		freemsg(bp);
		return;
	}
	s = splstr();

	/*
	 * Block for up to STRTIMOUT sec if there is a outstanding
	 * ioctl for this stream already pending.  All processes
	 * sleeping here will be awakened as a result of an ACK
	 * or NAK being received for the outstanding ioctl, or
	 * as a result of the timer expiring on the outstanding
	 * ioctl (a failure), or as a result of any waiting
	 * process's timer expiring (also a failure).
	 */
	stp->sd_flag |= STR2TIME;
	timeout(str2time, (caddr_t)stp, STRTIMOUT*HZ);

	while (stp->sd_flag & IOCWAIT) {
		stp->sd_iocwait++;
		if (sleep((caddr_t)&stp->sd_iocwait, STIPRI|PCATCH) ||
		    !(stp->sd_flag & STR2TIME)) {
			stp->sd_iocwait--;
			u.u_error = (stp->sd_flag&STR2TIME ? EINTR : ETIME);
			if (!stp->sd_iocwait)
				stp->sd_flag &= ~STR2TIME;
			(void) splx(s);
			untimeout(str2time, (caddr_t)stp);
			freemsg(bp);
			return;
		}
		stp->sd_iocwait--;
		if (stp->sd_flag & (STRHUP|STRERR|STPLEX)) {
			u.u_error = ((stp->sd_flag & STPLEX) ?
			    EINVAL : stp->sd_error);
			if (!stp->sd_iocwait)
				stp->sd_flag &= ~STR2TIME;
			(void) splx(s);
			untimeout(str2time, (caddr_t)stp);
			freemsg(bp);
			return;
		}
	}
	untimeout(str2time, (caddr_t)stp);
	if (!stp->sd_iocwait)
		stp->sd_flag &= ~STR2TIME;

	/*
	 * Have control of ioctl mechanism.
	 * Send down ioctl packet and wait for
	 * response.
	 */
	if (stp->sd_iocblk) {
		freemsg(stp->sd_iocblk);
		stp->sd_iocblk = NULL;
	}
	stp->sd_flag |= IOCWAIT;

	/*
	 * Assign sequence number.
	 */
	stp->sd_iocid = getiocseqno();
	iocbp->ioc_id = stp->sd_iocid;

	(void) splx(s);
	(*stp->sd_wrq->q_next->q_qinfo->qi_putp)(stp->sd_wrq->q_next, bp);
	if (qready())
		runqueues();


	/*
	 * Timed wait for acknowledgment.  The wait time is limited by the
	 * timeout value, which must be a positive integer (number of seconds
	 * to wait, or 0 (use default value of STRTIMOUT seconds), or -1
	 * (wait forever).  This will be awakened either by an ACK/NAK
	 * message arriving, the timer expiring, or the timer expiring
	 * on another ioctl waiting for control of the mechanism.
	 */
	s = splstr();
	if (strioc->ic_timout >= 0)
		timeout(str3time, (caddr_t)stp,
		    (strioc->ic_timout ? strioc->ic_timout: STRTIMOUT) * HZ);

	stp->sd_flag |= STR3TIME;
	/*
	 * If the reply has already arrived, don't sleep.  If awakened from
	 * the sleep, fail only if the reply has not arrived by then.
	 * Otherwise, process the reply.
	 */
	while (!stp->sd_iocblk) {
		if (stp->sd_flag & (STRERR|STPLEX)) {
			u.u_error = ((stp->sd_flag & STPLEX) ?
			    EINVAL : stp->sd_error);
			stp->sd_flag &= ~(STR3TIME|IOCWAIT);
			if (strioc->ic_timout >= 0)
				untimeout(str3time, (caddr_t)stp);
			(void) splx(s);
			wakeup((caddr_t)&(stp->sd_iocwait));
			return;
		}

		if (sleep((caddr_t)stp, STIPRI|PCATCH) ||
		    !(stp->sd_flag & STR3TIME))  {
			u.u_error = ((stp->sd_flag&STR3TIME) ? EINTR : ETIME);
			stp->sd_flag &= ~(STR3TIME|IOCWAIT);
			if (stp->sd_iocblk) {
				freemsg(stp->sd_iocblk);
				stp->sd_iocblk = NULL;
			}
			if (strioc->ic_timout >= 0)
				untimeout(str3time, (caddr_t)stp);
			(void) splx(s);
			wakeup((caddr_t)&(stp->sd_iocwait));
			return;
		}
	}
	ASSERT(stp->sd_iocblk);
	bp = stp->sd_iocblk;
	stp->sd_iocblk = NULL;
	stp->sd_flag &= ~(STR3TIME|IOCWAIT);
	if (strioc->ic_timout >= 0)
		untimeout(str3time, (caddr_t)stp);
	(void) splx(s);
	wakeup((caddr_t)&(stp->sd_iocwait));


	/*
	 * Have received acknowlegment
	 */

	iocbp = (struct iocblk *)bp->b_rptr;
	switch (bp->b_datap->db_type) {
	case M_IOCACK:
		/*
		 * Positive ack
		 */

		/*
		 * set error if indicated
		 */
		if (iocbp->ioc_error) {
			u.u_error = iocbp->ioc_error;
			break;
		}

		/*
		 * set return value
		 */
		u.u_rval1 = iocbp->ioc_rval;

		/*
		 * Data may have been returned in ACK message (ioc_count > 0).
		 * If so, copy it out to the user's buffer.
		 */
		if (iocbp->ioc_count)
			if (!getiocd(bp, strioc->ic_dp, copyflg))
				break;
		strioc->ic_len = iocbp->ioc_count;
		break;

	case M_IOCNAK:
		/*
		 * Negative ack
		 *
		 * The only thing to do is set error as specified
		 * in neg ack packet
		 */
		u.u_error = (iocbp->ioc_error ? iocbp->ioc_error : EINVAL);
		break;

	default:
		ASSERT(0);
		break;
	}

	freemsg(bp);
}



/*
 * Return the next available "ioctl" sequence number.
 * Exported, so that streams modules can send "ioctl" messages downstream
 * from their open routine.
 */
int
getiocseqno()
{
	return (++ioc_id);
}

/*
 * Get the next message from the read queue.  If the message is
 * priority, STRPRI will have been set by strrput().  This flag
 * should be reset only when the entire message at the front of the
 * queue as been consumed.
 */
int
strgetmsg(vp, mctl, mdata, flagp, fmode)
	register struct vnode *vp;
	register struct strbuf *mctl;
	register struct strbuf *mdata;
	register caddr_t flagp;
	int fmode;
{
	register s;
	register struct stdata *stp;
	register mblk_t *bp, *nbp;
	mblk_t *savemp = NULL;
	mblk_t *savemptail = NULL;
	int rflag;
	int flg = 0;
	int more = 0;
	int n;
	int bcnt;
	char *ubuf;


	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if (stp->sd_flag & (STRERR|STPLEX)) {
		u.u_error = ((stp->sd_flag&STPLEX) ? EINVAL : stp->sd_error);
		return (0);
	}
	if (copyin(flagp, (caddr_t)&rflag, sizeof (int))) {
		u.u_error = EFAULT;
		return (0);
	}
	if (rflag & (~RS_HIPRI)) {
		u.u_error = EINVAL;
		return (0);
	}

	s = splstr();
	stp->sd_deficit = 0;	/* force wakeup on any input */
	while (((rflag & RS_HIPRI) && !(stp->sd_flag & STRPRI)) ||
	    !(bp = getq(RD(stp->sd_wrq)))) {
		/*
		 * If STRHUP, return 0 length control and data
		 */
		if (stp->sd_flag & STRHUP) {
			mctl->len = mdata->len = 0;
			if (copyout((caddr_t)&flg, flagp, sizeof (int)))
				u.u_error = EFAULT;
			(void) splx(s);
			return (0);
		}
		if (strwaitq(stp, READWAIT, fmode)) {
			(void) splx(s);
			return (0);
		}
	}
	(void) splx(s);

	if (bp->b_datap->db_type == M_PASSFP) {
		putbq(RD(stp->sd_wrq), bp);
		u.u_error = EBADMSG;
		return (0);
	}

	if (qready())
		runqueues();

	/*
	 * Set HIPRI flag if message is priority.
	 */
	if (stp->sd_flag & STRPRI)
		flg |= RS_HIPRI;

	/*
	 * First process PROTO or PCPROTO blocks, if any.
	 */
	if (mctl->maxlen >= 0 && bp && bp->b_datap->db_type != M_DATA) {
		bcnt = mctl->maxlen;
		ubuf = mctl->buf;
		while (bp && bp->b_datap->db_type != M_DATA && bcnt >= 0) {
			if ((n = MIN(bcnt, bp->b_wptr - bp->b_rptr)) &&
			    copyout((caddr_t)bp->b_rptr, ubuf, (u_int)n)) {
				u.u_error = EFAULT;
				s = splstr();
				stp->sd_flag &= ~STRPRI;
				(void) splx(s);
				more = 0;
				freemsg(bp);
				goto getmout;
			}
			ubuf += n;
			bp->b_rptr += n;
			if (bp->b_rptr >= bp->b_wptr) {
				nbp = bp;
				bp = bp->b_cont;
				freeb(nbp);
			}
			if ((bcnt -= n) <= 0)
				break;
		}
		mctl->len = mctl->maxlen - bcnt;
	} else
		mctl->len = -1;


	if (bp && bp->b_datap->db_type != M_DATA) {
		/*
		 * more PROTO blocks in msg
		 */
		more |= MORECTL;
		savemp = bp;
		while (bp && bp->b_datap->db_type!=M_DATA) {
			savemptail = bp;
			bp = bp->b_cont;
		}
		savemptail->b_cont = NULL;
	}

	/*
	 * Now process DATA blocks, if any
	 */
	if (mdata->maxlen >= 0 && bp) {
		bcnt = mdata->maxlen;
		ubuf = mdata->buf;
		while (bp && bcnt >= 0) {
			if ((n = MIN(bcnt, bp->b_wptr - bp->b_rptr)) &&
			    copyout((caddr_t)bp->b_rptr, ubuf, (u_int)n)) {
				u.u_error = EFAULT;
				s = splstr();
				stp->sd_flag &= ~STRPRI;
				(void) splx(s);
				more = 0;
				freemsg(bp);
				goto getmout;
			}
			ubuf += n;
			bp->b_rptr += n;
			if (bp->b_rptr >= bp->b_wptr) {
				nbp = bp;
				bp = bp->b_cont;
				freeb(nbp);
			}
			if ((bcnt -= n) <= 0)
				break;
		}
		mdata->len = mdata->maxlen - bcnt;
	} else
		mdata->len = -1;

	if (bp) {			/* more data blocks in msg */
		more |= MOREDATA;
		if (savemp)
			savemptail->b_cont = bp;
		else
			savemp = bp;
	}

	s = splstr();
	if (savemp)
		putbq(RD(stp->sd_wrq), savemp);
	else
		stp->sd_flag &= ~STRPRI;
	(void) splx(s);

	if (copyout((caddr_t)&flg, flagp, sizeof (int)))
		u.u_error = EFAULT;

	/*
	 * Getmsg cleanup processing - if the state of the queue has changed
	 * some signals may need to be sent and/or poll awakened.
	 */
getmout:
	while ((bp=RD(stp->sd_wrq)->q_first) && (bp->b_datap->db_type==M_SIG)) {
		bp = getq(RD(stp->sd_wrq));
		switch (*bp->b_rptr) {
		case SIGPOLL:
			if (stp->sd_sigflags & S_MSG)
				strsendsig(stp->sd_siglist, S_MSG);
			break;

		default:
			if (stp->sd_pgrp)
				gsignal(stp->sd_pgrp, (int)*bp->b_rptr);
			break;
		}
		freemsg(bp);
		if (qready())
			runqueues();
	}
	/*
	 * If we have just received a high priority message and a
	 * regular message is now at the front of the queue, send
	 * signals in S_INPUT processes and wake up processes polling
	 * on POLLIN or selecting on input.
	 */
	if (RD(stp->sd_wrq)->q_first &&
	    !(stp->sd_flag & STRPRI) && (flg & RS_HIPRI)) {
		s = splstr();
		if (stp->sd_selr) {
			selwakeup(stp->sd_selr,
			    (stp->sd_flag & RCOLL) != 0);
			stp->sd_selr = 0;
			stp->sd_flag &= ~RCOLL;
		}
		if (stp->sd_sigflags & S_INPUT)
			strsendsig(stp->sd_siglist, S_INPUT);
		if (stp->sd_pollflags & POLLIN)
			strwakepoll(stp, POLLIN);
		(void) splx(s);
	}
	return (more);
}


/*
 * Put a message downstream
 */
strputmsg(vp, mctl, mdata, flag, fmode)
	register struct vnode *vp;
	register struct strbuf *mctl;
	register struct strbuf *mdata;
	register flag;
	int fmode;
{
	register struct stdata *stp;
	mblk_t *mp;
	register s;
	register msgsize;
	short rmin, rmax;
	struct uio uio;
	struct iovec iov;

	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if (stp->sd_flag & (STRHUP|STRERR|STPLEX)) {
		u.u_error = ((stp->sd_flag&STPLEX) ? EINVAL : stp->sd_error);
		return;
	}

	/*
	 * Check for legal flag value
	 */
	if ((flag & ~RS_HIPRI) || ((flag & RS_HIPRI) && (mctl->len < 0))) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * make sure ctl and data sizes together fall within the limits of the
	 * max and min receive packet sizes and do not exceed system limit
	 */
	rmin = stp->sd_wrq->q_next->q_minpsz;
	rmax = stp->sd_wrq->q_next->q_maxpsz;
	if (rmax == INFPSZ)
		rmax = strmsgsz;
	else
		rmax = MIN(rmax, strmsgsz);
	msgsize = mdata->len;
	if (msgsize < 0) {
		msgsize = 0;
		rmin = 0;	/* no range check for NULL data part */
	}
	if (msgsize < rmin || msgsize > rmax || mctl->len > strctlsz) {
		u.u_error = ERANGE;
		return;
	}

	iov.iov_base = mdata->buf;
	iov.iov_len = mdata->len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_segflg = UIO_USERSPACE;
	uio.uio_fmode = 0;
	uio.uio_resid = iov.iov_len;

	s = splstr();
	while (!(flag & RS_HIPRI) && !canput(stp->sd_wrq->q_next)) {
		if (strwaitq(stp, WRITEWAIT, fmode)) {
			(void) splx(s);
			return;
		}
	}
	(void) splx(s);

	if (!(mp = strmakemsg(mctl, mdata->len, &uio, (int)stp->sd_wroff,
	    (long)flag, 0)))
		return;

	/*
	 * Put message downstream
	 */
	(*stp->sd_wrq->q_next->q_qinfo->qi_putp)(stp->sd_wrq->q_next, mp);
	if (qready())
		runqueues();
}



/*
 * Determines whether the necessary conditions are set on a stream
 * for it to be readable, writeable, or have exceptions; used by the "poll"
 * system call.
 */
int
strpoll(stp, events, anyyet)
	register struct stdata *stp;
	short events;
{
	register retevents = 0;
	register s;
	register struct strevent *sep;

	if (stp->sd_flag & STPLEX)
		return (POLLNVAL);

	s = splstr();
	if (stp->sd_flag & STRERR) {
		(void) splx(s);
		return (POLLERR);
	}

	if (stp->sd_flag & STRHUP)
		retevents |= POLLHUP;

	if ((events & POLLOUT) &&
	    canput(stp->sd_wrq->q_next) &&
	    !(stp->sd_flag & STRHUP))
		retevents |= POLLOUT;

	if ((events & POLLIN) &&
	    !(stp->sd_flag & STRPRI) &&
	    RD(stp->sd_wrq)->q_first)
		retevents |= POLLIN;

	if ((events & POLLPRI) &&
	    (stp->sd_flag & STRPRI))
		retevents |= POLLPRI;

	ASSERT((retevents & (POLLPRI|POLLIN)) != (POLLPRI|POLLIN));

	if (retevents) {
		(void) splx(s);
		pollreset(stp);
		return (retevents);
	}


	/*
	 * if poll() has not found any events yet, set up event cell
	 * to wake up the poll if a requested event occurs on this
	 * stream.  This will occasionally result in adding an event
	 * cell on top of one that's already there, but both will get
	 * cleaned up in the pollout: section of poll() in str_syscalls.c.
	 * Hence, it is not worth checking for here.
	 */
	if (!anyyet) {
		if (sep = sealloc(SE_SLEEP)) {
			sep->se_procp = u.u_procp;
			sep->se_events = events;
			sep->se_next = stp->sd_pollist;
			stp->sd_pollist = sep;
			stp->sd_pollflags |= events;
			(void) splx(s);
			return (0);
		}
		(void) splx(s);
		pollreset(stp);
		u.u_error = EAGAIN;
		return (0);
	}
	/*
	 * If we get here the poll has already found an event but
	 * there are no return events for this stream. Remove
	 * any event cell for this process from the pollist, and
	 * recalculate the pollflags.
	 * A cell will exist if this stream was previously scanned
	 * in this poll().
	 */
	(void) splx(s);
	pollreset(stp);
	return (retevents);
}

/*
 * Determines whether the necessary conditions are set on a stream
 * for it to be readable, writeable, or have exceptions; used by the "select"
 * system call.
 */
int
strselect(vp, which)
	register struct vnode *vp;
	int which;
{
	register struct stdata *stp;
	register struct proc *p;
	register int s;

	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if (stp->sd_flag & STPLEX) {
		u.u_error = EINVAL;
		return (0);
	}

	s = splstr();
	if (stp->sd_flag & STRERR) {
		(void) splx(s);
		u.u_error = stp->sd_error;
		return (0);
	}

	switch (which) {

	case FREAD:
		if (!(stp->sd_flag & STRPRI) && RD(stp->sd_wrq)->q_first) {
			/*
			 * Regular data is available.
			 */
			(void) splx(s);
			return (1);
		}
		if ((p = stp->sd_selr) && p->p_wchan == (caddr_t)&selwait)
			stp->sd_flag |= RCOLL;
		else
			stp->sd_selr = u.u_procp;
		break;

	case FWRITE:
		if (canput(stp->sd_wrq->q_next) && !(stp->sd_flag & STRHUP)) {
			(void) splx(s);
			return (1);
		}
		if ((p = stp->sd_selw) && p->p_wchan == (caddr_t)&selwait)
			stp->sd_flag |= WCOLL;
		else
			stp->sd_selw = u.u_procp;
		break;
	case 0:
		if (stp->sd_flag & STRPRI) {
			(void) splx(s);
			return (1);
		}
		if ((p = stp->sd_sele) && p->p_wchan == (caddr_t)&selwait)
			stp->sd_flag |= ECOLL;
		else
			stp->sd_sele = u.u_procp;
		break;
	}
	(void) splx(s);
	return (0);
}

/*
 * Attach a stream device or module.
 * qp is a read queue; the new queue goes in so its next
 * read ptr is the argument, and the write queue corresponding
 * to the argument points to this queue.  Return the return
 * value given us by the module or driver's open routine, or
 * OPENFAIL if something else altogether goes wrong.
 */
static int
qattach(qinfo, qp, dev, oflag, sflag)
	register struct streamtab *qinfo;
	register queue_t *qp;
	dev_t dev;
	int oflag, sflag;
{
	register queue_t *rq;
	register int s;
	register int unit;

	if (!(rq = allocq()))
		return (OPENFAIL);

	s = splstr();
	rq->q_next = qp;
	WR(rq)->q_next = WR(qp)->q_next;
	if (WR(qp)->q_next) {
		ASSERT(sflag == MODOPEN);
		OTHERQ(WR(qp)->q_next)->q_next = rq;
	}
	WR(qp)->q_next = WR(rq);
	setq(rq, qinfo->st_rdinit, qinfo->st_wrinit);
	rq->q_flag |= QWANTR;
	WR(rq)->q_flag |= QWANTR;

	/*
	 * Open the attached module or driver.
	 * The open may sleep, but it must always return here.  Therefore,
	 * all sleeps must set PCATCH or ignore all signals to avoid a
	 * longjump if a signal arrives.
	 */
	unit = (*rq->q_qinfo->qi_qopen)(rq, dev, oflag, sflag);
	if (unit == OPENFAIL) {
		qdetach(rq, 0, 0);
		(void) splx(s);
		return (OPENFAIL);
	}
	(void) splx(s);
	return (unit);
}

/*
 * Detach a stream module or device.
 * If clmode == 1 then the module or driver was opened and its
 * close routine must be called.  If clmode == 0, the module
 * or driver was never opened or the open failed, and so its close
 * should not be called.
 */
static int
qdetach(qp, clmode, flag)
	register queue_t *qp;
{
	register s;
	register queue_t *q, *prev = NULL;

	if (clmode) {
		if (qready())
			runqueues();
		s = splstr();
		(*qp->q_qinfo->qi_qclose)(qp, (qp->q_next ? 0 : flag));
		/*
		 * Check if queues are still enabled, and remove from
		 * runlist if necessary.
		 */
		if (qp->q_flag & QENAB || (WR(qp)->q_flag) & QENAB) {
			for (q = qhead; q; q = q->q_link)  {
				if (q == qp || q == WR(qp)) {
					if (prev)
						prev->q_link = q->q_link;
					else
						qhead = q->q_link;
					if (q == qtail)
						qtail = prev;
				}
				prev = q;
			}
		}
		flushq(qp, FLUSHALL);
		flushq(WR(qp), FLUSHALL);
	} else
		s = splstr();

	if (WR(qp)->q_next)
		backq(qp)->q_next = qp->q_next;
	if (qp->q_next)
		backq(WR(qp))->q_next = WR(qp)->q_next;
	freeq(qp);
	(void) splx(s);
}


/*
 * Are we ignoring or blocking the sig or is the process in vfork?
 */
#define	SHOULDSTOP(sig)  (!(p->p_sigignore & sigmask((sig))) && \
			    !(p->p_sigmask & sigmask((sig))) && \
			    !(p->p_flag & SVFORK))

/*
 * return 0 when the current process can read the tty "stp".
 * if it can't read the tty, return errno why not.
 */
tty_rd_wait(stp)
	struct stdata *stp;
{
	register struct proc *p = u.u_procp;

	/*
	 * while in background
	 *	if orphaned or don't want to stop
	 *		EIO
	 *	signal TTIN and sleep
	 */
	while (p->p_pgrp != stp->sd_pgrp) {
		if ((p->p_flag & SORPHAN) || !SHOULDSTOP(SIGTTIN))
			return (EIO);
		gsignal(p->p_pgrp, SIGTTIN);
		(void) sleep((caddr_t)&lbolt, STIPRI);
	}
	return (0);
}

/*
 * return 0 when the current process can write the tty "stp".
 * if it can't write the tty, return errno why not.
 * if flg is set, check for TOSTOP as well as normal checks.
 */
tty_wr_wait(stp, flg)
	struct stdata *stp;
{
	register struct proc *p = u.u_procp;

	/*
	 * while in background
	 *	if checking tostop flag and tostop clear OR
	 *	   don't want to stop
	 *		do the write
	 *	if orphaned
	 *		EIO
	 *	signal TTOU and sleep
	 */
	while (p->p_pgrp != stp->sd_pgrp) {
		if ((flg && !(stp->sd_flag & STRTOSTOP)) ||
		    !SHOULDSTOP(SIGTTOU))
			return (0);
		if (p->p_flag & SORPHAN)
			return (EIO);
		gsignal(p->p_pgrp, SIGTTOU);
		(void) sleep((caddr_t)&lbolt, STOPRI);
	}
	return (0);
}
#undef	SHOULDSTOP(stp)


/*
 * This function is placed in the callout table to wake up a process
 * waiting to close a stream that has not completely drained.
 */
static int
strtime(stp)
	struct stdata *stp;
{

	if (stp->sd_flag & STRTIME) {
		wakeup((caddr_t)stp->sd_wrq);
		stp->sd_flag &= ~STRTIME;
	}
}

/*
 * This function is placed in the callout table to wake up all
 * processes waiting to send an ioctl down a particular stream,
 * as well as the process whose ioctl is still outstanding.  The
 * process placing this function in the callout table will remove
 * it if he gets control of the ioctl mechanism for the stream -
 * this should only run if there is a failure.  This wakes up
 * the same processes as str3time below.
 */
static int
str2time(stp)
	struct stdata *stp;
{

	if (stp->sd_flag & STR2TIME) {
		wakeup((caddr_t)&stp->sd_iocwait);
		stp->sd_flag &= ~STR2TIME;
	}
}

/*
 * This function is placed on the callout table to wake up the
 * the process that has an outstanding ioctl waiting acknowledgement
 * on a stream, as well as any processes waiting to send their
 * own ioctl messages.  It should be removed from the callout table
 * when the acknowledgement arrives.  If this function runs, it
 * is the result of a failure.  This wakes up the same processes
 * as str2time above.
 */
static int
str3time(stp)
	struct stdata *stp;
{

	if (stp->sd_flag & STR3TIME) {
		wakeup((caddr_t)stp);
		stp->sd_flag &= ~STR3TIME;
	}
}

/*
 * Put ioctl data from kernel or user buffer to message.
 * Return 0 for failure, 1 for success.
 */
static int
putiocd(bp, arg, copymode)
	register mblk_t *bp;
	caddr_t arg;
	int copymode;
{
	register mblk_t *tmp;
	register int count, n;

	count = ((struct iocblk *)bp->b_rptr)->ioc_count;

	/*
	 * strdoioctl validates ioc_count, so if this assert fails it
	 * cannot be due to user error.
	 */
	ASSERT(count >= 0);

	while (count) {
		n = MIN(MAXIOCBSZ, count);
		if (!(tmp = allocb(n, BPRI_HI))) {
			u.u_error = EAGAIN;
			return (0);
		}
		switch (copymode) {

		case K_TO_K:
		case K_TO_K_BOTH:
			bcopy((caddr_t)arg, (caddr_t)tmp->b_wptr, (u_int)n);
			break;

		case U_TO_K:
			if (copyin((caddr_t)arg, (caddr_t)tmp->b_wptr,
			    (u_int)n)) {
				freeb(tmp);
				u.u_error = EFAULT;
				return (0);
			}
			break;

		default:
			ASSERT(0);
			freeb(tmp);
			return (0);
		}
		arg += n;
		tmp->b_datap->db_type = M_DATA;
		tmp->b_wptr += n;
		count -= n;
		bp = (bp->b_cont = tmp);
	}
	return (1);
}

/*
 * Get ioctl data from message into kernel or user buffer.
 * Return 0 for failure, 1 for success.
 */
static int
getiocd(bp, arg, copymode)
	register mblk_t *bp;
	caddr_t arg;
	int copymode;
{
	register int count, n;

	count = ((struct iocblk *)bp->b_rptr)->ioc_count;
	ASSERT(count >= 0);

	for (bp = bp->b_cont;
	    bp && count; count -= n, bp = bp->b_cont, arg += n) {
		n = MIN(count, bp->b_wptr - bp->b_rptr);
		switch (copymode) {

		case K_TO_K_BOTH:
			bcopy((caddr_t)bp->b_rptr, (caddr_t)arg, (u_int)n);
			break;

		case K_TO_K:
		case U_TO_K:
			if (copyout((caddr_t)bp->b_rptr, (caddr_t)arg,
			    (u_int)n)) {
				u.u_error = EFAULT;
				return (0);
			}
			break;

		default:
			ASSERT(0);
			return (0);
		}
	}
	ASSERT(count==0);
	return (1);
}



/*
 * allocate a linkblk table entry for the triple:
 * (write queue of bottom module of top stream, write queue of stream head of
 * bottom stream, file table index number)
 *
 * linkblk table entries are freed by nulling the l_qbot field.
 */
static struct linkblk *
alloclink(qup, qdown, index)
	queue_t *qup, *qdown;
	int index;
{
	register struct linkblk *linkblkp;

	for (linkblkp = &linkblk[0]; linkblkp < &linkblk[nmuxlink]; linkblkp++)
		if (!linkblkp->l_qbot) {
			linkblkp->l_qtop = qup;
			linkblkp->l_qbot = qdown;
			linkblkp->l_index = index;
			return (linkblkp);
		}

	return (NULL);
}


/*
 * Check for a potential linking cycle.
 * Qup is the upper queue in a multiplexor that is going to be linked.
 * Qdown is initially the queue to be linked below the multiplexor.
 * Linkcycle() is called to recursively scan the tree of links
 * rooted at qdown to determine if qup is contained in that tree.
 */
static int
linkcycle(qup, qdown)
	register queue_t *qup, *qdown;
{
	register struct linkblk *linkblkp;

	for (linkblkp = &linkblk[0]; linkblkp < &linkblk[nmuxlink]; linkblkp++){
		/*
		 * The first clause of the test verifies that the linkblk
		 * we're checking is actually in use.
		 */
		if (linkblkp->l_qbot != NULL && linkblkp->l_qtop == qdown)
			if ((linkblkp->l_qbot == qup) ||
			    linkcycle(qup, linkblkp->l_qbot))
				return (1);
	}
	return (0);
}


/*
 * find linkblk table entry corresponding to triple.
 * A NULL parameter means any value matches that member of the triple.
 * Return pointer to linkblk entry.
 */
struct	linkblk *
findlinks(qup, qdown, index)
	register queue_t *qup, *qdown;
	int index;
{
	register struct linkblk *linkblkp;

	ASSERT(qup || qdown || index);

	for (linkblkp = &linkblk[0]; linkblkp < &linkblk[nmuxlink]; linkblkp++){
		if (linkblkp->l_qbot &&
		    (!qup || (qup == linkblkp->l_qtop)) &&
		    (!qdown || (qdown == linkblkp->l_qbot)) &&
		    (!index || (index == linkblkp->l_index)))
			return (linkblkp);
	}
	return (NULL);
}


/*
 * given a queue ptr, follow the chain of q_next pointers until you reach the
 * last queue on the chain and return it
 */
queue_t *
getendq(q)
	register queue_t *q;
{
	while (q->q_next)
		q = q->q_next;
	return (q);
}


/*
 * unlink a multiplexer link.  Stp is the controlling stream for the
 * link, fpdown is the file pointer for the lower stream, and
 * linkblkp points to the link's entry in the linkblk table.
 */
static int
munlinkone(stp, fpdown, linkblkp, cflag)
	struct stdata *stp;
	struct file *fpdown;
	struct linkblk *linkblkp;
	int cflag;
{
	register int s;
	struct strioctl strioc;
	struct stdata *stpdown;
	queue_t *rq;

	strioc.ic_cmd = I_UNLINK;
	strioc.ic_timout = 0;
	strioc.ic_len = sizeof (struct linkblk);
	strioc.ic_dp = (char *) linkblkp;

	strdoioctl(stp, &strioc, K_TO_K, 0);

	/*
	 * If there was an error and this is not called via strclose,
	 * return to the user.  Otherwise, pretend there was no error
	 * and close the link.
	 */
	if (u.u_error) {
		if (cflag) {
			log(LOG_WARNING,
		"munlink: could not perform unlink ioctl, closing anyway\n");
			u.u_error = 0;
			/* allows strdoioctl() to work */
			s = splstr();
			stp->sd_flag &= ~STRERR;
			(void) splx(s);
		} else
			return (-1);
	}

	stpdown = ((struct vnode *)(fpdown->f_data))->v_stream;
	s = splstr();
	stpdown->sd_flag &= ~STPLEX;
	(void) splx(s);
	rq = RD(stpdown->sd_wrq);
	setq(rq, &strdata, &stwdata);
	rq->q_ptr = WR(rq)->q_ptr = (caddr_t)stpdown;
	linkblkp->l_qbot = NULL;
	closef(fpdown);
	return (0);
}



/*
 * unlink all multiplexer links for which stp is the controlling stream.
 */
static void
munlinkall(stp, cflag)
	register struct stdata *stp;
	int cflag;
{

	register struct file *fpdown;
	register struct linkblk *linkblkp;
	register queue_t *qup;

	qup = getendq(stp->sd_wrq);
	while (linkblkp = findlinks(qup, (queue_t *)NULL, 0)) {
		fpdown = &file[linkblkp->l_index];
		ASSERT((fpdown >= file) && (fpdown < fileNFILE));
		if (munlinkone(stp, fpdown, linkblkp, cflag) == -1) {
			ASSERT(0);
			return;
		}
	}
	return;
}

/*
 * Set the interface values for a pair of queues (qinit structure,
 * packet sizes, water marks).
 */
void
setq(rq, rinit, winit)
	queue_t *rq;
	struct qinit *rinit, *winit;
{
	register queue_t  *wq;
	register int s;

	wq = WR(rq);
	s = splstr();
	rq->q_qinfo = rinit;
	rq->q_hiwat = rinit->qi_minfo->mi_hiwat;
	rq->q_lowat = rinit->qi_minfo->mi_lowat;
	rq->q_minpsz = rinit->qi_minfo->mi_minpsz;
	rq->q_maxpsz = rinit->qi_minfo->mi_maxpsz;
	wq->q_qinfo = winit;
	wq->q_hiwat = winit->qi_minfo->mi_hiwat;
	wq->q_lowat = winit->qi_minfo->mi_lowat;
	wq->q_minpsz = winit->qi_minfo->mi_minpsz;
	wq->q_maxpsz = winit->qi_minfo->mi_maxpsz;
	(void) splx(s);
}

/*
 * Make a protocol message given control and data buffers
 */
mblk_t *
strmakemsg(mctl, count, uio, wroff, flag, nowait)
	register struct strbuf *mctl;
	int count;
	struct uio *uio;
	int wroff;
	long flag;
	int nowait;
{
	register mblk_t *mp = NULL;
	register mblk_t *bp;
	caddr_t base;
	int pri;
	int msgtype;
	int offlg = 0;

	if (flag & RS_HIPRI)
		pri = BPRI_MED;
	else
		pri = BPRI_LO;

	/*
	 * Create control part of message, if any
	 */
	if (mctl != NULL && mctl->len >= 0) {
		register int	ctlcount;

		if (flag & RS_HIPRI)
			msgtype = M_PCPROTO;
		else
			msgtype = M_PROTO;

		ctlcount = mctl->len;
		base = mctl->buf;

		/*
		 * range checking has already been done, simply try
		 * to allocate a message block for the ctl part.
		 */
		while (!(bp = allocb(ctlcount, pri))) {
			if (nowait) {
				u.u_error = EWOULDBLOCK;
				return (NULL);
			}
			if (strwaitbuf(ctlcount, pri, 1))
				return (NULL);
		}

		bp->b_datap->db_type = msgtype;
		if (copyin(base, (caddr_t)bp->b_wptr, (u_int)ctlcount)) {
			u.u_error = EFAULT;
			freeb(bp);
			return (NULL);
		}
		bp->b_wptr += ctlcount;
		mp = bp;
	}

	/*
	 * Create data part of message, if any.
	 *
	 * This code relies on strmsgsz being positive, so that it constrains
	 * the maximum message size.  If we change the system to allow
	 * unbounded sizes, then the code below should be changed to fragment
	 * the message through the b_cont field, probably with each chunk no
	 * larger than the system page size.
	 */
	if (count >= 0) {
		register int size;

		size = count + (offlg ? 0 : wroff);
		while ((bp = allocb(size, pri)) == NULL) {
			if (nowait) {
				freemsg(mp);
				u.u_error = EWOULDBLOCK;
				return (NULL);
				}
			if (strwaitbuf(size, pri, 1)) {
				freemsg(mp);
				return (NULL);
			}
		}

		if (wroff && !offlg++ &&
		    (wroff < bp->b_datap->db_lim - bp->b_wptr)) {
			bp->b_rptr += wroff;
			bp->b_wptr += wroff;
		}
		size = MIN(count, bp->b_datap->db_lim - bp->b_wptr);
		if (size) {
			u.u_error = uiomove((caddr_t)bp->b_wptr, size,
			    UIO_WRITE, uio);
			if (u.u_error) {
				freeb(bp);
				freemsg(mp);
				return (NULL);
			}
		}
		bp->b_wptr += size;
		count -= size;
		if (!mp)
			mp = bp;
		else
			linkb(mp, bp);
	}
	return (mp);
}

/*
 * Wait for a buffer to become available.  Return 1 if not able to wait,
 * 0 if buffer is probably there.  'ncalls' is # of bufcall() attempts.
 *
 * XXX:	The ncalls argument should now be obsolete, given that the class-based
 *	buffer allocation method is gone, but rather than chase down all
 *	callers, we leave the argument in place for now.
 */
/* ARGSUSED */
int
strwaitbuf(size, pri, ncalls)
	int size, pri, ncalls;
{
	register int s;

	/* spl: else may set process running before it goes to sleep */
	s = splstr();
	if (!bufcall((u_int)size, pri, wakeup, (long)&(u.u_procp->p_flag))) {
		(void) splx(s);
		u.u_error = ENOSR;
		return (1);
	}

	if (sleep((caddr_t)&(u.u_procp->p_flag), STOPRI|PCATCH)) {
		(void) splx(s);
		strunbcall(size, u.u_procp);
		u.u_error = EINTR;
		return (1);
	}
	(void) splx(s);
	strunbcall(size, u.u_procp);
	return (0);
}

/*
 * Remove a setrun for the given process from the bufcall list for
 * the given buffer size.  'size' can be -1 for externally-supplied buffers.
 */
/* ARGSUSED */
void
strunbcall(size, p)
	int size;
	struct proc *p;
{
	/*
	 * This routine is now a no-op.  We no longer use setrun as the
	 * bufcall function for strwaitq et al, but instead use wakeup.  Since
	 * doing redundant wakeups is harmless (albeit potentially
	 * inefficient), it's not worthwhile trying to purge these bufcalls.
	 */
}

/*
 * This function waits for a read or write event to happen on a stream.
 *
 * flag dictates how to wait.  It is composed as follows.  Exactly one of
 * READWAIT or WRITEWAIT must be set; the value determines sleep address,
 * priority, etc.  Setting NOINTR forces u.u_error to remain unchanged when
 * blocking is disallowed or an interrupt has broken the sleep, so that the
 * caller can return successfully.  Setting INTRRESTART arranges to restart
 * the system call if an interrupt breaks the sleep.
 *
 * Return value is 0 for a successfully completed wait and 1 for failure (with
 * no possibility of successful retry).
 */
static int
strwaitq(stp, flag, fmode)
	register struct stdata *stp;
	int flag;
	int fmode;
{
	register int s;
	int slpflg, slppri, errs;
	caddr_t slpadr;

	/*
	 * Check for nonblocking i/o.  If any of the variations are in effect,
	 * the call returns; respecting the NOINTR setting.
	 */
	if (fmode & FNONBIO) {
		if (!(flag & NOINTR))
			u.u_error = EAGAIN;
		return (1);
	}
	/*
	 * 4.2BSD-style non-blocking I/O (no need to test for FNDELAY,
	 * if it's set then NBIO is also set in sd_flag).
	 */
	if (stp->sd_flag & NBIO) {
		if (!(flag & NOINTR))
			u.u_error = EWOULDBLOCK;
		return (1);
	}
	if (fmode & FNBIO) {
		if (!(flag & NOINTR))
			u.u_error = EAGAIN;
		return (1);
	}

	/*
	 * Set sleep address, priority, etc.
	 */
	if (flag & READWAIT) {
		slpflg = RSLEEP;
		slpadr = (caddr_t)RD(stp->sd_wrq);
		slppri = STIPRI;
		errs = STRERR|STPLEX;
	} else {
		slpflg = WSLEEP;
		slpadr = (caddr_t)stp->sd_wrq;
		slppri = STOPRI;
		errs = STRERR|STRHUP|STPLEX;
	}

	s = splstr();
	stp->sd_flag |= slpflg;
	if (sleep(slpadr, slppri|PCATCH)) {
		stp->sd_flag &= ~slpflg;
		(void) splx(s);
		wakeup(slpadr);
		if (!(flag & NOINTR)) {
			/*
			 * NOINTR not set; therefore, either set error or
			 * restart.
			 */
			if (!(flag & INTRRESTART) ||
			    (u.u_sigintr & sigmask(u.u_procp->p_cursig)))
				u.u_error = EINTR;
			else
				u.u_eosys = RESTARTSYS;
		}
		return (1);
	}
	(void) splx(s);
	if (stp->sd_flag & errs) {
		u.u_error = ((stp->sd_flag & STPLEX) ? EINVAL : stp->sd_error);
		return (1);
	}
	return (0);
}

static int
strvtime(stp)
	register struct stdata *stp;
{
	stp->sd_flag &= ~CLKRUNNING;
	if (stp->sd_vtime == 0)
		return;		/* VTIME not in effect */
	stp->sd_flag |= TIMEDOUT;
	stp->sd_flag &= ~RSLEEP;
	wakeup((caddr_t)RD(stp->sd_wrq));
}
