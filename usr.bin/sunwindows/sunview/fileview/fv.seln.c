#ifndef lint
static  char sccsid[] = "@(#)fv.seln.c 1.1 92/07/30 Sun Micro";
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

#include <stdio.h>
#include <string.h>

#ifdef SV1
#include <suntool/sunview.h>
#include <suntool/seln.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#include <suntool/panel.h>
#else
#include <view2/view2.h>
#include <view2/seln.h>
#include <view2/canvas.h>
#include <view2/scrollbar.h>
#include <view2/panel.h>
#endif

#include "fv.port.h"
#include "fv.h"
#include "fv.extern.h"

#define PRIMARY_CHOICE		0	/* get/set the primary selection */
#define SECONDARY_CHOICE	1	/* get/set the secondary selection */
#define SHELF_CHOICE		2	/* get/set the shelf */

#define FIRST_BUFFER		0
#define NOT_FIRST_BUFFER	1

static int Selection_type = PRIMARY_CHOICE;

static Seln_client S_client;	/* selection client handle */
static char *Selection_bufs[3];	/* contents of each of the three selections;
				   they are set only when the user hits a set
				   or a get button */

extern char *fv_malloc();
int fv_func_key_proc();
Seln_result fv_reply_proc();
Seln_result fv_read_proc();

fv_startseln()
{
	S_client = seln_create(fv_func_key_proc, fv_reply_proc, NULL);
	if (S_client == NULL) {
		(void)fprintf(stderr, Fv_message[MESELN]);
		exit(1);
	}
}


fv_stopseln()
{
	seln_destroy(S_client);
}


char *
fv_shelve_primary_selection(cut)
	BOOLEAN cut;		/* Cut rather than copy? */
{
	char *value;
	char *fv_getmyselns();

	if (seln_acquire(S_client, SELN_SHELF) != SELN_SHELF) {
		(void)fprintf(stderr, Fv_message[MESELN]);
		return(NULL);
	}
	if (Selection_bufs[SHELF_CHOICE])
		free(Selection_bufs[SHELF_CHOICE]);

	if (value = fv_getmyselns(cut))
	{
		Selection_bufs[SHELF_CHOICE] = fv_malloc((unsigned)strlen(value)+1);
		if (Selection_bufs[SHELF_CHOICE])
			(void)strcpy(Selection_bufs[SHELF_CHOICE], value);
	}

	return(value);
}


fv_set_primary_selection()
{
	char *value;
	char *fv_getmyselns();

	if (seln_acquire(S_client, SELN_PRIMARY) != SELN_PRIMARY) {
		(void)fprintf(stderr, Fv_message[MESELN]);
		return(NULL);
	}
	if (Selection_bufs[PRIMARY_CHOICE])
		free(Selection_bufs[PRIMARY_CHOICE]);

	if (value = fv_getmyselns(FALSE))
	{
		Selection_bufs[PRIMARY_CHOICE] = fv_malloc((unsigned)strlen(value)+1);
		if (Selection_bufs[PRIMARY_CHOICE])
			(void)strcpy(Selection_bufs[PRIMARY_CHOICE], value);
	}
}


static void
to_shelf(cut)
	BOOLEAN cut;
{
	if (fv_shelve_primary_selection(cut))
		fv_putmsg(FALSE, Fv_message[MHOWTOPASTE],
		(int)(cut ? "Move" : "Copy"), 0);
}


void
fv_copy_to_shelf()
{
	to_shelf(FALSE);
}


void
fv_cut_to_shelf()
{
	to_shelf(TRUE);
}


fv_paste()
{
	Seln_holder holder;
	char context = FIRST_BUFFER;

	/* Who has the selection? */

	holder = seln_inquire(SELN_SHELF);
	if (holder.state == SELN_NONE)
	{
		holder = seln_inquire(SELN_PRIMARY);
		if (holder.state == SELN_NONE)
		{
			fv_putmsg(TRUE, Fv_message[MENOSELN], 0, 0);
			return;
		}
	}


	/* Ask for length and contents of selection,
	 * ...fv_read_proc() reads it in.
	 */

	(void)seln_query(&holder, fv_read_proc, &context,
		SELN_REQ_BYTESIZE, 0,
		SELN_REQ_CONTENTS_ASCII, 0,
		0);

	/* Paste primary/shelf selections */

	if (Selection_bufs[SHELF_CHOICE])
		fv_paste_seln(Selection_bufs[SHELF_CHOICE]);
	else if (Selection_bufs[PRIMARY_CHOICE])
		fv_paste_seln(Selection_bufs[PRIMARY_CHOICE]);
	else
		fv_putmsg(TRUE, Fv_message[MENOSELN], 0, 0);
}


Seln_result
fv_read_proc(buffer)
	Seln_request *buffer;
{
	char *reply;
	unsigned len;
	int bytes_to_copy;
	static int selection_have_bytes;
	static int selection_len;

	if (*buffer->requester.context == FIRST_BUFFER)
	{
		if (buffer == NULL ||
		    *((Seln_attribute *)buffer->data) != SELN_REQ_BYTESIZE)
			return(SELN_UNRECOGNIZED);

		reply = buffer->data + sizeof(Seln_attribute);

		selection_len = *(int *)reply;
		reply += sizeof(int);
		len = buffer->buf_size - sizeof(Seln_attribute) - sizeof(int);

		if (Selection_bufs[Selection_type])
			free(Selection_bufs[Selection_type]);
		if ((Selection_bufs[Selection_type] = fv_malloc((unsigned)selection_len+1)) == NULL)
			return(SELN_FAILED);
		selection_have_bytes = 0;

		if (*(Seln_attribute *)reply != SELN_REQ_CONTENTS_ASCII)
		{
			fv_putmsg(TRUE, Fv_message[MESELN], 0, 0);
			return(SELN_FAILED);
		}
		reply += sizeof(Seln_attribute);
		len -= sizeof(Seln_attribute);
		*buffer->requester.context = NOT_FIRST_BUFFER;
	}
	else
	{
		reply = buffer->data;
		len = buffer->buf_size;
	}

	/* Copy from received buffer to selection buffer */

	bytes_to_copy = selection_len - selection_have_bytes;
	if (len < bytes_to_copy)
		bytes_to_copy = len;
	(void)strncpy(&Selection_bufs[Selection_type][selection_have_bytes],
		reply, bytes_to_copy);
	selection_have_bytes += bytes_to_copy;
	if (selection_have_bytes == selection_len)
		Selection_bufs[Selection_type][selection_have_bytes] = NULL;

	return(SELN_SUCCESS);
}

/*
 * func_key_proc
 *
 * called by the selection service library whenever a change in the state of
 * the function keys requires an action (for instance, put the primary
 * selection on the shelf if the user hit PUT)
 */
/*ARGSUSED*/
fv_func_key_proc(client_data, args)
	char *client_data;		/* unused */
	Seln_function_buffer *args;
{
	Seln_holder *holder;
	Seln_request *buf;
	char *p;
	Seln_response i;
	
	/* Use seln_figure_response to decide what action to take */

	if ((i = seln_figure_response(args, &holder)) == SELN_SHELVE)
		if (seln_acquire(S_client, SELN_SHELF) != SELN_SHELF)
			fv_putmsg(TRUE, Fv_message[MESELN], 0, 0);
		else
			(void)fv_shelve_primary_selection(FALSE);
	else if (i == SELN_REQUEST)
	{
		if (holder->state == SELN_NONE)
			fv_putmsg(TRUE, Fv_message[MENOSELN], 0, 0);
		if (args->addressee_rank !=SELN_CARET)
			(void)fprintf(stderr, Fv_message[MESELN]);
		buf = seln_ask(holder, SELN_REQ_CONTENTS_ASCII, 0, 0);
		p = buf->data + sizeof(Seln_attribute);
	}
}


/*
 * reply_proc
 *
 * called by the selection service library whenever a request comes from
 * another client for one of the selections we currently hold
 */

Seln_result
fv_reply_proc(item, context, length)
	Seln_attribute item;
	Seln_replier_data *context;
	int length;
{
	int size, needed;
	char *seln, *destp;
	
	/* Choose the appropriate selection */

	switch (context->rank) {
	case SELN_PRIMARY:
		seln = Selection_bufs[PRIMARY_CHOICE];
		break;
	case SELN_SECONDARY:
		seln = Selection_bufs[SECONDARY_CHOICE];
		break;
	case SELN_SHELF:
		seln = Selection_bufs[SHELF_CHOICE];
		break;
	default:
		seln = NULL;
	}

	/* Process the request */

	switch (item) 
	{
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
			(void)strcpy(destp, context->context);
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
			(void)strncpy(destp, context->context, length);
			destp += length;
			context->response_pointer = (char **)destp;
			context->context += length;
			return(SELN_CONTINUED);
		}
	case SELN_REQ_YIELD:
		/* deselect the selection we have (turn off highlight, etc.) */

		*context->response_pointer++ = (char *)SELN_SUCCESS;
		if (context->rank == SELN_PRIMARY)
		{
			fv_filedeselect();
			fv_treedeselect();
		}
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
