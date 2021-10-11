#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_input.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * User input interpreter for text subwindows.
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <suntool/entity_view.h>
#include <errno.h>
#include <sundev/kbd.h>
	/* Only needed for SHIFTMASK and CTRLMASK */
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_input.h> /* needed for event_shift_is_down() */
#ifdef KEYMAP_DEBUG
#include "../../libsunwindow/win/win_keymap.h"
#else
#include <sunwindow/win_keymap.h>
#endif

extern int		errno;

extern Key_map_handle	textsw_do_filter();
extern struct pixrect 	*textsw_get_stopsign_icon();
extern struct pixrect	*textsw_get_textedit_icon();
extern Textsw_enum	textsw_get_menu_style_internal();
extern int		textsw_load_done_proc();
pkg_private void	textsw_init_timer();

#define SPACE_CHAR 0x20

pkg_private int
textsw_flush_caches(view, flags)
	register Textsw_view	view;
	register int		flags;
{
	register Textsw_folio	textsw = FOLIO_FOR_VIEW(view);
	register int		count;
	register int		end_clear = (flags & TFC_SEL);

	count = (textsw->func_state & TXTSW_FUNC_FILTER)
		? 0
		: (textsw->to_insert_next_free - textsw->to_insert);
	if (flags & TFC_DO_PD) {
	    if ((count > 0) || ((flags & TFC_PD_IFF_INSERT) == 0)) {
		ev_set(0, textsw->views, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
		(void) textsw_do_pending_delete(view, EV_SEL_PRIMARY,
						end_clear|TFC_INSERT);
		ev_set(0, textsw->views, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
		end_clear = 0;
	    }
	}
	if (end_clear) {
	    if ((count > 0) || ((flags & TFC_SEL_IFF_INSERT) == 0)) {
		(void) textsw_set_selection(
			    VIEW_REP_TO_ABS(view),
			    ES_INFINITY, ES_INFINITY, EV_SEL_PRIMARY);
	    }
	}
	if (flags & TFC_INSERT) {
	    if (count > 0) {
		/*   WARNING!  The cache pointers must be updated BEFORE
		 * calling textsw_do_input, so that if the client is being
		 * notified of edits and it calls textsw_get, it will not
		 * trigger an infinite recursion of textsw_get calling
		 * textsw_flush_caches calling textsw_do_input calling
		 * the client calling textsw_get calling ...
		 */
		textsw->to_insert_next_free = textsw->to_insert;
		(void) textsw_do_input(view, textsw->to_insert, count, 
		                       TXTSW_UPDATE_SCROLLBAR_IF_NEEDED);
	    }
	}
}

pkg_private void
textsw_read_only_msg(textsw, locx, locy)
	Textsw_folio	textsw;
	int		locx, locy;
{
	(void) alert_prompt(
	    (Frame)window_get(VIEW_FROM_FOLIO_OR_VIEW(textsw), WIN_OWNER),
	    (Event *) 0,
	    ALERT_MESSAGE_STRINGS,
	        "The text is read-only and cannot be edited.",
		"Press \"Continue\" to proceed.",
		0,
	    ALERT_BUTTON_YES,	"Continue",
	    ALERT_TRIGGER,	ACTION_STOP,
	    0);
}

pkg_private int
textsw_note_event_shifts(textsw, ie)
	register Textsw_folio		 textsw;
	register struct inputevent	*ie;
{
	int				 result = 0;

	if (ie->ie_shiftmask & SHIFTMASK)
	    textsw->state |= TXTSW_SHIFT_DOWN;
	else {
#ifdef VT_100_HACK
	    if (textsw->state & TXTSW_SHIFT_DOWN) {
		/* Hack for VT-100 keyboard until PIT is available. */
		result = 1;
	    }
#endif
	    textsw->state &= ~TXTSW_SHIFT_DOWN;
	}
	if (ie->ie_shiftmask & CTRLMASK)
	    textsw->state |= TXTSW_CONTROL_DOWN;
	else
	    textsw->state &= ~TXTSW_CONTROL_DOWN;
	return(result);
}

#ifdef GPROF
static
textsw_gprofed_routine(view, ie) 
	register Textsw_view		 view;
	register Event			*ie;
{
}
#endif

#define CTRL_D	004
#define CTRL_F	006
#define CTRL_G	007
#define CTRL_P	020

extern int
textsw_process_event(abstract, ie, arg)
	Textsw		 abstract;
	register Event	*ie;
	Notify_arg	 arg;
{
	extern void			 textsw_update_scrollbars();
	static int			 textsw_win_event();
	static int			 textsw_scroll_event();
	static int			 textsw_function_key_event();
	static int			 textsw_mouse_event();
	static int			 textsw_edit_function_key_event();
	static int			 textsw_caret_motion_event();
	static int			 textsw_field_event();

	int				 caret_was_up;
	int				 result = TEXTSW_PE_USED;
	register Textsw_view		 view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio		 textsw = FOLIO_FOR_VIEW(view);
	char				 current_selection[200], err_msg[300];
	int				 action = event_action(ie);
/*
 * Since the keymapping system no longer supports full translation of event
 * ids, shiftmasks and flags we must revert to keymapping 3.x accelerators
 * directly.  This stuff should really be coalesced together somewhere
 * rather than these local patches.
 */
	static short keyboard_accel_state_cached;
	static short keyboard_accel_state;
	static int is_cmdtool;

	if (!keyboard_accel_state_cached) {
	    keyboard_accel_state = (!strcmp("Enabled",
		    (char *) defaults_get_string(
			"/Compatibility/New_keyboard_accelerators",
			"Enabled", 0))) ? 1 : 0;
	    keyboard_accel_state_cached = 1;
	    is_cmdtool = (textsw->state & TXTSW_NO_RESET_TO_SCRATCH);
	}
	if (!keyboard_accel_state) {
	    switch (action) {
	      case CTRL_D:
		if (is_cmdtool) break;
	        event_set_id(ie, TXTSW_DELETE);
	        event_set_shiftmask(ie, 0);
	        break;
	      case CTRL_F:
		if (is_cmdtool) break;
		/* The FIND function expects up/down transitions but ascii
		 * up transitions are not generated so we must synthesize
		 * the appropriate up event */
	        if (event_shift_is_down(ie)) {
	            event_set_id(ie, TXTSW_FIND_BACKWARD);
	            event_set_shiftmask(ie, LEFTSHIFT|RIGHTSHIFT);
	            textsw_function_key_event(view, ie, &result);
	            event_set_up(ie);
	        } else {
	            event_set_id(ie, TXTSW_FIND_FORWARD);
	            event_set_shiftmask(ie, 0);
	            /*textsw_function_key_event(view, ie, &result);
	            event_set_up(ie);*/
	        }
	        break;
	      case CTRL_G:
		if (is_cmdtool) break;
	        event_set_id(ie, TXTSW_GET);
	        event_set_shiftmask(ie, 0);
	        break;
	      case CTRL_P:
	        event_set_id(ie, TXTSW_PUT_THEN_GET);
	        event_set_shiftmask(ie, 0);
	        break;
	      default:
		break;
	    }
	}

	caret_was_up = textsw->caret_state & TXTSW_CARET_ON;
	/* Watch out for caret turds */
	if ((action == LOC_MOVE
	||   action == LOC_STILL
	||   action == LOC_WINENTER
	||   action == LOC_WINEXIT
	||   action == LOC_RGNENTER
	||   action == LOC_RGNEXIT
	||   action == SCROLL_ENTER
	||   action == SCROLL_EXIT)
	&&  !TXTSW_IS_BUSY(textsw)) {
	    /* leave caret up */
	} else 
        if ( !((action == TXTSW_MOVE_RIGHT
        || action == TXTSW_MOVE_LEFT
        || action == TXTSW_MOVE_UP
        || action == TXTSW_MOVE_DOWN
        || action == TXTSW_MOVE_WORD_BACKWARD
        || action == TXTSW_MOVE_WORD_FORWARD
        || action == TXTSW_MOVE_WORD_END
        || action == TXTSW_MOVE_TO_LINE_START
        || action == TXTSW_MOVE_TO_LINE_END
        || action == TXTSW_MOVE_TO_NEXT_LINE_START
        || action == TXTSW_MOVE_TO_DOC_START
        || action == TXTSW_MOVE_TO_DOC_END)
        && !win_inputposevent(ie)) )
        {
	    textsw_take_down_caret(textsw);
	}
	switch (textsw_note_event_shifts(textsw, ie)) {
#ifdef VT_100_HACK
	  case 1:
	    if (textsw->func_state & TXTSW_FUNC_GET) {
		textsw_end_get(view);
	    }
#endif
	  default:
	    break;
	}

        /* NOTE: This is just a hack for performance */
        if ((action >= SPACE_CHAR) && (action <= ASCII_LAST))
            goto Process_ASCII;

	if (action == TXTSW_STOP) {
	    textsw_abort(textsw);
	} else if (action == TXTSW_HELP) {
	    if (win_inputposevent(ie))
		help_request(WINDOW_FROM_VIEW(view), textsw->help_data, ie);
#ifdef GPROF
	} else if (action == KEY_RIGHT(13)) {
	    if (win_inputposevent(ie)) {
		if (textsw->state & TXTSW_SHIFT_DOWN) {
		    moncontrol(0);
		}
	    } else {
		if ((textsw->state & TXTSW_SHIFT_DOWN) == 0) {
		    moncontrol(1);
		}
	    }
	} else if (action == KEY_RIGHT(15)) {
	    if (win_inputposevent(ie)) {
	    } else {
		moncontrol(1);
		textsw_gprofed_routine(view, ie);
		moncontrol(0);
	    }
#endif
	/*
	 * Check Resize/Repaint early to avoid skipping due to 2nd-ary Seln. 
	 */
	} else if (textsw_win_event(view, ie, caret_was_up)) {	
	    /* Its already taken care by the procedure */

	} else if (textsw_scroll_event(view, ie, arg)) {
	    /* Its already taken care by the procedure */

	} else if ((textsw->track_state &
		    (TXTSW_TRACK_ADJUST|TXTSW_TRACK_POINT)) &&
		   track_selection(view, ie) ) {
	/*
	 * Selection tracking and function keys.
	 *   track_selection() always called if tracking.  It consumes
	 * the event unless function key up while tracking secondary
	 * selection, which stops tracking and then does function.
	 */
	} else if (textsw_function_key_event(view, ie, &result)) {
		/* Its already taken care by the procedure */

	} else if (textsw_mouse_event(view, ie)) { 
	    /* Its already taken care by the procedure */

	} else if (textsw_edit_function_key_event(view, ie, &result)) {
	    /* Its already taken care by the procedure */

	} else if (textsw_caret_motion_event (view, ie, &action)) {
	    /* Its already taken care by the procedure */

	} else if (action == TXTSW_CAPS_LOCK) {
	    if (TXTSW_IS_READ_ONLY(textsw))
		goto Read_Only;
	    if (win_inputposevent(ie)) {
	    } else {
		textsw->state ^= TXTSW_CAPS_LOCK_ON;
		textsw_notify(view, TEXTSW_ACTION_CAPS_LOCK,
			       (textsw->state & TXTSW_CAPS_LOCK_ON), 0);
	    }	    

	/*
	 * Type-in
	 */
	} else if (textsw->track_state & TXTSW_TRACK_SECONDARY) {
	    /* No type-in processing during secondary (function) selections */
	} else if (textsw_field_event(view, ie, &action)) {
	    /* Its already taken care by the procedure */

	} else if (action == TXTSW_PUT_THEN_GET) {
	    if (win_inputposevent(ie)) {
	        if (TXTSW_IS_READ_ONLY(textsw))
		    goto Read_Only;
	        textsw_put_then_get(view);
	    }

	} else if (action == TXTSW_EMPTY_DOCUMENT) {
	    if (win_inputposevent(ie)) {
	        textsw_empty_document(abstract, ie);
	        goto Done;
	    }

	} else if (action == TXTSW_INCLUDE_FILE) {
	    int		primary_selection_exists = textsw_is_seln_nonzero(
				textsw, EV_SEL_PRIMARY);

	    if (win_inputposevent(ie)) {
		if (primary_selection_exists) {
		    textsw_file_stuff(view, ie->ie_locx, ie->ie_locy);
	        } else
		    textsw_post_need_selection(abstract, ie);
	    }

	} else if (action == TXTSW_LOAD_FILE_AS_MENU) {
	    if (win_inputposevent(ie)) {
	        textsw_load_file_as_menu(abstract, ie);
	        goto Done;
	    }
	
	} else if (action == TXTSW_LOAD_FILE) {
	    int		event_is_shifted = event_shift_is_down(ie);
	    int		result;

	    if (win_inputposevent(ie)) {
	        if ((textsw->state & TXTSW_NO_LOAD) && (!(event_is_shifted)))
		    goto Insert;
	        textsw_flush_caches(view, TFC_STD);

	        result = textsw_handle_esc_accelerators(abstract, ie);
		switch (result) {
		    case (0):
		        goto Read_Only;
		    case (1):
			goto Insert;
		  /*default:  ie == 2 
			let drop through */
		}
	    }

	} else if (textsw_file_operation(view, ie)) {

	} else if ((action <= META_LAST || 
                        (action >= ISO_FIRST && action <= ISO_LAST) ||
			action == ACTION_ERASE_CHAR_BACKWARD ||
			action == ACTION_ERASE_CHAR_FORWARD  ||
			action == ACTION_ERASE_WORD_BACKWARD ||
			action == ACTION_ERASE_WORD_FORWARD  ||
			action == ACTION_ERASE_LINE_BACKWARD ||
			action == ACTION_ERASE_LINE_END) &&
			win_inputposevent(ie)) {
	    unsigned	edit_unit;
            if (action >= ISO_FIRST && action <= ISO_LAST)
                        action -= ISO_FIRST;    /* get ISO character code */
	    if (textsw->func_state & TXTSW_FUNC_FILTER) {
			if (textsw->to_insert_next_free <
			    textsw->to_insert + sizeof(textsw->to_insert)) {
			    *textsw->to_insert_next_free++ = (char) action;
			}
	    } else {
		    int direction = 0 /* forward direction */;

		    switch (action) {
				case ACTION_ERASE_CHAR_BACKWARD:
				    edit_unit = EV_EDIT_CHAR;
				    direction = EV_EDIT_BACK;
				    break;
				case ACTION_ERASE_CHAR_FORWARD:
					edit_unit = EV_EDIT_CHAR;
				    break;
				case ACTION_ERASE_WORD_BACKWARD:
				    edit_unit = EV_EDIT_WORD;
				    direction = EV_EDIT_BACK;
				    break;
				case ACTION_ERASE_WORD_FORWARD:
				    edit_unit = EV_EDIT_WORD;
				    break;
				case ACTION_ERASE_LINE_BACKWARD:
				    edit_unit = EV_EDIT_LINE;
				    direction = EV_EDIT_BACK;
				    break;
				case ACTION_ERASE_LINE_END:
				    edit_unit = EV_EDIT_LINE;
				    break;
				default:
					edit_unit = 0;
			}

			if (edit_unit != 0) {
			    if (TXTSW_IS_READ_ONLY(textsw))
				goto Read_Only;
			    textsw_flush_caches(view,
				TFC_INSERT|TFC_PD_IFF_INSERT|TFC_DO_PD|TFC_SEL);
			    (void) textsw_do_edit(view, edit_unit, direction);
			} else {
Process_ASCII:		 
			switch (action) { 
			  case (short) '\r':	/* Fall through */
			  case (short) '\n':
			    if ((textsw->state & TXTSW_CONTROL_DOWN) &&
				(win_get_vuid_value(WIN_FD_FOR_VIEW(view),
						    'm') == 0) &&
				(win_get_vuid_value(WIN_FD_FOR_VIEW(view),
						    'j') == 0) &&
				(win_get_vuid_value(WIN_FD_FOR_VIEW(view),
						    'M') == 0) &&
				(win_get_vuid_value(WIN_FD_FOR_VIEW(view),
						    'J') == 0) ) {
				/* do nothing */		    
			} else {
				if (TXTSW_IS_READ_ONLY(textsw))
				    goto Read_Only;
				(void) textsw_do_newline(view);
		    }
		    break;
		  default:
		        if (TXTSW_IS_READ_ONLY(textsw))
			    goto Read_Only;
		        if (textsw->state & TXTSW_CAPS_LOCK_ON) {
			    if ((char)action >= 'a' &&
			        (char)action <= 'z')
			        ie->ie_code += 'A' - 'a';
			        /*BUG ALERT: above may need to set event_id*/
		        }
Insert:
		        *textsw->to_insert_next_free++ = (char) event_action(
		        						ie);
		        if (textsw->to_insert_next_free ==
			    textsw->to_insert + sizeof(textsw->to_insert)) {
			    textsw_flush_caches(view, TFC_STD);
		        }

		    break;		   
		}
	      }
	    }
	/*
	 * User filters
	 */
	} else if (textsw_do_filter(view, ie)) {
	/*
	 * Miscellaneous
	 */
	} else {
	    result &= ~TEXTSW_PE_USED;
	}
	/*
	 * Cleanup
	 */
	if ((textsw->state & TXTSW_EDITED) == 0)
	    textsw_possibly_edited_now_notify(textsw);
Done:
	if (TXTSW_IS_BUSY(textsw))
	    result |= TEXTSW_PE_BUSY;
	return(result);
Read_Only:
	result |= TEXTSW_PE_READ_ONLY;
	goto Done;
}

static Key_map_handle
find_key_map(textsw, ie)
	register Textsw_folio	 textsw;
	register Event		*ie;
{
	register Key_map_handle	 current_key = textsw->key_maps;

	while (current_key) {
	    if (current_key->event_code == event_action(ie)) {
		break;
	    }
	    current_key = current_key->next;
	}
	return(current_key);
}

pkg_private Key_map_handle
textsw_do_filter(view, ie)
	register Textsw_view	 view;
	register Event		*ie;
{
	register Textsw_folio	 textsw = FOLIO_FOR_VIEW(view);
	register Key_map_handle	 result = find_key_map(textsw, ie);

	if (result == 0)
	    goto Return;
	if (win_inputposevent(ie)) {
	    switch (result->type) {
	      case TXTSW_KEY_SMART_FILTER:
	      case TXTSW_KEY_FILTER:
		textsw_flush_caches(view, TFC_STD);
		textsw->func_state |= TXTSW_FUNC_FILTER;
		result = 0;
		break;
	    }
	    goto Return;
	}
	switch (result->type) {
	  case TXTSW_KEY_SMART_FILTER:
	  case  TXTSW_KEY_FILTER: {
	    extern int	textsw_call_smart_filter();
	    extern int	textsw_call_filter();
	    int		again_state, filter_result;

	    again_state = textsw->func_state & TXTSW_FUNC_AGAIN;
	    (void) textsw_record_filter(textsw, ie);
	    textsw->func_state |= TXTSW_FUNC_AGAIN;
	    textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
			           (caddr_t)TEXTSW_INFINITY-1);
	    if (result->type == TXTSW_KEY_SMART_FILTER) {
		filter_result = textsw_call_smart_filter(view, ie,
		    (char **)LINT_CAST(result->maps_to));
	    } else {
		filter_result = textsw_call_filter(view,
		    (char **)LINT_CAST(result->maps_to));
	    }
	    textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
			           (caddr_t)TEXTSW_INFINITY-1);
	    switch (filter_result) {
	      case 0:
	      default:
		break;
	      case 1: {
		char	msg[300];
		if (errno == ENOENT) {
		    (void) sprintf(msg, "Cannot locate filter '%s'.",
			    ((char **)LINT_CAST(result->maps_to))[0]);
		} else {
		    (void) sprintf(msg, "Unexpected problem with filter '%s'.",
			    ((char **)LINT_CAST(result->maps_to))[0]);
		}
		(void) alert_prompt(
			(Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER),
			(Event *) 0,
			ALERT_BUTTON_YES,		"Continue",
			ALERT_TRIGGER,		ACTION_STOP,
			ALERT_MESSAGE_STRINGS,	msg, 0,
	        	0);
		break;
	      }
	    }
	    textsw->func_state &= ~TXTSW_FUNC_FILTER;
	    textsw->to_insert_next_free = textsw->to_insert;
	    if (again_state == 0)
		textsw->func_state &= ~TXTSW_FUNC_AGAIN;
	    result = 0;
	  }
	}
Return:
	return(result);
}


static int
textsw_do_newline(view)
	register Textsw_view	view;
{
	Es_index		ev_get_insert();
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	int			delta;
	Es_index		first = ev_get_insert(folio->views),
				last_plus_one,
				previous;

	textsw_flush_caches(view, TFC_INSERT|TFC_PD_SEL);
	if (folio->state & TXTSW_AUTO_INDENT)
	    first = ev_get_insert(folio->views);
	delta = textsw_do_input(view, "\n", 1, TXTSW_UPDATE_SCROLLBAR);
	if (folio->state & TXTSW_AUTO_INDENT) {
	    previous = first;
	    textsw_find_pattern(folio, &previous, &last_plus_one,
				"\n", 1, EV_FIND_BACKWARD);
	    if (previous != ES_CANNOT_SET) {
		char			 buf[100];
		struct es_buf_object	 esbuf;
		register char		*c;

		esbuf.esh = folio->views->esh;
		esbuf.buf = buf;
		esbuf.sizeof_buf = sizeof(buf);
		if (es_make_buf_include_index(&esbuf, previous, 0) == 0) {
		    if (AN_ERROR(buf[0] != '\n')) {
		    } else {
			for (c = buf+1; c < buf+sizeof(buf); c++) {
			   switch (*c) {
			      case '\t':
			      case ' ':
				break;
			      default:
				goto Did_Scan;
			   }
			}
Did_Scan:
			if (c != buf+1) {
			    delta += textsw_do_input(view, buf+1,
						     (int) (c-buf-1),
						     TXTSW_UPDATE_SCROLLBAR_IF_NEEDED);
			}
		    }
		}
	    }
	}
	return(delta);
}

pkg_private Es_index
textsw_get_saved_insert(textsw)
	register Textsw_folio	textsw;
{
	Ev_finger_handle	saved_insert_finger;

	saved_insert_finger = ev_find_finger(
			&textsw->views->fingers, textsw->save_insert);
	return(saved_insert_finger ? saved_insert_finger->pos : ES_INFINITY);
}

pkg_private int
textsw_clear_pending_func_state(textsw)
	register Textsw_folio	textsw;
{
	if (!EV_MARK_IS_NULL(&textsw->save_insert)) {
	    if (textsw->func_state & TXTSW_FUNC_PUT) {
		Es_index	old_insert = textsw_get_saved_insert(textsw);
		if AN_ERROR(old_insert == ES_INFINITY) {
		} else {
		    textsw_set_insert(textsw, old_insert);
		}
	    } else
		ASSUME(textsw->func_state & TXTSW_FUNC_GET);
	    ev_remove_finger(&textsw->views->fingers, textsw->save_insert);
	    (void) ev_init_mark(&textsw->save_insert);
	}
	if (textsw->func_state & TXTSW_FUNC_FILTER) {
	    textsw->to_insert_next_free = textsw->to_insert;
	}
	textsw->func_state &= ~(TXTSW_FUNC_ALL | TXTSW_FUNC_EXECUTE);
}

/*	==========================================================
 *
 *		Input mask initialization and setting.
 *
 *	==========================================================
 */

static struct inputmask	basemask_kbd, mousebuttonmask_kbd;
static struct inputmask	basemask_pick, mousebuttonmask_pick;
static int		masks_have_been_initialized;	/* Defaults to FALSE */

static setupmasks()
{
	register struct inputmask	*mask;
	register int			i;

	/*
	 * Set up the standard kbd mask.
	 */
	mask = &basemask_kbd;
	input_imnull(mask);
        mask->im_flags |= IM_EUC | IM_NEGEVENT | IM_META | IM_NEGMETA | IM_ISO;
	for (i=1; i<17; i++) {
	    win_setinputcodebit(mask, KEY_LEFT(i));
	    win_setinputcodebit(mask, KEY_TOP(i));
	    win_setinputcodebit(mask, KEY_RIGHT(i));
	}
	/* Unset TOP and OPEN because will look for them in pick mask */
	/*
	 * Use win_keymap_*inputcodebig() for events that are semantic,
	 * i.e., have no actual inputmask bit assignment.  This is done
	 * in textsw_set_base_mask() below since we need the fd to know
	 * what keymap to look in.
	 */
	win_setinputcodebit(mask, KBD_USE);
	win_setinputcodebit(mask, KBD_DONE);
#ifdef VT_100_HACK
	win_setinputcodebit(mask, SHIFT_LEFT);		/* Pick up the shift */
	win_setinputcodebit(mask, SHIFT_RIGHT);		/*   keys for VT-100 */
	win_setinputcodebit(mask, SHIFT_LOCK);		/*   compatibility */
#endif
	/*
	 * Set up the standard pick mask.
	 */
	mask = &basemask_pick;
	input_imnull(mask);
	win_setinputcodebit(mask, WIN_STOP);
	win_setinputcodebit(mask, LOC_WINENTER);
	win_setinputcodebit(mask, LOC_WINEXIT);
	win_setinputcodebit(mask, TXTSW_POINT);
	win_setinputcodebit(mask, TXTSW_ADJUST);
	win_setinputcodebit(mask, TXTSW_MENU);
	win_setinputcodebit(mask, KBD_REQUEST);
	win_setinputcodebit(mask, LOC_MOVE);	/* NOTE: New */
	win_setinputcodebit(mask, LOC_STILL);	/* NOTE: Detect shift up??? */
	mask->im_flags |= IM_NEGEVENT;
	/* Make "mouse" mask use base mask */
	mousebuttonmask_kbd = basemask_kbd;
	mousebuttonmask_pick = basemask_pick;
	masks_have_been_initialized = TRUE;
}

pkg_private int
textsw_set_base_mask(fd)
	int	fd;
{
	if (masks_have_been_initialized == FALSE) {
	    setupmasks();
	}

	win_keymap_set_smask_class(fd, KEYMAP_FUNCT_KEYS);
	win_keymap_set_smask_class(fd, KEYMAP_EDIT_KEYS);
	win_keymap_set_smask_class(fd, KEYMAP_MOTION_KEYS);
	win_keymap_set_smask_class(fd, KEYMAP_TEXT_KEYS);
        win_keymap_set_smask(fd, TXTSW_HELP);

	win_keymap_unset_imask_from_std_bind(&basemask_kbd, TXTSW_TOP);
	win_keymap_unset_imask_from_std_bind(&basemask_kbd, TXTSW_OPEN);
	win_keymap_unset_imask_from_std_bind(&basemask_kbd, TXTSW_PROPS);
	win_keymap_unset_imask_from_std_bind(&basemask_kbd, TXTSW_HELP);
	win_set_kbd_mask(fd, &basemask_kbd);

	win_keymap_set_imask_from_std_bind(&basemask_pick, TXTSW_TOP);
	win_keymap_set_imask_from_std_bind(&basemask_pick, TXTSW_OPEN);
	win_keymap_set_imask_from_std_bind(&basemask_pick, TXTSW_PROPS);
	win_keymap_set_imask_from_std_bind(&basemask_pick, TXTSW_HELP);
	win_set_pick_mask(fd, &basemask_pick);
}

pkg_private int
textsw_set_mouse_button_mask(fd)
	int	fd;
{
	if (masks_have_been_initialized == FALSE) {
	    setupmasks();
	}
	win_set_kbd_mask(fd, &mousebuttonmask_kbd);

	win_set_pick_mask(fd, &mousebuttonmask_pick);
}

/*	==========================================================
 *
 *		Functions invoked by function keys.
 *
 *	==========================================================
 */

pkg_private void
textsw_begin_function(view, function)
	Textsw_view		view;
	unsigned		function;
{
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);

	textsw_flush_caches(view, TFC_STD);
	if ((folio->state & TXTSW_CONTROL_DOWN) &&
	    !TXTSW_IS_READ_ONLY(folio))
	    folio->state |= TXTSW_PENDING_DELETE;
	folio->track_state |= TXTSW_TRACK_SECONDARY;
	folio->func_state |= function|TXTSW_FUNC_EXECUTE;
	textsw_init_timer(folio);
	if AN_ERROR(folio->func_state & TXTSW_FUNC_SVC_SAW(function))
	    /* Following covers up inconsistent state with Seln. Svc. */
	    folio->func_state &= ~TXTSW_FUNC_SVC_SAW(function);
}

pkg_private void
textsw_init_timer(folio)
	Textsw_folio	folio;
{
	/*
	 * Make last_point/_adjust/_ie_time close (but not too close) to
	 *   current time to avoid overflow in tests for multi-click.
	 */
	folio->last_point.tv_sec -= 1000;
	folio->last_adjust = folio->last_point;
	folio->last_ie_time = folio->last_point;
};


pkg_private void
textsw_end_function(view, function)
	Textsw_view		view;
	unsigned		function;
{
	pkg_private void	textsw_end_selection_function();
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);

	folio->state &= ~TXTSW_PENDING_DELETE;
	folio->track_state &= ~TXTSW_TRACK_SECONDARY;
	folio->func_state &=
	    ~(function | TXTSW_FUNC_SVC_SAW(function) | TXTSW_FUNC_EXECUTE);
	textsw_end_selection_function(folio);
}

static
textsw_begin_again(view)
	Textsw_view	 view;
{
	textsw_begin_function(view, TXTSW_FUNC_AGAIN);
}

static
textsw_end_again(view, x, y)
	Textsw_view	 view;
	int			 x, y;
{
	textsw_do_again(view, x, y);
	textsw_end_function(view, TXTSW_FUNC_AGAIN);
	/* 
	 * Make sure each sequence of again will not accumulate.
	 * Like find, delete and insert.
	 */
	textsw_checkpoint_again(VIEW_REP_TO_ABS(view));
}

pkg_private int
textsw_again(view, x, y)
	Textsw_view	 view;
	int		 x, y;
{
	textsw_begin_again(view);
	textsw_end_again(view, x, y);
}

static
textsw_begin_delete(view)
	Textsw_view	view;
{
	register Textsw_folio	textsw = FOLIO_FOR_VIEW(view);

	textsw_begin_function(view, TXTSW_FUNC_DELETE);
	if (!TXTSW_IS_READ_ONLY(textsw))
	    textsw->state |= TXTSW_PENDING_DELETE;
	    /* Force pending-delete feedback as it is implicit in DELETE */
	(void) textsw_inform_seln_svc(textsw, TXTSW_FUNC_DELETE, TRUE);
}

pkg_private int
textsw_end_delete(view)
	Textsw_view	view;
{
	extern void		textsw_init_selection_object();
	extern void		textsw_clear_secondary_selection();
	Textsw_selection_object	selection;
	int			result = 0;
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);

	(void) textsw_inform_seln_svc(folio, TXTSW_FUNC_DELETE, FALSE);
	if ((folio->func_state & TXTSW_FUNC_DELETE) == 0)
	    return(0);
	if ((folio->func_state & TXTSW_FUNC_EXECUTE) == 0)
	    goto Done;
	textsw_init_selection_object(folio, &selection, "", 0, FALSE);
	if (TFS_IS_ERROR(textsw_func_selection(folio, &selection, 0)))
	    goto Done;
	if (selection.type & TFS_IS_SELF) {
	    switch(textsw_adjust_delete_span(folio, &selection.first,
					     &selection.last_plus_one)) {
	      case TEXTSW_PE_READ_ONLY:
		textsw_clear_secondary_selection(folio, EV_SEL_SECONDARY);
		result = TEXTSW_PE_READ_ONLY;
		break;
	      case TXTSW_PE_EMPTY_INTERVAL:
		break;
	      case TXTSW_PE_ADJUSTED:
		textsw_set_selection(VIEW_REP_TO_ABS(folio->first_view),
				     ES_INFINITY, ES_INFINITY, selection.type);
		/* Fall through to delete remaining span */
	      default:
		textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
					(caddr_t)TEXTSW_INFINITY-1);
		(void) textsw_delete_span(
			view, selection.first, selection.last_plus_one,
			(unsigned)((selection.type & EV_SEL_PRIMARY)
			    ? TXTSW_DS_RECORD|TXTSW_DS_SHELVE
			    : TXTSW_DS_SHELVE));
		textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
					(caddr_t)TEXTSW_INFINITY-1);
		break;
	    }
	}
Done:
	textsw_end_function(view, TXTSW_FUNC_DELETE);
	textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
	return(result);
}

pkg_private int
textsw_function_delete(view)
	Textsw_view	view;
{
	int		result;

	textsw_begin_delete(view);
	result = textsw_end_delete(view);
	return(result);
}

static
textsw_begin_undo(view)
	Textsw_view	view;
{
	textsw_begin_function(view, TXTSW_FUNC_UNDO);
	textsw_flush_caches(view, TFC_SEL);
}

static
textsw_end_undo(view)
	Textsw_view	view;
{
	textsw_do_undo(view);
	textsw_end_function(view, TXTSW_FUNC_UNDO);
	textsw_update_scrollbars(FOLIO_FOR_VIEW(view), TEXTSW_VIEW_NULL);
}

static
textsw_undo_notify(folio, start, delta)
	register Textsw_folio	folio;
	register Es_index	start, delta;
{
	extern void		textsw_notify_replaced();
	extern Es_index		ev_get_insert(),
				ev_set_insert();
	register Ev_chain	chain = folio->views;
	register Es_index	old_length =
				    es_get_length(chain->esh) - delta;
	Es_index		old_insert;

	if (folio->notify_level & TEXTSW_NOTIFY_EDIT)
	    old_insert = ev_get_insert(chain);
	(void) ev_set_insert(chain, (delta > 0) ? start+delta : start);
	ev_update_after_edit(chain,
			     (delta > 0) ? start : start-delta,
			     delta, old_length, start);
	if (folio->notify_level & TEXTSW_NOTIFY_EDIT) {
	    textsw_notify_replaced((Textsw_opaque)folio->first_view, 
			  old_insert, old_length,
			  (delta > 0) ? start : start+delta,
			  (delta > 0) ? start+delta : start,
			  (delta > 0) ? delta : 0);
	}
	textsw_checkpoint(folio);
}

static
textsw_do_undo(view)
	Textsw_view			view;
{
	extern Ev_finger_handle		ev_add_finger();
	extern int			ev_remove_finger();
	extern Es_index			ev_get_insert(),
					textsw_set_insert();
	register Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	register Ev_finger_handle	saved_insert_finger;
	register Ev_chain		views = folio->views;
	Ev_mark_object			save_insert;

	if (!TXTSW_DO_UNDO(folio))
	    return;
	if (folio->undo[0] == es_get(views->esh, ES_UNDO_MARK)) {
	    /* Undo followed immediately by another undo.
	     * Note: textsw_set_internal guarantees that folio->undo_count != 1
	     */
	    bcopy((caddr_t)&folio->undo[1], (caddr_t)&folio->undo[0],
		  (int)((folio->undo_count-2)*sizeof(folio->undo[0])));
	    folio->undo[folio->undo_count-1] = ES_NULL_UNDO_MARK;
	}
	if (folio->undo[0] == ES_NULL_UNDO_MARK)
	    return;
	/* Undo the changes to the piece source. */
	(void) ev_add_finger(&views->fingers, ev_get_insert(views), 0,
			     &save_insert);

	ev_set(0, views, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
	es_set(views->esh,
	       ES_UNDO_NOTIFY_PAIR, textsw_undo_notify, (caddr_t)folio,
	       ES_UNDO_MARK, folio->undo[0],
	       0);
	ev_set(0, views, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
	ev_update_chain_display(views);
	saved_insert_finger =
	    ev_find_finger(&views->fingers, save_insert);
	if AN_ERROR(saved_insert_finger == 0) {
	} else {
	    (void) textsw_set_insert(folio, saved_insert_finger->pos);
	    ev_remove_finger(&views->fingers, save_insert);
	}
	/* Get the new mark. */
	folio->undo[0] = es_get(views->esh, ES_UNDO_MARK);
	/* Check to see if this has undone all edits to the folio. */
	if (textsw_has_been_modified(VIEW_REP_TO_ABS(folio->first_view))
	    == 0) {
	    char	*name;
	    if (textsw_file_name(folio, &name) == 0) {
		textsw_notify(view, TEXTSW_ACTION_LOADED_FILE, name, 0);
	    }
	    folio->state &= ~TXTSW_EDITED;
	}
}

extern int
textsw_undo(textsw)
	Textsw_folio	 textsw;
{
	textsw_begin_undo(textsw->first_view);
	textsw_end_undo(textsw->first_view);
}

static int
textsw_win_event(view, ie, caret_was_up)
	register Textsw_view		 view;
	register Event			*ie;
	int				caret_was_up;
{
	Textsw_folio			folio = FOLIO_FOR_VIEW(view);
	int				is_win_event = FALSE;
	extern void			textsw_resize();
	extern void			textsw_repaint();
	int				action = event_action(ie);

	if (action == WIN_RESIZE) {
	    is_win_event = TRUE;
	    textsw_resize(view);
	} else if (action == WIN_REPAINT) {
	    is_win_event = TRUE;
	    ev_set(view->e_view, EV_NO_REPAINT_TIL_EVENT, FALSE, 0);
	    textsw_repaint(view);
	    /* if caret was up and we took it down, put it back */
	    if (caret_was_up
	    && (folio->caret_state & TXTSW_CARET_ON) == 0) {
		textsw_remove_timer(folio);
		textsw_timer_expired(folio, 0);
	    }
	}
	return(is_win_event);
}

static int
textsw_scroll_event(view, ie, arg)
	register Textsw_view		 view;
	register Event			*ie;
	Notify_arg	 		 arg;
{
	Textsw_folio			folio = FOLIO_FOR_VIEW(view);
	int				is_scroll_event = FALSE;
	extern void			textsw_scroll();
	extern void			textsw_update_scrollbars();
	int				action = event_action(ie);

	if (action == SCROLL_REQUEST) {
	    is_scroll_event = TRUE;
	    textsw_scroll((Scrollbar)arg);
	} else if (action == SCROLL_ENTER) {
	    is_scroll_event = TRUE;
	    textsw_update_scrollbars(folio, view);
	}

	return(is_scroll_event);
}

static int
textsw_function_key_event(view, ie, result)
	register Textsw_view		 view;
	register Event			*ie;
	int				*result;
{
	Textsw_folio			folio = FOLIO_FOR_VIEW(view);
	int				is_function_key_event = FALSE;
	int				action = event_action(ie);

	if (action == TXTSW_AGAIN) {
	    is_function_key_event = TRUE;	
	    if (win_inputposevent(ie)) {
		textsw_begin_again(view);
	    } else if (folio->func_state & TXTSW_FUNC_AGAIN) {
		textsw_end_again(view, ie->ie_locx, ie->ie_locy);
	    }
#ifdef VT_100_HACK
	} else if (action == TXTSW_AGAIN) {
	    is_function_key_event = TRUE;
	    /* Bug in releases through K3 only generates down, no up */
	    if (win_inputposevent(ie)) {
		textsw_begin_again(view);
		textsw_end_again(view, ie->ie_locx, ie->ie_locy);
	    } else if (folio->func_state & TXTSW_FUNC_AGAIN) {

	    }
#endif
	} else if (action == TXTSW_UNDO) {
	    is_function_key_event = TRUE;
	    if (TXTSW_IS_READ_ONLY(folio)) {
	    	*result |= TEXTSW_PE_READ_ONLY;
	    }
	    if (win_inputposevent(ie)) {
		textsw_begin_undo(view);
	    } else if (folio->func_state & TXTSW_FUNC_UNDO) {
		textsw_end_undo(view);
	    }
#ifdef VT_100_HACK
	} else if (action == TXTSW_UNDO) {
	    /* Bug in releases through K3 only generates down, no up */
	    is_function_key_event = TRUE;
	    if (win_inputposevent(ie)) {
		textsw_begin_undo(view);
		textsw_end_undo(view);
	    } else if (folio->func_state & TXTSW_FUNC_UNDO) {

	    }
#endif
	} else if ((action == TXTSW_TOP) ||
		   (action == TXTSW_BOTTOM) ||
		   (action == TXTSW_OPEN) ||
		   (action == TXTSW_PROPS)) {
	    is_function_key_event = TRUE;   
	    /* These key should only work on up event */
	    if (!win_inputposevent(ie))
	        textsw_notify(view, TEXTSW_ACTION_TOOL_MGR, ie, 0);
	} else if ((action == TXTSW_FIND_FORWARD) ||
	          (action == TXTSW_FIND_BACKWARD) ||
	          (action == TXTSW_REPLACE)) {
	    is_function_key_event = TRUE;

	    if (win_inputposevent(ie)) {	
		textsw_begin_find(view);
		folio->func_x = ie->ie_locx;
		folio->func_y = ie->ie_locy;
		folio->func_view = view;
	    } else 
		textsw_end_find(view, action, ie->ie_locx, ie->ie_locy);
	}   
	return(is_function_key_event);
}

static int
textsw_mouse_event(view, ie)
	register Textsw_view		 view;
	register Event			*ie;
{
	Textsw_folio			folio = FOLIO_FOR_VIEW(view);
	int				is_mouse_event = FALSE;
	int				action = event_action(ie);

	if (action == TXTSW_POINT) {
	    is_mouse_event = TRUE;
	    if (win_inputposevent(ie)) {
#ifdef VT_100_HACK
		if ((folio->state & TXTSW_SHIFT_DOWN) &&
		    (textsw->func_state & TXTSW_FUNC_ALL) == 0) {
		    /* Hack for VT-100 keyboard until PIT is available. */
		    textsw_begin_get(view);
		    folio->func_x = ie->ie_locx;
		    folio->func_y = ie->ie_locy;
		    folio->func_view = view;
		} else
#endif
		start_selection_tracking(view, ie);
	    }
	    /* Discard negative events that get to here, as state is wrong. */
	} else if (action == TXTSW_ADJUST) {
	    is_mouse_event = TRUE;
	    if (win_inputposevent(ie)) {
		start_selection_tracking(view, ie);
	    }
	    /* Discard negative events that get to here, as state is wrong. */
	} else if (action == LOC_WINENTER) {
	 /* extern int	textsw_sync_with_seln_svc();

	    is_mouse_event = TRUE;
	    textsw_sync_with_seln_svc(folio); */
	    
	    /* This is done for performance improvment */
	    is_mouse_event = TRUE;
	    folio->state |= TXTSW_DELAY_SEL_INQUIRE;
	} else if (action == LOC_WINEXIT || action == KBD_DONE) {

	    is_mouse_event = TRUE;
	    textsw_may_win_exit(folio);
	/*
	 * Menus
	 */
	} else if (action == TXTSW_MENU) {
	    is_mouse_event = TRUE;

	    if (win_inputposevent(ie)) {
		extern textsw_do_menu();
		textsw_flush_caches(view, TFC_STD);
		textsw_do_menu(view, ie);
	    }
	}

	return(is_mouse_event);
}

static int
textsw_edit_function_key_event(view, ie, result)
	register Textsw_view		 view;
	register Event			*ie;
	int				*result;
{
	Textsw_folio			folio = FOLIO_FOR_VIEW(view);
	int				is_edit_function_key_event = FALSE;
	int				action = event_action(ie);

	if (action == TXTSW_DELETE) {
	    is_edit_function_key_event = TRUE;
	    if (win_inputposevent(ie)) {
		textsw_begin_delete(view);
		folio->func_x = ie->ie_locx;
		folio->func_y = ie->ie_locy;
		folio->func_view = view;
	    } else {
		*result |= textsw_end_delete(view);
	    }
	} else if (action == TXTSW_GET) {
	    is_edit_function_key_event = TRUE;
	    if (win_inputposevent(ie)) {
		textsw_begin_get(view);
		folio->func_x = ie->ie_locx;
		folio->func_y = ie->ie_locy;
		folio->func_view = view;
	    } else {
		*result |= textsw_end_get(view);
	    }
	} else if (action == TXTSW_PUT) {
	    is_edit_function_key_event = TRUE;
	    if (win_inputposevent(ie)) {
		textsw_begin_put(view, TRUE);
		folio->func_x = ie->ie_locx;
		folio->func_y = ie->ie_locy;
		folio->func_view = view;
	    } else {
		*result |= textsw_end_put(view);
	    }
	} 

	return(is_edit_function_key_event);
}	 

static int
textsw_caret_motion_event (view, ie, action)
	register Textsw_view	 view;
	register Event   	*ie;
	register int		*action;
{
	short	ch;
	int     		 	 is_caret_motion_event =  TRUE;
	pkg_private void		 textsw_move_down_a_line();
	pkg_private void		 textsw_move_left_a_character();
	pkg_private void		 textsw_move_right_a_character();
	pkg_private void		 textsw_move_up_a_line();
	pkg_private void		 textsw_move_forward_a_word();
	pkg_private void		 textsw_move_backward_a_word();
	pkg_private void		 textsw_move_to_line_end();
	pkg_private void		 textsw_move_to_line_start();	
	pkg_private void	 	 textsw_move_caret();

	switch (*action) {
	    case TXTSW_MOVE_RIGHT:
	        if (win_inputposevent(ie))
	            textsw_move_caret(view, TXTSW_CHAR_FORWARD);
	        break;
	    case TXTSW_MOVE_LEFT:
	        if (win_inputposevent(ie))
	            textsw_move_caret(view, TXTSW_CHAR_BACKWARD);
	        break;
	    case TXTSW_MOVE_UP:
	        if (win_inputposevent(ie))
	            textsw_move_caret(view, TXTSW_PREVIOUS_LINE);
	        break;            
	    case TXTSW_MOVE_DOWN:
	        if (win_inputposevent(ie))
	            textsw_move_caret(view, TXTSW_NEXT_LINE);
	        break;	            
	    case TXTSW_MOVE_WORD_BACKWARD:
	        if (win_inputposevent(ie))
	            textsw_move_caret(view, TXTSW_WORD_BACKWARD);
	        break;
	    case TXTSW_MOVE_WORD_FORWARD:
	        if (win_inputposevent(ie))
	            textsw_move_caret(view, TXTSW_WORD_FORWARD);
	        break;
	    case TXTSW_MOVE_WORD_END:
	        if (win_inputposevent(ie))
	            textsw_move_caret(view, TXTSW_WORD_END);
	        break;    	            
	    case TXTSW_MOVE_TO_LINE_START:
	        if (win_inputposevent(ie))
	            textsw_move_caret(view, TXTSW_LINE_START);
	        break;
	    case TXTSW_MOVE_TO_LINE_END:
	        if (win_inputposevent(ie))
	            textsw_move_caret(view, TXTSW_LINE_END);
	        break;	
	    case TXTSW_MOVE_TO_NEXT_LINE_START:
	        if (win_inputposevent(ie))
	            textsw_move_caret(view, TXTSW_NEXT_LINE_START);
	        break;    
	    case TXTSW_MOVE_TO_DOC_START:
	        if (win_inputposevent(ie)) {
	            /*  This is a gross hack to make sure  control m is CR */
	            if (win_get_vuid_value(WIN_FD_FOR_VIEW(view), 'm') ||
	                  win_get_vuid_value(WIN_FD_FOR_VIEW(view), 'M')) {
	                is_caret_motion_event = FALSE;
	                event_set_id(ie, 13);  /* ASCII value of CR */
	                *action = 13;
	            } else
	                textsw_move_caret(view, TXTSW_DOCUMENT_START);

	        }
	        break;
	    case TXTSW_MOVE_TO_DOC_END:
	        if (win_inputposevent(ie)) {
	            /*  This is a gross hack to make sure  control m is CR */
	            if (win_get_vuid_value(WIN_FD_FOR_VIEW(view), 'm') ||
	                  win_get_vuid_value(WIN_FD_FOR_VIEW(view), 'M')) {
	                is_caret_motion_event = FALSE;
	                event_set_id(ie, 13);  /* ASCII value of CR */
	                *action = 13;
	            } else
	                textsw_move_caret(view, TXTSW_DOCUMENT_END);
	        }
	        break;	                    
	    default:
	        is_caret_motion_event = FALSE;
	        break;

	 }

	return(is_caret_motion_event);
}

static int
textsw_field_event(view, ie, action)
	register Textsw_view		 view;
	register Event			*ie;
	register int			*action;
{
	int				is_field_event = FALSE;

	if (*action == TXTSW_NEXT_FIELD) {
	/*  This is a gross hack to make sure  control i is TAB */
	    if (win_get_vuid_value(WIN_FD_FOR_VIEW(view), 'i') ||
	          win_get_vuid_value(WIN_FD_FOR_VIEW(view), 'I')) {
	        event_set_id(ie, 9);  /* ASCII value of TAB */
	        *action = 9;
	    } else {	    
	        is_field_event = TRUE;
	        
	        if (win_inputposevent(ie)) {
	            textsw_flush_caches(view, TFC_STD);
	            (void) textsw_match_selection_and_normalize(view, "|>", 
	                TEXTSW_FIELD_FORWARD);
	        }
	    }	
	} else if (*action == TXTSW_PREV_FIELD) {
	/*  This is a gross hack to make sure  control i is TAB */
	    if (win_get_vuid_value(WIN_FD_FOR_VIEW(view), 'i') ||
	          win_get_vuid_value(WIN_FD_FOR_VIEW(view), 'I')) {
	        event_set_id(ie, 9);  /* ASCII value of TAB */
	        *action = 9;
	    } else {
	        is_field_event = TRUE;
	        
	        if (win_inputposevent(ie)) {
	            textsw_flush_caches(view, TFC_STD);
	            (void) textsw_match_selection_and_normalize(view, "<|", 
	                TEXTSW_FIELD_BACKWARD);
	        }
	    }
	} else  if (*action == TXTSW_MATCH_DELIMITER) {
	    char		*start_marker;

	    is_field_event = TRUE;
	    if (win_inputposevent(ie)) {
	        textsw_flush_caches(view, TFC_STD);
	        (void) textsw_match_selection_and_normalize(view, start_marker, 
	            TEXTSW_NOT_A_FIELD);
	    }

	}
	return(is_field_event);
}


static int
textsw_file_operation(view, ie)
	register Textsw_view		 view;
	register Event			*ie;
{
	int				is_file_op_event = FALSE;
	register Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	int				action = event_action(ie);

	if (action == TXTSW_LOAD_FILE_AS_MENU) {
	    int		no_cd;
	    int		result, dont_load_file = 0;

	    if (win_inputposevent(ie)) {
	        if (textsw_has_been_modified(VIEW_REP_TO_ABS(view))) {
	                result = alert_prompt(
		            (Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER),
		            ie,
		            ALERT_MESSAGE_STRINGS,
		            "The text has been edited.",
		            "Load File will discard these edits. Please confirm.",
		                0,
		            ALERT_BUTTON_YES,	"Confirm, discard edits",
		            ALERT_BUTTON_NO,    "Cancel",
		            0);
	                if (result != ALERT_FAILED) {
			    dont_load_file = (result == 1);
		        }
	        }
	        no_cd = ((folio->state & TXTSW_NO_CD) ||
		     ((folio->state & TXTSW_LOAD_CAN_CD) == 0));
	        if (!dont_load_file) {
	            if (textsw_load_selection(
		        folio, ie->ie_locx, ie->ie_locy, no_cd) == 0) {
	            }
	        }
	        is_file_op_event = TRUE;
	    }

	} else if (action == TXTSW_STORE_FILE) {
	    if (win_inputposevent(ie))
	        (void) textsw_store_to_selection(folio, ie->ie_locx, ie->ie_locy);
	    is_file_op_event = TRUE;
	}
	return(is_file_op_event);
}
