/*	@(#)testsubsin.c 1.1 92/07/30 SMI	*/
#include "cgidefs.h"
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
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: test_input_ask 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int test_input_ask()
	{
	    Clogical v,e,s,a;
	    int nl,nv,ns,nc,nst,nt;
	    inquire_input_capabilities(&v,&e,&s,&a,&nl,&nv,&ns,&nc,&nst,&nt);
	    printf("input capabilities %d %d %d %d %d %d %d %d %d",
	    &v,&e,&s,&a,&nl,&nv,&ns,&nc,&nst,&nt);
	    return(0);
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: test_negot 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int test_negot()
	{
	    char name[100], *funs[100];
	    int in,out,seg;
	    int x,y,x1,y1;
	    inquire_device_identification(name);
	    printf(" device name: %s \n", name);
	    inquire_device_class(&in,&out,&seg);
	    printf(" device class: %d %d %d \n", in,out,seg);
	    inquire_physical_coordinate_system(0,&x,&y,&x1,&y1);
	    printf(" device coordinate system: %d %d %d %d \n", 
		x,y,x1,y1);
	    printf( "output list (it really puts out!) \n");
	    inquire_output_capabilities (1,out,funs);
	    for (i = 1; i < out; i++) printf(" %s \n ",funs[i]);
	    return(0);
	}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: test_locator_input 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int test_input_locator()
	{
	/* test stroke */
	center.x =6400;
	center.y =6400;
	ival.xypt = &center;
	initialize_lid(IC_LOCATOR,1,&ival);
	lower.x = 32767;
	lower.y = 32767;
	bounds.upper = &lower;
	top.x =0;
	top.y =0;
	bounds.lower= &top;
 	track_on(IC_LOCATOR,1,1,&bounds,&ival); 
	printf("The envelope \n");
 	associate(2,IC_LOCATOR,1); 
	printf("postion 1 x y %d %d \n",ival.xypt->x,ival.xypt-> y);
	enable_events(IC_LOCATOR,1,err);
	await_event(10000000,&valid,&dev,&name,&ival,&mess,&rep,&tim,&qs,&qo);
	printf("postion 1 x y %d %d \n",ival.xypt->x,ival.xypt-> y);
	disable_events(IC_LOCATOR,1,err);
	initiate_request(IC_LOCATOR,1);
	printf("do it now \n");
	sleep(2);
	get_last_requested_input(IC_LOCATOR,1,&valid,&ival);
	printf("postion 2 x y %d %d \n",ival.xypt->x,ival.xypt-> y);
	request_input(IC_LOCATOR,1,10000000,&stat,&ival,&trig); 
	printf("postion 3 x y %d %d \n",ival.xypt->x,ival.xypt-> y); 
	}
