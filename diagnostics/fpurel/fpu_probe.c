/*
 *
 *static char     frefsccsid[] = "@(#)fpu_probe.c 1.1 7/30/92  Copyright Sun Microsystems";
 */

#include "fpu.h"

/*
 * File: 	fpu_probe.c				Date:	4/22/88
 *
 * Function:	To allow the FPU diagnostics to detect the presence and
 *		type of Floating Point Co-processor.
 *
 * Routines:	fpu_probe, and fpu_type
 *
 * History:	5/11/88 - Orig coding (BDS)
 *		6/27/88 - Updated fpu_bad_ti for New Improved TI
 */

/* 
 * external functions required
 */

extern	int get_fsr();
extern	int check_fpu();
extern	int fpu_present;			/* Global T/F flag	*/
extern  unsigned long error_ok;			/* Trap variables	*/
extern  unsigned long	trap_flag;		/* Trap variables	*/

/*
 *  Name:  	fpu_probe
 *  Function:	To detect the presence of a Floating Point Co-processor
 *		and report it to the application level.
 *  Returns:	TRUE (0) if installed or FALSE (1) if not
 *  Calling:	if (fpu_probe())
 */

fpu_probe()
{
	if (fpu_present > 1) {		/* .. if unknown then read 	*/
	    if (check_fpu() == 0) {	/* .... from kmem		*/
		fpu_present = 0;	/* advise user that there is one */
	    }
	   fpu_present = 1; 		/* Advise user no fpu installed  */
	}
	return(fpu_present);		/* return the flag to caller	*/
}

/**/
/*
 * Name:	fpu_type
 * Function:	Identifies the type of Floating Point Co-processor installed
 *		in the system.
 * Returns:	See table below
 * Convention:	if (fpu_type() == WEITEK)
 *
 */

/*
 *  The FPU type is encoded in the status register bits 17 and 18.  The
 *  meaning is shown below.
 *
 *       31                16                   0
 *        |                 |                   |
 *   FSR [....|....|....|.xx.|....|....|....|....]
 *                        00 = Weitek
 *                        01 = FAB6
 *                        10 = TI8847
 *                        11 = Invalid
 */
fpu_type()
{
unsigned long	status_reg;
int	device;
	device = INVALID;
	if (fpu_probe() == 0) {			/* see if any present	*/
	   status_reg = get_fsr();		/* get FPU Status reg 	*/
	   status_reg = (status_reg >> 17);	/* set up bits 17&18	*/
	   switch(status_reg & 0x3) { 		/* isolate valid bits 	*/
		case 0 : device = WEITEK;	/* return Weitek code	*/
			 break;
		case 1 : device = FAB6;		/* return FAB6 code	*/
			 break;
		case 2 : device = TI8847;	/* return TI8847 code	*/
			 break;
		/************************
		 * Add new devices here *
		 ************************/
		default: device = INVALID;	/* return invalid code	*/
	        }
	   }
	return(device);				/* Advise device type	*/
}

/***************************************************************************
 *  			Special situation test function
 ***************************************************************************
 *
 *  A problem has been identified with the TI8847 processor that can
 *  be detected by preforming the following step.  If the result is not
 *  equal to 0 then the device is the bad one.
 *  See Cassy Ng (ex 2248) for details.
 *
 *  4026531840.0 = 0xF0000000  (64 bits)
 *
 *  if ( ( fdtoi(4026531840) == 0x7fffffff ) && ( Overflow_Trap ) )
 *	then FPU ok
 *	else FPU bad (ie.. Old Type of TI with BUG)
 */

fpu_badti()
{
	unsigned long result;

	result = float_int_d(4026531840.0);
	if ( result == 0x7fffffff ) {
	    return(0);
	}
	return(1);
}

/*
 *	Simple FPU processor type calls
 *
 *	These two calls are designed for a routine that simply wants
 *	to do an if - then function
 *
 */

fpu_is_weitek()
{
	if (fpu_type() == WEITEK) {
		return(1);
	}
	return(0);
}

fpu_is_ti8847()
{
	if (fpu_type() == TI8847) {
		return(1);
	}
	return(0);
}

fpu_is_fab6()
{
	if (fpu_type() == FAB6) {
		return(1);
	}
	return(0);
}

fpu_is_invalid()
{
	if (fpu_type() == INVALID) {
		return(1);
	}
	return(0);
}
