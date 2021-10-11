/*      @(#)io_stream.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef suntool_io_stream_DEFINED
#define suntool_io_stream_DEFINED
#include <sunwindow/sun.h> 

#ifndef makedev
#include <sys/types.h>
#endif


/* TYPES */

struct	input_ops_vector {
	int	(*str_getc) ();	/* (struct Stream *in)
				 * Note: it is the responsibility of the
				 * implementor of the stream to return EOF
				 * to indicate that the stream is empty,
				 * and to insure that subsequent attempts
				 * to read from that stream will continue
				 * to return EOF.
				 */
	int	(*ungetc) ();	/* (char c, struct Stream *in)
				 * implementor only responsible for
				 * for putting back characters that were there.
				 * should return EOF if was unable to put back
				 * the character.
				 * Note: if not provided by implementor,
				 * i.e., ungetc = NULL, will be supplied
				 * by stream package first time it is required.
*** NOT IMPLEMENTED YET ***
				 * See comment regarding ungetc below. 
				 */
	char	*(*str_fgets) ();/* (char *, n, struct Stream *in)
				 * reads n-1 characters, or up to a newline
				 * character, whichever comes first,
				 * from the stream in into the string s.
				 * The last character read into s is
				 * followed by a null character.
				 * The newline is included.						 * returns NULL on end of file or error.
				 * not every stream will implement this
				 * procedure. For those that don't, the
				 * equivalent n calls to str_getc will be
				 * performed.
				 */
	int	(*chars_avail) (); /* (struct Stream *in) */
	struct posrec (*get_pos) ();	/* (struct Stream *in) 
				 * returns postion record consisting of
				 * line number and character position,
				 * character position is how many characters have been read.
				 * this is the value to give to set_pos
				 * if lineno = -1, means stream doesn't know
				 * lineno. This can occur after having
				 * reset with set_pos
				 */
	int	(*set_pos) ();	/* (int n, struct Stream *in)
				 * not every input stream will implement this
				 * procedure. returns -1 if not implemented
				 * or some other failure.
				 */
	
	void	(*close) ();	/* (struct Stream *in) */
};

struct	output_ops_vector {
	int	(*str_putc) ();	/* (char c, struct Stream *out) */
	void	(*str_fputs) ();	/* (char *, struct Stream *out)
				 * copies the null-terminated string s to out.
				 * does not append a newline
				 */
	struct posrec	(*get_pos) ();	/* (struct Stream *out) */
	void	(*flush) ();	/* (struct Stream *out)
				 * can be defaulted to NULL = NOP
				 */
	void	(*close) ();	/* (struct Stream *stream) */
};

union	Ops_Pointer {
	struct input_ops_vector		*input_ops;
	struct output_ops_vector	*output_ops;
};

enum Stream_Type {Input, Output};

struct	Stream {
	enum Stream_Type	stream_type;
	char			*stream_class; /* e.g. "String_Input_Stream" */
	union Ops_Pointer	ops; /* the procedures that implement the stream */
	struct Stream		*backing_stream;	/* for streams that are composed by layering one stream on top of the other, such as filtered_comments_stream. Allows clients to get at the lower stream, e.g. to find out how many characters were seen at that level */
	caddr_t			client_data;
};

struct	posrec {
	int		lineno;
	int		charpos;
};

typedef	struct Stream STREAM;

/* GENERIC INPUT FUNCTIONS */

int stream_getc();
/* int stream_getc(in)
 *	STREAM	*in;
 * int so can return EOF
 */

stream_ungetc();
/* char stream_ungetc(c, in)
 *	STREAM	*in;
 *	char		c;
 * if stream does not implement ungetc, first time ungetc is called
 * the stream will be redefined to be a layered stream which knows
 * how to implement ungetc. This will also occur if client tries
 * to put back a character  other than the one that was just read
 * and stream is unprepared to deal with
 * this, e.g. a file stream.   
 */

int stream_peekc();
/* int stream_peekc(in)
 *	struct Stream	*in;
 * returns next character in stream without consuming it, i.e., the
 * next call to getc or peekc will see the same character
 * int so can return EOF. uses ungetc
 */

char *stream_gets();
/* char *stream_gets(s, in)
 * 	char		 *s;
 *	STREAM	*in;
 * analagous to gets:
 * reads characters from in until newline is read.
 * Terminating newline character is not stored in s, and s is terminated
 * with null character
  * newlines preceded by \  are treated the same as any other character, i.e.
 * both the \ and the newline are copied into s. 
 * if an error occurs, or if no characters are read, the value NULL is returned
 *

char *stream_fgets();
/* char *stream_fgets(s, n, in)
 * 	char		 *s;
 *	int	n;
 *	struct Stream	*in;
 * analagous to fgets, i.e., like stream_gets, except newline is included
 */

int stream_chars_avail();
/* int chars_avail(in)
 *	struct Stream	*in;
 */

stream_ungetstr();
/* stream_ungetstr(s, in)
 *	char		*s;
 *	struct Stream	*in;

*** NOT IMPLEMENTED YET ***
Implements "stuffing" of a sequence of characters.
redefines in so that the next n characters it returns are those in s.
Then in returns to its normal state. For a similar operation
involving another stream, see append_streams below.
*/

append_streams();
/* append_streams(in1, in2)
 *	struct Stream	in1, in2;
 *
*** NOT IMPLEMENTED YET ***
 * redefines in2 so that characters are read from in1 until it returns EOF,
 * then in2 is returned to its normal state, i.e. subsequent calls
 * to getc will return characters from in2.
 */
  
int stream_set_pos();
/* int stream_position(n, stream)
 *	int	n;
 * 	STREAM	*stream;
 * positions stream so that next character read will be the nth character.
 */

#ifndef CHARCLASS	 /* also defined in io_streams.h */
#define CHARCLASS
enum CharClass {Break, Sepr, Other};
#endif

char *stream_get_token ();
/* char *stream_get_token (in, dest, charproc)
 *	struct Stream	*in;
 *	char *dest;
 *	enum CharClass (*charproc) ();

 * stream_get_token is used for tokenizing input, where more degree of
 * flexibility is required than simply delimiting tokens by white spaces
 * characters are divided into three classes, Break, Sepr, and Other.
 * separators (Sepr) serve to delimit a token. Leading separators are skipped.
 * think of separators as white space. Break characters delimit tokens, and
 * are themselves tokens. Thus, if a break character is the first thing seen
 * it is returned as the token. If any non-separator characters have been seen,
 * then they are returned as the token, and the break character will be the
 * returned as the result of the next call to get_token.
 * for example, if charproc returns Sepr for space, and Break for '(' and ')'
 * and Other for all alphabetic characters, then the string "now (is) the"
 * will yield five tokens consisting of "now" "(" "is" ")" and "the"

 * get_token stores the token that it constructs into dest,
 * which is also returned as its value.

 * get_token returns NULL, rather than the empty string, corresponding to
 * the case where the token is empty
 */


void skip_over();
/* skip_over(in, charproc)
 *	struct Stream	*in;
 *	enum CharClass (*charproc) ();
 * reads and discards all characters from in for which charproc
 * returns sepr.
 * e.g. skip_over(in, white_space) skips over all white space
 */
  
#ifndef WHITESPACE
#define WHITESPACE
extern enum CharClass white_space();
/* returns sepr for blanks, newlines, and tabs, other for everything else */
#endif


#ifndef CHARACTION	 /* also defined in io_streams.h */
#define CHARACTION
struct CharAction {
	Bool	stop;
	Bool	include;
};
#endif

char * stream_get_sequence ();
/* char *stream_get_sequence (in, dest, charproc);
 *	struct Stream	*in;
 *	char *dest;
 *	struct CharAction (*charproc) ();

 * stream_get_sequence is a more primitive tokenizer than get_token.
 * it takes a procedure which for each character specifies whether the
 * character is to terminate the sequence, and whether or not the
 * character is to be included in the sequence.
 * (If the character terminates the sequence, but is not included, then
 * it will be seen again on the next call.)
 * For example, having seen a \"\, to read to the matching \"\, call 
 * get_sequence with an action procedure that returns {TRUE, TRUE} for \"\
 * and  {FALSE, TRUE} for everything else. (If you want to detect the
 * case where a " is preceded by a \\, simply save the last character
 * and modify the procedure accordingly.

 * Note that gettoken can be defined in terms of get_sequence by
 * having Other characters return {FALSE, TRUE}, and also noticing whether
 * any have been seen yet, having Seprs return
 * {(seen_some_others ? TRUE : FALSE), FALSE}
 * and Break characters return {TRUE, (seen_some_others ? FALSE : TRUE)}

 * returns NULL for the empty sequence
 */

#ifndef EVERYTHING
#define EVERYTHING
struct CharAction everything();
/* returns {FALSE, TRUE} for all characters */
#endif


/* GENERIC OUTPUT FUNCTIONS */

int stream_putc();
/* stream_putc(c, out)
 *	char		c;
 *	struct Stream	*out;
 * returns c if successful, otherwise EOF
 */

void stream_puts();
/* stream_puts(s, out)
 *	char		*s;
 *	struct Stream	*out;
 * analagous to puts:
 * writes s to out until a null is encountered (which is not written). 
 * then writes a newline.
 */

void stream_fputs();
/* stream_fputs(s, out)
 *	char		*s;
 *	struct Stream	*out;
 * analagous to fputs, i.e. like stream_puts but does not include newline
 */


/* GENERIC FUNCTIONS THAT APPLY TO BOTH INPUT AND OUTPUT */

void stream_close();
/* void stream_close(stream)
 *	struct Stream	*stream;
 */

struct posrec stream_get_pos();
/* int stream_position(stream)
 * 	STREAM	*stream;
 * returns number of lines and characters read from/ written to stream
 * lineno = -1, means stream doesn't know line number. This occurs
 * if stream has been reset via setpos, or implementor simply doesn't
 * bother to keep count
 * Note also that lineno may be inaccurate if someone calls puts
 * on a string which contains a newline in the middle somewhere.
 */

void stream_flush();
/* void stream_flush(out)
 *	struct Stream	*out;
 */


/* VARIOUS STREAM TYPES */

struct Stream *string_input_stream();
/* struct Stream *string_input_stream(s, in)
 *	char	*s;
 *	struct Stream	*in; 
		if in =  NULL, creates new one, otherwise reuses this one
 */

struct Stream *string_output_stream();
/* struct Stream *string_output_stream(s, out)
 *	char	*s;
 *	struct Stream	*out; 
		if out =  NULL, creates new one, otherwise reuses this one
 */

struct Stream *file_input_stream();
/* struct Stream *file_input_stream(s, fp)
 *	char	*s;
 *	FILE	*fp;	
 *	if fp != NULL, it is used and s is ignored, otherwise
 * s is opened using fopen, with nmode "r"
 * e.g. file_input_stream(NULL, STDIN) works.
 */

struct Stream *file_output_stream();
/* struct Stream *file_output_stream(s, fp, append)
 *	char	*s;
 * 	FILE 	*fp;
 *	Bool	append;
 *	if fp != NULL, it is used and s is ignored, otherwise
 * s is opened using fopen and mode "w" is append is FALSE, otherwise "a" 
 * e.g. file_output_stream(NULL, stdout) works.
 */

struct Stream *filter_comments_stream();
/* struct Stream *filter_comments_stream(in)
 *	struct Stream *in;
 *	creates a stream that obtains its input from in and discards all
 * characters that are inside of comments, where a comment is either
 * a line that begins with a #, or a sequence of characters bracketed by
 * slash* and *slash.
 */

#endif
