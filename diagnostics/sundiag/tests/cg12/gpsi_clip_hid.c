
#ifndef lint
static  char sccsid[] = "@(#)gpsi_clip_hid.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <suntool/sunview.h>
#include <pixrect/gp1cmds.h>
#include <esd.h>

Matrix_3D para = {
     1.0, 0.0, 0.0, 0.0,
     0.0, 1.0, 0.0, 0.0,
     0.0, 0.0, 1.0, 0.0,
     0.0, 0.0, 0.0, 1.0,
};

/**********************************************************************/
char *
gpsi_clip_hid()
/**********************************************************************/
 
{
    char * enterprise();
    char *vector_lissa();
    char *picktest();
    char *disks();

    Rect clprects[2];
    char *errmsg;
    double ri;
    float xscale;
    float xoff;
    float yscale;
    float yoff;
    float zscale;
    float zoff;

    func_name = "gpsi_clip_hid";
    TRACE_IN

    /* extract test images from tar file */
    (void)xtract(GPSI_CLIP_CHK);
    (void)xtract(GPSI_HID_CHK);

    /* clear all planes */
    clear_all();

    /* set clipping context */
    clprects[0].r_left = 128;
    clprects[0].r_top = 128;
    clprects[0].r_width = 256;
    clprects[0].r_height = 256;

    clprects[1].r_left = 600;
    clprects[1].r_top = 400;
    clprects[1].r_width = 311;
    clprects[1].r_height = 321;

    errmsg = ctx_set(CTX_SET_CLIP_LIST, 2, &clprects[0], 0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    ri = 14.5;
    errmsg = vector_lissa(ri);
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    errmsg = enterprise();
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    errmsg = chksum_verify(GPSI_CLIP_CHK);
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    /* clear all planes */
    clear_all();

    errmsg = disks();
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    errmsg = chksum_verify(GPSI_HID_CHK);
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xscale = 144.0; /*x scale factor from NDC to screen*/
    xoff = 576.0;   /*center in screen*/

    yscale = 112.0; /*y scale factor from NDC to screen*/
    yoff = 450.0;

    zscale = (float) 0xffff; /*use full z-buffer*/

    zoff = 0.0;
    errmsg = ctx_set(CTX_SET_HIDDEN_SURF, GP1_ZBALL,
			CTX_SET_VWP_3D, xscale, xoff,
					yscale, yoff,
					zscale, zoff,
			CTX_SET_MAT_NUM, 1,
			CTX_SET_MAT_3D, 1, &para,
			0);
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    errmsg = picktest();
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    TRACE_OUT
    return errmsg;

}
