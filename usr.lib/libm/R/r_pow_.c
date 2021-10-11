
#ifndef lint
static	char sccsid[] = "@(#)r_pow_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <math.h>

static double
ln2     =   6.93147180559945286227e-01,   /* 0x3fe62e42, 0xfefa39ef */
invln2  =   1.44269504088896338700e+00,   /* 0x3ff71547, 0x652b82fe */
dtwo	=   2.0,
done 	=   1.0,
dhalf	=   0.5,
dhuge	=   1e100,
dtiny	=   1e-100,
d32	=   32.0,
d1_32	=   0.03125,
A0 	=   1.999999999813723303647511146995966439250e+0000,
A1 	=   6.666910817935858533770138657139665608610e-0001,
t0      =   2.000000000004777489262405315073203746943e+0000,
t1      =   1.666663408349926379873111932994250726307e-0001;

static double S[] = {
 1.00000000000000000000e+00     , /* 3FF0000000000000 */
 1.02189714865411662714e+00     , /* 3FF059B0D3158574 */
 1.04427378242741375480e+00     , /* 3FF0B5586CF9890F */
 1.06714040067682369717e+00     , /* 3FF11301D0125B51 */
 1.09050773266525768967e+00     , /* 3FF172B83C7D517B */
 1.11438674259589243221e+00     , /* 3FF1D4873168B9AA */
 1.13878863475669156458e+00     , /* 3FF2387A6E756238 */
 1.16372485877757747552e+00     , /* 3FF29E9DF51FDEE1 */
 1.18920711500272102690e+00     , /* 3FF306FE0A31B715 */
 1.21524735998046895524e+00     , /* 3FF371A7373AA9CB */
 1.24185781207348400201e+00     , /* 3FF3DEA64C123422 */
 1.26905095719173321989e+00     , /* 3FF44E086061892D */
 1.29683955465100964055e+00     , /* 3FF4BFDAD5362A27 */
 1.32523664315974132322e+00     , /* 3FF5342B569D4F82 */
 1.35425554693689265129e+00     , /* 3FF5AB07DD485429 */
 1.38390988196383202258e+00     , /* 3FF6247EB03A5585 */
 1.41421356237309514547e+00     , /* 3FF6A09E667F3BCD */
 1.44518080697704665027e+00     , /* 3FF71F75E8EC5F74 */
 1.47682614593949934623e+00     , /* 3FF7A11473EB0187 */
 1.50916442759342284141e+00     , /* 3FF82589994CCE13 */
 1.54221082540794074411e+00     , /* 3FF8ACE5422AA0DB */
 1.57598084510788649659e+00     , /* 3FF93737B0CDC5E5 */
 1.61049033194925428347e+00     , /* 3FF9C49182A3F090 */
 1.64575547815396494578e+00     , /* 3FFA5503B23E255D */
 1.68179283050742900407e+00     , /* 3FFAE89F995AD3AD */
 1.71861929812247793414e+00     , /* 3FFB7F76F2FB5E47 */
 1.75625216037329945351e+00     , /* 3FFC199BDD85529C */
 1.79470907500310716820e+00     , /* 3FFCB720DCEF9069 */
 1.83400808640934243066e+00     , /* 3FFD5818DCFBA487 */
 1.87416763411029996256e+00     , /* 3FFDFC97337B9B5F */
 1.91520656139714740007e+00     , /* 3FFEA4AFA2A490DA */
 1.95714412417540017941e+00     , /* 3FFF50765B6E4540 */
};
  

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

static float
one  = 1.0,
zero = 0.0;

FLOATFUNCTIONTYPE r_pow_(x,y)
float *x, *y;
{
	float fx= *x,fy= *y;
	float  fw,fz,ft;
	long inf=0x7f800000;
	int ix,iy,jx,jy,k,iw;

	ix = *(int*)x;
	iy = *(int*)y;
	jx = ix&0x7fffffff;
	jy = iy&0x7fffffff;

	if(jy==0) RETURNFLOAT(one);		/* x**0 = 1 */
	if(((inf-jx)|(inf-jy))<0) {
		fz = fx + fy;			/* +-NaN return x+y */
		RETURNFLOAT(fz);
	}

    /* determine if y is an odd int */
	if(ix<0) {	
	    if(jy>=0x4b800000) {	
		k  = 0; 	/* |y|>=2**24: y must be even */
		iw = iy;
	    } else {
		k  = (int) fy;
		fw = (float)k;
		iw = *(int*)&fw;
	    }		/* y is an odd int iff k is odd and iw==iy */
	} 

    /* special value of y */
	if((jy&(~inf))==0) { 	
	    if (jy==inf) {			/* y is +-inf */
	        if(jx==0x3f800000) 		
		    fz = fy - fy;		/* inf**+-1 is NaN */
	        else if (jx >0x3f800000)	/* (|x|>1)**+,-inf = inf,0 */
		    { if(iy>0) fz=    fy; else fz = zero;}
	        else				/* (|x|<1)**-,+inf = inf,0 */
		    { if(iy<0) fz= -(fy); else fz = zero;}
		RETURNFLOAT(fz);
	    } 
	    if(jy==0x3f800000) {		/* y is  +-1 */
		if(iy<0) fx = one/fx; 		/* y is -1 */
		RETURNFLOAT(fx);
	    }
	    if(iy==0x40000000) {
		fz = fx*fx;	 		/* y is  2 */
		RETURNFLOAT(fz);
	    }
	    if(iy==0x3f000000) {
		if(jx!=0&&jx!=inf)
		return r_sqrt_(x);		/* y is  0.5 */
	    }
	}

    /* special value of x */
	if((jx&(~inf))==0) { 	
	    if(jx==inf||jx==0||jx==0x3f800000) {/* x is +-0,+-inf,+-1 */
		*(int*)&fz = jx;
		if(iy<0) fz = one/fz;		/* fz = |x|**y */
		if(ix<0) {
		    if(jx==0x3f800000&&iw!=iy) 
			fz = zero/zero;		/* (-1)**non-int is NaN */
		    else if(iw==iy&&(k&1)!=0) 
			fz = -fz;		/* (x<0)**odd = -(|x|**odd) */
		}
		RETURNFLOAT(fz);
	    }
	}

    
    /* (x<0)**(non-int) is NaN */
	if(ix<0&&iw!=iy) {			
	    fz = zero/zero;
	    RETURNFLOAT(fz);
	}


    /* compute exp(y*log(|x|)) 
	fx = *(float*)&jx;
	fz = (float) exp(((double)fy)*log((double)fx)) ; 
     */
	{ 
	  double dx,dy,dz,ds; int *px= (int*)&dx, *pz= (int*)&dz,i,j,n,m;
	  j = 0; if(*(int*)&done==0) j=1;
	  fx = *(float*)&jx;
	  dx = (double)fx;

	/* compute log(x)/ln2 */
	  i   = px[j]+0x4000;
	  n   = (i>>20)-0x3ff;
	  pz[j] = i&0xffff8000; pz[1-j]=0;
	  ds  = (dx-dz)/(dx+dz);
	  i   = (i>>15)&0x1f;
	  dz  = ds*ds;
	  dy  = invln2*(TBL[i]+ds*(A0+dz*A1));
	  if(n==0) dz  = (double)fy *  dy;
	  else     dz  = (double)fy * (dy+(double)n);

	/* compute exp2(dz=y*ln(x)) */
	  i   = pz[j];
	  if ( (i&0x7fffffff)>=0x40640000 ) {	/* |z| >= 160.0 */
	      if(i>0) fz = (float)dhuge;	/* overflow */
	      else    fz = (float)dtiny;	/* underflow */
	      if(ix<0&&(k&1)==1) fz = -fz;	/* (-ve)**(odd int) */
	      RETURNFLOAT(fz);
	  }
	  n   = d32*dz+((i>0)?dhalf: -dhalf);
	  i   = n&0x1f; m = n>>5;
	  dy  = ln2*(dz - d1_32*(double)n);
	  dx  = S[i]*(done-(dtwo*dy)/(dy*(done-dy*t1)-t0));
	  if(m==0) 
	      fz = (float)dx;
	  else {
	      px[j] += (m<<20);
	      fz = (float)dx;
	  }
	}

    /* end of computing exp(y*log(x)) */

	if(ix<0&&(k&1)==1) fz = -fz;		/* (-ve)**(odd int) */

	RETURNFLOAT(fz);
}
