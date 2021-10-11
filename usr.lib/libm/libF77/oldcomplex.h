/*	@(#)oldcomplex.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

typedef struct { float real, imag; } complex;
typedef struct { double dreal, dimag; } dcomplex;

/*	So far the following works for all architectures.	*/

#define COMPLEXFUNCTIONTYPE	double
#define RETURNCOMPLEX(R,M)	{ union { double _d; float _f[2]} _kluge ; _kluge._f[0] = (R) ; _kluge._f[1] = (M) ; return(_kluge._d) ; }
