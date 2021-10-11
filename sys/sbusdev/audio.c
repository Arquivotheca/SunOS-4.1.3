#ident "@(#) audio.c 1.1@(#) Copyright (c) 1991-92 Sun Microsystems, Inc."

/*
 * Generic AUDIO driver
 *
 * This file contains the generic routines for handling a STREAMS-based
 * audio device.  The SPARCstation 1 audio chips and the SPARCstation 3
 * DBRI chips are examples of such devices.
 */

#include <sys/errno.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/file.h>
#include <sys/debug.h>

#include <sun/audioio.h>
#include <sbusdev/audiovar.h>
#include "audiodebug.h"

/* debugging printf's */
int Audio_debug = 0;

/* Local private routines */
static void		audio_append_cmd();
static void		audio_delete_cmds();
static void		audio_ioctl();
void			audio_flush_cmdlist();
static aud_return_t	audio_do_setinfo();
void			audio_pause_record();
void			audio_pause_play();
void			audio_resume_record();
void			audio_resume_play();
int			audio_xmit_garbage_collect();
static int		audio_receive_collect();


/* define macros to add and remove aud_cmd structures from the free list */
#define	audio_alloc_cmd(v, d)	{				\
		(d) = (v)->free;				\
		if ((d) != NULL)				\
			(v)->free = (d)->next;			\
		}						\
		/* ATRACE(0, ATR_CMDALLOC, (d)); */

#define	audio_free_cmds(v, f, l) {				\
		/* ATRACE(0, ATR_CMDFREE, (int)(f)); */		\
		(l)->next = (v)->free;				\
		(v)->free = (f);				\
		}


/*
 * Append a command block to a list of chained commands
 */
static void
audio_append_cmd(list, cmdp)
	struct aud_cmdlist	*list;
	aud_cmd_t		*cmdp;
{
	ATRACE(audio_append_cmd, ATR_APPEND, (int)cmdp);

	cmdp->next = NULL;

	if (list->tail != NULL)
		list->tail->next = cmdp;
	else
		list->head = cmdp;
	list->tail = cmdp;

	return;
}


/*
 * audio_delete_cmds - Remove one or more cmds from anywhere on a command
 * list.  Deletes the commands from headp to lastp inclusive.
 */
static void
audio_delete_cmds(list, headp, lastp)
	struct aud_cmdlist	*list;
	aud_cmd_t		*headp;
	aud_cmd_t		*lastp;
{
	aud_cmd_t		*cmdp;

	if (list->head == NULL)
		return;

	if (list->head == headp) {	/* Delete from head of list */
		list->head = lastp->next;
		if (list->head == NULL)
			list->tail = NULL;
	} else {			/* Delete from middle/end of list */
		/* Find element directly preceeding headp of list to delete */
		for (cmdp = list->head; cmdp->next != NULL; cmdp = cmdp->next) {
			if (cmdp->next == headp)
				break;
		}

		if (cmdp->next != headp)	/* match not found */
			panic("match not found when aborting cmdlist\n");

		cmdp->next = lastp->next;
		if (cmdp->next == NULL)
			list->tail = cmdp;
	}
	audio_free_cmds(list, headp, lastp);	/* queue cmds to free list */
} /* audio_delete_cmds */


/*
 * Flush a command list.
 * Audio i/o must be stopped on the list before flushing it.
 */
void
audio_flush_cmdlist(as)
	aud_stream_t		*as;
{
	struct aud_cmdlist	*list;
	aud_cmd_t		*cmdp;
	int			s;

	s = splstr();

	list = &as->cmdlist;
	AUD_FLUSHCMD(as);

	/* Release STREAMS command block */
	for (cmdp = list->head; cmdp != NULL; cmdp = cmdp->next)  {
		if (cmdp->dihandle)
			freemsg((mblk_t *)cmdp->dihandle);

		cmdp->dihandle = NULL;
		cmdp->data = 0;
		cmdp->enddata = 0;
		cmdp->lastfragment = 0;
		cmdp->processed = 0;
		cmdp->skip = 0;
		cmdp->iotype = 0;
	}

	/* remove entire command list */
	audio_delete_cmds(list, list->head, list->tail);

	(void) splx(s);
	return;
} /* audio_flush_cmdlist */


/*
 * The ordinary audio device may only have a single reader and single
 * writer.  However, a special minor device is defined for which multiple
 * opens are permitted.  Reads and writes are prohibited for this
 * 'control' device, but certain ioctls, such as AUDIO_SETINFO, are
 * allowed.
 *
 * Note that this is NOT a generic STREAMS open routine.  It must be
 * called from a device-dependent open routine that sets 'as'
 * appropriately.
 *
 */
int
audio_open(as, q, flag, sflag)
	aud_stream_t		*as;	/* must be the correct "as" */
	queue_t			*q;
	int			flag;
	int			sflag;
{
	int			wantwrite;
	int			wantread;
	int			cansleep;

	ATRACEINIT();		/* init tracing, if not already done */
	ATRACE(audio_open, 'flag', flag);

	ASSERT(as != NULL);

	if ((ISDATASTREAM(as) && ((flag & (FREAD|FWRITE)) == FREAD)
		&& (as != as->record_as))) {
		call_debug("audio_open: failure: flag==FREAD && as!=as->record_as");
		return(OPENFAIL);
	}

	/*
	 * If this is a data device: allow only one reader and one writer.
	 * If the requested access is busy, return EBUSY or hang until
	 * close().
	 *
	 * If this device does not carry data, then it is not an
	 * exclusive open device. Open is very simple for such devices.
	 */
	if (!ISDATASTREAM(as)) {
		/* If this is the first open, init the Streams queues */
		if (as->info.open == FALSE) {
			as->info.open = TRUE;
			if (as->openflag) {
				/* XXX - close should clear this */
				as->openflag = 0;
			}
			as->readq = q;
			as->writeq = WR(q);
			WR(q)->q_ptr = (caddr_t)(as);
			q->q_ptr = (caddr_t)(as);
		}

		as->openflag |= (flag & (FREAD|FWRITE));

		ATRACE(audio_open, ATR_OPENED, (int)as->info.minordev);
		return (0);
	}

	wantwrite = ((flag & FWRITE) != 0);	/* write access requested */
	wantread = ((flag & FREAD) != 0);	/* read access requested */

	if (!wantwrite && !wantread) {
		u.u_error = EINVAL;	/* must read and/or write data */
		return (OPENFAIL);
	}

	/*
	 * XXX - Because of limitation in vnode/streams interaction,
	 * blocking open only works on the clone device.
	 */
	/* if O_NDELAY, return immediately */
	cansleep = (sflag == CLONEOPEN) && ((flag & (FNBIO | FNDELAY)) == 0);

	/* While desired access is busy... */
	while ((wantwrite && as->play_as->info.open) ||
		(wantread && as->record_as->info.open)) {
		int	notify;

		ATRACE(audio_open, ATR_BUSY, flag);

		if (!cansleep) {
			ATRACE(audio_open, '!slp', as);
			u.u_error = EBUSY;
			return (OPENFAIL);
		}

		/*
		 * Otherwise, hang until device is closed or a signal
		 * interrupts the sleep.
		 *
		 * If this is the first process to request access, signal
		 * the control device so that it can detect the status
		 * change.  Unfortunately, if the current process has
		 * opened and enabled signals on the control device, this
		 * signal will break the sleep we're about to do.
		 * However, the process should already be prepared for
		 * this by retrying the open() when EINTR is returned.
		 */
		if (wantwrite && !as->play_as->info.waiting) {
			as->play_as->info.waiting = TRUE;
			notify = TRUE;
		} else {
			notify = FALSE;
		}
		if (wantread && !as->record_as->info.waiting) {
			as->record_as->info.waiting = TRUE;
			notify = TRUE;
		}
		if (notify)
			audio_sendsig(as->control_as);

		if (sleep((caddr_t)(as->control_as), SLEEPPRI)) {
			u.u_error = EINTR;
			return (OPENFAIL);
		}
		ATRACE(audio_open, ATR_NOTBUSY, as);
	}

	/*
	 * Set up the streams pointers for the requested access modes.
	 * If opened read/write, set the streams q_ptr to &v->play and
	 * mark the stream accordingly.
	 */
	if (wantread) {
		as->record_as->info.open = TRUE;
		as->record_as->openflag = flag & (FREAD|FWRITE);
		as->record_as->readq = q;
		as->record_as->writeq = WR(q);
		WR(q)->q_ptr = (caddr_t)(as->record_as);
		q->q_ptr = (caddr_t)(as->record_as);

		ATRACE(audio_open, ATR_OPENREC, as->record_as);

		/*
		 * XXX - Device-dependent code needs to call
		 * process_record after it initializes everything it
		 * needs to.
		 */
	}

	if (wantwrite) {
		as->play_as->info.open = TRUE;
		as->play_as->openflag = flag & (FREAD|FWRITE);
		as->play_as->readq = q;
		as->play_as->writeq = WR(q);
		WR(q)->q_ptr = (caddr_t)(as->play_as);
		q->q_ptr = (caddr_t)(as->play_as);

		ATRACE(audio_open, ATR_OPENPLAY, as->play_as);
	}

	/* Signal a state change */
	audio_sendsig(as->control_as);

	if (wantwrite) {
		ASSERT(as->play_as->info.minordev != 0);
	} else {
		ASSERT(as->record_as->info.minordev != 0);
	}
	return (0);
}


/*
 * Close the device.  Be careful to only dismantle the open parts.
 *
 * If the device is open for both play and record, q_ptr will have been
 * set to as->play_as and as->openflag to (FREAD|FWRITE).
 */
void
audio_close(q, flag)
	queue_t			*q;
	int			flag;
{
	int			s;
	aud_stream_t		*as;
	aud_stream_t		*as_output;
	aud_stream_t		*as_input;
	int			isplay;
	int			isrecord;

#ifdef lint
	flag = flag;
#endif

	as = (aud_stream_t *)q->q_ptr;
	ASSERT(as != NULL);

	as_output = as->play_as;
	ASSERT(as_output != NULL);

	as_input = as->record_as;
	ASSERT(as_input != NULL);

	ATRACE(audio_close, ATR_BEGINCLOSE, as->info.minordev);

	isplay = ISPLAYSTREAM(as);
	isrecord = ISRECORDSTREAM(as);

	if (isplay) {
		ATRACE(audio_close, 'cply', as);
	}
	if (isrecord) {
		ATRACE(audio_close, 'crec', as);
	}

	/* Stop recording */
	if (isrecord) {
		ATRACE(audio_close, ATR_CLOSEREC, as->info.minordev);

		AUD_STOP(as_input);
		audio_flush_cmdlist(as_input);

		/* Call the device-dependent close routine */
		AUD_CLOSE(as_input);

		as_input->info.open = FALSE;
		as_input->info.pause = FALSE;
		as_input->info.error = FALSE;
		as_input->info.eof = 0;
		as_input->info.waiting = FALSE;

		/* Clear the record-side stream info */
		as_input->readq = NULL;
		as_input->writeq = NULL;

		as_input->openflag &= ~FREAD;
		if (isplay)
			as_output->openflag &= ~FREAD;
	}

	/* Stop playing */
	if (isplay) {
		ATRACE(audio_close, ATR_CLOSEPLAY, as->info.minordev);

		/*
		 * If there is any data waiting to be written, then sleep
		 * until it is all gone.  This makes most applications
		 * behave as expected.  Note that a signal (eg, ^C) will
		 * end the sleep.
		 *
		 * Since a process may use the control device to pause
		 * output, it is not necessary to worry about the pause
		 * flag here.
		 */
		if (!(u.u_procp->p_flag & SWEXIT)) {
			ATRACE(audio_close, ATR_EXITCLOSE, as->info.minordev);
			s = splstr(); /* XXX - close always called at splstr */
			as_output->draining = TRUE;
			audio_process_play(as_output);
			if (as_output->draining) {
				ATRACE(audio_close, ATR_CLOSEDRAIN,
					as->info.minordev);
				/*
				 * Sleep until output complete or signal.
				 *
				 * XXX - checking p->cursig here is
				 * promiscuous but useful since the
				 * signal may have been caught by the
				 * stream head's dumb drain code before
				 * we got a shot at it.
				 */
				if (u.u_procp->p_cursig == 0)
					(void) sleep((caddr_t)
						    &as_output->draining,
						    SLEEPPRI);
			}
			(void) splx(s);
			as_output->draining = FALSE;
		}
		AUD_STOP(as_output);
		audio_flush_cmdlist(as_output);

		/* Call the device-dependent close routine */
		AUD_CLOSE(as_output);

		as_output->info.open = FALSE;
		as_output->info.pause = FALSE;
		as_output->info.error = FALSE;
		as_output->info.eof = 0;
		as_output->info.waiting = FALSE;

		/* Clear the record-side stream info */
		as_output->readq = NULL;
		as_output->writeq = NULL;
		as_output->openflag &= ~FWRITE;
		if (isrecord)
			as_input->openflag &= ~FWRITE;
	}

	/* If closing play or record, signal the control stream */
	if (ISDATASTREAM(as)) {
		audio_sendsig(as->control_as);
		wakeup((caddr_t)as->control_as); /* wakeup audio_open() */
	}

	/*
	 * If this stream is only a control stream, then cleanup is needed.
	 * Otherwise, the following is redundant.
	 */
	as->info.open = FALSE;
	as->readq = NULL;
	as->writeq = NULL;
	as->openflag = 0;

	ATRACE(audio_close, ATR_CLOSED, as);

	return;
}


/*
 * In addition to the streamio(4) and filio(4) ioctls, the driver accepts:
 * 	AUDIO_DRAIN	- hang until output is drained
 * 	AUDIO_GETINFO	- get state information
 * 	AUDIO_SETINFO	- set state information
 *
 * Other ioctls may be processed by the device-specific ioctl handler.
 *
 * If the IOCTL is done on the control stream and channel is not
 * specified assume normal play/record stream. If channel is "all" then
 * return status of all streams (need to use streams data xfer
 * messgages?). If channel is specified, then return/affect only that
 * stream.
 *
 * XXX - fix up this definition.
 */
static void
audio_ioctl(as, q, mp)
	aud_stream_t		*as;
	queue_t			*q;
	mblk_t			*mp;
{
	aud_state_t		*v;
	struct iocblk		*iocp;
	audio_info_t		*ip;
	aud_return_t		change;
	aud_stream_t		*as_input;
	aud_stream_t		*as_output;

	as = (aud_stream_t *)q->q_ptr;
	v = as->v;

	iocp = (struct iocblk *)mp->b_rptr;
	iocp->ioc_count = 0;			/* assume nothing returned */
	iocp->ioc_error = 0;			/* initialize to no error */
	change = AUDRETURN_NOCHANGE;		/* detect dev state change */

	/* If there is output for this ioctl, allocate the space now */
	if (((iocp->ioc_cmd & _IOC_OUT) != 0) && (mp->b_cont == NULL)) {
		mblk_t		*datap;
		int		size;

		size = (iocp->ioc_cmd >> 16) & _IOCPARM_MASK;
		if ((datap = (mblk_t *)allocb(size, BPRI_HI)) == NULL) {
			iocp->ioc_error = ENOSR;	/* no space */
			goto othererrno;
		} else {
			mp->b_cont = datap;
		}
	}

	ATRACE(audio_ioctl, ATR_IOCTL, iocp->ioc_cmd);

	switch (iocp->ioc_cmd) {
	case AUDIO_SETINFO: {			/* Set state information */
		int		s;
		unsigned int	play_eof;
		unsigned char	play_err;
		unsigned char	rec_err;

		ip = (audio_info_t *)mp->b_cont->b_rptr;

		as_output = as->play_as;
		as_input = as->record_as;

		/*
		 * Error indicators and play eof count are updated
		 * atomically so that processes may reset them safely.
		 * Sample counts are also updated like this, but are
		 * handled in the device-specific setinfo routine.
		 */
		s = splstr();
		play_eof = as_output->info.eof;	/* Save old values */
		play_err = as_output->info.error;
		rec_err = as_input->info.error;
		change = audio_do_setinfo(as, mp, iocp);
		(void) splx(s);

		if (iocp->ioc_error != 0)
			goto einval;

		/* Copy current state */
		ip->play = as_output->info;
		ip->record = as_input->info;
		ip->monitor_gain = v->monitor_gain;
		ip->output_muted = v->output_muted;

		ip->play.eof = play_eof;	/* Restore old values */
		ip->play.error = play_err;
		ip->record.error = rec_err;

		iocp->ioc_count = sizeof (audio_info_t);
		break;
	    }

	case AUDIO_GETINFO:		/* Get state information */
		ip = ((audio_info_t *)mp->b_cont->b_rptr);
		mp->b_cont->b_wptr += sizeof (audio_info_t);

		as_output = as->play_as;
		as_input = as->record_as;
		/* Update values that are not stored in state structure */
		AUD_GETINFO(as_output);

		/* Copy current state */
		ip->play = as_output->info;
		ip->record = as_input->info;
		ip->monitor_gain = v->monitor_gain;
		ip->output_muted = v->output_muted;

		iocp->ioc_count = sizeof (audio_info_t);
		break;

	case AUDIO_DRAIN:			/* Drain output */
		/*
		 * AUDIO_DRAIN must be queued to the service procedure,
		 * since there is no user context in which to sleep.  If
		 * the request is not for a play device, return an error.
		 */
		if (!ISPLAYSTREAM(as))
			goto einval;
		putq(q, mp);
		return;				/* don't acknowledge now */

	/* Other ioctls may be handled by the device-specific module */
	default:
		change = AUD_IOCTL(as, mp, iocp);
		if (iocp->ioc_error == 0)
			break;
einval:					/* NACK gives EINVAL by default */
othererrno:
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_rval = -1;
		goto reply;
	} /* switch */

	/*
	 * At this point, there has been no error, but we may or may
	 * not be ready to reply depending on the value of change.
	 * If change indicates no reply yet (DELAY), the device
	 * specific code is free to modify any of the fields we
	 * have just filled in here. It becomes the resonsibility
	 * of the device specific code therefore, to do a sendsig
	 */
	mp->b_datap->db_type = M_IOCACK;	/* everything was OK */
	if (change == AUDRETURN_DELAYED)
		return;
reply:
	qreply(q, mp);			/* Send M_IOCACK (or NAK) upstream */

	/*
	 * If a parameter change has occurred send SIGPOLL upstream.  The
	 * user process must have executed "ioctl(fd, I_SETSIG, S_MSG)"
	 * and "signal(SIGPOLL, subroutine)" to receive the signal.
	 */
	if (change == AUDRETURN_CHANGE) {
		/* Signal the the control device stream if it is open */
		audio_sendsig(as->control_as);
	}
}


/*
 * Set all modified fields in the AUDIO_SETINFO structure.  Return TRUE
 * if no error, with 'v' updated to reflect new values.  Otherwise,
 * returns FALSE.
 */
static aud_return_t
audio_do_setinfo(as, mp, iocp)
	aud_stream_t	*as;
	mblk_t		*mp;
	struct iocblk	*iocp;
{
	aud_stream_t	*as_output;
	aud_stream_t	*as_input;
	audio_info_t	*ip;
	aud_return_t	ret;

	as_output = as->play_as;
	as_input = as->record_as;
	ip = (audio_info_t *)mp->b_cont->b_rptr;

	/*
	 * Make sure user structure is reasonable.
	 * Unsigned fields don't need bounds check for < 0
	 */
	if ((Modify(ip->play.gain) && (ip->play.gain > AUDIO_MAX_GAIN)) ||
	    (Modify(ip->record.gain) && (ip->record.gain > AUDIO_MAX_GAIN)) ||
	    (Modify(ip->monitor_gain) && (ip->monitor_gain > AUDIO_MAX_GAIN))) {
		iocp->ioc_error = EINVAL;
		return (AUDRETURN_NOCHANGE);	/* if error, return ignored */
	}
	if ((Modifyc(ip->play.balance) &&
	    (ip->play.balance > AUDIO_RIGHT_BALANCE)) ||
	    (Modifyc(ip->record.balance) &&
	    (ip->record.balance > AUDIO_RIGHT_BALANCE))) {
		iocp->ioc_error = EINVAL;
		return (AUDRETURN_NOCHANGE);
	}

	/* Validate and set device-specific values */
	ret = AUD_SETINFO(as, mp, iocp);
	if (iocp->ioc_error != 0)
		return (ret);

	/*
	 * The following parameters are zeroed on close() of the i/o
	 * device.  Attempts to change them are silently ignored if it is
	 * closed.  Applications should check the info struct returned by
	 * AUDIO_SETINFO to determine whether they succeeded.
	 */
	if (as_output->info.open) {
		if (Modifyc(ip->play.pause)) {
			if (ip->play.pause) {
				audio_pause_play(as_output);
			} else {
				audio_resume_play(as_output);
			}
		}

		if (Modify(ip->play.eof))
			as_output->info.eof = ip->play.eof;

		if (Modifyc(ip->play.error))
			as_output->info.error = (ip->play.error != 0);

		/* The waiting flags may only be set.  close() clears them. */
		if (Modifyc(ip->play.waiting) && ip->play.waiting)
			as_output->info.waiting = TRUE;

		/*
		 * Get active flag again, since pause/resume may have
		 * changed them.  If we called the getinfo routine here,
		 * then the sample count would get overwritten as well.
		 */
		as_output->info.active = AUD_GETFLAG(as_output, AUD_ACTIVE);
	}

	if (as_input->info.open) {
		if (Modifyc(ip->record.pause)) {
			if (ip->record.pause) {
				audio_pause_record(as_input);
			} else {
				audio_resume_record(as_input);
			}
		}
		if (Modifyc(ip->record.error))
			as_input->info.error = (ip->record.error != 0);
		if (Modifyc(ip->record.waiting) && ip->record.waiting)
			as_input->info.waiting = TRUE;

		/* Get active flag again */
		as_input->info.active = AUD_GETFLAG(as_input, AUD_ACTIVE);
	}
	return (ret);
}


/*
 * audio_wput - Stream write queue put procedure.  All messages from
 * above arrive first in this routine.  All control device messages
 * should be handled or dismissed here.
 */
void
audio_wput(q, mp)
	queue_t			*q;
	mblk_t			*mp;
{
	aud_stream_t		*as;

	ASSERT(q != NULL);
	ASSERT(mp != NULL);

	as = (aud_stream_t *)q->q_ptr;

	if (as == NULL) {
		call_debug("audio_wput called with q->q_ptr == NULL");
		freemsg(mp);
		return;
	}

	ATRACE(audio_wput, 'wpas', as);

	switch (mp->b_datap->db_type) {
	case M_CTL:			/* device specific streams messages */
		/*
		 * XXX - The intent is for M_CTL messages to be available
		 * for use by the device specific portion of a driver.
		 * However, no one has tried them yet, so throw them away
		 * for now.
		 */
		freemsg(mp);
		break;

	case M_DATA:					/* regular data */
		/* Only queue data on play stream */
		if (ISPLAYSTREAM(as)) {
			/*
			 * If audio_process_play() has previously
			 * executed, as it would have during open(), then
			 * it may have found an empty queue (getq()).  If
			 * the queue was previously found empty, then
			 * getq() will have set QWANTR in the queue_t and
			 * this call to putq() will schedule the write
			 * service procedure, audio_wsrv(). Therefore,
			 * there is no need for this routine to directly
			 * call audio_process_play().
			 *
			 * XXX - Correct?
			 */
			putq(q, mp);
			qenable(q);	/* XXX should not need this? */
			ATRACE(audio_wput, ATR_WQ, mp);
		} else {
			freemsg(mp);	/* No data on ctl or record streams */
		}
		break;

	case M_IOCTL:					/* ioctl */
		/*
		 * Most ioctls take effect immediately.  audio_ioctl()
		 * queues AUDIO_DRAIN to the service procedure.
		 */
		audio_ioctl(as, q, mp);
		break;

	case M_FLUSH:				/* flush queues */
		/*
		 * Any stream can flush its queues.  We must be careful
		 * to flush the device command list only when flushing
		 * the relevant queue.
		 */
		ATRACE(audio_wput, ATR_FLUSH, *mp->b_rptr);
		if (*mp->b_rptr & FLUSHW) {
			*mp->b_rptr &= ~FLUSHW;
			flushq(q, FLUSHDATA);
			if (ISPLAYSTREAM(as)) {
				AUD_STOP(as->play_as);	/* XXX ? */
				audio_flush_cmdlist(as->play_as);
				qenable(q);	/* schedule audio_wsrv() */
			}
		}

		if (*mp->b_rptr & FLUSHR) {
			/*
			 * Don't bother flushing the record buffers if
			 * this is not the record device or recording is
			 * already paused (buffers are flushed when
			 * pausing record).
			 */
			if (ISRECORDSTREAM(as) && !as->info.pause) {
				AUD_STOP(as->record_as);
				audio_flush_cmdlist(as->record_as);
			}
			flushq(RD(q), FLUSHDATA);
			qreply(q, mp);
			qenable(RD(q));		/* schedule audio_rsrv() */
		} else {
			freemsg(mp);
		}
		break;

	default:
		freemsg(mp);
		break;
	}

	return;
}


/*
 * Write service procedure can find the following on its queue:
 *	data messages queued for writing
 *	AUDIO_DRAIN ioctl messages
 * Only messages for the audio i/o stream should be found on the queue.
 */
void
audio_wsrv(q)
	queue_t		*q;
{
	aud_stream_t		*as;

	ATRACE(audio_wsrv, ATR_NONE, 0);

	ASSERT(q != NULL);

	as = (aud_stream_t *)q->q_ptr;

	ASSERT(as != NULL);

	audio_process_play(as->play_as);

	return;
}


/*
 * audio_xmit_garbage_collect - Reclaim used transmit buffers.
 *
 * Always called at splstr().
 *
 * Returns TRUE if application needs to be signaled.
 */
int
audio_xmit_garbage_collect(as)
	aud_stream_t	*as;
{
	mblk_t		*mp;
	aud_cmd_t	*cmdp;
	aud_cmd_t	*headp;
	aud_cmd_t	*lastp;
	int		notify;

	notify = FALSE;

	/*
	 * Insure that beyond first processed cmd lies a valid done
	 * command.
	 *
	 * XXX - Remember error case where entire list is NULLED out
	 * cmdptr == NULL and so is cmdlast. (pas)
	 */
	for (cmdp = as->cmdlist.head; cmdp != NULL; cmdp = as->cmdlist.head) {

		/* Don't look at cmd's still owned by the device */
		if (!cmdp->done)
			break;

		/*
		 * Headp points to the first cmd on the list that can
		 * be deleted.
		 *
		 * It may be that the head of the list has been previously
		 * processed and commands completed since then allow for the
		 * head to be removed (ouch!).
		 *
		 * Everything from headp to lastp will be removed from the
		 * list.
		 */
		headp = cmdp;

		if (cmdp->processed) {
			/*
			 * If the command has been processed here already,
			 * it still cannot be reclaimed if it is the last
			 * done command in the transmit chain.
			 */
			if ((cmdp->next == NULL) || !cmdp->next->done)
				break;

			cmdp = cmdp->next;
			ASSERT(cmdp != NULL);

			/*
			 * Headp is left pointing at the "processed"
			 * command while cmdp has advanced to the next command.
			 */
		}

		ASSERT(cmdp->processed == 0);
		ASSERT(cmdp->lastfragment != NULL);

		lastp = cmdp->lastfragment;

		/*
		 * Check if the last command of the packet is done and do
		 * nothing if it is still uncompleted.
		 */
		if (!lastp->done)
			break;

		mp = (mblk_t *)lastp->dihandle;
		ATRACE(audio_xmit_garbage_collect, 'mp  ', mp);

		switch (cmdp->iotype) {
		case M_IOCTL:
			ATRACE(audio_xmit_garbage_collect, 'ictl', cmdp);

			/* ACK the AUDIO_DRAIN ioctl */
			mp->b_datap->db_type = M_IOCACK;
			qreply(as->writeq, mp);
			lastp->dihandle = 0;

			/* ignore error after drain */
			(void) AUD_GETFLAG(as, AUD_ERRORRESET);

			/*
			 * Do not delete device's "continuation" command.
			 */
			headp = cmdp;

			/* Delete everything from headp to lastp */
			break;

		case (unsigned char)(0xff): /* XXX - Pseudo IO, Audio Marker */
			ATRACE(audio_xmit_garbage_collect, 'psio', cmdp);

			if (mp) {
				ATRACE(audio_xmit_garbage_collect, 'fmsg', mp);
				freemsg(mp);
				mp = NULL;
				lastp->dihandle = NULL;
			}

			headp = cmdp;	/* Device's cmd will remain */

			if (as->mode == AUDMODE_AUDIO) {
				ATRACE(audio_xmit_garbage_collect,
					ATR_EOF, mp);

				as->info.eof++;
				notify = TRUE;

				/* ignore error after eof */
				(void) AUD_GETFLAG(as, AUD_ERRORRESET);
			}
			break;

		case M_DATA:
			ATRACE(audio_xmit_garbage_collect, 'data', cmdp);

			/*
			 * The current aud_cmd has been completely transmitted
			 * or otherwise processed. Therefore, it is ok to free
			 * the mblk.
			 */
			if (mp != NULL) {
				ATRACE(audio_xmit_garbage_collect, 'fmsg', mp);
				freemsg(mp);
				mp = NULL;
				lastp->dihandle = NULL;
			}

			if (cmdp->skip) {
				ATRACE(audio_xmit_garbage_collect, 'skip',
					cmdp);
				/*
				 * XXX - We will probably have to
				 * differentiate between "skip" which is
				 * never seen by the device, and some new
				 * flag, "error", which is on the device
				 * IO list.
				 */
				/*
				 * There was a transmission error, and
				 * the packet was marked as "skip" as
				 * part of discarding it. Transmission
				 * errors include trying to transmit on
				 * an inactive channel.
				 *
				 * XXX - check for other types of errors,
				 * does this code still work?
				 */
				headp = cmdp;	/* Device's cmd will remain */

				/* Delete this entire aud_cmd */
				break;
			}

			/*
			 * aud_cmd contained real data.
			 */

			/*
			 * If the packet was owned by the device, then it
			 * may be important for the device specific code
			 * to retain partial "ownership" of the last
			 * command so that it can pick up the forward
			 * pointer if it is told simply to "continue IO".
			 *
			 * If the packet following the current packet is
			 * also marked as "done", then the current packet
			 * can be completely garbage collected,
			 * otherwise, the last fragment of the current
			 * packet must remain on the list.
			 */

			if (lastp->next != NULL &&
			    lastp->next->done &&
			    !lastp->next->skip) {
				/*
				 * This packet, and the possible
				 * preceeding "processed" command, can be
				 * completely gc'ed, which is what headp
				 * and lastp currently indicate.
				 *
				 * XXX - Packets marked skip MAY be on
				 * the device IO list!
				 */
				;
				ATRACE(audio_xmit_garbage_collect, 'all ', mp);
			} else if (cmdp == lastp) {
				/*
				 * This packet consists of one fragment
				 * and there is no completed packet after
				 * it.  It must remain on the chain.
				 */
				cmdp->processed = TRUE;

				/*
				 * If there was a previously "processed"
				 * packet at the head of the list, it can
				 * now be removed.
				 */
				if (headp != cmdp) {
					lastp = headp->lastfragment;
				} else {
					ATRACE(audio_xmit_garbage_collect,
						'nada', cmdp);
					headp = NULL;	/* XXX - not needed */
					lastp = NULL;	/* nothing to delete */
				}
			} else {
				aud_cmd_t	*p;

				/*
				 * This is a multi-fragment packet where
				 * all but the last fragment can be
				 * collected.
				 *
				 * Set lastp to the penultimate fragment.
				 */
				for (p = cmdp;
				    p != p->lastfragment;
				    p = p->next) {
					lastp = p;
				}

				ASSERT(lastp != NULL);
				cmdp->lastfragment->processed = TRUE;
				ATRACE(audio_xmit_garbage_collect,
					'pcsd', cmdp->lastfragment);
			}

			break;

		default:
			ATRACE(audio_xmit_garbage_collect, 'unkn', cmdp);
			if (mp) {
				freemsg(mp);
				cmdp->dihandle = 0;
			}
			break;
		} /* switch */

		/* Delete cmd struct from play list and add to free list */
		if (lastp != NULL) {
			struct aud_cmd	*p;

			ASSERT(headp != NULL);

			/*
			 * Be tidy...
			 */
			for (p = headp; p; p = p->next) {
				ASSERT(p->dihandle == 0);
				p->lastfragment = 0;
				p->iotype = 0;

				if (p == lastp)
					break;
			}

			audio_delete_cmds(&as->cmdlist, headp, lastp);
		}
	} /* for loop */

	return (notify);
}


/*
 * audio_process_play - Deliver new play buffers to the interrupt routine
 * and clean up used buffers.
 *
 * XXX - Returns TRUE if output buffers are in use.  Returns FALSE if
 * output drained.
 */
void
audio_process_play(as)
	aud_stream_t		*as;
{
	mblk_t		*mp;
	int		s;
	aud_cmd_t	*cmdp;
	aud_cmd_t	*head_cmdp;
	int		notify;
	int		iotype;

	as = as->play_as;			/* XXX */
	/* If no write access, don't even bother trying */
	if (!ISPLAYSTREAM(as)) {
		aprintf("audio_process_play: as(%x) not output stream\n",
			(unsigned int)as);
		return;
	}

	s = splstr();

	/*
	 * Free recently emptied play buffers.
	 */
	notify = audio_xmit_garbage_collect(as);

	/*
	 * Dequeue messages as long as there are command blocks available.
	 */
	mp = NULL;
	if (as->cmdlist.free == NULL) {
		ATRACEI(audio_process_play, 'full', as);
	}
	while ((as->cmdlist.free != NULL) && ((mp = getq(as->writeq)) != NULL)){
		mblk_t	*head_mp;

		head_mp = mp;
		head_cmdp = 0;
		iotype = mp->b_datap->db_type;

		/*
		 * Attach each element of a mblk chain to a command structure.
		 */
		do {
			/*
			 * Allocate and initialize a command block
			 */

			/*
			 * It is assumed that an mblk_t and all of its
			 * continuation blocks are of the same type.
			 * The processing of M_DATA messages depends on
			 * this assumption.
			 */
			ASSERT(iotype == mp->b_datap->db_type);

			/*
			 * Do not allocate command blocks for null fragments
			 * in M_DATA messages.
			 */
			if ((iotype == M_DATA) && (mp->b_rptr == mp->b_wptr)) {
				ATRACE(audio_process_play, 'Zlen', mp);
				mp = mp->b_cont;
				continue;
			}

			/*
			 * cmdp gets the next free aud_cmd_t from the free list
			 */
			audio_alloc_cmd(&as->cmdlist, cmdp);

			if (head_cmdp == NULL) {
				head_cmdp = cmdp;
			}

			/*
			 * Initialize aud_cmd defaults
			 */
			cmdp->data		= 0;
			cmdp->enddata		= 0;
			cmdp->next		= 0;
			cmdp->lastfragment	= NULL;
			cmdp->iotype		= iotype;
			cmdp->skip		= FALSE;
			cmdp->done		= FALSE;
			cmdp->processed		= FALSE;
			cmdp->dihandle		= 0;

			/*
			 * AUDIO_DRAIN M_IOCTL, 0 length M_DATA messages (EOF),
			 * and M_CTL messages go through the command path for
			 * synchronization but do not get played.
			 */
			if (iotype != M_DATA) {
				cmdp->skip = TRUE;
				cmdp->dihandle = (void *)mp;
			} else {
				/*
				 * Non-null M_DATA fragment.
				 *
				 * Empty M_DATA fragments have already
				 * been filtered out.
				 */
				cmdp->skip = FALSE;
				cmdp->data = mp->b_rptr;
				cmdp->enddata = mp->b_wptr;
			}

			/*
			 * NB: although the "driver" list is being appended
			 * here, the "device" list is not being affected.
			 * Even if the device is currently running, it is not
			 * allowed to "notice" these new aud_cmd's until
			 * the AUD_QUEUE() primitive is executed.
			 */
			audio_append_cmd(&as->cmdlist, cmdp);

			/*
			 * Since the device routine doesn't
			 * really process non-M_DATA messages,
			 * it is not important to allocate a
			 * separate aud_cmd for each one.
			 */
			if (iotype != M_DATA) {
				/* Exit loop successfully */
				mp = NULL;
			} else {
				/* Advance to next mblk on chain */
				mp = mp->b_cont;
			}

			/*
			 * Stop when there are no more mblk fragments
			 * or when there are no free aud_cmd_t's left.
			 */

		} while ((mp != NULL) && (as->cmdlist.free != NULL));

		/*
		 * If non-zero, head_cmdp points to the start of the first
		 * aud_cmd representing the first M_DATA fragment that has
		 * some data in it, or, if not an M_DATA message, the first
		 * fragment of the mblk.
		 *
		 * If head_cmdp is zero, it is because the message was a zero
		 * length M_DATA message. If we are here, there was at least
		 * one aud_cmd structure on the free list.
		 *
		 * If non-zero, cmdp points to the last aud_cmd used to
		 * represent the mblk.
		 */

		if (head_cmdp == NULL) {
			ATRACE(audio_process_play, ATR_EOF, cmdp);
			/*
			 * Zero length M_DATA is used as an "Audio Marker".
			 * It is queued the same as a non-data message.
			 */
			audio_alloc_cmd(&as->cmdlist, cmdp);

			/*
			 * The encompassing while loop condition ensures that
			 * there is at least one aud_cmd structure available.
			 */
			ASSERT(cmdp != NULL);

			cmdp->iotype	= (unsigned char)(0xff); /* XXX */;
			cmdp->data	= 0;
			cmdp->enddata	= 0;
			cmdp->next	= 0;
			cmdp->lastfragment = NULL;	/* set later */
			cmdp->skip	= TRUE;
			cmdp->done	= FALSE;
			cmdp->processed	= FALSE;
			cmdp->dihandle	= NULL;	/* later set to head_mp */

			head_cmdp = cmdp;
			audio_append_cmd(&as->cmdlist, cmdp);

			/*
			 * XXX - It would be nice to freemsg(mp) now, but
			 * other code uses the db_type field.
			 */
		} else if (mp != NULL) {
			ATRACE(audio_process_play, ATR_TXOUT, mp);
			/*
			 * If mp is not null, it is because we ran out of
			 * aud_cmd structures. Release any aud_cmd's that
			 * may have been used and put the mblk back on
			 * the queue.
			 *
			 * XXX - If mblk_t is larger than MAX resources
			 * then it will block the queue forever!
			 */

			/*
			 * Release mblk and command chains at the tail of
			 * the list.
			 */
			putbq(as->writeq, head_mp);	/* restore mblk chain */
			audio_delete_cmds(&as->cmdlist, head_cmdp, cmdp);

			/*
			 * Don't try to process any more mblk's at this
			 * time.
			 */
			break;
		}

		/*
		 * Cmdp still points to the last fragment in the chain.
		 * Set the lastfragment pointer in each aud_cmd to point
		 * to the last fragment to simplify future processing.
		 */
		{
			struct aud_cmd	*p;

			for (p = head_cmdp; p; p = p->next) {
				p->lastfragment = cmdp;
			}
		}

		ASSERT(head_cmdp->lastfragment != 0);
		ASSERT(head_cmdp->lastfragment->lastfragment != 0);

		/*
		 * The last fragment gets the pointer to the mblk.
		 */
		head_cmdp->lastfragment->dihandle = (void *)head_mp;

		/*
		 * Make the device aware of the new output tasks
		 */
		AUD_QUEUECMD(as, head_cmdp);
	}
	ATRACE(audio_process_play, 'DONE', cmdp);

	/*
	 * The device dependent portion of the driver is responsible for
	 * completing pseudo IO. The device dependent driver must mark
	 * the pseudo IO as "done" and then call audio_xmit_garbage_collect().
	 */

	/*
	 * If no messages left, and no data in write buffers, wake up
	 * audio_close() if necessary.  Ignore errors if draining.
	 *
	 * XXX - The test for "empty list" is ugly due to the "processed"
	 * fragment that may be at the end of the list.
	 */
	if (as->draining && (mp == NULL) &&
	    ((as->cmdlist.head == NULL) ||
	    (as->cmdlist.head == as->cmdlist.tail &&
	    as->cmdlist.head->processed))) {
		as->draining = FALSE;
		wakeup((caddr_t)&as->draining);
	} else if (AUD_GETFLAG(as, AUD_ERRORRESET)) {
		/* Only signal when this flag is set for the first time */
		if (!as->info.error) {
			as->info.error = TRUE;
			notify = TRUE;
		}
	}
	(void) splx(s);

	/* If a state change occurred, send a signal to the control device */
	if (notify)
		audio_sendsig(as->control_as);

	return;
} /* audio_process_play */


/*
 * Since putnext() is a macro, it is convenient to have this simple read
 * put procedure to keep from having to dequeue packets in the service
 * procedure.
 */
void
audio_rput(q, mp)
	queue_t		*q;
	mblk_t		*mp;
{
	ATRACE(audio_rput, ATR_RQ, mp->b_datap->db_type);

	aprintf("NOT EXPECTED: audio_rput called!\n");
	call_debug("audio_rput");
	putnext(q, mp);

	return;
}


/*
 * The read service procedure is scheduled when the upstream read queue
 * is flushed, to make sure that further record buffers are processed.
 *
 * It can also be scheduled if the driver is flow controlled by (canput() == 0)
 */
void
audio_rsrv(q)
	queue_t		*q;
{
	aud_stream_t	*as;

	ATRACE(audio_rsrv, ATR_NONE, q);
	as = (aud_stream_t *)q->q_ptr;
	audio_process_record(as->record_as);

	return;
}


/*
 * audio_receive_collect - Collect completed receive buffers. Also
 * garbage collect unused receive buffers when IO has been stopped.
 * Return 1 if read side was flow controlled.
 */
static int
audio_receive_collect(as)
	aud_stream_t	*as;
{
	mblk_t		*mp;
	struct {
		aud_cmd_t	*head;
		aud_cmd_t	*tail;
	}	packet;
	aud_cmd_t	*cmdp;
	int		flow_control;

	ASSERT(as != NULL);

	for (cmdp = as->cmdlist.head;
	    cmdp != NULL && cmdp->done;
	    cmdp = as->cmdlist.head) {

		packet.head = NULL;
		do {
			mp = (mblk_t *)cmdp->dihandle;	/* get buffer ptr */
			ASSERT(mp != NULL);

			/*
			 * Empty fragments should not hurt anyone.
			 * Packet.head gets set to 1st non-empty fragment.
			 */
			if (cmdp->skip) {
				ATRACEI(audio_receive_collect, 'FREE', mp);
				freemsg(mp);
				/* XXX - TIDY this up */
				cmdp->dihandle = 0;
				cmdp->data = 0;
				cmdp->enddata = 0;
				mp = 0;

				/*
				 * XXX - If this is going to remain a
				 * subroutine, it should return both
				 * "notify" and "flow_control".
				 */
				audio_sendsig(as); /* notify user of error */
			} else {
				/* Set STREAMS end of data */
				mp->b_wptr = cmdp->data;

				ATRACEI(audio_receive_collect,
					'frag', mp->b_wptr - mp->b_rptr);

				/*
				 * If start of packet not set yet, this must
				 * be it.  Don't chain the 1st fragment to the
				 * "previous".
				 */
				if (packet.head == NULL)  {
					packet.head = cmdp;
				} else {
					mblk_t	*tmp;

					/* Chain up current mblk to list */
					tmp = (mblk_t *)packet.tail->dihandle;
					tmp->b_cont = mp;
				}
			}
			packet.tail = cmdp;

			if (cmdp == cmdp->lastfragment)
				break;
		} while ((cmdp = cmdp->next) != NULL);

		if (packet.head) {
			/*
			 * Collect new received packets on driver's private
			 * readq for this aud_stream.
			 */
			ASSERT(as->readq->q_flag & QREADR);
			mp = (mblk_t *)packet.head->dihandle;
			putq(as->readq, mp);
		}

		/*
		 * XXX - As soon as we start using DBRI's CDP command for
		 * the receive side, we will need logic similar to that
		 * in audio_xmit_garbage_collect() in order to maintain
		 * an end-of-list command structure for the benefit of
		 * the device.
		 */
		audio_delete_cmds(&as->cmdlist, as->cmdlist.head,
				    packet.tail);
	} /* for loop */
	ATRACEI(audio_receive_collect, 'DONE', cmdp);

	flow_control = FALSE;
	ASSERT(as->readq->q_flag & QREADR);
	while ((mp = getq(as->readq)) != NULL) {
		if (mp->b_datap->db_type <= QPCTL &&
		    !canput(as->readq->q_next)) {
			ATRACEI(audio_receive_collect,
				'flow', as->readq->q_next);
			putbq(as->readq, mp);	/* read side is blocked */
			flow_control = TRUE;
			break;
		}
		ATRACEI(audio_receive_collect, 'putn', mp);

		/*
		 * Flow control is ok. Send received packet to upper module.
		 */
		putnext(as->readq, mp);
	}

	return (flow_control);
}


/*
 * Send record buffers upstream, if ready.  If recording is not paused,
 * make sure record buffers are allocated.
 */
void
audio_process_record(as)
	aud_stream_t		*as;
{
	mblk_t		*mp;
	int		s;
	aud_cmd_t	*cmdp;
	aud_cmd_t	*headp;
	int		flow_control;

	ASSERT(as != NULL);

	/* If no read access, don't bother even trying */
	if (!ISRECORDSTREAM(as)) {
		ATRACE(audio_process_record, 'bgus', as);
		return;
	}
	ATRACE(audio_process_record, 'recP', as);

	/*
	 * Collect finished record buffers and send upstream.  If
	 * recording was paused, all buffers were marked done, even if
	 * they were unused. The same goes for error condition.
	 *
	 * Note: We need to chain up potentially multiple mblks for
	 * datacomm.
	 */
	s = splstr();

	flow_control = audio_receive_collect(as);

	if (flow_control) {
		ATRACE(audio_process_record, '-fc-', as);
	} else {
		ATRACE(audio_process_record, '-ok-', as);
	}

	/*
	 * If paused or upstream flow control hit high water, don't
	 * allocate new record buffers.
	 */
	headp = (aud_cmd_t *)NULL;
	if (!as->info.pause && !flow_control) {
		/*
		 * As long as there are free command blocks, allocate new
		 * buffers for recording.
		 */
		mp = NULL;
		while ((as->cmdlist.free != NULL) &&
			((mp = allocb(as->input_size + 8, BPRI_MED)) != NULL)) {
			/* XXX3 - 2 word safety region on buffer */

			/* Allocate and initialize a command block */
			audio_alloc_cmd(&as->cmdlist, cmdp);
			if (headp == NULL) {
				headp = cmdp;
			}
			cmdp->data = mp->b_rptr = mp->b_wptr;
			cmdp->enddata = cmdp->data + as->input_size;
			cmdp->dihandle = (void *)mp;

			/* iotype not used for receive */
			cmdp->iotype = M_DATA;
			cmdp->lastfragment = cmdp; /* not known yet */
			cmdp->done = FALSE;
			cmdp->skip = FALSE;
			cmdp->processed = FALSE;

			/* Add it to the cmd chain */
			audio_append_cmd(&as->cmdlist, cmdp);
			ATRACE(audio_process_record, ATR_NEWR, mp);

			ASSERT(cmdp->enddata - cmdp->data >= as->input_size);
		}

		if  ((mp == NULL) && (as->cmdlist.free != NULL)) {
			aprintf("audio_process_record: allocb mp == NULL\n");
		}

		/*
		 * Queue up dbri cmd after available free cmds are
		 * chained up and send to device if we have allocated new
		 * command blocks.
		 */
		if (headp != NULL) {
			AUD_QUEUECMD(as, headp);
		}
	}

	/* If record overflow occurred, send a signal to the control device */
	if (AUD_GETFLAG(as, AUD_ERRORRESET)) {
		/* Only signal when this flag is set for the first time */
		if (!as->info.error) {
			as->info.error = TRUE;
			audio_sendsig(as->control_as);
		}
	}

	(void) splx(s);
	return;
} /* audio_process_record */


/*
 * Send a SIGPOLL up the specified stream.
 */
void
audio_sendsig(as)
	aud_stream_t		*as;	/* always points to write side */
{
	mblk_t		*notify_mp;

	/* If stream is not open, simply return */
	if (as->readq == NULL)
		return;

	/* Init a message to send a SIGPOLL upstream */
	if ((notify_mp = allocb(sizeof (char), BPRI_HI)) == NULL)
		return;

	notify_mp->b_datap->db_type = M_PCSIG;
	*notify_mp->b_wptr++ = SIGPOLL;

	/* Signal the specified stream */
	ATRACE(audio_sendsig, ATR_SIGCTL, as);
	putnext(as->readq, notify_mp);

	return;
}


/*
 * The next two routines are used to pause reads or writes.  Pause is
 * used to temporarily suspend i/o without losing the contents of the
 * buffer.
 */
void
audio_pause_record(as)
	aud_stream_t	*as;
{
	aud_cmd_t	*cmdp;

	if (!ISRECORDSTREAM(as) || (as->mode != AUDMODE_AUDIO))
		return;

	as->info.pause = TRUE;
	AUD_STOP(as);

	/*
	 * When recording is paused, partially filled buffers are sent
	 * upstream and unused buffers are released.  Mark all command
	 * buffers done and let audio_process_record() handle them.
	 *
	 * XXX - There could be a problem here as packets have multiple
	 * cmds.
	 */
	for (cmdp = as->cmdlist.head; cmdp != NULL; cmdp = cmdp->next)
		cmdp->done = TRUE;

	/* Flush the device's chained command list */
	AUD_FLUSHCMD(as);

	/* Process partially filled buffer and release the rest */
	audio_process_record(as);
	return;
}


void
audio_pause_play(as)
	aud_stream_t	*as;
{
	int	s;

	if (as != as->play_as) {
		aprintf("pause_play called for non-play stream!\n");
		return;
	}

	if (as->info.open && (as->mode == AUDMODE_AUDIO)) {
		s = splstr();
		as->info.pause = TRUE;
		AUD_STOP(as);
		(void) splx(s);
	}

	return;
}


/*
 * The next two routines are called from ioctls to resume paused
 * read/write.
 */
void
audio_resume_record(as)
	aud_stream_t	*as;
{
	if (!as->info.pause)
		return;

	/* Must clear pause flag before calling audio_process_record */
	as->info.pause = FALSE;

	/* audio_process_record() will call the AUD_START routine */
	audio_process_record(as);

	return;
}


void
audio_resume_play(as)
	aud_stream_t	*as;
{
	if (!as->info.pause)
		return;

	/* Must clear pause flag before calling audio_process_play */
	as->info.pause = FALSE;

	/* Queue up output buffers and enable output conversion */
	audio_process_play(as);
	AUD_START(as);

	return;
}
