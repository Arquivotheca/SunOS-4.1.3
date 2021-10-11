
#ifndef lint
static	char sccsid[] = "@(#)r_trig_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

/* r_sin_(x), r_cos_(x), r_tan_(x), r_sincos_(x,s,c)
 * Single precision trigonometric functions.
 * Coeff from Peter Tang's "Some Software Implementations of the 
 * Functions Sin and Cos." 
 *						-- K.C. Ng
 *
 * Static kernel function:
 *	r_trig()	... sin, cos, and tan
 *	r_argred()	... argument reduction for r_argred functions
 *
 * Method.
 *      Let S and C denote the polynomial approximations to sin and cos 
 *      respectively on [-PI/4, +PI/4].
 *      1. Call r_argred to obtain y = x-n*pi/2 in double in
 *	   [-pi/2 , +pi/2] and the last three bit of n in k.  
 *	2. Let S=S(y), C=C(y). Depending on k, we have
 *
 *          k        sin(x)      cos(x)        tan(x)	
 *     ----------------------------------------------------------
 *	    0	       S	   C		 S/C
 *	    1	       C	  -S		-C/S
 *	    2	      -S	  -C		 S/C
 *	    3	      -C	   S		-C/S
 *     ----------------------------------------------------------
 *
 *   Notes: Let z = y*y, then
 *      S = S(y) = y*(1-z*(S0+z*(S1+z*S2)))
 *	C = C(y) = 1 - z*(0.5-z*(C0+z*(C1+z*C2)));
 *
 * Special cases:
 *      Let trig be any of sin, cos, or tan.
 *      trig(+-INF)  is NaN, with signals;
 *      trig(NaN)    is that NaN;
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

static double 
S0 =  1.66666552424430847168e-01,	/* S0 2^ -3 * Hex  1.5555460000000 */
S1 = -8.33219196647405624390e-03,	/* S1 2^ -7 * Hex -1.11077E0000000 */
S2 =  1.95187909412197768688e-04,	/* S2 2^-13 * Hex  1.9956B60000000 */
C0 =  4.16666455566883087158e-02,	/* C0 2^ -5 * Hex  1.55554A0000000 */
C1 = -1.38873036485165357590e-03,	/* C1 2^-10 * Hex -1.6C0C1E0000000 */
C2 =  2.44309903791872784495e-05;	/* C2 2^-16 * Hex  1.99E24E0000000 */

static float			
f1 	= 1.0,
f1_2 	= 0.5,
f1_6	= 0.1666666666666666666,
f1_3	= 0.3333333333333333333,
finvpio2= 0.636619772367581343075535;   /* 2^ -1  * Hex  1.45F306DC9C883 */

static	double
big	= 1.0e20,
pio2    = 1.57079632679489661923,       /* 2^  0  * Hex  1.921FB54442D18 */
pio2_1  = 1.570796326734125614166,      /* 2^  0  * Hex  1.921FB54400000 */
pio2_t	= 6.077100506506192601475e-11,  /* 2^-34  * Hex  1.0B4611A626331 */
pio2_t66= 6.077100506303965976596e-11,  /* 2^-34  * Hex  1.0B4611A600000 */
pio2_t53= 6.077094383272196864709e-11,  /* 2^-34  * Hex  1.0B46000000000 */
one 	= 1.0,
half	= 0.5;

static FLOATFUNCTIONTYPE r_trig();
static int r_argred(),dummy();
static float sincos_s,sincos_c;


FLOATFUNCTIONTYPE r_sin_(x)
float *x;
{
	return r_trig(x,0);
}

FLOATFUNCTIONTYPE r_cos_(x)
float *x;
{
	return r_trig(x,1);
}

FLOATFUNCTIONTYPE r_tan_(x)
float *x;
{
	return r_trig(x,2);
}

void r_sincos_(x,s,c)
float *x,*s,*c;
{
	r_trig(x,3);
	*s = sincos_s;
	*c = sincos_c;
}


static FLOATFUNCTIONTYPE r_trig(x,k) 
float *x; int k;
{
	double z,y,w,ss,cc;
	float ft,fz;
	long ix,iy,n;

	iy = *((long*)x);	/* iy =  x  */
	ix = iy&(~0x80000000);	/* ix = |x| */

    /* small argument */
	if (ix < 0x3c000000) {	/* if |x| < 0.0078125 = 2**-7 */
	    if (ix<0x3727C5AC) {	/* if |x| < tiny=1e-5 */
		dummy((double)*x+big);	/* create inexact flag if x!=0 */
		if(k==3) {sincos_s= *x;sincos_c=f1;return ;}
		ft = (k==1)? f1:*x;
		RETURNFLOAT(ft);
	    }
	    ft = *x; fz = ft*ft;
	    switch(k) {
		case 0: ft -= fz*(ft*f1_6) ; break;
		case 1: ft  = f1 - f1_2*fz ; break;
		case 2: ft += fz*(ft*f1_3) ; break;
		case 3: sincos_s = ft - fz*ft*f1_6; sincos_c = f1 - f1_2*fz;
			return ;
	    }
	    RETURNFLOAT(ft);
	}

    /* argument reduction */
	y = (double) *x;	/* y = x in double */
	n = 0;
	if(ix > 0x3F490FDB) {	/* if |x| > pio4 */
	    if ( ix <= 0x3fc90FDB) {		/* |x| <= pio2 */
		if(iy>0) 	{ n =  1; y -= pio2; }
		else		{ n = -1; y += pio2; }
	    } else if (ix <= 0x49C90FD8) {	/* |x| < 2^19*pi/2 */
		if(iy>0) 	{
		    n   = (int) ((*x)*finvpio2+f1_2);
		} else {
		    n   = (int) ((*x)*finvpio2-f1_2);
		}
		w   = (double)n;
		z   = y - w*pio2;

	    /* usually z is good enough, if not, recompute z */
		ix  = ((*(long*)&w)&0x7ff00000)-((*(long*)&z)&0x7ff00000);
		if(ix>0x01600000)
		y   = (y-w*pio2_1)-w* ((fp_pi==fp_pi_66)? 
			pio2_t66: (fp_pi==fp_pi_infinite)? pio2_t:pio2_t53);
		else y = z;
	    } else if(ix>=0x7f800000) { 
	    	ft = (*x)-(*x);			/* trig(NaN or INF) is NaN */
	    	if(k==3) {sincos_c = sincos_s = ft; return ;}
	    	RETURNFLOAT(ft);
	    } else
		n = r_argred(y,&y);		/* huge x */
	}

    /* compute sin, cos, tan, sincos on primary range */ 
	z = y*y;
	if(k<2) { 
	    n += k;		/* sin, cos */
	    if((n&1)==0) {
		ft = y*(one-z*(S0+z*(S1+z*S2)));
	    } else {
		ft = one - z*(half-z*(C0+z*(C1+z*C2)));
	    }
	    if((n&2)!=0) ft = -ft;
	} else {
	    ss = y*(one-z*(S0+z*(S1+z*S2)));
	    cc = one-z*(half-z*(C0+z*(C1+z*C2)));
	    if(k==3) {		/* sincos */
		switch(n&3) {
		    case 0: sincos_s =  ss; sincos_c =  cc; return ;
		    case 1: sincos_s =  cc; sincos_c = -ss; return ;
		    case 2: sincos_s = -ss; sincos_c = -cc; return ;
		    case 3: sincos_s = -cc; sincos_c =  ss; return ;
		}
	    } else {		/* tan */
		if ((n&1)==0) 
		    ft =   ss/cc;	/*  sin/cos */
		else 
		    ft =  -cc/ss;	/* -cos/sin */
	    }
	}
	RETURNFLOAT(ft);
}

static int dummy(x)
double x;
{
	return 1;
}

/*
 * r_argred(x,y)
 * huge argument reduction routine (single precision)
 */
static double fv_53[] = {
6.36619746685028076172e-01,     /* 2^-1    * Hex 1.45F3060000000 */
2.56825529731941060163e-08,     /* 2^-26   * Hex 1.B939100000000 */
3.18525892782032388206e-16,     /* 2^-52   * Hex 1.6F3C400000000 */
1.93901821176707448727e-22,     /* 2^-73   * Hex 1.D4D36A0000000 */
1.04464734748536477775e-29,     /* 2^-97   * Hex 1.A7C2620000000 */
2.84710934252472767508e-37,     /* 2^-122  * Hex 1.8387480000000 */
3.77640111052463901703e-44,     /* 2^-145  * Hex 1.AF30540000000 */
2.00661093781429402782e-51,     /* 2^-169  * Hex 1.8063EA0000000 */
};

static double fv_66[] = {
6.36619746685028076172e-01,     /* 2^-1    * Hex 1.45F3060000000 */
2.56825529731941060163e-08,     /* 2^-26   * Hex 1.B939100000000 */
2.93710368526323151173e-16,     /* 2^-52   * Hex 1.52A0000000000 */
5.10449803663170550951e-24,     /* 2^-78   * Hex 1.8AF1000000000 */
6.72238323924519411869e-31,     /* 2^-101  * Hex 1.B44EC00000000 */
5.60802111708089003462e-37,     /* 2^-121  * Hex 1.7DA9820000000 */
4.44864500117760072482e-44,     /* 2^-145  * Hex 1.FBF20A0000000 */
9.69241260771952909720e-52,     /* 2^-170  * Hex 1.7356E80000000 */
};

static double fv_inf[] = {
6.36619746685028076172e-01,     /* 2^-1    * Hex 1.45F3060000000 */
2.56825529731941060163e-08,     /* 2^-26   * Hex 1.B939100000000 */
2.93709521493375896872e-16,     /* 2^-52   * Hex 1.529FC00000000 */
3.25437940017324563823e-23,     /* 2^-75   * Hex 1.3ABE880000000 */
1.20896137088285372011e-29,     /* 2^-97   * Hex 1.EA69BA0000000 */
5.66755679574007135868e-37,     /* 2^-121  * Hex 1.81B6C40000000 */
2.62040311120972145968e-44,     /* 2^-145  * Hex 1.2B32780000000 */
7.05395768088062862277e-52,     /* 2^-170  * Hex 1.0E41040000000 */
};


static int r_argred(x,y) 	/* return the last three bits of N */
double x, *y;
{
	double q[8],*fv,t;		
	int signx,k,n,mm,i,k1,k2;

	fv = fv_66;
	if((int)fp_pi==0) fv = fv_inf; 
	if((int)fp_pi==2) fv = fv_53; 

    /* save sign bit and replace x by its absolute value */
	signx = signbit(x); 
	x = fabs(x);
	n = ilogb(x);	/* assume n>0 */
    /*
     *	k1 = min{(n-26)/24 chopped,0}
     *  k2 = ceil{(n+37)/24} = (n+60)/24 chopped to gurantee 7 more bit
     */
	k1 = (n-26)/24; if(k1<0) k1 = 0;
	k2 = (n+60)/24;

	k  = k2-k1;
	for(i=0;i<=k;i++) q[i] = fv[i+k1]*x;
	q[0] -= 8*(aint(q[0]*0.125));
	q[1] -= 8*(aint(q[1]*0.125));
	n     = q[2]*0.125; if(n!=0) q[2] -= (double)(n<<3);
	t = q[k];
	for(i=k-1;i>=0;i--) t += q[i] ;
	mm = (int) t;
	t -= (double)mm;
	if (t>0.5) {mm += 1; t -= 1.0;}

    /* if |t| < 2^-19, recompute t */
	if(fabs(t) < .0000019073486328125) {	
	    t = q[0]-(double)mm;
	    for(i=1;i<=k;i++) t += q[i];
	}
	*y  = t*pio2;
	if(signx!=0) { *y = -(*y); mm = -mm;}
	return mm&7;
}
