#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ev_update.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986, 1988 by Sun Microsystems, Inc.
 */

/*
 * Initialization and finalization of entity views.
 */

#include <suntool/primal.h>

#include <suntool/ev_impl.h>
#include <stdio.h>
#include <sys/time.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include "sunwindow/sv_malloc.h"

/* for ev_lt_format ... */
static struct ev_impl_line_data line_data = {-1, -1, -1, -1};

/*
 * - Invariant on input:
 *   those lines in the line table that have not been damaged or deleted
 *   still correspond to what is on the screen.
 *   damaged lines have a damaged index > -1.
 *   deleted lines have line table entries that are repeated.
 *   an entirely deleted line is not marked damaged: repetition suffices.
 *   this routine is called whenever a simple insert or delete has occurred.
 *   all operations are on old line table.
 * - Invariant on output:
 *   the delta due to the insert or delete has been propagated.
 *   the line table may be passed to this function again,
 *   or to the format routine.
 * 
 *   find line table entry for start of change
 *   if insertion
 *     set damaged index to min of unsigned damaged index and
 *       1st damaged entity relative to line start
 *     propagate delta but not damaged index through rest of line table
 *   else
 *     if deleting partial line at start of span
 *       set damaged index to min of unsigned damaged index and
 *         1st damaged entity relative to line start
 *     for lt = first completely deleted line
 *     until lt = first line not completely deleted
 *       *lt = start of deleted span
 *       damaged index = -1
 *     if deleting partial line at end of span
 *       set damaged index to min of unsigned damaged index and
 *         1st damaged entity relative to line start
 *     propagate delta but not damaged index through rest of line table
 *
 * If the table needs any repainting, return 1; otherwise return 0.
 */
int
ev_lt_delta(view, before_edit, delta)
    Ev_handle		view;
    Es_index		before_edit;
    int			delta;
{
    int			result = 0;
    int			lt_index;
    Es_index		range_min = before_edit;
    Ev_impl_line_seq	seq = (Ev_impl_line_seq)
						view->line_table.seq;

    /* If edit is not before beginning of table ... */
    if (before_edit >= seq[0].pos) {
	if (delta < 0)
	    range_min += delta;
	lt_index = ft_bounding_index(&view->line_table, range_min);
	/* We know end of edit is not before beginning of table.
	 * If it starts before beginning of table, start it at
	 * beginning of table.
	 */
	if (lt_index == view->line_table.last_plus_one) {
	    Es_index	pos;
	    lt_index = 0;
	    pos = ev_line_start(view, range_min);
	    seq[lt_index].considered += seq[lt_index].pos - pos;
	    seq[lt_index].damaged = 0;
	    seq[lt_index].pos = pos;
	}
	/* If edit is not after end of visible lines in table ... */
	if (lt_index+1 < view->line_table.last_plus_one) {
	    result = 1;
	    range_min -= seq[lt_index].pos;
	    seq[lt_index].damaged =
		seq[lt_index].damaged >= 0 ?
		min(seq[lt_index].damaged, range_min) : range_min;
	    if (delta < 0) {
		int		max_lt_index;

		max_lt_index
		    = ft_bounding_index(&view->line_table, before_edit);
		if (max_lt_index+1 < view->line_table.last_plus_one
		&&  seq[max_lt_index].pos < before_edit
		&&  seq[max_lt_index].pos >= (before_edit+delta)) {
		    seq[max_lt_index].damaged = 0;
		}
		for (--max_lt_index;
		     max_lt_index > lt_index;
		     --max_lt_index) {
		    seq[max_lt_index].damaged = -1;
		}
	    }
	}
    }
    ev_update_lt_after_edit(&view->line_table, before_edit, delta);
    return result;
}

#define ev_lt_fmt_find_damage(new, old, new_ix, old_ix, lpo) \
    save_old_ix = old_ix; \
    for (tmp = old; old_ix+1 < lpo; ++old_ix, ++new_ix, ++tmp) { \
	if (tmp->damaged > -1 || (tmp+1)->damaged > -1 \
	||  tmp->pos == ES_INFINITY) \
	    break; \
    } \
    if (tmp > old) { \
	bcopy(old, new, \
	    (old_ix-save_old_ix) * sizeof(struct ev_impl_line_seq)); \
	old = tmp; \
	new += (old_ix-save_old_ix); \
    } \
    new->pos = old->pos;

/*
 * To format:
 * - Invariant on input:
 *   deltas have been propagated and damage marked as per the input invariant
 *   on insert/delete.
 * - Invariant on output:
 *   a trial line table has been constructed, with correct line breaks.
 *   lines in the new line table that should be blitted down are
 *   flagged for blit down.
 *   lines in the new line table that should be repainted have a
 *   damage index > -1.
 *   lines that should be blitted up or left alone are unmarked.
 *     (that is, damage index = -1.)
 *   the old and new line tables may be passed to the paint routine.
 */
void
ev_lt_format(view, new_lt, old_lt)
    Ev_handle		 view;
    Ev_line_table	*new_lt;
    Ev_line_table	*old_lt;
{
    register Ev_impl_line_seq	 new = (Ev_impl_line_seq) new_lt->seq;
    register Ev_impl_line_seq	 old = (Ev_impl_line_seq) old_lt->seq;
    register Ev_impl_line_seq	 tmp;
    register int	 new_ix = 0;
    register int	 old_ix = 0;
    register int	 lpo = old_lt->last_plus_one;
    register Es_index	 length = es_get_length(view->view_chain->esh);
    int			 save_old_ix;
    struct ei_process_result line_lpo, ev_line_lpo();

    ev_lt_fmt_find_damage(new, old, new_ix, old_ix, lpo);

    /*
     * Invariant: we know new->pos is correct;
     * This while loop computes
     * new->damaged, new->blit_down, and (new+1)->pos.
     */
    while (new_ix+1 < lpo) {

	new->blit_down = -1;
	new->blit_up = -1;
	if (new->pos == ES_INFINITY) {
	    ft_set(*new_lt, new_ix, lpo, ES_INFINITY, &line_data);
	    old = &((Ev_impl_line_seq) old_lt->seq)[new_ix];
	    if (old->pos < ES_INFINITY
	    &&  old->pos + old->considered > length)
		new->damaged = 0;
	    break;
	}
	if (old_ix+1 < lpo && new->pos == old->pos) {
	    /*
	     * Skip deleted lines.  If there is a deletion,
	     * blit from the bottom of the deletion.
	     */
	    while (old_ix+1 < lpo
	    && (old+1)->pos == old->pos) {
		++old;
		++old_ix;
	    }
	    /* undamaged old lines either get blitted or left alone */
	    if (new_ix > old_ix && old->pos < length) {
		new->blit_down = old_ix;
	    }
	    if (new_ix < old_ix) {
		if (old_ix+1 < lpo) {
		    new->blit_up = old_ix;
		} else {
		    old->damaged = 0;
		}
	    }
	    if (old->damaged == -1) {
		/*
		 * If there is damage on the next line, we may have to
		 * suck up characters from it.
		 */
		if ((old+1)->damaged > -1
		&&  (old+1)->damaged + (old+1)->pos <=
		    old->considered + old->pos) {
		    line_lpo = ev_line_lpo(view, new->pos);
		    ++old;
		    /* If we are sucking chars from the next line ... */
		    if (line_lpo.last_plus_one > old->pos) {
			new->damaged = old->pos - new->pos;
		    }
		    new->considered = line_lpo.considered - new->pos;
		    ++new;
		    if (line_lpo.last_plus_one == length
		    && line_lpo.considered == length) {
			new->pos = ES_INFINITY;
		    } else {
			new->pos = line_lpo.last_plus_one;
		    }
		} else {
		    new->considered = old->considered;
		    (++new)->pos = (++old)->pos;
		}
		++new_ix;
		++old_ix;
		continue;
	    }
	}
	/*
	 * Invariant: old is damaged or rewrapped and old_ix+1 < lpo
	 * Reaching here repetitively indicates massive rewrap.
	 *
	 * BUG ALERT on "old->damaged = -1;":
	 * if we ever want to delay between format and paint,
	 * such that formatting may need to be done again,
	 * move this reset of old->damaged to the paint code.
	 */
	new->damaged = (old_ix+1 >= lpo || new->pos != old->pos) ?
			0 : old->damaged;
	if (old_ix < lpo)
	    old->damaged = -1;
	line_lpo = ev_line_lpo(view, new->pos);
	if (line_lpo.last_plus_one < new->pos + new->damaged)
	    new->damaged = line_lpo.last_plus_one - new->pos;
	new->considered = line_lpo.considered - new->pos;
	++new;
	++new_ix;
	if (line_lpo.last_plus_one == length
	&& line_lpo.considered == length) {
	    new->pos = ES_INFINITY;
	} else {
	    new->pos = line_lpo.last_plus_one;
	}
	/*
	 * If we didn't insert >= 1 line of text at this point,
	 * catch old up with new, skipping deleted lines
	 */
	while (old_ix+1 < lpo && (old+1)->pos <= line_lpo.last_plus_one) {
	    ++old;
	    ++old_ix;
	}
	if (old_ix == new_ix && old->pos == new->pos) {
	    ev_lt_fmt_find_damage(new, old, new_ix, old_ix, lpo);
	}
    }   /* while  (new_ix+1 < lpo) */
}

/*
 * Set up a pair of rects for blitting.
 * Note that old_rect->r_height is not used and therefore
 * is not precisely set.
 */
static void
ev_set_up_rect(view, new_rect, old_rect, new_top, old_top, new_bot)
    Ev_handle		 view;
    Rect		*new_rect;
    Rect		*old_rect;
    int			 new_top;
    int			 old_top;
    int			 new_bot;
{
    Rect		 tmp_rect;
    Rect		 ev_rect_for_line();

    tmp_rect = ev_rect_for_line(view, new_top);
    new_rect->r_top = tmp_rect.r_top;
    tmp_rect = ev_rect_for_line(view, new_bot);
    new_rect->r_height = (tmp_rect.r_top - new_rect->r_top)
    			+ tmp_rect.r_height;
    tmp_rect = ev_rect_for_line(view, old_top);
    old_rect->r_top = tmp_rect.r_top;
#ifdef notdef
    ev_add_margins(view, new_rect);
    ev_add_margins(view, old_rect);
#endif notdef
}

static int
ev_get_width(view, first, last_plus_one, lt_index)
    register Ev_handle		 view;
    Es_index			 first, last_plus_one;
    int				 lt_index;
{
    struct ei_process_result	 ei_measure;
    struct ei_process_result	 ev_ei_process();
    Ev_pos_info			*cache;
    Ev_chain			 chain = view->view_chain;
    Ev_chain_pd_handle		 private = EV_CHAIN_PRIVATE(chain);
    Ev_pd_handle		 view_private = EV_PRIVATE(view);
    Rect			 new_rect;

    if (first == last_plus_one)
	return 0;
    /* Try using (barely(?) invalid) cached insert info.
     * This may fall apart when multiple edits are involved.
     */
    new_rect = ev_rect_for_line(view, lt_index);
    cache = &view_private->cached_insert_info;
    if ((cache->edit_number > 0) &&
        (cache->pos == last_plus_one) &&
        (ft_bounding_index(&view->tmp_line_table, last_plus_one) == lt_index) &&
        (cache->edit_number == private->edit_number-1) &&
        (cache->index_of_first == EV_VIEW_FIRST(view)) ) {

	return cache->pr_pos.x - new_rect.r_left;
    }
    ei_measure = ev_ei_process(view, first, last_plus_one);
    return ei_measure.pos.x - new_rect.r_left;
}

static void
ev_display_line(view, width_before, lt_index, first, last_plus_one)
    register Ev_handle		 view;
    int				 width_before;
    int				 lt_index;
    Es_index			 first, last_plus_one;
{
#ifndef BUFSIZE
#define BUFSIZE			 2048
#endif  BUFSIZE
    static char			*buf;	/* Avoids constant malloc/free */
    char			*bp;
    Ev_chain			chain = view->view_chain;
    Ev_chain_pd_handle		chain_private =	EV_CHAIN_PRIVATE(chain);
    Ei_handle			eih = chain->eih;
    struct ei_process_result	result;
    struct ei_process_result	ev_ei_process();
    Rect			rect;
    Rect			ev_rect_for_line();
    Es_buf_object		esbuf;
    struct range		range;
    int				rc = -1;
    Ev_overlay_handle		glyph;
    Es_index			glyph_pos = ES_INFINITY;
    register Ev_impl_line_seq	line_seq =(Ev_impl_line_seq)
                                          view->line_table.seq;
    
#define ESBUF_SET_POSITION(pos)						\
	es_set_position(esbuf.esh, 					\
			(last_plus_one = esbuf.last_plus_one = (pos)))

    /* Don't statically allocate buf to avoid blowing up bss */
    if (buf == (char *)0) {
	buf = sv_malloc(BUFSIZE+1);
    }
    if (first == ES_INFINITY)
		first = es_get_length(chain->esh);
    if (last_plus_one == ES_INFINITY)
		last_plus_one = es_get_length(chain->esh);
    rect = ev_rect_for_line(view, lt_index);
    rect.r_width -= width_before;
    if (rect.r_width == 0)
	return;
    rect.r_left += width_before;
    /*
     * at the end of the last line, just clear to end of line.
     */
    if (first == es_get_length(chain->esh)
    	||  first == last_plus_one) {
		ev_clear_rect(view, &rect);
		return;
    }
    result.pos.x = rect.r_left;
    result.pos.y = rect.r_top;
    ev_range_info(chain_private->op_bdry, first, &range);
    range.last_plus_one = min(range.last_plus_one, last_plus_one);
    esbuf.esh = chain->esh;
    es_set_position(esbuf.esh, first);
    esbuf.sizeof_buf = last_plus_one - first;
    bp = (esbuf.sizeof_buf >= BUFSIZE) ? malloc(esbuf.sizeof_buf+1) : buf;
    if (!bp) {
		fprintf(stderr, "ev_display_line: out of memory\n");
		return;
    }
    esbuf.buf = bp;
    esbuf.buf[esbuf.sizeof_buf] = '\0';
    if (esbuf.sizeof_buf > 0) {
		rc = ev_fill_esbuf(&esbuf, &first);
		ASSERT (rc == 0);
    }
    ev_clear_from_margins(view, &rect, NULL);
    if (rc == 0) {
	do {
	    esbuf.last_plus_one = range.last_plus_one;
	    esbuf.sizeof_buf = esbuf.last_plus_one - esbuf.first;
	    result.bounds = rect;
	    
	    if (range.ei_op & EI_OP_EV_OVERLAY) {
		extern Op_bdry_handle	ev_find_glyph();
		Op_bdry_handle			glyph_op_bdry;
		    
		range.ei_op &= ~EI_OP_EV_OVERLAY;
		glyph_op_bdry = ev_find_glyph(chain, line_seq[lt_index].pos);
		    
		if (glyph_op_bdry) {
			glyph = (Ev_overlay_handle)
				   LINT_CAST(glyph_op_bdry->more_info);
			glyph_pos = glyph_op_bdry->pos;
			if (esbuf.last_plus_one > glyph_pos) {
				ESBUF_SET_POSITION(glyph_pos);
			}
		}
	    }	    
	    result = ei_process(
		eih, range.ei_op|EI_OP_CLEAR_INTERIOR|EI_OP_CLEAR_BACK,
		&esbuf, result.pos.x, result.pos.y, PIX_SRC|PIX_DST,
		view->pw, &rect, /* tab_origin */ view->rect.r_left);
	    if (esbuf.last_plus_one == glyph_pos) {
	        extern void			ev_do_glyph();
		    
		ev_do_glyph(view, &glyph_pos, &glyph, &result);
	    }
	    
	    if (esbuf.last_plus_one < last_plus_one) {
		esbuf.buf += esbuf.sizeof_buf;
		esbuf.first = esbuf.last_plus_one;
		ev_range_info(chain_private->op_bdry, esbuf.first, &range);
		range.last_plus_one =
		    	min(range.last_plus_one, last_plus_one);
	    }
	} while (esbuf.last_plus_one < last_plus_one);
    }
    if (bp != buf)
	free(bp);
}

static void
ev_copy_and_fix(view, new_rect, old_rect)
    Ev_handle		 view;
    Rect		*new_rect, *old_rect;
{
    Pixwin		*pw = view->pw;
    Ev_chain_pd_handle
			 private = EV_CHAIN_PRIVATE(view->view_chain);

    pw_copy(pw, new_rect->r_left, new_rect->r_top,
	    new_rect->r_width, new_rect->r_height, PIX_SRC,
	    pw, old_rect->r_left, old_rect->r_top);
    if (! rl_empty(&pw->pw_fixup)) {
	ev_display_fixup(view);
    }
 /*   ev_clear_from_margins(view, old_rect, new_rect); */
    if (private->notify_level & EV_NOTIFY_SCROLL)
	ev_notify(view,
		  EV_ACTION_SCROLL, old_rect, new_rect,
		  0);
}

/*
 * To paint:
 * - Invariant on input:
 *   the new line table contains correct line breaks.
 *   its damaged indices indicate where on a line to start painting.
 *   lines to be blitted down are flagged for blit down.
 * - Invariant on output:
 *   the new line table corresponds to what is on the screen.
 */
void
ev_lt_paint(view, new_lt, old_lt, clear_to_bottom)
    Ev_handle		 view;
    Ev_line_table	*new_lt;
    Ev_line_table	*old_lt;
    int			 clear_to_bottom;	/* This is a kludge */
{
    register Ev_impl_line_seq	 new = & ((Ev_impl_line_seq)
    				 new_lt->seq)[new_lt->last_plus_one-2];
    register Ev_impl_line_seq	 old = & ((Ev_impl_line_seq) old_lt->seq)[0];
    register Ev_impl_line_seq	 new_eob;
    Rect		 new_rect;
    Rect		 old_rect;
    int			 new_lim;
    int			 new_end_of_block;
    register int	 new_ix = new_lt->last_plus_one-2;
    int			 lpo = old_lt->last_plus_one;
    Es_index		 length = es_get_length(view->view_chain->esh);
    Es_index		 next_pos;
    
    if (ev_get(view, EV_NO_REPAINT_TIL_EVENT)) 
        return;

    new_rect = view->rect;
    ev_add_margins(view, &new_rect);
    old_rect = new_rect;
    
    /* Blit down all blocks of lines flagged for blit_down. */
    new_lim = 0;
    while (new > (Ev_impl_line_seq) new_lt->seq) {
	if (new->blit_down > -1) {
	    /* find top of blit block. */
	    for (new_end_of_block = new_ix, new_eob = new;
	         new_end_of_block > new_lim
	    &&   (new_eob-1)->blit_down > -1
	    &&   (new_eob-1)->blit_down + 1 == new_eob->blit_down;
	         --new_end_of_block, --new_eob) {
		new_eob->blit_down = -1;
	    }
	    /* set up the rect to slide */
	    ev_set_up_rect(view, &new_rect, &old_rect,
	        new_end_of_block, new_eob->blit_down, new_ix);
	    new_eob->blit_down = -1;
	    
	    ev_copy_and_fix(view, &new_rect, &old_rect);

	    /* decrement past the blit block we just processed */
	    new = new_eob-1;
	    new_ix = new_end_of_block-1;
	} else {
	    --new;
	    --new_ix;
	}
    }
    
    /* Step forward, blitting up and repairing damage */
    new = (Ev_impl_line_seq) new_lt->seq;
    new_ix = 0;
    new_lim = lpo-2;
    while (new_ix + 1 < lpo && new->pos < length) {
	/* Blit up */
	new_eob = new;
	if (new->blit_up > -1) {
	    /* find bottom of blit block. */
	    new_end_of_block = new_ix;
	    for (;
	         new_end_of_block < new_lim
	    &&   (new_eob+1)->blit_up > -1
	    &&   (new_eob+1)->blit_up - 1 == new_eob->blit_up;
	         ++new_end_of_block, ++new_eob)
	    ;
	    /* set up the rect to slide */
	    ev_set_up_rect(view, &new_rect, &old_rect,
	        new_ix, new->blit_up, new_end_of_block);
	    
	    ev_copy_and_fix(view, &new_rect, &old_rect);
	}
	/* Repair damage */
	while (new <= new_eob) {
	    if (new->damaged > -1) {
		next_pos = (new+1)->pos == ES_INFINITY ? length : (new+1)->pos;
		old = & ((Ev_impl_line_seq) old_lt->seq)[new_ix];
		if (old->pos != ES_INFINITY
		&&  (new+1)->pos == ES_INFINITY
		&&  old->pos + old->considered > next_pos)
		    next_pos = old->pos + old->considered;
		if (new->pos + new->damaged <= next_pos) {
		    /* measure to new->damaged and paint from there. */
		    next_pos = (new+1)->pos == ES_INFINITY ?
			length : (new+1)->pos;
		    ev_display_line(view,
			ev_get_width(view,
			    new->pos, new->pos + new->damaged, new_ix),
			new_ix, new->pos + new->damaged, next_pos);
		}
		new->damaged = -1;
	    }
	    new->blit_up = -1;
	    ++new;
	    ++new_ix;
	}
    }
    /*
     *  Performance enhancement:  Added more constraints
     *    &&  (new->damaged > -1 || new->blit_up > -1
     *    ||   ((Ev_impl_line_seq) old_lt->seq)[new_ix].considered > -1)
     */
    if (new_ix + 1 < lpo && new->pos >= length
        && (new->blit_up > -1 || new->damaged > -1 || old->damaged > -1
            || ((Ev_impl_line_seq) old_lt->seq)[new_ix].considered > -1)
            || clear_to_bottom) {
	new->damaged = -1;
	new->blit_up = -1;
	new_rect = ev_rect_for_line(view, new_ix);
	ev_clear_to_bottom(view, &new_rect, new_rect.r_top, 0);
    }
}
