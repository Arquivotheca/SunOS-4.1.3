#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)panel_dclick.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 *  Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* panel_dclick.c:  Double (mouse) click routines  */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <sys/time.h>


#define ONE_SEC         1000000
#define THREE_QUARTER_SEC 750000
#define TWO_THIRD_SEC   666666
#define HALF_SEC        500000
#define ONE_THIRD_SEC   333333
#define ONE_QUARTER_SEC 250000
#define ONE_EIGHTH_SEC 	125000
#define ONE_SIXTEENTH_SEC 62500


/* 
 * Declare local routine
 */

static void panel_timer_diff();


/* 
 * Static data.
 */

static struct timeval click_prev = { 0, 0 };
static struct timeval click_interval = { 0, ONE_THIRD_SEC };



/*  panel_dclick_is: Recognizes double clicks. */
int
panel_dclick_is(click)
    struct timeval      *click;
{
    struct timeval      diff;
    int                 ret;


    panel_timer_diff(click, &click_prev, &diff);
    ret = timercmp(&diff, &click_interval, <);
    click_prev.tv_sec = click->tv_sec;
    click_prev.tv_usec = click->tv_usec;
    return(ret);
}



struct timeval
*panel_dclick_get_interval()
{
    return(&click_interval);
}



void
panel_dclick_set_interval(new_interval)
    struct timeval      *new_interval;
{
    click_interval.tv_sec = new_interval->tv_sec;
    click_interval.tv_usec = new_interval->tv_usec;
    return;
}



static void
panel_timer_diff(t2, t1, diff)
    struct timeval      *t2, *t1, *diff;
{
    struct timeval      *temp;


    if (timercmp(t2, t1, <)) {
        temp = t1; t1 = t2; t2 = temp;
    }
    diff->tv_sec = t2->tv_sec - t1->tv_sec;
    diff->tv_usec = t2->tv_usec - t1->tv_usec;
    if (diff->tv_usec < 0) {
        diff->tv_sec--;
        diff->tv_usec += ONE_SEC;
    }
    return;
}

