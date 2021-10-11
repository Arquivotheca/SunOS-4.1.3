
#ifdef sccsid
static	char sccsid[] = "@(#)atan.c 1.1 92/07/30 SMI"; 
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/* atan(x)
 * Table look-up algorithm
 * By K.C. Ng, March 9, 1989
 *
 * Algorithm.
 *
 * The algorithm is based on atan(x)=atan(y)+atan((x-y)/(1+x*y)).
 * We use poly1(x) to approximate atan(x) for x in [0,1/8] with 
 * error (relative)
 * 	|(atan(x)-poly1(x))/x|<= 2^-115.94 	long double
 * 	|(atan(x)-poly1(x))/x|<= 2^-58.85	double 
 * 	|(atan(x)-poly1(x))/x|<= 2^-25.53 	float
 * and use poly2(x) to approximate atan(x) for x in [0,1/65] with
 * error (absolute)
 *	|atan(x)-poly2(x)|<= 2^-122.15		long double
 *	|atan(x)-poly2(x)|<= 2^-64.79		double
 *	|atan(x)-poly2(x)|<= 2^-35.36		float
 * Here poly1 and poly2 are odd polynomial with the following form:
 *		x + x^3*(a1+x^2*(a2+...))
 *
 * (0). Purge off Inf and NaN and 0
 * (1). Reduce x to positive by atan(x) = -atan(-x).
 * (2). For x <= 1/8, use
 *	(2.1) if x < 2^(-prec/2), atan(x) =  x  with inexact flag raised
 *	(2.2) Otherwise
 *	   	atan(x) = poly1(x)
 * (3). For x >= 8 then
 *	(3.1) if x >= 2^prec,  	  atan(x) = atan(inf) - pio2lo
 *	(3.2) if x >= 2^(prec/3), atan(x) = atan(inf) - 1/x
 *	(3.3) if x >  65,         atan(x) = atan(inf) - poly2(1/x)
 *	(3.4) Otherwise,	  atan(x) = atan(inf) - poly1(1/x)
 *
 * (4). Now x is in (0.125, 8)
 *      Find y that match x to 4.5 bit after binary (easy).
 *	If iy is the high word of y, then
 *		single : j = (iy - 0x3e000000) >> 19
 *		double : j = (iy - 0x3fc00000) >> 16
 *		quad   : j = (iy - 0x3ffc0000) >> 12
 *
 *	Let s = (x-y)/(1+x*y). Then
 *	atan(x) = atan(y) + poly1(s)
 *		= _tbl_atan_hi[j] + (_tbl_atan_lo[j] + poly2(s) )
 *
 *	Note. |s| <= 1.5384615385e-02 = 1/65. Maxium occurs at x = 1.03125
 *
 */

#define GENERIC double

extern GENERIC _tbl_atan_hi[], _tbl_atan_lo[];
static GENERIC
huge	=   1.0e37,
one	=   1.0,
p1	=  -3.333333333333257064286879310098319538072e-0001,
p2	=   1.999999999904898469228047944616853734873e-0001,
p3	=  -1.428571389073182094616952569783776070846e-0001,
p4	=   1.111103562698367513143327053952403821435e-0001,
p5	=  -9.083602403665988394837178282435182863463e-0002,
p6	=   7.342719679330981060455285205948978810403e-0002,
q1	=  -3.333333333329642804642056573866203460109e-0001,
q2	=   1.999999918685375261810514494941669902332e-0001,
q3	=  -1.428032339681243145479045405108695633455e-0001,
pio2hi	=   1.570796326794896558e+00,
pio2lo	=   6.123233995736765886e-17;

GENERIC atan(x)
GENERIC x;
{
	GENERIC y,z,r,p,s;
	static dummy();
	long *px = (long*)&x, *py = (long*)&y;
	int i0,i1,ix,iy,sign,j;

    /* determine word ordering: i0 and i1 may be replaced by constant
     * if the floating point word ordering is known
     */
	i0 = 0; i1 = 1;
	if(*(int*)&one==0) {i0=1; i1=0;}

	ix = px[i0];
	sign = ix&0x80000000; ix ^= sign;


    /* for |x| < 1/8 */
	if(ix < 0x3fc00000) {
	    if(ix<0x3f500000) {		/* when |x| < 2**(-prec/6-2) */
	    	if(ix<0x3e300000) {		/* if |x| < 2**(-prec/2-2) */
		    dummy(huge+x);
		    return x;
	    	}
	   	z = x*x;
		if(ix<0x3f100000) {		/* if |x| < 2**(-prec/4-1) */
		    return x+(x*z)*p1;
		} else {			/* if |x| < 2**(-prec/6-2) */
		    return x+(x*z)*(p1+z*p2);
		}
	    }
	    z = x*x;
	    return x+(x*z)*(p1+z*(p2+z*(p3+z*(p4+z*(p5+z*p6)))));
	}

    /* for |x| >= 8.0 */
	if(ix >= 0x40200000) {
	    px[i0] = ix;
	    if(ix < 0x40504000) {		/* x <  65 */
		r = one/x; z = r*r;
		y = r*(one+z* 			/* poly1 */
	    		(p1+z*(p2+z*(p3+z*(p4+z*(p5+z*p6))))));
		y-= pio2lo;
	    } else if(ix <  0x41200000) {	/* x <  2**(prec/3+2) */
		r = one/x; z = r*r;
		y = r*(one+z*(q1+z*(q2+z*q3)))-pio2lo;	/* poly2 */
	    } else if(ix <  0x43600000) {	/* x <  2**(prec+2) */
		y = one/x-pio2lo;
	    } else if(ix <  0x7ff00000) {	/* x <  inf */	
		y = -pio2lo;
	    } else {				/* x is inf or NaN */
		if(((ix-0x7ff00000)|px[i1])!=0) return x-x;
		y = -pio2lo;
	    }

	    if(sign==0) 
		return  pio2hi-y;
	    else 
		return  y-pio2hi;
	}


    /* now x is between 1/8 and 8 */
	px[i0] = ix;
	iy     = (ix+0x00008000)&0x7fff0000;
	py[i0] = iy; py[i1]=0;
	j      = (iy-0x3fc00000)>>16;

	if(sign==0) s = (x-y)/(one+x*y);
	else 	    s = (y-x)/(one+x*y);
	z = s*s;
	if(ix==iy) p = s*(one+z*q1);
	else 	   p = s*(one+z*(q1+z*(q2+z*q3)));
	if(sign==0) {
	    r = p+_tbl_atan_lo[j];
	    return r+_tbl_atan_hi[j];
	} else {
	    r = p-_tbl_atan_lo[j];
	    return r-_tbl_atan_hi[j];
	}
}

static dummy(x)
GENERIC x;
{
	return 0;
}
