
#ifndef lint
static  char sccsid[] = "@(#)fb_video_mem_i860.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <gttest.h>
#include <errmsg.h>

static char errtxt[512];

/**********************************************************************/
char *
fb_video_mem_i860()
/**********************************************************************/

{

    int lock;
    char *errmsg;

    func_name = "fb_video_mem_i860";
    TRACE_IN

    xtract_hdl(VIDEO_MEM_I860_IMAGE_A);
    errmsg = exec_dl(getfname(VIDEO_MEM_I860_IMAGE_A, HDL_CHK));

    if (errmsg) {
	sprintf(errtxt, DLXERR_VIDEO_MEM_I860_IMAGE_A, errmsg);
	TRACE_OUT;
	return errtxt;
    }

    check_key();
    (void)unlock_desktop();
    lock = lock_desktop(device_name);

    xtract_hdl(VIDEO_MEM_I860_IMAGE_B);
    errmsg = exec_dl(getfname(VIDEO_MEM_I860_IMAGE_B, HDL_CHK));

    if (errmsg) {
	sprintf(errtxt, DLXERR_VIDEO_MEM_I860_IMAGE_B, errmsg);
	TRACE_OUT;
	return errtxt;
    }

    check_key();
    /* unlock the screen to show verbose message if verbose */
    if ((lock >= 0) && verbose) {
	/* wait for SunView to redraw the screen */
	(void)unlock_desktop();
    }
    lock = lock_desktop(device_name);

    xtract_hdl(VIDEO_MEM_I860_DEPTH);
    errmsg = exec_dl(getfname(VIDEO_MEM_I860_DEPTH, HDL_CHK));

    if (errmsg) {
	sprintf(errtxt, DLXERR_VIDEO_MEM_I860_DEPTH, errmsg);
	TRACE_OUT;
	return errtxt;
    }

    check_key();
    /* unlock the screen to show verbose message if verbose */
    if ((lock >= 0) && verbose) {
	/* wait for SunView to redraw the screen */
	(void)unlock_desktop();
    }
    lock = lock_desktop(device_name);

    xtract_hdl(VIDEO_MEM_I860_WID);
    errmsg = exec_dl(getfname(VIDEO_MEM_I860_WID, HDL_CHK));

    if (errmsg) {
	sprintf(errtxt, DLXERR_VIDEO_MEM_I860_WID, errmsg);
	TRACE_OUT;
	return errtxt;
    }

    check_key();
    /* unlock the screen to show verbose message if verbose */
    if ((lock >= 0) && verbose) {
	/* wait for SunView to redraw the screen */
	(void)unlock_desktop();
    }
    lock = lock_desktop(device_name);

    xtract_hdl(VIDEO_MEM_I860_CURSOR);
    errmsg = exec_dl(getfname(VIDEO_MEM_I860_CURSOR, HDL_CHK));

    if (errmsg) {
	sprintf(errtxt, DLXERR_VIDEO_MEM_I860_CURSOR, errmsg);
	TRACE_OUT;
	return errtxt;
    }

    check_key();
    /* unlock the screen to show verbose message if verbose */
    if ((lock >= 0) && verbose) {
	/* wait for SunView to redraw the screen */
	(void)unlock_desktop();
    }
    lock = lock_desktop(device_name);

    xtract_hdl(VIDEO_MEM_I860_FCS_A);
    errmsg = exec_dl(getfname(VIDEO_MEM_I860_FCS_A, HDL_CHK));

    if (errmsg) {
	sprintf(errtxt, DLXERR_VIDEO_MEM_I860_FCS_A, errmsg);
	TRACE_OUT;
	return errtxt;
    }

    check_key();
    /* unlock the screen to show verbose message if verbose */
    if ((lock >= 0) && verbose) {
	/* wait for SunView to redraw the screen */
	(void)unlock_desktop();
    }
    lock = lock_desktop(device_name);

    xtract_hdl(VIDEO_MEM_I860_FCS_B);
    errmsg = exec_dl(getfname(VIDEO_MEM_I860_FCS_B, HDL_CHK));

    if (errmsg) {
	sprintf(errtxt, DLXERR_VIDEO_MEM_I860_FCS_B, errmsg);
	TRACE_OUT;
	return errtxt;
    }

    check_key();
    /* unlock the screen to show verbose message if verbose */
    if ((lock >= 0) && verbose) {
	/* wait for SunView to redraw the screen */
	(void)unlock_desktop();
    }
    lock = lock_desktop(device_name);

    xtract_hdl(FB_WLUT);
    errmsg = exec_dl(getfname(FB_WLUT, HDL_CHK));

    if (errmsg) {
	sprintf(errtxt, DLXERR_WLUT, errmsg);
	TRACE_OUT;
	return errtxt;
    }

    check_key();
    /* unlock the screen to show verbose message if verbose */
    if ((lock >= 0) && verbose) {
	/* wait for SunView to redraw the screen */
	(void)unlock_desktop();
    }
    lock = lock_desktop(device_name);

    xtract_hdl(FB_CLUT);
    errmsg = exec_dl(getfname(FB_CLUT, HDL_CHK));

    if (errmsg) {
	sprintf(errtxt, DLXERR_CLUT, errmsg);
	TRACE_OUT;
	return errtxt;
    }

    TRACE_OUT
    return errmsg;
}

