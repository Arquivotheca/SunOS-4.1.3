/*
 *
 *static char     frefsccsid[] = "@(#)fpu_probe.c 1.1 7/30/92 Copyright Sun Microsystems";
 */
/*
 * File: 	fpu_probe.c				Date:	4/22/88
 *
 * Function:	To allow the FPU diagnostics to detect the presence and
 *		type of Floating Point Co-processor.
 *
 * Routines:	fpu_probe, and fpu_type
 */

#ifndef SVR4
#include <machine/cpu.h>
#endif SVR4

#ifndef CPU_SUN4_460
#define CPU_SUN4_460	0x24
#endif

/* 
 * external functions required
 */

extern	int get_fsr();
extern	int check_fpu();

/*
 *  Name:  	fpu_probe
 *  Function:	To detect the presence of a Floating Point Co-processor
 *		and report it to the application level.
 *  Returns:	TRUE (0) if installed or FALSE (1) if not
 *  Calling:	if (fpu_probe())
 */

fpu_probe()
{
	int	present;
	if (check_fpu() == 0)
		return(0);		/* advise user that there is one */
	return(1); 			/* Advise user no fpu installed  */
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
 *  Co-Processor Types :: this needs to move to a global def file.
 */

#define	WEITEK		0		/* Weitek family		*/
#define	TI8847		10		/* TI family 		8847	*/
#define MC68881		20		/* Motorola family 	68881	*/
#define FABS		30		/* FABS 			*/
#define SUNRAY_FPC	40		/* Sunray - uses the TI8847	*/
#define INVALID		-1		/* Invalid device code		*/
#define	WTL3170		11		/* Weitek WTL3170/2		*/
#define MEIKO		100		/* Meiko LSIL L64804		*/
#define NONE		111		/* No FPU installed		*/

#define CPU_TYPE_SHIFT	24		/* bits to shift to get the cpu type */
#define CPU_TYPE_MASK	0xff		/* 1 byte indicates cpu type	*/

/*
 *  History: Modified fpu_type to reflect the new FSR_version values
 *  that came into effect through June 1990. In particular the bit 19
 *  is now being used to encode the FPU type for the Meiko FPU.
 *
 *
 *  The FPU type is encoded in the status register bits 17, 18 and 19.
 *  The meaning is shown below.
 *
 *       31                16                   0
 *        |                 |                   |
 *   FSR [....|....|....|xxx.|....|....|....|....]
 *                        00 = Weitek
 *                        01 = Weitek + fab
 *                        10 = TI8847
 *                        11 = Weitek WTL3170/2
 *                       100 = Meiko FPU
 *			 111 = No FPU attached to IU.
 *
 *  Note:  The Sunray cpu uses the Cypress chip set and the bits 17-18 in
 *  the FSR indicate only the Rev level and not the FPU type.
 */
fpu_type()
{
	unsigned long	status_reg;
	int	device;
	int	cputype = 0;

	device = INVALID;
	if (fpu_probe() == 0) {			/* see if any present	*/
	    cputype = (gethostid() >> CPU_TYPE_SHIFT) & CPU_TYPE_MASK;
	    if (cputype == CPU_SUN4_460)
	        device = SUNRAY_FPC;
	    else {
	        status_reg = get_fsr();		/* get FPU Status reg 	*/
	   	status_reg = (status_reg >> 17);/* set up bits 17,18&19	*/
	        switch(status_reg & 0x7) {	/* isolate valid bits 	*/
		    case 0 : device = WEITEK;	/* Weitek 1164/65 */
			     break;	/* FUJITSU MB 86910 (1-4) */
		    case 1 : device = WEITEK;	/* Weitek 1164/65 */
			     break;	/* FUJITSU MB 86910 (1-4) */
		    case 2 : device = TI8847;	/* TI ACT8847 */
			     break;	/* LSIL L64802 */
		    case 3 : device = WTL3170;	/* Weitek WTL3170/2 */
			     break;
		    case 4 : device = MEIKO;	/* LSIL L64804 */
			     break;
		    case 7 : device = NONE;	/* No FPU attached */
			     break;
		    /************************
		     * Add new devices here *
		     ************************/
		    default: device = INVALID;	/* return invalid code	*/
	        }
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
 *  if ( ( fdtoi(4026531840) != 0x7fffffff ) || ( No_Overflow_Trap ) )
 *	then FPU bad
 *	else FPU ok
 *
 *  Bad FPU will return 0xF0000000 (or 0xFFFFFFFF)
 *
 */

fpu_badti()
{
   unsigned long result;

	result = float_int_d(4026531840.0);
	if (result == 0x7FFFFFFF) {
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

fpu_is_wtl3170()
{
	if (fpu_type() == WTL3170) {
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

fpu_is_meiko()
{
	if (fpu_type() == MEIKO) {
		return(1);
	}
	return(0);
}

fpu_is_none()
{
	if (fpu_type() == NONE) {
		return(1);
	}
	return(0);
}

fpu_is_sunray()
{
	if (fpu_type() == SUNRAY_FPC)
		return(1);
	return(0);
}
