/*
 *static	char sccsid[] = "@(#)winitfp.c 1.1 7/30/92  Copyr 1985 Sun Micro";
 */
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/file.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include "fpcrttypes.h"

extern int errno, sig_err_flag, seg_sig_flag;
int open_fpa;
extern	make_unix_msg(), make_broadcast_msg(), time_of_day();
extern	char	send_err_msg[];
int	restore_svmask;
int	restore_svonstack;
void	(*restore_svhandler)();
int	res_seg_svmask;
int	res_seg_svonstack;
void	(*res_seg_svhandler)();
struct sigvec newfpe, oldfpe, oldseg, newseg;

void	sigsegv_handler( sig, code, scp)
int sig, code ;
struct sigcontext *scp ;
{
	char    *ptr1, *ptr2, *ptr3;
        FILE    *input_file, *fopen();

        ptr1 = send_err_msg;
	time_of_day();
        if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL)
        {
	 printf("FPA Reliability Test Error.\n");
          printf("    Could not access /usr/adm/diaglog file, can be accessed only under root.\n");
                exit(1);
        }
        strcat(ptr1," : Sun FPA Reliability Test Failed due to System Segment Violation error.\n");
        fputs(ptr1,input_file);
        fclose(input_file);
        make_broadcast_msg();
        make_unix_msg();
        exit(1);

}

void	sigfpe_handler( sig, code, scp)
int sig, code ;
struct sigcontext *scp ;

{
	char	bus_msg[100];
	char    *ptr1, *ptr2, *ptr3;
        FILE    *input_file, *fopen();
	int	*ptr4, *ptr5;
	int	bus_errno;

	ptr4 = (int *)0xE0000F84; /* soft clear pipe */
	ptr5 = (int *)0xE0000F1C; /* ierr register to check for nack test */
	ptr1 = send_err_msg;
	bus_msg[0] = '\0';
	ptr2 = bus_msg;
	if ((seg_sig_flag) && (code == FPE_FPA_ERROR))
	{
		
		*ptr4 = 0x0;
		if (((*ptr5 >> 16) & 0xff) != 0x20)
			bus_errno = 1;
		else {
			*ptr5 = 0x0; /* clear the ierr register */	
                	return;
		}
	}
	else if (code == FPE_FPA_ERROR)
	{
		if (sig_err_flag) 
		{
			if (fpa_handler(sig, code, scp)) 
				return;
		}
		else  
			bus_errno = 2;
	}
	else
	{
		bus_errno = 3;
		switch(code)
		{
        		case  FPE_INTDIV_TRAP : strcat(ptr2,"INTDIV_TRAP"); break;
        		case  FPE_CHKINST_TRAP : strcat(ptr2,"CHKINST_TRAP"); break;
        		case  FPE_TRAPV_TRAP : strcat(ptr2,"TRAPV_TRAP"); break;
        		case  FPE_FLTBSUN_TRAP : strcat(ptr2,"FLTBSUN_TRAP"); break;
        		case  FPE_FLTINEX_TRAP : strcat(ptr2,"FLTINEX_TRAP"); break;
        		case  FPE_FLTDIV_TRAP : strcat(ptr2,"FLTDIV_TRAP"); break;
        		case  FPE_FLTUND_TRAP : strcat(ptr2,"FLTUND_TRAP"); break;
        		case  FPE_FLTOPERR_TRAP : strcat(ptr2,"FLTOPERR_TRAP"); break;
        		case  FPE_FLTOVF_TRAP : strcat(ptr2,"FLTOVF_TRAP"); break;
        		case  FPE_FLTNAN_TRAP: strcat(ptr2,"FLTNAN_TRAP"); break;
        		case  FPE_FPA_ENABLE : strcat(ptr2,"FPA_ENABLE"); break;
        		case  FPE_FPA_ERROR : strcat(ptr2,"FPA_ERROR"); break;
			default: strcat(ptr2,"Other than fpa error code"); 
        	}
	}
	time_of_day();
        if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL)
        {
	 printf("FPA Reliability Test Error.\n");
          printf("    Could not access /usr/adm/diaglog file, can be accessed only under root.\n");
                exit(1);
        }
	strcat(ptr1," : Sun FPA Reliability Test Failed");
	switch(bus_errno)
	{
		case 1 :
		 strcat(ptr1,". Nack Test: Pipe is not hung");
		 break;
		case 2 : 
		 strcat(ptr1," due to bus error : FPA_ERROR");
		 break;
		case 3 :
		 strcat(ptr1," due to bus error : ");
		 strcat(ptr1,ptr2);
		 break;
	}
        strcat(ptr1,".\n");
        fputs(ptr1,input_file);
        fclose(input_file);
        make_broadcast_msg();
        make_unix_msg();
        exit(1);

}

int winitfp_()

/*
 *	Procedure to determine if a physical FPA and 68881 are present and
 *	set fp_state_sunfpa and fp_state_mc68881 accordingly.
	Also returns 1 if both present, 0 otherwise.
 */

{
int openfpa, mode81 ;
long *fpaptr ;

if (fp_state_sunfpa == fp_unknown) 
	{
	if (minitfp_() != 1) fp_state_sunfpa = fp_absent ;
	else
		{
		openfpa = open("/dev/fpa", O_RDWR);
		open_fpa = openfpa;
		if ((openfpa < 0) && (errno != EEXIST)) 
			{ /* openfpa < 0 */
			if (errno == EBUSY) 
				{
/*				fprintf(stderr,"\n No Sun FPA contexts available - all in use\n"); fflush(stderr) ;
*/				}
			fp_state_sunfpa = fp_absent ;
			} /* openfpa < 0 */
		else
			{ /* openfpa >= 0 */
			if (errno == EEXIST) 
				{
/*				fprintf(stderr,"\n  FPA was already open \n") ; fflush(stderr) ;
*/
				}
			/* to close FPA context on execve() */
			fcntl(openfpa, F_SETFD, 1);
			restore_svmask = oldfpe.sv_mask;
			restore_svonstack = oldfpe.sv_onstack;
			restore_svhandler = oldfpe.sv_handler;
			newfpe.sv_mask = 0 ;
			newfpe.sv_onstack = 0 ;
			newfpe.sv_handler = sigfpe_handler ;
			sigvec( SIGFPE, &newfpe, &oldfpe ) ; 

			res_seg_svmask = oldseg.sv_mask; 
	                res_seg_svonstack = oldseg.sv_onstack; 
       	                res_seg_svhandler = oldseg.sv_handler; 
	                newseg.sv_mask = 0 ; 
       	                newseg.sv_onstack = 0 ; 
                	newseg.sv_handler = sigsegv_handler; 
                	sigvec(SIGSEGV, &newseg, &oldseg);

			fpaptr = (int*) 0xe00008d0 ; /* write mode register */
			*fpaptr = 2 ; /* set to round integers toward zero */
			fpaptr = (int*) 0xe0000f14 ;
			*fpaptr = 1 ; /* set imask to one */
			fp_state_sunfpa = fp_enabled ;
/*			if (!MA93N()) 
			{
			fprintf(stderr,"\n Warning! Sun FPA works best with 68881 mask A93N \n") ;
			fflush(stderr) ;
                	}
*/
			} /* openfpa >= 0 */
		}
	}
if (fp_state_sunfpa == fp_enabled) fp_switch = fp_sunfpa ;
return((fp_state_sunfpa == fp_enabled) ? 1 : 0) ;
}


restore_signals()
{

	oldseg.sv_mask = res_seg_svmask;
	oldseg.sv_onstack = res_seg_svonstack;
	oldseg.sv_handler = res_seg_svhandler;
	sigvec(SIGSEGV,&oldseg, &newseg);
	oldfpe.sv_mask = restore_svmask;
	oldfpe.sv_onstack = restore_svonstack;
	oldfpe.sv_handler = restore_svhandler;
	sigvec(SIGFPE, &oldfpe, &newfpe);
	
}
