#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_field.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Procedures to do field matching in text subwindows.
 */

#include <string.h>
#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <suntool/ev_impl.h>
#include <sys/time.h>
#include <suntool/panel.h>
#include <suntool/wmgr.h>
#include <suntool/window.h>
#include <suntool/frame.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_screen.h>
#define    ONE_FIELD_LENGTH     2
#define    MAX_SYMBOLS		8
#define    NUM_OF_COL		2
#define    MY_BUF_SIZE		1024	
#define	   MAX_PANEL_ITEMS      6		

static
void textsw_get_match_symbol();

static  char    *match_table[NUM_OF_COL][MAX_SYMBOLS] =
			 {{"{", "(", "\"", "'", "`", "[", "|>", "/*", },
			  {"}", ")", "\"", "'", "`", "]", "<|", "*/", }};
			  
typedef enum {          
    BACKWARD_ITEM		= 0,
    ENCLOSE_ITEM		= 1,
    FORWARD_ITEM		= 2,
    BLINK_PARENT_ITEM		= 4,
    DONE_ITEM			= 3,
    CHOICE_ITEM			= 5
    
} Panel_item_enum;
			  
			  
static Panel_item	panel_items[MAX_PANEL_ITEMS];
static Frame		match_frame; /* This is only use within base_frame_event_proc() and textsw_create_match_frame() */
  

pkg_private int
textsw_begin_match_field(view)
	register Textsw_view	 view;
{
	textsw_begin_function(view, TXTSW_FUNC_FIELD);
	(void) textsw_inform_seln_svc(FOLIO_FOR_VIEW(view),
				      TXTSW_FUNC_FIELD, TRUE);
}

pkg_private int
textsw_end_match_field(view, event_code, x, y)
	register Textsw_view	view;
	int			x, y;
{
	pkg_private int		textsw_match_selection_and_normalize();
	register Textsw_folio	folio= FOLIO_FOR_VIEW(view);
	unsigned		field_flag = (event_code == TXTSW_NEXT_FIELD) ? TEXTSW_FIELD_FORWARD : TEXTSW_FIELD_BACKWARD;
	char			*start_marker;

	(void) textsw_inform_seln_svc(folio, TXTSW_FUNC_FIELD, FALSE);
	if ((folio->func_state & TXTSW_FUNC_FIELD) == 0)
	    return(ES_INFINITY);
	if ((folio->func_state & TXTSW_FUNC_EXECUTE) == 0)
	    goto Done;
	start_marker = (field_flag == TEXTSW_FIELD_FORWARD) ? "|>" : "<|";
	(void) textsw_match_selection_and_normalize(view, start_marker, field_flag);
Done:
	textsw_end_function(view, TXTSW_FUNC_FIELD);
	return(0);
}


static int
check_selection(buf, buf_len, first, last_plus_one,
		marker1, marker1_len, field_flag) 
	Es_index	*first, *last_plus_one;
	char		*buf, *marker1;
	unsigned	buf_len, marker1_len;	
	unsigned	field_flag;
	
	
{
	int		 do_search;
	    	
	if ((field_flag == TEXTSW_FIELD_FORWARD) ||
	    (field_flag == TEXTSW_DELIMITER_FORWARD)) {
	     if  (buf_len  >= marker1_len) {
	         if (strncmp(buf, marker1, marker1_len) == 0) {
		     
		     if (buf_len >= (marker1_len * 2)) { /* Assuming open and close markers have the same size */
	                 char		 marker2[3];
			 int		 marker2_len;
			 unsigned        direction;
			 static	void	textsw_get_match_symbol();
			 
	                 buf = buf + (buf_len - marker1_len);
	                 (void) textsw_get_match_symbol(marker1, marker1_len, 
	                                                marker2, &marker2_len, &direction);
	                 if ((strncmp(buf, marker2, marker2_len) == 0) ||
	                     (buf_len >= (MY_BUF_SIZE-1))) { /* Buffer overflow case */
	                     if (*first  == *last_plus_one) {
	                         *first =  (*first - buf_len);
	                     }
	                     *first = *first + marker1_len;
	                     do_search = TRUE;
	                 }
	             }
	          } else {
	             do_search = TRUE;
	         }
	     } else {
	         do_search = TRUE;
	     }
	} else if ((field_flag & TEXTSW_FIELD_BACKWARD)  ||
		   (field_flag & TEXTSW_DELIMITER_BACKWARD)) {	   	
	           if  (buf_len  >= marker1_len) {
	               char		*temp;
	               
		       temp = buf + (buf_len - marker1_len);
	               if (strncmp(temp, marker1, marker1_len) == 0) {
	                  if (buf_len >= (marker1_len*2)) {
	                     char		 marker2[3];
			     int		 marker2_len;
			     unsigned           direction;
			     static void	textsw_get_match_symbol();	
			     
			     (void) textsw_get_match_symbol(marker1, marker1_len, 
	                                                marker2, &marker2_len, &direction);	             
	                     if (strncmp(buf, marker2, marker2_len) == 0)  { 
	                         *last_plus_one = *last_plus_one - marker2_len;
	                         *first = *last_plus_one;
	                         do_search = TRUE;
	                 }
	             }
	         } else {
	             if (buf_len >= (MY_BUF_SIZE-1)) {/* Buffer overflow case */
	                 *last_plus_one = *last_plus_one - marker1_len;
	                 *first = *last_plus_one;
	             }
	             do_search = TRUE;
	         }
	     } else {
	         do_search  = TRUE;
	     }
	     
	}
	return(do_search);
}

static Es_index
get_start_position(folio, first, last_plus_one,
		   symbol1, symbol1_len, 
		   symbol2, symbol2_len, field_flag, do_search) 
        Textsw_folio	 folio;
	Es_index	*first, *last_plus_one;
	char		*symbol1, *symbol2;
	unsigned	symbol1_len, symbol2_len;
	unsigned	 field_flag;
	int		 do_search;

{
    	Es_index 	start_at = ES_CANNOT_SET;
    	
    	
    	unsigned	 direction = ((field_flag == TEXTSW_FIELD_FORWARD) || 
	  			      (field_flag == TEXTSW_DELIMITER_FORWARD)) ? 
	  			       EV_FIND_DEFAULT : EV_FIND_BACKWARD;
    	
    	if (do_search) {
	    (void) textsw_find_pattern(folio, first, last_plus_one,
				     symbol1, symbol1_len, direction);
	}
				     
	switch  (field_flag) {
	    case TEXTSW_NOT_A_FIELD:
	    case TEXTSW_FIELD_FORWARD:
	    case TEXTSW_DELIMITER_FORWARD:
	        start_at = *first;
	        break;
	    case TEXTSW_FIELD_ENCLOSE:
	    case TEXTSW_DELIMITER_ENCLOSE: {
		unsigned	dummy;
		
		if (symbol2_len == 0)	
		    textsw_get_match_symbol(symbol1, symbol1_len,
				            symbol2, &symbol2_len, &dummy);	
				        
		 start_at = ev_find_enclose_end_marker (folio->views->esh, 
		                                        symbol1, symbol1_len, 
		                                        symbol2, symbol2_len, 
		                                        *first);
	        break;
	    }
	    case TEXTSW_FIELD_BACKWARD:
	    case TEXTSW_DELIMITER_BACKWARD:{
	         start_at = ((*first == ES_CANNOT_SET)
			    ? ES_CANNOT_SET : *last_plus_one);
	        break;
	    }
       }
	
       return(start_at);
}

static void 
textsw_get_match_symbol(buf, buf_len, match_buf, match_buf_len, direction) 
	char		*buf, *match_buf;
	int		buf_len; 
	int		*match_buf_len;
	unsigned	*direction;
{
	int		i, j, index;
	
	*match_buf_len = 0;
	*direction = EV_FIND_DEFAULT;
	match_buf[0] = NULL;
	
	for (i = 0; i < NUM_OF_COL; i++) {
	    for (j = 0; j < MAX_SYMBOLS; j++) {
	        if (strncmp(buf, match_table[i][j], buf_len) == 0) {
	            index = ((i == 0) ? 1 : 0);
	            strcpy(match_buf, match_table[index][j]);
	            *match_buf_len = strlen(match_buf);
	            if (index == 0)
	                *direction = EV_FIND_BACKWARD;
	            return;
	        }
	    }
	}
}


pkg_private Es_index
textsw_match_same_marker(folio, marker, marker_len, start_pos, direction)
	Textsw_folio	 folio;
	char		*marker;
	int		 marker_len;
	Es_index	 start_pos;
	unsigned	 direction;	
{
        Es_index	first, last_plus_one;
        Es_index	result_pos;
        int		plus_or_minus_one = (direction == EV_FIND_BACKWARD) ? -1 : 1;
        
        first = last_plus_one = start_pos + plus_or_minus_one;
	  			      	
	(void) textsw_find_pattern(folio, &first, &last_plus_one,
				     marker, marker_len, direction);
				     
	result_pos = (direction == EV_FIND_BACKWARD) ? last_plus_one : first;			     
	if (result_pos == start_pos)  
	    result_pos = ES_CANNOT_SET;
	else if (result_pos != ES_CANNOT_SET)
	    result_pos = result_pos + plus_or_minus_one;
	    			     
	return(result_pos);	
}

/* Caller must set *first to be position at which to start the search. */
pkg_private int
textsw_match_field(textsw, first, last_plus_one, symbol1, symbol1_len,
		   symbol2, symbol2_len, field_flag, do_search)
	Textsw_folio	 textsw;
	Es_index	*first, *last_plus_one;
	char		*symbol1, *symbol2;
	int		 symbol1_len, symbol2_len;
	unsigned	 field_flag;
	int		 do_search;	/* If TRUE, is called by textsw_match_bytes */
{
	
	Es_handle	 esh = textsw->views->esh;
	Es_index	 start_at = *first;
	Es_index	 result_pos;
	unsigned	 direction = ((field_flag == TEXTSW_FIELD_FORWARD) || 
	  			      (field_flag == TEXTSW_DELIMITER_FORWARD)) ? 
	  			       EV_FIND_DEFAULT : EV_FIND_BACKWARD;
	

	start_at = get_start_position (textsw, first, last_plus_one,
					symbol1, symbol1_len,
					symbol2, symbol2_len, field_flag, do_search);
					
	if ((symbol1_len == 0)  || (start_at == ES_CANNOT_SET)) {
	    *first = ES_CANNOT_SET;
	    return;
	}
	
	if (symbol2_len == 0)
	   textsw_get_match_symbol(symbol1, symbol1_len,
				   symbol2, &symbol2_len, &direction);
				   
	if ((symbol2_len == 0) || (symbol2_len != symbol1_len)) {
	    *first = ES_CANNOT_SET;
	    return;
	}
	
	if ((direction == EV_FIND_BACKWARD) && 
	    (field_flag == TEXTSW_NOT_A_FIELD)) {
	    start_at = *last_plus_one;
	}
	
	if (strncmp(symbol1, symbol2, symbol1_len) == 0)  {
	
	    direction = ((field_flag == TEXTSW_NOT_A_FIELD) || 
	   		(field_flag == TEXTSW_FIELD_FORWARD) ||
	  		(field_flag == TEXTSW_DELIMITER_FORWARD)) ? 
	  		       EV_FIND_DEFAULT : EV_FIND_BACKWARD;
	    result_pos = textsw_match_same_marker(textsw, symbol1, symbol1_len, start_at, direction);		       
	} else
	   result_pos = ev_match_field_in_esh(esh, symbol1, symbol1_len,
					   symbol2, symbol2_len, start_at,
					   direction);
	
	if ((field_flag == TEXTSW_FIELD_FORWARD) ||
	    (field_flag == TEXTSW_DELIMITER_FORWARD) ||
	    ((field_flag == TEXTSW_NOT_A_FIELD) && (direction != EV_FIND_BACKWARD))) {

	    *first = start_at;
	    *last_plus_one = ((result_pos >= start_at)
			     ? result_pos : ES_CANNOT_SET);			     	 
	}  else {
	    *first = ((result_pos <= start_at) ? result_pos : ES_CANNOT_SET);
	    *last_plus_one = start_at;

	}
	
}

/* ARGSUSED */
pkg_private int
textsw_match_field_and_normalize(
    view, first, last_plus_one, buf1, buf_len1, field_flag, do_search)
	Textsw_view		 view;
	register Es_index	*first, *last_plus_one;
	char			*buf1;
	int			 buf_len1;
	unsigned		 field_flag;
	int			 do_search;
	
{

	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	char			 buf2[MY_BUF_SIZE];
	int			 buf_len2 = 0;
	register Es_index	 ro_bdry;
	register int		 want_pending_delete;
	int			 matched = FALSE;
	Es_index		 save_first = *first;
	Es_index		 save_last_plus_one = *last_plus_one;

	(void) textsw_match_field(folio, first, last_plus_one,
				  buf1, buf_len1, buf2, buf_len2,
				  field_flag, do_search);
				  
	if (((*first == save_first) && (*last_plus_one == save_last_plus_one)) ||
	    ((*first == ES_CANNOT_SET) || (*last_plus_one == ES_CANNOT_SET))) {
	    (void) window_bell(WINDOW_FROM_VIEW(view));
	} else {
	    /*   Unfortunately, textsw_possibly_normalize_and_set_selection
	     * will not honor request for pending-delete, so we call
	     * textsw_set_selection explicitly in such cases.
	     *   WARNING!  However, we don't allow pending-delete if we
	     * overlap the read-only portion of the window.
	     */
	    want_pending_delete = ((field_flag == TEXTSW_FIELD_FORWARD) ||
	    			  (field_flag == TEXTSW_FIELD_BACKWARD) ||
	    			  (field_flag == TEXTSW_FIELD_ENCLOSE));
	    			  
	    if (want_pending_delete) {
		ro_bdry = TXTSW_IS_READ_ONLY(folio) ? *last_plus_one
		      : textsw_read_only_boundary_is_at(folio);
		if (*last_plus_one <= ro_bdry) {
		    want_pending_delete = FALSE;
		}
	    }
	    (void) textsw_possibly_normalize_and_set_selection(
		VIEW_REP_TO_ABS(view), *first, *last_plus_one,
		((want_pending_delete) ? 0 : EV_SEL_PRIMARY));
	    if (want_pending_delete) {
		(void) textsw_set_selection(
		    VIEW_REP_TO_ABS(view), *first, *last_plus_one,
		    (EV_SEL_PRIMARY | EV_SEL_PD_PRIMARY));
	    }
	    (void) textsw_set_insert(folio, *last_plus_one);
	    textsw_record_match(folio, field_flag, buf1); 
	    matched = TRUE;
	}
	return(matched);
}



pkg_private int
textsw_match_selection_and_normalize(view, start_marker, field_flag)
	register Textsw_view	 view;
	register unsigned	 field_flag;
	char			 *start_marker;
{
	register Textsw_folio	 textsw = FOLIO_FOR_VIEW(view);
	pkg_private int		 textsw_get_selection();	
	Es_index		 first, last_plus_one;
	char			 buf[MY_BUF_SIZE];
	int			 str_length = MY_BUF_SIZE;
	int			 do_search = TRUE;
	
	/* 
	 * This procedure should only be called if field flag is set to
	 * TEXTSW_FIELD_FORWARD, TEXTSW_FIELD_BACKWARD, 
	 * TEXTSW_DELIMITER_FORWARD, TEXTSW_DELIMITER_BACKWARD.
	 */ 
	
	if (textsw_get_selection(view, &first, &last_plus_one, NULL, 0)) {
	    if ((last_plus_one-first) < MY_BUF_SIZE)
               str_length =  (last_plus_one-first);
         
            textsw_get(VIEW_REP_TO_ABS(view), TEXTSW_CONTENTS, first, buf, str_length);
        
            if (str_length == MY_BUF_SIZE) 
               str_length--;
                   
            buf[str_length] = NULL;
            
            if (field_flag == TEXTSW_NOT_A_FIELD) {
	        if (str_length > ONE_FIELD_LENGTH) {
	            (void) window_bell(WINDOW_FROM_VIEW(view));
	            return(FALSE);			/* Not a vaild marker size */
	        }
	        start_marker = buf;
	        do_search = FALSE;
	    } else {
	        do_search = check_selection(buf, str_length, &first, &last_plus_one,
	                             start_marker, strlen(start_marker), field_flag);
	    }
	} else { 
	     if (field_flag == TEXTSW_NOT_A_FIELD) {	/* Matching must have a selection */
	        (void) window_bell(WINDOW_FROM_VIEW(view));
		return(FALSE);
	    }	
	    first = last_plus_one = ev_get_insert(textsw->views);
	}
   		
		
	return (textsw_match_field_and_normalize(
	    view, &first, &last_plus_one,
	    start_marker, strlen(start_marker), field_flag, do_search));
	    
}

static void
my_destroy_proc (frame)
	Frame		frame;
{
	Textsw_view	view = (Textsw_view) window_get(frame, WIN_CLIENT_DATA, 0);
	Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	
	window_set(frame, FRAME_NO_CONFIRM, TRUE, 0);
	window_destroy(frame);
	
	match_frame = folio->match_frame = NULL;
}

static void
select_text_backward(view, panel)
	Textsw_view	view;
	Panel		panel;
{
	int		value = (int)panel_get(
				   panel_items[(int)CHOICE_ITEM], PANEL_VALUE);
	switch (value) {
	    case 0 : 
	         (void) textsw_match_selection_and_normalize(view, ")", TEXTSW_DELIMITER_BACKWARD);
	        break;
	    case 1 : 
	        (void) textsw_match_selection_and_normalize(view, "\"", TEXTSW_DELIMITER_BACKWARD);
	        break;	    
	    case 2 : 
	        (void) textsw_match_selection_and_normalize(view, "'", TEXTSW_DELIMITER_BACKWARD);
	        break;
	    case 3 : 
	        (void) textsw_match_selection_and_normalize(view, "`", TEXTSW_DELIMITER_BACKWARD);
	        break;
	    case 4 : 
	        (void) textsw_match_selection_and_normalize(view, "}", TEXTSW_DELIMITER_BACKWARD);
	        break;
	    case 5 : 
	        (void) textsw_match_selection_and_normalize(view, "]", TEXTSW_DELIMITER_BACKWARD);
	        break;	    
	    case 6 : 
	        (void) textsw_match_selection_and_normalize(view, "<|", TEXTSW_DELIMITER_BACKWARD);
	        break;
	    case 7 : 
	        (void) textsw_match_selection_and_normalize(view, "*/", TEXTSW_DELIMITER_BACKWARD);
	        break;

	}   			   
}

static void
select_enclose_text(view, panel)
	Textsw_view	view;
	Panel		panel;
{
	Textsw_folio    folio= FOLIO_FOR_VIEW(view);
	int		value = (int)panel_get(
				   panel_items[(int)CHOICE_ITEM], PANEL_VALUE);
	int		first, last_plus_one;	
	
	first = last_plus_one = ev_get_insert(folio->views);		   
	switch (value) {
	    case 0 : 
	         
                 (void) textsw_match_field_and_normalize(view, &first, &last_plus_one, ")", 1, TEXTSW_DELIMITER_ENCLOSE, FALSE);
	        break;

	    case 1 : 
	        (void) textsw_match_field_and_normalize(view, &first, &last_plus_one, "\"", 1, TEXTSW_DELIMITER_ENCLOSE, FALSE);
	        break;	    
	    case 2 : 
	        (void) textsw_match_field_and_normalize(view, &first, &last_plus_one, "'", 1, TEXTSW_DELIMITER_ENCLOSE, FALSE);
	        break;
	    case 3 : 
	        (void) textsw_match_field_and_normalize(view, &first, &last_plus_one, "`", 1, TEXTSW_DELIMITER_ENCLOSE, FALSE);
	        break;
	    case 4 : 
	        (void) textsw_match_field_and_normalize(view, &first, &last_plus_one, "}", 1, TEXTSW_DELIMITER_ENCLOSE, FALSE);
	        break;
	    case 5 : 
	        (void) textsw_match_field_and_normalize(view, &first, &last_plus_one, "]", 1, TEXTSW_DELIMITER_ENCLOSE, FALSE);
	        break;	    
	    case 6 : 
	        (void) textsw_match_field_and_normalize(view, &first, &last_plus_one, "<|", 2, TEXTSW_DELIMITER_ENCLOSE, FALSE);
	        break;
	    case 7 : 
	        (void) textsw_match_field_and_normalize(view, &first, &last_plus_one, "*/", 2, TEXTSW_DELIMITER_ENCLOSE, FALSE);
	        break;

	}   			   
}

static void
select_text_forward(view, panel)
	Textsw_view	view;
	Panel		panel;
{
	int		value = (int)panel_get(
				   panel_items[(int)CHOICE_ITEM], PANEL_VALUE);
	switch (value) {
	    case 0 : 
	         (void) textsw_match_selection_and_normalize(view, "(", TEXTSW_DELIMITER_FORWARD);
	        break;
	    case 1 : 
	        (void) textsw_match_selection_and_normalize(view, "\"", TEXTSW_DELIMITER_FORWARD);
	        break;	    
	    case 2 : 
	        (void) textsw_match_selection_and_normalize(view, "'", TEXTSW_DELIMITER_FORWARD);
	        break;
	    case 3 : 
	        (void) textsw_match_selection_and_normalize(view, "`", TEXTSW_DELIMITER_FORWARD);
	        break;
	    case 4 : 
	        (void) textsw_match_selection_and_normalize(view, "{", TEXTSW_DELIMITER_FORWARD);
	        break;
	    case 5 : 
	        (void) textsw_match_selection_and_normalize(view, "[", TEXTSW_DELIMITER_FORWARD);
	        break;	    
	    case 6 : 
	        (void) textsw_match_selection_and_normalize(view, "|>", TEXTSW_DELIMITER_FORWARD);
	        break;
	    case 7 : 
	        (void) textsw_match_selection_and_normalize(view, "/*", TEXTSW_DELIMITER_FORWARD);
	        break;

	}   			   
}

	

static void
cmd_proc(item, event)
	Panel_item	item;
    	Event		*event;
{
	pkg_private Textsw_view   view_from_panel_item();
	Textsw_view	view = view_from_panel_item(item);
	Textsw_folio 	folio = FOLIO_FOR_VIEW(view);
	Panel		panel = panel_get(item, PANEL_PARENT_PANEL, 0);
	
	if (item == panel_items[(int)BACKWARD_ITEM]) {
	    select_text_backward(view, panel);
	} else if (item == panel_items[(int)ENCLOSE_ITEM]) {
	    select_enclose_text(view, panel);
	} else if (item == panel_items[(int)FORWARD_ITEM]) {
	    select_text_forward(view, panel);
	} else if (item == panel_items[(int)BLINK_PARENT_ITEM]) {
	    (void) blink_window(VIEW_REP_TO_ABS(view), 3);
	} else if (item == panel_items[(int)DONE_ITEM]) {
	    (void) my_destroy_proc(folio->match_frame);
	    
	} 	
}

static void
create_match_item(panel, view) 
  	Panel		panel;
  	Textsw_view	view;
{

	char		*curly_bracket = " { }  ";
	char		*parenthesis = 	" ( )  ";
	char		*double_quote =  " \" \"  ";
	char		*single_quote =  " ' '  ";
	char		*back_qoute = " ` `  ";
	char		*square_bracket = " [ ]  ";
	char		*field_marker = " |> <|  ";
	char		*comment =  " /* */  ";
	
	char		*done = "Done";
	char		*backward = "Backward";
	char		*enclose =  "Expand";
	char		*forward =  "Forward";
	char		*blink_parent =  "Blink Owner";

        
        panel_items[(int)BACKWARD_ITEM] = panel_create_item(panel, PANEL_BUTTON,
            PANEL_LABEL_X,                 ATTR_COL(0),
            PANEL_LABEL_Y,                 ATTR_ROW(0),
            PANEL_LABEL_IMAGE,
                        panel_button_image(panel, backward, 8, NULL),
            PANEL_NOTIFY_PROC,		   cmd_proc,
	    HELP_DATA,			   "textsw:fieldbackward",
	    0); 
            
        panel_items[(int)ENCLOSE_ITEM] =  panel_create_item(panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,
                        panel_button_image(panel, enclose, 8, NULL),
            PANEL_NOTIFY_PROC,  	cmd_proc,
 	    HELP_DATA,			   "textsw:fieldforward",
            0); 
            
        panel_items[(int)FORWARD_ITEM] =  panel_create_item(panel, PANEL_BUTTON,
            
            PANEL_LABEL_IMAGE,
                        panel_button_image(panel, forward, 8, NULL),
            PANEL_NOTIFY_PROC, 		   cmd_proc,
            0); 

         panel_items[(int)BLINK_PARENT_ITEM] =  panel_create_item(panel,PANEL_BUTTON,
            PANEL_LABEL_X,                 ATTR_COL(58),
            PANEL_LABEL_Y,                 ATTR_ROW(0),
            PANEL_LABEL_IMAGE,
                        panel_button_image(panel, blink_parent, 0, NULL),
            PANEL_NOTIFY_PROC, 		   cmd_proc,
 	    HELP_DATA,			   "textsw:fieldblink",
            0);
                        
         panel_items[(int)DONE_ITEM] =   panel_create_item(panel, PANEL_BUTTON,
            PANEL_LABEL_Y,                 ATTR_ROW(0),           
            PANEL_LABEL_IMAGE,
                        panel_button_image(panel, done, 7, NULL),
            PANEL_NOTIFY_PROC,		   cmd_proc,
	    HELP_DATA,			   "textsw:fielddone",
	    0);
	       
	panel_items[(int)CHOICE_ITEM] = panel_create_item(panel, PANEL_CHOICE,
            PANEL_LABEL_X,                 ATTR_COL(0),
            PANEL_LABEL_Y,                 ATTR_ROW(1),           
            PANEL_CHOICE_STRINGS, 	parenthesis, double_quote, 
                        		single_quote, back_qoute, 
                        		curly_bracket, square_bracket, 
            			  	field_marker, comment, 0,
	    HELP_DATA,			"textsw:fieldchoice",
            0);	       
                                       
}

extern Panel
textsw_create_match_panel(frame, view)
	Frame		frame;
	Textsw_view	view;
{
	Panel		panel;
	
	panel = (Panel)(LINT_CAST(window_create((Window)frame, PANEL,
			  HELP_DATA,	"textsw:fieldpanel",
	    		  0)));
	create_match_item(panel, view);  
	
	return(panel);  
}


static Notify_value
base_frame_event_proc(base_frame, event, arg, type)
    Frame base_frame;
    Event *event;
    Notify_arg arg;
    Notify_event_type type;
{
    Notify_value value = notify_next_event_func(base_frame, event, arg, type);

    switch (event_action(event)) {
      case WIN_REPAINT:
      case TXTSW_OPEN:
      case MS_RIGHT:
	if (window_get(base_frame, FRAME_CLOSED)) {
	     if (window_get(match_frame, WIN_SHOW))
	         window_set(match_frame, WIN_SHOW, FALSE, 0);
	} else {
	    if (!window_get(match_frame, WIN_SHOW))
	         window_set(match_frame, WIN_SHOW, TRUE, 0);
	}
	break;
    }
    return value;
}				
	
extern void
textsw_create_match_frame(view)
	Textsw_view	view;
{	
	
	Textsw_folio    folio= FOLIO_FOR_VIEW(view);
	Frame		base_frame = (Frame) window_get(view, WIN_OWNER); 
	Panel		panel;
	char		*label = "Find Marked Text";
	static int	already_interposed;
	
	if (!base_frame) {
	    (void)textsw_alert_for_old_interface(view);
	    return;
	}
	
	if (match_frame) {
	    register int	match_frame_fd = (int) window_get(match_frame, WIN_FD, 0);
	    register int	root_fd;
	    
   	     folio->match_frame = match_frame;  /* In case the popup is for a different textsw */
   	     window_set(folio->match_frame, WIN_CLIENT_DATA, view, 0);
   	     
	    if (win_getlink(match_frame_fd, WL_COVERING) == WIN_NULLLINK) {
	       (void) blink_window(folio->match_frame, 5);
	    } else {
	        root_fd = rootfd_for_toolfd(match_frame_fd);
	        wmgr_top(match_frame_fd, root_fd);
	        (void)close (root_fd);
	    }

	    return;
	    }
	    
	  
	match_frame = folio->match_frame = window_create(base_frame, FRAME,
			FRAME_SHOW_LABEL, TRUE,
			FRAME_DONE_PROC, my_destroy_proc,
			0);
	window_set(folio->match_frame, WIN_CLIENT_DATA, view, 0);		
	panel = textsw_create_match_panel(folio->match_frame, view);
	
	if (!already_interposed)
	    notify_interpose_event_func(base_frame, base_frame_event_proc, NOTIFY_SAFE);
	else
	    already_interposed = TRUE;
	    
	(void) window_fit(panel);
	(void) window_fit(folio->match_frame);
	textsw_set_pop_up_location (base_frame, folio->match_frame);
	(void) window_set(folio->match_frame, FRAME_LABEL, label, WIN_SHOW, TRUE, 0);

}


/*
 *  If the pattern is matched, return the index where it is found,
 *  else return -1.
 */
extern int
textsw_match_bytes(abstract, first, last_plus_one, start_sym, start_sym_len,
                   end_sym, end_sym_len, field_flag)
	Textsw		 abstract;	/* find in this textsw */
	Textsw_index	*first;		/* start here, return start of 
					 * found pattern here */
	Textsw_index	*last_plus_one;	/* return end of found pattern */
	char		*start_sym, *end_sym;		/* begin and end pattern */
	int		 start_sym_len, end_sym_len;	/* patterns length */
	unsigned	 field_flag;
{
	Textsw_folio	folio = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));
	int		save_first = *first;
	int		save_last_plus_one = *last_plus_one;

	
	if ((field_flag == TEXTSW_DELIMITER_FORWARD) || 
	    (field_flag == TEXTSW_FIELD_FORWARD)) {
	    (void) textsw_match_field(folio, first, last_plus_one, 
	                          start_sym, start_sym_len,
			          end_sym,   end_sym_len, field_flag, TRUE);
	} else {
	    (void) textsw_match_field(folio, first, last_plus_one, 
	    		          end_sym,   end_sym_len,
	                          start_sym, start_sym_len, field_flag, 
	                          ((field_flag == TEXTSW_DELIMITER_BACKWARD) ||
	                          (field_flag == TEXTSW_FIELD_BACKWARD)));
	}		          
			    
	if ((*first == ES_CANNOT_SET) || (*last_plus_one == ES_CANNOT_SET)) {
	    *first = save_first;
	    *last_plus_one = save_last_plus_one;
	    return -1;
	} else {
	    return *first;
	}
}

