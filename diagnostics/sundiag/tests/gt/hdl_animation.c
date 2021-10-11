
#ifndef lint
static  char sccsid[] = "@(#)hdl_animation.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/wait.h>
#include <pixrect/pixrect_hs.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <gttest.h>
#include <errmsg.h>

extern
char *getfname();

extern
Pixrect		*open_pr_device();

static
int pid;

/**********************************************************************/
char *
hdl_arbitration()
/**********************************************************************/

{

    char *xecute_in_bg();
    char *pr_arbitration_test();

    static char errtxt[256];

    char *errmsg;
    int exclude_pg[4];
    int pg_count;
    int count;
    int err=0;


    func_name = "hdl_arbitration";
    TRACE_IN

    exclude_pg[0] = PIXPG_24BIT_COLOR;
    pg_count = 1;

    (void)clear_all();

    /* draw double-buffered, test WID and CURSOR planes */
    errmsg = xecute_in_bg(ARBITRATION_DB_CHK);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    errmsg = pr_arbitration_test(exclude_pg, pg_count);

    /* stop back ground process */
    (void)stop_dl();

    if (errmsg) {
	(void)strcpy(errtxt, DLXERR_ACCELERATOR_PORT_DRAWING_TO_DB);
	(void)strcat(errtxt, errmsg);
	(void)error_report(errtxt);
	err = 1;
    }
    
    (void)clear_all();

    if (err) {
	TRACE_OUT
	return(DLXERR_ARBITRATION_FAILED);
    } else {
	TRACE_OUT
	return(char *)0;
    }

}

/**********************************************************************/
char *
xecute_in_bg(testname)
/**********************************************************************/
char *testname;

{
    extern char *exec_dl_nowait();
    extern int fastclear_set;
    char *errmsg;
    int fcs_save;


    func_name = "xecute_in_bg";
    TRACE_IN

    (void)xtract_hdl(testname);

    
    fcs_save = fastclear_set;
    fastclear_set = 0;
    
    (void)clear_all();
    errmsg = exec_dl_nowait(getfname(testname, HDL_CHK));
    
    fastclear_set = fcs_save;
    

    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    } else {
	sleep(3);
    }

    TRACE_OUT
    return (char *)0;

}

/**********************************************************************/
char *
pr_arbitration_test(exclude_pg, pg_count)
/**********************************************************************/
int *exclude_pg;
int pg_count;

{

    extern char *pixrect_complete_test();
    static char errtxt[256];

    char *errmsg;
    int i;
    Pixrect *pr;


    func_name = "pr_arbitration_test";
    TRACE_IN

    pr = open_pr_device();
    if (pr == NULL) {
	(void)sprintf(errtxt, DLXERR_OPEN, device_name);
	TRACE_OUT
	return(errtxt);
    }

    (void)get_available_plane_groups(pr);
    for (i = 0 ; i < pg_count ; i++) {
	(void)exclude_plane_group(exclude_pg[i]);
    }

    errmsg = pixrect_complete_test(pr);
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    TRACE_OUT
    return (char *)0;

}
