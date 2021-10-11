
#ifndef lint
static	char sccsid[] = "@(#)r_fmod_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

static int 
is 	= 0x80000000,
im 	= 0x007fffff,
ii	= 0x7f800000,
iu 	= 0x00800000;

static float	zero	= 0.0;

FLOATFUNCTIONTYPE r_fmod_(x,y)
float *x, *y;
{
	float w;
	int ix,iy,iz,n,k,ia,ib;
	ix = (*(int*)x)&0x7fffffff;
	iy = (*(int*)y)&0x7fffffff;

    /* purge off exception values */
	if(ix>=ii||iy>ii||iy==0) {
	    w = (*x)*(*y); w = w/w; 
	} else if(ix<=iy) {
	    if(ix<iy) 	w = *x;		/* return x if |x|<|y| */
	    else 	w = zero*(*x);	/* return sign(x)*0.0  */
	} else if(iy<0x0c800000) 
	    w = (float) fmod((double)*x,(double)*y);
	else
    {

    /* a,b is in reasonable range. Now prepare for fix point arithmetic */
	ia = ix&ii; ib = iy&ii;
	n  = ((ia-ib)>>23);
	ix = iu|(ix&im);
	iy = iu|(iy&im);

    /* fix point fmod */
	/*
	while(n--) {
	    iz=ix-iy;
	    if(iz<0) ix = ix+ix;
	    else if(iz==0){*(int*)&w=is&(*(int*)x); RETURNFLOAT(w);}
	    else ix = iz+iz;
	}
	*/
    /* unroll the above loop 4 times to gain performance */
	k = n&3; n >>= 2;
	while(n--) {
	    iz=ix-iy;
	    if(iz<0) ix = ix+ix;
	    else if(iz==0){*(int*)&w=is&(*(int*)x); RETURNFLOAT(w);}
	    else ix = iz+iz;
	    iz=ix-iy;
	    if(iz<0) ix = ix+ix;
	    else if(iz==0){*(int*)&w=is&(*(int*)x); RETURNFLOAT(w);}
	    else ix = iz+iz;
	    iz=ix-iy;
	    if(iz<0) ix = ix+ix;
	    else if(iz==0){*(int*)&w=is&(*(int*)x); RETURNFLOAT(w);}
	    else ix = iz+iz;
	    iz=ix-iy;
	    if(iz<0) ix = ix+ix;
	    else if(iz==0){*(int*)&w=is&(*(int*)x); RETURNFLOAT(w);}
	    else ix = iz+iz;
	}
	switch(k) {
	case 3:
	    iz=ix-iy;
	    if(iz<0) ix = ix+ix;
	    else if(iz==0){*(int*)&w=is&(*(int*)x); RETURNFLOAT(w);}
	    else ix = iz+iz;
	case 2:
	    iz=ix-iy;
	    if(iz<0) ix = ix+ix;
	    else if(iz==0){*(int*)&w=is&(*(int*)x); RETURNFLOAT(w);}
	    else ix = iz+iz;
	case 1: 
	    iz=ix-iy;
	    if(iz<0) ix = ix+ix;
	    else if(iz==0){*(int*)&w=is&(*(int*)x); RETURNFLOAT(w);}
	    else ix = iz+iz;
	}
	iz=ix-iy;
	if(iz>=0) ix = iz;

    /* convert back to floating value and restore the sign */
	if(ix==0){*(int*)&w=is&(*(int*)x); RETURNFLOAT(w);}
	while((ix&iu)==0) {
	    ix += ix;
	    ib -= iu;
	}
	ib |= (*(int*)x)&is;
	*(int*)&w = ib|(ix&im);
    }
    RETURNFLOAT(w);
}

