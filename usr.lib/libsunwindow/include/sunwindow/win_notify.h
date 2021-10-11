/*	@(#)win_notify.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * SunWindows related notification definitions (see also notify.h).
 */

/* Flags for win_register call */
#define	PW_RETAIN	 0x1	/* Manage retained window (move to pixwin) */
#define	PW_FIXED_IMAGE	 0x2	/* Treat retained image as fixed size 
				   (move to pixwin) */
#define	PW_INPUT_DEFAULT 0x4	/* Input sink when cant be determined by x/y */
#define	PW_NO_LOC_ADJUST 0x8	/* Don't adjust x y to rgn coordinate space */
#define	PW_REPAINT_ALL	 0x10	/* Repaint entire pw on repaint */

/* Pixwin registration with SunWindows notification support routines */
extern	int win_register();			/* (Notify_client client,
						    Pixwin *pw,
						    Notify_func event_func,
						    Notify_func destroy_func,
						    u_int flags) */
extern	int win_unregister();			/* (Notify_client client) */
extern	int win_set_flags();			/* (Notify_client client,
						    u_int flags) */
extern	u_int win_get_flags();			/* (Notify_client client) */

/* Extraction of interesting values from window notifier clients */
extern	int win_get_fd();			/* (Notify_client client) */
extern	Pixwin *win_get_pixwin();		/* (Notify_client client) */

/* Posting of client events to window notifier clients */
extern	Notify_error win_post_id();		/* (Notify_client client,
						    short id,
						    Notify_event_type when) */
extern	Notify_error win_post_id_and_arg();	/* (Notify_client client,
						    short id,
						    Notify_event_type when,
						    Notify_arg arg,
						    Notify_copy copy_func,
						    Notify_release release_func)
						 */
extern	Notify_error win_post_event();		/* (Notify_client client,
						    Event *event,
						    Notify_event_type when) */
extern	Notify_error win_post_event_arg();	/* (Notify_client client,
						    Event *event,
						    Notify_event_type when,
						    Notify_arg arg,
						    Notify_copy copy_func,
						    Notify_release release_func)
						 */

/* Utilities to call if posting with win_post_id_and_arg or win_post_event_arg*/
extern	Notify_arg win_copy_event();		/* (Notify_client client,
						    Notify_arg arg,
						    Event **event_ptr) */
extern	void win_free_event();			/* (Notify_client client,
						    Notify_arg arg,
						    Event *event) */
