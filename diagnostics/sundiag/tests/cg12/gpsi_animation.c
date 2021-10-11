
#ifndef lint
static  char sccsid[] = "@(#)gpsi_animation.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <signal.h>
#include <sys/wait.h>
#include <esd.h>

static int pid = -1;		/* PID of the background process which does
			   the drawing */

/**********************************************************************/
char *
gpsi_animation()
/**********************************************************************/
 
{
    extern char *pr_arbi_test();

    int count;
    char *errmsg;

    func_name = "gpsi_animation";
    TRACE_IN

    /* extract test images from tar file */
/*********
    (void)xtract(GPSI_ANIMATION_CHK);
*********/

    /* clear all planes */
    clear_all();

    if ((pid = fork()) == -1) {
        (void)fb_send_message(SKIP_ERROR, WARNING, errmsg_list[13]);
	TRACE_OUT
	return(errmsg_list[9]);
    }

    if (pid == 0) {

	(void)do_animation();/* back ground process doing animation */

    } else {		/*while foreground process is testing pixrect*/

	sleep(3);
	(void)check_key();
	errmsg = pr_arbi_test();
	(void)check_key();
	sleep(3);

    }


    /* wait for the background program to complete */
    count = 1000;
    while ((wait4(pid, NULL, WNOHANG | WUNTRACED, NULL) == NULL) && count--) {
	sleep(1);
    }

    if (!count) { /* kill background task if it isn't already dead */
    	(void)kill(pid, SIGKILL);
	sleep(5);
    }

    (void)clear_24bit_plane();

    (void)rgb_stripes();


    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }


/**********************************************
    errmsg = chksum_verify(GPSI_ANIMATION_CHK);
    if (errmsg) {
	errmsg = errmsg_list[51];
    }
******* replaced with sleep (30) ****************/

    for (count = 30 ; count ; count--) {
	(void)check_key();
	sleep(1);
    }

    TRACE_OUT
    return (char *)0;
	
}


/**********************************************************************/
stop_draw()
/**********************************************************************/

{

    int count;

    if (pid == 0) {
	(void)kill(getpid(), SIGKILL);
    } else if (pid > 0) {

	(void)kill(pid, SIGKILL);
	/* wait for the tar program to complete */
	count = 1000000;
	while ((wait4(pid, NULL, WNOHANG | WUNTRACED, NULL) == NULL) &&
								    count--);

	if (count == 0) {
	    (void)fb_send_message(SKIP_ERROR, WARNING,
		    "Background process wouldn't die.\n");
	}
    }
}
