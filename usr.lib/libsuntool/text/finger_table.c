#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)finger_table.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Utilities for manipulation of finger tables.
 *
 * See finger_table.h for descriptions of what the routines do.
 */

#include <sunwindow/rect.h>
#include <suntool/primal.h>
#include <suntool/finger_table.h>
#include "sunwindow/sv_malloc.h"

static void
ft_validate_first_infinity(finger_table)
	register ft_handle  finger_table;
{
	register Es_index  *seq_i_addr = 0;
	register int	    addr_delta = finger_table->sizeof_element;
	register int	    first_infinity = finger_table->first_infinity;
	int		    save_last_bounding_index;

	if (first_infinity < finger_table->last_plus_one) {
	    seq_i_addr = FT_ADDR(finger_table, first_infinity, addr_delta);
	    if (*seq_i_addr == ES_INFINITY) {
		while ((first_infinity > 0) &&
		       (seq_i_addr = FT_PREV_ADDR(seq_i_addr, addr_delta)) &&
		       (*seq_i_addr == ES_INFINITY)) {
		    first_infinity--;
		}
	    } else if ((seq_i_addr = FT_NEXT_ADDR(seq_i_addr, addr_delta)) &&
		       ((*seq_i_addr) == ES_INFINITY)) {
		first_infinity++;
	    } else {
		seq_i_addr = 0;
	    }
	}
	if (seq_i_addr == 0) {
	    save_last_bounding_index = finger_table->last_bounding_index;
	    first_infinity =
		ft_bounding_index(finger_table, ES_INFINITY-1);
	    if (first_infinity < finger_table->last_plus_one)
		first_infinity++;
	    finger_table->last_bounding_index = save_last_bounding_index;
	}
	finger_table->first_infinity = first_infinity;
}

ft_add_delta(finger_table, from, delta)
	ft_object	finger_table;
	int		from;
	long int	delta;
{
	register int		 ft_index;
	register Es_index	*seq_i_addr;
	register int		 addr_delta = finger_table.sizeof_element;

	if (from >= finger_table.last_plus_one)
		return;
	seq_i_addr = FT_ADDR(&finger_table, from, addr_delta);
	for (ft_index=from; ft_index < finger_table.last_plus_one; ft_index++) {
		if (*seq_i_addr == ES_INFINITY)
			break;
		*seq_i_addr = *seq_i_addr + delta;
		seq_i_addr = FT_NEXT_ADDR(seq_i_addr, addr_delta);
	}
}

ft_object	
ft_create(last_plus_one, sizeof_client_data)
	int		last_plus_one, sizeof_client_data;
{
	ft_object	result;

	/* Guarantee that result.sizeof_element meets all alignment
	 * restrictions by adding trailing padding when necessary.
	 */
	result.sizeof_element = sizeof (Es_index) + sizeof_client_data;
	while (result.sizeof_element % (sizeof (Es_index)))
		result.sizeof_element++;
	result.last_plus_one = last_plus_one;
	result.seq = (Es_index *) LINT_CAST(sv_calloc(
		(unsigned)last_plus_one + 1, result.sizeof_element));
	result.last_bounding_index = 0;
	result.first_infinity = 0;
	return(result);
}

ft_destroy(table)
	ft_handle	table;
{
	free((char *)table->seq);
	table->seq = 0;
	table->last_plus_one = 0;
}

void
ft_expand(table, by)
	register ft_handle	 table;
	int			 by;
{
	int			 old_last_plus_one = table->last_plus_one;

	if (by == 0)
	    return;
	table->last_plus_one += by;
        table->seq = (Es_index *)sv_realloc((char*)table->seq,
                (unsigned)table->last_plus_one * (unsigned)table->sizeof_element);
	if (by > 0) {
	    ft_set(*table, old_last_plus_one, table->last_plus_one,
		ES_INFINITY, (char *)0);
	}
}

ft_shift_up(table, first, last_plus_one, expand_by)
	register ft_handle	 table;
	int			 first, last_plus_one, expand_by;
{
	register int		 addr_delta = table->sizeof_element,
				 shift_count, ft_index, stop_plus_one;
	register Es_index	*seq_i_addr;

	ft_validate_first_infinity(table);
	if (expand_by > 0) {
	    stop_plus_one = table->last_plus_one - (last_plus_one-1-first);
	    if (table->first_infinity >= stop_plus_one) {
		ft_expand(table, expand_by);
	    }
	}
	shift_count = table->first_infinity - first;
	shift_count = min(shift_count, table->last_plus_one - last_plus_one);
	if (shift_count != 0) {
	    seq_i_addr = FT_ADDR(table, first, addr_delta);
	    bcopy((char *)seq_i_addr,
		((char *)FT_ADDR(table, last_plus_one, addr_delta)),
		addr_delta*(shift_count));
	}
	if (table->first_infinity < table->last_plus_one)
	    table->first_infinity += (last_plus_one-first);
}

ft_shift_out(table, first, last_plus_one)
	ft_handle	table;
	int		first, last_plus_one;
{
	register int		 addr_delta = table->sizeof_element,
				 to_move;
	register char		*first_addr, *lpo_addr;
	register Es_index	*seq_i_addr;

	ft_validate_first_infinity(table);
	if (last_plus_one < table->first_infinity) {
	    to_move = table->first_infinity - last_plus_one;
	    first_addr = ((char *)table->seq + first*addr_delta);
	    lpo_addr = ((char *)table->seq + last_plus_one*addr_delta);
	    bcopy(lpo_addr, first_addr, to_move*addr_delta);
	} else
	    to_move = 0;
	ft_set(*table, first+to_move, table->first_infinity,
		ES_INFINITY, (char *)0);
	table->first_infinity = first + to_move;
}

ft_set(finger_table, first, last_plus_one, to, client_data)
	ft_object		 finger_table;
	int			 first, last_plus_one;
	Es_index		 to;
	char			*client_data;
{
	register int		 ft_index;
	register Es_index	*seq_i_addr;
	register int		 addr_delta = finger_table.sizeof_element;

	if (first >= finger_table.last_plus_one)
	    return;
	seq_i_addr = FT_ADDR(&finger_table, first, addr_delta);
	for (ft_index=first; ft_index < last_plus_one; ft_index++) {
	    *seq_i_addr = to;
	    if (client_data) {
		bcopy(client_data, ((char *)seq_i_addr) + sizeof(Es_index),
		    addr_delta - sizeof(Es_index));
	    }
	    seq_i_addr = FT_NEXT_ADDR(seq_i_addr, addr_delta);
	}
}

ft_set_esi_span(finger_table, first, last_plus_one, to, client_data)
	ft_object		 finger_table;
	Es_index		 first, last_plus_one, to;
	char			*client_data;
{
	register int		 index_of_first = 0, index_of_last_plus_one;
	register Es_index	*seq_i_addr = finger_table.seq;
	register int		 addr_delta = finger_table.sizeof_element;

	if AN_ERROR(finger_table.last_plus_one == 0) {
	    return;
	}
	while (first > *seq_i_addr) {
	    if (++index_of_first == finger_table.last_plus_one)
		return;
	    seq_i_addr = FT_NEXT_ADDR(seq_i_addr, addr_delta);
	}
	index_of_last_plus_one = index_of_first;
	while (last_plus_one > *seq_i_addr) {
	    if (++index_of_last_plus_one == finger_table.last_plus_one)
		break;
	    seq_i_addr = FT_NEXT_ADDR(seq_i_addr, addr_delta);
	}
	ft_set(finger_table, index_of_first, index_of_last_plus_one,
		to, client_data);
}

ft_bounding_index(finger_table, pos)
	register Ft_table	 finger_table;
	Es_index		 pos;
/*   The 3.0 version of this code used linear search with no caching, but a
 * table of pieces can get to be 100-1000 elements long, and then linear
 * search is way too slow.
 */
{
	register Es_index	*seq_i_addr = finger_table->seq;
	register int		 addr_delta = finger_table->sizeof_element;
	register int		 index, start, stop_plus_one;

	stop_plus_one = finger_table->last_plus_one;
	if (pos < *seq_i_addr || stop_plus_one == 0) {
	    index = stop_plus_one;
	    goto Return;
	}
	/* Assert: seq[0] <= pos && 0 < stop_plus_one */
	/* Check the cache */
	index = finger_table->last_bounding_index;
	if (index < stop_plus_one) {
	    seq_i_addr = FT_ADDR(finger_table, index, addr_delta);
	    if (*seq_i_addr <= pos) {
		if (index+1 == stop_plus_one)
		    goto Return;
		seq_i_addr = FT_NEXT_ADDR(seq_i_addr, addr_delta);
		if (pos < *seq_i_addr)
		    goto Return;
	    }
	}
	/* No luck, so do the search */
	index = stop_plus_one-1;
	seq_i_addr = FT_ADDR(finger_table, index, addr_delta);
	if (*seq_i_addr <= pos)
	    goto Return;
	/* Assert: pos < seq[stop_plus_one-1] &&
	 *	   1 < stop_plus_one (else would have goto'd above)
	 */
	start = 0;
	/* Assert: seq[start] <= pos && start+1 == 1 < stop_plus_one */
	FOREVER {
	    index = (start + stop_plus_one) / 2;
	    /* Assert: start+1 <= index <= stop_plus_one-1 */
	    seq_i_addr = FT_ADDR(finger_table, index, addr_delta);
	    if (pos < *seq_i_addr) {
		if (index+1 == stop_plus_one) {
		    /* Assert: start+1 == index (else contradiction), so
		     * seq[start] <= pos < seq[index], and we are done.
		     */
		    index = start;
		    goto Return;
		}
		stop_plus_one = index+1;
		/* Assert: start+1 < index+1 == (new)stop_plus_one */
	    } else {
		start = index;
		/* Assert: (new)start == index < stop_plus_one-1, else
		 * seq[index == stop_plus_one-1] > pos, a contradiction
		 */
	    }
	    /* Assert: seq[start] <= pos < seq[stop_plus_one-1] */
	}
Return:
	finger_table->last_bounding_index = index;
	return(index);
}

ft_index_for_position(finger_table, pos)
	ft_object		finger_table;
	Es_index		pos;
{
	register int		 ft_index;
	register Es_index	*seq_i_addr = finger_table.seq;
	register int		 addr_delta = finger_table.sizeof_element;

	for (ft_index = 0; ft_index < finger_table.last_plus_one; ft_index++) {
		if (*seq_i_addr == pos)
			return(ft_index);
		if (*seq_i_addr > pos)
			break;
		seq_i_addr = FT_NEXT_ADDR(seq_i_addr, addr_delta);
	}
	return(finger_table.last_plus_one);
}

Es_index
ft_position_for_index(finger_table, index)
	ft_object	finger_table;
	int		index;
{
	register Es_index	*seq_i_addr;
	register int		 addr_delta = finger_table.sizeof_element;

	if (index >= finger_table.last_plus_one)
		return(ES_CANNOT_SET);
	seq_i_addr = FT_ADDR(&finger_table, index, addr_delta);
	return(*seq_i_addr);
}

#ifdef DEBUG
fprintf_ft(finger_table)
	ft_object	finger_table;
{
	register Es_index	*seq_i_addr = finger_table.seq;
	register int		 addr_delta = finger_table.sizeof_element;
	register int		 i, cd_i;
	register int		*client_data_ptr;
	FILE			*out_file = stderr;

	if (finger_table.last_plus_one > 999) {
	    (void) fprintf(out_file,
			    "You passed the ft_handle, not the ft_object!\n");
	    return(FALSE);
	}
	(void) fprintf(out_file,
		       "last_plus_one = %d, sizeof_element = %d, seq = 0x%lx\n",
			finger_table.last_plus_one,
			finger_table.sizeof_element,
			finger_table.seq
			);
	(void) fprintf(out_file, "seq[] pos  client_data\n");
	for (i = 0; i < finger_table.last_plus_one; i++) {
	    (void) fprintf(out_file, "%2d  ", i);
	    switch (*seq_i_addr) {
		case ES_INFINITY:
		    (void) fprintf(out_file, "  INF  ");
		    break;
		case ES_CANNOT_SET:
		    (void) fprintf(out_file, " ~SET  ");
		    break;
		default:
		    if (*seq_i_addr < 100000)
			(void) fprintf(out_file, "%5d  ", *seq_i_addr);
		    else
			(void) fprintf(out_file, "%d  ", *seq_i_addr);
		    break;
	    }
	    for (cd_i = 1; cd_i < (addr_delta/(sizeof(*client_data_ptr)));
		 cd_i++) {
		client_data_ptr = ((int *)seq_i_addr);
		client_data_ptr += cd_i;
		(void) fprintf(out_file, "%8X  ", *client_data_ptr);
	    }
	    (void) fprintf(out_file, "\n");
	    seq_i_addr += (addr_delta/sizeof(*seq_i_addr));
	}
	return(TRUE);
}
#endif
