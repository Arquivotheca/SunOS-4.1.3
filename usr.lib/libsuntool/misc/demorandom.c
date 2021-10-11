#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)demorandom.c 1.1 92/07/30 SMI"; /* from Lucasfilm Ltd., 1982 */
#endif
#endif

#include <stdio.h>
/*
 * canonical mainline routine for screen hacks
 */

#define P	   98
#define Q	   27
#define Pm1	 (P-1)
static int I=Pm1, J=(Pm1+Q)%P;
static short Table[P]={
0020651,0147643,0164707,0125262,0104256,0074760,0114470,0052607,0045551,
0134031,0024107,0030766,0154073,0114777,0024540,0111012,0011042,0104067,
0056332,0142244,0131107,0034074,0052641,0163046,0026303,0131352,0077724,
0002462,0110775,0127346,0020100,0137011,0136163,0145552,0144223,0134111,
0075001,0075221,0176705,0000210,0103625,0120246,0062614,0016147,0054723,
0151200,0105223,0021001,0016224,0073377,0150716,0014557,0112613,0037466,
0002677,0052542,0063572,0105462,0106436,0063302,0053171,0133243,0113130,
0123222,0072371,0041043,0163614,0037432,0147330,0153403,0130306,0056455,
0175640,0120567,0100601,0042371,0154635,0051133,0074252,0174525,0163223,
0052022,0022564,0135512,0021760,0006743,0006451,0067445,0106210,0025417,
0066566,0062723,0124224,0144643,0164502,0025342,0003521,0024050,
};
static
demo_random(){
	if(I==Pm1)I=0;
	else I++;
	if(J==Pm1)J=0;
	else J++;
	Table[I]+=Table[J];
	return(Table[I]);
}
initrandom(r){
	register i;
	for(i=0;i!=P;i++)
		Table[i]^=r;
}
r(i, j){
	int k;
	k=demo_random()%(j-i+1);
	if(k<0)
		return(k+j+1);
	return(k+i);
}
sqroot(a)
register a;
{
	register x, y;
	if(a<=0)return(0);
	for(y=a,x=1;y!=0;y>>=2,x<<=1);
	while((y=(a/x+x)>>1)<x)x=y;
	return(x);
}
/*
 * Sun Circles by Jeffrey Mogul
 *	stolen from: Hackmem Minksy?
 *	draw a circle of radius "radius" centered at cx, cy
 *	pv is either 1 or 0 for white or black
 *	r should be something like 4
 */

/*
clearscreen(){
	drasterop(GXclear, 0, 0, NULL, 1024, 1024);
}
setscreen(){
	drasterop(GXset, 0, 0, NULL, 1024, 1024);
}
circ(radius, r, cx, cy, pv)
register int r, cx, cy;
{
	register origin(cx, cy);
	register int x,y;
	register int i;
	register int ilast;
	register xmax=screen.w/2;
	register ymax=screen.h/2;
	lockscreen();
	width(1);
	setmode(pv);
	x=radius;
	y=0;
	ilast=radius<<(r-3);
	for(i=0;i<ilast;i++){
		if(-xmax<=x && x<xmax && -ymax<=y && y<ymax)
			point(x, y);
		x+=y>>r;
		y-=x>>r;
	}
	unlockscreen();
}
combox(op, x, y, xs, ys){
	if(x<0){
		xs+=x;
		x=0;
	}
	if(y<0){
		ys+=y;
		y=0;
	}
	if(x<WIDTH && y<HEIGHT){
		if(x+xs>=WIDTH)
			xs=WIDTH-1-x;
		if(y+ys>=HEIGHT)
			ys=HEIGHT-y;
		if(xs>0 && ys>0)
			drasterop(op, x, y, NULL, xs, ys);
	}
}
*/
