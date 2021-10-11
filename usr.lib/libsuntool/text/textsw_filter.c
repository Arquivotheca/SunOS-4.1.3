#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)textsw_filter.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Filter invocation routines for textsw package.
 */

#include <suntool/primal.h>

#include <suntool/textsw_impl.h>
#include <fcntl.h>
#include <signal.h>
#include <vfork.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#define FAIL		-1
#define	INPUT		0
#define	OUTPUT		1
#define	STDIN		0
#define	STDOUT		1
#define	STDERR		2
#define	PIPSIZ		4096		/* See uipc_pipe.c */
#define	SLOP_SIZE	16

#define	REPLY_ERROR	-1
#define	REPLY_OKAY	 0
#define	REPLY_SEND	 1

/* performance: global cache of getdtablesize() */
extern int dtablesize_cache;
#define GETDTABLESIZE() \
	(dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

extern int		 errno;

extern char		*getlogindir();
pkg_private Es_index	 textsw_do_input();
static short unsigned	 type_for_filter_rec();
static int		 event_code_for_filter_rec();

/* WARNING: this is a hack to force the variable to be in memory.
 *	this var gets changed somehow when it is in register in the
 *	context of a vfork();
 */
static int		 execvp_failed;

pkg_private Ev_mark_object	textsw_add_mark_internal();
extern Notify_value		notify_default_wait3();

static int
interpret_filter_reply(view, buffer, buffer_length, delta)
	Textsw_view	 view;
	char		*buffer;
	int		 buffer_length;
	Es_index	*delta;
{
	*delta = textsw_do_input(view, buffer, (long)buffer_length, 
	                         TXTSW_UPDATE_SCROLLBAR_IF_NEEDED);
	if AN_ERROR(*delta != buffer_length)
	    return(REPLY_ERROR);
	return(REPLY_OKAY);
}

/*
 * Ignores SIGPIPE so that write(2) to nonexistent or dead filter
 *  does not cause parent (containing textsw) to terminate.
 */
/* ARGSUSED */
static Notify_value
textsw_sigpipe_func(textsw, sig, mode)
	Textsw_folio		textsw;		/* Currently unused */
	int			sig;		/* Currently unused */
	Notify_signal_mode	mode;		/* Currently unused */
{
	return(NOTIFY_DONE);
}

static int
textsw_filter_selection(folio, fill_in)
	register Textsw_folio			folio;
	register Textsw_selection_handle	fill_in;
/* Note: this routine guarantees to return a valid (but possibly empty)
 *	selection, thus callers need not error check the return value.
 */
{
	textsw_init_selection_object(folio, fill_in, "", 0, FALSE);
	fill_in->type = textsw_func_selection_internal(
					folio, fill_in, EV_SEL_PRIMARY, 0);
	if (TFS_IS_ERROR(fill_in->type) || (fill_in->type & TFS_IS_OTHER)) {
	    fill_in->last_plus_one = fill_in->first = ES_INFINITY;
	    fill_in->type = EV_SEL_PRIMARY | TFS_IS_SELF;
	}
	if (fill_in->first < fill_in->last_plus_one) {
	    textsw_set_selection(VIEW_REP_TO_ABS(folio->first_view),
				 ES_INFINITY, ES_INFINITY,
				 (unsigned) fill_in->type);
	} else {
	    fill_in->type &= ~EV_SEL_PENDING_DELETE;
	}
	return(fill_in->type);
}

pkg_private int
textsw_call_filter(view, filter_argv)
	register Textsw_view	 view;
	char			*filter_argv[];
{
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	int			 filter_input, filter_output,
				 result = 0;
	Es_index		 save_length;
	Ev_mark_object		 save_lpo_id;
	Ev_mark_object		 save_insert = NULL;
	int			 pid;
	Textsw_selection_object	 selection;
	Notify_func		 old_sigpipe = (Notify_func)0;
	Es_index		 ev_get_insert();

	(void) textsw_filter_selection(folio, &selection);
	if (selection.type & EV_SEL_PENDING_DELETE) {
	    save_length = selection.last_plus_one - selection.first;
	    save_lpo_id =
		textsw_add_mark_internal(folio, selection.last_plus_one,
					 TEXTSW_MARK_DEFAULTS);
	}
	pid = start_filter(filter_argv, &filter_input, &filter_output);
	if (pid == FAIL) {
	    result = 1;
	    goto NoFilterReturn;
	}
	ev_set(0, folio->views, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
	if ((int) ev_get(folio->views, EV_CHAIN_LOWER_CONTEXT)
	    != EV_NO_CONTEXT)
	    save_insert = textsw_add_mark_internal(folio,
				     ev_get_insert(folio->views),
				     TEXTSW_MARK_MOVE_AT_INSERT);
	old_sigpipe =
	    notify_set_signal_func((Notify_client)folio, textsw_sigpipe_func,
				   SIGPIPE, NOTIFY_ASYNC);
	(void) notify_set_wait3_func((Notify_client)folio,
				     notify_default_wait3, pid);
	    /* Note that there is no meaningful old_wait3_func */
	switch (talk_to_filter(view, filter_input, filter_output,
			       selection.first, selection.last_plus_one,
			       interpret_filter_reply)) {
	  case 0:
	    break;
	  case 1:
	    goto ErrorReturn;
	}

NormalReturn:
	(void) close(filter_output);
	if ((result == 0) && (selection.type & EV_SEL_PENDING_DELETE)) {
	    Es_index		saved_lpo;

	    saved_lpo = textsw_find_mark_internal(folio, save_lpo_id);
	    if AN_ERROR(saved_lpo == ES_INFINITY) {
	    } else {
		(void) textsw_delete_span(view, saved_lpo-save_length,
					  saved_lpo,
					  TXTSW_DS_ADJUST|TXTSW_DS_SHELVE);
	    }
	}
	if (old_sigpipe)
	    (void) notify_set_signal_func((Notify_client)folio, old_sigpipe,
					  SIGPIPE, NOTIFY_ASYNC);

NoFilterReturn:
        (void) close(filter_input); /* ECD fix */
        (void) close(filter_output); /* ECD fix */
	if (selection.type & EV_SEL_PENDING_DELETE)
	    textsw_remove_mark_internal(folio, save_lpo_id);
	/* Complete the deferred display update and autoscroll. */
	ev_set(0, folio->views, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
	ev_update_chain_display(folio->views);
	textsw_do_insert_makes_visible(view);
	if (save_insert) {
	    ev_scroll_if_old_insert_visible(folio->views,
		textsw_find_mark_internal(folio, save_insert),
		/* Assign delta to minimum to ensure correct behavior,
		 * may not be optimal.
		 */
		1);
	    textsw_remove_mark_internal(folio, save_insert);
	}
	return(result);

ErrorReturn:
	result = 2;
	goto NormalReturn;
}

static int
talk_to_filter(view, filter_input, filter_output, first, last_plus_one,
	       interpret_reply)
	register Textsw_view	  view;
	int			  filter_input, filter_output;
	register Es_index	  first, last_plus_one;
	int			(*interpret_reply)();
{
#define	NR_FIRST		0
#define	NR_LAST_PLUS_ONE	1
	extern Es_index		  ev_get_insert();
	register Textsw_folio	  folio = FOLIO_FOR_VIEW(view);
	int			  max_fds = GETDTABLESIZE(),
				  nr_valid = 0,
				  status,
				  result = 0;
	unsigned long		  buffer_and_slop[(PIPSIZ+SLOP_SIZE)/
						  sizeof(unsigned long)];
	unsigned char		 *buffer;
	struct timeval		  tv;
	Es_index		  insert, next_request[2];

	/* buffer_and_slop is declared unsigned long in order to meet the
	 * most restrictive of the alignment restrictions.
	 */
	buffer = (unsigned char *)LINT_CAST(buffer_and_slop) + SLOP_SIZE;
	/*
	 * Copy data into the filter.  As filter makes the filtered
	 *   data available, copy it to output_file.
	 * Stdio is not used to read from the filter because it is necessary
	 *   to make sure that the read does not block.  In addition, for both
	 *   reading and writing of the filter, the stdio automatic buffering
	 *   is only a hinderance.
	 */
	for (;;) {
	    register int		 nfds;
	    fd_set			 exceptfds, readfds, writefds;
	    long int			 written[3];

	    FD_ZERO(&exceptfds); FD_ZERO(&readfds); FD_ZERO(&writefds);
	    FD_SET(filter_output, &readfds);
	    if (first < last_plus_one) {
		/* writefds |= (1 << filter_input); */
		FD_SET(filter_input, &writefds);
	    } else {
		/* For a "dumb" filter, we must close the input so that it
		 *   does not hang waiting for more of the selection from us.
		 */
		if (interpret_reply == interpret_filter_reply) {
		    (void) close(filter_input);
		    filter_input = -1;
		}
	    }
	    /* Filter only has tv.tv_sec seconds to reply, or we assume it
	     *   is permanently hung.
	     * BUG ALERT! This should be customizable by user and/or client.
	     */
	    tv.tv_sec = 15; tv.tv_usec = 0;
	    nfds = select(max_fds, &readfds, &writefds, &exceptfds, &tv);
	    switch (nfds) {
	      case -1:	if (errno == EINTR)
			    continue;		/* Probably itimer expired */
			goto ErrorReturn;
	      case 0:	goto ErrorReturn;	/* Assume filter has hung */
	      default:	break;
	    }
	    /* if (readfds & (1 << filter_output)) { */
	    if (FD_ISSET(filter_output, &readfds)) {
		register int	bytes_read;

		bytes_read = read(filter_output, (char *)buffer,
				  sizeof(buffer_and_slop)-SLOP_SIZE);
		switch (bytes_read) {
		  case 0:
		    if (first >= last_plus_one)
		        goto NormalReturn;
		    break;
		  case -1:
		    if (errno == EINTR)
			break;			/* Probably itimer expired */
		    goto ErrorReturn;
		  default:
		    insert = ev_get_insert(folio->views);
		    status = interpret_reply(view, buffer, bytes_read,
					     &written[0]);
		    switch (status) {
		      case REPLY_ERROR:
			goto ErrorReturn;
		      case REPLY_OKAY:
			break;
		      case REPLY_SEND:
			if AN_ERROR(nr_valid != 0) {
			} else {
			    nr_valid = 1;
			    next_request[NR_FIRST] = written[1];
			    next_request[NR_LAST_PLUS_ONE] = written[2];
			}
			break;
		    }
		    if (insert <= first) {
			first += written[0];
			last_plus_one += written[0];
		    }
		    break;
		}
	    
	    } 
	    
	    /*  if (writefds & (1 << filter_input)) { */
	    if (FD_ISSET(filter_input, &writefds)) {
		int		to_write;
		Es_index	next;

		(void) es_set_position(folio->views->esh, first);
		to_write = sizeof(buffer_and_slop) - SLOP_SIZE;
		if (to_write > last_plus_one-first)
		    to_write = last_plus_one-first;
		next = es_read(folio->views->esh, to_write, buffer, &to_write);
		if (READ_AT_EOF(first, next, to_write)) {
		    first = last_plus_one;
		} else {
		    ASSUME(to_write <= PIPSIZ);
		    written[0] = write(filter_input, (char *)buffer, to_write);
		    if (written[0] == -1) {
			if (errno == EWOULDBLOCK) {
			    /* Go around and wait for filter to run */
			} else goto ErrorReturn;
		    } else {
			if (written[0] < to_write) 
			    next = (first + written[0]);
			     
			first = next;
		    }
		}
	    }
	    if (first >= last_plus_one) {
		if (nr_valid != 0) {
		    first = next_request[NR_FIRST];
		    last_plus_one = next_request[NR_LAST_PLUS_ONE];
		    nr_valid = 0;
		}
	    }
	}

NormalReturn:
	if (filter_input != -1)
	    (void) close(filter_input);
	return(result);

ErrorReturn:
	result = 1;
	goto NormalReturn;
#undef	NR_FIRST
#undef	NR_LAST_PLUS_ONE
}

static int
smarter_interpret_filter_reply(view, buffer, buffer_length, delta)
	Textsw_view		 view;
	register char		*buffer;
	int			 buffer_length;
	Es_index		*delta;
{
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	char			*save_buffer = buffer;
	register char		*buffer_last_plus_one;
	int			*int_buffer;
	int			 result = REPLY_OKAY;

	/* First deal with old stuff that is left over */
	if (folio->owed_by_filter < 0) {
	    bcopy(buffer, buffer-SLOP_SIZE-folio->owed_by_filter,
		  buffer_length);
	    buffer_length -= folio->owed_by_filter;
	    buffer -= SLOP_SIZE;
	    folio->owed_by_filter = 0;
	}
	buffer_last_plus_one = buffer + buffer_length;
	/* Process the requests from the filter */
	while (buffer < buffer_last_plus_one) {
	    if (folio->owed_by_filter > 0) {
		int	 to_insert = folio->owed_by_filter;
		if (to_insert > buffer_last_plus_one-buffer)
		    to_insert = buffer_last_plus_one-buffer;
		*delta = textsw_do_input(view, buffer, (long)to_insert,
					 TXTSW_UPDATE_SCROLLBAR_IF_NEEDED);
		if AN_ERROR(*delta != to_insert)
		    return(REPLY_ERROR);
		folio->owed_by_filter -= to_insert;
		buffer += ((to_insert+sizeof(int)-1)/sizeof(int))*sizeof(int);
		continue;
	    }
	    int_buffer = (int *)LINT_CAST(buffer);
	    if ((buffer_last_plus_one-buffer < 2*sizeof(int)) ||
		((int_buffer[0]) != TEXTSW_FILTER_MAGIC))
		goto Deal_With_Slop;
	    switch ((Textsw_filter_command)(int_buffer[1])) {
	      case TEXTSW_FILTER_DELETE_RANGE:
		if (buffer_last_plus_one-buffer < 4*sizeof(int))
		    goto Deal_With_Slop;
		*delta = textsw_delete_span(view, (long)int_buffer[2],
					    (long)int_buffer[3],
					    TXTSW_DS_ADJUST|TXTSW_DS_SHELVE);
		buffer += 4*sizeof(int);
		break;
	      case TEXTSW_FILTER_INSERT:
		if (buffer_last_plus_one-buffer < 3*sizeof(int))
		    goto Deal_With_Slop;
		folio->owed_by_filter = int_buffer[2];
		buffer += 3*sizeof(int);
		break;
	      case TEXTSW_FILTER_SEND_RANGE: {
		if (buffer_last_plus_one-buffer < 4*sizeof(int))
		    goto Deal_With_Slop;
		delta[1] = int_buffer[2];
		delta[2] = int_buffer[3];
		result = REPLY_SEND;
		buffer += 4*sizeof(int);
		break;
	      }
	      case TEXTSW_FILTER_SET_INSERTION:
		if (buffer_last_plus_one-buffer < 3*sizeof(int))
		    goto Deal_With_Slop;
		(void) ev_set_insert(folio->views, (Es_index)int_buffer[2]);
		buffer += 3*sizeof(int);
		break;
	      case TEXTSW_FILTER_SET_SELECTION:
		if (buffer_last_plus_one-buffer < 4*sizeof(int))
		    goto Deal_With_Slop;
		textsw_set_selection(VIEW_REP_TO_ABS(folio->first_view),
				     (Es_index)int_buffer[2],
				     (Es_index)int_buffer[3],
				     EV_SEL_PRIMARY);
		buffer += 4*sizeof(int);
		break;
	      default:
		return(REPLY_ERROR);
	    }
	}
	return(result);

Deal_With_Slop:
	ASSUME(buffer_last_plus_one-buffer < SLOP_SIZE);
	if (buffer >= save_buffer)
	    bcopy(buffer, buffer-SLOP_SIZE, buffer_last_plus_one-buffer);
	folio->owed_by_filter = buffer-buffer_last_plus_one;
	return(result);
}

pkg_private int
textsw_call_smart_filter(view, event, filter_argv)
	register Textsw_view	 view;
	Event			*event;
	char			*filter_argv[];
{
/*
 *   A smart filter is expected to do more than just filter the current
 * selection.  As a result, it is passed more information; in particular:
 *	the entire input event that resulted in the filter invocation
 *	the position of the insertion point, and the first and last_plus_one
 *		positions of the line containing the insertion point,
 *	the first and last_plus_one positions of the selection,
 *	the line containing the insertion point,
 *	the selection
 */
#define	FILTER_STARTED		0
#define	HEADER_WRITTEN		1
#define	INSERT_LINE_WRITTEN	2
	extern Es_index		 ev_get_insert();
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	int			 filter_input, filter_output,
				 max_fds = GETDTABLESIZE(),
				 result = 0;
	Textsw_filter_attribute	 filter_attribute;
	unsigned char		 buffer[PIPSIZ];
	struct timeval		 tv;
	Es_index		 insert, save_length;
	Es_index		 insert_line_first, insert_line_last_plus_one;
	unsigned		 state = FILTER_STARTED;
	Ev_mark_object		 save_lpo_id;
	int			 pid;
	Textsw_selection_object	 selection;
	Notify_func		 old_sigpipe = (Notify_func)0;

	insert = ev_get_insert(folio->views);
	ev_span(folio->views, insert, &insert_line_first,
		&insert_line_last_plus_one, EI_SPAN_LINE);
	if (insert_line_first == ES_CANNOT_SET) {
	    insert_line_first = insert_line_last_plus_one = insert;
	}
	(void) textsw_filter_selection(folio, &selection);
	if (selection.type & EV_SEL_PENDING_DELETE) {
	    save_length = selection.last_plus_one - selection.first;
	    save_lpo_id =
		textsw_add_mark_internal(folio, selection.last_plus_one,
					 TEXTSW_MARK_DEFAULTS);
	}
	pid = start_filter(filter_argv, &filter_input, &filter_output);
	if (pid == FAIL) {
	    result = 1;
	    goto NoFilterReturn;
	}
	old_sigpipe =
	    notify_set_signal_func((Notify_client)folio, textsw_sigpipe_func,
				   SIGPIPE, NOTIFY_ASYNC);
	(void) notify_set_wait3_func((Notify_client)folio,
				     notify_default_wait3, pid);
	    /* Note that there is no meaningful old_wait3_func */
	tv.tv_sec = 1; tv.tv_usec = 0;
	
	ev_set(0, folio->views, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
	/*
	 * Give the filter the initial information
	 */
	for (;;) {
	    register int		 nfds;
	    fd_set			 exceptfds, readfds, writefds;
	    int				 written;
	    register struct timeval	*tv_to_use;

	    FD_ZERO(&exceptfds); FD_ZERO(&readfds); FD_ZERO(&writefds);
            FD_SET(filter_input, &writefds);
	    tv_to_use = (timerisset(&tv)) ? &tv : 0;
	    nfds = select(max_fds, &readfds, &writefds, &exceptfds, tv_to_use);
	    switch (nfds) {
	      case -1:	if (errno == EINTR)
			    continue;		/* Probably itimer expired */
			goto ErrorReturn;
	      case 0:	/* Fall through */	
	      default:	timerclear(&tv);
			break;
	    }
	    /* if (writefds & (1 << filter_input)) { */
	    if (FD_ISSET(filter_input, &writefds)) {
		int		to_write;
		Es_index	next;

		if (state == FILTER_STARTED) {
		    filter_attribute = TEXTSW_FATTR_INPUT_EVENT;
		    if ((write(filter_input, (char *)&filter_attribute,
			       sizeof(filter_attribute)) == -1) ||
			(write(filter_input, (char *)event,
			       sizeof(*event)) == -1))
			goto Write_Failed;
		    if (folio->to_insert_next_free > folio->to_insert) {
			filter_attribute = TEXTSW_FATTR_INPUT;
			to_write = folio->to_insert_next_free-
				   folio->to_insert;
			if ((write(filter_input, (char *)&filter_attribute,
				   sizeof(filter_attribute)) == -1) ||
			    (write(filter_input, (char *)&to_write,
				   sizeof(to_write)) == -1) ||
			    (write(filter_input, folio->to_insert,
				   to_write) == -1))
			    goto Write_Failed;
		    }
		    filter_attribute = TEXTSW_FATTR_INSERTION_POINTS;
		    if ((write(filter_input, (char *)&filter_attribute,
			       sizeof(filter_attribute)) == -1) ||
			(write(filter_input, (char *)&insert,
			       sizeof(insert)) == -1) ||
			(write(filter_input, (char *)&insert_line_first,
			       sizeof(insert_line_first)) == -1) ||
			(write(filter_input,
			       (char *)&insert_line_last_plus_one,
			       sizeof(insert_line_last_plus_one)) == -1))
			goto Write_Failed;
		    filter_attribute = TEXTSW_FATTR_SELECTION_ENDPOINTS;
		    if ((write(filter_input, (char *)&filter_attribute,
			       sizeof(filter_attribute)) == -1) ||
			(write(filter_input, (char *)&selection.first,
			       sizeof(selection.first)) == -1) ||
			(write(filter_input, (char *)&selection.last_plus_one,
			       sizeof(selection.last_plus_one)) == -1))
			goto Write_Failed;
		    if (insert_line_first < insert_line_last_plus_one) {
			filter_attribute = TEXTSW_FATTR_INSERTION_LINE;
			if (write(filter_input, (char *)&filter_attribute,
			       sizeof(filter_attribute)) == -1)
			goto Write_Failed;
		    }
		    state = HEADER_WRITTEN;
		}
		if (state == HEADER_WRITTEN) {
		    if (insert_line_last_plus_one <= insert_line_first)
			goto Done_Initial_Data;
		    (void) es_set_position(folio->views->esh,
					   insert_line_first);
		    to_write = sizeof(buffer);
		    if (to_write > insert_line_last_plus_one-insert_line_first)
			to_write = insert_line_last_plus_one-insert_line_first;
		    next = es_read(folio->views->esh, to_write, buffer,
				   &to_write);
		    if (READ_AT_EOF(insert_line_first, next, to_write)) {
			goto Done_Initial_Data;
		    } else {
			ASSUME(to_write <= PIPSIZ);
			written = write(filter_input, (char *)buffer,
					to_write);
			if (written == -1)
			    goto Write_Failed;
			insert_line_first = next;
		    }
		}
	    }
	    continue;
Write_Failed:
	    if (errno == EMSGSIZE) {
		/* Give filter a chance to run */
		tv.tv_sec = 0;
		tv.tv_usec = 50000;
	    } else goto ErrorReturn;
	}
Done_Initial_Data:
	state = INSERT_LINE_WRITTEN;
	switch (talk_to_filter(view, filter_input, filter_output,
			       ES_INFINITY, ES_INFINITY,
			       smarter_interpret_filter_reply)) {
	  case 0:
	    break;
	  case 1:
	    goto ErrorReturn;
	}

NormalReturn:
	(void) close(filter_output);
	if ((result == 0) && (selection.type & EV_SEL_PENDING_DELETE)) {
	    Es_index		saved_lpo;

	    saved_lpo = textsw_find_mark_internal(folio, save_lpo_id);
	    if AN_ERROR(saved_lpo == ES_INFINITY) {
	    } else {
		(void) textsw_delete_span(view, saved_lpo-save_length,
					  saved_lpo,
					  TXTSW_DS_ADJUST|TXTSW_DS_SHELVE);
	    }
	}
	if (old_sigpipe)
	    (void) notify_set_signal_func((Notify_client)folio, old_sigpipe,
					  SIGPIPE, NOTIFY_ASYNC);

NoFilterReturn:
        (void) close(filter_input); /* ECD fix */
        (void) close(filter_output); /* ECD fix */
	if (selection.type & EV_SEL_PENDING_DELETE)
	    textsw_remove_mark_internal(folio, save_lpo_id);
	folio->owed_by_filter = 0;
	ev_set(0, folio->views, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
	ev_update_chain_display(folio->views);
	return(result);

ErrorReturn:
	result = 3;
	goto NormalReturn;
}

extern void
close_nonstd_fds_on_exec()
{
	int	fd, max_fds = GETDTABLESIZE();

	for (fd = STDERR+1; fd < max_fds; fd++)
		(void) fcntl(fd, F_SETFD, 1);
}

static int
start_filter(filter_argv, filter_input, filter_output)
	char	*filter_argv[];
	int	*filter_input, *filter_output;
{
	int	 to_filter[2], from_filter[2], pid;

	errno = 0;
	if ((pipe(to_filter) != 0) || (pipe(from_filter) != 0))
	    return(FAIL);
	pid = vfork();
	if (pid == 0) {		/* child */
	    /*
	     * Fiddle with the file descriptors, since the exec'd filter
	     *   expects to read the data from stdin, and to write to stdout.
	     */
	    if ((dup2(to_filter[INPUT], STDIN) == -1) ||
		(dup2(from_filter[OUTPUT], STDOUT) == -1))
		    _exit(6);
	    close_nonstd_fds_on_exec();
	    (void) execvp(filter_argv[0], filter_argv);
	    /*
	     * Since we used vfork, the child is sharing the parent's address
	     *   space.  The advantage is that we can let the parent know that
	     *   the execvp failed simply by changing the (shared)
	     *   execvp_failed.  The disadvantage is that we have to be very
	     *   careful about how we terminate the (now unwanted) child.
	     */
	    execvp_failed = TRUE;
	    _exit(7);
	    }
	/* parent */
	if (execvp_failed || (pid < 0)) {
	    execvp_failed = FALSE;
	    return(FAIL);
	}
	if ((close(to_filter[INPUT]) == -1) ||
	    (close(from_filter[OUTPUT]) == -1))
	    return(FAIL);
	/*
	 * The pipes to the filter must be non-blocking so that caller can
	 *   avoid deadly embrace while pushing the data through the filter.
	 */
	if (fcntl(to_filter[OUTPUT], F_SETFL, FNDELAY) == -1)
	    return(FAIL);
	if (fcntl(from_filter[INPUT], F_SETFL, FNDELAY) == -1)
	    return(FAIL);
	*filter_input = to_filter[OUTPUT];
	*filter_output = from_filter[INPUT];
	return(pid);
}

/*
 *	Parser of the .textswrc file at startup time
 */

#include <sys/dir.h>
#include "sunwindow/filter.h"
#undef FOREVER
#undef strdup
#include "sunwindow/io_stream.h"

extern STREAM		*file_input_stream();
extern STREAM		*filter_comments_stream();

pkg_private int
textsw_parse_rc(textsw)
	Textsw_folio		 textsw;
{
	char			 *base_name = ".textswrc",
				  file_name[MAXNAMLEN],
				 *login_dir;
	STREAM			 *rc_stream = NULL;
	STREAM			 *rc_wo_comments_stream = NULL;
	Key_map_handle		  current_key;
	Key_map_handle		 *previous_key;
	int			  i;
	short unsigned		  type;
	short			  event_code;
	int			  result = 0;
	struct filter_rec	**filters = NULL;

	textsw->key_maps = NULL;
	if ((login_dir = getlogindir()) == NULL)
	    return(1);
	(void) sprintf(file_name, "%s/%s", login_dir, base_name);
	if ((rc_stream = file_input_stream(file_name, (FILE *)0)) == NULL) {
	    result = 2;
	    goto Done;
	}
	if ((rc_wo_comments_stream = filter_comments_stream(rc_stream))
	    == NULL) {
	    result = 3;
	    goto Done;
	}
	if ((filters = parse_filter_table(rc_wo_comments_stream, base_name)) == NULL) {
	    result = 4;
	    goto Done;
	}
	previous_key = &textsw->key_maps;
	for (i = 0; filters[i]; i++) {
	    if ((event_code = event_code_for_filter_rec(filters[i])) == -1) {
		continue;
	    }
	    if ((type = type_for_filter_rec(filters[i])) == TXTSW_KEY_NULL) {
		continue;
	    }
	    *previous_key = current_key = NEW(Key_map_object);
	    current_key->next = NULL;
	    current_key->event_code = event_code;
	    current_key->type = type;
	    current_key->shifts = 0;
	    current_key->maps_to = (caddr_t) filters[i]->call;
	    filters[i]->call = NULL;	/* Transfer storage ownership. */
	    previous_key = &(current_key->next);
	}
Done:
	if (rc_stream != NULL)
	    stream_close(rc_stream);
	if (rc_wo_comments_stream != NULL)
	    stream_close(rc_wo_comments_stream);
	if (filters != NULL)
	    free_filter_table(filters);
	return(result);
}

static char	*type_groups[] = {
	"FILTER", "SMART_FILTER", "MACRO", NULL
};

static short unsigned
type_for_filter_rec(rec)
	struct filter_rec	*rec;
{
	int			 count;

	count = match_in_table(rec->class, type_groups);
	switch (count) {
	  case -1:
	    return(TXTSW_KEY_NULL);
	  case TXTSW_KEY_FILTER:
	  case TXTSW_KEY_SMART_FILTER:
	  case TXTSW_KEY_MACRO:
	    return(count);
	  default:
	    LINT_IGNORE(ASSUME(0));
	    return(TXTSW_KEY_NULL);
	}
}

static char	*key_groups[] = {
	"KEY_LEFT", "KEY_TOP", "KEY_RIGHT", "KEY_BOTTOM",
	"L", "T", "F", "R", "B", NULL
};

static int
event_code_for_filter_rec(rec)
	struct filter_rec	*rec;
{
	int			 count;

	count = match_in_table(rec->key_name, key_groups);
	switch (count) {
	  case -1:
	    return(-1);
	  case 0:
	  case 4:
	    return((rec->key_num < 0 || rec->key_num > 15)
		    ? -1
		    : KEY_LEFT(rec->key_num));
	  case 1:
	  case 5:
	  case 6:
	    return((rec->key_num < 0 || rec->key_num > 15)
		    ? -1
		    : KEY_TOP(rec->key_num));
	  case 2:
	  case 7:
	    return((rec->key_num < 0 || rec->key_num > 15)
		    ? -1
		    : KEY_RIGHT(rec->key_num));
	  case 3:
	  case 8:
	    return((rec->key_num < 0 || rec->key_num > 1)
		    ? -1
		    : KEY_BOTTOMLEFT + rec->key_num);
	  default:
	    LINT_IGNORE(ASSUME(0));
	    return(-1);
	}
}
