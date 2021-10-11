/*	@(#)string_utils.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef suntool_string_utils_DEFINED
#define suntool_string_utils_DEFINED

#include <sunwindow/sun.h> /* defines TRUE, FALSE */


extern int string_find();
/* int string_find(s, target, case_matters)
 *	char *s, *target;
 *	Bool case_matters;
 * strfind searches one instance of a string for another.
 * If successful, returns the position in the string where the match began,
 * otherwise -1.
 * If case_matters = FALSE, 'a' will match with 'a' or 'A'.
 */

extern Bool string_equal();
/* Bool string_equal(s1, s2, case_matters)
 *	char *s1, *s2;
 *	Bool case_matters;
 * strequal compares two strings. 
 * If case_matters = FALSE, 'a' will match with 'a' or 'A'.
 * either s1 or s2 can be NULL without harm.
 */


extern Bool substrequal();
/* Bool substrequal(s1, start1, s2, start2, n, case_matters)
 *	char *s1, *s2;
 *	int start1, start2;
 *	Bool case_matters;
 * substrequal compares two substrings without having to construct them.
 * If case_matters = FALSE, 'a' will match with 'a' or 'A'.
 */

#ifndef CHARCLASS	 /* also defined in io_streams.h */
#define CHARCLASS
enum CharClass {Break, Sepr, Other};
#endif

extern char *string_get_token();
/*	char *s;
 *	int *index;
 *	char *dest;
 *	enum CharClass (*charproc) (char c);

 * string_get_token is used for tokenizing input, where more degree of
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
 * index marks the current position in the string to "begin reading from"
 * it is updated so that the client program does not have to keep track of
 * how many characters have been read.

 * get_token returns NULL, rather than the empty string, corresponding to
 * the case where the token is empty
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

extern char *string_get_sequence();
/*	char *s;
 *	int *index;
 *	char *dest;
 *	struct CharAction (*charproc) ();

 * string_get_sequence is a more primitive tokenizer than get_token.
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

#endif


