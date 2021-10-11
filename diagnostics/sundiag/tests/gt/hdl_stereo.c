
#ifndef lint
static  char sccsid[] = "@(#)hdl_stereo.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <errmsg.h>
#include <gttest.h>

/**********************************************************************/
char *
hdl_stereo()
/**********************************************************************/

{
    extern int fastclear_set;
    char *getfname();
    char *exec_dl_nowait();
    char *errmsg;
    int fcs_save;

    int forever = TRUE;



    func_name = "hdl_stereo";
    TRACE_IN

    (void)clear_all();

    (void)save_fb_regs();
    (void)set_fb_stereo();


    xtract_hdl(STEREO_CHK);

    fcs_save = fastclear_set;
    fastclear_set = 0;
    
    errmsg = exec_dl_nowait(getfname(STEREO_CHK, HDL_CHK));
    if (errmsg) {
	(void)fb_send_message(SKIP_ERROR, WARNING, errmsg);
    } else {
	sleep(3);
    }

    fastclear_set = fcs_save;

    /* loop untill interrupted by user */
    while (forever) {
	if (check_key() == (int)'q') {
	    break;
	}
    }

    (void)restore_fb();
    TRACE_OUT
    return (char *)0;
}

