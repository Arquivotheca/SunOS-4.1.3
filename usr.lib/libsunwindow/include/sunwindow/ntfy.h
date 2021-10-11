/*	@(#)ntfy.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ntfy.h - Private header file for the implementation of the notifier.
 * Contains definitions common to the two major parts (dectector &
 * dispather) of the notifier.
 */

#ifndef	NTFY_DEFINED
#define	NTFY_DEFINED

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sunwindow/notify.h>

#ifdef	COMMENT
********************** Implementation Goals ****************************
Here are the major goals of the implementation of the notifier,
other than faithfully supporting the programming interface:

1) The implementation should be fast in common cases.  In particular,
the select, notify of input pending, notify of client event, back to
select again loop must be very efficient.

2) The implementation should be clean and general so that it
is easy to understand, extend and maintain.  This code will be
combed over heavily either to extend or debug it.

3) The implementation should be safe for asynchronous client event
notifications.  Reenterant calls must be safe, i.e., before
calling out to the client, data structures must be in a consistent state.


********************** Implementation Division *************************
The notifier will be divided internally into two basic parts:

1) The detector is responsible for noticing the occurrence of UNIX
events.  It maintains a complete list of clients that are awaiting
notification.  It maintains a per client list of conditions that the
client is awaiting.

2) The dispatcher is responsible for actual delivery of notifications
to clients.  It maintains a list of clients with notifications pending.
It maintains a per client queue of conditions that have already
happened.

The two parts of the notifier will share the same critical section
protections scheme (described below).

The public programming interface that each part of the notifier
supports is described in the each of two header files ndec.h & ndis.h.

Data and definitions shared between the detector and dispatcher
are contained in ntfy.h (ntfy_/NTFY_ name prefices).
Notifier public data structures and constants have
are contained in notify.h (notify_/NOTIFY_ name prefices).

#endif	COMMENT

/*
 * Ntfy_client defines the notifier's view of a client.
 */
typedef struct	ntfy_client {
	struct	ntfy_client *next;	/* Next client in list (this field must
					   be the first in the structure) */
	Notify_client nclient;		/* Client's private data handle */
	struct	ntfy_condition *conditions;
					/* Linked list of clients */
	struct	ntfy_condition *condition_latest;
					/* Hint when searching conditions */
	Notify_value (*prioritizer)();	/* Function to call to do notification
					   prioritization (defaults if null) */
	u_int	flags;			/* Per client boolean state */
#define	NCLT_EVENT_PROCESSING	0x01	/* Dispatcher in process of notifying
					   client of client event */
} NTFY_CLIENT;
#define	NTFY_CLIENT_NULL	((NTFY_CLIENT *)0)

/*
 * Ntfy_type defines the types of conditions that the notifier supports.
 */
typedef enum ntfy_type {
	NTFY_UNKNOWN=0,
	NTFY_INPUT=1,
	NTFY_OUTPUT=2,
	NTFY_EXCEPTION=3,
	NTFY_SYNC_SIGNAL=4,
	NTFY_ASYNC_SIGNAL=5,
	NTFY_REAL_ITIMER=6,
	NTFY_VIRTUAL_ITIMER=7,
	NTFY_WAIT3=8,
	NTFY_SAFE_EVENT=9,
	NTFY_IMMEDIATE_EVENT=10,
	NTFY_DESTROY=11,
} NTFY_TYPE;

/*
 * Ntfy_condition defines the notifier's view of a condition.
 * For conditions that have more than 32 bits of additional information,
 * dynamically allocate a node to hold this data.
 */
typedef struct ntfy_condition {
	struct	ntfy_condition *next;	/* Next condition for client (this field
					   must be the first in the structure)*/
	enum	ntfy_type type;		/* Type of condition */
	char	func_count;		/* Number of functions managing */
	char	func_next;		/* Next function to call */
	union	{			/* Switch on func_count > 1 */
		Notify_func function;	/* Notification function */
		Notify_func *functions;	/* Array of interposed functions */
#define	NTFY_FUNCS_MAX	(sizeof(NTFY_NODE)/sizeof(Notify_func))
	} callout;
	union {				/* Type specific data */
		int	fd;		/* NTFY_INPUT, NTFY_OUTPUT,
					   NTFY_EXCEPTION */
		int	signal;		/* NTFY_*_SIGNAL */
		u_int	an_u_int;	/* Generic unsigned int used for
					   instance matching */
		Notify_event event;	/* NTFY_*_EVENT */
		Destroy_status status;	/* NTFY_DESTROY */
		struct	ntfy_itimer *ntfy_itimer;
					/* NTFY_*_ITIMER (ndet only)
					   malloced struct */
		struct	ntfy_wait3_data *wait3;	/* NTFY_WAIT3 (ndis only)
					   malloced struct */
		int	pid;		/* NTFY_WAIT3 (ndet only) */
	} data;
	Notify_arg arg;			/* Event arg (ndis only) */
	Notify_release release;		/* arg release func (ndis only) */
} NTFY_CONDITION;
#define	NTFY_CONDITION_NULL	((NTFY_CONDITION *)0)
#define	NTFY_ITIMER_NULL	((struct itimerval *)0)
#define	NTFY_TIMEVAL_NULL	((struct timeval *)0)

/*
 * Ntfy_wait3_data is additional data for the NTFY_WAIT3 condition.
 */
typedef	struct ntfy_wait3_data {
	int	pid;			/* Process waiting for */
	union	wait status;		/* Return value from wait3 */
	struct	rusage rusage;		/* Return value from wait3 */
} NTFY_WAIT3_DATA;
#define	NTFY_WAIT3_DATA_NULL	((NTFY_WAIT3_DATA *)0)

/*
 * Ntfy_itimer is additional data for the NTFY_*_ITIMER condition.
 */
typedef	struct ntfy_itimer {
	struct	itimerval itimer;	/* REAL: Client passed in itimer */
					/* VIRTUAL: Client passed in itimer but
					   with it_value decremented to current
					   interval until expiration */
	struct	timeval set_tv;		/* REAL: Time-of-day when set (or reset)
					   the condition */
					/* VIRTUAL: Most recent interval to
					   which the process virtual itimer
					   was set */
} NTFY_ITIMER;

/*
 * Shared global private data
 */
extern	int ntfy_sigs_blocked;	/* count of nesting of signal blocking*/
extern	int ntfy_interrupts;	/* count of interrupts handling (0 or 1) */
extern	int ntfy_nodes_avail;	/* count of nodes available without having
				   to go to the system heap */
extern	u_int ntfy_sigs_delayed;/* Bit mask of signals received while in
				   critical section */

/*
 * Critical section protection macros.  A critical section is any section
 * of code that modifies or uses global data.  Details below.
 */
#define	NTFY_BEGIN_CRITICAL	ntfy_sigs_blocked++
#define	NTFY_IN_CRITICAL	(ntfy_sigs_blocked > 0)
#define	NTFY_END_CRITICAL   	ntfy_end_critical()
void	ntfy_end_critical();

/*
 * Interrupt detection macros.  Set when processing at signal interrupt
 * level.  Explanation below.
 */
#define	NTFY_BEGIN_INTERRUPT	ntfy_interrupts++
#define	NTFY_IN_INTERRUPT	(ntfy_interrupts > 0)
#define	NTFY_END_INTERRUPT   	ntfy_interrupts--

/*
 * Storage management definitions.  Details below.
 */
typedef	struct	ntfy_node {
	union {	/* List all data types that use the allocator */
		/* Extra data for NTFY_CONDITION */
		NTFY_ITIMER ntfy_itimer;
		/* Other structs that use nodes */
		NTFY_CLIENT client;
		NTFY_CONDITION condition;
		struct	ntfy_node *next;
	} n;
} NTFY_NODE;
#define	NTFY_NODE_NULL	((NTFY_NODE *)0)

typedef	enum	ntfy_enum {		/* Enumeration return values */
	NTFY_ENUM_NEXT=0,		/* Next condition please (normal) */
	NTFY_ENUM_TERM=1,		/* Enumeration terminated in middle */
	NTFY_ENUM_SKIP=2,		/* Skip rest of conditions in client */
} NTFY_ENUM;
typedef	NTFY_ENUM (*NTFY_ENUM_FUNC)();
#define	NTFY_ENUM_FUNC_NULL	((NTFY_ENUM_FUNC)0)
typedef	caddr_t	NTFY_ENUM_DATA;
#define	NTFY_ENUM_DATA_NULL	((NTFY_ENUM_DATA)0)
typedef	caddr_t	NTFY_DATA;
#define	NTFY_DATA_NULL		((NTFY_DATA)0)
#define	NTFY_DATA_PTR_NULL	((NTFY_DATA *)0)

#define	NTFY_PRE_ALLOCED_MIN	10	/* Min pre-allocated nodes that must
					   be available before notify interrupt
					   (asynchronous) signal handler */
#define	NTFY_SIGS_PER_LOOP	3	/* Reasonable number of signals that
					   might expect between each
					   notification loop */
#define	NTFY_PRE_ALLOCED	(NTFY_PRE_ALLOCED_MIN*NTFY_SIGS_PER_LOOP)
					/* Number of nodes kept pre-allocated */
#define	NTFY_NODE_BYTES		sizeof(NTFY_NODE)
					/* NTFY_NODE_BYTES is the node size
					   that use the storage allocator */
#define	NTFY_MIN_NODES		22	/* Reasonable number of conditions per
					   normal application */
#define	NTFY_NODES_PER_BLOCK	(NTFY_PRE_ALLOCED+(NTFY_MIN_NODES*2))
					/* Number of nodes allocated every time
					   go to heap (2 accounts for each
					   condition taking 2 nodes, one for
					   detector and one for dispatcher) */

/*
 * Node pool routines
 */
extern	NTFY_NODE *ntfy_alloc_node();	/* () */
extern	void ntfy_free_node();		/* (NTFY_NODE *node) */
extern	void ntfy_replenish_nodes();	/* () */

extern	char *ntfy_malloc();		/* (u_int size) */
extern	void ntfy_free_malloc();	/* (char *ptr) */
extern	void ntfy_flush_tb_freed();	/* () called from non-interrupt level */
/*
 * Ntfy_list_* routines need to be executed while data protected
 */
extern	void ntfy_append_node();	/* (NTFY_NODE **node_list, *new_node) */
extern	void ntfy_remove_node();	/* (NTFY_NODE **node_list, *node_axe) */
extern	void ntfy_free_list();		/* (NTFY_NODE **node_list) */

/*
 * Ntfy_client routines
 */
extern	NTFY_CLIENT *ntfy_find_nclient();/* (NTFY_CLIENT *client_list,
					    Notify_client nclient,
					    NTFY_CLIENT **client_latest) */
extern	NTFY_CLIENT *ntfy_new_nclient();/* Creates and inits client if can't
					   find (NTFY_CLIENT **client_list,
					    Notify_client nclient,
					    NTFY_CLIENT **client_latest) */
extern	void ntfy_remove_client();	/* Removes client from client_list
					   and makes sure that all conditions
					   all removed first
					   (NTFY_CLIENT **client_list,
					    NTFY_CLIENT *client,
					    NTFY_CLIENT **condition_latest,
					    enum ntfy_who who) */


#define	ntfy_alloc_client()		(NTFY_CLIENT *)ntfy_alloc_node()
#define	ntfy_free_client(client)	ntfy_free_node((NTFY_NODE *)(client))

#define	ntfy_append_client(client_list, client) \
			ntfy_append_node((NTFY_NODE **)(client_list), \
			    (NTFY_NODE *)(client))
#define	ntfy_free_client_list(client_list) \
			node_free_list((NTFY_NODE **)(client_list))

/*
 * Ntfy_condition_* routines
 */
extern	NTFY_CONDITION *ntfy_find_condition();/* Finds the 1st condition of
					   type.  If use_data then use data to
					   refine the match by comparing with
					   data.an_u_int.
					   (NTFY_CONDITION *condition_list,
					    NTFY_TYPE type,
					    NTFY_CONDITION **condition_latest,
					    NTFY_DATA data, int use_data) */
extern	NTFY_CONDITION *ntfy_new_condition();/* Creates condition if can't find.
					   Inits to data if use_data else 0.
					   (NTFY_CONDITION **condition_list,
					    NTFY_TYPE type,
					    NTFY_CONDITION **condition_latest,
					    NTFY_DATA data, int use_data) */
#define	NTFY_USE_DATA		1	/* Use_data flag value */
#define	NTFY_IGNORE_DATA	0	/* Use_data flag value */

extern	void ntfy_unset_condition();	/* Removes condition from client if
					   condition->function is null &
					   client from client_list if no
					   conditions in it.
					   (NTFY_CLIENT **client_list,
					    NTFY_CLIENT *client,
					    NTFY_CONDITION *condition,
					    NTFY_CLIENT **client_latest,
					    NTFY_WHO who) */
extern	void ntfy_remove_condition();	/* Removes condition from client
					   (NTFY_CLIENT *client,
					    NTFY_CONDITION *condition,
					    NTFY_WHO who) */
/*
 * Enumerates all conditions on client_list.  If enum_func returns
 * NTFY_ENUM_SKIP then go to next client (NTFY_CLIENT **client_list,
 * NTFY_ENUM_FUNC emum_func) Enum_func takes (NTFY_CLIENT *client,
 * NTFY_CONDITION *condition).  Normal return value is NTFY_ENUM_NEXT
 * but a NTFY_ENUM_TERM is possible if the enumeration terminated early.
 */
extern	NTFY_ENUM ntfy_paranoid_enum_conditions(); /* May remove any client or
					   condition during call.  But may not
					   be call recursively. */
extern	NTFY_ENUM ntfy_enum_conditions(); /* May remove condition during call.
					   If remove client during call then
					   don't return NTFY_ENUM_NEXT.
					   Can't remove any arbitrary
					   client or condition safely.
					   May be called recursively. */

typedef	enum ntfy_who {
        NTFY_NDET=0,
        NTFY_NDIS=1,
} NTFY_WHO;


#define	ntfy_alloc_condition()		(NTFY_CONDITION *)ntfy_alloc_node()
#define	ntfy_free_condition(condition)	ntfy_free_node((NTFY_NODE *)(condition))

#define	ntfy_append_condition(condition_list, condition) \
			ntfy_append_node((NTFY_NODE **)(condition_list), \
			    (NTFY_NODE *)(condition))
#define	ntfy_free_condition_list(condition_list) \
			node_free_list((NTFY_NODE **)(condition_list))

#define	ntfy_alloc_ntfy_itimer()	(NTFY_ITIMER *)ntfy_alloc_node()

#define	ntfy_alloc_functions()		(Notify_func *)ntfy_alloc_node()
#define	ntfy_free_functions(functions)	ntfy_free_node((NTFY_NODE *)(functions))
#define	NTFY_FUNC_PTR_NULL	((Notify_func *)0)

#ifdef	COMMENT
*************** Data Integrity and Storage Management ****************
The protection/integrity of notifier data structures from
asynchronous signal interference is done by "lightweight
blocking" of notifier related signals when accessing any of these
values.  The lightweight signal blocking mechanism is based on the
notion that if a signal arrives while the notifer is in a critical
section that its recognition can be delayed until the critical
section is left.

In order to be reentrant, data structures are not relied upon to be
unchanged between critical sections.  Critical sections are only
allowed within notifier code.  Critical sections cannot be in force
during blocking calls (e.g., select) or calls out (either from the
detector to the dispatcher or from the dispatcher to clients).

The notifier will use its own dynamic storage allocation mechanism.
It will be very fast by taking advantage of fixed size of nodes.
It will be safe by refusing to interact with the system heap during
signal interrupts.

A single fixed size block of data is managed by the storage manager.
Its size is the max of sizeof(NTFY_CLIENT)[12],
sizeof(NTFY_CONDITION)[16], and sizeof(NTFY_ITIMER)[24].
Typically, there will be approximately twice the number of
NTFY_CONDITION nodes compared to the total of all
the other nodes.  The 8 byte slop is judged to be acceptable
(the notifer could go to a multiple fixed size block mechanism if the
slop situation enlarges).  Allocating an fixed size node is done by 
taking a node off the free list.  If the free list is empty, then
the notifier allocates a large chunk from the system heap that is
subdivided into fixed size nodes.  Freeing an object is done by
putting it back on a linked list of available nodes.  The heap storage
used for this fixed size block mechanism is never returned to the system
heap.

Unfortunately, we can't cavalierly call the system heap from a signal
interrupt.  If we call the system heap while someone else is in the
process of allocating or freeing something then we are likely to destroy
the integrity of the heap.  We do three things to address this problem:

a) Keep an certain number, NTFY_PRE_ALLOCED, of nodes pre-allocated
to be available at interrupt time (more on this below).

b) Allocate the large chunk for NTFY_WAIT3_DATA (72 bytes) out of
the system heap.  By luck, the notifer only has to do this during
synchronous processing.

c) Return system heap nodes by placing them on a pending-free list.
The contents of the pending-free list is emptied and returned to
the system heap every time around the notification loop.  This is
to accomodate notify_remove being called from an interrupt.

The number of pre-allocated nodes is replenished back up to
NTFY_PRE_ALLOCED every time a critical section is exited.
Another number, NTFY_PRE_ALLOCED_MIN is the minimum number of
pre-allocated nodes that need be on hand before notifying during
a signal interrupt.  If this number is not on hand, then the
signal is scheduled to be handled when the next critical section
is exited.  Note, this delaying of interrupt signal handling is
the same approach used to protect notifier internal data structures.

If the available pre-allocated nodes run out during an interrupt
time notification then an error is returned to the caller saying
that his request could not be honored.  Thus, we need to make
NTFY_PRE_ALLOCED large.  Also, NTFY_PRE_ALLOCED_MIN should be public
knowledge so that interrupt time conditions can have some idea
about how many notify_set_*_func or notify_post_* calls can reliably
be made.  Note, we are putting a burden on clients to allocate
their minimum number of interrupt time notify_set_*_func and
notify_post_* calls among all their conditions (immediate client events
as well as asynchronous signal conditions).

#endif	COMMENT

/*
 * Debugging aids.
 */
#define	NTFY_DEBUG	1
/*
 * Ntfy_set_errno is for setting notify_errno when there is really something
 * wrong.  An error message is displayed with notifier code has been compiled
 * with NTFY_DEBUG.  Set the global int ntfy_errno_print to 0 if want to
 * surpress debugging messages.
 */
#ifdef	NTFY_DEBUG
#define	ntfy_set_errno(err)	ntfy_set_errno_debug((err))
void	ntfy_set_errno_debug();
#else	NTFY_DEBUG
#define	ntfy_set_errno(err)	notify_errno = err
#endif	NTFY_DEBUG
/*
 * Ntfy_set_warning is for setting notify_errno when you don't usually want
 * to generate a message; the caller may be using the call in a valid manner
 * and shouldn't be slapped in the hand with a debugging message (even when
 * NTFY_DEBUG is true).  Set the global int ntfy_warning_print to 1 if want
 * warning messages; must be compiled with NTFY_DEBUG as 1 too.
 */
#ifdef	NTFY_DEBUG
#define	ntfy_set_warning(err) ntfy_set_warning_debug((err))
void	ntfy_set_warning_debug();
#else	NTFY_DEBUG
#define	ntfy_set_warning(err) notify_errno = err
#endif	NTFY_DEBUG

#ifdef	NTFY_DEBUG
#define	ntfy_assert(bool, msg)	if (!(bool)) ntfy_assert_debug((msg))
void	ntfy_assert_debug();
#else	NTFY_DEBUG
#define	ntfy_assert(bool, msg)	{}
#endif	NTFY_DEBUG

void	ntfy_fatal_error();

#define	pkg_private	extern
#define	pkg_private_data

#endif	NTFY_DEFINED

