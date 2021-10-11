
#ifndef lint
static	char sccsid[] = "@(#)fmod.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

static long 
is 	= 0x80000000,
im 	= 0x000fffff,
iu 	= 0x00100000;

static double	
zero	= 0.0,
two110 	= 1.29807421463370690713e+33, 	/* 46D00000 00000000 */
twom110 = 7.70371977754894341222e-34, 	/* 39100000 00000000 */
twom969	= 2.00416836000897277800e-292;	/* 03600000 00000000 */

double fmod(x,y)
double x,y ;
{
	double a,b;
	long *px = (long*)&x;
	long *pa = (long*)&a;
	long *pb = (long*)&b;
	int n0,n1,n,k,ia,ib;
	int x0,y0,z0;
	unsigned x1,y1,z1;

    /* purge off exception values */
	if(!finite(x)||y!=y||y==zero) return (x*y)/(x*y);
	a = fabs(x); b = fabs(y);
	if(a<=b) { if(a<b) return x; else return zero*x;}
	if(b<twom969) {
		b *= two110;
		return twom110*fmod(two110*fmod(x,b),b);
	}

    /* check out ordering of floating point number */
	if ((*(int*)&two110)==0x46d00000) {n0=0;n1=1;} else {n0=1;n1=0;}

    /* a,b is in reasonable range. Now prepare for fix point arithmetic */
	ia = pa[n0]&0x7ff00000; ib = pb[n0]&0x7ff00000;
	n  = ((ia-ib)>>20);
	x0 = iu|(im&pa[n0]); x1 = pa[n1];
	y0 = iu|(im&pb[n0]); y1 = pb[n1];

    /* fix point fmod */
	/*
	while(n--) {
	    z0=x0-y0;z1=x1-y1; if(x1<y1) z0 -= 1;
	    if(z0<0){x0 = x0+x0+((x1&is)!=0); x1 = x1+x1;}
	    else {
	    	if((z0|z1)==0){pa[n0]=px[n0]&is;pa[n1]=0;return a;}
	    	x0 = z0+z0+((z1&is)!=0); x1 = z1+z1;
	    }
	}
	*/
    /* unroll the above loop 4 times to gain performance */
	k = n&3; n >>= 2;
	while(n--) {
	    z0=x0-y0;z1=x1-y1; if(x1<y1) z0 -= 1;
	    if(z0<0){x0 = x0+x0+((x1&is)!=0); x1 = x1+x1;}
	    else { if((z0|z1)==0){pa[n0]=px[n0]&is;pa[n1]=0;return a;}
	    	x0 = z0+z0+((z1&is)!=0); x1 = z1+z1; }
	    z0=x0-y0;z1=x1-y1; if(x1<y1) z0 -= 1;
	    if(z0<0){x0 = x0+x0+((x1&is)!=0); x1 = x1+x1;}
	    else { if((z0|z1)==0){pa[n0]=px[n0]&is;pa[n1]=0;return a;}
	    	x0 = z0+z0+((z1&is)!=0); x1 = z1+z1; }
	    z0=x0-y0;z1=x1-y1; if(x1<y1) z0 -= 1;
	    if(z0<0){x0 = x0+x0+((x1&is)!=0); x1 = x1+x1;}
	    else { if((z0|z1)==0){pa[n0]=px[n0]&is;pa[n1]=0;return a;}
	    	x0 = z0+z0+((z1&is)!=0); x1 = z1+z1; }
	    z0=x0-y0;z1=x1-y1; if(x1<y1) z0 -= 1;
	    if(z0<0){x0 = x0+x0+((x1&is)!=0); x1 = x1+x1;}
	    else { if((z0|z1)==0){pa[n0]=px[n0]&is;pa[n1]=0;return a;}
	    	x0 = z0+z0+((z1&is)!=0); x1 = z1+z1; }
	}
	switch(k) {
	    case 3:
	    z0=x0-y0;z1=x1-y1; if(x1<y1) z0 -= 1;
	    if(z0<0){x0 = x0+x0+((x1&is)!=0); x1 = x1+x1;}
	    else { if((z0|z1)==0){pa[n0]=px[n0]&is;pa[n1]=0;return a;}
	    	x0 = z0+z0+((z1&is)!=0); x1 = z1+z1; }
	    case 2:
	    z0=x0-y0;z1=x1-y1; if(x1<y1) z0 -= 1;
	    if(z0<0){x0 = x0+x0+((x1&is)!=0); x1 = x1+x1;}
	    else { if((z0|z1)==0){pa[n0]=px[n0]&is;pa[n1]=0;return a;}
	    	x0 = z0+z0+((z1&is)!=0); x1 = z1+z1; }
	    case 1: 
	    z0=x0-y0;z1=x1-y1; if(x1<y1) z0 -= 1;
	    if(z0<0){x0 = x0+x0+((x1&is)!=0); x1 = x1+x1;}
	    else { if((z0|z1)==0){pa[n0]=px[n0]&is;pa[n1]=0;return a;}
	    	x0 = z0+z0+((z1&is)!=0); x1 = z1+z1; }
	}
	z0=x0-y0;z1=x1-y1; if(x1<y1) z0 -= 1;
	if(z0>=0) {x0=z0;x1=z1;}

    /* convert back to floating value and restore the sign */
	if((x0|x1)==0) {pa[n0]=px[n0]&is;pa[n1]=0;return a;}
	while((x0&iu)==0) {
		x0 = x0+x0+((x1&is)!=0); x1 = x1+x1;
		ib -= iu;
	}
	ib |= px[n0]&is;
	pa[n0] = ib|(x0&im);
	pa[n1] = x1;
	return a;
}
