/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)infotocap.c 1.1 92/07/30 SMI"; /* from S5R3 1.5 */
#endif

/*
    Routines to convert terminfo string parameters to termcap form.
    Not all terminfo strings will be, or could be, converted.

    The following parameter forms will at least be handled.

	%p1%d			-> %d
	%p1%2.2d		-> %2
	%p1%3.3d		-> %3
	%p1%02d			-> %2
	%p1%03d			-> %3
	%p1%c			-> %.
	%p1%'x'%+%c		-> %+x
	%i			-> %i
	%p2 before %p1		-> %r
	%p1%'x'%>%+%p1%'y'%+%;	-> %>xy
*/

#include "curses.h"

/* externs from libc */
extern char *strcpy();
#if !defined(SYSV) && !defined(USG) && !defined(strchr) /* handle both Sys Vr2 and Vr3 curses */
# define strchr index
#endif /* SYSV || USG */
extern char *strchr();

#define NULLPTR	((char *) 0)

/*
    lookat looks at a string such as "%p1%d" and a pattern such as "%p*%d",
    where '*' is the only wild character. Each place that the star matches,
    the corresponding character in the string is placed in args. If the
    pattern matches the string, 1 is returned.
*/

static int lookat(string, pattern, args)
register char *string, *pattern, *args;
{
    register val, pat;

    while ((pat = *pattern++) && (val = *string++))
	if (pat == '*')
	    *args++ = val;
	else if (val != pat)
	    return 0;
    if (pat == '\0')
	return 1;
    else
	return 0;
}

static int currentparm, otherparm, reversedparms;
static char *newvalue;
static char _newvalue[1024] = "!!! MUST CHANGE BY HAND !!!";
#define BYHANDMSGLEN 27

/*
    Setparms() and checkparms() are used by infotocap() to make
    sure that the parameters in the terminfo entry alternate and are
    only '1' or '2'. If the order has been reversed, then %r is added
    in to the new value being built.
 */

static setparms()
{
    currentparm = 1;
    otherparm = 2;
    reversedparms = 0;
    newvalue = &_newvalue[BYHANDMSGLEN];
}

static checkparms(arg)
register int arg;
{
    arg -= '0';
    if (arg != 1 && arg != 2)
	return 1;
    else if (arg != currentparm)
	if (reversedparms)
	    return 1;
	else if (!reversedparms && arg == otherparm)
	    {
	    (void) strcpy(newvalue, "%r");
	    newvalue += 2;
	    reversedparms = TRUE;
	    }
	else
	    return 1;
    else
	{
	otherparm = currentparm;
	currentparm = 3 - currentparm;
	}
    return 0;
}

/*
    Infotocap looks at the string capability to see if it has any
    stack manipulation operators. If possible, they are converted to
    termcap form. If any operator is found that cannot be modified,
    prepend a message to the beginning of the original value and
    set err to 1. A pointer to the new copy of the string is returned
    in either case. 
*/

char *infotocap(value, err)
register char *value;
int *err;
{
    char args[4];
    char *savevalue;

    *err = 0;
    if (strchr(value, '%') == NULLPTR)
	return value;

    setparms();

    savevalue = value;
    while (*value)
	if (*value != '%')
	    *newvalue++ = *value++;
	else if (lookat(value, "%p*%d", args))
	    {
	    if (checkparms(args[0]))
		goto dobyhand;
	    (void) strcpy(newvalue, "%d");
	    newvalue += 2;
	    value += 5;
	    }
	else if (lookat(value, "%p*%02d", args))
	    {
	    if (checkparms(args[0]))
		goto dobyhand;
	    (void) strcpy(newvalue, "%2");
	    newvalue += 2;
	    value += 7;
	    }
	else if (lookat(value, "%p*%03d", args))
	    {
	    if (checkparms(args[0]))
		goto dobyhand;
	    (void) strcpy(newvalue, "%3");
	    newvalue += 2;
	    value += 7;
	    }
	else if (lookat(value, "%p*%2.2d", args))
	    {
	    if (checkparms(args[0]))
		goto dobyhand;
	    (void) strcpy(newvalue, "%2");
	    newvalue += 2;
	    value += 8;
	    }
	else if (lookat(value, "%p*%3.3d", args))
	    {
	    if (checkparms(args[0]))
		goto dobyhand;
	    (void) strcpy(newvalue, "%3");
	    newvalue += 2;
	    value += 8;
	    }
	else if (lookat(value, "%p*%c", args))
	    {
	    if (checkparms(args[0]))
		goto dobyhand;
	    (void) strcpy(newvalue, "%.");
	    newvalue += 2;
	    value += 5;
	    }
	else if (lookat(value, "%p*%'*'%+%c", args))
	    {
	    if (checkparms(args[0]))
		goto dobyhand;
	    (void) sprintf(newvalue, "%%+%c", args[1]);
	    newvalue += 3;
	    value += 11;
	    }
	else if (lookat(value, "%i", args))
	    {
	    (void) strcpy(newvalue, "%i");
	    newvalue += 2;
	    value += 2;
	    }
	else if (lookat(value, "%%", args))
	    {
	    (void) strcpy(newvalue, "%%");
	    newvalue += 2;
	    value += 2;
	    }
	else if (lookat(value, "p*%'*'%>%+%p*%'*'%+%;", args))
	    {
	    if (args[0] != args[2])
		goto dobyhand;
	    if (checkparms(args[0]))
		goto dobyhand;
	    (void) sprintf(newvalue, "%%>%c%c", args[1], args[3]);
	    newvalue += 2;
	    value += 21;
	    }
	else
	    goto dobyhand;

    *newvalue = '\0';
    return &_newvalue[BYHANDMSGLEN];

dobyhand:
    (void) strcpy(&_newvalue[BYHANDMSGLEN], savevalue);
    *err = 1;
    return _newvalue;
}
