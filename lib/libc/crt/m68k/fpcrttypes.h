/*      @(#)fpcrttypes.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *      Sun Floating Point Definitions for libc/crtlib 
 */

/*			C TYPES			*/

typedef enum fp_switch_type {	
	fp_unspecified,	/* Floating point unit not specified yet. */
	fp_software,	/* Floating point performed in software. */
	fp_skyffp,	/* Floating point performed on Sky FFP. */
	fp_mc68881,	/* Floating point performed on Motorola 68881. */
	fp_sunfpa	/* Floating point performed on Sun FPA. */
	} ;	

typedef enum fp_state_type {
	fp_unknown,	/* We don't know status yet. */
	fp_absent,	/* Not on system or not available to this user. */
	fp_enabled	/* Available for use by this user and initialized. */
	} ;

/*			C VARIABLES		*/

extern enum fp_switch_type 	/* Current floating point variety. */
	fp_switch; 

extern enum fp_state_type	/* State of various floating point units. */
	fp_state_software,	/* Floating point software. */
	fp_state_skyffp,	/* Sky FFP. */
	fp_state_mc68881,	/* Motorola 68881. */	
	fp_state_sunfpa;	/* Sun FPA. */

extern char *			/* 0 if fp_state_skyffp != fp_enabled */ 
	_skybase; 		/* sky access address 
				     if fp_state_skyffp == fp_enabled */

extern	int
	Fmode,			/* Software floating point modes. */
	Fstatus;		/* Software floating point status. */

