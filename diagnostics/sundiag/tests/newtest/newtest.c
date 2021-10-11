#ifndef lint
static	char sccsid[] = "@(#)newtest.c 1.1 92/07/30 Copyright Sun Micro";
#endif

#include <stdio.h>
#include "newtest_msg.h"
#include "sdrtns.h"	/* sdrtns.h should always be included */
#include "../../../lib/include/libonline.h"    /* online library include */

#define	DEVICE_NAME	"newdevice"
#define	TOTAL_PASS	7     /* number of test loops */
#define ERROR_LIMIT	5     /* max number of errors allowed if run_on_error */

/* error return code definitions(can be put in a .h file) */
#define	NEWTEST_ERROR	1
#define	TOO_MANY_ERRORS	2

static char  *test_usage_msg = "[test_specific_cmd1] [test_specific_cmd2]";
static int   flag1, flag2;
/*
 * This is a test template for sundiag, and is intended to show how you
 * should write a test to be run by sundiag.
 *
 * First, get the command-line arguments, then validate and/or assign the 
 * device name, probe for the device, and then run the test(s). The verify 
 * mode is not currently used by sundiag, but may be useful for running the 
 * test by itself under Unix. Note that the average test should run from 
 * three to five minutes, and the TOTAL_PASS symbolic constant can be adjusted
 * to generate proper testing period.
 *
 * Note: When run by sundiag, stdout and stderr will be redirected to sundiag's
 *	 console window. stdin is disabled. 
 */

main(argc, argv)
int	argc;
char	*argv[];
{
extern int process_test_args();
extern int routine_usage();

    versionid = "1.1";

    /* device_name can be fixed or passed in from command line */
    device_name = DEVICE_NAME;          /* fixed in this case */

    /* Always start with test_init to enter sundiag environment */
    test_init(argc, argv, process_test_args, routine_usage, test_usage_msg); 

    if (!exec_by_sundiag)
    {
      valid_dev_name();
      probe_newdev();
    }

    run_tests(single_pass ? 1 : TOTAL_PASS);
    /* pass the desired loop count and do real testing from here */

    clean_up();
    /* Always end with test_end to close up sundiag environment */
    test_end();				
}

/**
 * Process_test_args() processes test-specific command line aguments.
 */
process_test_args(argv, argindex)
char *argv[];
int  argindex;
{
   if (strcmp(argv[argindex], "test_specific_command_1") == 0)
	flag1 = TRUE;
   else if (strcmp(argv[argindex], "test_specific_command_2") == 0)
	flag2 = TRUE;
   else
	return FALSE;

   return TRUE;
}

/*
 * routine_usage() explain the meaning of each test-specific command argument.
 */
routine_usage()
{
  send_message(0, CONSOLE, "%s specific arguments [defaults]:\n\
	test_specific_cmd1 = meaning of command 1\n\
	test_specific_cmd2 = meaning of command 2\n", test_name);
}


/*
 * You may also want to consider validating the device name, if your test is
 * also to be run standalone (i.e., without sundiag) under Unix.
 */
valid_dev_name()
{
   func_name = "valid_dev_name";
   TRACE_IN
   TRACE_OUT
   return (0);
}

/*
 * The probing function should check that the specified device is available
 * to be tested(optional if run by Sundiag). Usually, this involves opening
 * the device file, and using an appropriate ioctl to check the status of the
 * device, and then closing the device file. There are several flavors of
 * ioctls: see dkio(4s), fbio(4s), if(4n), mtio(4), and so on. It is nice
 * to put the probe code into a separate code module, because it usually has
 * most of the code which needs to be changed for a new SunOS release or port.
 */
probe_newdev()
{
   func_name = "probe_newdev";
   TRACE_IN
   TRACE_OUT
   return(0);
}

/*
 * Run the test while pass < total_pass.
 *
 * Re: send_message , there are 3 variables for 
 * send_message(exit_code, type, msg).
 * "exit_code" indicates the type of error(such as TOO_MANY_ERRORS). A non-zero
 * exit_code will force send_message to terminate the test and use it 
 * as exit code.
 * Ret_code 97-99 are reserved by sundiag for RPCFAILED_RETURN(97),
 * INTERRUPT_RETURN(98), and EXCEPTION_RETURN(99).
 * For "type", use INFO, WARNING, FATAL, ERROR, LOGFILE, or CONSOLE.
 * For "msg", use a pointer (char *) to a message buffer.
 *
 * send_message() always write the "msg" to stderr, and if run by sundiag, 
 * it will also log "msg" to INFO log and ERROR log
 * (for msg_type FATAL and ERROR).
 *
 * The "msg" is formatted as follows before written:
 *   (void)sprintf(msg_buf, "%02d/%02d/%02d %02d:%02d:%02d %s %s %s: %s\n",
 *	(tp->tm_mon + 1), tp->tm_mday, tp->tm_year,
 *	tp->tm_hour, tp->tm_min, tp->tm_sec,
 *	device_name, test_name,
 *	msg_type, msg);
 *
 * For more details, read sdrtns.c and sdrtns.h.
 */
run_tests(total_pass)
int total_pass;
{
  int	pass=0;
  int	errors=0;

    func_name = "run_tests";
    TRACE_IN
    while (++pass <= total_pass)
    {
	send_message(0, VERBOSE, "Pass= %d, Error= %d",
		pass, errors);

	if (!newdev_test())
	    if (!run_on_error)
	        send_message(NEWTEST_ERROR, ERROR, failed_msg);
	    else if (++errors >= ERROR_LIMIT)
		send_message(TOO_MANY_ERRORS, FATAL, err_limit_msg);
    }
    TRACE_OUT
}

/*
 * The actual test.
 * Return True if the test passed, otherwise FALSE.
 *
 * Note: the "quick_test" flag is used to force an error here.
 *	 its normal use is to force a "quick(short)" version of the test.
 */
int	newdev_test()
{
  func_name = "newdev_test";
  TRACE_IN
  TRACE_OUT
  return(quick_test?FALSE:TRUE);
}

/*
 * clean_up(), contains necessary code to clean up resources before exiting.
 * Note: this function is always required in order to link with sdrtns.c
 * successfully.
 */
clean_up()
{
  func_name = "clean_up";
  TRACE_IN
  TRACE_OUT
  return(0);
}
