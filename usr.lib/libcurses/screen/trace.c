/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)trace.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.8 */
#endif

#include "curses_inc.h"

traceon()
{
#ifdef DEBUG
    if (outf == NULL)
    {
	outf = fopen("trace", "a");
	if (outf == NULL)
	{
	    perror("trace");
	    exit(-1);
	}
	fprintf(outf, "trace turned on\n");
    }
#endif /* DEBUG */
    return (OK);
}

traceoff()
{
#ifdef DEBUG
    if (outf != NULL)
    {
	fprintf(outf, "trace turned off\n");
	fclose(outf);
	outf = NULL;
    }
#endif /* DEBUG */
    return (OK);
}

#ifdef DEBUG
#include <ctype.h>

char *
_asciify(str)
register char *str;
{
    static	char	string[1024];
    register	char	*p1 = string;
    register	char	*p2;
    register	char	c;

    while (c = *str++)
    {
	p2 = unctrl(c);
	while (*p1 = *p2++)
	    p1++;
    }
    return string;
}
#endif /* DEBUG */
