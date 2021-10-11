/* @(#)klm_prot.x 1.1 92/07/30 Copyr 1987 Sun Micro */

/*
 * Kernel/lock manager protocol definition
 * Copyright (C) 1986 Sun Microsystems, Inc.
 *
 * protocol used between the UNIX kernel (the "client") and the
 * local lock manager.  The local lock manager is a deamon running
 * above the kernel.
 */
program KLM_PROG {
	version KLM_VERS {

		klm_testrply	KLM_TEST (struct klm_testargs) =	1;

		klm_stat	KLM_LOCK (struct klm_lockargs) =	2;

		klm_stat	KLM_CANCEL (struct klm_lockargs) =	3;
		/* klm_granted=> the cancel request fails due to lock is already granted */
		/* klm_denied=> the cancel request successfully aborts
lock request  */

		klm_stat	KLM_UNLOCK (struct klm_unlockargs) =	4;
		/*
		 * remote lock manager call-back to grant lock
		 */
		klm_stat	KLM_GRANTED(struct klm_lockargs) =	5;
	} = 1;
} = 100020;

const	LM_MAXSTRLEN = 1024;

/*
 * lock manager status returns
 */
enum klm_stats {
	klm_granted = 0,	/* lock is granted */
	klm_denied = 1,		/* lock is denied */
	klm_denied_nolocks = 2, /* no lock entry available */
	klm_working = 3, 	/* lock is being processed */
	klm_deadlck = 5		/* EINTR = 5,is used in krpc/klm_lockmgr.c */
};

/*
 * lock manager lock identifier
 */
struct klm_lock {
	string server_name<LM_MAXSTRLEN>;
	netobj fh;		/* a counted file handle */
	int base;		/* beginning offset of the lock */
	int length;		/* byte length of the lock;
				 * zero means through end of file */
	int type;
	int granted;
	int color;
	int LockID;
	int pid;                /* holder of the lock */
	int class;
	long rsys;
	long rpid;
};

/*
 * lock holder identifier
 */
struct klm_holder {
	bool exclusive;		/* FALSE if shared lock */
	int base;               /* beginning offset of the lock */
        int length;             /* byte length of the lock;
                                 * zero means through end of file */
	int type;
	int granted;
        int color;
        int LockID;
	int pid;                /* holder of the lock (pid) */
        int class;
	long rsys;
	long rpid;
};

/*
 * reply to KLM_LOCK / KLM_UNLOCK / KLM_CANCEL
 */
struct klm_stat {
	klm_stats stat;
};

/*
 * reply to a KLM_TEST call
 */
union klm_testrply switch (klm_stats stat) {
	case klm_denied:
		struct klm_holder holder;
	default: /* All other cases return no arguments */
		void;
};

/*
 * arguments to KLM_LOCK
 */
struct klm_lockargs {
	bool block;
	bool exclusive;
	struct klm_lock alock;
};

/*
 * arguments to KLM_TEST
 */
struct klm_testargs {
	bool exclusive;
	struct klm_lock alock;
};

/*
 * arguments to KLM_UNLOCK
 */
struct klm_unlockargs {
	struct klm_lock alock;
};
