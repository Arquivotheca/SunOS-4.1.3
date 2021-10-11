
#ifndef lint
static	char sccsid[] = "@(#)exp.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* exp(x)
 * Hybrid alogrithm of Peter Tang's Table driven method (for 
 * large arguments) and rational approximation (for small arguments).
 * Written by K.C. Ng, November 1988.
 * Method (Table driven):
 *	1. Argument Reduction: given the input x, find r and integer k 
 *	   and j such that
 *	             x = (32k+j)*ln2 + r,  |r| <= (1/64)*ln2 .  
 *
 *	2. exp(x) = 2^k * (2^(j/32) + 2^(j/32)*expm1(r)) 
 *	   Note:
 *	   a. expm1(r) is computed by 
 *		(i)  If divide is fast:
 *		     expm1(r) = (2r)/(2-R), R = r - r^2*(t1 + t2*r^2)
 *		(ii) Otherwise (divide is more than 7 multiplications time)
 *		     expm1(r) = r + t1*r^2 + t2*r^3 + ... + t5*r^6
 *	   b. 2^(j/32) is represented as S[j]+S_trail[j] where
 *	      S[j] = 2^(j/32) rounded and S_trail[j] = 2^(j/32) - S[j].
 *	      We also define S2[j]=2*S[j] for computing a(i)'s expm1: 
 *			expm1(r) = (S2[j]*r)/(2.0-R)
 *			
 * Special cases:
 *	exp(INF) is INF, exp(NaN) is NaN;
 *	exp(-INF)=  0;
 *	for finite argument, only exp(0)=1 is exact.
 *
 * Accuracy:
 *	according to an error analysis, the error is always less than
 *	an ulp (unit in the last place).
 *
 * Misc. info.
 *	For IEEE double 
 *		if x >  7.09782712893383973096e+02 then exp(x) overflow
 *		if x < -7.45133219101941108420e+02 then exp(x) underflow
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

#include <math.h>

extern double SVID_libm_err();

static double
threshold1	= 7.09782712893383973096e+02,	/* 0x40862E42, 0xFEFA39EF */
threshold2	= 7.45133219101941108420e+02,	/* 0x40874910, 0xD52D3051 */
huge		= 1.0e30,
one		= 1.0,
ln2_onehalf     = 1.03972077083991796412e+00,   /* 0x3FF0A2B2, 0x3F3BAB73 */
ln2		= 6.93147180559945286227e-01,	/* 0x3fe62e42, 0xfefa39ef */
ln2_2		= 3.46573590279972643113e-01,	/* 0x3fd62e42, 0xfefa39ef */
ln2hi           = 6.93147180369123816490e-01,   /* 0x3fe62e42, 0xfee00000 */
ln2lo           = 1.90821492927058770002e-10,   /* 0x3dea39ef, 0x35793c76 */
ln2_64		= 1.08304246962491450973e-02,   /* 0x3f862e42, 0xfefa39ef */
ln2_32hi	= 2.16608493865351192653e-02,	/* 0x3f962e42, 0xfee00000 */
ln2_32lo	= 5.96317165397058656257e-12,	/* 0x3d9a39ef, 0x35793c76 */
invln2		= 1.44269504088896338700e+00,	/* 0x3ff71547, 0x652b82fe */
invln2_32       = 4.61662413084468283841e+01,   /* 0x40471547, 0x652b82fe */
twom18		= 3.81469726562500000000e-06,	/* 0x3ed00000, 0x00000000 */
twom28		= 3.72529029846191406250e-09,	/* 0x3e300000, 0x00000000 */
twom54		= 5.55111512312578270212e-17;	/* 0x3c900000, 0x00000000 */

/* rational or polynomial approximation on [0,(ln2)/2] */
#ifdef SLOWDIV
static double
p1 = 5.00000000000000000000e-1,
p2 = 1.66666666666666784982e-1,
p3 = 4.16666666666197204123e-2,
p4 = 8.33333333331901796590e-3,
p5 = 1.38888889206249588980e-3,
p6 = 1.98412698941833241823e-4,
p7 = 2.48015161960545453685e-5,
p8 = 2.75572352833991096674e-6,
p9 = 2.76222881768990971035e-7,
p10= 2.51123092975208043742e-8;
#else
static double
p1     =  1.6666666666666601904E-1    , /*Hex  2^-3    *  1.555555555553E */
p2     = -2.7777777777015593384E-3    , /*Hex  2^-9    * -1.6C16C16BEBD93 */
p3     =  6.6137563214379343612E-5    , /*Hex  2^-14   *  1.1566AAF25DE2C */
p4     = -1.6533902205465251539E-6    , /*Hex  2^-20   * -1.BBD41C5D26BF1 */
p5     =  4.1381367970572384604E-8    ; /*Hex  2^-25   *  1.6376972BEA4D0 */
#endif

/* rational or polynomial approximation on [0,(ln2)/64] */
#ifdef SLOWDIV
static double
t1    =   5.0000000000000000000e-1    , /* 3fe0000000000000 */ 
t2    =   1.6666666666526086527e-1    , /* 3fc5555555548f7c */ 
t3    =   4.1666666666226079285e-2    , /* 3fa5555555545d4e */ 
t4    =   8.3333679843421958056e-3    , /* 3f811115b7aa905e */ 
t5    =   1.3888949086377719040e-3    ; /* 3f56c1728d739765 */ 
#else
static double		
t1    =   1.6666666666627465031E-1    , /* 3FC5555555551E29 */
t2    =  -2.7777669763261076298E-3    ; /* BF66C1664A3720A8 */
#endif

static double S[] = {
 1.00000000000000000000e+00	, /* 3FF0000000000000 */
 1.02189714865411662714e+00	, /* 3FF059B0D3158574 */
 1.04427378242741375480e+00	, /* 3FF0B5586CF9890F */
 1.06714040067682369717e+00	, /* 3FF11301D0125B51 */
 1.09050773266525768967e+00	, /* 3FF172B83C7D517B */
 1.11438674259589243221e+00	, /* 3FF1D4873168B9AA */
 1.13878863475669156458e+00	, /* 3FF2387A6E756238 */
 1.16372485877757747552e+00	, /* 3FF29E9DF51FDEE1 */
 1.18920711500272102690e+00	, /* 3FF306FE0A31B715 */
 1.21524735998046895524e+00	, /* 3FF371A7373AA9CB */
 1.24185781207348400201e+00	, /* 3FF3DEA64C123422 */
 1.26905095719173321989e+00	, /* 3FF44E086061892D */
 1.29683955465100964055e+00	, /* 3FF4BFDAD5362A27 */
 1.32523664315974132322e+00	, /* 3FF5342B569D4F82 */
 1.35425554693689265129e+00	, /* 3FF5AB07DD485429 */
 1.38390988196383202258e+00	, /* 3FF6247EB03A5585 */
 1.41421356237309514547e+00	, /* 3FF6A09E667F3BCD */
 1.44518080697704665027e+00	, /* 3FF71F75E8EC5F74 */
 1.47682614593949934623e+00	, /* 3FF7A11473EB0187 */
 1.50916442759342284141e+00	, /* 3FF82589994CCE13 */
 1.54221082540794074411e+00	, /* 3FF8ACE5422AA0DB */
 1.57598084510788649659e+00	, /* 3FF93737B0CDC5E5 */
 1.61049033194925428347e+00	, /* 3FF9C49182A3F090 */
 1.64575547815396494578e+00	, /* 3FFA5503B23E255D */
 1.68179283050742900407e+00	, /* 3FFAE89F995AD3AD */
 1.71861929812247793414e+00	, /* 3FFB7F76F2FB5E47 */
 1.75625216037329945351e+00	, /* 3FFC199BDD85529C */
 1.79470907500310716820e+00	, /* 3FFCB720DCEF9069 */
 1.83400808640934243066e+00	, /* 3FFD5818DCFBA487 */
 1.87416763411029996256e+00	, /* 3FFDFC97337B9B5F */
 1.91520656139714740007e+00	, /* 3FFEA4AFA2A490DA */
 1.95714412417540017941e+00	, /* 3FFF50765B6E4540 */
};

static double S_trail[] = {
 0.00000000000000000000e+00	, 
 5.10922502897344389359e-17	, /* 3C8D73E2A475B465 */
 8.55188970553796365958e-17	, /* 3C98A62E4ADC610A */
-7.89985396684158212226e-17	, /* BC96C51039449B3A */
-3.04678207981247114697e-17	, /* BC819041B9D78A76 */
 1.04102784568455709549e-16	, /* 3C9E016E00A2643C */
 8.91281267602540777782e-17	, /* 3C99B07EB6C70573 */
 3.82920483692409349872e-17	, /* 3C8612E8AFAD1255 */
 3.98201523146564611098e-17	, /* 3C86F46AD23182E4 */
-7.71263069268148813091e-17	, /* BC963AEABF42EAE2 */
 4.65802759183693679123e-17	, /* 3C8ADA0911F09EBC */
 2.66793213134218609523e-18	, /* 3C489B7A04EF80D0 */
 2.53825027948883149593e-17	, /* 3C7D4397AFEC42E2 */
-2.85873121003886075697e-17	, /* BC807ABE1DB13CAC */
 7.70094837980298946162e-17	, /* 3C96324C054647AD */
-6.77051165879478628716e-17	, /* BC9383C17E40B497 */
-9.66729331345291345105e-17	, /* BC9BDD3413B26456 */
-3.02375813499398731940e-17	, /* BC816E4786887A99 */
-3.48399455689279579579e-17	, /* BC841577EE04992F */
-1.01645532775429503911e-16	, /* BC9D4C1DD41532D8 */
 7.94983480969762085616e-17	, /* 3C96E9F156864B27 */
-1.01369164712783039808e-17	, /* BC675FC781B57EBC */
 2.47071925697978878522e-17	, /* 3C7C7C46B071F2BE */
-1.01256799136747726038e-16	, /* BC9D2F6EDB8D41E1 */
 8.19901002058149652013e-17	, /* 3C97A1CD345DCC81 */
-1.85138041826311098821e-17	, /* BC75584F7E54AC3B */
 2.96014069544887330703e-17	, /* 3C811065895048DD */
 1.82274584279120867698e-17	, /* 3C7503CBD1E949DB */
 3.28310722424562658722e-17	, /* 3C82ED02D75B3706 */
-6.12276341300414256164e-17	, /* BC91A5CD4F184B5C */
-1.06199460561959626376e-16	, /* BC9E9C23179C2893 */
 8.96076779103666776760e-17	, /* 3C99D3E12DD8A18B */
};

static double S2[] = {
 2.00000000000000000000e+00	, /* 4000000000000000 */
 2.04379429730823325428e+00	, /* 400059B0D3158574 */
 2.08854756485482750961e+00	, /* 4000B5586CF9890F */
 2.13428080135364739434e+00	, /* 40011301D0125B51 */
 2.18101546533051537935e+00	, /* 400172B83C7D517B */
 2.22877348519178486441e+00	, /* 4001D4873168B9AA */
 2.27757726951338312915e+00	, /* 4002387A6E756238 */
 2.32744971755515495104e+00	, /* 40029E9DF51FDEE1 */
 2.37841423000544205379e+00	, /* 400306FE0A31B715 */
 2.43049471996093791049e+00	, /* 400371A7373AA9CB */
 2.48371562414696800403e+00	, /* 4003DEA64C123422 */
 2.53810191438346643977e+00	, /* 40044E086061892D */
 2.59367910930201928110e+00	, /* 4004BFDAD5362A27 */
 2.65047328631948264643e+00	, /* 4005342B569D4F82 */
 2.70851109387378530258e+00	, /* 4005AB07DD485429 */
 2.76781976392766404516e+00	, /* 4006247EB03A5585 */
 2.82842712474619029095e+00	, /* 4006A09E667F3BCD */
 2.89036161395409330055e+00	, /* 40071F75E8EC5F74 */
 2.95365229187899869245e+00	, /* 4007A11473EB0187 */
 3.01832885518684568282e+00	, /* 40082589994CCE13 */
 3.08442165081588148823e+00	, /* 4008ACE5422AA0DB */
 3.15196169021577299318e+00	, /* 40093737B0CDC5E5 */
 3.22098066389850856694e+00	, /* 4009C49182A3F090 */
 3.29151095630792989155e+00	, /* 400A5503B23E255D */
 3.36358566101485800814e+00	, /* 400AE89F995AD3AD */
 3.43723859624495586829e+00	, /* 400B7F76F2FB5E47 */
 3.51250432074659890702e+00	, /* 400C199BDD85529C */
 3.58941815000621433640e+00	, /* 400CB720DCEF9069 */
 3.66801617281868486131e+00	, /* 400D5818DCFBA487 */
 3.74833526822059992512e+00	, /* 400DFC97337B9B5F */
 3.83041312279429480014e+00	, /* 400EA4AFA2A490DA */
 3.91428824835080035882e+00	, /* 400F50765B6E4540 */
};

double exp(x)
double x;
{
	double y,z,c,t,hi,lo;
	int k,xsb,j,m;

	y = fabs(x);

    /* filter out non-finite arugment */
	if(!finite(x)) {
	    if(x!=x) return x+x; else return (signbit(x)==0)? x:0.0;
	}

	if(y > ln2_64) {
	    xsb = signbit(x);
	    if (y<=ln2_onehalf) {
		k=0;
	        if(y>ln2_2) 
		    if(xsb==0)
			{hi = x - ln2hi; lo =  ln2lo; x = hi - lo; k =  1;}
		    else
			{hi = x + ln2hi; lo = -ln2lo; x = hi - lo; k = -1;}
		t  = x*x;
#ifdef SLOWDIV
		c  = (-t)*(p1+x*(p2+x*(p3+x*(p4+x*(p5+
			x*(p6+x*(p7+x*(p8+x*(p9+x*p10)))))))));
		if(k==0) return one-(c-x); 
		else {
		    y = one-((lo+c)-hi);
#else
		c  = x - t*(p1+t*(p2+t*(p3+t*(p4+t*p5))));
		if(k==0) return one-((x*c)/(c-2.0)-x); 
		else {
		    y = one-((lo-(x*c)/(2.0-c))-hi);
#endif
		    if(k==1) return y+y; else return 0.5*y;
		}
	    } else if(y>((xsb==0)?threshold1:threshold2)) {
		if(xsb==0) return SVID_libm_err(x,x,6); /* overflow */
		else 	   return SVID_libm_err(x,x,7); /* underflow */
	    } else {
		k  = invln2_32*x+((xsb==0)?0.5:-0.5);
		j  = k&0x1f; m = k>>5;
		t  = k;
		z  = (x - t*ln2_32hi) - t*ln2_32lo;
	    }
	} else if(y < twom18) {
	    dummy(x+huge);	/* raise inexact flags */
	    if(y < twom28)
		return one+x;
	    else
		return one+x*(one+0.5*x);
	} else {
	    z = x;
	    m = 0;
	    j = 0;
	}

    /* z is now in primary range */
#ifdef SLOWDIV
	t = (-z)*(1.0+z*(t1+z*(t2+z*(t3+z*(t4+z*t5)))));
	y = S[j]-(S[j]*t-S_trail[j]);
#else
	t = z*z;
	y = S[j]-((S2[j]*z)/((z-t*(t1+t*t2))-2.0)-S_trail[j]);
#endif
	if(m==0) 	return y; 
	else if (m < -1021)	
		return twom54*scalbn(y,m+54); 	/* subnormal output */
	else
		return scalbn(y,m);		/* guaranteed normal output */
}

static dummy(x)
double x;
{
	return 0;
}
