#ifdef sccsid
static	char sccsid[] = "@(#)extra.c 1.1 92/07/30 SMI"; 
#endif

#include "complex.h"

char
i_conv_c(cp,len,ip)
char *cp;
int len;
int *ip;
{
	*cp = *ip;
}


c_conv_i(cp)
char *cp;
{

	return(*cp);
}

tstz_s(s)
short s;
{
	return(s <= 0 ? (s < 0 ? 0 : 1 ) : 2);
}

tstz_i(i)
int i;
{
	return(i <= 0 ? (i < 0 ? 0 : 1 ) : 2);
}

tstz_f(f)
FLOATPARAMETER f;
{
	return(FLOATPARAMETERVALUE(f) <= 0. ? (FLOATPARAMETERVALUE(f) < 0. ? 0 : 1 ) : 2);
}

tstz_d(d)
double d;
{
	return(d <= 0. ? (d < 0. ? 0 : 1 ) : 2);
}

void
c_cmplx(resp,fp1,fp2)
register complex *resp;
register float *fp1, *fp2;
{
	resp->real = *fp1;
	resp->imag = *fp2;
}

void
d_cmplx(resp,dp1,dp2)
register dcomplex *resp;
register double *dp1,*dp2;
{
	resp->dreal = *dp1;
	resp->dimag = *dp2;
}
