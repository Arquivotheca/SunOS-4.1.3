#ifndef lint
static	char sccsid[] = "@(#)point.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/27/83 */
#endif

point(xi,yi){
		move(xi,yi);
		label(".");
		return;
}
