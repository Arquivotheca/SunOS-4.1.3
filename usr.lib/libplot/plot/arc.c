#ifndef lint
static	char sccsid[] = "@(#)arc.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include <stdio.h>
arc(xi,yi,x0,y0,x1,y1){
	putc('a',stdout);
	putsi(xi);
	putsi(yi);
	putsi(x0);
	putsi(y0);
	putsi(x1);
	putsi(y1);
}
