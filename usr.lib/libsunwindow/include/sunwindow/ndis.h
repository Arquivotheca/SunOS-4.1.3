/*	@(#)ndis.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndis.h - Private header file for the dispatcher part of the notifier.
 * The dispatcher is responsible for actual delivery of notifications
 * to clients.  It maintains a list of clients with notifications pending.
 * It maintains a per client queue of conditions that have already happened.
 */

#ifndef	NDIS_DEFINED
#define	NDIS_DEFINED

#ifdef	COMMENT

********************** Dispatcher Loop Notes ****************************
Each dispatcher client has a queue of conditions pending notification.
This is a scenario of actually doing the notification:

0) A condition is queued up by the detector.  If the condition is
immediate or asynchronous, it is delivered now.  Otherwise, the
condition is queued up on the client's queue.  Queuing up a client
event sets a dispatcher flag (see NDIS_EVENT_QUEUED below).

1) Later, the scheduler is called with a list of clients that have
non-empty queues.  This list is compiled by linearly sweeping the
dispatcher's client list.  Typically, this list is one or two clients
long.

2) The arguments for a client's prioritizer call are compiled.  The
arguments are initialized to empty (6 assignments).  The condition
queue is traversed and the appropriate entries are made in the arguments.
Typically, the queue is one or two entries long.  NDIS_EVENT_QUEUED
is reset.  The call to the prioritizer is made, usually this is the
default prioritizer.

3) The default prioritizer does 6 comparisions everytime to notice
the general area of notification required.  It sometimes iterates over
the non-zero bit fields to find out the specifics of what to notify.

4) A notify_* (say input) call is made.  The dispatcher needs to turn
the client handle in to a queue pointer, search the queue, remove the
queue entry and make the notification.  Any of these steps can generate
an error or ignored condition.

5) If NDIS_EVENT_QUEUED is set after returning from a prioritizer,
go back to step 2 if client is not NULL.  Otherwise, the scheduler
goes on to the next client after NULLing out the client slot that
he is done running.

Reentrant usage of data structures (client list, client queues,
NDIS_EVENT_QUEUED) is done by only changing them during normal
synchronous processing.  Also, these data structures are not relied
upon to be unchanged between calls out to clients.


********************** Public Interface Supporting *********************
The public programming interface that the dispatcher supports follows:

notify_set_scheduler_func

notify_input
notify_output
notify_exception
notify_itimer
notify_signal
notify_wait3
notify_event

notify_client

prioritizer_func
scheduler_func
input_func
output_func
exception_func
itimer_func
signal_func
wait3_func
destroy_func
event_func

notify_get_scheduler_func

#endif	COMMENT

/*
 * Dispatcher global data.  The dispatcher uses ndis_/NDIS_ name prefices.
 */
extern	u_int	ndis_flags;		/* Flags */
#define	NDIS_EVENT_QUEUED	0x01	/* A safe client event has been queued
					   and therefore notify_client should
					   try rerunning the currently running
					   prioritizer (if any) */
#define	NDIS_CLIENT_FLUSHED	0x02	/* A notify_flush_pending was done.
					   If in the middle of a call out of the
					   dispatcher, ignore TBD */

extern	NTFY_CLIENT *ndis_clients;	/* Clients with notifications pending */
extern	NTFY_CLIENT *ndis_client_latest;/* Latest Notify_client=>NTFY_CLIENT
					   conversion success: for fast lookup
					   (nulled if client flushed) */

extern	Notify_value (*ndis_scheduler)(); /* The scheduler */
extern	Notify_value ndis_default_scheduler(); /* The default scheduler */
extern	Notify_value ndis_default_prioritizer(); /* Default prioritizer */

/*
 * The following is part of the private interface to the dispatcher
 * that the detector calls:
 */
extern	Notify_error ndis_enqueue();	/* Detector calls to q notification
					   (takes a ntfy_client followed by a
					   ntfy_condition) */
extern  Notify_error ndis_dispatch();	/* Dispatch to all clients according
					   to the scheduler */

void	ndis_flush_condition();		/* Flush condition from dispatcher q
					   (Notify_client client,
					    NTFY_TYPE type, NTFY_DATA data,
					    int use_data) */
void	ndis_flush_wait3();		/* Flush all conditions for pid
					   (Notify_client client, int pid) */
#endif	NDIS_DEFINED

