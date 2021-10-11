
#ifndef lint
static	char sccsid[] = "@(#)bessel.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * floating point Bessel's function of the first and second kinds of order 
 * zero: j0(x),y0(x); of order one: j1(x), y1(x); of order n: jn(n,x),yn(n,x).
 * Code originated from 4.3bsd.
 * Modified by K.C. Ng for SUN 4.0 libm.
 *          
 * Special cases:
 *	y0(0)=y1(0)=yn(n,0) = -inf with division by zero signal;
 *	y0(-ve)=y1(-ve)=yn(n,-ve) are NaN with invalid signal.
 *
 * Note 1. About j0,j1,y0,y1:
 *	There is a niggling bug in J0 and J1 which
 *	causes errors up to 2e-16 for x in the
 *	interval [-8,8].
 *	The bug is caused by an inappropriate order
 *	of summation of the series.  rhm will fix it
 *	someday.
 *	Coefficients are from Hart & Cheney.
 *	#5849 (19.22D)		... for j0
 *	#6549 (19.25D)		... for j0
 *	#6949 (19.41D)		... for j0
 *	#6245 (18.78D)		... for y0
 *	#6549 (19.25D)		... for y0
 *	#6949 (19.41D)		... for y0
 *	#6050 (20.98D)		... for j1
 *	#6750 (19.19D)		... for j1
 *	#7150 (19.35D)		... for j1
 *	#6447 (22.18D)		... for y1
 *	#6750 (19.19D)		... for y1
 *	#7150 (19.35D)		... for y1
 *
 * Note 2. About jn(n,x), yn(n,x)
 *	For n=0, j0(x) is called,
 *	for n=1, j1(x) is called,
 *	for n<x, forward recursion us used starting
 *	from values of j0(x) and j1(x).
 *	for n>x, a continued fraction approximation to
 *	j(n,x)/j(n-1,x) is evaluated and then backward
 *	recursion is used starting from a supposed value
 *	for j(n,x). The resulting value of j(0,x) is
 *	compared with the actual value to correct the
 *	supposed value of j(n,x).
 *
 *	yn(n,x) is similar in all respects, except
 *	that forward recursion is used for all
 *	values of n>1.
 *	
 */

#include <math.h>
#include "libm.h"
static double zero = 0.e0;
static double pzero, qzero;
static double tpi	= .6366197723675813430755350535e0;
static double pio4	= .7853981633974483096156608458e0;
static double p1[] = {
	0.4933787251794133561816813446e21,
	-.1179157629107610536038440800e21,
	0.6382059341072356562289432465e19,
	-.1367620353088171386865416609e18,
	0.1434354939140344111664316553e16,
	-.8085222034853793871199468171e13,
	0.2507158285536881945555156435e11,
	-.4050412371833132706360663322e8,
	0.2685786856980014981415848441e5,
};
static double q1[] = {
	0.4933787251794133562113278438e21,
	0.5428918384092285160200195092e19,
	0.3024635616709462698627330784e17,
	0.1127756739679798507056031594e15,
	0.3123043114941213172572469442e12,
	0.6699987672982239671814028660e9,
	0.1114636098462985378182402543e7,
	0.1363063652328970604442810507e4,
	1.0
};
static double p2[] = {
	0.5393485083869438325262122897e7,
	0.1233238476817638145232406055e8,
	0.8413041456550439208464315611e7,
	0.2016135283049983642487182349e7,
	0.1539826532623911470917825993e6,
	0.2485271928957404011288128951e4,
	0.0,
};
static double q2[] = {
	0.5393485083869438325560444960e7,
	0.1233831022786324960844856182e8,
	0.8426449050629797331554404810e7,
	0.2025066801570134013891035236e7,
	0.1560017276940030940592769933e6,
	0.2615700736920839685159081813e4,
	1.0,
};
static double p3[] = {
	-.3984617357595222463506790588e4,
	-.1038141698748464093880530341e5,
	-.8239066313485606568803548860e4,
	-.2365956170779108192723612816e4,
	-.2262630641933704113967255053e3,
	-.4887199395841261531199129300e1,
	0.0,
};
static double q3[] = {
	0.2550155108860942382983170882e6,
	0.6667454239319826986004038103e6,
	0.5332913634216897168722255057e6,
	0.1560213206679291652539287109e6,
	0.1570489191515395519392882766e5,
	0.4087714673983499223402830260e3,
	1.0,
};
static double p4[] = {
	-.2750286678629109583701933175e20,
	0.6587473275719554925999402049e20,
	-.5247065581112764941297350814e19,
	0.1375624316399344078571335453e18,
	-.1648605817185729473122082537e16,
	0.1025520859686394284509167421e14,
	-.3436371222979040378171030138e11,
	0.5915213465686889654273830069e8,
	-.4137035497933148554125235152e5,
};
static double q4[] = {
	0.3726458838986165881989980e21,
	0.4192417043410839973904769661e19,
	0.2392883043499781857439356652e17,
	0.9162038034075185262489147968e14,
	0.2613065755041081249568482092e12,
	0.5795122640700729537480087915e9,
	0.1001702641288906265666651753e7,
	0.1282452772478993804176329391e4,
	1.0,
};

double
j0(arg) double arg;{
	double argsq, n, d;
	double sin(), cos(), sqrt();
	int i;

	if(isnan(arg)) return arg+arg;
	if(arg < 0.) arg = -arg;
	if(arg > 8.){
		if(!finite(arg)) return 0.0;
		asympt(arg);
		n = arg - pio4;
		return(sqrt(tpi/arg)*(pzero*cos(n) - qzero*sin(n)));
	}
	argsq = arg*arg;
	for(n=0,d=0,i=8;i>=0;i--){
		n = n*argsq + p1[i];
		d = d*argsq + q1[i];
	}
	return(n/d);
}

double
y0(arg) double arg;{
	double argsq, n, d;
	double sin(), cos(), sqrt(), log(), j0();
	int i;

	if(isnan(arg)) return arg+arg;
	if(arg <= 0.){
		if(arg==0) {
		/*	d= -1.0/(arg-arg); */
			return SVID_libm_err(arg,arg,8);
		} else {
		/* 	d = (arg-arg)/(arg-arg); */
			return SVID_libm_err(arg,arg,9);
		}
	}
	if(arg > 8.){
		if(!finite(arg)) return 0.0;
		asympt(arg);
		n = arg - pio4;
		return(sqrt(tpi/arg)*(pzero*sin(n) + qzero*cos(n)));
	}
	argsq = arg*arg;
	for(n=0,d=0,i=8;i>=0;i--){
		n = n*argsq + p4[i];
		d = d*argsq + q4[i];
	}
	return(n/d + tpi*j0(arg)*log(arg));
}

static
asympt(arg) double arg;{
	double zsq, n, d;
	int i;
	zsq = 64./(arg*arg);
	for(n=0,d=0,i=6;i>=0;i--){
		n = n*zsq + p2[i];
		d = d*zsq + q2[i];
	}
	pzero = n/d;
	for(n=0,d=0,i=6;i>=0;i--){
		n = n*zsq + p3[i];
		d = d*zsq + q3[i];
	}
	qzero = (8./arg)*(n/d);
}

/* coefficients for j1,y1 */
static double xp1[] = {
	0.581199354001606143928050809e21,
	-.6672106568924916298020941484e20,
	0.2316433580634002297931815435e19,
	-.3588817569910106050743641413e17,
	0.2908795263834775409737601689e15,
	-.1322983480332126453125473247e13,
	0.3413234182301700539091292655e10,
	-.4695753530642995859767162166e7,
	0.2701122710892323414856790990e4,
};
static double xq1[] = {
	0.1162398708003212287858529400e22,
	0.1185770712190320999837113348e20,
	0.6092061398917521746105196863e17,
	0.2081661221307607351240184229e15,
	0.5243710262167649715406728642e12,
	0.1013863514358673989967045588e10,
	0.1501793594998585505921097578e7,
	0.1606931573481487801970916749e4,
	1.0,
};
static double xp2[] = {
	-.4435757816794127857114720794e7,
	-.9942246505077641195658377899e7,
	-.6603373248364939109255245434e7,
	-.1523529351181137383255105722e7,
	-.1098240554345934672737413139e6,
	-.1611616644324610116477412898e4,
	0.0,
};
static double xq2[] = {
	-.4435757816794127856828016962e7,
	-.9934124389934585658967556309e7,
	-.6585339479723087072826915069e7,
	-.1511809506634160881644546358e7,
	-.1072638599110382011903063867e6,
	-.1455009440190496182453565068e4,
	1.0,
};
static double xp3[] = {
	0.3322091340985722351859704442e5,
	0.8514516067533570196555001171e5,
	0.6617883658127083517939992166e5,
	0.1849426287322386679652009819e5,
	0.1706375429020768002061283546e4,
	0.3526513384663603218592175580e2,
	0.0,
};
static double xq3[] = {
	0.7087128194102874357377502472e6,
	0.1819458042243997298924553839e7,
	0.1419460669603720892855755253e7,
	0.4002944358226697511708610813e6,
	0.3789022974577220264142952256e5,
	0.8638367769604990967475517183e3,
	1.0,
};
static double xp4[] = {
	-.9963753424306922225996744354e23,
	0.2655473831434854326894248968e23,
	-.1212297555414509577913561535e22,
	0.2193107339917797592111427556e20,
	-.1965887462722140658820322248e18,
	0.9569930239921683481121552788e15,
	-.2580681702194450950541426399e13,
	0.3639488548124002058278999428e10,
	-.2108847540133123652824139923e7,
	0.0,
};
static double xq4[] = {
	0.5082067366941243245314424152e24,
	0.5435310377188854170800653097e22,
	0.2954987935897148674290758119e20,
	0.1082258259408819552553850180e18,
	0.2976632125647276729292742282e15,
	0.6465340881265275571961681500e12,
	0.1128686837169442121732366891e10,
	0.1563282754899580604737366452e7,
	0.1612361029677000859332072312e4,
	1.0,
};

double
j1(arg) double arg;{
	double xsq, n, d, x;
	double sin(), cos(), sqrt();
	int i;

	if(isnan(arg)) return arg+arg;
	x = arg;
	if(x < 0.) x = -x;
	if(x > 8.){
		if(!finite(arg)) return 1.0/arg;
		xasympt(x);
		n = x - 3.*pio4;
		n = sqrt(tpi/x)*(pzero*cos(n) - qzero*sin(n));
		if(arg <0.) n = -n;
		return(n);
	}
	xsq = x*x;
	for(n=0,d=0,i=8;i>=0;i--){
		n = n*xsq + xp1[i];
		d = d*xsq + xq1[i];
	}
	return(arg*n/d);
}

double
y1(arg) double arg;{
	double xsq, n, d, x;
	double sin(), cos(), sqrt(), log(), j1();
	int i;

	if(isnan(arg)) return arg+arg;
	x = arg;
	if(x <= 0.){
		if(arg==0) {
		/*	d= -1.0/(arg-arg); */
			return SVID_libm_err(arg,arg,10);
		} else {
		/*	d = (arg-arg)/(arg-arg); */
			return SVID_libm_err(arg,arg,11);
		}
	}
	if(x > 8.){
		if(!finite(arg)) return 0.0;
		xasympt(x);
		n = x - 3*pio4;
		return(sqrt(tpi/x)*(pzero*sin(n) + qzero*cos(n)));
	}
	xsq = x*x;
	for(n=0,d=0,i=9;i>=0;i--){
		n = n*xsq + xp4[i];
		d = d*xsq + xq4[i];
	}
	return(x*n/d + tpi*(j1(x)*log(x)-1./x));
}

static
xasympt(arg) double arg;{
	double zsq, n, d;
	int i;
	zsq = 64./(arg*arg);
	for(n=0,d=0,i=6;i>=0;i--){
		n = n*zsq + xp2[i];
		d = d*zsq + xq2[i];
	}
	pzero = n/d;
	for(n=0,d=0,i=6;i>=0;i--){
		n = n*zsq + xp3[i];
		d = d*zsq + xq3[i];
	}
	qzero = (8./arg)*(n/d);
}


double
jn(n,x) int n; double x;{
	int i;
	double a, b, temp;
	double xsq, t;
	double j0(), j1();

	if(n<0){
		n = -n;
		x = -x;
	}
	if(n==0) return(j0(x));
	if(n==1) return(j1(x));
	if(x!=x) return x+x;
	if(x == 0.||!finite(x)) return(0.);
	if(n>x) goto recurs;

	a = j0(x);
	b = j1(x);
	for(i=1;i<n;i++){
		temp = b;
		b = (2.*i/x)*b - a;
		a = temp;
	}
	return(b);

recurs:
	xsq = x*x;
	for(t=0,i=n+16;i>n;i--){
		t = xsq/(2.*i - t);
	}
	t = x/(2.*n-t);

	a = t;
	b = 1;
	for(i=n-1;i>0;i--){
		temp = b;
		b = (2.*i/x)*b - a;
		a = temp;
	}
	return(t*j0(x)/b);
}

double yn(n,x) 
int n; double x;{
	int i;
	int sign;
	double a, b, temp;
	double y0(), y1();

	if(x!=x) return x+x;
	if (x <= 0) {
		if(x==0) {
		/*	temp = -1.0/(x-x); */
			return SVID_libm_err((double)n,x,12);
		} else {
		/*	temp = (x-x)/(x-x); */
			return SVID_libm_err((double)n,x,13);
		}
	}
	sign = 1;
	if(n<0){
		n = -n;
		if(n%2 == 1) sign = -1;
	}
	if(n==0) return(y0(x));
	if(n==1) return(sign*y1(x));
	if(!finite(x)) return 0.0;

	a = y0(x);
	b = y1(x);
	for(i=1;i<n;i++){
		temp = b;
		b = (2.*i/x)*b - a;
		a = temp;
	}
	return(sign*b);
}
