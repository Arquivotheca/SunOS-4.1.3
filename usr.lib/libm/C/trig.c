
#ifndef lint
static  char sccsid[] = "@(#)trig.c 1.1 92/07/30 SMI"; 
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* sin(x), cos(x), tan(x), sincos(x,s,c)
 * Coefficients obtained from Peter Tang's "Some Software 
 * Implementations of the Functions Sin and Cos." 
 * Code by K.C. Ng for SUN 4.0, Feb 3, 1987. Revised on April, 1988.
 *
 * Static kernel function:
 *	trig()		... sin, cos, tan, and sincos
 *      argred()        ... huge argument reduction
 *
 * Method.
 *      Let S and C denote the polynomial approximations to sin and cos 
 *      respectively on [-PI/4, +PI/4].
 *      1. Assume the argument x is reduced to y1+y2 = x-k*pi/2 in
 *	   [-pi/2 , +pi/2], and let n = k mod 4.
 *	2. Let S=S(y1+y2), C=C(y1+y2). Depending on n, we have
 *
 *          n        sin(x)      cos(x)        tan(x)
 *     ----------------------------------------------------------
 *	    0	       S	   C		 S/C
 *	    1	       C	  -S		-C/S
 *	    2	      -S	  -C		 S/C
 *	    3	      -C	   S		-C/S
 *     ----------------------------------------------------------
 *
 *   Notes:
 *      1. S = S(y1+y2) is computed by:
 *		y1 + ((y2-0.5*z*y2) - (y1*z)*ss)
 *         where z=y1*y1 and ss is an approximation of (y1-sin(y1))/y1^3.
 *
 *	2. C = C(y1+y2) is computed by:
 *		z  = y1*y1
 *		hz = z*0.5
 *		t  = y1*y2
 *		if(z >= thresh1) 	c = 5/8   - ((hz-3/8 )-(cc-t));
 *		else if (z >= thresh2)  c = 13/16 - ((hz-3/16)-(cc-t));
 *		else 			c = 1     - ( hz      -(cc-t));
 *         where
 *	        cc is an approximation of cos(y1)-1+y1*y1/2 
 *              thresh1 = (acos(3/4)**2)  = 2^-1 * Hex 1.0B70C6D604DD5 
 *              thresh2 = (acos(7/8)**2)  = 2^-2 * Hex 1.0584C22231902 
 *
 *	3. Since S(y1+y2)/C(y1+y2) = tan(y1+y2) ~ tan(y1) + y2 + y1*y1*y2, we 
 *	   have
 *	      S(y1+y2)/C(y1+y2) = S(y1)/C(y1) + y2 + y1*y1*y2 
 *				= [S(y1)+C(y1)*(y2+y1*y1*y2)]/C(y1)
 *	   and hence 
 *	      C(y1+y2)/S(y1+y2) = C(y1)/[S(y1)+C(y1)*(y2+y1*y1*y2)]
 *
 *         For better accuracy, S/C and -C/S are computed as follow. 
 *	   Using the same notation as 1 and 2, 
 *		z  = y1*y1
 *		hz = ysq*0.5
 *		if(z >= thresh1) 	c = 5/8   - ((hz-3/8 )-cc);
 *		else if (z >= thresh2)  c = 13/16 - ((hz-3/16)-cc);
 *		else 			c = 1     - ( hz      -cc);
 *		
 *				     hz - (cc+ss)
 *		 S/C  =  y1 + ( y1*(--------------- +y1*y2) + y2)
 *					 c 
 *
 *		-C/S  = -c/(y1+(y1*ss+c*(y2+z*y2)))
 *
 * Special cases:
 *      Let trig be any of sin, cos, or tan.
 *      trig(+-INF)  is NaN, with signals;
 *      trig(NaN)    is that NaN;
 *
 * Accuracy:
 *	According to fp_pi, computer TRIG(x) returns 
 *		trig(x)    	nearly rounded if fp_pi == 0 =fp_pi_infinte;
 *		trig(x*pi/PI66)	nearly rounded if fp_pi == 1 == fp_pi_66;
 *		trig(x*pi/PI53)	nearly rounded if fp_pi == 2 == fp_pi_53;
 *
 *      In decimal:
 *			pi   = 3.141592653589793 23846264338327 ..... 
 *    		66 bits PI66 = 3.141592653589793 23845859885078 ..... ,
 *    		53 bits PI53 = 3.141592653589793 115997963 ..... ,
 *
 *      In hexadecimal:
 *              	pi   = 3.243F6A8885A308D313198A2E....
 *    		66 bits PI66 = 3.243F6A8885A308D3 =  2 * 1.921FB54442D184698
 *    		53 bits PI53 = 3.243F6A8885A30    =  2 * 1.921FB54442D18    
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

#include <math.h>

enum fp_pi_type fp_pi = fp_pi_66; 	/* initialize to 66 bit PI */
static int argred(),dummy();
static double trig(),sincos_c;

double sin(x)
double x;
{
	return trig(x,0);
}

double cos(x)
double x;
{
	return trig(x,1);
}

double tan(x)
double x;
{
	return trig(x,2);
}

void sincos(x,s,c)
double x,*s,*c;
{
	*s = trig(x,3);
	*c = sincos_c;
}


/*
 * thresh1:  (acos(3/4))**2
 * thresh1:  (acos(7/8))**2
 * invpio2:  53 bits of 2/pi
 * pio4:     53 bits of pi/4
 * pio2:     53 bits of pi/2
 * pio2_1:   first  33 bit of pi/2
 * pio2_1t:  pi/2 - pio2_1
 * pio2_1t5: (53 bit pi)/2 - pio2_1
 * pio2_2:   second 33 bit of pi/2
 * pio2_2t:  pi/2 - pio2_2
 * pio2_3:   third  33 bit of pi/2
 * pio2_3t:  pi/2 - pio2_3
 */
static double   /* see above for the definition of the values */
thresh1 = 0.52234479296242375401249,    /* 2^ -1  * Hex  1.0B70C6D604DD5 */
thresh2 = 0.25538924535466389631466,    /* 2^ -2  * Hex  1.0584C22231902 */
invpio2 = 0.636619772367581343075535,   /* 2^ -1  * Hex  1.45F306DC9C883 */
pio4    = 0.78539816339744830961566,    /* 2^ -1  * Hex  1.921FB54442D18 */
pio2    = 1.57079632679489661923,       /* 2^  0  * Hex  1.921FB54442D18 */
pio2_1  = 1.570796326734125614166,      /* 2^  0  * Hex  1.921FB54400000 */
pio2_1t = 6.077100506506192601475e-11,  /* 2^-34  * Hex  1.0B4611A626331 */
pio2_1t5= 6.077094383272196864709e-11,  /* 2^-34  * Hex  1.0B46000000000 */
pio2_2  = 6.077100506303965976596e-11,  /* 2^-34  * Hex  1.0B4611A600000 */
pio2_2t = 2.022266248795950732400e-21,  /* 2^-69  * Hex  1.3198A2E037073 */
pio2_3  = 2.022266248711166455796e-21,  /* 2^-69  * Hex  1.3198A2E000000 */
pio2_3t = 8.478427660368899643959e-32;  /* 2^-104 * Hex  1.B839A252049C1 */

static double C[] = {
	/* C[n] = coefficients for cos(x)-1+x*x/2 with pi, n=0,...,5 */
 4.16666666666666019E-2    , /*C0 2^ -5 * Hex  1.555555555554C */
-1.38888888888744744E-3    , /*C1 2^-10 * Hex -1.6C16C16C1521F */
 2.48015872896717822E-5    , /*C2 2^-16 * Hex  1.A01A019CBF671 */
-2.75573144009911252E-7    , /*C3 2^-22 * Hex -1.27E4F812B495B */
 2.08757292566166689E-9    , /*C4 2^-29 * Hex  1.1EE9F14CDC590 */
-1.13599319556004135E-11   , /*C5 2^-37 * Hex -1.8FB12BAF59D4B */
 0.0,
 0.0,
	/* C[8+n] = coefficients for cos(x)-1+x*x/2 with PI66, n=0,...,5 */
 4.16666666666666019E-2    , /*C0 2^ -5 * Hex  1.555555555554C */
-1.38888888888744744E-3    , /*C1 2^-10 * Hex -1.6C16C16C1521F */
 2.48015872896717822E-5    , /*C2 2^-16 * Hex  1.A01A019CBF671 */
-2.75573144009911252E-7    , /*C3 2^-22 * Hex -1.27E4F812B495B */
 2.08757292566166689E-9    , /*C4 2^-29 * Hex  1.1EE9F14CDC590 */
-1.13599319556004135E-11   , /*C5 2^-37 * Hex -1.8FB12BAF59D4B */
 0.0,
 0.0,
	/* C[16+n] = coefficients for cos(x)-1+x*x/2 with PI53, n=0,...,5 */
 4.1666666666666504759E-2    , /*C0 2^ -5 * Hex  1.555555555553E */
-1.3888888888865301516E-3    , /*C1 2^-10 * Hex -1.6C16C16C14199 */
 2.4801587269650015769E-5    , /*C2 2^-16 * Hex  1.A01A01971CAEB */
-2.7557304623183959811E-7    , /*C3 2^-22 * Hex -1.27E4F1314AD1A */
 2.0873958177697780076E-9    , /*C4 2^-29 * Hex  1.1EE3B60DDDC8C */
-1.1250289076471311557E-11   , /*C5 2^-37 * Hex -1.8BD5986B2A52E */
 0.0,
 0.0,
};

static double S[] = {
	/* S[n] = coefficients for (x-sin(x))/x with pi, n=0,...,5 */
 1.66666666666666796E-1    , /* S0 2^ -3 * Hex  1.555555555555A */
-8.33333333333178931E-3    , /* S1 2^ -7 * Hex -1.1111111110D97 */
 1.98412698361250482E-4    , /* S2 2^-13 * Hex  1.A01A019E4A9AC */
-2.75573156035895542E-6    , /* S3 2^-19 * Hex -1.71DE37262ECF8 */
 2.50510254394993115E-8    , /* S4 2^-26 * Hex  1.AE5F933569B98 */
-1.59108690260756780E-10   , /* S5 2^-33 * Hex -1.5DE23AD2495F2 */
 0.0,
 0.0,
	/* S[8+n] = coefficients for (x-sin(x))/x with PI66, n=0,...,5 */
 1.66666666666666796E-1    , /* S0 2^ -3 * Hex  1.555555555555A */
-8.33333333333178931E-3    , /* S1 2^ -7 * Hex -1.1111111110D97 */
 1.98412698361250482E-4    , /* S2 2^-13 * Hex  1.A01A019E4A9AC */
-2.75573156035895542E-6    , /* S3 2^-19 * Hex -1.71DE37262ECF8 */
 2.50510254394993115E-8    , /* S4 2^-26 * Hex  1.AE5F933569B98 */
-1.59108690260756780E-10   , /* S5 2^-33 * Hex -1.5DE23AD2495F2 */
 0.0,
 0.0,
	/* S[16+n] = coefficients for (x-sin(x))/x with PI53, n=0,...,5 */
 1.6666666666666463126E-1    , /* S0 2^ -3 * Hex  1.555555555550C */
-8.3333333332992771264E-3    , /* S1 2^ -7 * Hex -1.111111110C461 */
 1.9841269816180999116E-4    , /* S2 2^-13 * Hex  1.A01A019746345 */
-2.7557309793219876880E-6    , /* S3 2^-19 * Hex -1.71DE3209CDCD9 */
 2.5050225177523807003E-8    , /* S4 2^-26 * Hex  1.AE5C0E319A4EF */
-1.5868926979889205164E-10   , /* S5 2^-33 * Hex -1.5CF61DF672B13 */
 0.0,
 0.0,
};

static double			
medium  = 1647099.0,    /* 2^19*(pi/2): threshold for medium arg red */
tiny   	= 1.0e-10,	/* fl(1.0 + tiny*tiny) == 1.0 */
big	= 1.0e10,	/* 1/tiny */
zero	= 0.0,
one 	= 1.0,
half	= 0.5,
c5_8	= 0.625,
c3_8	= 0.375,
c13_16	= 0.8125,
c3_16	= 0.1875;

static double trig(x,k) 
double x; int k;
{
        double y1,y2,t,z,hz,ss,w,cc,fn;
	register j,m;
        int n,signx;
	if(!finite(x))  return sincos_c=x-x;	/* trig(NaN or INF) is NaN */
	signx = signbit(x); t = fabs(x);
	if(t<0.002246) {
	    if (t<tiny) {
		dummy(t+big);	/* create inexact flag if x!=0 */
		if(k==3) {sincos_c = one; return x;}
		return (k==1)? one:x; 
	    }
	    z = x*x;
	    if (t<0.000085) {
		switch(k) {
		case 0: return x - z*x*0.1666666666666666666;
		case 1: return 1 - 0.5*z;
		case 2: return x + z*x*0.3333333333333333333;
		case 3: sincos_c = 1 - 0.5*z;
			return x - z*x*0.1666666666666666666;
		}
	    }
	    switch(k) {
	    case 0:	return 
		x-z*x*(0.1666666666666666666-0.008333333333333333333*z);
	    case 1:	return 
		1-z*(0.5-0.04166666666666666666*z);
	    case 2:	return 
		x+z*x*(0.3333333333333333333+0.133333333333333333333*z);
	    case 3:	sincos_c = 1-z*(0.5-0.04166666666666666666*z); return
		x-z*x*(0.1666666666666666666-0.008333333333333333333*z);

	    }
	}
	n  = j = 0; 	/* j will be used to indicate whether y2=0 */
	y1 = x;
	if(t > pio4) {
	    switch((int)fp_pi) {
		case 0: j = 1;
			if(t<=pio2) {
			    n  = 1;
			    z  = t - pio2_1;
			    y1 = z - pio2_1t;
			    if(fabs(y1)>0.00390625) y2=(z-y1)-pio2_1t;
			    else {
				z -= pio2_2;
				y1 = z - pio2_2t;
				if(fabs(y1)>9.094947e-13) y2=(z-y1)-pio2_2t;
				else {
				    z -= pio2_3;
				    y1 = z - pio2_3t;
				    y2 = (z-y1)-pio2_3t;
				}
			    }
			    if(signx==1) {n= -n; y1= -y1; y2= -y2;}
			} else if(t<=medium) {
			    n  = (int) (t*invpio2+half);
			    fn = n;
			    z  = t - fn*pio2_1;
			    w  = fn*pio2_1t;
			    y1 = z - w;
			    if(fabs(y1)>0.00390625) y2=(z-y1)-w;
			    else {
				z -= fn*pio2_2;
			        w  = fn*pio2_2t;
				y1 = z - w;
				if(fabs(y1)>9.094947e-13) y2=(z-y1)-w;
				else {
				    z -= fn*pio2_3;
				    w  = fn*pio2_3t;
				    y1 = z - w;
				    y2 = (z-y1)-w;
				}
			    }
			    if(signx==1) {n= -n; y1= -y1; y2= -y2;}
			} else n = argred(x,&y1,&y2);
			break;
		case 1: j = 1;
			if(t<=pio2) {
			    n  = 1;
			    z  = t - pio2_1;
			    y1 = z - pio2_2;
			    y2 = (z-y1) - pio2_2;
			    if(signx==1) {n= -n; y1= -y1; y2= -y2;}
			} else if(t<=medium) {
			    n  = (int) (t*invpio2+half);
			    z  = (t - n*pio2_1);
			    w  = n*pio2_2;
			    y1 = z-w;
			    y2 = (z-y1)-w;
			    if(signx==1) {n= -n; y1= -y1; y2= -y2;}
			} else n = argred(x,&y1,&y2);
			break;
		case 2: if(t<=pio2) {
			    n  = 1;
			    y1 = t - pio2;
			    if(signx==1) {n= -1; y1= -y1;}
			} else if(t<=medium) {
			    n  = (int) (t*invpio2+half);
			    y1 = (t - n*pio2_1)-n*pio2_1t5;
			    if(signx==1) {n= -n; y1= -y1;}
			} else {j=1; n = argred(x,&y1,&y2);}
			break;

	    }
	}

    /* compute sin, cos, tan, sincos */ 
	z = y1*y1;
	m  = ((int) fp_pi)<<3;
	if(k<2) { 		/* sin, cos */
	    n += k;
	    if((n&1)==0) {	/* sin on primary range */
		t = y1*z;
		ss=S[m]+z*(S[m+1]+z*(S[m+2]+z*(S[m+3]+z*(S[m+4]+z*S[m+5]))));
		if(j==0) w = y1 - t*ss; else w = y1 - (t*ss -(y2-0.5*(z*y2)));
		return ((n&2)==0)? w: -w;		/* n=0,2 */
	    } else {
		cc = (z*z)*(C[m]+z*(C[m+1]+z*(C[m+2]+
			z*(C[m+3]+z*(C[m+4]+z*C[m+5])))));
		hz = z*half;
		if(j!=0) cc -= y1*y2;
		if     (z>=thresh1) 	w = c5_8  - ((hz-c3_8 )-cc);
		else if(z>=thresh2) 	w = c13_16- ((hz-c3_16)-cc);
		else 			w = one   - (hz        -cc); 
		return ((n&2)==0)? w: -w;		/* n=1,3 */
	    }
	} else {		/* tan,sincos */
	    t =z*z;
	    ss=(S[m]+z*(S[m+1]+z*(S[m+2]+z*(S[m+3]+z*(S[m+4]+z*S[m+5])))));
	    cc=t*(C[m]+z*(C[m+1]+z*(C[m+2]+z*(C[m+3]+z*(C[m+4]+z*C[m+5])))));
	    hz = z*half;

	    if (k==2) {		/* tan */
	        ss *= z;
	        if     (z>=thresh1) 	w = c5_8  - ((hz-c3_8 )-cc);
	        else if(z>=thresh2) 	w = c13_16- ((hz-c3_16)-cc);
	        else 			w = one   - (hz        -cc); 
	        if ((n&1)==0) { 	/*  tan =  sin/cos */
		    if(j==0) return y1+y1*((hz-(ss+cc))/w);	else

    /* return y1+(y1*(y1*y2+(hz-(ss+cc))/w)+y2): rearrange to force ordering */
		    return y1-(y1*(((ss+cc)-hz)/w-y1*y2)-y2);
	        } else {		/*  tan = -cos/sin */
		    if(j==0) return -w/(y1-y1*ss); else
		    return -w/(y1-(y1*ss-w*(y2+y2*z)));
	        }
	    } else {	/* sincos: make sure to get same answers as sin,cos */
	        if(j!=0) cc -= y1*y2;
	        if     (z>=thresh1) 	w = c5_8  - ((hz-c3_8 )-cc);
	        else if(z>=thresh2) 	w = c13_16- ((hz-c3_16)-cc);
	        else 			w = one   - (hz        -cc); 
	        t = y1*z;
	        t = (j==0)? y1 - t*ss: y1 - (t*ss -(y2-hz*y2));
	        switch (n&3) {
		    case 0:  sincos_c = w;  return t;
		    case 1:  sincos_c = -t; return w;
		    case 2:  sincos_c = -w; return -t;
		    default: sincos_c = t;  return -w;
	        }
	    }
	}
}

static int dummy(x)
double x;
{
	return 1;
}

/*
 * huge argument reduction routine
 */
static long p_53[] = {
0xA2F983, 0x6E4E44, 0x16F3C4, 0xEA69B5, 0xD3E131, 0x60E1D2,
0xD7982A, 0xC031F5, 0xD67BCC, 0xFD1375, 0x60919B, 0x3FA0BB,
0x612ABB, 0x714F9B, 0x03DA8A, 0xC05948, 0xD023F4, 0x5AFA37,
0x51631D, 0xCD7A90, 0xC0474A, 0xF6A6F3, 0x1A52E1, 0x5C3927,
0x3ADA45, 0x4E2DB5, 0x64E8C4, 0x274A5B, 0xB74ADC, 0x1E6591,
0x2822BE, 0x4771F5, 0x12A63F, 0x83BD35, 0x2488CA, 0x1FE1BE,
0x42C21A, 0x682569, 0x2AFB91, 0x68ADE1, 0x4A42E5, 0x9BE357,
0xB79675, 0xCE998A, 0x83AF8B, 0xE645E6, 0xDF0789, 0x9E9747,
0xAA15FF, 0x358C3F, 0xAF3141, 0x72A3F7, 0x2BF1D4, 0xF3AD96,
0x7D759F, 0x257FCE, 0x29FB69, 0xB1B42C, 0xC32DE1, 0x8C0BBD,
0x31EC2F, 0x942026, 0x85DCE7, 0x653FF3, 0x136FA7, 0x0D7A5F,
};	/* 396 Hex digits (476 decimal) of 2/PI, PI = 53 bits of pi */

static long p_66[] = {
0xA2F983, 0x6E4E44, 0x152A00, 0x062BC4, 0x0DA276, 0xBED4C1,
0xFDF905, 0x5CD5BA, 0x767CEC, 0x1F80D6, 0xC26053, 0x3A0070,
0x107C2A, 0xF68EE9, 0x687B7A, 0xB990AA, 0x38DE4B, 0x96CFF3,
0x92735E, 0x8B34F6, 0x195BFC, 0x27F88E, 0xA93EC5, 0x3958A5,
0x3E5D13, 0x1C55A8, 0x5B4A8B, 0xA42E04, 0x12D105, 0x35580D,
0xF62347, 0x450900, 0xB98BCA, 0xF7E8A4, 0xA2E5D5, 0x69BC52,
0xF0381D, 0x1A0A88, 0xFE8714, 0x7F6735, 0xBB7D4D, 0xC6F642,
0xB27E80, 0x6191BF, 0xB6B750, 0x52776E, 0xD60FD0, 0x607DCC,
0x68BFAF, 0xED69FC, 0x6EB305, 0xD2557D, 0x25BDFB, 0x3E4AA1,
0x84472D, 0x8B0376, 0xF77740, 0xD290DF, 0x15EC8C, 0x45A5C3,
0x6181EF, 0xC5E7E8, 0xD8909C, 0xF62144, 0x298428, 0x6E5D9D,
};	/* 396 Hex digits (476 decimal) of 2/PI, PI = 66 bits of pi */

static long p_inf[] = {
0xA2F983, 0x6E4E44, 0x1529FC, 0x2757D1, 0xF534DD, 0xC0DB62, 
0x95993C, 0x439041, 0xFE5163, 0xABDEBB, 0xC561B7, 0x246E3A, 
0x424DD2, 0xE00649, 0x2EEA09, 0xD1921C, 0xFE1DEB, 0x1CB129, 
0xA73EE8, 0x8235F5, 0x2EBB44, 0x84E99C, 0x7026B4, 0x5F7E41, 
0x3991D6, 0x398353, 0x39F49C, 0x845F8B, 0xBDF928, 0x3B1FF8, 
0x97FFDE, 0x05980F, 0xEF2F11, 0x8B5A0A, 0x6D1F6D, 0x367ECF, 
0x27CB09, 0xB74F46, 0x3F669E, 0x5FEA2D, 0x7527BA, 0xC7EBE5, 
0xF17B3D, 0x0739F7, 0x8A5292, 0xEA6BFB, 0x5FB11F, 0x8D5D08, 
0x560330, 0x46FC7B, 0x6BABF0, 0xCFBC20, 0x9AF436, 0x1DA9E3, 
0x91615E, 0xE61B08, 0x659985, 0x5F14A0, 0x68408D, 0xFFD880, 
0x4D7327, 0x310606, 0x1556CA, 0x73A8C9, 0x60E27B, 0xC08C6B, 
};	/* 396 Hex digits (476 decimal) of 2/pi */

	/* p_##[i] contains the (24*i)-th to (24*i+23)-th bit
	 * of 2/pi or 2/PI after binary point. The corresponding 
	 * floating value fv[i] = fv(pr[i]) is
	 *	fv[i] = scalbn((double)pr[i],-24*(i+1)).
	 * Note that unless fv[i] = 0, we have
	 *	2**(-24*(i+1)) <= fv[i] < 2**(-24*i).
	 *	2**(-24*(i+1)) <= ulp(fv[i]) .
	 */

/*
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */
static double			
two24  = 16777216.0,
twon24 = 5.9604644775390625E-8,
p1     = 1.5707962512969970703E0,       /* first  24 bit of pi/2 */
p2     = 7.5497894158615963534E-8,      /* second 24 bit of pi/2 */
p3_inf = 5.3903025299577647655E-15,     /* third  24 bit of pi/2 */
p3_66  = 5.3903008358918702569E-15,     /* third  18 bit of pi/2 */
p3_53  = 5.3290705182007513940E-15;     /* third   5 bit of pi/2 */

static int argred(x,y1,y2) 	/* return the last three bits of N */
double x, *y1, *y2;
{
	long *pr;
	double x1,x2,x3,t,t1,t2,t3,p3,y,z,fvm1,fvm2;
	double q[10],fv[10];		
	double q1,q2,q3;
	int signx,k,n,mm=0,n1=1;
	unsigned long *py = (unsigned long *) &y;

    /* determine double ordering */
	if((*(1+(long *)&one))==0x3ff00000) n1=0;

    /* save sign bit and replace x by its absolute value */
	signx = signbit(x); 
	x = fabs(x);
	n = ilogb(x);
    /* Let  x <=| y stands for either y is zero or x is less than the least
     * non-zero significant bit of y. 
     * We will break x into three 24-bit pieces x1+x2+x3.
     * Here we choose k so that 
     * 		2**3 <=| x1*fv[k-1] 
     * where fv[k-1] is the floating value of pr[k-1].
     * Analysis: since 2**(-24*k) <=| fv[k-1] < 2**(-24*(k-1)) and
     *		       2**(n-23)  <=| x1      < 2**(n+1),
     *                 2**(n-23)  <=| x2*2**24< 2**(n+1),
     *                 2**(n-23)  <=| x3*2**48< 2**(n+1),
     * we have
     *		    2**(n-24k-23) <=| x1*fv[k-1] < 2**(n-24k+25).
     *		    2**(n-24k-23) <=| x2*fv[k-2] < 2**(n-24k+25).
     *		    2**(n-24k-23) <=| x3*fv[k-3] < 2**(n-24k+25).
     * Thus 
     *		    2**(n-24k-28) <=| x1*fv[k-1]+x2*fv[k-2]+x3*fv[k-3]
     * Thus n-24k-23 >= 3 or (n-26)/24 >= k.
     */
	k = (n-26)/24;
	if(k<=0) k = 0;			/* k must be >= 0 */
	else x = scalbn(x,-24*k);
    /* break x in three 24-bit pieces */
	y = x;
	py[n1] &= 0xffffffe0; 	/* 48 bit of x */
	x3 = y;
	py[n1] &= 0xe0000000; 	/* 24 bit of x */
	x1 = y;
	x2 = x3-y;
	x3 = x-x3;

    /* convert pr to fv */
	pr = p_inf; p3 = p3_inf;
	if(fp_pi==fp_pi_66) {pr = p_66;p3 = p3_66;}
	if(fp_pi==fp_pi_53) {pr = p_53;p3 = p3_53;}
	if(k>=1) {
	    fvm1 = (double)pr[k-1];
	    if(k> 1) fvm2 = (double)pr[k-2]*two24;
	}
	z     = twon24;
	fv[0] = ((double)pr[k])*z;
	z    *= twon24;
	fv[1] = ((double)pr[k+1])*z;
	z    *= twon24;
	fv[2] = ((double)pr[k+2])*z;
	z    *= twon24;
	fv[3] = ((double)pr[k+3])*z;
	z    *= twon24;
	fv[4] = ((double)pr[k+4])*z;

    /* compute q[0],q[1],...q[4] */
	if (x3==zero) {
	    if (x2==zero) {
		q[0] = x1*fv[0];
		q[1] = x1*fv[1];
		q[2] = x1*fv[2];
		q[3] = x1*fv[3];
		q[4] = x1*fv[4];
	    } else {
		if(k>0) q[0] = x1*fv[0] + x2*fvm1; else q[0] = x1*fv[0];
		q[1] = x1*fv[1]+x2*fv[0];
		q[2] = x1*fv[2]+x2*fv[1];
		q[3] = x1*fv[3]+x2*fv[2];
		q[4] = x1*fv[4]+x2*fv[3];
	    }
	} else {
	    if(k>=1) {
		if(k>1) q[0] = x1*fv[0]+x2*fvm1 +x3*fvm2;
		else    q[0] = x1*fv[0]+x2*fvm1 ;
	    	q[1] = x1*fv[1]+x2*fv[0]+x3*fvm1;
	    } else {
		q[0] = x1*fv[0];
	    	q[1] = x1*fv[1]+x2*fv[0];
	    }
	    q[2] = x1*fv[2]+x2*fv[1]+x3*fv[0];
	    q[3] = x1*fv[3]+x2*fv[2]+x3*fv[1];
	    q[4] = x1*fv[4]+x2*fv[3]+x3*fv[2];
	}

    /* trim off integer part that is bigger than 8 */
	q[0] -= 8.0*aint(q[0]*0.125);
	q[1] -= 8.0*aint(q[1]*0.125);
	n = q[2]*0.125; 
	if(n!=0) q[2] -= (double) (n<<3);

    /* default situation sum q[0] to q[4] */
    /* sum q[i] into t1 and t2 with integer part in mm and t1 < 0.5 */
	t   = q[4]; 
	t  += q[3]; 
	t  += q[2]; 
	t  += q[1]; 
	t  += q[0];
	t2  = q[0]-t; 
	t2 += q[1];
	t2 += q[2];
	t2 += q[3];
	t2 += q[4];
	mm  = (int) t; t -= (double) mm;
	t1  = t+t2;
	t2 += t - t1;
	if(t1>0.5||(t1==0.5&&t2>zero)) {
	    mm += 1;
	    t   = t1-1.0;
	    t1  = t+t2;
	    t2 += t-t1;
	}

    /* may need to re-calculate t1 and t2 */
	if( ilogb(t1)-ilogb(q[3]) < 20 )  {
	    t2  = q[1];
	    t   = q[0] - (double) mm;
	    t1  = t2+t;
	    t2  = q[2]-((t1-t)-t2);
	    t   = t1;
	    t1  = t2+t;
	    t2  = q[3]-((t1-t)-t2);
	    t   = t1;
	    t1  = t2+t;
	    t2  = q[4]-((t1-t)-t2);
	    z  *= twon24;
	    fv[5] = ((double)pr[k+5])*z;
	    if (x3==zero) {
	    	if (x2==zero) 	q[5] = x1*fv[5];
	        else 		q[5] = x1*fv[5]+x2*fv[4];
	    } else   		q[5] = x1*fv[5]+x2*fv[4]+x3*fv[3];
	    if( ilogb(t1)-ilogb(q[4]) >= 20 ) {
		t2 += q[5]; goto final_adjust;
	    } else {
	    	t   = t1;
	    	t1  = t2+t;
	    	t2  = q[5]-((t1-t)-t2);
	    }
	    z  *= twon24;
	    fv[6] = ((double)pr[k+6])*z;
	    if (x3==zero) {
	    	if (x2==zero) 	q[6] = x1*fv[6];
	        else 		q[6] = x1*fv[6]+x2*fv[5];
	    } else   		q[6] = x1*fv[6]+x2*fv[5]+x3*fv[4];
	    if( ilogb(t1)-ilogb(q[5]) >= 20 ) { 
		t2 += q[6]; goto final_adjust;
	    } else {
	    	t   = t1;
	    	t1  = t2+t;
	    	t2  = q[6]-((t1-t)-t2);
	    }
	    z  *= twon24;
	    fv[7] = ((double)pr[k+7])*z;
	    if (x3==zero) {
	    	if (x2==zero) 	q[7] = x1*fv[7];
	        else 		q[7] = x1*fv[7]+x2*fv[6];
	    } else   		q[7] = x1*fv[7]+x2*fv[6]+x3*fv[5];
	    t2 += q[7]; 
	final_adjust:
	    t   = t1;
	    t1  = t2+t;
	    t2  = t2 - (t1-t);
	}

    /* break t1+t2 into 3 pieces t1+t2+t3 with t1 and t2 24-bit */
	z  	 = y  = t1;
	t  	 = t2;
	py[n1]  &= 0xffffffe0;
	t3 	 = y;
	py[n1]  &= 0xe0000000;
	t1 	 = y;
	t2	 = t3 - y ;
	t3 	 = t - (t3 - z);


    /* compute (p1+p2+p3)*(t1+t2+t3) to y1 and y2 */
			/*  p1*t1 p1*t2 p1*t3 */
			/*        p2*t1 p2*t2 */
			/*		p3*t1 */
	q3  = p2*t2+p3*t1;
	q3 += p1*t3;
	q2  = p1*t2+p2*t1;
	q1  = p1*t1;
	t1  = q1+q2;
	t2  = q3 - ((t1-q1)-q2);
	*y1 = t1+t2;
	*y2 = t2 - (*y1 - t1);
	if (signx==1)  {*y1= -(*y1); *y2= -(*y2); mm= -mm;}
	return mm&7;
}
