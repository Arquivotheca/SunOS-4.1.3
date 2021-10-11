#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ttysw_modes.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 *  Manages mode changes between cmdsw and ttysw.
 *
 *  The modes are as follows:
 *
 *  cmdsw
 *    cooked, echo
 *	append_only_log
 *	  caret must be at the end.  all input is buffered until
 *	  a command completion character or interrupt character is inserted.
 *	!append_only_log
 *	  if caret is at the end, interpret as in append_only_log.
 *	  otherwise, ignore edits.
 *
 *    direct (![cooked, echo])
 *	append_only_log
 *	  caret must be at pty mark.  insertion into the text subwindow
 *	  is refused and characters that were supposed to be inserted are
 *	  instead copied into the pty input buffer.
 *	!append_only_log
 *	  if caret is at pty mark, interpret as in append_only_log.
 *	  otherwise, ignore edits.
 *	  ctrl-Return should move to pty mark, not end.
 *
 *  ttysw
 *    split view
 *	append_only_log
 *	  read only, no caret at all.
 *	!append_only_log
 *	  caret anywhere, no interpretation of input.
 */


#include <errno.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/win_lock.h>
#include <suntool/frame.h>
#include <suntool/icon.h>
#include <suntool/window.h>
#include <suntool/ttysw.h>
#include <suntool/textsw.h>
#include <suntool/tool_struct.h>
#include <suntool/ttysw_impl.h>
#include <suntool/ttytlsw_impl.h>

extern	Cmdsw		*cmdsw;

extern	Notify_value	 ttysw_itimer_expired();
extern	Notify_value	 ttysw_pty_input_pending();
int		 	 ttysw_waiting_for_pty_input;
extern  Textsw_status	 textsw_set();

swap_windows(old_window, new_window, iwbp, iebp)
	caddr_t	 old_window, new_window;
	char	*iwbp, *iebp;
{
	int		 set_focus = 0;
	int		 old_winfd =
			 (int) window_get(old_window, WIN_FD);
	Event		 this_ie;
	int		 action;
	Rect		*new_win_rect;
	int		 iconic_offset;
	Frame		 frame = (Frame)window_get(old_window, WIN_OWNER);

	if (window_get(old_window, WIN_KBD_FOCUS)) {
	    set_focus = 1;
	}
	
	new_win_rect = (Rect *)window_get(new_window, WIN_RECT);
	iconic_offset = tool_sw_iconic_offset((Tool *)(LINT_CAST(frame)));
	
	if (new_win_rect->r_top < iconic_offset) {
	    new_win_rect->r_top += iconic_offset;
	    window_set(new_window, WIN_RECT, new_win_rect, 0);
	}
	(void)window_set(new_window, WIN_SHOW, TRUE, 0);
	
	(void)window_set(old_window, WIN_SHOW, FALSE, 0);
	
	if (set_focus) {
	    (void)window_set(new_window, WIN_KBD_FOCUS, TRUE, 0);
	}
	
	cmdsw->ttysw_resized = 0;
	/* drain old_winfd */
	while (read(old_winfd, (char *)&this_ie, sizeof(this_ie)) != -1) {
	    if (event_action(&this_ie) <= EUC_LAST && iwbp <= iebp) {  
		(void) win_post_event((Notify_client)new_window, &this_ie,
		    NOTIFY_SAFE);
	    }
	}
	if (errno != EWOULDBLOCK) {
	    perror("cmdsw: input failed");
	}
}

/*
 *  Assume ttysw currently is a cmdsw.
 */
ttysw_be_ttysw(ttysw)
	Ttysw *ttysw;
{
  	extern Menu_item	ttysw_get_scroll_cmd_from_menu_for_ttysw();
  	extern Menu_item
ttysw_get_scroll_cmd_from_menu_for_textsw();
	Menu_item		enable_scroll, disable_scroll;
	Textsw		 	textsw = (Textsw)ttysw->ttysw_hist;
	Menu			textsw_menu;
	int			off = 0;
	
	
	if (textsw && ttysw) {
	    textsw_menu = (Menu)textsw_get(textsw, TEXTSW_MENU);
	    enable_scroll = ttysw_get_scroll_cmd_from_menu_for_ttysw(
		 			  (Menu)ttysw->ttysw_menu);
	    disable_scroll = ttysw_get_scroll_cmd_from_menu_for_textsw(textsw_menu);
	    
	    if (cmdsw->enable_scroll_stay_on) {
	        /* In case this procedure is called by running out of disk space */
	        (void)menu_set(enable_scroll, MENU_INACTIVE, FALSE, 0); 
	        cmdsw->enable_scroll_stay_on = FALSE;
	    } else {
	        /*  In case if this procedure is called by starting up vi */
	    
	        (void)menu_set(enable_scroll, MENU_INACTIVE, TRUE, 0); 
	        
	    }
	    (void)menu_set(disable_scroll, MENU_INACTIVE, TRUE, 0);
	 }   
	
	if (!ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
	    return(-1);
	}
	

	ttysw->ttysw_wfd = (int)window_get((Window)(LINT_CAST(ttysw)), WIN_FD);
	(void)window_set((Window)(LINT_CAST(textsw)), TEXTSW_READ_ONLY, TRUE, 0);
	(void)window_set((Window)(LINT_CAST(ttysw)),
		   WIN_WIDTH,	(int)window_get((Window)(LINT_CAST(textsw)), WIN_WIDTH),
		   WIN_HEIGHT,	(int)window_get((Window)(LINT_CAST(textsw)), WIN_HEIGHT),
		   WIN_X,	(int)window_get((Window)(LINT_CAST(textsw)), WIN_X),
		   WIN_Y,	(int)window_get((Window)(LINT_CAST(textsw)), WIN_Y),
		   0);
	(void)swap_windows((caddr_t)textsw, (caddr_t)ttysw,
			   ttysw->ttysw_ibuf.cb_wbp,
			   ttysw->ttysw_ibuf.cb_ebp);
#ifdef notdef
	tool_kbd_done(window_get(ttysw, WIN_OWNER),(caddr_t)textsw);
#endif notdef
	(void) ioctl(ttysw->ttysw_pty, TIOCREMOTE, &off);
	(void) ioctl(ttysw->ttysw_tty, TIOCGETP, &cmdsw->sgttyb);
	(void) ioctl(ttysw->ttysw_tty, TIOCGETC, &cmdsw->tchars);
	(void) ioctl(ttysw->ttysw_tty, TIOCGLTC, &cmdsw->ltchars);

	(void)drawCursor(0, 0);
	
	if (!ttysw_waiting_for_pty_input) {
	    (void) notify_set_input_func((Notify_client)(LINT_CAST(ttysw)), ttysw_pty_input_pending,
					 ttysw->ttysw_pty);
	    /* Wait for child process to die */
	    ttysw_waiting_for_pty_input = 1;
	}
	
	
	return(0);
}

/*
 *  Assume ttysw currently is a ttysw.
 */
ttysw_be_cmdsw(ttysw)
	Ttysw *ttysw;
{
	extern void	 ttysw_set_scrolling();
	Textsw		 textsw = (Textsw)ttysw->ttysw_hist;
	Menu		 textsw_menu;
	Pixwin		*pw;
	
	if (textsw) {
	    /* Fix up the menu */
	    textsw_menu = (Menu)textsw_get(textsw, TEXTSW_MENU);
	    (void)ttysw_set_scrolling(textsw_menu, (Menu)ttysw->ttysw_menu, 0);
	}
	
	if (ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT) || !textsw) {
		return(-1);
	}    
	
        (void) notify_set_itimer_func(
	    (Notify_client)(LINT_CAST(ttysw)),
	    ttysw_itimer_expired,
	    ITIMER_REAL,
	    (struct itimerval *) 0,
	    (struct itimerval *) 0);

	(void)ttysw_clear(ttysw);

	ttysw->ttysw_wfd = (int)window_get((Window)(LINT_CAST(textsw)), WIN_FD);
	(void)window_set((Window)(LINT_CAST(textsw)), TEXTSW_READ_ONLY, FALSE, 0);
	if (cmdsw->ttysw_resized > 1) {
	    (void)window_set((Window)(LINT_CAST(textsw)),
		       WIN_WIDTH,	(int)window_get((Window)(LINT_CAST(ttysw)), WIN_WIDTH),
		       WIN_HEIGHT,	(int)window_get((Window)(LINT_CAST(ttysw)), WIN_HEIGHT),
		       WIN_X,	(int)window_get((Window)(LINT_CAST(ttysw)), WIN_X),
		       WIN_Y,	(int)window_get((Window)(LINT_CAST(ttysw)), WIN_Y),
		       0);
	    cmdsw->ttysw_resized = 0;
	} else if ((int)window_get((Window)(LINT_CAST(ttysw)), WIN_ROWS) !=
		   (int)window_get((Window)(LINT_CAST(textsw)), WIN_ROWS)
	||	   (int)window_get((Window)(LINT_CAST(ttysw)), WIN_COLUMNS) !=
		   (int)window_get((Window)(LINT_CAST(textsw)), WIN_COLUMNS))
	{
	    (void)ttynewsize(textsw_screen_column_count(textsw),
		       textsw_screen_line_count(textsw));
	}
	pw = (Pixwin *)window_get(textsw, WIN_PIXWIN);
	if (!pw->pw_prretained) 
	    window_set(textsw, TEXTSW_NO_REPAINT_TIL_EVENT, TRUE, 0);
	    
	(void)swap_windows((caddr_t)ttysw, (caddr_t)textsw,
			   ttysw->ttysw_ibuf.cb_wbp,
			   ttysw->ttysw_ibuf.cb_ebp);
	
	(void) ioctl(ttysw->ttysw_tty, TIOCGETC, &cmdsw->tchars);
	(void) ioctl(ttysw->ttysw_tty, TIOCGLTC, &cmdsw->ltchars);
	(void) ttysw_getp(ttysw);

	if (!ttysw_waiting_for_pty_input) {
	    (void) notify_set_input_func((Notify_client)(LINT_CAST(ttysw)), ttysw_pty_input_pending,
					 ttysw->ttysw_pty);
	    /* Wait for child process to die */
	    ttysw_waiting_for_pty_input = 1;
	}
	
	    
	return(0);
}

/*
 *  Do a TIOCGETP on the tty and set cooked_echo mode accordingly.
 */
ttysw_getp(ttysw)
	Ttysw *ttysw;
{
	int			cooked_echo;
    
	(void) ioctl(ttysw->ttysw_tty, TIOCGETP, &cmdsw->sgttyb);
	if (ttysw->ttysw_hist) {
	    cooked_echo = cmdsw->cooked_echo;
	    cmdsw->cooked_echo = cmdsw->sgttyb.sg_flags & ECHO &&
				 !(cmdsw->sgttyb.sg_flags & (RAW|CBREAK));
	    (void)ttysw_cooked_echo(ttysw, cooked_echo, cmdsw->cooked_echo);
	    return (0);
	} else {
	    return (-1);
	}
}

/*
 *  Change cooked_echo mode.
 */
ttysw_cooked_echo(ttysw, old_cooked_echo, new_cooked_echo)
	Ttysw *ttysw;
	int old_cooked_echo, new_cooked_echo;
{
	Textsw	textsw = (Textsw)ttysw->ttysw_hist;
	Textsw_index	length;

	if (old_cooked_echo && !new_cooked_echo) {
	    cmdsw->history_limit =
	        (int)textsw_get(textsw, TEXTSW_HISTORY_LIMIT);
	    (void)textsw_set(textsw, TEXTSW_HISTORY_LIMIT, 0, 0);
	} else if (!old_cooked_echo && new_cooked_echo) {
	    (void)textsw_set(textsw,
	    	TEXTSW_HISTORY_LIMIT, cmdsw->history_limit,
	    	0);
	    /* if insertion point == pty insert point,
	     * move it to the end, doing whatever is necessary to the
	     * read_only_mark.
	     */
	    if (textsw_find_mark(textsw, cmdsw->pty_mark)
	    ==  (int)textsw_get(textsw, TEXTSW_INSERTION_POINT)) {
	    	if (cmdsw->append_only_log) {
			/* Remove read_only_mark to allow insert */
			textsw_remove_mark(textsw, cmdsw->read_only_mark);
	    	}
	    	length = (int)textsw_get(textsw,TEXTSW_LENGTH);
	    	(void)textsw_set(textsw,
			TEXTSW_INSERTION_POINT, length,
			0);
	    	if (cmdsw->append_only_log) {
		    cmdsw->read_only_mark =
			textsw_add_mark(textsw, length, TEXTSW_MARK_READ_ONLY);
	    	}
	    }
	}
	if (new_cooked_echo)
	    new_cooked_echo = 1;
	else if (old_cooked_echo && cmdsw->cmd_started) {
	    ttysw_scan_for_completed_commands(ttysw, -1, 0);
	}
	if (old_cooked_echo != new_cooked_echo
        || !ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
#ifdef DEBUG
	    fprintf(stderr, "REMOTE %d\n", new_cooked_echo);
#endif DEBUG
	    (void) ioctl(ttysw->ttysw_pty, TIOCREMOTE, &new_cooked_echo);
        }
}
