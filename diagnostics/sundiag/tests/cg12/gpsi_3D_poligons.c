
#ifndef lint
static  char sccsid[] = "@(#)gpsi_3D_poligons.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <math.h>
#include <pixrect/gp1cmds.h>
#include <esd.h>


/**********************************************************************/
char *
gpsi_3D_poligons()
/**********************************************************************/
 
{
    char *pgon_lissa();
    char *errmsg;
    double ri;

    func_name = "gpsi_3D_poligons";
    TRACE_IN

    /* extract test images from tar file */
    (void)xtract(GPSI_3D_POLIGONS_CHK);

    /* clear all planes */
    clear_all();

    /* set context */
    errmsg = ctx_set(0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    ri = M_PI/5.5;
    errmsg = pgon_lissa(ri);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }


    errmsg = chksum_verify(GPSI_3D_POLIGONS_CHK);

    TRACE_OUT
    return errmsg;
}


/**********************************************************************/
char *
pgon_lissa(inc)
/**********************************************************************/
double inc;
/* draws the lissajou figure with poligon command */
{

    register double rd;
    int  nbnds = 8;
    register int total = 48;
    register short *p;
    register int k;
    int color = 1;
    int res;
    unsigned int seed;
    float aux;

    func_name = "pgon_lissa";
    TRACE_IN

    rd = inc;

    p = allocbuf();
    if (p == (short *)-1) {
	TRACE_OUT
	return (errmsg_list[52]);
    }

    *p++ = GP2_XF_PGON_FLT_3D_RGB | GP2_RGB_COLOR_PACK;
    *p++ = nbnds;
    for (k = 0 ; k < nbnds ; k++) {
	*p++ = total/nbnds;
    }
    for (k = 0 ; k < total ; k++) {
	aux = (float)cos(rd);
	GP1_PUT_F(p, aux);
	aux = (float)sin(rd);
	GP1_PUT_F(p, aux);
	aux = (float)k/(float)total;
	GP1_PUT_F(p, aux);

	color += 0xffffff/total;
	GP1_PUT_I(p, color);

	rd += inc;
    }
    res = postbuf(p);
    if (res == -1) {
	TRACE_OUT
	return (errmsg_list[12]);
    }

    TRACE_OUT
    return (char *)0;

}
