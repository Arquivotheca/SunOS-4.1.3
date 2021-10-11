#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)string_utils.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <ctype.h>
#include <sunwindow/sun.h>
#include <sunwindow/string_utils.h>


/*
 * substring extracts a specified substring out of another string. It is a
 * generalization of strncpy. substring copies n characters from s to dest,
 * starting at position start. if start is negative, start = strlen(s) -
 * start. for example, substring(s, -3, 3, dest) will store into dest the
 * last three characters of s. returns True if successful, False if error,
 * e.g. n < 0, there weren't n charcters in s, etc. in case of failure, dest
 * will contain an empty string. 
 */
Bool
substring(s, start, n, dest)
	char           *s;
	int             start, n;
	char           *dest;
{
	int             slen;
	int             i;

	if (s == NULL)
		return (False);
	slen = strlen(s);
	if (start < 0)
		start = (slen - start);	/* negative numbers mean count from
					 * back */
	if ((start < 0) || (n < 0))
		goto fail;
	for (i = 0; i < n; i++)
		if (s[start + i] == '\0')
			goto fail;
		else
			dest[i] = s[start + i];
	dest[i] = '\0';
	return (True);
fail:	dest[0] = '\0';
	return (False);
}



/*
 * substrequal compares two substrings without having to construct them. If
 * case_matters = False, 'a' will match with 'a' or 'A'. 
 */
Bool
substrequal(s1, start1, s2, start2, n, case_matters)
	char           *s1, *s2;
	int             start1, start2;
	Bool            case_matters;
{
	int             i;

	if ((s1 == NULL) || (s2 == NULL))
		return ((n == 0 && s2 == s2) ? True : False);
	for (i = 0; i < n; i++) {
		char            c1, c2;
		c1 = s1[start1 + i];
		c2 = s2[start2 + i];
		if (c1 == c2) {
		} else if (case_matters)
			return (False);
		else if (isupper(c1)) {
			if (islower(c2)) {
				if ((c1 - 'A') != (c2 - 'a'))
					return (False);
			} else
				return (False);
		} else if (islower(c1)) {
			if (isupper(c2)) {
				if ((c1 - 'a') != (c2 - 'A'))
					return (False);
			} else
				return False;
		} else
			return False;
	}
	return (True);
}


/*
 * strequal compares two strings It uses substrequal. If case_matters =
 * False, 'a' will match with 'a' or 'A'. either s1 or s2 can be NULL without
 * harm. 
 */
Bool
string_equal(s1, s2, case_matters)
	char           *s1, *s2;
	Bool            case_matters;
{
	int             i;
	if (s1 == s2)
		return (True);
	else if ((s1 == NULL) || (s2 == NULL))
		return (False);
	for (i = 0;; i++) {
		char            c1, c2;
		c1 = s1[i];
		c2 = s2[i];
		if (c1 == c2) {
			if (s1[i] == '\0')
				return (True);
		} else if (case_matters)
			return (False);
		else if (isupper(c1)) {
			if (islower(c2)) {
				if ((c1 - 'A') != (c2 - 'a'))
					return (False);
			} else
				return (False);
		} else if (islower(c1)) {
			if (isupper(c2)) {
				if ((c1 - 'a') != (c2 - 'A'))
					return (False);
			} else
				return (False);
		} else
			return (False);
	}
}

/*
 * string_find searches one instance of a string for another. If successful,
 * returns the position in the string where the match began, otherwise -1. If
 * case_matters = False, 'a' will match with 'a' or 'A'. 
 */
int
string_find(s, target, case_matters)
	char           *s, *target;
	Bool            case_matters;
{
	int             i, n;
	if (s == NULL)
		return (-1);
	else if (target == NULL)
		return (0);
	n = strlen(target);
	for (i = 0;; i++)
		if (s[i] == '\0')
			return (-1);
		else if (substrequal(s, i, target, 0, n, case_matters))
			return (i);
}


/*
 * string_get_token is used for tokenizing input, where more degree of
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
 * returned as its value. index marks the current position in the string to
 * "begin reading from" it is updated so that the client program does not
 * have to keep track of how many characters have been read. 
 *
 * get_token returns NULL, rather than the empty string, corresponding to the
 * case where the token is empty 
 */

char           *
string_get_token(s, index, dest, charproc)
	char           *s;
	int            *index;
	char           *dest;
	enum CharClass  (*charproc) ();
{
	char            c;
	int             i = 0;
	for (;;) {
		c = s[(*index)++];
		if (c == '\0')
			goto backup;
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
				dest[i++] = c;
				goto exit;
			} else
				goto backup;
		    case Other:
			dest[i++] = c;
		}
	}
backup:
	(*index)--;
exit:
	dest[i] = '\0';
	return (i == 0 ? NULL : dest);
}


/*
 * string_get_sequence is a more primitive tokenizer than get_token. it takes
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
string_get_sequence(s, index, dest, charproc)
	char           *s;
	int            *index;
	char           *dest;
	struct CharAction (*charproc) ();
{
	char            c;
	struct CharAction action;
	int             i = 0;

	for (;;) {
		c = s[(*index)++];
		if (c == '\0')
			goto backup;
		action = (*charproc) (c);
		if (action.include)
			dest[i++] = c;
		if (action.stop) {
			if (!action.include)
				goto backup;	/* if c was not included,
						 * then need to back up */
			else
				goto exit;
		}
	}

backup:
	(*index)--;
exit:
	dest[i] = '\0';
	return (i == 0 ? NULL : dest);
}
