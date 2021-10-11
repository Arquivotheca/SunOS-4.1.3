#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)sel_policy.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

#include <suntool/selection_impl.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_input.h>

/*
 *	sel_policy.c:	implement the user interface policy
 *			in the selection_service
 */

static int	seln_non_null_primary();

 
/*
 *	Figure how to respond to a function buffer from the server
 */
Seln_response
seln_figure_response(buffer, holder)
    Seln_function_buffer *buffer;
    Seln_holder         **holder;
{
    Seln_holder          *me;
    char	         *client;

    if (buffer->function == SELN_FN_ERROR) {
	return SELN_IGNORE;
    }
    switch (buffer->addressee_rank) {
      case SELN_CARET:
	me = &buffer->caret;
	break;
      case SELN_PRIMARY:
	me = &buffer->primary;
	break;
      case SELN_SECONDARY:
	me = &buffer->secondary;
	break;
      case SELN_SHELF:
	me = &buffer->shelf;
	break;
      default:
	complain("figure_response got a malformed buffer.");
	return SELN_IGNORE;
    }
    client = ((Seln_client_node *) (LINT_CAST((me->access.client))))->client_data;
    switch (buffer->function) {
      case SELN_FN_GET:
	if (!seln_holder_same_client(&buffer->caret, client)) {
	    return SELN_IGNORE;
	}
	if (seln_secondary_made(buffer)) {
	    *holder = &buffer->secondary;
	} else {
	    *holder = &buffer->shelf;
	}
	if ((*holder)->state == SELN_NONE) {
	    return SELN_IGNORE;
	} else {
	    buffer->addressee_rank = SELN_CARET;
	    return SELN_REQUEST;
	}

      case SELN_FN_PUT:
	if (seln_secondary_exists(buffer)) {
	    if (!seln_holder_same_client(&buffer->secondary, client)) {
		return SELN_IGNORE;
	    } else {
		*holder = &buffer->primary;
		buffer->addressee_rank = SELN_SECONDARY;
		return SELN_REQUEST;
	    }
	}
	if (seln_secondary_made(buffer) ||	/* made & canceled */
	    !seln_holder_same_client(&buffer->primary, client)) {
	    return SELN_IGNORE;
	}
	*holder = &buffer->shelf;
	buffer->addressee_rank = SELN_PRIMARY;
	return SELN_SHELVE;

      case SELN_FN_FIND:
	if (!seln_holder_same_client(&buffer->caret, client)) {
	    return SELN_IGNORE;
	}
	buffer->addressee_rank = SELN_CARET;
	if (!seln_secondary_made(buffer)) {
	    if (seln_non_null_primary(&buffer->primary)) {
		*holder = &buffer->primary;
		return SELN_FIND;
	    }
	    *holder = &buffer->shelf;
	    return SELN_FIND;
	}
	if (!seln_secondary_exists(buffer)) {	/* made & canceled */
	    return SELN_IGNORE;
	}
	*holder = &buffer->secondary;  /* secondary exists  */
	return SELN_FIND;

      case SELN_FN_DELETE:
	if (seln_secondary_exists(buffer)) {
	    if (!seln_holder_same_client(&buffer->secondary, client)) {
		return SELN_IGNORE;
	    } else {
		*holder = &buffer->shelf;
		buffer->addressee_rank = SELN_SECONDARY;
		return SELN_DELETE;
	    }
	}
	if (seln_secondary_made(buffer) ||	/* made & canceled */
	    !seln_holder_same_client(&buffer->primary, client)) {
	    return SELN_IGNORE;
	}
	*holder = &buffer->shelf;
	buffer->addressee_rank = SELN_PRIMARY;
	return SELN_DELETE;
      default:
	complain("figure_response got a malformed buffer.");
	return SELN_IGNORE;
    }
}


void
seln_report_event(seln_client, event)
    Seln_client	         seln_client;
    struct inputevent    *event;
{
    Seln_function         func;
    Seln_function_buffer  buffer;
    Seln_client_node	  *client;

    client = (Seln_client_node *)(LINT_CAST(seln_client));
    switch (event_action(event)) {
      case ACTION_COPY:		/* formerly KEY_LEFT(6): */
	func = SELN_FN_PUT;
	break;
      case ACTION_PASTE:	/* formerly KEY_LEFT(8): */
	func = SELN_FN_GET;
	break;
      case ACTION_FIND_FORWARD:	/* formerly KEY_LEFT(9): */
      case ACTION_FIND_BACKWARD:
	func = SELN_FN_FIND;
	break;
      case ACTION_CUT:		/* formerly KEY_LEFT(10): */
	func = SELN_FN_DELETE;
	break;
      default:
	return;
    }
    buffer = seln_inform((Seln_client)(LINT_CAST(client)), func, win_inputposevent(event));
    if (buffer.function != SELN_FN_ERROR &&
	client != (Seln_client_node *) NULL) {
	client->ops.do_function(client->client_data, &buffer);
    }
}

static int
seln_non_null_primary(holder)
    Seln_holder		*holder;
{
    Seln_request	 buffer;
    u_int		*bufp = (u_int *) (LINT_CAST(buffer.data));

    seln_init_request(&buffer, holder, SELN_REQ_BYTESIZE, 0, 0);
    if (seln_request(holder, &buffer) != SELN_SUCCESS	||
	    *bufp++ != (u_int) SELN_REQ_BYTESIZE	||
	    *bufp == 0)
	return FALSE;
    else
	return TRUE;
}
