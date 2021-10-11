
#ifndef lint
static	char sccsid[] = "@(#)sqrt.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Warning: This correctly rounded sqrt is extreme slow because it computes
 * the sqrt bit by bit using integer arithmetic.
 */

#include <math.h>
#include "libm.h"

static unsigned long 	SIGN = 0x80000000, 
			EXPB = 0x7ff00000;

double sqrt(x)
double x;
{
double	one	= 1.0;
int	n0;

	double z;
	unsigned long r,t1,s1,ix1,q1;
	long *px = (long *) &x, *pz = (long *) &z;
	long ix0,s0,j,k,q,m,n,s,t;

	if ((* (int *) &one) != 0) n0=0;	/* not a i386 */
	else n0=1;				/* is a i386  */

	ix0 = px[n0]; ix1 = px[1-n0];
	if((ix0&0x7ff00000)==0x7ff00000) {
	    if(ix0==0xfff00000&&ix1==0) return SVID_libm_err(x,x,26); 
	    else return x+x;
	} 
	if(((ix0&0x7fffffff)|ix1)==0) return x;
	if(ix0<0) {
	    if ((ix1|(ix0&(~SIGN)))!=0) {
		x = SVID_libm_err(x,x,26);	/* invalid */
	    }
	    return x;
	} else if ((ix0&EXPB)==EXPB) return x+x;
	else {
	    m   = ilogb(x);
	    z   = scalbn(x,-m);
	    ix0 = (pz[n0]&0x000fffff)|0x00100000;
	    ix1 = pz[1-n0];
	    n   = m/2;
	    if ((n+n)!=m) { 
		ix0 += ix0 + ((ix1&SIGN)>>31);
		ix1 += ix1;
		m   -= 1;
		n    = m/2;
	    }

	/* generate sqrt(x) bit by bit */
	    ix0 += ix0 + ((ix1&SIGN)>>31);
	    ix1 += ix1;
	    q = q1 = s0 = s1 = 0;
	    r = 0x00200000;

	    for (j=1;j<=22;j++) {
		t = s0+r; 
		if(t<=ix0) { 
		    s0   = t+r; 
		    ix0 -= t; 
		    q   += r; 
		} 
	        ix0 += ix0 + ((ix1&SIGN)>>31);
		ix1 += ix1;
		r>>=1;
	    }

	    r = SIGN;
	    for (j=1;j<=32;j++) {
		t1 = s1+r; 
		t  = s0;
		if((t<ix0)||((t==ix0)&&(t1<=ix1))) { 
		    s1  = t1+r;
		    if(((t1&SIGN)==SIGN)&&(s1&SIGN)==0) s0 += 1;
		    ix0 -= t;
		    if (ix1 < t1) ix0 -= 1;
		    ix1 -= t1;
		    q1  += r;
		}
	        ix0 += ix0 + ((ix1&SIGN)>>31);
		ix1 += ix1;
		r>>=1;
	    }

	    if ((ix0|ix1)==0) goto done;
	    z = two52-twom52; /* trigger inexact flag */
	    if (z<two52) goto done;
	    z = two52+twom52;
	    if (q1==0xffffffff) { q1=0; q += 1; goto done;}
	    if (z>two52) {
		if (q1==0xfffffffe) q+=1;
		q1+=2; goto done;
	    }
	    q1 += (q1&1);
done:	
	    pz[n0] = (q>>1)+0x3fe00000;
	    pz[1-n0] = q1>>1;
	    if ((q&1)==1) pz[1-n0] |= SIGN;
	    return scalbn(z,n);
	}
}


