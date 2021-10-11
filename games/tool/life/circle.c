#ifndef lint
static  char sccsid[] = "@(#)circle.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems Inc.
 */

#include <math.h>
#include <stdio.h>

 /* 
  *  Circle:	usage: circle length thickness
  *
  *  Computes circle on length by length square grid.  Circle has
  *  thickness pixels.
  *  Prints out picture of dots that should be colored.  Of course the
  *  picture looks very oval, because a character is much taller than it
  *  is wide.
  *
  *  The center has coordinates (0,0).  The upper
  *  left hand dot has coordinates (-(length-1)/2, (length-1)/2).
  *  The circle has radius length/2.
  */

#define MAXLENGTH 64		/* length must be less than this */
#define CHAR 1			/* char used to print out picture */

char circlearr[MAXLENGTH][MAXLENGTH];

double	sqroot();

drawcircle (diameter, white, thickness)
{
	float a, b, x, y, radius, corner, center;
	int i, j;

	radius = diameter/2.0;
	corner = (diameter-1)/2.0;
	center = (diameter & 01) ? 0 : 0.5;

	if (diameter == 1) {
		circlearr[0][0] = 1;
		return;
	}
	if (diameter == 2) {
		circlearr[0][0] = 1;
		circlearr[1][0] = 1;
		circlearr[0][1] = 1;
		circlearr[1][1] = 1;
		return;
	}
	if (thickness > corner) {
		fprintf (stderr, "thickness must be less than %g\n", corner);
		exit(1);
	}
	for (x = -corner, i = 0; x <= -center; x++, i++) {
		a = sqroot(radius * radius - x * x);
		b = sqroot((radius-thickness) * (radius-thickness) - x * x);
		if (!white)
			b = 0;
		for (y = corner, j = (diameter-1); y >= center; y--, j--)
			if (a >= y && y >= b) {
				circlearr[i][j] = CHAR;
				circlearr[diameter - 1 - i][j] = CHAR;
				circlearr[i][diameter - 1 - j] = CHAR;
				circlearr[diameter-1-i][diameter-1-j] = CHAR;
			}
			else {
				circlearr[i][j] = 0;
				circlearr[diameter - 1 - i][j] = 0;
				circlearr[i][diameter - 1 - j] = 0;
				circlearr[diameter-1-i][diameter-1-j] = 0;
			}
	}
}

double
sqroot(z)
	double	z;
{
	if (z <= 0)
		return ((double)0);
	else
		return (sqrt(z));
}
