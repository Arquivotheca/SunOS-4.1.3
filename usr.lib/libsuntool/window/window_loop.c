#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)window_loop.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/frame.h>
#include <suntool/fullscreen.h>


extern struct pixrect	*save_bits();
extern int		restore_bits();

static short		no_return /* = FALSE */;
static caddr_t		return_value;


caddr_t
window_loop(frame)
register Frame	frame;
{
    register Window	first_window;
    register Frame	shadow;
    register int	frame_fd, first_fd;
    Pw_pixel_cache	*pixel_cache; 
    struct fullscreen	*fs;
    Rect		screen_rect, rect;
    Event		event;
    

    /* we don't support recursive window_loop() calls yet.
     * Also, no support for already-shown confirmer.
     * Also, can't be iconic.
     */
    if (no_return || window_get(frame, WIN_SHOW) || 
	window_get(frame, FRAME_CLOSED))
	return (caddr_t) 0;

    /* insert the frame in the window tree.
     * Note that this will not alter the screen
     * until the WIN_REPAINT is sent below.
     */
    (void)window_set(frame, WIN_SHOW, TRUE, 0);

    screen_rect 	= *((Rect *) (LINT_CAST(
    				window_get(frame, WIN_SCREEN_RECT))));    
    frame_fd 		= (int) window_get(frame, WIN_FD);

    first_window 	= window_get(frame, FRAME_NTH_SUBWINDOW, 0);
    first_fd 		= (int) window_get(first_window, WIN_FD);
    
    shadow = window_get(frame, FRAME_SHADOW, 0);

    /* constrain the frame rect to be on the
     * screen.
     */
    (void)wmgr_constrainrect(&rect, &screen_rect, 0, 0);

    /* lock the window tree in hopes of
     * preventing screen updates until the
     * grabio.
     */
    (void)win_lockdata(frame_fd);
    
    /* make sure the frame and first_window
     * are in sync with their current size.
     * NOTE: Assuming WIN_REPAINT will not happens after WIN_RESIZE.
     */
     
    (void)pw_exposed(window_get(frame, WIN_PIXWIN));
    if (shadow != NULL)
        (void)pw_exposed(window_get(shadow, WIN_PIXWIN));
    (void)pw_exposed(window_get(first_window, WIN_PIXWIN));
    
    (void)win_post_id(first_window, WIN_RESIZE, NOTIFY_IMMEDIATE);      
    (void)win_post_id(frame,        WIN_RESIZE, NOTIFY_IMMEDIATE);
    
    rect 		= *((Rect *) (LINT_CAST(
    				window_get(frame, WIN_RECT))));
    rect.r_left = rect.r_top = 0;
    fs = fullscreen_init(frame_fd);
    
    /* account for the shadow area if present */
    if (shadow != NULL) {
        Rect	shadow_rect;
        
        /* use win_getrect() rather than WIN_RECT to get rect
         * in same space as the frame (screen coords.).
         */
        shadow_rect = *((Rect *) (LINT_CAST(
    				window_get(shadow, WIN_RECT))));       
        rect = rect_bounding(&rect, &shadow_rect);
    }
    
    if ((pixel_cache = pw_save_pixels(fs->fs_pixwin, &rect)) == 0) {
	(void)win_unlockdata(frame_fd);
	(void)fullscreen_destroy(fs);
	return (caddr_t) 0;
    }
    
    (void)pw_preparesurface(fs->fs_pixwin, &rect);
    (void)fullscreen_destroy(fs);
       
    (void)win_grabio(first_fd);
    (void)win_unlockdata(frame_fd);    
       
    /* paint the frame */
    (void)win_post_id(frame, WIN_REPAINT, NOTIFY_IMMEDIATE);

    /* paint the first subwindow */
    (void)win_post_id(first_window, WIN_REPAINT, NOTIFY_IMMEDIATE);
    (void)win_post_id(first_window, LOC_WINENTER, NOTIFY_IMMEDIATE);
    (void)win_post_id(first_window, KBD_USE, NOTIFY_IMMEDIATE);

/*   Moved to before win_post_id(first_window, WIN_RESIZE, NOTIFY_IMMEDIATE) 
    (void)win_grabio(first_fd);

    (void)win_unlockdata(frame_fd); 
 */

    /* read and post events to the
     * first subwindow until window_return()
     * is called.
     */
    no_return = TRUE;
    while (no_return) {
	(void)input_readevent(first_fd, &event);
	switch (event_action(&event)) {
	    case LOC_WINENTER:
	    case LOC_WINEXIT:
	    case KBD_USE:
	    case KBD_DONE:
	    case ACTION_FRONT:  case ACTION_BACK:	/* top/bottom */
	    case ACTION_OPEN:  case ACTION_CLOSE:	/* close/open */
		break;

	    default:
            /* don't mess with the data lock --
             * leave the grabio lock on; either the
             * kernel will be fixed to allow all windows in this
             * process to paint, or we require that only the
             * first_fd window can paint.
                (void)win_lockdata(frame_fd);
                (void)win_releaseio(first_fd);
             */

		(void)win_post_event(first_window, &event, NOTIFY_IMMEDIATE);

	     /*
		(void)win_grabio(first_fd);
		(void)win_unlockdata(frame_fd);
	     */
		break;
	}
    }

    /* lock the window tree until the
     * bits are restored.
     */
    (void)win_lockdata(frame_fd);
    (void)win_releaseio(first_fd);

    (void)win_post_id(first_window, KBD_DONE, NOTIFY_IMMEDIATE);
    (void)win_post_id(first_window, LOC_WINEXIT, NOTIFY_IMMEDIATE);

    fs = fullscreen_init(frame_fd);
    pw_restore_pixels(fs->fs_pixwin, pixel_cache);
    (void)fullscreen_destroy(fs);

    (void)window_set(frame, WIN_SHOW, FALSE, 0);
    if (shadow) 
        (void)window_set(shadow, WIN_SHOW, FALSE, 0);

    (void)win_unlockdata(frame_fd);

    no_return = FALSE;
    return return_value;
}


void
window_return(value)
caddr_t	value;
{
    no_return = FALSE;
    return_value = value;
}
