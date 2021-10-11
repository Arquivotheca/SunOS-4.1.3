
#ifndef lint
static	char sccsid[] = "@(#)r_sqrt_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

static double huge=1.0e100, tiny=1.0e-300;

FLOATFUNCTIONTYPE r_sqrt_(x)
float *x;
{
	double dz; float w;
	long *pdz = (long *) &dz, *pw = (long *) &w;
	long ix,j,k,r,q,m,n,s,t;
	w = *x;
	ix = pw[0];
	if(ix<=0) {
	    if ((ix&0x7fffffff)!=0) w = (w-w)/(w-w);
	    RETURNFLOAT(w);
	} else if ((ix&0x7f800000)==0x7f800000) {
	    w = w+w;
	    RETURNFLOAT(w);
	} else {
	    m  = ir_ilogb_(&w);
	    n  = -m;
	    ASSIGNFLOAT(w,r_scalbn_(&w,&n));
	    ix = (pw[0]&0x007fffff)|0x00800000;
	    n  = m/2;
	    if ((n+n)!=m) { ix = ix+ix; m -=1; n=m/2;}

	/* generate sqrt(x) bit by bit */
	    ix <<= 1;
	    q = s = 0;
	    r = 0x01000000;
	    for (j=1;j<=25;j++) {
		t = s+r; 
		if(t<=ix) { s = t+r; ix -= t; q+=r; } 
		ix <<= 1; r>>=1;
	    }
	    if (ix==0) goto done;
	    dz = huge-tiny; /* trigger inexact flag */
	    if (dz<huge) goto done;
	    dz = huge+tiny;
	    if (dz>huge) q+=1;
	    q += (q&1);
done:	    pw[0] = (q>>1)+0x3f000000;
	    return r_scalbn_(&w,&n);
	}
}

