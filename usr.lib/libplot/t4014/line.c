#ifndef lint
static	char sccsid[] = "@(#)line.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/27/83 */
#endif

line(x0,y0,x1,y1){
	move(x0,y0);
	cont(x1,y1);
}
