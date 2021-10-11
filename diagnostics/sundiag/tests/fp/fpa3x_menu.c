/******************************************************************************
	@(#)fpa3x_menu.c - Rev 1.1 - 7/30/92
	Copyright (c) 1988 by Sun Microsystems, Inc.
	Deyoung Hong

	This file contains a debug menu which allows the execution
	of a specific FPA-3X test as well as all tests.
******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sundev/fpareg.h>
#include "fpa3x_def.h"
#include "fpa3x_msg.h"
#include "../../include/sdrtns.h"	/* make sure not from local directory*/


/* Function type declarations */
int ierr_test(), imask_test(), loadptr_test(), mode_test();
int wstatus_test(), datareg_test(), shadow_test(), nack_test();
int simpleins_test(), pointer_test(), lock_test(), jumpcond_test();
int tipath_test(), tiop_test(), tistatus_test(), timing_test();
char *gets();


/*
 * Debug menu prompts to test each individual command.
 */
runmenu()
{
	char cmd[MAXSTRING];
	u_int addr, val;

	verbose = TRUE;
	for (;;)
	{
		printmenu();
		(void) gets(cmd);
		(void) printf("\n");

		switch (*cmd)
		{
			case '1': runtest(); break;
			case '2': run_fpa(ierr_test); break;
			case '3': run_fpa(imask_test); break;
			case '4': run_fpa(loadptr_test); break;
			case '5': run_fpa(mode_test); break;
			case '6': run_fpa(wstatus_test); break;
			case '7': run_fpa(datareg_test); break;
			case '8': run_fpa(shadow_test); break;
			case '9': run_fpa(nack_test); break;
			case 'a': run_fpa(simpleins_test); break;
			case 'b': run_fpa(pointer_test); break;
			case 'c': run_fpa(lock_test); break;
			case 'd': run_fpa(jumpcond_test); break;
			case 'e': run_fpa(tipath_test); break;
			case 'f': run_fpa(tiop_test); break;
			case 'g': run_fpa(tistatus_test); break;
			case 'h': run_fpa(timing_test); break;
			case 'i':
				slinpack_test();
				dlinpack_test();
				break;
			case 'j':
				spmath_test();
				dpmath_test();
				dptrig_test();
				break;
			case 'r':
				(void) printf(">Enter address in hex: ");
				(void) gets(cmd);
				if (sscanf(cmd,"%x",&addr) != 1) break;
				addr |= FPA3X_BASE;
				if (read_fpa((u_int *)addr,&val,ON)) break;
				(void) printf("Read %08X = %08X\n",addr,val);
				break;
			case 'w':
				(void) printf(">Enter address and value in hex: ");
				(void) gets(cmd);
				if (sscanf(cmd,"%x%x",&addr,&val) != 2) break;
				addr |= FPA3X_BASE;
				if (write_fpa((u_int *)addr,val,ON)) break;
				(void) printf("Write %08X = %08X\n",addr,val);
				break;
			case 'o': (void) open_context(); break;
			case 'p': (void) close_context(); break;
			case 'q': exit(0);
		}

		(void) printf("\nPress <Return> to continue ...");
		(void) gets(cmd);
	}
}


/*
 * Function to print the debug menu.
 */
printmenu()
{
	(void) printf("\f*** FPA-3X Sundiag Debug Test Menu - 11/88 ***\n\n");
	(void) printf("   1) Default test\t9) Nack\t\t\th) TI timing\n");
	(void) printf("   2) IERR\t\ta) Simple instruction\ti) Linpack\n");
	(void) printf("   3) IMASK\t\tb) Pointer\t\tj) Math\n");
	(void) printf("   4) LOAD_PTR\t\tc) Lock\t\t\tr) Read FPA address\n");
	(void) printf("   5) MODE\t\td) Jump condition\tw) Write FPA address\n");
	(void) printf("   6) WSTATUS\t\te) TI data path\t\to) Open context\n");
	(void) printf("   7) Register file\tf) TI operation\t\tp) Close context\n");
	(void) printf("   8) Shadow\t\tg) TI status\t\tq) Quit\n");
	(void) printf("\nCommand ==> ");
}


