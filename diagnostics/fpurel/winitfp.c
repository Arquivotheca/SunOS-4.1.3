/*
 *"@(#)winitfp.c 1.1 7/30/92  Copyright Sun Microsystems";
 */
#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <nlist.h>

/*
#include "fpcrttypes.h"
*/

extern int errno ;
int open_fpa;
extern	make_unix_msg(), make_broadcast_msg(), time_of_day();
extern	char	send_err_msg[];
extern  unsigned long error_ok;

int	restore_svmask;
int	restore_svonstack;
void	(*restore_svhandler)();
int	res_seg_svmask;
int	res_seg_svonstack;
void	(*res_seg_svhandler)();
void    sigfpe_handler();
unsigned long   fsr_at_trap;
unsigned long	trap_flag;

extern	int verbose_mode;

struct sigvec newfpu, oldfpu, oldseg, newseg;
struct nlist nl[] = {
        { "_fpu_exists" },
        "",
};
/*
 *      returns '0' if there is fpu
 *      else '1' if there is no fpu
 *      others if there is an error
 */
check_fpu()
{
        register int off;
        int value;
        int kmem;

	if (geteuid() != 0) {
		(void) fprintf(stderr, "Must be root to run fpurel.\n");
		exit(1);
        }

        if (nlist("/vmunix", nl) == -1) {
           if (verbose_mode)
                printf("error in reading namelist %d\n",errno);
	   close(kmem);
           return(2);
        }
        if (nl[0].n_value == 0) {
           if (verbose_mode)
                printf ("Variables missing from namelist\n");
	   close(kmem);
           return(3);
        }
        if ((kmem = open("/dev/kmem", 0)) < 0) {
           if (verbose_mode)
                printf("can't open kmem\n");
	   close(kmem);
           return(4);
        }

        off = nl[0].n_value;
        if (lseek(kmem, off, 0) == -1) {
           if (verbose_mode)
                printf("can't seek in kmem\n");
	   close(kmem);
           return(5);
        }
        if (read(kmem, &value, sizeof(int)) != sizeof (int)) {
           if (verbose_mode)
                printf("can't read fpu_exists from kmem\n");
	   close(kmem);
           return(6);
        }
	close(kmem);
        if (value == 1)
                return(0);
        else
           return(1);
 
}

int	sigsegv_handler( sig, code, scp)
int sig, code ;
struct sigcontext *scp ;
{
	char    *ptr1, *ptr2, *ptr3;
        FILE    *input_file, *fopen();
        
	ptr1 = send_err_msg;
	time_of_day();
        if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL)
        { 
	  if (verbose_mode) {
	 printf("FPU Reliability Test Error (Segmentation Violation).\n");
          printf("    Could not access /usr/adm/diaglog file, can be accessed only under root.\n");
	   }
                exit(1);
        }
        strcat(ptr1," : Sun FPU Reliability Test Failed due to System Segment Violation error.\n");
        fputs(ptr1,input_file);
	ptr2 = send_err_msg;
	strcat(ptr2,"  While Doing the ");
	strcat(ptr2, " test.\n");
	fputs(ptr2, input_file);
	
        fclose(input_file);
        make_broadcast_msg();
/*
        make_unix_msg();
*/
        exit(1);

}

void
sigfpe_handler( sig, code, scp)
int sig, code ;
struct sigcontext *scp ;

{
	char	bus_msg[100];
	char    *ptr1, *ptr2, *ptr3;
        FILE    *input_file, *fopen();
	int	*ptr4, *ptr5;
	int	bus_errno;

	fsr_at_trap = get_fsr();
	trap_flag = 0x1;
	if (error_ok)
		return;
	ptr1 = send_err_msg;
	bus_msg[0] = '\0';
	ptr2 = bus_msg;
	time_of_day();
        if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL)
        {
	  if (verbose_mode) {
	 printf("FPU Reliability Test Error Due to FPU Exception.\n");
          printf("    Could not access /usr/adm/diaglog file, can be accessed only under root.\n");
	  }
                exit(1);
        }
	strcat(ptr1," : Sun FPU Reliability Test Failed");


        strcat(ptr1," Due to FPU Exception.\n");
        fputs(ptr1,input_file);
	ptr2 = send_err_msg;
        strcat(ptr2,"  While Doing the ");
        strcat(ptr2, " test.\n");
        fputs(ptr2, input_file);

        fclose(input_file);
        make_broadcast_msg();
        make_unix_msg();
        exit(1);


}

int winitfp()

/*
 *	Procedure to determine if a physical FPA and 68881 are present and
 *	set fp_state_sunfpa and fp_state_mc68881 accordingly.
	Also returns 1 if both present, 0 otherwise.
 */

{
int openfpa, mode81, psr_val;
long *fpaptr ;
			/* get the psr */

	if (check_fpu())
		return(-1);

	restore_svmask = oldfpu.sv_mask;
	restore_svonstack = oldfpu.sv_onstack;
	restore_svhandler = oldfpu.sv_handler;
	newfpu.sv_mask = 0 ;
	newfpu.sv_onstack = 0 ;
	newfpu.sv_handler = sigfpe_handler ;
	sigvec( SIGFPE, &newfpu, &oldfpu ) ; 

	res_seg_svmask = oldseg.sv_mask; 
        res_seg_svonstack = oldseg.sv_onstack; 
        res_seg_svhandler = oldseg.sv_handler; 
        newseg.sv_mask = 0 ; 
        newseg.sv_onstack = 0 ; 
      	newseg.sv_handler = (void *)sigsegv_handler; 
       	sigvec(SIGSEGV, &newseg, &oldseg);

	return(0);
}


restore_signals()
{

	oldseg.sv_mask = res_seg_svmask;
	oldseg.sv_onstack = res_seg_svonstack;
	oldseg.sv_handler = res_seg_svhandler;
	sigvec(SIGSEGV,&oldseg, &newseg);
	oldfpu.sv_mask = restore_svmask;
	oldfpu.sv_onstack = restore_svonstack;
	oldfpu.sv_handler = restore_svhandler;
	sigvec(SIGFPE, &oldfpu, &newfpu);
	
}
