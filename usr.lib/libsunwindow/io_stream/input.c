#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)input.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* last edited January 28, 1985, 13:19:14 */
#include <sunwindow/io_stream.h>

#define Get_Input_Ops \
	struct input_ops_vector *ops;\
	if (in->stream_type != Input) exit(1);\
	ops = in->ops.input_ops

char           *
stream_getstring();

/* GENERIC INPUT FUNCTIONS */

int
stream_chars_avail(in)
	STREAM         *in;
{
	Get_Input_Ops;
	return ((*ops->chars_avail) (in));
}

int
stream_getc(in)
	STREAM         *in;
{
	Get_Input_Ops;
	return ((*ops->str_getc) (in));
}

int
stream_peekc(in)
	STREAM         *in;
{
	int             c;
	c = stream_getc(in);
	if (c != EOF)
		(void) stream_ungetc(c, in);
	return (c);
}

int
stream_ungetc(c, in)
	char            c;
	STREAM         *in;
{
	Get_Input_Ops;
	return ((*ops->ungetc) (c, in));
	/* NOT IMPLEMENTED YET :  GENERIC CASE WHERE ungetc not DEFINED */
}

char           *
stream_gets(s, in)
	char           *s;
	STREAM         *in;
{
	return (stream_getstring(s, 10000, in, False));
}

char           *
stream_fgets(s, n, in)
	char           *s;
	int             n;
	STREAM         *in;
{
	return (stream_getstring(s, n, in, True));
}

static char    *
stream_getstring(s, n, in, include_newline)
	char           *s;
	int             n;
	STREAM         *in;
	Bool            include_newline;
{
	int             i, c;

	Get_Input_Ops;
	if (ops->str_fgets != NULL) {
		if ((*ops->str_fgets) (s, n, in) == NULL)
			return (NULL);
		else if (!include_newline) {
			i = strlen(s);
			if (s[i - 1] == '\n')
				s[i - 1] = '\0';
		}
		return (s);
	}
	/* str_fgets not defined. simulate with n calls to getc */
	for (i = 0; i < n; i++) {
		c = (*ops->str_getc) (in);
		if (c == EOF) {
			s[i] = '\0';
			break;
		} else if (c == '\n' && (i == 0 || s[i - 1] != '\\')) {
			/*
			 * \ followed by newline copies both \ and newline
			 * into s without terminating 
			 */
			if (include_newline)
				s[i++] = c;
			break;
		}
		s[i] = c;
	}
	if (i == 0)
		return (NULL);
	s[i] = '\0';
	return (s);
}

/*
 * stream_get_token is used for tokenizing input, where more degree of
 * flexibility is required than simply delimiting tokens by white spaces
 * characters are divided into three classes, Break, Sepr, and Other.
 * separators (Sepr) serve to delimit a token. Leading separators are
 * skipped. think of separators as white space. Break characters delimit
 * tokens, and are themselves tokens. Thus, if a break character is the first
 * thing seen it is returned as the token. If any non-separator characters
 * have been seen, then they are returned as the token, and the break
 * character will be the returned as the result of the next call to
 * get_token. for example, if charproc returns Sepr for space, and Break for
 * '(' and ')' and Other for all alphabetic characters, then the string "now
 * (is) the" will yield five tokens consisting of "now" "(" "is" ")" and
 * "the" 
 *
 * get_token stores the token that it constructs into dest, which is also
 * returned as its value. STREAM	*in; 
 *
 * get_token returns NULL, rather than the empty string, corresponding to the
 * case where the token is empty 
 */

char           *
stream_get_token(in, dest, charproc)
	STREAM         *in;
	char           *dest;
	enum CharClass  (*charproc) ();
{
	int             c;
	int             i = 0;
	for (;;) {
		c = stream_getc(in);
		if (c == EOF)
			goto done;
		switch ((*charproc) (c)) {
		    case Sepr:
			if (i != 0)	/* something seen */
				goto backup;
			else
				continue;
		    case Break:
			if (i == 0) {
				/*
				 * nothing seen yet, this character is the
				 * token 
				 */
				dest[i++] = (char) c;
				goto done;
			} else
				goto backup;
		    case Other:
			dest[i++] = (char) c;
		}
	}
backup:
	(void) stream_ungetc((char) c, in);
done:
	dest[i] = '\0';
	return (i == 0 ? NULL : dest);
}

/*
 * stream_get_sequence is a more primitive tokenizer than get_token. it takes
 * a procedure which for each character specifies whether the character is to
 * terminate the sequence, and whether or not the character is to be included
 * in the sequence. (If the character terminates the sequence, but is not
 * included, then it will be seen again on the next call.) For example,
 * having seen a \"\, to read to the matching \"\, call get_sequence with an
 * action procedure that returns {True, True} for \"\ and  {False, True} for
 * everything else. (If you want to detect the case where a " is preceded by
 * a \\, simply save the last character and modify the procedure accordingly. 
 *
 * Note that gettoken can be defined in terms of get_sequence by having Other
 * characters return {False, True}, and also noticing whether any have been
 * seen yet, having Seprs return {(seen_some_others ? True : False), False}
 * and Break characters return {True, (seen_some_others ? False : True)} 
 *
 * returns NULL for the empty sequence 
 */

char           *
stream_get_sequence(in, dest, charproc)
	STREAM         *in;
	char           *dest;
	struct CharAction (*charproc) ();
{
	int             c;
	struct CharAction action;
	int             i = 0;

	FOREVER {
		c = stream_getc(in);
		if (c == EOF)
			goto done;
		action = (*charproc) (c);
		if (action.include)
			dest[i++] = (char) c;
		if (action.stop) {
			if (!action.include)
				goto backup;	/* if c was not included,
						 * then need to put it back */
			else
				goto done;
		}
	}
backup:
	(void) stream_ungetc((char) c, in);
done:
	dest[i] = '\0';
	return (i == 0 ? NULL : dest);
}

void
skip_over(in, charproc)
	STREAM         *in;
	enum CharClass  (*charproc) ();

{
	char            c;
	FOREVER {
		c = stream_getc(in);
		if (charproc(c) != Sepr) {
			(void) stream_ungetc(c, in);
			break;
		}
	}
}

int
stream_set_pos(in, n)
	STREAM         *in;
	int             n;
{
	Get_Input_Ops;
	if (ops->set_pos)
		return (*ops->set_pos) (in, n);
	else
		return (-1);

}
