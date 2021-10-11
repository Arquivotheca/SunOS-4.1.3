
#ifndef lint
static	char sccsid[] = "@(#)ieee_flags.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>
#include "libm.h"

#define divbyz (1<<(int)fp_division)
#define unflow (1<<(int)fp_underflow)
#define ovflow (1<<(int)fp_overflow)
#define iexact (1<<(int)fp_inexact)
#define ivalid (1<<(int)fp_invalid)
#define allexc (divbyz|unflow|ovflow|iexact|ivalid)
#define comexc (divbyz|ovflow|ivalid)

extern enum fp_direction_type _swapRD();
extern enum fp_precision_type _swapRP();
extern _swapEX(); /* _swapEX returns an integer where each bit corresponds 
		   * to an exception-occurred accrued status flag (0 off,1 on)
		   * (cf.  enum fp_exception_type in <sys/ieeefp.h>)
		   */
/* swap..() returns a negative number if the corresponding feature is
 * not available
 */

int ieee_flags(action,mode,in,out)
char *action, *mode, *in, **out;
{
	char a,m,i;
	enum fp_direction_type rd;
	enum fp_precision_type rp;
	int ex;
	a = action[0];
	m = mode[0];
	switch(a) {
    
    /* get */
	    case 'g':
		switch(m) {
		    case 'd':		/* direction: default fp_nearest */
			rd  = _swapRD(fp_nearest);
			switch(rd) {
			    case fp_tozero   : *out = "tozero"  ; break;
			    case fp_negative : *out = "negative"; break;
			    case fp_positive : *out = "positive"; break;
			    case fp_nearest  : 
			    default: 
			    	*out = "nearest" ; 
				return (int)fp_nearest;
			}
			_swapRD(rd);	/* restore rd */
			return (int) rd; 
		    case 'p':		/* precision: default fp_extended */
			rp  = _swapRP(fp_extended);
			switch(rp) {
			    case fp_single   : *out = "single"  ; break;
			    case fp_double   : *out = "double"  ; break;
			    case fp_extended : 
			    default: 
			    	*out = "extended"; return (int)fp_extended;
			}
			_swapRP(rp);	/* restore rp */
			return (int) rp; 
		    case 'e':
			ex  = _swapEX(0);
			if(ex<=0) {
			    *out = "not available";
			    if(ex==0) *out = "";
			    return 0;
			}
			i   = in[0];
			_swapEX(ex);	/* restore ex */
			switch(i) {
			    case 'i':
				i = in[2];
				if(i=='v'&&(ex&ivalid)!=0) {
				    *out = "invalid"; return ex; }
				else if(i=='e'&&(ex&iexact)!=0) {
				    *out = "inexact"; return ex; }
				break;
			    case 'o':
				if((ex&ovflow)!=0) {*out="overflow";return ex;}
				break;
			    case 'u':
				if((ex&unflow)!=0) {*out="underflow";return ex;}
				break;
			    case 'd':
				if((ex&divbyz)!=0) {*out="division";return ex;}
			}
			if((ex&ivalid)!=0) *out="invalid";
			else if((ex&ovflow)!=0) *out="overflow";
			else if((ex&divbyz)!=0) *out="division";
			else if((ex&unflow)!=0) *out="underflow";
			else *out="inexact";	/* ex==iexact */
			return ex;
		    default:
			*out = "mode not available";
			return 0; 
		}	

    /* set */
	    case 's':
		switch(m) {
		    case 'd':
			i = in[0];
			switch(i) {
			    case 't': rd = _swapRD(fp_tozero); break;
			    case 'n': i = in[2];
				if (i=='a')     rd = _swapRD(fp_nearest);
				else if(i=='g') rd = _swapRD(fp_negative);
				else return 1;
				break;
			    case 'p': rd = _swapRD(fp_positive); break;
			    default: return 1;
			}
			if(((int) rd) < 0) return 1; else return 0;
		    case 'p':
			i = in[0];
			switch(i) {
			    case 'e': rp = _swapRP(fp_extended); break;
			    case 'd': rp = _swapRP(fp_double); break;
			    case 's': rp = _swapRP(fp_single); break;
			    default: return 1;
			}
			if(((int) rd) < 0) return 1; else return 0;
		    case 'e':
			i = in[0];
			ex= _swapEX(0);
			if(ex<0) return 1; 
			switch(i) {
			    case 'd': ex |= divbyz; break;
			    case 'i': 
				i = in[2]; 
				if(i=='v') ex |= ivalid; 
				else if(i=='e') ex |= iexact; 
				break;
			    case 'o': ex |= ovflow; break;
			    case 'u': ex |= unflow; break;
			    case 'a': ex |= allexc;
					break;
			    case 'c': ex |= comexc; break;
			}
			_swapEX(ex); 
			return 0;
		    default:
			return 1;	/* not available */
		}

    /* clear or clearall */
	    case 'c':
		a = action[5];
		if(a=='a') {		/* action == clearall */
		    			/* restore default modes and status */
		    _swapRD(fp_nearest);
		    _swapRP(fp_extended);
		    _swapEX(0);
		} else {
		    switch (m) {
			case 'd':
			    _swapRD(fp_nearest); break;
			case 'p':
			    _swapRP(fp_extended); break;
			case 'e':
			    i = in[0];
			    ex= _swapEX(0);
			    switch(i) {
				case 'd': ex &= (~ divbyz); break;
				case 'i': 
				    i = in[2]; 
				    if(i=='v') ex &= (~ ivalid); 
				    else if(i=='e') ex &= (~ iexact); 
				    break;
				case 'o': ex &= (~ ovflow); break;
				case 'u': ex &= (~ unflow); break;
				case 'a': ex &= (~ allexc); break;
				case 'c': ex &= (~ comexc);
						break;
			    }
			    _swapEX(ex);
		    }
		}
	    default:
		return 0;
	}
}

