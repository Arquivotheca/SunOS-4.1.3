/*	@(#)sdrtns.h	1.1 7/30/92 Copyright Sun Micro	*/

/******************************************************************************
 * sdrtns.h								      *
 *									      *
 * This file should be included by all tests to be run by Sundiag.	      *
 ******************************************************************************/

#ifndef sdrtns_h
#define sdrtns_h

#define throwup send_message

#ifndef TRUE
#define TRUE    1
#endif
 
#ifndef FALSE
#define FALSE   0
#endif

#define MESSAGE_SIZE		512	/* max. message size from tests */
#define MAXHOSTNAMELEN          64

/* reserved error exit codes */
#define SKIP_ERROR		0	/* minor error, keep going */
#define FREERUN_ERROR		93	/* error end with run_on_error */
#define RPCINIT_RETURN		94	/* failed in client create */
#define NONROOT_ERROR		95      /* not in superuser mode */
#define USAGE_ERROR             96	/* command usage error */
#define RPCFAILED_RETURN        97      /* failed in sending message by RPC */
#define INTERRUPT_RETURN        98      /* exited due to interrupt */
#define EXCEPTION_RETURN        99      /* exited due to caught exception */

/***** message types *****/
#define INFO                    0	/* to info. log only */
#define WARNING                 1	/* to info. log only */
#define FATAL                   2	/* to info. and error logs */
#define ERROR                   3	/* to info. and error logs */
#define LOGFILE                 4	/* to info. log (unformatted) */
#define CONSOLE			5	/* to sundiag console(unformatted) */
#define VERBOSE			6	/* to sundiag console(unformatted) */
#ifdef  DEBUG				/* to pacify <sys/debug.h> */
#undef  DEBUG
#endif  DEBUG
#define DEBUG			7	/* to sundiag console(unformatted) */
#define TRACE			8       /* to sundiag console(unformatted) */

#ifdef  NO_CLEAN_UP
int clean_up() {}
#endif 

#define TRACE_IN	(void)trace_before((char *)NULL);
#define TRACE_OUT	(void)trace_after((char *)NULL);

/***** external variables to be used by sundiag tests *****/
extern  int     single_pass;            /* TRUE, if single pass enabled */
extern  int     core_file;              /* TRUE, if core file to be created */
extern  int     verbose;                /* TRUE, if verbose mode enabled */
extern  int     quick_test;             /* TRUE, if quick test enabled */
extern  int     run_on_error;           /* TRUE, if run_on_error enabled */
extern  int     exec_by_sundiag;        /* TRUE, if executed by Sundiag */
extern  int     list_usage;
extern  int     debug;
extern  int	trace;			/* TRUE, if trace mode is enabled */
extern  int	trace_level;		/* trace level relative to main */
extern  int	test_error;		/* test error count */
extern  char	*func_name;		/* name of the function being run */
extern  char    *device_name;           /* name of UNIX device being tested */
extern  char    *test_name;             /* name of the test being run */
extern  char    *hostname;

/***** external functions to be called by sundiag tests *****/
/******************************************************************************
 * process_sundiag_args(), searchs for the sundiag-specific command line      *
 * arguments and set the corresponding variables.			      *
 * Input: argc and argv, command line arguments.			      *
 *	  index, pointer to the index of the argument to be checked.	      *
 * Output: TRUE, if matched; otherwise FALSE.				      *
 ******************************************************************************/
extern	int	process_sundiag_args();

/******************************************************************************
 * send_message(), the message handler for sundiag tests.		      *
 * Input: exit_code, the exit code to be used when exiting(if != 0).	      *
 *	  type, the mesage type, could be one of INFO, WARNING, FATAL,    *
 *		    ERROR, or LOGFILE.					      *
 *	  msg, the unformatted message itself.				      *
 * Outout: none.							      *
 * Note: the entire process will exit from here if ret_code != 0.	      *
 ******************************************************************************/
extern	void 	send_message();

/******************************************************************************
 * exception(), catches the signals, which otherwise may cause a core dump.   *
 * Note: used by executing signal(SIGNAME, exception) if core_file == FALSE.  *
 ******************************************************************************/
extern	void	exception();

/******************************************************************************
 * finish(), the signal handler for SIGHUP, SIGTERM, and SIGINT.	      *
 * Note: This function should be the signal handler for SIGHUP, SIGTERM, and  *
 *       SIGINT for all tests to be executed by Sundiag. clean_up() needs     *
 *	 to be coded and declared external in the test code to do any         *
 *	 necessary clean ups before exiting.				      *
 ******************************************************************************/
extern	void	finish();
extern  void    display_usage();
extern  void    standard_usage();
/***************************************************************************
 test_init(), paired with test_end() to construct Sundiag main code. This
 simplifies the sundiag test code a lot and relieve most of sundiag chore
 off the programmer.
        argc, argv: unix standard input arguments.
        process_test_args: pointer to function that process test-specific
                           command line arguments.
        routine_usage: pointer to function that explain test-specific
                       command arguments.
        test_usage_msg: pointer to string that contains the general usage
                        of the test command.
***************************************************************************/
extern  void    test_init();
/**************************************************************************
 test_end(), Sundiag test code enclosure routine.  Always use this routine
 to leave Sundiag.  This routine will be expanded to do some work more
 general to Sundiag for visual or statistical purpose.
***************************************************************************/
extern  void	test_end();
extern  void    trace_before();
extern  void    trace_after();
#endif sdrtns_h
