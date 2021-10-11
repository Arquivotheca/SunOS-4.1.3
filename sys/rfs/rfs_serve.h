/*	@(#)rfs_serve.h 1.1 92/07/30 SMI	*/

/*
 * Server process include file 
 */

#ifndef _rfs_rfs_serve_h
#define _rfs_rfs_serve_h

/*
 * Server process structure. One allocated for each running server process.
 * This structure contains all per-process info specific to an RFS server 
 * process for the duration of one service request and for its lifetime. 
 * In AT&T code these fields are added to the proc table and user structure. 
 */
struct serv_proc {
	int 	s_op;		/* Current RFS request being serviced */
	int 	s_pid;		/* Process id of this server */
	int 	s_sysid;	/* Machine id of remote client being served */
	int 	s_epid;		/* Process id of remote client being served */
        struct  rcvd  	*s_rdp;	/* server msg arrived on this queue */
        struct  sndd  	*s_sdp;	/* Sd to send reply to client on */
	int	s_mntindx;	/* Mount index received from client */
        long 	s_fmode;	/* Client access modes to current served file */
	struct gdp *s_gdpp;	/* Virtual circuit for client being served */
	char 	*s_sleep;	/* Sleep/wakeup for arriving messages */
	struct proc 	*s_proc; 	/* Pointer to assoc. proc table entry */
	struct serv_proc *s_rlink;  	/* Linked list of server procs */
	long s_tlast;		/* Time last request was serviced (time.tv_sec) */
};

/* 
 * Structure for list of server processes
 */
struct serv_list {
	struct serv_proc *sl_head;	/* Head of list */
	int sl_count;			/* Number of procs on list */
};

/* 
 * Server process lists. A server process can only be on one of these lists
 * at a time.
 */
extern struct serv_list s_active;	/* Currently processing RFS call */
extern struct serv_list s_idle;		/* Waiting for work */

extern int maxserve;			/* Max number of servers allowed */
extern int minserve;			/* Min number of servers allowed */
extern int nservers;			/* Current number of servers */
extern int nzombies;			/* Current number of zomby servers */

extern struct serv_proc *serv_proc;	/* Array of server process structures */
extern struct serv_proc *get_proc();
extern struct serv_proc *sp_alloc();
extern void sp_free();
extern void add_proc();
extern void del_proc();


/*
 * Server locking stuff. The server must keep locking records so that it
 * can release locks on close and on loss of a connection which are held 
 * on behalf of remote clients. 
 */

/* 
 * Server locking record. These records are allocated when a lock 
 * request is made and freed when the associated file is closed or
 * a disconnect occurs. They are attached to the receive descriptor
 * for the file being locked. Only one record is allocated per client 
 * process and file, regardless of how many locks the client places 
 * on the file. The record is not freed by explicit RFS lock-release 
 * requests.
 */
struct rs_lock {
	struct rs_lock *lk_next; /* Linked list of locks */
	short lk_epid;		 /* Pid of remote client holding lock */
	short lk_sysid;		 /* Sysid (server's) for client machine */
	u_short lk_uid;		/* uid of locker */
	u_short lk_gid;		/* gid of locker */
};


/* lkflags */
#define LK_LOCKED	0x01
#define LK_WANT		0x02

/* Create unique 32 bit pseudo-pid which identifies the client to the
 * filesystem lockctl interface. This id must not conflict with local pids 
 * which use 0x00000000 - 0x0000ffff. Also it must uniquely identify all 
 * remote client processes. To do this, basically prepend a client machine 
 * id to the remote process id of the client process.
 */
#define RSL_PID(mntindx, epid)	\
	(((-(mntindx+1)) << 16) | ((epid) & 0x0ffff))

/*
 * Lock and unlock lock lists.
 * Use these to prevent races which could occur if two people want to
 * manipulate the same node list at the same time and sleep in the middle 
 * of the operation (e.g., because of kmem_alloc, or VOP_LOCKCTL calls).
 */
#define rsl_lock(ru) { \
	register s; \
	s=splrf(); \
	while ((ru)->ru_lkflags & LK_LOCKED) { \
		(ru)->ru_lkflags |= LK_WANT; \
		(void) sleep((caddr_t) &((ru)->ru_lkflags), PREMOTE); \
        } \
	(ru)->ru_lkflags |= LK_LOCKED; \
	(void) splx(s); \
}

#define rsl_unlock(ru) { \
	register s; \
	s=splrf(); \
	(ru)->ru_lkflags &= ~LK_LOCKED; \
	if ((ru)->ru_lkflags & LK_WANT) { \
		(ru)->ru_lkflags &= ~LK_WANT; \
                wakeup((caddr_t) &((ru)->ru_lkflags)); \
        } \
	(void) splx(s); \
}

extern void rsl_lockrelease();
extern void rsl_alloc();

#endif /*!_rfs_rfs_serve_h*/
