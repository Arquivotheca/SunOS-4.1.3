#ifndef lint
static	char sccsid[] = "@(#)trace.c 1.1 7/30/92 Copyright Sun Microsystems";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/******************************************************************************

  trace.c

  Author:	Anantha N. Srirama		anantha@ravi

  This file contains the generic trace routine which helps in debugging/tracing
  a test (in this case nettest).

******************************************************************************/
#include	<sdrtns.h>
#include	"trace.h"	/* should be removed when added to sdrtns.h */


/******************************************************************************

  trace

  This function takes two arguments: one flag and another pointer to a string.
  If the flag matches with the predefined constant 'ENTER' then the message
  "Entering" is prepended to the string and the function 'send_message' is
  called with the DEBUG as the target (should change to TRACE when this option
  is recognized by send_message).  Otherwise the message "Exiting" is prepended

******************************************************************************/
void	trace (flag, string)
int	flag;
char*	string;
{
    if (debug) {
	if (flag == ENTER) {
	    send_message (0, DEBUG, "Entering %s\n", string);
	}
	else {
	    send_message (0, DEBUG, "Exiting %s\n", string);
	}
    }
}


