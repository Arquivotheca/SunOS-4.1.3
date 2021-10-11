
#ifndef lint
static	char sccsid[] = "@(#)r_trigpi_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* r_sinpi_(x), r_cospi_(x), r_tanpi_(x), r_sincospi_(x,s,c)
 * return single precision trig(pi*x).
 */

#include <math.h>

static float 	
pi	= 3.14159265358979323846,
four	= 4.0,
two	= 2.0,
oneh	= 1.5,
one	= 1.0,
half	= 0.5,
quad	= 0.25,
tiny	= 1e-10;

FLOATFUNCTIONTYPE r_sinpi_(x)
float *x;
{
	FLOATFUNCTIONTYPE w;
	float y,t;
	long ix;
	if(!ir_finite_(x))  {t = *x - *x; RETURNFLOAT(t);}
	ix   = *((long *)x);
	*((long *)&t) = ix&0x7fffffff;		/* t = |*x| */
	if(t<quad) { t = pi*(*x); return r_sin_(&t);}
	t *= half;
	w  = r_aint_(&t);
	y  = two*(t - *((float *)&w)) ;		/* y = x mod 2.0 */
	switch ((int)(y*four)) {
	    case 0:   t = pi*y;  	w =  r_sin_(&t); break;
	    case 1: 
	    case 2:   t = pi*(half-y); 	w =  r_cos_(&t); break;
	    case 3:
	    case 4:   t = pi*(one-y); 	w =  r_sin_(&t); break;
	    case 5:
	    case 6:   t = pi*(y-oneh); 	w =  r_cos_(&t); ix = -ix; break;
	    default:  t = pi*(y-two); 	w =  r_sin_(&t); break;
	    }
	if(ix<0) *(int*)&w ^= 0x80000000;	/* w = -w */
	return w;
}

FLOATFUNCTIONTYPE r_cospi_(x)
float *x;
{
	FLOATFUNCTIONTYPE w;
	int ix;
	float y,t;
	if(!ir_finite_(x))  {t = *x - *x; RETURNFLOAT(t);}
	*((long *)&t) = (*((long *)x))&0x7fffffff;	/* t = |*x| */
	if(t<quad) {if(t>tiny) t *= pi; return r_cos_(&t);}
	t *= half;
	w  = r_aint_(&t);
	y  = two*(t - *((float *)&w)) ;		/* y = x mod 2.0 */
	ix = 0;
	switch ((int)(y*four)) {
	    case 0:   t = pi*y;  	w =  r_cos_(&t); break;
	    case 1:
	    case 2:   t = pi*(half-y); 	w =  r_sin_(&t); break;
	    case 3:
	    case 4:   t = pi*(one-y); 	w =  r_cos_(&t); ix = -1; break;
	    case 5:   t = pi*(y-oneh); 	w =  r_sin_(&t); break;
	    case 6:   t = pi*(oneh-y); 	w =  r_sin_(&t); ix = -1; break;
	    default:  t = pi*(y-two); 	w =  r_cos_(&t); 
	    }
	if(ix<0) *(int*)&w ^= 0x80000000;	/* w = -w */
	return w;
}

FLOATFUNCTIONTYPE r_tanpi_(x)
float *x;
{
	float s,c;
	void r_sincospi_();
	r_sincospi_(x,&s,&c);
	s = s/c; 
	RETURNFLOAT(s);
}

void r_sincospi_(x,s,c)
float *x,*s,*c;
{
	FLOATFUNCTIONTYPE w;
	float y,t;
	long ix;
	if(!ir_finite_(x))  *s = *c = *x - *x;
	else {
	    ix = *((long *)x);
	    *((long *)&t) = ix&0x7fffffff;	/* t = |*x| */
	    if(t<quad) { t = (*x)*pi; r_sincos_(&t,s,c);}
	    else {
		t *= half;
		w  = r_aint_(&t);
		y  = two*(t - *((float *)&w)) ;	/* y = x mod 2.0 */
		switch ((int)(y*four)) {
		    case 0:   t = pi*y;  	r_sincos_(&t,s,c); break;
		    case 1:
		    case 2:   t = pi*(half-y); 	r_sincos_(&t,c,s); break;
		    case 3:
		    case 4:   t = pi*(one-y); 	r_sincos_(&t,s,c); 
					*c = -(*c); break;
		    case 5:   t = pi*(y-oneh); 	r_sincos_(&t,c,s); 
					*s = -(*s); break;
		    case 6:   t = pi*(oneh-y); 	r_sincos_(&t,c,s); 
					*c = -(*c); *s = -(*s); break;
		    default:  t = pi*(y-two); 	r_sincos_(&t,s,c); break;
		}
		if(ix<0) *s = -(*s);
	    }
	}
}
