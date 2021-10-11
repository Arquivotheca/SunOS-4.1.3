/*	@(#)win_impl.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Defines sunwindow library PRIVATE data structures used in centralized
 * window event management.
 */

typedef	struct pixwin_handles {
	Pixwin		*pw;	/* Fd from pixwin */
	Notify_client	client;	/* May be same as pw */
	int		flags;	/* Defined in win_notify.h */
#define	PW_DELAYED_ENTER   0x0100/* Send LOC_RGNENTER very soon */
#define	PW_RESIZED   	   0x0200/* Have sent WIN_RESIZE to this client */
	struct pixwin_handles	*next;	/* Next pixwin in list */
} Pixwin_handles;
#define	PIXWIN_HANDLES_NULL	((Pixwin_handles *)0)

typedef	struct window_handles {
	Pixwin_handles	*next;	/* List of pixwins in window */
	int		flags;
#define	WH_SIZE_CHANGED   0x01	/* Indicates that our user process changed size
				   of window (and thus need to treat next
				   SIGWINCH as a size change situation) */
#define	WH_GRABBED_INPUT  0x02	/* Indicates current gets next input no matter
				   where the cursor is */
#define	WH_OWN_SIGWINCH   0x04	/* Window's sigwinch processing being handled
				   by client */
#ifndef PRE_FLAMINGO
#define WH_NOTIFY_ALL	  0x08	/* Send REPAINT on all sigwinches */
#endif
	Pixwin_handles	*current;/* Current pixwin that locator is in */
	Pixwin_handles	*latest;/* Latest pixwin client to be sent input */
	Rect	 	rect;	/* User process local notion of win rect */
	Event		event;	/* Last event read from kernel */
} Window_handles;
#define	WINDOW_HANDLES_NULL	((Window_handles *)0)

