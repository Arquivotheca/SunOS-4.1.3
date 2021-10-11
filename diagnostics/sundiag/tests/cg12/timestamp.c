
#ifndef lint
static  char sccsid[] = "@(#)timestamp.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */

/**********************************************************************/
char *timestamp(text)
/**********************************************************************/
char *text;

{
    long timeofday, time();
    char *ctime();
    static char textbuf[512];

    func_name = "timestamp";
    TRACE_IN

    timeofday = time((time_t *)0);
    (void)strncpy(textbuf, ctime((long *)&timeofday), 24);
    textbuf[24] = '\0';
    (void)strcat(textbuf, ": ");
    (void)strcat(textbuf, text);


    TRACE_OUT
    return(textbuf);
}
