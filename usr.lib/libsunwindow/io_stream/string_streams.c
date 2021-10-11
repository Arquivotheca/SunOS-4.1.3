#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)string_streams.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/io_stream.h>

/* STREAM FROM STRING */

#define GetSISData struct string_input_stream_data	*data = (struct string_input_stream_data*) in->client_data

static struct string_input_stream_data {
	char           *string;
	int             charpos;
};

static void
string_input_stream_close(in)
	STREAM         *in;
{
	GetSISData;
	free((char *) data);
}

static int
string_input_stream_getc(in)
	STREAM         *in;
{
	GetSISData;
	char            c = data->string[data->charpos];

	if (c == '\0')
		return (EOF);
	data->charpos++;
	return (c);
}

/* ARGSUSED */
static struct posrec
string_input_stream_get_pos(in, n)
	STREAM         *in;
	int             n;
{
	struct posrec   p;
	GetSISData;
	p.charpos = data->charpos;
	p.lineno = -1;		/* unlikely that lineno when reading from
				 * string will be interesting */
	return (p);
}

static int
string_input_stream_set_pos(in, n)
	STREAM         *in;
	int             n;
{
	GetSISData;
	data->charpos = n;
	return (0);
}

static int
string_input_stream_ungetc(c, in)
	char            c;
	STREAM         *in;
{
	GetSISData;

	if (c == data->string[data->charpos - 1]) {
		data->charpos--;
		return (c);
	} else			/* character being put back is not the one
				 * that was read last */
		return (EOF);
}

static int
string_input_stream_chars_avail(in)
	STREAM         *in;
{
	GetSISData;

	return (strlen(data->string) - data->charpos);
}

static struct input_ops_vector string_input_stream_ops = {
	string_input_stream_getc,
	string_input_stream_ungetc,
	NULL,			/* gets. IMPLEMENT USING strcpy */
	string_input_stream_chars_avail,
	string_input_stream_get_pos,
	string_input_stream_set_pos,
	string_input_stream_close
};


STREAM         *
string_input_stream(s, in)
	char           *s;
	STREAM         *in;	/* if NULL, creates new one, otherwise reuses
				 * this one */
{
	STREAM         *value;
	if (in != NULL) {
		GetSISData;
		data->string = s;
		data->charpos = 0;
		return (in);
	} else {
		struct string_input_stream_data	*data;

		value = (STREAM *) malloc(sizeof (STREAM));
		if (value == NULL) {	/* malloc can fail */
			(void) fprintf(stderr, "malloc failed\n");
			return (NULL);
		}
		value->stream_type = Input;
		value->stream_class = "Input Stream From String";
		value->ops.input_ops = &string_input_stream_ops;
		data = (struct string_input_stream_data *) malloc(
				   sizeof (struct string_input_stream_data));
		if (data == NULL) {
			(void) fprintf(stderr, "malloc failed\n");
			return (NULL);
		}
		data->string = s;
		data->charpos = 0;
		value->client_data = (caddr_t) data;
		return (value);
	}
}



/* STREAM TO STRING */

#define GetSOSData struct string_output_stream_data *data = (struct string_output_stream_data*) out->client_data

static struct string_output_stream_data {
	char           *string;
	int             charpos;
};

static void
string_output_stream_close(out)
	STREAM         *out;
{
	GetSOSData;
	free((char *) data);
}

static int
string_output_stream_putc(c, out)
	char            c;
	STREAM         *out;
{
	GetSOSData;

	data->string[data->charpos++] = c;
	data->string[data->charpos] = '\0';
	return (c);
}

/* ARGSUSED */
static struct posrec
string_output_stream_getpos(n, out)
	int             n;
	STREAM         *out;
{
	struct posrec   p;
	GetSOSData;
	p.charpos = data->charpos;
	p.lineno = -1;
	return (p);
}


static struct output_ops_vector string_output_stream_ops = {
	string_output_stream_putc,
	NULL,			/* fputs. IMPLEMENT WITH strcpy */
	string_output_stream_getpos,
	NULL,			/* flush */
	string_output_stream_close
};


STREAM         *
string_output_stream(s, out)
	char           *s;
	STREAM         *out;	/* if NULL, creates new one, otherwise reuses
				 * this one */
{
	STREAM         *value;
	if (out != NULL) {
		GetSOSData;
		data->string = s;
		data->charpos = 0;
		return (out);
	}
	else {
		struct string_output_stream_data	*data;
 
		value = (STREAM *) malloc(sizeof (STREAM));
		if (value == NULL) {	/* malloc can fail */
			(void) fprintf(stderr, "malloc failed\n");
			return (NULL);
		}
		value->stream_type = Output;
		value->stream_class = "Output Stream To String";
		value->ops.output_ops = &string_output_stream_ops;
		data = (struct string_output_stream_data *) malloc(
				  sizeof (struct string_output_stream_data));
		if (data == NULL) {
			(void) fprintf(stderr, "malloc failed\n");
			return (NULL);
		}
		data->string = s;
		data->charpos = 0;
		value->client_data = (caddr_t) data;
		return (value);
	}
}
