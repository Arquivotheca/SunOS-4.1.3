#ifndef lint
static	char sccsid[] = "@(#)circle.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/27/83 */
#endif

circle(x,y,r){
	arc(x,y,x+r,y,x+r,y);
}
