#ifndef lint
static  char sccsid[] = "@(#)estream.c 1.1 92/08/05 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Entity stream implementation for permitting veto of insertion into
 * a piece stream.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <suntool/primal.h>
#include <suntool/entity_stream.h>

extern char	*malloc();

static Es_index  ts_replace();
static int	 ts_set();

static void	 make_current_valid();

static struct es_ops	*ps_ops;
static struct es_ops	 ts_ops;

Es_handle
ts_create(ttysw, original, scratch)
	char			*ttysw;
	Es_handle		 original, scratch;
{
	extern Es_handle	 ps_create();
	Es_handle		 piece_stream;
	
	piece_stream = ps_create(ttysw, original, scratch);
	if (piece_stream) {
	    if (ps_ops == 0) {
		ps_ops = piece_stream->ops;
		ts_ops = *ps_ops;
		ts_ops.replace = ts_replace;
		ts_ops.set = ts_set;
	    }
	    piece_stream->ops = &ts_ops;
	}
	return(piece_stream);
}

static Es_index
ts_replace(esh, last_plus_one, count, buf, count_used)
	Es_handle	 esh;
	int		 last_plus_one, count,
			*count_used;
	char		*buf;

{
	Es_index pos;
	
	pos = es_get_position(esh);	
	tmpremoveglyph();	/* take down glyph */
	invalidate(pos);	/* reset char to lineno cacheing */
	es_set_position(esh, pos);
	return (ps_ops->replace(esh, last_plus_one, count, buf, count_used));
}

static int
ts_set(esh, attr_argv)
	Es_handle	 esh;
	caddr_t		*attr_argv;
{
	caddr_t			*attrs;
	Es_index pos;
	
	for (attrs = attr_argv; *attrs; attrs = attr_next(attrs)) {
	    if ((Es_attribute)*attrs == ES_HANDLE_TO_INSERT) {
		    pos = es_get_position(esh);	
		    tmpremoveglyph();	/* take down glyph */
		    invalidate(pos);	/* reset char to lineno cacheing */
		    es_set_position(esh, pos);
	    }
	}
	return (ps_ops->set(esh, attr_argv));
}
