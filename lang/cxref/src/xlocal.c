#ifndef lint
static	char sccsid[] = "@(#)xlocal.c 1.1 92/07/30 SMI"; /* from S5R2 1.5 */
#endif

#include <stdio.h>
#include "cpass1.h"
#ifndef FLEXNAMES
#	define LCHNM	8
#	define LFNM	15
#endif
#include "lmanifest.h"
/*
 * this file contains the functions local to CXREF
 * they put their output to outfp
 * the others contain lint's 1st pass with output thrown away
 * cgram.c has calls to these functions whenever a NAME is seen
 */

char infile[120];
FILE *outfp;

ref( i, line)
	int i, line;
{
#ifdef FLEXNAMES
	fprintf(outfp, "R%s\t%05d\n", STP(i)->sname, line);
#else
	fprintf(outfp, "R%.8s\t%05d\n",STP(i)->sname,line);
#endif
}

def( i, line)
	int i, line;
{
	if (STP(i)->sclass == EXTERN)
		ref(i, line);
	else
#ifdef FLEXNAMES
		fprintf(outfp, "D%s\t%05d\n", STP(i)->sname, line);
#else
		fprintf(outfp,"D%.8s\t%05d\n",STP(i)->sname,line);
#endif
}


newf(i, line)
	int i, line;
{
#ifdef FLEXNAMES
	fprintf(outfp, "F%s\t%05d\n", STP(i)->sname, line);
#else
	fprintf(outfp,"F%.8s\t%05d\n",STP(i)->sname, line);
#endif
}
