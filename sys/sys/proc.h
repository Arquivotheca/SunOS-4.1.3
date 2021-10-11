/*	@(#)proc.h 1.1 92/07/30 SMI; from UCB 4.24 83/07/01	*/

#ifndef	_sys_proc_h
#define	_sys_proc_h

#include <sys/user.h>

/*
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 */
struct	proc {
	struct	proc *p_link;	/* linked list of running processes */
	struct	proc *p_rlink;
	struct	proc *p_nxt;	/* linked list of allocated proc slots */
	struct	proc **p_prev;	/* also zombies, and free procs */
	struct	as *p_as;	/* address space description */
	struct seguser *p_segu;	/* "u" segment */
	/*
	 * The next 2 fields are derivable from p_segu, but are
	 * useful for fast access to these places.
	 * In the LWP future, there will be multiple p_stack's.
	 */
	caddr_t p_stack;	/* kernel stack top for this process */
	struct user *p_uarea;	/* u area for this process */
	char	p_usrpri;	/* user-priority based on p_cpu and p_nice */
	char	p_pri;		/* priority, negative is high */
	char	p_cpu;		/* (decayed) cpu usage solely for scheduling */
	char	p_stat;
	char	p_time;		/* seconds resident (for scheduling) */
	char	p_nice;		/* nice for cpu usage */
	char	p_slptime;	/* seconds since last block (sleep) */
	char	p_cursig;
	int	p_sig;		/* signals pending to this process */
	int	p_sigmask;	/* current signal mask */
	int	p_sigignore;	/* signals being ignored */
	int	p_sigcatch;	/* signals being caught by user */
	int	p_flag;
	uid_t	p_uid;		/* user id, used to direct tty signals */
	uid_t	p_suid;		/* saved (effective) user id from exec */
	gid_t	p_sgid;		/* saved (effective) group id from exec */
	short	p_pgrp;		/* name of process group leader */
	short	p_pid;		/* unique process id */
	short	p_ppid;		/* process id of parent */
	u_short	p_xstat;	/* Exit status for wait */
	short	p_cpticks;	/* ticks of cpu time, used for p_pctcpu */
	struct	ucred *p_cred;  /* Process credentials */
	struct	rusage *p_ru;	/* mbuf holding exit information */
	int	p_tsize;	/* size of text (clicks) */
	int	p_dsize;	/* size of data space (clicks) */
	int	p_ssize;	/* copy of stack size (clicks) */
	int 	p_rssize; 	/* current resident set size in clicks */
	int	p_maxrss;	/* copy of u.u_limit[MAXRSS] */
	int	p_swrss;	/* resident set size before last swap */
	caddr_t p_wchan;	/* event process is awaiting */
	long	p_pctcpu;	/* (decayed) %cpu for this process */
	struct	proc *p_pptr;	/* pointer to process structure of parent */
	struct	proc *p_cptr;	/* pointer to youngest living child */
	struct	proc *p_osptr;	/* pointer to older sibling processes */
	struct	proc *p_ysptr;	/* pointer to younger siblings */
	struct	proc *p_tptr;	/* pointer to process structure of tracer */
	struct	itimerval p_realtimer;
	struct	sess *p_sessp;	/* pointer to session info */
	struct	proc *p_pglnk;	/* list of pgrps in same hash bucket */
	short	p_idhash;	/* hashed based on p_pid for kill+exit+... */
	short	p_swlocks;	/* number of swap vnode locks held */
	struct aiodone *p_aio_forw; /* (front)list of completed asynch IO's */
	struct aiodone *p_aio_back; /* (rear)list of completed asynch IO's */
	int	p_aio_count;	/* number of pending asynch IO's */
	int	p_threadcnt;	/* ref count of number of threads using proc */
#ifdef	sun386
	struct	v86dat *p_v86;	/* pointer to v86 structure */
#endif	sun386
#ifdef	sparc
/*
 * Actually, these are only used for MULTIPROCESSOR
 * systems, but we want the proc structure to be the
 * same size on all 4.1.1psrA SPARC systems.
 */
	int	p_cpuid;	/* processor this process is running on */
	int	p_pam;		/* processor affinity mask */
#endif	sparc
};

#define	PGRPHSZ		64
#define	PGRPHASH(pgrp)	((pgrp) & (PIDHSZ - 1))
#define	PIDHSZ		64
#define	PIDHASH(pid)	((pid) & (PIDHSZ - 1))

/*
 * Get unique id for process or thread.
 * Useful in locking (RLOCK, ILOCK, etc.).
 */
#ifdef	KERNEL
extern struct proc *masterprocp;
#ifdef	LWP
extern struct lwp_t *__Curproc;
extern int runthreads;
#define	uniqpid()	(runthreads ? (long)__Curproc : (long)masterprocp)
#else	LWP
#define	uniqpid()	((long)masterprocp)
#endif	LWP

struct	proc *pgrphash[PGRPHSZ];
struct	proc *pgfind();
struct	proc *pgnext();
void	pgenter();
void	pgexit();
short	pidhash[PIDHSZ];
struct	proc *pfind();
struct	proc *proc, *procNPROC;	/* the proc table itself */
struct	 proc *freeproc, *zombproc, *allproc;
			/* lists of procs in various states */
int	nproc;

#define	NQS	32		/* 32 run queues */
struct	prochd {
	struct	proc *ph_link;	/* linked list of running processes */
	struct	proc *ph_rlink;
} qs[NQS];
int	whichqs;		/* bit mask summarizing non-empty qs's */
#endif	KERNEL

/* stat codes */
#define	SSLEEP	1		/* awaiting an event */
#define	SWAIT	2		/* (abandoned state) */
#define	SRUN	3		/* running */
#define	SIDL	4		/* intermediate state in process creation */
#define	SZOMB	5		/* has exited, waiting for parent to pick up status */
#define	SSTOP	6		/* stopped */

/* flag codes */
#define	SLOAD	0x0000001	/* in core */
#define	SSYS	0x0000002	/* swapper or pager process */
#define	SLOCK	0x0000004	/* process being swapped out */
#define	SSWAP	0x0000008	/* save area flag */
#define	STRC	0x0000010	/* process is being traced */
#define	SWTED	0x0000020	/* parent has been told that this process stopped */
#define	SULOCK	0x0000040	/* user settable lock in core */
#define	SPAGE	0x0000080	/* process in page wait state XXX: never set */
#define	SKEEP	0x0000100	/* another flag to prevent swap out */
#define	SOMASK	0x0000200	/* restore old mask after taking signal */
#define	SWEXIT	0x0000400	/* working on exiting */
#define	SPHYSIO	0x0000800	/* doing physical i/o (bio.c) */
#define	SVFORK	0x0001000	/* process resulted from vfork() */
#define	SVFDONE	0x0002000	/* another vfork flag */
#define	SNOVM	0x0004000	/* no vm, parent in a vfork() */
#define	SPAGI	0x0008000	/* init data space on demand, from inode */
#define	SSEQL	0x0010000	/* user warned of sequential vm behavior */
#define	SUANOM	0x0020000	/* user warned of random vm behavior */
#define	STIMO	0x0040000	/* timing out during sleep */
#define	SORPHAN	0x0080000	/* process is orphaned (can't be ^Z'ed) */
#define	STRACNG	0x0100000	/* process is tracing another process */
#define	SOWEUPC	0x0200000	/* owe process an addupc() call at next ast */
#define	SSEL	0x0400000	/* selecting; wakeup/waiting danger */
#define	SLOGIN	0x0800000	/* a login process (legit child of init) */
#define	SFAVORD	0x2000000	/* favored treatment in swapout and pageout */
#define	SLKDONE 0x4000000	/* record-locking has been done */
#define	STRCSYS 0x8000000	/* tracing system calls */
#define	SNOCLDSTOP\
		0x10000000	/* SIGCHLD not sent when child stops (posix) */
#define	SEXECED	0x20000000	/* this process has completed an exec  " */
#define	SRPC	0x40000000	/* Forgive me for this hack to overcome */
				/* sunview window locking problems */
#define	SSETSID	0x80000000	/* process has called setsid() directly */

#endif	/* !_sys_proc_h */
