#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)panel_seln.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/***********************************************************************/
/* panel_seln.c                                                        */
/* Copyright (c) 1984 by Sun Microsystems, Inc.                        */
/***********************************************************************/

#include <suntool/panel_impl.h>
#include <sunwindow/sun.h>
#include <suntool/selection_attributes.h>

#define MIN_SELN_TIMEOUT        0
#define DEFAULT_SELN_TIMEOUT    10	/* 10 seconds */
#define MAX_SELN_TIMEOUT        300	/* 5 minutes */

extern void     (*panel_seln_inform_proc) (),
                (*panel_seln_destroy_proc) ();

static void     panel_seln_destroy_info(),
                panel_seln_function(),
                panel_seln_get(),
                panel_seln_put();

static void     panel_seln_report_event(), check_cache();


static Seln_result panel_seln_request();

/*
 * panel_seln_init -- register with the selection service
 */
void
panel_seln_init(panel)
    register panel_handle panel;
{
    /*
     * static so we only try to contact the selection service once.
     */
    static          no_selection_service;	/* Defaults to FALSE */
    int             selection_timeout;


    register panel_selection_handle primary = panel_seln(panel, SELN_PRIMARY);
    register panel_selection_handle secondary = panel_seln(panel, SELN_SECONDARY);
    register panel_selection_handle caret = panel_seln(panel, SELN_CARET);
    register panel_selection_handle shelf = panel_seln(panel, SELN_SHELF);

    /*
     * don't register with the selection service unless we are using the
     * notifier (i.e. panel_begin() was used instead of panel_create().
     */
    if (!using_notifier(panel) || no_selection_service)
	return;

    selection_timeout = defaults_get_integer_check("/SunView/Selection_Timeout",
	       DEFAULT_SELN_TIMEOUT, MIN_SELN_TIMEOUT, MAX_SELN_TIMEOUT, 0);
    seln_use_timeout(selection_timeout);	/* set selection timeout */

    panel->seln_client = seln_create(panel_seln_function, panel_seln_request,
		                     (char *) (LINT_CAST(panel)));
    if (!panel->seln_client) {
	no_selection_service = TRUE;
	return;
    }

    panel_seln_destroy_proc = panel_seln_destroy_info;
    panel_seln_inform_proc = (void (*) ()) panel_seln_report_event;

    primary->rank = SELN_PRIMARY;
    primary->is_null = TRUE;
    primary->ip = (panel_item_handle) 0;

    secondary->rank = SELN_SECONDARY;
    secondary->is_null = TRUE;
    secondary->ip = (panel_item_handle) 0;

    caret->rank = SELN_CARET;
    caret->is_null = TRUE;
    caret->ip = (panel_item_handle) 0;

    shelf->rank = SELN_SHELF;
    shelf->is_null = TRUE;
    shelf->ip = (panel_item_handle) 0;
}


/*
 * panel_seln_inquire -- inquire about the holder of a selection
 */
Seln_holder
panel_seln_inquire(panel, rank)
    panel_handle    panel;
    Seln_rank       rank;
{
    /*
     * always ask the service, even if we have not setup contact before (i.e.
     * no text items). This could happen if some other item has
     * PANEL_ACCEPT_KEYSTROKE on. if (!panel->seln_client) holder.rank =
     * SELN_UNKNOWN; else
     */
    return(seln_inquire(rank));
}


static void
panel_seln_report_event(panel, event)
    panel_handle    panel;
    Event          *event;
{
    seln_report_event(panel->seln_client, event);

    if (!panel->seln_client)
	return;

    check_cache(panel, SELN_PRIMARY);
    check_cache(panel, SELN_SECONDARY);
    check_cache(panel, SELN_CARET);
}

static void
check_cache(panel, rank)
    register panel_handle panel;
    register Seln_rank rank;
{
    Seln_holder     holder;

    if (panel_seln(panel, rank)->ip) {
	holder = seln_inquire(rank);
	if (!seln_holder_same_client(&holder, (char *) (LINT_CAST(panel))))
	    panel_seln_cancel(panel, rank);
    }
}



/*
 * panel_seln_acquire -- acquire the selection and update the state
 */
void
panel_seln_acquire(panel, ip, rank, is_null)
    register panel_handle panel;
    Seln_rank       rank;
    panel_item_handle ip;
    int             is_null;
{
    register panel_selection_handle selection;

    if (!panel->seln_client)
	return;

    switch (rank) {
	case SELN_PRIMARY:
	case SELN_SECONDARY:
	case SELN_CARET:
	    selection = panel_seln(panel, rank);
	    /*
	     * if we already own the selection, don't ask the service for it.
	     */
	    if (ip && selection->ip == ip)
		break;
	    /* otherwise fall through ... */

	default:
	    rank = seln_acquire(panel->seln_client, rank);
	    switch (rank) {
		case SELN_PRIMARY:
		case SELN_SECONDARY:
		case SELN_CARET:
		case SELN_SHELF:
		    selection = panel_seln(panel, rank);
		    break;

		default:
		    return;
	    }
	    break;
    }


    /* if there was an old selection, de-hilite it */
    if (selection->ip)
	panel_seln_dehilite(selection->ip, rank);

    /* record the selection & hilite it if it's not null */
    selection->ip = ip;
    selection->is_null = is_null;
    if (!is_null)
	panel_seln_hilite(selection);
}

/*
 * panel_seln_cancel -- cancel the current selection.
 */
void
panel_seln_cancel(panel, rank)
    panel_handle    panel;
    Seln_rank       rank;
{
    panel_selection_handle selection = panel_seln(panel, rank);

    if (!panel->seln_client || !selection->ip)
	return;

    /* de-hilite the selection */
    panel_seln_dehilite(selection->ip, rank);
    panel_clear_pending_delete(rank);
    selection->ip = (panel_item_handle) 0;
    (void) seln_done(panel->seln_client, rank);
}


/*
 * panel_seln_destroy -- destroy myself as a selection client
 */
static void
panel_seln_destroy_info(panel)
    register panel_handle panel;
{
    if (!panel->seln_client)
	return;

    /*
     * cancel PRIMARY and SECONDARY to get rid of possible highlighting
     */
    panel_seln_cancel(panel, SELN_PRIMARY);
    panel_seln_cancel(panel, SELN_SECONDARY);
    if (panel->shelf) {
	free(panel->shelf);
	panel->shelf = (char *) 0;
    }
    seln_destroy(panel->seln_client);
}

/* Callback routines */

static void
panel_seln_function(panel, buffer)
    register panel_handle panel;
    register Seln_function_buffer *buffer;
{
    Seln_holder    *holder;
    panel_selection_handle selection;

    /* A function key has gone up -- handle it. */

    if (!panel->caret)
	return;

    switch (seln_figure_response(buffer, &holder)) {
    case SELN_IGNORE:
	break;

    case SELN_REQUEST:
	panel_seln_process_request(panel, buffer, holder);
	break;

    case SELN_SHELVE:
	selection = panel_seln(panel, buffer->addressee_rank);
	panel_seln_shelve(panel, selection, buffer->addressee_rank);
	break;

    case SELN_FIND:
	(void) seln_ask(holder,
			SELN_REQ_COMMIT_PENDING_DELETE,
			SELN_REQ_YIELD, 0,
			0);
	break;

    case SELN_DELETE:
	selection = panel_seln(panel, buffer->addressee_rank);
	if (buffer->addressee_rank == SELN_PRIMARY ||
	    buffer->addressee_rank == SELN_SECONDARY) {
	    if (selection->ip && !selection->is_null) {
	        panel_seln_shelve(panel, selection, buffer->addressee_rank);
	        panel_seln_delete(selection->ip, buffer->addressee_rank);
	        selection->ip = (panel_item_handle) 0;
	        (void) seln_done(panel->seln_client, buffer->addressee_rank);
	    }
	}
	break;

    default:
	/* ignore anything else */
	break;
    }
}


/*
 * Respond to a request about my selections.
 */
static          Seln_result
panel_seln_request(attr, context, max_length)
    Seln_attribute  attr;
    register Seln_replier_data *context;
    int             max_length;
{
    register panel_handle panel = (panel_handle) LINT_CAST(context->client_data);
    register panel_selection_handle selection;
    char           *selection_string = (char *) 0;
    char            save_char;
    u_long          selection_length;
    Seln_result     result;

    selection = panel_seln(panel, context->rank);
    switch (context->rank) {
    case SELN_PRIMARY:
    case SELN_SECONDARY:
	if (selection->ip && !selection->is_null)
	    panel_get_text_selection(selection->ip, &selection_string,
				     &selection_length, context->rank);
	break;

    case SELN_SHELF:
	selection_string = panel->shelf ? panel->shelf : "";
	selection_length = strlen(selection_string);
	break;

    default:
	break;
    }

    switch (attr) {
    case SELN_REQ_BYTESIZE:
	if (!selection_string)
	    return SELN_DIDNT_HAVE;

	*context->response_pointer++ = (caddr_t) selection_length;
	return SELN_SUCCESS;

    case SELN_REQ_CONTENTS_ASCII:
	{
	    char           *temp = (char *) context->response_pointer;
	    int             count;

	    if (!selection_string)
		return SELN_DIDNT_HAVE;

	    count = selection_length;
	    if (count <= max_length) {
		bcopy(selection_string, temp, count);
		temp += count;
		while ((unsigned) temp % sizeof(*context->response_pointer))
		    *temp++ = '\0';
		context->response_pointer = (char **) LINT_CAST(temp);
		*context->response_pointer++ = 0;
		return SELN_SUCCESS;
	    }
	    return SELN_FAILED;
	}

    case SELN_REQ_YIELD:
	result = SELN_FAILED;
	switch (context->rank) {
	case SELN_SHELF:
	    if (panel->shelf) {
		free(panel->shelf);
		panel->shelf = 0;
		(void) seln_done(panel->seln_client, SELN_SHELF);
		result = SELN_SUCCESS;
	    }
	    break;

	default:
	    if (selection->ip) {
		panel_seln_cancel(panel, context->rank);
		result = SELN_SUCCESS;
	    }
	    break;
	}
	*context->response_pointer++ = (caddr_t) result;
	return result;

    case SELN_REQ_COMMIT_PENDING_DELETE:
	if (panel_is_pending_delete(context->rank)) {
	    if (context->rank == SELN_SECONDARY)
		panel_seln_shelve(panel, selection, context->rank);
	    if (selection->ip && !selection->is_null)
	        panel_seln_delete(selection->ip, context->rank);
	}
	*context->response_pointer++ = (caddr_t) SELN_SUCCESS;
	return SELN_SUCCESS;

    default:
	return SELN_UNRECOGNIZED;
    }
}


/* Selection utilities */

panel_seln_process_request(panel, buffer, holder)
    register panel_handle panel;
    register Seln_function_buffer *buffer;
    Seln_holder    *holder;
{

    switch (buffer->function) {
    case SELN_FN_GET:		/* PASTE */
	(void)panel_seln_get(panel, holder, buffer->addressee_rank);
	break;

    case SELN_FN_PUT:		/* COPY */
	(void)panel_seln_put(panel, holder, buffer);
	break;
    }
}

/*
 * Get the selection from holder and put it in the text item that owns the
 * selection rank.
 */
static void
panel_seln_put(panel, holder, buffer)
    panel_handle    panel;
    Seln_holder    *holder;
    register Seln_function_buffer *buffer;
{
    Seln_request   	*reqbuf;
    register Attr_avlist avlist;
    register u_char     *cp;
    Event               event;
    int                 num_chars;
    int		    old_primary_caret_position;
    int		    new_primary_caret_position;
    int		    old_secondary_caret_position;
    int		    new_secondary_caret_position;
    int		    caret_was_on;
    panel_selection_handle selection = panel_seln(panel, buffer->addressee_rank);
    panel_item_handle ip = selection->ip;


    if (!panel->seln_client)
	return;

    /*
     * get the primary selection and save in a buffer
     */

    /* if the request is too large, drop it on the floor. */
    reqbuf = seln_ask(holder,
		      SELN_REQ_BYTESIZE, 0,
		      SELN_REQ_CONTENTS_ASCII, 0,
		      0);

    cp = NULL;
    if (reqbuf->status != SELN_FAILED) {
	avlist = (Attr_avlist) LINT_CAST(reqbuf->data);
	if ((Seln_attribute) * avlist++ == SELN_REQ_BYTESIZE) {
	    num_chars = (int) *avlist++;
	    if ((Seln_attribute) * avlist++ == SELN_REQ_CONTENTS_ASCII)
	        cp = (u_char *) avlist;
	}
    }

    /*
     * make sure the string is null terminated in the last byte of the
     * selection.
     */
    if (cp != NULL) {
        cp[num_chars] = '\0';

        if ((cp = (u_char *)strdup(cp)) == NULL) {
	    fprintf(stderr, "panel_seln: Out of memory!");
	    exit(1);
        }
    }

    /*
     * dehilite (and possibly shelf and delete) the secondary selection
     */
    reqbuf = seln_ask(buffer->secondary,
		      SELN_REQ_COMMIT_PENDING_DELETE,
		      0);

    /* now, clean up */
    if (holder->rank == SELN_PRIMARY) {
        reqbuf = seln_ask(holder,
		      SELN_REQ_COMMIT_PENDING_DELETE,
		      0);
    }
    else {
	reqbuf = seln_ask(holder,
		      SELN_REQ_COMMIT_PENDING_DELETE,
		      SELN_REQ_YIELD, 0,
		      0);
    }

    reqbuf = seln_ask(buffer->secondary,
		      SELN_REQ_YIELD, 0,
		      0);

    if (reqbuf->status == SELN_FAILED)
	return;
    
    if (cp == (u_char *)0)
	return;

    if (buffer->addressee_rank == SELN_SECONDARY) {
        caret_was_on = 0;
        if (ip->panel->caret_on) {
	    panel_caret_on(panel, FALSE);
	    caret_was_on = 1;
        }

        /*
         * INSERT TEXT AT SECONDARY CARET OFFSET
         */
        old_primary_caret_position = panel_get_caret_position(ip, SELN_PRIMARY);
        old_secondary_caret_position = panel_get_caret_position(ip, SELN_SECONDARY);
        panel_set_caret_position(ip, SELN_PRIMARY, old_secondary_caret_position);
    }

    /*
     * put the primary selection where secondary selection was
     */
    /* initialize the event */
    event_init(&event);
    event_set_down(&event);

    while (*cp) {
	event_id(&event) = (short) *cp++;
	if (event_id(&event) >= 0x80)
	    event_id(&event) += ISO_FIRST;
	(void) panel_handle_event((Panel_item) ip, &event);
    }

    if (buffer->addressee_rank == SELN_SECONDARY) {
        /* yes, this is correct */
        new_secondary_caret_position = panel_get_caret_position(ip, SELN_PRIMARY);

        if (old_secondary_caret_position <= old_primary_caret_position) {
	    new_primary_caret_position = new_secondary_caret_position -
                                         old_secondary_caret_position +
                                         old_primary_caret_position;
        }
	else
	    new_primary_caret_position = old_primary_caret_position;

	if (ip == ip->panel->caret)
            panel_set_caret_position(ip, SELN_PRIMARY, new_primary_caret_position);
        if (caret_was_on)
	    panel_caret_on(panel, TRUE);
    }
}


/*
 * Get the selection from holder and put it in the text item that owns the
 * selection rank.
 */
static void
panel_seln_get(panel, holder, rank)
    panel_handle    panel;
    Seln_holder    *holder;
    Seln_rank       rank;
{
    Seln_request   *reqbuf;
    register Attr_avlist avlist;
    register u_char *cp;
    Event           event;
    Seln_holder     shelf_holder;
    int             num_chars = 0;
    int		    had_shelf = 0;
    int		    had_primary_pending_delete = 0;
    char 	    *selection_string = NULL;
    int 	    selection_length = 0;

    panel_selection_handle selection = panel_seln(panel, rank);
    panel_item_handle ip = selection->ip;

    if (!panel->seln_client)
	return;

    if (panel->shelf) {
	(void) seln_done(panel->seln_client, SELN_SHELF);
	had_shelf = 1;
    }

    /* preserve primary selection in case overlapping secondary selection */
    if (holder->rank == SELN_SECONDARY &&
	        ip == panel_seln(panel, SELN_PRIMARY)->ip &&
	        panel_is_pending_delete(SELN_PRIMARY)) {

	panel_get_text_selection(ip, &selection_string,
				 &selection_length, SELN_PRIMARY);

        if ((selection_string = (char *)strdup(selection_string)) == NULL) {
	    fprintf(stderr, "panel_seln: Out of memory!");
	    exit(1);
        }
	selection_string[selection_length] = '\0';	/* terminate string */

	had_primary_pending_delete = 1;
    }

    /*
     * if the request is too large, drop it on the floor.
     */
    if (holder->rank == SELN_SECONDARY)
	reqbuf = seln_ask(holder,
			  SELN_REQ_BYTESIZE, 0,
			  SELN_REQ_CONTENTS_ASCII, 0,
			  SELN_REQ_COMMIT_PENDING_DELETE,
			  SELN_REQ_YIELD, 0,
			  0);
    else
	reqbuf = seln_ask(holder,
			  SELN_REQ_BYTESIZE, 0,
			  SELN_REQ_CONTENTS_ASCII, 0,
			  SELN_REQ_COMMIT_PENDING_DELETE,
			  0);

    if (reqbuf->status == SELN_FAILED)
	return;

    avlist = (Attr_avlist) LINT_CAST(reqbuf->data);

    if ((Seln_attribute) * avlist++ != SELN_REQ_BYTESIZE)
	return;

    num_chars = (int) *avlist++;

    if ((Seln_attribute) * avlist++ != SELN_REQ_CONTENTS_ASCII)
	return;

    cp = (u_char *) avlist;

    /*
     * make sure the string is null terminated in the last byte of the
     * selection.
     */
    cp[num_chars] = '\0';

    /* reclaim the shelf if no one else took it */
    if (had_shelf) {
        shelf_holder = seln_inquire(SELN_SHELF);
	if (shelf_holder.state == SELN_NONE)
	    panel_seln_acquire(panel, (panel_item_handle) 0, SELN_SHELF, TRUE);
    }

    /* initialize the event */
    event_init(&event);
    event_set_down(&event);

    /* insert the text */
    while (*cp) {
	event_id(&event) = (short) *cp++;
	if (event_id(&event) >= 0x80)
	    event_id(&event) += ISO_FIRST;
	(void) panel_handle_event((Panel_item) ip, &event);
    }

    if (had_primary_pending_delete) {
	free(panel->shelf);
	panel->shelf = selection_string;
    }
}


/*
 * panel_seln_shelve -- put the panel selection of specified rank on the
 * shelf
 */

panel_seln_shelve(panel, selection, rank)
    panel_handle    panel;
    panel_selection_handle selection;
    Seln_rank       rank;
{
    char           *selection_string = (char *) 0;
    char            save_char;
    u_long          selection_length;

    if (rank != SELN_PRIMARY && rank != SELN_SECONDARY)
	return;

    if (panel->shelf)
	free(panel->shelf);

    /*
     * shelve the requested selection, not the caret selection
     */
    if (selection->is_null || !selection->ip) {
	panel->shelf = panel_strsave("");
	return;
    }

    panel_get_text_selection(selection->ip, &selection_string,
			     &selection_length, rank);
    if (selection_string) {
	save_char = *(selection_string + selection_length);
	*(selection_string + selection_length) = 0;
	panel->shelf = panel_strsave(selection_string);
	*(selection_string + selection_length) = save_char;
    }
    else
	panel->shelf = panel_strsave("");

    panel_seln_acquire(panel, (panel_item_handle) 0, SELN_SHELF, TRUE);
}
