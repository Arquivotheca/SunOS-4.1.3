
#ifndef lint
static  char sccsid[] = "@(#)cg12.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
#include <cg12.h>


/**********************************************************************/
/* External variables */
/**********************************************************************/

extern
char			*timestamp();

extern
char			*timestamp(),
			*sprintf(),
			*func_namer,
			*device_name,
			*test_name;

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
int			chksum_dump = 0;	/* flag for to dump
					screen */

/**********************************************************************/
/* Static variables */
/**********************************************************************/

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

    func_name = "main";
    TRACE_IN

    /* Start with test_init to enter sundiag environment */
    (void)test_init(argc, argv, process_test_args, routine_usage,
							test_usage_msg);

    (void)run_tests(sub, sl, bl);

    /* make cg12 device is closed normally */

    (void)close_gp();

    if (errcnt > 0) {
	TRACE_OUT
	exit(1);
    } else {
	TRACE_OUT
	(void)test_end();
    }
}

/**********************************************************************/
pmessage(inftext)
/**********************************************************************/
char *inftext;

{
    extern int verbose;

    func_name = "pmessage";
    TRACE_IN

    if (verbose) {
	(void)fb_send_message(SKIP_ERROR, VERBOSE, inftext);
    }
    TRACE_OUT
}

/**********************************************************************/
tmessage(text)
/**********************************************************************/
char *text;

{
    func_name = "tmessage";
    TRACE_IN

    pmessage(timestamp(text));

    TRACE_OUT
}

/**********************************************************************/
error_report(errtxt)
/**********************************************************************/
char *errtxt;

{
    func_name = "error_report";
    TRACE_IN

    (void)fb_send_message(SKIP_ERROR, ERROR, errtxt);

    errcnt++;
 
    if (errcnt > ERROR_LIMIT) {
	(void)fb_send_message(EGRET_FATAL_ERROR, FATAL, "Too many errors.");
    }
    TRACE_OUT
}

/**********************************************************************/
fatal_error_exit(errtxt)
/**********************************************************************/
char *errtxt;
{
    func_name = "fatal_error_exit";
    TRACE_IN

    (void)clean_up();
    (void)fb_send_message(EGRET_FATAL_ERROR, FATAL, errtxt);

    TRACE_OUT
}

/**********************************************************************/
clean_up()
/**********************************************************************/
{
    func_name = "clean_up";
    TRACE_IN

    (void)close_gp();
    (void)unlock_desktop();

    TRACE_OUT
}
/**********************************************************************/
process_test_args(argv, arrcount)
/**********************************************************************/
char    *argv[];
int     arrcount;
{
    func_name = "process_test_args";
    TRACE_IN

    if (strncmp(argv[arrcount], "D=", 2) == 0) {
	device_name = argv[arrcount]+2;
	TRACE_OUT
	return TRUE;
    } else if (strncmp(argv[arrcount], "S=", 2) == 0) {
	sub = atoi(argv[arrcount]+2);
	TRACE_OUT
	return TRUE;
    } else if (strncmp(argv[arrcount], "F=", 2) == 0) {
	sl = atoi(argv[arrcount]+2);
	TRACE_OUT
	return TRUE;
    } else if (strncmp(argv[arrcount], "B=", 2) == 0) {
	bl = atoi(argv[arrcount]+2);
	TRACE_OUT
	return TRUE;
    } else if (strncmp(argv[arrcount], "write", 5) == 0) {
	chksum_gen = 1;
	TRACE_OUT
	return TRUE;
    } else if (strncmp(argv[arrcount], "dump", 4) == 0) {
	chksum_dump = 1;
	TRACE_OUT
	return TRUE;
    } else {
	TRACE_OUT
	return FALSE;
    }

}

/**********************************************************************/
routine_usage()
/**********************************************************************/
{
    
    func_name = "routine_usage";
    TRACE_IN

    (void)send_message(SKIP_ERROR, CONSOLE, routine_msg, test_name);
    (void)send_message(SKIP_ERROR, CONSOLE, routine_msg1);

    TRACE_OUT
}

/**********************************************************************/
run_tests(tconfig, flc, blc)
/**********************************************************************/
int tconfig;
int flc;
int blc;

{
    extern int quick_test;
    struct test *tl;
    int it;
    int sc;
    int bc;
    char *errmsg;

    func_name = "run_tests";
    TRACE_IN

    (void)lock_desktop(device_name);

    if (quick_test) {
	tconfig = 1;
	flc = 1;
	blc = 1;
    }

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

    TRACE_OUT
}
