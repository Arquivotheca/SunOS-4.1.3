#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)other_streams.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/io_stream.h>

/* FILTER COMMENTS STREAM */

#define GetFCSData struct filter_comments_stream_data *data = (struct filter_comments_stream_data*) in->client_data

static struct filter_comments_stream_data {
	Bool            backed_up;
	char            backup, lastchar;
};

static void
filter_comments_stream_close(in)
	STREAM         *in;
{
	GetFCSData;
	free((char *) data);
}

static int
filter_comments_stream_getc(in)
	STREAM         *in;
{
	char            c, c1;
	STREAM         *backing_stream = in->backing_stream;

	GetFCSData;
	if (data->backed_up) {
		c = data->backup;
		data->backed_up = False;
	} else
		c = stream_getc(backing_stream);
	switch (c) {
	    case '/':
		if ((c1 = stream_getc(backing_stream)) == '*') {
			/* start of a slash * comment */
			FOREVER {
				if (stream_getc(backing_stream) != '*')
					continue;
				if (stream_getc(backing_stream) == '/')
					break; /* end of comment */
			}
			/* finished skipping over comment */
			c = stream_getc(backing_stream);
		} else {
			/*
			 * the / was not start of a comment. therefore we
			 * read one too far 
			 */
			(void) stream_ungetc(c1, in);
		}
		break;
	    case '#':
		if ((data->lastchar == '\n') || (data->lastchar == '\0'))
			/* start of a crunch comment */
			FOREVER {
				c = stream_getc(backing_stream);
				if (c == '\\')
					/*
					 * no matter what next character is,
					 * it is part of the comment so read
					 * and discard it 
					 */
					(void) stream_getc(backing_stream);
				else if (c == '\n')
					break; /* end of comment */
			}
		break;
	}
	data->lastchar = c;
	return (c);
}

static struct posrec
filter_comments_stream_get_pos(in)
	STREAM         *in;
{
	return (stream_get_pos(in->backing_stream));	/* client probably wants
							 * position in
							 * underlying stream */
}

static int
filter_comments_stream_ungetc(c, in)
	char            c;
	STREAM         *in;
{
	GetFCSData;
	data->backup = c;
	data->backed_up = True;
	return (c);
}

static int
filter_comments_stream_chars_avail(in)
	STREAM         *in;
{
	return (MIN(stream_chars_avail(in->backing_stream), 1));
	/*
	 * can't return the full amount as some of the characters may be in
	 * comments 
	 */

}

static struct input_ops_vector filter_comments_stream_ops = {
	filter_comments_stream_getc,
	filter_comments_stream_ungetc,
	NULL,			/* fgets */
	filter_comments_stream_chars_avail,
	filter_comments_stream_get_pos,
	NULL,			/* setpos */
	filter_comments_stream_close
};


STREAM         *
filter_comments_stream(in)
	STREAM         *in;
{
	STREAM         *value;
	struct filter_comments_stream_data *data;

	value = (STREAM *) malloc(sizeof (STREAM));
	if (value == NULL) {
		(void) fprintf(stderr, "malloc failed\n");
		return (NULL);
	}
	value->stream_type = Input;
	value->stream_class = "Filter Comments Stream";
	value->ops.input_ops = &filter_comments_stream_ops;
	value->backing_stream = in;
	data = (struct filter_comments_stream_data *) malloc(
				sizeof (struct filter_comments_stream_data));
	if (data == NULL) {
		(void) fprintf(stderr, "malloc failed\n");
		return (NULL);
	}
	data->backed_up = False;
	value->client_data = (caddr_t) data;
	return (value);
}
