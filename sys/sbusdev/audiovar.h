/*
 * @(#) audiovar.h 1.1@(#) Copyright (c) 1991-92 Sun Microsystems, Inc.
 */

#ifndef _sbusdev_audiovar_h
#define	_sbusdev_audiovar_h

/*
 * The audio driver is divided into generic (device-independent) and
 * device-specific modules.  The generic routines handle most STREAMS
 * protocol issues, communicating with the device-specific code via
 * function callouts and a chained control block structure.
 *
 * Separate control block lists are maintained for reading (record) and
 * writing (play).  These control blocks simulate a chained-DMA
 * structure, in that each block controls the transfer of data between
 * the device and a single contiguous memory segment.
 *
 * The command block contains buffer start and stop addresses, a link
 * address to the next block in the chain, a 'done' flag, a 'skip' flag
 * (indicating that this command block contains no data), and a pointer
 * to the STREAMS data block structure which is private to the generic
 * driver.
 *
 * The device-specific audio driver code is expected to honor the 'skip'
 * flag and set the 'done' flag when it has completed processing the
 * command block (i.e., the data transfer, if any, is complete).  For
 * record command blocks, it is also expected to add to the 'data'
 * pointer the number of bytes successfully read from the device.
 *
 *
 * The device-specific driver module must initialize the static STREAMS
 * control structures and must provide an identify routine (sbus-only),
 * an attach routine, and an open routine.  The open routine verifies the
 * device unit number and calls the generic open routine with the address
 * of the audio_state structure for that unit.
 *
 *
 * The generic audio driver makes calls to the device-specific code
 * through an ops-vector table.  The following routines must be provided:
 *
 * The 'close' routine is called after either the play or record stream
 * is closed.  It may perform device-specific cleanup and initialization.
 *
 * void dev_close(as)
 * 	aud_stream_t		*as;	// Pointer to audio device state
 *
 *
 * The 'ioctl' routine is called from the STREAMS write put procedure
 * when a non-generic ioctl is encountered (AUDIO_SETINFO, AUDIO_GETINFO,
 * and AUDIO_DRAIN are the generic ioctls).  Any required data mblk_t is
 * allocated; its address is given by mp->b_cont (if this is a read/write
 * ioctl, the user-supplied buffer at mp->b_cont is reused).  If data is
 * successfully returned, the iocp->ioc_count field should be set with
 * the number of bytes returned.  If an error occurs, the 'ioctl' routine
 * should set iocp->ioc_error to the appropriate error code.  Otherwise,
 * the returned value should be AUDRETURN_CHANGE if a device state change
 * occurred (in which case a signal is sent to the control device, if any) and
 * AUDRETURN_NOCHANGE if no signal should be sent. If the ioctl can not complete
 * right away, it should return AUDRETURN_DELAYED indicating that
 * it will ack the ioctl at a later time.
 *
 * aud_return_t dev_ioctl(as, mp, iocp)
 * 	aud_stream_t	*as;		// Pointer to audio device state
 * 	mblk_t		*mp;		// ioctl STREAMS message block
 * 	struct iocblk	*iocp;		// M_IOCTL message data
 *
 *
 * The 'start' routine is called to start i/o.  Ordinarily, it is called
 * from the device-specific 'queuecmd' routine, but it is also called
 * when paused output is resumed.
 *
 * void dev_start(as)
 * 	aud_stream_t	*as;		// Pointer to audio device state
 *
 *
 * The 'stop' routine is called to stop i/o.  It is called when i/o is
 * paused, flushed, or the device is closed.  Note that currently queued
 * command blocks should not be flushed by this routine, since i/o may be
 * resumed from the current point.
 *
 * void dev_stop(as)
 * 	aud_stream_t	*as;		// Pointer to audio device state
 *
 *
 * The 'setflag' routine is called to get a single device-specific flag.
 * The flag argument is either AUD_ACTIVE (return the active flag) or
 * AUD_ERRORRESET (zero the error flag, returning its previous value).
 * (The val argument is currently ignored.)
 *
 * void dev_setflag(as, flag, val)
 * 	aud_stream_t	*as;		// Pointer to audio device state
 * 	enum aud_opflag	flag;		// AUD_ACTIVE || AUD_ERRORESET
 *
 *
 * The 'setinfo' routine is called to get or set device-specific fields
 * in the audio state structure.  If mp is NULL, the sample counters and
 * active flags should be set in v.  If mp is not NULL, then mp->b_cont->data
 * points to the audio_info_t structure supplied in an AUDIO_SETINFO ioctl (ip).
 * All device-specific fields (gains, ports, sample counts) in both v and the
 * device itself should be updated, as long as the corresponding field in
 * ip is not set to AUD_INIT_VALUE.  When the sample counters are set,
 * the value returned in v should be the previous value. If the setinfo
 * can not complete right away, it should return AUDRETURN_DELAYED
 * indicating that it will ack the ioctl at a later time. If an error
 * occurs on setinfo, the iocp->ioc_error should be set as in dev_ioctl
 *
 * aud_return_t dev_setinfo(as, mp, iocp)
 * 	aud_stream_t	*as;		// Pointer to audio device state
 * 	mblk_t		*mp;		// user info structure or NULL
 *	struct iocblk	*iocp;		// M_IOCTL message data
 *
 *
 * The 'queuecmd' routine is called whenever a new command block is
 * linked into the chained command list.  The device-specific code must
 * ensure that the command is enqueued to the device and that i/o, if not
 * currently active, is started.
 *
 * void dev_queuecmd(as, cmdp)
 * 	aud_stream_t	*as;		// Pointer to audio device state
 * 	struct aud_cmd	*cmdp;		// new command block to queue
 *
 *
 * The 'flushcmd' routine is called whenever the chained command list is
 * flushed.  It is only called after i/o has been stopped (via the 'stop'
 * routine) and after the command list in the audio state structure has
 * been cleared.  The device-specific code should flush the device's
 * queued command list.
 *
 * void dev_flushcmd(as)
 * 	aud_stream_t	*as;		// Pointer to audio device state
 */


#if defined(STANDALONE)
/*
 * If we compile with standalone, we'll include our copy of the generic
 * driver but it will have all external symbols prefixed with an 's'.
 * This allows one to test a new DBRI driver with an old generic+AMD
 * driver in the kernel or test a new AMD driver with an older
 * generic+DBRI kernel in the driver.
 */
#define	Audio_debug		sAudio_debug
#define	audio_resume_record	saudio_resume_record
#define	audio_flush_cmdlist	saudio_flush_cmdlist
#define	audio_do_setinfo	saudio_do_setinfo
#define	audio_pause_play	saudio_pause_play
#define	audio_process_record	saudio_process_record
#define	audio_resume_play	saudio_resume_play
#define	audio_open		saudio_open
#define	audio_rput		saudio_rput
#define	audio_wput		saudio_wput
#define	audio_rsrv		saudio_rsrv
#define	audio_wsrv		saudio_wsrv
#define	audio_pause_record	saudio_pause_record
#define	audio_process_play	saudio_process_play
#define	audio_close		saudio_close
#define	audioamd_ops		saudioamd_ops
#define	audio_79C30_info	saudio_79C30_info
#define	audio_receive_collect	saudio_receive_collect
#define	audio_xmit_garbage_collect	saudio_xmit_garbage_collect
#define	audio_sendsig		saudio_sendsig
#define	audio_append_command	saudio_append_command
#define	audio_ch2as		saudio_ch2as
#define	audio_delete_cmds	saudio_delete_cmds
#define	audio_ioctl		saudio_ioctl
#endif

/* Various generic audio driver constants */
#define	AUD_INITVALUE	(~0)
#define	Modify(X)	((unsigned)(X) != AUD_INITVALUE)
#define	Modifys(X)	((X) != (unsigned short)AUD_INITVALUE)
#define	Modifyc(X)	((X) != (unsigned char)AUD_INITVALUE)

#define	SLEEPPRI	((PZERO + 1) | PCATCH)

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif /* !TRUE */

/* Define the virtual chained-DMA control structure */
typedef struct aud_cmd {
	unsigned char		*data;		/* address of next transfer */
	unsigned char		*enddata;	/* address+1 of last transfer */
	struct aud_cmd		*next;		/* pointer to next or NULL */
	struct aud_cmd		*lastfragment;	/* last fragment in packet */
	/* XXX - Note that skip and done MUST be halfword aligned! */
	unsigned char		skip;		/* TRUE => no xfers on buffer */
	unsigned char		done;		/* TRUE => buffer processed */
	unsigned char		iotype;		/* copy of mblk's db_type */
	unsigned char		processed;	/*
						 * Processed cmd at head of list
						 * so that md's never hit NULL
						 * pointer. Intr routine sets
						 * true on all EOF frames.
						 */
	void			*dihandle;	/* dihandle to generic driver */
} aud_cmd_t;

/* Define the list-head for queued control structures */
typedef struct aud_cmdlist {
	aud_cmd_t		*head;	/* First queued command block */
	aud_cmd_t		*tail;	/* Last queued command block */
	aud_cmd_t		*free;	/* Head of free list */
	caddr_t			memory;	/* Cmdlist pool from kmem_alloc */
	int			size;	/* Size of cmdlist pool */
} aud_cmdlist_t;

/* Define possible return values from the setinfo and ioctl calls */
typedef enum {
	AUDRETURN_CHANGE,
	AUDRETURN_NOCHANGE,
	AUDRETURN_DELAYED,
}	aud_return_t;

/* Define the ops-vector table for device-specific callouts */
struct aud_ops {
	void			(*close)();	/* device close routine */
	aud_return_t		(*ioctl)();	/* device ioctl routine */
	void			(*start)();	/* device start routine */
	void			(*stop)();	/* device stop routine */
	unsigned		(*setflag)();	/* read/alter state value */
	aud_return_t		(*setinfo)();	/* update/alter device info */
	void			(*queuecmd)();	/* add a chained cmd */
	void			(*flushcmd)();	/* flush chained cmd block */
};

/* Define pseudo-routine names for the device-specific callouts */
#define	AUD_CLOSE(A)		(*(A)->v->ops->close)(A)
#define	AUD_IOCTL(A, M, I)	(*(A)->v->ops->ioctl)(A, M, I)
#define	AUD_START(A)		(*(A)->v->ops->start)(A)
#define	AUD_STOP(A)		(*(A)->v->ops->stop)(A)
#define	AUD_SETFLAG(A, F, X)	(*(A)->v->ops->setflag)(A, F, X)
#define	AUD_GETFLAG(A, F)	(*(A)->v->ops->setflag)(A, F, AUD_INITVALUE)
#define	AUD_SETINFO(A, M, I)	(*(A)->v->ops->setinfo)(A, M, I)
#define	AUD_GETINFO(A)		(void) (*(A)->v->ops->setinfo)(A, NULL, NULL)
#define	AUD_QUEUECMD(A, C)	(*(A)->v->ops->queuecmd)(A, C)
#define	AUD_FLUSHCMD(A)		(*(A)->v->ops->flushcmd)(A)

/* Define legal values for the 'flag' argument to the 'setflag' callout */
enum aud_opflag {
	AUD_ACTIVE,		/* active flag */
	AUD_ERRORRESET,		/* error flag (reset after read) */
};

/*
 * The audio stream type determines the legal operations for a stream in the
 * generic portion of an audio driver.
 */
typedef enum {
	AUDTYPE_NONE	= 00,	/* Not a legal device */
	AUDTYPE_DATA	= 01,	/* Data, IOCTL, etc., but not signals */
	AUDTYPE_CONTROL	= 02,	/* IOCTL, etc., but not M_DATA */
	AUDTYPE_BOTH	= 03,	/* Anything is ok, signals delivered */
}	aud_streamtype_t;

#define	ISPLAYSTREAM(as)	(ISDATASTREAM(as) && (as->openflag & FWRITE))
#define	ISRECORDSTREAM(as)	(ISDATASTREAM(as) && (as->openflag & FREAD))
#define	ISDATASTREAM(as)	((((int)as->type)&((int)AUDTYPE_DATA)) != 0)
#define	ISCONTROLSTREAM(as)	((((int)as->type)&((int)AUDTYPE_CONTROL)) != 0)

typedef enum {
	AUDMODE_NONE	= 00,	/* Not a legal mode */
	AUDMODE_AUDIO	= 01,	/* Transparent audio mode */
	AUDMODE_HDLC	= 02,	/* HDLC datacomm mode */
}	aud_modetype_t;


/* This structure describes the state of the audio device and queues */
typedef struct aud_state {
	void		*devdata;	/* device-specific data ptr */
	unsigned	monitor_gain;	/* input to output mix: 0 - 255 */
	struct aud_ops	*ops;		/* audio ops vector */
	unsigned char	output_muted;	/* TRUE if speaker muted */
} aud_state_t;

/*
 * STREAMS routines pass the address of a 'struct audstream' when
 * calling put and service procedures.  This structure points to
 * the STREAMS queues and back to the containing 'struct aud_state'.
 */
typedef struct aud_stream {
	aud_state_t		*v;		/* pointer to driver data */
	aud_streamtype_t	type;		/* defines legal operations */
	aud_modetype_t		mode;		/* xparent audio or HDLC data */
	struct aud_stream	*control_as;	/* stream's control stream */
	struct aud_stream	*play_as;	/* stream's play stream */
	struct aud_stream	*record_as;	/* stream's record stream */
	int			openflag;	/* open flag & (FREAD|FWRITE) */
	int			draining;	/* TRUE if output draining */
	queue_t			*readq;		/* STREAMS read queue */
	queue_t			*writeq;	/* STREAMS write queue */
	struct aud_cmdlist	cmdlist;	/* command chains */
	audio_prinfo_t		info;		/* info for this stream side */
	int			input_size;	/* private record buffer size */
} aud_stream_t;


/* Global routines defined in audio.c for use by device-dependent modules */
extern int	audio_open();
extern void	audio_close();
extern void	audio_process_play();
extern void	audio_process_record();
extern void	audio_sendsig();
extern void	audio_wput();
extern void	audio_wsrv();
extern void	audio_rput();
extern void	audio_rsrv();


/*
 * XXX - Routines not defined elsewhere and which should not need to be
 * defined here.
 */
extern char	*sprintf();
extern int	printf();
extern int	strncmp();
extern int	kmem_free();
extern int	assfail();
extern int	addintr();
extern int	remintr();
extern int	timeout();
extern int	untimeout();
extern int	adddma();
extern int	splr();
extern int	splx();
extern int	spltty();
extern int	usec_delay();
extern int	call_debug();
extern int	wakeup();
extern int	sleep();
extern int	panic();
extern int	freemsg();
extern int	qreply();
extern int	flushq();
extern int	putbq();
extern int	canput();

#endif /* !_sbusdev_audiovar_h */
