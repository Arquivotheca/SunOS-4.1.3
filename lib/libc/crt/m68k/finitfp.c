#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)finitfp.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "fpcrtdefs.h"
#include "fpcrttypes.h"

finitfp_()
{
fp_switch = fp_software ;
fp_state_software = fp_enabled ;
Fmode = ROUNDTODOUBLE ; Fstatus = 0 ;
return(1) ;
}

/*
 * return fp_switch value for programs that want to do their own floating point
 */
int
getfptype()
{
	return ((int)fp_switch);
}
