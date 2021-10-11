#ifndef lint
#ifdef sccs
static char	    sccsid[] = "@(#)ttyselect.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc. 
 */

#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sundev/kbd.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <suntool/tool.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>
#include <suntool/ttysw.h>
#include <suntool/ttysw_impl.h>
#include <suntool/ttyansi.h>
#include <suntool/charimage.h>
#include <suntool/charscreen.h>

#define MIN_SELN_TIMEOUT        0
#define DEFAULT_SELN_TIMEOUT    10		/* 10 seconds */
#define MAX_SELN_TIMEOUT        300		/* 5 minutes */

/*	global which can be used to make a shell tool which
 *	doesn't talk to the service
 */
int			ttysel_use_seln_service = 1;

/*	global & private procedures	*/
void			ttyhiliteselection();

static void		tvsub(),
			ttycountchars(),
			ttyenumerateselection(),
			ttyhiliteline(),
			ttysel_empty(),
			ttysel_resolve(),
			ttysortextents(),
			ttysel_read(),
			ttysel_write(),
			ttyputline(),
			ttysel_function(),
			ttysel_end_request();

static int		ttysel_insel(),
			ttysel_eq();

static Seln_result	ttysel_copy_in(),
			ttysel_copy_out(),
			ttysel_reply();

static struct ttyselection *
			ttysel_from_rank();

struct ttysel_context {
    unsigned              continued;
    struct ttysubwindow  *ttysw;
    unsigned              bytes_left;
};

#define	SEL_NULLPOS	-1

#define	TSE_NULL_EXTENT	{SEL_NULLPOS, SEL_NULLPOS}

static struct textselpos tse_null_extent = TSE_NULL_EXTENT;
static struct ttyselection ttysw_nullttysel = {
   0, 0, SEL_CHAR, 0, TSE_NULL_EXTENT, TSE_NULL_EXTENT, {0, 0}
};

static struct ttysubwindow *ttysel_ttysw;	/* stash for ttysel_read   */
static struct ttyselection *ttysel_ttysel;	/* stash for ttysel_write  */

static struct timeval maxinterval = {0, 400000};	/* XXX - for now */

static char		*ttysel_filename = "/tmp/ttyselection";

int
ttysw_is_seln_nonzero(ttysw, rank)
    register struct ttysubwindow  *ttysw;
    Seln_rank	rank;
{
    Seln_holder 	holder;
    Seln_request	*req_buf;
    char		**argv;
    int			bytesize = 0;

    if (!ttysw_getopt((caddr_t)ttysw, TTYOPT_SELSVC)) {
	return 0;
    }
    holder = seln_inquire(rank);
    if (holder.state != SELN_NONE) {
	req_buf = seln_ask(&holder, SELN_REQ_BYTESIZE, 0, 0);
	argv = (char **)LINT_CAST(req_buf->data);
	if (*argv++ == (char *)SELN_REQ_BYTESIZE) {
	    bytesize = (int)*argv;
	}
    }
    return bytesize;
}


/*
 * init_client: 
 */
void
ttysel_init_client(ttysw)
    register struct ttysubwindow  *ttysw;
{
   int          selection_timeout;

    if (!ttysw_getopt((caddr_t)ttysw, TTYOPT_SELSVC)) {
	return;
    }
    ttysw->ttysw_caret = ttysw_nullttysel;
    ttysw->ttysw_primary = ttysw_nullttysel;
    ttysw->ttysw_secondary = ttysw_nullttysel;
    ttysw->ttysw_shelf = ttysw_nullttysel;

   selection_timeout = defaults_get_integer_check("/SunView/Selection_Timeout",
		DEFAULT_SELN_TIMEOUT, MIN_SELN_TIMEOUT, MAX_SELN_TIMEOUT, 0);

   seln_use_timeout(selection_timeout);		/* set selection timeout */

    ttysw->ttysw_seln_client =
	seln_create(ttysel_function, ttysel_reply, (char *) ttysw);
    if (ttysw->ttysw_seln_client == (char *) NULL) {
	(void)ttysw_setopt((caddr_t)ttysw, TTYOPT_SELSVC, FALSE);
    }
}

void
ttysel_destroy(ttysw)
    register struct ttysubwindow  *ttysw;
{
    if (!ttysw_getopt((caddr_t)ttysw, TTYOPT_SELSVC)) {
	return;
    }
    if (ttysw->ttysw_seln_client != (char *) NULL) {
	seln_destroy(ttysw->ttysw_seln_client);
	ttysw->ttysw_seln_client = (char *) NULL;
    }
}

/*
 * Make sure we have desired selection. 
 */
void
ttysel_acquire(ttysw, sel_desired)
    register struct ttysubwindow  *ttysw;
    register Seln_rank             sel_desired;
{
    register Seln_rank             sel_received;
    register struct ttyselection  *ttysel;

    if (!ttysw_getopt((caddr_t)ttysw, TTYOPT_SELSVC)) {
	return;
    }
    ttysel = ttysel_from_rank(ttysw, sel_desired);
    if (!ttysel->sel_made) {
	if (sel_desired == SELN_CARET) {
	    Seln_holder		    holder;

	    holder = seln_inquire(SELN_UNSPECIFIED);
	    if (holder.rank != SELN_PRIMARY)
		return;
	}
	sel_received = seln_acquire(ttysw->ttysw_seln_client, sel_desired);
	if (sel_received == sel_desired) {
	    ttysel->sel_made = TRUE;
	    ttysel_empty(ttysel);
	} else {
	    (void)seln_done(ttysw->ttysw_seln_client, sel_received);
	}
    }
}

/*
 * Return rank of global selection state.
 */
Seln_rank
ttysel_mode(ttysw)
    struct ttysubwindow  *ttysw;
{
    Seln_holder             	   sel_holder;

    if (!ttysw_getopt((caddr_t)ttysw, TTYOPT_SELSVC)) {
	return SELN_PRIMARY;
    } else {
        sel_holder = seln_inquire(SELN_UNSPECIFIED);
	return sel_holder.rank;
    }
}

/*
 * Make a new selection. If multi is set, check for multi-click. 
 */
void
ttysel_make(ttysw, event, multi)
    register struct ttysubwindow  *ttysw;
    register struct inputevent    *event;
    int                   multi;
{
    register Seln_rank             sel_received;
    register struct ttyselection  *ttysel;
    struct textselpos     tspb, tspe;
    struct timeval        td;

    if (!ttysw_getopt((caddr_t)ttysw, TTYOPT_SELSVC)) {
	sel_received = SELN_PRIMARY;
    } else {
	sel_received =
	    seln_acquire(ttysw->ttysw_seln_client, SELN_UNSPECIFIED);
    }
    if (sel_received == SELN_PRIMARY) {
	ttysel = &ttysw->ttysw_primary;
	if (ttysw_getopt((caddr_t)ttysw, TTYOPT_SELSVC)) {
	    ttysel_acquire(ttysw, SELN_CARET);
	    
	}
	if (ttysw->ttysw_secondary.sel_made) {
	    (void)ttysel_cancel(ttysw, SELN_SECONDARY);	/* insurance  */
	}
    } else if (sel_received == SELN_SECONDARY) {
	ttysel = &ttysw->ttysw_secondary;
    } else {
	return;
    }
    ttysel_resolve(&tspb, &tspe, SEL_CHAR, event);
    if (multi && ttysel->sel_made) {
	tvsub(&td, &event->ie_time, &ttysel->sel_time);
	if (ttysel_insel(ttysel, &tspe) && timercmp(&td, &maxinterval, <)) {
	    (void)ttysel_adjust(ttysw, event, 1);
	    return;
	}
    }
    if (ttysel->sel_made)
	ttysel_deselect(ttysel, sel_received);
    ttysel->sel_made = TRUE;
    ttysel->sel_begin = tspb;
    ttysel->sel_end = tspe;
    ttysel->sel_time = event->ie_time;
    ttysel->sel_level = SEL_CHAR;
    ttysel->sel_anchor = 0;
    ttysel->sel_null = FALSE;
    ttyhiliteselection(ttysel, sel_received);
}

/*
 * Move the current selection. 
 */
void
ttysel_move(ttysw, event)
    register struct ttysubwindow  *ttysw;
    register struct inputevent    *event;
{
    register Seln_rank             rank;
    register struct ttyselection  *ttysel;
    struct textselpos     tspb, tspe;

    if (ttysw->ttysw_secondary.sel_made) {
	rank = SELN_SECONDARY;
	ttysel = &ttysw->ttysw_secondary;
    } else if (ttysw->ttysw_primary.sel_made) {
	rank = SELN_PRIMARY;
	ttysel = &ttysw->ttysw_primary;
    } else {
	return;
    }
    ttysel_resolve(&tspb, &tspe, ttysel->sel_level, event);
    ttyhiliteselection(ttysel, rank);
    ttysel->sel_begin = tspb;
    ttysel->sel_end = tspe;
    ttysel->sel_time = event->ie_time;
    ttysel->sel_anchor = 0;
    ttysel->sel_null = FALSE;
    ttyhiliteselection(ttysel, rank);
}

/*
 * Adjust the current selection according to the event.
 * If multi is set, check for multi-click. 
 */
ttysel_adjust(ttysw, event, multi)
    register struct ttysubwindow  *ttysw;
    struct inputevent    *event;
    int                   multi;
{
    register struct textselpos *tb;
    register struct textselpos *te;
    Seln_rank             rank;
    int                   count;
    int                   extend = 0;
    struct textselpos     tspc, tspb, tspe, tt;
    struct ttyselection  *ttysel;
    struct timeval        td;

    if (ttysw->ttysw_secondary.sel_made) {
	rank = SELN_SECONDARY;
	ttysel = &ttysw->ttysw_secondary;
    } else if (ttysw->ttysw_primary.sel_made) {
	rank = SELN_PRIMARY;
	ttysel = &ttysw->ttysw_primary;
    } else {
	return;
    }
    tb = &ttysel->sel_begin;
    te = &ttysel->sel_end;
    if (!ttysel->sel_made || ttysel->sel_null)
	return;
    ttysel_resolve(&tspb, &tspc, SEL_CHAR, event);
    if (multi) {
	tvsub(&td, &event->ie_time, &ttysel->sel_time);
	if (ttysel_insel(ttysel, &tspc) && timercmp(&td, &maxinterval, <)) {
	    extend = 1;
	    if (++ttysel->sel_level > SEL_MAX) {
		ttysel->sel_level = SEL_CHAR;
		extend = 0;
	    }
	}
	ttysel->sel_time = event->ie_time;
	ttysel->sel_anchor = 0;
    }
    ttysel_resolve(&tspb, &tspe, ttysel->sel_level, event);
    /*
     * If inside current selection, pull in closest end. 
     */
    if (!extend && ttysel_insel(ttysel, &tspc)) {
	int                   left_end, right_end;

	if (ttysel->sel_anchor == 0) {
	    /* count chars to left */
	    count = 0;
	    tt = *te;
	    *te = tspc;
	    ttyenumerateselection(ttysel, ttycountchars, (char *)(&count));
	    *te = tt;
	    left_end = count;
	    /* count chars to right */
	    count = 0;
	    tt = *tb;
	    *tb = tspc;
	    ttyenumerateselection(ttysel, ttycountchars, (char *)(&count));
	    *tb = tt;
	    right_end = count;
	    if (right_end <= left_end)
		ttysel->sel_anchor = -1;
	    else
		ttysel->sel_anchor = 1;
	}
	if (ttysel->sel_anchor == -1) {
	    if (!ttysel_eq(te, &tspe)) {
		/* pull in right end */
		tt = *tb;
		*tb = tspe;
		tb->tsp_col++;
		ttyhiliteselection(ttysel, rank);
		*tb = tt;
		*te = tspe;
	    }
	} else {
	    if (!ttysel_eq(tb, &tspb)) {
		/* pull in left end */
		tt = *te;
		*te = tspb;
		te->tsp_col--;
		ttyhiliteselection(ttysel, rank);
		*te = tt;
		*tb = tspb;
	    }
	}
    } else {
	/*
	 * Determine which end to extend. Both ends may extend if selection
	 * level has increased. 
	 */
	int                   newanchor = 0;

	if (tspe.tsp_row > te->tsp_row ||
	    tspe.tsp_row == te->tsp_row && tspe.tsp_col > te->tsp_col) {
	    /* extend right end */
	    if (ttysel->sel_anchor == 1) {
		ttysel->sel_anchor = -1;
		ttyhiliteselection(ttysel, rank);
		*tb = *te;
		ttyhiliteselection(ttysel, rank);
	    } else if (ttysel->sel_anchor == 0)
		newanchor = -1;
	    tt = *tb;
	    *tb = *te;
	    tb->tsp_col++;	       /* check for overflow? */
	    *te = tspe;
	    ttyhiliteselection(ttysel, rank);
	    *tb = tt;
	}
	if (tspb.tsp_row < tb->tsp_row ||
	    tspb.tsp_row == tb->tsp_row && tspb.tsp_col < tb->tsp_col) {
	    /* extend left end */
	    if (ttysel->sel_anchor == -1) {
		ttysel->sel_anchor = 1;
		ttyhiliteselection(ttysel, rank);
		*te = *tb;
		ttyhiliteselection(ttysel, rank);
	    } else if (ttysel->sel_anchor == 0)
		if (newanchor == 0)
		    newanchor = 1;
		else
		    newanchor = 0;
	    tt = *te;
	    *te = *tb;
	    te->tsp_col--;	       /* check for underflow? */
	    *tb = tspb;
	    ttyhiliteselection(ttysel, rank);
	    *te = tt;
	}
	if (ttysel->sel_anchor == 0)
	    ttysel->sel_anchor = newanchor;
    }
}

/*
 * Clear out the current selection. 
 */
ttysel_cancel(ttysw, rank)
    register struct ttysubwindow  *ttysw;
    Seln_rank             rank;
{
    struct ttyselection  *ttysel;

    switch (rank)  {
      case SELN_CARET:
	ttysel = &ttysw->ttysw_caret;
	break;
      case SELN_PRIMARY:
	ttysel = &ttysw->ttysw_primary;
	break;
      case SELN_SECONDARY:
	ttysel = &ttysw->ttysw_secondary;
	break;
      case SELN_SHELF:
	ttysel = &ttysw->ttysw_shelf;
	break;
      default:
	return;
    }
    if (!ttysel->sel_made)
	return;
    ttysel_deselect(ttysel, rank);
    ttysel->sel_made = FALSE;
    if (!ttysw_getopt((caddr_t)ttysw, TTYOPT_SELSVC))
	(void)seln_done(ttysw->ttysw_seln_client, rank);
}

/* XXX - compatibility kludge */
void
ttynullselection(ttysw)
    struct ttysubwindow	*ttysw;
{
    (void)ttysel_cancel(ttysw, SELN_PRIMARY);
}

/*
 * Remove a selection from the screen 
 */
void
ttysel_deselect(ttysel, rank)
    register struct ttyselection  *ttysel;
    Seln_rank             rank;
{
    if (!ttysel->sel_made)
	return;
    ttyhiliteselection(ttysel, rank);
    if (!ttysel->sel_null)
	ttysel_empty(ttysel);
}

/*
 * check all selections, and hilite any that exist 
 */
void
ttysel_hilite(ttysw)
    register struct ttysubwindow  *ttysw;
{
/*    struct ttyselection  *ttysel;  Delete this? */

    if (ttysw->ttysw_primary.sel_made)
	ttyhiliteselection(&ttysw->ttysw_primary, SELN_PRIMARY);
    if (ttysw->ttysw_secondary.sel_made)
	ttyhiliteselection(&ttysw->ttysw_secondary, SELN_SECONDARY);
}

/*
 * Hilite a selection.
 * Enumerate all the lines of the selection; hilite each one as appropriate. 
 */
void
ttyhiliteselection(ttysel, rank)
    register struct ttyselection  *ttysel;
    Seln_rank             rank;
{
    struct pr_size        offsets;

    if (!ttysel->sel_made || ttysel->sel_null) {
	return;
    }
    switch (rank) {
      case SELN_SECONDARY:
	offsets.x = (chrheight + chrbase) / 2;
	offsets.y = 1;
	break;
      case SELN_PRIMARY:
	offsets.x = 0;
	offsets.y = chrheight;
    }
    ttyenumerateselection(ttysel, ttyhiliteline, (char *)(&offsets));
}

/*
 * Set local selection as global one. 
 */
void
ttysetselection(ttysw)
    register struct ttysubwindow  *ttysw;
{
    int                   count;
    struct selection      selection;
    struct ttyselection  *ttysel;

    if (ttysw->ttysw_secondary.sel_made ||
	!ttysw->ttysw_primary.sel_made)
	return;
    ttysel = &ttysw->ttysw_primary;
    ttysel_ttysel = ttysel;	       /* stash for ttysel_write	 */
    selection = selnull;
    selection.sel_type = SELTYPE_CHAR;
    count = 0;
    ttyenumerateselection(ttysel, ttycountchars, (char *)(&count));
    selection.sel_items = count;
    selection.sel_itembytes = 1;
    selection.sel_pubflags = SEL_PRIMARY;
    selection.sel_privdata = 0;
    (void)selection_set(&selection, (int (*)())(LINT_CAST(ttysel_write)), 
    	(int (*)())0, ttysw->ttysw_wfd);
}

/*
 * Read global selection into input stream. 
 */
void
ttygetselection(ttysw)
    struct ttysubwindow  *ttysw;
{
    ttysel_ttysw = ttysw;	       /* stash for ttysel_read	 */
    (void)selection_get((int (*)())(LINT_CAST(ttysel_read)), ttysw->ttysw_wfd);
}


/* internal (static) routines	 */

/*
 *	convert from a selection rank to a ttysel struct
 */
static struct ttyselection *
ttysel_from_rank(ttysw, rank)
    struct ttysubwindow  *ttysw;
    Seln_rank		  rank;
{
    switch (rank) {
      case SELN_CARET:
	return &ttysw->ttysw_caret;
      case SELN_PRIMARY:
	return &ttysw->ttysw_primary;
      case SELN_SECONDARY:
	return &ttysw->ttysw_secondary;
      case SELN_SHELF:
	return &ttysw->ttysw_shelf;
      default:
	break;
    }
    return (struct ttyselection *) NULL;
}

/*
 * Make a selection be empty
 */
static void
ttysel_empty(ttysel)
    register struct ttyselection  *ttysel;
{
    ttysel->sel_null = TRUE;
    ttysel->sel_level = SEL_CHAR;
    ttysel->sel_begin = tse_null_extent;
    ttysel->sel_end = tse_null_extent;
}

/*
 * Is the specified position within the current selection? 
 */
static int
ttysel_insel(ttysel, tsp)
    struct ttyselection  *ttysel;
    register struct textselpos *tsp;
{
    register struct textselpos *tb = &ttysel->sel_begin;
    register struct textselpos *te = &ttysel->sel_end;

    if (tsp->tsp_row < tb->tsp_row || tsp->tsp_row > te->tsp_row)
	return (0);
    if (tb->tsp_row == te->tsp_row)
	return (tsp->tsp_col >= tb->tsp_col &&
		tsp->tsp_col <= te->tsp_col);
    if (tsp->tsp_row == tb->tsp_row)
	return (tsp->tsp_col >= tb->tsp_col);
    if (tsp->tsp_row == te->tsp_row)
	return (tsp->tsp_col <= te->tsp_col);
    return (1);
}

static int
ttysel_eq(t1, t2)
    register struct textselpos *t1, *t2;
{

    return (t1->tsp_row == t2->tsp_row && t1->tsp_col == t2->tsp_col);
}

static void
tvsub(tdiff, t1, t0)
    register struct timeval       *tdiff, *t1, *t0;
{

    tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
    tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
    if (tdiff->tv_usec < 0)
	tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

static void
ttyenumerateselection(ttysel, proc, data)
    struct ttyselection 	  *ttysel;
    register void                 (*proc) ();
    register char                 *data;
{
    struct textselpos		  *xbegin, *xend;
    register struct textselpos    *begin, *end;
    register int                   row;

    if (!ttysel->sel_made || ttysel->sel_null)
	return;
    /*
     * Sort extents 
     */
    ttysortextents(ttysel, &xbegin, &xend);
    begin = xbegin;
    end = xend;
    /*
     * Process a line at a time 
     */
    for (row = begin->tsp_row; row <= end->tsp_row; row++) {
	if (row == begin->tsp_row && row == end->tsp_row) {
	    /*
	     * Partial line hilite in middle 
	     */
	    proc(begin->tsp_col, end->tsp_col, row, data);
	} else if (row == begin->tsp_row) {
	    /*
	     * Partial line hilite from beginning 
	     */
	    proc(begin->tsp_col, length(image[row]), row, data);
	} else if (row == end->tsp_row) {
	    /*
	     * Partial line hilite not to end 
	     */
	    proc(0, end->tsp_col, row, data);
	} else {
	    /*
	     * Full line hilite 
	     */
	    proc(0, length(image[row]), row, data);
	}
    }
}

static void
ttyhiliteline(start, finish, row, offsets)
    int                   start, finish, row;
    struct pr_size       *offsets;
{
    struct rect           r;

    rect_construct(&r, col_to_x(start),
		   row_to_y(row) + offsets->x,
		   col_to_x(finish + 1) - col_to_x(start),
		   offsets->y);
    if (r.r_width == 0)
	return;
    (void)pselectionhilite(&r);
}

static void
ttysel_resolve(tb, te, level, event)
    register struct textselpos    *tb, *te;
    int                   level;
    struct inputevent    *event;
{
    register unsigned char        *line;

    tb->tsp_row = y_to_row(event->ie_locy);
    if (tb->tsp_row >= bottom)
	tb->tsp_row = max(0, bottom - 1);
    line = image[tb->tsp_row];
    tb->tsp_col = x_to_col(event->ie_locx);
    if (tb->tsp_col > length(line))
	tb->tsp_col = length(line);
    *te = *tb;
    switch (level) {
      case SEL_CHAR:
	break;
      case SEL_WORD:{
	    register int          col, chr;

	    for (col = te->tsp_col; col < length(line); col++) {
		chr = line[col];
		if (!isalnum(chr) && chr != '_')
		    break;
	    }
	    te->tsp_col = max(col - 1, tb->tsp_col);
	    for (col = tb->tsp_col; col >= 0; col--) {
		chr = line[col];
		if (!isalnum(chr) && chr != '_')
		    break;
	    }
	    tb->tsp_col = min(col + 1, te->tsp_col);
	    break;
	}
      case SEL_LINE:
	tb->tsp_col = 0;
	te->tsp_col = length(line) - 1;
	break;
      case SEL_PARA:{
	    register int          row;

	    for (row = tb->tsp_row; row >= top; row--)
		if (length(image[row]) == 0)
		    break;
	    tb->tsp_row = min(tb->tsp_row, row + 1);
	    tb->tsp_col = 0;
	    for (row = te->tsp_row; row < bottom; row++)
		if (length(image[row]) == 0)
		    break;
	    te->tsp_row = max(te->tsp_row, row - 1);
	    te->tsp_col = length(image[te->tsp_row]);
	    break;
	}
    }
}

static void
ttysortextents(ttysel, begin, end)
    register struct ttyselection  *ttysel;
    struct textselpos   **begin, **end;
{

    if (ttysel->sel_begin.tsp_row == ttysel->sel_end.tsp_row) {
	if (ttysel->sel_begin.tsp_col > ttysel->sel_end.tsp_col) {
	    *begin = &ttysel->sel_end;
	    *end = &ttysel->sel_begin;
	} else {
	    *begin = &ttysel->sel_begin;
	    *end = &ttysel->sel_end;
	}
    } else if (ttysel->sel_begin.tsp_row > ttysel->sel_end.tsp_row) {
	*begin = &ttysel->sel_end;
	*end = &ttysel->sel_begin;
    } else {
	*begin = &ttysel->sel_begin;
	*end = &ttysel->sel_end;
    }
}

static void
ttycountchars(start, finish, row, count)
    register int          start, finish, row, *count;
{

    *count += finish + 1 - start;
    if (length(image[row]) == finish && finish == right) {
	*count -= 1;	/*	no CR on wrapped lines	*/
    }
}

/* ARGSUSED */
static void
ttysel_write(sel, file)
    struct selection     *sel;
    FILE                 *file;
{
    ttyenumerateselection(ttysel_ttysel, ttyputline, (char *)(file));
}

#ifdef lint
#undef putc
#define putc(_char, _file) \
	_file = (FILE *)(_file ? _file : 0)
#endif lint

static void
ttyputline(start, finish, row, file)
    int                   start;
    register int	  finish;
    register int	  row;
    register FILE        *file;
{
    register int          col;

    finish++;
    for (col = start; col < finish; col++) {
	if (length(image[row]) == col) {
	    /*
	     * For full width lines, don't put in CR so can grab command lines
	     * that wrap. 
	     */
	    if (col != right) {
		putc('\n', file);
	    }
	} else {
	    putc(*(image[row] + col), file);
	}
    }
}

static void
ttysel_read(sel, file)
    struct selection     *sel;
    register FILE        *file;
{
    register int          buf;
    char                  c;

    if (sel->sel_type != SELTYPE_CHAR || sel->sel_itembytes != 1)
	return;
    while ((buf = getc(file)) != EOF) {
	c = buf;		   /* goddamn possibly-signed characters!  */
	(void) ttysw_input((caddr_t)ttysel_ttysw, &c, 1);
    }
    (void)ttysw_reset_conditions(ttysel_ttysw);
}

/*
 * Do_function: respond to notification that a function key went up 
 */
static void
ttysel_function(ttysw, buffer)
    register struct ttysubwindow *ttysw;
    register Seln_function_buffer *buffer;
{
    Seln_response         response;
    Seln_holder          *holder;

    response = seln_figure_response(buffer, &holder);
    switch (response) {
      case SELN_IGNORE:
	return;
      case SELN_DELETE:
      case SELN_FIND:
	break;
      case SELN_REQUEST:
	if (ttysw->ttysw_seln_client == (char *) NULL) {
	    ttygetselection(ttysw);
	} else {
	    (void)ttysel_get_selection(ttysw, holder);
	}
	if (holder->rank == SELN_SECONDARY) {
	    ttysel_end_request(ttysw, holder, SELN_SECONDARY);
	}
	break;
      case SELN_SHELVE: {
	    FILE                 *held_file;
	    struct ttyselection  *ttysel;

	    ttysel = ttysel_from_rank(ttysw, SELN_PRIMARY);
	    if (!ttysel->sel_made) {
		return;
	    }
	    if ((held_file = fopen(ttysel_filename, "w")) == (FILE *) NULL) {
		return;
	    }
	    (void) fchmod (fileno(held_file), 00666);  /* Allowing world to read and write */
	    ttyenumerateselection(ttysel, ttyputline, (char *)(held_file));
	    (void)fclose(held_file);
	    (void)seln_hold_file(SELN_SHELF, ttysel_filename);
	    break;
	}
      default:
	(void) fprintf(stderr,
		"ttysw didn't recognize function to perform on selection\n");
	break;
    }
    ttysel_resynch(ttysw, buffer);
    if (buffer->addressee_rank == SELN_SECONDARY) {
	(void)ttysel_cancel(ttysw, buffer->addressee_rank);
    }
}

ttysel_get_selection(ttysw, holder)
    Ttysw		 *ttysw;
    Seln_holder		 *holder;
{
    struct ttysel_context context;

    context.continued = FALSE;
    context.ttysw = ttysw;
    (void) seln_query(holder, ttysel_copy_in, (char *)(LINT_CAST(&context)),
		      SELN_REQ_BYTESIZE, 0,
		      SELN_REQ_CONTENTS_ASCII, 0,
		      0);
}

/*
 * Do the action for "Put, then Get" menu item.
 */
void
ttysw_do_put_get(ttysw)
    register Ttysw *ttysw;
{
    Seln_holder		  holder;
    Seln_function_buffer  buffer;

    /*
     *  if there is a nonzero length primary selection,
     *      do put_then_get:
     *        copy the primary selection to the tty cursor,
     *        then fake Put down, up.
     *        NOTE: can't Put, then Get from shelf because of **race**
     *  else
     *      do get_only:
     *        copy the shelf to the tty cursor.
     */
    if (ttysw_is_seln_nonzero(ttysw, SELN_PRIMARY)) {
	holder = seln_inquire(SELN_PRIMARY);
	(void)ttysel_get_selection(ttysw, &holder);
	(void)seln_inform(ttysw->ttysw_seln_client, SELN_FN_PUT, TRUE);
	buffer = seln_inform(ttysw->ttysw_seln_client, SELN_FN_PUT, FALSE);
	if (buffer.function != SELN_FN_ERROR &&
	    ttysw->ttysw_seln_client != NULL) {
	    ttysel_function(ttysw, &buffer);
	}
    } else if (ttysw_is_seln_nonzero(ttysw, SELN_SHELF)) {
	holder = seln_inquire(SELN_SHELF);
	(void)ttysel_get_selection(ttysw, &holder);
    }
}

/*
 * Reply:  respond to a request sent from another process 
 */
static Seln_result
ttysel_reply(request, context, buffer_length)
    Seln_attribute        request;
    register
    Seln_replier_data    *context;
    int                   buffer_length;
{
    unsigned              count;
    struct ttysubwindow  *ttysw;
    struct ttyselection  *ttysel;

    ttysw = (struct ttysubwindow *)LINT_CAST(context->client_data);
    ttysel = ttysel_from_rank(ttysw, context->rank);
    if (!ttysel->sel_made) {
	return SELN_DIDNT_HAVE;
    }
    switch (request) {
      case SELN_REQ_BYTESIZE:
	if (buffer_length < sizeof (char **)) {
	    return SELN_FAILED;
	}
	count = 0;
	if (!ttysel->sel_null) {
	    ttyenumerateselection(ttysel, ttycountchars, (char *)(&count));
	}
	*context->response_pointer++ = (char *) count;
	return SELN_SUCCESS;
      case SELN_REQ_CONTENTS_ASCII:
	if (ttysel->sel_null) {
	    *context->response_pointer++ = (char *) 0;
	    return SELN_SUCCESS;
	}
	return ttysel_copy_out(ttysel, context, buffer_length);
      case SELN_REQ_YIELD:
	if (buffer_length < sizeof (char **)) {
	    return SELN_FAILED;
	}
	if (!ttysel->sel_made) {
	    *context->response_pointer++ = (char *) SELN_DIDNT_HAVE;
	} else {
	    (void)ttysel_cancel(ttysw, context->rank);
	    *context->response_pointer++ = (char *) SELN_SUCCESS;
	}
	return SELN_SUCCESS;
      default:
	return SELN_UNRECOGNIZED;
    }
}

/*
 *  Send_deselect: tell another process to deselect
 */
static void
ttysel_end_request(ttysw, addressee, rank)
    struct ttysubwindow  *ttysw;
    Seln_holder		 *addressee;
    Seln_rank		  rank;
{
    Seln_request	  buffer;

    if (seln_holder_same_client(addressee, (char *)ttysw))  {
	(void)ttysel_cancel(ttysw, rank);
	return;
    }
    seln_init_request(&buffer, addressee, SELN_REQ_COMMIT_PENDING_DELETE,
		      SELN_REQ_YIELD, 0, 0);
    (void) seln_request(addressee, &buffer);
}

static Seln_result
ttysel_copy_out(ttysel, context, max_length)
    register struct ttyselection *ttysel;
    register Seln_replier_data *context;
    register int          max_length;
{
    register int            curr_col, curr_row, index, row_len;
    register unsigned char *dest, *src;
    int                     start_col, end_col, last_row;

    if (context->context != (char *) NULL) {
	ttysel = (struct ttyselection *) LINT_CAST(context->context);
    }
    start_col = ttysel->sel_begin.tsp_col;
    end_col = ttysel->sel_end.tsp_col;
    last_row = ttysel->sel_end.tsp_row;
    dest = (unsigned char *) context->response_pointer;

    curr_col = start_col;
    curr_row = ttysel->sel_begin.tsp_row;
    while (curr_row < last_row) {
	row_len = min(length(image[curr_row]) - curr_col, max_length);
	index = row_len;
	src = image[curr_row] + curr_col;
	while (index--) {
	    *dest++ = *src++;
	}
	if ((max_length -= row_len) <= 0) {
	    goto continue_reply;
	}
	if (row_len + curr_col != right) {
	    *dest++ = '\n';
	    max_length--;
	}
	curr_col = 0;
	curr_row += 1;
    }
    row_len = min(end_col + 1 - curr_col, max_length);
    index = row_len;
    src = image[curr_row] + curr_col;
    while (index--) {
	*dest++ = *src++;
    }
    if ((max_length -= row_len) <= 0) {
	goto continue_reply;
    }
    if (end_col == length(image[curr_row]) && end_col < right) {
	dest[-1] = '\n';
	*dest = '\0';
	max_length--;
    }
    /* round up to word boundary  */
    while ((unsigned) dest % 4 != 0) {
	*dest++ = '\0';
    }

    context->response_pointer = (char **) LINT_CAST(dest);
    *context->response_pointer++ = 0;
    if (context->context != (char *) NULL) {
	free(context->context);
    }
    return SELN_SUCCESS;

continue_reply:
    {
	register struct ttyselection *ttysel2;

	if (context->context == (char *) NULL) {
	    ttysel2 =
		(struct ttyselection *)
		LINT_CAST(malloc(sizeof(struct ttyselection)));
	    if (ttysel2 == (struct ttyselection *) NULL) {
		(void) fprintf(stderr, "malloc failed for selection copy-out");
		return SELN_FAILED;
	    }
	    *ttysel2 = *ttysel;
	} else {
	    ttysel2 = (struct ttyselection *) LINT_CAST(context->context);
	}
	ttysel2->sel_begin.tsp_row = curr_row;
	ttysel2->sel_begin.tsp_col = curr_col + row_len;
	ttysel2->sel_end.tsp_row = last_row;
	ttysel2->sel_end.tsp_col = end_col;
	context->context = (char *) ttysel2;
	context->response_pointer = (char **) LINT_CAST(dest);
	return SELN_CONTINUED;
    }
}


static Seln_result
ttysel_copy_in(buffer)
    register Seln_request *buffer;
{
    register Seln_attribute *response_ptr;
    register struct ttysel_context *context;
    struct ttysubwindow  *ttysw;
    register int          current_size;

    if (buffer == (Seln_request *) NULL) {
	return SELN_UNRECOGNIZED;
    }
    context = (struct ttysel_context *) LINT_CAST(buffer->requester.context);
    ttysw = context->ttysw;
    response_ptr = (Seln_attribute *) LINT_CAST(buffer->data);
    if (!context->continued) {
	if (*response_ptr++ != SELN_REQ_BYTESIZE) {
	    return SELN_FAILED;
	}
	context->bytes_left = (int) *response_ptr++;
	current_size = min(context->bytes_left,
			   buffer->buf_size - 3 * sizeof (Seln_attribute));
	if (*response_ptr++ != SELN_REQ_CONTENTS_ASCII) {
	    return SELN_FAILED;
	}
    } else {
	current_size = min(context->bytes_left, buffer->buf_size);
    }
    (void) ttysw_input((caddr_t)ttysw, (char *) response_ptr, current_size);
    (void)ttysw_reset_conditions(ttysw);
    if (buffer->status == SELN_CONTINUED) {
	context->continued = TRUE;
	context->bytes_left -= current_size;
    }
    return SELN_SUCCESS;
}

static
ttysel_resynch(ttysw, buffer)
    register struct ttysubwindow *ttysw;
    register Seln_function_buffer *buffer;
{
    if (ttysw->ttysw_caret.sel_made &&
	!seln_holder_same_client(&buffer->caret, (char *)ttysw)) {
	ttysel_deselect(&ttysw->ttysw_caret, SELN_CARET);
	ttysw->ttysw_caret.sel_made = FALSE;
    }
    if (ttysw->ttysw_primary.sel_made &&
	!seln_holder_same_client(&buffer->primary, (char *)ttysw)) {
	ttysel_deselect(&ttysw->ttysw_primary, SELN_PRIMARY);
	ttysw->ttysw_primary.sel_made = FALSE;
    }
    if (ttysw->ttysw_secondary.sel_made &&
	!seln_holder_same_client(&buffer->secondary, (char *)ttysw)) {
	ttysel_deselect(&ttysw->ttysw_secondary, SELN_SECONDARY);
	ttysw->ttysw_secondary.sel_made = FALSE;
    }
    if (ttysw->ttysw_shelf.sel_made &&
	!seln_holder_same_client(&buffer->shelf, (char *)ttysw)) {
	ttysel_deselect(&ttysw->ttysw_shelf, SELN_SHELF);
	ttysw->ttysw_shelf.sel_made = FALSE;
    }
}
