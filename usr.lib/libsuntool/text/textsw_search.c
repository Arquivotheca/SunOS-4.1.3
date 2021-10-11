#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_search.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Routines for moving textsw caret.
 */
 
 
#include <suntool/primal.h>

#include <suntool/textsw_impl.h>
#include <suntool/ev_impl.h>
#include <sys/time.h>
#include <signal.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/window.h>
#include <suntool/textsw.h>
#include <suntool/walkmenu.h>
#include <suntool/wmgr.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_screen.h>


#define		MAX_DISPLAY_LENGTH	50
#define   	MAX_STR_LENGTH		1024
#define		MAX_PANEL_ITEMS		10

#define    DONT_RING_BELL		0x00000000
#define    RING_IF_NOT_FOUND		0x00000001
#define    RING_IF_ONLY_ONE		0x00000002

#define HELP_INFO(s) HELP_DATA,s,

typedef enum {
    FIND_ITEM			= 0,
    FIND_STRING_ITEM		= 1,
    DONE_ITEM			= 2,
    REPLACE_ITEM		= 3,
    REPLACE_STRING_ITEM		= 4,
    WRAP_ITEM			= 5,
    FIND_THEN_REPLACE_ITEM	= 6,
    REPLACE_THEN_FIND_ITEM	= 7,
    REPLACE_ALL_ITEM		= 8,
    BLINK_PARENT_ITEM		= 9
} Panel_item_enum;


static Panel_item	panel_items[MAX_PANEL_ITEMS];
static Menu		direction_menu;
int			process_aborted;	/* Stop key is down */

/*---------------------------------------------------
  search_frame represents the frame that the user can
  bring up to do find-and-replace operations on the text.
  It is also stored in the folio which owns the view
  from which this search_frame was created.
---------------------------------------------------*/
Frame			search_frame;

pkg_private Textsw_view
view_from_panel_item(panel_item)
	Panel_item	panel_item;
{
	Panel		panel = panel_get(panel_item, PANEL_PARENT_PANEL, 0);
	Window		search_frame = (Window) window_get (panel, WIN_OWNER, 0);
	Textsw_view	view = (Textsw_view) window_get(search_frame, WIN_CLIENT_DATA, 0);

	return(view);	
}

pkg_private int
textsw_get_selection(view, first, last_plus_one, selected_str, max_str_len)
	Textsw_view 	view;
	int		*first, *last_plus_one;
	char		*selected_str;
	int		max_str_len;
{
/*
 *  Return true iff primary selection is in the textsw of current view.
 *  If there is a selection in any of the textsw and selected_str is not null, then
 *  it will copy it to selected_str.
 */
	Textsw_folio		 folio = FOLIO_FOR_VIEW(view);
	Textsw_selection_object	 selection;
	char			 selection_buf[MAX_STR_LENGTH];
	unsigned        	 options = EV_SEL_PRIMARY;
	
	textsw_init_selection_object(
	    folio, &selection, selection_buf, sizeof(selection_buf), FALSE);
	
	selection.type = textsw_func_selection_internal(
		folio, &selection, EV_SEL_BASE_TYPE(options),
		TFS_FILL_ALWAYS);
	
	textsw_clear_secondary_selection(folio, selection.type);
	
	if ((selection.type & TFS_IS_SELF) &&
	    (selection.type & EV_SEL_PRIMARY)) {
	    /* If this window owns the primary selection, do nothing. */
	} else {        	    				    	    
	    selection.first = selection.last_plus_one = ES_CANNOT_SET;	    
	}
	
	
	if ((selection.type & EV_SEL_PRIMARY) &&
	     (selection.buf_len > 0) && (selected_str != NULL)) {
	     
	   if (selection.buf_len >= max_str_len)
	       selection.buf_len = max_str_len-1;
	         
	   strncpy(selected_str, selection.buf, selection.buf_len);
	   selected_str[selection.buf_len] = NULL;
	} 
		
	*first = selection.first;
	*last_plus_one = selection.last_plus_one;
	
	return((*first != ES_CANNOT_SET) && (*last_plus_one != ES_CANNOT_SET));
}


Es_index
do_search_proc(view, direction, ring_bell_status, wrapping_off)
	Textsw_view 		view;
	unsigned		direction;
	unsigned		ring_bell_status;
	int			wrapping_off;
	
{
	Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	Es_index	first, last_plus_one;
	char		buf[MAX_STR_LENGTH];
	int		str_len;
	Es_index	start_pos;	
	
	
	if (!textsw_get_selection(view, &first, &last_plus_one, NULL, 0))
	    first = last_plus_one = ev_get_insert(folio->views);
	    
	if (direction == EV_FIND_DEFAULT)  
	    first = last_plus_one;
	      
	strncpy (buf, (char *)panel_get( 
	        panel_items[(int)FIND_STRING_ITEM], PANEL_VALUE), MAX_STR_LENGTH);
	        
	str_len = strlen(buf);
	start_pos = (direction & EV_FIND_BACKWARD)
			? first : (first - str_len);

	textsw_find_pattern(folio, &first, &last_plus_one, buf, str_len, direction); 
	
	if (wrapping_off) {
	    if (direction == EV_FIND_DEFAULT)
	       first = (start_pos > last_plus_one) ? ES_CANNOT_SET : first;
	    else
	       first = (start_pos < last_plus_one) ? ES_CANNOT_SET : first;   
	}
	       
        if ((first == ES_CANNOT_SET) || (last_plus_one == ES_CANNOT_SET)) {
            if (ring_bell_status & RING_IF_NOT_FOUND)
               (void) window_bell(WINDOW_FROM_VIEW(view));
            return(ES_CANNOT_SET);
        } else {
            if ((ring_bell_status & RING_IF_ONLY_ONE) && (first == start_pos))
                (void) window_bell(WINDOW_FROM_VIEW(view));
                
            textsw_possibly_normalize_and_set_selection(
		VIEW_REP_TO_ABS(view), first, last_plus_one, EV_SEL_PRIMARY);
		
	    (void) textsw_set_insert(folio, last_plus_one);
	    textsw_record_find(folio, buf, str_len, (int)direction);
            return ((direction == EV_FIND_DEFAULT) ? last_plus_one : first);
        }	
}
	

static int
do_replace_proc(view)
    	Textsw_view 	view;
{
	Textsw		textsw = VIEW_REP_TO_ABS(view);
	char		buf[MAX_STR_LENGTH];
	
	int		selection_found;
	int		first, last_plus_one;
	
	
	
	 if (selection_found = textsw_get_selection(view, &first, &last_plus_one, NULL, 0)) {
	     strncpy (buf, (char *)panel_get (
	             panel_items[(int)REPLACE_STRING_ITEM], PANEL_VALUE), MAX_STR_LENGTH);
	     textsw_replace(textsw, first, last_plus_one, buf, strlen(buf));
	 }
	
	return(selection_found);
}



static void
replace_all_proc(view, do_replace_first, direction)
	Textsw_view 	view;
	int		do_replace_first;
	unsigned	direction;
{
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	int			start_checking = FALSE;   /* See if now is the time to check for wrap point */
	Es_index		cur_pos, prev_pos;
	register Es_index	cur_mark_pos, 
				first, last_plus_one;
	Ev_mark_object		mark;
	register int		exit_loop = FALSE;
	int			first_time = TRUE;
	int			wrapping_off = (int)panel_get(panel_items[(int)WRAP_ITEM], PANEL_VALUE);
        char str_buf[MAX_STR_LENGTH];
	

	if (do_replace_first)
	    (void) do_replace_proc(view);

	process_aborted = FALSE;
	
	cur_mark_pos = prev_pos = cur_pos = do_search_proc(view, direction, RING_IF_NOT_FOUND, wrapping_off);
	
	exit_loop = (cur_pos == ES_CANNOT_SET);
      
        /* get the selected string */
        strcpy (str_buf, (char *)panel_get
               (panel_items[(int)FIND_STRING_ITEM], PANEL_VALUE), 
                MAX_STR_LENGTH);
   
	while (!process_aborted && !exit_loop) {
	    if (start_checking) {
	        cur_mark_pos = textsw_find_mark_internal(folio, mark);
		     
		exit_loop =  (direction == EV_FIND_DEFAULT) ?
		    	(cur_mark_pos <= cur_pos) : (cur_mark_pos >= cur_pos);
	    } else {
	        /* Did we wrap around the file already */
	        if (!first_time && 
                    (prev_pos == cur_pos 
                     || ((strlen(str_buf) + prev_pos) > cur_pos)))

	        /* Only one instance of the pattern in the file. */
	            start_checking = TRUE;		 
	        else
	            start_checking = (direction == EV_FIND_DEFAULT) ? 
	                 (prev_pos > cur_pos) : (cur_pos > prev_pos);
		/*
	         * This is a special case
	         * Start search at the first instance of the pattern in the file.
	         */

	         if (start_checking) {
		    cur_mark_pos = textsw_find_mark_internal(folio, mark);
		    exit_loop =  (direction == EV_FIND_DEFAULT) ?
		        (cur_mark_pos <= cur_pos) : (cur_mark_pos >= cur_pos);
	         }     
	     }
	     
	     if (!exit_loop) {
		 (void) do_replace_proc(view);
		 
		 if (first_time) {
		     mark = textsw_add_mark_internal(folio, cur_mark_pos, 
		           TEXTSW_MARK_MOVE_AT_INSERT);
		     first_time = FALSE;
		 }
		 
	         prev_pos = cur_pos;    
	         cur_pos = do_search_proc(view, direction, DONT_RING_BELL, wrapping_off);
	         exit_loop = (cur_pos == ES_CANNOT_SET);
	     }
	}
	
	if (process_aborted)
	    window_bell(VIEW_REP_TO_ABS(view));
}

static void
do_destroy_proc (frame)
	Frame		frame;
{
	Textsw_view	view = (Textsw_view) window_get(frame, WIN_CLIENT_DATA, 0);
	Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	
	window_set(frame, FRAME_NO_CONFIRM, TRUE, 0);
	window_destroy(frame);
	
	search_frame = folio->search_frame = NULL;
}
	

static void
search_event_proc(item, event)
    	Panel_item	item;
    	Event		*event;
{
	Panel		panel = panel_get(item, PANEL_PARENT_PANEL, 0);
	Textsw_view	view = view_from_panel_item(item);
        int             wrapping_off = (int)panel_get(panel_items[(int)WRAP_ITEM], PANEL_VALUE);
	
	if (event_action(event) == MS_RIGHT && event_is_down(event)) {
	    int choice	= (int) menu_show(direction_menu, panel, event, 0);
	    switch (choice) {
	        case 1: 
	            (void) do_search_proc(view, EV_FIND_DEFAULT, 
	            	       (RING_IF_NOT_FOUND | RING_IF_ONLY_ONE), wrapping_off); 
		    break;
	        case 2:
	            (void) do_search_proc(view, EV_FIND_BACKWARD, 
	            	       (RING_IF_NOT_FOUND | RING_IF_ONLY_ONE), wrapping_off);
	             break;
	         default:
	             break; 
	    }
	    
	} else
	    panel_default_handle_event(item, event);
}


static void
slp(x)
{
	struct timeval tv;
	
	tv.tv_sec = x / 64;
	tv.tv_usec = (x % 64) * (1000000/64);
	select(32, 0, 0, 0, &tv);
}

pkg_private void
blink_window (win, num_of_blink)
	Window			win;
{
	register struct pixwin *pw;
	int			i;
	int			w, h;
	Frame			base_frame;
	Frame			frame;
	
	if (win == NULL)
	    return;
	    	
	if (base_frame = (Frame) window_get(win, WIN_OWNER))
	    frame = (window_get(base_frame, FRAME_CLOSED)) ? base_frame : win;
	else
	    frame = win;
		
	pw = (struct pixwin *)(LINT_CAST(window_get(frame, WIN_PIXWIN)));
	h = (int)window_get(frame, WIN_HEIGHT);
	w = (int)window_get(frame, WIN_WIDTH); 
	      
        for (i = 0; i < num_of_blink; i++) {
            (void)pw_writebackground(pw, 0, 0, w, h, PIX_NOT(PIX_DST));
            slp(2);
            (void)pw_writebackground(pw, 0, 0, w, h, PIX_NOT(PIX_DST));
        }
}
	
static void
cmd_proc(item, event)
	Panel_item	item;
    	Event		*event;
{
	Textsw_view	view = view_from_panel_item(item);
	Textsw_folio 	folio = FOLIO_FOR_VIEW(view);
        int             wrapping_off = (int)panel_get(panel_items[(int)WRAP_ITEM], PANEL_VALUE);
	
	if (item == panel_items[(int)FIND_ITEM]) {
	    (void) do_search_proc(view, EV_FIND_DEFAULT, (RING_IF_NOT_FOUND | RING_IF_ONLY_ONE), wrapping_off);
	    
	} else if (item == panel_items[(int)REPLACE_ITEM]) {
	    if (TXTSW_IS_READ_ONLY(folio) || !do_replace_proc(view)) {
	       (void) window_bell(VIEW_REP_TO_ABS(view));
	    }
	} else if (item == panel_items[(int)FIND_THEN_REPLACE_ITEM]) {
	    if (!TXTSW_IS_READ_ONLY(folio)) {
	        if (do_search_proc(view, EV_FIND_DEFAULT, RING_IF_NOT_FOUND, wrapping_off) != ES_CANNOT_SET) 
                   (void) do_replace_proc(view);
	    }
	} else if (item == panel_items[(int)REPLACE_THEN_FIND_ITEM]) {
	    if (!TXTSW_IS_READ_ONLY(folio)) {
                (void) do_replace_proc(view);
                (void) do_search_proc(view, EV_FIND_DEFAULT, RING_IF_NOT_FOUND, wrapping_off);
	    }
	} else if (item == panel_items[(int)DONE_ITEM]) {
	    (void) do_destroy_proc(folio->search_frame);
	    
	} else if (item == panel_items[(int)REPLACE_ALL_ITEM]) {
	    (void) replace_all_proc(view, FALSE, EV_FIND_DEFAULT);
	    
	} else if (item == panel_items[(int)BLINK_PARENT_ITEM]) {
	    (void) blink_window(VIEW_REP_TO_ABS(view), 3);
	}	
}

    		
static void
create_search_item(panel, view) 
  	Panel		panel;
  	Textsw_view	view;
{

	static char		*search = "Find";
	static char		*replace = "Replace";
	static char		*all    = "Replace All";
	static char		*search_replace = "Find then Replace";
	static char		*replace_search = "Replace then Find";
	static char		*done = "Done";
	static char		*backward = "Backward";
	static char		*forward = "Forward";
	static char		*blink_parent = "Blink Owner";
	char			search_string[MAX_STR_LENGTH];
	int			dummy;
	
	
	
	direction_menu = menu_create(
		MENU_ITEM,
		    MENU_STRING, forward,
		    MENU_VALUE, 1,
		    HELP_INFO("textsw:mdirforward")
		    0,
		MENU_ITEM,
		    MENU_STRING, backward,
		    MENU_VALUE, 2,
		    HELP_INFO("textsw:mdirbackward")
		    0,
		HELP_INFO("textsw:mdirection")
		0);	

	panel_items[(int)FIND_ITEM] = panel_create_item(panel,PANEL_BUTTON,
            PANEL_LABEL_IMAGE,
                        panel_button_image(panel, search, 7, NULL),
            PANEL_NOTIFY_PROC, cmd_proc,
            PANEL_EVENT_PROC,  search_event_proc,
	    HELP_INFO("textsw:find")
            0);
        
        search_string[0] = NULL;
	(void) textsw_get_selection(view, &dummy, &dummy, search_string, MAX_STR_LENGTH);
	                                         
        panel_items[(int)FIND_STRING_ITEM] =  panel_create_item(panel, PANEL_TEXT,
            PANEL_LABEL_Y,                 ATTR_ROW(0),
            PANEL_VALUE_DISPLAY_LENGTH,    MAX_DISPLAY_LENGTH,
            PANEL_VALUE_STORED_LENGTH,     MAX_STR_LENGTH,
            PANEL_LABEL_STRING,            ":",
            PANEL_VALUE, 		   search_string,
	    HELP_INFO("textsw:findstring")
            0);
        
        panel_items[(int)DONE_ITEM] =   panel_create_item(panel,PANEL_BUTTON,
            PANEL_LABEL_Y,                 ATTR_ROW(0),
            PANEL_LABEL_IMAGE,
                        panel_button_image(panel, done, 7, NULL),
            PANEL_NOTIFY_PROC,		   cmd_proc,
	    HELP_INFO("textsw:done")
	    0);

        
        panel_items[(int)REPLACE_ITEM] = panel_create_item(panel,PANEL_BUTTON,
            PANEL_LABEL_X,                 ATTR_COL(0),
            PANEL_LABEL_Y,                 ATTR_ROW(1),
            PANEL_LABEL_IMAGE,
                        panel_button_image(panel, replace, 7, NULL),
            PANEL_NOTIFY_PROC,		   cmd_proc,
	    HELP_INFO("textsw:replace")
	    0); 
         

        panel_items[(int)REPLACE_STRING_ITEM] = panel_create_item(panel, PANEL_TEXT,
            PANEL_LABEL_Y,                 ATTR_ROW(1),
            PANEL_VALUE_DISPLAY_LENGTH,    MAX_DISPLAY_LENGTH,
            PANEL_VALUE_STORED_LENGTH,     MAX_STR_LENGTH,
            PANEL_LABEL_STRING,            ":",
	    HELP_INFO("textsw:replacestring")
            0); 
        
        panel_items[(int)WRAP_ITEM] = panel_create_item(panel, PANEL_CYCLE,
            PANEL_LABEL_Y,                 ATTR_ROW(1),           
            PANEL_CHOICE_STRINGS, 	   "All Text", "To End", 0,
	    HELP_INFO("textsw:wrap")
            0); 
               
        panel_items[(int)FIND_THEN_REPLACE_ITEM] =  panel_create_item(panel,PANEL_BUTTON,
            PANEL_LABEL_X,                 ATTR_COL(0),
            PANEL_LABEL_Y,                 ATTR_ROW(2),
	    PANEL_LABEL_IMAGE,
                        panel_button_image(panel, search_replace, 0, NULL),
            PANEL_NOTIFY_PROC,  	cmd_proc,
	    HELP_INFO("textsw:findreplace")
            0); 
            
        panel_items[(int)REPLACE_THEN_FIND_ITEM] =  panel_create_item(panel,PANEL_BUTTON,
            
            PANEL_LABEL_IMAGE,
                        panel_button_image(panel, replace_search, 0, NULL),
            PANEL_NOTIFY_PROC, 		   cmd_proc,
	    HELP_INFO("textsw:replacefind")
            0); 
            
        panel_items[(int)REPLACE_ALL_ITEM] =  panel_create_item(panel,PANEL_BUTTON,
            
            PANEL_LABEL_IMAGE,
                        panel_button_image(panel, all, 0, NULL),
            PANEL_NOTIFY_PROC, 		   cmd_proc,
	    HELP_INFO("textsw:replaceall")
            0);
            
         panel_items[(int)BLINK_PARENT_ITEM] =  panel_create_item(panel,PANEL_BUTTON,
            PANEL_LABEL_X,                 ATTR_COL(59),
            PANEL_LABEL_IMAGE,
                        panel_button_image(panel, blink_parent, 0, NULL),
            PANEL_NOTIFY_PROC, 		   cmd_proc,
	    HELP_INFO("textsw:blinkparent")
            0);
                   
        if (search_string[0] != NULL)
            window_set(panel, PANEL_CARET_ITEM, panel_items[(int)REPLACE_STRING_ITEM], 0);	           
                                          
}

extern Panel
textsw_create_search_panel(frame, view)
	Frame		frame;
	Textsw_view	view;
{
	Panel		panel;
	
	panel = (Panel)(LINT_CAST(
	    window_create((Window)frame, PANEL,
			  HELP_INFO("textsw:searchpanel")
			  0)));
	create_search_item(panel, view);  
	
	return(panel);  
}

static Notify_value
abort_search_proc (frame, signal, when)
	int			*frame;
	int			signal;
	Notify_signal_mode	when;
{
	if ((when == NOTIFY_ASYNC) && (signal == SIGURG))
	    process_aborted = TRUE;
	    
	return(NOTIFY_DONE);
}	

static Notify_value
base_frame_event_proc(base_frame, event, arg, type)
    Frame 	 base_frame;
    Event 	 *event;
    Notify_arg 	 arg;
    Notify_event_type type;
{
    Notify_value value = notify_next_event_func(base_frame, event, arg, type);

    switch (event_action(event)) {
      case WIN_REPAINT:
      case TXTSW_OPEN:
      case MS_RIGHT:
        if (search_frame != NULL) {
	    if (window_get(base_frame, FRAME_CLOSED)) {
	         if (window_get(search_frame, WIN_SHOW))
	             window_set(search_frame, WIN_SHOW, FALSE, 0);
	    } else {
	        if (!window_get(search_frame, WIN_SHOW))
	             window_set(search_frame, WIN_SHOW, TRUE, 0);
	    }
	}
	break;
    }
    return value;
}				
	
pkg_private void
textsw_set_pop_up_location (base_frame, pop_up_frame)
	Frame		base_frame, pop_up_frame;
{
/* base_frame and pop_up_frame has to be created with window_create not tool_create */
#define MY_OFFSET		4
	
	Rect		base_rect, pop_up_rect, screen_rect;
	short		new_x, new_y;
	int		max_cover_area;
	int		win_fd;
	struct screen	screen;
	
	if (!base_frame || !pop_up_frame)
	    return;
	    
	
	screen_rect = *((Rect *) (LINT_CAST(window_get(base_frame, WIN_SCREEN_RECT))));
	base_rect = *((Rect *) (LINT_CAST(window_get(base_frame, WIN_RECT))));
	pop_up_rect = *((Rect *) (LINT_CAST(window_get(pop_up_frame, WIN_RECT))));	    
	
	new_x = pop_up_rect.r_left;
	new_y = pop_up_rect.r_top;
	max_cover_area = base_rect.r_width / 3;
	
	if ((base_rect.r_top - (pop_up_rect.r_height + MY_OFFSET)) >= 0)
	    new_y = base_rect.r_top - (pop_up_rect.r_height + MY_OFFSET);
	else if ((base_rect.r_left - pop_up_rect.r_width + MY_OFFSET) >= 0)
	    new_x = base_rect.r_left - (pop_up_rect.r_width + MY_OFFSET);
	else if ((base_rect.r_left + base_rect.r_width + pop_up_rect.r_width + MY_OFFSET) <= 
	         screen_rect.r_width)
	    new_x = base_rect.r_left + base_rect.r_width;
	else if ((pop_up_rect.r_width + MY_OFFSET - base_rect.r_left) <= max_cover_area)
	    new_x = 0;
	else if ((base_rect.r_left + base_rect.r_width - max_cover_area) <= (screen_rect.r_width - (pop_up_rect.r_width + MY_OFFSET)))
	    new_x = screen_rect.r_width - (pop_up_rect.r_width + MY_OFFSET);        
        if (new_y < 0) new_y = 0;
	    
	pop_up_rect.r_left = new_x;        
        pop_up_rect.r_top = new_y; 
        
       
       win_setrect((int)window_get(pop_up_frame, WIN_FD), &pop_up_rect);
#undef MY_OFFSET	
}

	
extern void
textsw_create_search_frame(view)
	Textsw_view	view;
{
        
        register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
        Frame			base_frame = (Frame) window_get(view, WIN_OWNER);
	Panel			panel;
	char			*label = "Find and Replace";
	static int		already_interposed;
	
	if (!base_frame) {
	    (void)textsw_alert_for_old_interface(view);
	    return;
	}
	
	if (search_frame) {
	    register int	search_frame_fd = (int) window_fd(search_frame, WIN_FD);    
	    register int	root_fd;
	    char		str[MAX_STR_LENGTH];
	    int			dummy;
	    
	    folio->search_frame = search_frame;  /* In case the popup is for a different textsw */
	    window_set(folio->search_frame, WIN_CLIENT_DATA, view, 0);
	    
	    str[0] = NULL;
	    (void) textsw_get_selection(view, &dummy, &dummy, str, MAX_STR_LENGTH);
	    
	    if (strlen(str) > 0) {
	        panel = panel_get(panel_items[(int)FIND_STRING_ITEM], PANEL_PARENT_PANEL, 0);
	        (void) panel_set_value(panel_items[(int)FIND_STRING_ITEM], str);
	        (void) window_set(panel, PANEL_CARET_ITEM, panel_items[(int)REPLACE_STRING_ITEM], 0);
	    }
	    
	    if (win_getlink(search_frame_fd, WL_COVERING) == WIN_NULLLINK) {
	       (void) blink_window(folio->search_frame, 5);
	    } else {
	        root_fd = rootfd_for_toolfd(search_frame_fd);
	        wmgr_top(search_frame_fd, root_fd);
	        (void)close (root_fd);
	    }
	    

	    return;
	}
	    	  
	search_frame = folio->search_frame = window_create(base_frame, FRAME,
			FRAME_SHOW_LABEL, 	TRUE,
			FRAME_DONE_PROC, 	do_destroy_proc,
			WIN_CLIENT_DATA, 	view,
			0);
	
	if (!search_frame)
	    return;
	    		
	panel = textsw_create_search_panel(folio->search_frame, view);
	
	(void) notify_set_signal_func(folio->search_frame, abort_search_proc, SIGURG,  NOTIFY_ASYNC);
	
	if (!already_interposed) {
	    notify_interpose_event_func(base_frame, base_frame_event_proc, NOTIFY_SAFE);
	    already_interposed = TRUE;
	}
	
	(void) window_fit(panel);
	(void) window_fit(folio->search_frame);
	(void) textsw_set_pop_up_location(base_frame, folio->search_frame);
	(void) window_set(folio->search_frame, FRAME_LABEL, label, WIN_SHOW, TRUE, 0);
	
}	
	
