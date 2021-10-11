
#ifndef lint
static	char sccsid[] = "@(#)ieee_handler.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <floatingpoint.h>
#include "libm.h"

#define divbyz (1<<(int)fp_division)
#define unflow (1<<(int)fp_underflow)
#define ovflow (1<<(int)fp_overflow)
#define iexact (1<<(int)fp_inexact)
#define ivalid (1<<(int)fp_invalid)
#define allexc (divbyz|unflow|ovflow|iexact|ivalid)
#define comexc (divbyz|ovflow|ivalid)

extern _swapTE();	/* _swapTE(ex) exchanges ex with the current 
			 * exception trap enable bits. Here ex is an integer
			 * where each bit corresponds to an 
			 * exception trap enable status flag (0 off,1 on).
			 * (cf. enum fp_exception_type in <sys/ieeefp.h>) 
			 */

int ieee_handler(action,exception,hdl)
char *action, *exception;
sigfpe_handler_type hdl;
{
	char a,e;
	int ex;
	int default_or_ignore;

	a = action[0];
	e = exception[0];
	switch(a) {
	    case 'g':
		switch(e) {
		    case 'd': ex  = (int) fp_division;  break;
		    case 'u': ex  = (int) fp_underflow; break;
		    case 'o': ex  = (int) fp_overflow;  break;
		    case 'i': e = exception[2];
			if(e=='v') ex  = (int)fp_invalid; else
			if(e=='e') ex  = (int)fp_inexact;
			else {return (int) SIGFPE_DEFAULT;}
			break;
		    default:
			return (int) SIGFPE_DEFAULT;
		}
		return (int) ieee_handlers[ex ];
	    case 's':

		default_or_ignore = (hdl == SIGFPE_DEFAULT) | 
				    (hdl == SIGFPE_IGNORE) ;
		if (!default_or_ignore) _test_sigfpe_master();
					/* Make sure master SIGFPE handler
					   is enabled. */
		ex = _swapTE(0);
		if (ex < 0) return 1;	/* action not available */
		switch(e) {
		    case 'd':
			if(default_or_ignore) ex &= (~ divbyz); 
			else ex |= divbyz;
			ieee_handlers[(int)fp_division] = hdl; break;
		    case 'u':
			if(default_or_ignore) ex &= (~ unflow); 
			else ex |= unflow;
			ieee_handlers[(int)fp_underflow] = hdl; break;
		    case 'o':
			if(default_or_ignore) ex &= (~ ovflow); 
			else ex |= ovflow;
			ieee_handlers[(int)fp_overflow] = hdl; break;
		    case 'a':
			if(default_or_ignore) ex &= (~ allexc); 
			else ex |= allexc;
			ieee_handlers[(int)fp_inexact] = hdl; 
			ieee_handlers[(int)fp_underflow] = hdl; 
			ieee_handlers[(int)fp_invalid] = hdl; 
			ieee_handlers[(int)fp_division] = hdl; 
			ieee_handlers[(int)fp_overflow] = hdl; break;
		    case 'c':
			if(default_or_ignore) ex &= (~ comexc); 
			else ex |= comexc;
			ieee_handlers[(int)fp_invalid] = hdl; 
			ieee_handlers[(int)fp_division] = hdl; 
			ieee_handlers[(int)fp_overflow] = hdl; break;
		    case 'i':
			e = exception[2];
			if(e=='v') {
			    if(default_or_ignore) ex &= (~ ivalid); 
			    else ex |= ivalid;
			    ieee_handlers[(int)fp_invalid] = hdl; break;
			} else if(e=='e') {
			    if(default_or_ignore) ex &= (~ iexact); 
			    else ex |= iexact;
			    ieee_handlers[(int)fp_inexact] = hdl; break;
			}
		    default:
			return 1;	/* action not available */
		}
		if(ex!=0) _swapTE(ex);
		return 0;
	    case 'c':
		ex = _swapTE(0);
		if (ex < 0) return 0;	/* action not available */
		switch(e) {
		    case 'a': ex &= (~ allexc); return 0;
		    case 'd': ex &= (~ divbyz); break;
		    case 'u': ex &= (~ unflow); break;
		    case 'o': ex &= (~ ovflow); break;
		    case 'c': ex &= (~ comexc); break;
		    case 'i': e=exception[2];
			if(e=='v') ex &= (~ ivalid); else
			if(e=='e') ex &= (~ iexact);
			break;
		}
		if(ex!=0) _swapTE(ex);
		return 0;
	    default:
		return 1;		/* action not available */
	}
}
