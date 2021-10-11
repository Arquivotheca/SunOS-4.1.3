#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)cmdsw_notify.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Notifier related routines for the cmdsw.
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sgtty.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_notify.h>
#include <suntool/ttysw.h>
#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/alert.h>
#include <suntool/ttysw_impl.h>

extern Textsw	  textsw_first();
extern Textsw	  textsw_next();
extern caddr_t	  textsw_get();
extern int	  textsw_default_notify();
extern char	  *textsw_checkpoint_undo();
extern Textsw_index	textsw_insert();
extern Textsw_status	textsw_set();
extern Textsw_index	textsw_erase();
extern void		textsw_display();

extern Notify_value	ttysw_text_input_pending();
extern void		ttysw_sigwinch();
extern void		ttysw_sendsig();


extern Cmdsw	*cmdsw;

#ifdef DEBUG
#define ERROR_RETURN(val)	abort();	/* val */
#else
#define ERROR_RETURN(val)	return(val);
#endif DEBUG

/* performance: global cache of getdtablesize() */
extern int dtablesize_cache;
#define GETDTABLESIZE() \
	(dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

/* shorthand - Duplicate of what's in ttysw_main.c */

#define	iwbp	ttysw->ttysw_ibuf.cb_wbp
#define	iebp	ttysw->ttysw_ibuf.cb_ebp
#define	ibuf	ttysw->ttysw_ibuf.cb_buf

/* Pkg_private */ Notify_value
ttysw_text_destroy(textsw, status)
    register Textsw     textsw;
    Destroy_status        status;
{
    Ttysw              *ttysw;
    Notify_value	nv = NOTIFY_IGNORED;
    int			last_view;
    

    /* WARNING: call on notify_next_destroy_func invalidates textsw,
     *	and thus the information about the textsw must be extracted first.
     */
    last_view = (textsw_next(textsw_first(textsw)) == (Textsw) 0);
    ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
	/* WARNING: ttysw may be NULL */

    if (status == DESTROY_CHECKING) {
        return (NOTIFY_IGNORED);
    } else {
	nv = notify_next_destroy_func((Notify_client)(LINT_CAST(textsw)), status);
    }
    if (last_view && ttysw) {
	nv = ttysw_destroy(ttysw, status);
    }
    return nv;
}

static Textsw_index
find_and_remove_mark(textsw, mark)
    Textsw		textsw;
    Textsw_mark		mark;
{
    Textsw_index	result;

    result = textsw_find_mark(textsw, mark);
    if (result != TEXTSW_INFINITY)
	textsw_remove_mark(textsw, mark);
    return(result);
}

/* Pkg_private */ void
ttysw_move_mark(textsw, mark, to, flags)
    Textsw		 textsw;
    Textsw_mark		*mark;
    Textsw_index	 to;
    int			 flags;
{
    textsw_remove_mark(textsw, *mark);
    *mark = textsw_add_mark(textsw, to, (unsigned)flags);
}

/* Pkg_private */ Notify_value
ttysw_text_event(textsw, event, arg, type)
    register Textsw	  textsw;
    Event                *event;
    Notify_arg     	  arg;
    Notify_event_type     type;
{
    Ttysw		*ttysw;
    int			 insert = TEXTSW_INFINITY;
    int			 length = TEXTSW_INFINITY;
    int			 cmd_start;
    int			 did_map = 0;
    Notify_value	 nv = NOTIFY_IGNORED;
    Menu		 textsw_menu, cmd_modes_menu;
    Menu_item		 scroll_cmd, cmd_modes_item;
    int			 action = (cmdsw->cooked_echo ? event_action(event) : event_id(event));


    ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
    
    if ((action == MS_RIGHT) && event_is_down(event)) {
    /*
     * To ensure the textsw from MENU_CLIENT_DATA is one where the
     * menu come up, instead of the one the menu is created from.
     *
     **In SV3.4 and later, the menu structure has been modified so that
     * the last cmd in the menu is a pullright menu with the previous
     * two cmds nested within it.
     */
        textsw_menu = (Menu)textsw_get(textsw, TEXTSW_MENU);  /* top level */
        /* The reason for using menu_get() is menu_find() has problem. */
	cmd_modes_item = (Menu_item)menu_get(
	  textsw_menu,
	  MENU_NTH_ITEM,
	  menu_get(textsw_menu, MENU_NITEMS, 0));
        cmd_modes_menu = (Menu)menu_get(
	  cmd_modes_item,
	  MENU_PULLRIGHT,
	  0);
        scroll_cmd = (Menu_item)menu_get(
	  cmd_modes_menu,
	  MENU_NTH_ITEM,
	  menu_get(cmd_modes_menu, MENU_NITEMS, 0)); 
        			
        (void)menu_set(scroll_cmd, MENU_CLIENT_DATA, textsw, 0);
    }    
    
    if (!ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	nv = notify_next_event_func((Notify_client)(LINT_CAST(textsw)), 
		(Notify_event)(LINT_CAST(event)), arg, type);
        return (nv);
    }
    
    if (cmdsw->cooked_echo
    && (cmdsw->cmd_started == 0)
    && (insert = (int)textsw_get(textsw, TEXTSW_INSERTION_POINT))
    == (length = (int)textsw_get(textsw, TEXTSW_LENGTH))) {
   
		(void)textsw_checkpoint_again(textsw);
    }

    if (cmdsw->cooked_echo
    &&  cmdsw->cmd_started
    &&  cmdsw->literal_next
    &&  action <= ASCII_LAST
    && (insert = (int)textsw_get(textsw, TEXTSW_INSERTION_POINT))
    == (length = (int)textsw_get(textsw, TEXTSW_LENGTH))) {
  
		char	input_char = (char)action;

		textsw_replace_bytes(textsw, length-1, length, &input_char, 1);
		cmdsw->literal_next = FALSE;
		return NOTIFY_DONE;
    }

    /* ^U after prompt, before newline should only erase back to prompt. */
    if (cmdsw->cooked_echo
    &&  event_id(event) == (short)cmdsw->erase_line
    &&  event_is_down(event)
    && !event_shift_is_down(event)
    &&  cmdsw->cmd_started != 0
    &&  ((insert = (int)textsw_get(textsw, TEXTSW_INSERTION_POINT))) >
        (cmd_start = (int)textsw_find_mark(textsw, cmdsw->user_mark))) {

        int			pattern_start = cmd_start;
        int			pattern_end = cmd_start;
        char			newline = '\n';

        if (textsw_find_bytes(
            textsw, (long *)&pattern_start, (long *)&pattern_end,
            &newline, 1, 0) == -1
        || (pattern_start <= cmd_start || pattern_start >= (insert-1))) {
	    (void)textsw_erase(textsw,
	        (Textsw_index)cmd_start, (Textsw_index)insert);
	    return NOTIFY_DONE;
        }
    }

    if (!cmdsw->cooked_echo
    &&  (action == '\r'|| action == '\n')
    &&  (event->ie_shiftmask & CTRLMASK)
    &&  (win_get_vuid_value(ttysw->ttysw_wfd, 'm') == 0)
    &&  (win_get_vuid_value(ttysw->ttysw_wfd, 'j') == 0)
    &&  (win_get_vuid_value(ttysw->ttysw_wfd, 'M') == 0)
    &&  (win_get_vuid_value(ttysw->ttysw_wfd, 'J') == 0) ) {

		/* Implement "go to end of file" ourselves. */
		/* First let textsw do it to get at normalize_internal. */
		nv = notify_next_event_func((Notify_client)(LINT_CAST(textsw)), 
			(Notify_client)(LINT_CAST(event)), arg, type);

	/* Now fix it up.
	 * Only necessary when !append_only_log because otherwise
	 * the read-only mark at INFINITY-1 gets text to implement
	 * this function for us.
	 */
	if (!cmdsw->append_only_log)
	    (void)textsw_set(textsw, TEXTSW_INSERTION_POINT,
		       textsw_find_mark(textsw, cmdsw->pty_mark), 0);

    } else if (!cmdsw->cooked_echo
    &&  action <= ASCII_LAST 
    &&  (iscntrl((char)action) || (char)action == '\177')
    &&  (insert = (int)textsw_get(textsw, TEXTSW_INSERTION_POINT))
    ==  textsw_find_mark(textsw, cmdsw->pty_mark)) {
   
		/* In !cooked_echo, ensure textsw doesn't gobble up control chars */
		char	input_char = action;	
		
		(void)textsw_insert(textsw, &input_char, (long) 1);
		nv = NOTIFY_DONE;

    } else if (cmdsw->cooked_echo
    &&  action == cmdsw->tchars.t_stopc) {
   
		/* implement flow control characters as page mode */
		(void) ttysw_freeze(ttysw, 1);

    } else if (cmdsw->cooked_echo
    &&  action == cmdsw->tchars.t_startc) {
  
		(void) ttysw_freeze(ttysw, 0);
		(void) ttysw_reset_conditions(ttysw);

    } else if (!cmdsw->cooked_echo
    ||  action != (short)cmdsw->tchars.t_eofc) {

		/* Nice normal event */
		nv = notify_next_event_func((Notify_client)(LINT_CAST(textsw)), 
			(Notify_event)(LINT_CAST(event)), arg, type);
    }
   
    if (nv == NOTIFY_IGNORED) {
		if ((action > META_LAST) &&
		    (action < LOC_MOVE || action > WIN_UNUSED_15)) {

		    did_map = (ttysw_domap(ttysw, event) == TTY_DONE);
		    nv = did_map ? NOTIFY_DONE : NOTIFY_IGNORED;
		}
    }
  
    /* the following switch probably belongs in a state transition table */
    switch (action) {
	    case WIN_REPAINT:
			ttysw_sigwinch(ttysw);
			nv = NOTIFY_DONE;
			break;
		case WIN_RESIZE:
			(void)ttysw_resize(ttysw);
			nv = NOTIFY_DONE;
			break;
		case KBD_USE:
			if ((Textsw)ttysw->ttysw_hist != textsw) {
			    (Textsw)ttysw->ttysw_hist = textsw;
			    (void)ttynewsize(textsw_screen_column_count(textsw),
				textsw_screen_line_count(textsw));
			    ttysw->ttysw_wfd =
			    	(int)window_get((Window)(LINT_CAST(textsw)), WIN_FD);
			}
			break;
		case KBD_DONE:
			break;
		case LOC_MOVE:
			break;
	    case LOC_STILL:
			break;
	    case LOC_WINENTER:
		 	break;
	    case LOC_WINEXIT:
			break;
	    default:
#ifdef DEBUG
	if (action <= ASCII_LAST) {
	    int		ctrl_state = event->ie_shiftmask & CTRLMASK;
	    int		shift_state = event->ie_shiftmask & SHIFTMASK;
	    char	ie_code = action;
	}
#endif DEBUG
			if (!cmdsw->cooked_echo)
		   	    break;
			/* Only send interrupts when characters are actually typed. */
			if (action == cmdsw->tchars.t_intrc) {
			    ttysw_sendsig(ttysw, textsw, SIGINT);
			} else if (action == cmdsw->tchars.t_quitc) {
			    ttysw_sendsig(ttysw, textsw, SIGQUIT);
			} else if (action == cmdsw->ltchars.t_suspc
			       ||  action == cmdsw->ltchars.t_dsuspc) {
			    ttysw_sendsig(ttysw, textsw, SIGTSTP);
			} else if (action == cmdsw->tchars.t_eofc) {
			    if (insert == TEXTSW_INFINITY)
				insert = (int)textsw_get(textsw, TEXTSW_INSERTION_POINT);
			    if (length == TEXTSW_INFINITY)
				length = (int)textsw_get(textsw, TEXTSW_LENGTH);
			    if (length == insert) {
				/* handle like newline or carriage return */
				if (cmdsw->cmd_started
				&&  length > textsw_find_mark(textsw, cmdsw->user_mark)) {
				    if (ttysw_scan_for_completed_commands(ttysw, -1, 0))
						nv = NOTIFY_IGNORED;
					} else {
					    /* but remember to send eot. */
					    cmdsw->pty_eot = iwbp - ibuf;
					    cmdsw->cmd_started = 0;
					    (void)ttysw_reset_conditions(ttysw);
					}
			    } else {   /* length != insert */
					nv = notify_next_event_func(
						(Notify_client)(LINT_CAST(textsw)), 
						(Notify_client)(LINT_CAST(event)), arg, type);
			    }
			}
    }   /* switch */
    return(nv);
}

static Notify_value
ttysw_cr(ttysw, tty)
    Ttysw                *ttysw;
    int                   tty;
{
    int			  nfds = 0;
    fd_set		  wfds;
    static struct timeval timeout = {0, 0};
    int maxfds	= GETDTABLESIZE();
    
    /*
     *  GROSS HACK:
     *
     *  There is a race condition such that between the notifier's
     *  select() call and our write, the app may write to the tty,
     *  causing our write to block.  The tty cannot be flushed because
     *  we don't get to read the pty because our write is blocked.
     *  This GROSS HACK doesn't eliminate the race condition; it merely
     *  narrows the window, making it less likely to occur.
     *  We don't do an fcntl(tty, FN_NODELAY) because that affects the
     *  file, not merely the file descriptor, and we don't want to change
     *  what the application thinks it sees.
     *
     *  The right solution is either to invent an ioctl that will allow
     *  us to set the tty driver's notion of the cursor position, or to
     *  avoid using the tty driver altogether.
     */
    FD_ZERO(&wfds);
    FD_SET(tty, &wfds);
    if ((nfds = select(maxfds, NULL, &wfds, NULL, &timeout)) < 0) {
	perror("ttysw_cr: select");
	return (NOTIFY_IGNORED);
    }
    /* if (nfds == 0 || !((1 << tty) & wfds)) { */
    if (nfds == 0 || !FD_ISSET(tty, &wfds)) {
	return (NOTIFY_IGNORED);
    }
    if (write(tty, "\r", 1) < 0) {
	fprintf(stderr, "for ttysw %x, tty fd %d, ",
		ttysw, ttysw->ttysw_tty);
	perror("TTYSW tty write failure");
    }
    (void) notify_set_output_func((Notify_client)(LINT_CAST(ttysw)), 
    	NOTIFY_FUNC_NULL, tty);
    return (NOTIFY_DONE);
}

static void
ttysw_reset_column(ttysw)
    Ttysw                *ttysw;
{
    if ((cmdsw->sgttyb.sg_flags & XTABS)
    &&  notify_get_output_func((Notify_client)(LINT_CAST(ttysw)),
				ttysw->ttysw_tty) != ttysw_cr) {
	if (notify_set_output_func((Notify_client)(LINT_CAST(ttysw)),
	    ttysw_cr, ttysw->ttysw_tty) == NOTIFY_FUNC_NULL) {
	    fprintf(stderr,
	    	    "cannot set output func on ttysw %x, tty fd %d\n",
	    	    ttysw, ttysw->ttysw_tty);
	}
    }
}

/* Pkg_private */ int
ttysw_scan_for_completed_commands(ttysw, start_from, maybe_partial)
    Ttysw		*ttysw;
    int			 start_from;
    int			 maybe_partial;
{
    register Textsw	 textsw = (Textsw)ttysw->ttysw_hist;
    register char	*cp;
    int			 length = (int)textsw_get(textsw, TEXTSW_LENGTH);
    int			 use_mark = (start_from == -1);
    int			 cmd_length;

    if (use_mark) {
	if (TEXTSW_INFINITY == (
		start_from = textsw_find_mark(textsw, cmdsw->user_mark) ))
		ERROR_RETURN(1);
	if (start_from == length)
	    return(0);
    }
    cmd_length = length - start_from;
    /* Copy these commands into the buffer for pty */
    if ((iwbp+cmd_length) < iebp) {
	(void) textsw_get(textsw,
		TEXTSW_CONTENTS, start_from, iwbp, cmd_length);
	if (maybe_partial) {
	    /* Discard partial commands. */
	    for (cp = iwbp+cmd_length-1; cp >= iwbp; --cp) {
		switch (*cp) {
		  case '\n':
		  case '\r':
		    goto Done;
		  default:
		    if (*cp == cmdsw->tchars.t_brkc)
		      goto Done;
		    cmd_length--;
		    break;
		}
	    }
	}
Done:
	if (cmd_length > 0) {
	    iwbp += cmd_length;
	    cp = iwbp-1;
	    (void)ttysw_reset_conditions(ttysw);
	    if (*cp == '\n'
	    ||  *cp == '\r') {
		ttysw_reset_column(ttysw);
	    }
	    ttysw_move_mark(textsw, &cmdsw->pty_mark,
	    	      (Textsw_index)(start_from+cmd_length),
		      TEXTSW_MARK_DEFAULTS);
	    if (cmdsw->cmd_started) {
		if (start_from+cmd_length < length) {
		    ttysw_move_mark(textsw, &cmdsw->user_mark,
			      (Textsw_index)(start_from+cmd_length),
			      TEXTSW_MARK_DEFAULTS);
		} else {
		    cmdsw->cmd_started = 0;
		}
		if (cmdsw->append_only_log) {
		    ttysw_move_mark(textsw, &cmdsw->read_only_mark,
			      (Textsw_index)(start_from+cmd_length),
			      TEXTSW_MARK_READ_ONLY);
		}
	    }
	    cmdsw->pty_owes_newline = 0;
	}
	return(0);
    } else {
	Event	alert_event;

	(void) alert_prompt(
		(Frame)window_get(textsw, WIN_OWNER),
		&alert_event,
		ALERT_MESSAGE_STRINGS, 
			"Pty cmd buffer overflow: last cmd ignored.",
			0,
		ALERT_BUTTON_YES,	"Continue",
		ALERT_TRIGGER,		ACTION_STOP,
		0);
	return(0);
    }
}

/* Pkg_private */ void
ttysw_doing_pty_insert(textsw, commandsw, toggle)
    register Textsw	 textsw;
    Cmdsw		*commandsw;
    unsigned		 toggle;
{
    unsigned		 notify_level = (unsigned) window_get(textsw,
    					 TEXTSW_NOTIFY_LEVEL);
    commandsw->doing_pty_insert = toggle;
    if (toggle) {
	window_set(textsw,
	    TEXTSW_NOTIFY_LEVEL, notify_level & (~TEXTSW_NOTIFY_EDIT),
	    0);
    } else {
	window_set(textsw,
	    TEXTSW_NOTIFY_LEVEL, notify_level | TEXTSW_NOTIFY_EDIT,
	    0);
    }
}

int
ttysw_cmd(ttysw_opaque, buf, buflen)
    caddr_t             ttysw_opaque;
    char               *buf;
    int                 buflen;
{
    Ttysw		*ttysw = (Ttysw *) LINT_CAST(ttysw_opaque);
    
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)
    &&  cmdsw->cooked_echo) {
	return (ttysw_cooked_echo_cmd(ttysw_opaque, buf, buflen));
    } else {
	return (ttysw_input((caddr_t)ttysw, buf, buflen));
    }
}

/* Pkg_private */ int 
ttysw_cooked_echo_cmd(ttysw_opaque, buf, buflen)
    caddr_t             ttysw_opaque;
    char               *buf;
    int                 buflen;
{
    Ttysw		*ttysw = (Ttysw *) LINT_CAST(ttysw_opaque);
    register Textsw	 textsw = (Textsw)ttysw->ttysw_hist;
    Textsw_index	 insert = (Textsw_index)textsw_get(textsw,
						  TEXTSW_INSERTION_POINT);
    int			 length = (Textsw_index)textsw_get(textsw,
						  TEXTSW_LENGTH);
    Textsw_index	 insert_at;
    Textsw_mark		 insert_mark;

    if (cmdsw->append_only_log) {
	textsw_remove_mark(textsw, cmdsw->read_only_mark);
    }
    if (cmdsw->cmd_started) {
	insert_at = find_and_remove_mark(textsw, cmdsw->user_mark);
	if (insert_at == TEXTSW_INFINITY)
	    ERROR_RETURN(-1);
	if (insert == insert_at) {
	    insert_mark = TEXTSW_NULL_MARK;
	} else {
	    insert_mark =
		textsw_add_mark(textsw, insert, TEXTSW_MARK_DEFAULTS);
	}
    } else {
	if (insert == length)
	    (void)textsw_checkpoint_again(textsw);
	    cmdsw->next_undo_point = textsw_checkpoint_undo(textsw,
	           		(caddr_t)TEXTSW_INFINITY);
	insert_at = length;
    }
    if (insert != insert_at) {
	(void)textsw_set(textsw, TEXTSW_INSERTION_POINT, insert_at, 0);
    }
    (void)textsw_checkpoint_undo(textsw, cmdsw->next_undo_point);
    /* Stop this insertion from triggering the cmd scanner! */
    ttysw_doing_pty_insert(textsw, cmdsw, TRUE);
    (void)textsw_insert(textsw, buf, (long) buflen);
    ttysw_doing_pty_insert(textsw, cmdsw, FALSE);
    (void) ttysw_scan_for_completed_commands(ttysw, (int) insert_at, TRUE);
    if (cmdsw->cmd_started) {
	insert_at = (Textsw_index)textsw_get(textsw, TEXTSW_INSERTION_POINT);
	if (insert_at == TEXTSW_INFINITY)
	    ERROR_RETURN(-1);
	cmdsw->user_mark =
	    textsw_add_mark(textsw, (Textsw_index)insert_at, TEXTSW_MARK_DEFAULTS);
	if (cmdsw->append_only_log) {
	    cmdsw->read_only_mark =
		textsw_add_mark(textsw,
		    cmdsw->cooked_echo ? insert_at : TEXTSW_INFINITY-1,
		    TEXTSW_MARK_READ_ONLY);
	}
	if (insert_mark != TEXTSW_NULL_MARK) {
	    insert = find_and_remove_mark(textsw, insert_mark);
	    if (insert == TEXTSW_INFINITY)
		ERROR_RETURN(-1);
	    (void)textsw_set(textsw, TEXTSW_INSERTION_POINT, insert, 0);
	}
    } else {
	if (insert < length)
	    (void)textsw_set(textsw, TEXTSW_INSERTION_POINT, insert, 0);
	if (cmdsw->append_only_log) {
	    length = (int)textsw_get(textsw, TEXTSW_LENGTH);
	    cmdsw->read_only_mark =
		textsw_add_mark(textsw,
		    (Textsw_index)(cmdsw->cooked_echo ? length : TEXTSW_INFINITY-1),
		    TEXTSW_MARK_READ_ONLY);
	}
    }
    return (0);
}

/* ARGSUSED */
static void
ttysw_textsw_changed_handler(textsw, insert_before, length_before,
                             replaced_from, replaced_to, count_inserted)
    Textsw		textsw;
    int			insert_before;
    int			length_before;
    int			replaced_from;
    int			replaced_to;
    int			count_inserted;
{
    Ttysw              *ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw,
    					  TEXTSW_CLIENT_DATA));
    char		last_inserted;
    
    if (insert_before != length_before)
        return;
    if (cmdsw->cmd_started == 0) {
	if (cmdsw->cmd_started = (count_inserted > 0)) {
	    (void)textsw_checkpoint_undo(textsw, cmdsw->next_undo_point);
	    ttysw_move_mark(textsw, &cmdsw->user_mark,
		      (Textsw_index)length_before,
		      TEXTSW_MARK_DEFAULTS);
	}
    }
    if (!cmdsw->cmd_started)
	cmdsw->next_undo_point =
	       (caddr_t)textsw_checkpoint_undo(textsw,
			(caddr_t)TEXTSW_INFINITY);
    if (count_inserted >= 1) {
        /* Get the last inserted character. */
        (void)textsw_get(textsw, TEXTSW_CONTENTS,
                   replaced_from + count_inserted - 1,
                   &last_inserted, 1);
	if (last_inserted == cmdsw->ltchars.t_rprntc) {
#ifndef	BUFSIZE
#define	BUFSIZE 1024
#endif	BUFSIZE
	    char 		buf[BUFSIZE+1];
	    char 		cr_nl[3];
	    int			buflen = 0;
	    Textsw_index	start_from;
	    Textsw_index	length =
	    	(int)textsw_get(textsw, TEXTSW_LENGTH);
	    
	    cr_nl[0] = '\r';  cr_nl[1] = '\n';  cr_nl[2] = '\0';
	    start_from = textsw_find_mark(textsw, cmdsw->user_mark);
	    if (start_from == (length - 1)) {
		*buf = '\0';
	    } else {
		(void)textsw_get(textsw, TEXTSW_CONTENTS,
                   start_from, buf,
                   (buflen = min(BUFSIZE, length - 1 - start_from)));
            }
            cmdsw->pty_owes_newline = 0;
            cmdsw->cmd_started = 0;
            ttysw_move_mark(textsw, &cmdsw->pty_mark, length,
		      TEXTSW_MARK_DEFAULTS);
            if (cmdsw->append_only_log) {
		ttysw_move_mark(textsw, &cmdsw->read_only_mark, length,
			  TEXTSW_MARK_READ_ONLY);
	    }
	    ttysw_output(ttysw, cr_nl, 2);
	    if (buflen > 0)
		ttysw_input(ttysw, buf, buflen);
	} else if (last_inserted == cmdsw->ltchars.t_lnextc) {
	    cmdsw->literal_next = TRUE;
	} else if (last_inserted == cmdsw->tchars.t_brkc
	||  last_inserted == '\n'
	||  last_inserted == '\r') {
	    (void) ttysw_scan_for_completed_commands(ttysw, -1, 0);
	}
    }
}

/* Pkg_private */ int
ttysw_textsw_changed(textsw, attributes)
    Textsw		textsw;
    Attr_avlist		attributes;
{
    register Attr_avlist	 attrs;
    register Ttysw		*ttysw;
    int				 do_default = 0;

    for (attrs = attributes; *attrs; attrs = attr_next(attrs)) {
        switch ((Textsw_action)(*attrs)) {
	  case TEXTSW_ACTION_CAPS_LOCK:
	    ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
	    ttysw->ttysw_capslocked = (attrs[1] != 0)? TTYSW_CAPSLOCKED: 0;
	    (void)ttysw_display_capslock(ttysw);
	    break;
          case TEXTSW_ACTION_REPLACED:
            if (!cmdsw->doing_pty_insert)
		ttysw_textsw_changed_handler(textsw,
		    (int)attrs[1], (int)attrs[2], (int)attrs[3],
		    (int)attrs[4], (int)attrs[5]);
            break;
          case TEXTSW_ACTION_SPLIT_VIEW: {
            int				 text_fd;

	    ttysw = (Ttysw *)
	        LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
	    (void) notify_interpose_event_func(attrs[1], ttysw_text_event,
		                               NOTIFY_SAFE);
	    (void) notify_interpose_destroy_func(attrs[1],
		                                 ttysw_text_destroy);
	    text_fd = (int)
	        LINT_CAST(window_get((Textsw)(LINT_CAST(attrs[1])), WIN_FD));
	    (void) notify_interpose_input_func((Notify_client)attrs[1],
		ttysw_text_input_pending, text_fd);
	    if ((Ttysw *)
	        LINT_CAST(textsw_get(
	        (Textsw)(LINT_CAST(attrs[1])), TEXTSW_CLIENT_DATA))
	    !=  ttysw)
		(void)textsw_set(
		(Textsw)(LINT_CAST(attrs[1])), TEXTSW_CLIENT_DATA, ttysw, 0);
	    if ((int)window_get(textsw, WIN_KBD_FOCUS)
	    &&  (int)window_get((Textsw)(LINT_CAST(attrs[1])), WIN_Y) >
	        (int)window_get(textsw, WIN_Y))
	        (void)window_set(
	        (Textsw)(LINT_CAST(attrs[1])), WIN_KBD_FOCUS, TRUE, 0);
            break;
	  }
          case TEXTSW_ACTION_DESTROY_VIEW: {
            Textsw	coalesce_with = NULL;
            /*
             *  There is no way for the user to directly destroy
             *  the current view if it is replaced by the ttysw.
             *  Therefore, he tried to destroy the first view and
             *  textsw decided to destroy this view instead.
             */
	    ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
	    if (! (coalesce_with = (Textsw) window_get(textsw,
		TEXTSW_COALESCE_WITH))) {
		coalesce_with = textsw_first(textsw);
	    }
	    if ((ttysw != NULL) && (Textsw)ttysw->ttysw_hist == textsw) {
		ttysw->ttysw_hist = (FILE *) LINT_CAST(coalesce_with);
	    }
	    if ((int)window_get(textsw, WIN_KBD_FOCUS)) {
		window_set(coalesce_with, WIN_KBD_FOCUS, TRUE, 0);
            }
            break;
          }
          case TEXTSW_ACTION_LOADED_FILE: {
            Textsw_index		insert;
            Textsw_index		length;
            
	    ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
            insert =
		(Textsw_index)textsw_get(textsw, TEXTSW_INSERTION_POINT);
	    length = (Textsw_index)textsw_get(textsw, TEXTSW_LENGTH);
	    if (length == insert+1) {
		(void)textsw_set(textsw, TEXTSW_INSERTION_POINT, length, 0);
		ttysw_reset_column(ttysw);
	    } else if (length == 0) {
		ttysw_reset_column(ttysw);
	    }
	    if (length < textsw_find_mark(textsw, cmdsw->pty_mark)) {
		ttysw_move_mark(textsw, &cmdsw->pty_mark, length,
			  TEXTSW_MARK_DEFAULTS);
	    }
	    if (cmdsw->append_only_log) {
		ttysw_move_mark(textsw, &cmdsw->read_only_mark,
			  length, TEXTSW_MARK_READ_ONLY);
	    }
	    cmdsw->cmd_started = FALSE;
	    cmdsw->pty_owes_newline = 0;
	  }
          default:
            do_default = TRUE;
            break;
        }
    }
    if (do_default) {
	(void)textsw_default_notify(textsw, attributes);
    }
}
