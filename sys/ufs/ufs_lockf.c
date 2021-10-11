#ifndef lint
static char sccsid[] = "@(#)ufs_lockf.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * File locking code that is shared between the Lock manager and the
 * kernel. Primary difference between the two are that the kernel uses
 * statically allocated arrays while the lock manager can allocate memory
 * using malloc.
 *
 * This file defines the external functions :
 * lock()		- Get a lock on a region of something.
 * unlock()		- Unlock a region of something
 * test_lock()		- Return a pointer to a blocking lock on this thing
 * dead_lock()		- Determine if a deadlock would occur.
 * kill_proc_locks()	- Remove all locks owned by a process.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/vfs.h>
#include <sys/vfs_stat.h>

#include <ufs/fs.h>
#include <ufs/inode.h>
#include <ufs/mount.h>
#include <ufs/fsdir.h>
#include "lockf.h"

#include <vm/hat.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>
#include <vm/rm.h>
#include <vm/swap.h>

#define LOCK_DEBUG

/* files included by <rpc/rpc.h> */
#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <nfs/nfs.h>
#include <nfs/nfs_clnt.h>
#include <nfs/rnode.h>

#include <krpc/lockmgr.h>
#include <rpcsvc/klm_prot.h>
#include <rpcsvc/nlm_prot.h>

static int	LockID = 0;	/* Monotonically increasing id */
#define	STATIC	static

/*
 * Sized to be MAXPROCESSes which actually gives us some spare slots
 * because as many processes (system types) won't ask for file locks
 */
#define	MAXPROCESS 135

static struct process_locks 	*proc_lists = NULL;
static struct process_locks	*proc_locks = NULL;

int				max_processes = 0; /* statistics keeper */

/*
 * The defines {GETLOCK, FREELOCK} and {GETREF, FREEREF} will allocate
 * file lock and lock reference structures.
 */

/* Cached free lists, reuse discarded structs if possible. */
static struct data_lock	*free_locks = NULL;
static struct lock_ref	*free_refs = NULL;

/* some statistics are kept to tune the size of the kernel arrays */
int			max_file_locks = 0;
int			max_lock_refs = 0;

#ifdef LOCK_DEBUG
void    	print_depend();
extern int	lock_debug_on;
#endif

/* to keep track of granted locks to be supplied to local lock mgr thru	*/
/* fcntl(F_RSETLKW, F_UNLCK) calls					*/
static struct grant_lock	grant_locks;
static int			num_grant_locks = 0;
static int			max_grant_space = MAX_GRANT_LOCKS;
static struct grant_lock	*grant_locks_p = &grant_locks;

/*
 * Function get_lock() this static will get the next available lock entry
 * and return a pointer to it. If there are no pointers available it returns
 * NULL. In the kernel it is optimized to use a statically allocated array
 * of entries, when freed in either the kernel or when compiled in the lock
 * manager it pushes free entries on to a linked list for faster access.
 */
STATIC struct data_lock *
get_lock()
{
	struct data_lock  	*l; /* Temp lock pointer */

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...get_lock()");
#endif
	if (free_locks) {
		l = free_locks;
		free_locks = l->Next;
		if (free_locks) free_locks->Prev = NULL;
		bzero((caddr_t) l, sizeof (*l));
		return (l);
	} else {
		l = (struct data_lock *)
		    new_kmem_zalloc(sizeof (struct data_lock), KMEM_SLEEP);
		bzero((caddr_t) l, sizeof (*l));
	}
	max_file_locks++; /* statistics information */
	return (l);
}

#define	GETLOCK(l)	(l) = get_lock()
#define	FREELOCK(l)  { \
	(l)->pid = 0; \
	(l)->Next = free_locks; \
	if (free_locks) free_locks->Prev = (l); \
	(l)->Prev = NULL; \
	(l)->NextProc = 0; \
	free_locks = (l); }

/*
 * Function get_ref() is identical to get lock except that is gets lock
 * reference entries.
 */
STATIC struct lock_ref *
get_ref()
{
	struct lock_ref  *r;

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...get_ref()");
#endif
	if (free_refs) {
		r = free_refs;
		free_refs = r->Next;
		if (free_refs) free_refs->Prev = NULL;
		bzero ((caddr_t) r, sizeof (*r));
		return (r);
	} else {
		r = (struct lock_ref *)
		    new_kmem_zalloc(sizeof (struct lock_ref), KMEM_SLEEP);
		bzero((caddr_t) r, sizeof (*r));
	}
	max_lock_refs++; /* statistics information */
	return (r);
}

#define	GETREF(r)	(r) = get_ref()
#define	FREEREF(r) { \
	(r)->Type = 0; \
	(r)->Next = free_refs; \
	if (free_refs) free_refs->Prev = (r); \
	(r)->Prev = NULL; \
	free_refs = (r); }

/*
 * Function add_ref() will add a blocking reference between two locks, this
 * consists of adding a blocked_by reference and is_blocking reference.
 */
STATIC int
add_ref(a, b)
	struct data_lock	*a;	/* The active lock */
	struct data_lock	*b;	/* The blocked lock. */
{
	struct lock_ref		*ref;
	struct lock_ref		*tref;

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("... ADD_REF()");
#endif
	/* Add a reference to the blocking chain... */
	GETREF(ref);
	if (!ref)
		return (1);
	ref->Type = BLOCKING;
	ref->ThisLock = a;
	ref->Conflict = b;
	ref->Next = NULL;

	if (a->Is_Blocking) {
		for (tref = a->Is_Blocking; tref->Next; tref = tref->Next)
			;
		tref->Next = ref;
		ref->Prev = tref;
	} else {
		a->Is_Blocking = ref;
		ref->Prev = NULL;
	}

	/* Add a reference to the Blocked_by chain ... */
	GETREF(ref);
	if (!ref)
		return (1);
	ref->Type	= BLOCKED;
	ref->ThisLock	= b;
	ref->Conflict	= a;
	ref->Next	= NULL;
	if (b->Blocked_By) {
		for (tref = b->Blocked_By; tref->Next; tref = tref->Next)
			;
		tref->Next = ref;
		ref->Prev = tref;
	} else {
		b->Blocked_By = ref;
		ref->Prev = NULL;
	}
	return (0);
}

/*
 * Function kill_ref() will delete the blocking and blocked_by references
 * for a given lockpair. It is passed the conflicting reference and gets
 * the needed data from that.
 */

STATIC void
kill_ref(r)
	struct lock_ref 	*r;	/* Lock reference */
{
	struct lock_ref		*oref;

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...kill_ref()");
#endif
	if (r == NULL)  /* panic city ... */
		panic("kill locking reference");

	/* Find the "Other" reference..." */
	if (r->Type == BLOCKING)	/* Comes from the "Is_Blocking" chain */
		for (oref = (r->Conflict)->Blocked_By;
		    (oref && (oref->Conflict != r->ThisLock));
		    oref = oref->Next);
	else				/* Comes from the "Blocked_By" chain */
		for (oref = (r->Conflict)->Is_Blocking;
		    (oref && (oref->Conflict != r->ThisLock));
		    oref = oref->Next);

	if (oref == NULL)  /* panic city ... */
		panic("corrupt file lock list");

	/* Now remove the two references from their respective chains */
	if (r->Next != NULL)
		(r->Next)->Prev = r->Prev;
	if (r->Prev != NULL)
		(r->Prev)->Next = r->Next;
	else if (r->Type == BLOCKING)
		(r->ThisLock)->Is_Blocking = r->Next;
	else
		(r->ThisLock)->Blocked_By = r->Next;

	if (oref->Next != NULL)
		(oref->Next)->Prev = oref->Prev;
	if (oref->Prev != NULL)
		(oref->Prev)->Next = oref->Next;
	else if (oref->Type == BLOCKING)
		(oref->ThisLock)->Is_Blocking = oref->Next;
	else
		(oref->ThisLock)->Blocked_By = oref->Next;

	FREEREF(r);	/* Free the two reference pointers */
	FREEREF(oref);

	return;
}

/*
 * Function kill_all_refs()
 *
 * This functions will kill all the references attached to this lock.
 */
STATIC void
kill_all_refs(l)
	struct data_lock	*l;
{
	struct lock_ref		*ThisRef; /* Some temporary pointers */
	struct lock_ref		*NextRef;

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...kill_all_refs()");
#endif
	/* Remove Blocked_By references ... */
	ThisRef = l->Blocked_By;
	while (ThisRef != NULL) {
		NextRef = ThisRef->Next;
		/* This can cause a panic if the lock list is bad */
		kill_ref(ThisRef);
		ThisRef = NextRef;
	}
	l->Blocked_By = NULL;

	/* And is_blocking refrences */
	ThisRef = l->Is_Blocking;
	while (ThisRef != NULL) {
		NextRef = ThisRef->Next;
		/* This can cause a panic if the lock list is bad */
		kill_ref(ThisRef);
		ThisRef = NextRef;
	}
	l->Is_Blocking = NULL;
}

/*
 * make_ref(l)
 *
 * This function will build all the blocking references for this lock. And
 * the given list of locks, it builds both the Blocking and Is_Blocked
 * chains.
 *
 * And although it allows for processes in the future that can have multiple
 * pending locks it should not encounter any in the current kernel.
 */
STATIC int
make_ref(t, list)
	struct data_lock	*t;	/* Lock to make references for */
	struct data_lock	*list;	/* Pending lock list */
{
	struct data_lock	*a;	/* Temp pointer used to walk the list */

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...make_ref()");
#endif

	for (a = list; a; a = a->Next) {
		if ((a->LockID == t->LockID) || SAMEOWNER(a, t))
			continue; /* Don't compare to our own locks */

		/* They overlap in some fashion */
#ifdef LOCK_DEBUG
		if (lock_debug_on) printf("MAKE_REF : END(t)=%d TOUCHING(t, a) = %d\n",
			END(t), TOUCHING(t, a));
#endif
		if (TOUCHING(t, a) && ((t->type & EXCLUSIVE) || (a->type & EXCLUSIVE)) && add_ref(t, a))
			return (1); /* Add ref failed (unlikely) */
	} /* For all locks in the list ... */
	return (0);
}

/*
 * test_dead_lock(lock, mylock)
 *
 * This function recursively scans the dependency tree to identify deadlocks.
 * Currently it is not depth limited, although that may be required for
 * minimizing stack requirements.
 */
STATIC int
test_dead_lock(l, x)
	struct data_lock	*l;	/* Pointer to the blocking lock */
	struct data_lock	*x;	/* Pointer to lock we want	*/
{
	struct lock_ref *r;	/* Temp holds the current reference */

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...test_dead_lock()");
#endif
		for (r = ((l->MyProc)->lox)->Blocked_By; r; r = r->Next) {

			/* The deadlock condition, waiting on a lock we own. */
			if (SAMEOWNER(r->Conflict, x))
				return (1);

			/* This lock already checked */
			if ((r->Conflict)->color == x->LockID)
				return (0);

			/* mark it as checked (colored search) */
			(r->Conflict)->color = x->LockID;

			/* Now check for deadlocks on the conflict */
			if (test_dead_lock(r->Conflict, x))
				return (1);
		}

	return (0);
}

/*
 * can_lock(list, lock)
 *
 * This function implements the locking policy for the locking code.
 * The policies are :
 * 	An exclusive lock blocks any other lock.
 *	A Shared lock blocks an Exclusive lock but not another shared lock.
 *	A Remote lock does *not* block an I/O (MFRL) lock.
 * If the lock in question can be granted a 0 is returned.
 * In a lock, the owner is uniquely identified by the triple
 *	(pid, r_pid, r_sys)
 */
int
can_lock(list, l)
	struct data_lock	*list;	/* Active list to use */
	struct data_lock	*l;	/* Lock to check */
{
	struct data_lock *a;	/* Temp pointer to walk list with */

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...can_lock()");
#endif

	/* Check to see if it is blocked by an active lock... */
	for (a = list; a; a = a->Next) {
#ifdef LOCK_DEBUG
		if (lock_debug_on) {
			printf("CAN_LOCK : l->pid=%d a->pid = %d l->base=%d ",
				l->pid, a->pid, l->base);
			printf("a->base=%d END(l)=%d END(a)=%d TOUCHING(a, l)=%d\n",
				a->base, END(l), END(a), TOUCHING(a, l));
		}
#endif
		if (a->LockID == l->LockID) {
#ifdef LOCK_DEBUG
			if (lock_debug_on) printf("proc %d: Attempting to relock same lock? \n",
				l->pid);
#endif
			return (1); /* Already Active ! */
		}
		if (!TOUCHING(a, l) ||
		    ((l->class == IO_LOCK) && (a->class == LOCKMGR)))
			continue; /* Accellerator */
		if (!SAMEOWNER(a, l))
			return (1); /* Blocked by 'a' */
	} /* For all locks on list ... */
	return (0);
}

/*
 * Function get_proc_list(p)
 *
 * This function will use one of the process lock list slots and assign
 * it to this process.
 * XXX - enhance performance with free_procs list pointer ?
 */
struct process_locks *
get_proc_list(p)
	long	p;	/* Process ID to use */
{
	struct process_locks *proclist;

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...get_proc_list()");
#endif

	/*
	 * Reuse freed process lock list slot.
	 */
	proclist = proc_lists;

	if (proc_lists)
		proc_lists = proc_lists->next;
	else
		proclist = (struct process_locks *) new_kmem_zalloc(sizeof (*proclist), KMEM_SLEEP);
	max_processes++;
	proclist->pid = p;
	proclist->next = proc_locks;
	proc_locks = proclist;
	proclist->lox = NULL;
	return (proclist);
}

STATIC void
free_proc_list(list)
	struct process_locks *list;
{
	struct process_locks *cur, *prev;

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...free_proc_list()");
#endif

	for (cur = proc_locks, prev = NULL;
	    cur != NULL && cur != list;
	    prev = cur, cur = cur->next)
		;

	if (cur == NULL) {
		panic("free_proc_list");
	}

	/*
	 * Collect freed process locks.  We don't actually free them but keep
	 * them in the free list for later use.
	 */
	if (prev)
		prev->next = cur->next;
	else
		proc_locks = cur->next;
	cur->next = proc_lists;
	proc_lists = cur;
	cur->pid= 0;
#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...free_proc_list()..set pid=0");
#endif
}

/*
 * Function add_proc_lock(l)
 *
 * Add's the passed lock to the list of locks owned by this lock's process.
 * should probably be a macro.
 */
STATIC void
add_proc_lock(l)
	struct data_lock	*l;
{
#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...add_proc_lock()");
#endif
	l->NextProc = (l->MyProc)->lox;
	(l->MyProc)->lox = l;
}

/*
 * Function rem_proc_lock(l)
 *
 * The slightly more complicated brother of add above it removes the
 * given lock from the process' lock list.
 */
STATIC void
rem_proc_lock(l)
	struct data_lock *l;
{
	struct data_lock 	*t; /* Temps, Thislock and previous lock */
	struct data_lock	*p;

#ifdef LOCK_DEBUG
	if (lock_debug_on) {
		printf("...rem_proc_lock()");
		printf("l->pid %d l->base %d l->length %d l->type %d\n",
			l->pid, l->base, l->length, l->type);
	}
#endif
	if (l->MyProc == NULL)
		return;

	/* Find the lock */
	for (t = l->MyProc->lox, p = NULL; t != NULL;
		p = t, t = t->NextProc) {
#ifdef LOCK_DEBUG
		if (lock_debug_on) printf("t->pid %d t->base %d t->length %d t->type %d\n",
		    t->pid, t->base, t->length, t->type);
#endif
		/* Update the list */
		if (t == l) {
			if (p)
				p->NextProc = t->NextProc;
			else {
				(l->MyProc)->lox = l->NextProc;
				if (!(l->NextProc)) {
					/*
				 	* This is the last lock this process was
			 		* holding so free list pointer
				 	*/
					free_proc_list(l->MyProc);
					l->MyProc= NULL;
				}
			}
			return;
		}
	}
}

/*
 * Function add_lock(list, a, t)
 *
 * This function inserts the lock on the list pointed to by list...
 * The value in a determines it's behaviour, if a == NULL then the
 * lock gets added to the end of the list, if a != NULL then the
 * lock gets inserted on the list in front of a. if (a == *list) then
 * this inserts the lock at the head of the list.
 */
void
add_lock(list, t, a)
	struct data_lock	**list;	/* The Active list to use */
	struct data_lock	*t;	/* The lock to add */
	struct data_lock	*a;	/* The position in the list */
{
	/* Add lock into the list lock chain ... */
#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...add_lock()");
#endif
	t->Next = NULL; /* Default */
	if (a) {			/* Place in lock chain ... */
		if (a->Prev != NULL) {
			(a->Prev)->Next = t;
			t->Prev = a->Prev;
		} else {
			*list = t;
			t->Prev = NULL;
		}
		a->Prev = t;
		t->Next = a;
	} else if (*list == NULL) { 	/* No locks on list */
			*list = t;
			t->Prev = NULL;
	} else {			/* New last lock on list */
		for (a = *list; a->Next; a = a->Next)
			;
		t->Prev = a;
		a->Next = t;
	}
}

/*
 * Function rem_lock(list, l)
 *
 * This function will remove the indicated lock from the list by
 * fixing up the pointers of the lock ahead and behind it.
 */
void
rem_lock(list, l)
	struct data_lock	**list; /* The list pointer pointer */
	struct data_lock	*l;	/* The lock */
{
#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...rem_lock()");
#endif
	if (l->Next != NULL)
		(l->Next)->Prev = l->Prev;
	if (l->Prev != NULL)
		(l->Prev)->Next = l->Next;
	else
		*list = l->Next;
	l->Prev= l->Next= NULL;		/* re-init the next and prev pointer */
}

/*
 * Function merge_lock(list, plist, l)
 *
 * This function merges the lock 'l' into the list. The validity of this
 * operation has already been checked by the higher levels of the locking
 * code. The merge operation can cause one or more locks to become coalesced
 * into one lock.
 *
 * This function handles all three cases :
 *	The lock is completely within an existing lock (easy)
 *	The lock is touching one or more other locks (coalesce)
 *	The lock is a completely new lock. (not too tough)
 */
int
merge_lock(list, plist, l)
	struct data_lock	**list; /* List to add it too   */
	struct data_lock	*plist; /* Pending lock list    */
	struct data_lock	*l;	/* The lock in question */
{
	struct data_lock	*a;	/* Temporary pointer */
	struct data_lock	*n; 	/* Temporary pointer */
	struct data_lock	*t; 	/* Temporary pointer */
	struct data_lock	*spanlist; /* Temporary list of spanned locks */
	int			e;	/* Temp for error information */

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...merge_lock()");
#endif
	for (a = *list, spanlist = NULL; a; a = n) {
		n = a->Next; /* Cache the next pointer */

		/* Case one, it is completely within another lock */
		if (WITHIN(l, a) && SAMEOWNER(l, a) &&
		    (l->class == a->class)) {
			/* 
			 * we signify here that the lock is granted 
			 * and there is a lock exists already
			 * eg. repeating locking request
			 */
			l->granted= -1;
			return (0); /* merge successful */
		}

		/* Case two, it completely surrounds one or more locks */
#ifdef LOCK_DEBUG
		if (lock_debug_on) printf("MERGE_LOCK : END(l)=%d TOUCHING(l, a)=%d\n",
			END(l), TOUCHING(l, a));
#endif
		if ((TOUCHING(l, a) || ADJACENT(l, a)) &&
		    SAMEOWNER(l, a) && (l->class == a->class)) {
			/* remove this lock from this list */
			rem_lock(list, a);
			/* Add it to the spanned list */
			add_lock(&spanlist, a, (struct data_lock *)NULL);
		}
	}

	e = 0; /* errors */
	/*
	 * On errors, the lock() code will clean up the lock.
	 * Case two continued, merge the spanned locks.  Expand l
	 * to include the furthest "left" extent of all locks on
	 * the span list -- and to include the furthest "right" extent as
	 * well.  Remove all of the spanned locks.
	 */
	if (spanlist) {
		for (n = spanlist; n; n = t) {
			t = n->Next;
			if (n->base < l->base) {
				if (END(l) >= 0)
					l->length += l->base - n->base;
				l->base = n->base;
			}
			if (END(l) >= 0 && END(n) >= 0)
				l->length = (END(l) < END(n) ? END(n) : END(l))
				    + 1 - l->base;
			else
				l->length = LOCK_TO_EOF;
			kill_all_refs(n);
			rem_lock(&spanlist, n);
			rem_proc_lock(n);
			FREELOCK(n);
		}

		/* Lock now spans the range of all previous locks. */
		e = make_ref(l, plist);	/* Build a new blocking reference */
		if (!e) 		/* Add it in where we left off above */
			add_lock(list, l, a);
		return (e);
	}
	/* Case three, its a new lock with no siblings */
	e = make_ref(l, plist); /* build it's reference list */
	if (!e)
		add_lock(list, l, a);
	return (e);
}

/*
 * Function lock_region()
 *
 * This function will attempt the lock the selected region on the
 * appropriate list. If the lock is granted it returns true otherwise
 * it returns false and the calling program can deal with it.
 */
int
lock_region(lox, l)
	struct lock_list	*lox; 	/* Region the lock refers too */
	struct data_lock 	*l;	/* The lock to make on this region */
{
#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...lock_region()");
#endif
	if (((l->type & EXCLUSIVE) != 0) &&
	    !can_lock(lox->exclusive, l) && !can_lock(lox->shared, l)) {
		l->granted = 1;
		return (merge_lock(&(lox->exclusive), lox->pending, l));
	} else if (((l->type & EXCLUSIVE) == 0) &&
		    (!can_lock(lox->exclusive, l))) {
		l->granted = 1;
		return (merge_lock(&(lox->shared), lox->pending, l));
	}
	return (1);
}



/*
 * Function unlock_region()
 *
 * This function will unlock the requested region from the specified
 * lock chain. This function can have one of three effects :
 *	(1) - Nothing, no lock existed so it succeeds by default
 *	(2) - Multiple locks are removed as the unlocked region spans
 *		several locks.
 *	(3) - A new lock is created as the unlock region creates a 'hole'
 *		in an existing lock. (can fail if no more lock entries!)
 */
int
unlock_region(l, list, pending)
	struct data_lock	*l;	/* The region to unlock */
	struct data_lock	**list; /* The list we are unlocking from */
	struct data_lock	*pending; /* Locks that are waiting */
{
	struct data_lock	*t;	/* Some temporaries */
	struct data_lock	*n;
	struct data_lock	*x;

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...unlock_region()");
#endif
	for (t = *list; t; t = n) {
		n = t->Next;
#ifdef LOCK_DEBUG
		if (lock_debug_on) {
			printf("END(t)=%d END(l)=%d t->base=%d l->base=%d ",
				END(t), END(l), t->base, l->base);
			printf("t->pid=%d t->rpid=%d t->rsys=%x l->pid=%d l->rpid=%d ",
				t->pid, t->rpid, t->rsys, l->pid, l->rpid);
			printf("l->rsys=%x\n",
				l->rsys);
		}
#endif
		if (SAMEOWNER(t, l) && (t->class == l->class) &&
		    TOUCHING(l, t)) {

			if (WITHIN(t, l)) {
out:				/* Remove blocking referencese */
				kill_all_refs(t);
				rem_lock(list, t);
				rem_proc_lock(t);
				FREELOCK(t);
			/* case one region 'l' is to the left of 't' */
			} else if (l->base <= t->base) {
				kill_all_refs(t); /* Remove it's references */
				if (END(t) >= 0 && END(l) >= 0) {
					if (END(l) >= END(t))
						goto out;
					else {
						t->length = END(t) - END(l);
						t->base = END(l) + 1;
					}
				} else if (END(t) < 0 && END(l) >= 0) {
					t->length = LOCK_TO_EOF;
					t->base = END(l) + 1;
				} else if ((END(t) >= 0 && END(l) < 0) ||
					(END(t) < 0 && END(l) < 0)) {
					goto out;
				}
				(void) make_ref(t, pending);
			/* Case two new lock is to the right of 't' */
			} else if (((END(l) >= 0) && (END(t) >= 0) &&
				(END(l) >= END(t))) || (END(l) < 0)) {
				kill_all_refs(t); /* Remove it's references */
				t->length = l->base - t->base;
				(void) make_ref(t, pending);
			/* Case three, new lock in in the middle of 't' */
			} else {
				/*
				 * This part can fail if there isn't another
				 * lock entry and 'n' lock_refs available to
				 * build it's dependencies.
				 */
				GETLOCK(x);
				if (!x)
					return (1);
				kill_all_refs(t);
				/* Structure assignment! */
				*x = *t; 		/* Copy the original */
				x->LockID = LockID++;	/* give it a fresh id */
				t->length = l->base - t->base;
				if (END(x) >= 0 && END(l) >= 0) {
					x->length = END(x) - END(l);
					x->base = END(l) + 1;
				} else if (END(x) < 0 && END(l) >= 0) {
					x->length = LOCK_TO_EOF;
					x->base = END(l) + 1;
				}
				/* make 'X' officially part of the chain */
				x->Prev = t;
				if (t->Next != NULL)
					t->Next->Prev = x;
				t->Next = x;
				t->NextProc = x;
				/* Can't fail, we freed enough of them above */
				(void) make_ref(t, pending);
				if (make_ref(x, pending)) {
					/* Emergency abort should be panic ? */
					t->Next = x->Next;
					t->NextProc = x->NextProc;
					rem_lock(list, x);
					kill_all_refs(x);
					rem_proc_lock(x);
					FREELOCK(x);
					return (1);
				}
			}
		}
	}
	return (0);
}

/*
 * Function unlock()
 *
 * This function unlocks the specified region. This may cause the
 * creation of additional lock structures if the unlock operation
 * splits one lock into two smaller locks.
 */
int
unlock(vp, ld, cmd, p, class)
	struct vnode	*vp;	/* the vnode we are using */
	struct eflock	*ld; 	/* the flock structure */
	int		cmd;	/* The command sent (F_SETLK{W}) */
	int		p; 	/* Process ID */
	int		class;	/* Lock class (NONE, IO, REMOTE) */
{
	struct data_lock	a;	/* The region we want to unlock */
	struct inode		*ip;	/* The inode of this vnode */
	struct lock_list 	*lox;	/* list of locks we're holding */
	int			e;	/* An error variable */
	int			iunlock_flag;

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("\n unlock(pid=%d, l_pid=%d rpid=%d base=%d, len=%d)\n[",
			p, ld->l_pid, ld->l_rpid, ld->l_start, ld->l_len);
#endif

	/*
	 * l_xxx is set to >0 if a previous unlock by lockmgr results in
	 * granting other lock mgr results.  lockmgr will call fcntl()
	 * repeatedly to get the info of the granted locks
	 * when the last granted lock is reported to lockmgr, l_xxx is 
	 * set to <0
	 */
	if ((class == LOCKMGR) && (ld->l_xxx  == GRANT_LOCK_FLAG)) {
		return (do_grant_locks(vp, ld));
	}

	ip = VTOI(vp);
	ILOCK(ip);	/* Lock down the inode */
	iunlock_flag = FALSE;

	lox = (struct lock_list *)(vp->v_filocks);
	e = 0;
	if (!lox) {
#ifdef LOCK_DEBUG
		if (lock_debug_on) printf("] no locks.\n");
#endif
		IUNLOCK(ip);
		return (0); /* No locks so we must succeed */
	}

	/* Initialize lock structure */
	a.base   = (ld->l_start >= 0) ? ld->l_start : 0;
	a.length = (ld->l_len == 0) ? LOCK_TO_EOF : ld->l_len;
	a.pid    = p;
	a.type   = cmd;
	a.class  = class;
	if (class == LOCKMGR) {
		a.pid = ld->l_pid;
		a.rpid = ld->l_rpid;
		a.rsys = ld->l_rsys;
	} else {
		ld->l_pid = p;
		a.rpid = 0L;
		a.rsys = 0L;
	}

	/* First unlock it on the exclusive list */
	if (unlock_region(&a, &(lox->exclusive), lox->pending) ||
	    unlock_region(&a, &(lox->shared), lox->pending)) {
		e = ENOLCK;
	}

	/* Take it off the pending list 			   */
	/* this is needed when a remote blocking lock is cancelled */
	/* local locking case is handled in the lock() routine	   */
	(void) unlock_region(&a, &(lox->pending), lox->pending);

	/* Now see if any of the pending locks can be granted. */
	e = chk_granted(vp, ld, ip, class, &iunlock_flag);

	if (!iunlock_flag)
		IUNLOCK(ip);
	/* XXX make handling this pointer easier ... */
	if (!(lox->exclusive) && !(lox->shared) && !(lox->pending)) {
		if (vp->v_filocks != NULL) {
			kmem_free((caddr_t)vp->v_filocks, 
				sizeof (struct lock_list));
			vp->v_filocks = NULL;
			VN_RELE(vp);	/* release the vp hold */
		}
	}
#ifdef LOCK_DEBUG
	if (lock_debug_on) {
		printf("]\n----------------unlock START \n");
		print_depend(vp);
		printf("----------------unlock END \n");
	}
#endif

	return (e); /* return any error status if there is any */
}


/*
 * Function lock()
 *
 * This function will attempt to aquire a lock on the requested region of
 * the vnode. If it is successful it returns 0, if it encounters an error
 * it returns the error code.
 *
 * Mandatory file and record locking uses this code too, a read or write
 * will first attempt an exclusive or shared lock of the class "I/O LOCK"
 * When it has been granted the I/O will proceed, following the I/O the
 * lock will unlocked by a call to unlock.
 *
 * Proxy locks are locks on a file on the server, that are held by a process
 * on the server for a process on a client.
 */
#ifdef LOCK_DEBUG
static char *lckclass[3] = {" FILE", " I/O ", "PROXY"};
#endif

int
lock(vp, ld, cmd, p, class)
	struct vnode	*vp;	/* 'vnode' that gets locked 		*/
	struct eflock  *ld;	/* Area of interest 			*/
	int		cmd;	/* Lock command 			*/
	int		p;	/* Process ID of proc wanting the lock 	*/
	int		class;	/* Lock class (FILE_LOCK, IO_LOCK, LOCKMGR) */
{
	struct data_lock	*lck; 	/* New lock pointer */
	struct data_lock	*t;	/* Temporary */
	struct lock_list	*lox;	/* Region lock lists */
	struct lock_ref		*r;	/* Temp pointer to a reference */
	struct inode		*ip;	/* Inode pointer 	*/
	struct process_locks	*pl;	/* List of locks owned by our proc */
	int			deadlock; /* Remember if we saw a deadlock */
	int			Lockerror; /* Carry the lock error forward  */
	int			e;	/* An error variable */
	int			iunlock_flag;

	Lockerror = 0;
#ifdef LOCK_DEBUG
	if (lock_debug_on) {
		printf("lock(whence=%d pid=%d, l_pid=%d rpid=%d base=%d, ",
			ld->l_whence, p, ld->l_pid, ld->l_rpid, ld->l_start);
		printf("len=%d, type=%s%c, class=%s)\n[",
	    	ld->l_len, ((ld->l_type == F_WRLCK) ? "EXCL" : "SHRD"),
	    	((cmd == F_SETLK) ? ' ' : 'W'), lckclass[class]);
	}
#endif
	if ((class == LOCKMGR) && (ld->l_xxx  == GRANT_LOCK_FLAG)) {
		return (do_grant_locks(vp, ld));
	}

	iunlock_flag = FALSE;

	GETLOCK(lck);
	if (!lck)
		return (ENOLCK);

	/* Lock structure initialization */
	bzero((caddr_t)lck, sizeof (struct data_lock));
	lck->type = ((ld->l_type == F_RDLCK) ? 0 : EXCLUSIVE);
	lck->length = (ld->l_len == 0) ? LOCK_TO_EOF : ld->l_len;
	lck->base = (ld->l_start >= 0) ? ld->l_start : 0;
	lck->pid = p;
	lck->class = class;
	lck->LockID = LockID++;
	lck->vp	= vp;
	if (lck->class == LOCKMGR) {
		lck->pid = ld->l_pid;
		lck->rpid = ld->l_rpid;	/* Proxy values */
		lck->rsys = ld->l_rsys;
	} else {
		ld->l_pid = lck->pid;
		lck->rpid = 0L;
		lck->rsys = 0L;
	}

	/*
	 * Get it's process' lock list XXX move to proc structure.
	 * For remote cases, server lock manager is the real to call
	 * for the lock.  So, the pid in proc lock would not represent
	 * the right client's process.  Use clients' pid and system id
	 * in datalock stucture to find the right process lock for the
	 * requested lock.
	 */
	for (pl = proc_locks; (pl && !SAMEOWNER(pl->lox,lck)); pl = pl->next)
		;

	if (pl)
		lck->MyProc = pl;
	else
		lck->MyProc = get_proc_list((long)p);

	/* chk for get_proc_list returning NULL */
	if (!lck->MyProc)
		return(ENOLCK);

	add_proc_lock(lck);

	/* The inode is used as a synchronization point while locking */
	ip = VTOI(vp);
	ILOCK(ip);

	/*
	 * XXX this is pretty gross in the kernel, make it work then
	 * make it fast
	 */
	if (! vp->v_filocks) {
		VN_HOLD(vp);	/* hold the vnode down until the */
				/* lock is released */
		vp->v_filocks = (long *)new_kmem_zalloc(
				sizeof (struct lock_list), KMEM_SLEEP);
	}
	lox = (struct lock_list *)(vp->v_filocks);
	if (!lock_region(lox, lck)) {
		e = 0;
		/* This unlock will complete the upgrade of any shared locks */
		if (lck->granted == -1) {
			kill_all_refs(lck);
			rem_proc_lock(lck);
			FREELOCK(lck);	/* Give back the lock */
		} else if ((lck->type & EXCLUSIVE) == 0) {
			(void)unlock_region(lck, &lox->exclusive, lox->pending);
			(void)unlock_region(lck, &(lox->pending), lox->pending);
			/* Now see if any of the pending locks can be granted. */
			e = chk_granted(vp, ld, ip, class, &iunlock_flag);
		} else if ((lck->type & EXCLUSIVE) != 0)
                        (void)unlock_region(lck, &lox->shared, lox->pending);
		if (!iunlock_flag)
			IUNLOCK(ip);
#ifdef LOCK_DEBUG
		if (lock_debug_on) {
			printf("\n]\n------------------lock START \n");
			print_depend(vp);
			printf("------------------lock END \n");
		}
#endif
		return (e);
	} else if (cmd == F_SETLKW || cmd == F_RSETLKW) {

		/*
		 * it's blocked and they want to wait for it ... so we
		 * build up the dependency lists and put it on the pending
		 * list ...
		 */
		for (t = lox->exclusive; t; t = t->Next) {
			/* check if the nlm lock request is already queued */
			if (lck->class == LOCKMGR &&
				SAMEOWNER(lck, t) && SAMELOCK(lck, t)) {
				IUNLOCK(ip);
				return(EINTR);
			}
		}

		for (t = lox->exclusive; t; t = t->Next) {
#ifdef LOCK_DEBUG
			if (lock_debug_on) {
				print_depend(vp);
				printf("LOCK EX : END(lck)=%d TOUCHING(lck, t)=%d ",
					END(lck), TOUCHING(lck, t));
				printf("!SAMEOWNER(lck, t)=%d\n", !SAMEOWNER(lck, t));
				print_depend(vp);
			}
#endif

			if (TOUCHING(lck, t) &&
			    !SAMEOWNER(lck, t) && add_ref(t, lck))
				Lockerror = ENOLCK;
		}
		/* If it's exclusive it's blocked by shared locks too */
		if ((lck->type & EXCLUSIVE) != 0)
			for (t = lox->shared; t; t = t->Next) {
#ifdef LOCK_DEBUG
				if (lock_debug_on) {
					printf("LOCK SHARED : END(lck)=%d ", END(lck));
					printf("TOUCHING(lck, t)=%d\n",
						TOUCHING(lck, t));
				}
#endif
				if (TOUCHING(lck, t) &&
				    !SAMEOWNER(lck, t) && add_ref(t, lck))
					Lockerror = ENOLCK;
			}

#ifdef LOCK_DEBUG
		if (lock_debug_on) print_depend(vp);
#endif
		/* If we can't build the dependency list we exit with no lock */
		if (Lockerror) {
			IUNLOCK(ip);
			kill_all_refs(lck);
			rem_proc_lock(lck);
			FREELOCK(lck);
			return (Lockerror);
		}

		/*
		 * Deadlock detection :
		 *   Check each lock blocking this one to see if it is
		 * blocked by a lock that is held by the current process.
		 * If so we are about to deadlock so we don't do the sleep.
		 *
		 * For each lock that this is blocked by, we check to see
		 * if the owner of the lock blocking us is waiting on any
		 * locks. If so each lock blocking that lock is checked
		 * to see if any of them are blocked by one of our locks.
		 * This is done recursively by the test_dead_lock() routine.
		 * Each lock that is checked is 'colored' with the current
		 * LockID so  that we won't bother checking the same lock
		 * twice, nor will we loop around in the dependency tree.
		 * If a deadlock is detected, this call will abort with
		 * the EDEADLCK error, if no deadlock is detected then the
		 * process goes to sleep.
		 */
		deadlock = 0;
		for (r = lck->Blocked_By; r && !deadlock; r = r->Next) {
			t = r->Conflict; /* Get the conflicting lock */
			deadlock = test_dead_lock(t, lck);
		}
		if (! deadlock) {
			/* Add it to the pending list */
			add_lock(&(lox->pending), lck,
				(struct data_lock *)NULL);
			IUNLOCK(ip);

			/*
			 * This is kind of hokey, the lock manager really
			 * doesn't want to sleep on a blocking lock it really
			 * just wants to get notified, we do that in
			 * unlock_region() so here we return EINTR which the
			 * the lock manager takes to mean, "You didn't get
			 * the lock but I'll let you know when you get it."
			 */
			if (lck->class == LOCKMGR)
				return (EINTR);
#ifdef LOCK_DEBUG
			if (lock_debug_on) printf("Proc %d : sleeping on lock.\n", lck->pid);
#endif
			if (sleep((caddr_t) lck, PFLCK+PCATCH) &&
				!(lck->granted)) {
				Lockerror = EINTR;
				/* Take it off the pending list */
				rem_lock(&(lox->pending), lck);
				kill_all_refs(lck);
				/* take if off the proccess lock list */
				rem_proc_lock(lck);
				FREELOCK(lck);
			}
			if ((lck->type & EXCLUSIVE) != 0)
                        	(void)unlock_region(lck, &lox->shared, lox->pending);
			else if ((lck->type & EXCLUSIVE) == 0)
				(void)unlock_region(lck, &lox->exclusive, lox->pending);
#ifdef LOCK_DEBUG
			if (lock_debug_on) {
				if (!Lockerror)
					printf("\nProc %d : Lock granted.\n", lck->pid);
				else
					printf("\nProc %d : Lock denied.\n", lck->pid);
				printf("\n------------------lock START (sleeping) \n");
				print_depend(vp);
				printf("------------------lock END \n");
			}
#endif
			return (Lockerror);
		} else
			Lockerror = EDEADLK;
		if (lck) {
			kill_all_refs(lck);
			rem_proc_lock(lck);
			FREELOCK(lck);
		}
		IUNLOCK(ip);
		return (Lockerror);
	} /* if (cmd == F_SETLKW) */
	/* Lock failed and they didn't want to wait for it. */
	rem_proc_lock(lck);
	IUNLOCK(ip);	/* Unlock the inode */
	FREELOCK(lck);	/* Give back the lock */
	return (EACCES);
}

/*ARGSUSED*/
/*
 * Function test_lock(vp, ld, cmd, pid, class)
 *
 * This function provides System V compatible lock 'testing'. It is called
 * with ld containing the desired lock, and returns with the contents of
 * ld set to the first lock that is blocking this one.
 */

void
test_lock(vp, ld, cmd, pid, class)
	struct vnode	*vp;
	struct eflock	*ld;
	int		cmd;	/* Command (F_GETLK) */
	int		pid;	/* process id for current process */
	int		class;	/* Lock class, be it I/O, FILE, or LOCKMGR */
{
	struct data_lock	lck;	/* The lock we are testing */
	struct data_lock	*a;	/* A temp for holding our place */

#ifdef LOCK_DEBUG
	if (lock_debug_on) {
		printf("test_lock(whence=%d pid=%d, l_pid=%d rpid=%d base=%d, ",
			ld->l_whence, pid, ld->l_pid, ld->l_rpid, ld->l_start);
		printf("len=%d, type=%s%c, class=%s)\n[",
			ld->l_len, ((ld->l_type == F_WRLCK) ? "EXCL" : "SHRD"),
			((cmd == F_SETLK) ? ' ' : 'W'), lckclass[class]);
	}	
#endif
	/* set up the internal lock structure */
	lck.type = ((ld->l_type == F_RDLCK) ? 0 : EXCLUSIVE);
	lck.length = (ld->l_len == 0) ? LOCK_TO_EOF : ld->l_len;
	lck.base   = (ld->l_start >= 0) ? ld->l_start : 0;
	lck.pid    = pid;
	lck.class  = 0;
	lck.LockID = LockID++;
	if (!(vp->v_filocks)) {
#ifdef LOCK_DEBUG
		if (lock_debug_on) printf("No locks so we succeed\n");
#endif
		ld->l_type = F_UNLCK;
		if (lck.length == LOCK_TO_EOF) lck.length = 0;
		return; /* No locks so we succeed */
	}

	if (ld->l_type == F_WRLCK) {
		/* Now we check the shared list */
		for (a = ((struct lock_list *)(vp->v_filocks))->shared;
		    a; a = a->Next) {
#ifdef LOCK_DEBUG
			if (lock_debug_on) printf("SHARED LIST : END(a)=%d TOUCHING(a, &lck)=%d\n",
				END(a), TOUCHING(a, &lck));
#endif
			if (TOUCHING(a, &lck) && a->pid != lck.pid) {
				ld->l_type = F_RDLCK;
				ld->l_start = a->base;
				ld->l_len = a->length;
				ld->l_pid = a->pid;
				/* Remote locks return system and pid as well */
				if (class == LOCKMGR) {
					ld->l_rpid = a->rpid;
					ld->l_rsys = a->rsys;
				}
				if (ld->l_len == LOCK_TO_EOF) ld->l_len = 0;
				return; /* Blocked by 'a' */
			}
		} /* For all locks on list ... */

		/* Now we check the exclusive list */
		for (a = ((struct lock_list *)(vp->v_filocks))->exclusive;
			a; a = a->Next) {
#ifdef LOCK_DEBUG
			if (lock_debug_on) {
				printf("EX LIST : a->pid=%d lck.pid=%d a->base=%d ",
					a->pid, lck.pid, a->base);
				printf("lck.base=%d END(&lck)=%d END(a)=%d ",
					lck.base, END(&lck), END(a));
				printf("TOUCHING(a, &lck)=%d\n", TOUCHING(a, &lck));
			}
#endif
			if (TOUCHING(a, &lck) && a->pid != lck.pid) {
				ld->l_type = F_WRLCK;
				ld->l_start = a->base;
				ld->l_len = a->length;
				ld->l_pid = a->pid;
				if (class == LOCKMGR) {
					ld->l_rpid = a->rpid;
					ld->l_rsys = a->rsys;
				}
				if (ld->l_len == LOCK_TO_EOF)
					ld->l_len = 0;
				return; /* Blocked by 'a' */
			}
		} /* For all locks on list ... */
	} else {
		/* Now we check the exclusive list */
		for (a = ((struct lock_list *)(vp->v_filocks))->exclusive;
			a; a = a->Next) {
#ifdef LOCK_DEBUG
			if (lock_debug_on) printf("EX LIST : END(a)=%d TOUCHING(a, &lck)=%d\n",
				END(a), TOUCHING(a, &lck));
#endif
			if (TOUCHING(a, &lck) && a->pid != lck.pid) {
				ld->l_type = F_WRLCK;
				ld->l_start = a->base;
				ld->l_len = a->length;
				ld->l_pid = a->pid;
				if (class == LOCKMGR) {
					ld->l_rpid = a->rpid;
					ld->l_rsys = a->rsys;
				}
				if (ld->l_len == LOCK_TO_EOF)
					ld->l_len = 0;
				return; /* Blocked by 'a' */
			}
		} /* For all locks on list ... */

		if (ld->l_type == F_RDLCK) {
			ld->l_type = F_UNLCK;
			return;
		}
	}

	ld->l_type = F_UNLCK;
#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("RETURN test_lock(whence=%d base=%d, len=%d type=%d\n[",
		ld->l_whence, ld->l_start, ld->l_len, ld->l_type);
#endif
	if (ld->l_len == LOCK_TO_EOF) ld->l_len = 0;
	return;
}

/*
 * function kill_proc_locks(pid)
 *
 * This function will kill any locks that are held by the passed process.
 * This is useful with the lockmanager because it retains locks on files
 * that it no longer has open (proxy locks) and these need to be removed.
 * It uses the back pointer to the vnode to remove the locks.
 *
 * This function has two purposes, one to kill all locks associated with
 * a given process, and two to kill all locks that are being held by the
 * given process for a remote system. Local, non-lockmangager type processes
 * will always have a system ID of 0 in their locks.
 */

void
kill_proc_locks(pid, rsys)
	int	pid;	/* Local process ID */
	long	rsys;	/* Remote system ID (if needed) */
{
	struct eflock		tlock;	/* The lock we will unlock */
	struct process_locks	*pl;	/* Pointer to all locks owned by us */
	struct data_lock	*t;	/* For walking the lock list.	 */
	struct data_lock	*n;	/* temporary for walking the list */
	struct vnode		*vp;	/* Pointer to a vnode from the lock */
	int			e;	/* error status */

#ifdef LOCK_DEBUG
	if (lock_debug_on) printf("...kill_proc_locks()");
#endif

	/* After last lock is unlocked pl->pid will be set to 0 */
	for (pl = proc_locks; pl; pl = pl->next) {
		for (t = pl->lox; t; t = n) {
			n = t->NextProc; /* Next lock owned by this process */
			if (!(rsys) || (t->rsys == rsys)) {
				tlock.l_start = t->base;
				tlock.l_len = t->length;
				tlock.l_type = F_UNLCK;
				tlock.l_rpid = t->rpid;
				tlock.l_pid = t->pid;
				tlock.l_rsys = t->rsys;
				vp = t->vp;	/* Get the vnode pointer */
				e = unlock(vp, &tlock, F_SETLK, pid, t->class);
				if (e)	/* XXX panic here? */
					printf("Warning, unlock failed.\n");
			}
		}
	}
	return;
}

#ifdef LOCK_DEBUG
static char *clss[3] = {"FILE", " I/O", "LMGR"};

/*
 * return a string representation of the lock.
 */
void
print_lock(l)
	struct data_lock	*l;
{

	printf("[ID=%d, pid=%d, base=%d, len=%d, ",
		l->LockID, l->pid, l->base, l->length);
	printf("type=%s class=%s, rpid=%d, ",
		((l->type & EXCLUSIVE) != 0) ? "EXCL" : "SHRD", clss[l->class],
		l->rpid);
	printf("rsys=%x]", l->rsys);
	return;
}
#endif

#ifdef LOCK_DEBUG
/*
 * This function will print lock dependencies in the active and pending lists.
 */
static void
print_depend(vp)
	struct vnode	*vp;
{
	struct data_lock	*a;	/* temporary */
	struct lock_ref		*ref;	/* temporary */

	printf("Locks held on vnode (%x).\n", (unsigned long)(vp));
	if (!(vp->v_filocks)) {
		printf("--none--\n");
		return;
	}
	printf("Active Exclusive Locks :\n");
	for (a = Exclusive; a; a = a->Next) {
		if (a->Blocked_By) {
			printf("ERROR! Active lock is Blocked????\n");
			printf("Offending lock is ");
			print_lock((a->Blocked_By)->Conflict);
			printf("\n");
		}
		printf("    ");
		print_lock(a);
		printf("-> Prev = ");
		if (a->Prev)
			printf("Lock #%d\n", (a->Prev)->LockID);
		else
			printf("(NULL)\n");
		printf("\n");
		printf("Blocking list start:\n");
		for (ref = a->Is_Blocking; ref; ref = ref->Next) {
			printf("	");
			print_lock(ref->Conflict);
			printf("\n");
		}
		printf("Blocking list end:\n");
	}
	printf("Active Shared Locks :\n");
	for (a = Shared; a; a = a->Next) {
		if (a->Blocked_By) {
			printf("ERROR! Active lock is Blocked????\n");
			printf("Offending lock is ");
			print_lock((a->Blocked_By)->Conflict);
			printf("\n");
		}
		printf("    ");
		print_lock(a);
		printf(" -> Prev = ");
		if (a->Prev)
			printf("Lock #%d\n", (a->Prev)->LockID);
		else
			printf("(NULL)\n");
		printf("\n");
		printf("Blocking list start:\n");
		for (ref = a->Is_Blocking; ref; ref = ref->Next) {
			printf("	");
			print_lock(ref->Conflict);
			printf("\n");
		}
		printf("Blocking list end:\n");
	}
	printf("\nPending Locks :\n");
	for (a = Pending; a; a = a->Next) {
		printf("    ");
		print_lock(a);
		printf(" -> Prev = ");
		if (a->Prev)
			printf("Lock #%d\n", (a->Prev)->LockID);
		else
			printf("(NULL)\n");
		printf("\n");
		printf("	Blocked By list start:\n");
		for (ref = a->Blocked_By; ref; ref = ref->Next) {
			printf("	    ");
			print_lock(ref->Conflict);
			printf("\n");
		}
		printf("	Blocked By list end:\n");
		printf("	Is Blocking list start:\n");
		for (ref = a->Is_Blocking; ref; ref = ref->Next) {
			printf("	    ");
			print_lock(ref->Conflict);
			printf("\n");
		}
		printf("	Is Blocking list end:\n");
	}
}
#endif

chk_granted(vp, ld, ip, class, iunlock_flag)
	struct vnode	*vp;	/* the vnode we are using */
	struct eflock	*ld; 	/* the flock structure */
	struct inode	*ip;	/* The inode of this vnode */
	int		class;	/* Lock class (NONE, IO, REMOTE) */
	int		*iunlock_flag;
{
	struct data_lock	*t;	/* Temporaries */
	struct data_lock	*n;
	struct lock_list 	*lox;	/* list of locks we're holding */
	struct grant_lock	*grant_tmp_p;
	struct eflock 		eld;
	lockhandle_t 		lh;
	int 			command;
	int			e = 0;	/* An error variable */

	lox = (struct lock_list *)(vp->v_filocks);

	for (t = lox->pending; t; t = n) {
		n = t->Next; /* cache this pointer. */
		if (t->Blocked_By == NULL) { /* All blocking locks are gone */
#ifdef LOCK_DEBUG
			if (lock_debug_on) printf("...(LOCK #%d, pid=%d unblocked.)...", t->LockID, t->pid);
#endif
			rem_lock(&(lox->pending), t);
			/*
			 * If lock is really disk I/O waiting just wake the
			 * process
			 */
			if (!lock_region(lox, t)) {
				/*
				 * If it is a network lock tell the lock
				 * manager it has been granted.
				 */
				if (t->class == LOCKMGR) {
				    /* if the current lock request is not */
				    /* from lock mgr, we can then send rpc  */
				    /* KLM_GRANTED locks to lockmgr	    */ 
				    /* otherwise...see else		    */
				    if (class != LOCKMGR) {
					/* NOTE:			     */
					/* we need to IUNLOCK here as it may */
					/* cause deadlock w/ lockmgr if its  */
					/* blocking on fcntl()		     */
					/* ie.lockmgr blocked on fcntl() will*/
					/* not receive our rpc requests      */
					if (!(*iunlock_flag)) {
						IUNLOCK(ip);
						*iunlock_flag = TRUE;
					}
					lh.lh_vp = vp;
					lh.lh_servername = hostname;
					bcopy((caddr_t)vtofh(vp),
						(caddr_t)&lh.lh_id,
						sizeof (fhandle_t));
					eld.l_type = (t->type == EXCLUSIVE ? F_WRLCK : F_RDLCK);
					eld.l_whence = 0;
					eld.l_start = t->base;
					eld.l_len = t->length;
					eld.l_pid = t->pid;
					eld.l_rpid = t->rpid;
					eld.l_rsys = t->rsys;
					command = KLM_GRANTED;
#ifdef LOCK_DEBUG
					if (lock_debug_on) print_depend(vp);
#endif
					e = klm_lockctl(&lh, &eld, command, u.u_cred, u.u_procp->p_pid);
				   } else {
					/* store the locks to be granted   */
					/* in grant_locks_p so that the    */
					/* nxt fcntl() call by lock mgr    */
					/* will get each granted lock info */
					if (++num_grant_locks > max_grant_space) {
					    /* if we need extra space to  */
					    /* the lock info, alloc'ed it */
					    /* will be freed by nxt 	  */
					    /* fcntl() calls		  */
					    grant_tmp_p= (struct grant_lock *) new_kmem_zalloc(sizeof (struct grant_lock), KMEM_SLEEP);
					    grant_tmp_p->next = grant_locks_p;
					    grant_locks_p = grant_tmp_p;
					    max_grant_space += MAX_GRANT_LOCKS;
					}

					/* store the ptr of the data_lock of */
					/* the lock to be granted	     */
					grant_locks_p->grant_lock_list[(num_grant_locks-1)%MAX_GRANT_LOCKS] = t;
					/* set flag to indicate exisitence */
					/* of granted locks		   */
					ld->l_xxx = GRANT_LOCK_FLAG;
				   }
				} else
					wakeup((caddr_t)t);
			} else {
				printf("unlock - file lock list exhausted");
				t->granted = 0; /* owner will see EINTR */
				e = EINTR; /* So that we no this occurred */
				/* put it back on pending list at front */
				add_lock(&(lox->pending), t, lox->pending);
				wakeup((caddr_t) t);
			}
		}
	}
	return(e);
}

do_grant_locks(vp, ld)
	struct vnode	*vp;	/* the vnode we are using */
	struct eflock	*ld; 	/* the flock structure */
{
	struct lock_list 	*lox;	/* list of locks we're holding */
	struct data_lock	*t;	/* Temporaries */
	struct grant_lock	*grant_tmp_p;
	int 			grant_index;

	/* invalid args if no locks r reported & fcntl is called */
	/* to get the granted locks				 */
	if (num_grant_locks == 0)
		return(EINVAL);

	/* store the granted lock in the ld structure passed in */
	grant_index = (--num_grant_locks % MAX_GRANT_LOCKS);
	t = grant_locks_p->grant_lock_list[grant_index];
	/*
	 * Make sure all the holding locks are gone.
	 */
	lox = (struct lock_list *)(vp->v_filocks);
	if ((t->type & EXCLUSIVE) != 0)
		(void)unlock_region(t, &lox->shared, lox->pending);
	else if ((t->type & EXCLUSIVE) == 0)
		(void)unlock_region(t, &lox->exclusive, lox->pending);

	ld->l_type = (t->type == EXCLUSIVE ? F_WRLCK : F_RDLCK);
	ld->l_whence = 0;
	ld->l_start = (t->base < 0) ? 0x7fffffff : t->base;
	ld->l_len = (t->length == LOCK_TO_EOF) ? 0 : t->length;
	ld->l_pid = t->pid;
	ld->l_rpid = t->rpid;
	ld->l_rsys = t->rsys;
	if (grant_index == 0) {
		/* free up the alloc space if any that's used to */
		/* store the granted locks			 */
		if (grant_locks_p != &grant_locks) {
			grant_tmp_p = grant_locks_p->next;
			kmem_free((caddr_t)grant_locks_p, 
				sizeof (struct grant_lock));
			grant_locks_p = grant_tmp_p;
			max_grant_space -= MAX_GRANT_LOCKS;
		}
	}
	/* signifies end of last granted lock */
	if (num_grant_locks == 0)
		ld->l_xxx = -1;
	return (0);
}
