#ifndef lint
static char sccsid[] = "@(#)seln_demo.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*
 * seln_demo.c
 *
 * demonstrate how to use the selection service library
 */

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/seln.h>

static Frame frame;
static Panel panel;

int err = 0;

char *malloc();

/*
 * definitions for the panel
 */

static Panel_item text_item, type_item, source_item, mesg_item;
static Panel_item set_item[3], get_item[3];
static void set_button_proc(), get_button_proc(), change_label_proc();

#define PRIMARY_CHOICE		0	/* get/set the primary selection */
#define SECONDARY_CHOICE	1	/* get/set the secondary selection */
#define SHELF_CHOICE		2	/* get/set the shelf */

#define ITEM_CHOICE		0	/* use the text item literally as the
					   selection */
#define FROMFILE_CHOICE		1	/* use the text item as the name of a
					   file which contains the selection */

int selection_type = PRIMARY_CHOICE;
int selection_source = ITEM_CHOICE;

char *text_labels[3][2] = {
	{
		"New primary selection:",
		"File containing new primary selection:"
	},
	{
		"New secondary selection:",
		"File containing new secondary selection:"
	},
	{
		"New shelf:",
		"File containing new shelf:"
	}
};

char *mesg_labels[3][2] = {
	{
		"Type in a selection and hit the Set Selection button",
		"Type in a filename and hit the Set Selection button"
	},
	{
		"Type in a selection and hit the Set Secondary button",
		"Type in a filename and hit the Set Secondary button"
	},
	{
		"Type in a selection and hit the Set Shelf button",
		"Type in a filename and hit the Set Shelf button"
	}
};

Seln_rank type_to_rank[3] = { SELN_PRIMARY, SELN_SECONDARY, SELN_SHELF };

/*
 * definitions for selection service handlers
 */

static Seln_client s_client;	/* selection client handle */

#define FIRST_BUFFER	0
#define NOT_FIRST_BUFFER	1

char *selection_bufs[3];	/* contents of each of the three selections;
				   they are set only when the user hits a set
				   or a get button */

int func_key_proc();
Seln_result reply_proc();
Seln_result read_proc();

/********************************************************************/
/* main routine                                                     */
/********************************************************************/

main(argc, argv)
int argc;
char **argv;
{
	/* create frame first */

	frame = window_create(NULL, FRAME, 
			FRAME_ARGS,	argc, argv, 
			WIN_ERROR_MSG, "Cannot create frame", 
			FRAME_LABEL, "seln_demo", 
			0);

	/* create selection service client before creating subwindows
	   (since the panel package also uses selections) */

	s_client = seln_create(func_key_proc, reply_proc, (char *)0);
	if (s_client == NULL) {
		fprintf(stderr, "seln_demo: seln_create failed!\n");
		exit(1);
	}

	/* now create the panel */

	panel = window_create(frame, PANEL, 
			WIN_ERROR_MSG, "Cannot create panel", 
			0);

	init_panel(panel);

	window_fit_height(panel);

	window_fit_height(frame);

	window_main_loop(frame);

	/* yield any selections we have and terminate connection with the
	   selection service */

	seln_destroy(s_client);

	exit(0);
	/* NOTREACHED */

}

/***********************************************************************/
/* routines involving setting a selection                              */
/***********************************************************************/

/*
 * acquire the selection type specified by the current panel choices;
 * this will enable requests from other clients which want to get
 * the selection's value, which is specified by the source_item and text_item
 */

static void
set_button_proc(/* args ignored */)
{
	Seln_rank ret;
	char *value = (char *)panel_get_value(text_item);

	if (selection_source == FROMFILE_CHOICE) {
		/* set the selection from a file; the selection service will
		   actually acquire the selection and handle all requests */

		if (seln_hold_file(type_to_rank[selection_type], value)
							!= SELN_SUCCESS) {
			panel_set(mesg_item, PANEL_LABEL_STRING, 
				"Could not set selection from named file!", 0);
			err++;
		} else if (err) {
			panel_set(mesg_item, PANEL_LABEL_STRING, 
				mesg_labels[selection_type][selection_source],0);
			err = 0;
		}
		return;
	}
	ret = seln_acquire(s_client, type_to_rank[selection_type]);

	/* check that the selection rank we received is the one we asked for */

	if (ret != type_to_rank[selection_type]) {
		panel_set(mesg_item, PANEL_LABEL_STRING, 
				"Could not acquire selection!", 0);
		err++;
		return;
	}

	set_selection_value(selection_type, selection_source, value);
}

/*
 * copy the new value of the appropriate selection into its
 * buffer so that if the user changes the text item and/or the current
 * selection type, the selection won't mysteriously change
 */

set_selection_value(type, source, value)
int type, source;
char *value;
{
	if (selection_bufs[type] != NULL)
		free(selection_bufs[type]);
	selection_bufs[type] = malloc(strlen(value) + 1);
	if (selection_bufs[type] == NULL) {
		panel_set(mesg_item, PANEL_LABEL_STRING, "Out of memory!", 0);
		err++;
	} else {
		strcpy(selection_bufs[type], value);
		if (err) {
			panel_set(mesg_item, PANEL_LABEL_STRING,
						mesg_labels[type][source], 0);
			err = 0;
		}
	}
}

/*
 * func_key_proc
 *
 * called by the selection service library whenever a change in the state of
 * the function keys requires an action (for instance, put the primary
 * selection on the shelf if the user hit PUT)
 */

func_key_proc(client_data, args)
char *client_data;
Seln_function_buffer *args;
{
	Seln_holder *holder;

	/* use seln_figure_response to decide what action to take */

	switch (seln_figure_response(args, &holder)) {
	case SELN_IGNORE:
		/* don't do anything */
		break;
	case SELN_REQUEST:
		/* expected to make a request */
		break;
	case SELN_SHELVE:
		/* put the primary selection (which we should have) on the
		   shelf */
		if (seln_acquire(s_client, SELN_SHELF) != SELN_SHELF) {
			panel_set(mesg_item, PANEL_LABEL_STRING, 
					"Could not acquire shelf!", 0);
			err++;
		} else {
			shelve_primary_selection();
		}
		break;
	case SELN_FIND:
		/* do a search */
		break;
	case SELN_DELETE:
		/* do a delete */
		break;
	}
}

shelve_primary_selection()
{
	char *value = selection_bufs[PRIMARY_CHOICE];

	if (selection_bufs[SHELF_CHOICE] != NULL)
		free(selection_bufs[SHELF_CHOICE]);
	selection_bufs[SHELF_CHOICE] = malloc(strlen(value)+1);
	if (selection_bufs[SHELF_CHOICE] == NULL) {
		panel_set(mesg_item, PANEL_LABEL_STRING, "Out of memory!", 0);
		err++;
	} else {
		strcpy(selection_bufs[SHELF_CHOICE], value);
	}
}

/*
 * reply_proc
 *
 * called by the selection service library whenever a request comes from
 * another client for one of the selections we currently hold
 */

Seln_result
reply_proc(item, context, length)
Seln_attribute item;
Seln_replier_data *context;
int length;
{
	int size, needed;
	char *seln, *destp;

	/* determine the rank of the request and choose the 
	   appropriate selection */

	switch (context->rank) {
	case SELN_PRIMARY:
		seln = selection_bufs[PRIMARY_CHOICE];
		break;
	case SELN_SECONDARY:
		seln = selection_bufs[SECONDARY_CHOICE];
		break;
	case SELN_SHELF:
		seln = selection_bufs[SHELF_CHOICE];
		break;
	default:
		seln = NULL;
	}

	/* process the request */

	switch (item) {
	case SELN_REQ_CONTENTS_ASCII:
		/* send the selection */

		/* if context->context == NULL then we must start sending
		   this selection; if it is not NULL, then the selection
		   was too large to fit in one buffer and this call must
		   send the next buffer; a pointer to the location to start
		   sending from was stored in context->context on the
		   previous call */

		if (context->context == NULL) {
			if (seln == NULL)
				return(SELN_DIDNT_HAVE);
			context->context = seln;
		}
		size = strlen(context->context);
		destp = (char *)context->response_pointer;
		
		/* compute how much space we need: the length of the selection
		   (size), plus 4 bytes for the terminating null word, plus 0
		   to 3 bytes to pad the end of the selection to a word
		   boundary */

		needed = size + 4;
		if (size % 4 != 0)
			needed += 4 - size % 4;
		if (needed <= length) {
			/* the entire selection fits */
			strcpy(destp, context->context);
			destp += size;
			while ((int)destp % 4 != 0) {
				/* pad to a word boundary */
				*destp++ = '\0';
			}
			/* update selection service's pointer so it can
			   determine how much data we are sending */
			context->response_pointer = (char **)destp;
			/* terminate with a NULL word */
			*context->response_pointer++ = 0;
			return(SELN_SUCCESS);
		} else {
			/* selection doesn't fit in a single buffer; rest
			   will be put in different buffers on subsequent
			   calls */
			strncpy(destp, context->context, length);
			destp += length;
			context->response_pointer = (char **)destp;
			context->context += length;
			return(SELN_CONTINUED);
		}
	case SELN_REQ_YIELD:
		/* deselect the selection we have (turn off highlight, etc.) */

		*context->response_pointer++ = (char *)SELN_SUCCESS;
		return(SELN_SUCCESS);
	case SELN_REQ_BYTESIZE:
		/* send the length of the selection */

		if (seln == NULL)
			return(SELN_DIDNT_HAVE);
		*context->response_pointer++ = (char *)strlen(seln);
		return(SELN_SUCCESS);
	case SELN_REQ_END_REQUEST:
		/* all attributes have been taken care of; release any
		   internal storage used */
		return(SELN_SUCCESS);
		break;
	default:
		/* unrecognized request */
		return(SELN_UNRECOGNIZED);
	}
	/* NOTREACHED */
}

/*******************************************************************/
/* routines involving getting a selection                          */
/*******************************************************************/

/*
 * get the value of the selection type specified by the current panel choices
 * from whichever client is currently holding it
 */

static void
get_button_proc(/* args ignored */)
{
	Seln_holder holder;
	int len;
	char context = FIRST_BUFFER; /* context value used when a very long
				        message is received; see procedure
					comment for read_proc */

	if (err) {
		panel_set(mesg_item, PANEL_LABEL_STRING,
			mesg_labels[selection_type][selection_source], 0);
		err = 0;
	}

	/* determine who has the selection of the rank we want */

	holder = seln_inquire(type_to_rank[selection_type]);
	if (holder.state == SELN_NONE) {
		panel_set(mesg_item, PANEL_LABEL_STRING, 
				"You must make a selection first!", 0);
		err++;
		return;
	}

	/* ask for the length of the selection and then the actual
	   selection; read_proc actually reads it in */

	(void) seln_query(&holder, read_proc, &context,
				SELN_REQ_BYTESIZE, 0, 
				SELN_REQ_CONTENTS_ASCII, 0,
				0);


	/* display the selection in the panel */

	len = strlen(selection_bufs[selection_type]);
	if (len > (int)panel_get(text_item, PANEL_VALUE_STORED_LENGTH))
		panel_set(text_item, PANEL_VALUE_STORED_LENGTH, len, 0);
	panel_set_value(text_item, selection_bufs[selection_type]);
}

/*
 * called by seln_query for every buffer of information received; short
 * messages (under about 2000 bytes) will fit into one buffer; for larger
 * messages, read_proc will be called with each buffer in turn; the context
 * pointer passed to seln_query is modified by read_proc so that we will know
 * if this is the first buffer or not
 */

Seln_result
read_proc(buffer)
Seln_request *buffer;
{
	char *reply;	/* pointer to the data in the buffer received */
	unsigned len;	/* amount of data left in the buffer */
	int bytes_to_copy;
	static int selection_have_bytes; /* number of bytes of the selection
			   which have been read; cumulative over all calls for
			   the same selection (it is reset when the first
			   buffer of a selection is read) */
	static int selection_len;	/* total number of bytes in the current
					   selection */

	if (*buffer->requester.context == FIRST_BUFFER) {

		/* this is the first buffer */

		if (buffer == (Seln_request *)NULL) {
			panel_set(mesg_item, PANEL_LABEL_STRING, 
				"Error reading selection - NULL buffer", 0);
			err++;
			return(SELN_UNRECOGNIZED);
		}
		reply = buffer->data;
		len = buffer->buf_size;

		/* read in the length of the selection */

		if (*((Seln_attribute *)reply) != SELN_REQ_BYTESIZE) {
			panel_set(mesg_item, PANEL_LABEL_STRING, 
			     "Error reading selection - unrecognized request",
			     0);
			err++;
			return(SELN_UNRECOGNIZED);
		}
		reply += sizeof(Seln_attribute);
		len = buffer->buf_size - sizeof(Seln_attribute);
		selection_len = *(int *)reply;
		reply += sizeof(int); /* this only works since an int is 4
				         bytes; all values must be padded to
				         4-byte word boundaries */
		len -= sizeof(int);

		/* create a buffer to store the selection */

		if (selection_bufs[selection_type] != NULL)
			free(selection_bufs[selection_type]);
		selection_bufs[selection_type] = malloc(selection_len + 1);
		if (selection_bufs[selection_type] == NULL) {
			panel_set(mesg_item, PANEL_LABEL_STRING, 
						"Out of memory!", 0);
			err++;
			return(SELN_FAILED);
		}
		selection_have_bytes = 0;

		/* start reading the selection */

		if (*(Seln_attribute *)reply !=	SELN_REQ_CONTENTS_ASCII) {
			panel_set(mesg_item, PANEL_LABEL_STRING, 
			     "Error reading selection - unrecognized request", 
			     0);
			err++;
			return(SELN_UNRECOGNIZED);
		}
		reply += sizeof(Seln_attribute);
		len -= sizeof(Seln_attribute);
		*buffer->requester.context = NOT_FIRST_BUFFER;
	} else {
		/* this is not the first buffer, so the contents of the buffer
		   is just more of the selection */

		reply = buffer->data;
		len = buffer->buf_size;
	}

	/* copy data from the received buffer to the selection buffer
	   allocated above */

	bytes_to_copy = selection_len - selection_have_bytes;
	if (len < bytes_to_copy)
		bytes_to_copy = len;
	strncpy(&selection_bufs[selection_type][selection_have_bytes],
							reply, bytes_to_copy);
	selection_have_bytes += bytes_to_copy;
	if (selection_have_bytes == selection_len)
		selection_bufs[selection_type][selection_len] = '\0';
	return(SELN_SUCCESS);
}

/***********************************************************/
/* panel routines                                          */
/***********************************************************/

/* panel initialization routine */

init_panel(panel)
Panel panel;
{
	mesg_item = panel_create_item(panel, PANEL_MESSAGE, 
			PANEL_LABEL_STRING,
				mesg_labels[PRIMARY_CHOICE][ITEM_CHOICE], 
			0);
	type_item = panel_create_item(panel, PANEL_CYCLE, 
			PANEL_LABEL_STRING,	"Set/Get: ", 
			PANEL_CHOICE_STRINGS,	"Primary Selection", 
						"Secondary Selection", 
						"Shelf", 
						0, 
			PANEL_NOTIFY_PROC,	change_label_proc, 
			PANEL_LABEL_X,		ATTR_COL(0), 
			PANEL_LABEL_Y,		ATTR_ROW(1), 
			0);
	source_item = panel_create_item(panel, PANEL_CYCLE, 
			PANEL_LABEL_STRING,	"Text item contains:", 
			PANEL_CHOICE_STRINGS,	"Selection", 
					"Filename Containing Selection", 
						0, 
			PANEL_NOTIFY_PROC,	change_label_proc, 
			0);
	text_item = panel_create_item(panel, PANEL_TEXT, 
			PANEL_LABEL_STRING,	
				text_labels[PRIMARY_CHOICE][ITEM_CHOICE], 
			PANEL_VALUE_DISPLAY_LENGTH, 20, 
			0);
	set_item[0] = panel_create_item(panel, PANEL_BUTTON, 
			PANEL_LABEL_IMAGE,	panel_button_image(panel, 
							"Set Selection", 15,0),
			PANEL_NOTIFY_PROC,	set_button_proc, 
			PANEL_LABEL_X,		ATTR_COL(0), 
			PANEL_LABEL_Y,		ATTR_ROW(5), 
			0);
	set_item[1] = panel_create_item(panel, PANEL_BUTTON, 
			PANEL_LABEL_IMAGE,	panel_button_image(panel, 
							"Set Secondary", 15,0),
			PANEL_NOTIFY_PROC,	set_button_proc, 
			PANEL_LABEL_X,		ATTR_COL(0), 
			PANEL_LABEL_Y,		ATTR_ROW(5), 
			PANEL_SHOW_ITEM,	FALSE, 
			0);
	set_item[2] = panel_create_item(panel, PANEL_BUTTON, 
			PANEL_LABEL_IMAGE,	panel_button_image(panel, 
							"Set Shelf", 15,0),
			PANEL_NOTIFY_PROC,	set_button_proc, 
			PANEL_LABEL_X,		ATTR_COL(0), 
			PANEL_LABEL_Y,		ATTR_ROW(5), 
			PANEL_SHOW_ITEM,	FALSE, 
			0);
	get_item[0] = panel_create_item(panel, PANEL_BUTTON, 
			PANEL_LABEL_IMAGE,	panel_button_image(panel, 
							"Get Selection", 15,0),
			PANEL_NOTIFY_PROC,	get_button_proc, 
			PANEL_LABEL_X,		ATTR_COL(20), 
			PANEL_LABEL_Y,		ATTR_ROW(5), 
			0);
	get_item[1] = panel_create_item(panel, PANEL_BUTTON, 
			PANEL_LABEL_IMAGE,	panel_button_image(panel, 
							"Get Secondary", 15,0),
			PANEL_NOTIFY_PROC,	get_button_proc, 
			PANEL_SHOW_ITEM,	FALSE, 
			PANEL_LABEL_X,		ATTR_COL(20), 
			PANEL_LABEL_Y,		ATTR_ROW(5), 
			0);
	get_item[2] = panel_create_item(panel, PANEL_BUTTON, 
			PANEL_LABEL_IMAGE,	panel_button_image(panel, 
							"Get Shelf", 15,0),
			PANEL_NOTIFY_PROC,	get_button_proc, 
			PANEL_SHOW_ITEM,	FALSE, 
			PANEL_LABEL_X,		ATTR_COL(20), 
			PANEL_LABEL_Y,		ATTR_ROW(5), 
			0);
}

/*
 * change the label of the text item to reflect the currently chosen selection
 * type
 */

static void
change_label_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
	int old_selection_type = selection_type;

	selection_type = (int)panel_get_value(type_item);
	selection_source = (int)panel_get_value(source_item);
	panel_set(text_item, PANEL_LABEL_STRING,
			text_labels[selection_type][selection_source], 0);
	panel_set(mesg_item, PANEL_LABEL_STRING,
			mesg_labels[selection_type][selection_source], 0);
	if (old_selection_type != selection_type) {
		panel_set(set_item[old_selection_type],
				PANEL_SHOW_ITEM, FALSE, 0);
		panel_set(set_item[selection_type],
				PANEL_SHOW_ITEM, TRUE, 0);
		panel_set(get_item[old_selection_type],
				PANEL_SHOW_ITEM, FALSE, 0);
		panel_set(get_item[selection_type],
				PANEL_SHOW_ITEM, TRUE, 0);
	}
}
