#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)output.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/io_stream.h>

#define Get_Output_Ops \
	struct output_ops_vector *ops;\
	if (out->stream_type != Output) exit(1);\
	ops = out->ops.output_ops


static void
stream_putstring();

/* GENERIC OUTPUT FUNCTIONS */

int
stream_putc(c, out)
	char            c;
	STREAM         *out;
{
	Get_Output_Ops;
	return ((*ops->str_putc) (c, out));
}

void
stream_puts(s, out)
	char           *s;
	STREAM         *out;
{
	stream_putstring(s, out, True);
}

void
stream_fputs(s, out)
	char           *s;
	STREAM         *out;
{
	stream_putstring(s, out, False);
}

static void
stream_putstring(s, out, include_newline)
	char           *s;
	STREAM         *out;
	Bool            include_newline;
{
	int             i;

	Get_Output_Ops;
	if (ops->str_fputs != NULL)
		(*ops->str_fputs) (s, out);
	else
		for (i = 0; s[i] != '\0'; i++)
			(*ops->str_putc) (s[i], out);

	if (include_newline)
		(*ops->str_putc) ('\n', out);

}


void
stream_flush(out)
	STREAM         *out;
{
	void            (*fn) ();

	Get_Output_Ops;
	fn = ops->flush;
	if (fn != NULL)
		(*fn) (out);
}
