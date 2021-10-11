
#ifndef lint
static char sccsid[] = "@(#)x_pw_circle.c 1.1 92/07/30 Copyright 1988, 1990 Sun Micro";
#endif
 
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <xview/xview.h>

pw_circle(pw, cx, cy, r, color)
	Pixwin *pw;
	int cx, cy, r, color;
{
	int x, y, s;
	struct rect rect;
	
	x = 0;
	y = r;
	s = 3 - (2 * y);

	rect.r_left = cx - r;
	rect.r_top = cy - r;
	rect.r_width = rect.r_height = 2 * r;

	while(y > x) {
		put(pw, cx, cy, x, y, color);
		if(s < 0) s += 4 * (x++) + 6;
		else s += 4 *((x++) - (y--)) + 10;
	}
	if (x == y) put(pw, cx, cy, x, y, color);
}

put(pw, cx, cy, x, y, color)
	Pixwin *pw;
	int cx, cy, x, y, color;
{
	int i, j, k, tmp;

	for(i = 0; i < 2; i++) {
		for(j = 0; j < 2; j++) {
			for(k = 0; k < 2; k++) {
				pw_put(pw, x + cx, y + cy, color);
				x = (-x);
			}
			y = (-y);
		}
		tmp = x;
		x = y;
		y = tmp;
	}
}

