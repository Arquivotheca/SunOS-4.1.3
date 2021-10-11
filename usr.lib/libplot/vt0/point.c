#ifndef lint
static	char sccsid[] = "@(#)point.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/27/83 */
#endif

extern vti;
extern xnow,ynow;
point(xi,yi){
	struct {char pad,c; int x,y;} p;
	p.c = 2;
	p.x = xnow = xsc(xi);
	p.y = ynow =  ysc(yi);
	write(vti,&p.c,5);
}
