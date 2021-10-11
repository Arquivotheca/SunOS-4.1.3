#ifndef lint
static	char sccsid[] = "@(#)ms.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Mouse streams module.
 */

#include "ms.h"
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/ttold.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <sundev/vuid_event.h>
#include <sundev/msreg.h>
#include <sundev/msio.h>

/*
 * Note: We use spl5 for the mouse because it is functionally the
 * same as spl6 and the tty mechanism is using spl5.  The original
 * code that was doing its own select processing was using spl6.
 */
#define	spl_ms	spl5

#define	ABS(x)		((x) < 0 ? -(x) : (x))
#define	BYTECLIP(x)	(char)((x) > 127 ? 127 : ((x) < -128 ? -128 : (x)))

struct msdata {
	struct ms_softc msd_softc;
	queue_t	*msd_readq;		/* upstream read queue */
	mblk_t	*msd_iocpending;	/* "ioctl" awaiting buffer */
	int	msd_iocid;		/* ID of "ioctl" being waited for */
	int	msd_iocerror;		/* error return from "ioctl" */
	int	msd_rbufcid;		/* ID of pending read-side bufcall */
	int	msd_wbufcid;		/* ID of pending write-side bufcall */
	int	msd_flags;		/* random flags */
	short	msd_xerox;		/* 1 if a Xerox mouse */
	char	msd_oldbutt;		/* button state at last sample */
	short	msd_state;		/* state counter for input routine */
	short	msd_jitter;
};

/*
 * Values for msd_flags field:
 */
#define	MS_OPEN		0x00000001	/* mouse is open for business */
#define	MS_IOCWAIT	0x00000002	/* open waiting for ioctl to finish */

/*
 * Input routine states. See msinput().
 */
#define	MS_WAIT_BUTN	0
#define	MS_WAIT_X	1
#define	MS_WAIT_Y	2
#define	MS_WAIT_X2	3
#define	MS_WAIT_Y2	4

int		ms_overrun_msg;	/* Message when overrun circular buffer */
int		ms_overrun_cnt;	/* Increment when overrun circular buffer */

/*
 * Max pixel delta of jitter controlled. As this number increases the jumpiness
 * of the ms increases, i.e., the coarser the motion for medium speeds.
 */
int		ms_jitter_thresh = 0;

/*
 * ms_jitter_thresh is the maximum number of jitters suppressed. Thus,
 * hz/ms_jitter_thresh is the maximum interval of jitters suppressed. As
 * ms_jitter_thresh increases, a wider range of jitter is suppressed. However,
 * the more inertia the mouse seems to have, i.e., the slower the mouse is to
 * react.
 */

/*
 * Measure how many (ms_speed_count) ms deltas exceed threshold
 * (ms_speedlimit). If ms_speedlaw then throw away deltas over ms_speedlimit.
 * This is to keep really bad mice that jump around from getting too far.
 */
int		ms_speedlimit = 48;
int		ms_speedlaw = 0;
int		ms_speed_count;
int		msjitterrate = 12;

#define	JITTER_TIMEOUT (hz/msjitterrate)

int		msjittertimeout; /* Timeout used when mstimeout in effect */

/*
 * Mouse buffer size in bytes.  Place here as variable so that one could
 * massage it using adb if it turns out to be too small.
 */
int		MS_BUF_BYTES = 4096;

int		MS_DEBUG;

/*
 * Most of these should be "void", but the people who defined the "streams"
 * data structures for S5 didn't understand data types.
 */
static int msopen(/*queue_t *q, int dev, int oflag, int sflag*/);
static int msclose(/*queue_t *q*/);
static int mswput(/*queue_t *q, mblk_t *mp*/);
static int msrput(/*queue_t *q, mblk_t *mp*/);
static int msrserv(/*queue_t *q*/);

static struct module_info msmiinfo = {
	0,
	"ms",
	0,
	INFPSZ,
	2048,
	128
};

static struct qinit msrinit = {
	msrput,
	msrserv,
	msopen,
	msclose,
	NULL,
	&msmiinfo
};

static struct module_info msmoinfo = {
	0,
	"ms",
	0,
	INFPSZ,
	2048,
	128
};

static struct qinit mswinit = {
	mswput,
	NULL,
	msopen,
	msclose,
	NULL,
	&msmoinfo
};

struct streamtab ms_info = {
	&msrinit,
	&mswinit,
	NULL,
	NULL,
	NULL
};

static int	msresched(/*long msdptr*/);
static int	msreioctl(/*long msdptr*/);
static void	msioctl(/*queue_t *q, mblk_t *mp*/);
static int	ms_getparms(/*register Ms_parms *data*/);
static int	ms_setparms(/*register Ms_parms *data*/);
static void	msflush(/*struct msdata *msd*/);
static void	msinput(/*struct msdata *msd, char c*/);
static int	msincr(/*struct msdata *msd*/);

/*
 * Open a mouse.
 */
/*ARGSUSED*/
static int
msopen(q, dev, oflag, sflag)
    queue_t	*q;
    int		dev, oflag, sflag;
{
    register struct mousebuf *b;
    register struct ms_softc *ms;
    register struct msdata *msd;
    mblk_t	 *mp;
    mblk_t	 *datap;
    register struct iocblk *iocb;
    register struct termios *cb;

    if (q->q_ptr != NULL)
	return (0);		/* already attached */

    if (sflag != MODOPEN)
	return (OPENFAIL);

    /*
     * Allocate an msdata structure.
     */
    msd = (struct msdata *)new_kmem_zalloc(
			sizeof (struct msdata), KMEM_SLEEP);

    /*
     * Set up queue pointers, so that the "put" procedure will accept
     * the reply to the "ioctl" message we send down.
     */
    q->q_ptr = (caddr_t)msd;
    WR(q)->q_ptr = (caddr_t)msd;

    /*
     * Setup tty modes.
     */
    if ((mp = allocb(sizeof (struct iocblk), BPRI_HI)) == NULL) {
	if (strwaitbuf(sizeof (struct iocblk), BPRI_HI, 1))
	    return (OPENFAIL);
    }
    if ((datap = allocb(sizeof (struct termios), BPRI_HI)) == NULL) {
	if (strwaitbuf(sizeof (struct termios), BPRI_HI, 1)) {
	    freemsg(mp);
	    return (OPENFAIL);
	}
    }
    iocb = (struct iocblk *)mp->b_wptr;
    iocb->ioc_cmd = TCSETSF;
    iocb->ioc_uid = 0;
    iocb->ioc_gid = 0;
    iocb->ioc_id = getiocseqno();
    iocb->ioc_count = sizeof (struct iocblk);
    iocb->ioc_error = 0;
    iocb->ioc_rval = 0;
    mp->b_wptr += (sizeof *iocb)/(sizeof *datap->b_wptr);
    mp->b_datap->db_type = M_IOCTL;
    cb = (struct termios *)datap->b_wptr;
    cb->c_iflag = 0;
    cb->c_oflag = 0;
    cb->c_cflag = CREAD|CS8|B1200;
    cb->c_lflag = 0;
    cb->c_line = 0;
    bzero((caddr_t)cb->c_cc, NCCS);
    datap->b_wptr += (sizeof *cb)/(sizeof *datap->b_wptr);
    datap->b_datap->db_type = M_DATA;
    mp->b_cont = datap;
    msd->msd_flags |= MS_IOCWAIT;	/* indicate that we're waiting for */
    msd->msd_iocid = iocb->ioc_id;	/* this response */
    putnext(WR(q), mp);

    /*
     * Now wait for it.  Let our read queue put routine wake us up
     * when it arrives.
     */
    while (msd->msd_flags & MS_IOCWAIT) {
	if (sleep((caddr_t)&msd->msd_iocerror, STOPRI|PCATCH)) {
	    u.u_error = EINTR;
	    goto error;
	}
    }
    if (u.u_error = msd->msd_iocerror)
	goto error;

    /*
     * Set up private data.
     */
    msd->msd_state = MS_WAIT_BUTN;
    msd->msd_readq = q;
    msd->msd_iocpending = NULL;
    ms = &msd->msd_softc;

    /*
     * Allocate buffer and initialize data.
     */
    if (ms->ms_buf == 0) {
	ms->ms_bufbytes = MS_BUF_BYTES;
	b = (struct mousebuf *)new_kmem_zalloc(
				(u_int)ms->ms_bufbytes, KMEM_SLEEP);
	b->mb_size = 1 + (ms->ms_bufbytes - sizeof (struct mousebuf))
	    / sizeof (struct mouseinfo);
	ms->ms_buf = b;
	ms->ms_vuidaddr = VKEY_FIRST;
	msjittertimeout = JITTER_TIMEOUT;
	msflush(msd);
    }

    msd->msd_flags = MS_OPEN;

    /*
     * Tell the module below us that it should return input immediately.
     */
    (void) putctl1(WR(q)->q_next, M_CTL, MC_SERVICEIMM);

    return (0);

error:
    /*
     * Free dynamically allocated resources.
     */
    if (ms->ms_buf != NULL)
	kmem_free((caddr_t)ms->ms_buf, (u_int)ms->ms_bufbytes);
    kmem_free((caddr_t)msd, sizeof (*msd));
    return (OPENFAIL);
}

/*
 * Close the mouse
 */
static int
msclose(q)
    queue_t	*q;
{
    register struct msdata *msd = (struct msdata *)q->q_ptr;
    register struct ms_softc *ms;

    /*
     * Tell the module below us that it need not return input immediately.
     */
    (void) putctl1(q->q_next, M_CTL, MC_SERVICEDEF);

    /*
     * Since we're about to destroy our private data, turn off
     * our open flag first, so we don't accept any more input
     * and try to use that data.
     */
    msd->msd_flags = 0;

    if (msd->msd_iocpending != NULL) {
	/*
	 * We were holding an "ioctl" response pending the
	 * availability of an "mblk" to hold data to be passed up;
	 * another "ioctl" came through, which means that "ioctl"
	 * must have timed out or been aborted.
	 */
	freemsg(msd->msd_iocpending);
	msd->msd_iocpending = NULL;
    }
    /*
     * Cancel outstanding bufcall requests.
     */
    if (msd->msd_rbufcid)
	unbufcall(msd->msd_rbufcid);
    if (msd->msd_wbufcid)
	unbufcall(msd->msd_wbufcid);

    ms = &msd->msd_softc;
    /* Free mouse buffer */
    if (ms->ms_buf != NULL)
	kmem_free((caddr_t)ms->ms_buf, (u_int)ms->ms_bufbytes);
    /* Free msdata structure */
    kmem_free((caddr_t)msd, sizeof (*msd));
}

/*
 * Read queue service routine.
 * Turn buffered mouse events into stream messages.
 */
static int
msrserv(q)
    register queue_t *q;
{
    struct msdata *msd = (struct msdata *)q->q_ptr;
    register struct ms_softc *ms;
    register struct mousebuf *b;
    register struct mouseinfo *mi;
    register int    button_number;
    register int    hwbit, pri;
    mblk_t	 *bp;

    ms = &msd->msd_softc;
    b = ms->ms_buf;
    pri = spl_ms();
    while (canput(q->q_next) && ms->ms_oldoff != b->mb_off) {
	mi = &b->mb_info[ms->ms_oldoff];
	switch (ms->ms_readformat) {

	case MS_3BYTE_FORMAT: {
	    register char *cp;

	    if ((bp = allocb(3, BPRI_HI)) != NULL) {
		cp = (char *)bp->b_wptr;

		*cp++ = 0x80 | mi->mi_buttons;
		/* Update read buttons */
		ms->ms_prevbuttons = mi->mi_buttons;

		*cp++ = mi->mi_x;
		*cp++ = -mi->mi_y;
		/* lower pri to avoid mouse droppings */
		(void) splx(pri);
		bp->b_wptr = (u_char *)cp;
		putnext(q, bp);
		pri = spl_ms();
	    } else {
		/*
		 * Arrange to try again later.
		 */
		msd->msd_rbufcid = bufcall(3, BPRI_HI, msresched, (long)msd);
		if (msd->msd_rbufcid != 0)
		    return;
		/* bufcall failed; just pitch this event */
		/* or maybe flush queue? */
	    }
	    ms->ms_oldoff++;	/* next event */
	    if (ms->ms_oldoff >= b->mb_size)
		ms->ms_oldoff = 0;	/* circular buffer wraparound */
		break;
	}

	case MS_VUID_FORMAT: {
	    register Firm_event *fep;

	    bp = NULL;
	    switch (ms->ms_eventstate) {

	    case EVENT_X:
		/*
		 * Send x if changed.
		 */
		if (mi->mi_x != 0) {
		    if ((bp = allocb(sizeof (Firm_event), BPRI_HI)) != NULL) {
			fep = (Firm_event *)bp->b_wptr;
			fep->id = vuid_id_addr(ms->ms_vuidaddr) |
			    vuid_id_offset(LOC_X_DELTA);
			fep->pair_type = FE_PAIR_ABSOLUTE;
			fep->pair = LOC_X_ABSOLUTE;
			fep->value = mi->mi_x;
			fep->time = mi->mi_time;
		    } else {
			/*
			 * Arrange to try again later.
			 */
			msd->msd_rbufcid = bufcall(sizeof (Firm_event), BPRI_HI,
			    msresched, (long)msd);
			if (msd->msd_rbufcid != 0)
			    return;
			/* bufcall failed; just pitch this event */
			/* or maybe flush queue? */
			ms->ms_eventstate = EVENT_BUT3;
		    }
		}
		break;

	    case EVENT_Y:
		/*
		 * Send y if changed.
		 */
		if (mi->mi_y != 0) {
		    if ((bp = allocb(sizeof (Firm_event), BPRI_HI)) != NULL) {
			fep = (Firm_event *)bp->b_wptr;
			fep->id = vuid_id_addr(ms->ms_vuidaddr) |
			    vuid_id_offset(LOC_Y_DELTA);
			fep->pair_type = FE_PAIR_ABSOLUTE;
			fep->pair = LOC_Y_ABSOLUTE;
			fep->value = -mi->mi_y;
			fep->time = mi->mi_time;
		    } else {
			/*
			 * Arrange to try again later.
			 */
			msd->msd_rbufcid = bufcall(sizeof (Firm_event), BPRI_HI,
			    msresched, (long)msd);
			if (msd->msd_rbufcid != 0)
			    return;
			/* bufcall failed; just pitch this event */
			/* or maybe flush queue? */
			ms->ms_eventstate = EVENT_BUT3;
		    }
		}
		break;

	    case EVENT_BUT1:
	    case EVENT_BUT2:
	    case EVENT_BUT3:
		/*
		 * Test the button, and send an event for it if it changed.
		 */
		button_number = ms->ms_eventstate - EVENT_BUT1;
		hwbit = MS_HW_BUT1 >> button_number;
		if ((ms->ms_prevbuttons & hwbit) != (mi->mi_buttons & hwbit)) {
		    if ((bp = allocb(sizeof (Firm_event), BPRI_HI)) != NULL) {
			fep = (Firm_event *)bp->b_wptr;
			fep->id = vuid_id_addr(ms->ms_vuidaddr) |
			    vuid_id_offset(BUT(1) + button_number);
			fep->pair_type = FE_PAIR_NONE;
			fep->pair = 0;
			/* Update read buttons and set value */
			if (mi->mi_buttons & hwbit) {
			    fep->value = 0;
			    ms->ms_prevbuttons |= hwbit;
			} else {
			    fep->value = 1;
			    ms->ms_prevbuttons &= ~hwbit;
			}
			fep->time = mi->mi_time;
		    } else {
			/*
			 * Arrange to try again later.
			 */
			msd->msd_rbufcid = bufcall(sizeof (Firm_event), BPRI_HI,
			    msresched, (long)msd);
			if (msd->msd_rbufcid != 0)
			    return;
			/* bufcall failed; just pitch this event */
			/* or maybe flush queue? */
			ms->ms_eventstate = EVENT_BUT3;
		    }
		}
		break;
	    }
	    if (bp != NULL) {
		/* lower pri to avoid mouse droppings */
		(void) splx(pri);
		bp->b_wptr += sizeof (Firm_event);
		putnext(q, bp);
		pri = spl_ms();
	    }
	    if (ms->ms_eventstate == EVENT_BUT3) {
		ms->ms_eventstate = EVENT_X;
		ms->ms_oldoff++;	/* next event */
		if (ms->ms_oldoff >= b->mb_size)
		    ms->ms_oldoff = 0;	/* circular buffer wraparound */
	    } else
		ms->ms_eventstate++;
	}
	}
    }
}

static int
msresched(msdptr)
    long msdptr;
{
    register queue_t *q;
    register struct msdata *msd = (struct msdata *)msdptr;

    /*
     * The bufcall that led us here is no longer pending.
     */
    msd->msd_rbufcid = 0;

    if ((q = msd->msd_readq) != 0)
	qenable(q);	/* run the service procedure */
}

/*
 * Line discipline output queue put procedure: handles M_IOCTL
 * messages.
 */
static int
mswput(q, mp)
    register queue_t *q;
    register mblk_t *mp;
{

    /*
     * Process M_FLUSH, and some M_IOCTL, messages here; pass
     * everything else down.
     */
    switch (mp->b_datap->db_type) {

    case M_FLUSH:
	if (*mp->b_rptr & FLUSHW)
	    flushq(q, FLUSHDATA);
	if (*mp->b_rptr & FLUSHR)
	    flushq(RD(q), FLUSHDATA);

    default:
	putnext(q, mp);	/* pass it down the line */
	break;

    case M_IOCTL:
	msioctl(q, mp);
	break;
    }
}

static int
msreioctl(msdptr)
    long msdptr;
{
    struct msdata *msd = (struct msdata *)msdptr;
    register queue_t *q;
    register mblk_t *mp;

    /*
     * The bufcall that led us here is no longer pending.
     */
    msd->msd_wbufcid = 0;

    if ((q = msd->msd_readq) == 0)
	return;
    if ((mp = msd->msd_iocpending) != NULL) {
	msd->msd_iocpending = NULL;	/* not pending any more */
	msioctl(WR(q), mp);
    }
}

static void
msioctl(q, mp)
    register queue_t *q;
    register mblk_t *mp;
{
    struct msdata *msd;
    register struct ms_softc *ms;
    register struct iocblk *iocp;
    caddr_t	 data;
    Vuid_addr_probe *addr_probe;
    u_int	   ioctlrespsize;
    int		pri;
    int		err = 0;

    msd = (struct msdata *)q->q_ptr;
    if (msd == 0) {
	err = EINVAL;
	goto out;
    }
    ms = &msd->msd_softc;

    iocp = (struct iocblk *)mp->b_rptr;
    if (mp->b_cont != NULL)
	data = (caddr_t)mp->b_cont->b_rptr;

    if (MS_DEBUG)
	printf("mswput(M_IOCTL, %X, %X)\n", iocp->ioc_cmd, data);

    switch (iocp->ioc_cmd) {
	case VUIDSFORMAT:
	if (*(int *) data == ms->ms_readformat)
	    break;
	ms->ms_readformat = *(int *) data;
	/*
	 * Flush mouse buffer because the messages upstream of us are in
	 * the old format.
	 */
	msflush(msd);
	break;

	case VUIDGFORMAT: {
	register mblk_t *datap;

	if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
	    ioctlrespsize = sizeof (int);
	    goto allocfailure;
	}
	*(int *)datap->b_wptr = ms->ms_readformat;
	datap->b_wptr += sizeof (int);
	mp->b_cont = datap;
	iocp->ioc_count = sizeof (int);
	break;
	}

	case VUIDSADDR:
	addr_probe = (Vuid_addr_probe *) data;
	if (addr_probe->base != VKEY_FIRST) {
	    err = ENODEV;
	    break;
	}
	ms->ms_vuidaddr = addr_probe->data.next;
	break;

	case VUIDGADDR:
	addr_probe = (Vuid_addr_probe *) data;
	if (addr_probe->base != VKEY_FIRST) {
	    err = ENODEV;
	    break;
	}
	addr_probe->data.current = ms->ms_vuidaddr;
	break;

	case MSIOGETBUF: {
	register mblk_t *datap;

	if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
	    ioctlrespsize = sizeof (int);
	    goto allocfailure;
	}
	*(int *)datap->b_wptr = (int) ms->ms_buf;
	datap->b_wptr += sizeof (int);
	mp->b_cont = datap;
	iocp->ioc_count = sizeof (int);
	break;
	}

	case MSIOGETPARMS: {
	register mblk_t *datap;

	if (MS_DEBUG)
	    printf("ms_getparms\n");
	if ((datap = allocb(sizeof (Ms_parms), BPRI_HI)) == NULL) {
	    ioctlrespsize = sizeof (Ms_parms);
	    goto allocfailure;
	}
	err = ms_getparms((Ms_parms *)datap->b_wptr);
	datap->b_wptr += sizeof (Ms_parms);
	mp->b_cont = datap;
	iocp->ioc_count = sizeof (Ms_parms);
	break;
	}

	case MSIOSETPARMS:
	if (MS_DEBUG)
	    printf("ms_setparms\n");
	err = ms_setparms((Ms_parms *)data);
	break;

	default:
	putnext(q, mp);	/* pass it down the line */
	return;
    }

out:
    if (err != 0) {
	iocp->ioc_rval = 0;
	iocp->ioc_error = err;
	mp->b_datap->db_type = M_IOCNAK;
    } else {
	iocp->ioc_rval = 0;
	iocp->ioc_error = 0;	/* brain rot */
	mp->b_datap->db_type = M_IOCACK;
    }
    qreply(q, mp);
    return;

allocfailure:
    /*
     * We needed to allocate something to handle this "ioctl", but
     * couldn't; save this "ioctl" and arrange to get called back when
     * it's more likely that we can get what we need.
     * If there's already one being saved, throw it out, since it
     * must have timed out.
     */
    pri = splstr();
    if (msd->msd_iocpending != NULL)
	freemsg(msd->msd_iocpending);
    msd->msd_iocpending = mp;
    if (msd->msd_wbufcid)
	unbufcall(msd->msd_wbufcid);
    msd->msd_wbufcid = bufcall(ioctlrespsize, BPRI_HI, msreioctl, (long)msd);
    (void) splx(pri);
    return;
}

static int
ms_getparms(data)
    register Ms_parms *data;
{
    data->jitter_thresh = ms_jitter_thresh;
    data->speed_law = ms_speedlaw;
    data->speed_limit = ms_speedlimit;
    return (0);
}

static int
ms_setparms(data)
    register Ms_parms *data;
{
    ms_jitter_thresh = data->jitter_thresh;
    ms_speedlaw = data->speed_law;
    ms_speedlimit = data->speed_limit;
    return (0);
}

static void
msflush(msd)
    register struct msdata *msd;
{
    register struct ms_softc *ms = &msd->msd_softc;
    int	s = spl_ms();
    register queue_t *q;

    ms->ms_oldoff = 0;
    ms->ms_eventstate = EVENT_X;
    ms->ms_buf->mb_off = 0;
    ms->ms_prevbuttons = MS_HW_BUT1 | MS_HW_BUT2 | MS_HW_BUT3;
    msd->msd_oldbutt = ms->ms_prevbuttons;
    if ((q = msd->msd_readq) != NULL && (q = q->q_next) != NULL)
	(void) putctl1(q, M_FLUSH, FLUSHR);
    (void) splx(s);
}

/*
 * Mouse read queue put procedure.
 */
static int
msrput(q, mp)
    register queue_t *q;
    register mblk_t *mp;
{
    register struct msdata *msd = (struct msdata *)q->q_ptr;
    register mblk_t *bp;
    register char *readp;
    struct iocblk *iocp;

    if (msd == 0)
	return;

    switch (mp->b_datap->db_type) {

    case M_FLUSH:
	if (*mp->b_rptr & FLUSHW)
	    flushq(WR(q), FLUSHDATA);
	if (*mp->b_rptr & FLUSHR)
	    flushq(q, FLUSHDATA);

    default:
	putnext(q, mp);
	return;

    case M_IOCACK:
    case M_IOCNAK:
	/*
	 * If we are doing an "ioctl" ourselves, check if this
	 * is the reply to that code.  If so, wake up the
	 * "open" routine, and toss the reply, otherwise just
	 * pass it up.
	 */
	iocp = (struct iocblk *)mp->b_rptr;
	if (!(msd->msd_flags & MS_IOCWAIT) ||
	    iocp->ioc_id != msd->msd_iocid) {
	    /*
	     * This isn't the reply we're looking for.  Move along.
	     */
	    putnext(q, mp);
	} else {
	    msd->msd_flags &= ~MS_IOCWAIT;
	    msd->msd_iocerror = iocp->ioc_error;
	    wakeup((caddr_t)&msd->msd_iocerror);
	    freemsg(mp);
	}
	return;

	case M_DATA:
	break;
    }

    /*
     * A data message, consisting of bytes from the mouse.
     * Hand each byte to our input routine.
     */
    bp = mp;

    do {
	readp = (char *)bp->b_rptr;
	while (readp < (char *)bp->b_wptr)
	    msinput(msd, *readp++);
	bp->b_rptr = (unsigned char *)readp;
    } while ((bp = bp->b_cont) != NULL);	/* next block, if any */

    freemsg(mp);
   }

/*
 * Mouse input routine; process a byte received from a mouse and
 * assemble into a mouseinfo message for the window system.
 * Handles both XEROX mice and Mouse Systems mice.
 *
 * The MSC mice send a five-byte packet organized as
 *	button, dx, dy, dx, dy
 * where dx and dy can be any signed byte value. The mouseinfo message
 * is organized as
	dx, dy, button, timestamp
 * Our strategy, then, is to split the MSC packet into two mouseinfo
 * messages, retaining the button information for the second message.
 *
 * Xerox mice are similar but the packet is only three bytes long:
 * 	button, dx, dy
 * Xerox mice have button byte 0x88; MSC mice have button byte
 * 0x80, both with button values in low 3 bits. We'll use this to
 * distinguish between them.
 *
 * Basic algorithm: throw away bytes until we get a [potential]
 * button byte. Collect button, dx, dy, and send. MSC mouse?
 * Save button, collect new dx, dy, and send. Repeat.
 *
 * Watch out for overflow!
 */

static void
msinput(msd, c)
    register struct msdata *msd;
    char c;
{
    register struct ms_softc *ms;
    register struct mousebuf *b;
    register struct mouseinfo *mi;
    register int    jitter_radius;
    register int    temp;

    ms = &msd->msd_softc;
    b = ms->ms_buf;
    if (b == NULL)
	return;

    mi = &b->mb_info[b->mb_off];

    switch (msd->msd_state) {

    case MS_WAIT_BUTN:
	if ((c & 0xf0) != 0x80) {
	    if (MS_DEBUG)
		printf("Mouse input char %x discarded\n", (int) c & 0xff);
	    return;
	}

	/*
	 * Probably a button byte.  Lower 3 bits are left, middle, right.
	 * We're going to try to use the allegation that Xerox mice set bit 3
	 * (0x08) to decide what to do next.
	 */
	mi->mi_buttons = c & (MS_HW_BUT1 | MS_HW_BUT2 | MS_HW_BUT3);
#ifndef IGNORE_XEROX
	msd->msd_xerox = c & 8;
#endif
	break;

    case MS_WAIT_X:
	/*
	 * Delta X byte.  Add the delta X from this sample to the delta X
	 * we're accumulating in the current event.
	 */
	temp = (int) (mi->mi_x + c);
	mi->mi_x = BYTECLIP(temp);
	uniqtime(&mi->mi_time);	/* record time when sample arrived */
	break;

    case MS_WAIT_Y:
	/*
	 * Delta Y byte.  Add the delta Y from this sample to the delta Y
	 * we're accumulating in the current event. (Subtract, actually,
	 * because the mouse reports increasing Y up the screen.)
	 */
	temp = (int) (mi->mi_y - c);
	mi->mi_y = BYTECLIP(temp);
	break;

    case MS_WAIT_X2:
	/*
	 * Second delta X byte. Only for MSC mice.
	 */
	temp = (int) (mi->mi_x + c);
	mi->mi_x = BYTECLIP(temp);
	uniqtime(&mi->mi_time);
	break;

    case MS_WAIT_Y2:
	/*
	 * Second delta Y byte. Only for MSC mice.
	 */
	temp = (int) (mi->mi_y - c);
	mi->mi_y = BYTECLIP(temp);
	break;

    }

    /*
     * Done yet?
     */
    if ((msd->msd_state == MS_WAIT_Y2) ||
	(msd->msd_xerox && (msd->msd_state == MS_WAIT_Y)))
	msd->msd_state = MS_WAIT_BUTN;		/* BONG. Start again. */
    else {
	msd->msd_state += 1;
	return;
    }

    if (msd->msd_jitter) {
	untimeout(msincr, (caddr_t) msd);
	msd->msd_jitter = 0;
    }

    if (mi->mi_buttons == msd->msd_oldbutt) {
	/*
	 * Buttons did not change; did position?
	 */
	if (mi->mi_x == 0 && mi->mi_y == 0) {
	    return;	/* no, position did not change - boring event */
	}

	/*
	 * Did the mouse move more than the jitter threshhold?
	 */
	jitter_radius = ms_jitter_thresh;
#ifndef IGNORE_XEROX
	if (msd->msd_xerox)
	    /*
	     * Account for double resolution of xerox mouse.
	     */
	    jitter_radius *= 2;
#endif
	if (ABS((int) mi->mi_x) <= jitter_radius &&
	    ABS((int) mi->mi_y) <= jitter_radius) {
	    /*
	     * Mouse moved less than the jitter threshhold. Don't indicate an
	     * event; keep accumulating motions. After "msjittertimeout"
	     * ticks expire, treat the accumulated delta as the real delta.
	     */
	    msd->msd_jitter = 1;
	    timeout(msincr, (caddr_t) msd, msjittertimeout);
	    return;
	}
    }
    msd->msd_oldbutt = mi->mi_buttons;
    msincr(msd);
}

/*
 * Increment the mouse sample pointer.
 * Called either immediately after a sample or after a jitter timeout.
 */
static int
msincr(msd)
    struct msdata  *msd;
{
    register struct ms_softc *ms = &msd->msd_softc;
    register struct mousebuf *b;
    register struct mouseinfo *mi;
    char	    oldbutt;
    register short  xc, yc;
    register int    wake;
    register int    speedlimit = ms_speedlimit;
    register int    xabs, yabs;
    int		s;

    b = ms->ms_buf;
    if (b == NULL)
	return;
    s = spl_ms();
    mi = &b->mb_info[b->mb_off];

    if (ms_speedlaw) {
#ifndef IGNORE_XEROX
	if (msd->msd_xerox)
	    /*
	     * Account for double resolution of Xerox mouse.
	     */
	    speedlimit *= 2;
#endif
	xabs = ABS((int) mi->mi_x);
	yabs = ABS((int) mi->mi_y);
	if (xabs > speedlimit || yabs > speedlimit)
	    ms_speed_count++;
	if (xabs > speedlimit)
	    mi->mi_x = 0;
	if (yabs > speedlimit)
	    mi->mi_y = 0;
    }

    oldbutt = mi->mi_buttons;

#ifndef IGNORE_XEROX
    /*
     * XEROX mice are 200/inch; scale to 100/inch.
     */
    if (msd->msd_xerox) {
	/*
	 * Xc and yc carry over fractional part. You might think that we have
	 * to worry about mi->mi_[xy] being negative here, but remember that
	 * using shift to divide always leaves a positive remainder!
	 */
	xc = mi->mi_x & 1;
	yc = mi->mi_y & 1;
	mi->mi_x >>= 1;
	mi->mi_y >>= 1;
    } else
#endif
	xc = yc = 0;

    /* See if we need to wake up anyone waiting for input */
    wake = b->mb_off == ms->ms_oldoff;

    /* Adjust circular buffer pointer */
    if (++b->mb_off >= b->mb_size) {
	b->mb_off = 0;
	mi = b->mb_info;
    } else {
	mi++;
    }

    /*
     * If over-took read index then flush buffer so that mouse state
     * is consistent.
     */
    if (b->mb_off == ms->ms_oldoff) {
	if (ms_overrun_msg)
	    printf("Mouse buffer flushed when overrun.\n");
	msflush(msd);
	ms_overrun_cnt++;
	mi = b->mb_info;
    }

    /* Remember current buttons and fractional part of x & y */
    mi->mi_buttons = oldbutt;
    mi->mi_x = (char) xc;
    mi->mi_y = (char) yc;
    if (wake)
	qenable(msd->msd_readq);	/* run the service procedure */
    (void) splx(s);
}
