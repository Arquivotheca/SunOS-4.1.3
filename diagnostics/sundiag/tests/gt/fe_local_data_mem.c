
#ifndef lint
static  char sccsid[] = "%Z%%M% %I% %E% Copyr 1990 Sun Micro";
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
fe_local_data_mem()
/**********************************************************************/

{
    char *getfname();
    char *errmsg;


    func_name = "fe_local_data_mem";
    TRACE_IN

    xtract_hdl(FE_LOCAL_DATA_MEM);

    errmsg = exec_dl(getfname(FE_LOCAL_DATA_MEM, HDL_CHK));
    if (errmsg) {
	sprintf(errtxt, DLXERR_LDM_FAILED, errmsg);
	TRACE_OUT
	return errtxt;
    } else { 

	TRACE_OUT
	return (char *)0;
    }
}

