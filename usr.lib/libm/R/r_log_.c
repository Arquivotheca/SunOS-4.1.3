
#ifdef sccsid
static	char sccsid[] = "@(#)r_log_.c 1.1 92/07/30 SMI"; 
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/* r_log_(x) 
 * Single precision log;
 * Table look-up algorithm
 * By K.C. Ng, Jan 18, 1989.
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
#include <math.h>

extern float _tbl_r_log_hi[], _tbl_r_log_lo[];

static float
zero	=   0.0,
one	=   1.0,
two	=   2.0,
two24   =   16777216.0,
ln2hi	=   6.931457519531250e-1,   /* 0x3fe17200 */
ln2lo	=   1.428606820286227e-6,   /* 0x35bfbe8e */
Bp1	=  -4.999996375804685823573958549562299962948e-0001,
Bp2	=   3.332876052778867563867062788080136631742e-0001,
Bp3	=  -2.481956765240820685943090256285822729909e-0001,
Bp4	=   1.714558145040048040035004076793152347174e-0001,
Bn1	=  -4.999994759413434573585341492223685712068e-0001,
Bn2	=   3.333975294998482570262574626287250035353e-0001,
Bn3	=  -2.475706542840484266818244545672477946459e-0001,
Bn4	=   2.355982881354679546528538019239950233010e-0001,
A1	=   6.667515563167808670229962750219736640405e-0001;

static float TBL[] = {
 0.000000000e+00, 3.077165782e-02, 6.062462181e-02, 8.961215615e-02,
 1.177830324e-01, 1.451820135e-01, 1.718502641e-01, 1.978257447e-01,
 2.231435478e-01, 2.478361577e-01, 2.719337046e-01, 2.954642177e-01,
 3.184537292e-01, 3.409265876e-01, 3.629055023e-01, 3.844116926e-01,
 4.054650962e-01, 4.260843992e-01, 4.462870955e-01, 4.660897255e-01,
 4.855078161e-01, 5.045560002e-01, 5.232481360e-01, 5.415973067e-01,
 5.596157908e-01, 5.773153901e-01, 5.947071314e-01, 6.118015647e-01,
 6.286086440e-01, 6.451379657e-01, 6.613984704e-01, 6.773988008e-01,
};

FLOATFUNCTIONTYPE r_log_(x)
float  *x;
{
	float fx,f,s,z,fn;
	long *px = (long*)&fx;
	long *pz = (long*)&z;
	int i,j,ix,n;

	n  = 0;
	fx = *x;
	ix = *(long*)x;
	if(ix>=0x3f6c0000) {	/* if x >= 1-0.078125 */
	    if(ix< 0x3f8a0000) {		/* if x < 1+0.078125 */
		f= fx-one;
		if(ix==0x3f800000) RETURNFLOAT(zero);
		z = f*f;
		if(ix>0x3f800000) {
		    if(ix<0x3f804000) {
			if(ix<0x3f800400)	/* 0 < f < 2**-13 */
			    fx = f + z*Bp1;
			else 			/* 0 < f < 2**-9  */
			    fx = f + z*(Bp1+f*Bp2);
		    } else 
		    	fx = f + z*(Bp1+f*(Bp2+f*(Bp3+f*Bp4)));
		} else {
		    if(ix>0x3f7f8000) {
			if(ix>0x3f7ff800)	/* -2**-13 < f < 0 */
			    fx = f + z*Bn1;
			else 			/* -2**-9  < f < 0 */
			    fx = f + z*(Bn1+f*Bn2);
		    } else 
		    	fx = f + z*(Bn1+f*(Bn2+f*(Bn3+f*Bn4)));
		}
		RETURNFLOAT(fx);
	    }
	    if(ix>=0x41bc0000) {
		if(ix>=0x7f800000) {
		    fx += fx;
		    RETURNFLOAT(fx); 	/* x is +inf or NaN */
		}
		goto LARGE_N;
	    }
	    goto SMALL_N;
	}
	if(ix>=0x3dc00000) goto SMALL_N;
	if(ix>=0x00800000) goto LARGE_N;
	i = ix&0x7fffffff;
	if(i==0) { 	/* log(0)= -inf */
	    fx = -one/zero;
	    RETURNFLOAT (fx);
	}
	if(ix<0) {
	    if(ix>=0xff800000) fx -= fx;	/* x is -inf or NaN */
	    else fx = zero/zero;		/* log(x<0) is NaN  */
	    RETURNFLOAT (fx);
	}
    /* subnormal x */
	fx *= two24; n = -24;  ix = px[0];
LARGE_N:
	n  += ((ix+0x20000)>>23)-0x7f;
	ix = (ix&0x007fffff)|0x3f800000;	/* scale x to [1,2] */
	px[0] = ix;
	i   = ix + 0x20000;
	pz[0] = i&0xfffc0000;
	s  = (fx-z)/(fx+z);
	j  = (i>>18)&0x1f;
	z  = s*s;
	fn = (float)n;
	f  = fn*ln2lo+s*(two+z*A1);
	f += TBL[j];
	fx = fn*ln2hi+f;
	RETURNFLOAT(fx);
SMALL_N:
	pz[0] = (ix+0x20000)&0xfffc0000; 
	s = (fx-z)/(fx+z);
	j = (ix-0x3dbe0000)>>18;	/* combine -(0x20000-0x3dc00000) */
	z = s*s;
	f = _tbl_r_log_lo[j]+s*(two+z*A1);
	fx = _tbl_r_log_hi[j]+f;
	RETURNFLOAT(fx);
}
