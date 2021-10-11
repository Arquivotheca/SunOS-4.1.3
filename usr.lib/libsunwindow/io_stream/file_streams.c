#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)file_streams.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/io_stream.h>

/* STREAM FROM FILE */

#define GetFISData struct file_input_stream_data *data = (struct file_input_stream_data*) in->client_data

static struct file_input_stream_data {
	FILE           *fp;
	int             lineno;
};

static void
file_input_stream_close(in)
	STREAM         *in;
{
	GetFISData;
	(void) fclose(data->fp);
	free((char *) data);
}

static int
file_input_stream_getc(in)
	STREAM         *in;
{
	char            c;
	GetFISData;

	c = fgetc(data->fp);
	if (c == '\n' && data->lineno != -1)
		data->lineno++;
	return (c);
}

static struct posrec
file_input_stream_get_pos(in)
	STREAM         *in;
{
	GetFISData;
	struct posrec   p;

	p.charpos = ftell(data->fp);
	p.lineno = data->lineno;
	return (p);
}
static int
file_input_stream_set_pos(in, n)
	STREAM         *in;
	int             n;
{
	GetFISData;

	(void) fseek(data->fp, (long) n, 0);
	data->lineno = -1;
	return (n);
}

static int
file_input_stream_ungetc(c, in)
	char            c;
	STREAM         *in;
{
	GetFISData;
	if (c == '\n' && data->lineno != -1)
		data->lineno++;
	return (ungetc(c, data->fp));
}

static char    *
file_input_stream_fgets(s, n, in)
	char           *s;
	int             n;
	STREAM         *in;
{
	GetFISData;
	if (fgets(s, n, data->fp) == NULL)
		return (NULL);
	if (data->lineno != -1 && s[strlen(s) - 1] == '\n')
		data->lineno++;
	return (s);
}

static int
file_input_stream_chars_avail(in)
	STREAM         *in;
{
	int             pos, val;
	GetFISData;

	pos = ftell(data->fp);	/* where we are now */
	(void) fseek(data->fp, (long) 0, 2);
	val = (ftell(data->fp) - pos);	/* end of file */
	(void) fseek(data->fp, (long) pos, 0);
	return (val);
}

static struct input_ops_vector file_input_stream_ops = {
	file_input_stream_getc,
	file_input_stream_ungetc,
	file_input_stream_fgets,
	file_input_stream_chars_avail,
	file_input_stream_get_pos,
	file_input_stream_set_pos,
	file_input_stream_close
};

STREAM         *
file_input_stream(s, fp)
	char           *s;
	FILE           *fp;
{
	STREAM         *value;
	struct file_input_stream_data *data;

	if (fp == NULL) {
		if ((fp = fopen(s, "r")) == NULL)
			return (NULL);
	}
	value = (STREAM *) malloc(sizeof (STREAM));
	if (value == NULL) {	/* malloc can fail */
		(void) fprintf(stderr, "malloc failed\n");
		return (NULL);
	}
	value->stream_type = Input;
	value->stream_class = "Input Stream From File";
	value->ops.input_ops = &file_input_stream_ops;
	data = (struct file_input_stream_data *) malloc(
				     sizeof (struct file_input_stream_data));
	if (data == NULL) {
		(void) fprintf(stderr, "malloc failed\n");
		return (NULL);
	}
	data->fp = fp;
	data->lineno = 1;
	value->client_data = (caddr_t) data;
	return (value);
}

/* STREAM TO FILE */

#define GetFOSData struct file_output_stream_data *data = (struct file_output_stream_data*) out->client_data

static struct file_output_stream_data {
	FILE           *fp;
	int             lineno;
};

static void
file_output_stream_close(out)
	STREAM         *out;
{
	GetFOSData;
	(void) fclose(data->fp);
	free((char *) data);
}

static void
file_output_stream_flush(out)
	STREAM         *out;
{
	GetFOSData;
	(void) fflush(data->fp);
	free((char *) data);
}

static int
file_output_stream_putc(c, out)
	char            c;
	STREAM         *out;
{
	GetFOSData;
	if (c == '\n' && data->lineno != -1)
		data->lineno++;
	return (fputc(c, data->fp));
}

static void
file_output_stream_fputs(s, out)
	char           *s;
	STREAM         *out;
{
	GetFOSData;
	fputs(s, data->fp);
}

static struct posrec
file_output_stream_get_pos(out)
	STREAM         *out;
{
	struct posrec   p;
	GetFOSData;

	p.charpos = ftell(data->fp);
	p.lineno = data->lineno;
	return (p);
}


static struct output_ops_vector file_output_stream_ops = {
	file_output_stream_putc,
	file_output_stream_fputs,
	file_output_stream_get_pos,
	file_output_stream_flush,
	file_output_stream_close
};


STREAM         *
file_output_stream(s, fp, append)
	char           *s;
	FILE           *fp;
	Bool            append;
{
	STREAM         *value;
	struct file_output_stream_data *data;

	if (fp == NULL) {
		fp = fopen(s, (append == True ? "a" : "w"));
		if (fp == NULL)
			return (NULL);
	}
	value = (STREAM *) malloc(sizeof (STREAM));
	if (value == NULL) {	/* malloc can fail */
		(void) fprintf(stderr, "malloc failed\n");
		return (NULL);
	}
	value->stream_type = Output;
	value->stream_class = "Output Stream To File";
	value->ops.output_ops = &file_output_stream_ops;
	data = (struct file_output_stream_data *) malloc(
				    sizeof (struct file_output_stream_data));
	if (data == NULL) {
		(void) fprintf(stderr, "malloc failed\n");
		return (NULL);
	}
	data->fp = fp;
	data->lineno = 1;
	value->client_data = (caddr_t) data;
	return (value);
}
