
#ifndef lint
static  char sccsid[] = "@(#)esda.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/**********************************************************************/
/* Include files */
/**********************************************************************/

#include <stdio.h>
#include <strings.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <libonline.h>  /* online library include */
#include <esd.h>
#include <esda.h>


/**********************************************************************/
/* External variables */
/**********************************************************************/

extern
char			*timestamp();

extern
char			*timestamp(),
			*sprintf();

extern
struct test		sb_testlist[];

extern
char			*routine_msg,
			*routine_msg1,
			*test_usage_msg;

/**********************************************************************/
/*  Global variables */
/**********************************************************************/

char			*program_name;
int			chksum_gen = 0;	/* flag for test images
					generation or verification */

/**********************************************************************/
/* Static variables */
/**********************************************************************/

static
char	usage_msg[] = "";


static
int			errcnt;

static
char			textbuf[512];

static
int			sub = -1,
			sl = 1,
			bl = 1;

/**********************************************************************/
main (argc, argv)
/**********************************************************************/
int argc;
char **argv;

{
    extern int process_test_args();
    extern int routine_usage();

    program_name = argv[0];
    test_name = argv[0];
    device_name = FRAME_BUFFER_DEVICE_NAME;

    /* Start with test_init to enter sundiag environment */
    test_init(argc, argv, process_test_args, routine_usage, test_usage_msg);

    (void)run_tests(sub, sl, bl);

    test_end();

    if (errcnt > 0) {
	exit(1);
    } else {
	exit(0);
    }
}

/**********************************************************************/
pmessage(inftext)
/**********************************************************************/
char *inftext;

{
    if (verbose) {
	send_message(SKIP_ERROR, INFO, inftext);
    }
}

/**********************************************************************/
tmessage(text)
/**********************************************************************/
char *text;

{
	pmessage(timestamp(text));
}

/**********************************************************************/
error_report(errtxt)
/**********************************************************************/
char *errtxt;

{
    send_message(SKIP_ERROR, ERROR, errtxt);

    errcnt++;
 
    if (errcnt > ERROR_LIMIT) {
	send_message(EGRET_FATAL_ERROR, FATAL, "Too many errors.");
    }
}

/**********************************************************************/
fatal_error_exit(errtxt)
/**********************************************************************/
char *errtxt;
{
    (void)clean_up();
    send_message(EGRET_FATAL_ERROR, FATAL, errtxt);
}

/**********************************************************************/
clean_up()
/**********************************************************************/
{
    (void)unlock_desktop();
}
/**********************************************************************/
process_test_args(argv, arrcount)
/**********************************************************************/
char    *argv[];
int     arrcount;
{
    if (strncmp(argv[arrcount], "dev=", 4) == 0) {
	device_name = argv[arrcount]+4;
	return TRUE;
    } else if (strncmp(argv[arrcount], "sub=", 4) == 0) {
	sub = atoi(argv[arrcount]+4);
	return TRUE;
    } else if (strncmp(argv[arrcount], "sl=", 3) == 0) {
	sl = atoi(argv[arrcount]+3);
	return TRUE;
    } else if (strncmp(argv[arrcount], "bl=", 3) == 0) {
	bl = atoi(argv[arrcount]+3);
	return TRUE;
    } else if (strncmp(argv[arrcount], "write", 5) == 0) {
	chksum_gen = 1;
	return TRUE;
    } else {
	return FALSE;
    }

}

/**********************************************************************/
routine_usage()
/**********************************************************************/
{
    
    send_message(SKIP_ERROR, CONSOLE, routine_msg, test_name);
    send_message(SKIP_ERROR, CONSOLE, routine_msg1);
}

/**********************************************************************/
run_tests(tconfig, flc, blc)
/**********************************************************************/
int tconfig;
int flc;
int blc;

{
    struct test *tl;
    int it;
    int sc;
    int bc;
    char *errmsg;

    (void)lock_desktop(device_name);

    for(bc = blc ; bc ; bc--) {
	for (tl = sb_testlist, it = 1; tl ; tl = tl->nexttest, it *=2) {
	    if (it & tconfig) {
		for(sc = flc ; sc ; sc--) {

		    errmsg = (tl->proc)();

		    (void)sprintf(textbuf, "%s: ", tl->testname);
		    (void)strcpy(textbuf, textbuf);
		    if (errmsg) {
			(void)strcat(textbuf, "*** ");
			(void)strcat(textbuf, errmsg);
			error_report(textbuf);
		    } else {
			(void)strcat(textbuf, "ok.\n");
			pmessage(textbuf);
		    }
		}
	    }
	}
    }

    (void)unlock_desktop();
}
