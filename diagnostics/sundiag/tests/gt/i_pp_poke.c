
#ifndef lint
static  char sccsid[] = "@(#)i_pp_poke.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <fe_includes.h>
#include <hdwr_regs.h>

/* Debug mode */
/*
#define DEBUG		1
*/

/* This code attemps to determine the revision of the PP. Older
 * PP had problems with fastclear. This problem has been corrected
 * in new PP. The arbitration test uses fast clear whenever possible.
 * If a new PP is reconized the RISC register 3 is left untouched
 * with the fastclear set set by the host, otherwise reg 3 is set
 * to -1 for non-fastclear arbitration test.
 */
/**********************************************************************/
void
i_pp_poke(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */

{
    unsigned int *pp_csr = (unsigned int *)PP_CSR;

    if (!(*pp_csr & 0x10000)) {
	ctxp->risc_regs[3] = -1;
    }
    return;

}
