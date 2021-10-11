#ifndef lint
static	char sccsid[] = "@(#)msgs.c 1.1 92/07/30 SMI"; /* from UCB 1.4 83/07/06 */
#endif

/* 
 * a package to display what is happening every MSG_INTERVAL seconds
 * if we are slow connecting.
 */

#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include "talk.h"

#define MSG_INTERVAL 4
#define LONG_TIME 100000

char *current_state;
int current_line = 0;

static struct itimerval itimer;
static struct timeval wait = { MSG_INTERVAL , 0};
static struct timeval undo = { LONG_TIME, 0};
    

disp_msg()
{
    message(current_state);
}

start_msgs()
{
    message(current_state);
    signal(SIGALRM, disp_msg);
    itimer.it_value = itimer.it_interval = wait;
    setitimer(ITIMER_REAL, &itimer, (struct timerval *)0);
}

end_msgs()
{
    signal(SIGALRM, SIG_IGN);
    timerclear(&itimer.it_value);
    timerclear(&itimer.it_interval);
    setitimer(ITIMER_REAL, &itimer, (struct timerval *)0);
}
