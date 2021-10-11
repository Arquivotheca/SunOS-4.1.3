#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ttysw_entity_stream.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 *  Copyright (c) 1986 by Sun Microsystems Inc.
 */

/*
 * Entity stream implementation for permitting veto of insertion into
 * a piece stream.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <suntool/ttysw.h>
#include <suntool/primal.h>
#include <suntool/entity_stream.h>
#include <suntool/ttysw_impl.h>

extern Cmdsw	*cmdsw;
extern char	*malloc();

static Es_index  ts_replace();
static int	 ts_set();

static void	 make_current_valid();

static struct es_ops	*ps_ops;
static struct es_ops	 ts_ops;

#define	iwbp	ttysw->ttysw_ibuf.cb_wbp
#define	irbp	ttysw->ttysw_ibuf.cb_rbp
#define	iebp	ttysw->ttysw_ibuf.cb_ebp
#define	ibuf	ttysw->ttysw_ibuf.cb_buf
#define	owbp	ttysw->ttysw_obuf.cb_wbp
#define	orbp	ttysw->ttysw_obuf.cb_rbp

Es_handle
ts_create(ttysw, original, scratch)
	Ttysw			*ttysw;
	Es_handle		 original, scratch;
{
	extern Es_handle	 ps_create();
	Es_handle		 piece_stream;
	
	piece_stream = ps_create((char *)(LINT_CAST(ttysw)), original, scratch);
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

#define NO_LOCAL_ECHO(_cmdsw, _textsw, _private, _count) \
	   (!(_cmdsw)->cooked_echo \
	&& (!(_cmdsw)->doing_pty_insert) \
	&& ((_cmdsw)->append_only_log \
	|| ((_count) > 0 \
	&&  es_get_position((_private)) \
	==  textsw_find_mark((_textsw), (_cmdsw)->pty_mark) )))

static Es_index
ts_replace(esh, last_plus_one, count, buf, count_used)
	Es_handle	 esh;
	int		 last_plus_one, count,
			*count_used;
	unsigned char	*buf;

{
	register Ttysw	*ttysw = (Ttysw *)
				 LINT_CAST(es_get(esh, ES_CLIENT_DATA));
	Textsw		 textsw = (Textsw)ttysw->ttysw_hist;

	/*
	 *  if in ![cooked, echo] mode and the caret is at the pty mark,
	 *  and the operation is an insertion,
	 *  then don't locally echo insertions.
	 */
	if (NO_LOCAL_ECHO(cmdsw, textsw, esh, count)) {
	    /* copy buf into iwbp */
	    bcopy(buf, iwbp, min(count, iebp-iwbp));
	    iwbp += min(count, iebp-iwbp);
	    (void)ttysw_reset_conditions(ttysw);
	    (void)es_set(esh, ES_STATUS, ES_REPLACE_DIVERTED, 0);
	    return (ES_CANNOT_SET);
	}
	return (ps_ops->replace(esh, last_plus_one, count, buf, count_used));
}

static int
ts_set(esh, attr_argv)
	Es_handle	 esh;
	caddr_t		*attr_argv;
{
	register Ttysw	*ttysw = (Ttysw *)
				 LINT_CAST(es_get(esh, ES_CLIENT_DATA));
	Textsw			 textsw = (Textsw)ttysw->ttysw_hist;
	caddr_t			*attrs;
	Es_handle		 to_insert;
	u_int			 result;
	
	/* do this only if we're not in cooked echo mode */
	for (attrs = attr_argv; *attrs; attrs = attr_next(attrs)) {
	    if ((Es_attribute)*attrs == ES_HANDLE_TO_INSERT) {
		to_insert = (Es_handle)LINT_CAST(attrs[1]);
		if (NO_LOCAL_ECHO(cmdsw, textsw, esh,
	    			  es_get_length(to_insert)) ) {
		    (void) es_set_position(to_insert, 0);
		    /* Really should loop, in case esh > iebp-iwbp */
		    (void) es_read(to_insert, iebp-iwbp, iwbp, &result);
		    iwbp += result;
		    (void)ttysw_reset_conditions(ttysw);
		    ATTR_CONSUME(*attrs);
		}
	    }
	}
	return (ps_ops->set(esh, attr_argv));
}

