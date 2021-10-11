#ifndef lint
static  char sccsid[] = "@(#)i_i860_proc_test.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/***********************************************************************
	This is only a place holder for more sophisticated one which
	will be written later by the exec diagnostics group.
***********************************************************************/

#include <fe_includes.h>

#define REG(i)	i


/**********************************************************************/
void
proc_test(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */

{

/* 
    register unsigned int int_oprnd1;
    register unsigned int int_oprnd2;
    register unsigned int int_result;
    register unsigned int int_pattern;
    register float f_oprnd1;
    register float f_oprnd2;
    register float f_result;
    register float f_pattern;

    int_oprnd1 = 0xAAAAAAAA;
    int_oprnd2 = 0x55555555;
    int_pattern = 0xFFFFFFFF;
    int_result = int_oprnd1 + int_oprnd2;
    if (int_result != int_pattern) {
	umcbp->errorcode = -1;
	return;
    }

    f_oprnd1 = 0.12345;
    f_oprnd2 = 0.1;
    f_pattern = 0.012345;
    f_result = f_oprnd1 * f_oprnd2;
    if (f_result != f_pattern) {
	umcbp->errorcode = -1;
	return;
    }
*/

    umcbp->errorcode = 0;
    return;

}

