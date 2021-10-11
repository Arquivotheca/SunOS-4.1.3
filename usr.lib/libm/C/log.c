
#ifdef sccsid
static	char sccsid[] = "@(#)log.c 1.1 92/07/30 SMI"; 
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/* log(x)
 * Table look-up algorithm
 * By K.C. Ng, Jan 17, 1989
 *
 * (a). For x in [31/33,33/31], using a special approximation:
 *	f = x - 1;
 *	s = f/(2.0+f);		... here |s| <= 0.03125
 *	z = s*s;
 *	return f-s*(f-z*(B1+z*(B2+z*(B3+z*B4))));
 *
 * (b). For 0.09375 <= x < 23.5
 *	Use an 8-bit table look-up (3-bit for exponent and 5 bit for
 *	significand): find a y that match x to 5.5 significand bits,
 *	then 
 *	    log(x) = log(y*(x/y)) = log(y) + log(x/y). 
 *	Here the leading and trailing values of log(y) are obtained from 
 *	a size-512 table. For log(x/y), let s = (x-y)/(x+y), then
 *	    log(x/y) = log((1+s)/(1-s)) = 2s + 2/3 s^3 + 2/5 s^5 +...
 *	Note that |s|<2**-7=0.0078125. We use an odd s-polynomial 
 *	approximation to compute log(x/y).
 *
 * (c). Otherwise, get "n", the exponent of x, and then normalize x to 
 *	z in [1,2). Then similar to (b) find a y that matches z to 5.5
 *	significant bits. Then
 *	    log(x) = n*ln2 + log(y) + log(z/y).
 *	Here log(y) is obtained from a size-32 table, log(z/y) is 
 *	computed by the same s-polynomial as in (b) (s = (z-y)/(z+y)).
 *
 * Special cases:
 *	log(x) is NaN with signal if x < 0 (including -INF) ; 
 *	log(+INF) is +INF; log(0) is -INF with signal;
 *	log(NaN) is that NaN with no signal.
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

extern double SVID_libm_err(),_tbl_log_hi[],_tbl_log_lo[];

static double
zero	=   0.0,
one_third = 0.333333333333333333333333,
half	=   0.5,
one	=   1.0,
two	=   2.0,
two52   =   4503599627370496.0,
ln2hi	=   6.93147180369123816490e-01,   /* 0x3fe62e42, 0xfee00000 */
ln2lo	=   1.90821492927058770002e-10,   /* 0x3dea39ef, 0x35793c76 */
A1	=   6.666666666674771727490195679289113159045e-0001,
A2	=   3.999999826873793772977541401396094405837e-0001,
A3	=   2.858255498773828156474290815531352737802e-0001,
B1	=   6.666666666666550986298925277570199758720e-0001,
B2	=   4.000000001150663891470403033013455029869e-0001,
B3	=   2.857139299728113781995522584194355262443e-0001,
B4	=   2.226555769734402660003547103954600874552e-0001;

static double TBL[] = {
 0.00000000000000000e+00, 3.07716586667536873e-02, 6.06246218164348399e-02,
 8.96121586896871380e-02, 1.17783035656383456e-01, 1.45182009844497889e-01,
 1.71850256926659228e-01, 1.97825743329919868e-01, 2.23143551314209765e-01,
 2.47836163904581269e-01, 2.71933715483641758e-01, 2.95464212893835898e-01,
 3.18453731118534589e-01, 3.40926586970593193e-01, 3.62905493689368475e-01,
 3.84411698910332056e-01, 4.05465108108164385e-01, 4.26084395310900088e-01,
 4.46287102628419530e-01, 4.66089729924599239e-01, 4.85507815781700824e-01,
 5.04556010752395312e-01, 5.23248143764547868e-01, 5.41597282432744409e-01,
 5.59615787935422659e-01, 5.77315365034823613e-01, 5.94707107746692776e-01,
 6.11801541105992941e-01, 6.28608659422374094e-01, 6.45137961373584701e-01,
 6.61398482245365016e-01, 6.77398823591806143e-01,
};

double log(x)
double x;
{
	double f,s,z,dn;
	long *px = (long*)&x;
	long *pz = (long*)&z;
	int i,j,k,ix,i0,i1,n;

    /* get double precision word ordering */
	if(*(int*)&one == 0) {i0=1;i1=0;} else {i0=0;i1=1;}	

	n  = 0;
	ix = px[i0];
	if(ix> 0x3fee0f8e) {		/* if x >  31/33 */
	    if(ix< 0x3ff10842) {		/* if x < 33/31 */
		f=x-one;
		if(((ix+4)&(~7))==0x3ff00000) {	/* -2^-19<x-1<2^-18 */
		    z = f*f;
		    if(((ix-0x3ff00000)|px[i1])==0) return zero;/*log(1)= +0*/
		    return f - z*(half-one_third*f);
		}
		s = f/(two+f);		/* |s| <= 0.03125 */
		i = (ix-0x3fefe000)|(0x3ff00fff-ix); /* |x-1|<=2**-8 ? */
		z = s*s;
		if (i>=0) return f-s*(f-z*(B1+z*B2));
		return f-s*(f-z*(B1+z*(B2+z*(B3+z*B4))));
	    }
	    if(ix>=0x40378000) {
		if(ix>=0x7ff00000) return x+x;	/* x is +inf or NaN */
		goto LARGE_N;
	    }
	    goto SMALL_N;
	}
	if(ix>=0x3fb80000) goto SMALL_N;
	if(ix>=0x00100000) goto LARGE_N;
	i = ix&0x7fffffff;
	if((i|px[i1])==0) return SVID_libm_err(x,x,16);	/* log(0.0) = -inf */
	if(ix<0) {
	    if(ix>=0xfff00000) return x-x;	/* x is -inf or NaN */
	    return SVID_libm_err(x,x,17);	/* log(x<0) is NaN  */
	}
    /* subnormal x */
	x *= two52; n = -52;  ix = px[i0];
LARGE_N:
	n  += ((ix+0x4000)>>20)-0x3ff;
	ix = (ix&0x000fffff)|0x3ff00000;	/* scale x to [1,2] */
	px[i0] = ix;
	i   = ix + 0x4000;
	pz[i0] = i&0xffff8000; pz[i1]=0;
	s = (x-z)/(x+z);
	j = (i>>15)&0x1f;
	z = s*s;
	dn = (double) n;
	f  = dn*ln2lo+s*(two+z*(A1+z*(A2+z*A3)));
	f += TBL[j];
	return dn*ln2hi+f;
SMALL_N:
	pz[i0] = (ix+0x4000)&0xffff8000; pz[i1]=0;
	s = (x-z)/(x+z);
	j = (ix-0x3fb7c000)>>15;	/* combine -(0x4000-0x3fb80000) */
	z = s*s;
	f = _tbl_log_lo[j]+s*(two+z*(A1+z*(A2+z*A3)));
	return _tbl_log_hi[j]+f;
}
