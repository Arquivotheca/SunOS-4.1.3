/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)matinv.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *
 *	_core_matinv - Matrix inversion with solution of linear equations
 * 
 * Operation:
 * 	Performs a matrix inversion on up to a 16 by 16 matrix solving
 * 	the associated set of linear equations and returning the value
 * 	of the determinant.
 * Usage:
 * 	float a(nt,nt)		! matrix to invert nt by nt square
 * 	int   nt		! size of matrix (2..16)
 * 	double b(nt,m)		! m sets of nt input values to be solved
 * 				! returned solutions to m equations.
 * 	int   m			! number of equations to solve. (0..4)
 * 	double determ		! returned value of determinant
 *   
 * 	_core_matinv( a,nt,b,m,&determ);
 */

#include <math.h>
#define elem(a,i,j) *(a+(a/**/x * (i) + (j))

_core_matinv(a, n, b, m, determ)
    float a[4][4];
int n, m;
double b[4][2], *determ;

{
    short index[16][2], ipivot[16];
    float pivot[16];
    short row, colum;
    float max;
    short i, j, k, l;

    *determ = 1.0;		/* Initialization */
    for (j = 0; j < n; j++)
	ipivot[j] = 0;

    for (i = 0; i < n; i++) {	/* do matrix inversion */
	max = 0.0;
	for (j = 0; j < n; j++) {	/* search for pivot element */
	    if (ipivot[j] == 1)
		continue;
	    for (k = 0; k < n; k++) {
		if (ipivot[k] == 1)
		    continue;
		if (ipivot[k] > 1)
		    return (0);
		if (fabs(max) < fabs(a[j][k])) {
		    row = j;
		    colum = k;
		    max = a[j][k];
		}
	    }
	}
	if (max == 0.0) {
	    *determ = 0.0;
	    return (0);
	}
	ipivot[colum] += 1;
	if (row != colum) {	/* interchange rows to put */
	    *determ = -*determ;	/* pivot element on diagonal */
	    for (l = 0; l < n; l++) {
		max = a[row][l];
		a[row][l] = a[colum][l];
		a[colum][l] = max;
	    }
	    if (m > 0)
		for (l = 0; l < m; l++) {
		    max = b[row][l];
		    b[row][l] = b[colum][l];
		    b[colum][l] = max;
		}
	}
	index[i][0] = row;
	index[i][1] = colum;
	pivot[i] = a[colum][colum];
	*determ *= pivot[i];
	a[colum][colum] = 1.0;	/* divide pivot row by pivot element */
	for (l = 0; l < n; l++)
	    a[colum][l] /= pivot[i];
	if (m > 0)
	    for (l = 0; l < m; l++)
		b[colum][l] /= pivot[i];
	for (j = 0; j < n; j++)
	    if (j != colum) {
		max = a[j][colum];
		a[j][colum] = 0.0;
		for (l = 0; l < n; l++)
		    a[j][l] -= a[colum][l] * max;
		if (m > 0)
		    for (l = 0; l < n; l++)
			b[j][l] -= b[colum][l] * max;
	    }
    }

    for (i = 0; i < n; i++) {	/* interchange columns */
	l = n - 1 - i;
	if (index[l][0] != index[l][1]) {
	    row = index[l][0];
	    colum = index[l][1];
	    for (k = 0; k < n; k++) {
		max = a[k][row];
		a[k][row] = a[k][colum];
		a[k][colum] = max;
	    }
	}
    }
    return (0);
}
