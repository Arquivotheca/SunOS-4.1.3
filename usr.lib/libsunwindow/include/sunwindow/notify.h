
/*	@(#)notify.h 1.1 92/07/30 SMI	*/
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef	NOTIFY_DEFINED
#define	NOTIFY_DEFINED

#include <sys/types.h>           /* to define fd_set */

/*
 * Opaque client handle.
 */
typedef	char * Notify_client;
#define	NOTIFY_CLIENT_NULL	((Notify_client)0)

/*
 * Opaque client event.
 */
typedef	char * Notify_event;

/*
 * Opaque client event optional argument.
 */
typedef	char * Notify_arg;
#define	NOTIFY_ARG_NULL	((Notify_arg)0)

/*
 * Client notification function return values for notifier to client calls.
 */
enum	notify_value {
	NOTIFY_DONE=0,		/* Handled notification */
	NOTIFY_IGNORED=1,	/* Did nothing about notification */
	NOTIFY_UNEXPECTED=2,	/* Notification not expected */
};
typedef	enum notify_value Notify_value;

/*
 * Error codes for client to notifier calls (returned when no other
 * return value or stored in notify_errno when other return value
 * indicates error condition).
 */
enum notify_error {
	NOTIFY_OK=0,		/* Success */
	NOTIFY_UNKNOWN_CLIENT=1,/* Client argument unknown to notifier */
	NOTIFY_NO_CONDITION=2,	/* Client not registered for given condition */
	NOTIFY_BAD_ITIMER=3,	/* Itimer type unknown */
	NOTIFY_BAD_SIGNAL=4,	/* Signal number out of range */
	NOTIFY_NOT_STARTED=5,	/* Notify_stop called & notifier not started */
	NOTIFY_DESTROY_VETOED=6,/* Some client didn't want to die when called
				   notify_die(DESTROY_CHECKING) */
	NOTIFY_INTERNAL_ERROR=7,/* Something wrong in the notifier */
	NOTIFY_SRCH=8,		/* No such process */
	NOTIFY_BADF=9,		/* Bad file number */
	NOTIFY_NOMEM=10,	/* Not enough core */
	NOTIFY_INVAL=11,	/* Invalid argument */
	NOTIFY_FUNC_LIMIT=12,	/* Too many interposition functions */
};
typedef	enum notify_error Notify_error;

extern	Notify_error notify_errno;

/*
 * Argument types
 */
enum	notify_signal_mode {
	NOTIFY_SYNC=0,
	NOTIFY_ASYNC=1,
};
typedef enum notify_signal_mode Notify_signal_mode;

enum	notify_event_type {
	NOTIFY_SAFE=0,
	NOTIFY_IMMEDIATE=1,
};
typedef enum notify_event_type Notify_event_type;

enum	destroy_status {
	DESTROY_PROCESS_DEATH=0,
	DESTROY_CHECKING=1,
	DESTROY_CLEANUP=2,
};
typedef enum destroy_status Destroy_status;

/*
 * A pointer to a function returning a Notify_value.
 */
typedef	Notify_value (*Notify_func)();

#define	NOTIFY_FUNC_NULL	((Notify_func)0)

/*
 * A pointer to a function returning a Notify_arg (used for client
 * event additional argument copying).
 */
typedef	Notify_arg (*Notify_copy)();

#define	NOTIFY_COPY_NULL	((Notify_copy)0)

/*
 * A pointer to a function returning void (used for client
 * event additional argument storage releasing).
 */
typedef	void (*Notify_release)();

#define	NOTIFY_RELEASE_NULL	((Notify_release)0)

/*
 * External references
 */
extern	struct itimerval NOTIFY_NO_ITIMER;	/* {{0,0},{0,0}} */
extern	struct itimerval NOTIFY_POLLING_ITIMER;	/* {{0,1},{0,1}} */

extern	Notify_error	notify_errno;
extern	void		notify_perror();
extern	Notify_value	notify_nop();
extern	Notify_value	notify_default_wait3();

extern	Notify_func	notify_set_input_func();
extern  Notify_func	notify_set_output_func();
extern  Notify_func	notify_set_exception_func();
extern  Notify_func	notify_set_itimer_func();
extern  Notify_func	notify_set_signal_func();
extern  Notify_func	notify_set_wait3_func();
extern  Notify_func	notify_set_destroy_func();
extern  Notify_func	notify_set_event_func();

extern	Notify_error	notify_interpose_input_func();
extern  Notify_error	notify_interpose_output_func();
extern  Notify_error	notify_interpose_exception_func();
extern  Notify_error	notify_interpose_itimer_func();
extern  Notify_error	notify_interpose_signal_func();
extern  Notify_error	notify_interpose_wait3_func();
extern  Notify_error	notify_interpose_destroy_func();
extern  Notify_error	notify_interpose_event_func();

extern	Notify_value	notify_next_input_func();
extern  Notify_value	notify_next_output_func();
extern  Notify_value	notify_next_exception_func();
extern  Notify_value	notify_next_itimer_func();
extern  Notify_value	notify_next_signal_func();
extern  Notify_value	notify_next_wait3_func();
extern  Notify_value	notify_next_destroy_func();
extern  Notify_value	notify_next_event_func();

extern  Notify_error	notify_itimer_value();
extern  Notify_error	notify_post_event();
extern  Notify_error	notify_post_event_and_arg();
extern  Notify_error	notify_post_destroy();
extern  Notify_error	notify_veto_destroy();

extern  Notify_error	notify_start();
extern  Notify_error	notify_dispatch();
extern  Notify_error	notify_stop();
extern	Notify_error	notify_die();
extern	Notify_error	notify_remove();
extern	Notify_error	notify_do_dispatch();
extern	Notify_error	notify_no_dispatch();

extern	Notify_func	notify_get_input_func();
extern	Notify_func	notify_get_output_func();
extern	Notify_func	notify_get_exception_func();
extern	Notify_func	notify_get_itimer_func();
extern	Notify_func	notify_get_signal_func();
extern	Notify_func	notify_get_wait3_func();
extern	Notify_func	notify_get_destroy_func();
extern	Notify_func	notify_get_event_func();

extern	Notify_error	notify_remove_input_func();
extern  Notify_error	notify_remove_output_func();
extern  Notify_error	notify_remove_exception_func();
extern  Notify_error	notify_remove_itimer_func();
extern  Notify_error	notify_remove_signal_func();
extern  Notify_error	notify_remove_wait3_func();
extern  Notify_error	notify_remove_destroy_func();
extern  Notify_error	notify_remove_event_func();

extern	Notify_func	notify_set_prioritizer_func();
extern	Notify_func	notify_set_scheduler_func();

extern	Notify_error	notify_input();
extern	Notify_error	notify_output();
extern	Notify_error	notify_exception();
extern	Notify_error	notify_itimer();
extern	Notify_error	notify_signal();
extern	Notify_error	notify_wait3();
extern	Notify_error	notify_event();
extern	Notify_error	notify_destroy();

extern	Notify_error	notify_client();

extern	Notify_func	notify_get_prioritizer_func();
extern	Notify_func	notify_get_scheduler_func();

extern	void		notify_flush_pending();

extern	int		notify_get_signal_code();
extern	struct sigcontext *	notify_get_signal_context();

/*
* FD manipulation funtions
*/
extern int ntfy_fd_cmp_and();
extern int ntfy_fd_cmp_or();
extern int ntfy_fd_anyset();
extern fd_set *ntfy_fd_cpy_or();
extern fd_set *ntfy_fd_cpy_and();
extern fd_set *ntfy_fd_cpy_xor();
/*
 * Mask bit generating macros (for prioritizer):
 */
#define	SIG_BIT(sig)	(1 << ((sig)-1))

/*
 * Debugging utility:
 */
typedef	enum notify_dump_type {
	NOTIFY_ALL=0,
	NOTIFY_DETECT=1,
	NOTIFY_DISPATCH=2,
} Notify_dump_type;
extern	void		notify_dump();

#endif	NOTIFY_DEFINED

