/*      @(#)sdutil.c	1.1 7/30/92 Copyright Sun Micro     */
#include <stdio.h>
#ifndef SVR4
#include <mon/idprom.h>
#endif
#include "sdrtns.h"

/****************************************************************************
 NAME: trace_before(), trace_after() -  Trace where you are as test in progress.
	Use trace_before() prior to entering one place, trace_after right
	after leaving that place.
 SYNOPSIS:
        void trace_before(string)
	void trace_after(string)

 ARGUMENTS:
	input:  char *string - name of the place.	 
 
 DESCRIPTION:
	When trace_before(string) is called,  the following message will 
	display at on-line mode:
	  "@test_name device_name: L# func_name (before) string".
	For trace_after(string):
	  "@test_name device_name: L# func_name (after) string"; 

	where, L# indicates the scope level of function func_name relative
	to main (L0).  The string should be the place to enter or leave.

	At Sundaig mode, "@test_name device_name" is replaced by a standard
	formatted prepending message.
 
 MACROS:
	MAX_LEVEL - the maximun scope level depth that this trace allows.
	FUNC_NAME_LENGTH - the length limit of function name (func_name).

 NOTE:
	There is two macros pertaining to trace_before and trace_after:
	   TRACE_IN  --  defined in sdrtns.h as
			 (void)trace_before((char *)NULL)
	   TRACE_OUT --  (void)trace_after((char *)NULL)
	They are used as follows:
		function()
		{
			func_name = "function";
		      	TRACE_IN
			...
			...
			TRACE_OUT
			return(0)
		} 
	At on-line mode, TRACE_IN will display:
	"@test_name device_name: L# func_name(Enter) string".
        At Sundaig mode, "@test_name device_name" is replaced by a standard 
        formatted prepending message. 
 
 RETURN VALUE:  void
*****************************************************************************/
#define MAX_LEVEL	  40
#define FUNC_NAME_LENGTH  80
static char func_name_buf[MAX_LEVEL+1][FUNC_NAME_LENGTH];

void trace_before(string)
char *string;
{
    extern int  trace_level;
    char  *control_str;

    if( (string== (char *)NULL  || string[0]==NULL) &&
		trace_level < MAX_LEVEL) {
 	strcpy (func_name_buf[++trace_level], func_name);
	send_message (0, TRACE, "%s%d %s(Enter)", "L", trace_level,
		func_name);
    }
    else
    	send_message (0, TRACE, "%s%d %s (before) %s", "L", trace_level, 
		func_name, string);
}

void trace_after(string)
char *string;
{
    extern int trace_level;

    if( (string== (char *)NULL  || string[0]==NULL) && 
	  trace_level >0 ) {   
        send_message (0, TRACE, "%s%d %s(Leave)", "L", trace_level,
		func_name_buf[trace_level]);
	trace_level -= 1;
	func_name = "unknown_function";
    }
    else
        send_message (0, TRACE, "%s%d %s (after) %s", "L", trace_level, 
		func_name, string);
} 

