/*      @(#)win_lock.h 1.1 92/07/30 SMI      */


/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* Note: this file is private to the pixwin package and kernel.
 */

#ifndef WIN_LOCK_DEFINED
#define	WIN_LOCK_DEFINED

#define	WIN_LOCK_VERSION	4

/* cpu types */
#define	WIN_LOCK_68000		1
#define	WIN_LOCK_RISC		2

#define	WIN_LOCK_MAX_DTOP	10	/* max desktops supported */
#define	WIN_LOCK_MAX_PLANES	32	/* max planes supported */

#ifdef ecd.cursor
#define WIN_LOCK_CURSOR_MAX_IMAGE_WORDS CURSOR_MAX_IMAGE_WORDS
                /* Now supporting double (relative to SunOS 4.0) cursors */
#else
#define	WIN_LOCK_CURSOR_MAX_IMAGE_WORDS	4*CURSOR_MAX_IMAGE_WORDS
		/* 4xs for eventual cursor that is double the size */
#endif ecd.cursor

typedef struct win_shared_cursor {
	struct	cursor	cursor;	/* current cursor */
	int	x;		/* cursor screen relative x position */
	int	y;		/* cursor screen relative y position */
	int	plane_group;	/* plane group that cursor belongs to */

	/* cursor flags */
	unsigned int	cursor_active	: 1;	/* cursor is over the dtop */
	unsigned int	cursor_is_up	: 1;	/* cursor is drawn */
	unsigned int	spare3		: 1;	/* spare bits */
	unsigned int	spare4		: 1;
	unsigned int	spare5		: 1;
	unsigned int	spare6		: 1;	/* cursor_active_cache */
	unsigned int	spare7		: 1;	/* hardware_cursor */
#define	cursor_active_cache	spare6	/* Set if cursor is on this desktop */
#define	hardware_cursor		spare7	/* Set if doing hw cursor tracking */

	struct pixrect	pr;    		/* cursor image under cursor */
	struct	mpr_data pr_data;	/* cursor pr_data */
	short	image[WIN_LOCK_CURSOR_MAX_IMAGE_WORDS];
					/* cursor pr_data.md_image */

	struct pixrect	screen_pr;    	/* screen image under cursor */
	struct mpr_data screen_pr_data;	/* screen pr_data under cursor */
	short screen_image[WIN_LOCK_CURSOR_MAX_IMAGE_WORDS*WIN_LOCK_MAX_PLANES];

	/* for future expansion */
	int	sparea;
	int	spareb;
	int	sparec;
	int	spared;
	short	spareA[WIN_LOCK_CURSOR_MAX_IMAGE_WORDS];
					/* future expansion for cursor mask */
	short	spareB[WIN_LOCK_CURSOR_MAX_IMAGE_WORDS];
	short	spareC[WIN_LOCK_CURSOR_MAX_IMAGE_WORDS];
} Win_shared_cursor;


typedef struct win_lock_status {
	unsigned int	update_cursor		: 1;
	unsigned int	update_cursor_active	: 1;
	unsigned int	update_mouse_xy		: 1;
	unsigned int	new_cursor	: 1;	/* cursor should be re-drawn */
	unsigned int	spare1		: 1;	/* spare bits */
	unsigned int	spare2		: 1;
	unsigned int	spare3		: 1;
	unsigned int	spare4		: 1;
	unsigned int	spare5		: 1;
} Win_lock_status;

typedef	struct	win_lock {
	int	lock;		/* used for test & set */
	int	pid;		/* process that last/currently held lock */
	int	id;		/* id when lock was taken */
	int	count;		/* unlock when count == 0 */
} Win_lock;

/* The shared event queue */

typedef struct win_shared_eventqueue {
	struct inputevent *events;	/* input event buffer */
	int size;			/* size of event buffer */
	int head;			/* index into events */
	int tail;			/* index into events */
} Win_shared_eventqueue;

typedef	struct	win_lock_block {
	int		version;	/* for old code / new code check */
	int		cpu_type;	/* test and set procedure agreement */
	Win_lock 	mutex;		/* semaphore for mutual exclusion */
	Win_shared_cursor cursor_info;	/* shared cursor data */
	Win_lock 	display;	/* display lock info */

	/* following are written only by the kernel */
	int		waiting;	/* !0 if process waiting for a lock */
	int		go_to_kernel;	/* !0 if kernel wants to do all */
	Win_lock 	data;		/* window database is locked */
	Win_lock	grabio;		/* grabio is active */
	Win_lock_status	status;		/* status info for updates */
	int		clip_ids[256];	/* clip ids of all possible windows */
	int		mouse_x;	/* mouse screen relative x position */
	int		mouse_y;	/* mouse screen relative y position */
	int		mouse_plane_group;/* plane group should draw in next */
	Win_shared_eventqueue shared_eventq;/* shared event q for X */
	int		spare5;
} Win_lock_block;


/* fixup a pixrect to point to its parts */
#define	win_lock_fixup_pixrect(pixrect, mpr, image_data)	\
	{ extern struct pixrectops mem_ops; \
	  (pixrect).pr_ops = &mem_ops; \
	  (pixrect).pr_data = (caddr_t) &(mpr); \
	  (mpr).md_image = (image_data); \
	}

#define win_lock_prepare_screen_pr(cursor_info)	\
	win_lock_fixup_pixrect((cursor_info)->screen_pr, \
	    (cursor_info)->screen_pr_data, (cursor_info)->screen_image)

#define win_lock_prepare_cursor_pr(cursor_info)	\
	win_lock_fixup_pixrect((cursor_info)->pr, (cursor_info)->pr_data, \
	    (cursor_info)->image)

#define	win_lock_locked(which_lock)	((which_lock)->lock)

#define	win_lock_mutex_locked(wl)	win_lock_locked(&((wl)->mutex))
#define	win_lock_grabio_locked(wl)	win_lock_locked(&((wl)->grabio))
#define	win_lock_data_locked(wl)	win_lock_locked(&((wl)->data))
#define	win_lock_display_locked(wl)	win_lock_locked(&((wl)->display))


#define win_lock_set(which_lock, on)	((which_lock)->lock = (on))

#endif WIN_LOCK_DEFINED
