/******************************************************************************
	@(#)fpa3x.c - Rev 1.1 - 7/30/92
	Copyright (c) 1988 by Sun Microsystems, Inc.
	Deyoung Hong

	This file contains the entry and support functions for the system
	test of FPA-3X board.  The test is designed to be incorporated
	in Sundiag, but can also be run by itself under Sun-OS.
******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <varargs.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sundev/fpareg.h>
#include "../../include/sdrtns.h"	/* make sure not from local directory*/
#include "fpa3x_def.h"
#include "fpa3x_msg.h"


/* Global variables */
int fpa_version = FPA3X_VERSION;	/* FPA board version */
int menu_mode = FALSE;			/* run test in menu mode */
int numpass = 1;			/* default number of passes */
int passcount = 0;			/* test pass count */
int errorcount = 0;			/* error count */
int fpafd = -1;				/* device descriptor */
u_int context = -1;			/* FPA-3X context */
jmp_buf env;				/* setjump buffer */
char rw_flag = OFF;			/* read write FPA-3X flag */


/* FPA-3X device pointer from <sundev/fpareg.h> */
struct fpa_device *fpa = (struct fpa_device *)FPA3X_BASE;


/* Function type declarations */
void buserr(), sigfpe();
char *sprintf(), *strcpy(), *strcat();
int ierr_test(), imask_test(), loadptr_test(), mode_test();
int wstatus_test(), datareg_test(), shadow_test(), nack_test();
int simpleins_test(), pointer_test(), lock_test(), jumpcond_test();
int tipath_test(), tiop_test(), tistatus_test(), timing_test();

static char* test_usage_msg = "[menu] [np=<pass>]";


/*
 * Function to start FPA-3X test.  It performs initialization, parses the
 * user command, probe the devices, then run all tests sequentially.
 */
main(argc, argv)
int argc;
char *argv[];
{
extern int process_fpa_args();
extern int routine_usage();

	test_init(argc, argv, process_fpa_args, routine_usage, test_usage_msg);
        device_name = DEVICE_NAME;              /* default device name */

	(void) signal(SIGBUS,buserr);		/* FPA-3X nack signal */
	(void) signal(SIGFPE,buserr);

	if (menu_mode) runmenu();		/* run test in menu mode */
	else runtest();				/* run all tests default */

	test_end();
}


/*
 * Fpa test specific arguments check.
 */
process_fpa_args(argv, argindex)
char *argv[];
int   argindex;
{
	if(strncmp(argv[argindex],"np=",3) == 0) {
  	    if (sscanf(&argv[argindex][3],"%d",&numpass) <= 0)
                   return FALSE;	    
	}
        else if (strncmp(argv[argindex],"menu",4) == 0)
            menu_mode = TRUE;
	else
	    return FALSE;

	return TRUE;
}


routine_usage()
{
    (void) printf("Routine specific arguments [defaults]:\n\
	menu = menu_mode, suppersede sd\n\
	np = number of pass\n");
}

/*
 * Function to execute test for specified number of passes.
 */
runtest()
{
	register int k, nocontext;

	if (exec_by_sundiag)
		nocontext = 1;
	else
		nocontext = quick_test ? 1 : FPA_NCONTEXTS;

	for (passcount = 0; passcount < numpass; )
	{
		for (k = 0; k < nocontext; k++)
		{
			if (open_context()) return;
			send_message(0,VERBOSE,tst_context,
				(fpa_version&FPA3X_VERSION)? "+":"",context);
			run_fpa(ierr_test);
			run_fpa(imask_test);
			run_fpa(loadptr_test);
			run_fpa(mode_test);
			run_fpa(wstatus_test);
			run_fpa(datareg_test);
			run_fpa(shadow_test);
			run_fpa(nack_test);
			run_fpa(simpleins_test);
			run_fpa(pointer_test);
			run_fpa(lock_test);
			run_fpa(jumpcond_test);
			run_fpa(tipath_test);
			run_fpa(tiop_test);
			run_fpa(tistatus_test);
			run_fpa(timing_test);
			slinpack_test();
			dlinpack_test();
			spmath_test();
			dpmath_test();
			dptrig_test();
			if (close_context()) return;
		}
		++passcount;
	}
}


/*
 * Function to open a new FPA-3X context.  It also checks if the hardware
 * exists.  Function returns 0 if successful, else nonzero.
 */
open_context()
{
	u_int imask;

	/* open device if not already opened */
	if (fpafd < 0)
	{
		if ((fpafd = open("/dev/fpa",O_RDWR)) < 0)
		{
			switch (errno)
			{
			case ENXIO:
				send_message(FPERR,FATAL,er_nofpa);
				break;
			case ENOENT:
				send_message(FPERR,FATAL,er_no68881);
				break;
			default:
				send_message(FPERR,FATAL,er_opendev,device_name);
				break;
			}
			return(TRUE);
		}
	}

	/* read context */
	if (read_fpa(&fpa->fp_state,&context,ON)) return(TRUE);
	context &= CONTEXT_MASK;

	/* check board version */
	if (read_fpa(&fpa->fp_imask,&imask,ON)) return(TRUE);
	fpa_version = imask & FPA3X_VERSION;

	send_message(0,VERBOSE,tst_open,context);
	return(FALSE);
}


/*
 * Function to close the current FPA-3X context.
 * Function returns 0 if successful, else nonzero.
 */
close_context()
{
	if (close(fpafd) < 0)
	{
		send_message(FPERR,FATAL,er_closedev,device_name);
		return(TRUE);
	}
	fpafd = -1;
	send_message(0,VERBOSE,tst_close,context);
	return(FALSE);
}


/*
 * Function to initialize FPA-3X before and after executing a device test.
 */
run_fpa(devtest)
register int (*devtest)();
{
	if (write_fpa(&fpa->fp_restore_mode3_0,FPA_MODE3_INITVAL,ON)) return;
	(*devtest)();
	if (write_fpa(&fpa->fp_clear_pipe,X,ON)) return;
	if (write_fpa(&fpa->fp_imask,CLEAR,ON)) return;
	(void) write_fpa(&fpa->fp_ierr,CLEAR,ON);
}


/*
 * Function to write to a memory map long word address and detect bus error.
 * The printflag indicates whether to print a message if bus error occurs.
 * Function returns 0 if no bus error, else nonzero.
 */
write_fpa(addr,value,printflag)
register u_int *addr, value;
register int printflag;
{
	int bus_error;

	rw_flag = ON;
	bus_error = setjmp(env);
	if (!bus_error) *addr = value;
	else if (printflag) send_message(0,ERROR,er_writefpa,addr);
	rw_flag = OFF;
	return(bus_error);
}


/*
 * Function to read a memory map long word address and detect bus error.
 * The printflag indicates whether to print a message if bus error occurs.
 * Function returns 0 if no bus error, else nonzero.
 */
read_fpa(addr,value,printflag)
register u_int *addr, *value;
register int printflag;
{
	int bus_error;

	rw_flag = ON;
	bus_error = setjmp(env);
	if (!bus_error) *value = *addr;
	else if (printflag) send_message(0,ERROR,er_readfpa,addr);
	rw_flag = OFF;
	return(bus_error);
}


/*
 * Function to handle bus error exception.  Function will do a long jump to
 * return upon write read FPA-3X memory map location, and call exception()
 * if bus error occurs unexpectedly.
 */
void buserr()
{
	if (rw_flag) longjmp(env,TRUE);
	else exception(SIGBUS,X,(struct sigcontext *)X,(char *)X);
}


/*
 * Function to clean up due to abnormal exit.
 */
clean_up()
{
	if (fpafd > 0) {
		(void) write_fpa(&fpa->fp_clear_pipe,X,ON);
		(void) close(fpafd);
	}
}


