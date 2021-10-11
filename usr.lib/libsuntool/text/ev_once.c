#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ev_once.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Initialization and finalization of entity views.
 */

#include <suntool/primal.h>

#include <sys/time.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <suntool/tool.h>
#include <suntool/ev_impl.h>

extern Es_index	ev_index_for_line();

static short caret_image[7]={
	0x1000, 0x3800, 0x3800, 0x7C00, 0x7C00, 0xFE00, 0xFE00
};
mpr_static(ev_caret_mpr, 7, 7, 1, caret_image);
static short ghost_caret[7]={
	0x1000, 0x2800, 0x5400, 0xAA00, 0x5400, 0x2800, 0x1000
};
#ifdef notdef
static short ghost_caret[7]={
	0x1000, 0x2800, 0x2800, 0x5400, 0x5400, 0xAA00, 0xAA00
};
#endif notdef
mpr_static(ev_ghost_mpr, 7, 7, 1, ghost_caret);

Ev_chain
ev_create_chain(esh, eih)
	Es_handle	esh;
	Ei_handle	eih;
{
	Ev_chain		chain = NEW(struct ev_chain_object);
	Ev_chain_pd_handle	private =
				NEW(struct ev_chain_private_data_object);

	chain->esh = esh;
	chain->eih = eih;
	chain->fingers = FT_CLIENT_CREATE(10, struct ev_finger_datum);
	chain->client_data = (caddr_t) 0;
	FT_CLEAR_ALL(chain->fingers);
	private->insert_pos = ES_CANNOT_SET;
	(void) ev_init_mark(&private->selection[0]);
	(void) ev_init_mark(&private->selection[1]);
	private->op_bdry = FT_CLIENT_CREATE(10, struct op_bdry_datum);
	FT_CLEAR_ALL(private->op_bdry);
	private->lower_context = EV_NO_CONTEXT;
	private->upper_context = EV_NO_CONTEXT;
	private->notify_level = 0;
	private->notify_proc = (int (*)()) 0;
	private->edit_number = 1;
	private->caret_is_ghost = TRUE;
	private->caret_pr = &ev_caret_mpr;
	private->caret_hotpoint.x = (ev_caret_mpr.pr_size.x+1) >> 1;
	private->caret_hotpoint.y = 2;
	private->ghost_pr = &ev_ghost_mpr;
	private->ghost_hotpoint.x = (ev_ghost_mpr.pr_size.x+1) >> 1;
	private->ghost_hotpoint.y = 2;
	private->cache_pos_for_file_line.index_of_first = ES_CANNOT_SET;
	private->cache_pos_for_file_line.edit_number = 0;
	private->cache_pos_for_file_line.first_line_number = 0;
	private->cache_pos_for_file_line.line_count = 0;
	chain->private_data = (caddr_t) private;
	return(chain);
}

Ev_handle
ev_create_view(chain, pw, rect)
	Ev_chain	chain;
	struct pixwin	*pw;
	struct rect	*rect;
{
	extern Ev_line_table	ev_ft_for_rect();
	Ev_handle		view = NEW(struct ev_object);
	Ev_pd_handle		private = NEW(struct ev_private_data_object);

	view->pw = pw;
	view->rect = *rect;
	view->line_table = ev_ft_for_rect(chain->eih, rect);
	view->tmp_line_table = ev_ft_for_rect(chain->eih, rect);
	view->view_chain = chain;
	view->next = chain->first_view;
	chain->first_view = view;
	private->caret_pr_pos.x = EV_NULL_DIM;
	private->caret_pr_pos.y = EV_NULL_DIM;
	private->left_margin = private->right_margin = 0;
	private->right_break = EV_CLIP;
	/*   Note: calloc correctly initializes private->cached_insert_info
	 * and private->cached_line_info.
	 */
	view->client_data = (caddr_t) 0;
	view->private_data = (caddr_t) private;
	return(view);
}

ev_destroy(view)
	Ev_handle		view;
{
	register Ev_handle	first_view = view->view_chain->first_view;

	if (first_view == view) {
		view->view_chain->first_view = first_view->next;
	} else {
		register Ev_handle prior = first_view;
		while (prior->next != view) {
			prior = prior->next;
		}
		prior->next = view->next;
	}
	ft_destroy(&view->line_table);
	ft_destroy(&view->tmp_line_table);
	free((char *)(view->private_data));
	free((char *)view);
}

Ev_chain
ev_destroy_chain_and_views(chain)
	Ev_chain	chain;
{
	Ev_handle	next_view = chain->first_view;

	while (next_view) {
		next_view = next_view->next;
		ev_destroy(chain->first_view);
	}
	ft_destroy(&chain->fingers);
	free(chain->private_data);
	free((char *)chain);
	return(0);
}

Ev_handle
ev_view_above(this_view)
	Ev_handle	this_view;
{
	register int		max_top = -1, this_top = this_view->rect.r_top;
	register Ev_handle	view, result = 0;

	FORALLVIEWS(this_view->view_chain, view) {
	    if (view->rect.r_top > max_top && view->rect.r_top < this_top) {
		max_top = view->rect.r_top;
		result = view;
	    }
	}
	return(result);
}

Ev_handle
ev_view_below(this_view)
	Ev_handle	this_view;
{
	register int		min_top = 20000,
				this_top = this_view->rect.r_top;
	register Ev_handle	view, result = 0;

	FORALLVIEWS(this_view->view_chain, view) {
	    if (view->rect.r_top < min_top && view->rect.r_top > this_top) {
		min_top = view->rect.r_top;
		result = view;
	    }
	}
	return(result);
}

Ev_handle
ev_highest_view(chain)
	Ev_chain	chain;
{
	register Ev_handle	view, result = chain->first_view;

	FORNEXTVIEWS(result, view) {
	    if (view->rect.r_top < result->rect.r_top)
		result = view;
	}
	return(result);
}

Ev_handle
ev_lowest_view(chain)
	Ev_chain	chain;
{
	register Ev_handle	view, result = chain->first_view;

	FORNEXTVIEWS(result, view) {
	    if (view->rect.r_top > result->rect.r_top)
		result = view;
	}
	return(result);
}

extern void
ev_line_info(view, top, bottom)
	register Ev_handle	 view;
	int			*top, *bottom;
/* Return the 1-origin line numbers of the physical lines present on the
 * first and last screen lines of the view.
 */
{
	Es_index		 last_plus_one, old_first, delta_first;
	Ev_pd_handle		 private = EV_PRIVATE(view);
	register Ev_chain	 chain = view->view_chain;
	Ev_chain_pd_handle	 chain_private = EV_CHAIN_PRIVATE(chain);
	int			 edit_number_ok, first_ok;
	register Ev_physical_line_info
				*cache = &(private->cached_line_info);

	edit_number_ok = (cache->edit_number == chain_private->edit_number);
	first_ok = (cache->index_of_first == EV_VIEW_FIRST(view));
	if (!(first_ok && edit_number_ok)) {
	    /*
	     * Cache is invalid - incremental fix is:
	     * a) possible iff edit_number_ok;
	     * b) worthwhile unless view moved closer to start of file and
	     *    view is now closer to start than to old position.
	     */
	    old_first = cache->index_of_first;
	    ev_view_range(view, &cache->index_of_first, &last_plus_one);
	    delta_first = old_first - cache->index_of_first;
	    if ((edit_number_ok) && (
		(delta_first < 0) || (delta_first < cache->index_of_first) )) {
		if (delta_first < 0) {
		    cache->first_line_number +=
			ev_newlines_in_esh(chain->esh,
					   old_first, cache->index_of_first);
		} else {
		    cache->first_line_number -=
			ev_newlines_in_esh(chain->esh,
					   cache->index_of_first, old_first);
		}
	    } else {
		cache->first_line_number =
		    ev_newlines_in_esh(chain->esh, 0L,
				       cache->index_of_first);
	    }
	    cache->line_count =
		ev_newlines_in_esh(chain->esh,
				   cache->index_of_first,
				   last_plus_one);
	    cache->edit_number = chain_private->edit_number;
	}
	if (top)
	    *top = cache->first_line_number + 1;
	if (bottom)
	    *bottom = cache->first_line_number +
		      cache->line_count;
}

extern int
ev_newlines_in_esh(esh, first, last_plus_one)
	Es_handle	esh;
	Es_index	first, last_plus_one;
{
#define BUFSIZE 2096
	char			buf[BUFSIZE+4];
	register char		*in_buf, *buf_last_plus_one;
	register int		count = 0;
	int			 read;
	register Es_index	new_pos = first, pos;

	es_set_position(esh, new_pos);
	while (new_pos < last_plus_one) {
	    pos = new_pos;
	    new_pos = es_read(esh, BUFSIZE, buf, &read);
	    if (READ_AT_EOF(pos, new_pos, read))
		goto Return;
	    if (read > 0) {
		if (pos+read > last_plus_one) {
		    read = last_plus_one - pos;
		}
		buf_last_plus_one = buf + read;
		for (in_buf = buf; in_buf < buf_last_plus_one; in_buf++) {
		    if (*in_buf == '\n')
			count++;
		}
	    }
	}
Return:
	return(count);
#undef BUFSIZE
}

extern int
ev_rect_for_ith_physical_line(view, phys_line, first, rect, skip_white_space)
	register Ev_handle	 view;
	int			 phys_line;
	register Es_index	*first;
	Rect			*rect;
	int			 skip_white_space;
{
	extern Rect		 ev_rect_for_line();
	int			 lt_index;
	Es_index		 last_plus_one;
	char			*newline_str = "\n";

	ev_view_range(view, first, &last_plus_one);
	if (phys_line == 0) {
	    lt_index = 0;
	} else {
	    ev_find_in_esh(view->view_chain->esh, newline_str, 1,
			   *first, (unsigned)phys_line, 0,
			   first, &last_plus_one);
	    if (*first == ES_CANNOT_SET)
		return(-1);
	    lt_index = ft_bounding_index(&view->line_table, last_plus_one);
	}
	*first = ev_index_for_line(view, lt_index);
	*rect = ev_rect_for_line(view, lt_index);
	if (skip_white_space) {
	    Es_index	span_first;
	    ev_span(view->view_chain, *first, &span_first, &last_plus_one,
		EI_SPAN_SP_AND_TAB|EI_SPAN_RIGHT_ONLY|EI_SPAN_IN_CLASS_ONLY);
	    if (span_first != ES_CANNOT_SET) {
		*first = last_plus_one;
		(void) ev_xy_in_view(view, *first, &lt_index, rect);
	    }
	}
	return(lt_index);
}

/*
 *   Semantics added to catch filemerge problems: a line MUST contain at
 * least one character.  Prior to this additional restriction, this routine
 * would return the stream length when asked to map line n, for streams
 * containing exactly n newlines, and no characters after the last newline.
 */
extern Es_index
ev_position_for_physical_line(chain, line, skip_white_space)
	Ev_chain	 chain;
	register int	 line;
	int		 skip_white_space;
{
	register Ev_chain_pd_handle
			 chain_private = EV_CHAIN_PRIVATE(chain);
	register Ev_physical_line_info
			*cache = &chain_private->cache_pos_for_file_line;
	Es_index	 first, last_plus_one;
	Es_index	 start_search;
	register int	 count;
	char		 buf[2];

	if (line > 0) {
	    count = line;
	    if (cache->edit_number == chain_private->edit_number &&
		line >= cache->first_line_number) {
		start_search = cache->index_of_first;
		count -= cache->first_line_number;
	    } else {
		start_search = 0L;
	    }
	    if (count) {
		buf[0] = '\n';
		ev_find_in_esh(chain->esh, buf, 1, start_search, count, 0,
			       &first, &last_plus_one);
	    } else {
		/* Consecutive identical queries: make sure that following
		 * tests and assignments on first and last_plus_one will 
		 * generate the correct result.
		 */
		first = last_plus_one = start_search;
	    }
	    if (first != ES_CANNOT_SET) {
		/* Check for match at end-of-stream. */
		if (last_plus_one < es_get_length(chain->esh)) {
		    first = last_plus_one;
		    cache->edit_number = chain_private->edit_number;
		    cache->first_line_number = line;
		    cache->index_of_first = first;
		} else {
		    first = ES_CANNOT_SET;
		}
	    }
	} else if (line < 0) {
	    first = ES_CANNOT_SET;
	} else {
	    first = 0;
	}
	if ((first != ES_CANNOT_SET) && skip_white_space) {
	    Es_index	span_first;
	    ev_span(chain, first, &span_first, &last_plus_one,
		  EI_SPAN_SP_AND_TAB|EI_SPAN_RIGHT_ONLY|EI_SPAN_IN_CLASS_ONLY);
	    if (span_first != ES_CANNOT_SET)
		first = last_plus_one;
	}
	return(first);
}

ev_find_in_esh(esh, pattern, pattern_length, position, count, flags,
		first, last_plus_one)
	Es_handle	 esh;		  /* stream handle          */
	char		*pattern;	  /* pattern to search for  */
	unsigned	 pattern_length;  /* no. of chars in pat.   */
	Es_index	 position;	  /* start search from here */
	unsigned	 count;		  /* no. occurrances of pat */
	unsigned	 flags;		  /* fwd or back            */
	Es_index	*first;		  /* start of found pattern */
	Es_index	*last_plus_one;	  /* end of found pattern   */
{
/*
 * Currently only simple patterns are supported.
 */
#define BUFSIZE 2096
	char			 buf[BUFSIZE+4];
	register char		*in_buf, *buf_last_plus_one,
				*matched_to, *done_match;
	register Es_index	 new_pos, pos, start_pattern = position;
	Es_index		 length;
	int			 read;
	int			 useful_bufsize = BUFSIZE;

	*first = ES_CANNOT_SET;
	if (flags & EV_FIND_RE)
	    goto Return;
	matched_to = pattern;
	done_match = pattern + pattern_length;
	if (flags & EV_FIND_BACKWARD) {
	    struct es_buf_object	esbuf;
	    esbuf.esh = esh;
	    esbuf.buf = buf;
	    esbuf.sizeof_buf = BUFSIZE;
	    esbuf.first = ES_INFINITY;
	    start_pattern--;
Init_Backward:
	    FOREVER {
		if (start_pattern < 0)
		    goto Return;
		switch (es_make_buf_include_index(&esbuf,
			start_pattern+(long)(pattern_length-1), BUFSIZE-1)) {
		  case 0:
		    pos = esbuf.first;
		    read = esbuf.last_plus_one - pos;
		    in_buf = buf + (start_pattern-pos);
		    FOREVER {
			ASSERT(in_buf <= buf + read);
			if (*matched_to++ != *in_buf++) {
			    start_pattern--;
			} else if (matched_to == done_match) {
			    if (--count == 0) {
				*first = start_pattern;
				*last_plus_one = pos + (in_buf-buf);
				goto Return;
			    }
			    start_pattern -= pattern_length;
				/* Overlap matches are not found. */
			} else
			    continue;
			matched_to = pattern;
			if (start_pattern >= pos) {
			    in_buf = buf + (start_pattern-pos);
			} else {
			    /*
			     * We need to back up as start_pattern is not
			     *   in the current buffer.
			     */
			    goto Init_Backward;
			}
		    }
		    break;
		  case 1:
		    length = es_get_length(esh);
		    if ((start_pattern+pattern_length-1) < length)
			goto Return;
		    goto Skip_Backward;
		  case 2:
		    /* Gap in entity indices (hopefully due to wrap-around
		     *   of bounded stream).  Move start_pattern over gap.
		     * NOTE: this implies that we never match a pattern
		     *   against a span of indices containing a gap.
		     */
		    length = esbuf.last_plus_one;
Skip_Backward:
		    /*
		     * Following test is paranoia to detect infinite loop
		     *   in "impossible" case.
		     */
		    if (esbuf.first == ES_CANNOT_SET)
			goto Return;
		    esbuf.first = ES_CANNOT_SET;
		    start_pattern = length-pattern_length;
		    break;
		}
	    }
	} else {
	    if (pattern_length == 1 && count == 1)
		useful_bufsize = 256;
Init_Forward:
	    new_pos = start_pattern;
	    es_set_position(esh, new_pos);
	    FOREVER {
		pos = new_pos;
		new_pos = es_read(esh, useful_bufsize, buf, &read);
		if (read == 0) {
		    if (pos == new_pos)
			goto Return;
		    /* Gap in entity indices (hopefully due to wrap-around
		     *   of bounded stream).  Move start_pattern over gap.
		     * NOTE: this implies that we never match a pattern
		     *   against a span of indices containing a gap.
		     */
		    start_pattern = new_pos;
		    goto Init_Forward;
		} else {
		    in_buf = buf;
		    buf_last_plus_one = buf + read;
		    FOREVER {
			if (*matched_to++ == *in_buf++) {
			    if (matched_to == done_match) {
				if (--count == 0) {
				    *first = start_pattern;
				    *last_plus_one = pos + (in_buf-buf);
				    goto Return;
				} else {
				    matched_to = pattern;
				    start_pattern = pos + (in_buf-buf);
					/* Overlap matches are not found. */
				}
			    }
			} else {
			    matched_to = pattern;
			    start_pattern++;
			    if (start_pattern >= pos) {
				in_buf = buf + (start_pattern-pos);
			    } else {
				/*
				 * We need to back up as start_pattern is not
				 *   in the current buffer.
				 */
				goto Init_Forward;
			    }
			}
			if (in_buf == buf_last_plus_one)
			    break;		/* Next buffer */
		    }
		}
	    }
	}
Return:
	return;
#undef BUFSIZE
}

