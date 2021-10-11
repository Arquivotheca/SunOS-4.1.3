
#ifndef lint
static	char sccsid[] = "@(#)id_fp_class_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

enum fp_class_type id_fp_class_(x)
double *x;
{
	return fp_class(*x);
}
