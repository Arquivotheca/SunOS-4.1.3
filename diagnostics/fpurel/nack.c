
/*
 *"@(#)nack.c 1.1 7/30/92  Copyright Sun Microsystems";
 */

/*
 *	Nack test : In this test the program will create all
 *		    possible floating point traps and
 *		    checks whether it occured or not.
 */

extern  unsigned long	trap_flag;
extern  unsigned long   fsr_data;
extern  unsigned long   fsr_at_trap;
extern  unsigned long	donot_dq;
extern  unsigned long	error_ok;
extern  int verbose_mode;

fpu_nack_test()
{
	unsigned long fsr_status;

	error_ok = 1;

	fsr_status = get_fsr();
	fsr_status = fsr_status | 0xF800000; /* set the TEM bits */
	set_fsr(fsr_status);
	donot_dq = 0x0;	
	trap_flag = 0x0; 	/* unset the trap flag */
	/* Test that there should not be any exception */	

	wadd_sp(0x3f800000, 0x3f800000);
	if (trap_flag) {
		if (verbose_mode)
		printf("\nError: Bus error occured. ftt = %x\n", (fsr_at_trap & 0x1c000) >> 14);
			return(-1);
	}

	/* now test the IEEE exception */
	trap_flag = 0x0;        /* unset the trap flag */      
	wdiv_sp(0x3f800000, 0x00000);
	if (!trap_flag) {      
             if (verbose_mode)
		printf("\nError: Bus error did not occur(IEEE exception). ftt = %x\n", (fsr_at_trap & 0x1C000) >> 14);
	     	return(-1);
        }
/*
 *
 *  Second level:  Confirm that the ftt bits in the FSR are set correctly
 *                 *** Note this is not valid with SunOs >= 4.0 because
 *                 the OS clears the FSR on an exception.
 */
/*DISABLED - BDS
 *
 *      if ((fsr_at_trap & 0x1C000) != 0x4000) {
 *              printf("\nError: ftt bits expected / observed = 1 / %x",
 *                              (fsr_at_trap & 0x0001C000) >> 14);
 *              ftt_decode(fsr_at_trap);
 *              return(-1);
 *      }
 */
	/* test for unfinished exception */
	trap_flag = 0x0;
	wadd_sp(0x7fbfffff, 0x0);

	if (!trap_flag) {
		if (verbose_mode)
             printf("\nError: Bus error did not occur(Unfinished exception). ftt = %x\n",
			(fsr_at_trap & 0x1C000) >> 14);
             	return(-1);
        }
	wadd_sp(1,1);			/* clear the exception error */
/*
 *  Second level:  Confirm that the ftt bits in the FSR are set correctly
 *                 *** Note this is not valid with SunOs >= 4.0 because
 *                 the OS clears the FSR on an exception.
 */
/*DISABLED - BDS
 *      if ((fsr_at_trap & 0x1c000) != 0x8000) {
 *              printf("\nError: ftt bits expected / observed = 2 / %x\n",
 *                               (fsr_at_trap & 0x0001c000) >> 14);
 *              ftt_decode(fsr_at_trap);
 *              return(-1);
 *       }
 */

	wadd_sp(1,1);			/* clear the exception error */
	/* now test for the unimplemented exception */
	set_fsr(0x0);
	error_ok = 0;
	return(0);
}	
