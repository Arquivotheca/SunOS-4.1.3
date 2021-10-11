/*	@(#)param.c 1.1 92/07/30 SMI; from UCB 6.1 83/07/29	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/clist.h>
#include <sys/mbuf.h>
#include <sys/stream.h>
#include <sys/kernel.h>
#include <ufs/inode.h>
#include <ufs/quota.h>
#include <specfs/fifo.h>

#if defined(IPCSEMAPHORE) || defined(IPCMESSAGE) || defined(IPCSHMEM)
#include <sys/map.h>
#include <sys/ipc.h>
#endif IPCSEMAPHORE || IPCMESSAGE || IPCSHMEM

#ifdef IPCSEMAPHORE
#include <sys/sem.h>
#endif IPCSEMAPHORE

#ifdef IPCMESSAGE
#include <sys/msg.h>
#endif IPCMESSAGE

#ifdef IPCSHMEM
#include <sys/shm.h>
#endif IPCSHMEM

#include <sys/asynch.h>

/*
MAXASYNCHIO - 
	This is per system limit and represents
	the maximum number of memory resources that
	are consumed to support "n" asynchronous i/o
	requests in progress.
								
perproc_maxasynchio -
	This is a per process limit and represents the
	the maximum number of memory resources that
	a single process may consume to support "n" asynchronous i/o
	requests in progress.

maxasynchio_cached -
	This represents how many memory resources can be cached
	at any one time. Essentially this allows the implementation
	to return memory resources it comsumed during peak activity.
	
	Thus maxasynchio provides a cap on the amount of resources consumed
	during peak activity. The later provides a cap of resources that
	the implementation may "own" i.e the resources will not be returned
	to the system. If memory is an issue, then maxasynchio_cached should be
	set to a value to service normal asynchio activity and should be less
	than maxasynchio
 */

int	maxasynchio_cached = 4;
int	perproc_maxasynchio = 50;

/*
 * System parameter formulae.
 *
 * This file is copied into each directory where we compile
 * the kernel; it should be modified there to suit local taste
 * if necessary.
 *
 * Compiled with -DMAXUSERS=xx
 */

#ifdef sparc
#define	HZ 100
#else
#define	HZ 50
#endif sparc
int	hz = HZ;
int	tick = 1000000 / HZ;
int	tickadj = 1000000 / HZ / 10;
#ifdef sun
#define	NPROC (10 + 16 * MAXUSERS)
#undef MAXUPRC
#define	MAXUPRC	(NPROC - 5)
#else
#define	NPROC (10 + 8 * MAXUSERS)
#endif
int	nproc = NPROC;
int	maxuprc = MAXUPRC;
int	ninode = (NPROC + 16 + MAXUSERS) + 64 + 12 * MAXUSERS;
int	ncsize = (NPROC + 16 + MAXUSERS) + 64;
#if defined(sun3x) || defined(sun4m)
/*
 * Minimum of sixteen page tables per user, plus
 * sixty-four for the kernel.
 */
int	minpts = 64 + 16 * MAXUSERS;
#endif
#include "win.h"
#if NWIN > 0
/* if using the window system, will need more open files */
int	nfile = 16 * (NPROC + 16 + MAXUSERS) / 5 + 64;
#else
int	nfile = 16 * (NPROC + 16 + MAXUSERS) / 10 + 64;
#endif
int	ncallout = 16 + NPROC;
int	nclist = 100 + 16 * MAXUSERS;
#ifdef QUOTA
int	ndquot = (MAXUSERS*NMOUNT)/4 + NPROC;
#else
int	ndquot = 0;
#endif

/*
 * Minimum amount of swap space.  If you have less than this you get a
 * warning at boot time.  (In 512-byte blocks).
 * Default is 256K bytes.
 */
#ifndef	SWAPWARN
#define	SWAPWARN	512
#endif
u_int	swapwarn = SWAPWARN;	/* ref'd in autoconf.c:swapconf() */

/*
 * These have to be allocated somewhere; allocating
 * them here forces loader errors if this file is omitted.
 */
struct	proc *proc, *procNPROC;
struct	file *file, *fileNFILE;
struct 	callout *callout;
struct	cblock *cfree;
#ifdef QUOTA
struct	dquot *dquot, *dquotNDQUOT;
#endif

/* initialize SystemV named-pipe (and pipe()) information structure */
struct fifoinfo fifoinfo = {
	FIFOBUF,
	FIFOMAX,
	FIFOBSZ,
	FIFOMNB
};

#ifdef IPCMESSAGE
/* initialize SystemV IPC information structures */
struct msginfo msginfo = {
	MSGMAP,
	MSGMAX,
	MSGMNB,
	MSGMNI,
	MSGSSZ,
	MSGTQL,
	MSGSEG,
};
#endif IPCMESSAGE

#ifdef IPCSEMAPHORE
struct seminfo seminfo = {
	SEMMAP,
	SEMMNI,
	SEMMNS,
	SEMMNU,
	SEMMSL,
	SEMOPM,
	SEMUME,
	SEMUSZ,
	SEMVMX,
	SEMAEM,
};
#endif IPCSEMAPHORE

#ifdef IPCSHMEM
struct shminfo shminfo = {
	SHMMAX,
	SHMMIN,
	SHMMNI,
	0,	/* (obsolete entry) */
	0,	/* (obsolete entry) */
};
#endif IPCSHMEM

/*
 * Stream data structures.
 */
#define	NMUXLINK	87
#define	NSTREVENT	256
#define	NSTRPUSH	9
#define	MAXSEPGCNT	1
#define	STRMSGSZ	4096
#define	STRCTLSZ	1024

/*
 * Number of stream heads to allocate:
 * initial amount and subsequent increment(s).
 */
#ifndef	STREAM_INIT
#define	STREAM_INIT	32
#endif	STREAM_INIT
#ifndef	STREAM_INCR
#define	STREAM_INCR	 8
#endif	STREAM_INCR
int	stream_init = STREAM_INIT;
int	stream_incr = STREAM_INCR;

/*
 * Number of streams queue pairs to allocate:
 * initial amount and subsequent increment(s).
 */
#ifndef	QUEUE_INIT
#define	QUEUE_INIT	100
#endif	QUEUE_INIT
#ifndef	QUEUE_INCR
#define	QUEUE_INCR	 10
#endif	QUEUE_INCR
int	queue_init = QUEUE_INIT;
int	queue_incr = QUEUE_INCR;

struct	linkblk linkblk[NMUXLINK];
struct	strevent strevent[NSTREVENT];
int	strmsgsz = STRMSGSZ;
int	strctlsz = STRCTLSZ;
int	nmuxlink = NMUXLINK;
int	nstrpush = NSTRPUSH;
int	nstrevent = NSTREVENT;
int	maxsepgcnt = MAXSEPGCNT;

/*
 * Streams buffer allocation statistics.
 *
 * This array is used only for keeping track of how many STREAMS buffers of
 * various sizes the system has allocated since booting.  It has no effect on
 * how STREAMS buffers are allocated.  The sizes listed below serve only to
 * define histogram buckets into which to place counts for the sizes supplied
 * in calls to allocb().  Programs such as netstat(8) examine this array as
 * part of preparing reports on buffer allocation statistics.
 *
 * The values below must be positive, be given in ascending order, and must be
 * terminated with a zero value.  There is no restriction on how many values
 * appear, although they are searched linearly, so that the system will run
 * more efficiently with fewer rather than more of them.
 */
uint	strbufsizes[] = {
	16, 32, 128, 512, 1024, 2048, 8192, 0
};

#ifdef	NFSCLIENT
#ifndef	UFS
/* We guess that since we don't have UFS defined we're diskless */
#define	MAXCLIENTS	9
u_int	nchtable = 3;		/* Target number of free client handles */
#else	UFS
/* We assume that having UFS defined means we're diskfull */
#define	MAXCLIENTS	5
u_int	nchtable = 2;		/* Target number of free client handles */
#endif	UFS
u_int	ndesauthtab = MAXCLIENTS;
u_int	nunixauthtab = MAXCLIENTS;
#endif	NFSCLIENT

/*
 * tcptli
 *
 * Struct tt_softc is declared here since the file nettli/tcp_tlivar.h
 * is part of an optional package which may not be installed by a user.
 */
#include "tcptli.h"
#if	NTCPTLI >0
#include <sys/socketvar.h>
#include <netinet/in.h>
struct tt_softc {
	/* The tt_unit & tt_unitnext fields aren't yet used. */
	struct tt_softc *tt_next;       /* link to next device instance */
	u_short         tt_unit;        /* instance number */
	u_short         tt_unitnext;    /* next unit # to be used on open */

	queue_t         *tt_rq;         /* cross-link to read queue */
	struct socket   *tt_so;         /* socket for this device instance */
	mblk_t          *tt_merror;     /* pre-allocated M_ERROR message */
	mblk_t          *tt_errack;     /* pre-allocated T_error_ack message */
	u_int           tt_state;       /* current state of the tli automaton */
	long            tt_seqnext;     /* next sequence number to assign */
	u_long          tt_flags;       /* see below */
	u_long          tt_event;       /* service event inidication */
	struct  proc    *tt_auxprocp;   /* Aux proc handle */
	struct  in_addr tt_laddr;       /* saved local address */
	u_short         tt_lport;       /* saved local port number */
}tt_softc[NTCPTLI];
int	tcptli_limit = NTCPTLI;
#endif	NTCPTLI >0

/*
 * timod
 */
#include "tim.h"
#if	NTIM >0
struct tim_tim {
	long	tim_flags;
	queue_t	*tim_rdq;
	mblk_t	*tim_iocsave;
};
struct tim_tim tim_tim[NTIM];
int tim_cnt = NTIM;
#endif	NTIM >0

/*
 * tirdwr
 */
#include "tirw.h"
#if	NTIRW >0
struct trw_trw {
	long	trw_flags;
	queue_t	*trw_rdq;
	mblk_t	*trw_mp;
};
struct trw_trw trw_trw[NTIRW];
int trw_cnt = NTIRW;
#endif	NTIM >0

#if	defined(sun4) || defined(sun4c)
#if	defined(NSWPMEG)
int	npmgrpssw = NSWPMEG;
#else	defined(NSWPMEG) 
int	npmgrpssw = 0;		/* startup will set an appropriate default */
#endif	defined(NSWPMEG) 
#endif	defined(sun4) || defined(sun4c)
