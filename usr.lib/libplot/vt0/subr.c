#ifndef lint
static	char sccsid[] = "@(#)subr.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/27/83 */
#endif

extern float obotx;
extern float oboty;
extern float boty;
extern float botx;
extern float scalex;
extern float scaley;
xsc(xi){
	int xa;
	xa = (xi-obotx)*scalex+botx;
	return(xa);
}
ysc(yi){
	int ya;
	ya = (yi-oboty)*scaley+boty;
	return(ya);
}
