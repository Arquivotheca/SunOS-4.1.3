#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_attr.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Attribute set/get routines for text subwindows.
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <suntool/ev_impl.h>
#include <varargs.h>
#include <sys/dir.h>
#include <pixrect/pixfont.h>
#include <suntool/window.h>
#include <suntool/walkmenu.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <sunwindow/defaults.h>

extern void			ev_line_info();
extern Es_handle		es_file_create(), es_mem_create();
pkg_private Es_handle		textsw_add_scrollbar_to_view();
pkg_private Es_handle		textsw_create_ps();
pkg_private void		textsw_display_view_margins();
pkg_private void		textsw_init_again(), textsw_init_undo();
pkg_private Es_status		textsw_load_file_internal();
extern Textsw_index		textsw_position_for_physical_line();
pkg_private Textsw_index	textsw_replace();
pkg_private Es_index		textsw_get_contents();

#ifndef CTRL
#define CTRL(c)		('c' & 037)
#endif
#define	DEL		0x7f

#define SET_BOOL_FLAG(flags, to_test, flag)			\
	if ((unsigned)(to_test) != 0) (flags) |= (flag);	\
	else (flags) &= ~(flag)

#define BOOL_FLAG_VALUE(flags, flag)				\
	((flags & flag) ? TRUE : FALSE)

static int	TEXTSW_count;		/* = 0 for -A-R */



static Textsw_status
set_first(view, error_msg, filename, reset_mode, first, first_line, all_views)
	register Textsw_view	 view;
	char			*error_msg;
	char			*filename;
	int			 reset_mode;
	Es_index		 first;
	int			 first_line;
	int			 all_views;
{
	char			 msg_buf[MAXNAMLEN+100];
	char			*msg;
	Es_status		 load_status;
	Textsw_status		 result = TEXTSW_STATUS_OKAY;
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	extern void		 textsw_normalize_view();

	msg = (error_msg) ? error_msg : msg_buf;
	if (filename) {
	    char		 scratch_name[MAXNAMLEN];
	    Es_handle		 new_esh;
	    /* Ensure no caret turds will leave behind */
	    textsw_take_down_caret(folio);
	    load_status =
		textsw_load_file_internal(
		    folio, filename, scratch_name, &new_esh, ES_CANNOT_SET,
		    1);
	    if (load_status == ES_SUCCESS) {
		if (first_line > -1) {
		    first = textsw_position_for_physical_line(
				VIEW_REP_TO_ABS(view), first_line+1);
		}
		if (reset_mode != TEXTSW_LRSS_CURRENT) {
		    Textsw_view   view_ptr;

		    (void) ev_set(view->e_view,
				  EV_FOR_ALL_VIEWS,
				  EV_DISPLAY_LEVEL, EV_DISPLAY_NONE,
				  EV_DISPLAY_START, first,
				  EV_DISPLAY_LEVEL, EV_DISPLAY,
				  0);
		    for (view_ptr = folio->first_view; view_ptr;
			view_ptr = view_ptr->next) {
			textsw_set_scroll_mark(VIEW_REP_TO_ABS(view_ptr));
		    }		  
		}
		textsw_notify(view,
			      TEXTSW_ACTION_LOADED_FILE, filename,
			      0);
	    } else {
		textsw_format_load_error(msg, load_status,
					 filename, scratch_name);
		if (error_msg == NULL) {
                    Event   alert_event;
 
                    (void) alert_prompt(
                       	    (Frame)window_get(WINDOW_FROM_VIEW(folio), WIN_OWNER),
                            &alert_event,
                            ALERT_MESSAGE_STRINGS, msg,
                            ALERT_BUTTON_YES, "Continue",
                            0);
                }
		result = TEXTSW_STATUS_OTHER_ERROR;
	    }
	} else {
	    if (first_line > -1) {
		first = textsw_position_for_physical_line(
				VIEW_REP_TO_ABS(view), first_line+1);
	    }
	    if (first != ES_CANNOT_SET) {
	       if (all_views) {
		 Textsw_view   view_ptr;
		 for (view_ptr = folio->first_view; view_ptr; view_ptr = view_ptr->next) {
		 	textsw_normalize_internal(view_ptr, first, first, 0, 0,
				  TXTSW_NI_DEFAULT);     
		 };

	      } else
		 textsw_normalize_view(VIEW_REP_TO_ABS(view), first);     
	    } else {
		result = TEXTSW_STATUS_OTHER_ERROR;
	    }
	}
	return(result);
}

pkg_private Textsw_status
textsw_set_internal(view, attrs)
	Textsw_view			 view;
	register Attr_avlist		 attrs;
{
	Textsw_status			 status, status_dummy;
	Textsw_status			*status_ptr = &status_dummy;
	Textsw_view			 next;
	char				 expand_attr[100];
	char				 error_dummy[256];
	char				*error_msg = error_dummy;
	char				*file = NULL;
	int				 reset_mode = -1;
	register int			 temp;
	int				 all_views = 0;
	int				 display_views = 0;
	int				 update_scrollbar = 0;
	int				 ceased_read_only = 0;
	int				 set_read_only_esh = 0;
	int				 str_length = 0;
	Es_handle			 ps_esh, scratch_esh, mem_esh;
	Es_status			 es_status;
	Es_index			 old_insert_pos, old_length;
	register int			 consume_attrs = 0;
	register Textsw_folio		 textsw = FOLIO_FOR_VIEW(view);

	status = TEXTSW_STATUS_OKAY;
	for (; *attrs; attrs = attr_next(attrs)) {
	    if (consume_attrs & 1) consume_attrs |= 0x10000; 
	    switch ((Textsw_attribute)(*attrs)) {
	      case TEXTSW_FOR_ALL_VIEWS:
		all_views = TRUE;
		break;
	      case TEXTSW_END_ALL_VIEWS:
		all_views = FALSE;
		break;

	      case TEXTSW_ADJUST_IS_PENDING_DELETE:
		SET_BOOL_FLAG(textsw->state, attrs[1], TXTSW_ADJUST_IS_PD);
		break;
	      case TEXTSW_AGAIN_LIMIT:
		/* Internally we want one more again slot that client does.  */
		temp = (int)(attrs[1]);
		temp = (temp > 0) ? temp+1 : 0;
		textsw_init_again(textsw, temp);
		break;
	      case TEXTSW_AGAIN_RECORDING:
		SET_BOOL_FLAG(textsw->state, !attrs[1],
			      TXTSW_NO_AGAIN_RECORDING);
		break;
	      case TEXTSW_AUTO_INDENT:
		SET_BOOL_FLAG(textsw->state, attrs[1], TXTSW_AUTO_INDENT);
		break;
	      case TEXTSW_AUTO_SCROLL_BY:
		(void) ev_set(view->e_view,
			      EV_CHAIN_AUTO_SCROLL_BY, (int)(attrs[1]),
			      0);
		break;
	      case TEXTSW_BLINK_CARET:
		SET_BOOL_FLAG(textsw->caret_state, attrs[1],
			      TXTSW_CARET_FLASHING);
		break;
	      case TEXTSW_BROWSING:
		temp = TXTSW_IS_READ_ONLY(textsw);
		SET_BOOL_FLAG(textsw->state, attrs[1], TXTSW_READ_ONLY_SW);
		ceased_read_only = (temp && !TXTSW_IS_READ_ONLY(textsw));
		break;
	      case TEXTSW_CLIENT_DATA:
		textsw->client_data = attrs[1];
		break;
	      case HELP_DATA:
		textsw->help_data = attrs[1];
		break;
	      case TEXTSW_COALESCE_WITH:
		/* Get only */
		break;
	      case TEXTSW_CONFIRM_OVERWRITE:
		SET_BOOL_FLAG(textsw->state, attrs[1],
			      TXTSW_CONFIRM_OVERWRITE);
		break;
	      case TEXTSW_CONTENTS:
		temp = (textsw->state & TXTSW_NO_AGAIN_RECORDING);
		if (!(textsw->state & TXTSW_INITIALIZED))
		    textsw->state |= TXTSW_NO_AGAIN_RECORDING;
		(void) textsw_replace(VIEW_REP_TO_ABS(view), 0,
				 TEXTSW_INFINITY, attrs[1], strlen(attrs[1]));
		if (!(textsw->state & TXTSW_INITIALIZED))
		    SET_BOOL_FLAG(textsw->state, temp,
				  TXTSW_NO_AGAIN_RECORDING);
		break;
	      case TEXTSW_CONTROL_CHARS_USE_FONT:
		(void) ei_set(textsw->views->eih,
			      EI_CONTROL_CHARS_USE_FONT, attrs[1], 0);
		break;
	      case TEXTSW_DESTROY_ALL_VIEWS:
		SET_BOOL_FLAG(textsw->state, attrs[1],
			      TXTSW_DESTROY_ALL_VIEWS);
		break;
	      case TEXTSW_DISABLE_CD:
		SET_BOOL_FLAG(textsw->state, attrs[1], TXTSW_NO_CD);
		break;
	      case TEXTSW_DISABLE_LOAD:
		SET_BOOL_FLAG(textsw->state, attrs[1], TXTSW_NO_LOAD);
		break;
	      case TEXTSW_STORE_CHANGES_FILE:
		SET_BOOL_FLAG(textsw->state, attrs[1],
			      TXTSW_STORE_CHANGES_FILE);
		break;
	      case TEXTSW_EDIT_BACK_CHAR:
		textsw->edit_bk_char = (char)(attrs[1]);
		break;
	      case TEXTSW_EDIT_BACK_WORD:
		textsw->edit_bk_word = (char)(attrs[1]);
		break;
	      case TEXTSW_EDIT_BACK_LINE:
		textsw->edit_bk_line = (char)(attrs[1]);
		break;
	      case TEXTSW_ERROR_MSG:
		error_msg = (char *)(attrs[1]);
		error_msg[0] = '\0';
		break;
	      case TEXTSW_ES_CREATE_PROC:
		textsw->es_create = (Es_handle (*)())LINT_CAST(attrs[1]);
		break;
	      case TEXTSW_FILE:
		file = (char *)(attrs[1]);
		break;
	      case TEXTSW_FILE_CONTENTS: 
	      
	        textsw_flush_caches(view, TFC_PD_SEL);		 
		ps_esh = ES_NULL;
		str_length = (attrs[1] ? strlen(attrs[1]) : 0);
		
		if (!file) {    
		    if (str_length > 0) {
		        scratch_esh = es_file_create(attrs[1], 0, &es_status);
		        /* Ensure no caret turds will leave behind */
	    		textsw_take_down_caret(textsw);
	
		    } else
		        scratch_esh = textsw->views->esh;
		    
		    if (scratch_esh) {
		        mem_esh = es_mem_create(es_get_length(scratch_esh) + 1, "");
		        if (mem_esh) {
			    if (es_copy(scratch_esh, mem_esh, FALSE) != ES_SUCCESS) {
				es_destroy(mem_esh);
				mem_esh = ES_NULL;
			    }
		        }
		        if (str_length > 0)  {
			    es_destroy(scratch_esh);
		        }
			if (mem_esh) {
			    scratch_esh = es_mem_create(textsw->es_mem_maximum, "");
			    if (scratch_esh) {
				ps_esh = textsw_create_ps(textsw, mem_esh, scratch_esh,
							  &es_status);
			    } else {
				es_destroy(mem_esh);
			    }
			}
		    }
		}
		if (ps_esh) {	    
		    textsw_replace_esh(textsw, ps_esh);
		    
		    if (str_length > 0)  {
		        Ev_handle		ev_next;
		        Ev_impl_line_seq	seq;
		        
			(void) ev_set_insert(textsw->views, es_get_length(ps_esh));
			
			FORALLVIEWS(textsw->views, ev_next) {
			    seq = (Ev_impl_line_seq) ev_next->line_table.seq;
			    seq[0].damaged = 0;  /* Set damage bit in line table to force redisplay  */
		        }
		        display_views = 2;  /* TEXTSW_FIRST will set it to 0 to avoid repaint */
		        all_views = TRUE; /*  For TEXTSW_FIRST, or TEXTSW_FIRST_LINE */	
                        /* bug 1009822 unecessary painting of caret
			textsw_invert_caret(textsw);		          
                        */ 
		    }
		} else {
		    *status_ptr = TEXTSW_STATUS_OTHER_ERROR;
		}
		
		break;
	      case TEXTSW_FIRST:
		*status_ptr = set_first(view, error_msg, file, reset_mode,
					(Es_index)(attrs[1]), -1,
					all_views);
		/*
		 *  Make sure the scrollbar get updated,
		 *  when this attribute is called with TEXT_FIRST.
		 */
		if (file && !update_scrollbar) 
			update_scrollbar = 2;

		file = NULL;
		display_views = 0;
		break;
	      case TEXTSW_FIRST_LINE:
		*status_ptr = set_first(view, error_msg, file, reset_mode,
					ES_CANNOT_SET, (int)(attrs[1]),
					all_views);
		/*
		 *  Make sure the scrollbar get updated,
		 *  when this attribute is called with TEXT_FIRST.
		 */
		if (file && !update_scrollbar) 
			update_scrollbar = 2;

		file = NULL;
		display_views = 0;
		break;
	      case TEXTSW_CHECKPOINT_FREQUENCY:
		textsw->checkpoint_frequency = (int)(attrs[1]);
		break;
	      case TEXTSW_FONT:
	      case WIN_FONT:
		if (attrs[1]) {
		    Ev_handle	ev_next;
		    if (textsw->state & TXTSW_INITIALIZED) {
			if (textsw->state & TXTSW_OPENED_FONT) {
		    	    PIXFONT	*old_font;
			    old_font = (PIXFONT *)LINT_CAST(
			    		ei_get(textsw->views->eih, EI_FONT) );
			    pf_close(old_font);
			    textsw->state &= ~TXTSW_OPENED_FONT;
			}
			(void) ei_set(textsw->views->eih,
					EI_FONT, attrs[1], 0);
		    }
		/* Adjust the views to account for the font change */
		    FORALLVIEWS(textsw->views, ev_next) {
		    (void) ev_set(ev_next,
			    EV_CLIP_RECT, &ev_next->rect,
			    EV_RECT, &ev_next->rect, 0);
		    }
		}
		break;
	      case TEXTSW_HISTORY_LIMIT:
		/* Internally we want one more again slot that client does.  */
		temp = (int)(attrs[1]);
		temp = (temp > 0) ? temp+1 : 0;
		textsw_init_undo(textsw, temp);
		break;
	      case TEXTSW_IGNORE_LIMIT:
		textsw->ignore_limit = (unsigned)(attrs[1]);
		break;
	      case TEXTSW_INSERT_FROM_FILE: {
	      	pkg_private Textsw_status   textsw_get_from_file();
	      	
	        *status_ptr = textsw_get_from_file(view, (char *)(attrs[1]),
	        				   (status_ptr == &status_dummy));
	        if (*status_ptr == TEXTSW_STATUS_OKAY)
	            update_scrollbar = 2;
		break;
		};
	      case TEXTSW_INSERT_MAKES_VISIBLE:
		switch ((Textsw_enum)attrs[1]) {
		  case TEXTSW_NEVER:
		  case TEXTSW_ALWAYS:
		  case TEXTSW_IF_AUTO_SCROLL:
		    textsw->insert_makes_visible = (Textsw_enum)attrs[1];
		    break;
		  default:
		    *status_ptr = TEXTSW_STATUS_BAD_ATTR_VALUE;
		    break;
		}
		break;
	      case TEXTSW_INSERTION_POINT:
		(void) textsw_set_insert(textsw, (Es_index)(attrs[1]));
		break;
	      case TEXTSW_LEFT_MARGIN:
		(void) ev_set(view->e_view,
			      (all_views) ?
				 EV_FOR_ALL_VIEWS : EV_END_ALL_VIEWS,
			      EV_LEFT_MARGIN, (int)(attrs[1]),
			      0);
		display_views = (all_views) ? 2 : 1;
		break;
	      case TEXTSW_LINE_BREAK_ACTION: {
		Ev_right_break	ev_break_action;
		switch ((Textsw_enum)attrs[1]) {
		  case TEXTSW_CLIP:
		    ev_break_action = EV_CLIP;
		    goto TLBA_Tail;
		  case TEXTSW_WRAP_AT_CHAR:
		    ev_break_action = EV_WRAP_AT_CHAR;
		    goto TLBA_Tail;
		  case TEXTSW_WRAP_AT_WORD:
		    ev_break_action = EV_WRAP_AT_WORD;
TLBA_Tail:
		    (void) ev_set(view->e_view,
				  (all_views) ?
				    EV_FOR_ALL_VIEWS : EV_END_ALL_VIEWS,
				  EV_RIGHT_BREAK, ev_break_action,
				  0);
		    display_views = (all_views) ? 2 : 1;
		    break;
		  default:
		    *status_ptr = TEXTSW_STATUS_BAD_ATTR_VALUE;
		    break;
		}
		break;
	      }
	      case TEXTSW_LOAD_DIR_IS_CD:
		switch ((Textsw_enum)attrs[1]) {
		  case TEXTSW_ALWAYS:
		    textsw->state |= TXTSW_LOAD_CAN_CD;
		    break;
		  case TEXTSW_NEVER:
		    textsw->state &= ~TXTSW_LOAD_CAN_CD;
		    break;
		  default:
		    *status_ptr = TEXTSW_STATUS_BAD_ATTR_VALUE;
		    break;
		}
		break;
	      case TEXTSW_LOWER_CONTEXT:
		(void) ev_set(view->e_view,
			      EV_CHAIN_LOWER_CONTEXT, (int)(attrs[1]),
			      0);
		break;
	      case TEXTSW_MEMORY_MAXIMUM:
		textsw->es_mem_maximum = (unsigned)(attrs[1]);
		if (textsw->es_mem_maximum == 0) {
		    textsw->es_mem_maximum = TEXTSW_INFINITY;
		} else if (textsw->es_mem_maximum < 1000)
		    textsw->es_mem_maximum = 1000;
		break;
	      case WIN_MENU:
	      case TEXTSW_MENU:
		textsw->menu = attrs[1];
		break;
	      case TEXTSW_MULTI_CLICK_SPACE:
		if ((int)(attrs[1]) != -1)
		    textsw->multi_click_space = (int)(attrs[1]);
		break;
	      case TEXTSW_MULTI_CLICK_TIMEOUT:
		if ((int)(attrs[1]) != -1)
		    textsw->multi_click_timeout = (int)(attrs[1]);
		break;
		
	      case TEXTSW_NO_REPAINT_TIL_EVENT:
	        ev_set(view->e_view, EV_NO_REPAINT_TIL_EVENT, (int)(attrs[1]), 
	               0);
		break;		
	      case TEXTSW_NO_RESET_TO_SCRATCH:
		SET_BOOL_FLAG(textsw->state, attrs[1],
			      TXTSW_NO_RESET_TO_SCRATCH);
		break;
	      case TEXTSW_NOTIFY_LEVEL:
		textsw->notify_level = (int)(attrs[1]);
		break;
	      case TEXTSW_NOTIFY_PROC:
		textsw->notify = (int (*)())LINT_CAST(attrs[1]);
		if (textsw->notify_level == 0)
		    textsw->notify_level = TEXTSW_NOTIFY_STANDARD;
		break;
	      case TEXTSW_READ_ONLY:
		temp = TXTSW_IS_READ_ONLY(textsw);
		SET_BOOL_FLAG(textsw->state, attrs[1], TXTSW_READ_ONLY_ESH);
		set_read_only_esh = (textsw->state & TXTSW_READ_ONLY_ESH);
		ceased_read_only = (temp && !TXTSW_IS_READ_ONLY(textsw));
		break;
	      case TEXTSW_RESET_MODE:
		reset_mode = (int)(attrs[1]);
		break;
	      case TEXTSW_RESET_TO_CONTENTS:
		(void)textsw_reset_2(VIEW_REP_TO_ABS(view), 0, 0, TRUE, FALSE);
		break;
	      case TEXTSW_RIGHT_MARGIN:
		(void) ev_set(view->e_view,
			      (all_views) ?
				EV_FOR_ALL_VIEWS : EV_END_ALL_VIEWS,
			      EV_RIGHT_MARGIN, (int)(attrs[1]),
			      0);
		display_views = (all_views) ? 2 : 1;
		break;
	      case TEXTSW_SCROLLBAR:
	      case WIN_VERTICAL_SCROLLBAR:
		textsw_add_scrollbar_to_view(view, (Scrollbar)(attrs[1]));
		break;
	      case TEXTSW_STATUS:
		status_ptr = (Textsw_status *)LINT_CAST(attrs[1]);
		*status_ptr = TEXTSW_STATUS_OKAY;
		break;
	      case TEXTSW_STORE_SELF_IS_SAVE:
		SET_BOOL_FLAG(textsw->state, attrs[1],
			      TXTSW_STORE_SELF_IS_SAVE);
		break;
	      case TEXTSW_TAB_WIDTH:
		(void) ei_set(textsw->views->eih, EI_TAB_WIDTH, attrs[1], 0);
		break;
	      case TEXTSW_TEMP_FILENAME:
	        textsw->temp_filename = strdup((char *)LINT_CAST(attrs[1]));
	        break;
	      case TEXTSW_TOOL:
		textsw->tool = (attrs[1]);
		break;
	      case TEXTSW_UPDATE_SCROLLBAR:
		update_scrollbar = (all_views) ? 2 : 1;
		break;
	      case TEXTSW_UPPER_CONTEXT:
		(void) ev_set(view->e_view,
			      EV_CHAIN_UPPER_CONTEXT, (int)(attrs[1]),
			      0);
		break;
	      case TEXTSW_WRAPAROUND_SIZE:
		es_set(textsw->views->esh,
		       ES_PS_SCRATCH_MAX_LEN, attrs[1],
		       0);
		break;
#ifdef DEBUG
	      case TEXTSW_MALLOC_DEBUG_LEVEL:
		malloc_debug((int)attrs[1]);
		break;
#endif
	      case TEXTSW_HEIGHT:
	      case TEXTSW_WIDTH:
		/* BUG ALERT!  Explicitly ignore these for now. */
		break;
	      case TEXTSW_EDIT_COUNT:
		(void) fprintf(stderr, "%s: %s is read-only attribute.\n",
				"textsw_set",
				attr_sprint(expand_attr, (u_int)attrs[0]));
		break;

	      case TEXTSW_CONSUME_ATTRS:
		consume_attrs = (attrs[1] ? 1 : 0);
		break;

	      default:
		if (ATTR_PKG_TEXTSW == ATTR_PKG(attrs[0])) {
		    extern char *attr_sprint();
		    (void) fprintf(stderr, "%s: %s not a valid attribute.\n",
				   "textsw_set",
				   attr_sprint(expand_attr, (u_int)attrs[0]));
		}
		break;
	    }
	    if (consume_attrs & 0xFFFF0000)
		ATTR_CONSUME(*attrs);
	}

	if (file) {
	    *status_ptr = set_first(view, error_msg, file, reset_mode,						    ES_CANNOT_SET, -1, all_views);
	    /* 
	     *  This is for resetting the TXTSW_READ_ONLY_ESH flag that got 
	     *  cleared in textsw_replace_esh 
	     */	
	    if (set_read_only_esh) {
	    	SET_BOOL_FLAG(textsw->state, TRUE, TXTSW_READ_ONLY_ESH);
	    };

	    display_views = 0;
	}
	if (display_views && (textsw->state & TXTSW_DISPLAYED)) {
	    FORALL_TEXT_VIEWS(textsw, next) {
		if ((display_views == 1) && (next != view))
		    continue;
		textsw_display_view_margins(next, RECT_NULL);
		ev_display_view(next->e_view);
	    }
	    update_scrollbar = display_views;
	}
	if (update_scrollbar) {
	    textsw_update_scrollbars(textsw,
			(update_scrollbar == 2) ? (Textsw_view)0 : view);
	}
	if (ceased_read_only) {
	    textsw_add_timer(textsw, &textsw->timer);
	}
	return(status);
}

static Defaults_pairs insert_makes_visible_pairs[] = {
	"If_auto_scroll",	(int)TEXTSW_IF_AUTO_SCROLL,
	"Always",		(int)TEXTSW_ALWAYS,
	NULL, 			(int)TEXTSW_IF_AUTO_SCROLL
};

static Defaults_pairs load_file_of_dir_pairs[] = {
	"Is_error",		(int)TEXTSW_NEVER,
	"Is_set_directory",	(int)TEXTSW_ALWAYS,
	NULL, 			(int)TEXTSW_NEVER
};

static Defaults_pairs line_break_pairs[] = {
	"Clip",		(int)TEXTSW_CLIP,
	"Wrap_char",	(int)TEXTSW_WRAP_AT_CHAR,
	"Wrap_word",	(int)TEXTSW_WRAP_AT_WORD,
	NULL,		(int)TEXTSW_WRAP_AT_CHAR
};

static caddr_t
textsw_get_from_defaults(attribute)
	register Textsw_attribute	 attribute;
{
	char		*def_str;   /* Points to strings owned by defaults. */

	switch (attribute) {
	  case TEXTSW_ADJUST_IS_PENDING_DELETE:
	    return((caddr_t)
		defaults_get_boolean("/Text/Adjust_is_pending_delete",
				     False, NULL));
	  case TEXTSW_AGAIN_LIMIT:
	    return((caddr_t)
		defaults_get_integer_check("/Text/Again_limit",
					   1, 0, 500, NULL));
	  case TEXTSW_AUTO_INDENT:
	    return((caddr_t)
		defaults_get_boolean("/Text/Auto_indent", False, NULL));
	  case TEXTSW_AUTO_SCROLL_BY:
	    return((caddr_t)
		defaults_get_integer_check("/Text/Auto_scroll_by",
					   1, 0, 100, NULL));
	  case TEXTSW_BLINK_CARET:
	    return((caddr_t)
		defaults_get_boolean("/Text/Blink_caret", True, NULL));
	  case TEXTSW_CHECKPOINT_FREQUENCY:
	    /* Not generally settable via defaults */
	    return((caddr_t) 0);
	  case TEXTSW_CONFIRM_OVERWRITE:
	    return((caddr_t)
		defaults_get_boolean("/Text/Confirm_overwrite", True, NULL));
	  case TEXTSW_CONTROL_CHARS_USE_FONT:
	    return((caddr_t)
		defaults_get_boolean("/Text/Control_chars_use_font",
		    False, NULL));
	  case TEXTSW_EDIT_BACK_CHAR:
	    return((caddr_t)
		defaults_get_character("/Text/Edit_back_char",
					  DEL, (int *)0));
	  case TEXTSW_EDIT_BACK_WORD:
	    return((caddr_t)
		defaults_get_character("/Text/Edit_back_word",
					  CTRL(W), (int *)0));
	  case TEXTSW_EDIT_BACK_LINE:
	    return((caddr_t)
		defaults_get_character("/Text/Edit_back_line",
					  CTRL(U), (int *)0));
	  case WIN_FONT:
	  case TEXTSW_FONT: {
	    PIXFONT	*pf_open();
	    PIXFONT	*font;
	    def_str = defaults_get_string("/Text/Font", (char *)0, (int *)0);
	    if ((font = pf_open(def_str)) == NULL) {
		font = pf_open((char *)0);	/* standard system font */
	    }
	    return((caddr_t)font);
	  }
	  case TEXTSW_HISTORY_LIMIT:
	    return((caddr_t)
		defaults_get_integer_check("/Text/History_limit",
					   50, 0, 500, NULL));
	  case TEXTSW_INSERT_MAKES_VISIBLE:
	    def_str = defaults_get_string("/Text/Insert_makes_caret_visible",
					  (char *)0, (int *)0);
	    return((caddr_t)
		defaults_lookup(def_str, insert_makes_visible_pairs));
	  case TEXTSW_LINE_BREAK_ACTION:
	    def_str = defaults_get_string("/Text/Long_line_break_mode",
					  (char *)0, (int *)0);
	    return((caddr_t)
		defaults_lookup(def_str, line_break_pairs));
	  case TEXTSW_LOAD_DIR_IS_CD:
	    def_str = defaults_get_string("/Text/Load_file_of_directory",
					  (char *)0, (int *)0);
	    return((caddr_t)
		defaults_lookup(def_str, load_file_of_dir_pairs));
	  case TEXTSW_LOWER_CONTEXT:
	    return((caddr_t)
		defaults_get_integer_check("/Text/Lower_context",
					   0, EV_NO_CONTEXT, 50, NULL));
	  case TEXTSW_MULTI_CLICK_SPACE:
	    return((caddr_t)
		defaults_get_integer_check("/Text/Multi_click_space",
					   3, 0, 500, NULL));
	  case TEXTSW_MULTI_CLICK_TIMEOUT:
	    return((caddr_t)
		defaults_get_integer_check("/Text/Multi_click_timeout",
					   390, 0, 2000, NULL));
	  case TEXTSW_STORE_CHANGES_FILE:
	    return((caddr_t)
		defaults_get_boolean("/Text/Store_changes_file", True, NULL));
	  case TEXTSW_STORE_SELF_IS_SAVE:
	    return((caddr_t)
		defaults_get_boolean("/Text/Store_self_is_save", True, NULL));
	  case TEXTSW_UPPER_CONTEXT:
	    return((caddr_t)
		defaults_get_integer_check("/Text/Upper_context",
					   2, EV_NO_CONTEXT, 50, NULL));
	  case TEXTSW_LEFT_MARGIN:
	    return((caddr_t)
		defaults_get_integer_check("/Text/Left_margin",
					   4, 0, 2000, NULL));
	  case TEXTSW_RIGHT_MARGIN:
	    return((caddr_t)
		defaults_get_integer_check("/Text/Right_margin",
					   0, 0, 2000, NULL));
	  case TEXTSW_TAB_WIDTH:
	    return((caddr_t)
		defaults_get_integer_check("/Text/Tab_width",
					   8, 0, 50, NULL));
	  default:
	    return((caddr_t)0);
	}
}

#ifndef lint
static int
count_args(arg1, va_alist)
	caddr_t	arg1;
	va_dcl
{
	register int	count;
	va_list		args;

	va_start(args);
	for (count = 2; va_arg(args, caddr_t); count++) {}
	va_end(args);
	return(count);
}
#endif

/* VARARGS1 */
extern caddr_t
textsw_get(abstract, va_alist)
	Textsw			 abstract;
	va_dcl
{
	Textsw_view		 view;
	register Textsw_folio	 textsw;
	Es_index		 pos;	  /* pos, buf and buf_len are */
	char			*buf;	  /* temporaries for TEXTSW_CONTENTS */
	int			 buf_len;
	Textsw_attribute	 attribute;
	va_list			 args;

	if ((caddr_t (*)())LINT_CAST(abstract) == textsw_window_object) {
	    /* Client called textsw_get(TEXTSW, TEXTSW_XXX); */
	    int		i;
#ifndef lint
	    if (TEXTSW_count == 0)
		TEXTSW_count = count_args(TEXTSW, 0) - 1;
#endif
	    va_start(args);
	    for (i = 0; i < TEXTSW_count-1; i++) {va_arg(args, caddr_t);}
	    attribute = va_arg(args, Textsw_attribute);
	    va_end(args);
	    return(textsw_get_from_defaults(attribute));
	}

	if (abstract == 0)
	    return((caddr_t)0);

	view = VIEW_ABS_TO_REP(abstract);
	textsw = FOLIO_FOR_VIEW(view);
	va_start(args);
	attribute = va_arg(args, Textsw_attribute);
	switch (attribute) {
	  case TEXTSW_INSERTION_POINT:
	  case TEXTSW_LENGTH:
	    textsw_flush_caches(view, TFC_STD);
	    break;
	  case TEXTSW_CONTENTS:
	    pos = va_arg(args, Es_index);
	    buf = va_arg(args, caddr_t);
	    buf_len = va_arg(args, int);
	    break;
	}
	va_end(args);
	/* Note that ev_get(chain, EV_CHAIN_xxx) casts chain to be view in
	 * order to keep lint happy.
	 */
	switch (attribute) {
	  case TEXTSW_ADJUST_IS_PENDING_DELETE:
	    return((caddr_t)
		   BOOL_FLAG_VALUE(textsw->state, TXTSW_ADJUST_IS_PD));
	  case TEXTSW_AGAIN_LIMIT:
	    return((caddr_t)
		   ((textsw->again_count) ? (textsw->again_count-1) : 0));
	  case TEXTSW_AGAIN_RECORDING:
	    return((caddr_t)
		   !BOOL_FLAG_VALUE(textsw->state, TXTSW_NO_AGAIN_RECORDING));
	  case TEXTSW_AUTO_INDENT:
	    return((caddr_t)
		   BOOL_FLAG_VALUE(textsw->state, TXTSW_AUTO_INDENT));
	  case TEXTSW_AUTO_SCROLL_BY:
	    return(ev_get((Ev_handle)(textsw->views),
			  EV_CHAIN_AUTO_SCROLL_BY));
	  case TEXTSW_BLINK_CARET:
	    return((caddr_t)
		   BOOL_FLAG_VALUE(textsw->caret_state, TXTSW_CARET_FLASHING));
	  case TEXTSW_BROWSING:
	    return((caddr_t)
		   BOOL_FLAG_VALUE(textsw->state, TXTSW_READ_ONLY_SW));
	  case TEXTSW_CLIENT_DATA:
	    return(textsw->client_data);
	  case HELP_DATA:
	    return(textsw->help_data);
	  case TEXTSW_COALESCE_WITH:
	    return((caddr_t)textsw->coalesce_with);
	  case TEXTSW_CONFIRM_OVERWRITE:
	    return((caddr_t)
		   BOOL_FLAG_VALUE(textsw->state, TXTSW_CONFIRM_OVERWRITE));
	  case TEXTSW_CONTENTS:
	    return((caddr_t)
		   textsw_get_contents(textsw, pos, buf, buf_len) ); 
	  case TEXTSW_CONTROL_CHARS_USE_FONT:
	    return((caddr_t)
		   ei_get(textsw->views->eih, EI_CONTROL_CHARS_USE_FONT) );
	  case TEXTSW_DESTROY_ALL_VIEWS:
	    return((caddr_t)
		   BOOL_FLAG_VALUE(textsw->state, TXTSW_DESTROY_ALL_VIEWS));
	  case TEXTSW_DISABLE_CD:
	    return((caddr_t)
		   BOOL_FLAG_VALUE(textsw->state, TXTSW_NO_CD));
	  case TEXTSW_DISABLE_LOAD:
	    return((caddr_t)
		   BOOL_FLAG_VALUE(textsw->state, TXTSW_NO_LOAD));
	  case TEXTSW_EDIT_BACK_CHAR:
	    return((caddr_t)textsw->edit_bk_char);
	  case TEXTSW_EDIT_BACK_WORD:
	    return((caddr_t)textsw->edit_bk_word);
	  case TEXTSW_EDIT_BACK_LINE:
	    return((caddr_t)textsw->edit_bk_line);
	  case TEXTSW_EDIT_COUNT:
	    return(ev_get((Ev_handle)(textsw->views),
			  EV_CHAIN_EDIT_NUMBER));
	  case TEXTSW_WRAPAROUND_SIZE:
	    return(es_get(textsw->views->esh, ES_PS_SCRATCH_MAX_LEN));
	  case TEXTSW_ES_CREATE_PROC:
	    return((caddr_t)LINT_CAST(textsw->es_create));
	  case TEXTSW_FILE: {
	    char	*name;
	    if (textsw_file_name(textsw, &name))
		return((caddr_t)0);
	    else
		return((caddr_t)name);
	  }
	  case TEXTSW_FIRST:
	    return((caddr_t)
	      ft_position_for_index(view->e_view->line_table, 0));
	  case TEXTSW_FIRST_LINE: {
	    int	top, bottom;
	    ev_line_info(view->e_view, &top, &bottom);
	    return((caddr_t)top-1);
	  }
	  case TEXTSW_FONT:
	  case WIN_FONT:
	    return(ei_get(textsw->views->eih, EI_FONT));
	  case TEXTSW_HEIGHT:
	  case WIN_HEIGHT:
	    return((caddr_t)textsw->first_view->rect.r_height);
	  case TEXTSW_HISTORY_LIMIT:
	    return((caddr_t)
		   ((textsw->undo_count) ? (textsw->undo_count-1) : 0));
	  case TEXTSW_IGNORE_LIMIT:
	    return((caddr_t)textsw->ignore_limit);
	  case TEXTSW_INSERT_MAKES_VISIBLE:
	    return((caddr_t)textsw->insert_makes_visible);
	  case TEXTSW_INSERTION_POINT:
	    return((caddr_t)ev_get_insert(textsw->views));
	  case TEXTSW_LENGTH:
	    return((caddr_t)es_get_length(textsw->views->esh));
	  case TEXTSW_LOAD_DIR_IS_CD:
	    return((caddr_t)((textsw->state & TXTSW_LOAD_CAN_CD)
			     ? TEXTSW_ALWAYS : TEXTSW_NEVER) );
	  case TEXTSW_LOWER_CONTEXT:
	    return(ev_get((Ev_handle)(textsw->views), EV_CHAIN_LOWER_CONTEXT));
	  case TEXTSW_MODIFIED:
	    return((caddr_t)textsw_has_been_modified(abstract));
	  case TEXTSW_MEMORY_MAXIMUM:
	    return((caddr_t)textsw->es_mem_maximum);
	  case WIN_MENU:
	  case TEXTSW_MENU: {
	    extern Menu textsw_get_unique_menu();

	    return((caddr_t)textsw_get_unique_menu(textsw));
	    }
	  case TEXTSW_MULTI_CLICK_SPACE:
	    return((caddr_t)textsw->multi_click_space);
	  case TEXTSW_MULTI_CLICK_TIMEOUT:
	    return((caddr_t)textsw->multi_click_timeout);
	  case TEXTSW_NO_RESET_TO_SCRATCH:
	    return((caddr_t)
		   BOOL_FLAG_VALUE(textsw->state, TXTSW_NO_RESET_TO_SCRATCH));
	  case TEXTSW_NOTIFY_LEVEL:
	    return((caddr_t)textsw->notify_level);
	  case TEXTSW_NOTIFY_PROC:
	    return((caddr_t)LINT_CAST(textsw->notify));
	  case TEXTSW_PIXWIN:
	    return((caddr_t)PIXWIN_FOR_VIEW(view));
	  case TEXTSW_READ_ONLY:
	    return((caddr_t)
		   BOOL_FLAG_VALUE(textsw->state, TXTSW_READ_ONLY_ESH));
	  case TEXTSW_STORE_SELF_IS_SAVE:
	    return((caddr_t)
		   BOOL_FLAG_VALUE(textsw->state, TXTSW_STORE_SELF_IS_SAVE));
	  case TEXTSW_TAB_WIDTH:
	    return(ei_get(textsw->views->eih, EI_TAB_WIDTH));
	  case TEXTSW_TEMP_FILENAME:
	    return(textsw->temp_filename);
	  case TEXTSW_TOOL:
	    return((caddr_t)textsw->tool);
	  case TEXTSW_UPPER_CONTEXT:
	    return(ev_get((Ev_handle)(textsw->views), EV_CHAIN_UPPER_CONTEXT));
	  case TEXTSW_LEFT_MARGIN:
	    return(ev_get(view->e_view, EV_LEFT_MARGIN));
	  case TEXTSW_RIGHT_MARGIN:
	    return(ev_get(view->e_view, EV_RIGHT_MARGIN));
	  case TEXTSW_SCROLLBAR:
	  case WIN_VERTICAL_SCROLLBAR:
	    return((caddr_t)SCROLLBAR_FOR_VIEW(view));
	  case TEXTSW_WIDTH:
	  case WIN_WIDTH:
	    return((caddr_t)textsw->first_view->rect.r_width);
	  case TEXTSW_LINE_BREAK_ACTION:/* Bug: should be able to get this. */
	  case TEXTSW_MENU_STYLE:
	    return((caddr_t)textsw_get_menu_style_internal());
	  default:
	    return((caddr_t)0);
	}
}

/* VARARGS1 */
extern Textsw_status
textsw_set(abstract, va_alist)
	Textsw		abstract;
	va_dcl
{
	caddr_t		attr_argv[ATTR_STANDARD_SIZE];
	va_list		args;

	va_start(args);
	(void) attr_make(attr_argv, ATTR_STANDARD_SIZE, args);
	va_end(args);
	return(textsw_set_internal(VIEW_ABS_TO_REP(abstract), attr_argv));
}

/* VARARGS1 */
pkg_private void
textsw_notify(view, va_alist)
	Textsw_view		view;
	va_dcl
{
	register Textsw_folio	folio;
	int			doing_event;
	caddr_t			attr_argv[ATTR_STANDARD_SIZE];
	va_list			args;

	va_start(args);
	view = VIEW_FROM_FOLIO_OR_VIEW(view);
	(void) attr_make(attr_argv, ATTR_STANDARD_SIZE, args);
	va_end(args);
	folio = FOLIO_FOR_VIEW(view);
	doing_event = (folio->state & TXTSW_DOING_EVENT);
	folio->state &= ~TXTSW_DOING_EVENT;
	folio->notify(VIEW_REP_TO_ABS(view), attr_argv);
	if (doing_event)
	    folio->state |= TXTSW_DOING_EVENT;
}

pkg_private void
textsw_notify_replaced(folio_or_view, insert_before, length_before,
		       replaced_from, replaced_to, count_inserted)
	Textsw_opaque		folio_or_view;
	Es_index		insert_before;
	Es_index		length_before;
	Es_index		replaced_from;
	Es_index		replaced_to;
	Es_index		count_inserted;
{
	Textsw_view		view = VIEW_FROM_FOLIO_OR_VIEW(folio_or_view);
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	int			in_notify_proc =
					folio->state & TXTSW_IN_NOTIFY_PROC;

	folio->state |= TXTSW_IN_NOTIFY_PROC;
	textsw_notify(view, TEXTSW_ACTION_REPLACED,
		      insert_before, length_before,
		      replaced_from, replaced_to, count_inserted, 0);
	if (!in_notify_proc)
	    folio->state &= ~TXTSW_IN_NOTIFY_PROC;
}

pkg_private Es_index
textsw_get_contents(textsw, position, buffer, buffer_length)
	register Textsw_folio	 textsw;
	Es_index		 position;
	char			*buffer;
	register int		 buffer_length;

{
	Es_index	 next_read_at;
	int		 read;

	es_set_position(textsw->views->esh, position);
	next_read_at = es_read(textsw->views->esh, buffer_length, buffer,
			       &read);
	if AN_ERROR(read != buffer_length) {
	    bzero(buffer+read, buffer_length-read);
	}
	return(next_read_at);
}

