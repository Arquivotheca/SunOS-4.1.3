/*	@(#)stream.h 1.1 92/07/30 SMI; from S5R3.1 sys/stream.h 10.10.1.1	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _sys_stream_h
#define _sys_stream_h

/*
 * data queue
 */
struct	queue {
	struct	qinit	*q_qinfo;	/* procs and limits for queue */
	struct	msgb	*q_first;	/* first data block */
	struct	msgb	*q_last;	/* last data block */
	struct	queue	*q_next;	/* Q of next stream */
	struct	queue	*q_link;	/* to next Q for scheduling */
	caddr_t	q_ptr;			/* to private data structure */
	ushort	q_count;		/* estimate of number of bytes on Q */
	ushort	q_flag;			/* queue state */
	short	q_minpsz;		/* min packet size accepted by this module */
	short	q_maxpsz;		/* max packet size accepted by this module */
	ushort	q_hiwat;		/* queue high water mark */
	ushort	q_lowat;		/* queue low water mark */
};

typedef struct queue queue_t;

/*
 * Queue flags
 */
#define	QENAB	01			/* Queue is already enabled to run */
#define	QWANTR	02			/* Someone wants to read Q */
#define	QWANTW	04			/* Someone wants to write Q */
#define	QFULL	010			/* Q is considered full */
#define	QREADR	020			/* This is the reader (first) Q */
#define	QUSE	040			/* This queue in use (allocation) */
#define	QNOENB	0100			/* Don't enable Q via putq */



/*
 * module information structure
 */
struct module_info {
	ushort	mi_idnum;		/* module id number */
	char 	*mi_idname;		/* module name */
	short	mi_minpsz;		/* min packet size accepted */
	short	mi_maxpsz;		/* max packet size accepted */
	ushort	mi_hiwat;		/* hi-water mark */
	ushort 	mi_lowat;		/* lo-water mark */
};


/*
 * queue information structure
 */
struct	qinit {
	int	(*qi_putp)();		/* put procedure */
	int	(*qi_srvp)();		/* service procedure */
	int	(*qi_qopen)();		/* called on startup */
	int	(*qi_qclose)();		/* called on finish */
	int	(*qi_qadmin)();		/* for 3bnet only */
	struct module_info *qi_minfo;	/* module information structure */
	struct module_stat *qi_mstat;	/* module statistics structure */
};




/*
 * Streamtab (used in cdevsw and fmodsw to point to module or driver)
 */

struct streamtab {
	struct qinit	*st_rdinit;	/* qinit for read side */
	struct qinit	*st_wrinit;	/* qinit for write side */
	struct qinit	*st_muxrinit;	/* mux qinit for read */
	struct qinit	*st_muxwinit;	/* mux qinit for write */
	char		**st_modlist;	/* list of modules to be pushed */
};



/*
 * Header for a stream: interface to rest of system
 */

struct stdata {
	struct	queue *sd_wrq;		/* write queue */
	struct	msgb *sd_iocblk;	/* return block for ioctl */
	struct	vnode *sd_vnode;	/* backptr, for hangups */
	struct 	streamtab *sd_strtab;	/* pointer to streamtab for stream */
	long	sd_flag;		/* state/flags */
	long	sd_iocid;		/* ioctl id */
	ushort	sd_iocwait;		/* count of procs waiting to do ioctl */
	short	sd_pgrp;		/* process group, for job signals */
	short	sd_sigio;		/* pid/pgrp for SIGIO */
	ushort	sd_wroff;		/* write offset */
	unsigned char  sd_error;	/* hangup or error to set u.u_error */
	short	sd_vmin;		/* minimum characters to return on read */
	ushort	sd_vtime;		/* timeout for sd_vmin */
	int	sd_deficit;		/* amount needed to reach sd_vmin */
	int	sd_pushcnt;		/* number of pushes done on stream */
	struct	strevent *sd_siglist;	/* pid linked list to rcv SIGPOLL sig */
	struct	strevent *sd_pollist;	/* pid linked list to wakeup poll() */
	int	sd_sigflags;		/* logical OR of all siglist events */
	int	sd_pollflags;		/* logical OR of all pollist events */
	struct	proc *sd_selr;		/* process selecting on read */
	struct	proc *sd_selw;		/* process selecting on write */
	struct	proc *sd_sele;		/* process selecting on exception */
	struct	stdata *sd_next;	/* linked list of active streams */
	struct	stdata *sd_prev;	/* doubly-linked, that is */
};



/*
 * stdata flag field defines
 */
#define	IOCWAIT		 01		/* Someone wants to do ioctl */
#define	RSLEEP		 02		/* Someone wants to read/recv msg */
#define	WSLEEP		 04		/* Someone wants to write */
#define	STRPRI	        010		/* An M_PCPROTO is at stream head */
#define	STRHUP	        020		/* Device has vanished */
#define	STWOPEN	        040		/* waiting for 1st open */
#define	STPLEX	       0100		/* stream is being multiplexed */
#define	NOCTTY	       0200		/* don't make this a ctty */
#define	RMSGDIS	       0400		/* read msg discard */
#define	RMSGNODIS     01000		/* read msg no discard */
#define	STRERR	      02000		/* fatal error from M_ERROR */
#define	STRTIME       04000		/* used with timeout strtime */
#define	STR2TIME     010000		/* used with timeout str2time */
#define	STR3TIME     020000		/* used with timeout str3time */
#define	NBIO         040000		/* 4.2-style non-blocking I/O */
#define	ASYNC       0100000		/* 4.2-style asynchronous I/O */
#define OLDNDELAY   0200000		/* use old TTY semantics for NDELAY reads */
#define STRTOSTOP   0400000		/* stream TOSTOP bit */
#define	CLKRUNNING 01000000		/* VTIME clock running */
#define	TIMEDOUT   02000000		/* VTIME clock ran out */
#define	RCOLL	   04000000		/* collision on read select */
#define	WCOLL	  010000000		/* collision on write select */
#define	ECOLL	  020000000		/* collision on exception select */
#define	STRCLOSE  040000000		/* wait for a close to complete */


/* 
 * structure for storing triples of mux'ed streams 
 */
struct linkblk {
	queue_t *l_qtop;	/* lowest level write queue of upper stream */
	queue_t *l_qbot;	/* highest level write queue of lower stream */
	int      l_index;	/* file[] index for lower stream. */
};


/*
 * Structure of list of processes to be sent SIGPOLL signal
 * on request, or for processes sleeping on poll().  The valid 
 * SIGPOLL events are defined in stropts.h, and the valid poll()
 * events are defined in poll.h.
 */
struct strevent {
	union {
		struct {
			struct proc 	*procp;
			long 		events;
			} e;	/* stream event */
		struct {
			int (*func)();
			long arg;
			int size;
			} b;	/* bufcall event */
	} x;
	struct strevent *se_next;
};

#define se_procp	x.e.procp
#define se_events	x.e.events
#define se_func		x.b.func
#define se_arg		x.b.arg
#define se_size		x.b.size

#define SE_SLEEP	0	/* ok to sleep in allocation */
#define SE_NOSLP	1	/* don't sleep in allocation */

/*
 * bufcall list
 */
struct bclist {
	struct strevent	*bc_head;
	struct strevent	*bc_tail;
};


/*
 * Class 0 data buffer freeing routine
 */
struct free_rtn {
	void (*free_func)();
	char *free_arg;
};

/*
 *  Data block descriptor
 */

#define	DB_CACHESIZE	8	/* must be >= 4 bytes for compatibility */

struct datab {
	union {
		struct datab	*freep;
		struct free_rtn *frtnp;
	} db_f;
	unsigned char	*db_base;
	unsigned char	*db_lim;
	unsigned char	db_ref;
	unsigned char	db_type;
	unsigned char	db_class;
	unsigned char	db_pad;		/* db_cache must be word-aligned */
	unsigned int	db_size;
	unsigned char	db_cache[DB_CACHESIZE];
};

#define db_freep db_f.freep
#define db_frtnp db_f.frtnp


/*
 * Message block descriptor
 */
struct	msgb {
	struct	msgb	*b_next;
	struct	msgb	*b_prev;
	struct	msgb	*b_cont;
	unsigned char	*b_rptr;
	unsigned char	*b_wptr;
	struct	datab 	*b_datap;
};

typedef struct msgb mblk_t;
typedef struct datab dblk_t;
typedef struct free_rtn frtn_t;


/********************************************************************************/
/* 			Streams message types				        */
/********************************************************************************/


/*
 * Data and protocol messages (regular priority)
 */
#define	M_DATA		00		/* regular data */
#define	M_PROTO		01		/* protocol control */

/*
 * Control messages (regular priority)
 */
#define	M_BREAK		010		/* line break */
#define	M_PASSFP	011		/* pass file pointer */
#define	M_SIG		013		/* generate process signal */
#define	M_DELAY		014		/* real-time xmit delay (1 param) */
#define	M_CTL		015		/* device-specific control message */
#define	M_IOCTL		016		/* ioctl; set/get params */
#define	M_SETOPTS	020		/* set various stream head options */


/*
 * Control messages (high priority; go to head of queue)
 */
#define	M_IOCACK	0201		/* acknowledge ioctl */
#define	M_IOCNAK	0202		/* negative ioctl acknowledge */
#define	M_PCPROTO	0203		/* priority proto message */
#define	M_PCSIG		0204		/* generate process signal */
#define	M_FLUSH		0206		/* flush your queues */
#define	M_STOP		0207		/* stop transmission immediately */
#define	M_START		0210		/* restart transmission after stop */
#define	M_HANGUP	0211		/* line disconnect */
#define	M_ERROR		0212		/* fatal error used to set u.u_error */
#define	M_STOPI		0213		/* stop reception immediately */
#define	M_STARTI	0214		/* restart reception after stop */
#define	M_UNHANGUP	0215		/* line reconnect, sigh */


/*
 * Queue message class definitions.  
 */
#define QNORM    0			/* normal messages */
#define QPCTL 0200			/* priority cntrl messages */



/*
 *  IOCTL structure - this structure is the format of the M_IOCTL message type.
 */
struct iocblk {
	int 	ioc_cmd;		/* ioctl command type */
	ushort	ioc_uid;		/* effective uid of user */
	ushort	ioc_gid;		/* effective gid of user */
	uint	ioc_id;			/* ioctl id */
	uint	ioc_count;		/* count of bytes in data field */
	int	ioc_error;		/* error code */
	int	ioc_rval;		/* return value  */
};


/*
 * Options structure for M_SETOPTS message.  This is sent upstream
 * by driver to set stream head options.
 */
struct stroptions {
	short	so_flags;		/* options to set */
	short	so_readopt;		/* read option */
	ushort	so_wroff;		/* write offset */
	short	so_minpsz;		/* minimum read packet size */
	short	so_maxpsz;		/* maximum read packet size */
	ushort	so_hiwat;		/* read queue high water mark */
	ushort	so_lowat;		/* read queue low water mark */
	ushort	so_vmin;		/* minimum characters to return on read */
	ushort	so_vtime;		/* timeout for so_vmin */
	char	so_tostop;		/* stop on background write mode */
};

/* flags for stream options set message */

#define SO_ALL		 077		/* set all options */
#define SO_READOPT	  01		/* set read option */
#define SO_WROFF	  02		/* set write offset */
#define SO_MINPSZ	  04		/* set min packet size */
#define SO_MAXPSZ	 010		/* set max packet size */
#define SO_HIWAT	 020		/* set high water mark */
#define SO_LOWAT	 040		/* set low water mark */
#define SO_VMIN		0100		/* set vmin */
#define SO_VTIME	0200		/* set vtime */
#define SO_NDELON	0400		/* old TTY semantics for NDELAY reads and writes */
#define SO_NDELOFF     01000		/* old TTY semantics for NDELAY reads and writes */
#define SO_TOSTOP      02000		/* set stop on background write mode */




/********************************************************************************/
/*		Miscellaneous parameters and flags			        */
/********************************************************************************/

/*
 * Default timeout in seconds for ioctls and close
 */
#define STRTIMOUT 15

/*
 * Stream head default high/low water marks 
 */
#define STRHIGH	5120
#define STRLOW	1024

/*
 * flag values for stream io waiting procedure (strwaitq)
 */
#define WRITEWAIT	0x1	/* waiting for write event */
#define READWAIT	0x2	/* waiting for read event */
#define NOINTR		0x4	/* EINTR is not to be set for signal */
				/* and EAGAIN/EWOULDBLOCK is not to */
				/* be set if operation would block */
#define INTRRESTART	0x8	/* restart the system call if interrupted */

/*
 * sleep priorities for stream io
 */
#define	STIPRI	PZERO+3
#define	STOPRI	PZERO+3

/*
 * Block allocation parameters
 */
/* #define MAXBSIZE	4096	/* max block size */
#define MAXIOCBSZ	1024	/* max ioctl data block size */

/*
 * Copy modes for ioctls
 */
#define	K_TO_K_BOTH	0	/* Kernel to Kernel in and out */
#define	U_TO_K 		1	/* User to Kernel in, Kernel to User out*/
#define	K_TO_K  	2	/* Kernel to Kernel in, Kernel to User out */

/*
 * Values for stream flag in open to indicate module open, clone open;
 * return value for failure, and return flag to request that this
 * stream become a controlling tty.
 */
#define MODOPEN 	0x1	/* open as a module */
#define CLONEOPEN	0x2	/* open for clone, pick own minor device */
#define OPENFAIL	-1	/* returned for open failure */
#define	NEWCTTY		0x8000	/* ORed into return value */

/*
 * Priority definitions for block allocation.
 */
#define BPRI_LO		1
#define BPRI_MED	2
#define BPRI_HI		3

/*
 * Value for packet size that denotes infinity
 */
#define INFPSZ		-1

/*
 * Flags for flushq()
 */
#define FLUSHALL	1	/* flush all messages */
#define FLUSHDATA	0	/* don't flush control messages */


/************************************************************************/
/*	Definitions of Streams macros and function interfaces.	        */
/************************************************************************/

/*
 *  Queue scheduling macros
 */
#define setqsched()     qrunflag = 1	/* set up queue scheduler */
#define qready()	qrunflag	/* test if queues are ready to run */

/*
 * Definition of spl function needed to provide critical region protection
 * for streams drivers and modules.
 */
#define splstr() spltty()

/*
 * noenable - set queue so that putq() will not enable it.
 * enableok - set queue so that putq() can enable it.
 * canenable - check if queue can be enabled by putq().
 */
#define noenable(q)	(q)->q_flag |= QNOENB
#define enableok(q)	(q)->q_flag &= ~QNOENB
#define canenable(q)	!((q)->q_flag & QNOENB)

/*
 * Finding related queues
 */
#define	WR(q)		((q)+1)
#define	RD(q)		((q)-1)
#define	OTHERQ(q)	((q)->q_flag&QREADR? WR(q): RD(q))

/*
 * put a message of the next queue of the given queue
 */
#define putnext(q, mp)	((*q->q_next->q_qinfo->qi_putp)(q->q_next, mp))

/*
 * free a queue pair
 */
#define freeq(q) { \
	kmem_fast_free(&(caddr_t)queue_free, (caddr_t)q); \
	strst.queue.use--; \
}

/*
 * Test if data block type is one of the data messages (i.e. not a control
 * message).
 */
#define datamsg(type) (type == M_DATA || type == M_PROTO || type == M_PCPROTO)

/*
 * extract queue class of message block
 */
#define queclass(bp) (bp->b_datap->db_type & QPCTL)

/*
 * Align address on next lower word boundary
 */
#define straln(a)	(caddr_t)((long)(a) & ~(sizeof(int)-1))

/*
 * Copy data from one data buffer to another.
 * The addresses must be word aligned - if not, use bcopy!
 */
#ifdef u3b2
/*
 * Use the MOVBLW instruction on the 3b2.  
 */
asm	void
strbcpy(s, d, c)
{
%mem	s,d,c;
	
	MOVW	s,%r0
	MOVW	d,%r1
	MOVW	c,%r2
	ADDW2	&3,%r2
	LRSW3	&2,%r2,%r2
	MOVBLW
}

#else
#define	strbcpy(s, d, c)	bcopy((caddr_t)(s), (caddr_t)(d), (u_int)(c))
#endif

#ifdef	KERNEL
/*
 * declarations of common routines
 */
extern mblk_t *rmvb();
extern mblk_t *dupmsg();
extern mblk_t *copymsg();
extern mblk_t *allocb();
extern mblk_t *unlinkb();
extern mblk_t *dupb();
extern mblk_t *copyb();
extern mblk_t *getq();
extern int    putq();
extern queue_t *backq();
extern struct stdata *allocstr();
extern void freestr();
extern queue_t *allocq();
extern int    qenable();
extern mblk_t *unlinkb();
extern int    pullupmsg();
extern int    adjmsg();
extern struct queue *getendq();
extern struct linkblk *findlinks();
extern struct file *getf();
extern struct strevent *sealloc();
extern int   sefree();
extern mblk_t *esballoc();
extern int   esbbcall();
extern int   bufcall();
extern void  unbufcall();

/*
 * shared or externally configured data structures
 */
extern struct stdata *stream_free;	/* list of available stdata structs */
extern struct stdata *allstream;	/* list of active stdata structs */
extern queue_t	*queue_free;		/* list of available queue pairs */
extern struct linkblk linkblk[];	/* multiplexor link table */
extern struct strevent strevent[];	/* table of stream event cells */
extern struct strstat strst;		/* Streams statistics structure */
extern queue_t	*qhead;			/* head of runnable queue list */
extern queue_t	*qtail;			/* tail of runnable queue list */
extern int strmsgsz;			/* maximum stream message size */
extern int strctlsz;			/* maximum size of ctl part of message */
extern int nmuxlink;			/* number of multiplexor links */
extern int nstrpush;			/* maxmimum number of pushes allowed */
extern int nstrevent;			/* initial number of stream event cells */
extern int maxsepgcnt;			/* page limit for event cell allocation */
extern char qrunflag;			/* set if there are queues to run */
extern struct bclist	strbcalls;	/* list of waiting bufcalls */
extern unsigned int strbufsizes[];	/* bucket sizes for allocb stats */
extern unsigned int nstrbufsz;		/* number of entries in strbufsizes */

#define	pollwait	selwait		/* so select() and poll() will */
#define	SPOLL		SSEL		/*   work together */
extern int pollwait;			/* poll sleeps on &pollwait */

#endif	KERNEL

#endif /*!_sys_stream_h*/
