#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)io.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/io_stream.h>

/* GENERIC FUNCTIONS THAT APPLY TO BOTH INPUT AND OUTPUT */

void
stream_close(stream)
	STREAM         *stream;
{
	switch (stream->stream_type) {
	    case Input:	{
		struct input_ops_vector *ops = stream->ops.input_ops;
		(*(ops->close)) (stream);
		goto out;
		}

	    case Output:	{
		struct output_ops_vector *ops = stream->ops.output_ops;
		(*(ops->close)) (stream);
		goto out;
		}

	    default:
		exit(1);
	}
out:	free((char *) stream);		/* client should have freed the client data */
}

struct posrec
stream_get_pos(stream)
	STREAM         *stream;
{
	switch (stream->stream_type) {
	    case Input:	{
		struct input_ops_vector *ops = stream->ops.input_ops;
		return (*ops->get_pos) (stream);
		}
	    case Output: {
		struct output_ops_vector *ops = stream->ops.output_ops;
		return (*ops->get_pos) (stream);
		}
	    default:
		exit(1);
		/* NOTREACHED */
	}
}
