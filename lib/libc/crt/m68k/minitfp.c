#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)minitfp.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <signal.h>
#include <stdio.h>
#include "fpcrttypes.h"

#ifdef EMULATOR
extern Emulator() ;
#endif

int EMTSignalled ;

void EMTSpecial (sig, code, scp)
int sig, code ;
struct sigcontext *scp ;

/*	Special EMT handler that sets EMTSignalled if invoked and 
	increments pc by 6 = length of instruction
		fmoveml	<zeros>,fpcr/fpsr
 */

{
EMTSignalled = 1 ;
scp->sc_pc += 6 ;
}

int minitfp_()

/*
 *	Procedure to determine if a physical 68881 is present and
 *	set 68881 status accordingly.  Also returns 0 if absent, 1 if present.
 */

{
struct sigvec new,old ;

if (fp_state_mc68881 != fp_unknown) 
	{
	if (fp_state_mc68881 == fp_enabled) fp_switch = fp_mc68881 ;
	return((fp_state_mc68881==fp_enabled) ? 1 : 0) ;
	}
new.sv_handler = EMTSpecial ;
new.sv_mask = 0 ;
new.sv_flags = 0 ;
sigvec( SIGEMT, &new, &old ) ;
EMTSignalled = 0 ;
Mdefault() ;
sigvec( SIGEMT, &old, &new ) ;
if (EMTSignalled) fp_state_mc68881 = fp_absent ; else fp_state_mc68881 = fp_enabled ;
if (fp_state_mc68881 == fp_enabled) 
	{
	fp_switch = fp_mc68881 ;
	if (!ma93n_())
       	 	{
        	fprintf(stderr,"\n Please note: MC68881 upgrade to A93N mask may be advisable. \n") ;
        	fprintf(stderr,"See Floating-Point Programmer's Guide.\n") ;
		fflush(stderr) ;
        	}
	Mdefault() ;	/* Restore default status. */
	}
return((fp_state_mc68881==fp_enabled) ? 1 : 0) ;
}
