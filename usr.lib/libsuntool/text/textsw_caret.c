#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_caret.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Routines for moving textsw caret.
 */
 
 
#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <suntool/ev_impl.h>


extern void	textsw_record_caret_motion();

static void
make_insert_visible(view, visible_status, old_insert, new_insert) 
  	Textsw_view	view;
  	unsigned	visible_status;  /* For old_insert */
  	Es_index	old_insert, new_insert;
{
	Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	register Ev_chain	chain = folio->views;
	register Ev_handle	ev_handle = view->e_view;
	register unsigned	normalize_flag = (TXTSW_NI_NOT_IF_IN_VIEW | TXTSW_NI_MARK);
	int			lower_context, upper_context, scroll_lines;
	

	
	/*  
	 *  This procedure will only do display if the new_insert
	 *  position is not in view.
	 *  It will scroll, if the old_insert position is in view, but not
	 *  new_insert.
	 *  It will repaint the entire view, if the old_insert and new_insert 
	 *  position are not in view.  
	 */
	lower_context = (int)LINT_CAST(ev_get(chain, EV_CHAIN_LOWER_CONTEXT));
	upper_context = (int)LINT_CAST(ev_get(chain, EV_CHAIN_UPPER_CONTEXT));		
	if (visible_status == EV_XY_VISIBLE) {
	    int			lines_in_view = textsw_screen_line_count(VIEW_REP_TO_ABS(view));
	
	    if (old_insert > new_insert) /* scroll backward */
	       scroll_lines = ((upper_context > 0) && (lines_in_view >= upper_context)) ? -upper_context : -1;   
	    else
	       scroll_lines = ((lower_context > 0) && (lines_in_view >= lower_context)) ? lower_context : 1;
	    
	    while (!ev_insert_visible_in_view(ev_handle)) {
	        ev_scroll_lines(ev_handle, scroll_lines, FALSE);
	    } 
	    textsw_update_scrollbars(folio, view);   
	} else {
	    if (old_insert <= new_insert) {
	        normalize_flag |= TXTSW_NI_AT_BOTTOM;
	        upper_context = 0;
	    }
	        lower_context = 0;
	     
	          
	    textsw_normalize_internal(view, new_insert, new_insert, 
	        upper_context, lower_context, normalize_flag);
	}
}
  			
pkg_private Es_index
textsw_move_backward_a_word(view, pos) 
  	register Textsw_view		view;
  	Es_index 			pos;
{
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	register Ev_chain	chain = folio->views;
	unsigned		span_flag = (EI_SPAN_WORD | EI_SPAN_LEFT_ONLY);
	Es_index		first, last_plus_one;
	unsigned		span_result = EI_SPAN_NOT_IN_CLASS;
	
	
	if (pos == 0)           /* This is already at the beginning of the file */
	    return (ES_CANNOT_SET);
	    
	while ((pos != 0) && (pos != ES_CANNOT_SET) && 
	       (span_result & EI_SPAN_NOT_IN_CLASS)) {
	    span_result = ev_span(chain, pos, &first, &last_plus_one, span_flag);
	    pos = ((pos == first) ? pos-- : first);
	}
	
	return(pos);
}


pkg_private Es_index
textsw_move_down_a_line(view, pos, file_length, lt_index, rect)
  	register Textsw_view		view;
  	Es_index 			pos, file_length;
  	int				lt_index;
  	struct rect			rect;
  	
{
	register Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	register Ev_handle		ev_handle = view->e_view;
	int				line_height =  ei_line_height
	 				     (ev_handle->view_chain->eih);
	int				new_x, new_y;
	Es_index			new_pos;
  	int				lower_context, scroll_lines;
	Ev_impl_line_seq		line_seq = (Ev_impl_line_seq)
					    ev_handle->line_table.seq;
	pkg_private int			textsw_get_recorded_x();

	
	if ((pos >= file_length) ||
	    (line_seq[lt_index+1].pos == ES_INFINITY) ||
	    (line_seq[lt_index+1].pos == file_length))
	    return(ES_CANNOT_SET);
	
	
	if (pos >= line_seq[ev_handle->line_table.last_plus_one-2].pos) {
	   int			lines_in_view = textsw_screen_line_count(VIEW_REP_TO_ABS(view));
	   
	   lower_context = (int)LINT_CAST(ev_get(folio->views, 
	    			  EV_CHAIN_LOWER_CONTEXT));
	   scroll_lines = ((lower_context > 0)  && (lines_in_view > lower_context)) ? lower_context + 1 : 1;
	   ev_scroll_lines(ev_handle, scroll_lines, FALSE);
	   new_y = rect.r_top - (line_height * (scroll_lines - 1));
	   textsw_update_scrollbars(folio, view);
	} else
	    new_y = rect.r_top + line_height;
	    
	new_x = textsw_get_recorded_x(view);
	
	if (new_x < rect.r_left)
	    new_x = rect.r_left;
	    
	(void) textsw_record_caret_motion(folio, TXTSW_NEXT_LINE, new_x); 
	
	new_pos = ev_resolve_xy(ev_handle, new_x, new_y);  
	           
	return ((new_pos >= 0 && new_pos <= file_length) ? 
	           new_pos : ES_CANNOT_SET);	
}	

pkg_private Es_index
textsw_move_forward_a_word(view, pos, file_length) 
  	Textsw_view	view;
  	Es_index 	pos;
  	Es_index	file_length;
{
	Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	register Ev_chain	chain = folio->views;
	unsigned		span_flag = (EI_SPAN_WORD | EI_SPAN_RIGHT_ONLY);
	Es_index		first, last_plus_one;
	unsigned		span_result = EI_SPAN_NOT_IN_CLASS;
	
	if (pos >= file_length)
	    return(ES_CANNOT_SET);
	    
	/*  Get to the end of current word, and use that as start pos */
	(void) ev_span(chain, pos, &first, &last_plus_one, span_flag);
	
	pos = ((last_plus_one == file_length) ? ES_CANNOT_SET : last_plus_one);
	
	while ((pos != ES_CANNOT_SET) && (span_result & EI_SPAN_NOT_IN_CLASS)) {
	    span_result = ev_span(chain, pos, &first, &last_plus_one, 
	                          span_flag);
	    
	    if (pos == last_plus_one) 
		pos = ((pos == file_length) ? ES_CANNOT_SET : pos++);
	    else	
	        pos = last_plus_one;
	}
	
	    
	return ((pos == ES_CANNOT_SET) ? ES_CANNOT_SET : first);	
	
}	

pkg_private Es_index
textsw_move_to_word_end(view, pos, file_length) 
  	Textsw_view	view;
  	Es_index 	pos;
  	Es_index	file_length;
{
	Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	register Ev_chain	chain = folio->views;
	unsigned		span_flag = (EI_SPAN_WORD | EI_SPAN_RIGHT_ONLY);
	Es_index		first, last_plus_one;
	unsigned		span_result = EI_SPAN_NOT_IN_CLASS;
	
	if (pos >= file_length)
	    return(ES_CANNOT_SET);
	    
	while ((pos != ES_CANNOT_SET) && (span_result & EI_SPAN_NOT_IN_CLASS)) {
	    span_result = ev_span(chain, pos, &first, &last_plus_one, 
	    		          span_flag);
	        
	    if (pos == last_plus_one) 
	        pos = ((pos == file_length) ? ES_CANNOT_SET : pos++);
	    else	
	        pos = last_plus_one;
	}
	
			
	return(pos); 
}

pkg_private Es_index
textsw_move_left_a_character(view, pos) 
  	Textsw_view	view;
  	Es_index 	pos;
{
			
	return((pos == 0) ? ES_CANNOT_SET : --pos);
}

pkg_private Es_index
textsw_move_right_a_character(view, pos, file_length) 
  	Textsw_view	view;
  	Es_index 	pos;
  	Es_index	file_length;
{
	
	return((pos >= file_length) ? ES_CANNOT_SET : ++pos);
}


pkg_private Es_index
textsw_move_to_line_start(view, pos) 
  	Textsw_view	view;
  	Es_index	pos;
{
	Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	unsigned		span_flag = (EI_SPAN_LINE | EI_SPAN_LEFT_ONLY);
	Es_index		first, last_plus_one;
	
	if (pos == 0)
	    return(ES_CANNOT_SET);
	    
	(void)ev_span(folio->views, pos, &first, &last_plus_one, span_flag);
	
	if (pos == first) 
	    (void)ev_span(folio->views, --pos, &first, &last_plus_one, 
	                       span_flag);
	 
		
	return(first);
}



pkg_private Es_index
textsw_move_next_line_end(view, pos, file_length) 
  	Textsw_view	view;
  	Es_index	pos;
  	Es_index	file_length;
{
	Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	register Ev_chain	chain = folio->views;
	unsigned		span_flag = (EI_SPAN_LINE | EI_SPAN_RIGHT_ONLY);
	Es_index		first, last_plus_one;
	
	if (pos >= file_length)
	    return(ES_CANNOT_SET);
	    
	(void)ev_span(chain, pos, &first, &last_plus_one, span_flag);
	    
	if (pos == (last_plus_one - 1)) 
	    (void)ev_span(chain, ++pos, &first, &last_plus_one, span_flag);
	
	if (last_plus_one < file_length)
	    return(--last_plus_one);
	else {
	    char	buf[1];
	    
	    (void) textsw_get_contents(folio, --last_plus_one, buf, 1);
	    return((buf[0] == '\n') ? last_plus_one : file_length);
	}
	
	
}


pkg_private Es_index
textsw_move_next_line_start(view, pos, file_length) 
  	Textsw_view	view;
  	Es_index	pos;
  	Es_index	file_length;
{
	Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	unsigned	span_flag = (EI_SPAN_LINE | EI_SPAN_RIGHT_ONLY);
	Es_index	first, last_plus_one;
	
	if (pos >= file_length)
	    return(ES_CANNOT_SET);
	    
	(void)ev_span(folio->views, pos, &first, &last_plus_one, span_flag);
		
	return((last_plus_one == file_length) ? ES_CANNOT_SET : last_plus_one);
}


pkg_private Es_index
textsw_move_up_a_line(view, pos, file_length, lt_index, rect)
  	register Textsw_view		view;
  	Es_index			pos;
  	Es_index 			file_length;
  	int				lt_index;
  	struct rect			rect;
{
	register Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	register Ev_chain		chain = folio->views;
	register Ev_handle		ev_handle = view->e_view;
	int				line_height = ei_line_height(
				 	       ev_handle->view_chain->eih);
	int				new_x, new_y;
	Es_index			new_pos;
	int				upper_context, scroll_lines;
	Ev_impl_line_seq		line_seq = (Ev_impl_line_seq)
					    ev_handle->line_table.seq;
	pkg_private int			textsw_get_recorded_x();
	
	if ((pos == 0) || (line_seq[lt_index].pos == 0))
	    return(ES_CANNOT_SET);
	    	
	if (pos < line_seq[1].pos) {
	   int			lines_in_view = textsw_screen_line_count(VIEW_REP_TO_ABS(view));
	   
	   upper_context = (int)LINT_CAST(ev_get(chain, 
	    			  EV_CHAIN_UPPER_CONTEXT));
	    			  
	   scroll_lines = ((upper_context > 0) && (lines_in_view > upper_context)) ? upper_context + 1 : 1;
	   
	   ev_scroll_lines(ev_handle, -scroll_lines, FALSE);
	   textsw_update_scrollbars(folio, view);
	   new_y = rect.r_top + (line_height * (scroll_lines - 1));
	} else
	   new_y = rect.r_top - line_height;
	
	new_x = textsw_get_recorded_x(view);  
	
	if (new_x < rect.r_left)
	    new_x = rect.r_left;
	     
	(void) textsw_record_caret_motion(folio, TXTSW_PREVIOUS_LINE, new_x); 
	
	new_pos = ev_resolve_xy(ev_handle, new_x, new_y); 
	
	return ((new_pos >= 0 && new_pos <= file_length) ?
	           new_pos : ES_CANNOT_SET);    
}

pkg_private void
textsw_move_caret(view, direction)
	Textsw_view			view;
	Textsw_Caret_Direction		direction;	
	
{
	int					ok = TRUE;
	Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	register Ev_chain	chain = folio->views;	
	Es_index 			old_pos, pos;
	unsigned			visible_status;
	Es_index			file_length = es_get_length(chain->esh);
	int					lt_index;
	struct	rect		rect;
	Es_index		    first, last_plus_one;
	
	if (TXTSW_IS_READ_ONLY(folio) || (file_length == 0)) {
	    (void) window_bell(WINDOW_FROM_VIEW(view));
	    return;
	}
	
	textsw_flush_caches(view, TFC_STD);
	textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
			       (caddr_t)TEXTSW_INFINITY-1);
	pos = ES_CANNOT_SET;		       
	old_pos = ev_get_insert(chain);	
	visible_status = ev_xy_in_view(view->e_view, old_pos, &lt_index, &rect);
		       
	switch	(direction) {
	    case TXTSW_CHAR_BACKWARD:
	    	pos = textsw_move_left_a_character(view, old_pos);
	        break;
	    case TXTSW_CHAR_FORWARD:
	    	pos = textsw_move_right_a_character(view, old_pos, file_length);
	        break;
	    case TXTSW_DOCUMENT_END:
	                         	          
	        if ((old_pos < file_length) || 
	            (visible_status != EV_XY_VISIBLE)) {
	            pos = file_length;
	            visible_status = EV_XY_BELOW;
	        } else 
	            pos = ES_CANNOT_SET;   
	                         	          
	        break;
	    case TXTSW_DOCUMENT_START:	                         	           
	        if ((old_pos > 0) || (visible_status != EV_XY_VISIBLE)) {
	            pos = 0;
	            visible_status = EV_XY_ABOVE;
	        } else 
	            pos = ES_CANNOT_SET;    
	                         	            
	        break;
	    case TXTSW_LINE_END:
	    	pos = textsw_move_next_line_end(view, old_pos, file_length);
	        break;
	    case TXTSW_LINE_START:
	    	pos = textsw_move_to_line_start(view, old_pos);
	        break;  
	    case TXTSW_NEXT_LINE_START:
	    	pos = textsw_move_next_line_start(view, old_pos, file_length);
	        break;      
	    case TXTSW_NEXT_LINE: {
	        int      lower_context = (int)LINT_CAST(
	        		ev_get(chain, EV_CHAIN_LOWER_CONTEXT));
	        		
	    	if (visible_status != EV_XY_VISIBLE) {	    	    
	    	    textsw_normalize_internal(view, old_pos, old_pos,
	    	         0, lower_context + 1, 
	                 (TXTSW_NI_NOT_IF_IN_VIEW | TXTSW_NI_MARK | 
	                  TXTSW_NI_AT_BOTTOM));
	    	    visible_status = ev_xy_in_view(view->e_view, old_pos, 
	    	                                   &lt_index, &rect);
	    	}
	    	if (visible_status == EV_XY_VISIBLE)    
	    	    pos = textsw_move_down_a_line(view, old_pos, file_length,
	    				      lt_index, rect);	    			             		        	
	    	break;
	     }  
	    case TXTSW_PREVIOUS_LINE: {
	        int      upper_context = (int)LINT_CAST(
	        		ev_get(chain, EV_CHAIN_UPPER_CONTEXT));
	        		
	    	if (visible_status != EV_XY_VISIBLE) {
	    	    textsw_normalize_internal(view, old_pos, old_pos,
	    	         upper_context + 1, 0, 
	                 (TXTSW_NI_NOT_IF_IN_VIEW | TXTSW_NI_MARK));
	    	    visible_status = ev_xy_in_view(view->e_view, old_pos, 
	    	    			           &lt_index, &rect);
	    	}
	    	if (visible_status == EV_XY_VISIBLE)    
	    	    pos = textsw_move_up_a_line(view, old_pos, file_length,
	    				    lt_index, rect);
	        break;
	    } 
	    case TXTSW_WORD_BACKWARD:
	    	pos = textsw_move_backward_a_word(view, old_pos);
	        break;   
	    case TXTSW_WORD_FORWARD:
	    	pos = textsw_move_forward_a_word(view, old_pos, file_length);
	        break;
	    case TXTSW_WORD_END:
	    	pos = textsw_move_to_word_end(view, old_pos, file_length);
	        break;    
	    default:
	    	ok = FALSE;
	    	break;	               	
	}
	
	if  (ok) {
	    if ((pos == ES_CANNOT_SET) && (visible_status != EV_XY_VISIBLE))
	        pos = old_pos;  /* Put insertion point in view anyway */
	        
	    if (pos == ES_CANNOT_SET) 
	        (void) window_bell(WINDOW_FROM_VIEW(view));
	    else {
	        textsw_set_insert(folio, pos);
	        make_insert_visible(view, visible_status, old_pos, pos);
	        /* Replace pending delete with primary selection */
	        if (EV_SEL_PENDING_DELETE & ev_get_selection(
				    chain, &first, &last_plus_one, EV_SEL_PRIMARY)) 
		        (void) textsw_set_selection(
		            VIEW_REP_TO_ABS(view), first, last_plus_one,
		            EV_SEL_PRIMARY);
	        
	    }
	    textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
				   (caddr_t)TEXTSW_INFINITY-1);
				   
	    if  ((direction != TXTSW_NEXT_LINE) && 
	         (direction != TXTSW_PREVIOUS_LINE))		   
	         (void) textsw_record_caret_motion(folio, direction, -1);

	}
		
}

