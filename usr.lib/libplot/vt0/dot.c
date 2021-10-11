#ifndef lint
static	char sccsid[] = "@(#)dot.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/27/83 */
#endif

extern vti;
dot(xi,yi,dx,n,pat)
int pat[];
{
	struct {char pad,c; int xi,yi,dx;} p;
	p.c = 7;
	p.xi = xsc(xi);
	p.yi = ysc(yi);
	p.dx = xsc(dx);
	write(vti,&p.c,7);
	write(vti,pat,n?n&0377:256);
}
