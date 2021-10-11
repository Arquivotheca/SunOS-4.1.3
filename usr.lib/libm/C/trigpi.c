
#ifndef lint
static  char sccsid[] = "@(#)trigpi.c 1.1 92/07/30 SMI"; 
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/* sinpi(x), cospi(x), tanpi(x), sincospi(x,s,c)
 * return double precision trig(pi*x).
 */

#include <math.h>
static double 
pi 	= 3.14159265358979323846,	/* 400921FB,54442D18 */
two52	= 4503599627370496.0,		/* 43300000,00000000 */
two53	= 9007199254740992.0;		/* 43400000,00000000 */

double sinpi(x)
double x;
{
	double y,z;
	int i,n;
	if(!finite(x)) 	 return x-x;
	y = fabs(x);
	if(y<0.25) return sin(pi*x);
	i = signbit(x); 

    /*
     * argument reduction, make sure inexact flag not raised if input
     * is an integer
     */
	z = aint(y);
	if(z!=y) {				/* inexact anyway */
	    y  *= 0.5;
	    y   = 2.0*(y - aint(y));		/* y = |x| mod 2.0 */
	    n   = (int) (y*4.0);
	} else {
	    if(z>=two53) {
		y = 0.0; n = 0;			/* y must be even */
	    } else {
		if(z<two52) z = y+two52;		/* exact */
	    	n  = (*((*(int*)&two52==0)+(int*)&z))&1;/* lower word of z */
	    	y  = n;
	    	n<<= 2;
	    }
	}
	switch (n) {
	    case 0:   y =  sin(pi*y); break;
	    case 1:   
	    case 2:   y =  cos(pi*(0.5-y)); break;
	    case 3:  
	    case 4:   y =  sin(pi*(1.0-y)); break;
	    case 5:
	    case 6:   y = -cos(pi*(y-1.5)); break;
	    default:  y =  sin(pi*(y-2.0)); break;
	    }
	return (i==0)? y: -y;
}

double cospi(x)
double x;
{
	double y,z;
	int n;
	if(!finite(x)) 	 return x-x;
	y = fabs(x);
	if(y<0.25) {if(y>1.0e-20) y*=pi; return cos(y);}
    /*
     * argument reduction, make sure inexact flag not raised if input
     * is an integer
     */
	z = aint(y);
	if(z!=y) {				/* inexact anyway */
	    y  *= 0.5;
	    y   = 2.0*(y - aint(y));		/* y = |x| mod 2.0 */
	    n   = (int) (y*4.0);
	} else {
	    if(z>=two53) {
		y = 0.0; n = 0;			/* y must be even */
	    } else {
		if(z<two52) z = y+two52;			/* exact */
	    	n   = (*((*(int*)&two52==0)+(int*)&z))&1;
	    	y   = n;
	    	n <<= 2;
	    }
	}
	switch (n) {
	    case 0:  return  cos(pi*y);
	    case 1:  
	    case 2:  return  sin(pi*(0.5-y));
	    case 3: 
	    case 4:  return -cos(pi*(1.0-y));
	    case 5:  return  sin(pi*(y-1.5));
	    case 6:  return -sin(pi*(1.5-y));
	    default: return  cos(pi*(y-2.0));
	}
}

double tanpi(x)
double x;
{
	double s,c;
	void sincospi();
	sincospi(x,&s,&c);
	return s/c;
}

void sincospi(x,s,c)
double x,*s,*c;
{
	double y,z;
	int i,n;
	if(!finite(x)) 	 { *s = *c = x-x; }
	else {y=fabs(x); if(y<0.25) sincos(pi*x,s,c);
	else {
	    i = signbit(x); 
	    y *= 0.5;
	    y = 2.0*(y - aint(y));		/* y = x mod 2.0 */
    /*
     * argument reduction, make sure inexact flag not raised if input
     * is an integer
     */
	    z = aint(y);
	    if(z!=y) {				/* inexact anyway */
	        y  *= 0.5;
	        y   = 2.0*(y - aint(y));		/* y = |x| mod 2.0 */
	        n   = (int) (y*4.0);
	    } else {
	        if(z>=two53) {
		    y = 0.0; n = 0;			/* y must be even */
	        } else {
		    if(z<two52) z = y+two52;			/* exact */
	    	    n   = (*((*(int*)&two52==0)+(int*)&z))&1;
	    	    y   = n;
	    	    n <<= 2;
	        }
	    }
	    switch (n) {
		case 0:  sincos(pi*y,s,c);			break;
		case 1:
		case 2:  sincos(pi*(0.5-y),c,s);		break;
		case 3:
		case 4:  sincos(pi*(1.0-y),s,c); *c = -(*c);	break;
		case 5:  sincos(pi*(y-1.5),c,s); *s = -(*s);	break;
		case 6:  sincos(pi*(1.5-y),c,s); *s = -(*s); *c = -(*c);break;
		default: sincos(pi*(y-2.0),s,c); 		break;
		}
	    if(i!=0) *s = -(*s);
	    }
	}
}
