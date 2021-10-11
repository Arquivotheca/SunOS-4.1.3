#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_p_event.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_p_event.c - Notify_post_event implementation.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/ndis.h>
#include <sunwindow/nint.h>

static Notify_error ndet_p_event();

/*
 * Need to let when_hint == SAFE events through immediately as much as possible.
 * There are two ways that this could be done correctly:
 * 	1) Note when it is not safe and redirect events to delayed deliver
 *	if they come through during unsafe times.
 *	2) Note when it is safe and and redirect events to delayed deliver 
 *      if they come through during unsafe times.
 * We want to choose the lowest overhead approach.
 *
 * It is not safe during an interrupt or while another event is being processed
 * by the client.  We would like to have the prioritizer get a look at
 * all SAFE events, but this is not required.  After all, the prioritizer
 * doesn't look at destroy notifications.  If someone wants to look at all
 * events, then they had better interpose themselves in a client's event path.
 *
 * Where should we keep a PROCESSING_EVENT flag, the detector or the dispatcher.
 * The dispatcher knows, but the detector needs to know.
 */
extern Notify_error
notify_post_event(nclient, event, when_hint)
	Notify_client nclient;
	Notify_event event;
	Notify_event_type when_hint;
{
	return (notify_post_event_and_arg(nclient, event, when_hint,
	    NOTIFY_ARG_NULL, NOTIFY_COPY_NULL, NOTIFY_RELEASE_NULL));
}

extern Notify_error
notify_post_event_and_arg(nclient, event, when_hint, arg, copy_func,
    release_func)
	Notify_client nclient;
	Notify_event event;
	Notify_event_type when_hint;
	Notify_arg arg;
	Notify_copy copy_func;
	Notify_release release_func;
{
	NTFY_TYPE type;
	Notify_error return_code;
	Notify_value rc;

	/* Check arguments */
	if (ndet_check_when(when_hint, &type))
		return(notify_errno);
	/* Try send event to condition type that matches hint */
	return_code = ndet_p_event(nclient, event, type,
	    arg, copy_func, release_func, &rc);
	/*
	 * If tried to send immediate, there are two circumstances when
	 * we want to try to send safe.  One is when an immediate func
	 * could not be found.  The other is when the event was ignored.
	 *
	 * If tried to send safe, you only try to send immediately if
	 * there was no safe condition.
	 */
	if (return_code == NOTIFY_NO_CONDITION ||
	    (type == NTFY_IMMEDIATE_EVENT && rc == NOTIFY_IGNORED)) {
		/* Try send event to condition type that differs from hint */
		if ((return_code = ndet_p_event(nclient, event,
		    (type == NTFY_SAFE_EVENT)? NTFY_IMMEDIATE_EVENT:
		      NTFY_SAFE_EVENT, arg, copy_func, release_func, &rc)) ==
		    NOTIFY_NO_CONDITION)
			/*
			 * Set notify_errno again because NOTIFY_NO_CONDITION
			 * was originally just set as a warning in ndet_p_event.
			 */
			ntfy_set_errno(NOTIFY_NO_CONDITION);
	}
	return(return_code);
}

static
Notify_error
ndet_p_event(nclient, event, cond_type, arg, copy_func, release_func,
    rc)
	Notify_client nclient;
	Notify_event event;
	NTFY_TYPE cond_type;
	Notify_arg arg;
	Notify_copy copy_func;
	Notify_release release_func;
	Notify_value *rc;
{
	register NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;
	Notify_event_type now_type;

	NTFY_BEGIN_CRITICAL;
	/* Find client */
	if ((client = ntfy_find_nclient(ndet_clients, nclient,
            &ndet_client_latest)) == NTFY_CLIENT_NULL) {
		ntfy_set_errno(NOTIFY_UNKNOWN_CLIENT);
		goto Error;
	}
	/* Find condition that matches condition type */
	if ((condition = ntfy_find_condition(client->conditions, cond_type,
	    &(client->condition_latest), NTFY_DATA_NULL, NTFY_IGNORE_DATA)) ==
	    NTFY_CONDITION_NULL) {
		ntfy_set_warning(NOTIFY_NO_CONDITION);
		goto Error;
	}
	/* If have an immediate event condition then send it now */
	if (cond_type == NTFY_IMMEDIATE_EVENT) {
		Notify_client nclient_now;
		Notify_func func;

		now_type = NOTIFY_IMMEDIATE;
Now:
		/*
		 * Remember nclient because client->nclient may not be valid
		 * when return from func.
		 */
		nclient_now = client->nclient;
		/* While sending, protect client from recursive safe events */
		client->flags |= NCLT_EVENT_PROCESSING;
		/* Push condition on interposition stack */
		func = nint_push_callout(client, condition);
		NTFY_END_CRITICAL;
		*rc = func(nclient_now, event, arg, now_type);
		/* Pop condition from interposition stack */
		nint_pop_callout();
		/* Don't protect client from recursive safe events anymore */
		ndet_set_event_processing(nclient_now, 0);
	} else {
		/* If have a safe event and it is now safe then send it now */
		if (!(client->flags & NCLT_EVENT_PROCESSING) &&
		    !NTFY_IN_INTERRUPT) {
			now_type = NOTIFY_SAFE;
			goto Now;
		}
		/* Copy data into condition so can pass it to ndis_enqueue */
		condition->data.event = event;
		condition->release = release_func;
		if (copy_func == NOTIFY_COPY_NULL)
			condition->arg = arg;
		else {
			NTFY_END_CRITICAL;
			/* Note side affect on condition->data.event */
			condition->arg = copy_func(nclient, arg,
			    &condition->data.event);
			NTFY_BEGIN_CRITICAL;
			/* Check to see if client still exists */
			if (ntfy_find_nclient(ndet_clients, nclient,
			    &ndet_client_latest) != client) {
				ntfy_set_errno(NOTIFY_UNKNOWN_CLIENT);
				goto Error;
			}
			/* Check to see if condition still exists */
			if (ntfy_find_condition(client->conditions, cond_type,
			    &(client->condition_latest), NTFY_DATA_NULL,
			    NTFY_IGNORE_DATA) != condition) {
				ntfy_set_warning(NOTIFY_NO_CONDITION);
				goto Error;
			}
		}
		/* Enqueue condition for later delivery */
		if (ndis_enqueue(client, condition) != NOTIFY_OK) {
			ntfy_set_errno(NOTIFY_INTERNAL_ERROR);
			goto Error;
		}
		NTFY_END_CRITICAL;
		/* Dispatch the notification if not in notifier loop */
		if (!(ndet_flags & NDET_STARTED) &&
		    (ndis_dispatch() != NOTIFY_OK))
				return(notify_errno);
	}
	return(NOTIFY_OK);
Error:
	NTFY_END_CRITICAL;
	return(notify_errno);
}

pkg_private void
ndet_set_event_processing(nclient, on)
	Notify_client nclient;
	int on;
{
	register NTFY_CLIENT *client;

	NTFY_BEGIN_CRITICAL;
	/* Find client */
	if ((client = ntfy_find_nclient(ndet_clients, nclient,
            &ndet_client_latest)) == NTFY_CLIENT_NULL) {
		ntfy_set_warning(NOTIFY_UNKNOWN_CLIENT);
	} else {
		if (on)
			client->flags |= NCLT_EVENT_PROCESSING;
		else
			client->flags &= ~NCLT_EVENT_PROCESSING;
	}
	NTFY_END_CRITICAL;
}

