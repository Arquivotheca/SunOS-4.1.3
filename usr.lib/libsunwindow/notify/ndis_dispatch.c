#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndis_dispatch.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndis_dispatch.c - Central control mechanism for the dispatcher.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndis.h>
#include <sunwindow/nint.h>
#include <sunwindow/ndet.h>	/* For ndet_set_event_processing/ndet_flags */
#include <signal.h>

/* performance: global cache of getdtablesize() */
extern int dtablesize_cache;
#define GETDTABLESIZE() \
	(dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

pkg_private_data u_int	ndis_flags = 0;
pkg_private_data NTFY_CLIENT *ndis_clients = 0;
pkg_private_data NTFY_CLIENT *ndis_client_latest = 0;

pkg_private_data Notify_value (*ndis_scheduler)() = ndis_default_scheduler;

static	Notify_client *ndis_sched_nclients;/* Parallel data structure to
					   ndis_clients for use with scheduler*/
static	u_int ndis_sched_count;		/* Last valid position (plus one) in
					   ndis_sched_nclients */
static	u_int ndis_sched_length;	/* Current length of
					   ndis_sched_nclients */

static	Notify_event *ndis_events;	/* List of events currently being
					   dispatched for a given client */
static	Notify_arg *ndis_args;		/* List of event args currently being
					   dispatched for a given client
					   (parallels ndis_events) */
static	u_int ndis_event_count;		/* Last valid position (plus one) in
					   ndis_events & ndis_args */
static	u_int ndis_event_length;	/* Current length of ndis_events &
					   ndis_args */

static	Notify_error ndis_send_func();	/* Used to get func for sending notify*/

#define	NDIS_RELEASE_NULL	((Notify_release *)0)

static	NTFY_ENUM ndis_setup_sched_clients();

/* Enqueue condition from dectector on to the dispatchers client list */
pkg_private Notify_error
ndis_enqueue(ndet_client, ndet_condition)
	register NTFY_CLIENT *ndet_client;
	register NTFY_CONDITION *ndet_condition;
{
	register NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;

	ntfy_assert(NTFY_IN_CRITICAL, "Not protected when enqueue condition");
	/* Find/create client that corresponds to ndet_client */
	if ((client = ntfy_new_nclient(&ndis_clients, ndet_client->nclient,
            &ndis_client_latest)) == NTFY_CLIENT_NULL)
		goto Error;
	client->prioritizer = ndet_client->prioritizer;
	/* Allocate condition */
	if ((condition = ntfy_alloc_condition()) == NTFY_CONDITION_NULL)
		goto Error;
	/* Initialize condition */
	condition->next = NTFY_CONDITION_NULL;
	condition->type = ndet_condition->type;
	condition->release = ndet_condition->release;
	condition->arg = ndet_condition->arg;
	switch (condition->type) {
	case NTFY_REAL_ITIMER:
	case NTFY_VIRTUAL_ITIMER:
		condition->data.an_u_int = 0;
		break;
	case NTFY_WAIT3:
		if ((condition->data.wait3 = (NTFY_WAIT3_DATA *) ntfy_malloc(
		    sizeof(NTFY_WAIT3_DATA))) == NTFY_WAIT3_DATA_NULL)
			goto Error;
		*condition->data.wait3 = *ndet_condition->data.wait3;
		break;
	default:
		condition->data.an_u_int = ndet_condition->data.an_u_int;
		break;
	}
	condition->func_count = ndet_condition->func_count;
	condition->func_next = 0;
	if (nint_copy_callout(condition, ndet_condition) != NOTIFY_OK)
		goto Error;
	/* Append to condition list */
	ntfy_append_condition(&(client->conditions), condition);
	/* Set up condition hint */
	client->condition_latest = condition;
	/* Set dispatcher flag */
	ndis_flags |= NDIS_EVENT_QUEUED;
	return(NOTIFY_OK);
Error:
	return(notify_errno);
}

/* Dispatch to all clients according to the scheduler */
pkg_private Notify_error
ndis_dispatch()
{
	Notify_value (*sched_func)();

	/* Quick bailout if nothing to send */
	if (ndis_clients == NTFY_CLIENT_NULL)
		return(NOTIFY_OK);
	ntfy_assert(!NTFY_IN_CRITICAL, "In critical when dispatch");
	ntfy_assert(!NTFY_IN_INTERRUPT, "In interrupt when dispatch");
	NTFY_BEGIN_CRITICAL;
	/* Build nclient list for scheduler */
	for (;;) {
		ndis_sched_count = 0;
		/* If enumerator returns NTFY_ENUM_TERM then list too short */
		if (ntfy_enum_conditions(ndis_clients, ndis_setup_sched_clients,
		     NTFY_ENUM_DATA_NULL) == NTFY_ENUM_TERM) {
			/* Free previous list */
			if (ndis_sched_nclients)
				ntfy_free_malloc(
				    (NTFY_DATA)ndis_sched_nclients);
			/* Allocate new list */
			ndis_sched_length += 20;
			if ((ndis_sched_nclients = (Notify_client *)
			  ntfy_malloc(ndis_sched_length*sizeof(Notify_client)))
			  == (Notify_client *)0) {
				NTFY_END_CRITICAL;
				return(notify_errno);
			}
		} else
			break;
	}
	/* Call scheduler */
	sched_func = ndis_scheduler;
	NTFY_END_CRITICAL;
	if (sched_func(ndis_sched_count, ndis_sched_nclients) ==
	    NOTIFY_UNEXPECTED)
		/*
		 * Scheduler sets notify_errno and returns error if there
		 * is some non-callout related problem.
		 */
		return(notify_errno);
	return(NOTIFY_OK);
}

/* ARGSUSED */
static NTFY_ENUM
ndis_setup_sched_clients(client, condition, context)
	NTFY_CLIENT *client;
	NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	/* Terminate enumeration if next slot overflows size of client table */
	if (ndis_sched_count == ndis_sched_length)
		return(NTFY_ENUM_TERM);
	*(ndis_sched_nclients+ndis_sched_count) = client->nclient;
	ndis_sched_count++;
	return(NTFY_ENUM_SKIP);
}

/* Flush given nclient & its conditions */
extern void
notify_flush_pending(nclient)
	Notify_client nclient;
{
	NTFY_CLIENT *client;
	register int i;

	NTFY_BEGIN_CRITICAL;
	if ((client = ntfy_find_nclient(ndis_clients, nclient,
            &ndis_client_latest)) == NTFY_CLIENT_NULL)
		goto Done;
	else {
		/* Flush any pending notification from dispatcher */
		ntfy_remove_client(&ndis_clients, client, &ndis_client_latest,
		    NTFY_NDIS);
		/* Remove nclient from list of clients to be scheduled */
		for (i = 0; i < ndis_sched_length; i++)
			if (ndis_sched_nclients[i] == nclient)
				ndis_sched_nclients[i] = NOTIFY_CLIENT_NULL;
		/*
		 * The dispatcher may be calling out to an nclient at this time.
		 * When an nclient calls back in, we always check the validity
		 * of the handle.  Thus, the dispatcher ignores any further
		 * references to client.
		 */
	}
Done:
	NTFY_END_CRITICAL;
	return;
}

extern Notify_error
notify_client(nclient)
	Notify_client nclient;
{
	NTFY_CLIENT *client;
	register NTFY_CONDITION *cdn;
	int sigbits, auto_sigbits;
	fd_set ibits, obits, ebits;
	    /* Don't make register because take address of them */
	Notify_value (*pri_func)();
	int maxfds	= GETDTABLESIZE();

	/* Check if heap access protected */
	ntfy_assert(!NTFY_IN_INTERRUPT, "In interrupt when notify");
	NTFY_BEGIN_CRITICAL;
Retry_Client:
	/* Find client */
	if ((client = ntfy_find_nclient(ndis_clients, nclient,
	    &ndis_client_latest)) == NTFY_CLIENT_NULL)
		/* Can get here on first time if client removed pending cond */
		goto Done;
	/* Reset flags that detects when another condition enqueued */
	ndis_flags &= ~NDIS_EVENT_QUEUED;
Retry_Conditions:
	/* Initialize prioritizer arguments */
	sigbits = auto_sigbits = 0;
	FD_ZERO(&ibits); FD_ZERO(&obits);  FD_ZERO(&ebits);
	ndis_event_count = 0;
	/* Fill out prioritizer arguments based on conditions */
	for (cdn = client->conditions; cdn; cdn = cdn->next) {
		switch (cdn->type) {
		case NTFY_INPUT:
			FD_SET(cdn->data.fd, &ibits);
			break;
		case NTFY_OUTPUT:
			FD_SET(cdn->data.fd, &obits);
			break;
		case NTFY_EXCEPTION:
			FD_SET(cdn->data.fd, &ebits);
			break;
		case NTFY_SYNC_SIGNAL:
			sigbits |= SIG_BIT(cdn->data.signal);
			break;
		case NTFY_REAL_ITIMER:
			auto_sigbits |= SIG_BIT(SIGALRM);
			break;
		case NTFY_VIRTUAL_ITIMER:
			auto_sigbits |= SIG_BIT(SIGVTALRM);
			break;
		case NTFY_WAIT3:
			auto_sigbits |= SIG_BIT(SIGCHLD);
			break;
		case NTFY_DESTROY:
			switch (cdn->data.status) {
			case DESTROY_CLEANUP:
				auto_sigbits |= SIG_BIT(SIGTERM);
				break;
			case DESTROY_PROCESS_DEATH:
				auto_sigbits |= SIG_BIT(SIGKILL);
				break;
			case DESTROY_CHECKING:
				auto_sigbits |= SIG_BIT(SIGTSTP);
				break;
			}
			break;
		case NTFY_SAFE_EVENT:
			/*
			 * Build event list for prioritizer.
			 * Terminate condition traversal if next slot
			 * overflows size of event table.
			 */
			if (ndis_event_count == ndis_event_length) {
				/* Free previous list */
				if (ndis_events) {
					ntfy_free_malloc(
					    (NTFY_DATA)ndis_events);
					ntfy_free_malloc(
					    (NTFY_DATA)ndis_args);
				}
				/* Allocate new list */
				ndis_event_length += 20;
				if ((ndis_events = (Notify_event *)
				    ntfy_malloc(ndis_event_length*
				    sizeof(Notify_event))) == (Notify_event *)0)
					goto Error;
				if ((ndis_args = (Notify_arg *)
				    ntfy_malloc(ndis_event_length*
				    sizeof(Notify_arg))) == (Notify_arg *)0)
					goto Error;
				/* Restart condition traversal */
				goto Retry_Conditions;
			}
			*(ndis_events+ndis_event_count) = cdn->data.event;
			*(ndis_args+ndis_event_count) = cdn->arg;
			ndis_event_count++;
			break;
		default:
			ntfy_fatal_error("Unexpected dispatcher cond");
		}
	}
	/* Call prioritizer */
	pri_func = client->prioritizer;
	NTFY_END_CRITICAL;
/* the second arg to pri_func should be max fd # for select call ??? */
	(void) pri_func(nclient, maxfds, &ibits, &obits, &ebits,
	    NSIG, &sigbits, &auto_sigbits, &ndis_event_count, ndis_events,
	    ndis_args);
	NTFY_BEGIN_CRITICAL;
	/* Check for NDIS_EVENT_QUEUED and redo loop */
	if (ndis_flags & NDIS_EVENT_QUEUED)
		goto Retry_Client;
Done:
	NTFY_END_CRITICAL;
	return(NOTIFY_OK);
Error:
	NTFY_END_CRITICAL;
	return(notify_errno);
}

static
Notify_error
notify_fd(nclient, fd, type)
	Notify_client nclient;
	int fd;
	NTFY_TYPE type;
{
	Notify_func func;

	/* Check arguments and get function to call */
	if (ndet_check_fd(fd) ||
	    ndis_send_func(nclient, type, (NTFY_DATA)fd, NTFY_USE_DATA,
	    &func, NTFY_DATA_PTR_NULL, NDIS_RELEASE_NULL) != NOTIFY_OK)
		return(notify_errno);
	(void) func(nclient, fd);
	/* Pop condition off interposition stack */
	nint_pop_callout();
	return(NOTIFY_OK);
}

extern Notify_error
notify_input(nclient, fd)
	Notify_client nclient;
	int fd;
{
	return(notify_fd(nclient, fd, NTFY_INPUT));
}

extern Notify_error
notify_output(nclient, fd)
	Notify_client nclient;
	int fd;
{
	return(notify_fd(nclient, fd, NTFY_OUTPUT));
}

extern Notify_error
notify_exception(nclient, fd)
	Notify_client nclient;
	int fd;
{
	return(notify_fd(nclient, fd, NTFY_EXCEPTION));
}

extern Notify_error
notify_itimer(nclient, which)
	Notify_client nclient;
	int which;
{
	NTFY_TYPE type;
	Notify_func func;

	/* Check arguments and get function to call */
	if (ndet_check_which(which, &type) ||
	    ndis_send_func(nclient, type, NTFY_DATA_NULL, NTFY_IGNORE_DATA,
	    &func, NTFY_DATA_PTR_NULL, NDIS_RELEASE_NULL) != NOTIFY_OK)
		return(notify_errno);
	(void) func(nclient, which);
	/* Pop condition off interposition stack */
	nint_pop_callout();
	return(NOTIFY_OK);
}

extern Notify_error
notify_event(nclient, event, arg)
	Notify_client nclient;
	Notify_event event;
	Notify_arg arg;
{
	Notify_func func;
	Notify_release release_func;

	/* Get function to call */
	if (ndis_send_func(nclient, NTFY_SAFE_EVENT, (NTFY_DATA)event,
	    NTFY_USE_DATA, &func, NTFY_DATA_PTR_NULL, &release_func) !=
	    NOTIFY_OK)
		return(notify_errno);
	(void) ndet_set_event_processing(nclient, 1);
	(void) func(nclient, event, arg, NOTIFY_SAFE);
	(void) ndet_set_event_processing(nclient, 0);
	/* Pop condition off interposition stack */
	nint_pop_callout();
	/* Release arg and event */
	if (release_func != NOTIFY_RELEASE_NULL)
		(void) release_func(nclient, arg, event);
	return(NOTIFY_OK);
}

extern Notify_error
notify_signal(nclient, sig)
	Notify_client nclient;
	int sig;
{
	Notify_func func;

	/* Check arguments and get function to call */
	if (ndet_check_sig(sig) ||
	    ndis_send_func(nclient, NTFY_SYNC_SIGNAL, (NTFY_DATA)sig,
	    NTFY_USE_DATA, &func, NTFY_DATA_PTR_NULL, NDIS_RELEASE_NULL) !=
	    NOTIFY_OK)
		return(notify_errno);
	(void) func(nclient, sig, NOTIFY_SYNC);
	/* Pop condition off interposition stack */
	nint_pop_callout();
	return(NOTIFY_OK);
}

extern Notify_error
notify_destroy(nclient, status)
	Notify_client nclient;
	Destroy_status status;
{
	Notify_func func;

	/* Check arguments and get function to call */
	if (ndet_check_status(status) ||
	    ndis_send_func(nclient, NTFY_DESTROY, NTFY_DATA_NULL,
	    NTFY_IGNORE_DATA, &func, NTFY_DATA_PTR_NULL, NDIS_RELEASE_NULL) !=
	    NOTIFY_OK)
		return(notify_errno);
	ndet_flags &= ~NDET_VETOED; /* Note: Unprotected */
	(void) func(nclient, status);
	/* Pop condition off interposition stack */
	nint_pop_callout();
	if (status == DESTROY_CHECKING) {
		if (ndet_flags & NDET_VETOED)
			return(NOTIFY_DESTROY_VETOED);
	} else {
		NTFY_BEGIN_CRITICAL;
		/* remove client (if client hasn't done so himself) */
		if (ntfy_find_nclient(ndet_clients, nclient,
		    &ndet_client_latest) != NTFY_CLIENT_NULL) {
			NTFY_END_CRITICAL;
			return(notify_remove(nclient));
		}
		NTFY_END_CRITICAL;
	}
	return(NOTIFY_OK);
}

extern Notify_error
notify_wait3(nclient)
	Notify_client nclient;
{
	Notify_func func;
	NTFY_WAIT3_DATA *wd;
	NTFY_CLIENT *client;

	/* Loop until no more wait conditions to notify */
	for (;;) {
		/*
		 * See if WAIT3 condition is pending.  This WAIT3
		 * notify proc differs from the others in this module
		 * because a client might have multiple wait3 events
		 * pending but the prioritizer only can tell if there
		 * is any waiting at all.  Thus we must search out ALL
		 * the WAIT3 conditions.
		 * Protect as critical section because list searches
		 * check to make sure that in critical.
		 */
		NTFY_BEGIN_CRITICAL;
		if ((client = ntfy_find_nclient(ndis_clients, nclient,
		      &ndis_client_latest)) == NTFY_CLIENT_NULL ||
		    ntfy_find_condition(client->conditions, NTFY_WAIT3,
		      &(client->condition_latest), NTFY_DATA_NULL,
		      NTFY_IGNORE_DATA) == NTFY_CONDITION_NULL) {
			NTFY_END_CRITICAL;
			break;
		}
		NTFY_END_CRITICAL;
		/* Get function to call */
		if (ndis_send_func(nclient, NTFY_WAIT3, NTFY_DATA_NULL,
		    NTFY_IGNORE_DATA, &func, (NTFY_DATA *)&wd,
		    NDIS_RELEASE_NULL) != NOTIFY_OK)
			return(notify_errno);
		(void) func(nclient, wd->pid, &wd->status, &wd->rusage);
		/* Free wait record */
		NTFY_BEGIN_CRITICAL;
		/* Pop condition off interposition stack */
		nint_unprotected_pop_callout();
		ntfy_free_malloc((NTFY_DATA)wd);
		NTFY_END_CRITICAL;
	}
	return(NOTIFY_OK);
}

static Notify_error
ndis_send_func(nclient, type, data, use_data, func_ptr, data_ptr,
    release_func_ptr)
	Notify_client nclient;
	NTFY_TYPE type;
	NTFY_DATA data;
	int use_data;
	Notify_func *func_ptr;
	NTFY_DATA *data_ptr;
	Notify_release *release_func_ptr;
{
	register NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;

	NTFY_BEGIN_CRITICAL;
	/* Find client */
	if ((client = ntfy_find_nclient(ndis_clients, nclient,
            &ndis_client_latest)) == NTFY_CLIENT_NULL) {
		ntfy_set_warning(NOTIFY_UNKNOWN_CLIENT);
		goto Error;
	}
	/* Find condition */
	if ((condition = ntfy_find_condition(client->conditions, type,
	    &(client->condition_latest), data, use_data)) ==
	    NTFY_CONDITION_NULL) {
		ntfy_set_warning(NOTIFY_NO_CONDITION);
		goto Error;
	}
	/* Remember function */
	*func_ptr = nint_push_callout(client, condition);
	/* Snatch condition data and null it */
	if (data_ptr) {
		*data_ptr = (NTFY_DATA)condition->data.an_u_int;
		condition->data.an_u_int = 0;
	}
	/* Remember release function so can use later and null it */
	if (release_func_ptr) {
		*release_func_ptr = condition->release;
		condition->release = NOTIFY_RELEASE_NULL;
	}
	/* Remove condition */
	ntfy_unset_condition(&ndis_clients, client, condition,
	    &ndis_client_latest, NTFY_NDIS);
	NTFY_END_CRITICAL;
	return(NOTIFY_OK);
Error:
	NTFY_END_CRITICAL;
	return(notify_errno);
}

pkg_private void
ndis_flush_condition(nclient, type, data, use_data)
	Notify_client nclient;
	NTFY_TYPE type;
	NTFY_DATA data;
	int use_data;
{
	register NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;

	NTFY_BEGIN_CRITICAL;
	for (;;) {
		/* Find client */
		if ((client = ntfy_find_nclient(ndis_clients, nclient,
		    &ndis_client_latest)) == NTFY_CLIENT_NULL)
			break;
		/* Find condition */
		if ((condition = ntfy_find_condition(client->conditions, type,
		    &(client->condition_latest), data, use_data)) ==
		    NTFY_CONDITION_NULL)
			break;
		/* Remove condition */
		ntfy_unset_condition(&ndis_clients, client, condition,
		    &ndis_client_latest, NTFY_NDIS);
	}
	NTFY_END_CRITICAL;
}

pkg_private void
ndis_flush_wait3(nclient, pid)
	Notify_client nclient;
	int pid;
{
	register NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;
	register NTFY_CONDITION *next;

	NTFY_BEGIN_CRITICAL;
	/* Find client */
	if ((client = ntfy_find_nclient(ndis_clients, nclient,
	    &ndis_client_latest)) == NTFY_CLIENT_NULL)
		goto Done;
	/* Find condition */
        for (condition = client->conditions; condition; condition = next) {
                next = condition->next;
                if (condition->type == NTFY_WAIT3 &&
		    condition->data.wait3->pid == pid) {
			/* Remove condition */
			ntfy_unset_condition(&ndis_clients, client, condition,
			    &ndis_client_latest, NTFY_NDIS);
			break;
		}
	}
Done:
	NTFY_END_CRITICAL;
}


