#ifndef lint
static char sccsid[] = "@(#)testi1.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

#include "cgidefs.h"
/*
int cgpixwindd();
int pixwindd();
*/
int test_negot();
int test_input_ask();
int test_input();
int test_text();
int test_geometry();
Cvwsurf device;
Ccoor tpos,upper,lower,center,top,left;
Ccoor list[3];
Ccoorlist points;
char *tstr;
float uxe;
short *source,xs,ys,sizex,sizey,*target,xd,yd;
int i, *err,*result,*trig;
Cint name;
Cflag rr;
Clogical *stat;
Cinrep ival;
Ccoorpair bounds;
Ctextatt tthing;
Clinatt lthing;
Cfillatt fthing;
Cmarkatt mthing;
Cpatternatt pathing;
Cint name;
Cawresult valid;
Cdevoff dev;
Cint name;
Cmesstype mess;
Cint rep,tim;
Cqtype qs;
Ceqflow qo;
main () /*Initial test of locator input*/
{
	
	device.dd = CG1DD;
	device.dd = PIXWINDD;
	open_cgi();
	open_vws(&name,&device);
	tpos.x=4000;
	tpos.y=7000;
	tstr = "INPUT READY";
	text_font_index(STICK);
	text(&tpos,tstr);
	test_input_locator();
	sleep(4);
	close_cgi();
}

