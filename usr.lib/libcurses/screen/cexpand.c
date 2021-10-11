/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)cexpand.c 1.1 92/07/30 SMI"; /* from S5R3 1.4 */
#endif

#include <stdio.h>
#include <ctype.h>
#include "print.h"

#define BACKSLASH	'\\'
#define BACKBACK	"\\\\"

extern char *strcat();

static char retbuffer[1024];
static char ret2buffer[1024];

/*
    Remove the padding sequences from the input string.
    Return the new string without the padding sequences
    and the padding itself in padbuffer.
*/
char *rmpadding (str, padbuffer, padding)
register char *str, *padbuffer;
int *padding;
{
    static char rmbuffer[1024];
    register char ch;
    register char *pbufptr;
    register char *retptr = rmbuffer;
    char *svbufptr;
    int padbylines = 0;
    int paddigits = 0;
    int paddecimal = 0;

    padbuffer[0] = rmbuffer[0] = '\0';
    if (padding)
	*padding = 0;
    if (str == NULL)
	return rmbuffer;

    while (ch = (*str++ & 0377))
	switch (ch)
	    {
	    case '$':
		if (*str == '<')
		    {
		    svbufptr = ++str;		/* skip '<' */

		    /* copy number */
		    pbufptr = padbuffer;
		    for ( ; *str && isdigit (*str) ; str++)
			{
			*svbufptr++ = *str;
			*pbufptr++ = *str;
			}
		    *pbufptr = '\0';
		    paddigits += atoi (padbuffer);
		    /* check for decimal */
		    if (*str == '.')
			{
			str++;
			pbufptr = padbuffer;
			for ( ; *str && isdigit (ch) ; str++)
			    {
			    *svbufptr++ = *str;
			    *pbufptr++ = *str;
			    }
			*pbufptr = '\0';
			paddecimal += atoi (padbuffer);
			}
		    for ( ; (*str == '*') || (*str == '/') ; str++)
			{
			if (*str == '*')
			    padbylines = 1;
			/* Termcap does not support mandatory padding */
			/* marked with /. Just remove it. */
			else
			    {
			    extern char *progname;
			    (void) fprintf (stderr,
				"%s: mandatory padding removed\n", progname);
			    }
			}
		    /* oops, not a padding spec after all */
		    /* put us back after the '$<' */
		    if (*str != '>')
			{
			str = svbufptr;
			*retptr++ = '$';
			*retptr++ = '<';
			}
		    else
			str++;		/* skip the '>' */
		    /* Flag padding info that is not at the end */
		    /* of the string. */
		    if (*str != '\0')
			{
			extern char *progname;
			(void) fprintf (stderr,
			    "%s: padding information moved to end\n", progname);
			}
		    }
		else
		    *retptr++ = ch;
		break;

	    default:
		*retptr++ = ch;
	    }
    *retptr = '\0';

    if (paddecimal > 10)
	{
	paddigits += paddecimal / 10;
	paddecimal %= 10;
	}

    if (paddigits > 0 && paddecimal > 0)
	(void) sprintf (padbuffer, "%d.%d", paddigits, paddecimal);
    else if (paddigits > 0)
	(void) sprintf (padbuffer, "%d", paddigits);
    else if (paddecimal > 0)
	(void) sprintf (padbuffer, ".%d", paddecimal);
    if (padbylines)
	(void) strcat (padbuffer, "*");
    if (padding)
	*padding = paddecimal;
    return rmbuffer;
}

/*
    Convert a character, making appropriate changes to make it printable
    for a termcap source entry. Change escape, tab, etc., into their
    appropriate equivalents. Return the number of characters printed.
*/
char *cconvert(string)
char *string;
{
    register int c;
    register char *retptr = retbuffer;

    retbuffer[0] = '\0';
    if (string == NULL)
	return retbuffer;

    while (c = *string++)
	{
	/* should check here to make sure that there is enough room */
	/* in retbuffer and realloc it if necessary. */
	c &= 0377;
	/* we ignore the return value from sprintf because BSD/V7 */
	/* systems return a (char *) rather than an int. */
	if (c >= 0200)
	    { (void) sprintf (retptr, "\\%.3o", c); retptr += 4; }
	else if (c == '\033')
	    { (void) sprintf (retptr, "\\E"); retptr += 2; }
	else if (c == '\t')
	    { (void) sprintf (retptr, "\\t"); retptr += 2; }
	else if (c == '\b')
	    { (void) sprintf (retptr, "\\b"); retptr += 2; }
	else if (c == '\f')
	    { (void) sprintf (retptr, "\\f"); retptr += 2; }
	else if (c == '\n')
	    { (void) sprintf (retptr, "\\n"); retptr += 2; }
	else if (c == '\r')
	    { (void) sprintf (retptr, "\\r"); retptr += 2; }

	/* unfortunately \: did not work */
	else if (c == ':')
	    { (void) sprintf (retptr, "\\072"); retptr += 4; }
	else if (c == '^')
	    { (void) sprintf (retptr, "\\^"); retptr += 2; }
	else if (c == BACKSLASH)
	    { (void) sprintf (retptr, BACKBACK); retptr += 2; }
	else if (c < ' ' || c == 0177)
	    { (void) sprintf (retptr, "^%c", c ^ 0100); retptr += 2; }
	else
	    { (void) sprintf (retptr, "%c", c); retptr++; }
	}
    *retptr = '\0';
    return retbuffer;
}

/*
    Convert the terminfo string into a termcap string.
    Most of the work is done by rmpadding() above and cconvert(); this
    function mainly just pieces things back together. A pointer to the
    return buffer is returned.

    NOTE: Some things can not be done at all: converting the terminfo
    parameterized strings into termcap parameterized strings.
*/

char *cexpand (str)
register char *str;
{
    char padbuffer[512];
    char *retptr;

    retptr = rmpadding (str, padbuffer, (int *) 0);
    (void) sprintf (ret2buffer, "%s%s", padbuffer, cconvert(retptr));

    return ret2buffer;
}

/*
    Print out a string onto a stream, changing unprintables into
    termcap printables.
*/
int cpr (stream, string)
register FILE *stream;
register char *string;
{
    register char *ret;
    if (string != NULL)
	{
	ret = cexpand (string);
	(void) fprintf (stream, "%s", ret);
	return strlen(ret);
	}
    else
	return 0;
}
