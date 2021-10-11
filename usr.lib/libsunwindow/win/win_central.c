#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)win_central.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Implements library routines for centralized window event management.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_planegroups.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_notify.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/sv_malloc.h>
#include <sunwindow/win_impl.h>

static	Window_handles *wins;
				/* Dynamically allocated to be getdtablesize */
				/* (Isn't there a constant can use instead?) */
				/* Position in table corresponds to fd number*/
static  int win_num;		/* Size of wins array */
static  int wins_managing;	/* Number of active windows in wins array */

static	Window_handles *win_get_handles();	/* (Notify_client client,
						    Pixwin_handles **pwh) */
static	Pixwin_handles *win_find_handles();	/* (Notify_client client,
						    Window_handles *win) */
static	Pixwin_handles *win_find_consumer();	/* (Window_handles *win,
						    Event *event) */
static	Notify_value input_pending();		/* (Notify_client client,
						    int fd) */
static	Notify_value pw_change();		/* (Notify_client client,
						    int signal,
						    Notify_signal_mode mode) */
static	Notify_value win_send_resize();		/* (Window_handles *win) */
static	Notify_value win_send_repaint();	/* (Window_handles *win) */
static	Notify_value pw_destroy_handles();
static	Notify_error win_send();

extern	int errno;

static	int win_remove_handles();
	int win_clear_cursor_planes();
	int win_cursor_planes_available();

extern	int	sv_journal;
char	*shell_prompt;
int	prompt_len;

void    notify_perror();
/*
 * Public interface:
 */

/* performance: global cache of getdtablesize() */
extern int dtablesize_cache;
#define GETDTABLESIZE() \
	(dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

int
win_register(client, pw, event_func, destroy_func, flags)
	register Notify_client client;
	Pixwin	*pw;
	Notify_func event_func;
	Notify_func destroy_func;
	register u_int	flags;
{
	register Window_handles *win;
	register Pixwin_handles *pwh;
	register int fd = pw->pw_windowfd;
	char *calloc();
	char *shell_ptr;
	char *journal_ptr;
	extern	char *getenv();

	/* 
	 * Allocating window handles in increments has the 
	 * following problem:
	 * In the case where an event/notify proc calls window_create
	 * and additional window handles are created, *and* "wins" 
	 * is realloc'ed, the caller program's event pointer will
	 * no longer be valid. We therefore need to allocate all 
	 * handles in one big chunk until the mechanism for passing
	 * event pointers to event/notify procs is changed. Currently,
	 * the events are obtained directly from the appropriate
	 * Window_handle.
	 */

	/* Allocate wins if not done so already */
	if (wins == WINDOW_HANDLES_NULL) {
		win_num = GETDTABLESIZE();
		wins = (Window_handles *)(LINT_CAST(
			calloc((unsigned)win_num,sizeof(Window_handles))));
		if (wins == WINDOW_HANDLES_NULL) {
			errno = ENOMEM;
			perror("win_register");
			goto Error;
		}
		shell_prompt = (char *)sv_calloc(40,sizeof(char));
		if ((shell_ptr = getenv("SVJ_PROMPT")) == NULL) 
			strcpy(shell_prompt,"% ");
		else 
			strcpy(shell_prompt,shell_ptr);
		prompt_len = strlen(shell_prompt);
                sv_journal = FALSE;
                if ((journal_ptr = getenv("WINDOW_JOURNAL")) != NULL) {
			if (strcmp(journal_ptr,"ON") == 0)
                        	sv_journal = TRUE;
		}
	}

	/* See if this window has any pixwins yet */
	win = &wins[fd];
	if (win->next == PIXWIN_HANDLES_NULL) {
		/* Set up input notifier function (one per window) */
		if (notify_set_input_func(client, input_pending, fd) ==
		    NOTIFY_FUNC_NULL) {
			notify_perror("win_register");
			return(-1);
		}
		(void)win_getrect(fd, &win->rect);
	}
	/* Set event handler */
	if (notify_set_event_func(client, event_func, NOTIFY_SAFE) ==
	      NOTIFY_FUNC_NULL) {
		notify_perror("win_register");
		goto Error;
	}
	/* Set destroy handler */
	if (destroy_func == NOTIFY_FUNC_NULL) {
		if ((destroy_func = notify_get_destroy_func(client)) ==
		    NOTIFY_FUNC_NULL) 
			/*
			 * Set up our destroy routine so at least we clean
			 * up ourselves.
			 */
			destroy_func = pw_destroy_handles;
	}
	if (notify_set_destroy_func(client, destroy_func) == NOTIFY_FUNC_NULL) {
		notify_perror("win_register");
		goto Error;
	}
	/* Allocate pixwin_handles */
	pwh = (Pixwin_handles *)(LINT_CAST(
		calloc(1, sizeof(Pixwin_handles))));
	if (pwh == PIXWIN_HANDLES_NULL) {
		errno = ENOMEM;
		perror("win_register");
		goto Error;
	}
	/* Conditionally make pixwin retained */
	if ((flags & PW_RETAIN) && (pw->pw_prretained == ((struct pixrect *)0)))
		win_set_retain(pw, win);
	/* Fill in structure */
	pwh->pw = pw;
	pwh->client = client;
	pwh->flags = flags; 
	/* Add to head of client list */
	pwh->next = win->next;
	win->next = pwh;

	if (++wins_managing == 1) {
		/* Set up signal notifier function (one per process) */
		if (notify_set_signal_func((Notify_client)wins, pw_change, SIGWINCH,
		    NOTIFY_SYNC) == NOTIFY_FUNC_NULL) {
			notify_perror("win_register");
			goto Error;
		}
	}
	return(0);
Error:
	/* Reset client entanglements with notifier */
	(void)notify_remove(client);
	return(-1);
}

int
win_unregister(client)
	register Notify_client client;
{
	Pixwin_handles *pwh;
	register Window_handles *win = win_get_handles(client, &pwh);

	if ((win == WINDOW_HANDLES_NULL) || (pwh == PIXWIN_HANDLES_NULL))
		return(-1);
	/*
	 * Although pw_close will release retained image, we do it now
	 * because we allocated it originally.
	 */
	if (pwh->flags & PW_RETAIN) {
		(void)pr_destroy(pwh->pw->pw_prretained);
		pwh->pw->pw_prretained = (struct pixrect *)0;
	}
	/*
	 * Remove conditions from notifier.  Could remove just conditions
	 * set in win_register.  But we are doing everyone a favor here because
	 * it is overwhelmingly the common case.  Common conditions set by
	 * the first pixwin will be cleared by that pixwin's unregister.
	 */
	(void) notify_remove(client);
	/* Do local data structure clean up */
	if (win->next == pwh && pwh->next == PIXWIN_HANDLES_NULL)
		/* If last client for win then zero win handles */
		bzero((caddr_t)win, sizeof(Window_handles));
	else
		/* Axe client from list */
		(void)win_remove_handles(win, pwh);
	if (--wins_managing == 0)
		/* Remove SunWindows notifier's notifier entanglements */
		(void) notify_remove((Notify_client)wins);
	/* Release client record */
	free((caddr_t)pwh);
	return(0);
}

int
win_set_flags(client, flags)
	Notify_client client;
	u_int flags;
{
	Pixwin_handles *pwh;
	Window_handles *win = win_get_handles(client, &pwh);

	if ((win == WINDOW_HANDLES_NULL) || (pwh == PIXWIN_HANDLES_NULL))
		return(-1);
	if ((pwh->flags ^ flags) & PW_RETAIN) {
		if ((flags & PW_RETAIN) &&
		    (pwh->pw->pw_prretained == ((struct pixrect *)0)))
			win_set_retain(pwh->pw, win);
		else if (pwh->flags & PW_RETAIN) {
			(void)pr_destroy(pwh->pw->pw_prretained);
			pwh->pw->pw_prretained = (struct pixrect *)0;
		}
	}
	pwh->flags = flags; 
	return(0);
}

u_int
win_get_flags(client)
	Notify_client client;
{
	Pixwin_handles *pwh;
	Window_handles *win = win_get_handles(client, &pwh);

	if ((win == WINDOW_HANDLES_NULL) || (pwh == PIXWIN_HANDLES_NULL))
		return(0);
	return(pwh->flags);
}

Notify_error
win_post_id(client, id, when)
	Notify_client client;
	short id;
	Notify_event_type when;
{
	Pixwin_handles *pwh;
	Window_handles *win = win_get_handles(client, &pwh);
	int fd = win_get_fd(client);

	if (win == WINDOW_HANDLES_NULL)
		return(NOTIFY_UNKNOWN_CLIENT);

	/* TODO: Get current event from kernel instead of win_event */
	/* In light of keymapping, probably don't want to do this */
	/* Using event_set_id() will zero out the upper 8 bit of the */
	/* shiftmask.  This is kind of redundant, since win_keymap_map() */
	/* will set those bit.  But this is good practice for consistency */

	event_set_id(&win->event, id);

	/*
	 * Event is mapped to insure semantic event is correct in the
	 * event struct.  It appears that flags and shiftmask are obtained
	 * implicitly elsewhere.
	 */
	(void)win_keymap_map(fd, &win->event);

	return (win_send(pwh, &win->event, when, (Notify_arg)0, win_copy_event,
	    win_free_event));
}

Notify_error
win_post_id_and_arg(client, id, when, arg, copy_func, release_func)
	Notify_client client;
	short id;
	Notify_event_type when;
	Notify_arg arg;
	Notify_copy copy_func;
	Notify_release release_func;
{
	Pixwin_handles *pwh;
	Window_handles *win = win_get_handles(client, &pwh);
	int fd = win_get_fd(client);

	if (win == WINDOW_HANDLES_NULL)
		return(NOTIFY_UNKNOWN_CLIENT);

	/* TODO: Get current event from kernel instead of win_event */
	/* In light of keymapping, probably don't want to do this */
	/* Using event_set_id() will zero out the upper 8 bit of the */
	/* shiftmask.  This is kind of redundant, since win_keymap_map() */
	/* will set those bit.  But this is good practice for consistency */

	event_set_id(&win->event, id);


	/*
	 * Event is mapped to insure semantic event is correct in the
	 * event struct.  It appears that flags and shiftmask are obtained
	 * implicitly elsewhere.
	 */
	(void)win_keymap_map(fd, &win->event);

	return (win_send(pwh, &win->event, when, arg, copy_func, release_func));
}

Notify_error
win_post_event(client, event, when)
	Notify_client client;
	Event *event;
	Notify_event_type when;
{
	Pixwin_handles *pwh;
	Window_handles *win = win_get_handles(client, (Pixwin_handles **)0);
	int fd = win_get_fd(client);

	if (win == WINDOW_HANDLES_NULL)
		return(NOTIFY_UNKNOWN_CLIENT);
	/* Find client for this event */
	if ((pwh = win_find_consumer(win, event)) == PIXWIN_HANDLES_NULL)
		return(NOTIFY_UNKNOWN_CLIENT);
	/* Remember this event as latest one for this window */
	(void)win_keymap_map(fd, event);
	win->event = *event;

	/* Send event */
	return (win_send(pwh, event, when, (Notify_arg)0, 
		win_copy_event, win_free_event));
}

Notify_error
win_post_event_arg(client, event, when, arg, copy_func, release_func)
	Notify_client client;
	Event *event;
	Notify_event_type when;
	Notify_arg arg;
	Notify_copy copy_func;
	Notify_release release_func;
{
	Pixwin_handles *pwh;
	Window_handles *win = win_get_handles(client, &pwh);
	int fd = win_get_fd(client);

	if (win == WINDOW_HANDLES_NULL)
		return(NOTIFY_UNKNOWN_CLIENT);
	/* Find client for this event */
	if ((pwh = win_find_consumer(win, event)) == PIXWIN_HANDLES_NULL)
		return(NOTIFY_UNKNOWN_CLIENT);
	/* Remember this event as latest one for this window */
	win->event = *event;
	(void)win_keymap_map(fd, &win->event);

	/* Send event */
	return (win_send(pwh, event, when, arg, copy_func, release_func));
}

/*ARGSUSED*/
Notify_arg
win_copy_event(client, arg, event_ptr)
	Notify_client client;
	Notify_arg arg;
	Event **event_ptr;
{
	caddr_t malloc();
	Event *event_new;

	if (*event_ptr != EVENT_NULL) {
		event_new = (Event *) (LINT_CAST(sv_malloc(sizeof (Event))));
		*event_new = **event_ptr;
		*event_ptr = event_new;
	}
	return (0);
}

/*ARGSUSED*/
void
win_free_event(client, arg, event)
	Notify_client client;
	Notify_arg arg;
	Event *event;
{
	if (event != EVENT_NULL)
		free((caddr_t)event);
}

int
win_get_fd(client)
	Notify_client client;
{
	Pixwin_handles *pwh;
	Window_handles *win = win_get_handles(client, &pwh);

	if (win == WINDOW_HANDLES_NULL)
		return(-1);
	else
		return(pwh->pw->pw_windowfd);
}

Pixwin *
win_get_pixwin(client)
	Notify_client client;
{
	Pixwin_handles *pwh;
	Window_handles *win = win_get_handles(client, &pwh);

	if (win == WINDOW_HANDLES_NULL)
		return((Pixwin *)0);
	else
		return(pwh->pw);
}

int
pw_set_region_rect(pw, r, use_same_pr)
        register Pixwin *pw;
        register Rect *r;
	int use_same_pr;
{
	struct pixwin_clipdata *pwcd = pw->pw_clipdata;

	/* It is an error if the pixwin is not a region */
	if (pwcd->pwcd_regionrect == RECT_NULL)
		return (-1);
	/* This call is a no op if the existing region & new rect are equal */
	if (rect_equal(pwcd->pwcd_regionrect, r))
		return (0);
	/*
	 * Set rect.  Caller is responsible for seeing that r is within any
	 * containing regions.
	 */
	*pwcd->pwcd_regionrect = *r;
	/*
	 * Reset retained image, if any.  This doesn't work if the
	 * retained image is not a primary pixrect.
	 */
	if (!use_same_pr)
		(void)pw_set_retain(pw, r->r_width, r->r_height);
	/* Figure new clipping */
	(void)pw_getclipping(pw);
	return (0);
}

int
pw_get_region_rect(pw, r)
        Pixwin *pw;
        Rect *r;
{
	if (pw->pw_clipdata->pwcd_regionrect == RECT_NULL)
		return (-1);
	*r = *pw->pw_clipdata->pwcd_regionrect;
	return (0);
}

pw_restrict_clipping(pw, rl)
        struct  pixwin *pw;
        struct  rectlist *rl;
{
        (void)rl_intersection(&pw->pw_clipdata->pwcd_clipping, rl,
            &pw->pw_clipdata->pwcd_clipping);
        (void)pw_setup_clippers(pw);
	return;
}

pw_reduce_clipping(pw, rl)
        struct  pixwin *pw;
        struct  rectlist *rl;
{
        (void)rl_difference(&pw->pw_clipdata->pwcd_clipping, rl,
            &pw->pw_clipdata->pwcd_clipping);
        (void)pw_setup_clippers(pw);
	return;
}

pw_setup_clippers(pw)
        struct  pixwin *pw;
{
        struct  rect screenrect;
	int saved_clipid, saved_damageid;
	Rectlist saved_clipping;
	Rect *saved_regionrect;
	register struct pixwin_clipdata *pwcd = pw->pw_clipdata;

	/* Remember stuff that pwco_reinitclipping destroies */
	saved_clipid = pwcd->pwcd_clipid;
	saved_damageid = pwcd->pwcd_damagedid;
	saved_clipping = rl_null;
	(void)rl_copy(&pwcd->pwcd_clipping, &saved_clipping);
	/* Free existing clipping */
	pwco_reinitclipping(pw);
	/* Restore clipping and clipid */
	pwcd->pwcd_clipping = saved_clipping;
	pwcd->pwcd_damagedid = saved_damageid;
	pwcd->pwcd_clipid = saved_clipid;
	/* Get screen rect used by _pw_setclippers */
	(void)win_getscreenrect(pw->pw_windowfd, &screenrect);
	/* Null out stuff that _pw_setclippers only should see once */
	saved_regionrect = pwcd->pwcd_regionrect;
	pwcd->pwcd_regionrect = RECT_NULL;
	/* Do some stuff that _pw_setclippers would do if was region */
	if (saved_regionrect) {
		Rect regionrect;

		/* Trim screenrect to regionrect */
		regionrect = *saved_regionrect;
		rect_passtoparent(screenrect.r_left, screenrect.r_top,
		    &regionrect);
		(void)rect_intersection(&regionrect, &screenrect, &screenrect);
	}
	/* Restore clippers */
        (void)_pw_setclippers(pw, &screenrect);
	/* Restore nulled out stuff */
	pwcd->pwcd_regionrect = saved_regionrect;
	return;
}

win_getscreenrect(windowfd, rect)
	int windowfd;
	struct rect *rect;
{
	int left, top;

        /* Note: Probably should cache screenrect */
	(void)win_getsize(windowfd, rect);
	/* Note: Don't use &screenrect.r_left because is a short */
	(void)win_getscreenposition(windowfd, &left, &top);
	rect->r_left = left;
	rect->r_top = top;
	return;
}

win_set_retain(pw, win)
	register Pixwin *pw;
	register Window_handles *win;
{
	if (pw->pw_clipdata->pwcd_regionrect == RECT_NULL)
		(void)pw_set_retain(pw,
		    win->rect.r_width, win->rect.r_height);
	else
		(void)pw_set_retain(pw,
		    pw->pw_clipdata->pwcd_regionrect->r_width,
		    pw->pw_clipdata->pwcd_regionrect->r_height);
}

/*
 * Change retained image to be width, height, (computed depth) if new
 * image will be different.  Should be called when resize window or change
 * colormap segment depth.
 */
pw_set_retain(pw, width, height)
	register Pixwin *pw;
	int width, height;
{
	struct colormapseg cms;
	register int depth;
	register struct pixrect *pr = pw->pw_prretained;

	/* Conserve memory on depth > 1 plane pixrects if only need bit map. */
	(void)pw_getcmsdata(pw, &cms, (int *)0);
	depth = (cms.cms_size == 2 && pw->pw_pixrect->pr_depth<32)
	    ? 1: pw->pw_pixrect->pr_depth;
	/* Release existing retained image */
	if (pr) {
		/* See if new pr would be the same as the existing one */
		if (pr->pr_width == width && pr->pr_height == height &&
		    pr->pr_depth == depth)
			return;
		(void)pr_destroy(pr);
	}
	/* Allocate new retained image.  Failure will return NULL pointer. */
	pw->pw_prretained = mem_create(width, height, depth);
	return;
}

/*
 * Referenced across sunwindow modules:
 */
win_getrect_local(windowfd, rect)
	int	windowfd;
	struct	rect *rect;
{
	if (wins == WINDOW_HANDLES_NULL ||
	    wins[windowfd].next == PIXWIN_HANDLES_NULL)
		(void)win_getrect_from_source(windowfd, rect);
	else
		*rect = wins[windowfd].rect;
	return;
}

win_setrect_local(windowfd, rect)
	int	windowfd;
	struct	rect *rect;
{
	register Window_handles *win;

	/* Update kernel's notion of rect */
	(void)win_setrect_at_source(windowfd, rect);
	/* See if this window has a client associated with it */
	if (wins != WINDOW_HANDLES_NULL &&
	    (win = &wins[windowfd]) && (win->next != PIXWIN_HANDLES_NULL)) {
		/* Note if size has changed */
		if (rect_sizes_differ(&(win->rect), rect))
			win->flags |= WH_SIZE_CHANGED;
		/* Update local notion of rect */
		win->rect = *rect;
	}
	return;
}

win_grabio_local(windowfd)
	int 	windowfd;
{
	if (wins == WINDOW_HANDLES_NULL)
		return;
	if (wins[windowfd].next)
		wins[windowfd].flags |= WH_GRABBED_INPUT;
	return;
}

win_releaseio_local(windowfd)
	int 	windowfd;
{
	if (wins == WINDOW_HANDLES_NULL)
		return;
	if (wins[windowfd].next)
		wins[windowfd].flags &= ~WH_GRABBED_INPUT;
	return;
}

win_is_io_grabbed(windowfd)
	int 	windowfd;
{
	if (wins == WINDOW_HANDLES_NULL)
		return (0);
	if (wins[windowfd].next && (wins[windowfd].flags & WH_GRABBED_INPUT))
		return (1);
	return (0);
}

#ifndef PRE_FLAMINGO
win_getnotifyall(windowfd)
        int windowfd;
{
	return ((wins[windowfd].flags & WH_NOTIFY_ALL) != 0);
}

win_setnotifyall(windowfd, flag)
        int windowfd;
        int flag;
{
	if (flag)
		wins[windowfd].flags |= WH_NOTIFY_ALL;
	else
		wins[windowfd].flags &= ~WH_NOTIFY_ALL;
        (void)werror(ioctl(windowfd, WINSETNOTIFYALL, &flag), WINSETNOTIFYALL);
}
win_clear_cursor_planes(windowfd, rect)
        int windowfd;
        Rect    *rect;
{
  
    (void)werror(ioctl(windowfd, WINCLEARCURSORPLANES, rect), 
                                    WINCLEARCURSORPLANES);
}
win_cursor_planes_available(windowfd)
        int windowfd;
{
    struct win_plane_groups_available  available;

    if (ioctl(windowfd, WINGETAVAILPLANEGROUPS, &available) == -1)
        return (0);

    if ((available.plane_groups_available[PIXPG_CURSOR]) &&
            (available.plane_groups_available[PIXPG_CURSOR_ENABLE])) {
        return (1);
    } else {
        return (0);
    }
}

#endif

/*
 * Private to this module:
 */
/*ARGSUSED*/
static Notify_value
input_pending(client, fd)
	Notify_client client;
	int fd;
{
	Pixwin_handles *pwh;
	struct	inputevent *win_event;

	/* Validate */
	if (wins == WINDOW_HANDLES_NULL)
		return(NOTIFY_UNEXPECTED);
	/* Read input */
	win_event = &wins[fd].event;
	if (input_readevent(fd, win_event)) {
		perror("input_pending");
		return(NOTIFY_IGNORED);
	}
	/* Find client for this event */
	if ((pwh = win_find_consumer(&wins[fd], win_event)) ==
	    PIXWIN_HANDLES_NULL)
		return(NOTIFY_IGNORED);
	/* Send event */
	(void) win_send(pwh, win_event, NOTIFY_SAFE, (Notify_arg)0,
	    win_copy_event, win_free_event);
	return(NOTIFY_DONE);
}

/*
 * Pw_change has to be called directly if want to immediately paint
 * screen before entering the notifier.
 */
/*ARGSUSED*/
static Notify_value
pw_change(wins_dummy, sig, mode)
	Notify_client wins_dummy;
	int sig;
	Notify_signal_mode mode;
{
	register i;
	int global_resized;
	register Notify_value return_code;

	if ((sig != SIGWINCH) || (wins == WINDOW_HANDLES_NULL))
		return(NOTIFY_IGNORED);
	return_code = NOTIFY_IGNORED;
	/* Handle all resizes until there are no more changes */
	do {
		global_resized = 0;
		for (i = 0; i < win_num; i++) {
			switch (win_send_resize(&wins[i])) {
			case NOTIFY_DONE:
				if (return_code == NOTIFY_IGNORED)
					return_code = NOTIFY_DONE;
				global_resized = 1;
				break;
			case NOTIFY_IGNORED:
				break;
			default:
				return_code = NOTIFY_UNEXPECTED;
			}
		}
	} while (global_resized);
	/* Handle all repainting */
	for (i = 0; i < win_num; i++) {
		switch (win_send_repaint(&wins[i])) {
		case NOTIFY_DONE:
			if (return_code == NOTIFY_IGNORED)
				return_code = NOTIFY_DONE;
			break;
		case NOTIFY_IGNORED:
			break;
		default:
			return_code = NOTIFY_UNEXPECTED;
		}
	}
	return (return_code);	
}

static Notify_value
win_send_resize(win)
	register Window_handles *win;
{
	register Pixwin_handles *pwh;
	Rect rect;
	int win_resized;

	if (win->next == PIXWIN_HANDLES_NULL)
		return (NOTIFY_IGNORED);
	/* Get truth about rect size */
	(void)win_getrect_from_source(win->next->pw->pw_windowfd, &rect);
	/*
	 * See if size changed.  If another process changed it
	 * then the rects will differ.  If this process did it
	 * then the flag is set.
	 */
	win_resized = ((rect.r_width != win->rect.r_width) ||
	    (rect.r_height != win->rect.r_height) ||
	    (win->flags & WH_SIZE_CHANGED));
	win->flags &= ~WH_SIZE_CHANGED;
	/* Update local (user process) notion of rect */
	win->rect = rect;
	/* Mark a win that is doing its own sigwinch handling */
	win->flags &= ~WH_OWN_SIGWINCH;
	for (pwh = win->next;pwh; pwh = pwh->next) {
		if (notify_get_signal_func(pwh->client,
		    SIGWINCH, NOTIFY_SYNC) != NOTIFY_FUNC_NULL) {
			win->flags |= WH_OWN_SIGWINCH;
			return (NOTIFY_IGNORED);
		}
	}
	if (win_resized) {
		/* Send WIN_RESIZE notification to all pwh's of win */
		for (pwh = win->next;pwh; pwh = pwh->next) {
			switch (win_post_id(pwh->client, WIN_RESIZE,
			    NOTIFY_SAFE)) {
			case NOTIFY_OK:
				pwh->flags |= PW_RESIZED;
				/* Change retained image if manager of it */
				if (pwh->flags & PW_RETAIN)
					win_set_retain(pwh->pw, win);
				break;
			case NOTIFY_NO_CONDITION:
				/*
				 * Client may actually be handling SIGWINCHes
				 * himself
				 */
				break;
			default:
				notify_perror("pw_send_resize");
				return (NOTIFY_UNEXPECTED);
			}
		}
		return (NOTIFY_DONE);
	} else
		return (NOTIFY_IGNORED);
}

static Notify_value
win_send_repaint(win)
	register Window_handles *win;
{
#define	REDO_LIMIT	3
	register Pixwin_handles *pwh;
	register struct pixwin *pw = ((struct pixwin *)0);
	Notify_value return_code = NOTIFY_IGNORED;
	int resize;
	int damaged_id;
	int not_fixed_image;
	int done_damaged_called = 0;
	int repaint_all = 0;
	int redo_count = 0;

	if ((win->next == PIXWIN_HANDLES_NULL) ||
	    (win->flags & WH_OWN_SIGWINCH))
		return (NOTIFY_IGNORED);
Redo:
	/* Prevent unsolved flicker repaint situation */
	if (redo_count++ >= REDO_LIMIT) {
		(void)pw_donedamaged(pw);
		return (return_code);
	}
	/*
	 * Must get through entire repair process with damaged_id equalling
	 * its initial value at the first pw_damaged or should redo the
	 * fixup.  This is because more damage might have occured between
	 * the start and finish.
	 */
	damaged_id = 0;
	resize = 0;
	not_fixed_image = 0;
	/* See what needs to be fixed up */
	for (pwh = win->next;pwh; pwh = pwh->next) {
		pw = pwh->pw;
		(void)pw_damaged(pw);
		/* See if first time called pw_damaged */
		if (damaged_id == 0)
			damaged_id = pw->pw_clipdata->pwcd_damagedid;
		/*
		 * Repair as much as possible from retained image unless
		 * resizing.  Always repair if client specified a fixed
		 * sized image.
		 */
		if (pwh->flags & PW_RESIZED)
			resize = 1;
		if ((pwh->flags & PW_RETAIN) &&
		    pw->pw_prretained &&
		    ((!(pwh->flags & PW_RESIZED)) ||
		    (pwh->flags & PW_FIXED_IMAGE))) {
			struct rect retain_rect;
			struct rectlist retain_rl;

			/* Reduce repaired area by intersecting tiles */
			win_reduce_by_regions(win, pwh);
			/* Copy bits to the screen */
			(void)pw_repairretained(pw);
			/* Reduce damaged area by amount repaired */
			rect_construct(&retain_rect, 0, 0,
			    pw->pw_prretained->pr_width,
			    pw->pw_prretained->pr_height);
			(void)rl_initwithrect(&retain_rect, &retain_rl);
			(void)pw_reduce_clipping(pw, &retain_rl);
			(void)rl_free(&retain_rl);
			return_code = NOTIFY_DONE;
		}
		if (!(pwh->flags & PW_FIXED_IMAGE))
			not_fixed_image = 1;
		if (pwh->flags & PW_REPAINT_ALL)
			repaint_all = 1;
		pwh->flags &= ~PW_RESIZED;
	}
	/*
	 * If not fixed size image and resize then set up clipping to be
	 * the new entire exposed area.  Perhaps we could be smarter
	 * to reduce repainting by doing this on a pixwin by pixwin basis.
	 */
	if ((resize && not_fixed_image) || repaint_all) {
		if (pw->pw_clipdata->pwcd_damagedid != damaged_id)
			goto Redo;
		(void)pw_donedamaged(pw);
		done_damaged_called = 1;
	}
	/* See if any thing else to repair */
	for (pwh = win->next;pwh; pwh = pwh->next) {
		pw = pwh->pw;
		/*
		 * Make sure every pixwin uses exposed clipping (a redundant
		 * but harmless call for the pixwin that called pw_donedamaged).
		 */
		if (done_damaged_called)
			(void)pw_exposed(pw);
#ifndef PRE_FLAMINGO
		if (!rl_empty(&pw->pw_clipdata->pwcd_clipping) ||
		    win->flags & WH_NOTIFY_ALL) {
#else
		if (!rl_empty(&pw->pw_clipdata->pwcd_clipping)) {
#endif
			/* Clear fixed size image */
			if (pwh->flags & PW_FIXED_IMAGE)
				(void)pw_writebackground(pw, 0, 0,
				    win->rect.r_width, win->rect.r_height,
				    PIX_CLR);
			/* Notify client to repair the rest */
			switch (win_post_id(pwh->client, WIN_REPAINT,
			    NOTIFY_SAFE)) {
			case NOTIFY_OK:
				if (return_code == NOTIFY_IGNORED)
					return_code = NOTIFY_DONE;
				break;
			case NOTIFY_NO_CONDITION:
				/*
				 * Client may actually be handling
				 * SIGWINCHes himself
				 */
				break;
			default:
				notify_perror("pw_change");
				return_code = NOTIFY_UNEXPECTED;
			}
		}
		/*
		 * If haven't yet, set pixwin to exposed.  Last pixwin handled
		 * below.
		 */
		if (!done_damaged_called && pwh->next != PIXWIN_HANDLES_NULL)
			(void)pw_exposed(pw);
	}
	/* Tell kernel that fixed up image and reset clipping to exposed */
	if (!done_damaged_called) {
		if (pw->pw_clipdata->pwcd_damagedid != damaged_id)
			goto Redo;
		(void)pw_donedamaged(pw);
	}
	return (return_code);
}

/*
 * Reduce repaired area by intersecting tiles,
 * if full window pixwin.  Can't just reduce by
 * overlapping tiles because there is no notion
 * of overlapping tiles.  Choose only full window
 * pixwin because if did it arbitrarily, there
 * would not be any bits written for the intersection.
 * (This supports 3.0 textsw's that migrated to be able
 * to be retained in 3.2).
 */
static
win_reduce_by_regions(win, pwh_main)
	Window_handles *win;
	register Pixwin_handles *pwh_main;
{
	struct rectlist rl;
	register Pixwin_handles *pwh_region;

	if (pwh_main->pw->pw_clipdata->pwcd_regionrect)
		return;
	rl = rl_null;
	for (pwh_region = win->next;pwh_region; pwh_region = pwh_region->next) {
		if (pwh_region->pw->pw_clipdata->pwcd_regionrect == RECT_NULL)
			continue;
		(void)rl_rectunion(pwh_region->pw->pw_clipdata->pwcd_regionrect,
		    &rl, &rl);
	}
	(void)pw_reduce_clipping(pwh_main->pw, &rl);
	(void)rl_free(&rl);
}

static Notify_value
pw_destroy_handles(client, status)
	Notify_client client;
	Destroy_status status;
{
	if (status != DESTROY_CHECKING)
		(void)win_unregister(client);
	return(NOTIFY_DONE);
}

static Window_handles *
win_get_handles(client, pwh_ptr)
	Notify_client client;
	register Pixwin_handles **pwh_ptr;
{
	register int i;
	register Window_handles *win;
	Pixwin_handles *pwh;

	if (wins == WINDOW_HANDLES_NULL)
		return(WINDOW_HANDLES_NULL);
	/* Search through windows */
	for (i = 0; i < win_num; i++) {
		win = &wins[i];
		/* Search through pixwin clients */
		if ((pwh = win_find_handles(client, win)) !=
		    PIXWIN_HANDLES_NULL) {
			if (pwh_ptr)
				*pwh_ptr = pwh;
			return(win);
		}
	}
	/* Didn't find client */
	if (pwh_ptr)
		*pwh_ptr = PIXWIN_HANDLES_NULL;
	return(WINDOW_HANDLES_NULL);
}

static	int
win_remove_handles(win, pwh_axe)
	register Window_handles *win;
	register Pixwin_handles *pwh_axe;
{
	register Pixwin_handles **pwh_ptr;
	register Pixwin_handles *pwh;

	/* Search through pixwin clients */
	pwh_ptr = &win->next;
	for (pwh = win->next;pwh; pwh = pwh->next) {
		if (pwh == pwh_axe) {
			/* Fix up list pointer */
			*pwh_ptr = pwh->next;
			/* Remove win references to pwh */
			if (win->latest == pwh_axe)
				win->latest = PIXWIN_HANDLES_NULL;
			if (win->current == pwh_axe)
				win->current = PIXWIN_HANDLES_NULL;
			return (0);
		}
		pwh_ptr = &pwh->next;
	}
	return (-1);
}

static Pixwin_handles *
win_find_handles(client, win)
	Notify_client client;
	register Window_handles *win;
{
	register Pixwin_handles *pwh;

	/* Search through pixwin clients */
	for (pwh = win->next;pwh; pwh = pwh->next) {
		if (pwh->client == client)
			return(pwh);
	}
	return(PIXWIN_HANDLES_NULL);
}

static Pixwin_handles *
win_find_consumer(win, event)
	register Window_handles *win;
	register Event *event;
{
	register Pixwin_handles *pwh = PIXWIN_HANDLES_NULL;
	register Pixwin_handles *pwh_default = PIXWIN_HANDLES_NULL;
	Pixwin_handles *pwh_largest;
	Rect rect;
	int biggest_area, test_area;
	register int enters = 1;

	/* If input is grabbed then send to the grabber */
	if ((win->flags & WH_GRABBED_INPUT) && win->latest)
		return(win->latest);
	/* Search through pixwin clients */
	for (pwh = win->next;pwh; pwh = pwh->next) {
		/* Find region that includes x and y */
		if (pwh->pw->pw_clipdata->pwcd_regionrect)
			rect = *pwh->pw->pw_clipdata->pwcd_regionrect;
		else
			/* Make rect in own coordinate space */
			rect_construct(&rect, 0, 0, win->rect.r_width,
			    win->rect.r_height);
		if (rect_includespoint(&rect, event->ie_locx, event->ie_locy))
			goto Found;
		if (pwh->flags & PW_INPUT_DEFAULT)
			pwh_default = pwh;
	}
	/* Suppress sending of any enters */
	enters = 0;
	/*
	 * Event doesn't fall within any region's rectangle so assume
	 * send input to the default region.  Multiple regions that want
	 * to change the kbd focus would need to explicitly change the
	 * default region.
	 */
	if (pwh_default) {
		pwh = pwh_default;
		goto Found;
	}
	/*
	 * Since there wasn't any default region then guess that
	 * the latest region sent input to is the keyboard focus.
	 */
	if (win->latest) {
		pwh = win->latest;
		goto Found;
	}
	/*
	 * If there is no default or no latest, then the region with the
	 * biggest area is a third guess (assuming subwindow with scrollbars).
	 */
	pwh_largest = pwh;
	biggest_area = 0;
	for (pwh = win->next;pwh; pwh = pwh->next) {
		/* Find region that includes x and y */
		if (pwh->pw->pw_clipdata->pwcd_regionrect)
			rect = *pwh->pw->pw_clipdata->pwcd_regionrect;
		else
			/* Make rect in own coordinate space */
			rect_construct(&rect, 0, 0, win->rect.r_width,
			    win->rect.r_height);
		test_area = rect.r_width * rect.r_height;
		if (test_area > biggest_area) {
			pwh_largest = pwh;
			biggest_area = test_area;
		}
	}
	pwh = pwh_largest;
Found:
	/* Remember latest pixwin directed input towards */
	win->latest = pwh;
	/* See if exiting window */
	if (event->ie_code == LOC_WINEXIT)
		enters = 0;
	/*
	 * Send region exit if have entered region and the input recipent
	 * is now different.
	 */
	if (win->current &&
	    ((win->current != pwh) || (event->ie_code == LOC_WINEXIT)))
		win_rgnexit(win, event);
	/* Try sending region enter if last event sent was exit */
	if (enters && (win->current == PIXWIN_HANDLES_NULL)) {
		/* Delay region enter if window enter */
		if (event->ie_code == LOC_WINENTER) {
			pwh->flags |= PW_DELAYED_ENTER;
			win->current = pwh;
		} else
			win_rgnenter(win, event, pwh);
	}
	return(pwh);
}

static Notify_error
win_send(pwh, event, when, arg, copy_func, release_func)
	Pixwin_handles *pwh;
	register Event *event;
	Notify_event_type when;
	Notify_arg arg;
	Notify_copy copy_func;
	Notify_release release_func;
{
	register Rect *rect;
	short x_save = event->ie_locx;
	short y_save = event->ie_locy;
	Notify_error error;
	short pw_delayed_enter = (pwh->flags & PW_DELAYED_ENTER);
	Notify_client client = pwh->client;

	/* Translate mouse x and y */
	rect = pwh->pw->pw_clipdata->pwcd_regionrect;
	if (rect && !(pwh->flags & PW_NO_LOC_ADJUST)) {
		event->ie_locx -= rect->r_left;
		event->ie_locy -= rect->r_top;
	}
	/* Post event */
	error = notify_post_event_and_arg(client, (Notify_event)event,
	    when, arg, copy_func, release_func);
	if (error != NOTIFY_OK)
		notify_perror("win_send");
	/* Restore mouse x and y to base because win_event shared with others */
	event->ie_locx = x_save;
	event->ie_locy = y_save;
	/* Send delayed enter (don't rely on pwh being around any more) */
	if (pw_delayed_enter) {
		Window_handles *win = win_get_handles(client, &pwh);

		if (pwh != PIXWIN_HANDLES_NULL) {
			pwh->flags &= ~PW_DELAYED_ENTER;
			win_rgnenter(win, event, pwh);
		}
	}
	return (error);
}

static
win_rgnenter(win, event, pwh)
	register Window_handles *win;
	register Event *event;
	Pixwin_handles *pwh;
{
	unsigned short id = event->ie_code;
	unsigned short action = event_action(event);

	/* Make input sink */
	win->current = pwh;
	/* Send region enter event */
	event_set_id(event, LOC_RGNENTER);
	(void) win_send(pwh, event, NOTIFY_SAFE, (Notify_arg)0,
	    win_copy_event, win_free_event);
	event->ie_code = id;
	/* this madness because event_*_action semantics are screwed */
	if (action != id)
	    event_set_action(event, action);
	return;
}

static
win_rgnexit(win, event)
	register Window_handles *win;
	register Event *event;
{
	unsigned short id = event->ie_code;
	unsigned short action = event_action(event);

	if (win->current == PIXWIN_HANDLES_NULL)
		return;
	/* Send enter event */
	event_set_id(event, LOC_RGNEXIT);
	(void) win_send(win->current, event, NOTIFY_SAFE, (Notify_arg)0,
	    win_copy_event, win_free_event);
	event->ie_code = id;
	/* this madness because event_*_action semantics are screwed */
	if (action != id)
	    event_set_action(event, action);
	/* No one currently has locator in it */
	win->current = PIXWIN_HANDLES_NULL;
	return;
}
