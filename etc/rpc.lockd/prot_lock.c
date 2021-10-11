#ifndef lint
static char sccsid[] = "@(#)prot_lock.c 1.1 92/07/30 SMI";
#endif

	/*
	 * Copyright (c) 1988 by Sun Microsystems, Inc.
	 */

	/*
	 * prot_lock.c consists of low level routines that
	 * manipulates lock entries;
	 * place where real locking codes reside;
	 * it is (in most cases) independent of network code
	 */

#include <stdio.h>
#include <sys/file.h>
#include <sys/param.h>
#include "prot_lock.h"
#include "priv_prot.h"
#include <rpcsvc/sm_inter.h>
#include "sm_res.h"

#define same_proc(x, y) (obj_cmp(&x->lck.oh, &y->lck.oh))

static struct priv_struct priv;

int	LockID = 0;	/* Monotonically increasing id */

extern struct process_locks	*free_proc_locks;

extern remote_result res_working;

extern int		used_reclock;
extern int		used_me;
extern int		pid;			/* used by status monitor */
extern int 		debug;
extern int 		local_state;

extern char		*xmalloc();
extern char		hostname[MAXHOSTNAMELEN];	/* used by remote_data() */

extern struct process_locks *proc_locks;

extern msg_entry 	*retransmitted(), *msg_q;
extern struct stat_res 	*stat_mon();
extern struct reclock *del_req_from_list();
extern char *copy_str();

int			cancel();
int			obj_copy();
int			obj_alloc();
int			is_blocked();
int			add_reclock();
int 			contact_monitor();
void			lm_unlock_region();

void			add_fh();
void			add_fh_reclox();
void			rem_fh_reclox();
void			rem_data_lock();
void			rem_process_lock();

struct process_locks	*get_proc_list();
struct process_locks  	*copy_proclock();

struct data_lock	*get_lock();

reclock			*get_reclock();

bool_t			obj_cmp();
bool_t			same_op();
bool_t			same_lock();
bool_t                  same_lockregion();
bool_t			same_type();
bool_t			same_bound();
bool_t			remote_data();
bool_t			remote_clnt();

lm_vnode 		*search_fh();
lm_vnode		*find_me();
lm_vnode		*get_me();

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
void
merge_lock(req, list)
	struct reclock	*req;	/* The lock in question */
	struct reclock	**list;  /* List to add it too   */
{
	struct data_lock	*a;	/* Temporary pointer */
	struct data_lock	*n; 	/* Temporary pointer */
	struct reclock		*t; 	/* Temporary pointer */
	struct reclock		*spanlist; /* Temporary list of spanned locks */
	struct reclock		*tmp_req;
	struct data_lock	*l = &req->lck.lox;
	struct process_locks	*p;
	int			done_flag = FALSE;

	if (debug)
		printf("enter merge_lock(req %x list %x)...", req, *list);

	for (p = proc_locks; p && p->pid != l->pid; p = p->next);

	if (p != NULL) {
		for (a = p->lox, spanlist = NULL; a ; a = n) {
		    /* Cache the next pointer */
		    if ((n = a->Next) == p->lox)
			done_flag = TRUE;
	
		    if (obj_cmp(&(req->lck.fh), &(a->req->lck.fh))) {
			/* Case one, it is completely within another lock */
			if (WITHIN(l, a) && SAMEOWNER(l, a) &&
	    	    	(l->type == a->type)) {
				/* lock exists, so we can free the current */
				/* request				   */
				req->rel = 1;
				return; /* merge successful */
			}
	
			/* Case two, it completely surrounds one or more locks */
#ifdef LOCK_DEBUG
			printf("MERGE_LOCK : END(l)=%d TOUCHING(l, a)=%d\n",
				END(l), TOUCHING(l, a));
#endif
			if ((TOUCHING(l, a) || ADJACENT(l, a)) &&
	    	    	SAMEOWNER(l, a) && (l->type == a->type)) {
				/* remove this lock from this list */
					rem_process_lock(a);
				if ((tmp_req = del_req_from_list(a, list))
			    		!= NULL)
					/* Add it to the spanned list */
					add_req_to_list(tmp_req, &spanlist);
			}
		    }
	
		    if (done_flag)	break;
		}
	
		/*
 		 * Case two continued, merge the spanned locks.  Expand l
 		 * to include the furthest "left" extent of all locks on
 		 * the span list -- and to include the furthest "right" extent
 		 * as well.  Remove all of the spanned locks.
 		 */
		if (spanlist) {
			done_flag = FALSE;
			for (tmp_req = spanlist; tmp_req ; tmp_req = t) {
				if ((t = tmp_req->next) == spanlist)
					done_flag = TRUE;
				n = &tmp_req->lck.lox;
				if (n->base < l->base) {
					if (END(l) >= 0)
						l->length += l->base - n->base;
					l->base = n->base;
				}
				if (END(l) >= 0 && END(n) >= 0)
					l->length = (END(l) < END(n) ? 
						END(n) : END(l)) + 1 - l->base;
				else
					l->length = LOCK_TO_EOF;
	
				tmp_req->rel = 1;
				release_reclock(tmp_req);
	
				if (done_flag)	break;
			}
		}
	}
	
	/* Case three, its a new lock with no siblings */
	if (add_process_lock(l))
		add_req_to_list(req, list);
	return;
}

void
lm_unlock_region(req, list)
	struct reclock		*req; 	/* The region to unlock */
	struct reclock		**list;	/* The list we are unlocking from */
{
	struct data_lock 	*t,	/* Some temporaries */
				*n,
				*x;
	struct reclock		*tmp_req;
	struct reclock		*tmp_p;
	struct data_lock	*l = &req->lck.lox;
	struct process_locks	*p, *prev = NULL;
	int			done_flag = FALSE;

	if (debug)
		printf("enter lm_unlock_region(req %x list %x)....\n",
			req, *list);

	for (p = proc_locks; p && p->pid != l->pid; prev = p, p = p->next);

	if (p != NULL) {
		for (t = p->lox; t ; t = n) {
		    if ((n = t->Next) == p->lox)
			done_flag = TRUE;

		    if (obj_cmp(&(req->lck.fh), &(t->req->lck.fh))) {
			if (((l->type && SAMEOWNER(t, l)) || 
	    	     	     (!l->type && l->pid == t->pid)) && 
		    	    (t->class == l->class) && TOUCHING(l, t)) {
			     if (WITHIN(t, l)) {
				/* Remove blocking referencese */
out:			     	
			     	if ((tmp_req = 
			             del_req_from_list(t, list)) != NULL) {
					rem_process_lock(t);
			     		tmp_req->rel = 1;
			     		release_reclock(tmp_req);
			     	}
			     /* case one region 'a' is to the left of 't' */
			     } else if (l->base <= t->base) {
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
			     /* Case two new lock is to the right of 't' */
			     } else if (((END(l) >= 0) && (END(t) >= 0) &&
					(END(l) >= END(t))) || (END(l) < 0)) {
			        t->length = l->base - t->base;
			     /* Case three, new lock in in the middle of 't' */
			     } else {
			        /*
			         * This part can fail if there isn't another
			         * lock entry and 'n' lock_refs available to
			         * build it's dependencies.
			         */
			         if ((tmp_req = get_reclock()) != NULL) {
					for (tmp_p = *list; tmp_p; 
				     	     tmp_p = tmp_p->next) {
						if (&tmp_p->lck.lox == t)
							break;
						if (tmp_p->next == *list)
							return;
					}
					/* Structure assignment! */
					if (tmp_p != NULL)
						*tmp_req = *tmp_p;	/* Copy the original */
					if (init_reclock(tmp_req, tmp_p)) {
					    x = &tmp_req->lck.lox;	
					    /* give it a fresh id */
					    x->LockID = LockID++;	
					    t->length = l->base - t->base;
					    if (END(x) >= 0 && END(l) >= 0) {
						    x->length = END(x) - END(l);
						    x->base = END(l) + 1;
					    } else if (END(x) < 0 && END(l) >= 0) {
						    x->length = LOCK_TO_EOF;
						    x->base = END(l) + 1;
					        }
					    if (add_process_lock(x))
						    add_req_to_list(tmp_req, 
									list);
					    else {
						    tmp_req->rel = 1;
						    release_reclock(tmp_req);
					    }
					} else {
					    tmp_req->rel = 1;
					    release_reclock(tmp_req);
					}
			     	}
			     }
			}
		    }

		    if (done_flag)	break;
		}
		/*
		 * Collect freed process lock.
		 */
		if (p->lox == NULL) {
			if (prev)
				prev->next = p->next;
			else
				proc_locks = p->next;
			p->next = free_proc_locks;
			free_proc_locks = p;
		}
	}
	return;
}
	
int
init_reclock(to, from)
	struct reclock	*to;
	struct reclock	*from;
{
	if (from == NULL)
		return (FALSE);

	if (from->cookie.n_len) {
		if (obj_copy(&to->cookie, &from->cookie) == -1)
			return(FALSE);
	} else
		to->cookie.n_bytes= 0;

	to->prev = to->next = NULL;

	if ((to->alock.caller_name = copy_str(from->alock.caller_name)) 
		== NULL)
		return(FALSE);
	if (from->alock.clnt_name == from->alock.caller_name)
		to->alock.clnt_name = to->alock.caller_name;
	else {
		if ((to->alock.clnt_name = copy_str(from->alock.clnt_name)) 
			== NULL)
		return(FALSE);
	}
	if (from->alock.server_name == from->alock.caller_name)
		to->alock.server_name = to->alock.caller_name;
	else {
		if ((to->alock.server_name = copy_str(from->alock.server_name)) 
			== NULL)
		return(FALSE);
	}

	if (from->alock.fh.n_len) {
		if (obj_copy(&to->alock.fh, &from->alock.fh) == -1)
			return(FALSE);
	} else
		to->alock.fh.n_len = 0;
	if (from->alock.oh.n_len) {
		if (obj_copy(&to->alock.oh, &from->alock.oh) == -1)
			return(FALSE);
	} else
		to->alock.oh.n_len = 0;

	to->lck.lox.Next = to->lck.lox.Prev = NULL;
	to->lck.lox.req = to; /* set the data_lock back ptr to req */
	return (TRUE);
}

void
rem_process_lock(a)
	struct data_lock *a;
{
	struct data_lock *t;

	if (debug)
		printf("enter rem_process_lock.....\n");

	if ((t = a->MyProc->lox) != NULL) {
		while (t != a) {
			if (t->Next == a->MyProc->lox)
				return;
			else
				t = t->Next;
		}
		t->Prev->Next = t->Next;
		t->Next->Prev = t->Prev;
		if (t == a->MyProc->lox) {
			if (t->Next == t)
				a->MyProc->lox = NULL;
			else
				a->MyProc->lox = t->Next;
		}
	}
	return;
}

int
add_process_lock(a)
	struct data_lock *a;
{
	struct process_locks	*p;
	struct data_lock *t;

	/*
	 * Go through table and match pid
	 */
	if (debug)
		printf("enter add_process_lock.....\n");

	for (p = proc_locks; p && p->pid != a->pid; p = p->next);

	if (p == NULL) {
		if ((p = get_proc_list(a->pid)) == NULL) {
			printf("rpc.lockd: out of proc_list slot !\n");
			return (FALSE);
		}
	}

	if (debug)
		printf("PID : %d\n", p->pid);
	if ((t = p->lox) == NULL)
		p->lox = a->Prev = a->Next = a;
	else {
		t->Prev->Next = a;
		a->Next = t;
		a->Prev = t->Prev;
		t->Prev = a;
	}
	a->MyProc = p;
	return (TRUE);
}


bool_t
obj_cmp(a, b)
	struct netobj *a, *b;
{
	if (a->n_len != b->n_len)
		return (FALSE);
	if (bcmp(&a->n_bytes[0], &b->n_bytes[0], a->n_len) != 0)
		return (FALSE);
	else
		return (TRUE);
}

/*
 * duplicate b in a;
 * return -1, if malloc error;
 * returen 0, otherwise;
 */
int
obj_alloc(a, b, n)
	netobj *a;
	char *b;
	u_int n;
{
	a->n_len = n;
	if ((a->n_bytes = xmalloc(n)) == NULL) {
		return (-1);
	}
	else
		bcopy(b, a->n_bytes, a->n_len);
	return (0);
}

/*
 * copy b into a
 * returns 0, if succeeds
 * return -1 upon error
 */
int
obj_copy(a, b)
	netobj *a, *b;
{
	if (b == NULL) {
		/* trust a is already NULL */
		if (debug)
			printf(" obj_copy(a = %x, b = NULL), a\n", a);
		return (0);
	}
	return (obj_alloc(a, b->n_bytes, b->n_len));
}

bool_t
same_op(a, b)
	reclock *a, *b;
{
	if (((a-> lck.op & LOCK_EX) && (b-> lck.op & LOCK_EX)) ||
		((a-> lck.op & LOCK_SH) && (b-> lck.op & LOCK_SH)))
		return (1);
	else
		return (0);

}

bool_t
same_bound(a, b)
	struct data_lock *a, *b;
{
	if (a && b && (a->base == b->base) &&
		(a->length == b->length))
		return (1);
	else
		return (0);
}

bool_t
same_type(a, b)
	struct data_lock *a, *b;
{
	if (a->type == b->type)
		return (1);
	else
		return (0);
}

bool_t
same_lock(a, b)
	reclock *a;
	struct data_lock *b;
{
	if (a && b && same_type(&(a->lck.lox), b) && 
		same_bound(&(a->lck.lox), b) &&
		SAMEOWNER(&(a->lck.lox), b))
		return (TRUE);
	else
		return (FALSE);
}

bool_t
same_lockregion(a, b)
        reclock *a;
        struct data_lock *b;
{
        if (a && b && same_bound(&(a->lck.lox), b) && 
		(a->lck.lox.pid == b->pid && a->lck.lox.rpid == b->rpid)) 
                return (TRUE);
        else
                return (FALSE);
}

bool_t
simi_lock(a, b)
	reclock *a, *b;
{
	if (same_proc(a, b) && same_op(a, b) &&
	    WITHIN(&b->lck.lox, &a->lck.lox))
		return (TRUE);
	else
		return (FALSE);
}

bool_t
remote_data(a)
	reclock *a;
{
	if (strcmp(a->lck.svr, hostname) == 0)
		return (FALSE);
	else
		return (TRUE);
}


bool_t
remote_clnt(a)
	reclock *a;
{
	if (strcmp(a->lck.clnt, hostname) == 0)
		return (FALSE);
	else
		return (TRUE);
}

/*
 * translate monitor calls into modifying monitor chains
 * returns 0, if success
 * returns -1, in case of error
 */
int
add_mon(a, i)
	reclock *a;
	int i;
{
	if (debug)
		printf("Enter add_mon ........\n");
	if (strcmp(a->lck.svr, a->lck.clnt) == 0)
		/* local case, no need for monitoring */
		return (0);
	if (remote_data(a)) {		/* client */
		if (strlen(hostname) &&
			mond(hostname, PRIV_RECOVERY, a, i) == -1)
			return (-1);
		if (strlen(a->lck.svr) &&
			mond(a->lck.svr, PRIV_RECOVERY, a, i) == -1)
			return (-1);
	} else {			/* server */
		if (strlen(a->lck.clnt) &&
			mond(a->lck.clnt, PRIV_CRASH, a, i) == -1)
			return (-1);
	}
	return (0);
}

/*
 * mond set up the monitor ptr;
 * it return -1, if no more free mp entry is available when needed
 * or cannot contact status monitor
 */
int
mond(site, proc, a, i)
	char *site;
	int proc;
	reclock *a;
	int i;
{
	struct lm_vnode *new;

	if (i == 1) {		/* insert! */
		if ((new = find_me(site)) == NULL) { /* not found */
			if (( new = get_me()) == NULL) /* no more me entry */
				return (-1);
			else {	/* create a new mp */
				if ((new->svr = xmalloc(strlen(site)+1)) == NULL) {
					used_me--;
					free((char *) new);
					return (-1);
				}

				(void) strcpy(new->svr, site);
				/* contact status monitor */
				if (contact_monitor(proc, new, 1) == -1) {
					used_me--;
					xfree(&new->svr);
					free((char *) new);
					return (-1);
				} else {
					insert_me(new);
				}
			}
		}
		return (0);
	} else { /* i== 0; delete! */
		if ((new = find_me(site)) == NULL)
			return (0);   /* happen due to call back */
	}
}

int
contact_monitor(proc, new, i)
	int proc;
	struct lm_vnode *new;
	int i;
{
	struct stat_res *resp;
	int priv_size;
	int func;

	if (debug)
		printf("enter contact_monitor ....\n");
	switch (i) {
	case 0:
		func = SM_UNMON;
		break;
	case 1:
		func = SM_MON;
		break;
	default:
		fprintf(stderr, "unknown contact monitor (%d)\n", i);
		abort();
	}

	priv.pid = pid;
	priv.priv_ptr = (int *) new;
	if ((priv_size = sizeof (struct priv_struct)) > 16) /* move to init */
		fprintf(stderr, "contact_mon: problem with private data size (%d) to status monitor\n",
			priv_size);

	if ( !strcmp(new->svr, hostname) ) {
		return (0);
	}
	resp = stat_mon(new->svr, hostname, PRIV_PROG, PRIV_VERS,
		proc, func, priv_size, &priv);
	if (resp->res_stat == stat_succ) {
		if (resp->sm_stat == stat_succ) {
			local_state = resp->sm_state; /* update local state */
			return (0);
		} else {
			fprintf(stderr,
				"rpc.lockd: site %s does not subscribe to status monitor service \n", new->svr);
			return (-1);
		}
	} else {
		fprintf(stderr, "rpc.lockd cannot contact local statd\n");
		return (-1);
	}
}

