static char     fpasccsid[] = " @(#)fptest.c 1.1 7/30/92 Copyright Sun Microsystems";

/*
 * fptest.c:  main() routine, etc. used by fputest and mc68881
 */

#include <sys/types.h>
#include <stdio.h>
#include <sys/file.h>
#include <signal.h>
#ifdef SVR4
#include <sys/fcntl.h>
#include <siginfo.h>
#include <ucontext.h>
#endif SVR4
#include <math.h>
#include <errno.h>
#include <sys/mman.h>
#include <kvm.h>
#include <nlist.h>
#include <setjmp.h>

#include "fp.h"

#include <sys/time.h>
#include <sys/wait.h>
#include "sdrtns.h"


#define pi			3.141592654

#define FP_EXCEPTION_ERROR      3
#define COMPUTATION_ERROR       4
#define SPMATH_ERROR		5
#define SYSTEST_ERROR		6
#define PROBE_EEPROM_ERROR	7
#define DPMATH_ERROR		10
#define NO_COMPARE_FILES	69

#define TIMES   		10000
#define SLEEP_MOD 		1000
#define ANSWER_SB  		1.6364567280
#define MARGIN      		.0000000010
#define SPMARGIN		.0000001
#define DPMARGIN		.000000000000001
#define fp_absent		0
#define fp_enabled		1

#define	CTOB(x)			(((u_long)(x)) * pagesize)
#define BTOC(x)			(((u_long)(x)) / pagesize)
#define PADDR(page, addr)	(CTOB(page) + ((u_long)addr & (pagesize-1)))

int             simulate_error = 0;
char            perror_msg_buffer[30];
char           *perror_msg = perror_msg_buffer;

int		pass = 0;
int		errors = 0;
char		msg_buffer[MESSAGE_SIZE];
char		*msg = msg_buffer;
char		device_fpname[20];
char		*device = device_fpname;


char            sysdiag_directory[50];
char           *SD = sysdiag_directory;
extern int      errno;

#ifdef FPU
extern int      fpu_sysdiag();
#ifdef SVR4
extern void     sigfpe_handler(int, siginfo_t *, ucontext_t *);
struct sigaction newfpu, oldfpu;
#else SVR4
extern int      sigsegv_handler();
extern int      sigfpe_handler();
#endif SVR4
#endif

#ifdef sun4
#ifdef SVR4
struct nlist nl[] = {
        {"fpu_exists", 0, 0, 0, 0, 0 },
        { "" },
};
#else SVR4
struct nlist    nl[] = {
			{"_fpu_exists"},
			"",
};
#endif SVR4

/*
 *      returns '0' if there is fpu
 *      else '1' if there is no fpu
 *      else print message and exit
 */
check_fpu()
{
    kvm_t           *mem;
    int             value;
    char 	    *vmunix, *getenv();

    vmunix = getenv("KERNELNAME");
    if (( mem = kvm_open(vmunix, NULL, NULL, O_RDONLY, NULL)) == NULL)
	send_message(-PROBE_DEV_ERROR, ERROR, "kvm_open failed.");
    if (kvm_nlist(mem, nl) == -1)
	send_message(-PROBE_DEV_ERROR, ERROR, "kvm_nlist failed.");
    if (kvm_read(mem, nl[0].n_value, &value, sizeof(int)) != sizeof(int))
	send_message(-PROBE_DEV_ERROR, ERROR, "kvm_read failed.");
    kvm_close(mem);
    if ( value == 1 )
    	return(0);
    else
	return(1);
}
#endif					       /* end of sun4 */

static int      dummy(){return FALSE;}

main(argc, argv)
    int             argc;
    char           *argv[];
{
    register float  x, y, z;
    register int    i, j;
    int             fpa = FALSE;
    int             spmath(), dpmath();
    u_long         *fpa_pointer;
    int             type;
    char           *fp;
    int             probe881(), probe;
    char            temp;

    test_init(argc, argv, dummy, dummy, (char *)NULL);
    test_name = TEST_NAME;	/* has to be here to overwrite default argv[0]*/

    if (getenv("SD_LOG_DIRECTORY"))
	exec_by_sundiag = TRUE;
    if (getenv("SD_LOAD_TEST"))
	if (strcmp(getenv("SD_LOAD_TEST"), "yes") == 0)
	    quick_test = TRUE;

    sprintf(perror_msg, " perror says");

#ifdef sun3
    if (minitfp_()) {
	send_message(0, VERBOSE,"An MC68881 is installed.");
	strcpy(device, "MC68881");
    } else {
	send_message(0, VERBOSE, "No 68881 is installed."); 
	strcpy(device, "softfp");
    }
#else
#ifdef sun4
    if (check_fpu() == 0) {
	strcpy(device, "fpu");
	device_name = device;  /* need device name for verbose message */
	send_message(0, VERBOSE,"An FPU is installed.");
    }
#else
    strcpy(device, "softfp");
#endif
#endif

    device_name = device; 	/* Got the device name here. */

#ifdef FPU
#ifdef SVR4
        newfpu.sa_handler = sigfpe_handler;
        sigemptyset(&newfpu.sa_mask);
        newfpu.sa_flags = SA_SIGINFO;

        if(sigaction(SIGFPE, &newfpu, &oldfpu)) {
                perror("sigaction SIGFPE");
                exit();
        }
#else SVR4
        signal(SIGFPE, sigfpe_handler);
#endif SVR4
#endif

    while (pass == 0) {
	pass++;
#ifdef FPU
	if (fpu_sysdiag() == (-1)) {
	    errors++;
	    send_message(-SYSTEST_ERROR, FATAL, "Failed systest for FPU.");
	}
	send_message(0, VERBOSE, "Passed Systest for FPU.");
#endif

#ifdef MCNSOFT

	if (spmath())
	    send_message(-SPMATH_ERROR, ERROR, 
		"Failed single precision FPA math test.");
	send_message(0, VERBOSE, 
		"Passed single precision FPA math test using the %s.",
			Device);

	if (dpmath())
	    send_message(-DPMATH_ERROR, ERROR,
		"Failed double precision FPA math test.");
	send_message(0, VERBOSE, 
		"Passed double precision FPA math test using the %s.",
			Device);

	x = 1.4567;
	y = 1.1234;

	if (quick_test)
	    j = 1;
	else
	    j = TIMES;

	    send_message(0, VERBOSE,
		"Starting multiple multiplications.");
	for (i = 0; i < j; ++i) {
	    z = x * y;

	    send_message(0, DEBUG,
		"Result: was (%2.10f), expected (%2.10f), Diff (%2.10e)", 
			z, ANSWER_SB, z - ANSWER_SB);
	    if (z < (ANSWER_SB - MARGIN) || z > (ANSWER_SB + MARGIN)) {
		errors++;
		send_message(-COMPUTATION_ERROR, ERROR, 
	"Multiplication failed, result was (%2.10f), expected (%2.10f).",
			z, ANSWER_SB);
	    }
	    if (i % SLEEP_MOD == 0)
		send_message(0, DEBUG, "i = %d", i);
	    if (!debug && !quick_test && (i % SLEEP_MOD) == 0)
		sleep(3);
	}
	if (quick_test)
	    break;
	send_message(0, VERBOSE, "%s passed.",Device);
#endif					       /* endif for def of MC68881 */

    }/* end of while */

    test_end();
}/* end of main */


spmath()
{
    float           a, b, ans;
    int             spfpa;

    a = 1.2345;
    b = 0.9876;

    /* Basic tests of the following arithmetic operations: +, -, *, and /   */
    send_message(0, VERBOSE, "Starting SPMATH routine on %s.", 
		Device);
    for (spfpa = 1; spfpa < 100; ++spfpa) {
	if (spfpa % 10 == 0)
	    send_message(0, DEBUG, "%s: spfpa= %d", Device, spfpa);
	ans = a + b;
	ans = a + b;
	if (ans != 2.2221000) {
	    if (ans < (2.2221000 - SPMARGIN) || ans > (2.2221000 + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a + b\nExpected: 2.2221000    	Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = (a - b);
	if (ans != 0.2469000) {
	    if (ans < (0.2469000 - SPMARGIN) || ans > (0.2469000 + SPMARGIN)) {
		send_message(0, ERROR, "Error   a - b\nExpected: 0.2469000    	Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a * b;
	if (ans != 1.2191923) {
	    if (ans < (1.2191923 - SPMARGIN) || ans > (1.2191923 + SPMARGIN)) {
		send_message(0, ERROR, "Error   a * b\nExpected: 1.2191922    	Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a / b;
	if (ans != 1.2500000) {
	    if (ans < (1.2500000 - SPMARGIN) || ans > (1.2500000 + SPMARGIN)) {
		send_message(0, ERROR, "Error   a / b\nExpected: 1.2500000	Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a + (a - b);
	if (ans != 1.4814000) {
	    if (ans < (1.4814000 - SPMARGIN) || ans > (1.4814000 + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a + (a + b)\nExpected: 1.4814000 	Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a - (a + b);
	if (ans != -(0.9876000)) {
	    if (ans < (-(0.9876000) - SPMARGIN) ||
					ans > (-(0.9876000) + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a - (a + b)\nExpected: -0.9876000 	Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a + (a * b);
	if (ans != 2.4536924) {
	    if (ans < (2.4536924 - SPMARGIN) || ans > (2.4536924 + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a + (a * b)\nExpected: 2.4536924  	Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a - (a * b);
	if (ans != 0.0153078) {
	    if (ans < (0.0153078 - SPMARGIN) || ans > (0.0153078 + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a - (a * b)\nExpected: 0.0153078    	Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a + (a / b);
	if (ans != 2.4845002) {
	    if (ans < (2.4845002 - SPMARGIN) || ans > (2.4845002 + SPMARGIN)) {
		send_message(0, ERROR, "a + (a / b)\nExpected: 2.4845002   	 Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a - (a / b);
	if (ans != -(0.0155000)) {
	    if (ans < (-(0.0155000) - SPMARGIN) ||
					ans > (-(0.0155000) + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a - (a / b)\nExpected: -0.0155000   	 Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a * (a + b);
	if (ans != 2.7431827) {
	    if (ans < (2.7431827 - SPMARGIN) || ans > (2.7431827 + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a * (a + b)\nExpected: 2.7431825   	 Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a * (a - b);
	if (ans != 0.3047981) {
	    if (ans < (0.3047981 - SPMARGIN) || ans > (0.3047981 + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a * ( a - b)\nExpected: 0.3047980   	 Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a / (a + b);
	if (ans != 0.5555556) {
	    if (ans < (0.5555556 - SPMARGIN) || ans > (0.5555556 + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a / ( a - b)\nExpected: 0.5555550      Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a / (a - b);
	if (ans != 4.9999995) {
	    if (ans < (4.9999995 - SPMARGIN) || ans > (4.9999995 + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a / ( a - b)\nExpected: 5.0000000   	 Actual: %1.7f", ans);
		return (1);
	    }
	}
	ans = a * (a / b);
	if (ans != 1.5431250) {
	    if (ans < (1.5431250 - SPMARGIN) || ans > (1.5431250 + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a * ( a / b)\nExpected: 1.5431250   	 Actual: %1.7f)", ans);
		return (1);
	    }
	}
	ans = a / (a * b);
	if (ans != 1.0125557) {
	    if (ans < (1.0125557 - SPMARGIN) || ans > (1.0125557 + SPMARGIN)) {
		send_message(0, ERROR, "Error:  a / ( a * b)\nExpected: 1.0125557	 Actual: %1.7f)", ans);
		return (1);
	    }
	}
	if (quick_test)
	    break;
	if (!quick_test && (spfpa % 25) == 0)
	    sleep(1);
    }					       /* end of for loop */
    return (0);

}

dpmath()
{
    double          x, result;
#ifdef SVR4
    double	    a, b, ans;
#else SVR4
    long float      a, b, ans;
#endif SVR4
    int             dpfpa;

    a = 1.2345;
    b = 0.9876;
    /* Basic tests of the following arithmetic operations: +, -, *, and /   */
    send_message(0, VERBOSE, "Starting DPMATH routine on %s.", Device);
    for (dpfpa = 1; dpfpa < 100; ++dpfpa) {

	if (dpfpa % 10 == 0)
	    send_message(0, DEBUG, "%s: dpfpa= %d", Device, dpfpa);
	ans = (a + b);
	if (ans != 2.222100000000000) {
	    if (ans < (2.222100000000000 - DPMARGIN) ||
				ans > (2.222100000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a + b\nExpected: 2.222100000000000    	Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = (a - b);
	if (ans != 0.246899999999999) {
	    if (ans < (0.246899999999999 - DPMARGIN) ||
				ans > (0.246899999999999 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a - b\nExpected: 0.246899999999999    	Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a * b;
	if (ans != 1.219192199999999) {
	    if (ans < (1.219192199999999 - DPMARGIN) ||
				ans > (1.219192199999999 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a * b\nExpected: 1.219192199999999    	Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a / b;
	if (ans != 1.249999999999999) {
	    if (ans < (1.249999999999999 - DPMARGIN) ||
				ans > (1.249999999999999 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a / b\nExpected: 1.249999999999999	Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a + (a - b);
	if (ans != 1.481399999999999) {
	    if (ans < (1.481399999999999 - DPMARGIN) ||
				ans > (1.481399999999999 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a + (a - b)\nExpected: 1.481399999999999 	Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a - (a + b);
	if (ans != -(0.987600000000000)) {
	    if (ans < (-(0.987600000000000) - DPMARGIN) ||
				ans > (-(0.987600000000000) + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a - (a + b)\nExpected: -0.987600000000000 	Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a + (a * b);
	if (ans != 2.453692200000000) {
	    if (ans < (2.453692200000000 - DPMARGIN) ||
				ans > (2.453692200000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a + (a * b)\nExpected: 2.453692200000000  	Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a - (a * b);
	if (ans != 0.015307800000000) {
	    if (ans < (0.015307800000000 - DPMARGIN) || 
				ans > (0.015307800000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a - (a * b)\nExpected: 0.015307800000000    	Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a + (a / b);
	if (ans != 2.484500000000000) {
	    if (ans < (2.484500000000000 - DPMARGIN) || 
				ans > (2.484500000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a + (a / b)\nExpected: 2.484500000000000   	 Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a - (a / b);
	if (ans != -(0.015499999999999)) {
	    if (ans < (-(0.015499999999999) - DPMARGIN) || 
				ans > (-(0.015499999999999) + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a - (a / b)\nExpected: -0.015499999999999   	 Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a * (a + b);
	if (ans != 2.743182449999999) {
	    if (ans < (2.743182449999999 - DPMARGIN) || 
				ans > (2.743182449999999 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a * (a + b)\nExpected: 2.743182449999999   	 Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a * (a - b);
	if (ans != 0.304798049999999) {
	    if (ans < (0.304798049999999 - DPMARGIN) || 
				ans > (0.304798049999999 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a * (a - b)\nExpected: 0.304798049999999   	 Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a / (a + b);
	if (ans != 0.555555555555555) {
	    if (ans < (0.555555555555555 - DPMARGIN) || 
				ans > (0.555555555555555 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a / (a + b)\nExpected: 0.555555555555555      Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a / (a - b);
	if (ans != 5.000000000000002) {
	    if (ans < (5.000000000000002 - DPMARGIN) || 
				ans > (5.000000000000002 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a / (a - b)\nExpected: 5.000000000000002   	 Actual: %1.15f", ans);
		return (1);
	    }
	}
	ans = a * (a / b);
	if (ans != 1.543124999999999) {
	    if (ans < (1.543124999999999 - DPMARGIN) || 
				ans > (1.543124999999999 + DPMARGIN)) {
		send_message(0, ERROR, "Error:  a * (a / b)\nExpected: 1.543124999999999   	 Actual: %1.15f)", ans);
		return (1);
	    }
	}
	ans = a / (a * b);
	if (ans != 1.012555690562980) {
	    if (ans < (1.012555690562980 - DPMARGIN) || 
				ans > (1.012555690562980 + DPMARGIN)) {
		send_message(0, ERROR, "Error:   a / (a * b)\nExpected: 1.0125555690562980	 Actual: %1.15f)", ans);
		return (1);
	    }
	}
	/* Start Double Precision test of trg functions */

	/* sin of values in the range of -2pi to +2pi   */
	result = sin(-(pi * 2));
	if (result != -(0.000000000820413)) {
	    if (result < (-(0.000000000820413) - DPMARGIN) || 
				result > (-(0.000000000820413) + DPMARGIN)) {
		send_message(0, ERROR, "Error:  sin(-2pi)\nExpected: -0.000000000820413 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = sin((pi * (-3)) / 2);
	if (result != 1.0000000000000000) {
	    if (result < (1.0000000000000000 - DPMARGIN) || 
				result > (-0.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(-3pi/2)\nExpected: 1.0000000000000000 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = sin(-(pi));
	if (result != 0.000000000410206) {
	    if (result < (0.000000000410206 - DPMARGIN) || 
				result > (0.00000000410206 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(-pi)\nExpected: 0.000000000410206 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = sin(-(pi / 2));
	if (result != -(1.0000000000000000)) {
	    if (result < (-(1.0000000000000000) - DPMARGIN) || 
				result > (-(1.0000000000000000) + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(-pi/2)\nExpected: -1.0000000000000000 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = sin(0);
	if (result != (0.0000000000000000)) {
	    if (result < (0.0000000000000000 - DPMARGIN) || 
				result > (0.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(0)\nExpected: 0.0000000000000000       Actual: %1.15f", result);
		return (1);
	    }
	}
	result = sin(pi / 2);
	if (result != 1.0000000000000000) {
	    if (result < (1.0000000000000000 - DPMARGIN) || 
				result > (1.0000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(pi/2)\nExpected: 1.0000000000000000       Actual: %1.15f", result);
		return (1);
	    }
	}
	result = sin(pi);
	if (result != -(0.000000000410206)) {
	    if (result < (-(0.000000000410206) - DPMARGIN) || 
				result > (-(0.000000000410206) + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(pi)\nExpected: -0.000000000410206       Actual: %1.15f", result);
		return (1);
	    }
	}
	result = sin((pi * 3) / 2);
	if (result != -(1.0000000000000000)) {
	    if (result < (-(1.0000000000000000) - DPMARGIN) || 
				result > (-(1.0000000000000000) + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(3pi/2)\nExpected: -1.0000000000000000 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = sin(pi * 2);
	if (result != 0.000000000820143) {
	    if (result < (0.000000000820143 - DPMARGIN) || 
				result > (0.00000000820143 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(2pi)\nExpected: 0.000000000820143 Actual: %1.15f", result);
		return (1);
	    }
	}
	/* cos of values in the range of -2pi to +2pi   */
	result = cos(pi * (-2));
	if (result != 1.0000000000000000) {
	    if (result < (1.0000000000000000 - DPMARGIN) || 
				result > (1.0000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(-2pi)\nExpected: 1.0000000000000000       Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos((pi * (-3)) / 2);
	if (result != 0.000000000615310) {
	    if (result < (0.000000000615310 - DPMARGIN) || 
				result > (0.00000000615310 + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(-3pi/2)\nExpected: 0.000000000615310 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos(-pi);
	if (result != -(1.0000000000000000)) {
	    if (result < (-(1.0000000000000000) - DPMARGIN) || 
				result > (-(1.0000000000000000) + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(-pi)\nExpected: -1.0000000000000000 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos(-(pi / 2));
	if (result != -(0.000000000205103)) {
	    if (result < (-(0.000000000205103) - DPMARGIN) || 
				result > (-(0.000000000205103) + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(-pi/2)\nExpected: -0.000000000205103 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos(0);
	if (result != 1.0000000000000000) {
	    if (result < (1.0000000000000000 - DPMARGIN) || 
				result > (1.0000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(0)\nExpected: 1.0000000000000000       Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos(pi / 2);
	if (result != (-0.000000000205103)) {
	    if (result < (-(0.000000000205103) - DPMARGIN) || 
				result > (-(0.000000000205103) + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(pi/2)\nExpected: -0.000000000205103 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos(pi);
	if (result != (-1.0000000000000000)) {
	    if (result < (-(1.0000000000000000) - DPMARGIN) || 
				result > (-(1.0000000000000000) + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(pi)\nExpected: -1.0000000000000000 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos((pi * 3) / 2);
	if (result != (0.000000000615310)) {
	    if (result < (0.000000000615310 - DPMARGIN) || 
				result > (0.00000000615310 + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(3pi/2)\nExpected: 0.000000000615310 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos(pi * 2);
	if (result != 1.0000000000000000) {
	    if (result < (1.0000000000000000 - DPMARGIN) || 
				result > (1.0000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(pi/2)\nExpected: 1.0000000000000000       Actual: %1.15f", result);
		return (1);
	    }
	}
	/* sin and cos of: pi/4, 3pi/4, 5pi/4 and 7pi/4  */
	result = sin(pi / 4);
	if (result != (0.707106781259062)) {
	    if (result < (0.707106781259062 - DPMARGIN) || 
				result > (0.707106781259062 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(pi/4)\nExpected: 0.707106781259062 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = sin((pi * 3) / 4);
	if (result != 0.707106780969002) {
	    if (result < (0.707106780969002 - DPMARGIN) || 
				result > (0.707106780969002 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(3pi/4)\nExpected: 0.707106780969002 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = sin((pi * 5) / 4);
	if (result != -(0.707106781549122)) {
	    if (result < (-(0.707106781549122) - DPMARGIN) || 
				result > (-(0.707106781549122) + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(5pi/4)\nExpected: -0.707106781549122 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = sin((pi * 7) / 4);
	if (result != -(0.707106780678942)) {
	    if (result < (-(0.707106780678942) - DPMARGIN) || 
				result > (-(0.707106780678942) + DPMARGIN)) {
		send_message(0, ERROR, "Error: sin(7pi/4)\nExpected: -0.707106780678942 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos(pi / 4);
	if (result != 0.707106781114032) {
	    if (result < (0.707106781114032 - DPMARGIN) || 
				result > (0.707106781114032 + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(pi/4)\n Expected: 0.707106781114032 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos((pi * 3) / 4);
	if (result != -(0.707106781404092)) {
	    if (result < (-(0.707106781404092) - DPMARGIN) || 
				result > (-(0.707106781404092) + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(3pi/4)\n Expected: -0.707106781404092 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos((pi * 5) / 4);
	if (result != -(0.707106780823972)) {
	    if (result < (-(0.707106780823972) - DPMARGIN) || 
				result > (-(0.707106780823972) + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(5pi/4)\n Expected: -0.707106780823972 Actual: %1.15f", result);
		return (1);
	    }
	}
	result = cos((pi * 7) / 4);
	if (result != (0.707106781694152)) {
	    if (result < (0.707106781694152 - DPMARGIN) || 
				result > (0.707106781694152 + DPMARGIN)) {
		send_message(0, ERROR, "Error: cos(7pi/4)\n Expected: 0.707106781694152 Actual: %1.15f", result);
		return (1);
	    }
	}
	/* exponential		 */
	x = exp(0.0);
	if (x != 1.0000000000000000) {
	    if (x < (1.0000000000000000 - DPMARGIN) || 
				x > (1.0000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: exp(0)\n Expected: 1.0000000000000000 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = exp(1.0);
	if (x != 2.718281828459045) {
	    if (x < (2.718281828459045 - DPMARGIN) || 
				x > (2.718281828459045 + DPMARGIN)) {
		send_message(0, ERROR, "Error: exp(1)\n Expected: 2.718281828459045 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = exp(2.0);
	if (x != 7.389056098930650) {
	    if (x < (7.389056098930650 - DPMARGIN) || 
				x > (7.389056098930650 + DPMARGIN)) {
		send_message(0, ERROR, "Error: exp(2)\n Expected: 7.389056098930650 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = exp(5.0);
	if (x != 148.413159102576600) {
	    if (x < (148.413159102576600 - DPMARGIN) || 
				x > (148.413159102576600 + DPMARGIN)) {
		send_message(0, ERROR, "Error: exp(5)\n Expected: 148.413159102576600 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = exp(10.0);
	if (x != 22026.465794806718000) {
	    if (x < (22026.465794806718000 - DPMARGIN) || 
				x > (22026.465794806718000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: exp(10)\n Expected: 22026.465794806718000 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = exp(-1.0);
	if (x != 0.367879441171442) {
	    if (x < (0.367879441171442 - DPMARGIN) || 
				x > (0.367879441171442 + DPMARGIN)) {
		send_message(0, ERROR, "Error: exp(-1)\n Expected: 0.367879441171442 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = exp(-2.0);
	if (x != 0.135335283236612) {
	    if (x < (0.135335283236612 - DPMARGIN) || 
				x > (0.135335283236612 + DPMARGIN)) {
		send_message(0, ERROR, "Error: exp(-2)\nExpected: 0.135335283236612 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = exp(-5.0);
	if (x != 0.006737946999085) {
	    if (x < (0.006737946999085 - DPMARGIN) || 
				x > (0.006737946999085 + DPMARGIN)) {
		send_message(0, ERROR, "Error: exp(-5)\nExpected: 0.006737946999085 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = exp(-10.0);
	if (x != 0.000045399929762) {
	    if (x < (0.000045399929762 - DPMARGIN) || 
				x > (0.000045399929762 + DPMARGIN)) {
		send_message(0, ERROR, "Error: exp(-10)\nExpected: 0.000045399929762 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = exp(log(1.0));
	if (x != 1.0000000000000000) {
	    if (x < (1.0000000000000000 - DPMARGIN) || 
				x > (1.0000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: exp(log(1)\nExpected: 1.0000000000000000 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = exp(log(10.0));
	if (x != 10.000000000000002) {
	    if (x < (10.000000000000002 - DPMARGIN) || 
				x > (10.000000000000002 + DPMARGIN)) {
		send_message(0, ERROR, "Error: exp(log(10)\nExpected 10.000000000000002 Actual: %1.15f", x);
		return (1);
	    }
	}
	/* logarithms            */
	x = log(1.0);
	if (x != 0.0000000000000000) {
	    if (x < (0.0000000000000000 - DPMARGIN) || 
				x > (0.0000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: log(1)\nExpected: 0.0000000000000000 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = log(2.0);
	if (x != 0.693147180559945) {
	    if (x < (0.693147180559945 - DPMARGIN) || 
				x > (0.693147180559945 + DPMARGIN)) {
		send_message(0, ERROR, "Error: log(2)\nExpected: 0.693147180559945 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = log(10.0);
	if (x != 2.302585092994045) {
	    if (x < (2.302585092994045 - DPMARGIN) || 
				x > (2.302585092994045 + DPMARGIN)) {
		send_message(0, ERROR, "Error: log(10)\nExpected: 2.302585092994045 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = log(100.0);
	if (x != 4.605170185988091) {
	    if (x < (4.605170185988091 - DPMARGIN) || 
				x > (4.605170185988091 + DPMARGIN)) {
		send_message(0, ERROR, "Error: log(100)\nExpected: 4.605170185988091 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = log(exp(0.0));
	if (x != 0.0000000000000000) {
	    if (x < (0.0000000000000000 - DPMARGIN) || 
				x > (0.0000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: log(exp(0))\nExpected: 0.0000000000000000 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = log(exp(1.0));
	if (x != 1.0000000000000000) {
	    if (x < (1.0000000000000000 - DPMARGIN) || 
				x > (1.0000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: log(exp(1))\nExpected: 1.0000000000000000 Actual: %1.15f", x);
		return (1);
	    }
	}
	x = log(exp(10.0));
	if (x != 10.0000000000000000) {
	    if (x < (10.0000000000000000 - DPMARGIN) ||
				x > (10.0000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: log(exp(10))\nExpected: 10.0000000000000000 Actual: %1.15f", x);
		return (1);
	    }
	}
	/* These functions are supported by the 68881 but not the FPA  */

	x = tan(-(2 * pi));
	if (x != -(0.000000000820414));
	{
	    if (x < (-(0.000000000820414) - DPMARGIN) || 
				x > (-(0.000000000820414) + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(-2pi)\nExpected: -0.000000000820414  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan(-(7 * pi) / 4);
	if (x != 0.999999998564275);
	{
	    if (x < (0.999999998564275 - DPMARGIN) || 
				x > (0.999999998564275 + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(-7pi/4)\nExpected: 0.999999998564275  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan(-(5 * pi) / 4);
	if (x != -(1.000000001025517));
	{
	    if (x < (-(1.000000001025517) - DPMARGIN) || 
				x > (-(1.000000001025517) + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(-5pi/4)\nExpected: -1.000000001025517  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan(-(pi));
	if (x != -(0.000000000410207));
	{
	    if (x < (-(0.000000000410207) - DPMARGIN) || 
				x > (-(0.000000000410207) + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(-pi\nExpected: 0.000000000410207  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan(-(3 * pi) / 4);
	if (x != 0.999999999384690);
	{
	    if (x < (0.999999999384690 - DPMARGIN) || 
				x > (0.999999999384690 + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(-3pi/4)\nExpected: 0.999999999384690  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan(-(pi) / 4);
	if (x != -(1.000000000205103));
	{
	    if (x < (-(1.000000000205103) - DPMARGIN) || 
				x > (-(1.000000000205103) + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(-pi/4)\nExpected: -1.000000000205103  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan(0.0);
	if (x != 0.000000000000000);
	{
	    if (x < (0.000000000000000 - DPMARGIN) || 
				x > (0.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(0.0)\nExpected: 0.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan(pi / 4);
	if (x != 1.000000000205103);
	{
	    if (x < (1.000000000205103 - DPMARGIN) || 
				x > (1.000000000205103 + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(pi / 4)\nExpected: 1.000000000205103  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan((3 * pi) / 4);
	if (x != -0.999999999384690);
	{
	    if (x < (-(0.999999999384690) - DPMARGIN) || 
				x > (-(0.999999999384690) + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(3pi/4)\nExpected: -0.999999999384690  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan(pi);
	if (x != 0.000000000410207);
	{
	    if (x < (0.000000000410207 - DPMARGIN) || 
				x > (0.000000000410207 + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(pi)\nExpected: 0.000000000410207  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan((5 * pi) / 4);
	if (x != 1.000000001025517);
	{
	    if (x < (1.000000001025517 - DPMARGIN) || 
				x > (1.000000001025517 + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(5pi/4)\nExpected: 1.000000001025517  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan((7 * pi) / 4);
	if (x != -0.999999998564275);
	{
	    if (x < (-(0.999999998564275) - DPMARGIN) || 
				x > (-(0.999999998564275) + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(7pi/4)\nExpected: -0.999999998564275  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = tan((2 * pi));
	if (x != 0.000000000820414);
	{
	    if (x < (0.000000000820414 - DPMARGIN) || 
				x > (0.000000000820414 + DPMARGIN)) {
		send_message(0, ERROR, "Error: tan(2pi)\nExpected: 0.000000000820414  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = sqrt(0.0);
	if (x != 0.000000000000000) {
	    if (x < (0.000000000000000 - DPMARGIN) || 
				x > (0.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sqrt(0)\nExpected: 0.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = sqrt(1.0);
	if (x != 1.000000000000000) {
	    if (x < (1.000000000000000 - DPMARGIN) || 
				x > (1.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sqrt(1)\nExpected: 1.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = sqrt(4.0);
	if (x != 2.000000000000000) {
	    if (x < (2.000000000000000 - DPMARGIN) || 
				x > (2.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sqrt(4)\nExpected: 2.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = sqrt(9.0);
	if (x != 3.000000000000000) {
	    if (x < (3.000000000000000 - DPMARGIN) || 
				x > (3.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sqrt(9)\nExpected: 3.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = sqrt(16.0);
	if (x != 4.000000000000000) {
	    if (x < (4.000000000000000 - DPMARGIN) || 
				x > (4.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sqrt(16)\nExpected: 4.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = sqrt(25.0);
	if (x != 5.000000000000000) {
	    if (x < (5.000000000000000 - DPMARGIN) || 
				x > (5.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sqrt(25)\nExpected: 5.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = sqrt(36.0);
	if (x != 6.000000000000000) {
	    if (x < (6.000000000000000 - DPMARGIN) || 
				x > (6.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sqrt(36)\nExpected: 6.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = sqrt(49.0);
	if (x != 7.000000000000000) {
	    if (x < (7.000000000000000 - DPMARGIN) || 
				x > (7.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sqrt(49)\nExpected: 7.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = sqrt(64.0);
	if (x != 8.000000000000000) {
	    if (x < (8.000000000000000 - DPMARGIN) || 
				x > (8.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sqrt(64)\nExpected: 8.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = sqrt(81.0);
	if (x != 9.000000000000000) {
	    if (x < (9.000000000000000 - DPMARGIN) || 
				x > (9.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sqrt(81)\nExpected: 9.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	x = sqrt(100.0);
	if (x != 10.000000000000000) {
	    if (x < (10.000000000000000 - DPMARGIN) || 
				x > (10.000000000000000 + DPMARGIN)) {
		send_message(0, ERROR, "Error: sqrt(100)\nExpected: 10.000000000000000  Actual: %1.15f", x);
		return (1);
	    }
	}
	if (quick_test)
	    break;
	if (!quick_test && (dpfpa % 25) == 0)
	    sleep(1);
    }					       /* end of for loop */

    return (0);
}

float_exp()
{
    send_message(-FP_EXCEPTION_ERROR, FATAL, 
		"Floating point exception interrupt.");
}

clean_up()
{
}


#ifdef sun3
probe881()
{
    int             val;
    int             eeopen;
    int             pointer;
    int             seek_ok;
    int             nbytes;
    long            offset;
    unsigned char   ee_buffer[8];
    char            buf;
    int             i;

    offset = 0x0000BC;
    pointer = L_SET;
    eeopen = open("/dev/eeprom", O_RDONLY);
    send_message(0, DEBUG, "eeopen = %d", eeopen);
    if (eeopen < 0) {
	send_message(1, ERROR, "Cannot open /dev/eeprom ");
    }
    send_message(0, DEBUG, "passed open");
    for (i = 0; i < 14; i++) {
	seek_ok = lseek(eeopen, offset, pointer);
	send_message(0, DEBUG, "seek_ok = %x", seek_ok);
	if (seek_ok == -1) {
	    send_message(1, ERROR, "lseek failed");
	    exit(0);
	}
	send_message(0, DEBUG, "passed lseek");
	nbytes = 8;
	val = read(eeopen, ee_buffer, nbytes);
	send_message(0, DEBUG, "val = %d", val);
	if (val == -1) {
	    send_message(1, ERROR, "read failed");
	    exit(0);
	}
	send_message(0, DEBUG, "\
		ee_buffer[0] = %x\n\
		ee_buffer[1] = %x\n\
		ee_buffer[2] = %x\n\
		ee_buffer[3] = %x\n\
		ee_buffer[4] = %x\n\
		ee_buffer[5] = %x\n\
		ee_buffer[6] = %x\n\
		ee_buffer[7] = %x\n\
		passed read.", 
		ee_buffer[0], ee_buffer[1], ee_buffer[2],
		ee_buffer[3], ee_buffer[4], ee_buffer[5], 
		ee_buffer[6], ee_buffer[7]);
	if (ee_buffer[0] == 0xFF) {
	    send_message(0, CONSOLE, 
		"Could not find configuration for MC68881 in EEPROM\n");
	    return (-1);
	}
	if (ee_buffer[0] == 0x01) {
	    buf = ee_buffer[2];
   	    send_message(0, DEBUG, "buf = %x", buf);
	    buf = buf & 0x01;
            send_message(0, DEBUG, "buf = %x", buf);
	    if (buf == 1)
		return (1);
	    return (0);
	}
	offset = 0; 
	pointer = L_INCR;
	if (i == 13) {
	    send_message(1, ERROR, " System configuration of EEPROM is wrong.\n Expected to find FF which represents the end of the configuration.");
	    return (-1);
	}
    }
}
#endif


int process_fpatest_args(argv, arrcount)
char    *argv[];
int     arrcount;
{
   if (argv[arrcount][0] == 'e') {
       simulate_error = atoi(&argv[arrcount][1]);
       if (simulate_error > 0 && simulate_error < 8)
          return (TRUE);
   }
   else
          return (FALSE);
   return (TRUE);
}
