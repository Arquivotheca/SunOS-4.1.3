
#ifndef lint
static  char sccsid[] = "@(#)gpsi_std.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


extern char	*gpsi_3D_lines(),
		*gpsi_3D_poligons(),
		*gpsi_xf(),
		*gpsi_clip_hid(),
		*gpsi_depth_cueing(),
		*gpsi_shad(),
		*gpsi_animation();


/**********************************************************************/
void
gpsi_std()
/**********************************************************************/
/* test all graphical aspects of the GP with the GPSI */

{
    char * errmsg;

    /* Vector generations test */
    tmessage("Vectors generation: ");
    errmsg = gpsi_3D_lines();
    if (errmsg) {
	error_report(errmsg);
    } else {
	pmessage("ok.\n");
    }

#ifdef NOT_TESTING
    /* Poligons generations test */
    tmessage("Poligons generation: ");
    errmsg = gpsi_3D_poligons();
    if (errmsg) {
	error_report(errmsg);
    } else {
	pmessage("ok.\n");
    }

    /* Transformations test */
    tmessage("Transformations test: ");
    errmsg = gpsi_xf();
    if (errmsg) {
	error_report(errmsg);
    } else {
	pmessage("ok.\n");
    }


    /* Clipping & Surface removal test */
    tmessage("Clipping, Hidden Surf. Removal: ");
    errmsg = gpsi_clip_hid();
    if (errmsg) {
	error_report(errmsg);
    } else {
	pmessage("ok.\n");
    }

#endif NOT_TESTING

    /* Depth-cueing test */
    tmessage("Depth-cueing: ");
    errmsg = gpsi_depth_cueing();
    if (errmsg) {
	error_report(errmsg);
    } else {
	pmessage("ok.\n");
    }

    /* Lighting and shading test */
    tmessage("Lighting and shading: ");
    errmsg = gpsi_shad();
    if (errmsg) {
	error_report(errmsg);
    } else {
	pmessage("ok.\n");
    }

    /* Animation  and visual test */
    tmessage("Visualtest (please check screen display): ");
    errmsg = gpsi_animation();
    if (errmsg) {
	error_report(errmsg);
    } else {
	pmessage("ok.\n");
    }
}

