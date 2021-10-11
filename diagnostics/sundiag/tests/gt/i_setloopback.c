
#ifndef lint
static  char sccsid[] = "@(#)i_setloopback.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <fe_includes.h>
#include <hdwr_regs.h>

#define REG(i)	i

/**********************************************************************/
void
i_setloopback(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */

{

	unsigned int *reg;

        reg = (unsigned *)(F_PBM_CSR);
        *reg |= 0x80;               /* set SI loopback enable */

	return;
}

