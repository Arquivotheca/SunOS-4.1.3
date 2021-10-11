#ifndef lint
static  char sccsid[] = "@(#)gttest.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc. *
 */

/**********************************************************************/
/* Include files */
/**********************************************************************/

#include <stdio.h>
#include <strings.h>
#include <signal.h>
#include <dirent.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <libonline.h>  /* online library include */
#include <gttest.h>
#include <errmsg.h>


/**********************************************************************/
/* External variables */
/**********************************************************************/

extern
char			*sprintf();

extern
struct test		subtests[];

extern
char			*routine_msg,
			*routine_msg1,
			*routine_msg2,
			*routine_msg3,
			*routine_msg4,
			*test_usage_msg;

/**********************************************************************/
/*  Global variables */
/**********************************************************************/

char			*program_name;
char			testdata_filename[MAXNAMLEN];
int			chksum_gen = 0;	/* flag for test images */
int			test_debug = 0;

/**********************************************************************/
/* Static variables */
/**********************************************************************/

static
char	usage_msg[] = "";


static
char			textbuf[512];

static
int			sub = 0x7ffff,
			sl = 1,
			bl = 1;

static
char			status[512];

/* pointer of current subtest */
static
struct test		*tl = (struct test *)NULL;

/**********************************************************************/
main (argc, argv)
/**********************************************************************/
int argc;
char **argv;

{
    extern int process_test_args();
    extern int routine_usage();
    int sigalrm_handler();
    int sigterm_handler();

    program_name = argv[0];
    test_name = argv[0];
    device_name = FRAME_BUFFER_DEVICE_NAME;

    func_name = "main";
    TRACE_IN

    /* Start with test_init to enter sundiag environment */
    test_init(argc, argv, process_test_args, routine_usage, test_usage_msg);
    /* catch signal 14 due to bug ID  1058927 */
    if (signal(SIGALRM, sigalrm_handler) == (void (*)())-1) {
	fb_send_message(SKIP_ERROR, WARNING, "signal() error.\n", testdata_filename);
    }

    /* Installing sigterm handler to catch device driver signal when FE dies */
    signal(SIGTERM, sigterm_handler);

    /* Set up path to data file */
    (void)strcpy(testdata_filename, GTTEST_DATA);
    if (!file_exist(testdata_filename)) {
	(void)strcpy(testdata_filename, "/usr/diag/sundiag/");
	(void)strcat(testdata_filename, GTTEST_DATA);
	if (!file_exist(testdata_filename)) {
	    fb_send_message(SKIP_ERROR, WARNING, GTTEST_DATA_MISSING, testdata_filename);
	}
    }

    /* Start test */
    (void)run_tests(sub, sl, bl);

    /* Leave sundiag */
    test_end();
}

/**********************************************************************/
pmessage(inftext)
/**********************************************************************/
char *inftext;

{
    func_name = "pmessage";
    TRACE_IN

    if (verbose) {
	fb_send_message(SKIP_ERROR, VERBOSE, inftext);
    }
    TRACE_OUT
}

/**********************************************************************/
error_report(errtxt)
/**********************************************************************/
char *errtxt;

{
    func_name = "error_report";
    TRACE_IN

    fb_send_message(SKIP_ERROR, ERROR, "Error in %s test", tl->testname);
    fb_send_message(HAWK_FATAL_ERROR, ERROR, errtxt);

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
    dump_status();
    fb_send_message(HAWK_FATAL_ERROR, FATAL, errtxt);

    TRACE_OUT
}

/**********************************************************************/
clean_up()
/**********************************************************************/
{
    static int clean = 0;

    func_name = "clean_up";
    TRACE_IN

    if (clean) { /* prevent recursive calling of cleap_up */
	(void)unlock_desktop();
	return;
    }

    clean = 1;
    (void)restore_fb();
    (void)unlock_desktop();

    if (exec_by_sundiag) {
	sleep(5);		/* allow user to hit stop button */
    }

    (void)lightpen_disable();

    (void)close_hawk();

    TRACE_OUT
}
/**********************************************************************/
process_test_args(argv, arrcount)
/**********************************************************************/
char    *argv[];
int     arrcount;
{
    char *cptr;
    func_name = "process_test_args";
    TRACE_IN

    if (strncmp(argv[arrcount], "D=", 2) == 0) {
	device_name = argv[arrcount]+2;
	TRACE_OUT
	return TRUE;
    } else if (strncmp(argv[arrcount], "S=", 2) == 0) {
	cptr = (char *)argv[arrcount]+2;
	if (*cptr++ == '0' && (*cptr == 'x' || *cptr == 'X')) {
	    sscanf(argv[arrcount]+4, "%X", &sub);
	} else {
	    sub = atoi(argv[arrcount]+2);
	}
	if (sub == -1) {
	    sub = 0x7ffff;
	}
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
    } else if (strncmp(argv[arrcount], "debug", 5) == 0) {
	test_debug = 1;
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

    fb_send_message(SKIP_ERROR, CONSOLE, routine_msg, test_name);
    fb_send_message(SKIP_ERROR, CONSOLE, routine_msg1);
    fb_send_message(SKIP_ERROR, CONSOLE, routine_msg2);
    fb_send_message(SKIP_ERROR, CONSOLE, routine_msg3);
    fb_send_message(SKIP_ERROR, CONSOLE, routine_msg4);

    TRACE_OUT
}

/**********************************************************************/
run_tests(tconfig, flc, blc)
/**********************************************************************/
int tconfig;
int flc;
int blc;

{
    int it;
    int sc;
    int bc;
    int lock;
    char *errmsg;

    func_name = "run_tests";
    TRACE_IN

    lock = lock_desktop(device_name);

    for(bc = blc ; bc ; bc--) {
	for (tl = subtests, it = 1; tl ; tl = tl->nexttest, it *=2) {
	    if (it & tconfig) {
		for(sc = flc ; sc ; sc--) {

		    (void)sprintf(textbuf, "%s: ", tl->testname);
		    (void)strcat(textbuf, "started.\n");
		    pmessage(textbuf);

		    if (tl->acc_port) {
			/* Initialize accelerator port */
			if (!open_hawk()) {
			    sprintf(errmsg, DLXERR_SYSTEM_INITIALIZATION);
	 		    error_report(errmsg);
			}
		    }


		    errmsg = (tl->proc)();

		    if (errmsg) {
			error_report(errmsg);
		    } else {
		        (void)sprintf(textbuf, "%s: ", tl->testname);
			(void)strcat(textbuf, "OK.\n");
			pmessage(textbuf);
		    }

		}
	    }
	}
    }

    if (lock >= 0) {
    	/* wait for window manager to redraw the screen */
	lock = unlock_desktop();

#ifndef OL
	sleep(5);
#endif !OL
     }

    /* close system */
    (void)close_hawk();

    (void)close_pr_device();
    TRACE_OUT
}

/**********************************************************************/
file_exist(filename)
/**********************************************************************/
char *filename;
{

    FILE *fd;

    fd = fopen(filename, "r");
    if (fd == NULL) {
	return 0;
    } else {
	fclose(fd);
	return 1;
    }
}

/**********************************************************************/
dump_status()
/**********************************************************************/
{
    extern long time();
    extern char *ctime();
    extern char func_name_buf[];
    extern int trace_level;
    extern int trace;

    int i;
    int trace_save;
    int trace_level_save;

    if (tl) {
	fb_send_message(SKIP_ERROR, CONSOLE, "Current Subtest: %s\n", tl->testname);
	fb_send_message(SKIP_ERROR, ERROR, "Current Subtest: %s\n", tl->testname);
    }

    if (trace_level > 0) {
	fb_send_message(SKIP_ERROR, CONSOLE, "Nested subroutines on trace stack (most recent call on top):\n");
	trace_save = trace;
	trace_level_save = trace_level;
	trace = 1;
    }
    while (trace_level > 0) {
	trace_after((char *)NULL);
    }
	trace = trace_save;
	trace_level = trace_level_save;

}

/**********************************************************************/
sigalrm_handler(sig, code, scp, addr)
/**********************************************************************/
int sig;
int code;
struct sigcontext *scp;
char *addr;

{
    /* Why am I getting signal 14 here ??? */
    sleep(5);

    /*
    fb_send_message(SKIP_ERROR, LOGFILE, "Signal %d caught (BugID 1058927):\n", sig);

    fb_send_message(SKIP_ERROR, LOGFILE, "Code = %d\n", code);

    fb_send_message(SKIP_ERROR, LOGFILE, "scp->sc_sp = 0x%X\n", scp->sc_sp);

    fb_send_message(SKIP_ERROR, LOGFILE, "scp->sc_pc = 0x%X\n", scp->sc_pc);

    fb_send_message(SKIP_ERROR, LOGFILE, "scp->sc_npc = 0x%X\n", scp->sc_npc);

    fb_send_message(SKIP_ERROR, LOGFILE, "scp->sc_psr = 0x%X\n", scp->sc_psr);

    fb_send_message(SKIP_ERROR, LOGFILE, "scp->sc_g1 = 0x%X\n", scp->sc_g1);

    fb_send_message(SKIP_ERROR, LOGFILE, "scp->sc_o0 = 0x%X\n", scp->sc_o0);

    fb_send_message(SKIP_ERROR, LOGFILE, "scp->sc_wbcnt = 0x%X\n", scp->sc_wbcnt);

    fb_send_message(SKIP_ERROR, LOGFILE, "Addr = 0x%X\n", (unsigned int) addr);

    dump_status();
    */
}


/**********************************************************************/
sigterm_handler(sig, code, scp, addr)
/**********************************************************************/
/* If we get SIGTERM it means that the gt driver has detected that the FE
   has died, so we simply log this error to log files - see bugid 1072183 */

int sig;
int code;
struct sigcontext *scp;
char *addr;
{
    fb_send_message(SKIP_ERROR, ERROR, "Error in %s test: received SIGTERM\n", tl->testname);
    fb_send_message(SKIP_ERROR, ERROR, DLXERR_FE_TIMEOUT);
    exit(HAWK_FATAL_ERROR);
}
