#ifndef lint
static  char sccsid[] = "@(#)sdrtns.c	1.1  7/30/92 Copyright Sun Micro"; 
#endif

/******************************************************************************
 * sdrtns.c								      *
 *									      *
 * Purpose: Sundiag interface routines to be used by all the Sundiag test
 *          codes.
 * Routines:
 *	    int  process_sundiag_args(argc, argv, index)
 *	    void send_message(ret_code, msg_type, format[, arg]...)
 *	    void exception(sig, code, scp, addr)
 *	    void finish()
 *	    void display_usage(rtn_usage)
 *	    void standard_usage()
 *          void test_init(argc, argv, process_test_args, routine_usage, 
                        test_usage_msg)
 *          void test_end()
 *	    void check_usage(routine_usage)
 *	    void check_coredump()
 ******************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#ifdef	 SVR4
#include <sys/siginfo.h>
#endif	 SVR4
#include <varargs.h>
#include <ctype.h>
#include "sdrtns.h"
#include "testnames.h"
#include "../../lib/include/libonline.h"

/* external variables going to be used by sundiag */
int	single_pass = FALSE;
int	core_file = FALSE;
int	verbose = FALSE;
int	quick_test = FALSE;
int	run_on_error = FALSE;
int	exec_by_sundiag = FALSE;
int     list_usage = FALSE;
int     debug = FALSE;
int	trace = FALSE;
int	trace_level = 0;
int	test_error = 0;
char	*func_name = "unknown_function";
char    *hostname=NULL;
char	*device_name="unknown_device";
char	*test_name="unknown_test";

/* external functions  for this routine sdrtns.c */
#ifndef	SVR4
extern	char	*sprintf();
#endif	SVR4

/* static variables only used in routine sdrtns.c */
static  char  msg_buf[MESSAGE_SIZE];
static  char    *argstr;
static  va_list  arg_ptr;
static  int      argint;
static  double   argdouble;

/******************************************************************************
 * process_sundiag_args(), searchs for the sundiag-specific command line      *
 * arguments and set the corresponding variables.			      *
 * Input: argc and argv, command line arguments.			      *
 *	  index, pointer to the index of the argument to be checked.	      *
 * Output: TRUE, if matched; otherwise FALSE.				      *
 ******************************************************************************/
int process_sundiag_args(argc, argv, index)
int	argc;
char	*argv[];
int	*index;		/* pointer to the index of the argument to be checked */
{
    char  *arg;

    arg = argv[*index];

    if (strcmp(arg, "s") == 0)		/* is currently executed by SUndiag */
	exec_by_sundiag = TRUE;
    else if (strcmp(arg, "v") == 0)	/* should be run in verbose mode */
	verbose = TRUE;
    else if (strcmp(arg, "q") == 0)	/* should be run in quick(short) mode */
	quick_test = TRUE;
    else if (strcmp(arg, "r") == 0)	/* should be run_on_error */
	run_on_error = TRUE;
    else if (strcmp(arg, "p") == 0)	/* run only one internal pass */
	single_pass = TRUE;
    else if (strcmp(arg, "c") == 0)	/* catch signal to prevent core dump */
	core_file = TRUE;
    else if (strcmp(arg, "u") == 0)	/* list commamnd usage */
	list_usage = TRUE;
    else if (strcmp(arg, "d") == 0)    /* debug mode set up */
	debug = TRUE;
    else if (strcmp(arg, "t") == 0)     /* trace mode */
	trace = TRUE;
    else if (strcmp(arg, "h") == 0)	/* send messages to specified host */
    {
      if (*index+1 < argc)
        hostname = argv[++(*index)];
	/* get the hostname and increment the index */
      else
   	return(FALSE);			/* no hostname right after h */
    }
    else
	return(FALSE);			/* none matched */

    return(TRUE);			/* at least one of the above matched */
}

/******************************************************************************
 * send_message(exit_code, type, format[, arg]...), the message handler       *
 *              for sundiag tests. It also checks verbose/debug flag to 
		eliminate the work in test codes.
 * Input:
 *         int exit_code: the exit code to be used when exiting(if != 0).     *
 *	   int type:      the mesage type, could be one of INFO, WARNING, 
			  FATAL, ERROR, CONSOLE, LOGFILE, VERBOSE, DEBUG,
			  TRACE, and VERBOSE+DEBUG or DEBUG+VERBOSE ( it
			  means VERBOSE or DEBUG). 
			  Note: any undefined type will be treated as VERBOSE 
			  if d or v are set at command line, otherwise no 
			  message will be sent.
 *	   char *format:  same as format of printf;  
 * Output: none.							      *
 * Note: the entire process will exit from here if ret_code != 0.	      *
 ******************************************************************************/
void send_message (exit_code, type, va_alist)
int exit_code;
int type;
va_dcl
{
    char    *control;
    char    *msg;
    extern char *control_parse();
    int	    send_flag = FALSE;
    static int	cleanup_flag = FALSE;
 
    va_start(arg_ptr);
    control = va_arg(arg_ptr, char *);
    msg = control_parse (control);
    va_end(arg_ptr);
 
    error_base = 0;
    switch (type) {	/* verbose has formatted message, debug not */
	case INFO:
		error_base = 7000;		/* for sundiag mode only */
		send_flag = TRUE;
		break;
	case WARNING:
		error_base = 5000;		/* for sundiag mode only */
		send_flag = TRUE;
		break;
	case FATAL:
		error_base = 3000;		/* for sundiag mode only */
		send_flag = TRUE;
		break;
	case ERROR:
		error_base = 9000;		/* for sundiag mode only */
		send_flag = TRUE;
		test_error++;			/* increment error count */
		break;
	case LOGFILE:
	case CONSOLE:
		send_flag = TRUE;
		break;
        case VERBOSE:           /* verbose has formatted message, debug not */
		send_flag = (verbose)? TRUE : FALSE;
                break;
        case DEBUG:
		send_flag = (debug)? TRUE : FALSE;
                break;    
        case TRACE:
		send_flag = (trace)? TRUE : FALSE;
                break;
	case VERBOSE+DEBUG:
	default:
		if (verbose || debug) {
			send_flag = TRUE;
			type = VERBOSE;
		}
    } /* end of switch; send it over to the server by RPC */

    if (send_flag) send_rpc_msg(exit_code, type, msg);

    if (exit_code != 0) {    			/* exit from here */ 
        if (!cleanup_flag) {
	   cleanup_flag = TRUE;                /* only one is allowed */
           clean_up();                         /* clean up the mess first */
 	}
        exit(exit_code);
    }
}

static char *
control_parse(control_str)
char *control_str;
{
   char *msgptr;
   static char msg[MESSAGE_SIZE];
   extern char *strip_sym();
   extern char *strcat();

   msgptr = &msg[0];      
   while (*control_str != NULL) {
        if (*control_str == '%') {
            *msgptr = NULL;
	    if (*(control_str+1) == '%') {
		control_str++; 
		control_str++; 
		strcat (msg, "%");
	    }
	    else
                strcat (msg, strip_sym(&control_str));
            msgptr = msg + strlen(msg);
        }   /* end of if(*control_str) */
        else
           *msgptr++ = *control_str++;
   } /* end of while */
   *msgptr = NULL;
   return (msg);
}

static char *
strip_sym(strptr)
char **strptr;
{
     char  pattern[10], *pattern_ptr;
     static char  buf[50];
     char  *string;

     string = *strptr;
     pattern_ptr = &pattern[0];

     while (!isalpha(*string)) {
           *pattern_ptr++ = *string++;
           ++(*strptr);
     }

     ++(*strptr);
     *pattern_ptr++ = *string;
     *pattern_ptr = NULL;               /* end the string */

     switch (*string) {                                      
           case 'd':
           case 'i':
           case 'o':
           case 'u':
           case 'x':
           case 'c':
                      argint = va_arg(arg_ptr, int);
                      sprintf(buf, pattern, argint);
                      break;
           case 'f':
           case 'e':
           case 'E':
           case 'G':
           case 'g':
                      argdouble = va_arg(arg_ptr, double);
                      sprintf(buf, pattern, argdouble);
                      break;
           case 's':
                      argstr = va_arg(arg_ptr, char *);
                      sprintf(buf, pattern, argstr);
                      break;
           default:
                      argint = va_arg(arg_ptr, int);
                      sprintf(buf, pattern, argint);

     } /* end of switch */
     return (buf);
}
 
/******************************************************************************
 * exception(), catches the signals, which otherwise may cause a core dump.   *
 * Note: used by executing signal(SIGNAME, exception) if core_file == FALSE.  *
 ******************************************************************************/
/*ARGSUSED*/
#ifdef	SVR4
void	exception(sig)
int	sig ;
#else
void	exception(sig, code, scp, addr)
int	sig;				/* the signal number */
int	code;
struct	sigcontext	*scp;
char	*addr;
#endif	SVR4
{
  char	*core_msg;

  switch (sig)
  {
    case SIGILL:
	core_msg = "illegal instruction";
	break;
    case SIGTRAP:
	core_msg = "trace trap";
	break;
    case SIGEMT:
	core_msg = "emulator trap";
	break;
    case SIGFPE:
	core_msg = "arithmetic exception";
	break;
    case SIGBUS:
	core_msg = "bus error";
	break;
    case SIGSEGV:
	core_msg = "segmentation violation";
	break;
    case SIGSYS:
	core_msg = "bad argument to system call";
	break;
    default:
	core_msg = "unknown exception signal";
	break;
  }

# ifdef SVR4
  (void)sprintf(msg_buf,"%s(%d)", core_msg,sig);
# else
  (void)sprintf(msg_buf,"%s(%d), code= %d, Address= 0x%lx", core_msg,
			sig, code, (unsigned long)addr);
# endif SVR4
  send_message(EXCEPTION_RETURN, FATAL, msg_buf);
}

/******************************************************************************
 * finish(), the signal handler for SIGHUP, SIGTERM, and SIGINT.	      *
 * Note: This function should be the signal handler for SIGHUP, SIGTERM, and  *
 *       SIGINT for all tests to be executed by Sundiag. clean_up() needs     *
 *	 to be coded and declared external in the test code to do any         *
 *	 necessary clean ups before exiting.				      *
 ******************************************************************************/
void	finish()
{
    clean_up();				/* clean up the mess before exit */
    exit(INTERRUPT_RETURN);		/* INTERRUPT_RETURN is not an error */
}



/****************************************************************************
 NAME: display_usage - display sundiag command line usage for on-line mode.
 SYNOPSIS: (void) display_usage(rtn_usage)
 ARGUMENTS:
        Input: char *rtn_usage - test command line string.
 DESCRIPTION: display_usage is required for every test code.
 RETURN VALUE: void
 GLOBAL: send_message()
*****************************************************************************/
void display_usage(rtn_usage)
char *rtn_usage;
{
    (void)sprintf(msg_buf, 
	"\nUsage: %s %s%s[s] [c] [p] [r] [q] [u] [v] [d] [t] [h hostname]\n",
        test_name, 
        rtn_usage != NULL ? rtn_usage : "", 
	rtn_usage != NULL ? " "       : "");
    send_message (0, CONSOLE, msg_buf);
}


/****************************************************************************
 NAME: standard_usage - display the general usage of Sundiag command lines.
 SYNOPSIS: void standard_usage()

 DESCRIPTION: This standard usage of Sundiag command lines is required for
		every test code; test programmers should accordinglin provide 
		relative command features in their test codes. 

 RETURN VALUE: void
 GLOBAL: send_message()

*****************************************************************************/
void standard_usage()
{
    (void) sprintf(msg_buf, "\nStandard arguments:\n\
	s      = sundiag mode\n\
	c      = enable core file\n\
	p      = single pass\n\
	r      = run on error\n\
	q      = quick test\n\
	u      = list usage\n\
	v      = verbose mode\n\
	d      = debug mode\n\
	t      = trace mode\n\
	h hostname   = RPC hostname\n\n");
    send_message (0, CONSOLE, msg_buf);
}

/***************************************************************************
 test_init(), paired with test_end() to construct Sundiag main code. This
 simplifies the sundiag test code a lot and relieve most of sundiag chore
 off the programmer. 
	argc, argv: unix standard input arguments.
	process_test_args: pointer to function that processes test-specific
			   command line arguments.
	routine_usage: pointer to function that explains test-specific 
		       command arguments.
	test_usage_msg: pointer to string that contains the general usage
			of the test command.
***************************************************************************/  
void test_init(argc, argv, process_test_args, routine_usage, test_usage_msg)
int    argc;
char  *argv[];
int  (*process_test_args)();
int  (*routine_usage)();
char  *test_usage_msg;
{
    int argindex;
    extern void check_usage();
    extern void check_coredump();
    extern int  check_default();

    test_name = argv[0];
    test_id = get_test_id(test_name, testname_list);
    version_id = get_sccs_version (versionid);

    (void)signal(SIGHUP, finish);
    (void)signal(SIGTERM, finish);
    (void)signal(SIGINT, finish);
 
    if (argc > 1) {
        for (argindex = 1; argindex < argc; argindex++) {
             if (!process_sundiag_args(argc, argv, &argindex)) {
                 if (!process_test_args(argv, argindex)) {
                     display_usage(test_usage_msg);
                     send_message(USAGE_ERROR, ERROR,
                        "Invalid command argument: %s", argv[argindex]);
                 }
             }
             check_usage(routine_usage, test_usage_msg);
        }     
    }   
    else if (check_default(test_usage_msg)) ;
    else {     
        display_usage (test_usage_msg);
        exit (USAGE_ERROR);
    } 
    check_coredump();

    sprintf(msg_buf, "%s: Started.\n", test_name);

    if (verbose)  
        send_message(0, CONSOLE, msg_buf); 
	    /* can't use VERBOSE as no way to identify device_name till now */
	    /* Indicates "started" at very beginning of each test */
}

/**************************************************************************
 test_end(), Sundiag test code enclosure routine.  Always use this routine
 to leave Sundiag.  This routine will be expanded to do some work more
 general to Sundiag for visual or statistical purpose.  
***************************************************************************/
void test_end()
{
    if (test_error > 0) {
	if (!exec_by_sundiag)
		sprintf(msg_buf, "%s: Stopped with errors.", test_name);
	else
		sprintf(msg_buf, "Stopped with errors.");
    } 
    else {
	if (!exec_by_sundiag) 
                sprintf(msg_buf, "%s: Stopped successfully.", test_name); 
        else 
                sprintf(msg_buf, "Stopped successfully.");
    }
	
    send_message(0, VERBOSE, msg_buf);

    if (run_on_error &&  test_error >0)
	exit(FREERUN_ERROR);
    else 
    	exit(0);
}
 

/****************************************************************************
 NAME: check_default()
       Used by test_init() to check any requirement for routine-specific 
		arguments.
 ASRGUMENTS:
	input:  test_usage_msg - pointer to string that contains the 
				 general usage of the test command line.
 RETURN VALUE:  TRUE if test_usage_msg is (char *NULL), or NULL.
	
*****************************************************************************/
int check_default(test_usage_msg)
char *test_usage_msg;
{
    return ((test_usage_msg == NULL || *test_usage_msg == NULL)? TRUE : FALSE);
}

/****************************************************************************
 * check_usage() 
   Used by test_init() to display test_usage_msg, standard_usage and
   routine_usage for 'u' flag set in test command line.
	routine_usage:  pointer to function that explains test-specific
                        command arguments. 
        test_usage_msg: pointer to string that contains the general usage of
			the test command line.
 ****************************************************************************/
void check_usage(routine_usage, test_usage_msg)
int (*routine_usage)();
char *test_usage_msg;
{
    if (list_usage) {
	    display_usage(test_usage_msg);
            standard_usage();
            routine_usage();
            exit(0);
    }
}
 
/****************************************************************************
 * check_coredump(), used by test_init() to catch some signals to 
 * provent core dump.
*****************************************************************************/
void check_coredump()
{
#ifdef SVR4
extern void exception(int);

    if (!core_file)
    {
      (void)signal(SIGILL, exception);
      (void)signal(SIGBUS, exception);
      (void)signal(SIGSEGV, exception);
    }
#else
extern void exception();
 
     if (!core_file)
     {
        (void)signal(SIGILL, exception);
	(void)signal(SIGBUS, exception);
	(void)signal(SIGSEGV, exception);
     }
#endif SVR4
}

