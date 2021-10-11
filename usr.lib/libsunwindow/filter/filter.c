#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)filter.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <ctype.h>
#include <strings.h>
#include <sunwindow/io_stream.h>
#include <sunwindow/string_utils.h>
#include <sunwindow/sun.h>
#include <sunwindow/filter.h>
/*
 * This module parses a file which specifies the mapping from key stations to
 * filters to be invoked from the editor. The form of each entry in the file
 * is: key_station filter_name program_name arguments 
 *
 * where key_station is either Cnnn, or identifier(nn), e.g. L1, KEY_LEFT(1). 
 *
 * Each entry must be separated from the next by a blank line. Comments may
 * appear in the file, as indicated by either the appearance of a '#' at the
 * beginning of a line, or the C convention using /'s and *'s. 
 *
 * The parser returns an array of pointers to structures (filter_rec)
 * containing the key name, the integer, the filter name, and an argc-argv
 * array describing the program call. 
 *
 * Unresolved issues:  reporting error positions. 
 */

char	*stream_fgets();

enum CharClass
breakProc();
struct CharAction
digits();


struct filter_rec **
parse_filter_table(in, filename)
	STREAM         *in;
	char			*filename;	/* use this to label errors */
{
	struct filter_rec *rec_array[50];
	char           *arg_array[20];
	STREAM         *scratch_stream = NULL;
	char            scratch[256], scratch1[32];
	int             keynum, i, j, k;
	struct filter_rec *rec;
	struct filter_rec **val;
	struct posrec   pos;

	for (i = 0;; i++) {
		if (stream_get_token(in, scratch, breakProc) == NULL)
			break;
		rec = (struct filter_rec *) malloc(sizeof (struct filter_rec));
		if (rec == NULL) {
			(void) fprintf(stderr, "%s: malloc failed\n", filename);
			return (NULL);
		}
		rec->key_name = strdup(scratch);
		if (stream_get_sequence(in, scratch, digits) != NULL)
			keynum = atoi(scratch);
		else if (!strequal(stream_get_token(in, scratch, breakProc),
				   ")")) {
			(void)stream_get_sequence(in, scratch, digits);
			keynum = atoi(scratch);
			if (!strequal(stream_get_token(in, scratch, breakProc),
				      ")"))
				goto error;
		} else
			goto error;

		rec->key_num = keynum;
		(void)stream_get_token(in, scratch, white_space);
		rec->class = strdup(scratch);
		(void)stream_getc(in);/* discard the newline */
		scratch_stream = string_input_stream(
			     stream_fgets(scratch, 256, in), scratch_stream);

		/* So what if someone fails to specify a command after the
		 * key definition
		 */
		if (!strcmp(scratch, "\n") || !strcmp(scratch, "")) {
			fprintf(stderr, "%s: missing command-line; ", filename);
			pos = stream_get_pos(in);
			goto errorNoSkip;
		}
		/*
		 * storage for stream allocated first time it is needed.
		 * subsequent times, will be reused 
		 */
		if (any_shell_meta(scratch)) {
			char *shell;
			extern char *getenv();

			if ((shell = getenv("SHELL")) == NULL)
				shell = "/bin/sh";
			rec->call = (char **)sv_calloc(4, sizeof (char *));
			rec->call[0] = shell;
			rec->call[1] = "-c";
			rec->call[2] = strdup(scratch);
		} else {
			/* tokenize the call, copying results into arg_array */
			for (j = 0, k = 0;
			     (stream_get_token(scratch_stream,
					       scratch1, white_space) != NULL);
			     j++)
				arg_array[j] = strdup(scratch1);
			/*
			 * allocate a new array of appropriate size (j+1 so
			 * client will know where the array ends) 
			 */
			rec->call = (char **) sv_calloc((unsigned) j + 1,
						     sizeof (char *));
			/* and copy the strings from arg_array into it */
			for (k = 0; k < j; k++)
				rec->call[k] = arg_array[k];
		}
		rec_array[i] = rec;
		continue;
error:
		pos = stream_get_pos(in);
		for (; (stream_fgets(scratch, 256, in) != NULL);)
			/* skip characters until you see a blank line */
			if (scratch[0] == '\n')
				break;
		fprintf(stderr, "%s: ", filename);
errorNoSkip:
		if (pos.lineno != -1)
			(void) fprintf(stderr,
				"problem on line number %d\n", pos.lineno);
		else
			(void) fprintf(stderr,
				"problem near character position %d\n",
				pos.charpos);
		i--;
	}
	val = (struct filter_rec **) calloc((unsigned) i + 1,
					    sizeof (struct filter_rec *));
	if (val == NULL) {
		(void) fprintf(stderr, "%s: calloc failed\n", filename);
		return (NULL);
	}
	for (j = 0; j < i; j++)
		val[j] = rec_array[j];
	return (val);
}

void
free_filter_table(table)
	struct filter_rec **table;
{
	int             i;
	for (i = 0; table[i] != NULL; i++) {
		cfree((char *) table[i]->call);
		free((char *) table[i]);
	}
	cfree((char *) table);
}

static struct CharAction
digits(c)
	char            c;
{
	struct CharAction val;

	if (isdigit(c)) {
		val.include = True;
		val.stop = False;
	} else {
		val.include = False;
		val.stop = True;
	}
	return (val);
}


static enum CharClass
breakProc(c)
	char            c;
{
	switch (c) {
	    case ' ':
	    case '\n':
	    case '\t':
		return (Sepr);
	    case '(':
	    case ')':
		return (Break);
	    default:
		return (isdigit(c) ? Break : Other);
	}
}

/*
 * Are there any shell meta-characters in string s?
 */
static
any_shell_meta(s)
	register char *s;
{

	while (*s) {
		if (index("~{[*?$`'\"\\", *s))
			return (1);
		s++;
	}
	return (0);
}
