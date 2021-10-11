static char sccsid[] = "@(#)signal.c 1.1 7/30/92 Copyright Sun Microsystems"; 

/*
 * ****************************************************************************
 * Source File     : signal.c
 * Original Engr   : Nancy Chow
 * Date            : 10/13/88
 * Function        : This module contains the modules to initialize the fpa
 *		   : and the signal handlers used by fparel.
 * Revision #1 Engr: 
 * Date            : 
 * Change(s)       :
 * ****************************************************************************
 */

/* ***************
 * Include Files
 * ***************/

#include <sys/file.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include "fpa3x.h"
#include "fpcrttypes.h"

/* ***************
 * Local Globals
 * ***************/

struct sigvec newfpe;		/* new FPA signal handler */
struct sigvec oldfpe;		/* old FPA signal handler */
struct sigvec newseg;		/* new Seg Violation handler */
struct sigvec oldseg;		/* old Seg Violation handler */

/* *****************
 * sigsegv_handler
 * *****************/

/* Segmentation Violation Signal Handler */

void sigsegv_handler(sig, code, scp)
int               sig;
int               code;
struct sigcontext *scp;
{

FILE *input_file;			/* diag log file pointer */
char mesg[180];				/* message string */ 

    mesg[0] = '\0';			/* init str */

    time_of_day(mesg);			/* get time of day */

    if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL) {
	printf("FPA Reliability Test Error.\n");
	printf("    Could not access /usr/adm/diaglog file, can be accessed only under root.\n");
	exit(1);
    }

    strcat(mesg, " : Sun FPA Reliability Test Failed due to System Segment Violation error.\n");
    fputs(mesg, input_file);		/* log message */
    fclose(input_file);
    make_broadcast_msg();		/* send mesg to all users logged on */
    make_unix_msg(mesg);		/* send system message */
    exit(1);
}

/* ****************
 * sigfpe_handler
 * ****************/

/* FPA error signal handler */

void sigfpe_handler(sig, code, scp)
int               sig;
int               code;
struct sigcontext *scp;
{

FILE *input_file;			/* diag log file pointer */
char bus_msg[100];			/* buserror message */
char mesg[180];				/* message str */
int  bus_errno;				/* type of error */

    bus_msg[0] = '\0';			/* init message string */
    mesg[0]    = '\0';			/* init message string */

					/* if Nack test and FPA error */
    if ((seg_sig_flag == TRUE) && (code == FPE_FPA_ERROR)) {
	*(u_long *)REG_SCLEARPIPE = 0x0;	/* soft clear pipe */
	if (((*(u_long *)REG_IERR) & IERR_BMASK) != PIPE_HUNG)
	    bus_errno = NOT_HUNG;	/* pipe not hung during Nack test */
	else {
	    *(u_long *)REG_IERR = 0x0;	/* clear IERR register */
	    return;
	}
    }
    else 
	if (code == FPE_FPA_ERROR) {
	    if (sig_err_flag) {		/* use error handler on FPA error? */
		if (fpa_handler(sig, code, scp))
		    return;
	    }
	    else
		bus_errno = FPA_BUS;	/* bus error due to FPA error */
	}
	else {
	    bus_errno = BUS_ERR;	/* bus error */
	    switch (code) {
		case FPE_INTDIV_TRAP  : strcat(bus_msg,"INTDIV_TRAP"); break;
		case FPE_CHKINST_TRAP : strcat(bus_msg,"CHKINST_TRAP"); break;
		case FPE_TRAPV_TRAP   : strcat(bus_msg,"TRAPV_TRAP"); break;
		case FPE_FLTBSUN_TRAP : strcat(bus_msg,"FLTBSUN_TRAP"); break;
		case FPE_FLTINEX_TRAP : strcat(bus_msg,"FLTINEX_TRAP"); break;
		case FPE_FLTDIV_TRAP  : strcat(bus_msg,"FLTDIV_TRAP"); break;
		case FPE_FLTUND_TRAP  : strcat(bus_msg,"FLTUND_TRAP"); break;
		case FPE_FLTOPERR_TRAP: strcat(bus_msg,"FLTOPERR_TRAP"); break;
		case FPE_FLTOVF_TRAP  : strcat(bus_msg,"FLTOVF_TRAP"); break;
		case FPE_FLTNAN_TRAP  : strcat(bus_msg,"FLTNAN_TRAP"); break;
		case FPE_FPA_ENABLE   : strcat(bus_msg,"FPA_ENABLE"); break;
/*		case FPE_FPA_ERROR    : strcat(bus_msg,"FPA_ERROR"); break;*/
		default               : strcat(bus_msg,"Other than fpa error code");
	    }
	}

    time_of_day(mesg);			/* get time of day */
    if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL) {
	printf("FPA Reliability Test Error.\n");
	printf("    Could not access /usr/adm/diaglog file, can be accessed only under root.\n");
	exit(1);
    }

    strcat(mesg, " : Sun FPA Reliability Test Failed");
    switch (bus_errno) {
	case NOT_HUNG : strcat(mesg,". Nack Test: Pipe is not hung"); break;
	case FPA_BUS  : strcat(mesg," due to bus error : FPA_ERROR"); break;
	case BUS_ERR  : strcat(mesg," due to bus error : ");
		 strcat(mesg, bus_msg);
		 break;
    }
    strcat(mesg, ".\n");
    fputs(mesg, input_file);		/* log message */
    fclose(input_file);			/* close log file */
    make_broadcast_msg();		/* send msg to all users logged on */
    make_unix_msg(mesg);		/* send system message */
    exit(1);
}

/* **********
 * winitfp_
 * **********/

/* This routine is used to determine if a physical FPA and 68881 are present
   and set fp_state_sunfpa and fp_state_mc68881 accordingly.
   Returns PASS if both present and FAIL otherwise */

winitfp_()
{

    if (fp_state_sunfpa == fp_unknown) {
	if (minitfp_() != 1)
	    fp_state_sunfpa = fp_absent;
	else {
	    dev_no = open("/dev/fpa", O_RDWR);
	    if ((dev_no < 0) && (errno != EEXIST)) 
		fp_state_sunfpa = fp_absent ;
	    else {
	    	fcntl(dev_no, F_SETFD, 1);	/* set close-on-exec flag */

						/* set s/w FPA signal handler */
		newfpe.sv_mask = 0 ;			/* new signal mask */
		newfpe.sv_onstack = 0 ;			/* new onstack flag */
		newfpe.sv_handler = sigfpe_handler ;	/* new handler */
		sigvec( SIGFPE, &newfpe, &oldfpe ) ;	/* set sig handler */
		
				/* set s/w Seg Violation signal handler */
		newseg.sv_mask = 0 ;			/* new signal mask */
		newseg.sv_onstack = 0 ;			/* new onstack flag */
		newseg.sv_handler = sigsegv_handler;	/* new handler */
		sigvec(SIGSEGV, &newseg, &oldseg);	/* set sig handler */

		*(u_long *)INS_WRMODE = 2;	/* set to round integers toward zero */
		*(u_long *)REG_IMASK  = 1;	/* set IMASK to one */
		fp_state_sunfpa = fp_enabled ;
	    }
	}
    }
    if (fp_state_sunfpa == fp_enabled) 
	fp_switch = fp_sunfpa ;
    return((fp_state_sunfpa == fp_enabled) ? PASS : FAIL);
}
	    
/* *****************
 * restore_signals
 * *****************/

/* This routine is used to restore the previous signal handlers. */

restore_signals()
{
    sigvec(SIGSEGV,&oldseg, &newseg);	/* restore Seg Violation handler */
    sigvec(SIGFPE, &oldfpe, &newfpe);	/* restore FPA signal handler */
}
