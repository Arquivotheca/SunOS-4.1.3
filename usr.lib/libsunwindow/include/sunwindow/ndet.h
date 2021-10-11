/*	@(#)ndet.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985, 1988 by Sun Microsystems, Inc.
 */

/*
 * Ndet.h - Private header file for the detector part of the notifier.
 * The detector is responsible for noticing the occurrence of UNIX
 * events.  It maintains a complete list of clients that are awaiting
 * notification.  It maintains a per client list of conditions that the
 * client is awaiting.
 */

#ifndef	NDET_DEFINED
#define	NDET_DEFINED

#ifdef	COMMENT

********************** Detector Loop Notes ****************************
Here is some notes on the detector loop:

1) Any time that a notification changes (new, added, removed, modified),
the appropriate "changed" constant is ored into ndet_flags (see below). 
To determine the things that the notifier needs to ask UNIX
to watch for (fd activity, signals, itimers) requires scanning
all clients and notifications.  Comparing the result of this scan
with what the notifier is already watching for determines what work
has to be done to change what UNIX is doing.

2) A cycle of the notification loop can be entered in a variety of ways.

a) Notify_start can be called by the top level of the application.

b) When select is called, if notifications are being dispatched then
a real select system call is made.  Otherwise, a private notifier
client is generated that sets conditions equivalent to the select's
arguments.  Result bit masks and itimer expired flags are initialized.
Notify_start is then called.  If any of the select client's routines
are called, result bit masks and/or itimer expired flags are set.
In addition, notify_stop is called.  When notify_start returns, its
error code, errno, the result bit masks and/or itimer expired flags
are used to generate return values.

c) When read is called, if notifications are being dispatched then
a real read system call is made.  Otherwise, a private notifier
client is generated that sets an input pending condition for the read
fd.  A input pending flag is initialized.  Notify_start is then called.
If the read client's input pending routine is called, the input
pending flag is set.  In addition, notify_stop is called.  When
notify_stop returns, its error code, errno and the input pending flag
are used to figure out if a real read system call is to be done.
If so, a real read system call is done and the results returned.


********************** Public Interface Supporting *********************
The public programming interface that the detector supports follows:

notify_set_input_func
notify_set_output_func
notify_set_exception_func
notify_set_itimer_func
notify_set_signal_func
notify_set_wait3_func
notify_set_destroy_func
notify_set_event_func
notify_set_prioritizer_func

notify_get_itimer_value
notify_post_event
notify_post_destroy
notify_veto_destroy

notify_start
notify_stop
notify_die
notify_remove

fcntl
read
select

notify_get_input_func
notify_get_output_func
notify_get_exception_func
notify_get_itimer_func
notify_get_signal_func
notify_get_wait3_func
notify_get_destroy_func
notify_get_event_func
notify_get_prioritizer_func

#endif	COMMENT

/*
 * The detector uses ndet_/NDET_ name prefices.
 */

/*
 * Detector global data
 */
extern	u_int	ndet_flags;		/* Flags */
#define	NDET_STOP		0x01	/* Ntfy_stop called */
#define	NDET_FD_CHANGE		0x02	/* A fd condition changed */
#define	NDET_SIGNAL_CHANGE	0x04	/* A signal condition changed */
#define	NDET_REAL_CHANGE	0x08	/* A real itimer condition changed */
#define	NDET_VIRTUAL_CHANGE	0x10	/* A virtual itimer condition changed */
#define	NDET_WAIT3_CHANGE	0x20	/* A wait3 condition changed */
#define	NDET_DISPATCH		0x40	/* Calling ndis_dispatch (used to know
					   if should do real or notifier
					   version of read and select) */
#define	NDET_REAL_POLL		0x80	/* Real itimer wants to poll, invalid if
					   ndet_flags & NDET_REAL_CHANGE */
#define	NDET_VIRTUAL_POLL	0x100	/* Virtual itimer wants to poll, invalid
					   if ndet_flags & NDET_VIRTUAL_CHANGE*/
#define	NDET_POLL		(NDET_REAL_POLL|NDET_VIRTUAL_POLL)
					/* Do polling in select */
#define	NDET_INTERRUPT		0x200	/* Set when handling a signal interrupt
					   so that know not to go to system
					   heap */
#define	NDET_STARTED		0x400	/* In ntfy_start */
#define	NDET_EXIT_SOON		0x800	/* Notifier auto SIGTERM triggered,
					   exit(1) after finish dispatch */
#define	NDET_STOP_ON_SIG	0x1000	/* Notifier select client wants to break
					   if get a signal during real select */
#define	NDET_VETOED		0x2000	/* Notify_veto_destroy called */
#define	NDET_ITIMER_ENQ		0x4000	/* Itimer notification enqueued */
#define	NDET_NO_DELAY		0x8000	/* Caller of notify_start wants single
					   time around the loop */
#define	NDET_DESTROY_CHANGE	0x10000	/* A destroy condition changed */
#define	NDET_CONDITION_CHANGE	(NDET_FD_CHANGE|NDET_SIGNAL_CHANGE|\
    NDET_REAL_CHANGE|NDET_VIRTUAL_CHANGE|NDET_WAIT3_CHANGE|NDET_DESTROY_CHANGE)
					/* Combination of condition changes */

extern	NTFY_CLIENT *ndet_clients;	/* Active clients */
extern	NTFY_CLIENT *ndet_client_latest;/* Latest Notify_client=>NTFY_CLIENT
					   conversion success: for fast lookup
					   (nulled when client removed) */

extern	fd_set	ndet_fndelay_mask;	/* Mask of non-blocking read fds
					   (maintained by fcntl) */
extern	fd_set	ndet_fasync_mask;	/* Mask of fds that generate SIGIO
					   when data ready (maintained by
					   fcntl) */

extern	fd_set	ndet_ibits, ndet_obits, ndet_ebits;
					/* Select bit masks (invalid if
					   ndet_flags & NDET_FD_CHANGE) */
extern	struct	timeval ndet_polling_tv;/* Tv with select type polling value */

extern	u_int	ndet_sigs_auto;		/* Bits that indicate which signals the
					   notifier is automatically catching
					   (SIGIO, SIGURG, SIGCHLD, SIGTERM,
					   SIGALRM, SIGVTALRM) */
extern	void	ndet_toggle_auto();	/* When some condition that the notifier
					   is interested in changes, call this
					   to adjust ndet_sigs_auto (TBD reword)
					   (u_int obld_bits, sig) */
extern	Notify_value ndet_auto_sig_func();/* (Notify_client nclient,
					    int signal, Notify_signal_mode) */
extern	Notify_client ndet_auto_nclient;/* Private notifier client for auto
					   signal handling */
extern	NTFY_ENUM ndet_auto_sig_send();	/* Tell auto sig manager that one of
					   its condition has occurred
					   (NTFY_CLIENT *client,
					    NTFY_CONDITION *condition,
					    NTFY_ENUM_DATA context) */

extern	u_int	ndet_sigs_managing;	/* Signals that are managing (invalid if
					   ndet_flags & NDET_SIGNAL_CHANGE) */
extern	u_int	ndet_sigs_received;	/* Records signals received */
extern	void	ndet_enable_sig();	/* Call this routine (other than from
					   ndet_fig_sig_change) when you need
					   to make sure that a signal is being
					   caught but don't want to go through
					   the whole process of globally finding
					   out who else needs it. (u_int sig) */
extern	void	ndet_send_delayed_sigs();/* Process any async signal conditions
					   that have may have accumulated during
					   a critical section. */

extern	Notify_func ndet_set_fd_func();	/* (Notify_client nclient,
					    Notify_func func, int fd
					    NTFY_TYPE type) */
extern	Notify_func ndet_get_fd_func();	/* (Notify_client nclient,
					    int fd, NTFY_TYPE type) */
extern	Notify_func ndet_get_func();	/* (Notify_client nclient,
					    NTFY_TYPE type, NTFY_DATA data,
					    int use_data) */
extern	ndet_check_fd();		/* Returns 0 if all OK else -1 and
					   sets notify_errno to NOTIFY_BADF
					   (int fd) */

extern	ndet_check_sig();		/* Returns 0 if all OK else -1 and sets
					   notify_errno to NOTIFY_BAD_SIGNAL
					   (int sig) */
extern	ndet_check_mode();		/* Returns 0 if all OK else -1 and sets
					   notify_errno to NOTIFY_INVAL.  Sets
					   type_ptr if not null.
					   (Notify_signal_mode mode,
					    NTFY_TYPE *type_ptr) */

extern	ndet_check_which();		/* Turn itimer which into type.
					   Returns 0 if all OK else -1 and sets
					   notify_errno to NOTIFY_INVAL.  Sets
					   type_ptr if not null.
					   (int which, NTFY_TYPE *type_ptr) */
#define	ndet_tv_equal(_a, _b)	\
	(_a.tv_sec == _b.tv_sec && _a.tv_usec == _b.tv_usec)
#define	ndet_tv_polling(tv) ndet_tv_equal((tv), NOTIFY_POLLING_ITIMER.it_value)
extern	struct timeval ndet_tv_subt();	/* Subtracts b from a.  Will round down
					   any NOTIFY_POLLING_ITIMER.it_value
					   results to {0,0}.  This is to prevent
					   the notifier from generating a
					   polling timer
					   (struct timeval a, b) */
extern	struct timeval ndet_tv_min();	/* Find min of b and a.
					   (struct timeval a, b) */

extern	struct timeval ndet_real_min();	/* Figure the interval that has
					   transpired since this real interval
					   timer has been set.  The difference
					   between how much time the timer wants
					   to wait and how long it has waited is
					   the amount of time left to wait.
					   The time left to wait is returned.
					   (NTFY_ITIMER *ntfy_itimer,
					    struct timeval current_tv) */
extern	struct timeval ndet_virtual_min(); /* Update the interval until
					    expiration by subtracting the amount
					    of time on the process interval
					    timer (current_tv) from the value of
					    the process interval timer when it
					    was last looked at by this client
					    (ntfy_itimer->set_tv).
					    Return the amount of time that this
					    virtual interval timer has to go
					    before expiration
					    (ntfy_itimer->itimer.it_value).
					    Need to update ntfy_itimer->set_tv
					    with this value after calling
					    ndet_virtual_min.
					    (NTFY_ITIMER *ntfy_itimer,
					     struct timeval current_tv) */
extern	void ndet_reset_itimer_set_tv();/* Reset ntfy_itimer->set_tv based on
					   type (NTFY_CONDITION *condition) */

/*
 * NDET_ENUM_SEND is used to send the results of select around to conditions.
 */
typedef struct ndet_enum_send {
	fd_set	ibits;			/* Input devices selected */
	fd_set	obits;			/* Output devices selected */
	fd_set	ebits;			/* Exception devices selected */
	u_int   sigs;			/* Signals to process */
	NTFY_WAIT3_DATA *wait3;		/* Results of wait3 system call */
	struct  timeval current_tv;	/* NTFY_REAL_ITIMER time-of-day (now)
					   NTFY_VIRTUAL_ITIMER current itimer
					   it_value */
} NDET_ENUM_SEND;

/* 
 * When recomputing itimers, NDET_ENUM_ITIMER is used to pass the context 
 * for the real or virtual itimer around. 
 */ 
typedef	struct  ndet_enum_itimer {
	int		enqueued;	/* Not zero if enqd notification */
	NTFY_TYPE	type;		/* One of NTFY_*_ITIMER */
	u_int		polling_bit;	/* One of NDET_*_POLL */
	int		signal;		/* SIGALRM | SIGVTALRM */
	int		which;		/* VIRTUAL_ITIMER | REAL_ITIMER */
	struct timeval	(*min_func)();	/* Returns the interval that a given
					   itimer needs to wait until expiration
					   (virtual itimer resets set_time) */
	Notify_value	(*expire_func)();/* Called when itimer expiration
					   detected */
	struct timeval	current_tv;	/* Real is time-of-day, virtual is
					   current virtual itimer value */
	struct timeval	min_tv;		/* Global min of min_func return value*/
} NDET_ENUM_ITIMER;
extern	struct timeval	ndet_virtual_min();
extern	struct timeval	ndet_real_min();
extern	void ndet_update_virtual_itimer();/* Some virtual itimer related
					   condition has changed.  Update all
					   virtual itimers.  Determine minimum
					   wait and set virtual itimer.  Enable
					   (disable) notifier auto signal
					   catching of SIGVTALRM. (int send) */
extern	void ndet_update_real_itimer();	/* Some real itimer related condition
					   has changed.  Determine minimum wait
					   and set real itimer.  Enable(disable)
					   notifier auto signal catching of
					   SIGALRM. (int send) */
extern	NTFY_ENUM ndet_fd_send();	/* Enqueue notifications for any fds
					   in (NDET_ENUM_SEND *) context.
					   (NTFY_CLIENT *client,
					    NTFY_CONDITION *condition,
					    NTFY_ENUM_DATA context) */

extern	void ndet_set_event_processing();/* Called from dispatcher to tell
					   detector that nclient is in the
					   process of handling a client event
					   (on == 1) or done handling a client
					   event (on == 0).
					   (Notify_client nclient, int on) */
#ifdef	COMMENT
********************** Interval Timer Algorithms *************************
For both types of interval timers:
1) When an interval timer expires for a client, the reset value is used
to set the interval value.

2) The smallest interval of all clients is used to reset the process
interval timer.

3) Before recomputing a process timer, use the TIME_DIF (see below)
to update it_value in all the clients.  Any it_values <= zero are sent
notifications.

4) When a process timer expires, simply recomputing the process timer
has the side effect of sending notifications.

Note: Polling is inefficient if not special cased.  Polling is separate
from interval timer computation.  Polling is implemented as something
related to the select timer.

4) Caution:

	a) Signals being received while recomputing interval causing
	   duplicate notifications.

	b) Granularity of very short intervals may cause overlap
	   of notification and determining the next one.

For process virtual time interval timers TIME_DIF is the value used
to set the process timer.

Computation of real time interval timers differs from process virtual
time interval timers in that the real timer interval timer must be exact
enough so that if a client sets it to be the difference between now and
5:00am that he would be notifier at 5:00am.  To avoid cummulative delay
errors, we make real time interval timers relative to the time of day at
which they were set.  Thus, when setting a client timer
(ntfy_set_itimer_func or resetting from it_value), note the tod
(time-of-day) and the original it_value.  TIME_DIF is the difference
from the current tod and the original tod.  The original it_value-
TIME_DIF gives the current it_value.


********************** Handling Supplementary Signals ********************
In notification loop
1) Find fd that is async.
2) Set SIGIO signal catcher with ndet_sigio as function.
3) Ndet_sigio will search all conditions in its client for fds which match
ndet_fasync_mask.
4) Any that match have a FIOCNREAD done to see if any input for that fd.
5) Send notification if so, else ignore.
********************** Clients Considerations ****************************
Changing a condition during a notification:

1) Changes can come any time, synchronously or asynchronously,
as long as they are notification derived, i.e., a client didn't
catch a signal and call the notifier himself. 

2) Condition changes affect what will be waited on next time through
the notication loop.  If you remove a condition asynchronously then
it wouldn't be notified.  If you add a condition asynchronously that
happens to be selected on this time already then you may get a
notification this time around the notication loop instead of next time.

3) Multiple changes to the same condition are applied immediately, i.e.,
the last change that the notifier got will be the current one.

4) A notify_post_destroy or notify_die command applies immediately.
Callers of these routines should do so during synchronous processing.
To do so otherwise is an error (NOTIFY_CANT_INTERRUPT).  This is because
the notifier refuses to call out to a client that may be in an unsafe
condition unless the client has stated explicitely that it will
worry about safety himself, e.g., asynchronously signals and
immediate client events.

#endif	COMMENT

#endif	NDET_DEFINED

